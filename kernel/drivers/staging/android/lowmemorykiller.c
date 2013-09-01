/* drivers/misc/lowmemorykiller.c
 *
 * The lowmemorykiller driver lets user-space specify a set of memory thresholds
 * where processes with a range of oom_score_adj values will get killed. Specify
 * the minimum oom_score_adj values in
 * /sys/module/lowmemorykiller/parameters/adj and the number of free pages in
 * /sys/module/lowmemorykiller/parameters/minfree. Both files take a comma
 * separated list of numbers in ascending order.
 *
 * For example, write "0,8" to /sys/module/lowmemorykiller/parameters/adj and
 * "1024,4096" to /sys/module/lowmemorykiller/parameters/minfree to kill
 * processes with a oom_score_adj value of 8 or higher when the free memory
 * drops below 4096 pages and kill processes with a oom_score_adj value of 0 or
 * higher when the free memory drops below 1024 pages.
 *
 * The driver considers memory used for caches to be free, but if a large
 * percentage of the cached memory is locked this can be very inaccurate
 * and processes may not get killed until the normal oom killer is triggered.
 *
 * Copyright (C) 2007-2008 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/notifier.h>

#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
#include <linux/aee.h>
#include <linux/disp_assert_layer.h>
int lowmem_indicator = 1;
int in_lowmem = 0;
#endif

/* From page_alloc.c, for urgent allocations in preemptible situation */
extern size_t lmk_adjz_minfree;
extern void show_free_areas_minimum(void);

static uint32_t lowmem_debug_level = 2;
#ifdef CONFIG_MT_ENG_BUILD
static uint32_t lowmem_debug_adj = 0;
#endif
static DEFINE_SPINLOCK(lowmem_shrink_lock);
static int lowmem_adj[6] = {
	0,
	1,
	6,
	12,
};
static int lowmem_adj_size = 4;
static int lowmem_minfree[6] = {
	3 * 512,	/* 6MB */
	2 * 1024,	/* 8MB */
	4 * 1024,	/* 16MB */
	16 * 1024,	/* 64MB */
};
static int lowmem_minfree_size = 4;

static struct task_struct *lowmem_deathpending;
static unsigned long lowmem_deathpending_timeout;

#define lowmem_print(level, x...)			\
	do {						\
		if (lowmem_debug_level >= (level))	\
			printk(x);			\
	} while (0)

static int
task_notify_func(struct notifier_block *self, unsigned long val, void *data);

static struct notifier_block task_nb = {
	.notifier_call	= task_notify_func,
};

static int
task_notify_func(struct notifier_block *self, unsigned long val, void *data)
{
	struct task_struct *task = data;

	if (task == lowmem_deathpending)
		lowmem_deathpending = NULL;

	return NOTIFY_DONE;
}

static int lowmem_shrink(struct shrinker *s, struct shrink_control *sc)
{
	struct task_struct *tsk;
	struct task_struct *selected = NULL;
	int rem = 0;
	int tasksize;
	int i;
	int min_score_adj = OOM_SCORE_ADJ_MAX + 1;
	int selected_tasksize = 0;
	int selected_oom_score_adj;
	int array_size = ARRAY_SIZE(lowmem_adj);
	int other_free = global_page_state(NR_FREE_PAGES) - totalreserve_pages;
	int other_file = global_page_state(NR_FILE_PAGES) -
						global_page_state(NR_SHMEM);
						
    /*
	 * If we already have a death outstanding, then
	 * bail out right away; indicating to vmscan
	 * that we have nothing further to offer on
	 * this pass.
	 *
	 */
	if (lowmem_deathpending &&
	    time_before_eq(jiffies, lowmem_deathpending_timeout))
		return 0;
		
    if (!spin_trylock(&lowmem_shrink_lock)){
	    lowmem_print(4, "lowmem_shrink lock faild\n");
	    return -1;
	}

	/* For JB: FOREGROUND is adj0 (Default lowmem_adj of AMS is 0, 1, 2, 4, 9, 15) */
	/* For ICS: HOME is adj6 (Default lowmem_adj of AMS is 0, 1, 2, 4, 9, 15) */
	if (other_free <= lowmem_minfree[1]) {
		/* Notify Kernel that we should consider Android threshold */
		lmk_adjz_minfree = lowmem_minfree[0];
	} else {
		lmk_adjz_minfree = 0;
	}

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;
	if (lowmem_minfree_size < array_size)
		array_size = lowmem_minfree_size;

	for (i = 0; i < array_size; i++) {
		if (other_free < lowmem_minfree[i] &&
		    other_file < lowmem_minfree[i]) {
			min_score_adj = lowmem_adj[i];
			break;
		}
	}
	if (sc->nr_to_scan > 0)
		lowmem_print(3, "lowmem_shrink %lu, %x, ofree %d %d, ma %d\n",
				sc->nr_to_scan, sc->gfp_mask, other_free,
				other_file, min_score_adj);
	rem = global_page_state(NR_ACTIVE_ANON) +
		global_page_state(NR_ACTIVE_FILE) +
		global_page_state(NR_INACTIVE_ANON) +
		global_page_state(NR_INACTIVE_FILE);
	if (sc->nr_to_scan <= 0 || min_score_adj == OOM_SCORE_ADJ_MAX + 1) {
		lowmem_print(5, "lowmem_shrink %lu, %x, return %d\n",
			     sc->nr_to_scan, sc->gfp_mask, rem);
    /*
     * disable indication if low memory
     */
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
		if (in_lowmem) {
			in_lowmem = 0;
			lowmem_indicator = 1;
			DAL_LowMemoryOff();
		}
#endif
        spin_unlock(&lowmem_shrink_lock);
		return rem;
	}

	selected_oom_score_adj = min_score_adj;
	// add debug log
#ifdef CONFIG_MT_ENG_BUILD
	if (min_score_adj <= lowmem_debug_adj) {
		lowmem_print(1, "======low memory killer=====\n");
		lowmem_print(1, "Free memory other_free: %d, other_file:%d pages\n", other_free, other_file);
	}		
#endif

	rcu_read_lock();
	for_each_process(tsk) {
		struct task_struct *p;
		int oom_score_adj;

		if (tsk->flags & PF_KTHREAD)
			continue;

		p = find_lock_task_mm(tsk);
		if (!p)
			continue;

		if (test_tsk_thread_flag(p, TIF_MEMDIE) &&
		    time_before_eq(jiffies, lowmem_deathpending_timeout)) {
		    lowmem_print(1, "lowmem_shrink return directly, due to  %d (%s) is dying\n",
			     p->pid, p->comm);
			task_unlock(p);
			rcu_read_unlock();
			spin_unlock(&lowmem_shrink_lock);
			return 0;
		}

		/* We use oom_score_adj to represent oom_adj here although the later has been deprecated by kernel. */
		/* This is because that JB AMS still uses oom_adj to stand for the importantance of activities. */
		/* oom_score_adj = p->signal->oom_score_adj;					 - 2012.07.16 - */
		oom_score_adj = p->signal->oom_adj;
#ifdef CONFIG_MT_ENG_BUILD
		if (min_score_adj <= lowmem_debug_adj)
			lowmem_print(1, "Candidate %d (%s), adj %d, rss %lu, to kill\n",
				     p->pid, p->comm, oom_score_adj, get_mm_rss(p->mm));
#endif
		if (oom_score_adj < min_score_adj) {
			task_unlock(p);
			continue;
		}
		tasksize = get_mm_rss(p->mm);
		task_unlock(p);
		if (tasksize <= 0)
			continue;
		if (selected) {
			if (oom_score_adj < selected_oom_score_adj)
				continue;
			if (oom_score_adj == selected_oom_score_adj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		selected = p;
		selected_tasksize = tasksize;
		selected_oom_score_adj = oom_score_adj;
		lowmem_print(2, "select %d (%s), adj %d, size %d, to kill\n",
			     p->pid, p->comm, oom_score_adj, tasksize);
	}
	if (selected) {
		lowmem_print(1, "send sigkill to %d (%s), adj %d, size %d\n",
			     selected->pid, selected->comm,
			     selected_oom_score_adj, selected_tasksize);
	    lowmem_deathpending = selected;
		lowmem_deathpending_timeout = jiffies + HZ;
#ifdef CONFIG_MT_ENG_BUILD
		if (min_score_adj <= lowmem_debug_adj) {
		    lowmem_print(1, "low memory info:\n");
		    show_free_areas_minimum();
        }
#endif

        /*
		 * when kill adj=0 process trigger kernel warning, only in MTK internal eng/non_LCA load
		 */
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD) && \
    !defined (MTK_LCA_SUPPORT) && !defined (PARTIAL_BUILD) //mtk internal   
        if (selected_oom_score_adj <= 0) { // can set 16 for test 
            lowmem_print(1, "low memory trigger kernel warning\n");
            aee_kernel_warning_api("LMK", 0, DB_OPT_DEFAULT|DB_OPT_DUMPSYS_ACTIVITY, 
                "Framework low memory", "please contact AP/AF memory module owner\n");
        }
#endif
		/*
		 * show an indication if low memory
		 */
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
		if (lowmem_indicator && selected_oom_score_adj <= 1) {
			lowmem_print(5, "low memory: raise aee warning\n");
			in_lowmem = 1;
			lowmem_indicator = 0;
			DAL_LowMemoryOn();
			//aee_kernel_warning(module_name, lowmem_warning);
		}
#endif
		send_sig(SIGKILL, selected, 0);
		set_tsk_thread_flag(selected, TIF_MEMDIE);
		rem -= selected_tasksize;
	}
	lowmem_print(4, "lowmem_shrink %lu, %x, return %d\n",
		     sc->nr_to_scan, sc->gfp_mask, rem);
	rcu_read_unlock();
    spin_unlock(&lowmem_shrink_lock);
	return rem;
}

static struct shrinker lowmem_shrinker = {
	.shrink = lowmem_shrink,
	.seeks = DEFAULT_SEEKS * 16
};

static int __init lowmem_init(void)
{
    task_free_register(&task_nb);
	register_shrinker(&lowmem_shrinker);
	return 0;
}

static void __exit lowmem_exit(void)
{
	unregister_shrinker(&lowmem_shrinker);
	task_free_unregister(&task_nb);
}

/*
 * get_min_free_pages
 * returns the low memory killer watermark of the given pid,
 * When the system free memory is lower than the watermark, the LMK (low memory
 * killer) may try to kill processes.
 */
int get_min_free_pages(pid_t pid)
{
    struct task_struct *p = 0;
    int target_oom_adj = 0;
    int i = 0;
    int array_size = ARRAY_SIZE(lowmem_adj);

    if (lowmem_adj_size < array_size)
            array_size = lowmem_adj_size;
    if (lowmem_minfree_size < array_size)
            array_size = lowmem_minfree_size;

    for_each_process(p) {
        /* search pid */
        if (p->pid == pid) {
            task_lock(p);
            target_oom_adj = p->signal->oom_adj;
            task_unlock(p);
            /* get min_free value of the pid */
            for (i = array_size - 1; i >= 0; i--) {
                if (target_oom_adj >= lowmem_adj[i]) {
                    lowmem_print(3, KERN_INFO"pid: %d, target_oom_adj = %d, "
                            "lowmem_adj[%d] = %d, lowmem_minfree[%d] = %d\n",
                            pid, target_oom_adj, i, lowmem_adj[i], i,
                            lowmem_minfree[i]);
                    return lowmem_minfree[i];
                }
            }
            goto out; 
        }
    }

out:
    lowmem_print(3, KERN_ALERT"[%s]pid: %d, adj: %d, lowmem_minfree = 0\n", 
            __FUNCTION__, pid, p->signal->oom_adj);
    return 0;
}
EXPORT_SYMBOL(get_min_free_pages);

/* Query LMK minfree settings */
/* To query default value, you can input index with value -1. */
size_t query_lmk_minfree(int index)
{
	int which;

	/* Invalid input index, return default value */
	if (index < 0) {
		return lowmem_minfree[2];
	}
	
	/* Find a corresponding output */
	which = 5;
	do {
		if (lowmem_adj[which] <= index) {
			break;
		}
	} while (--which >= 0);

	/* Fix underflow bug */
	which = (which < 0)? 0 : which;

	return lowmem_minfree[which];
}
EXPORT_SYMBOL(query_lmk_minfree);

module_param_named(cost, lowmem_shrinker.seeks, int, S_IRUGO | S_IWUSR);
module_param_array_named(adj, lowmem_adj, int, &lowmem_adj_size,
			 S_IRUGO | S_IWUSR);
module_param_array_named(minfree, lowmem_minfree, uint, &lowmem_minfree_size,
			 S_IRUGO | S_IWUSR);
module_param_named(debug_level, lowmem_debug_level, uint, S_IRUGO | S_IWUSR);
#if defined (CONFIG_MTK_AEE_FEATURE) && defined (CONFIG_MT_ENG_BUILD)
module_param_named(lowmem_indicator, lowmem_indicator, uint, S_IRUGO | S_IWUSR);
#endif
#ifdef CONFIG_MT_ENG_BUILD
module_param_named(debug_adj, lowmem_debug_adj, uint, S_IRUGO | S_IWUSR);
#endif

module_init(lowmem_init);
module_exit(lowmem_exit);

MODULE_LICENSE("GPL");

