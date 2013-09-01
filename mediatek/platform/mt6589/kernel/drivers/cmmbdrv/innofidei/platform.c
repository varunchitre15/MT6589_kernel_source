#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>

#include <asm/irq.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>

struct device *g_innodev_platform_dev;

#define MTK_PLATFORM              
#ifdef MTK_PLATFORM                                                                      
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <mach/mt_gpio.h>
#include <linux/delay.h>
#include <mach/eint.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>
#include <mach/irqs.h>
#include <linux/io.h>                              
#include "if208.h"


#if 0
#define CUST_EINT_POLARITY_LOW              0
#define CUST_EINT_POLARITY_HIGH             1
#define CUST_EINT_DEBOUNCE_DISABLE          0
#define CUST_EINT_DEBOUNCE_ENABLE           1
#define CUST_EINT_EDGE_SENSITIVE            0
#define CUST_EINT_LEVEL_SENSITIVE           1
//////////////////////////////////////////////////////////////////////////////
#define CUST_EINT_SIANO_NUM              0
//static int CUST_EINT_SIANO_NUM =0;
#define CUST_EINT_SIANO_DEBOUNCE_CN      0
#define CUST_EINT_SIANO_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_SIANO_SENSITIVE        CUST_EINT_EDGE_SENSITIVE
#define CUST_EINT_SIANO_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE

#define GPIO_CMMB_POWER_PIN         GPIO180
#define GPIO_CMMB_POWER_PIN_M_GPIO   GPIO_MODE_00
#define GPIO_CMMB_POWER_PIN_M_CLK   GPIO_MODE_03
#define GPIO_CMMB_POWER_PIN_CLK     CLK_OUT5

#define GPIO_CMMB_EINT_PIN         GPIO187
#define GPIO_CMMB_EINT_PIN_M_GPIO   GPIO_MODE_00
#define GPIO_CMMB_EINT_PIN_M_EINT   GPIO_MODE_01

#define GPIO_CMMB_RESET_PIN         GPIO177
#define GPIO_CMMB_RESET_PIN_M_GPIO   GPIO_MODE_00
#define GPIO_CMMB_RESET_PIN_M_DAIPCMOUT   GPIO_MODE_01a
#endif   //if 0
#endif   // MTK_PLATFORM
/*
 * Innodev platform functions
 */
/*
 * register irq handler
 * parmaters pass by inno_core
 * @handler		-	if101 irq handler function pointer
 * @irq_type	-	if101 irq type (falling edge detect or rising)
 */
int inno_irq_setup(void (*interrupthandler)(void ))
{
	mt_set_gpio_mode(GPIO_CMMB_EINT_PIN, GPIO_CMMB_EINT_PIN_M_EINT);                 //set to eint MODE for enable eint function
	mt_set_gpio_dir(GPIO_CMMB_EINT_PIN, GPIO_DIR_IN); 
#if 1
	mt_set_gpio_pull_enable(GPIO_CMMB_EINT_PIN, 1);
	mt_set_gpio_pull_select(GPIO_CMMB_EINT_PIN,  1);
#endif
	inno_msg("CMMB GPIO EINT PIN mode:num:%d, %d, dir:%d,pullen:%d,pullup%d",GPIO_CMMB_EINT_PIN,mt_get_gpio_mode(GPIO_CMMB_EINT_PIN),
			mt_get_gpio_dir(GPIO_CMMB_EINT_PIN),mt_get_gpio_pull_enable(GPIO_CMMB_EINT_PIN),mt_get_gpio_pull_select(GPIO_CMMB_EINT_PIN));    

	mt65xx_eint_set_sens(CUST_EINT_CMMB_NUM, CUST_EINT_EDGE_SENSITIVE);
	mt65xx_eint_registration(CUST_EINT_CMMB_NUM, CUST_EINT_DEBOUNCE_DISABLE, CUST_EINT_POLARITY_LOW, interrupthandler, 0);         // 0:auto mask is no
	mt65xx_eint_unmask(CUST_EINT_CMMB_NUM);   
	//    mt65xx_eint_mask(CUST_EINT_CMMB_NUM);   
	return 0;
}

int inno_irq_open(void)
{
	mt65xx_eint_unmask(CUST_EINT_CMMB_NUM);   
        return 0;
}
void inno_irq_release(void)
{
	mt65xx_eint_mask(CUST_EINT_CMMB_NUM);   
	mt_set_gpio_pull_enable(GPIO_CMMB_EINT_PIN, 1);
	mt_set_gpio_pull_select(GPIO_CMMB_EINT_PIN,  0);
	mt_set_gpio_mode(GPIO_CMMB_EINT_PIN, GPIO_CMMB_EINT_PIN_M_GPIO);                 //set to eint MODE for enable eint function
	inno_msg("CMMB GPIO EINT PIN mode:num:%d, %d, dir:%d,pullen:%d,pullup%d",GPIO_CMMB_EINT_PIN,mt_get_gpio_mode(GPIO_CMMB_EINT_PIN),
			mt_get_gpio_dir(GPIO_CMMB_EINT_PIN),mt_get_gpio_pull_enable(GPIO_CMMB_EINT_PIN),mt_get_gpio_pull_select(GPIO_CMMB_EINT_PIN));    
//	mt_set_gpio_dir(GPIO_CMMB_EINT_PIN, GPIO_DIR_OUT);               // set to input avoid of leak power
}
void inno_chip_reset(void)
{
	mt_set_gpio_mode(GPIO_CMMB_RST_PIN, GPIO_CMMB_RST_PIN_M_GPIO);
	mt_set_gpio_dir(GPIO_CMMB_RST_PIN, GPIO_DIR_OUT);

//	mdelay(1);
	mt_set_gpio_out(GPIO_CMMB_RST_PIN, GPIO_OUT_ZERO); 			 
	mdelay(30);                                                                                  //delay for power to reset  typical:10ms max:50ms
	mt_set_gpio_out(GPIO_CMMB_RST_PIN, GPIO_OUT_ONE); 
//	mt_set_gpio_pull_enable(GPIO_CMMB_RST_PIN, 1);
//	mt_set_gpio_pull_select(GPIO_CMMB_RST_PIN,  1);
	inno_msg("CMMB GPIO RST PIN mode:num:%d, %d,out:%d, dir:%d,pullen:%d,pullup%d",GPIO_CMMB_RST_PIN,mt_get_gpio_mode(GPIO_CMMB_RST_PIN),mt_get_gpio_out(GPIO_CMMB_RST_PIN),mt_get_gpio_dir(GPIO_CMMB_RST_PIN),mt_get_gpio_pull_enable(GPIO_CMMB_RST_PIN),mt_get_gpio_pull_select(GPIO_CMMB_RST_PIN));    	 
	mdelay(30);                                                                                  //delay for waiting system ready typical:10ms max:50ms
}

#if 0
/*
 * IF202 fireware download
 */
extern int inno_download_firmware(char* fw_bin, int fw_size);

static int inno_request_firmware(char *fw_name)
{
	int ret;
	/* uses the default method to get the firmware */
	const struct firmware *fw_entry;

	if(fw_name==NULL) {
		inno_msg(KERN_ERR "innodev: error, firmware name is NULL");
		return -EINVAL;
	}

	inno_msg(KERN_INFO "innodev: if2xx requesting firmware (%s)", fw_name);

	if(request_firmware(&fw_entry, fw_name, g_innodev_platform_dev)!=0)
	{
		inno_msg(KERN_ERR "innodev: firmware (%s) not available!", fw_name);
		return -EINVAL;
	}

	inno_msg(KERN_INFO "innodev: if2xx loaded firmware %d bytes", fw_entry->size);
	ret = inno_download_firmware((char*)fw_entry->data, fw_entry->size);

	release_firmware(fw_entry);

	/* finish setting up the device */
	return ret;
}

int inno_check_firmware(void)
{
	struct innodev_platform_data *pdata;
	if(!g_innodev_platform_dev || !g_innodev_platform_dev->platform_data)
		return -EINVAL;
	pdata = g_innodev_platform_dev->platform_data;

	if(pdata->fw_spi_download)
		return inno_request_firmware(pdata->fw_name);
	return 0;
}
#endif

