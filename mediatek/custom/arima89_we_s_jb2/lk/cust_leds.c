//#include <platform/cust_leds.h>
#include <cust_leds.h>
#include <platform/mt_gpio.h>
#include <platform/mt_gpt.h>
//#include <asm/arch/mt6577_pwm.h>

//extern int DISP_SetBacklight(int level);

extern int disp_bls_set_backlight(unsigned int level);
void one_wire_control(unsigned int count)
{
	mt_set_gpio_mode(129, GPIO_MODE_GPIO);
	mt_set_gpio_dir(129, GPIO_DIR_OUT);
	
	count = 17-count;
		
	while(count--)	//count = 1~16
	{
		
		mt_set_gpio_out(129, 1);
		udelay(100);
		mt_set_gpio_out(129, 0);
		udelay(100);
		//mt_set_gpio_out(gpio_num, 1);
	}
	mt_set_gpio_out(129, 1);
}

int Cust_GPIO_SetBacklight(unsigned int level)
{
	unsigned int mapped_level;

	if(0 != level)
	{
	    mapped_level = level/16 + 1; //1-wire control in S5 phone only has 16 step 

		one_wire_control(mapped_level);
	}
	else
	{
		mt_set_gpio_out(129, 0);
	}
	return 0;
}
static struct cust_mt65xx_led cust_led_list[MT65XX_LED_TYPE_TOTAL] = {
#if defined(SONY_S1_SUPPORT) && defined(BACKLIGHT_IC_LM3533)
	{"red",               MT65XX_LED_MODE_LM3533, MT65XX_LED_LM3533_L3,{0}},
	{"green",             MT65XX_LED_MODE_LM3533, MT65XX_LED_LM3533_L2,{0}},
	{"blue",              MT65XX_LED_MODE_LM3533, MT65XX_LED_LM3533_L1,{0}},
#else //SONY added
	{"red",               MT65XX_LED_MODE_NONE, -1,{0}},
	{"green",             MT65XX_LED_MODE_NONE, -1,{0}},
	{"blue",              MT65XX_LED_MODE_NONE, -1,{0}},
#endif //SONY added
	{"jogball-backlight", MT65XX_LED_MODE_NONE, -1,{0}},
	{"keyboard-backlight",MT65XX_LED_MODE_NONE, -1,{0}},
//<2013/4/25-24279-jessicatseng, [5860] Remove useless LED ID	
	//{"button-backlight",  MT65XX_LED_MODE_PMIC, MT65XX_LED_PMIC_BUTTON,{0}},
	{"button-backlight",  MT65XX_LED_MODE_NONE, -1,{0}},
//>2013/4/25024279-jessicatseng	
//<2013/1/17-20471-jessicatseng, [Pelican] Integrate backlight IC LM3533 for PRE-MP SW
#if defined(BACKLIGHT_IC_LM3533)
	{"lcd-backlight",     MT65XX_LED_MODE_LM3533, MT65XX_LED_LM3533_H1,{0}},
#else
	{"lcd-backlight",     MT65XX_LED_MODE_CUST_BLS_PWM, (int)disp_bls_set_backlight,{0}},
#endif	
//>2013/1/17-20471-jessicatseng
};

struct cust_mt65xx_led *get_cust_led_list(void)
{
	return cust_led_list;
}

