#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>

#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/workqueue.h>
#include <linux/switch.h>
#include <linux/delay.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/wakelock.h>
#include <linux/time.h>

#include <linux/string.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_reg_base.h>
#include <mach/irqs.h>
#include <accdet_custom.h>
#include <mach/reg_accdet.h>
//#include <mach/mt6577_pll.h>
//#include <mach/mt_pmic_feature_api.h>
#include <mach/pmic_mt6329_sw_bank1.h>
#include <mach/pmic_mt6329_hw_bank1.h>
#include <mach/upmu_hw.h>
//#include <mach/mt_clock_manager.h>

/*----------------------------------------------------------------------
IOCTL
----------------------------------------------------------------------*/
#define ACCDET_DEVNAME "accdet"
#define ACCDET_IOC_MAGIC 'A'
#define ACCDET_INIT _IO(ACCDET_IOC_MAGIC,0)
#define SET_CALL_STATE _IO(ACCDET_IOC_MAGIC,1)
#define GET_BUTTON_STATUS _IO(ACCDET_IOC_MAGIC,2)

/*define for phone call state*/

#define CALL_IDLE 0
#define CALL_RINGING 1
#define CALL_ACTIVE 2
#define KEY_CALL	KEY_SEND
#define KEY_ENDCALL	KEY_HANGEUL


enum accdet_report_state
{
    NO_DEVICE =0,
    HEADSET_MIC = 1,
    HEADSET_NO_MIC = 2,
    HEADSET_ILEGAL = 3,
    //DOUBLE_CHECK_TV = 4
};

enum accdet_status
{
    PLUG_OUT = 0,
    MIC_BIAS = 1,
    //DOUBLE_CHECK = 2,
    HOOK_SWITCH = 2,
    MIC_BIAS_ILEGAL =3,
    //TV_OUT = 5,
    STAND_BY =4
};

char *accdet_status_string[5]=
{
    "Plug_out",
    "Headset_plug_in",
    //"Double_check",
    "Hook_switch",
    "Headset_ilegal",
    //"Tvout_plug_in",
    "Stand_by"
};

char *accdet_report_string[4]=
{
    "No_device",
    "Headset_mic",
    "Headset_no_mic",
    "HEADSET_ilegal",
   // "Double_check"
};

enum hook_switch_result
{
    DO_NOTHING =0,
    ANSWER_CALL = 1,
    REJECT_CALL = 2
};

typedef enum
{	
    TVOUT_STATUS_OK = 0,
    TVOUT_STATUS_ALREADY_SET,
    TVOUT_STATUS_ERROR,
} TVOUT_STATUS;

static int debug_enable = 1;
#define ACCDET_DEBUG(format, args...) do{ \
	if(debug_enable) \
	{\
		printk(KERN_ERR format,##args);\
	}\
}while(0)

