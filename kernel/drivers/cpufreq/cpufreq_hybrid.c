/*
 *  drivers/cpufreq/cpufreq_hybrid.c
 *
 *  Copyright (C)  2001 Russell King
 *            (C)  2003 Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>.
 *                      Jun Nakajima <jun.nakajima@intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */

#define DEF_FREQUENCY_DOWN_DIFFERENTIAL     (10)
#define DEF_FREQUENCY_UP_THRESHOLD          (80)
#define DEF_SAMPLING_DOWN_FACTOR            (1)
#define MAX_SAMPLING_DOWN_FACTOR            (100000)
#define MICRO_FREQUENCY_DOWN_DIFFERENTIAL   (15)
#define MIN_FREQUENCY_DOWN_DIFFERENTIAL     (5)
#define MAX_FREQUENCY_DOWN_DIFFERENTIAL     (20)
#define MICRO_FREQUENCY_UP_THRESHOLD        (85)
#define MICRO_FREQUENCY_MIN_SAMPLE_RATE     (30000)
#define MIN_FREQUENCY_UP_THRESHOLD          (21)
#define MAX_FREQUENCY_UP_THRESHOLD          (100)

#define DEF_CPU_DOWN_THRESHOLD          (80)
#define MICRO_CPU_DOWN_THRESHOLD        (80)
#define MIN_CPU_DOWN_THRESHOLD          (60)
#define MAX_CPU_DOWN_THRESHOLD          (85)

#define LOAD_CRITICAL_GRADE             (10)
#define LOAD_HIGH_GRADE                 (5)
#define LOAD_WARNING_GRADE              (0)
#define LOAD_MEDIUM_GRADE               (-10)
#define LOAD_LIGHT_GRADE                (-20)

#define LOAD_CRITICAL           (100)
#define LOAD_HIGH               (90)
#define LOAD_WARNING            (80)
#define LOAD_MEDIUM             (50)
#define LOAD_LIGHT              (0)

#define CPU_UP_BOUND            (100)
#define CPU_DOWN_AVG_TIMES      (50)

//#define DEBUG_LOG

/*
 * The polling frequency of this governor depends on the capability of
 * the processor. Default polling frequency is 1000 times the transition
 * latency of the processor. The governor will work on any processor with
 * transition latency <= 10mS, using appropriate sampling
 * rate.
 * For CPUs with transition latency > 10mS (mostly drivers with CPUFREQ_ETERNAL)
 * this governor will not work.
 * All times here are in uS.
 */
#define MIN_SAMPLING_RATE_RATIO			(2)

static unsigned int min_sampling_rate;

#define LATENCY_MULTIPLIER			(1000)
#define MIN_LATENCY_MULTIPLIER			(100)
#define TRANSITION_LATENCY_LIMIT		(10 * 1000 * 1000)

static void do_dbs_timer(struct work_struct *work);
static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_HYBRID
static
#endif
struct cpufreq_governor cpufreq_gov_hybrid = {
       .name                   = "hybrid",
       .governor               = cpufreq_governor_dbs,
       .max_transition_latency = TRANSITION_LATENCY_LIMIT,
       .owner                  = THIS_MODULE,
};

static int g_disable_hotplug = false;

#ifdef CONFIG_SMP

static int g_next_hp_action = 0;
static int g_cpu_up_sum_grade = 0;

static long g_cpu_down_sum_load_freq = 0;
static int g_cpu_down_count = 0;

static void hp_work_handler(struct work_struct *work);
static struct delayed_work hp_work;

#endif

static int g_sum_load = 0;
static int g_available_cpu_num = 0;
static int g_limit_cpu_num = 0;

/* Sampling types */
enum {DBS_NORMAL_SAMPLE, DBS_SUB_SAMPLE};

struct cpu_dbs_info_s {
	cputime64_t prev_cpu_idle;
	cputime64_t prev_cpu_iowait;
	cputime64_t prev_cpu_wall;
	cputime64_t prev_cpu_nice;
	struct cpufreq_policy *cur_policy;
	struct delayed_work work;
	struct cpufreq_frequency_table *freq_table;
	unsigned int freq_lo;
	unsigned int freq_lo_jiffies;
	unsigned int freq_hi_jiffies;
	unsigned int rate_mult;
	int cpu;
	unsigned int sample_type:1;
	/*
	 * percpu mutex that serializes governor limit change with
	 * do_dbs_timer invocation. We do not want do_dbs_timer to run
	 * when user is changing the governor or limits.
	 */
	struct mutex timer_mutex;
};
static DEFINE_PER_CPU(struct cpu_dbs_info_s, hb_cpu_dbs_info);

static unsigned int dbs_enable;	/* number of CPUs using this policy */

/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

/*
 * dbs_hotplug protects all hotplug related global variables
 */
static DEFINE_MUTEX(hp_mutex);

static DEFINE_MUTEX(hp_onoff_mutex);

static struct dbs_tuners {
    unsigned int sampling_rate;
    unsigned int up_threshold;
    unsigned int down_differential;
    unsigned int ignore_nice;
    unsigned int sampling_down_factor;
    unsigned int powersave_bias;
    unsigned int io_is_busy;
    unsigned int cpu_down_threshold;
    int load_critical_grade;
    int load_high_grade;
    int load_warning_grade;
    int load_medium_grade;
    int load_light_grade;
    unsigned int load_critical;
    unsigned int load_high;
    unsigned int load_warning;
    unsigned int load_medium;
    unsigned int load_light;
    unsigned int cpu_up_bound;
    unsigned int cpu_down_avg_times;
} dbs_tuners_ins = {
    .up_threshold = DEF_FREQUENCY_UP_THRESHOLD,
    .sampling_down_factor = DEF_SAMPLING_DOWN_FACTOR,
    .down_differential = DEF_FREQUENCY_DOWN_DIFFERENTIAL,
    .ignore_nice = 0,
    .powersave_bias = 0,
    .cpu_down_threshold = DEF_CPU_DOWN_THRESHOLD,
    .load_critical_grade = LOAD_CRITICAL_GRADE,
    .load_high_grade = LOAD_HIGH_GRADE,
    .load_warning_grade = LOAD_WARNING_GRADE,
    .load_medium_grade = LOAD_MEDIUM_GRADE,
    .load_light_grade = LOAD_LIGHT_GRADE,
    .load_critical = LOAD_CRITICAL,
    .load_high = LOAD_HIGH,
    .load_warning = LOAD_WARNING,
    .load_medium = LOAD_MEDIUM,
    .load_light = LOAD_LIGHT,
    .cpu_up_bound = CPU_UP_BOUND,
    .cpu_down_avg_times = CPU_DOWN_AVG_TIMES,
};

static inline u64 get_cpu_idle_time_jiffy(unsigned int cpu, u64 *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
		*wall = jiffies_to_usecs(cur_wall_time);

	return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu, cputime64_t *wall)
{
	u64 idle_time = get_cpu_idle_time_us(cpu, NULL);

	if (idle_time == -1ULL)
		return get_cpu_idle_time_jiffy(cpu, wall);
	else
		idle_time += get_cpu_iowait_time_us(cpu, wall);

	return idle_time;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
	u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

	if (iowait_time == -1ULL)
		return 0;

	return iowait_time;
}

/*
 * Find right freq to be set now with powersave_bias on.
 * Returns the freq_hi to be used right now and will set freq_hi_jiffies,
 * freq_lo, and freq_lo_jiffies in percpu area for averaging freqs.
 */
static unsigned int powersave_bias_target(struct cpufreq_policy *policy,
					  unsigned int freq_next,
					  unsigned int relation)
{
	unsigned int freq_req, freq_reduc, freq_avg;
	unsigned int freq_hi, freq_lo;
	unsigned int index = 0;
	unsigned int jiffies_total, jiffies_hi, jiffies_lo;
	struct cpu_dbs_info_s *dbs_info = &per_cpu(hb_cpu_dbs_info,
						   policy->cpu);

	if (!dbs_info->freq_table) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_next;
	}

	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_next,
			relation, &index);
	freq_req = dbs_info->freq_table[index].frequency;
	freq_reduc = freq_req * dbs_tuners_ins.powersave_bias / 1000;
	freq_avg = freq_req - freq_reduc;

	/* Find freq bounds for freq_avg in freq_table */
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_H, &index);
	freq_lo = dbs_info->freq_table[index].frequency;
	index = 0;
	cpufreq_frequency_table_target(policy, dbs_info->freq_table, freq_avg,
			CPUFREQ_RELATION_L, &index);
	freq_hi = dbs_info->freq_table[index].frequency;

	/* Find out how long we have to be in hi and lo freqs */
	if (freq_hi == freq_lo) {
		dbs_info->freq_lo = 0;
		dbs_info->freq_lo_jiffies = 0;
		return freq_lo;
	}
	jiffies_total = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	jiffies_hi = (freq_avg - freq_lo) * jiffies_total;
	jiffies_hi += ((freq_hi - freq_lo) / 2);
	jiffies_hi /= (freq_hi - freq_lo);
	jiffies_lo = jiffies_total - jiffies_hi;
	dbs_info->freq_lo = freq_lo;
	dbs_info->freq_lo_jiffies = jiffies_lo;
	dbs_info->freq_hi_jiffies = jiffies_hi;
	return freq_hi;
}

static void hybrid_powersave_bias_init_cpu(int cpu)
{
	struct cpu_dbs_info_s *dbs_info = &per_cpu(hb_cpu_dbs_info, cpu);
	dbs_info->freq_table = cpufreq_frequency_get_table(cpu);
	dbs_info->freq_lo = 0;
}

static void hybrid_powersave_bias_init(void)
{
	int i;
	for_each_online_cpu(i) {
		hybrid_powersave_bias_init_cpu(i);
	}
}

/************************** sysfs interface ************************/

static ssize_t show_sampling_rate_min(struct kobject *kobj,
				      struct attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", min_sampling_rate);
}

define_one_global_ro(sampling_rate_min);

/* cpufreq_hybrid Governor Tunables */
#define show_one_unsigned(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)              \
{									\
	return sprintf(buf, "%u\n", dbs_tuners_ins.object);		\
}

#define show_one_signed(file_name, object)					\
static ssize_t show_##file_name						\
(struct kobject *kobj, struct attribute *attr, char *buf)              \
{									\
	return sprintf(buf, "%d\n", dbs_tuners_ins.object);		\
}

show_one_unsigned(sampling_rate, sampling_rate);
show_one_unsigned(io_is_busy, io_is_busy);
show_one_unsigned(up_threshold, up_threshold);
show_one_unsigned(down_differential, down_differential);
show_one_unsigned(sampling_down_factor, sampling_down_factor);
show_one_unsigned(ignore_nice_load, ignore_nice);
show_one_unsigned(powersave_bias, powersave_bias);
show_one_unsigned(cpu_down_threshold, cpu_down_threshold);
show_one_signed(load_critical_grade, load_critical_grade);
show_one_signed(load_high_grade, load_high_grade);
show_one_signed(load_warning_grade, load_warning_grade);
show_one_signed(load_medium_grade, load_medium_grade);
show_one_signed(load_light_grade, load_light_grade);
show_one_unsigned(load_critical, load_critical);
show_one_unsigned(load_high, load_high);
show_one_unsigned(load_warning, load_warning);
show_one_unsigned(load_medium, load_medium);
show_one_unsigned(load_light, load_light);
show_one_unsigned(cpu_up_bound, cpu_up_bound);
show_one_unsigned(cpu_down_avg_times, cpu_down_avg_times);

/**
 * update_sampling_rate - update sampling rate effective immediately if needed.
 * @new_rate: new sampling rate
 *
 * If new rate is smaller than the old, simply updaing
 * dbs_tuners_int.sampling_rate might not be appropriate. For example,
 * if the original sampling_rate was 1 second and the requested new sampling
 * rate is 10 ms because the user needs immediate reaction from hybrid
 * governor, but not sure if higher frequency will be required or not,
 * then, the governor may change the sampling rate too late; up to 1 second
 * later. Thus, if we are reducing the sampling rate, we need to make the
 * new value effective immediately.
 */
static void update_sampling_rate(unsigned int new_rate)
{
	int cpu;

	dbs_tuners_ins.sampling_rate = new_rate
				     = max(new_rate, min_sampling_rate);

	for_each_online_cpu(cpu) {
		struct cpufreq_policy *policy;
		struct cpu_dbs_info_s *dbs_info;
		unsigned long next_sampling, appointed_at;

		policy = cpufreq_cpu_get(cpu);
		if (!policy)
			continue;
		dbs_info = &per_cpu(hb_cpu_dbs_info, policy->cpu);
		cpufreq_cpu_put(policy);

		mutex_lock(&dbs_info->timer_mutex);

		if (!delayed_work_pending(&dbs_info->work)) {
			mutex_unlock(&dbs_info->timer_mutex);
			continue;
		}

		next_sampling  = jiffies + usecs_to_jiffies(new_rate);
		appointed_at = dbs_info->work.timer.expires;


		if (time_before(next_sampling, appointed_at)) {

			mutex_unlock(&dbs_info->timer_mutex);
			cancel_delayed_work_sync(&dbs_info->work);
			mutex_lock(&dbs_info->timer_mutex);

			schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work,
						 usecs_to_jiffies(new_rate));

		}
		mutex_unlock(&dbs_info->timer_mutex);
	}
}

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	update_sampling_rate(input);
	return count;
}

static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b,
				   const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;
	dbs_tuners_ins.io_is_busy = !!input;
	return count;
}

static ssize_t store_up_threshold(struct kobject *a, struct attribute *b,
				  const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_UP_THRESHOLD ||
			input < MIN_FREQUENCY_UP_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.up_threshold = input;
	return count;
}

static ssize_t store_down_differential(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_FREQUENCY_DOWN_DIFFERENTIAL ||
			input < MIN_FREQUENCY_DOWN_DIFFERENTIAL) {
		return -EINVAL;
	}
	dbs_tuners_ins.down_differential = input;
	return count;
}

static ssize_t store_sampling_down_factor(struct kobject *a,
			struct attribute *b, const char *buf, size_t count)
{
	unsigned int input, j;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_SAMPLING_DOWN_FACTOR || input < 1)
		return -EINVAL;
	dbs_tuners_ins.sampling_down_factor = input;

	/* Reset down sampling multiplier in case it was active */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(hb_cpu_dbs_info, j);
		dbs_info->rate_mult = 1;
	}
	return count;
}

static ssize_t store_ignore_nice_load(struct kobject *a, struct attribute *b,
				      const char *buf, size_t count)
{
	unsigned int input;
	int ret;

	unsigned int j;

	ret = sscanf(buf, "%u", &input);
	if (ret != 1)
		return -EINVAL;

	if (input > 1)
		input = 1;

	if (input == dbs_tuners_ins.ignore_nice) { /* nothing to do */
		return count;
	}
	dbs_tuners_ins.ignore_nice = input;

	/* we need to re-evaluate prev_cpu_idle */
	for_each_online_cpu(j) {
		struct cpu_dbs_info_s *dbs_info;
		dbs_info = &per_cpu(hb_cpu_dbs_info, j);
		dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&dbs_info->prev_cpu_wall);
		if (dbs_tuners_ins.ignore_nice)
			dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];

	}
	return count;
}

static ssize_t store_powersave_bias(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1)
		return -EINVAL;

	if (input > 1000)
		input = 1000;

	dbs_tuners_ins.powersave_bias = input;
	hybrid_powersave_bias_init();
	return count;
}

static ssize_t store_cpu_down_threshold(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	if (ret != 1 || input > MAX_CPU_DOWN_THRESHOLD ||
		input < MIN_CPU_DOWN_THRESHOLD) {
		return -EINVAL;
	}
	dbs_tuners_ins.cpu_down_threshold = input;
	return count;
}

static ssize_t store_load_critical_grade(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	int input;
	int ret;
	ret = sscanf(buf, "%d", &input);

	dbs_tuners_ins.load_critical_grade = input;
	return count;
}

static ssize_t store_load_high_grade(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	int input;
	int ret;
	ret = sscanf(buf, "%d", &input);

	dbs_tuners_ins.load_high_grade = input;
	return count;
}

static ssize_t store_load_warning_grade(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	int input;
	int ret;
	ret = sscanf(buf, "%d", &input);

	dbs_tuners_ins.load_warning_grade = input;
	return count;
}

static ssize_t store_load_medium_grade(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	int input;
	int ret;
	ret = sscanf(buf, "%d", &input);

	dbs_tuners_ins.load_medium_grade = input;
	return count;
}

static ssize_t store_load_light_grade(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	int input;
	int ret;
	ret = sscanf(buf, "%d", &input);

	dbs_tuners_ins.load_light_grade = input;
	return count;
}

static ssize_t store_load_critical(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	dbs_tuners_ins.load_critical = input;
	return count;
}


static ssize_t store_load_high(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	dbs_tuners_ins.load_high = input;
	return count;
}

static ssize_t store_load_warning(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	dbs_tuners_ins.load_warning = input;
	return count;
}

static ssize_t store_load_medium(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	dbs_tuners_ins.load_medium = input;
	return count;
}

static ssize_t store_load_light(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	dbs_tuners_ins.load_light = input;
	return count;
}

static ssize_t store_cpu_up_bound(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	dbs_tuners_ins.cpu_up_bound = input;
	return count;
}

static ssize_t store_cpu_down_avg_times(struct kobject *a, struct attribute *b,
				    const char *buf, size_t count)
{
	unsigned int input;
	int ret;
	ret = sscanf(buf, "%u", &input);

	dbs_tuners_ins.cpu_down_avg_times = input;
	return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(up_threshold);
define_one_global_rw(down_differential);
define_one_global_rw(sampling_down_factor);
define_one_global_rw(ignore_nice_load);
define_one_global_rw(powersave_bias);
define_one_global_rw(cpu_down_threshold);
define_one_global_rw(load_critical_grade);
define_one_global_rw(load_high_grade);
define_one_global_rw(load_warning_grade);
define_one_global_rw(load_medium_grade);
define_one_global_rw(load_light_grade);
define_one_global_rw(load_critical);
define_one_global_rw(load_high);
define_one_global_rw(load_warning);
define_one_global_rw(load_medium);
define_one_global_rw(load_light);
define_one_global_rw(cpu_up_bound);
define_one_global_rw(cpu_down_avg_times);

static struct attribute *dbs_attributes[] = {
    &sampling_rate_min.attr,
    &sampling_rate.attr,
    &up_threshold.attr,
    &down_differential.attr,
    &sampling_down_factor.attr,
    &ignore_nice_load.attr,
    &powersave_bias.attr,
    &io_is_busy.attr,
    &cpu_down_threshold.attr,
    &load_critical_grade.attr,
    &load_high_grade.attr,
    &load_warning_grade.attr,
    &load_medium_grade.attr,
    &load_light_grade.attr,
    &load_critical.attr,
    &load_high.attr,
    &load_warning.attr,
    &load_medium.attr,
    &load_light.attr,
    &cpu_up_bound.attr,
    &cpu_down_avg_times.attr,
    NULL
};

static struct attribute_group dbs_attr_group = {
	.attrs = dbs_attributes,
	.name = "hybrid",
};

/************************** sysfs end ************************/

static void dbs_freq_increase(struct cpufreq_policy *p, unsigned int freq)
{
	if (dbs_tuners_ins.powersave_bias)
		freq = powersave_bias_target(p, freq, CPUFREQ_RELATION_H);
	else if (p->cur == p->max)
		return;

	__cpufreq_driver_target(p, freq, dbs_tuners_ins.powersave_bias ?
			CPUFREQ_RELATION_L : CPUFREQ_RELATION_H);
}

void disable_hotplug_policy(bool disable, int limit_cpu_num)
{
	mutex_lock(&hp_mutex);
	if (disable) {
		g_disable_hotplug = true;
		printk("cpufreq_hybrid: disable hotplug policy\n");
	}
	else {
		g_disable_hotplug = false;
		printk("cpufreq_hybrid: enable hotplug policy\n");
	}

	g_limit_cpu_num = limit_cpu_num;
	mutex_unlock(&hp_mutex);
}
EXPORT_SYMBOL(disable_hotplug_policy);

#ifdef CONFIG_SMP
static int hb_check_grade(int load)
{
	int grade = 0;

	if (load >= dbs_tuners_ins.load_critical)
		grade = dbs_tuners_ins.load_critical_grade;
	else if (load >= dbs_tuners_ins.load_high)
		grade = dbs_tuners_ins.load_high_grade;
	else if (load >= dbs_tuners_ins.load_warning)
		grade = dbs_tuners_ins.load_warning_grade;
	else if (load >= dbs_tuners_ins.load_medium)
		grade = dbs_tuners_ins.load_medium_grade;
	else if (load >= dbs_tuners_ins.load_light)
		grade = dbs_tuners_ins.load_light_grade;
	else
		grade = 0;

	#ifdef DEBUG_LOG
		printk("hb_check_grade: load = %d, grade = %d\n", load, grade);
	#endif

	return grade;
}

static void hp_work_handler(struct work_struct *work)
{
	if (mutex_trylock(&hp_onoff_mutex))
	{
		if (g_next_hp_action)
		{
			printk("hp_work_handler: cpu_up kick off\n");
			cpu_up(1);
			printk("hp_work_handler: cpu_up completion\n");
		}
		else
		{
			printk("hp_work_handler: cpu_down kick off\n");
			cpu_down(1);
			printk("hp_work_handler: cpu_down completion\n");
		}
		mutex_unlock(&hp_onoff_mutex);
	}
}

#endif

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	unsigned int max_load_freq;
	unsigned int sum_load_freq;

	struct cpufreq_policy *policy;
	unsigned int j;

	this_dbs_info->freq_lo = 0;
	policy = this_dbs_info->cur_policy;

	/*
	 * Every sampling_rate, we check, if current idle time is less
	 * than 20% (default), then we try to increase frequency
	 * Every sampling_rate, we look for a the lowest
	 * frequency which can sustain the load while keeping idle time over
	 * 30%. If such a frequency exist, we try to decrease to this frequency.
	 *
	 * Any frequency increase takes it to the maximum frequency.
	 * Frequency reduction happens at minimum steps of
	 * 5% (default) of current frequency
	 */

	/* Get Absolute Load - in terms of freq */
	max_load_freq = 0;
	sum_load_freq = 0;

	g_sum_load = 0;

	for_each_cpu(j, policy->cpus) {
		struct cpu_dbs_info_s *j_dbs_info;
		cputime64_t cur_wall_time, cur_idle_time, cur_iowait_time;
		unsigned int idle_time, wall_time, iowait_time;
		unsigned int load, load_freq;
		int freq_avg;

		j_dbs_info = &per_cpu(hb_cpu_dbs_info, j);

		cur_idle_time = get_cpu_idle_time(j, &cur_wall_time);
		cur_iowait_time = get_cpu_iowait_time(j, &cur_wall_time);

		wall_time = (unsigned int)
			(cur_wall_time - j_dbs_info->prev_cpu_wall);
		j_dbs_info->prev_cpu_wall = cur_wall_time;

		idle_time = (unsigned int)
			(cur_idle_time - j_dbs_info->prev_cpu_idle);
		j_dbs_info->prev_cpu_idle = cur_idle_time;

		iowait_time = (unsigned int)
			(cur_iowait_time - j_dbs_info->prev_cpu_iowait);
		j_dbs_info->prev_cpu_iowait = cur_iowait_time;

		if (dbs_tuners_ins.ignore_nice) {
			u64 cur_nice;
			unsigned long cur_nice_jiffies;

			cur_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE] -
					 j_dbs_info->prev_cpu_nice;
			/*
			 * Assumption: nice time between sampling periods will
			 * be less than 2^32 jiffies for 32 bit sys
			 */
			cur_nice_jiffies = (unsigned long)
					cputime64_to_jiffies64(cur_nice);

			j_dbs_info->prev_cpu_nice = kcpustat_cpu(j).cpustat[CPUTIME_NICE];
			idle_time += jiffies_to_usecs(cur_nice_jiffies);
		}

		/*
		 * For the purpose of hybrid, waiting for disk IO is an
		 * indication that you're performance critical, and not that
		 * the system is actually idle. So subtract the iowait time
		 * from the cpu idle time.
		 */

		if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
			idle_time -= iowait_time;

		if (unlikely(!wall_time || wall_time < idle_time))
			continue;

		load = 100 * (wall_time - idle_time) / wall_time;

		g_sum_load += load;

		freq_avg = __cpufreq_driver_getavg(policy, j);
		if (freq_avg <= 0)
			freq_avg = policy->cur;

		load_freq = load * freq_avg;
		if (load_freq > max_load_freq)
			max_load_freq = load_freq;
			
		sum_load_freq += load_freq;

		#ifdef DEBUG_LOG
		printk("dbs_check_cpu: cpu = %d\n", j);
		printk("dbs_check_cpu: wall_time = %d, idle_time = %d, load = %d\n", wall_time, idle_time, load);
		printk("dbs_check_cpu: freq_avg = %d, max_load_freq = %d, sum_load_freq = %d\n", freq_avg, max_load_freq, sum_load_freq);
		#endif
	}

	/* Check for frequency increase */
	if (max_load_freq > dbs_tuners_ins.up_threshold * policy->cur) {
		/* If switching to max speed, apply sampling_down_factor */
		if (policy->cur < policy->max)
			this_dbs_info->rate_mult =
				dbs_tuners_ins.sampling_down_factor;
		dbs_freq_increase(policy, policy->max);
		goto hp_check;
	}

	/* Check for frequency decrease */
	/* if we cannot reduce the frequency anymore, break out early */
	if (policy->cur == policy->min)
		goto hp_check;

	/*
	 * The optimal frequency is the frequency that is the lowest that
	 * can support the current CPU usage without triggering the up
	 * policy. To be safe, we focus 10 points under the threshold.
	 */
	if (max_load_freq <
	    (dbs_tuners_ins.up_threshold - dbs_tuners_ins.down_differential) *
	     policy->cur) {
		unsigned int freq_next;
		freq_next = max_load_freq /
				(dbs_tuners_ins.up_threshold -
				 dbs_tuners_ins.down_differential);

		/* No longer fully busy, reset rate_mult */
		this_dbs_info->rate_mult = 1;

		if (freq_next < policy->min)
			freq_next = policy->min;

		if (!dbs_tuners_ins.powersave_bias) {
			__cpufreq_driver_target(policy, freq_next,
					CPUFREQ_RELATION_L);
		} else {
			int freq = powersave_bias_target(policy, freq_next,
					CPUFREQ_RELATION_L);
			__cpufreq_driver_target(policy, freq,
				CPUFREQ_RELATION_L);
		}
	}

hp_check:

	#ifdef CONFIG_SMP
	mutex_lock(&hp_mutex);
	if (!g_disable_hotplug) {
		/* Check if power up slave CPU */
		if (num_online_cpus() == 1) {
			g_cpu_up_sum_grade += hb_check_grade((sum_load_freq / policy->cur));
			if (g_cpu_up_sum_grade < 0)
				g_cpu_up_sum_grade = 0;
			if (g_cpu_up_sum_grade >= dbs_tuners_ins.cpu_up_bound) {
				#ifdef DEBUG_LOG
				printk("dbs_check_cpu: g_cpu_up_sum_grade = %d, turn on CPU1\n", g_cpu_up_sum_grade);
				#endif
				if (num_online_cpus() < g_limit_cpu_num) {
					printk("dbs_check_cpu: turn on CPU1\n");
					g_next_hp_action = 1;
					schedule_delayed_work_on(0, &hp_work, 0);
				}
				g_cpu_up_sum_grade = 0;
			}
			#ifdef DEBUG_LOG
			printk("dbs_check_cpu: g_cpu_up_sum_grade = %d\n", g_cpu_up_sum_grade);
			#endif
		} else { /* Check if power down slave CPU */
			if (num_online_cpus() > 1) {
				g_cpu_down_count++;
				g_cpu_down_sum_load_freq += ((sum_load_freq) / 100);
				if (g_cpu_down_count == dbs_tuners_ins.cpu_down_avg_times) {
					g_cpu_down_sum_load_freq /= dbs_tuners_ins.cpu_down_avg_times;
					if (g_cpu_down_sum_load_freq < 
						((dbs_tuners_ins.cpu_down_threshold * policy->max) / 100)) {
						#ifdef DEBUG_LOG
						printk("dbs_check_cpu: g_cpu_down_sum_load_freq = %d, turn off CPU1\n", g_cpu_down_sum_load_freq);
						#endif
						dbs_freq_increase(policy, policy->max);
						printk("dbs_check_cpu: turn off CPU1\n");
						g_next_hp_action = 0;
						schedule_delayed_work_on(0, &hp_work, 0);
					}
					g_cpu_down_count = 0;
					g_cpu_down_sum_load_freq = 0;
				}
				#ifdef DEBUG_LOG
				printk("dbs_check_cpu: g_cpu_down_count = %d, g_cpu_down_sum_load_freq = %d\n", g_cpu_down_count, g_cpu_down_sum_load_freq);
				printk("dbs_check_cpu: cpu_down_freq = %d\n", dbs_tuners_ins.cpu_down_threshold * policy->max);
				#endif
			}
		}
	}
	mutex_unlock(&hp_mutex);
	#endif

	return;
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info =
		container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;
	int sample_type = dbs_info->sample_type;

	int delay;

	mutex_lock(&dbs_info->timer_mutex);

	/* Common NORMAL_SAMPLE setup */
	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	if (!dbs_tuners_ins.powersave_bias ||
	    sample_type == DBS_NORMAL_SAMPLE) {
		dbs_check_cpu(dbs_info);
		if (dbs_info->freq_lo) {
			/* Setup timer for SUB_SAMPLE */
			dbs_info->sample_type = DBS_SUB_SAMPLE;
			delay = dbs_info->freq_hi_jiffies;
		} else {
			/* We want all CPUs to do sampling nearly on
			 * same jiffy
			 */
			delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate
				* dbs_info->rate_mult);

			if (num_online_cpus() > 1)
				delay -= jiffies % delay;
		}
	} else {
		__cpufreq_driver_target(dbs_info->cur_policy,
			dbs_info->freq_lo, CPUFREQ_RELATION_H);
		delay = dbs_info->freq_lo_jiffies;
	}
	schedule_delayed_work_on(cpu, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);

	if (num_online_cpus() > 1)
		delay -= jiffies % delay;

	dbs_info->sample_type = DBS_NORMAL_SAMPLE;
	INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
	cancel_delayed_work_sync(&dbs_info->work);
}

/*
 * Not all CPUs want IO time to be accounted as busy; this dependson how
 * efficient idling at a higher frequency/voltage is.
 * Pavel Machek says this is not so for various generations of AMD and old
 * Intel systems.
 * Mike Chan (androidlcom) calis this is also not true for ARM.
 * Because of this, whitelist specific known (series) of CPUs by default, and
 * leave all others up to the user.
 */
static int should_io_be_busy(void)
{
#if defined(CONFIG_X86)
	/*
	 * For Intel, Core 2 (model 15) andl later have an efficient idle.
	 */
	if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL &&
	    boot_cpu_data.x86 == 6 &&
	    boot_cpu_data.x86_model >= 15)
		return 1;
#endif
	return 1; // io wait time should be subtracted from idle time
}

static int cpufreq_governor_dbs(struct cpufreq_policy *policy,
				   unsigned int event)
{
	unsigned int cpu = policy->cpu;
	struct cpu_dbs_info_s *this_dbs_info;
	unsigned int j;
	int rc;

	this_dbs_info = &per_cpu(hb_cpu_dbs_info, cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
		if ((!cpu_online(cpu)) || (!policy->cur))
			return -EINVAL;

		mutex_lock(&dbs_mutex);

		dbs_enable++;
		for_each_cpu(j, policy->cpus) {
			struct cpu_dbs_info_s *j_dbs_info;
			j_dbs_info = &per_cpu(hb_cpu_dbs_info, j);
			j_dbs_info->cur_policy = policy;

			j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j,
						&j_dbs_info->prev_cpu_wall);
			if (dbs_tuners_ins.ignore_nice)
				j_dbs_info->prev_cpu_nice =
						kcpustat_cpu(j).cpustat[CPUTIME_NICE];
		}
		this_dbs_info->cpu = cpu;
		this_dbs_info->rate_mult = 1;
		hybrid_powersave_bias_init_cpu(cpu);
		/*
		 * Start the timerschedule work, when this governor
		 * is used for first time
		 */
		if (dbs_enable == 1) {
			unsigned int latency;

			rc = sysfs_create_group(cpufreq_global_kobject,
						&dbs_attr_group);
			if (rc) {
				mutex_unlock(&dbs_mutex);
				return rc;
			}

			/* policy latency is in nS. Convert it to uS first */
			latency = policy->cpuinfo.transition_latency / 1000;
			if (latency == 0)
				latency = 1;
			/* Bring kernel and HW constraints together */
			min_sampling_rate = max(min_sampling_rate,
					MIN_LATENCY_MULTIPLIER * latency);
			dbs_tuners_ins.sampling_rate =
				max(min_sampling_rate,
				    latency * LATENCY_MULTIPLIER);
			dbs_tuners_ins.io_is_busy = should_io_be_busy();
			
			#ifdef DEBUG_LOG
			printk("cpufreq_governor_dbs: min_sampling_rate = %d\n", min_sampling_rate);
			printk("cpufreq_governor_dbs: dbs_tuners_ins.sampling_rate = %d\n", dbs_tuners_ins.sampling_rate);
			printk("cpufreq_governor_dbs: dbs_tuners_ins.io_is_busy = %d\n", dbs_tuners_ins.io_is_busy);
			#endif
		}
		mutex_unlock(&dbs_mutex);

		mutex_init(&this_dbs_info->timer_mutex);
		dbs_timer_init(this_dbs_info);
		break;

	case CPUFREQ_GOV_STOP:
		dbs_timer_exit(this_dbs_info);

		mutex_lock(&dbs_mutex);
		mutex_destroy(&this_dbs_info->timer_mutex);
		dbs_enable--;
		mutex_unlock(&dbs_mutex);
		if (!dbs_enable)
			sysfs_remove_group(cpufreq_global_kobject,
					   &dbs_attr_group);

		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->min, CPUFREQ_RELATION_L);
		mutex_unlock(&this_dbs_info->timer_mutex);
		break;
	}
	return 0;
}

int cpufreq_gov_dbs_get_sum_load(void)
{
	/* only for dual core turbo mode */
	return g_sum_load;
}

static int __init cpufreq_gov_dbs_init(void)
{
	u64 idle_time;
	int cpu = get_cpu();

	idle_time = get_cpu_idle_time_us(cpu, NULL);
	put_cpu();
	if (idle_time != -1ULL) {
		/* Idle micro accounting is supported. Use finer thresholds */
		dbs_tuners_ins.up_threshold = MICRO_FREQUENCY_UP_THRESHOLD;
		dbs_tuners_ins.down_differential =
					MICRO_FREQUENCY_DOWN_DIFFERENTIAL;
		dbs_tuners_ins.cpu_down_threshold =
					MICRO_CPU_DOWN_THRESHOLD;
		/*
		 * In nohz/micro accounting case we set the minimum frequency
		 * not depending on HZ, but fixed (very low). The deferred
		 * timer might skip some samples if idle/sleeping as needed.
		*/
		min_sampling_rate = MICRO_FREQUENCY_MIN_SAMPLE_RATE;
	} else {
		/* For correct statistics, we need 10 ticks for each measure */
		min_sampling_rate =
			MIN_SAMPLING_RATE_RATIO * jiffies_to_usecs(10);
	}

	g_limit_cpu_num = g_available_cpu_num = num_possible_cpus();

	#ifdef CONFIG_SMP
	INIT_DELAYED_WORK_DEFERRABLE(&hp_work, hp_work_handler);
	#endif

	#ifdef DEBUG_LOG
	printk("cpufreq_gov_dbs_init: min_sampling_rate = %d\n", min_sampling_rate);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.up_threshold = %d\n", dbs_tuners_ins.up_threshold);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.down_differential = %d\n", dbs_tuners_ins.down_differential);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.cpu_down_threshold = %d\n", dbs_tuners_ins.cpu_down_threshold);
	printk("cpufreq_gov_dbs_init: g_available_cpu_num = %d\n", g_available_cpu_num);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_critical_grade = %d\n", dbs_tuners_ins.load_critical_grade);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_high_grade = %d\n", dbs_tuners_ins.load_high_grade);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_warning_grade = %d\n", dbs_tuners_ins.load_warning_grade);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_medium_grade = %d\n", dbs_tuners_ins.load_medium_grade);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_light_grade = %d\n", dbs_tuners_ins.load_light_grade);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_critical = %d\n", dbs_tuners_ins.load_critical);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_high = %d\n", dbs_tuners_ins.load_high);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_warning = %d\n", dbs_tuners_ins.load_warning);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_medium = %d\n", dbs_tuners_ins.load_medium);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.load_light = %d\n", dbs_tuners_ins.load_light);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.cpu_up_bound = %d\n", dbs_tuners_ins.cpu_up_bound);
	printk("cpufreq_gov_dbs_init: dbs_tuners_ins.cpu_down_avg_times = %d\n", dbs_tuners_ins.cpu_down_avg_times);
	#endif 

	return cpufreq_register_governor(&cpufreq_gov_hybrid);
}

static void __exit cpufreq_gov_dbs_exit(void)
{
	#ifdef CONFIG_SMP
	cancel_delayed_work_sync(&hp_work);
	#endif

	cpufreq_unregister_governor(&cpufreq_gov_hybrid);
}


MODULE_AUTHOR("Venkatesh Pallipadi <venkatesh.pallipadi@intel.com>");
MODULE_AUTHOR("Alexey Starikovskiy <alexey.y.starikovskiy@intel.com>");
MODULE_DESCRIPTION("'cpufreq_hybrid' - A dynamic cpufreq governor for "
	"Low Latency Frequency Transition capable processors");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_HYBRID
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);
