#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cnt32_to_63.h>
#include <linux/proc_fs.h>
#include <asm/tcm.h>
#include <mach/mt6573_reg_base.h>
#include <mach/mt6573_gpt.h>
#include <mach/timer.h>
#include <mach/irqs.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

/*
	This is a Kernel driver used by user space. 
	This driver will using the interface of the GPT production driver.  
	We implement some IOCTLs: 
	1) Enable GPT  [En + mode GPT1/2] [En GPT3] [En+Lock GPT4] 
	2) Disable GPT 
	3) Set Time-out  [GPT1/GPT2]
	4) interrupt status ?  Register Interrupt Handler[GPT1/GPT2] 
	5) Set Presacler  [GPT1/2/3]  
	6) Get the Counter. [GPT3/4]	
*/

#define gptname	"uvvp_gpt"

/*IOCTL code Define*/
#define	UVVP_GPT_ENABLE				0
#define UVVP_GPT_DISABLE			1
#define UVVP_GPT_SET_MODE			2
#define UVVP_GPT_SET_TIMEOUT		3
#define UVVP_GPT_GET_TIMEOUT		4
#define UVVP_GPT_REGISTER_CALLBACK	5
#define UVVP_GPT_SET_PRESACLER		6
#define UVVP_GPT_GET_PRESACLER		7
#define UVVP_GPT_GET_CONUTER		8
#define UVVP_GPT_SET_CONFIG			9
#define UVVP_GPT_GPT1_INTR_CONUT	10
#define UVVP_GPT_GPT2_INTR_CONUT	11
#define UVVP_GPT_UNREGISTER_CALLBACK	12

#if 0
typedef enum
{
    GPT1 = 0,           /* 16K */
    GPT2,               /* 16K */
    GPT3,               /* 16K */
    GPT4,               /* 26M */
    GPT_TOTAL_COUNT
} GPT_NUM;

typedef enum
{
    GPT_ONE_SHOT = 0x0000,
    GPT_REPEAT   = 0x4000
} GPT_CON_MODE;

typedef enum
{
    GPT_CLK_DIV_1   = 0x0000,
    GPT_CLK_DIV_2   = 0x0001,
    GPT_CLK_DIV_4   = 0x0002,
    GPT_CLK_DIV_8   = 0x0003,
    GPT_CLK_DIV_16  = 0x0004,
    GPT_CLK_DIV_32  = 0x0005,
    GPT_CLK_DIV_64  = 0x0006,
    GPT_CLK_DIV_128 = 0x0007
} GPT_CLK_DIV;

typedef struct
{
    GPT_NUM num;          
    GPT_CON_MODE mode;    
    GPT_CLK_DIV clkDiv;
    UINT32 u4Timeout;    //Counter value for Free Run.
} GPT_CONFIG;
#endif

static int g_gpt1_irq;
static int g_gpt2_irq;

void GPT1_IRQ_Handler(UINT16 i)
{
	printk("******** GPT1 interrupt ********\n" );	
	g_gpt1_irq++;		
}

void GPT2_IRQ_Handler(UINT16 i)
{
	printk("******** GPT2 interrupt ********\n" );	
	g_gpt2_irq++;		
}

//Let's use struct GPT_CONFIG for all IOCTL: 
static int uvvp_gpt_ioctl(struct inode *inode, struct file *file,
							unsigned int cmd, unsigned long arg)
{
	#ifdef Lv_debug
	printk("\r\n******** uvvp_gpt_ioctl cmd[%d]********\r\n",cmd);
	#endif 
	
	void __user *argp = (void __user *)arg;
	int __user *p = argp;

	GPT_CONFIG config; 
	GPT_NUM num;  
	GPT_CON_MODE mode; 	
	GPT_CLK_DIV clkDiv;
	UINT32 u4Timeout;
	
	switch (cmd) {
		default:
			return -1;

		case UVVP_GPT_ENABLE:
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num = config.num;

			GPT_Start(num);
			return 0;

		case UVVP_GPT_DISABLE:
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num = config.num;

			GPT_Stop(num);
			return 0;

		case UVVP_GPT_SET_MODE:
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num  = config.num;
			mode = config.mode;
			
			GPT_SetOpMode(num, mode);
			return 0;

		case UVVP_GPT_SET_TIMEOUT:
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num  = config.num;
			u4Timeout = config.u4Timeout; 
				
			GPT_SetTimeout(num, u4Timeout);
			return 0;

		case UVVP_GPT_GET_TIMEOUT:
			//Leave other value unchanged.
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num  = config.num;
			
			config.u4Timeout = GPT_GetTimeout(num);
			copy_to_user(argp, &config, sizeof(GPT_CONFIG));
			return 0;
			// user space check the config.u4Timeout. 
			
		case UVVP_GPT_REGISTER_CALLBACK: 
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num  = config.num;
			
			//Callback, how to handle. new a handler!!
			if(GPT1 == num){
				GPT_Init(GPT1, GPT1_IRQ_Handler);
				g_gpt1_irq = 0;
			}
			else if(GPT2 == num) {
				GPT_Init(GPT2, GPT2_IRQ_Handler);
				g_gpt2_irq = 0;
			}
			return 0;
			
		case UVVP_GPT_UNREGISTER_CALLBACK:
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num  = config.num;

			if(GPT1 == num){
				GPT_Init(GPT1, NULL);
				g_gpt1_irq = 0;
			}
			else if(GPT2 == num) {
				GPT_Init(GPT2, NULL);
				g_gpt2_irq = 0;
			}											
			return 0;

		case UVVP_GPT_SET_PRESACLER: 
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num  = config.num;
			clkDiv = config.clkDiv; 
			
			GPT_SetClkDivisor(num, clkDiv);
			return 0;
			
		case UVVP_GPT_GET_PRESACLER:
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num  = config.num;

			config.clkDiv = GPT_GetClkDivisor(num);
			copy_to_user(argp, &config, sizeof(GPT_CONFIG));
			return 0;

		case UVVP_GPT_GET_CONUTER: //the same as UVVP_GPT_GET_TIMEOUT.
			//Leave other value unchanged.
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			num  = config.num;
			
			config.u4Timeout = GPT_GetTimeout(num);
			copy_to_user(argp, &config, sizeof(GPT_CONFIG));
			return 0;
			// user space check the config.u4Timeout. 

		case UVVP_GPT_SET_CONFIG:
			copy_from_user(&config, argp, sizeof(GPT_CONFIG));
			GPT_Config(config);
			return 0;
				
		case UVVP_GPT_GPT1_INTR_CONUT: //parameter is a "int"
			put_user(g_gpt1_irq, p);
			return 0;
			
		case UVVP_GPT_GPT2_INTR_CONUT: //parameter is a "int"
			put_user(g_gpt2_irq, p);
			return 0;						
	}

	return 0;	
}

static int uvvp_gpt_open(struct inode *inode, struct file *file)
{
	return 0;
}


static struct file_operations uvvp_gpt_fops = {
	.owner		= THIS_MODULE,
		
	.open		= uvvp_gpt_open,
	.ioctl		= uvvp_gpt_ioctl,
};

static struct miscdevice uvvp_gpt_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = gptname,
	.fops = &uvvp_gpt_fops,
};

static int __init uvvp_gpt_init(void)
{
	misc_register(&uvvp_gpt_dev);
	return 0;
}

static void __exit uvvp_gpt_exit(void)
{

}

module_init(uvvp_gpt_init);
module_exit(uvvp_gpt_exit);


