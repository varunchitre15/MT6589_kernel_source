
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>
//#include <asm/tcm.h>
#include <asm/io.h>

#include <mach/mt_reg_base.h>
//#include "mt6573_kpd.h"
#include <mach/mt_gpio.h>
#include <mach/irqs.h>
//#include <mach/mt6575_eint.h>
//#include <mach/mt_sc.h>

#define KPD_DEBUG	1

#define KPD_NAME	"kpd_ldvt"

#if KPD_DEBUG
#define kpd_print(fmt, arg...)	printk(KPD_SAY fmt, ##arg)
#else
#define kpd_print(fmt, arg...)	do {} while (0)
#endif

//#define GPIO_BASE (0xF7023000)

#define GPIO_DIR_BASE           GPIO_BASE
#define GPIO_PULLEN_BASE        (GPIO_BASE+0x200)
#define GPIO_PULLSEL_BASE       (GPIO_BASE+0x400)
#define GPIO_DINV_BASE          (GPIO_BASE+0x600)
#define GPIO_DOUT_BASE          (GPIO_BASE+0x800)
#define GPIO_DIN_BASE           (GPIO_BASE+0xA00)
#define GPIO_MODE_BASE          (GPIO_BASE+0xC00)

#define GPIO_SET_DIR(_n,dir)    *(volatile unsigned short *)(GPIO_DIR_BASE + (_n>>4)*0x10) =  \
                                    (*(volatile unsigned short *)(GPIO_DIR_BASE + (_n>>4)*0x10) & (~(0x1 <<(_n&0xF)))) | (dir<<(_n&0xF))   

#define GPIO_SET_PULL(_n,en)        *(volatile unsigned short *)(GPIO_PULLEN_BASE + (_n>>4)*0x10) =  \
                                    (*(volatile unsigned short *)(GPIO_PULLEN_BASE + (_n>>4)*0x10) & (~(0x1 <<(_n&0xF)))) | (en<<(_n&0xF))

#define GPIO_SELECT_PULL(_n,sel)    *(volatile unsigned short *)(GPIO_PULLSEL_BASE + (_n>>4)*0x10) =  \
                                    (*(volatile unsigned short *)(GPIO_PULLSEL_BASE + (_n>>4)*0x10) & (~(0x1 <<(_n&0xF)))) | (sel<<(_n&0xF))

#define GPIO_SET_MODE(_n,mode)      (*(volatile unsigned short *)(GPIO_MODE_BASE + (_n/5)*0x10)) =  \
                                    (*(volatile unsigned short *)(GPIO_MODE_BASE + (_n/5)*0x10)) & ((~(0x7 <<(3*(_n%0x5))))) | (mode<<(3*(_n%0x5)))

void kpd_setting_config(void);

/* Keypad registers */
#define KP_STA		(KP_BASE + 0x0000)
#define KP_MEM1		(KP_BASE + 0x0004)
#define KP_MEM2		(KP_BASE + 0x0008)
#define KP_MEM3		(KP_BASE + 0x000c)
#define KP_MEM4		(KP_BASE + 0x0010)
#define KP_MEM5		(KP_BASE + 0x0014)
#define KP_DEBOUNCE	(KP_BASE + 0x0018)

#define VRTC_CON1 (0xF7000000 + 0x2f62c)

#define KPD_DEBOUNCE_MASK	((1U << 14) - 1)
#define KPD_KEY_DEBOUNCE  1024      /* (val / 32) ms */

#define KPD_SAY		"[kpd_udvt]: "

struct udvt_cmd {
	int cmd;
	int value;
};

struct udvt_gpio {
	int gpio_num;
	int gpio_mode;
};

#define KPD_GPIO_NUM (14) /* col: 8, row: 8 */
static struct udvt_gpio kpd_gpio[KPD_GPIO_NUM] = {
	{98, 1}, /* KROW0 */	
	{97, 0}, /* KROW1 */	
	{95, 0}, /* KROW2 */
	{99, 0}, /* KROW3 */
	{104, 0}, /* KROW4 */
	{106, 0}, /* KROW5 */
	{110, 0}, /* KROW6 */
	{107, 0}, /* KROW7 */
	{103, 1}, /* KCOL0 */
	{108, 1}, /* KCOL1 */
	{105, 0}, /* KCOL2 */
	{101, 0}, /* KCOL3 */
	{102, 0}, /* KCOL4 */
	{100, 0}, /* KCOL5 */
	{109, 0}, /* KCOL6 */
	{96, 0}, /* KCOL7 */
};

//typedef  unsigned short u16;
#define GET_DEBOUNCE (1)
#define SET_DEBOUNCE (2)
#define GET_STATUS (3)  
#define SET_DEFAULT_MODE (4)
#define SET_RESTORE_MODE (5)
#define GET_KPD_MEM3 (6)
#define SET_KPD_SLEEP_MODE (7)
#define GPIO_DIR2_SET 	(GPIO_BASE + 0x0024)
#define GPIO_PULL2_EN_SET	(GPIO_BASE + 0x0224)
#define GPIO_PULL2_SEL_SET	(GPIO_BASE + 0x0424)
#define GPIO_MODE_SET7	(GPIO_BASE + 0x0C74)
#define GPIO_MODE_SET9	(GPIO_BASE + 0x0C94)


/*****************************************new add function*****************************************/  
#define  GPO_ADD 0xF0001E84
#define  GPO_DIR_ADD 0xF0001E88

int kpd_check_single_key(u16 column);
int kpd_check_double_key(u16 column);


static inline void kpd_get_keymap_state(u16 state[])
{
	state[0] = *(volatile u16 *)KP_MEM1;
	state[1] = *(volatile u16 *)KP_MEM2;
	state[2] = *(volatile u16 *)KP_MEM3;
	state[3] = *(volatile u16 *)KP_MEM4;
} 
             
static u16 kpd_keymap_state[4] = {
	0xffff, 0xffff, 0xffff, 0xffff
};



/* Function: fpga_kpd_auto_test
 * par: count stands for test time 
 */
int fpga_kpd_auto_test(int count) 
{
		u16 a;
		int test_time, ret;
		test_time = 0;
		
	 *(volatile u32 *)(GPO_ADD) = 0xFF;
	 *(volatile u32 *)(GPO_DIR_ADD) = 0x01;
	 
do{ 
	/*
	* signal key test for pulling one gpio
	*/
	for(a = 0;a < 8;a++){
	 		*(volatile u32 *)(GPO_ADD) = (0xFF^(0x01 << a));
	 		mdelay(1000);	 		
	 		ret = kpd_check_single_key(a);	
	 		if(ret != 0)
	 			{
	 				printk(KPD_SAY "test fail\n");
	 				break;
	 			}
			*(volatile u32 *)(GPO_ADD) = 0xFF;
			mdelay(1000);
			ret = kpd_check_single_key(a);
			if(ret != 0)
	 			{
	 				printk(KPD_SAY "test fail\n");
	 				break;
	 			}
		}
	/*
	* double key test for pulling two gpio
	*/	
	for(a = 0;a < 7; a++){
			*(volatile u32 *)(GPO_ADD) = (0xFF^(0x11 << a));
	 		mdelay(1000);
			ret = kpd_check_double_key(a);
			if(ret != 0)
	 			{
	 				printk(KPD_SAY "test fail\n");
	 				break;
	 			}
			*(volatile u32 *)(GPO_ADD) = 0xFF;
			mdelay(1000);
			ret = kpd_check_double_key(a);
			if(ret != 0)
	 			{
	 				printk(KPD_SAY "test fail\n");
	 				break;
	 			}
		}	 		
	 	test_time++;
	}while(test_time <= count);
	
	 *(volatile u32 *)(GPO_ADD) = 0xFF;
	 *(volatile u32 *)(GPO_DIR_ADD) = 0x01;	
	
	return 0;
			
}

int kpd_check_single_key(u16 column)
{
		u16 i, j;
		int b; 
		bool pressed;
		u16 new_state[4], change, mask;
		u16 hw_keycode, gen_keycode;
		kpd_get_keymap_state(new_state);
		
		b = 0;	 		
		for (i = 0; i < 4; i++) {
		change = new_state[i] ^ kpd_keymap_state[i];
		if (!change)
			continue;
		for (j = 0; j < 16; j++) {
			mask = 1U << j;
			if (!(change & mask))
				continue;
			hw_keycode = (i << 4) + j;
			/* bit is 1: not pressed, 0: pressed */
			pressed = !(new_state[i] & mask);
			gen_keycode = 8*b + column;
			b++;
			if(gen_keycode != hw_keycode)
				{printk(KPD_SAY "(%s) HW keycode = %u, gen_keycode = %u, b = %d\n",
				       pressed ? "pressed" : "released",
				       hw_keycode,gen_keycode,b);
				return -1;
				}
			printk(KPD_SAY "(%s) HW keycode = %u, b = %d\n",
				       pressed ? "pressed" : "released",
				       hw_keycode,b);
		}
	}
	return 0;
}

int kpd_check_double_key(u16 column)
{
		u16 i, j, k;
		int b;
		bool pressed;
		u16 new_state[4], change, mask;
		u16 hw_keycode, gen_keycode;
		kpd_get_keymap_state(new_state);
		
		b = 0;
		k = 0; 		
		for (i = 0; i < 4; i++) {
		change = new_state[i] ^ kpd_keymap_state[i];
		if (!change)
			continue;
		for (j = 0; j < 16; j++) {
			mask = 1U << j;
			if (!(change & mask))
					continue;
			hw_keycode = (i << 4) + j;
			/* bit is 1: not pressed, 0: pressed */
			pressed = !(new_state[i] & mask);
			if((k = 0))
			{
				gen_keycode = 8*b + column;
				k = 1;
			}	
			if((k = 1))
			{
				gen_keycode = 8*b + column + k;
				k = 0;
				b++;
			}
			
			if(gen_keycode != hw_keycode)
				{printk(KPD_SAY "(%s) HW keycode = %u, gen_keycode = %u, b = %d\n",
				       pressed ? "pressed" : "released",
				       hw_keycode,gen_keycode,b);
				return -1;
				}
			printk(KPD_SAY "(%s) HW keycode = %u, b = %d\n",
				       pressed ? "pressed" : "released",
				       hw_keycode,b);
		}
	}
	return 0;
}
/**********************************************************************************************************/



static void kpd_set_config(void)
{
	int i = 0, col_num = 7;
	//for stable Col0, else still can check keys pressed, set USBDL_EN bit 
	//unsigned short value = *(volatile unsigned short *)(0x7002f62c);
	//*(volatile unsigned short *)(0x7002f62c) = value&(~(0x1<<5));
		/* KROW0 ~ KROW7: output + pull disable + pull down */
	for(i = 0; i < 8; i++)
	{
		mt_set_gpio_mode(kpd_gpio[i].gpio_num, 1);
		mt_set_gpio_dir(kpd_gpio[i].gpio_num, 1);
		mt_set_gpio_pull_enable(kpd_gpio[i].gpio_num, 0);
		mt_set_gpio_pull_select(kpd_gpio[i].gpio_num, 0);
	}	
	
	/* KCOL0 ~ KCOL7: input + pull enable + pull up */
	for(i = col_num; i < 8 + col_num; i++)
	{
		mt_set_gpio_mode(kpd_gpio[i].gpio_num, 1);
		mt_set_gpio_dir(kpd_gpio[i].gpio_num, 0);
		mt_set_gpio_pull_enable(kpd_gpio[i].gpio_num, 1);
		mt_set_gpio_pull_select(kpd_gpio[i].gpio_num, 1);
	}
}

static int kpd_get_config(void)
{
	int i;
	
	for(i = 0; i < KPD_GPIO_NUM; i++)
	{
		if(mt_get_gpio_mode(kpd_gpio[i].gpio_num) != kpd_gpio[i].gpio_mode)
		{
			printk(KPD_SAY" error default value. gpio_num:%d, gpio_mode:%d\n", kpd_gpio[i].gpio_num, mt_get_gpio_mode(kpd_gpio[i].gpio_num));
			return -1;
		}
	}
	return 0;
} 

static inline u16 kpd_get_debounce(void)
{
	u16 val;;
	val = *(volatile u16 *)KP_DEBOUNCE;
	return (val & KPD_DEBOUNCE_MASK);
}

static inline void kpd_set_debounce(u16 val)
{
	*(volatile u16 *)KP_DEBOUNCE = (u16)(val & KPD_DEBOUNCE_MASK);
}

static inline u16 kpd_get_status(void)
{
	u16 val;;
	val = *(volatile u16 *)KP_STA;
	return val&0x01?0:-1;
	
}

long kpd_udvt_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	//void __user *uarg = (void __user *)arg;
	//u16 temp;
		
  struct udvt_cmd *pcmd = (struct udvt_cmd *)arg;
  //printk("cmd:%d, value:0x%x\n", pcmd->cmd, pcmd->value);
  switch(pcmd->cmd) {
  	case GET_DEBOUNCE:
  		pcmd->value = kpd_get_debounce();
  		break;
  	case SET_DEBOUNCE:
  		kpd_set_debounce(pcmd->value);
  		break;
  	case GET_STATUS:
  		pcmd->value = kpd_get_status();
  		break;
	case SET_DEFAULT_MODE:
		mt_set_gpio_mode(98, 0);		
		mt_set_gpio_mode(103, 0);
		break;
	case SET_RESTORE_MODE:
	  mt_set_gpio_mode(98, 1);		
		mt_set_gpio_mode(103, 1);
		break;	
	case  GET_KPD_MEM3:	
		pcmd->value =  *(volatile u16 *)KP_MEM3;
		break;
	case SET_KPD_SLEEP_MODE:
		//sc_set_wake_src(WAKE_SRC_KP,MT6577_KP_IRQ_ID);
		break;
  	default:
  		return -EINVAL;										
  }
  return 0;
}

static int kpd_udvt_dev_open(struct inode *inode, struct file *file)
{
	//kpd_setting_config();
	return 0;
}

static struct file_operations kpd_udvt_dev_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= kpd_udvt_dev_ioctl,
	.open		= kpd_udvt_dev_open,
};

static struct miscdevice kpd_udvt_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= KPD_NAME,
	.fops	= &kpd_udvt_dev_fops,
};

static int __init kpd_udvt_mod_init(void)
{
	int r;
	//u16 temp;
#if 0	
	temp =  *(volatile u16 *)VRTC_CON1;	
	temp &=~(0x1<<5);
	*(volatile u16 *)VRTC_CON1 = (u16)temp;
#endif	
	r = misc_register(&kpd_udvt_dev);
	if (r) {
		printk(KPD_SAY "register driver failed (%d)\n", r);
		return r;
	}

	return 0;
}

/* should never be called */
static void __exit kpd_udvt_mod_exit(void)
{
	int r;
	
	r = misc_deregister(&kpd_udvt_dev);
	if(r){
		printk(KPD_SAY"unregister driver failed\n");
	}
}

module_init(kpd_udvt_mod_init);
module_exit(kpd_udvt_mod_exit);

//module_param(kpd_show_hw_keycode, bool, 0644);
//module_param(kpd_show_register, bool, 0644);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("MT6573 Keypad (KPD) Driver v0.2");
MODULE_LICENSE("GPL");
