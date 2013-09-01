
#include <ssw.h>
#include "cust_ssw_fxla2203.h"
#include "cust_gpio_usage.h"
//#include <mach/mt_gpio.h>


#ifndef GPIO_SSW_CH_SWAP_PIN
#define GPIO_SSW_CH_SWAP_PIN GPIO101
#endif

#ifndef GPIO_SSW_CH_SWAP_PIN_M_GPIO
#define GPIO_SSW_CH_SWAP_PIN_M_GPIO GPIO_MODE_00
#endif

#ifndef GPIO_SSW_EN_PIN
#define GPIO_SSW_EN_PIN GPIO105
#endif

#ifndef GPIO_SSW_EN_PIN_M_GPIO
#define GPIO_SSW_EN_PIN_M_GPIO GPIO_MODE_00
#endif


unsigned int ch_swap;
unsigned int en;
unsigned int curr_ssw_mode = SSW_SING_TALK;
struct mutex ssw_mutex;


//sim switch hardware initial
static int ssw_init(unsigned int mode) 
{
	SSW_DBG("ssw_init: %s \n", mode?"Single Talk":"Dual Talk");

	unsigned int ch_mode, en_mode;
	//ch_swap = GPIO_SSW_CH_SWAP_PIN;
	//en = GPIO_SSW_EN_PIN;
	//ch_mode = GPIO_SSW_CH_SWAP_PIN_M_GPIO;
	//en_mode = GPIO_SSW_EN_PIN_M_GPIO;
	ch_swap = GPIO101;
	en = GPIO105;
	ch_mode = GPIO_MODE_00;
	en_mode = GPIO_MODE_00;
	
	//initial Ch_Swap pin: 1, host1->sim slot1, host2->sim slot2; 0, host1->sim slot2, host2->sim slot1
	mt_set_gpio_mode(ch_swap, ch_mode);
	mt_set_gpio_dir(ch_swap, GPIO_DIR_OUT);

	//initial EN pin: 1, enable sim slot; 0, disable sim slot
	mt_set_gpio_mode(en, en_mode);
	mt_set_gpio_dir(en, GPIO_DIR_OUT);

	curr_ssw_mode = mode;
	if (mode == SSW_DUAL_TALK) {
		mt_set_gpio_out(ch_swap, GPIO_OUT_ONE);
		
	} else if (mode == SSW_SING_TALK) {
		mt_set_gpio_out(ch_swap, GPIO_OUT_ZERO);
	}

	mt_set_gpio_out(en, GPIO_OUT_ONE);

	SSW_DBG("ssw_init: ch_swap=(%d %d %d), en=(%d %d %d) \n", 
		ch_swap, ch_mode, mt_get_gpio_out(ch_swap),
		en, en_mode, mt_get_gpio_out(en));

	return 0;
}


int ssw_switch_mode(char *buf, unsigned int len)
{
	int ret = 0;
	unsigned int mode = *((unsigned int *)buf);

	SSW_DBG("sim switch: %s(%d) \n", mode?"Single Talk":"Dual Talk", curr_ssw_mode);

	mutex_lock(&ssw_mutex);
	
	if (curr_ssw_mode != mode) {
		curr_ssw_mode = mode;
		
		if (curr_ssw_mode == SSW_DUAL_TALK)
			mt_set_gpio_out(ch_swap, GPIO_OUT_ONE);		
		else if (curr_ssw_mode == SSW_SING_TALK)
			mt_set_gpio_out(ch_swap, GPIO_OUT_ZERO);		
	}
	
	mutex_unlock(&ssw_mutex);

	SSW_DBG("sim switch(%d) OK, ch_swap=%d, en=%d \n", curr_ssw_mode,
		mt_get_gpio_out(ch_swap), mt_get_gpio_out(en));

	return 0;
	
}
EXPORT_SYMBOL(ssw_switch_mode);


static int ssw_probe(struct platform_device *dev)
{
	ssw_init(default_mode);
	mutex_init(&ssw_mutex);
	
	register_ccci_kern_func(ID_SSW_SWITCH_MODE, ssw_switch_mode);
	
	return 0;
}

static int ssw_remove(struct platform_device *dev)
{
	SSW_DBG("ssw_remove \n");
	return 0;
}

static void ssw_shutdown(struct platform_device *dev)
{
	SSW_DBG("ssw_shutdown \n");
}

static int ssw_suspend(struct platform_device *dev, pm_message_t state)
{
	SSW_DBG("ssw_suspend \n");
	return 0;
}

static int ssw_resume(struct platform_device *dev)
{
	SSW_DBG("ssw_resume \n");
	return 0;
}


static struct platform_driver ssw_driver =
{
	.driver     = {
		.name	= "sim-switch",
	},
	.probe		= ssw_probe,
	.remove		= ssw_remove,
	.shutdown	= ssw_shutdown,
	.suspend	= ssw_suspend,
	.resume		= ssw_resume,
};


static int __init ssw_driver_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&ssw_driver);
	if (ret) {
		SSW_DBG("ssw_driver register fail(%d)\n", ret);
		return ret;
	}
	 
	return ret;
}
 

static void __exit ssw_driver_exit(void)
{
	return;
}


module_init(ssw_driver_init);
module_exit(ssw_driver_exit);


MODULE_DESCRIPTION("MTK SIM Switch Driver");
MODULE_AUTHOR("Anny <Anny.Hu@mediatek.com>");
MODULE_LICENSE("GPL");


