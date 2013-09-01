#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/interrupt.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>

#include "mach/mt_typedefs.h"
#include "mach/irqs.h"
#include "mach/mt_ptp.h"
#include <mach/mt_spm_idle.h>
#include "mach/mt_reg_base.h"
#include "mach/mt_cpufreq.h"
#include "mach/mt_thermal.h"

#if En_ISR_log
#define clc_isr_info(fmt, args...)     clc_notice( fmt, ##args)
#else
#define clc_isr_info(fmt, args...)     clc_debug( fmt, ##args)
#endif

extern int mt_irq_mask_all(struct mtk_irq_mask *mask);
extern int mt_irq_mask_restore(struct mtk_irq_mask *mask);
extern u32 get_devinfo_with_index(u32 index);
//extern int spm_dvfs_ctrl_volt(u32 value);
extern unsigned int mt_cpufreq_max_frequency_by_DVS(unsigned int num);
extern void mt_fh_popod_save(void);
extern void mt_fh_popod_restore(void);
extern void mt_cpufreq_return_default_DVS_by_ptpod(void);
extern unsigned int mt_cpufreq_voltage_set_by_ptpod(unsigned int pmic_volt[], unsigned int array_size);


u32 PTP_Enable = 1;
volatile u32 ptp_data[3] = {0xffffffff, 0, 0};
u32 PTP_output_voltage_failed = 0;

u32 val_0 = 0x14f76907;
u32 val_1 = 0xf6AAAAAA;
u32 val_2 = 0x14AAAAAA;
u32 val_3 = 0x60260000;

unsigned int ptpod_pmic_volt[8] = {0x51, 0x47, 0x37, 0x27, 0x17, 0,0,0};

u8 freq_0, freq_1, freq_2, freq_3;
u8 freq_4, freq_5, freq_6, freq_7;

u32 ptp_level;

#if 0
static void ptp_do_mon_tasklet_handler(unsigned long data)
{
    #ifdef CONFIG_THERMAL
        PTP_MON_MODE();    
    #endif
}

static void ptp_do_init2_tasklet_handler(unsigned long data)
{
        PTP_INIT_02();    
}

static DECLARE_TASKLET(ptp_do_mon_tasklet, ptp_do_mon_tasklet_handler, 0);
static DECLARE_TASKLET(ptp_do_init2_tasklet, ptp_do_init2_tasklet_handler, 0);
#endif

static struct hrtimer mt_ptp_timer;
struct task_struct *mt_ptp_thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(mt_ptp_timer_waiter);

static int mt_ptp_timer_flag = 0;
static int mt_ptp_period_s = 2;
static int mt_ptp_period_ns = 0;

enum hrtimer_restart mt_ptp_timer_func(struct hrtimer *timer)
{
    mt_ptp_timer_flag = 1; wake_up_interruptible(&mt_ptp_timer_waiter);
    return HRTIMER_NORESTART;
}

int mt_ptp_thread_handler(void *unused)
{
    do
    {
        ktime_t ktime = ktime_set(mt_ptp_period_s, mt_ptp_period_ns);

        wait_event_interruptible(mt_ptp_timer_waiter, mt_ptp_timer_flag != 0);
        mt_ptp_timer_flag = 0;

        clc_notice("PTP_LOG: (%d) - (0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x) - (%d, %d, %d, %d, %d, %d, %d, %d)\n", \
                    mtktscpu_get_cpu_temp(), \
                    DRV_Reg32(PMIC_WRAP_DVFS_WDATA0), \
                    DRV_Reg32(PMIC_WRAP_DVFS_WDATA1), \
                    DRV_Reg32(PMIC_WRAP_DVFS_WDATA2), \
                    DRV_Reg32(PMIC_WRAP_DVFS_WDATA3), \
                    DRV_Reg32(PMIC_WRAP_DVFS_WDATA4), \
                    DRV_Reg32(PMIC_WRAP_DVFS_WDATA5), \
                    DRV_Reg32(PMIC_WRAP_DVFS_WDATA6), \
                    DRV_Reg32(PMIC_WRAP_DVFS_WDATA7), \
                    mt_cpufreq_max_frequency_by_DVS(0), \
                    mt_cpufreq_max_frequency_by_DVS(1), \
                    mt_cpufreq_max_frequency_by_DVS(2), \
                    mt_cpufreq_max_frequency_by_DVS(3), \
                    mt_cpufreq_max_frequency_by_DVS(4), \
                    mt_cpufreq_max_frequency_by_DVS(5), \
                    mt_cpufreq_max_frequency_by_DVS(6), \
                    mt_cpufreq_max_frequency_by_DVS(7));

        hrtimer_start(&mt_ptp_timer, ktime, HRTIMER_MODE_REL);
    } while (!kthread_should_stop());

    return 0;
}

#if 1
static void PTP_set_ptp_volt(void)
{
    #if Set_PMIC_Volt
    u32 array_size;

    array_size = 0;
    
    // set PTP_VO_0 ~ PTP_VO_7 to PMIC
    if( freq_0 != 0 )
    {
        // 1.6G : 1_188v, 1.5G : 1_1255v, 1.4G : 1_063v, 1.2G : 1_013v 
        if( ( (ptp_level == 4) && (PTP_VO_0 < 79) ) || ( (ptp_level == 3) && (PTP_VO_0 < 68) ) || ( (ptp_level == 2) && (PTP_VO_0 < 54) ) || ( (PTP_VO_0 < 51) ) )    
        {
            // restore default DVFS table (PMIC)
            clc_isr_info("PTP Error : ptp_level = 0x%x, PTP_VO_0 = 0x%x \n", ptp_level, PTP_VO_0);
            mt_cpufreq_return_default_DVS_by_ptpod();
            PTP_output_voltage_failed = 1;

            return;
        }
        	
        ptpod_pmic_volt[0] =  PTP_VO_0; 
        array_size++;
    }
    
    if( freq_1 != 0 )
    {
        // 1_013v
        if( (ptp_level != 0) && (ptp_level != 6) && (ptp_level != 7) && (PTP_VO_1 < 51) )  
        {
            // restore default DVFS table (PMIC)
            clc_isr_info("PTP Error : ptp_level = 0x%x, PTP_VO_1 = 0x%x \n", ptp_level, PTP_VO_1);
            mt_cpufreq_return_default_DVS_by_ptpod();
            PTP_output_voltage_failed = 1;

            return;
        }
    
        ptpod_pmic_volt[1] =  PTP_VO_1; 
        array_size++;
    }
    
    if( freq_2 != 0 )
    {
        ptpod_pmic_volt[2] =  PTP_VO_2; 
        array_size++;
    }
    
    if( freq_3 != 0 )
    {
        ptpod_pmic_volt[3] =  PTP_VO_3; 
        array_size++;
    }
    
    if( freq_4 != 0 )
    {
        ptpod_pmic_volt[4] =  PTP_VO_4; 
        array_size++;
    }

    mt_cpufreq_voltage_set_by_ptpod(ptpod_pmic_volt, array_size);
    
    #endif
}
#else
static void PTP_set_ptp_volt(void)
{
    #if Set_PMIC_Volt
    
    // set PTP_VO_0 ~ PTP_VO_7 to PMIC
    if( freq_0 != 0 )
    {
        // 1_188v, 1_063v, 1_013v 
        if( ( (ptp_level == 4) && (PTP_VO_0 < 79) ) || ( (ptp_level == 2) && (PTP_VO_0 < 54) ) || ( (PTP_VO_0 < 51) ) )    
        {
            // restore default DVFS table (PMIC)
            clc_isr_info("PTP Error : ptp_level = 0x%x, PTP_VO_0 = 0x%x \n", ptp_level, PTP_VO_0);
            mt_cpufreq_return_default_DVS_by_ptpod();

            return;
        }
        	
        ptp_write(PTP_PMIC_WRAP_DVFS_ADR0, 0x021A);
        ptp_write(PTP_PMIC_WRAP_DVFS_WDATA0, PTP_VO_0); 
    }
    
    if( freq_1 != 0 )
    {
        // 1_013v
        if( (ptp_level != 0) && (ptp_level != 6) && (ptp_level != 7) && (PTP_VO_1 < 51) )  
        {
            // restore default DVFS table (PMIC)
            clc_isr_info("PTP Error : ptp_level = 0x%x, PTP_VO_1 = 0x%x \n", ptp_level, PTP_VO_1);
            mt_cpufreq_return_default_DVS_by_ptpod();

            return;
        }
    
        ptp_write(PTP_PMIC_WRAP_DVFS_ADR1, 0x021A);
        ptp_write(PTP_PMIC_WRAP_DVFS_WDATA1, PTP_VO_1); 
    }
    
    if( freq_2 != 0 )
    {
        ptp_write(PTP_PMIC_WRAP_DVFS_ADR2, 0x021A);
        ptp_write(PTP_PMIC_WRAP_DVFS_WDATA2, PTP_VO_2); 
    }
    
    if( freq_3 != 0 )
    {
        ptp_write(PTP_PMIC_WRAP_DVFS_ADR3, 0x021A);
        ptp_write(PTP_PMIC_WRAP_DVFS_WDATA3, PTP_VO_3); 
    }
    
    if( freq_4 != 0 )
    {
        ptp_write(PTP_PMIC_WRAP_DVFS_ADR4, 0x021A);
        ptp_write(PTP_PMIC_WRAP_DVFS_WDATA4, PTP_VO_4); 
    }
    
    //ptp_write(PTP_PMIC_WRAP_DVFS_ADR5, 0x026C);
    //ptp_write(PTP_PMIC_WRAP_DVFS_WDATA5, PTP_VO_5); 
    
    //ptp_write(PTP_PMIC_WRAP_DVFS_ADR6, 0x026C);
    //ptp_write(PTP_PMIC_WRAP_DVFS_WDATA6, PTP_VO_6); 
    
    //ptp_write(PTP_PMIC_WRAP_DVFS_ADR7, 0x026C);
    //ptp_write(PTP_PMIC_WRAP_DVFS_WDATA7, PTP_VO_7);         
    
    #endif
}
#endif

irqreturn_t MT6589_PTP_ISR(int irq, void *dev_id)
{
    u32 PTPINTSTS, temp, temp_0, temp_ptpen;

    PTPINTSTS = ptp_read( PTP_PTPINTSTS );
    temp_ptpen = ptp_read(PTP_PTPEN);        
    clc_isr_info("===============ISR ISR ISR ISR ISR=============\n");
    clc_isr_info("PTPINTSTS = 0x%x\n", PTPINTSTS);
    clc_isr_info("PTP_PTPEN = 0x%x\n", temp_ptpen);
    ptp_data[1] = ptp_read(0xf100c240);
    ptp_data[2] = ptp_read(0xf100c27c);
    clc_isr_info("*(0x1100c240) = 0x%x\n", ptp_data[1]);
    clc_isr_info("*(0x1100c27c) = 0x%x\n", ptp_data[2]);
    ptp_data[0] = 0;    
    
    #if 0
    for( temp = 0xf100c200 ; temp <= 0xf100c25c ; temp+=4 )
    {
        clc_isr_info("*(0x%x) = 0x%x\n", temp, ptp_read(temp));
    }
    #endif

    if( PTPINTSTS == 0x1 ) // PTP init1 or init2
    {        
        if( (temp_ptpen & 0x7) == 0x1 ) // init 1 =============================
        {
            #if 0
            /*
            // read PTP_VO_0 ~ PTP_VO_3
            temp = ptp_read( PTP_VOP30 );
            PTP_VO_0 = temp & 0xff;
            PTP_VO_1 = (temp>>8) & 0xff;
            PTP_VO_2 = (temp>>16) & 0xff;
            PTP_VO_3 = (temp>>24) & 0xff;
            
            // read PTP_VO_3 ~ PTP_VO_7
            temp = ptp_read( PTP_VOP74 );
            PTP_VO_4 = temp & 0xff;
            PTP_VO_5 = (temp>>8) & 0xff;
            PTP_VO_6 = (temp>>16) & 0xff;
            PTP_VO_7 = (temp>>24) & 0xff;                  
            
            // show PTP_VO_0 ~ PTP_VO_7 to PMIC
            clc_isr_info("PTP_VO_0 = 0x%x\n", PTP_VO_0);
            clc_isr_info("PTP_VO_1 = 0x%x\n", PTP_VO_1);
            clc_isr_info("PTP_VO_2 = 0x%x\n", PTP_VO_2);
            clc_isr_info("PTP_VO_3 = 0x%x\n", PTP_VO_3);
            clc_isr_info("PTP_VO_4 = 0x%x\n", PTP_VO_4);
            clc_isr_info("PTP_VO_5 = 0x%x\n", PTP_VO_5);
            clc_isr_info("PTP_VO_6 = 0x%x\n", PTP_VO_6);
            clc_isr_info("PTP_VO_7 = 0x%x\n", PTP_VO_7);        
            clc_isr_info("ptp_level = 0x%x\n", ptp_level);        
            clc_isr_info("================================================\n");
            
            PTP_set_ptp_volt();            
            */
            #endif
            
            // Read & store 16 bit values DCVALUES.DCVOFFSET and AGEVALUES.AGEVOFFSET for later use in INIT2 procedure
            PTP_DCVOFFSET = ~(ptp_read(PTP_DCVALUES) & 0xffff)+1;  // hw bug, workaround
            PTP_AGEVOFFSET = ptp_read(PTP_AGEVALUES) & 0xffff;
        
            PTP_INIT_FLAG = 1;

            // Set PTPEN.PTPINITEN/PTPEN.PTPINIT2EN = 0x0 & Clear PTP INIT interrupt PTPINTSTS = 0x00000001
            ptp_write(PTP_PTPEN, 0x0);
            ptp_write(PTP_PTPINTSTS, 0x1);
            //tasklet_schedule(&ptp_do_init2_tasklet);

            mt_cpufreq_enable_by_ptpod(); // enable DVFS
            mt_fh_popod_restore(); // enable frequency hopping (main PLL)
            
            PTP_INIT_02();    
        }
        else if( (temp_ptpen & 0x7) == 0x5 ) // init 2 =============================
        {
            // read PTP_VO_0 ~ PTP_VO_3
            temp = ptp_read( PTP_VOP30 );
            PTP_VO_0 = temp & 0xff;
            PTP_VO_1 = (temp>>8) & 0xff;
            PTP_VO_2 = (temp>>16) & 0xff;
            PTP_VO_3 = (temp>>24) & 0xff;
            
            // read PTP_VO_3 ~ PTP_VO_7
            temp = ptp_read( PTP_VOP74 );
            PTP_VO_4 = temp & 0xff;
            PTP_VO_5 = (temp>>8) & 0xff;
            PTP_VO_6 = (temp>>16) & 0xff;
            PTP_VO_7 = (temp>>24) & 0xff;       

            // save init2 PTP_init2_VO_0 ~ PTP_VO_7
            PTP_init2_VO_0 = PTP_VO_0;
            PTP_init2_VO_1 = PTP_VO_1;
            PTP_init2_VO_2 = PTP_VO_2;
            PTP_init2_VO_3 = PTP_VO_3;
            PTP_init2_VO_4 = PTP_VO_4;
            PTP_init2_VO_5 = PTP_VO_5;
            PTP_init2_VO_6 = PTP_VO_6;
            PTP_init2_VO_7 = PTP_VO_7;
            
            // show PTP_VO_0 ~ PTP_VO_7 to PMIC
            clc_isr_info("===============ISR ISR ISR ISR INIT2=============\n");
            clc_isr_info("PTP_VO_0 = 0x%x\n", PTP_VO_0);
            clc_isr_info("PTP_VO_1 = 0x%x\n", PTP_VO_1);
            clc_isr_info("PTP_VO_2 = 0x%x\n", PTP_VO_2);
            clc_isr_info("PTP_VO_3 = 0x%x\n", PTP_VO_3);
            clc_isr_info("PTP_VO_4 = 0x%x\n", PTP_VO_4);
            clc_isr_info("PTP_VO_5 = 0x%x\n", PTP_VO_5);
            clc_isr_info("PTP_VO_6 = 0x%x\n", PTP_VO_6);
            clc_isr_info("PTP_VO_7 = 0x%x\n", PTP_VO_7);        
            clc_isr_info("ptp_level = 0x%x\n", ptp_level);        
            clc_isr_info("================================================\n");

            PTP_set_ptp_volt();      

            if( PTP_output_voltage_failed == 1 )
            {
                ptp_write(PTP_PTPEN, 0x0);  // disable PTP
                ptp_write(PTP_PTPINTSTS, 0x00ffffff);  // Clear PTP interrupt PTPINTSTS
                PTP_INIT_FLAG = 0;
                clc_isr_info("disable PTP : PTP_output_voltage_failed(init_2).\n");
                return IRQ_HANDLED;
            }
            
            PTP_INIT_FLAG = 1;

            // Set PTPEN.PTPINITEN/PTPEN.PTPINIT2EN = 0x0 & Clear PTP INIT interrupt PTPINTSTS = 0x00000001
            ptp_write(PTP_PTPEN, 0x0);
            ptp_write(PTP_PTPINTSTS, 0x1);
            //tasklet_schedule(&ptp_do_mon_tasklet);
            PTP_MON_MODE();    
        }
        else // error : init1 or init2 , but enable setting is wrong.
        {
            clc_isr_info("====================================================\n");
            clc_isr_info("PTP error_0 (0x%x) : PTPINTSTS = 0x%x\n", temp_ptpen, PTPINTSTS);
            clc_isr_info("PTP_SMSTATE0 (0x%x) = 0x%x\n", PTP_SMSTATE0, ptp_read(PTP_SMSTATE0) );
            clc_isr_info("PTP_SMSTATE1 (0x%x) = 0x%x\n", PTP_SMSTATE1, ptp_read(PTP_SMSTATE1) );
            clc_isr_info("====================================================\n");
            
            // disable PTP
            ptp_write(PTP_PTPEN, 0x0);
            
            // Clear PTP interrupt PTPINTSTS
            ptp_write(PTP_PTPINTSTS, 0x00ffffff);

            // restore default DVFS table (PMIC)
            mt_cpufreq_return_default_DVS_by_ptpod();

            PTP_INIT_FLAG = 0;
        }
    }
    else if( (PTPINTSTS & 0x00ff0000) != 0x0 )  // PTP Monitor mode
    {
        // check if thermal sensor init completed?
        temp_0 = (ptp_read( PTP_TEMP ) & 0xff); // temp_0
        clc_isr_info("thermal sensor temp_0 = 0x%x\n", temp_0);   
        
        if( (temp_0 > 0x4b) && (temp_0 < 0xd3) )
        {
            clc_isr_info("thermal sensor init has not been completed. (temp_0 = 0x%x)\n", temp_0);            
        }
        else
        {
            // read PTP_VO_0 ~ PTP_VO_3
            temp = ptp_read( PTP_VOP30 );
            PTP_VO_0 = temp & 0xff;
            PTP_VO_1 = (temp>>8) & 0xff;
            PTP_VO_2 = (temp>>16) & 0xff;
            PTP_VO_3 = (temp>>24) & 0xff;
            
            // read PTP_VO_3 ~ PTP_VO_7
            temp = ptp_read( PTP_VOP74 );
            PTP_VO_4 = temp & 0xff;
            PTP_VO_5 = (temp>>8) & 0xff;
            PTP_VO_6 = (temp>>16) & 0xff;
            PTP_VO_7 = (temp>>24) & 0xff;
            
            // show PTP_VO_0 ~ PTP_VO_7 to PMIC
            clc_isr_info("===============ISR ISR ISR ISR Monitor mode=============\n");
            clc_isr_info("PTP_VO_0 = 0x%x\n", PTP_VO_0);
            clc_isr_info("PTP_VO_1 = 0x%x\n", PTP_VO_1);
            clc_isr_info("PTP_VO_2 = 0x%x\n", PTP_VO_2);
            clc_isr_info("PTP_VO_3 = 0x%x\n", PTP_VO_3);
            clc_isr_info("PTP_VO_4 = 0x%x\n", PTP_VO_4);
            clc_isr_info("PTP_VO_5 = 0x%x\n", PTP_VO_5);
            clc_isr_info("PTP_VO_6 = 0x%x\n", PTP_VO_6);
            clc_isr_info("PTP_VO_7 = 0x%x\n", PTP_VO_7);        
            clc_isr_info("ptp_level = 0x%x\n", ptp_level);        
            clc_isr_info("ISR : TEMPSPARE1 = 0x%x\n", ptp_read(TEMPSPARE1));        
            clc_isr_info("=============================================\n");

            PTP_set_ptp_volt();
            
            if( PTP_output_voltage_failed == 1 )
            {
                ptp_write(PTP_PTPEN, 0x0);  // disable PTP
                ptp_write(PTP_PTPINTSTS, 0x00ffffff);  // Clear PTP interrupt PTPINTSTS
                PTP_INIT_FLAG = 0;
                clc_isr_info("disable PTP : PTP_output_voltage_failed(Mon).\n");
                return IRQ_HANDLED;
            }
            
        }
        
        // Clear PTP INIT interrupt PTPINTSTS = 0x00ff0000
        ptp_write(PTP_PTPINTSTS, 0x00ff0000);
    }
    else // PTP error
    {
        if( ((temp_ptpen & 0x7) == 0x1) || ((temp_ptpen & 0x7) == 0x5) )    // init 1  || init 2 error handler ======================
        {
            clc_isr_info("====================================================\n");
            clc_isr_info("PTP error_1 error_2 (0x%x) : PTPINTSTS = 0x%x\n", temp_ptpen, PTPINTSTS);
            clc_isr_info("PTP_SMSTATE0 (0x%x) = 0x%x\n", PTP_SMSTATE0, ptp_read(PTP_SMSTATE0) );
            clc_isr_info("PTP_SMSTATE1 (0x%x) = 0x%x\n", PTP_SMSTATE1, ptp_read(PTP_SMSTATE1) );
            clc_isr_info("====================================================\n");
                        
            // disable PTP
            ptp_write(PTP_PTPEN, 0x0);
            
            // Clear PTP interrupt PTPINTSTS
            ptp_write(PTP_PTPINTSTS, 0x00ffffff);

            // restore default DVFS table (PMIC)
            mt_cpufreq_return_default_DVS_by_ptpod();
            
            PTP_INIT_FLAG = 0;
        }
        else    // PTP Monitor mode error handler ======================
        {
            clc_isr_info("====================================================\n");
            clc_isr_info("PTP error_m (0x%x) : PTPINTSTS = 0x%x\n", temp_ptpen, PTPINTSTS);
            clc_isr_info("PTP_SMSTATE0 (0x%x) = 0x%x\n", PTP_SMSTATE0, ptp_read(PTP_SMSTATE0) );
            clc_isr_info("PTP_SMSTATE1 (0x%x) = 0x%x\n", PTP_SMSTATE1, ptp_read(PTP_SMSTATE1) );
            clc_isr_info("PTP_TEMP (0x%x) = 0x%x\n", PTP_TEMP, ptp_read(PTP_TEMP) );
            
            clc_isr_info("PTP_TEMPMSR0 (0x%x) = 0x%x\n", PTP_TEMPMSR0, ptp_read(PTP_TEMPMSR0) );
            clc_isr_info("PTP_TEMPMSR1 (0x%x) = 0x%x\n", PTP_TEMPMSR1, ptp_read(PTP_TEMPMSR1) );
            clc_isr_info("PTP_TEMPMSR2 (0x%x) = 0x%x\n", PTP_TEMPMSR2, ptp_read(PTP_TEMPMSR2) );
            clc_isr_info("PTP_TEMPMONCTL0 (0x%x) = 0x%x\n", PTP_TEMPMONCTL0, ptp_read(PTP_TEMPMONCTL0) );
            clc_isr_info("PTP_TEMPMSRCTL1 (0x%x) = 0x%x\n", PTP_TEMPMSRCTL1, ptp_read(PTP_TEMPMSRCTL1) );
            clc_isr_info("====================================================\n");
            
            // disable PTP
            ptp_write(PTP_PTPEN, 0x0);
            
            // Clear PTP interrupt PTPINTSTS
            ptp_write(PTP_PTPINTSTS, 0x00ffffff);
            
            #if 0
            // set init2 value to DVFS table (PMIC)
            PTP_VO_0 = PTP_init2_VO_0;
            PTP_VO_1 = PTP_init2_VO_1;
            PTP_VO_2 = PTP_init2_VO_2;
            PTP_VO_3 = PTP_init2_VO_3;
            PTP_VO_4 = PTP_init2_VO_4;
            PTP_VO_5 = PTP_init2_VO_5;
            PTP_VO_6 = PTP_init2_VO_6;
            PTP_VO_7 = PTP_init2_VO_7;
            PTP_set_ptp_volt();
            #else
            // restore default DVFS table (PMIC)
            mt_cpufreq_return_default_DVS_by_ptpod();
            #endif
        }
      
    }
    
    return IRQ_HANDLED;
}

static void PTP_Initialization_01(PTP_Init_T* PTP_Init_val)
{

    // config PTP register =================================================================
    ptp_write(PTP_DESCHAR, ((((PTP_Init_val->BDES)<<8) & 0xff00)  | ((PTP_Init_val->MDES) & 0xff)));
    ptp_write(PTP_TEMPCHAR, ((((PTP_Init_val->VCO)<<16) & 0xff0000) | (((PTP_Init_val->MTDES)<<8) & 0xff00)  | ((PTP_Init_val->DVTFIXED) & 0xff)));
    ptp_write(PTP_DETCHAR, ((((PTP_Init_val->DCBDET)<<8) & 0xff00)  | ((PTP_Init_val->DCMDET) & 0xff)));
    ptp_write(PTP_AGECHAR, ((((PTP_Init_val->AGEDELTA)<<8) & 0xff00)  | ((PTP_Init_val->AGEM) & 0xff)));
    ptp_write(PTP_DCCONFIG, ((PTP_Init_val->DCCONFIG)));
    ptp_write(PTP_AGECONFIG, ((PTP_Init_val->AGECONFIG)));

    if( PTP_Init_val->AGEM == 0x0 )
    {
        ptp_write(PTP_RUNCONFIG, 0x80000000);
    }
    else
    {
        u32 temp_i, temp_filter, temp_value;
        
        temp_value = 0x0;
        
        for (temp_i = 0 ; temp_i < 24 ; temp_i += 2 )
        {
            temp_filter = 0x3 << temp_i;
            	
            if( ((PTP_Init_val->AGECONFIG) & temp_filter) == 0x0 )
            {
                temp_value |= (0x1 << temp_i);
            }
            else
            {
                temp_value |= ((PTP_Init_val->AGECONFIG) & temp_filter);
            }
        }
        ptp_write(PTP_RUNCONFIG, temp_value);
    }

    ptp_write(PTP_FREQPCT30, ((((PTP_Init_val->FREQPCT3)<<24)&0xff000000) | (((PTP_Init_val->FREQPCT2)<<16)&0xff0000) | (((PTP_Init_val->FREQPCT1)<<8)&0xff00)  | ((PTP_Init_val->FREQPCT0) & 0xff)));
    ptp_write(PTP_FREQPCT74, ((((PTP_Init_val->FREQPCT7)<<24)&0xff000000) | (((PTP_Init_val->FREQPCT6)<<16)&0xff0000) | (((PTP_Init_val->FREQPCT5)<<8)&0xff00)  | ((PTP_Init_val->FREQPCT4) & 0xff)));

    ptp_write(PTP_LIMITVALS, ((((PTP_Init_val->VMAX)<<24)&0xff000000) | (((PTP_Init_val->VMIN)<<16)&0xff0000) | (((PTP_Init_val->DTHI)<<8)&0xff00)  | ((PTP_Init_val->DTLO) & 0xff)));
    ptp_write(PTP_VBOOT, (((PTP_Init_val->VBOOT)&0xff)));
    ptp_write(PTP_DETWINDOW, (((PTP_Init_val->DETWINDOW)&0xffff)));
    ptp_write(PTP_PTPCONFIG, (((PTP_Init_val->DETMAX)&0xffff)));

    // clear all pending PTP interrupt & config PTPINTEN =================================================================
    ptp_write(PTP_PTPINTSTS, 0xffffffff);
    ptp_write(PTP_PTPINTEN, 0x00005f01);

    // enable PTP INIT measurement =================================================================
    ptp_write(PTP_PTPEN, 0x00000001);
    
}


static void PTP_Initialization_02(PTP_Init_T* PTP_Init_val)
{

    // config PTP register =================================================================
    ptp_write(PTP_DESCHAR, ((((PTP_Init_val->BDES)<<8) & 0xff00)  | ((PTP_Init_val->MDES) & 0xff)));
    ptp_write(PTP_TEMPCHAR, ((((PTP_Init_val->VCO)<<16) & 0xff0000) | (((PTP_Init_val->MTDES)<<8) & 0xff00)  | ((PTP_Init_val->DVTFIXED) & 0xff)));
    ptp_write(PTP_DETCHAR, ((((PTP_Init_val->DCBDET)<<8) & 0xff00)  | ((PTP_Init_val->DCMDET) & 0xff)));
    ptp_write(PTP_AGECHAR, ((((PTP_Init_val->AGEDELTA)<<8) & 0xff00)  | ((PTP_Init_val->AGEM) & 0xff)));
    ptp_write(PTP_DCCONFIG, ((PTP_Init_val->DCCONFIG)));
    ptp_write(PTP_AGECONFIG, ((PTP_Init_val->AGECONFIG)));

    if( PTP_Init_val->AGEM == 0x0 )
    {
        ptp_write(PTP_RUNCONFIG, 0x80000000);
    }
    else
    {
        u32 temp_i, temp_filter, temp_value;
        
        temp_value = 0x0;
        
        for (temp_i = 0 ; temp_i < 24 ; temp_i += 2 )
        {
            temp_filter = 0x3 << temp_i;
            	
            if( ((PTP_Init_val->AGECONFIG) & temp_filter) == 0x0 )
            {
                temp_value |= (0x1 << temp_i);
            }
            else
            {
                temp_value |= ((PTP_Init_val->AGECONFIG) & temp_filter);
            }
        }
        
        ptp_write(PTP_RUNCONFIG, temp_value);
    }

    ptp_write(PTP_FREQPCT30, ((((PTP_Init_val->FREQPCT3)<<24)&0xff000000) | (((PTP_Init_val->FREQPCT2)<<16)&0xff0000) | (((PTP_Init_val->FREQPCT1)<<8)&0xff00)  | ((PTP_Init_val->FREQPCT0) & 0xff)));
    ptp_write(PTP_FREQPCT74, ((((PTP_Init_val->FREQPCT7)<<24)&0xff000000) | (((PTP_Init_val->FREQPCT6)<<16)&0xff0000) | (((PTP_Init_val->FREQPCT5)<<8)&0xff00)  | ((PTP_Init_val->FREQPCT4) & 0xff)));

    ptp_write(PTP_LIMITVALS, ((((PTP_Init_val->VMAX)<<24)&0xff000000) | (((PTP_Init_val->VMIN)<<16)&0xff0000) | (((PTP_Init_val->DTHI)<<8)&0xff00)  | ((PTP_Init_val->DTLO) & 0xff)));
    ptp_write(PTP_VBOOT, (((PTP_Init_val->VBOOT)&0xff)));
    ptp_write(PTP_DETWINDOW, (((PTP_Init_val->DETWINDOW)&0xffff)));
    ptp_write(PTP_PTPCONFIG, (((PTP_Init_val->DETMAX)&0xffff)));

    // clear all pending PTP interrupt & config PTPINTEN =================================================================
    ptp_write(PTP_PTPINTSTS, 0xffffffff);
    ptp_write(PTP_PTPINTEN, 0x00005f01);
    ptp_write(PTP_INIT2VALS, ((((PTP_Init_val->AGEVOFFSETIN)<<16) & 0xffff0000) | ((PTP_Init_val->DCVOFFSETIN) & 0xffff)));

    // enable PTP INIT measurement =================================================================
    ptp_write(PTP_PTPEN, 0x00000005);
    
}

static void PTP_CTP_Config_Temp_Reg(void)
{
    #if 0
    // configure temperature measurement polling times
    ptp_write(PTP_TEMPMONCTL1, 0x0);  // set the PERIOD_UNIT in number of cycles of hclk_ck cycles
    ptp_write(PTP_TEMPMONCTL2, 0x0);  // set the set the interval in number of PERIOD_UNIT between each sample of a sensing point
    ptp_write(PTP_TEMPAHBPOLL, 0x0);  // set the number of PERIOD_UNIT between ADC polling on the AHB bus
    ptp_write(PTP_TEMPAHBTO, 0x1998);  // set the number of PERIOD_UNIT for AHB polling the temperature voltage from AUXADC without getting any response

    ptp_write(PTP_TEMPMONIDET0, 0x2aa);  
    ptp_write(PTP_TEMPMONIDET1, 0x2aa); 
    ptp_write(PTP_TEMPMONIDET2, 0x2aa);  
    ptp_write(PTP_TEMPMONIDET3, 0x2aa); 

    ptp_write(PTP_TEMPH2NTHRE, 0xff); 
    ptp_write(PTP_TEMPHTHRE, 0xaa); 
    ptp_write(PTP_TEMPCTHRE, 0x133); 
    ptp_write(PTP_TEMPOFFSETH, 0xbb); 
    ptp_write(PTP_TEMPOFFSETL, 0x111); 
    
    // configure temperature measurement filter setting
    ptp_write(PTP_TEMPMSRCTL0, 0x6db);

    // configure temperature measurement addresses
    ptp_write(PTP_TEMPADCMUXADDR, PTP_TEMPSPARE1); // Addr for AUXADC mux select
    ptp_write(PTP_TEMPADCENADDR, PTP_TEMPSPARE1); // Addr for AUXADC enable
    ptp_write(PTP_TEMPPNPMUXADDR, PTP_TEMPSPARE1); // Addr for PNP sensor mux select
    ptp_write(PTP_TEMPADCVOLTADDR, PTP_TEMPSPARE1); // Addr for AUXADC voltage output
    
    ptp_write(PTP_TEMPADCPNP0, 0x0); // MUX select value for PNP0
    ptp_write(PTP_TEMPADCPNP1, 0x1); // MUX select value for PNP1
    ptp_write(PTP_TEMPADCPNP2, 0x2); // MUX select value for PNP2
    ptp_write(PTP_TEMPADCPNP3, 0x3); // MUX select value for PNP3

    ptp_write(PTP_TEMPADCMUX, 0x800); // AHB value for AUXADC mux select
    ptp_write(PTP_TEMPADCEXT, 0x22);
    ptp_write(PTP_TEMPADCEXT1, 0x33);
    ptp_write(PTP_TEMPADCEN, 0x800);
    
    ptp_write(PTP_TEMPPNPMUXADDR, PTP_TEMPSPARE0);
    ptp_write(PTP_TEMPADCMUXADDR, PTP_TEMPSPARE0);
    ptp_write(PTP_TEMPADCEXTADDR, PTP_TEMPSPARE0);
    ptp_write(PTP_TEMPADCEXT1ADDR, PTP_TEMPSPARE0);
    ptp_write(PTP_TEMPADCENADDR, PTP_TEMPSPARE0);
    ptp_write(PTP_TEMPADCVALIDADDR, PTP_TEMPSPARE0);
    ptp_write(PTP_TEMPADCVOLTADDR, PTP_TEMPSPARE0);
    
    ptp_write(PTP_TEMPRDCTRL, 0x0);
    ptp_write(PTP_TEMPADCVALIDMASK, 0x2c);
    ptp_write(PTP_TEMPADCVOLTAGESHIFT, 0x0);
    ptp_write(PTP_TEMPADCWRITECTRL, 0x2);    

    ptp_write(PTP_TEMPMONINT, 0x1fffffff);
    ptp_write(PTP_TEMPMONCTL0, 0xf);    
    #endif
}

static void PTP_Monitor_Mode(PTP_Init_T* PTP_Init_val)
{

    // config PTP register =================================================================
    ptp_write(PTP_DESCHAR, ((((PTP_Init_val->BDES)<<8) & 0xff00)  | ((PTP_Init_val->MDES) & 0xff)));
    ptp_write(PTP_TEMPCHAR, ((((PTP_Init_val->VCO)<<16) & 0xff0000) | (((PTP_Init_val->MTDES)<<8) & 0xff00)  | ((PTP_Init_val->DVTFIXED) & 0xff)));
    ptp_write(PTP_DETCHAR, ((((PTP_Init_val->DCBDET)<<8) & 0xff00)  | ((PTP_Init_val->DCMDET) & 0xff)));
    ptp_write(PTP_AGECHAR, ((((PTP_Init_val->AGEDELTA)<<8) & 0xff00)  | ((PTP_Init_val->AGEM) & 0xff)));
    ptp_write(PTP_DCCONFIG, ((PTP_Init_val->DCCONFIG)));
    ptp_write(PTP_AGECONFIG, ((PTP_Init_val->AGECONFIG)));
    ptp_write(PTP_TSCALCS, ((((PTP_Init_val->BTS)<<12) & 0xfff000)  | ((PTP_Init_val->MTS) & 0xfff)));

    if( PTP_Init_val->AGEM == 0x0 )
    {
        ptp_write(PTP_RUNCONFIG, 0x80000000);
    }
    else
    {
        u32 temp_i, temp_filter, temp_value;
        
        temp_value = 0x0;
        
        for (temp_i = 0 ; temp_i < 24 ; temp_i += 2 )
        {
            temp_filter = 0x3 << temp_i;
            	
            if( ((PTP_Init_val->AGECONFIG) & temp_filter) == 0x0 )
            {
                temp_value |= (0x1 << temp_i);
            }
            else
            {
                temp_value |= ((PTP_Init_val->AGECONFIG) & temp_filter);
            }
        }
        
        ptp_write(PTP_RUNCONFIG, temp_value);
    }

    ptp_write(PTP_FREQPCT30, ((((PTP_Init_val->FREQPCT3)<<24)&0xff000000) | (((PTP_Init_val->FREQPCT2)<<16)&0xff0000) | (((PTP_Init_val->FREQPCT1)<<8)&0xff00)  | ((PTP_Init_val->FREQPCT0) & 0xff)));
    ptp_write(PTP_FREQPCT74, ((((PTP_Init_val->FREQPCT7)<<24)&0xff000000) | (((PTP_Init_val->FREQPCT6)<<16)&0xff0000) | (((PTP_Init_val->FREQPCT5)<<8)&0xff00)  | ((PTP_Init_val->FREQPCT4) & 0xff)));

    ptp_write(PTP_LIMITVALS, ((((PTP_Init_val->VMAX)<<24)&0xff000000) | (((PTP_Init_val->VMIN)<<16)&0xff0000) | (((PTP_Init_val->DTHI)<<8)&0xff00)  | ((PTP_Init_val->DTLO) & 0xff)));
    ptp_write(PTP_VBOOT, (((PTP_Init_val->VBOOT)&0xff)));
    ptp_write(PTP_DETWINDOW, (((PTP_Init_val->DETWINDOW)&0xffff)));
    ptp_write(PTP_PTPCONFIG, (((PTP_Init_val->DETMAX)&0xffff)));

    // clear all pending PTP interrupt & config PTPINTEN =================================================================
    ptp_write(PTP_PTPINTSTS, 0xffffffff);
    
    if (ptp_level < 1 || ptp_level > 3) // non-turbo no PTPOD
    {
        ptp_write(PTP_PTPINTEN, 0x00FFA002);
    }
    else
    {
        ptp_write(PTP_PTPINTEN, 0x00FF0000);
    }

    // enable PTP monitor mode =================================================================
    ptp_write(PTP_PTPEN, 0x00000002);
    
}

static void init_PTP_interrupt(void)
{
        int r;
        
        // Set PTP IRQ =========================================
        r = request_irq(MT_PTP_FSM_IRQ_ID, MT6589_PTP_ISR, IRQF_TRIGGER_LOW, "PTP", NULL);
        if (r) 
        {
            clc_notice("PTP IRQ register failed (%d)\n", r);
            WARN_ON(1);
        }
        
        clc_notice("~~~ CLC : Set PTP IRQ OK.\n");
}

/*
 * PTP_INIT_TEST_01: PTP_INIT_TEST_01.
 * @psInput:
 * @psOutput:
 * Return CTP status code.
 */
u32 PTP_INIT_01(void)
{
    PTP_Init_T PTP_Init_value;

    ptp_data[0] = 0xffffffff;
    
    clc_notice("~~~ CLC : PTP_INIT_01() start (ptp_level = 0x%x).\n", ptp_level);

    #if PTP_Get_Real_Val
        val_0 = get_devinfo_with_index(16);
        val_1 = get_devinfo_with_index(17);
        val_2 = get_devinfo_with_index(18);
        val_3 = get_devinfo_with_index(19);
    #endif

    // PTP test code ================================
    PTP_Init_value.PTPINITEN = (val_0) & 0x1;
    PTP_Init_value.PTPMONEN = (val_0 >> 1) & 0x1;
    PTP_Init_value.MDES = (val_0 >> 8) & 0xff;
    PTP_Init_value.BDES = (val_0 >> 16) & 0xff;
    PTP_Init_value.DCMDET = (val_0 >> 24) & 0xff;
    
    PTP_Init_value.DCCONFIG = (val_1) & 0xffffff;
    PTP_Init_value.DCBDET = (val_1 >> 24) & 0xff;
    
    PTP_Init_value.AGECONFIG = (val_2) & 0xffffff;
    PTP_Init_value.AGEM = (val_2 >> 24) & 0xff;
    
    //PTP_Init_value.AGEDELTA = (val_3) & 0xff;
    PTP_Init_value.AGEDELTA = 0x88;    
    PTP_Init_value.DVTFIXED = (val_3 >> 8) & 0xff;
    PTP_Init_value.MTDES = (val_3 >> 16) & 0xff;
    PTP_Init_value.VCO = (val_3 >> 24) & 0xff;
    
    PTP_Init_value.FREQPCT0 = freq_0;
    PTP_Init_value.FREQPCT1 = freq_1;
    PTP_Init_value.FREQPCT2 = freq_2;
    PTP_Init_value.FREQPCT3 = freq_3;
    PTP_Init_value.FREQPCT4 = freq_4;
    PTP_Init_value.FREQPCT5 = freq_5;
    PTP_Init_value.FREQPCT6 = freq_6;
    PTP_Init_value.FREQPCT7 = freq_7;
    
    PTP_Init_value.DETWINDOW = 0xa28;  // 100 us, This is the PTP Detector sampling time as represented in cycles of bclk_ck during INIT. 52 MHz
    PTP_Init_value.VMAX = 0x5D; // 1.28125v (700mv + n * 6.25mv)
    PTP_Init_value.VMIN = 0x28; // 0.95v (700mv + n * 6.25mv)    
    PTP_Init_value.DTHI = 0x01; // positive
    PTP_Init_value.DTLO = 0xfe; // negative (2・s compliment)
    PTP_Init_value.VBOOT = 0x48; // 115v  (700mv + n * 6.25mv)    
    PTP_Init_value.DETMAX = 0xffff; // This timeout value is in cycles of bclk_ck.
    
    if(PTP_Init_value.PTPINITEN == 0x0)
    {
        clc_notice("~~~ CLC : PTPINITEN = 0x%x \n", PTP_Init_value.PTPINITEN);
        return 1;
    }    
    
    // start PTP_INIT_1 =========================================
    clc_notice("===================11111111111111===============\n");
    clc_notice("PTPINITEN = 0x%x\n", PTP_Init_value.PTPINITEN);
    clc_notice("PTPMONEN = 0x%x\n", PTP_Init_value.PTPMONEN);
    clc_notice("MDES = 0x%x\n", PTP_Init_value.MDES);
    clc_notice("BDES = 0x%x\n", PTP_Init_value.BDES);
    clc_notice("DCMDET = 0x%x\n", PTP_Init_value.DCMDET);

    clc_notice("DCCONFIG = 0x%x\n", PTP_Init_value.DCCONFIG);
    clc_notice("DCBDET = 0x%x\n", PTP_Init_value.DCBDET);
    
    clc_notice("AGECONFIG = 0x%x\n", PTP_Init_value.AGECONFIG);
    clc_notice("AGEM = 0x%x\n", PTP_Init_value.AGEM);

    clc_notice("AGEDELTA = 0x%x\n", PTP_Init_value.AGEDELTA);
    clc_notice("DVTFIXED = 0x%x\n", PTP_Init_value.DVTFIXED);
    clc_notice("MTDES = 0x%x\n", PTP_Init_value.MTDES);
    clc_notice("VCO = 0x%x\n", PTP_Init_value.VCO);

    clc_notice("FREQPCT0 = %d\n", PTP_Init_value.FREQPCT0);
    clc_notice("FREQPCT1 = %d\n", PTP_Init_value.FREQPCT1);
    clc_notice("FREQPCT2 = %d\n", PTP_Init_value.FREQPCT2);
    clc_notice("FREQPCT3 = %d\n", PTP_Init_value.FREQPCT3);
    clc_notice("FREQPCT4 = %d\n", PTP_Init_value.FREQPCT4);
    clc_notice("FREQPCT5 = %d\n", PTP_Init_value.FREQPCT5);
    clc_notice("FREQPCT6 = %d\n", PTP_Init_value.FREQPCT6);
    clc_notice("FREQPCT7 = %d\n", PTP_Init_value.FREQPCT7);

    mt_fh_popod_save();  // disable frequency hopping (main PLL)
    mt_cpufreq_disable_by_ptpod();  // disable DVFS and set vproc = 1.15v (1 GHz)
    
    PTP_Initialization_01( &PTP_Init_value );
    
    return 0;
}

/*
 * PTP_INIT_TEST_02: PTP_INIT_TEST_02.
 * @psInput:
 * @psOutput:
 * Return CTP status code.
 */
u32 PTP_INIT_02(void)
{
    PTP_Init_T PTP_Init_value;

    ptp_data[0] = 0xffffffff;
    
    clc_notice("~~~ CLC : PTP_INIT_02() start (ptp_level = 0x%x).\n", ptp_level);

    #if PTP_Get_Real_Val
        val_0 = get_devinfo_with_index(16);
        val_1 = get_devinfo_with_index(17);
        val_2 = get_devinfo_with_index(18);
        val_3 = get_devinfo_with_index(19);
    #endif

    // PTP test code ================================
    PTP_Init_value.PTPINITEN = (val_0) & 0x1;
    PTP_Init_value.PTPMONEN = (val_0 >> 1) & 0x1;
    PTP_Init_value.MDES = (val_0 >> 8) & 0xff;
    PTP_Init_value.BDES = (val_0 >> 16) & 0xff;
    PTP_Init_value.DCMDET = (val_0 >> 24) & 0xff;
    
    PTP_Init_value.DCCONFIG = (val_1) & 0xffffff;
    PTP_Init_value.DCBDET = (val_1 >> 24) & 0xff;
    
    PTP_Init_value.AGECONFIG = (val_2) & 0xffffff;
    PTP_Init_value.AGEM = (val_2 >> 24) & 0xff;
    
    //PTP_Init_value.AGEDELTA = (val_3) & 0xff;
    PTP_Init_value.AGEDELTA = 0x88;    
    PTP_Init_value.DVTFIXED = (val_3 >> 8) & 0xff;
    PTP_Init_value.MTDES = (val_3 >> 16) & 0xff;
    PTP_Init_value.VCO = (val_3 >> 24) & 0xff;

    PTP_Init_value.FREQPCT0 = freq_0;
    PTP_Init_value.FREQPCT1 = freq_1;
    PTP_Init_value.FREQPCT2 = freq_2;
    PTP_Init_value.FREQPCT3 = freq_3;
    PTP_Init_value.FREQPCT4 = freq_4;
    PTP_Init_value.FREQPCT5 = freq_5;
    PTP_Init_value.FREQPCT6 = freq_6;
    PTP_Init_value.FREQPCT7 = freq_7;

    PTP_Init_value.DETWINDOW = 0xa28;  // 100 us, This is the PTP Detector sampling time as represented in cycles of bclk_ck during INIT. 52 MHz
    PTP_Init_value.VMAX = 0x5D; // 1.28125v (700mv + n * 6.25mv)    
    PTP_Init_value.VMIN = 0x28; // 0.95v (700mv + n * 6.25mv)    
    PTP_Init_value.DTHI = 0x01; // positive
    PTP_Init_value.DTLO = 0xfe; // negative (2・s compliment)
    PTP_Init_value.VBOOT = 0x48; // 115v  (700mv + n * 6.25mv)    
    PTP_Init_value.DETMAX = 0xffff; // This timeout value is in cycles of bclk_ck.

    PTP_Init_value.DCVOFFSETIN= PTP_DCVOFFSET;
    PTP_Init_value.AGEVOFFSETIN= PTP_AGEVOFFSET;

    clc_notice("~~~ CLC : DCVOFFSETIN = 0x%x \n", PTP_Init_value.DCVOFFSETIN);
    clc_notice("~~~ CLC : AGEVOFFSETIN = 0x%x \n", PTP_Init_value.AGEVOFFSETIN);

    if(PTP_Init_value.PTPINITEN == 0x0)
    {
        clc_notice("~~~ CLC : PTPINITEN = 0x%x \n", PTP_Init_value.PTPINITEN);
        return 1;
    }

    // start PTP_INIT_2 =========================================
    clc_isr_info("===================22222222222222===============\n");
    clc_isr_info("PTPINITEN = 0x%x\n", PTP_Init_value.PTPINITEN);
    clc_isr_info("PTPMONEN = 0x%x\n", PTP_Init_value.PTPMONEN);
    clc_isr_info("MDES = 0x%x\n", PTP_Init_value.MDES);
    clc_isr_info("BDES = 0x%x\n", PTP_Init_value.BDES);
    clc_isr_info("DCMDET = 0x%x\n", PTP_Init_value.DCMDET);

    clc_isr_info("DCCONFIG = 0x%x\n", PTP_Init_value.DCCONFIG);
    clc_isr_info("DCBDET = 0x%x\n", PTP_Init_value.DCBDET);
    
    clc_isr_info("AGECONFIG = 0x%x\n", PTP_Init_value.AGECONFIG);
    clc_isr_info("AGEM = 0x%x\n", PTP_Init_value.AGEM);

    clc_isr_info("AGEDELTA = 0x%x\n", PTP_Init_value.AGEDELTA);
    clc_isr_info("DVTFIXED = 0x%x\n", PTP_Init_value.DVTFIXED);
    clc_isr_info("MTDES = 0x%x\n", PTP_Init_value.MTDES);
    clc_isr_info("VCO = 0x%x\n", PTP_Init_value.VCO);
    
    clc_isr_info("DCVOFFSETIN = 0x%x\n", PTP_Init_value.DCVOFFSETIN);
    clc_isr_info("AGEVOFFSETIN = 0x%x\n", PTP_Init_value.AGEVOFFSETIN);

    clc_isr_info("FREQPCT0 = %d\n", PTP_Init_value.FREQPCT0);
    clc_isr_info("FREQPCT1 = %d\n", PTP_Init_value.FREQPCT1);
    clc_isr_info("FREQPCT2 = %d\n", PTP_Init_value.FREQPCT2);
    clc_isr_info("FREQPCT3 = %d\n", PTP_Init_value.FREQPCT3);
    clc_isr_info("FREQPCT4 = %d\n", PTP_Init_value.FREQPCT4);
    clc_isr_info("FREQPCT5 = %d\n", PTP_Init_value.FREQPCT5);
    clc_isr_info("FREQPCT6 = %d\n", PTP_Init_value.FREQPCT6);
    clc_isr_info("FREQPCT7 = %d\n", PTP_Init_value.FREQPCT7);
    
    PTP_Initialization_02( &PTP_Init_value );

    return 0;
}

/*
 * PTP_MON_MODE_TEST: PTP_MON_MODE_TEST.
 * @psInput:
 * @psOutput:
 * Return CTP status code.
 */
u32 PTP_MON_MODE(void)
{
    PTP_Init_T PTP_Init_value;
    struct TS_PTPOD ts_info;
    
    clc_notice("~~~ CLC : PTP_MON_MODE() start (ptp_level = 0x%x).\n", ptp_level);

    #if PTP_Get_Real_Val
        val_0 = get_devinfo_with_index(16);
        val_1 = get_devinfo_with_index(17);
        val_2 = get_devinfo_with_index(18);
        val_3 = get_devinfo_with_index(19);
    #endif

    // PTP test code ================================
    PTP_Init_value.PTPINITEN = (val_0) & 0x1;
    PTP_Init_value.PTPMONEN = (val_0 >> 1) & 0x1;
    PTP_Init_value.ADC_CALI_EN = (val_0 >> 2) & 0x1;
    PTP_Init_value.MDES = (val_0 >> 8) & 0xff;
    PTP_Init_value.BDES = (val_0 >> 16) & 0xff;
    PTP_Init_value.DCMDET = (val_0 >> 24) & 0xff;
    
    PTP_Init_value.DCCONFIG = (val_1) & 0xffffff;
    PTP_Init_value.DCBDET = (val_1 >> 24) & 0xff;
    
    PTP_Init_value.AGECONFIG = (val_2) & 0xffffff;
    PTP_Init_value.AGEM = (val_2 >> 24) & 0xff;
    
    //PTP_Init_value.AGEDELTA = (val_3) & 0xff;
    PTP_Init_value.AGEDELTA = 0x88;    
    PTP_Init_value.DVTFIXED = (val_3 >> 8) & 0xff;
    PTP_Init_value.MTDES = (val_3 >> 16) & 0xff;
    PTP_Init_value.VCO = (val_3 >> 24) & 0xff;
    
    get_thermal_slope_intercept(&ts_info);
    PTP_Init_value.MTS = ts_info.ts_MTS; 
    PTP_Init_value.BTS = ts_info.ts_BTS; 

    PTP_Init_value.FREQPCT0 = freq_0;
    PTP_Init_value.FREQPCT1 = freq_1;
    PTP_Init_value.FREQPCT2 = freq_2;
    PTP_Init_value.FREQPCT3 = freq_3;
    PTP_Init_value.FREQPCT4 = freq_4;
    PTP_Init_value.FREQPCT5 = freq_5;
    PTP_Init_value.FREQPCT6 = freq_6;
    PTP_Init_value.FREQPCT7 = freq_7;

    PTP_Init_value.DETWINDOW = 0xa28;  // 100 us, This is the PTP Detector sampling time as represented in cycles of bclk_ck during INIT. 52 MHz
    PTP_Init_value.VMAX = 0x5D; // 1.28125v (700mv + n * 6.25mv)
    PTP_Init_value.VMIN = 0x28; // 0.95v (700mv + n * 6.25mv)    
    PTP_Init_value.DTHI = 0x01; // positive
    PTP_Init_value.DTLO = 0xfe; // negative (2・s compliment)
    PTP_Init_value.VBOOT = 0x48; // 115v  (700mv + n * 6.25mv)    
    PTP_Init_value.DETMAX = 0xffff; // This timeout value is in cycles of bclk_ck.

    if( (PTP_Init_value.PTPINITEN == 0x0) || (PTP_Init_value.PTPMONEN == 0x0) || (PTP_Init_value.ADC_CALI_EN == 0x0) )
    {
        clc_notice("~~~ CLC : PTPINITEN = 0x%x, PTPMONEN = 0x%x, ADC_CALI_EN = 0x%x \n", PTP_Init_value.PTPINITEN, PTP_Init_value.PTPMONEN, PTP_Init_value.ADC_CALI_EN);
        return 1;
    }

    // start PTP_Monitor Mode ==================================
    clc_isr_info("===================MMMMMMMMMMMM===============\n");
    clc_isr_info("PTPINITEN = 0x%x\n", PTP_Init_value.PTPINITEN);
    clc_isr_info("PTPMONEN = 0x%x\n", PTP_Init_value.PTPMONEN);
    clc_isr_info("MDES = 0x%x\n", PTP_Init_value.MDES);
    clc_isr_info("BDES = 0x%x\n", PTP_Init_value.BDES);
    clc_isr_info("DCMDET = 0x%x\n", PTP_Init_value.DCMDET);

    clc_isr_info("DCCONFIG = 0x%x\n", PTP_Init_value.DCCONFIG);
    clc_isr_info("DCBDET = 0x%x\n", PTP_Init_value.DCBDET);
    
    clc_isr_info("AGECONFIG = 0x%x\n", PTP_Init_value.AGECONFIG);
    clc_isr_info("AGEM = 0x%x\n", PTP_Init_value.AGEM);

    clc_isr_info("AGEDELTA = 0x%x\n", PTP_Init_value.AGEDELTA);
    clc_isr_info("DVTFIXED = 0x%x\n", PTP_Init_value.DVTFIXED);
    clc_isr_info("MTDES = 0x%x\n", PTP_Init_value.MTDES);
    clc_isr_info("VCO = 0x%x\n", PTP_Init_value.VCO);
    clc_isr_info("MTS = 0x%x\n", PTP_Init_value.MTS);
    clc_isr_info("BTS = 0x%x\n", PTP_Init_value.BTS);

    clc_isr_info("FREQPCT0 = %d\n", PTP_Init_value.FREQPCT0);
    clc_isr_info("FREQPCT1 = %d\n", PTP_Init_value.FREQPCT1);
    clc_isr_info("FREQPCT2 = %d\n", PTP_Init_value.FREQPCT2);
    clc_isr_info("FREQPCT3 = %d\n", PTP_Init_value.FREQPCT3);
    clc_isr_info("FREQPCT4 = %d\n", PTP_Init_value.FREQPCT4);
    clc_isr_info("FREQPCT5 = %d\n", PTP_Init_value.FREQPCT5);
    clc_isr_info("FREQPCT6 = %d\n", PTP_Init_value.FREQPCT6);
    clc_isr_info("FREQPCT7 = %d\n", PTP_Init_value.FREQPCT7);
    
    PTP_CTP_Config_Temp_Reg();
    PTP_Monitor_Mode( &PTP_Init_value );
    
    return 0;
}

u32 PTP_get_ptp_level(void)
{
    u32 ptp_level_temp;

    #if defined(MTK_FORCE_CPU_89T)
        return 3; // 1.5GHz
    #else
        ptp_level_temp = get_devinfo_with_index(3) & 0x7;
    
        if( ptp_level_temp == 0 ) // free mode
        {
            return ((get_devinfo_with_index(10) >> 4) & 0x7);
        }
        else if( ptp_level_temp == 1 ) // 1.5GHz
        {
            return 3;
        }
        else if( ptp_level_temp == 2 ) // 1.4GHz
        {
            return 2;
        }
        else if( ptp_level_temp == 3 ) // 1.3GHz
        {
            return 1;
        }
        else if( ptp_level_temp == 4 ) // 1.2GHz
        {
            return 0;
        }
        else if( ptp_level_temp == 5 ) // 1.1GHz
        {
            return 8;
        }
        else if( ptp_level_temp == 6 ) // 1.0GHz
        {
            return 9;
        }
        else  // 1.0GHz
        {
            return 10;
        }
    #endif
}

#if En_PTP_OD

static int ptp_probe(struct platform_device *pdev)
{
    #if PTP_Get_Real_Val
        val_0 = get_devinfo_with_index(16);
        val_1 = get_devinfo_with_index(17);
        val_2 = get_devinfo_with_index(18);
        val_3 = get_devinfo_with_index(19);
    #endif

    if( (val_0 & 0x1) == 0x0 )
    {
        clc_notice("~~~ CLC : PTPINITEN = 0x%x \n", (val_0 & 0x1));
        return 0;
    }

    // Set PTP IRQ =========================================
    init_PTP_interrupt();

    // Get DVFS frequency table ================================
    freq_0 = (u8)(mt_cpufreq_max_frequency_by_DVS(0) / 12000); // max freq 1200 x 100%
    freq_1 = (u8)(mt_cpufreq_max_frequency_by_DVS(1) / 12000);  // 1000
    freq_2 = (u8)(mt_cpufreq_max_frequency_by_DVS(2) / 12000);  // 715
    freq_3 = (u8)(mt_cpufreq_max_frequency_by_DVS(3) / 12000);  // 419
    freq_4 = (u8)(mt_cpufreq_max_frequency_by_DVS(4) / 12000);
    freq_5 = (u8)(mt_cpufreq_max_frequency_by_DVS(5) / 12000);
    freq_6 = (u8)(mt_cpufreq_max_frequency_by_DVS(6) / 12000);
    freq_7 = (u8)(mt_cpufreq_max_frequency_by_DVS(7) / 12000);

    ptp_level = PTP_get_ptp_level();
    
    PTP_INIT_01();    

    return 0;
}


static int ptp_resume(struct platform_device *pdev)
{
    PTP_INIT_02();    
    return 0;
}

static struct platform_driver mtk_ptp_driver = {
    .remove     = NULL,
    .shutdown   = NULL,
    .probe      = ptp_probe,
    .suspend	= NULL,
    .resume		= ptp_resume,
    .driver     = {
        .name = "mtk-ptp",
    },
};

void PTP_disable_ptp(void)
{
    unsigned long flags;  
    
    // Mask ARM i bit
    local_irq_save(flags);
    
    // disable PTP
    ptp_write(PTP_PTPEN, 0x0);
            
    // Clear PTP interrupt PTPINTSTS
    ptp_write(PTP_PTPINTSTS, 0x00ffffff);
            
    // restore default DVFS table (PMIC)
    mt_cpufreq_return_default_DVS_by_ptpod();

    PTP_Enable = 0; 
    PTP_INIT_FLAG = 0;            
    clc_notice("Disable PTP-OD done.\n");

    // Un-Mask ARM i bit
    local_irq_restore(flags);
}

/***************************
* show current PTP stauts
****************************/
static int ptp_debug_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;

    if (PTP_Enable)
        p += sprintf(p, "PTP enabled (0x%x, 0x%x)\n", val_0, ptp_level);
    else
        p += sprintf(p, "PTP disabled (0x%x, 0x%x)\n", val_0, ptp_level);

    len = p - buf;
    return len;
}

/************************************
* set PTP stauts by sysfs interface
*************************************/
static ssize_t ptp_debug_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int enabled = 0;

    if (sscanf(buffer, "%d", &enabled) == 1)
    {
        if (enabled == 0)
        {            
            // Disable PTP and Restore default DVFS table (PMIC)
            PTP_disable_ptp();
        }
        else
        {
            clc_notice("bad argument_0!! argument should be \"0\"\n");
        }
    }
    else
    {
        clc_notice("bad argument_1!! argument should be \"0\"\n");
    }

    return count;
}

/***************************************
* set PTP log enable by sysfs interface
****************************************/
static ssize_t ptp_log_en_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int enabled = 0;
    ktime_t ktime = ktime_set(mt_ptp_period_s, mt_ptp_period_ns);

    if (sscanf(buffer, "%d", &enabled) == 1)
    {
        if (enabled == 1)
        {
            clc_notice("ptp log enabled.\n");
            mt_ptp_thread = kthread_run(mt_ptp_thread_handler, 0, "ptp logging");
            if (IS_ERR(mt_ptp_thread))
            {
                printk("[%s]: failed to create ptp logging thread\n", __FUNCTION__);
            }
            hrtimer_start(&mt_ptp_timer, ktime, HRTIMER_MODE_REL);
        }
        else if (enabled == 0)
        {
           kthread_stop(mt_ptp_thread);
           hrtimer_cancel(&mt_ptp_timer);
        }
        else
        {
            clc_notice("ptp log disabled.\n");
            clc_notice("bad argument!! argument should be \"0\" or \"1\"\n");
        }
    }
    else
    {
        clc_notice("bad argument!! argument should be \"0\" or \"1\"\n");
    }

    return count;
}

static int __init ptp_init(void)
{
    struct proc_dir_entry *mt_entry = NULL;
    struct proc_dir_entry *mt_ptp_dir = NULL;
    int ptp_err = 0;

    ptp_data[0] = 0xffffffff;

    hrtimer_init(&mt_ptp_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    mt_ptp_timer.function = mt_ptp_timer_func;

    mt_ptp_dir = proc_mkdir("ptp", NULL);
    if (!mt_ptp_dir)
    {
        clc_notice("[%s]: mkdir /proc/ptp failed\n", __FUNCTION__);
    }
    else
    {
        mt_entry = create_proc_entry("ptp_debug", S_IRUGO | S_IWUSR | S_IWGRP, mt_ptp_dir);
        if (mt_entry)
        {
            mt_entry->read_proc = ptp_debug_read;
            mt_entry->write_proc = ptp_debug_write;
        }

        mt_entry = create_proc_entry("ptp_log_en", S_IRUGO | S_IWUSR | S_IWGRP, mt_ptp_dir);
        if (mt_entry)
        {
            mt_entry->write_proc = ptp_log_en_write;
        }
    }

    ptp_err = platform_driver_register(&mtk_ptp_driver);
    
    if (ptp_err)
    {
        clc_notice("PTP driver callback register failed..\n");
        return ptp_err;
    }
    
    return 0;
}

static void __exit ptp_exit(void)
{
    clc_notice("Exit PTP\n");
}

#endif

u32 PTP_INIT_01_API(void)
{
    PTP_Init_T PTP_Init_value;
    u32 ptp_counter = 0;

    clc_notice("~~~ CLC : PTP_INIT_01_API() start.\n");
    ptp_data[0] = 0xffffffff;
    ptp_data[1] = 0xffffffff;
    ptp_data[2] = 0xffffffff;

    #if PTP_Get_Real_Val
        val_0 = get_devinfo_with_index(16);
        val_1 = get_devinfo_with_index(17);
        val_2 = get_devinfo_with_index(18);
        val_3 = get_devinfo_with_index(19);
    #endif

    // disable PTP
    ptp_write(PTP_PTPEN, 0x0);

    PTP_Init_value.PTPINITEN = (val_0) & 0x1;
    PTP_Init_value.PTPMONEN = (val_0 >> 1) & 0x1;
    PTP_Init_value.MDES = (val_0 >> 8) & 0xff;
    PTP_Init_value.BDES = (val_0 >> 16) & 0xff;
    PTP_Init_value.DCMDET = (val_0 >> 24) & 0xff;
    
    PTP_Init_value.DCCONFIG = (val_1) & 0xffffff;
    PTP_Init_value.DCBDET = (val_1 >> 24) & 0xff;
    
    PTP_Init_value.AGECONFIG = (val_2) & 0xffffff;
    PTP_Init_value.AGEM = (val_2 >> 24) & 0xff;
    
    //PTP_Init_value.AGEDELTA = (val_3) & 0xff;
    PTP_Init_value.AGEDELTA = 0x88;    
    PTP_Init_value.DVTFIXED = (val_3 >> 8) & 0xff;
    PTP_Init_value.MTDES = (val_3 >> 16) & 0xff;
    PTP_Init_value.VCO = (val_3 >> 24) & 0xff;

    // Get DVFS frequency table ================================
    freq_0 = (u8)(mt_cpufreq_max_frequency_by_DVS(0) / 12000); // max freq 1200 x 100%
    freq_1 = (u8)(mt_cpufreq_max_frequency_by_DVS(1) / 12000);  // 1000
    freq_2 = (u8)(mt_cpufreq_max_frequency_by_DVS(2) / 12000);  // 715
    freq_3 = (u8)(mt_cpufreq_max_frequency_by_DVS(3) / 12000);  // 419
    freq_4 = (u8)(mt_cpufreq_max_frequency_by_DVS(4) / 12000);
    freq_5 = (u8)(mt_cpufreq_max_frequency_by_DVS(5) / 12000);
    freq_6 = (u8)(mt_cpufreq_max_frequency_by_DVS(6) / 12000);
    freq_7 = (u8)(mt_cpufreq_max_frequency_by_DVS(7) / 12000);
    
    PTP_Init_value.FREQPCT0 = freq_0;
    PTP_Init_value.FREQPCT1 = freq_1;
    PTP_Init_value.FREQPCT2 = freq_2;
    PTP_Init_value.FREQPCT3 = freq_3;
    PTP_Init_value.FREQPCT4 = freq_4;
    PTP_Init_value.FREQPCT5 = freq_5;
    PTP_Init_value.FREQPCT6 = freq_6;
    PTP_Init_value.FREQPCT7 = freq_7;
    
    PTP_Init_value.DETWINDOW = 0xa28;  // 100 us, This is the PTP Detector sampling time as represented in cycles of bclk_ck during INIT. 52 MHz
    PTP_Init_value.VMAX = 0x5D; // 1.28125v (700mv + n * 6.25mv)
    PTP_Init_value.VMIN = 0x28; // 0.95v (700mv + n * 6.25mv)    
    PTP_Init_value.DTHI = 0x01; // positive
    PTP_Init_value.DTLO = 0xfe; // negative (2・s compliment)
    PTP_Init_value.VBOOT = 0x48; // 115v  (700mv + n * 6.25mv)    
    PTP_Init_value.DETMAX = 0xffff; // This timeout value is in cycles of bclk_ck.
    
    if(PTP_Init_value.PTPINITEN == 0x0)
    {
        clc_notice("~~~ CLC : PTPINITEN = 0x%x \n", PTP_Init_value.PTPINITEN);
        return 0;
    }        

    // Set PTP IRQ =========================================
    init_PTP_interrupt();

    // start PTP init_1 =============================
    PTP_Initialization_01( &PTP_Init_value );

    // =========================================

    while(1)
    {
        ptp_counter++;

        if( ptp_counter >= 0xffffff )
        {
            clc_notice("~~~ CLC : ptp_counter = 0x%x \n", ptp_counter);
            return 0;
        }
        
        if(ptp_data[0] == 0)
        {
            break;
        }
    }

    return ((u32)(&ptp_data[1]));
}



MODULE_DESCRIPTION("MT6589 PTP Driver v0.1");
#if En_PTP_OD
late_initcall(ptp_init);
#endif

