
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

#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/tcm.h>

#include <mach/mt6575_reg_base.h>
#include <mach/irqs.h>

#define KPD_DEBUG	1

#define KPD_NAME	"kpd_udvt"
#define KPD_SAY		"[kpd_udvt]: "

#if KPD_DEBUG
#define kpd_print(fmt, arg...)	printk(KPD_SAY fmt, ##arg)
#else
#define kpd_print(fmt, arg...)	do {} while (0)
#endif

/* Keypad registers */
#define KP_STA		(KP_BASE + 0x0000)
#define KP_MEM1		(KP_BASE + 0x0004)
#define KP_MEM2		(KP_BASE + 0x0008)
#define KP_MEM3		(KP_BASE + 0x000c)
#define KP_MEM4		(KP_BASE + 0x0010)
#define KP_MEM5		(KP_BASE + 0x0014)
#define KP_DEBOUNCE	(KP_BASE + 0x0018)

#define KPD_DEBOUNCE_MASK	((1U << 14) - 1)
#define KPD_KEY_DEBOUNCE  1024      /* (val / 32) ms */

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

unsigned short kpd_mem_default[] = {0xffff, 0xffff, 0xffff, 0xffff, 0xff};
//typedef  unsigned short u16;
#define GET_DEBOUNCE (1)
#define SET_DEBOUNCE (2)
#define GET_STATUS (3)
#define SET_GPIO (4)
#define GET_GPIO (5)  
#define GET_KPD_MEM (6)

extern s32 mt_set_gpio_pull_enable(u32 pin, u32 enable);
extern s32 mt_set_gpio_dir(u32 pin, u32 dir);
extern s32 mt_set_gpio_mode(u32 pin, u32 mode);
extern s32 mt_set_gpio_pull_select(u32 pin, u32 select);
extern s32 mt_get_gpio_mode(u32 pin);

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

static int get_kpd_mem(void)
{
	int i;
	
	for(i = 0; i < sizeof(kpd_mem_default)/sizeof(short); i++)
	{
		if((*(volatile u16 *)(KP_MEM1+4*i)&kpd_mem_default[i]) != kpd_mem_default[i])
		{
			kpd_print("KP_MEM%d: expect: 0x%x, fact:0x%x", kpd_mem_default[i], *(volatile u16 *)(KP_MEM1+4*i));
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

static long kpd_udvt_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
		
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
  	case SET_GPIO:
  		kpd_set_config();
  		break;
  	case GET_GPIO:
  		pcmd->value = kpd_get_config();
  		break;
  	case GET_KPD_MEM:
  		pcmd->value = get_kpd_mem();
  		break;			
  	default:
  		return -EINVAL;										
  }
  return 0;
}

static int kpd_udvt_dev_open(struct inode *inode, struct file *file)
{
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
