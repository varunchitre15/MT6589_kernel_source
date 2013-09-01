#include <linux/sched.h>
#include <linux/aee.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/mm.h>

#include "aee-common.h"

#define RR_PROC_NAME "reboot-reason"


static struct proc_dir_entry *aee_rr_file;

struct last_reboot_reason aee_rr_last_rec;

#define WDT_NORMAL_BOOT 0
#define WDT_HW_REBOOT 1
#define WDT_SW_REBOOT 2

/*XXX Note: 2012/11/19 mtk_wdt_restart prototype is 
* different on 77 and 89 platform. the owner promise to modify it
*/
enum wk_wdt_type {
	WK_WDT_LOC_TYPE,
	WK_WDT_EXT_TYPE,
	WK_WDT_LOC_TYPE_NOLOCK,
	WK_WDT_EXT_TYPE_NOLOCK,
	
};
extern void mtk_wdt_restart(enum wk_wdt_type type);

void aee_rr_last(struct last_reboot_reason *lrr)
{
	memcpy(&aee_rr_last_rec, lrr, sizeof(struct last_reboot_reason));
}

static ssize_t aee_rr_read_reboot_reason(char *page, char **start, off_t off, 
					 int count,int *eof, void *data)
{
	int len, i;

	len =  sprintf(page, "WDT status: %d\n"
		       "fiq step: %u\n",
		       aee_rr_last_rec.wdt_status, aee_rr_last_rec.fiq_step);
	for (i = 0; i < NR_CPUS; i++) {
		len += sprintf(page + len, "CPU %d\n"
			       "  irq: enter(%d, %llu) quit(%d, %llu)\n"
			       "  sched: %llu, \"%s\"\n"
			       "  hotplug: %d, %d\n", i,
			       aee_rr_last_rec.last_irq_enter[i], aee_rr_last_rec.jiffies_last_irq_enter[i],
			       aee_rr_last_rec.last_irq_exit[i], aee_rr_last_rec.jiffies_last_irq_exit[i], 
			       aee_rr_last_rec.jiffies_last_sched[i], aee_rr_last_rec.last_sched_comm[i],
			       aee_rr_last_rec.hotplug_data1[i], aee_rr_last_rec.hotplug_data2[i]);
	}
	return len + 1;
}

void aee_rr_proc_init(struct proc_dir_entry *aed_proc_dir)
{
	aee_rr_file = create_proc_read_entry(RR_PROC_NAME, 
					     0444, aed_proc_dir, 
					     aee_rr_read_reboot_reason,
					     NULL);
	if (aee_rr_file == NULL) {
		printk(KERN_ERR "%s: Can't create rr proc entry\n", __func__);
	}
}
EXPORT_SYMBOL(aee_rr_proc_init);

void aee_rr_proc_done(struct proc_dir_entry *aed_proc_dir)
{
	remove_proc_entry(RR_PROC_NAME, aed_proc_dir);
}
EXPORT_SYMBOL(aee_rr_proc_done);

static inline unsigned int get_linear_memory_size(void)
{
   return (unsigned long)high_memory - PAGE_OFFSET; 
}

static char nested_panic_buf[4096];
static int aee_nested_printf(const char *fmt, ...)
{
    va_list args;
    static int total_len = 0;
    va_start(args, fmt);
    total_len += vsnprintf(nested_panic_buf, sizeof(nested_panic_buf), fmt, args);
    va_end(args);

    aee_sram_fiq_log(nested_panic_buf);
    
    return total_len;
}

static void print_error_msg(int len)
{
   static char error_msg[][50] = {"Bottom unaligned", "Bottom out of kernel addr", 
                                  "Top out of kernel addr", "Buf len not enough"};
   int tmp = (-len) - 1;
   
   aee_sram_fiq_log(error_msg[tmp]);
}

/*save stack as binary into buf, 
 *return value
 
    -1: bottom unaligned
    -2: bottom out of kernel addr space
    -3 top out of kernel addr addr
    -4: buff len not enough
    >0: used length of the buf
 */
int aee_dump_stack_top_binary(char *buf, int buf_len, 
                            unsigned long bottom,
		                    unsigned long top)
{
    /*should check stack address in kernel range*/
    if (bottom & 3) {
        return -1;
    }
    if (!((bottom >= (PAGE_OFFSET + THREAD_SIZE)) && 
                (bottom <= (PAGE_OFFSET + get_linear_memory_size())))){
        return -2;
    }

    if (!((top >= (PAGE_OFFSET + THREAD_SIZE)) && 
                (top <= (PAGE_OFFSET + get_linear_memory_size())))){
        return -3;
    }

    if(buf_len < top - bottom) {
        return -4;
    }

    memcpy((void *)buf, (void *)bottom, top - bottom);

    return top - bottom;
}

extern int hw_reboot_mode(void);
extern void mt_fiq_printf(const char *fmt, ...);

static atomic_t nested_panic_time = ATOMIC_INIT(0);
void aee_stop_nested_panic(struct pt_regs *regs)
{
	struct thread_info *thread = current_thread_info();
    int i = 0;
    int len = 0;
    int timeout = 1000000;
   
    local_irq_disable();
    preempt_disable();


    /*Data abort handler data abort again on many cpu.
      Since ram console api is lockless, this should be prevented*/
    if(atomic_xchg(&nested_panic_time, 1) != 0) {
        aee_nested_printf("multicore enters nested panic\n");
        goto out; 
    }

    mtk_wdt_restart(WK_WDT_LOC_TYPE_NOLOCK);
    hw_reboot_mode(); 

    /*must guarantee Only one cpu can run here*/
	aee_nested_printf("Nested panic\n");
	aee_nested_printf("Log for the previous panic:\n");
    aee_nested_printf("pc: %08lx lr: %08lx psr: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_pc, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_lr, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_cpsr);
    aee_nested_printf("sp: %08lx ip: %08lx fp: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_sp, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_ip, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_fp);
	aee_nested_printf("r10: %08lx r9: %08lx r8: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_r10, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r9, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r8);
	aee_nested_printf("r7: %08lx r6: %08lx r5: %08lx r4: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_r7, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r6, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r5, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r4);
	aee_nested_printf("r3: %08lx r2: %08lx r1: %08lx r0: %08lx\n",
			((struct pt_regs *)thread->regs_on_excp)->ARM_r3, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r2, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r1, 
			((struct pt_regs *)thread->regs_on_excp)->ARM_r0);

	aee_nested_printf("Log for the current panic:\n");
	aee_nested_printf("pc: %08lx lr: %08lx psr: %08lx\n",
			regs->ARM_pc, 
			regs->ARM_lr, 
			regs->ARM_cpsr);
	aee_nested_printf("sp: %08lx ip: %08lx fp: %08lx\n",
			regs->ARM_sp, 
			regs->ARM_ip, 
			regs->ARM_fp);
	aee_nested_printf("r10: %08lx r9: %08lx r8: %08lx\n",
			regs->ARM_r10, 
			regs->ARM_r9, 
			regs->ARM_r8);
	aee_nested_printf("r7: %08lx r6: %08lx r5: %08lx r4: %08lx\n",
			regs->ARM_r7, 
			regs->ARM_r6, 
			regs->ARM_r5, 
			regs->ARM_r4);
	len = aee_nested_printf("r3: %08lx r2: %08lx r1: %08lx r0: %08lx\n",
			regs->ARM_r3, 
			regs->ARM_r2, 
			regs->ARM_r1, 
			regs->ARM_r0);

    /*should not print stack info. this may overwhelms ram console used by fiq*/
    if(0!= in_fiq_handler()) {
            goto out;        
    }

    aee_nested_printf("stack [%08lx %08lx]", 
                      ((struct pt_regs *)thread->regs_on_excp)->ARM_sp,
                      ((struct pt_regs *)thread->regs_on_excp)->ARM_sp + 512);

    /*Dump first panic stack*/
    len = aee_dump_stack_top_binary(nested_panic_buf,
                             sizeof(nested_panic_buf), 
                             ((struct pt_regs *)thread->regs_on_excp)->ARM_sp,
                             ((struct pt_regs *)thread->regs_on_excp)->ARM_sp + 512
                            );
    

    if(len > 0) {
        aee_sram_fiq_save_bin(nested_panic_buf, len);
    }else{
        print_error_msg(len);
    }
    
    aee_nested_printf("stack [%08lx %08lx]", regs->ARM_sp,
                        regs->ARM_sp + 256);

    /*Dump second panic stack*/
    len  = aee_dump_stack_top_binary(nested_panic_buf, 
                            sizeof(nested_panic_buf), 
                            regs->ARM_sp, 
                            regs->ARM_sp + 256);


    if(len > 0) {
        aee_sram_fiq_save_bin(nested_panic_buf, len);
    }else {
        print_error_msg(len);
    }

out:
   /* waiting for the WDT timeout */
	while (1)
    {
        // output to UART directly to avoid printk nested panic
        mt_fiq_printf("%s hang here%d\t", __func__, i++);
        while(timeout--)
        {
            udelay(1);
        }
        timeout = 1000000;
    }

}






