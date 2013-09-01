#include <linux/device.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#ifdef CONFIG_SMP
#include <mach/hotplug.h>
#include <linux/cpu.h>
#endif

#define UNLOCK_KEY 0xC5ACCE55
#define HDBGEN (1 << 14)
#define MDBGEN (1 << 15)
#define DBGLAR 0xF0170FB0
#define DBGOSLAR 0xF0170300
#define DBGDSCR 0xF0170088
#define DBGWVR_BASE 0xF0170180
#define DBGWCR_BASE 0xF01701C0
#define DBGBVR_BASE 0xF0170100
#define DBGBCR_BASE 0xF0170140

#define DBGWFAR 0xF0170018
#define MAX_NR_WATCH_POINT 4
#define MAX_NR_BREAK_POINT 6
extern void save_dbg_regs(unsigned int data[]);
extern void restore_dbg_regs(unsigned int data[]);

void save_dbg_regs(unsigned int data[])
{
    register unsigned int cpu_id;
    int i;
    __asm__ __volatile__ ("MRC   p15, 0, %0, c0, c0, 5" :"=r"(cpu_id) );
    cpu_id &= 0xf;

    data[0] = *(volatile unsigned int *)(DBGDSCR + cpu_id * 0x2000);
    for(i = 0; i < MAX_NR_WATCH_POINT; i++) {
        data[i*2+1] = *(((volatile unsigned int *)(DBGWVR_BASE + cpu_id * 0x2000)) + i);
        data[i*2+2] = *(((volatile unsigned int *)(DBGWCR_BASE + cpu_id * 0x2000)) + i);
    }

    for(i = 0; i < MAX_NR_BREAK_POINT; i++) {
        data[i*2+9] = *(((volatile unsigned int *)(DBGBVR_BASE + cpu_id * 0x2000)) + i);
        data[i*2+10] = *(((volatile unsigned int *)(DBGBCR_BASE+ cpu_id * 0x2000)) + i);
    }
}

void restore_dbg_regs(unsigned int data[])
{
    register unsigned int cpu_id;
    int i;
    __asm__ __volatile__ ("MRC   p15, 0, %0, c0, c0, 5" :"=r"(cpu_id) );
    cpu_id &= 0xf;
    
    *(volatile unsigned int *)(DBGLAR   + cpu_id * 0x2000) = UNLOCK_KEY;
    *(volatile unsigned int *)(DBGOSLAR + cpu_id * 0x2000) = ~UNLOCK_KEY;
    *(volatile unsigned int *)(DBGDSCR  + cpu_id * 0x2000) = data[0];

    for(i = 0; i < MAX_NR_WATCH_POINT; i++) {
        *(((volatile unsigned int *)(DBGWVR_BASE + cpu_id * 0x2000)) + i) = data[i*2+1];
        *(((volatile unsigned int *)(DBGWCR_BASE + cpu_id * 0x2000)) + i) = data[i*2+2];
    } 
        
    for(i = 0; i < MAX_NR_BREAK_POINT; i++) {
        *(((volatile unsigned int *)(DBGBVR_BASE + cpu_id * 0x2000)) + i) = data[i*2+9];
        *(((volatile unsigned int *)(DBGBCR_BASE + cpu_id * 0x2000)) + i) = data[i*2+10];
    }
}

#ifdef CONFIG_SMP
static int __cpuinit
regs_hotplug_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
//        printk(KERN_ALERT "In hotplug callback\n");
	int i;
        unsigned int cpu = (unsigned int) hcpu;
        printk("regs_hotplug_callback cpu = %d\n", cpu);
        switch (action) {
        case CPU_ONLINE:
	case CPU_ONLINE_FROZEN:

	    *(volatile unsigned int *)(DBGLAR + cpu *0x2000) = UNLOCK_KEY;
            *(volatile unsigned int *)(DBGOSLAR + cpu * 0x2000) = ~UNLOCK_KEY;
	    *(volatile unsigned int *)(DBGDSCR + cpu * 0x2000) |= *(volatile unsigned int *)(DBGDSCR);
			
            for(i = 0; i < MAX_NR_WATCH_POINT; i++) {
                *(((volatile unsigned int *)(DBGWVR_BASE + cpu * 0x2000)) + i) = *(((volatile unsigned int *)DBGWVR_BASE) + i);
                *(((volatile unsigned int *)(DBGWCR_BASE + cpu * 0x2000)) + i) = *(((volatile unsigned int *)DBGWCR_BASE) + i);
            }
		
            for(i = 0; i < MAX_NR_BREAK_POINT; i++) {
                *(((volatile unsigned int *)(DBGBVR_BASE + cpu * 0x2000)) + i) = *(((volatile unsigned int *)DBGBVR_BASE) + i);
                *(((volatile unsigned int *)(DBGBCR_BASE + cpu * 0x2000)) + i) = *(((volatile unsigned int *)DBGBCR_BASE) + i);
            }
		
	break;

	default: 
	break;
        }

        return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata cpu_nfb = {
        .notifier_call = regs_hotplug_callback
};

static int __init regs_backup(void)
{
    
    register_cpu_notifier(&cpu_nfb);

    return 0;
}

module_init(regs_backup);
#endif



