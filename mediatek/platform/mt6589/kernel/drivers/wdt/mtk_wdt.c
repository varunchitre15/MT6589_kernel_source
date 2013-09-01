#include <linux/init.h>        /* For init/exit macros */
#include <linux/module.h>      /* For MODULE_ marcros  */
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/watchdog.h>
#include <linux/platform_device.h>

// Monkey.QHQ
#include <linux/list.h>
#include <linux/init.h>
#include <linux/sched.h>
// Monkey.QHQ

#include <asm/uaccess.h>
#include <mach/irqs.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_wdt.h>
#include <linux/delay.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
//#include "mach/mt6575_fiq.h"
#include <wd_kicker.h>
//#include <asm/tcm.h>
#include <linux/aee.h>
#include <mach/sync_write.h>



#undef  DRV_WriteReg32
#define DRV_WriteReg32(a, v)			mt65xx_reg_sync_writel(v, a) //write32


/**---------------------------------------------------------------------
 * Sub feature switch region
 *----------------------------------------------------------------------
 */
#define NO_DEBUG 1

#ifdef  CONFIG_MT6575_FPGA /* FPGA not support this */
#define __USING_DUMMY_WDT_DRV__ /* FPGA will set this flag */
#endif

//#define __ENABLE_WDT_TEST__

/*----------------------------------------------------------------------
 *   IRQ ID 
 *--------------------------------------------------------------------*/
#define AP_RGU_WDT_IRQ_ID    MT_WDT_IRQ_ID


/*----------------------------------------------------------------------
IOCTL
----------------------------------------------------------------------*/
#define WDT_DEVNAME "watchdog"

static struct class *wdt_class = NULL;
static int wdt_major = 0;
static dev_t wdt_devno;
static struct cdev *wdt_cdev;

#ifdef CONFIG_LOCAL_WDT
void mpcore_wdt_restart_fiq(void);

int mpcore_wk_wdt_config(enum wk_wdt_type type, enum wk_wdt_mode mode, int timeout_val);
void mpcore_wdt_restart(enum wk_wdt_type type);
void mpcore_wk_wdt_stop(void);
#endif
/* 
 * internal variables 
 */
static char expect_close; // Not use
//static spinlock_t rgu_reg_operation_spinlock = SPIN_LOCK_UNLOCKED;
static DEFINE_SPINLOCK(rgu_reg_operation_spinlock);
static unsigned int timeout;
static volatile BOOL  rgu_wdt_intr_has_trigger; // For test use
static int g_last_time_time_out_value = 0;
static int g_wdt_enable= 1;

extern void aee_wdt_printf(const char *fmt, ...);

enum {
	WDT_NORMAL_MODE,
	WDT_EXP_MODE
} g_wdt_mode = WDT_NORMAL_MODE;

/* 
 * module parameters 
 */
static int nowayout = WATCHDOG_NOWAYOUT;
module_param(nowayout, int, 0);
//MODULE_PARM_DESC(nowayout, "Watchdog cannot be stopped once started (default=CONFIG_WATCHDOG_NOWAYOUT)");

#ifndef __USING_DUMMY_WDT_DRV__ /* FPGA will set this flag */
/*
    this function set the timeout value.
    value: second
*/
static void mtk_wdt_set_time_out_value(unsigned int value)
{
	/*
	 * TimeOut = BitField 15:5
	 * Key	   = BitField  4:0 = 0x08
	 */	
	spin_lock(&rgu_reg_operation_spinlock);
	
	// 1 tick means 512 * T32K -> 1s = T32/512 tick = 64
	// --> value * (1<<6)
	timeout = (unsigned int)(value * ( 1 << 6) );

	timeout = timeout << 5; 

	DRV_WriteReg32(MTK_WDT_LENGTH, (timeout | MTK_WDT_LENGTH_KEY) );

	spin_unlock(&rgu_reg_operation_spinlock);
}
//EXPORT_SYMBOL(mtk_wdt_set_time_out_value);

/**
 * Set the reset lenght: we will set a special magic key.
 */
#if 0 // Mask this function to fix build warning that "defined but not used"
static void mtk_wdt_set_reset_length(unsigned int value)
{
	/* 
	 * Lenght = 0xFFF for BitField 11:0
	 */
	
	spin_lock(&rgu_reg_operation_spinlock);
	
	DRV_WriteReg32(MTK_WDT_INTERVAL, (value & 0xFFF) );

	spin_unlock(&rgu_reg_operation_spinlock);
}
//EXPORT_SYMBOL(mtk_rgu_wdt_set_reset_length);
#endif
#if 0
static void dual_mode(int enable)
{
    unsigned int tmp;
    //spin_lock(&rgu_reg_operation_spinlock);
	printk(" mtk_wdt_mode_config  mode value=%x,pid=%d\n",DRV_Reg32(MTK_WDT_MODE),current->pid);
	tmp = DRV_Reg32(MTK_WDT_MODE);
	tmp |= MTK_WDT_MODE_KEY;

	if(enable)
	{
	  tmp |= MTK_WDT_MODE_DUAL_MODE;
	}
	else
	{
	  tmp |= ~MTK_WDT_MODE_DUAL_MODE;
	}
	DRV_WriteReg32(MTK_WDT_MODE,tmp);
   // spin_unlock(&rgu_reg_operation_spinlock);
}
#endif
/*
    watchdog mode:
    debug_en:   debug module reset enable. 
    irq:        generate interrupt instead of reset
    ext_en:     output reset signal to outside
    ext_pol:    polarity of external reset signal
    wdt_en:     enable watch dog timer
*/
static void mtk_wdt_mode_config(	BOOL dual_mode_en, 
					BOOL irq, 
					BOOL ext_en, 
					BOOL ext_pol, 
					BOOL wdt_en )
{
	unsigned int tmp;
    
	spin_lock(&rgu_reg_operation_spinlock);
	//printk(" mtk_wdt_mode_config  mode value=%x,pid=%d\n",DRV_Reg32(MTK_WDT_MODE),current->pid);
	tmp = DRV_Reg32(MTK_WDT_MODE);
	tmp |= MTK_WDT_MODE_KEY;

	// Bit 0 : Whether enable watchdog or not
	if(wdt_en == TRUE)
		tmp |= MTK_WDT_MODE_ENABLE;
	else
		tmp &= ~MTK_WDT_MODE_ENABLE;

	// Bit 1 : Configure extern reset signal polarity.
	if(ext_pol == TRUE)
		tmp |= MTK_WDT_MODE_EXT_POL;
	else
		tmp &= ~MTK_WDT_MODE_EXT_POL;

	// Bit 2 : Whether enable external reset signal
	if(ext_en == TRUE)
		tmp |= MTK_WDT_MODE_EXTEN;
	else
		tmp &= ~MTK_WDT_MODE_EXTEN;

	// Bit 3 : Whether generating interrupt instead of reset signal
	if(irq == TRUE)
		tmp |= MTK_WDT_MODE_IRQ;
	else
		tmp &= ~MTK_WDT_MODE_IRQ;

	// Bit 6 : Whether enable debug module reset
	if(dual_mode_en == TRUE)
		tmp |= MTK_WDT_MODE_DUAL_MODE;
	else
		tmp &= ~MTK_WDT_MODE_DUAL_MODE;

	// Bit 4: WDT_Auto_restart, this is a reserved bit, we use it as bypass powerkey flag.
	//		Because HW reboot always need reboot to kernel, we set it always.
	tmp |= MTK_WDT_MODE_AUTO_RESTART;

	DRV_WriteReg32(MTK_WDT_MODE,tmp);
	//dual_mode(1); //always dual mode
	//mdelay(100);
	printk(KERN_INFO " mtk_wdt_mode_config  mode value=%x, tmp:%x,pid=%d\n",DRV_Reg32(MTK_WDT_MODE), tmp,current->pid);
	spin_unlock(&rgu_reg_operation_spinlock);
}
//EXPORT_SYMBOL(mtk_wdt_mode_config);

int mtk_wdt_enable(enum wk_wdt_en en)
{
	unsigned int tmp;
    
	spin_lock(&rgu_reg_operation_spinlock);

	tmp = DRV_Reg32(MTK_WDT_MODE);
	
	tmp |= MTK_WDT_MODE_KEY;
	if(WK_WDT_EN == en)
	{
	  tmp &= MTK_WDT_MODE_ENABLE;
	  g_wdt_enable = 1;
	}
	if(WK_WDT_DIS == en)
	{
	  tmp &= ~MTK_WDT_MODE_ENABLE;
	  g_wdt_enable = 0;
	}
    printk("mtk_wdt_enable value=%x,pid=%d\n",tmp,current->pid);
	DRV_WriteReg32(MTK_WDT_MODE,tmp);
    
	spin_unlock(&rgu_reg_operation_spinlock);

	return 0;
}
 

/* This function will disable watch dog */
/* disable API for other module */
void mtk_wdt_disable(void)
{
	mtk_wdt_enable(WK_WDT_DIS);
}

int hw_reboot_mode(void)
{
    
#ifdef CONFIG_LOCAL_WDT
	/* stop the local WDT and restart the global WDT */
	mpcore_wk_wdt_stop();
    mtk_wdt_restart(WK_WDT_EXT_TYPE);
#else
	//dual mode =0, irq=0
    mtk_wdt_mode_config(FALSE, FALSE, TRUE, FALSE, TRUE);
#endif

    return 0;
}

//*********************WDT API  end****************************/


/* This function will return WDT EN setting */
int mtk_wdt_get_en_setting(void)
{
	return (DRV_Reg32(MTK_WDT_MODE)&MTK_WDT_MODE_ENABLE);
}

/*Kick the watchdog*/

void mtk_wdt_restart(enum wk_wdt_type type)
{

    //printk("WDT:[mtk_wdt_restart] type  =%d, pid=%d\n",type,current->pid);
	if(type == WK_WDT_LOC_TYPE)
	{
	    #ifdef CONFIG_LOCAL_WDT
		mpcore_wdt_restart(type);
		#endif
		return;
	}
	else if(type == WK_WDT_EXT_TYPE) 
	{     
	      //printk("WDT:ext restart\n" );
	      spin_lock(&rgu_reg_operation_spinlock);
	      DRV_WriteReg32(MTK_WDT_RESTART, MTK_WDT_RESTART_KEY);
	      spin_unlock(&rgu_reg_operation_spinlock);
	}
	else if(type == WK_WDT_EXT_TYPE_NOLOCK) 
	{
	      *(volatile u32 *)( MTK_WDT_RESTART) =MTK_WDT_RESTART_KEY ;
	}
	else if(type == WK_WDT_LOC_TYPE_NOLOCK)
	{
	       #ifdef CONFIG_LOCAL_WDT
	       mpcore_wdt_restart_fiq();
		   #endif
	}
	else
	{
	      //printk("WDT:[mtk_wdt_restart] type error pid =%d\n",type,current->pid);
	}
}

//EXPORT_SYMBOL(mtk_wdt_restart);

void mtk_wdt_set_timer(unsigned int value)
{
	timeout = (unsigned int)(value * ( 1 << 6) );

	timeout = timeout << 5; 

	DRV_WriteReg32(MTK_WDT_LENGTH, (timeout | MTK_WDT_LENGTH_KEY));
	
	mtk_wdt_restart(WK_WDT_EXT_TYPE);

}

#if 0 // Mask this function to fix build warning that "defined but not used"
/*software reset*/
static void mtk_wdt_sw_trigger(void)
{
	// SW trigger WDT reset or IRQ
	spin_lock(&rgu_reg_operation_spinlock);	
	DRV_WriteReg32(MTK_WDT_SWRST, MTK_WDT_SWRST_KEY);
	spin_unlock(&rgu_reg_operation_spinlock);
}
//EXPORT_SYMBOL(mtk_wdt_SWTrigger);
#endif

#if 0 // Mask this function to fix build warning that "defined but not used"
/*
    check the watchdog status: reset reason.
    0x8000: reset due to watchdog reset
    0x4000: software triggered. 
*/
static unsigned char mtk_wdt_check_status(void)
{
	unsigned char status;

	spin_lock(&rgu_reg_operation_spinlock);	
	status = DRV_Reg32(MTK_WDT_STATUS);
	spin_unlock(&rgu_reg_operation_spinlock);
	
	return status;
}
//EXPORT_SYMBOL(mtk_wdt_CheckStatus);
#endif

void wdt_dump_reg(void)
{
	printk("****************dump wdt reg start*************\n");
	printk("MTK_WDT_MODE:0x%x\n", DRV_Reg32(MTK_WDT_MODE));
	printk("MTK_WDT_LENGTH:0x%x\n", DRV_Reg32(MTK_WDT_LENGTH));
	printk("MTK_WDT_RESTART:0x%x\n", DRV_Reg32(MTK_WDT_RESTART));
	printk("MTK_WDT_STATUS:0x%x\n", DRV_Reg32(MTK_WDT_STATUS));
	printk("MTK_WDT_INTERVAL:0x%x\n", DRV_Reg32(MTK_WDT_INTERVAL));
	printk("MTK_WDT_SWRST:0x%x\n", DRV_Reg32(MTK_WDT_SWRST));
	printk("MTK_WDT_NONRST_REG:0x%x\n", DRV_Reg32(MTK_WDT_NONRST_REG));
	printk("MTK_WDT_NONRST_REG2:0x%x\n", DRV_Reg32(MTK_WDT_NONRST_REG2));
	printk("MTK_WDT_REQ_MODE:0x%x\n", DRV_Reg32(MTK_WDT_REQ_MODE));
	printk("MTK_WDT_REQ_IRQ_EN:0x%x\n", DRV_Reg32(MTK_WDT_REQ_IRQ_EN));
	printk("MTK_WDT_DRAMC_CTL:0x%x\n", DRV_Reg32(MTK_WDT_DRAMC_CTL));
	printk("****************dump wdt reg end*************\n");
	
}


void wdt_arch_reset(char mode)
{

    unsigned int wdt_mode_val;
	printk("wdt_arch_reset called@Kernel mode =%c\n",mode);
	
	spin_lock(&rgu_reg_operation_spinlock);
	/* Watchdog Rest */
	DRV_WriteReg32(MTK_WDT_RESTART, MTK_WDT_RESTART_KEY);
	wdt_mode_val = DRV_Reg32(MTK_WDT_MODE);
	printk("wdt_arch_reset called MTK_WDT_MODE =%x \n",wdt_mode_val);
	/* clear autorestart bit: autoretart: 1, bypass power key, 0: not bypass power key */
	wdt_mode_val &=(~MTK_WDT_MODE_AUTO_RESTART);
	/* make sure WDT mode is hw reboot mode, can not config isr mode  */
	wdt_mode_val &=(~(MTK_WDT_MODE_IRQ|MTK_WDT_MODE_ENABLE | MTK_WDT_MODE_DUAL_MODE));
	if(mode)
	{
		/* mode != 0 means by pass power key reboot, We using auto_restart bit as by pass power key flag */
		 wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY|MTK_WDT_MODE_EXTEN|MTK_WDT_MODE_AUTO_RESTART);
		 
	}else
	{
	       wdt_mode_val = wdt_mode_val | (MTK_WDT_MODE_KEY|MTK_WDT_MODE_EXTEN);
		 
	}

	DRV_WriteReg32(MTK_WDT_MODE,wdt_mode_val);
	printk("wdt_arch_reset called end  MTK_WDT_MODE =%x \n",wdt_mode_val);
	udelay(100);
	DRV_WriteReg32(MTK_WDT_SWRST, MTK_WDT_SWRST_KEY);
        printk("wdt_arch_reset: SW_reset happen\n");
	spin_unlock(&rgu_reg_operation_spinlock);

	while (1)
	{
	    wdt_dump_reg();
		printk("wdt_arch_reset error\n");
	}
	
}
EXPORT_SYMBOL(wdt_arch_reset);

static int mtk_wdt_swsysret_config(int bit,int set_value)
{
    unsigned int wdt_sys_val;
	spin_lock(&rgu_reg_operation_spinlock);
	wdt_sys_val = DRV_Reg32(MTK_WDT_SWSYSRST);
	printk("fwq2 before set wdt_sys_val =%x\n",wdt_sys_val);
    wdt_sys_val |= MTK_WDT_SWSYS_RST_KEY;
	switch(bit)
	{
	  case MTK_WDT_SWSYS_RST_MD_RST:
	  	   if(1==set_value)
	  	   {
	  	     wdt_sys_val |= MTK_WDT_SWSYS_RST_MD_RST;
	  	   }
		   if(0==set_value)
		   {
		     wdt_sys_val |= ~MTK_WDT_SWSYS_RST_MD_RST;
		   }
	  	break;
	 case MTK_WDT_SWSYS_RST_MD_LITE_RST:
	  	   if(1==set_value)
	  	   {
	  	     wdt_sys_val |= MTK_WDT_SWSYS_RST_MD_LITE_RST;
	  	   }
		   if(0==set_value)
		   {
		     wdt_sys_val |= ~MTK_WDT_SWSYS_RST_MD_LITE_RST;
		   }
	  	break;
	  	   
	}
	DRV_WriteReg32(MTK_WDT_SWSYSRST,wdt_sys_val);
	spin_unlock(&rgu_reg_operation_spinlock);

	mdelay(10);
	printk("after set wdt_sys_val =%x,wdt_sys_val=%x\n",DRV_Reg32(MTK_WDT_SWSYSRST),wdt_sys_val);
    return 0;
}
EXPORT_SYMBOL(mtk_wdt_swsysret_config);	
#else 
//-------------------------------------------------------------------------------------------------
//      Dummy functions
//-------------------------------------------------------------------------------------------------
static void mtk_wdt_set_time_out_value(unsigned int value){}
static void mtk_wdt_set_reset_length(unsigned int value){}
static void mtk_wdt_mode_config(BOOL dual_mode_en,BOOL irq,	BOOL ext_en, BOOL ext_pol, BOOL wdt_en){}
void mtk_wdt_disable(void){}
int mtk_wdt_get_en_setting(void){return 0;}
void mtk_wdt_restart(enum wk_wdt_type type){}
static void mtk_wdt_sw_trigger(void){}
static unsigned char mtk_wdt_check_status(void){}
void wdt_arch_reset(char mode){}
EXPORT_SYMBOL(wdt_arch_reset);

#endif //#ifndef __USING_DUMMY_WDT_DRV__

/*where to config watchdog kicker*/
#ifdef CONFIG_MTK_WD_KICKER

//note: enable only can contrl ext wdt
static int mtk_wk_wdt_config(enum wk_wdt_type type, enum wk_wdt_mode mode, int timeout_val)
{

    #ifdef CONFIG_LOCAL_WDT
	if(type == WK_WDT_LOC_TYPE)
	{
		mpcore_wk_wdt_config(type, mode, timeout_val);
		return 0;
	}
	#endif
	
	g_wdt_mode = WDT_EXP_MODE;//always enable irq
	mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
#if 0
	//disable WDT reset, auto-restart disable , disable intr
	//mtk_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);
	//en debug, dis irq, dis ext, low pol, dis wdt
	mtk_wdt_mode_config(TRUE, FALSE, FALSE, FALSE, FALSE);
	mtk_wdt_restart(type);

	if (mode == WK_WDT_EXP_MODE) {
		g_wdt_mode = WDT_EXP_MODE;
		mtk_wdt_mode_config(TRUE, TRUE, FALSE, FALSE, TRUE);
	}
	else {
		g_wdt_mode = WDT_NORMAL_MODE;
		mtk_wdt_mode_config(TRUE, FALSE, FALSE, FALSE, TRUE);
	}
#endif
	g_last_time_time_out_value = timeout_val;
	mtk_wdt_set_time_out_value(timeout_val);

	return 0;
}


static struct wk_wdt mtk_wk_wdt = {
	.config 	= mtk_wk_wdt_config,
	.kick_wdt 	= mtk_wdt_restart,
	.enable     = mtk_wdt_enable,//used for proc control
};
#endif

#ifdef __ENABLE_WDT_TEST__
/*
 * Test Program :
 *				1. WDT_HW_Reset_test
 *				2. WDT_SW_Reset_test
 *				3. WDT_count_test
 */

void WDT_HW_Reset_test(void)
{
	printk("WDT_HW_Reset_test : System will reset after 5 secs\n");
	mtk_wdt_set_time_out_value(5);
	//enable WDT reset, auto-restart enable ,disable interrupt.
	//mtk_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);
	// en debug, dis irq, dis ext, low pol, en wdt
	mtk_wdt_mode_config(TRUE, FALSE, TRUE, FALSE, TRUE);
	mtk_wdt_restart(WK_WDT_EXT_TYPE);
	
	while(1);
}
EXPORT_SYMBOL(WDT_HW_Reset_test);


void WDT_HW_Reset_kick_6times_test(void)
{
	int kick_times = 6;
	
	mtk_wdt_set_time_out_value(5);	
	//enable WDT reset, auto-restart enable ,disable intr
	//mtk_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);
	// en debug, dis irq, dis ext, low pol, en wdt
	mtk_wdt_mode_config(TRUE, FALSE, TRUE, FALSE, TRUE);
	mtk_wdt_restart(WK_WDT_EXT_TYPE);

	// kick WDT test.
	while(kick_times >= 0)
	{
		mdelay(3000);
		printk("WDT_HW_Reset_test : reset after %d times !\n", kick_times);
		mtk_wdt_restart(WK_WDT_EXT_TYPE);
		kick_times--;
	}

	printk("WDT_HW_Reset_test : Kick stop,System will reset after 5 secs!!\n");
	while(1);
	
}
EXPORT_SYMBOL(WDT_HW_Reset_kick_6times_test);


void WDT_SW_Reset_test(void)
{
	printk("WDT_SW_Reset_test : System will reset Immediately\n");
		
	// disable WDT reset, auto-restart disable ,disable intr
	//mtk_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);
	// en debug, dis irq, dis ext, low pol, dis wdt
	mtk_wdt_mode_config(TRUE, FALSE, TRUE, FALSE, FALSE);
	
	mtk_wdt_set_time_out_value(1);
	mtk_wdt_restart(1);
	
	mtk_wdt_sw_trigger();
	
	while(1);
}
EXPORT_SYMBOL(WDT_SW_Reset_test);

void WDT_count_test(void)
{
	/*
	 * Try DVT testsuite : WDT_count_test (Non-reset test)
	 */
	printk("WDT_count_test Start..........\n");

	mtk_wdt_set_time_out_value(10);
	// enable WDT reset, auto-restart disable, enable intr
	//mtk_wdt_ModeSelection(KAL_TRUE, KAL_FALSE, KAL_TRUE);
	// en debug, en irq, dis ext, low pol, en wdt
	mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
	mtk_wdt_restart(WK_WDT_EXT_TYPE);

	printk("1/2 : Waiting 10 sec. for WDT with WDT_status.\n");
	
	// wait and check WDT status
	while(DRV_Reg32(MTK_WDT_STATUS) != MTK_WDT_STATUS_HWWDT_RST);	
	printk("WDT_count_test done by WDT_STATUS!!\n");

	/*status is checked*/

	/*check interrupt.*/ 
	rgu_wdt_intr_has_trigger = 0; // set to zero before WDT counting down

	/*can continue.*/
	mtk_wdt_restart(WK_WDT_EXT_TYPE);
	
	printk("2/2 : Waiting 10 sec. for WDT with IRQ.\n");

	//need a ISR, when interrupt, set rgu_wdt_intr_has_trigger to 1.
	while(rgu_wdt_intr_has_trigger == 0);
	printk("WDT_count_test done by IRQ!!\n");
	
	printk("WDT_count_test Finish !!\n");	
}
EXPORT_SYMBOL(WDT_count_test);
#endif //__ENABLE_WDT_TEST__


//void aee_bug(const char *source, const char *msg);

//Monkey.QHQ
#ifndef CONFIG_FIQ_GLUE
static void wdt_report_info (void)
{
    //extern struct task_struct *wk_tsk;
    struct task_struct *task ;
    task = &init_task ;
    
    printk ("Qwdt: -- watchdog time out\n") ;
    for_each_process (task)
    {
        if (task->state == 0)
        {
            printk ("PID: %d, name: %s\n backtrace:\n", task->pid, task->comm) ;
            show_stack (task, NULL) ;
            printk ("\n") ;
        }
    }
    
    
    printk ("backtrace of current task:\n") ;
    show_stack (NULL, NULL) ;
    
       
    printk ("Qwdt: -- watchdog time out\n") ;    
}
//Monkey.QHQ
#endif

/*
 * mtk WatchDogTimer's interrupt handler
 */
//static __tcmfunc irqreturn_t mtk_wdt_isr(int irq, void *dev_id)

/*
 * Kernel interface (Not use)
 */
static int mtk_wdt_open(struct inode *inode, struct file *file)
{
	printk( "\n******** WDT driver open!! ********\n" );
	
	#if NO_DEBUG
	
	//mtk_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);

	/*
	 * default : user can not stop WDT
	 *
	 * If the userspace daemon closes the file without sending
	 * this special character "V", the driver will assume that the daemon (and
	 * userspace in general) died, and will stop pinging the watchdog without
	 * disabling it first.  This will then cause a reboot.
	 */
	expect_close = 0;
	#endif	

	return nonseekable_open(inode, file);
}

static int mtk_wdt_release(struct inode *inode, struct file *file)
{
	printk( "\n******** WDT driver release!! ********\n");

	//if( expect_close == 42 )
	if( expect_close == 0 )
	{
		#if NO_DEBUG		
		//mtk_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);
		// en debug, dis irq, dis ext, low pol, dis wdt
		mtk_wdt_mode_config(TRUE, FALSE, TRUE, FALSE, FALSE);
		#endif
	}
	else
	{
		#if NO_DEBUG		
		mtk_wdt_restart(WK_WDT_EXT_TYPE);
		#endif
	}

	expect_close = 0;
	
	g_wdt_enable = 0;
		
	return 0;
}

static ssize_t mtk_wdt_write(struct file *file, const char __user *data,
								size_t len, loff_t *ppos)
{	
	printk( "\n******** WDT driver : write<%d> ********\n",len);	

	if(len) 
	{
		if(!nowayout) 
		{			
			size_t i;
			expect_close = 0;
			for( i = 0 ; i != len ; i++ ) 
			{
				char c;
				if( get_user(c, data + i) )
					return -EFAULT;
				
				if( c == 'V' )
				{
					expect_close = 42;
					printk( "\nnowayout=N and write=V, you can disable HW WDT\n");
				}
			}
		}		
	        		
        mtk_wdt_restart(WK_WDT_EXT_TYPE);
  
	}
	return len;
}

#define OPTIONS WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT | WDIOF_MAGICCLOSE

static struct watchdog_info mtk_wdt_ident = {
	.options          = OPTIONS,
	.firmware_version =	0,
	.identity         =	"MTK Watchdog",
};

//static int mtk_wdt_ioctl(struct inode *inode, struct file *file,
//							unsigned int cmd, unsigned long arg)
static long mtk_wdt_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int __user *p = argp;
	int new_time_out_value;

	printk("******** MTK WDT driver ioctl Cmd<%d>!! ********\n",cmd);
	switch (cmd) {
		default:
			return -ENOIOCTLCMD;

		case WDIOC_GETSUPPORT:
			return copy_to_user(argp, &mtk_wdt_ident, sizeof(mtk_wdt_ident)) ? -EFAULT : 0;

		case WDIOC_GETSTATUS:
		case WDIOC_GETBOOTSTATUS:
			return put_user(0, p);

		case WDIOC_KEEPALIVE:
			
			mtk_wdt_restart(WK_WDT_EXT_TYPE);
		
			return 0;

		case WDIOC_SETTIMEOUT:
			if( get_user(new_time_out_value, p) )
				return -EFAULT;
			
			mtk_wdt_set_time_out_value(new_time_out_value);

			g_last_time_time_out_value = new_time_out_value;
			g_wdt_enable = 1;

			if (g_wdt_mode == WDT_EXP_MODE){
				//mtk_wdt_ModeSelection(KAL_TRUE, KAL_FALSE, KAL_TRUE);
				// en debug, en irq, dis ext, low pol, en wdt
				mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);	        
			}else{
				//mtk_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);
				// en debug, dis irq, dis ext, low pol, en wdt
				mtk_wdt_mode_config(TRUE, FALSE, TRUE, FALSE, TRUE);
			}

			mtk_wdt_restart(WK_WDT_EXT_TYPE);

			//why not just retrun new_TimeOutValue or g_last_time_time_out_value.
			return put_user(timeout >> 11, p);

		case WDIOC_GETTIMEOUT:
			return put_user(timeout >> 11, p);
	}

	return 0;
}


static struct file_operations mtk_wdt_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.open		= mtk_wdt_open,
	.release	= mtk_wdt_release,
	.write		= mtk_wdt_write,
	//.ioctl	= mtk_wdt_ioctl,
	.unlocked_ioctl	= mtk_wdt_ioctl,
};
#ifdef CONFIG_FIQ_GLUE
static void wdt_fiq(void *arg, void *regs, void *svc_sp)
{
    // register int sp asm("sp");
     //unsigned int *preg = (unsigned int*)regs;
	 //printk("fwq:wdt fiq is going\n");
     #ifdef  CONFIG_MTK_WD_KICKER
     aee_wdt_printf("kick=0x%08x,check=0x%08x",get_kick_bit(),get_check_bit());
	 #endif 
     aee_wdt_fiq_info(arg, regs, svc_sp);
#if 0
     asm volatile("mov %0, %1\n\t"
                  "mov fp, %2\n\t"
                 : "=r" (sp)
                 : "r" (svc_sp), "r" (preg[11])
                 );

     *((volatile unsigned int*)(0x00000000)); /* trigger exception */
	 #endif
}
#else
static irqreturn_t mtk_wdt_isr(int irq, void *dev_id)

{
    printk("fwq mtk_wdt_isr\n" );
#ifndef __USING_DUMMY_WDT_DRV__ /* FPGA will set this flag */
	//mt65xx_irq_mask(AP_RGU_WDT_IRQ_ID);
	rgu_wdt_intr_has_trigger = 1;

	//Monkey.QHQ
	wdt_report_info () ;
	//Monkey.QHQ
	BUG();

	//need to modift. 
	//aee_bug("WATCHDOG", "Watch Dog Timeout");
	//mt65xx_irq_ack(AP_RGU_WDT_IRQ_ID);
	//mt65xx_irq_unmask(AP_RGU_WDT_IRQ_ID);
#endif	
	return IRQ_HANDLED;
}
#endif

/* 
 * Device interface 
 */
static int mtk_wdt_probe(struct platform_device *dev)
{
	int ret=0;
	unsigned int interval_val;
	
	printk("******** MTK WDT driver probe!! ********\n" );

#ifndef __USING_DUMMY_WDT_DRV__ /* FPGA will set this flag */
#ifndef CONFIG_FIQ_GLUE	
	//mt65xx_irq_set_sens(MT6575_APWDT_IRQ_LINE, MT65xx_EDGE_SENSITIVE);
	//mt6577_irq_set_sens(AP_RGU_WDT_IRQ_ID, MT65xx_EDGE_SENSITIVE);
        //mt6577_irq_set_polarity(AP_RGU_WDT_IRQ_ID, MT65xx_POLARITY_LOW);	
    printk("******** MTK WDT register irq ********\n" );
	ret = request_irq(AP_RGU_WDT_IRQ_ID, (irq_handler_t)mtk_wdt_isr, IRQF_TRIGGER_FALLING, "mtk_watchdog", NULL);
#else
    printk("******** MTK WDT register fiq ********\n" );
	ret = request_fiq(AP_RGU_WDT_IRQ_ID, wdt_fiq, IRQF_TRIGGER_FALLING, NULL);
#endif	
        if(ret != 0)
	{
		printk( "mtk_wdt_probe : failed to request irq (%d)\n", ret);
		return ret;
	}
	printk("mtk_wdt_probe : Success to request irq\n");

	/* Set timeout vale and restart counter */
	g_last_time_time_out_value=30;
	mtk_wdt_set_time_out_value(g_last_time_time_out_value);
		
	mtk_wdt_restart(WK_WDT_EXT_TYPE);
	//set mode
	g_wdt_mode = WDT_EXP_MODE;

	

	/**
	 * Set the reset lenght: we will set a special magic key.
	 * For Power off and power on reset, the INTERVAL default value is 0x7FF.
	 * We set Interval[1:0] to different value to distinguish different stage.
	 * Enter pre-loader, we will set it to 0x0
	 * Enter u-boot, we will set it to 0x1
	 * Enter kernel, we will set it to 0x2
	 * And the default value is 0x3 which means reset from a power off and power on reset
	 */
	#define POWER_OFF_ON_MAGIC	(0x3)
	#define PRE_LOADER_MAGIC	(0x0)
	#define U_BOOT_MAGIC		(0x1)
	#define KERNEL_MAGIC		(0x2)
	#define MAGIC_NUM_MASK		(0x3)

#ifdef CONFIG_LOCAL_WDT
	// Initialize to reset mode
	mtk_wdt_mode_config(TRUE, FALSE, TRUE, FALSE, TRUE);	
      //set local WDT time out 25s
      mtk_wk_wdt_config(WK_WDT_LOC_TYPE, WK_WDT_NORMAL_MODE, 25);
#else
    #ifdef  CONFIG_MTK_WD_KICKER	// Initialize to dual mode
	printk("mtk_wdt_probe : Initialize to dual mode \n");
	mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
	#else				// Initialize to disable wdt
	printk("mtk_wdt_probe : Initialize to disable wdt \n");
	mtk_wdt_mode_config(FALSE, FALSE, TRUE, FALSE, FALSE);
	g_wdt_enable =0;
	#endif
#endif

	/* Update interval register value and check reboot flag */
	interval_val = DRV_Reg32(MTK_WDT_INTERVAL);
	interval_val &= ~(MAGIC_NUM_MASK);
	interval_val |= (KERNEL_MAGIC);
	/* Write back INTERVAL REG */
	DRV_WriteReg32(MTK_WDT_INTERVAL, interval_val);
#endif

#ifdef CONFIG_MTK_WD_KICKER
	wk_register_wdt(&mtk_wk_wdt);
#endif
   udelay(100);
   printk("mtk_wdt_probe : done WDT_MODE(%x)\n",DRV_Reg32(MTK_WDT_MODE));


	return ret;
}

static int mtk_wdt_remove(struct platform_device *dev)
{
	printk("******** MTK wdt driver remove!! ********\n" );

#ifndef __USING_DUMMY_WDT_DRV__ /* FPGA will set this flag */
	free_irq(AP_RGU_WDT_IRQ_ID, NULL);
#endif
	return 0;
}

static void mtk_wdt_shutdown(struct platform_device *dev)
{	
	printk("******** MTK WDT driver shutdown!! ********\n" );

	//mtk_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);
	//kick external wdt
	//mtk_wdt_mode_config(TRUE, FALSE, FALSE, FALSE, FALSE);

	mtk_wdt_restart(WK_WDT_EXT_TYPE);

   printk("******** MTK WDT driver shutdown done ********\n" );
}

void mtk_wdt_suspend(void)
{
	//mtk_wdt_ModeSelection(KAL_FALSE, KAL_FALSE, KAL_FALSE);
	// en debug, dis irq, dis ext, low pol, dis wdt
	mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, FALSE);
#ifdef 	CONFIG_LOCAL_WDT	
	//disable locat wdt
	mpcore_wk_wdt_stop();
	mtk_wdt_restart(WK_WDT_EXT_TYPE);
	
#else
	mtk_wdt_restart(WK_WDT_EXT_TYPE);
#endif	

	aee_sram_printk("[WDT] suspend\n");
	printk("[WDT] suspend\n");
}

void mtk_wdt_resume(void)
{
	
	if ( g_wdt_enable == 1 ) 
	{
		mtk_wdt_set_time_out_value(g_last_time_time_out_value);
		
		if (g_wdt_mode == WDT_EXP_MODE){
			//mtk_wdt_ModeSelection(KAL_TRUE, KAL_FALSE, KAL_TRUE);
			// en debug, en irq, dis ext, low pol, en wdt
			mtk_wdt_mode_config(TRUE, TRUE, TRUE, FALSE, TRUE);
		}else{
			//mtk_wdt_ModeSelection(KAL_TRUE, KAL_TRUE, KAL_FALSE);
			// en debug, dis irq, dis ext, low pol, en wdt
			mtk_wdt_mode_config(TRUE, FALSE, TRUE, FALSE, TRUE);
		}
#ifdef 	CONFIG_LOCAL_WDT
		// resume local wdt
		mpcore_wdt_restart(WK_WDT_LOC_TYPE);
		mtk_wdt_restart(WK_WDT_EXT_TYPE);
#else
		mtk_wdt_restart(WK_WDT_EXT_TYPE);
#endif		
	}

	aee_sram_printk("[WDT] resume(%d)\n", g_wdt_enable);
	printk("[WDT] resume(%d)\n", g_wdt_enable);
}



static struct platform_driver mtk_wdt_driver =
{
	.driver     = {
		.name	= "mtk-wdt",
	},
	.probe	= mtk_wdt_probe,
	.remove	= mtk_wdt_remove,
	.shutdown	= mtk_wdt_shutdown,
//	.suspend	= mtk_wdt_suspend,
//	.resume	= mtk_wdt_resume,
};

struct platform_device mtk_device_wdt = {
		.name		= "mtk-wdt",
		.id		= 0,
		.dev		= {
		}
};

/*
 * init and exit function
 */
static int __init mtk_wdt_init(void)
{
	struct class_device *class_dev = NULL;
	int ret;
	
	ret = platform_device_register(&mtk_device_wdt);
	if (ret) {
		printk("****[mtk_wdt_driver] Unable to device register(%d)\n", ret);
		return ret;
	}

	ret = platform_driver_register(&mtk_wdt_driver);
	if (ret) {
		printk("****[mtk_wdt_driver] Unable to register driver (%d)\n", ret);
		return ret;
	}

	ret = alloc_chrdev_region(&wdt_devno, 0, 1, WDT_DEVNAME);
	if (ret) 
		printk("Error: Can't Get Major number for mtk WDT \n");
	
	wdt_cdev = cdev_alloc();
	wdt_cdev->owner = THIS_MODULE;
	wdt_cdev->ops = &mtk_wdt_fops;
	
	ret = cdev_add(wdt_cdev, wdt_devno, 1);
	if(ret)
	    printk("MTK WDT Error: cdev_add\n");
	
	wdt_major = MAJOR(wdt_devno);
	wdt_class = class_create(THIS_MODULE, WDT_DEVNAME);
	class_dev = (struct class_device *)device_create(wdt_class, 
							 NULL, 
							 wdt_devno, 
							 NULL, 
							 WDT_DEVNAME);

	return 0;	
}

static void __exit mtk_wdt_exit (void)
{
}

module_init(mtk_wdt_init);
module_exit(mtk_wdt_exit);

EXPORT_SYMBOL(mtk_wdt_set_timer);
MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("MT6577 Watchdog Device Driver");
MODULE_LICENSE("GPL");

