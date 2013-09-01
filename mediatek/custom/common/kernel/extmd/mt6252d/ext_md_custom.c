#include "extmd_mt6252d.h"
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/module.h>
//#include <mach/mt6575_gpio.h>
#include <cust_eint.h>

extern unsigned int mt65xx_eint_set_sens(unsigned int, unsigned int);
extern void mt65xx_eint_set_polarity(unsigned char, unsigned char);
extern void mt65xx_eint_set_hw_debounce(unsigned char, unsigned int);
extern void mt65xx_eint_registration(unsigned char, unsigned char, unsigned char, void(*func)(void),
					unsigned char);
extern void mt65xx_eint_unmask(unsigned int);
extern void mt65xx_eint_mask(unsigned int);

int cm_do_md_power_on(void)
{
	return 0;
}

void cm_hold_rst_signal(void)
{
	mt_set_gpio_dir(GPIO_DT_MD_RST_PIN, 1);
	mt_set_gpio_out(GPIO_DT_MD_RST_PIN, 0);
}

void cm_relese_rst_signal(void)
{
	mt_set_gpio_out(GPIO_DT_MD_RST_PIN, 1);
	mt_set_gpio_dir(GPIO_DT_MD_RST_PIN, 0);
}

int cm_do_md_go(void)
{
	//int high_signal_check_num=0;
	int ret = 0;
	unsigned int retry = 100;

	#if 0
	cm_relese_rst_signal();
	msleep(10);

	mt_set_gpio_dir(GPIO_DT_MD_PWR_KEY_PIN, 1);
	mt_set_gpio_out(GPIO_DT_MD_PWR_KEY_PIN, EXT_MD_PWR_KEY_ACTIVE_LVL);

	msleep(5000); 
	mt_set_gpio_dir(GPIO_DT_MD_PWR_KEY_PIN, 0);
	//mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN, 1); // VBus
	#endif
	// Release download key to let md can enter normal boot
	//mt_set_gpio_dir(102, 1);
	//mt_set_gpio_out(102, 1);
	mt_set_gpio_dir(GPIO_DT_MD_DL_PIN, 1);
	mt_set_gpio_out(GPIO_DT_MD_DL_PIN, 1);
	// Press power key
	mt_set_gpio_dir(GPIO_DT_MD_PWR_KEY_PIN, 1);
	mt_set_gpio_out(GPIO_DT_MD_PWR_KEY_PIN, 1);
	msleep(10);
	cm_relese_rst_signal();

	// Check WDT pin to high
	while(retry>0){
		retry--;
		if(mt_get_gpio_in(GPIO_DT_MD_WDT_PIN)==0)
			msleep(10);
		else
			return 100-retry;
	}
	//msleep(5000); 
	ret = -1;

	return ret;
}

void cm_do_md_rst_and_hold(void)
{
}

void cm_hold_wakeup_md_signal(void)
{
	mt_set_gpio_out(GPIO_DT_AP_WK_MD_PIN, 0);
}

void cm_release_wakeup_md_signal(void)
{
	mt_set_gpio_out(GPIO_DT_AP_WK_MD_PIN, 1);
}

void cm_gpio_setup(void)
{
	// MD wake up AP pin
	mt_set_gpio_pull_enable(GPIO_DT_MD_WK_AP_PIN, !0);
	mt_set_gpio_pull_select(GPIO_DT_MD_WK_AP_PIN, 1);
	mt_set_gpio_dir(GPIO_DT_MD_WK_AP_PIN, 0);
	mt_set_gpio_mode(GPIO_DT_MD_WK_AP_PIN, GPIO_DT_MD_WK_AP_PIN_M_EINT); // EINT3

	// AP wake up MD pin
	mt_set_gpio_mode(GPIO_DT_AP_WK_MD_PIN, GPIO_DT_AP_WK_MD_PIN_M_GPIO); // GPIO Mode
	mt_set_gpio_dir(GPIO_DT_AP_WK_MD_PIN, 1);
	mt_set_gpio_out(GPIO_DT_AP_WK_MD_PIN, 0);

	// Rest MD pin
	mt_set_gpio_mode(GPIO_DT_MD_RST_PIN, GPIO_DT_MD_RST_PIN_M_GPIO); //GPIO202 is reset pin
	mt_set_gpio_pull_enable(GPIO_DT_MD_RST_PIN, 0);
	mt_set_gpio_pull_select(GPIO_DT_MD_RST_PIN, 1);
	mt_set_gpio_dir(GPIO_DT_MD_RST_PIN, 1);
	mt_set_gpio_out(GPIO_DT_MD_RST_PIN, 0);// Default @ reset state

	// MD power key pin
	mt_set_gpio_mode(GPIO_DT_MD_PWR_KEY_PIN, GPIO_DT_MD_PWR_KEY_PIN_M_GPIO); //GPIO 200 is power key
	mt_set_gpio_pull_enable(GPIO_DT_MD_PWR_KEY_PIN, 0);
	mt_set_gpio_dir(GPIO_DT_MD_PWR_KEY_PIN, 0);// Using input floating
	//mt_set_gpio_out(GPIO_DT_MD_PWR_KEY_PIN, 1);// Default @ reset state

	// MD WDT irq pin
	mt_set_gpio_pull_enable(GPIO_DT_MD_WDT_PIN, !0);
	mt_set_gpio_pull_select(GPIO_DT_MD_WDT_PIN, 1);
	mt_set_gpio_dir(GPIO_DT_MD_WDT_PIN, 0);
	mt_set_gpio_mode(GPIO_DT_MD_WDT_PIN, GPIO_DT_MD_WDT_PIN_M_EINT); // EINT9

	// MD Download pin
	//.......
}

void cm_ext_md_rst(void)
{
	cm_hold_rst_signal();
	mt_set_gpio_out(GPIO_OTG_DRVVBUS_PIN, 0); // VBus EMD_VBUS_TMP_PIN
}

void cm_enable_ext_md_wdt_irq(void)
{
	mt65xx_eint_unmask(CUST_EINT_DT_EXT_MD_WDT_NUM);
}

void cm_disable_ext_md_wdt_irq(void)
{
	mt65xx_eint_mask(CUST_EINT_DT_EXT_MD_WDT_NUM);
}

void cm_enable_ext_md_wakeup_irq(void)
{
	mt65xx_eint_unmask(CUST_EINT_DT_EXT_MD_WK_UP_NUM);
}

void cm_disable_ext_md_wakeup_irq(void)
{
	mt65xx_eint_mask(CUST_EINT_DT_EXT_MD_WK_UP_NUM);
}


