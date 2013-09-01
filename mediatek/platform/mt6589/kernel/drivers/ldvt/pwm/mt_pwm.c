
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/autoconf.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/mt_pwm.h>
//include "mt_pwm.h"
#include <mach/mt_typedefs.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_gpio.h>
#include <mach/irqs.h>
#include <mach/mt_pm_ldo.h>

#define PWM_DEBUG
#ifdef PWM_DEBUG
	#define PWMDBG(fmt, args ...) printk(KERN_INFO "pwm %5d: " fmt, __LINE__,##args)
#else
	#define PWMDBG(fmt, args ...)
#endif

#define PWMMSG(fmt, args ...)  printk("<0>" fmt, ##args)

#define PWM_DEVICE "mt-pwm"

static enum {
	PWM_CON,
	PWM_HDURATION,
	PWM_LDURATION,
	PWM_GDURATION,
	PWM_BUF0_BASE_ADDR,
	PWM_BUF0_SIZE,
	PWM_BUF1_BASE_ADDR,
	PWM_BUF1_SIZE,
	PWM_SEND_DATA0,
	PWM_SEND_DATA1,
	PWM_WAVE_NUM,
	PWM_DATA_WIDTH,      
	PWM_THRESH,         
	PWM_SEND_WAVENUM,
	PWM_VALID
}PWM_REG_OFF;

static U32 PWM_register[PWM_NUM]={
	(PWM_BASE+0x0010),     //PWM0 REGISTER BASE,   15 registers
	(PWM_BASE+0x0050),     //PWM1 register base    15 registers
	(PWM_BASE+0x0090),     //PWM2 register base    15 registers
	(PWM_BASE+0x00d0),     //PWM3 register base    13 registers
	(PWM_BASE+0x0110),     //PWM4 register base    13 registers
	(PWM_BASE+0x0150),     //PWM5 register base    13 registers
	(PWM_BASE+0x0190)      //PWM6 register base    15 registers
};

struct pwm_device {
	const char      *name;
	atomic_t        ref;
	dev_t           devno;
	spinlock_t      lock;
	struct device   dev;
	struct miscdevice *miscdev;
};

static struct pwm_device pwm_dat = {
	.name = PWM_DEVICE,
	.ref = ATOMIC_INIT(0),
//	.lock = SPIN_LOCK_UNLOCKED
	.lock = __SPIN_LOCK_UNLOCKED(die.lock),
};

static struct pwm_device *pwm_dev = &pwm_dat;

int mt6575_pwm_open(struct inode *node, struct file *file)
{
	if (!pwm_dev) {
		PWMDBG("device is invalid.\n");
		return -EBADADDR;
	}

	atomic_inc(&pwm_dev->ref);
	file->private_data = pwm_dev;
	return nonseekable_open(node, file);
}

int mt6575_pwm_release(struct inode *node, struct file *file)
{
	if (!pwm_dev) {
		PWMDBG("device is invalid.\n");
		return -EBADADDR;
	}

	atomic_dec(&pwm_dev->ref);
	return RSUCCESS;
}

struct file_operations mt6575_pwm_fops={
	.owner = THIS_MODULE,
	.open=mt6575_pwm_open,
	.release=mt6575_pwm_release,
};

static struct miscdevice mt6575_pwm_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "mt6575_pwm",
	.fops = &mt6575_pwm_fops,
};

static int mt_pwm_open(struct inode *inode, struct file *file)
{
	return 0;
}

#define PWM_UVVF_FIFO_INTERRUPT 0x0631
#define PWM_UVVF_RANDOM_INTERRUPT 0x0632
#define PWM_UVVF_FIFO_WAVEFORM 0x0633
#define PWM_UVVF_FIFO_STOPBIT 0x0634
#define PWM_UVVF_FIFO_HLGDUR 0x0635
#define PWM_UVVF_FIFO_GIOUT 0x0636
#define PWM_UVVF_MEMO_WAVEFORM 0x0637
#define PWM_UVVF_MEMO_INTERRUPT 0x0638
#define PWM_UVVF_OLD_INTERRUPT 0x0639
#define PWM_UVVF_FIFO_INTERRUPT_MULTI 0x0640
#define PWM_UVVF_REGISTER_WR 0x0641
#define PWM_UVVF_MEMO_STOPBIT 0x0642
#define PWM_UVVF_MEMO_HLGDUR 0x0643
#define PWM_UVVF_MEMO_GIOUT 0x0644
#define PWM_UVVF_OLD_WAVEFORM 0x0645
#define PWM_UVVF_OLD_WAVEFORM_32K 0x0654

#define PWM_UVVF_OLD_THRESH 0x0646
#define PWM_UVVF_OLD_DTDUR 0x0647
#define PWM_UVVF_OLD_GIOUT 0x0648
#define PWM_UVVF_RANDOM_WAVEFORM 0654
#define PWM_UVVF_RANDOM_HLDUR 0x0650
#define PWM_UVVF_RANDOM_IOUT 0x0651
#define PWM_UVVF_SEQ 0x0652
#define PWM_UVVF_MEMO_DCM 0x0653
#define PWM_UVVF_AUTO 0x0655

/*
#define PWM_UVVF_FIFO_INTERRUPT 0x0631
#define PWM_UVVF_RANDOM_INTERRUPT 0x0632
#define PWM_UVVF_FIFO_WAVEFORM 0x0633
#define PWM_UVVF_FIFO_STOPBIT 0x0634
#define PWM_UVVF_FIFO_HLGDUR 0x0635
#define PWM_UVVF_FIFO_GIOUT 0x0636
#define PWM_UVVF_MEMO_WAVEFORM 0x0637
#define PWM_UVVF_MEMO_INTERRUPT 0x0638
#define PWM_UVVF_OLD_INTERRUPT 0x0639
#define PWM_UVVF_FIFO_INTERRUPT_MULTI 0x0640
#define PWM_UVVF_REGISTER_WR 0x0641
#define PWM_UVVF_MEMO_STOPBIT 0x0642
#define PWM_UVVF_MEMO_HLGDUR 0x0643
#define PWM_UVVF_MEMO_GIOUT 0x0644
#define PWM_UVVF_OLD_WAVEFORM 0x0645
#define PWM_UVVF_OLD_WAVEFORM_32K 0x0646
#define PWM_UVVF_OLD_THRESH 0x0647
#define PWM_UVVF_OLD_DTDUR 0x0648
#define PWM_UVVF_OLD_GIOUT 0x0649
#define PWM_UVVF_RANDOM_WAVEFORM 0x650
#define PWM_UVVF_RANDOM_HLDUR 0x0651
#define PWM_UVVF_RANDOM_IOUT 0x0652
#define PWM_UVVF_SEQ 0x0653
#define PWM_UVVF_MEMO_DCM 0x0654
#define PWM_UVVF_AUTO 0x0655
*/
//panghaowei add begin
S32 mt_pmic_init()
{
	u16 pmic_select;
	SETREG32(PWM_BASE+0x2D4, 7);
	pmic_select = INREG32(PWM_BASE+0x2D4);
	printk("ADDR==0x%x, pmic_select==0x%x\n", PWM_BASE+0x2D4, pmic_select);
	printk("<0>""==========mt_pmic_init========\n");	
}

static long mt_pwm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *uarg = (void __user *)arg;
	struct pwm_spec_config conf;
	//add by ranran.lu
	int i;
	s32 retval;
	static struct pwm_spec_config conf_old_mode= {
		.pwm_no = PWM0,
		.clk_div = CLK_DIV1,
	};
	//end add
	struct pwm_spec_config confseq[5];
	u32 *membuff;
	u8 *membuff0, *membuff1;
	u8* virt;
	dma_addr_t phys;

	u32 loopcnt;
	u32 pwm_no;
	u32 testregvalue = 0;
	u32 readregvalue;
	u32 statusflag = 0;
	u32 originalvalue;
	
	u32 hi_cnt = 0;
	u32 lo_cnt = 0;
	unsigned long cnt = 0;
	//u64 cnt = 0;
	u32* cnt_p = NULL;
  s32 read, read_old;
	unsigned long jiffies_old = 0;
	//u64 jiffies_old = 0;

//	PWM_SET_BITS(0x3, 0xf702fc80);
//	PWM_CLR_BITS(0x2, 0xf702fc80);
	static int iii=0;
	if (iii == 0) {
//		mt_power_on(PWM1);mt_power_on(PWM2);mt_power_on(PWM3);mt_power_on(PWM7);mt_power_on(PWM4);
		iii = 1;
	}

//	mt_set_pwm_eco();

//add by panghaowei
#if 0
	u16 pmic_select;
	SETREG32(PWM_BASE+0x2D4, 7);
	pmic_select = INREG32(PWM_BASE+0x2D4);
	printk("ADDR==0x%x, pmic_select==0x%x\n", PWM_BASE+0x2D4, pmic_select);
#endif
//end add
	switch ( arg ) {
		case PWM_UVVF_MEMO_DCM:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);

//		PWM_SET_BITS((0x1<<7), PDN_SET0);
//		printk(KERN_INFO "Power register:\nCG %x\n",(__raw_readl(0xf7026310)));

		membuff = virt;

		membuff[0] = 0xaaaaaaaa;
		membuff[1] = 0x0;
		membuff[2] = 0x0;
		membuff[3] = 0xffffffff;

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_MEMORY;
		conf.clk_div = CLK_DIV128;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		conf.intr = TRUE;
		conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_MEMORY_REGS.HDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.LDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.GDURATION = 0;
		conf.PWM_MODE_MEMORY_REGS.BUF0_BASE_ADDR = phys;
		conf.PWM_MODE_MEMORY_REGS.BUF0_SIZE = 3;
		conf.PWM_MODE_MEMORY_REGS.WAVE_NUM = 0;

		while(1)
		{
//			dcm_disable_all();
//			dcm_enable_all();

			printk(KERN_INFO "PWM: clk_div = %x, clk_src = %x, pwm_no = %x\n", conf.clk_div, conf.clk_src, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.clk_div --;
			if(CLK_DIV_MIN == conf.clk_div)
			{
				conf.clk_div = CLK_DIV128;
				conf.clk_src --;
			}
			if(PWM_CLK_OLD_MODE_32K == conf.clk_src)
			{
				conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

//		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_FIFO_INTERRUPT:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_FIFO;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = TRUE;
		conf.PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
		conf.PWM_MODE_FIFO_REGS.HDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.LDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.GDURATION = 0;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0x11111111;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xffffffff;
		conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 11;

//		pwm_set_spec_config(&conf);

		while(1)
		{
			pwm_set_spec_config(&conf);

			conf.pwm_no ++;
			if(conf.pwm_no == PWM_MAX)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}
		break;

	case PWM_UVVF_RANDOM_INTERRUPT:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff0 = virt + 1024;
		membuff1 = virt + 2048;
		memset(membuff0, 0x55555555, 1024);
		memset(membuff1, 0x11111111, 1024);

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_RANDOM;
//			conf.clk_div = CLK_DIV1;
			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
			conf.clk_div = CLK_DIV128;
//			conf.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		conf.intr = TRUE;
		conf.PWM_MODE_RANDOM_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_RANDOM_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_RANDOM_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_RANDOM_REGS.HDURATION = 1;
		conf.PWM_MODE_RANDOM_REGS.LDURATION = 1;
		conf.PWM_MODE_RANDOM_REGS.GDURATION = 0;
		conf.PWM_MODE_RANDOM_REGS.BUF0_BASE_ADDR = phys + 1024;
		conf.PWM_MODE_RANDOM_REGS.BUF0_SIZE = 255;
		conf.PWM_MODE_RANDOM_REGS.BUF1_BASE_ADDR = phys + 2048;
		conf.PWM_MODE_RANDOM_REGS.BUF1_SIZE = 255;
		conf.PWM_MODE_RANDOM_REGS.WAVE_NUM = 0;
		conf.PWM_MODE_RANDOM_REGS.VALID = 0;

		while(1)
		{
//				PWMDBG("PWM finish test prepare\n");
//				pwm_set_spec_config(&conf);
//				PWMDBG("PWM finish test start\n");
			//add by ranran.lu
//				for(i=50;i>0;i--){
//					retval = mt_get_intr_status(PWM0_INT_FINISH_ST + conf.pwm_no*2 );
//					PWMDBG ("PWM%d finish irq status is %d\n", conf.pwm_no, retval);
//					retval = mt_get_intr_status(PWM0_INT_UNDERFLOW_ST + conf.pwm_no*2 );
//					PWMDBG ("PWM%d underflow irq status is %d\n", conf.pwm_no, retval);
//					msleep(100);
//				}
			//add end
//				msleep(5000);
//				conf.PWM_MODE_RANDOM_REGS.BUF0_BASE_ADDR = NULL;
//				conf.PWM_MODE_RANDOM_REGS.BUF1_BASE_ADDR = NULL;




			PWMDBG("PWM underflow test prepare\n");
			pwm_set_spec_config(&conf);
			PWMDBG("PWM underflow test start\n");
//				msleep(5000);
			//add by ranran.lu
//				retval = mt_get_intr_status(PWM0_INT_UNDERFLOW_ST + conf.pwm_no*2 );
//				PWMDBG ("PWM%d underflow irq status is %d\n", conf.pwm_no, retval);
			//add end
			conf.pwm_no ++;

//				conf.PWM_MODE_RANDOM_REGS.BUF0_BASE_ADDR = phys + 1024;
//				conf.PWM_MODE_RANDOM_REGS.BUF1_BASE_ADDR = phys + 2048;

			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_FIFO_WAVEFORM:

#if 0
	//enable pmic to do pinmux test
	hwPowerOn(MT65XX_POWER_LDO_VMC1, VOL_DEFAULT, "pwm test");
	hwPowerOn(MT65XX_POWER_LDO_VMCH1,VOL_DEFAULT, "pwm test");
#endif

#if 1
	//set PMIC GPIO mode to PWM
	mt_set_gpio_mode(GPIOEXT32,GPIO_MODE_01);
	mt_set_gpio_mode(GPIOEXT33,GPIO_MODE_01);
	mt_set_gpio_mode(GPIOEXT34,GPIO_MODE_01);
#endif

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_FIFO;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
//			conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
		conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;

		conf.PWM_MODE_FIFO_REGS.HDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.LDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.GDURATION = 0;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0xaaaaaaaa;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xaaaaaaaa;
		conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: clk_div = %x, clk_src = %x, pwm_no = %x\n", conf.clk_div, conf.clk_src, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.clk_div ++;
			if(CLK_DIV_MAX == conf.clk_div)
			{
				conf.clk_div = CLK_DIV1;
				conf.clk_src ++;
			}
			if(PWM_CLK_SRC_NUM == conf.clk_src)
			{
				conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}

		}

		break;

	case PWM_UVVF_FIFO_STOPBIT:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_FIFO;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
		conf.PWM_MODE_FIFO_REGS.HDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.LDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.GDURATION = 0;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0x0000ff11;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xffffffff;
		conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: stop_bitpos_value = %x, pwm_no = %x\n", conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE >>= 1;
			if(conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE < 7)
			{
				conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		break;

	case PWM_UVVF_FIFO_HLGDUR:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_FIFO;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
		conf.PWM_MODE_FIFO_REGS.HDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.LDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.GDURATION = 0;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0x71111111;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xffffffff;
		conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: gduration = %x, lduration = %x, hduration = %x, pwm_no = %x\n", conf.PWM_MODE_FIFO_REGS.GDURATION, conf.PWM_MODE_FIFO_REGS.LDURATION, conf.PWM_MODE_FIFO_REGS.HDURATION, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.PWM_MODE_FIFO_REGS.GDURATION += 8;
			if(conf.PWM_MODE_FIFO_REGS.GDURATION > 9)
			{
				conf.PWM_MODE_FIFO_REGS.GDURATION = 0;
				conf.PWM_MODE_FIFO_REGS.LDURATION += 8;
			}
			if(conf.PWM_MODE_FIFO_REGS.LDURATION > 9)
			{
				conf.PWM_MODE_FIFO_REGS.LDURATION = 1;
				conf.PWM_MODE_FIFO_REGS.HDURATION += 3;
			}
			if(conf.PWM_MODE_FIFO_REGS.HDURATION > 4)
			{
				conf.PWM_MODE_FIFO_REGS.HDURATION = 1;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		break;

	case PWM_UVVF_FIFO_GIOUT:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_FIFO;
		conf.clk_div = CLK_DIV1;//CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
		conf.PWM_MODE_FIFO_REGS.HDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.LDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.GDURATION = 8;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0x11111111;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xffffffff;
		conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: guard_value = %x, idle_value = %x, wave_num = %x, pwm_no = %x\n", conf.PWM_MODE_FIFO_REGS.GUARD_VALUE, conf.PWM_MODE_FIFO_REGS.IDLE_VALUE, conf.PWM_MODE_FIFO_REGS.WAVE_NUM, conf.pwm_no);
			pwm_set_spec_config(&conf);

			if(0 == conf.PWM_MODE_FIFO_REGS.WAVE_NUM)
				conf.PWM_MODE_FIFO_REGS.GUARD_VALUE ++;
			else
				conf.PWM_MODE_FIFO_REGS.IDLE_VALUE ++;

			if(GUARD_MAX == conf.PWM_MODE_FIFO_REGS.GUARD_VALUE)
			{
				conf.PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
				conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 1;
			}
			if(IDLE_MAX == conf.PWM_MODE_FIFO_REGS.IDLE_VALUE)
			{
				conf.PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
				conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;
				conf.pwm_no ++;
			}

			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		break;

	case PWM_UVVF_MEMO_WAVEFORM:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff = virt;

		membuff[0] = 0xaaaaaaaa;
		membuff[1] = 0x0;
		membuff[2] = 0x0;
		membuff[3] = 0xfff00fff;

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_MEMORY;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_MEMORY_REGS.HDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.LDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.GDURATION = 0;
		conf.PWM_MODE_MEMORY_REGS.BUF0_BASE_ADDR = phys;
		conf.PWM_MODE_MEMORY_REGS.BUF0_SIZE = 3;
		conf.PWM_MODE_MEMORY_REGS.WAVE_NUM = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: clk_div = %x, clk_src = %x, pwm_no = %x\n", conf.clk_div, conf.clk_src, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.clk_div ++;
			if(CLK_DIV_MAX == conf.clk_div)
			{
				conf.clk_div = CLK_DIV1;
				conf.clk_src ++;
			}
			if(PWM_CLK_SRC_NUM == conf.clk_src)
			{
				conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_MEMO_INTERRUPT:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff = virt;

		membuff[0] = 0xaaaaaaaa;
		membuff[1] = 0x0;
		membuff[2] = 0x0;
		membuff[3] = 0xffffffff;

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_MEMORY;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = TRUE;
		conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_MEMORY_REGS.HDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.LDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.GDURATION = 0;
		conf.PWM_MODE_MEMORY_REGS.BUF0_BASE_ADDR = phys;
		conf.PWM_MODE_MEMORY_REGS.BUF0_SIZE = 15;
		conf.PWM_MODE_MEMORY_REGS.WAVE_NUM = 1;

		while(1)
		{
			pwm_set_spec_config(&conf);

			conf.pwm_no ++;
			if(conf.pwm_no == PWM_MAX)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

//		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_OLD_INTERRUPT:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_OLD;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_OLD_MODE_BLOCK;
		conf.intr = TRUE;
		conf.PWM_MODE_OLD_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_OLD_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_OLD_REGS.GDURATION = 0;
		conf.PWM_MODE_OLD_REGS.DATA_WIDTH = 100;
		conf.PWM_MODE_OLD_REGS.THRESH = 25;
		conf.PWM_MODE_OLD_REGS.WAVE_NUM = 1;

		while(1)
		{
			pwm_set_spec_config(&conf);

			conf.pwm_no ++;
			if(PWM4 == conf.pwm_no || PWM5 == conf.pwm_no || PWM6 == conf.pwm_no)
				conf.pwm_no ++;
			if(conf.pwm_no == PWM_MAX)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		break;

	case PWM_UVVF_FIFO_INTERRUPT_MULTI:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_FIFO;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = TRUE;
		conf.PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
		conf.PWM_MODE_FIFO_REGS.HDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.LDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.GDURATION = 0;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0x11111111;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xffffffff;
		conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 1;

		while(1)
		{
			pwm_set_spec_config(&conf);

			conf.pwm_no ++;
			if(conf.pwm_no == PWM_MAX)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}
		break;

	case PWM_UVVF_REGISTER_WR:

		testregvalue = 0x0;
		for(loopcnt = 0; loopcnt < 2; loopcnt++)
		{
			originalvalue = __raw_readl(PWM_ENABLE);
			__raw_writel(testregvalue, PWM_ENABLE);
			readregvalue = __raw_readl(PWM_ENABLE);
			if((readregvalue & 0x3007f) != (testregvalue & 0x3007f))
			{
				statusflag ++;
				printk(KERN_INFO "PWM: PWM_ENABLE fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x3007f));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x3007f));
			}
			__raw_writel(originalvalue, PWM_ENABLE);

			originalvalue = __raw_readl(PWM4_DELAY);
			__raw_writel(testregvalue, PWM4_DELAY);
			readregvalue = __raw_readl(PWM4_DELAY);
			if((readregvalue & 0x1ffff) != (testregvalue & 0x1ffff))
			{
				statusflag ++;
				printk(KERN_INFO "PWM: PWM4_DELAY fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x1ffff));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x1ffff));
			}
			__raw_writel(originalvalue, PWM4_DELAY);

			originalvalue = __raw_readl(PWM5_DELAY);
			__raw_writel(testregvalue, PWM5_DELAY);
			readregvalue = __raw_readl(PWM5_DELAY);
			if((readregvalue & 0x1ffff) != (testregvalue & 0x1ffff))
			{
				statusflag ++;
				printk(KERN_INFO "PWM: PWM5_DELAY fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x1ffff));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x1ffff));
			}
			__raw_writel(originalvalue, PWM5_DELAY);

			originalvalue = __raw_readl(PWM3_DELAY);
			__raw_writel(testregvalue, PWM3_DELAY);
			readregvalue = __raw_readl(PWM3_DELAY);
			if((readregvalue & 0x1ffff) != (testregvalue & 0x1ffff))
			{
				statusflag ++;
				printk(KERN_INFO "PWM: PWM3_DELAY fails.\n");
				printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x1ffff));
				printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x1ffff));
			}
			__raw_writel(originalvalue, PWM3_DELAY);

			for(pwm_no=0; pwm_no<7; pwm_no++)
			{
				originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_HDURATION);
				__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_HDURATION);
				readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_HDURATION);
				if((readregvalue & 0xffff) != (testregvalue & 0xffff))
				{
					statusflag ++;
					printk(KERN_INFO "PWM(%x): PWM_HDURATION fails.\n", pwm_no);
					printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffff));
					printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffff));
				}
				__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_HDURATION);

				originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_LDURATION);
				__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_LDURATION);
				readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_LDURATION);
				if((readregvalue & 0xffff) != (testregvalue & 0xffff))
				{
					statusflag ++;
					printk(KERN_INFO "PWM(%x): PWM_LDURATION fails.\n", pwm_no);
					printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffff));
					printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffff));
				}
				__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_LDURATION);

				originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_GDURATION);
				__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_GDURATION);
				readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_GDURATION);
				if((readregvalue & 0xffff) != (testregvalue & 0xffff))
				{
					statusflag ++;
					printk(KERN_INFO "PWM(%x): PWM_GDURATION fails.\n", pwm_no);
					printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffff));
					printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffff));
				}
				__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_GDURATION);

				originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_BUF0_BASE_ADDR);
				__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_BUF0_BASE_ADDR);
				readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_BUF0_BASE_ADDR);
				if((readregvalue & 0xffffffff) != (testregvalue & 0xffffffff))
				{
					statusflag ++;
					printk(KERN_INFO "PWM(%x): PWM_BUF0_BASE_ADDR fails.\n", pwm_no);
					printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffffffff));
					printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffffffff));
				}
				__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_BUF0_BASE_ADDR);

				originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_BUF0_SIZE);
				__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_BUF0_SIZE);
				readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_BUF0_SIZE);
				if((readregvalue & 0xffff) != (testregvalue & 0xffff))
				{
					statusflag ++;
					printk(KERN_INFO "PWM(%x): PWM_BUF0_SIZE fails.\n", pwm_no);
					printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffff));
					printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffff));
				}
				__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_BUF0_SIZE);

				originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_BUF1_BASE_ADDR);
				__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_BUF1_BASE_ADDR);
				readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_BUF1_BASE_ADDR);
				if((readregvalue & 0xffffffff) != (testregvalue & 0xffffffff))
				{
					statusflag ++;
					printk(KERN_INFO "PWM(%x): PWM_BUF1_BASE_ADDR fails.\n", pwm_no);
					printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffffffff));
					printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffffffff));
				}
				__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_BUF1_BASE_ADDR);

				originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_BUF1_SIZE);
				__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_BUF1_SIZE);
				readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_BUF1_SIZE);
				if((readregvalue & 0xffff) != (testregvalue & 0xffff))
				{
					statusflag ++;
					printk(KERN_INFO "PWM(%x): PWM_BUF1_SIZE fails.\n", pwm_no);
					printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffff));
					printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffff));
				}
				__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_BUF1_SIZE);

				originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_SEND_DATA0);
				__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_SEND_DATA0);
				readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_SEND_DATA0);
				if((readregvalue & 0xffffffff) != (testregvalue & 0xffffffff))
				{
					statusflag ++;
					printk(KERN_INFO "PWM(%x): PWM_SEND_DATA0 fails.\n", pwm_no);
					printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffffffff));
					printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffffffff));
				}
				__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_SEND_DATA0);

				originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_SEND_DATA1);
				__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_SEND_DATA1);
				readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_SEND_DATA1);
				if((readregvalue & 0xffffffff) != (testregvalue & 0xffffffff))
				{
					statusflag ++;
					printk(KERN_INFO "PWM(%x): PWM_SEND_DATA1 fails.\n", pwm_no);
					printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffffffff));
					printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffffffff));
				}
				__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_SEND_DATA1);
/*
				if(PWM1 == pwm_no || PWM2 == pwm_no || PWM3 == pwm_no || PWM7 == pwm_no)
				{
				*/
					originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_CON);
					__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_CON);
					readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_CON);
					if((readregvalue & 0xffef) != (testregvalue & 0xffef))
					{
						statusflag ++;
						printk(KERN_INFO "PWM(%x): PWM_CON fails.\n", pwm_no);
						printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0xffef));
						printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0xffef));
					}
					__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_CON);

					originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_DATA_WIDTH);
					__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_DATA_WIDTH);
					readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_DATA_WIDTH);
					if((readregvalue & 0x1fff) != (testregvalue & 0x1fff))
					{
						statusflag ++;
						printk(KERN_INFO "PWM(%x): PWM_DATA_WIDTH fails.\n", pwm_no);
						printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x1fff));
						printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x1fff));
					}
					__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_DATA_WIDTH);

					originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_THRESH);
					__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_THRESH);
					readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_THRESH);
					if((readregvalue & 0x1fff) != (testregvalue & 0x1fff))
					{
						statusflag ++;
						printk(KERN_INFO "PWM(%x): PWM_THRESH fails.\n", pwm_no);
						printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x1fff));
						printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x1fff));
					}
					__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_THRESH);
					/*
				}
				else
				{
					originalvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_CON);
					__raw_writel(testregvalue, PWM_register[pwm_no] + 4*PWM_CON);
					readregvalue = __raw_readl(PWM_register[pwm_no] + 4*PWM_CON);
					if((readregvalue & 0x7fef) != (testregvalue & 0x7fef))
					{
						statusflag ++;
						printk(KERN_INFO "PWM(%x): PWM_CON fails.\n", pwm_no);
						printk(KERN_INFO "readregvalue: %x.\n", (readregvalue & 0x7fef));
						printk(KERN_INFO "testregvalue: %x.\n", (testregvalue & 0x7fef));
					}
					__raw_writel(originalvalue, PWM_register[pwm_no] + 4*PWM_CON);
				}*/
			}

			testregvalue = ~testregvalue;
		}

		if(statusflag > 0)
			return -1;
		break;

	case PWM_UVVF_MEMO_STOPBIT:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff = virt;

		membuff[0] = 0x1111aaaa;
		membuff[1] = 0x0;
		membuff[2] = 0x0;
		membuff[3] = 0xffffffff;

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_MEMORY;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		conf.intr = FALSE;
		conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE = GUARD_FALSE;
//			conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 30;
		conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_MEMORY_REGS.HDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.LDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.GDURATION = 0;
		conf.PWM_MODE_MEMORY_REGS.BUF0_BASE_ADDR = phys;
		conf.PWM_MODE_MEMORY_REGS.BUF0_SIZE = 3;
		conf.PWM_MODE_MEMORY_REGS.WAVE_NUM = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: stop_bitpos_value = %x, pwm_no = %x\n", conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE >>= 1;
			if(conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE < 3)
			{
				conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 31;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_MEMO_HLGDUR:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff = virt;

		membuff[0] = 0xaaaaaaaa;
		membuff[1] = 0x0;
		membuff[2] = 0x0;
		membuff[3] = 0xffffffff;

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_MEMORY;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		conf.intr = FALSE;
		conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_MEMORY_REGS.HDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.LDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.GDURATION = 0;
		conf.PWM_MODE_MEMORY_REGS.BUF0_BASE_ADDR = phys;
		conf.PWM_MODE_MEMORY_REGS.BUF0_SIZE = 3;
		conf.PWM_MODE_MEMORY_REGS.WAVE_NUM = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: gduration = %x, lduration = %x, hduration = %x, pwm_no = %x\n", conf.PWM_MODE_MEMORY_REGS.GDURATION, conf.PWM_MODE_MEMORY_REGS.LDURATION, conf.PWM_MODE_MEMORY_REGS.HDURATION, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.PWM_MODE_MEMORY_REGS.GDURATION += 8;
			if(conf.PWM_MODE_MEMORY_REGS.GDURATION > 9)
			{
				conf.PWM_MODE_MEMORY_REGS.GDURATION = 0;
				conf.PWM_MODE_MEMORY_REGS.LDURATION += 8;
			}
			if(conf.PWM_MODE_MEMORY_REGS.LDURATION > 9)
			{
				conf.PWM_MODE_MEMORY_REGS.LDURATION = 1;
				conf.PWM_MODE_MEMORY_REGS.HDURATION += 3;
			}
			if(conf.PWM_MODE_MEMORY_REGS.HDURATION > 4)
			{
				conf.PWM_MODE_MEMORY_REGS.HDURATION = 1;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_MEMO_GIOUT:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff = virt;

		membuff[0] = 0xaaaaaaaa;
		membuff[1] = 0x0;
		membuff[2] = 0x0;
		membuff[3] = 0xffffffff;

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_MEMORY;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_MEMORY_REGS.HDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.LDURATION = 1;
		conf.PWM_MODE_MEMORY_REGS.GDURATION = 8;
		conf.PWM_MODE_MEMORY_REGS.BUF0_BASE_ADDR = phys;
		conf.PWM_MODE_MEMORY_REGS.BUF0_SIZE = 3;
		conf.PWM_MODE_MEMORY_REGS.WAVE_NUM = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: guard_value = %x, idle_value = %x, wave_num = %x, pwm_no = %x\n", conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE, conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE, conf.PWM_MODE_MEMORY_REGS.WAVE_NUM, conf.pwm_no);
			pwm_set_spec_config(&conf);

			if(0 == conf.PWM_MODE_MEMORY_REGS.WAVE_NUM)
				conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE ++;
			else
				conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE ++;

			if(GUARD_MAX == conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE)
			{
				conf.PWM_MODE_MEMORY_REGS.GUARD_VALUE = GUARD_FALSE;
				conf.PWM_MODE_MEMORY_REGS.WAVE_NUM = 1;
			}
			if(IDLE_MAX == conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE)
			{
				conf.PWM_MODE_MEMORY_REGS.IDLE_VALUE = IDLE_FALSE;
				conf.PWM_MODE_MEMORY_REGS.WAVE_NUM = 0;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_OLD_WAVEFORM:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_OLD;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_OLD_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_OLD_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_OLD_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_OLD_REGS.GDURATION = 0;
		conf.PWM_MODE_OLD_REGS.WAVE_NUM = 0;
		conf.PWM_MODE_OLD_REGS.DATA_WIDTH = 99;
		conf.PWM_MODE_OLD_REGS.THRESH = 49;
		//mt_set_pwm_eco();//whao


		while(1)
		{
			printk(KERN_INFO "PWM: clk_div = %x, clk_src = %x, pwm_no = %x\n", conf.clk_div, conf.clk_src, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.clk_div ++;
			if(CLK_DIV_MAX == conf.clk_div)
			{
				conf.clk_div = CLK_DIV1;
				conf.clk_src ++;
			}
			if(PWM_CLK_NEW_MODE_BLOCK == conf.clk_src)
			{
				conf.clk_src = PWM_CLK_OLD_MODE_BLOCK;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		break;
	//add by ranran.lu
	case PWM_UVVF_OLD_WAVEFORM_32K:

		conf_old_mode.mode = PWM_MODE_OLD;
		conf_old_mode.clk_src = PWM_CLK_OLD_MODE_32K;
		conf_old_mode.intr = FALSE;
		conf_old_mode.PWM_MODE_OLD_REGS.IDLE_VALUE = IDLE_FALSE;
		conf_old_mode.PWM_MODE_OLD_REGS.GUARD_VALUE = GUARD_FALSE;
		conf_old_mode.PWM_MODE_OLD_REGS.GDURATION = 0;
		conf_old_mode.PWM_MODE_OLD_REGS.WAVE_NUM = 0;
		conf_old_mode.PWM_MODE_OLD_REGS.DATA_WIDTH = 99;
		conf_old_mode.PWM_MODE_OLD_REGS.THRESH = 24;

		printk(KERN_INFO "PWM: clk_div = %x, clk_src = %x, pwm_no = %x\n", conf_old_mode.clk_div, conf_old_mode.clk_src, conf_old_mode.pwm_no);
		pwm_set_spec_config(&conf_old_mode);

		conf_old_mode.clk_div ++;
		if(CLK_DIV_MAX == conf_old_mode.clk_div)
		{
			conf_old_mode.clk_div = CLK_DIV1;
		}
		if(PWM_CLK_NEW_MODE_BLOCK == conf.clk_src)
		{
			conf_old_mode.pwm_no ++;
		}
		if(PWM_MAX == conf_old_mode.pwm_no)
		{
			conf_old_mode.pwm_no = PWM0;
			break;
		}

		break;
	//add end
	case PWM_UVVF_OLD_THRESH:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_OLD;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_OLD_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_OLD_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_OLD_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_OLD_REGS.GDURATION = 0;
		conf.PWM_MODE_OLD_REGS.WAVE_NUM = 0;
		conf.PWM_MODE_OLD_REGS.DATA_WIDTH = 99;
		conf.PWM_MODE_OLD_REGS.THRESH = 24;


		while(1)
		{
			printk(KERN_INFO "PWM: thresh = %x, pwm_no = %x\n", conf.PWM_MODE_OLD_REGS.THRESH, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.PWM_MODE_OLD_REGS.THRESH += 25;

			if(conf.PWM_MODE_OLD_REGS.THRESH > 75)
			{
				conf.PWM_MODE_OLD_REGS.THRESH = 24;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		break;

	case PWM_UVVF_OLD_DTDUR:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_OLD;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_OLD_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_OLD_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_OLD_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_OLD_REGS.GDURATION = 0;
		conf.PWM_MODE_OLD_REGS.WAVE_NUM = 0;
		conf.PWM_MODE_OLD_REGS.DATA_WIDTH = 99;
		conf.PWM_MODE_OLD_REGS.THRESH = 24;


		while(1)
		{
			printk(KERN_INFO "PWM: data_width = %x, gduration = %x, pwm_no = %x\n", conf.PWM_MODE_OLD_REGS.DATA_WIDTH, conf.PWM_MODE_OLD_REGS.GDURATION, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.PWM_MODE_OLD_REGS.DATA_WIDTH += 50;
			conf.PWM_MODE_OLD_REGS.GDURATION += 50;

			if(conf.PWM_MODE_OLD_REGS.DATA_WIDTH > 199)
			{
				conf.PWM_MODE_OLD_REGS.DATA_WIDTH = 99;
				conf.PWM_MODE_OLD_REGS.GDURATION = 0;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		break;

	case PWM_UVVF_OLD_GIOUT:

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_OLD;
		conf.clk_div = CLK_DIV128;//CLK_DIV1;
		conf.clk_src = PWM_CLK_OLD_MODE_BLOCK;//PWM_CLK_OLD_MODE_32K;//
		conf.intr = FALSE;
		conf.PWM_MODE_OLD_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_OLD_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_OLD_REGS.GDURATION = 50;
		conf.PWM_MODE_OLD_REGS.WAVE_NUM = 0;
		conf.PWM_MODE_OLD_REGS.DATA_WIDTH = 99;
		conf.PWM_MODE_OLD_REGS.THRESH = 24;


		while(1)
		{
			printk(KERN_INFO "PWM: guard_value = %x, idle_value = %x, wave_num = %x, pwm_no = %x\n", conf.PWM_MODE_OLD_REGS.GUARD_VALUE, conf.PWM_MODE_OLD_REGS.IDLE_VALUE, conf.PWM_MODE_OLD_REGS.WAVE_NUM, conf.pwm_no);
			pwm_set_spec_config(&conf);

			if(0 == conf.PWM_MODE_OLD_REGS.WAVE_NUM)
				conf.PWM_MODE_OLD_REGS.GUARD_VALUE ++;
			else
				conf.PWM_MODE_OLD_REGS.IDLE_VALUE ++;

			if(GUARD_MAX == conf.PWM_MODE_OLD_REGS.GUARD_VALUE)
			{
				conf.PWM_MODE_OLD_REGS.GUARD_VALUE = GUARD_FALSE;
				conf.PWM_MODE_OLD_REGS.WAVE_NUM = 1;
			}
			if(IDLE_MAX == conf.PWM_MODE_OLD_REGS.IDLE_VALUE)
			{
				conf.PWM_MODE_OLD_REGS.IDLE_VALUE = IDLE_FALSE;
				conf.PWM_MODE_OLD_REGS.WAVE_NUM = 0;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		break;

	case PWM_UVVF_RANDOM_WAVEFORM:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff0 = virt + 1024;
		membuff1 = virt + 2048;
		memset(membuff0, 0xf0f0f0f0, 1024);
		memset(membuff1, 0xaaaaaaaa, 1024);

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_RANDOM;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		//conf.intr = FALSE;
		conf.intr = TRUE;
		conf.PWM_MODE_RANDOM_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_RANDOM_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_RANDOM_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_RANDOM_REGS.HDURATION = 65535;
		conf.PWM_MODE_RANDOM_REGS.LDURATION = 65535;
		conf.PWM_MODE_RANDOM_REGS.GDURATION = 0;
		conf.PWM_MODE_RANDOM_REGS.BUF0_BASE_ADDR = phys + 1024;
		conf.PWM_MODE_RANDOM_REGS.BUF0_SIZE = 255;
		conf.PWM_MODE_RANDOM_REGS.BUF1_BASE_ADDR = phys + 2048;
		conf.PWM_MODE_RANDOM_REGS.BUF1_SIZE = 255;
		conf.PWM_MODE_RANDOM_REGS.WAVE_NUM = 0;
		conf.PWM_MODE_RANDOM_REGS.VALID = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: clk_div = %x, clk_src = %x, pwm_no = %x\n", conf.clk_div, conf.clk_src, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.clk_div ++;
			conf.PWM_MODE_RANDOM_REGS.HDURATION >>= 1;
			conf.PWM_MODE_RANDOM_REGS.LDURATION >>= 1;

			if(CLK_DIV_MAX == conf.clk_div)
			{
				conf.clk_div = CLK_DIV1;
				conf.clk_src ++;
				conf.PWM_MODE_RANDOM_REGS.HDURATION = 40;
				conf.PWM_MODE_RANDOM_REGS.LDURATION = 40;
			}
			if(PWM_CLK_SRC_NUM == conf.clk_src)
			{
				conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
				conf.pwm_no ++;
				conf.PWM_MODE_RANDOM_REGS.HDURATION = 65535;
				conf.PWM_MODE_RANDOM_REGS.LDURATION = 65535;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_RANDOM_HLDUR:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff0 = virt + 1024;
		membuff1 = virt + 2048;
		memset(membuff0, 0x55555555, 1024);
		memset(membuff1, 0x11111111, 1024);

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_RANDOM;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		conf.intr = FALSE;
		conf.PWM_MODE_RANDOM_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_RANDOM_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_RANDOM_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_RANDOM_REGS.HDURATION = 1;
		conf.PWM_MODE_RANDOM_REGS.LDURATION = 1;
		conf.PWM_MODE_RANDOM_REGS.GDURATION = 0;
		conf.PWM_MODE_RANDOM_REGS.BUF0_BASE_ADDR = phys + 1024;
		conf.PWM_MODE_RANDOM_REGS.BUF0_SIZE = 255;
		conf.PWM_MODE_RANDOM_REGS.BUF1_BASE_ADDR = phys + 2048;
		conf.PWM_MODE_RANDOM_REGS.BUF1_SIZE = 255;
		conf.PWM_MODE_RANDOM_REGS.WAVE_NUM = 0;
		conf.PWM_MODE_RANDOM_REGS.VALID = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: lduration = %x, hduration = %x, pwm_no = %x\n", conf.PWM_MODE_RANDOM_REGS.LDURATION, conf.PWM_MODE_RANDOM_REGS.HDURATION, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.PWM_MODE_RANDOM_REGS.LDURATION += 8;

			if(conf.PWM_MODE_RANDOM_REGS.LDURATION > 9)
			{
				conf.PWM_MODE_RANDOM_REGS.LDURATION = 1;
				conf.PWM_MODE_RANDOM_REGS.HDURATION += 3;
			}
			if(conf.PWM_MODE_RANDOM_REGS.HDURATION > 4)
			{
				conf.PWM_MODE_RANDOM_REGS.HDURATION = 1;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_RANDOM_IOUT:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff0 = virt + 1024;
		membuff1 = virt + 2048;
		memset(membuff0, 0x55555555, 1024);
		memset(membuff1, 0x11111111, 1024);

		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_RANDOM;
		conf.clk_div = CLK_DIV1;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
		conf.intr = FALSE;
		conf.PWM_MODE_RANDOM_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_RANDOM_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_RANDOM_REGS.STOP_BITPOS_VALUE = 31;
		conf.PWM_MODE_RANDOM_REGS.HDURATION = 1;
		conf.PWM_MODE_RANDOM_REGS.LDURATION = 1;
		conf.PWM_MODE_RANDOM_REGS.GDURATION = 0;
		conf.PWM_MODE_RANDOM_REGS.BUF0_BASE_ADDR = phys + 1024;
		conf.PWM_MODE_RANDOM_REGS.BUF0_SIZE = 255;
		conf.PWM_MODE_RANDOM_REGS.BUF1_BASE_ADDR = phys + 2048;
		conf.PWM_MODE_RANDOM_REGS.BUF1_SIZE = 255;
		conf.PWM_MODE_RANDOM_REGS.WAVE_NUM = 0;
		conf.PWM_MODE_RANDOM_REGS.VALID = 0;

		while(1)
		{
			printk(KERN_INFO "PWM: idle_value = %x, pwm_no = %x\n", conf.PWM_MODE_RANDOM_REGS.IDLE_VALUE, conf.pwm_no);
			pwm_set_spec_config(&conf);

			conf.PWM_MODE_RANDOM_REGS.IDLE_VALUE ++;

			if(IDLE_MAX == conf.PWM_MODE_RANDOM_REGS.IDLE_VALUE)
			{
				conf.PWM_MODE_RANDOM_REGS.IDLE_VALUE = IDLE_FALSE;
				conf.pwm_no ++;
			}
			if(PWM_MAX == conf.pwm_no)
			{
				conf.pwm_no = PWM0;
				break;
			}
		}

		dma_free_coherent(NULL, 4096, virt, phys);

		break;

	case PWM_UVVF_SEQ:

		virt = dma_alloc_coherent(NULL, 4096, &phys, GFP_KERNEL);
		printk(KERN_INFO "Address: virt = %x, phys = %x\n", virt, phys);
		membuff = virt;
		membuff0 = virt + 1024;
		membuff1 = virt + 2048;

		membuff[0] = 0x55555555;
		membuff[1] = 0x0;
		membuff[2] = 0x0;
		membuff[3] = 0xffffffff;

		memset(membuff0, 0x55555555, 1024);
		memset(membuff1, 0x00000000, 1024);

		confseq[0].pwm_no = PWM3;
		confseq[0].mode = PWM_MODE_FIFO;
		confseq[0].clk_div = CLK_DIV128;
		confseq[0].clk_src = PWM_CLK_NEW_MODE_BLOCK;
//		confseq[0].clk_div = CLK_DIV1;
//		confseq[0].clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		confseq[0].intr = FALSE;
		confseq[0].PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
		confseq[0].PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
		confseq[0].PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
		confseq[0].PWM_MODE_FIFO_REGS.HDURATION = 1;
		confseq[0].PWM_MODE_FIFO_REGS.LDURATION = 1;
		confseq[0].PWM_MODE_FIFO_REGS.GDURATION = 0;
		confseq[0].PWM_MODE_FIFO_REGS.SEND_DATA0 = 0x55555555;
		confseq[0].PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xffffffff;
		confseq[0].PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

		pwm_set_spec_config(&confseq[0]);

		confseq[1].pwm_no = PWM4;
		confseq[1].mode = PWM_MODE_MEMORY;
		confseq[1].clk_div = CLK_DIV128;
		confseq[1].clk_src = PWM_CLK_NEW_MODE_BLOCK;
//		confseq[1].clk_div = CLK_DIV1;
//		confseq[1].clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		confseq[1].intr = FALSE;
		confseq[1].PWM_MODE_MEMORY_REGS.IDLE_VALUE = IDLE_FALSE;
		confseq[1].PWM_MODE_MEMORY_REGS.GUARD_VALUE = GUARD_FALSE;
		confseq[1].PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE = 31;
		confseq[1].PWM_MODE_MEMORY_REGS.HDURATION = 1;
		confseq[1].PWM_MODE_MEMORY_REGS.LDURATION = 1;
		confseq[1].PWM_MODE_MEMORY_REGS.GDURATION = 0;
		confseq[1].PWM_MODE_MEMORY_REGS.BUF0_BASE_ADDR = phys;
		confseq[1].PWM_MODE_MEMORY_REGS.BUF0_SIZE = 3;
		confseq[1].PWM_MODE_MEMORY_REGS.WAVE_NUM = 0;

		pwm_set_spec_config(&confseq[1]);

		confseq[2].pwm_no = PWM5;
		confseq[2].mode = PWM_MODE_FIFO;
		confseq[2].clk_div = CLK_DIV128;
		confseq[2].clk_src = PWM_CLK_NEW_MODE_BLOCK;
//		confseq[2].clk_div = CLK_DIV1;
//		confseq[2].clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		confseq[2].intr = FALSE;
		confseq[2].PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
		confseq[2].PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
		confseq[2].PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
		confseq[2].PWM_MODE_FIFO_REGS.HDURATION = 1;
		confseq[2].PWM_MODE_FIFO_REGS.LDURATION = 1;
		confseq[2].PWM_MODE_FIFO_REGS.GDURATION = 0;
		confseq[2].PWM_MODE_FIFO_REGS.SEND_DATA0 = 0x55555555;
		confseq[2].PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xffffffff;
		confseq[2].PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

		pwm_set_spec_config(&confseq[2]);

		confseq[3].pwm_no = PWM6;
		confseq[3].mode = PWM_MODE_RANDOM;
		confseq[3].clk_div = CLK_DIV128;
		confseq[3].clk_src = PWM_CLK_NEW_MODE_BLOCK;
//		confseq[3].clk_div = CLK_DIV1;
//		confseq[3].clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		confseq[3].intr = TRUE;
		confseq[3].PWM_MODE_RANDOM_REGS.IDLE_VALUE = IDLE_FALSE;
		confseq[3].PWM_MODE_RANDOM_REGS.GUARD_VALUE = GUARD_FALSE;
		confseq[3].PWM_MODE_RANDOM_REGS.STOP_BITPOS_VALUE = 31;
		confseq[3].PWM_MODE_RANDOM_REGS.HDURATION = 1;
		confseq[3].PWM_MODE_RANDOM_REGS.LDURATION = 1;
		confseq[3].PWM_MODE_RANDOM_REGS.GDURATION = 0;
		confseq[3].PWM_MODE_RANDOM_REGS.BUF0_BASE_ADDR = phys + 1024;
		confseq[3].PWM_MODE_RANDOM_REGS.BUF0_SIZE = 255;
		confseq[3].PWM_MODE_RANDOM_REGS.BUF1_BASE_ADDR = phys + 2048;
		confseq[3].PWM_MODE_RANDOM_REGS.BUF1_SIZE = 255;
		confseq[3].PWM_MODE_RANDOM_REGS.WAVE_NUM = 0;
		confseq[3].PWM_MODE_RANDOM_REGS.VALID = 0;

		pwm_set_spec_config(&confseq[3]);

		confseq[4].pwm_no = PWM3;
		confseq[4].mode = PWM_MODE_DELAY;
		confseq[4].clk_div = CLK_DIV128;
		confseq[4].clk_src = PWM_CLK_NEW_MODE_BLOCK;
//		confseq[4].clk_div = CLK_DIV1;
//		confseq[4].clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		confseq[4].PWM_MODE_DELAY_REGS.PWM4_DELAY_DUR = 1024;
		confseq[4].PWM_MODE_DELAY_REGS.PWM4_DELAY_CLK = 0;
//		confseq[4].PWM_MODE_DELAY_REGS.PWM4_DELAY_CLK = 0x10000;
//		confseq[4].PWM_MODE_DELAY_REGS.PWM4_DELAY_DUR = 1;
		confseq[4].PWM_MODE_DELAY_REGS.PWM5_DELAY_DUR = 1024;
		confseq[4].PWM_MODE_DELAY_REGS.PWM5_DELAY_CLK = 0;
//		confseq[4].PWM_MODE_DELAY_REGS.PWM5_DELAY_CLK = 0x10000;
//		confseq[4].PWM_MODE_DELAY_REGS.PWM5_DELAY_DUR = 1;
		confseq[4].PWM_MODE_DELAY_REGS.PWM3_DELAY_DUR = 1024;
		confseq[4].PWM_MODE_DELAY_REGS.PWM3_DELAY_CLK = 0;
//		confseq[4].PWM_MODE_DELAY_REGS.PWM3_DELAY_CLK = 0x10000;
//		confseq[4].PWM_MODE_DELAY_REGS.PWM3_DELAY_DUR = 1;

		pwm_set_spec_config(&confseq[4]);

		break;
#if 1
	case PWM_UVVF_AUTO:
		//auto waveform detect test by GPIO
		
		conf.pwm_no = PWM0;
		conf.mode = PWM_MODE_FIFO;
		conf.clk_div = CLK_DIV128;
		conf.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
		conf.intr = FALSE;
		conf.PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
		conf.PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
		conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;

		conf.PWM_MODE_FIFO_REGS.HDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.LDURATION = 1;
		conf.PWM_MODE_FIFO_REGS.GDURATION = 0;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0x0;
		conf.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xFFFFFFFF;
		conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;

		printk(KERN_INFO "PWM: clk_div = %x, clk_src = %x, pwm_no = %x\n", conf_old_mode.clk_div, conf_old_mode.clk_src, conf_old_mode.pwm_no);
		pwm_set_spec_config(&conf);
/*		
		while (1) {
			if (mt_get_gpio_in(GPIO73)) {
				if (!hi_cnt) {
					printk(" lo_cnt = %d\n", lo_cnt);
					lo_cnt = 0;
				}
				hi_cnt++;
			} else {
				if (!lo_cnt) {
					printk(" hi_cnt = %d\n", hi_cnt);
					hi_cnt = 0;
				}
				lo_cnt++;
			}
			udelay(1);
		}
		*/
	/*	
		while (1) {
			read = mt_get_gpio_in(GPIO73);
			cnt_p = read ? &hi_cnt:&lo_cnt;
			if (read == read_old){
				(*cnt_p)++;
			} else {
				//compare delta
				(*cnt_p) = 1;
				read_old = read;
				if (read)
					printk(" lo_cnt = %d\n", lo_cnt);
				else
					printk(" hi_cnt = %d\n", hi_cnt);
			}
			udelay(1);
		}*/
		while (1) {
			read = mt_get_gpio_in(GPIO73);
			if (read != read_old){
				read_old = read;
				cnt = jiffies - jiffies_old;
				//printk(" jiffies = %u\n", jiffies);
				//printk(" jiffies_old = %u\n", jiffies_old);
				printk(" cnt = %u\n", cnt);
				if (cnt > 30) printk("cnt = %u!!!!!!!!!!!!!!!!\n", cnt);
				jiffies_old = jiffies;
			}
			udelay(1);
		}

		break;
#endif
	default:
		while(1)
		{
			PWMDBG ( "default.\n" );
		}
		break;
	}
	return 0;
}

static struct file_operations mt_pwm_fops =
{
    .owner=        THIS_MODULE,
    .unlocked_ioctl=        mt_pwm_ioctl,
    .open=         mt_pwm_open,
//    .release=      mt_gpio_release,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice mt_pwm_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "mtpwm",
    .fops = &mt_pwm_fops,
};

static int __init mt_pwm_misc_init(void)
{
	int ret;
	struct miscdevice *misc = &mt_pwm_device;
	ret = misc_register(misc);
	if ( ret < 0 )
			printk(KERN_INFO "pwm register char fails.\n");

out:
	return ret;
}

static void __exit mt_pwm_misc_exit(void)
{
}

module_init(mt_pwm_misc_init);
module_exit(mt_pwm_misc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cindy Zhang <cindy.zhang@mediatek.com>");
MODULE_DESCRIPTION(" This module is for mt6575 chip of mediatek");

