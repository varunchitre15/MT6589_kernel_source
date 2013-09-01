/*
 * drivers/leds/leds-mt65xx.c
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 * mt65xx leds driver
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include <linux/leds-mt65xx.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/slab.h>

#include <cust_leds.h>

#if defined (CONFIG_ARCH_MT6577) || defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)
#include <mach/mt_pwm.h>
#include <mach/mt_gpio.h>
#include <mach/pmic_mt6329_hw_bank1.h> 
#include <mach/pmic_mt6329_sw_bank1.h> 
#include <mach/pmic_mt6329_hw.h>
#include <mach/pmic_mt6329_sw.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>
#include <mach/mt_pmic_feature_api.h>
#include <mach/mt_boot.h>

#elif defined (CONFIG_ARCH_MT6589)
#include <mach/mt_pwm.h>
//#include <mach/mt_gpio.h>
#include <mach/pmic_mt6329_hw_bank1.h> 
#include <mach/pmic_mt6329_sw_bank1.h> 
#include <mach/pmic_mt6329_hw.h>
#include <mach/pmic_mt6329_sw.h>
#include <mach/upmu_common_sw.h>
#include <mach/upmu_hw.h>
//#include <mach/mt_pmic_feature_api.h>
//<2013/5/6-24610-jessicatseng, [5860] Show a white LED short pulse when user turns phone on
#include <mach/mt_boot.h>
//>2013/5/6-24610-jessicatseng

#endif

//<2013/1/17-20471-jessicatseng, [Pelican] Integrate backlight IC LM3533 for PRE-MP SW
// add for LM3533
#include <linux/i2c.h>
#include "lm3533.h"
//>2013/1/17-20471-jessicatseng

//<2013/3/21-23110-jessicatseng, [Pelican] Using SONY brightness curve
#if !defined(HW_PDP)
#define USING_MAP_TABLE_LM3533
#endif
//>2013/3/21-23110-jessicatseng

#ifndef CONTROL_BL_TEMPERATURE
#define CONTROL_BL_TEMPERATURE
#endif
//add for sync led work schedule
static DEFINE_MUTEX(leds_mutex);

//#define ISINK_CHOP_CLK

/****************************************************************************
 * LED Variable Settings
 ***************************************************************************/
#define NLED_OFF 0
#define NLED_ON 1
#define NLED_BLINK 2
#define MIN_FRE_OLD_PWM 32 // the min frequence when use old mode pwm by kHz
#define PWM_DIV_NUM 8
#define ERROR_BL_LEVEL 0xFFFFFFFF
struct nled_setting
{
	u8 nled_mode; //0, off; 1, on; 2, blink;
	u32 blink_on_time ;
	u32 blink_off_time;
};
 
struct cust_mt65xx_led* bl_setting = NULL;
unsigned int bl_brightness = 102;
unsigned int bl_duty = 21;
unsigned int bl_div = CLK_DIV1;
unsigned int bl_frequency = 32000;
#if defined CONFIG_ARCH_MT6589
typedef enum{  
    PMIC_PWM_0 = 0,  
    PMIC_PWM_1 = 1,  
    PMIC_PWM_2 = 2
} MT65XX_PMIC_PWM_NUMBER;
#endif

/*****************PWM *************************************************/
int time_array[PWM_DIV_NUM]={256,512,1024,2048,4096,8192,16384,32768};
u8 div_array[PWM_DIV_NUM] = {1,2,4,8,16,32,64,128};
unsigned int backlight_PWM_div = CLK_DIV1;// this para come from cust_leds.


/****************************************************************************
 * DEBUG MACROS
 ***************************************************************************/
static int debug_enable = 1;
#define LEDS_DEBUG(format, args...) do{ \
	if(debug_enable) \
	{\
		printk(KERN_EMERG format,##args);\
	}\
}while(0)

/****************************************************************************
 * structures
 ***************************************************************************/
struct mt65xx_led_data {
	struct led_classdev cdev;
	struct cust_mt65xx_led cust;
	struct work_struct work;
	int level;
	int delay_on;
	int delay_off;
};

//<2013/1/17-20471-jessicatseng, [Pelican] Integrate backlight IC LM3533 for PRE-MP SW
#if defined(BACKLIGHT_IC_LM3533)
//<2013/2/18-21837-jessicatseng, [Pelican] Fix the blinking function of led light can't work
static DEFINE_MUTEX(lm3533_i2c_access);
//>2013/2/18-21837-jessicatseng
//<2013/5/14-24899-jessicatseng, [5860] Show last android LED when turning off all TS LED
static int blink_set_lm3533(int led_num, struct nled_setting* led, int level);
static int brightness_set_lm3533(int led_num, int level);
//>2013/5/14-24899-jessicatseng
static int lm3533_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id);
static int lm3533_i2c_remove(struct i2c_client *i2c);

static struct platform_driver mt65xx_leds_driver;
static struct i2c_client *lm3533_i2c_client = NULL;
static struct i2c_board_info __initdata i2c_lm3533={I2C_BOARD_INFO("lm3533", (0x6c>>1))};

static const struct i2c_device_id lm3533_i2c_ids[] = {
	{ "lm3533", 0 },
	{ },
};

static struct i2c_driver lm3533_i2c_driver = {
	.driver = {
		   .name = "lm3533",
		   .owner = THIS_MODULE,
	},
	.id_table	= lm3533_i2c_ids,
	.probe		= lm3533_i2c_probe,
	.remove		= __devexit_p(lm3533_i2c_remove),
};

//<2013/5/7-24687-jessicatseng, [5860] Add VALUE_BUTTON_3 and VALUE_PATTERN_2 for LED illumination
static struct work_struct led_pulse_work;
static struct workqueue_struct * led_pulse_workqueue = NULL;
static struct hrtimer led_pulse_timer;
static kal_bool is_keep_light = KAL_FALSE;
static u8 keep_light_color = 0x00;
//>2013/5/7-24687-jessicatseng

//<2013/4/2-23447-ricehuang, [Pelican] connect native(lights.c) to driver(leds.c)
typedef struct  {
	int pattern_id;
//<2013/5/14-24904-ricehuang,add falsh_mode for lm3533 light
	int flash_mode;
//>2013/5/14-24904-ricehuang	
	u8 red;
	u8 green;
	u8 blue;
}lm3533_led_data;

//<2013/4/23-24160-ricehuang, [xssm]Power saving in LED
unsigned int notification_value_enable = 1;
unsigned int attention_value_enable = 1;
//>2013/4/23-24160-ricehuang	
//>2013/4/2-23447-ricehuang

//<2013/5/14-24898-jessicatseng, [5860] Fix no LED indicator when rebooting with USB / AC charger
static kal_bool is_show_power_on_pulse = KAL_FALSE;
extern boot_reason_t g_boot_reason;
//>2013/5/14-24898-jessicatseng

//<2013/5/14-24899-jessicatseng, [5860] Show last android LED when turning off all TS LED
static kal_bool pattern_enable = KAL_FALSE;
static struct mt65xx_led_data last_leds_data[MT65XX_LED_TYPE_TOTAL];
//>2013/5/14-24899-jessicatseng
#endif
//>2013/1/17-20471-jessicatseng

/****************************************************************************
 * function prototypes
 ***************************************************************************/

#if defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)|| defined (CONFIG_ARCH_MT6577)
extern void mt_pwm_power_off (U32 pwm_no);
extern S32 mt_set_pwm_disable ( U32 pwm_no ) ;
#elif defined CONFIG_ARCH_MT6589
extern void mt_pwm_disable(U32 pwm_no, BOOL pmic_pad);
#endif
extern unsigned int brightness_mapping(unsigned int level);
extern int mtkfb_get_backlight_pwm(unsigned int divider, unsigned int *freq);

/* export functions */
int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level);

static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, u32 level, u32 div);
//static int brightness_set_gpio(int gpio_num, enum led_brightness level);
static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level);
static void mt65xx_led_work(struct work_struct *work);
static void mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level);
static int  mt65xx_blink_set(struct led_classdev *led_cdev,
			     unsigned long *delay_on,
			     unsigned long *delay_off);

#ifdef CONTROL_BL_TEMPERATURE
int setMaxbrightness(int max_level, int enable);

#endif



/****************************************************************************
 * global variables
 ***************************************************************************/
static struct mt65xx_led_data *g_leds_data[MT65XX_LED_TYPE_TOTAL];
struct wake_lock leds_suspend_lock;

/****************************************************************************
 * add API for temperature control
 ***************************************************************************/

#ifdef CONTROL_BL_TEMPERATURE

//define int limit for brightness limitation
static unsigned  int limit = 255;
static unsigned  int limit_flag = 0;  
static unsigned  int last_level = 0; 
static unsigned  int current_level = 0; 
static DEFINE_MUTEX(bl_level_limit_mutex);
extern int disp_bls_set_max_backlight(unsigned int level);

//this API add for control the power and temperature
//if enabe=1, the value of brightness will smaller  than max_level, whatever lightservice transfers to driver
int setMaxbrightness(int max_level, int enable)
{

	struct cust_mt65xx_led *cust_led_list = get_cust_led_list();
 #if !defined (MTK_AAL_SUPPORT)    
	mutex_lock(&bl_level_limit_mutex);
	if (1 == enable)
	{
		limit_flag = 1;
		limit = max_level;
		mutex_unlock(&bl_level_limit_mutex);
		//LEDS_DEBUG("[LED] setMaxbrightness limit happen and release lock!!\n");
		printk("setMaxbrightness enable:last_level=%d, current_level=%d\n", last_level, current_level);
		//if (limit < last_level){
		if (0 != current_level){
			if(limit < last_level){
						//printk("mt65xx_leds_set_cust in setMaxbrightness:value control start! limit=%d\n", limit);
						mt65xx_led_set_cust(&cust_led_list[MT65XX_LED_TYPE_LCD], limit);
			}else{
						//printk("mt65xx_leds_set_cust in setMaxbrightness:value control start! last_level=%d\n", last_level);
						mt65xx_led_set_cust(&cust_led_list[MT65XX_LED_TYPE_LCD], last_level);
			}
		}
	}
	else
	{
		limit_flag = 0;
		limit = 255;
		mutex_unlock(&bl_level_limit_mutex);
		//LEDS_DEBUG("[LED] setMaxbrightness limit closed and and release lock!!\n");
		printk("setMaxbrightness disable:last_level=%d, current_level=%d\n", last_level, current_level);
		
		//if (last_level != 0){
		if (0 != current_level){
		//printk("control temperature close:limit=%d\n", limit);
		mt65xx_led_set_cust(&cust_led_list[MT65XX_LED_TYPE_LCD], last_level);
		
		//printk("mt65xx_leds_set_cust in setMaxbrightness:value control close!\n");
		}
	}
 	
	LEDS_DEBUG("[LED] setMaxbrightness limit_flag = %d, limit=%d, current_level=%d\n",limit_flag, limit, current_level);
 #else
 	printk("setMaxbrightness go through AAL\n");
 	disp_bls_set_max_backlight(max_level);
 #endif
	
	return 0;
	
}
#endif
/****************************************************************************
 * internal functions
 ***************************************************************************/
//<2013/1/17-20471-jessicatseng, [Pelican] Integrate backlight IC LM3533 for PRE-MP SW
#if defined(BACKLIGHT_IC_LM3533)
//<2013/1/24-20895-jessicatseng, [Pelican] Add LED fading function
struct Cmd_config
{
 unsigned char cmd;
 unsigned char data;
};

struct LED_pattern_table {
 unsigned char count;
 struct Cmd_config config_data[30];
};

//<2013/5/7-24687-jessicatseng, [5860] Add VALUE_BUTTON_3 and VALUE_PATTERN_2 for LED illumination
static const struct LED_pattern_table pattern_data[] = 
{
	// Continous short pulse with fade. Fade in 524ms, Fade out 1049ms
 {12,{{0x71,0x00},{0x72,0x00},{0x74,0x02},{0x75,0x03},{0x81,0x00},{0x82,0x00},{0x84,0x02},{0x85,0x03},{0x91,0x00},{0x92,0x00},{0x94,0x02},{0x95,0x03}}},
	// Continous short pulse with fade, Fade in 1049ms, Fade out 2097ms 
 {12,{{0x71,0x00},{0x72,0x00},{0x74,0x03},{0x75,0x04},{0x81,0x00},{0x82,0x00},{0x84,0x03},{0x85,0x04},{0x91,0x00},{0x92,0x00},{0x94,0x03},{0x95,0x04}}},
	// Continousy lit. Fades out with display. Fade in 524ms, lit until display is off, fade out 1049ms
 {12,{{0x71,0x00},{0x72,0x3C},{0x74,0x02},{0x75,0x03},{0x81,0x00},{0x82,0x3C},{0x84,0x02},{0x85,0x03},{0x91,0x00},{0x92,0x3C},{0x94,0x02},{0x95,0x03}}},
	// One short pulse with fade. Fade in 524ms, Fade out 1049ms
 {12,{{0x71,0x3C},{0x72,0x00},{0x74,0x02},{0x75,0x03},{0x81,0x3C},{0x82,0x00},{0x84,0x02},{0x85,0x03},{0x91,0x3C},{0x92,0x00},{0x94,0x02},{0x95,0x03}}},
	// One long pulse with fade. Fade in 2097ms, Fade out 4194ms
 {12,{{0x71,0x3C},{0x72,0x00},{0x74,0x04},{0x75,0x05},{0x81,0x3C},{0x82,0x00},{0x84,0x04},{0x85,0x05},{0x91,0x3C},{0x92,0x00},{0x94,0x04},{0x95,0x05}}}
};
//>2013/5/7-24687-jessicatseng

//<2013/2/18-21837-jessicatseng, [Pelican] Fix the blinking function of led light can't work
static int lm3533_write_reg(u8 reg, u8 writeData)
{
	u8 data_buf[2] = {0};
	int ret = 0;
		
	data_buf[0] = reg;
	data_buf[1] = writeData;
	ret = i2c_master_send(lm3533_i2c_client, (const char*)data_buf, 2);
	if(ret < 0)
	{
		LEDS_DEBUG("send command(0x%x) error!!\n", data_buf[0]);
		return -EFAULT;
	}
	
	return 0;
}

static int lm3533_read_reg(u8 reg, u8 *returnData)
{
	u8 data_buf[2] = {0};
	int ret = 0;
	
	data_buf[0] = reg;
	ret = i2c_master_send(lm3533_i2c_client, (const char*)data_buf, 1);
	if(ret < 0)
	{
		LEDS_DEBUG("send command(0x%x) error!!\n", data_buf[0]);
		return -EFAULT;
	}

	ret = i2c_master_recv(lm3533_i2c_client, (char*)returnData, 1);
	if(ret < 0)
	{
		LEDS_DEBUG("read reg(0x%x) data error!!\n", data_buf[0]);
		return -EFAULT;
	}
	
	return 0;
}

//<2013/5/14-24899-jessicatseng, [5860] Show last android LED when turning off all TS LED
void led_switch(void)
{
	int idx = MT65XX_LED_TYPE_RED;
	struct nled_setting nled_tmp_setting = {0,0,0};
	struct led_trigger *p_led_trigger;
	
	for(idx = MT65XX_LED_TYPE_RED; idx <= MT65XX_LED_TYPE_BLUE; idx++)
	{
		printk("led_switch g_leds_data[%d] = %d\n", idx, g_leds_data[idx]->level);
	
		if(g_leds_data[idx]->level != 0)
		{
			p_led_trigger = g_leds_data[idx]->cdev.trigger;
			
			//if(p_led_trigger == NULL)
				//printk("p_led_trigger == NULL\n");
			//else
			 //printk("name=%s\n", p_led_trigger->name);
			 
			//printk("delay_on=%d,delay_off=%d\n", g_leds_data[idx]->delay_on, g_leds_data[idx]->delay_off);
			
			if((p_led_trigger != NULL) && (!strcmp(p_led_trigger->name, "timer"))
			&& (g_leds_data[idx]->delay_on != 0) && (g_leds_data[idx]->delay_off != 0))
			{
				nled_tmp_setting.nled_mode = NLED_BLINK;
				nled_tmp_setting.blink_off_time = g_leds_data[idx]->delay_off;
				nled_tmp_setting.blink_on_time = g_leds_data[idx]->delay_on;
				if(idx == MT65XX_LED_TYPE_RED)
					blink_set_lm3533(MT65XX_LED_LM3533_L3, &nled_tmp_setting, g_leds_data[idx]->level);
				else if(idx == MT65XX_LED_TYPE_GREEN)
					blink_set_lm3533(MT65XX_LED_LM3533_L2, &nled_tmp_setting, g_leds_data[idx]->level);
				else if(idx == MT65XX_LED_TYPE_BLUE)
					blink_set_lm3533(MT65XX_LED_LM3533_L1, &nled_tmp_setting, g_leds_data[idx]->level);
			}
			else
			{
				if(idx == MT65XX_LED_TYPE_RED)
					brightness_set_lm3533(MT65XX_LED_LM3533_L3, g_leds_data[idx]->level);
				else if(idx == MT65XX_LED_TYPE_GREEN)
					brightness_set_lm3533(MT65XX_LED_LM3533_L2, g_leds_data[idx]->level);
				else if(idx == MT65XX_LED_TYPE_BLUE)
					brightness_set_lm3533(MT65XX_LED_LM3533_L1, g_leds_data[idx]->level);
			}	
		}

	}
}
//>2013/5/14-24899-jessicatseng

//<2013/5/7-24687-jessicatseng, [5860] Add VALUE_BUTTON_3 and VALUE_PATTERN_2 for LED illumination
void led_pulse_work_callback(struct work_struct *work)
{
	u8 enablevalue = 0;
	
	if(is_keep_light)
	{
		// disable pattern
		lm3533_write_reg(0x28, 0x00);		
	}
	else
	{		
		lm3533_read_reg(0x27, &enablevalue);
		
		// disable R/G/B 
		enablevalue = enablevalue & 0x03;
		lm3533_write_reg(0x27, enablevalue);

//<2013/5/14-24899-jessicatseng, [5860] Show last android LED when turning off all TS LED
		if(pattern_enable == KAL_FALSE)
			led_switch();
			
		pattern_enable = KAL_FALSE;	
//>2013/5/14-24899-jessicatseng
	}
}

static enum hrtimer_restart led_pulse_timer_func(struct hrtimer *timer)
{
	queue_work(led_pulse_workqueue, &led_pulse_work);	
	
	return HRTIMER_NORESTART;
}

//<2013/5/15-24943-jessicatseng, [5860] Add flash_mode property for LED VALUE_BUTTON_2 type
//<2013/3/29-23351-jessicatseng, [Pelican] Fix led illumination issues for SONY requirements
//static int pattern_set_lm3533(struct cust_mt65xx_led *cust, int pattern_id)
//<2013/4/2-23447-ricehuang, [Pelican] connect native(lights.c) to driver(leds.c)
//<2013/5/14-24904-ricehuang,add falsh_mode for lm3533 light
static int pattern_set_lm3533(struct cust_mt65xx_led *cust, int pattern_id, int flash_mode, u8 red_data, u8 green_data, u8 blue_data)
//static int pattern_set_lm3533(struct cust_mt65xx_led *cust, int pattern_id, u8 red_data, u8 green_data, u8 blue_data)
//>2013/5/14-24904-ricehuang
//static int pattern_set_lm3533(struct cust_mt65xx_led *cust, int pattern_id)
//>2013/4/2-23447-ricehuang
{
	//struct i2c_client *client = lm3533_i2c_client;
	u8 data_buf[2], enablevalue = 0, data_idx = 0, idx;
//<2013/4/2-23447-ricehuang, [Pelican] connect native(lights.c) to driver(leds.c)
//	u8 red_data = 0xAA, green_data = 0x0F, blue_data = 0x0F;
//>2013/4/2-23447-ricehuang
	printk("[LED]LM3533 pattern_set_lm3533 pattern_id=%d, red=0x%x, green=0x%x, blue=0x%x\n", pattern_id, red_data, green_data, blue_data);

	mutex_lock(&lm3533_i2c_access);

	// disable R/G/B 
	lm3533_read_reg(0x27, &enablevalue);
	printk("[LED]LM3533 pattern_set_lm3533 enablevalue=0x%x\n", enablevalue);
	enablevalue = enablevalue & 0x03;
	lm3533_write_reg(0x27, enablevalue);
	
	is_keep_light = KAL_FALSE;
	keep_light_color = 0x00;
	hrtimer_cancel(&led_pulse_timer);
 
//<2013/5/14-24899-jessicatseng, [5860] Show last android LED when turning off all TS LED
	if((0 == blue_data) && (0 == green_data) && (0 == red_data))
	{
		pattern_enable = KAL_FALSE;
		lm3533_write_reg(0x28, 0x00);
		queue_work(led_pulse_workqueue, &led_pulse_work);	
		mutex_unlock(&lm3533_i2c_access);
		return 0;		
	}
	
	pattern_enable = KAL_TRUE;
//>2013/5/14-24899-jessicatseng

	if(blue_data)
	{
		enablevalue = enablevalue | 0x04;
		lm3533_write_reg(0x42, blue_data); // set current for blue LED
	}

	if(green_data)
	{
		enablevalue = enablevalue | 0x08;
		lm3533_write_reg(0x43, green_data); // set current for green LED
	}
	
	if(red_data)
	{
		enablevalue = enablevalue | 0x10;
		lm3533_write_reg(0x44, red_data); // set current for red LED
	}

	// set pattern data
	switch(pattern_id)
	{
		case 1: if(flash_mode == 0) data_idx = 2; break;
		case 4: data_idx = 3; break;
		case 5: data_idx = 1; break;
		case 6: data_idx = 4; break;
	}	
 	
	for(idx=0; idx<pattern_data[data_idx].count; idx++)
	{
		lm3533_write_reg(pattern_data[data_idx].config_data[idx].cmd, pattern_data[data_idx].config_data[idx].data);
	}
		
	lm3533_write_reg(0x28, 0x15);
	lm3533_write_reg(0x27, enablevalue);

	// enable timer to turn off R/G/B LED
	switch(data_idx)
	{
		case 2:
		{
			is_keep_light = KAL_TRUE;
			keep_light_color = enablevalue;
			hrtimer_start(&led_pulse_timer, ktime_set(0, (500 * 1000000)), HRTIMER_MODE_REL);
		}
		break;			
		case 3:
			hrtimer_start(&led_pulse_timer, ktime_set(1, (500 * 1000000)), HRTIMER_MODE_REL);
		break;			
		case 4:
			hrtimer_start(&led_pulse_timer, ktime_set(4, (500 * 1000000)), HRTIMER_MODE_REL);
		break;			
	}
	
	mutex_unlock(&lm3533_i2c_access);
	
	return 0;
}
//>2013/1/24-20895-jessicatseng
//>2013/3/29-23351-jessicatseng
//>2013/5/7-24687-jessicatseng
//>2013/5/15-24943-jessicatseng

//<2013/3/14-22799-jessicatseng, [Pelican] Modify R/G/B LED's level can be controlled by android
static int blink_set_lm3533(int led_num, struct nled_setting* led, int level) 
{
	struct i2c_client *client = lm3533_i2c_client;
	u8 data_buf[2], patternvalue = 0, enablevalue = 0;
	u32 low_time = 0, high_time = 0;
	
	printk("[LED]LM3533 blink_set_lm3533 led_num=%d\n", led_num);

	mutex_lock(&lm3533_i2c_access);
	
	if(1000 >= led->blink_on_time)
	{
		high_time = led->blink_on_time / 16;
	}
	else if(10000 >= led->blink_on_time)
	{
		high_time = ((led->blink_on_time - 1000) / 131) + 0x3C;
	}		
	else 
	{
		high_time = ((led->blink_on_time - 10000) / 524) + 0x7F;
	}	
	
	if(0xFF < high_time)
	{
		high_time = 0xFF;
	}

	if(1000 >= led->blink_off_time)
	{	
		low_time = led->blink_off_time / 16;
	}
	else if(10000 >= led->blink_off_time)
	{
		low_time = ((led->blink_off_time - 1000) / 131) + 0x3C;
	}	
	else
	{
		low_time = ((led->blink_off_time - 10000) / 524) + 0x7F;
	}
	
	if(0xFF < low_time)
	{
		low_time = 0xFF;
	}

	if(level > 0xFF)
		level = 0xFF;

	//printk("[LED]LM3533 high_time=%d, low_time=%d\n", high_time, low_time);
		
	data_buf[0] = 0x27;
	i2c_master_send(client, (const char*)data_buf, 1);
	i2c_master_recv(client, (char*)&enablevalue, 1);
 
	// disable R/G/B 
	data_buf[0] = 0x27;
	data_buf[1] = enablevalue & 0x03;
	i2c_master_send(client, (const char*)data_buf, sizeof(data_buf)); 

	data_buf[0]=0x28;
	i2c_master_send(client, (const char*)data_buf, 1);
	i2c_master_recv(client, (char*)&patternvalue, 1);

//<2013/1/21-20895-jessicatseng, [Pelican] Add LED fading function
	if(led_num == MT65XX_LED_LM3533_L1)
	{
		lm3533_write_reg(0x71, (u8)low_time); // low time 
		lm3533_write_reg(0x72, (u8)high_time); // high time 
		//lm3533_write_reg(0x73, 0x03); // low level brightness
		lm3533_write_reg(0x74, 0x00); // rise time
		lm3533_write_reg(0x75, 0x00); // fall time
		lm3533_write_reg(0x42, level);

		patternvalue = patternvalue | 0x01;
		enablevalue = enablevalue | 0x04;		
	}
	
	if(led_num == MT65XX_LED_LM3533_L2)
	{
		lm3533_write_reg(0x81, (u8)low_time); // low time 
		lm3533_write_reg(0x82, (u8)high_time); // high time 
		//lm3533_write_reg(0x83, 0x03); // low level brightness
		lm3533_write_reg(0x84, 0x00); // rise time
		lm3533_write_reg(0x85, 0x00); // fall time
		lm3533_write_reg(0x43, level);

		patternvalue = patternvalue | 0x04;
		enablevalue = enablevalue | 0x08;		
	}
	
	if(led_num == MT65XX_LED_LM3533_L3)
	{
		lm3533_write_reg(0x91, (u8)low_time); // low time
		lm3533_write_reg(0x92, (u8)high_time); // high time
		//lm3533_write_reg(0x93, 0x03); // rise time
		lm3533_write_reg(0x94, 0x00); // rise time
		lm3533_write_reg(0x95, 0x00); // fall time
		lm3533_write_reg(0x44, level);

		patternvalue = patternvalue | 0x10;
		enablevalue = enablevalue | 0x10;		
	}
//>2013/1/24-20895-jessicatseng	

	lm3533_write_reg(0x28, patternvalue); // pattern enable
	lm3533_write_reg(0x27, enablevalue); // bank enable

	mutex_unlock(&lm3533_i2c_access);
	
	return 0;
}

//<2013/3/21-23110-jessicatseng, [Pelican] Using SONY brightness curve
#if defined(USING_MAP_TABLE_LM3533)
static int brightness_map_sony_lm3533(int level)
{
 if (level == 0) return 0;
	else if (level <= 20)	return 20;
	else if (level <= 80)	return ((level - 20) / 6) + 20; // max 30
	else if (level <= 100)	return ((level - 80) / 5) + 30; // max 34
	else if (level <= 140)	return ((level - 100) >> 1) + 34; // max 54
	else if (level <= 160) return (level - 140) + 54; // max 74
	else if (level <= 170)
	{
	 if (level <= 166) return (level - 160) + 74;
		else return ((level - 166) << 1) + 80; // max 88
	}
	else if (level <= 180)
	{
	 if (level <= 176) return (level - 170) + 88;
		else return ((level - 176) << 1) + 94; // max 102
	}
	else if (level <= 210)
	{
		if (level <= 191) return (level - 180) + 102;
		else return ((level - 191) << 1) + 113; // max 151
	}	
	else if (level <= 230)	return ((level - 210) << 1) + 152; // max 192
	else if (level < 255)
	{
	 if (level <= 244) return ((level - 230) << 1) + 192; // max 220
	 else return ((level - 244) * 3) + 220; // max 250
	}	
	else if (level >= 255)	return 255; // max 255
}
#endif
//>2013/3/21-23110-jessicatseng

//<2013/3/19-23017-jessicatseng, [Pelican] Fix backlight issue
static int brightness_set_lm3533(int led_num, int level)
{
	struct i2c_client *client = lm3533_i2c_client;
	u8 data_buf[2], enablevalue = 0, patternvalue = 0;
	int ret = 0;
	
	LEDS_DEBUG("[LED]LM3533#%d:%d\n", led_num, level);
	
	mutex_lock(&lm3533_i2c_access);	
	
	if(NULL == client)
	{
		LEDS_DEBUG("i2c client is null!!\n");
		mutex_unlock(&lm3533_i2c_access);
		return 0;
	}
	
//<2013/5/14-24899-jessicatseng, [5860] Show last android LED when turning off all TS LED
	if(pattern_enable)
	{
		if((led_num == MT65XX_LED_LM3533_L1) || (led_num == MT65XX_LED_LM3533_L2) || (led_num == MT65XX_LED_LM3533_L3))
		{
			mutex_unlock(&lm3533_i2c_access);
			return 0;
		}	
	}
//>2013/5/14-24899-jessicatseng
	
	lm3533_read_reg(0x27, &enablevalue);
	lm3533_read_reg(0x28, &patternvalue);

	//LEDS_DEBUG("[LED]LM3533 1 enablevalue = %d\n", enablevalue);

	// brightness
	if((led_num == MT65XX_LED_LM3533_H1) || (led_num == MT65XX_LED_LM3533_H2))
	{
//<2013/3/7-22577-jessicatseng, [Pelican] Modify backlight setting for DP0 HW (add bank B)
//<2013/3/21-23110-jessicatseng, [Pelican] Using SONY brightness curve
#if defined(USING_MAP_TABLE_LM3533)
		level = brightness_map_sony_lm3533(level);
#else
		if(level >= 0xFF)
			level = 0xFF;
		else if(level == 0)
			level = 0;
		else			
			level = 125 + (level >> 1);
#endif
//>2013/3/21-23110-jessicatseng

		lm3533_write_reg(0x40, level); // bank A
		
#if !defined(HW_PDP)	
		lm3533_write_reg(0x41, level); // bank B
#endif
//>2013/3/7-22577-jessicatseng
	}
	else if((led_num == MT65XX_LED_LM3533_L1) || (led_num == MT65XX_LED_LM3533_L2) || (led_num == MT65XX_LED_LM3533_L3))
	{
		if(level >= 0xFF)
			level = 0xFF;
		
		lm3533_write_reg((0x40+led_num), level); // B/G/R led
	}	
	
//<2013/1/24-20895-jessicatseng, [Pelican] Add LED fading function
	// disable pattern function
	if(led_num == MT65XX_LED_LM3533_L1)
	{
		printk("[LED]LM3533 brightness_set_lm3533 blue pre_patternvalue=0x%x patternvalue=0x%x\n", patternvalue, (0xFE & patternvalue));
		lm3533_write_reg(0x28, (0xFE & patternvalue));
	}
	else if(led_num == MT65XX_LED_LM3533_L2)
	{
		printk("[LED]LM3533 brightness_set_lm3533 green pre_patternvalue=0x%x patternvalue=0x%x\n", patternvalue, (0xFB & patternvalue));
		lm3533_write_reg(0x28, (0xFB & patternvalue));
	}
	else if(led_num == MT65XX_LED_LM3533_L3)
	{
		printk("[LED]LM3533 brightness_set_lm3533 red pre_patternvalue=0x%x patternvalue=0x%x\n", patternvalue, (0xEF & patternvalue));
		lm3533_write_reg(0x28, (0xEF & patternvalue));
	}
//>2013/1/24-20895-jessicatseng
		
	// on/off
//<2013/3/7-22577-jessicatseng, [Pelican] Modify backlight setting for DP0 HW (add bank B)
#if !defined(HW_PDP)	
	if((led_num == MT65XX_LED_LM3533_H1) || (led_num == MT65XX_LED_LM3533_H2))
	{
//<2013/5/14-24898-jessicatseng, [5860] Fix no LED indicator when rebooting with USB / AC charger
//<2013/5/6-24610-jessicatseng, [5860] Show a white LED short pulse when user turns phone on
		//if(first_time)
		if(is_show_power_on_pulse)
		{
			is_show_power_on_pulse = KAL_FALSE;
			
			if(!((g_boot_mode == KERNEL_POWER_OFF_CHARGING_BOOT)
				  ||(g_boot_mode == LOW_POWER_OFF_CHARGING_BOOT)
				  ||(g_boot_reason == BR_WDT_BY_PASS_PWK)))
			{
			 enablevalue = 0x03;
			}
		}	
//>2013/5/6-24610-jessicatseng
//>2013/5/14-24898-jessicatseng
		
//<2013/5/7-24687-jessicatseng, [5860] Add VALUE_BUTTON_3 and VALUE_PATTERN_2 for LED illumination
		if(level)
			enablevalue = enablevalue | 0x03;
		else 
		{
			if(is_keep_light)
			{
			 is_keep_light = KAL_FALSE;
			 enablevalue = keep_light_color;
			 keep_light_color = 0x00;
			 lm3533_write_reg(0x28, 0x15);
			 hrtimer_start(&led_pulse_timer, ktime_set(1, 0), HRTIMER_MODE_REL);
			}
			
			enablevalue = enablevalue & 0xFC;
		}	
//>2013/5/7-24687-jessicatseng
	}
	else
#endif		
//>2013/3/7-22577-jessicatseng
	{
//<2013/5/14-24898-jessicatseng, [5860] Fix no LED indicator when rebooting with USB / AC charger
		if(is_show_power_on_pulse)
		{
			is_show_power_on_pulse = KAL_FALSE;	
		}
//>2013/5/14-24898-jessicatseng
		
		if(level)
			enablevalue = (1<<led_num) | enablevalue;
		else 
			enablevalue = ~(1<<led_num) & enablevalue;
	}
	
	lm3533_write_reg(0x27, enablevalue);

	//LEDS_DEBUG("[LED]LM3533 2 enablevalue = %d\n", enablevalue);
	
	mutex_unlock(&lm3533_i2c_access);
	
	return 0;
}
//>2013/3/14-22799-jessicatseng
//>2013/3/19-23017-jessicatseng

int __devinit lm3533_device_init(struct lm3533 *lm3533)
{
	// LCM backlight
//<2013/4/17-23965-jessicatseng, [5860] Modify backlight current to 19.4mA	
//<2013/3/20-23031-jessicatseng, [Pelican] Modify backlight OVP to 24V for DP0 HW
//<2013/3/7-22577-jessicatseng, [Pelican] Modify backlight setting for DP0 HW (add bank B)
#if !defined(HW_PDP)
	lm3533_write_reg(0x1F, 0x12);//0x13);	// 25.8mA // 20.2mA // set bank A full-scale current (5mA ~ 29.8mA)
//<2013/2/8-21777-jessicatseng, [Pelican] Modify brightness curve of backlight to exponential mode	
//<2013/3/21-23110-jessicatseng, [Pelican] Using SONY brightness curve
#if defined(USING_MAP_TABLE_LM3533)
	lm3533_write_reg(0x1A, 0x0A); // bank A => brightness mode, linear mode
#endif
//>2013/3/21-23110-jessicatseng
//>2013/2/8-21777-jessicatseng	
	lm3533_write_reg(0x40, 0xA0); // brightness value for bank A

	lm3533_write_reg(0x20, 0x12);//0x13);
	lm3533_write_reg(0x41, 0xA0); // brightness value for bank B
	lm3533_write_reg(0x2C, 0x0B); // Boost OVP
#else
	lm3533_write_reg(0x1F, 0x13);	// 20.2mA // set bank A full-scale current (5mA ~ 29.8mA)
	lm3533_write_reg(0x40, 0xA0); // brightness value for bank A
	lm3533_write_reg(0x2C, 0x0F); // Boost OVP
#endif
//>2013/3/7-22577-jessicatseng
//>2013/3/20-23032-jessicatseng
//>2013/4/17-23965-jessicatseng

//<2013/5/6-24610-jessicatseng, [5860] Show a white LED short pulse when user turns phone on
//<2013/6/5-25709-jessicatseng, [5860] Modify R/G/B LED current from 5mA to 13mA
//<2013/6/20-26167-jessicatseng, [5860]Modify RGB LED current for white color
	// blue LED	
	lm3533_write_reg(0x21, 0x0A); // 13mA // bank C full-scale current
	lm3533_write_reg(0x1B, 0x04); // bank C => disable pattern generator, brightness mode, linear mode
	lm3533_write_reg(0x42, 0xFF); // brightness value for bank C
	
	// green LED
	lm3533_write_reg(0x22, 0x0A); // 13mA // bank D full-scale current
	lm3533_write_reg(0x1C, 0x04); // bank D => disable pattern generator, brightness mode, linear mode
	lm3533_write_reg(0x43, 0xFF); // brightness value for bank D
//>2013/6/20-26167-jessicatseng
	
	// red LED
	lm3533_write_reg(0x23, 0x0A); // 13mA // bank E full-scale current
	lm3533_write_reg(0x1D, 0x04); // bank E => disable pattern generator, brightness mode, linear mode
	lm3533_write_reg(0x44, 0xFF); // brightness value for bank E
//>2013/6/5-25709-jessicatseng	
	
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
	if((KERNEL_POWER_OFF_CHARGING_BOOT != get_boot_mode())
		&&(LOW_POWER_OFF_CHARGING_BOOT != get_boot_mode()))
#endif		
	{
		// blue LED
		lm3533_write_reg(0x71, 0xFF);	// low-time
		lm3533_write_reg(0x74, 0x02); // rise-time
		lm3533_write_reg(0x75, 0x03); // fall-time
	
		// green LED
		lm3533_write_reg(0x81, 0xFF);	// low-time
		lm3533_write_reg(0x84, 0x02); // rise-time
		lm3533_write_reg(0x85, 0x03); // fall-time
	
		// red LED
		lm3533_write_reg(0x91, 0xFF);	// low-time
		lm3533_write_reg(0x94, 0x02); // rise-time
		lm3533_write_reg(0x95, 0x03); // fall-time

		lm3533_write_reg(0x28, 0x15);
		lm3533_write_reg(0x27, 0x1F);
		
//<2013/5/14-24898-jessicatseng, [5860] Fix no LED indicator when rebooting with USB / AC charger
		is_show_power_on_pulse = KAL_TRUE;
//>2013/5/14-24898-jessicatseng
 }
//>2013/5/6-24610-jessicatseng
	
	return 0;
}
//>2013/2/28-21837-jessicatseng

static int lm3533_i2c_probe(struct i2c_client *i2c, const struct i2c_device_id *id)
{
	struct lm3533 *lm3533;
	struct i2c_client *new_client;
	int ret;
	
	printk("[LED] lm3533_i2c_probe\n");
	
	lm3533 = kzalloc(sizeof(struct lm3533), GFP_KERNEL);
	
	if (!lm3533)
		return -ENOMEM;

	lm3533->dev = &i2c->dev;
	lm3533->i2c = i2c;	//obj->client = client;
	new_client = lm3533->i2c;
	i2c_set_clientdata(new_client, lm3533);
	lm3533_i2c_client = new_client;

	/*if(ret = lm3533_create_attr(&mt65xx_leds_driver.driver))
	{
		LEDS_DEBUG("create attribute ret = %d\n", ret);
	}*/
	
//<2013/5/7-24687-jessicatseng, [5860] Add VALUE_BUTTON_3 and VALUE_PATTERN_2 for LED illumination
	led_pulse_workqueue = create_singlethread_workqueue("led_pulse");
	INIT_WORK(&led_pulse_work, led_pulse_work_callback);
	
	hrtimer_init(&led_pulse_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	led_pulse_timer.function = led_pulse_timer_func;
//>2013/5/7-24687-jessicatseng

	ret = lm3533_device_init(lm3533);
	if(ret)
	{
		kfree(lm3533);
		return ret;
	}
	
	return 0;
}

static int lm3533_i2c_remove(struct i2c_client *i2c)
{
	struct lm3533 *lm3533 = i2c_get_clientdata(i2c);
	
//<2013/5/6-24610-jessicatseng, [5860] Show a white LED short pulse when user turns phone on
	lm3533_write_reg(0x28, 0x00);
	lm3533_write_reg(0x27, 0x00); 
//>2013/5/6-24610-jessicatseng
	
	lm3533_i2c_client = NULL;
	i2c_unregister_device(i2c);
	kfree(lm3533);
	
	return 0;
}
#endif
//>2013/1/17-20471-jessicatseng
 
//#if 0
static int brightness_mapto64(int level)
{
        if (level < 30)
                return (level >> 1) + 7;
        else if (level <= 120)
                return (level >> 2) + 14;
        else if (level <= 160)
                return level / 5 + 20;
        else
                return (level >> 3) + 33;
}



int find_time_index(int time)
{	
	int index = 0;	
	while(index < 8)	
	{		
		if(time<time_array[index])			
			return index;		
		else
			index++;
	}	
	return PWM_DIV_NUM-1;
}

#if defined CONFIG_ARCH_MT6589
static int led_set_pwm(int pwm_num, struct nled_setting* led)
{
	//struct pwm_easy_config pwm_setting;
	struct pwm_spec_config pwm_setting;
	int time_index = 0;
	pwm_setting.pwm_no = pwm_num;
    pwm_setting.mode = PWM_MODE_OLD;
    
        LEDS_DEBUG("[LED]led_set_pwm: mode=%d,pwm_no=%d\n", led->nled_mode, pwm_num);  
	//if((pwm_num != PWM3 && pwm_num != PWM4 && pwm_num != PWM5))//AP PWM all support OLD mode in MT6589
		pwm_setting.clk_src = PWM_CLK_OLD_MODE_32K;
	//else
		//pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
    
	switch (led->nled_mode)
	{
		case NLED_OFF :
			pwm_setting.PWM_MODE_OLD_REGS.THRESH = 0;
			pwm_setting.clk_div = CLK_DIV1;
			pwm_setting.PWM_MODE_OLD_REGS.DATA_WIDTH = 100;
			break;
            
		case NLED_ON :
			pwm_setting.PWM_MODE_OLD_REGS.THRESH = 30;
			pwm_setting.clk_div = CLK_DIV1;			
			pwm_setting.PWM_MODE_OLD_REGS.DATA_WIDTH = 100;
			break;
            
		case NLED_BLINK :
			LEDS_DEBUG("[LED]LED blink on time = %d offtime = %d\n",led->blink_on_time,led->blink_off_time);
			time_index = find_time_index(led->blink_on_time + led->blink_off_time);
			LEDS_DEBUG("[LED]LED div is %d\n",time_index);
			pwm_setting.clk_div = time_index;
			pwm_setting.PWM_MODE_OLD_REGS.DATA_WIDTH = (led->blink_on_time + led->blink_off_time) * MIN_FRE_OLD_PWM / div_array[time_index];
			pwm_setting.PWM_MODE_OLD_REGS.THRESH = (led->blink_on_time*100) / (led->blink_on_time + led->blink_off_time);
	}
	
	pwm_setting.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GDURATION = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;
	pwm_set_spec_config(&pwm_setting);

	return 0;
	
}
#endif
#if defined (CONFIG_ARCH_MT6577) || defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)
static int led_set_pwm(int pwm_num, struct nled_setting* led)
{
	struct pwm_easy_config pwm_setting;
	int time_index = 0;
	pwm_setting.pwm_no = pwm_num;
    
        LEDS_DEBUG("[LED]led_set_pwm: mode=%d,pwm_no=%d\n", led->nled_mode, pwm_num);  
	if((pwm_num != PWM3 && pwm_num != PWM4 && pwm_num != PWM5))
		pwm_setting.clk_src = PWM_CLK_OLD_MODE_32K;
	else
		pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
    
	switch (led->nled_mode)
	{
		case NLED_OFF :
			pwm_setting.duty = 0;
			pwm_setting.clk_div = CLK_DIV1;
			pwm_setting.duration = 100;
			break;
            
		case NLED_ON :
			pwm_setting.duty = 30;
			pwm_setting.clk_div = CLK_DIV1;			
			pwm_setting.duration = 100;
			break;
            
		case NLED_BLINK :
			LEDS_DEBUG("[LED]LED blink on time = %d offtime = %d\n",led->blink_on_time,led->blink_off_time);
			time_index = find_time_index(led->blink_on_time + led->blink_off_time);
			LEDS_DEBUG("[LED]LED div is %d\n",time_index);
			pwm_setting.clk_div = time_index;
			pwm_setting.duration = (led->blink_on_time + led->blink_off_time) * MIN_FRE_OLD_PWM / div_array[time_index];
			pwm_setting.duty = (led->blink_on_time*100) / (led->blink_on_time + led->blink_off_time);
	}
	pwm_set_easy_config(&pwm_setting);

	return 0;
	
}

#endif

//***********************MT6589 led breath funcion*******************************//
#if defined CONFIG_ARCH_MT6589
#if 0
static int led_breath_pmic(enum mt65xx_led_pmic pmic_type, struct nled_setting* led)
{
	//int time_index = 0;
	//int duty = 0;
	LEDS_DEBUG("[LED]led_blink_pmic: pmic_type=%d\n", pmic_type);  
	
	if((pmic_type != MT65XX_LED_PMIC_NLED_ISINK0 && pmic_type!= MT65XX_LED_PMIC_NLED_ISINK1) || led->nled_mode != NLED_BLINK) {
		return -1;
	}
				
	switch(pmic_type){
		case MT65XX_LED_PMIC_NLED_ISINK0://keypad center 4mA
			upmu_set_isinks_ch0_mode(PMIC_PWM_0);// 6320 spec 
			upmu_set_isinks_ch0_step(0x0);//4mA
			upmu_set_isinks_breath0_trf_sel(0x04);
			upmu_set_isinks_breath0_ton_sel(0x02);
			upmu_set_isinks_breath0_toff_sel(0x03);
			upmu_set_isink_dim1_duty(15);
			upmu_set_isink_dim1_fsel(11);//6320 0.25KHz
			upmu_set_rg_spk_div_pdn(0x01);
			upmu_set_rg_spk_pwm_div_pdn(0x01);
			upmu_set_isinks_ch0_en(0x01);
			break;
		case MT65XX_LED_PMIC_NLED_ISINK1://keypad LR  16mA
			upmu_set_isinks_ch1_mode(PMIC_PWM_1);// 6320 spec 
			upmu_set_isinks_ch1_step(0x3);//16mA
			upmu_set_isinks_breath0_trf_sel(0x04);
			upmu_set_isinks_breath0_ton_sel(0x02);
			upmu_set_isinks_breath0_toff_sel(0x03);
			upmu_set_isink_dim1_duty(15);
			upmu_set_isink_dim1_fsel(11);//6320 0.25KHz
			upmu_set_rg_spk_div_pdn(0x01);
			upmu_set_rg_spk_pwm_div_pdn(0x01);
			upmu_set_isinks_ch1_en(0x01);
			break;	
		default:
		break;
	}
	return 0;

}

#endif
#endif

#if defined CONFIG_ARCH_MT6589

#define PMIC_PERIOD_NUM 13
//int pmic_period_array[] = {250,500,1000,1250,1666,2000,2500,3333,4000,5000,6666,8000,10000,13333,16000};
int pmic_period_array[] = {250,500,1000,1250,1666,2000,2500,3333,4000,5000,6666,8000,10000};
u8 pmic_clksel_array[] = {0,0,0,0,0,0,1,1,1,2,2,2,3,3,3};
//u8 pmic_freqsel_array[] = {24,26,28,29,30,31,29,30,31,29,30,31,29,30,31};
//u8 pmic_freqsel_array[] = {21,22,23,24,25,26,24,25,26,24,25,26,24,25,26};
u8 pmic_freqsel_array[] = {21,22,23,24,24,24,25,26,26,26,28,28,28};


int find_time_index_pmic(int time_ms) {
	int i;
	for(i=0;i<PMIC_PERIOD_NUM;i++) {
		if(time_ms<=pmic_period_array[i]) {
			return i;
		} else {
			continue;
		}
	}
	return PMIC_PERIOD_NUM-1;
}

static int led_blink_pmic(enum mt65xx_led_pmic pmic_type, struct nled_setting* led) {
	int time_index = 0;
	int duty = 0;
	LEDS_DEBUG("[LED]led_blink_pmic: pmic_type=%d\n", pmic_type);  
	
	if((pmic_type != MT65XX_LED_PMIC_NLED_ISINK0 && pmic_type!= MT65XX_LED_PMIC_NLED_ISINK1 && pmic_type!= MT65XX_LED_PMIC_NLED_ISINK2) || led->nled_mode != NLED_BLINK) {
		return -1;
	}
				
	LEDS_DEBUG("[LED]LED blink on time = %d offtime = %d\n",led->blink_on_time,led->blink_off_time);
	time_index = find_time_index_pmic(led->blink_on_time + led->blink_off_time);
	LEDS_DEBUG("[LED]LED index is %d clksel=%d freqsel=%d\n", time_index, pmic_clksel_array[time_index], pmic_freqsel_array[time_index]);
	duty=32*led->blink_on_time/(led->blink_on_time + led->blink_off_time);
	switch(pmic_type){
		case MT65XX_LED_PMIC_NLED_ISINK0://keypad center 4mA
			upmu_set_rg_bst_drv_1m_ck_pdn(0x0);
			upmu_set_isinks_ch0_mode(PMIC_PWM_0);// 6320 spec 
			upmu_set_isinks_ch0_step(0x0);//4mA
			upmu_set_isink_dim0_duty(duty);
			upmu_set_isink_dim0_fsel(pmic_freqsel_array[time_index]);
			//upmu_set_rg_spk_pwm_div_pdn(0x01);
			//upmu_set_rg_bst_drv_1m_ck_pdn(0x0);
			#ifdef ISINK_CHOP_CLK
				//upmu_set_isinks0_chop_en(0x1);
			#endif
			upmu_set_isinks_breath0_trf_sel(0x0);
			upmu_set_isinks_breath0_ton_sel(0x02);
			upmu_set_isinks_breath0_toff_sel(0x05);
			
			upmu_set_isinks_ch0_en(0x01);
			break;
		case MT65XX_LED_PMIC_NLED_ISINK1://keypad LR  16mA
			upmu_set_rg_bst_drv_1m_ck_pdn(0x0);
			upmu_set_isinks_ch1_mode(PMIC_PWM_0);// 6320 spec 
			upmu_set_isinks_ch1_step(0x3);//16mA
			upmu_set_isink_dim1_duty(duty);
			upmu_set_isink_dim1_fsel(pmic_freqsel_array[time_index]);
			upmu_set_isinks_breath1_trf_sel(0x0);
			upmu_set_isinks_breath1_ton_sel(0x02);
			upmu_set_isinks_breath1_toff_sel(0x05);
			//upmu_set_rg_spk_pwm_div_pdn(0x01);
			
			#ifdef ISINK_CHOP_CLK
				//upmu_set_isinks1_chop_en(0x1);
			#endif
			upmu_set_isinks_ch1_en(0x01);
			break;	
		case MT65XX_LED_PMIC_NLED_ISINK2://keypad LR  16mA
			upmu_set_rg_bst_drv_1m_ck_pdn(0x0);
			upmu_set_isinks_ch2_mode(PMIC_PWM_0);// 6320 spec 
			upmu_set_isinks_ch2_step(0x3);//16mA
			upmu_set_isink_dim2_duty(duty);
			upmu_set_isink_dim2_fsel(pmic_freqsel_array[time_index]);
			//upmu_set_rg_spk_pwm_div_pdn(0x01);
			upmu_set_isinks_breath2_trf_sel(0x0);
			upmu_set_isinks_breath2_ton_sel(0x02);
			upmu_set_isinks_breath2_toff_sel(0x05);
			#ifdef ISINK_CHOP_CLK
				//upmu_set_isinks2_chop_en(0x1);
			#endif
			upmu_set_isinks_ch2_en(0x01);
			break;	
		default:
		break;
	}
	return 0;
}

#elif defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T) || defined (CONFIG_ARCH_MT6577)
#define PMIC_PERIOD_NUM 15
int pmic_period_array[] = {250,500,1000,1250,1666,2000,2500,3333,4000,5000,6666,8000,10000,13333,16000};
u8 pmic_clksel_array[] = {0,0,0,0,0,0,1,1,1,2,2,2,3,3,3};
u8 pmic_freqsel_array[] = {24,26,28,29,30,31,29,30,31,29,30,31,29,30,31};

int find_time_index_pmic(int time_ms) {
	int i;
	for(i=0;i<PMIC_PERIOD_NUM;i++) {
		if(time_ms<=pmic_period_array[i]) {
			return i;
		} else {
			continue;
		}
	}
	return PMIC_PERIOD_NUM-1;
}

static int led_blink_pmic(enum mt65xx_led_pmic pmic_type, struct nled_setting* led) {
	int time_index = 0;
	int duty = 0;
	LEDS_DEBUG("[LED]led_blink_pmic: pmic_type=%d\n", pmic_type);  
	
	if((pmic_type != MT65XX_LED_PMIC_NLED_ISINK4 && pmic_type!= MT65XX_LED_PMIC_NLED_ISINK5) || led->nled_mode != NLED_BLINK) {
		return -1;
	}
				
	LEDS_DEBUG("[LED]LED blink on time = %d offtime = %d\n",led->blink_on_time,led->blink_off_time);
	time_index = find_time_index_pmic(led->blink_on_time + led->blink_off_time);
	LEDS_DEBUG("[LED]LED index is %d clksel=%d freqsel=%d\n", time_index, pmic_clksel_array[time_index], pmic_freqsel_array[time_index]);
	duty=32*led->blink_on_time/(led->blink_on_time + led->blink_off_time);
	switch(pmic_type){
		case MT65XX_LED_PMIC_NLED_ISINK4:
			upmu_isinks_ch4_mode(PMIC_PWM_1);
			upmu_isinks_ch4_step(0x3);
			upmu_isinks_ch4_cabc_en(0);
			upmu_isinks_dim1_duty(duty);
			upmu_isinks_dim1_fsel(pmic_freqsel_array[time_index]);
			pmic_bank1_config_interface(0x2e, pmic_clksel_array[time_index], 0x3, 0x6);
			upmu_top2_bst_drv_ck_pdn(0x0);
			hwBacklightISINKTurnOn(MT65XX_LED_PMIC_NLED_ISINK4);
		break;
		case MT65XX_LED_PMIC_NLED_ISINK5:
			upmu_isinks_ch5_mode(PMIC_PWM_2);
			upmu_isinks_ch5_step(0x3);
			upmu_isinks_ch5_cabc_en(0);
			upmu_isinks_dim2_duty(duty);
			upmu_isinks_dim2_fsel(pmic_freqsel_array[time_index]);
			pmic_bank1_config_interface(0x30, pmic_clksel_array[time_index], 0x3, 0x6);
			upmu_top2_bst_drv_ck_pdn(0x0);
			hwBacklightISINKTurnOn(MT65XX_LED_PMIC_NLED_ISINK5);
		break;
		default:
		break;
	}
			
	return 0;
}
#endif

//#if 0
#if defined CONFIG_ARCH_MT6589
static int backlight_set_pwm(int pwm_num, u32 level, u32 div, struct PWM_config *config_data)
{
	struct pwm_spec_config pwm_setting;
	pwm_setting.pwm_no = pwm_num;
	pwm_setting.mode = PWM_MODE_FIFO; //new mode fifo and periodical mode

	pwm_setting.pmic_pad = config_data->pmic_pad;
		
	if(config_data->div)
	{
		pwm_setting.clk_div = config_data->div;
		backlight_PWM_div = config_data->div;
	}
	else
		pwm_setting.clk_div = div;
	if(config_data->clock_source)
		pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK;
	else
		pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
	
	if(config_data->High_duration && config_data->low_duration)
		{
			pwm_setting.PWM_MODE_FIFO_REGS.HDURATION = config_data->High_duration;
			pwm_setting.PWM_MODE_FIFO_REGS.LDURATION = pwm_setting.PWM_MODE_FIFO_REGS.HDURATION;
		}
	else
		{
			pwm_setting.PWM_MODE_FIFO_REGS.HDURATION = 4;
			pwm_setting.PWM_MODE_FIFO_REGS.LDURATION = 4;
		}
	pwm_setting.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 31;
	pwm_setting.PWM_MODE_FIFO_REGS.GDURATION = (pwm_setting.PWM_MODE_FIFO_REGS.HDURATION+1)*32 - 1;
	pwm_setting.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;
		
	LEDS_DEBUG("[LED]backlight_set_pwm:duty is %d\n", level);
	LEDS_DEBUG("[LED]backlight_set_pwm:clk_src/div/high/low is %d%d%d%d\n", pwm_setting.clk_src,pwm_setting.clk_div,pwm_setting.PWM_MODE_FIFO_REGS.HDURATION,pwm_setting.PWM_MODE_FIFO_REGS.LDURATION);
	if(level>0 && level <= 32)
	{
		pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA0 =  (1 << level) - 1 ;
		//pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0 ;
		pwm_set_spec_config(&pwm_setting);
	}else if(level>32 && level <=64)
	{
		pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 1;
		level -= 32;
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA0 = (1 << level) - 1 ;
		//pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA0 =  0xFFFFFFFF ;
		//pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA1 = (1 << level) - 1;
		pwm_set_spec_config(&pwm_setting);
	}else
	{
		LEDS_DEBUG("[LED]Error level in backlight\n");
		//mt_set_pwm_disable(pwm_setting.pwm_no);
		//mt_pwm_power_off(pwm_setting.pwm_no);
		mt_pwm_disable(pwm_setting.pwm_no, config_data->pmic_pad);
	}
		//printk("[LED]PWM con register is %x \n", INREG32(PWM_BASE + 0x0150));
	return 0;
}
#endif
//#endif
#if defined (CONFIG_ARCH_MT6577) || defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)
static int backlight_set_pwm(int pwm_num, u32 level, u32 div, struct PWM_config *config_data)
{
	struct pwm_spec_config pwm_setting;
	pwm_setting.pwm_no = pwm_num;
	pwm_setting.mode = PWM_MODE_FIFO; //new mode fifo and periodical mode
		
	#ifdef CONTROL_BL_TEMPERATURE
	mutex_lock(&bl_level_limit_mutex);
			current_level = level;
			//printk("brightness_set_pwm:current_level=%d\n", current_level);
			if(0 == limit_flag){
				last_level = level;
				//printk("brightness_set_pwm:last_level=%d\n", last_level);
			}else {
					if(limit < current_level){
						level = limit;
					//	printk("backlight_set_pwm: control level=%d\n", level);
					}
			}	
	mutex_unlock(&bl_level_limit_mutex);
	#endif
	if(config_data->div)
	{
		pwm_setting.clk_div = config_data->div;
		backlight_PWM_div = config_data->div;
	}
	else
		pwm_setting.clk_div = div;
	if(config_data->clock_source)
		pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK;
	else
		pwm_setting.clk_src = PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625;
	
	if(config_data->High_duration && config_data->low_duration)
		{
			pwm_setting.PWM_MODE_FIFO_REGS.HDURATION = config_data->High_duration;
			pwm_setting.PWM_MODE_FIFO_REGS.LDURATION = config_data->low_duration;
		}
	else
		{
			pwm_setting.PWM_MODE_FIFO_REGS.HDURATION = 4;
			pwm_setting.PWM_MODE_FIFO_REGS.LDURATION = 4;
		}
	pwm_setting.PWM_MODE_FIFO_REGS.IDLE_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.GUARD_VALUE = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
	pwm_setting.PWM_MODE_FIFO_REGS.GDURATION = 0;
	pwm_setting.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;
		
	LEDS_DEBUG("[LED]backlight_set_pwm:duty is %d\n", level);
	LEDS_DEBUG("[LED]backlight_set_pwm:clk_src/div/high/low is %d%d%d%d\n", pwm_setting.clk_src,pwm_setting.clk_div,pwm_setting.PWM_MODE_FIFO_REGS.HDURATION,pwm_setting.PWM_MODE_FIFO_REGS.LDURATION);
	if(level>0 && level <= 32)
	{
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA0 =  (1 << level) - 1 ;
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0 ;
		pwm_set_spec_config(&pwm_setting);
	}else if(level>32 && level <=64)
	{
		level -= 32;
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA0 =  0xFFFFFFFF ;
		pwm_setting.PWM_MODE_FIFO_REGS.SEND_DATA1 = (1 << level) - 1;
		pwm_set_spec_config(&pwm_setting);
	}else
	{
		LEDS_DEBUG("[LED]Error level in backlight\n");
		mt_set_pwm_disable(pwm_setting.pwm_no);
		mt_pwm_power_off(pwm_setting.pwm_no);
	}
		//printk("[LED]PWM con register is %x \n", INREG32(PWM_BASE + 0x0150));
	return 0;
}

#endif
#if defined CONFIG_ARCH_MT6589
static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, u32 level, u32 div)
{
		//int tmp_level = level;
		//static bool backlight_init_flag[2] = {false, false};
		static bool led_init_flag[3] = {false, false, false};
		static bool first_time = true;
		
		LEDS_DEBUG("[LED]PMIC#%d:%d\n", pmic_type, level);
	
		if (pmic_type == MT65XX_LED_PMIC_LCD_ISINK)
		{
		#if 0
			if(backlight_init_flag[0] == false)
			{
				hwBacklightISINKTuning(1, PMIC_PWM_0, 0x3, 0);
				hwBacklightISINKTuning(2, PMIC_PWM_0, 0x3, 0);
				hwBacklightISINKTuning(3, PMIC_PWM_0, 0x3, 0);
				backlight_init_flag[0] = true;
			}
			
			if (level) 
			{
				level = brightness_mapping(tmp_level);
				if(level == ERROR_BL_LEVEL)
					level = tmp_level/17;
				hwPWMsetting(PMIC_PWM_0, level, div);
				upmu_top2_bst_drv_ck_pdn(0x0);
				hwBacklightISINKTurnOn(1);
				hwBacklightISINKTurnOn(2);
				hwBacklightISINKTurnOn(3);
				bl_duty = level;	
			}
			else 
			{
				hwBacklightISINKTurnOff(1);
				hwBacklightISINKTurnOff(2);
				hwBacklightISINKTurnOff(3);
				bl_duty = level;	
			}
			return 0;
		}
		else if(pmic_type == MT65XX_LED_PMIC_LCD_BOOST)
		{
			/*
			if(backlight_init_flag[1] == false)
			{
				hwBacklightBoostTuning(PMIC_PWM_0, 0xC, 0);
				backlight_init_flag[1] = true;
			}
			*/		
			if (level) 
			{
				level = brightness_mapping(tmp_level);
				if(level == ERROR_BL_LEVEL)
					level = tmp_level/42;
		
				upmu_boost_isink_hw_sel(0x1);
				upmu_boost_mode(3);
				upmu_boost_cabc_en(0);
	
				switch(level)
				{
					case 0: 			
						upmu_boost_vrsel(0x0);
						//hwPWMsetting(PMIC_PWM_0, 0, 0x15);
						break;
					case 1:
						upmu_boost_vrsel(0x1);
						//hwPWMsetting(PMIC_PWM_0, 4, 0x15);
						break;
					case 2:
						upmu_boost_vrsel(0x2);
						//hwPWMsetting(PMIC_PWM_0, 5, 0x15);
						break;
					case 3:
						upmu_boost_vrsel(0x3);
						//hwPWMsetting(PMIC_PWM_0, 6, 0x15);
						break;
					case 4:
						upmu_boost_vrsel(0x5);
						//hwPWMsetting(PMIC_PWM_0, 7, 0x15);
						break;
					case 5:
						upmu_boost_vrsel(0x8);
						//hwPWMsetting(PMIC_PWM_0, 8, 0x15);
						break;
					case 6:
						upmu_boost_vrsel(0xB);
						//hwPWMsetting(PMIC_PWM_0, 9, 0x15);
						break;
					default:
						printk("[LED] invalid brightness level %d->%d\n", tmp_level, level);
						break;
				}
				
				upmu_top2_bst_drv_ck_pdn(0x0);
				upmu_boost_en(0x1);
				bl_duty = level;	
			}
			else 
			{
				upmu_boost_en(0x0);
				bl_duty = level;	
				//upmu_top2_bst_drv_ck_pdn(0x1);
			}
			return 0;
		#endif
		}
		
		else if (pmic_type == MT65XX_LED_PMIC_BUTTON) 
		{
			
			if (level) 
			{
				upmu_set_kpled_dim_duty(0x9);
				upmu_set_kpled_en(0x1);
			}
			else 
			{
				upmu_set_kpled_en(0x0);
			}
			return 0;
			
		}
		else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK0)
		{
			if(first_time == true)
			{
				upmu_set_isinks_ch1_en(0x0);  //sw workround for sync leds status 
				upmu_set_isinks_ch2_en(0x0); 
				first_time = false;
			}
	
					//hwBacklightISINKTuning(0x0, 0x3, 0x0, 0);  //register mode, ch0_step=4ma ,disable CABC
					// hwBacklightISINKTurnOn(0x0);  //turn on ISINK0 77 uses ISINK4&5
					
	
			//if(led_init_flag[0] == false)
			{
				//hwBacklightISINKTuning(MT65XX_LED_PMIC_NLED_ISINK4, PMIC_PWM_1, 0x0, 0);
				upmu_set_isinks_ch0_mode(PMIC_PWM_0);
				upmu_set_isinks_ch0_step(0x0);//4mA
				//hwPWMsetting(PMIC_PWM_1, 15, 8);
				upmu_set_isink_dim0_duty(15);
				upmu_set_isink_dim0_fsel(11);//6320 0.25KHz
				led_init_flag[0] = true;
			}
			
			if (level) 
			{
				//upmu_top2_bst_drv_ck_pdn(0x0);
				//upmu_set_rg_spk_div_pdn(0x01);
				//upmu_set_rg_spk_pwm_div_pdn(0x01);
				
				upmu_set_rg_bst_drv_1m_ck_pdn(0x0);
				#ifdef ISINK_CHOP_CLK
				//upmu_set_isinks0_chop_en(0x1);
				#endif
				upmu_set_isinks_ch0_en(0x01);
				
			}
			else 
			{
				upmu_set_isinks_ch0_en(0x00);
				//upmu_set_rg_bst_drv_1m_ck_pdn(0x1);
				//upmu_top2_bst_drv_ck_pdn(0x1);
			}
			return 0;
		}
		else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK1)
		{
			if(first_time == true)
			{
				upmu_set_isinks_ch0_en(0);  //sw workround for sync leds status
				upmu_set_isinks_ch2_en(0); 
				first_time = false;
			}
	
					//hwBacklightISINKTuning(0x0, 0x3, 0x0, 0);  //register mode, ch0_step=4ma ,disable CABC
					//hwBacklightISINKTurnOn(0x0);  //turn on ISINK0
	
			//if(led_init_flag[1] == false)
			{
				//hwBacklightISINKTuning(MT65XX_LED_PMIC_NLED_ISINK5, PMIC_PWM_2, 0x0, 0);
				upmu_set_isinks_ch1_mode(PMIC_PWM_0);
				upmu_set_isinks_ch1_step(0x3);//4mA
				//hwPWMsetting(PMIC_PWM_2, 15, 8);
				upmu_set_isink_dim1_duty(15);
				upmu_set_isink_dim1_fsel(11);//6320 0.25KHz
				led_init_flag[1] = true;
			}
			if (level) 
			{
				//upmu_top2_bst_drv_ck_pdn(0x0);
				
				//upmu_set_rg_spk_div_pdn(0x01);
				//upmu_set_rg_spk_pwm_div_pdn(0x01);
				upmu_set_rg_bst_drv_1m_ck_pdn(0x0);
				#ifdef ISINK_CHOP_CLK
				//upmu_set_isinks1_chop_en(0x1);
				#endif
				upmu_set_isinks_ch1_en(0x01);
			}
			else 
			{
				upmu_set_isinks_ch1_en(0x00);
				//upmu_set_rg_bst_drv_1m_ck_pdn(0x1);
				//upmu_top2_bst_drv_ck_pdn(0x1);
			}
			return 0;
		}
		else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK2)
		{
		
			if(first_time == true)
			{
				upmu_set_isinks_ch0_en(0);  //sw workround for sync leds status
				upmu_set_isinks_ch1_en(0);
				first_time = false;
			}
	
					//hwBacklightISINKTuning(0x0, 0x3, 0x0, 0);  //register mode, ch0_step=4ma ,disable CABC
					//hwBacklightISINKTurnOn(0x0);  //turn on ISINK0
	
			//if(led_init_flag[1] == false)
			{
				//hwBacklightISINKTuning(MT65XX_LED_PMIC_NLED_ISINK5, PMIC_PWM_2, 0x0, 0);
				upmu_set_isinks_ch2_mode(PMIC_PWM_0);
				upmu_set_isinks_ch2_step(0x3);//16mA
				//hwPWMsetting(PMIC_PWM_2, 15, 8);
				upmu_set_isink_dim2_duty(15);
				upmu_set_isink_dim2_fsel(11);//6320 0.25KHz
				led_init_flag[2] = true;
			}
			if (level) 
			{
				//upmu_top2_bst_drv_ck_pdn(0x0);
				
				//upmu_set_rg_spk_div_pdn(0x01);
				//upmu_set_rg_spk_pwm_div_pdn(0x01);
				upmu_set_rg_bst_drv_1m_ck_pdn(0x0);
				#ifdef ISINK_CHOP_CLK
				//upmu_set_isinks2_chop_en(0x1);
				#endif
				upmu_set_isinks_ch2_en(0x01);
			}
			else 
			{
				upmu_set_isinks_ch2_en(0x00);
				//upmu_set_rg_bst_drv_1m_ck_pdn(0x1);
				//upmu_top2_bst_drv_ck_pdn(0x1);
			}
			return 0;
		}
		else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK01)
		{
	
			//hwBacklightISINKTuning(0x0, 0x3, 0x0, 0);  //register mode, ch0_step=4ma ,disable CABC
			//hwBacklightISINKTurnOn(0x0);  //turn on ISINK0
	
			//if(led_init_flag[1] == false)
			{

				upmu_set_isinks_ch0_mode(PMIC_PWM_0);
				upmu_set_isinks_ch0_step(0x0);//4mA
				upmu_set_isink_dim0_duty(1);
				upmu_set_isink_dim0_fsel(1);//6320 1.5KHz
				//hwBacklightISINKTuning(MT65XX_LED_PMIC_NLED_ISINK5, PMIC_PWM_2, 0x0, 0);
				upmu_set_isinks_ch1_mode(PMIC_PWM_0);
				upmu_set_isinks_ch1_step(0x3);//4mA
				//hwPWMsetting(PMIC_PWM_2, 15, 8);
				upmu_set_isink_dim1_duty(15);
				upmu_set_isink_dim1_fsel(11);//6320 0.25KHz
				led_init_flag[0] = true;
				led_init_flag[1] = true;
			}
			if (level) 
			{
				//upmu_top2_bst_drv_ck_pdn(0x0);
				
				//upmu_set_rg_spk_div_pdn(0x01);
				//upmu_set_rg_spk_pwm_div_pdn(0x01);
				upmu_set_rg_bst_drv_1m_ck_pdn(0x0);
				#ifdef ISINK_CHOP_CLK
				//upmu_set_isinks0_chop_en(0x1);
				//upmu_set_isinks1_chop_en(0x1);
				#endif
				upmu_set_isinks_ch0_en(0x01);
				upmu_set_isinks_ch1_en(0x01);
			}
			else 
			{
				upmu_set_isinks_ch0_en(0x00);
				upmu_set_isinks_ch1_en(0x00);
				//upmu_set_rg_bst_drv_1m_ck_pdn(0x1);
				//upmu_top2_bst_drv_ck_pdn(0x1);
			}
			return 0;
		}
		return -1;

}


#elif  defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)|| defined (CONFIG_ARCH_MT6577)
static int brightness_set_pmic(enum mt65xx_led_pmic pmic_type, u32 level, u32 div)
{
	int tmp_level = level;
	static bool backlight_init_flag[2] = {false, false};
	static bool led_init_flag[2] = {false, false};
	static bool first_time = true;
	
	LEDS_DEBUG("[LED]PMIC#%d:%d\n", pmic_type, level);

	if (pmic_type == MT65XX_LED_PMIC_LCD_ISINK)
	{
		if(backlight_init_flag[0] == false)
		{
			hwBacklightISINKTuning(1, PMIC_PWM_0, 0x3, 0);
			hwBacklightISINKTuning(2, PMIC_PWM_0, 0x3, 0);
			hwBacklightISINKTuning(3, PMIC_PWM_0, 0x3, 0);
			backlight_init_flag[0] = true;
		}
		
		if (level) 
		{
			level = brightness_mapping(tmp_level);
			if(level == ERROR_BL_LEVEL)
				level = tmp_level/17;
			hwPWMsetting(PMIC_PWM_0, level, div);
			upmu_top2_bst_drv_ck_pdn(0x0);
			hwBacklightISINKTurnOn(1);
			hwBacklightISINKTurnOn(2);
			hwBacklightISINKTurnOn(3);
			bl_duty = level;	
		}
		else 
		{
			hwBacklightISINKTurnOff(1);
			hwBacklightISINKTurnOff(2);
			hwBacklightISINKTurnOff(3);
			bl_duty = level;	
		}
		return 0;
	}
	else if(pmic_type == MT65XX_LED_PMIC_LCD_BOOST)
	{
		/*
		if(backlight_init_flag[1] == false)
		{
			hwBacklightBoostTuning(PMIC_PWM_0, 0xC, 0);
			backlight_init_flag[1] = true;
		}
		*/		
		if (level) 
		{
			level = brightness_mapping(tmp_level);
			if(level == ERROR_BL_LEVEL)
				level = tmp_level/42;
	
			upmu_boost_isink_hw_sel(0x1);
			upmu_boost_mode(3);
			upmu_boost_cabc_en(0);

			switch(level)
			{
				case 0:				
					upmu_boost_vrsel(0x0);
					//hwPWMsetting(PMIC_PWM_0, 0, 0x15);
					break;
				case 1:
					upmu_boost_vrsel(0x1);
					//hwPWMsetting(PMIC_PWM_0, 4, 0x15);
					break;
				case 2:
					upmu_boost_vrsel(0x2);
					//hwPWMsetting(PMIC_PWM_0, 5, 0x15);
					break;
				case 3:
					upmu_boost_vrsel(0x3);
					//hwPWMsetting(PMIC_PWM_0, 6, 0x15);
					break;
				case 4:
					upmu_boost_vrsel(0x5);
					//hwPWMsetting(PMIC_PWM_0, 7, 0x15);
					break;
				case 5:
					upmu_boost_vrsel(0x8);
					//hwPWMsetting(PMIC_PWM_0, 8, 0x15);
					break;
				case 6:
					upmu_boost_vrsel(0xB);
					//hwPWMsetting(PMIC_PWM_0, 9, 0x15);
					break;
				default:
					printk("[LED] invalid brightness level %d->%d\n", tmp_level, level);
					break;
			}
			
			upmu_top2_bst_drv_ck_pdn(0x0);
			upmu_boost_en(0x1);
			bl_duty = level;	
		}
		else 
		{
			upmu_boost_en(0x0);
			bl_duty = level;	
			//upmu_top2_bst_drv_ck_pdn(0x1);
		}
		return 0;
	}
	else if (pmic_type == MT65XX_LED_PMIC_BUTTON) 
	{
		if (level) 
		{
			hwBacklightKPLEDTuning(0x9, 0x0);
			hwBacklightKPLEDTurnOn();
		}
		else 
		{
			hwBacklightKPLEDTurnOff();
		}
		return 0;
	}
	else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK4)
	{
		if(first_time == true)
		{
			hwBacklightISINKTurnOff(MT65XX_LED_PMIC_NLED_ISINK5);  //sw workround for sync leds status 
			first_time = false;
		}

                hwBacklightISINKTuning(0x0, 0x3, 0x0, 0);  //register mode, ch0_step=4ma ,disable CABC
                hwBacklightISINKTurnOn(0x0);  //turn on ISINK0

		//if(led_init_flag[0] == false)
		{
			hwBacklightISINKTuning(MT65XX_LED_PMIC_NLED_ISINK4, PMIC_PWM_1, 0x0, 0);
			hwPWMsetting(PMIC_PWM_1, 15, 8);
			led_init_flag[0] = true;
		}
		
		if (level) 
		{
			upmu_top2_bst_drv_ck_pdn(0x0);
			hwBacklightISINKTurnOn(MT65XX_LED_PMIC_NLED_ISINK4);
		}
		else 
		{
			hwBacklightISINKTurnOff(MT65XX_LED_PMIC_NLED_ISINK4);
			//upmu_top2_bst_drv_ck_pdn(0x1);
		}
		return 0;
	}
	else if(pmic_type == MT65XX_LED_PMIC_NLED_ISINK5)
	{
		if(first_time == true)
		{
			hwBacklightISINKTurnOff(MT65XX_LED_PMIC_NLED_ISINK4);  //sw workround for sync leds status
			first_time = false;
		}

                hwBacklightISINKTuning(0x0, 0x3, 0x0, 0);  //register mode, ch0_step=4ma ,disable CABC
                hwBacklightISINKTurnOn(0x0);  //turn on ISINK0

		//if(led_init_flag[1] == false)
		{
			hwBacklightISINKTuning(MT65XX_LED_PMIC_NLED_ISINK5, PMIC_PWM_2, 0x0, 0);
			hwPWMsetting(PMIC_PWM_2, 15, 8);
			led_init_flag[1] = true;
		}
		if (level) 
		{
			upmu_top2_bst_drv_ck_pdn(0x0);
			hwBacklightISINKTurnOn(MT65XX_LED_PMIC_NLED_ISINK5);
		}
		else 
		{
			hwBacklightISINKTurnOff(MT65XX_LED_PMIC_NLED_ISINK5);
			//upmu_top2_bst_drv_ck_pdn(0x1);
		}
		return 0;
	}
	
	return -1;
}

#endif


#if 0
static int brightness_set_gpio(int gpio_num, enum led_brightness level)
{
	LEDS_DEBUG("[LED]GPIO#%d:%d\n", gpio_num, level);
	mt_set_gpio_mode(gpio_num, GPIO_MODE_GPIO);
	mt_set_gpio_dir(gpio_num, GPIO_DIR_OUT);

	if (level)
		mt_set_gpio_out(gpio_num, GPIO_OUT_ONE);
	else
		mt_set_gpio_out(gpio_num, GPIO_OUT_ZERO);

	return 0;
}
#endif

#if defined CONFIG_ARCH_MT6589
static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level)
{
	struct nled_setting led_tmp_setting = {0,0,0};
	int tmp_level = level;
	if (level > LED_FULL)
		level = LED_FULL;
	else if (level < 0)
		level = 0;

    printk("mt65xx_leds_set_cust: set brightness, name:%s, mode:%d, level:%d\n", 
		cust->name, cust->mode, level);
	switch (cust->mode) {
		
		case MT65XX_LED_MODE_PWM:
			if(strcmp(cust->name,"lcd-backlight") == 0)
			{
				bl_brightness = level;
				if(level == 0)
				{
					//mt_set_pwm_disable(cust->data);
					//mt_pwm_power_off (cust->data);
					mt_pwm_disable(cust->data, cust->config_data.pmic_pad);
					
				}else
				{
					level = brightness_mapping(tmp_level);
					if(level == ERROR_BL_LEVEL)
						level = brightness_mapto64(tmp_level);
						
					backlight_set_pwm(cust->data, level, bl_div,&cust->config_data);
				}
                bl_duty = level;	
				
			}else
			{
				if(level == 0)
				{
					led_tmp_setting.nled_mode = NLED_OFF;
				}else
				{
					led_tmp_setting.nled_mode = NLED_ON;
				}
				led_set_pwm(cust->data,&led_tmp_setting);
			}
			return 1;
          
		case MT65XX_LED_MODE_GPIO:
			//printk("brightness_set_cust:go GPIO mode!!!!!\n");
			#ifdef CONTROL_BL_TEMPERATURE
			mutex_lock(&bl_level_limit_mutex);
			current_level = level;
			//printk("brightness_set_cust:current_level=%d\n", current_level);
			if(0 == limit_flag){
				last_level = level;
				//printk("brightness_set_cust:last_level=%d\n", last_level);
			}else {
					if(limit < current_level){
						level = limit;
						printk("backlight_set_cust: control level=%d\n", level);
					}
			}	
			mutex_unlock(&bl_level_limit_mutex);
			#endif
			return ((cust_set_brightness)(cust->data))(level);
              
		case MT65XX_LED_MODE_PMIC:
			return brightness_set_pmic(cust->data, level, bl_div);
            
		case MT65XX_LED_MODE_CUST_LCM:
            if(strcmp(cust->name,"lcd-backlight") == 0)
			{
			    bl_brightness = level;
            }
			#ifdef CONTROL_BL_TEMPERATURE
			mutex_lock(&bl_level_limit_mutex);
			current_level = level;
			//printk("brightness_set_cust:current_level=%d\n", current_level);
			if(0 == limit_flag){
				last_level = level;
				//printk("brightness_set_cust:last_level=%d\n", last_level);
			}else {
					if(limit < current_level){
						level = limit;
						printk("backlight_set_cust: control level=%d\n", level);
					}
			}	
			mutex_unlock(&bl_level_limit_mutex);
			#endif
			//printk("brightness_set_cust:backlight control by LCM\n");
			return ((cust_brightness_set)(cust->data))(level, bl_div);

		
		case MT65XX_LED_MODE_CUST_BLS_PWM:
					if(strcmp(cust->name,"lcd-backlight") == 0)
					{
						bl_brightness = level;
					}
			#ifdef CONTROL_BL_TEMPERATURE
					mutex_lock(&bl_level_limit_mutex);
					current_level = level;
					//printk("brightness_set_cust:current_level=%d\n", current_level);
					if(0 == limit_flag){
						last_level = level;
						//printk("brightness_set_cust:last_level=%d\n", last_level);
					}else {
							if(limit < current_level){
								level = limit;
								printk("backlight_set_cust: control level=%d\n", level);
							}
					}	
					mutex_unlock(&bl_level_limit_mutex);
			#endif
					//printk("brightness_set_cust:backlight control by BLS_PWM!!\n");
			//#if !defined (MTK_AAL_SUPPORT)
			return ((cust_set_brightness)(cust->data))(level);
			printk("brightness_set_cust:backlight control by BLS_PWM done!!\n");
			//#endif
			
//<2013/1/21-20642-jessicatseng, [Pelican] Fix backlight can't be turned off
#if defined(BACKLIGHT_IC_LM3533)
		case MT65XX_LED_MODE_LM3533:
			return brightness_set_lm3533(cust->data, level);
#endif			
//>2013/1/21-20642-jessicatseng
            
		case MT65XX_LED_MODE_NONE:
		default:
			break;
	}
	return -1;
}

#elif defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)|| defined (CONFIG_ARCH_MT6577)
	static int brightness_set_gpio(int gpio_num, enum led_brightness level)
	{
		LEDS_DEBUG("[LED]GPIO#%d:%d\n", gpio_num, level);
		mt_set_gpio_mode(gpio_num, GPIO_MODE_GPIO);
		mt_set_gpio_dir(gpio_num, GPIO_DIR_OUT);
	
		if (level)
			mt_set_gpio_out(gpio_num, GPIO_OUT_ONE);
		else
			mt_set_gpio_out(gpio_num, GPIO_OUT_ZERO);
	
		return 0;
	}

	static int mt65xx_led_set_cust(struct cust_mt65xx_led *cust, int level)
	{
		struct nled_setting led_tmp_setting = {0,0,0};
		int tmp_level;
	
	/*#ifdef CONTROL_BL_TEMPERATURE
		level = control_backlight_brightness(level);
		
#endif*/
		tmp_level = level;
		if (level > LED_FULL)
			level = LED_FULL;
		else if (level < 0)
			level = 0;
	
		printk("mt65xx_leds_set_cust: set brightness, name:%s, mode:%d, level:%d\n", 
			cust->name, cust->mode, level);
		switch (cust->mode) {
			case MT65XX_LED_MODE_PWM:
				if(strcmp(cust->name,"lcd-backlight") == 0)
				{
					bl_brightness = level;
					if(level == 0)
					{
						mt_set_pwm_disable(cust->data);
						mt_pwm_power_off (cust->data);
					}else
					{
						level = brightness_mapping(tmp_level);
						if(level == ERROR_BL_LEVEL)
							level = brightness_mapto64(tmp_level);
							
						backlight_set_pwm(cust->data, level, bl_div,&cust->config_data);
					}
					bl_duty = level;	
					
				}else
				{
					if(level == 0)
					{
						led_tmp_setting.nled_mode = NLED_OFF;
					}else
					{
						led_tmp_setting.nled_mode = NLED_ON;
					}
					led_set_pwm(cust->data,&led_tmp_setting);
				}
				return 1;
				
			case MT65XX_LED_MODE_GPIO:
				return brightness_set_gpio(cust->data, level);
				
			case MT65XX_LED_MODE_PMIC:
				return brightness_set_pmic(cust->data, level, bl_div);
				
			case MT65XX_LED_MODE_CUST:
				if(strcmp(cust->name,"lcd-backlight") == 0)
				{
					bl_brightness = level;
				}
			#ifdef CONTROL_BL_TEMPERATURE
				mutex_lock(&bl_level_limit_mutex);
				current_level = level;
				//printk("brightness_set_cust:current_level=%d\n", current_level);
				if(0 == limit_flag){
					last_level = level;
					//printk("brightness_set_cust:last_level=%d\n", last_level);
				}else {
						if(limit < current_level){
							level = limit;
							printk("backlight_set_cust: control level=%d\n", level);
						}
				}	
				mutex_unlock(&bl_level_limit_mutex);
			#endif
				return ((cust_brightness_set)(cust->data))(level, bl_div);

//<2013/1/17-20471-jessicatseng, [Pelican] Integrate backlight IC LM3533 for PRE-MP SW
#if defined(BACKLIGHT_IC_LM3533)
		case MT65XX_LED_MODE_LM3533:
			return brightness_set_lm3533(cust->data, level);
#endif			
//>2013/1/17-20471-jessicatseng

			case MT65XX_LED_MODE_NONE:
			default:
				break;
		}
		return -1;
	}

#endif
static void mt65xx_led_work(struct work_struct *work)
{
	struct mt65xx_led_data *led_data =
		container_of(work, struct mt65xx_led_data, work);

	LEDS_DEBUG("[LED]%s:%d\n", led_data->cust.name, led_data->level);
	mutex_lock(&leds_mutex);
	mt65xx_led_set_cust(&led_data->cust, led_data->level);
	mutex_unlock(&leds_mutex);
}

static void mt65xx_led_set(struct led_classdev *led_cdev, enum led_brightness level)
{
	struct mt65xx_led_data *led_data =
		container_of(led_cdev, struct mt65xx_led_data, cdev);

	// do something only when level is changed
	if (led_data->level != level) {
		led_data->level = level;
		if(strcmp(led_data->cust.name,"lcd-backlight"))
		{
				schedule_work(&led_data->work);
		}else
		{
				LEDS_DEBUG("[LED]Set Backlight directly %d at time %lu\n",led_data->level,jiffies);
				mt65xx_led_set_cust(&led_data->cust, led_data->level);	
		}
	}
}

static int  mt65xx_blink_set(struct led_classdev *led_cdev,
			     unsigned long *delay_on,
			     unsigned long *delay_off)
{
	struct mt65xx_led_data *led_data =
		container_of(led_cdev, struct mt65xx_led_data, cdev);
	static int got_wake_lock = 0;
	struct nled_setting nled_tmp_setting = {0,0,0};
	

	// only allow software blink when delay_on or delay_off changed
	if (*delay_on != led_data->delay_on || *delay_off != led_data->delay_off) {
		led_data->delay_on = *delay_on;
		led_data->delay_off = *delay_off;
		if (led_data->delay_on && led_data->delay_off) { // enable blink
//<2013/3/14-22799-jessicatseng, [Pelican] Modify R/G/B LED's level can be controlled by android
			if(led_data->level > 255)
				led_data->level = 255; // when enable blink  then to set the level  (255)
//>2013/3/14-22799-jessicatseng				
			//if(led_data->cust.mode == MT65XX_LED_MODE_PWM && 
			//(led_data->cust.data != PWM3 && led_data->cust.data != PWM4 && led_data->cust.data != PWM5))

			//AP PWM all support OLD mode in MT6589
			
			if(led_data->cust.mode == MT65XX_LED_MODE_PWM)
			{
				nled_tmp_setting.nled_mode = NLED_BLINK;
				nled_tmp_setting.blink_off_time = led_data->delay_off;
				nled_tmp_setting.blink_on_time = led_data->delay_on;
				led_set_pwm(led_data->cust.data,&nled_tmp_setting);
				return 0;
			}

			#if defined CONFIG_ARCH_MT6589
			else if((led_data->cust.mode == MT65XX_LED_MODE_PMIC) && (led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK0
				|| led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK1 || led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK2))
			{
				//if(get_chip_eco_ver() == CHIP_E2) {
				if(1){
					
					nled_tmp_setting.nled_mode = NLED_BLINK;
					nled_tmp_setting.blink_off_time = led_data->delay_off;
					nled_tmp_setting.blink_on_time = led_data->delay_on;
					led_blink_pmic(led_data->cust.data, &nled_tmp_setting);
					return 0;
				} else {
					wake_lock(&leds_suspend_lock);
				}
			}
			#endif		
			
			#if defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)|| defined (CONFIG_ARCH_MT6577)
			else if((led_data->cust.mode == MT65XX_LED_MODE_PMIC) && (led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK4
				|| led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK5))
			{
				if(get_chip_eco_ver() == CHIP_E2) {
					nled_tmp_setting.nled_mode = NLED_BLINK;
					nled_tmp_setting.blink_off_time = led_data->delay_off;
					nled_tmp_setting.blink_on_time = led_data->delay_on;
					led_blink_pmic(led_data->cust.data, &nled_tmp_setting);
					return 0;
				} else {
					wake_lock(&leds_suspend_lock);
				}
			}
			#endif		

//<2013/3/14-22799-jessicatseng, [Pelican] Modify R/G/B LED's level can be controlled by android
//<2013/1/17-20471-jessicatseng, [Pelican] Integrate backlight IC LM3533 for PRE-MP SW
			#if defined(BACKLIGHT_IC_LM3533)
			else if(led_data->cust.mode == MT65XX_LED_MODE_LM3533)
			{
					nled_tmp_setting.nled_mode = NLED_BLINK;
					nled_tmp_setting.blink_off_time = led_data->delay_off;
					nled_tmp_setting.blink_on_time = led_data->delay_on;
					blink_set_lm3533(led_data->cust.data, &nled_tmp_setting, led_data->level);
				return 0;
			}
			#endif
//>2013/1/17-20471-jessicatseng
//>2013/3/14-22799-jessicatseng
			
			else if (!got_wake_lock) {
				wake_lock(&leds_suspend_lock);
				got_wake_lock = 1;
			}
		}
		else if (!led_data->delay_on && !led_data->delay_off) { // disable blink
			//if(led_data->cust.mode == MT65XX_LED_MODE_PWM && 
			//(led_data->cust.data != PWM3 && led_data->cust.data != PWM4 && led_data->cust.data != PWM5))

			//AP PWM all support OLD mode in MT6589
			
			if(led_data->cust.mode == MT65XX_LED_MODE_PWM)
			{
				nled_tmp_setting.nled_mode = NLED_OFF;
				led_set_pwm(led_data->cust.data,&nled_tmp_setting);
				return 0;
			}

			#if defined CONFIG_ARCH_MT6589
			else if((led_data->cust.mode == MT65XX_LED_MODE_PMIC) && (led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK0
				|| led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK1 || led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK2))
			{
				//if(get_chip_eco_ver() == CHIP_E2) {
				if(1){
					brightness_set_pmic(led_data->cust.data, 0, 0);
					return 0;
				} else {
					wake_unlock(&leds_suspend_lock);
				}
			}
			#endif	
			
			#if defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)|| defined (CONFIG_ARCH_MT6577)
			else if((led_data->cust.mode == MT65XX_LED_MODE_PMIC) && (led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK4
				|| led_data->cust.data == MT65XX_LED_PMIC_NLED_ISINK5))
			{
				if(get_chip_eco_ver() == CHIP_E2) {
					brightness_set_pmic(led_data->cust.data, 0, 0);
					return 0;
				} else {
					wake_unlock(&leds_suspend_lock);
				}
			}
			#endif	
			
//<2013/1/17-20471-jessicatseng, [Pelican] Integrate backlight IC LM3533 for PRE-MP SW
			#if defined(BACKLIGHT_IC_LM3533)
			else if(led_data->cust.mode == MT65XX_LED_MODE_LM3533)
			{
				brightness_set_lm3533(led_data->cust.data, 0);
				return 0;
			}
			#endif
//>2013/1/17-20471-jessicatseng

			else if (got_wake_lock) {
				wake_unlock(&leds_suspend_lock);
				got_wake_lock = 0;
			}
		}
		return -1;
	}

	// delay_on and delay_off are not changed
	return 0;
}

/****************************************************************************
 * external functions
 ***************************************************************************/
int mt65xx_leds_brightness_set(enum mt65xx_led_type type, enum led_brightness level)
{
	struct cust_mt65xx_led *cust_led_list = get_cust_led_list();

	LEDS_DEBUG("[LED]#%d:%d\n", type, level);

	if (type < 0 || type >= MT65XX_LED_TYPE_TOTAL)
		return -1;

	if (level > LED_FULL)
		level = LED_FULL;
	else if (level < 0)
		level = 0;

	return mt65xx_led_set_cust(&cust_led_list[type], level);

}

EXPORT_SYMBOL(mt65xx_leds_brightness_set);


static ssize_t show_duty(struct device *dev,struct device_attribute *attr, char *buf)
{
	LEDS_DEBUG("[LED]get backlight duty value is:%d \n",bl_duty);
	return sprintf(buf, "%u\n", bl_duty);
}
static ssize_t store_duty(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	char *pvalue = NULL;
	unsigned int level = 0;
	size_t count = 0;
	LEDS_DEBUG("set backlight duty start \n");
	level = simple_strtoul(buf,&pvalue,10);
	count = pvalue - buf;
	if (*pvalue && isspace(*pvalue))
		count++;
    
	if(count == size)
	{
	
		if(bl_setting->mode == MT65XX_LED_MODE_PMIC)
		{
			
#if defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)|| defined (CONFIG_ARCH_MT6577)
			//duty:0-16
			if((level >= 0) && (level <= 15))
			{
				brightness_set_pmic(MT65XX_LED_PMIC_LCD_BOOST, (level*17), bl_div);
			}
			else
			{
				LEDS_DEBUG("duty value is error, please select vaule from [0-15]!\n");
			}
#endif			
		}
		
		else if(bl_setting->mode == MT65XX_LED_MODE_PWM)
		//if(bl_setting->mode == MT65XX_LED_MODE_PWM)
		{
			if(level == 0)
			{
			#if defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)|| defined (CONFIG_ARCH_MT6577)
				mt_set_pwm_disable(bl_setting->data);
				mt_pwm_power_off (bl_setting->data);
			#elif defined CONFIG_ARCH_MT6589
				mt_pwm_disable(bl_setting->data, bl_setting->config_data.pmic_pad);
			#endif
			}else if(level <= 64)
			{
				backlight_set_pwm(bl_setting->data,level, bl_div,&bl_setting->config_data);
			}	
		}
            
    
		bl_duty = level;

	}
	
	return size;
}


static DEVICE_ATTR(duty, 0664, show_duty, store_duty);


static ssize_t show_div(struct device *dev,struct device_attribute *attr, char *buf)
{
	LEDS_DEBUG("get backlight div value is:%d \n",bl_div);
	return sprintf(buf, "%u\n", bl_div);
}

static ssize_t store_div(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	char *pvalue = NULL;
	unsigned int div = 0;
	size_t count = 0;
	LEDS_DEBUG("set backlight div start \n");
	div = simple_strtoul(buf,&pvalue,10);
	count = pvalue - buf;
	
	if (*pvalue && isspace(*pvalue))
		count++;
		
	if(count == size)
	{
         if(div < 0 || (div > 7))
		{
            LEDS_DEBUG("set backlight div parameter error: %d[div:0~7]\n", div);
            return 0;
		}
        
		if(bl_setting->mode == MT65XX_LED_MODE_PWM)
		{
            LEDS_DEBUG("set PWM backlight div OK: div=%d, duty=%d\n", div, bl_duty);
            backlight_set_pwm(bl_setting->data, bl_duty, div,&bl_setting->config_data);
		}
		
        else if(bl_setting->mode == MT65XX_LED_MODE_CUST_LCM || bl_setting->mode == MT65XX_LED_MODE_CUST_BLS_PWM)
        {
            LEDS_DEBUG("set cust backlight div OK: div=%d, brightness=%d\n", div, bl_brightness);
	        ((cust_brightness_set)(bl_setting->data))(bl_brightness, div);
        }
        
		bl_div = div;
	}
	
	return size;
}

static DEVICE_ATTR(div, 0664, show_div, store_div);


static ssize_t show_frequency(struct device *dev,struct device_attribute *attr, char *buf)
{
    if(bl_setting->mode == MT65XX_LED_MODE_PWM)
    {
        bl_frequency = 32000/div_array[bl_div];
    }
    else if(bl_setting->mode == MT65XX_LED_MODE_CUST_LCM)
    {
        //mtkfb_get_backlight_pwm(bl_div, &bl_frequency);  		
    }

    LEDS_DEBUG("[LED]get backlight PWM frequency value is:%d \n", bl_frequency);
    
	return sprintf(buf, "%u\n", bl_frequency);
}

static DEVICE_ATTR(frequency, 0664, show_frequency, NULL);



static ssize_t store_pwm_register(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	char *pvalue = NULL;
	unsigned int reg_value = 0;
	unsigned int reg_address = 0;
	if(buf != NULL && size != 0)
	{
		LEDS_DEBUG("store_pwm_register: size:%d,address:0x%s\n", size,buf);
		reg_address = simple_strtoul(buf,&pvalue,16);
	
		if(*pvalue && (*pvalue == '#'))
		{
			reg_value = simple_strtoul((pvalue+1),NULL,16);
			LEDS_DEBUG("set pwm register:[0x%x]= 0x%x\n",reg_address,reg_value);
			OUTREG32(reg_address,reg_value);
			
		}else if(*pvalue && (*pvalue == '@'))
		{
			LEDS_DEBUG("get pwm register:[0x%x]=0x%x\n",reg_address,INREG32(reg_address));
		}	
	}

	return size;
}

static ssize_t show_pwm_register(struct device *dev,struct device_attribute *attr, char *buf)
{
	return 0;
}

static DEVICE_ATTR(pwm_register, 0664, show_pwm_register, store_pwm_register);

//<2013/4/2-23447-ricehuang, [Pelican] connect native(lights.c) to driver(leds.c)
#if defined(BACKLIGHT_IC_LM3533)
static ssize_t store_lm3533(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	
	lm3533_led_data *ledData = (lm3533_led_data *)buf;	
        LEDS_DEBUG("[LED] size=%d, pattern_id=%d  , red=%x,  green=%x , blue=%x\n",  size,ledData->pattern_id, ledData->red, ledData->green,  ledData->blue);	 
 //<2013/5/14-24904-ricehuang,add falsh_mode for lm3533 light	
	pattern_set_lm3533(NULL, ledData->pattern_id, ledData->flash_mode ,ledData->red, ledData->green, ledData->blue);	
	//pattern_set_lm3533(NULL, ledData->pattern_id, ledData->red, ledData->green, ledData->blue);	
//>2013/5/14-24904-ricehuang	       		
	
	return size;
}

static DEVICE_ATTR(lm3533, 0664, NULL, store_lm3533);


//<2013/4/23-24160-ricehuang, [xssm]Power saving in LED
static ssize_t show_psnotification(struct device *dev,struct device_attribute *attr, char *buf)
{
	LEDS_DEBUG("show_psnotification=%d\n", notification_value_enable);
	return sprintf(buf, "%u", notification_value_enable);
}

static ssize_t show_psattention(struct device *dev,struct device_attribute *attr, char *buf)
{
	LEDS_DEBUG("show_psnotification=%d \n", attention_value_enable);
	return sprintf(buf, "%u", attention_value_enable);
}


static ssize_t store_psnotification(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{	
	char *pvalue = NULL;
	notification_value_enable = simple_strtoul(buf,&pvalue,10);
	LEDS_DEBUG("store_psnotification=%s",buf);	
	return size;
}

static ssize_t store_psattention(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{

	char *pvalue = NULL;
	attention_value_enable = simple_strtoul(buf,&pvalue,10);
	LEDS_DEBUG("store_psattention=%s",buf);	
	return size;
}

static DEVICE_ATTR(psnotification, 0664, show_psnotification, store_psnotification);
static DEVICE_ATTR(psattention, 0664, show_psattention, store_psattention);
//>2013/4/23-24160-ricehuang	

#endif
//>2013/4/2-23447-ricehuang
/****************************************************************************
 * driver functions
 ***************************************************************************/
static int __init mt65xx_leds_probe(struct platform_device *pdev)
{
	int i;
	int ret, rc;
	struct cust_mt65xx_led *cust_led_list = get_cust_led_list();
	LEDS_DEBUG("[LED]%s\n", __func__);

	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (cust_led_list[i].mode == MT65XX_LED_MODE_NONE) {
			g_leds_data[i] = NULL;
			continue;
		}

		g_leds_data[i] = kzalloc(sizeof(struct mt65xx_led_data), GFP_KERNEL);
		if (!g_leds_data[i]) {
			ret = -ENOMEM;
			goto err;
		}

		g_leds_data[i]->cust.mode = cust_led_list[i].mode;
		g_leds_data[i]->cust.data = cust_led_list[i].data;
		g_leds_data[i]->cust.name = cust_led_list[i].name;

		g_leds_data[i]->cdev.name = cust_led_list[i].name;
		g_leds_data[i]->cust.config_data = cust_led_list[i].config_data;//bei add

		g_leds_data[i]->cdev.brightness_set = mt65xx_led_set;
		g_leds_data[i]->cdev.blink_set = mt65xx_blink_set;

		INIT_WORK(&g_leds_data[i]->work, mt65xx_led_work);

		ret = led_classdev_register(&pdev->dev, &g_leds_data[i]->cdev);
        
		if(strcmp(g_leds_data[i]->cdev.name,"lcd-backlight") == 0)
		{
			rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_duty);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file duty fail!\n");
            }
            
            rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_div);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file duty fail!\n");
            }
            
            rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_frequency);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file duty fail!\n");
            }
            
	    rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_pwm_register);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file duty fail!\n");
            }

//<2013/4/2-23447-ricehuang, [Pelican] connect native(lights.c) to driver(leds.c) 
	    #if defined(BACKLIGHT_IC_LM3533)
            rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_lm3533);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file lm3533 fail!\n");
            }	

//<2013/4/23-24160-ricehuang, [xssm]Power saving in LED
            rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_psnotification);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file psnotification fail!\n");
            }	

            rc = device_create_file(g_leds_data[i]->cdev.dev, &dev_attr_psattention);
            if(rc)
            {
                LEDS_DEBUG("[LED]device_create_file psnotification fail!\n");
            }	
//>2013/4/23-24160-ricehuang				
	     #endif
//>2013/4/2-23447-ricehuang	    
			bl_setting = &g_leds_data[i]->cust;
		}

		if (ret)
			goto err;
		
	}
#ifdef CONTROL_BL_TEMPERATURE
	
		last_level = 0;  
		limit = 255;
		limit_flag = 0; 
		current_level = 0;
		printk("[LED]led probe last_level = %d, limit = %d, limit_flag = %d, current_level = %d\n",last_level,limit,limit_flag,current_level);
#endif


	return 0;

err:
	if (i) {
		for (i = i-1; i >=0; i--) {
			if (!g_leds_data[i])
				continue;
			led_classdev_unregister(&g_leds_data[i]->cdev);
			cancel_work_sync(&g_leds_data[i]->work);
			kfree(g_leds_data[i]);
			g_leds_data[i] = NULL;
		}
	}

	return ret;
}

static int mt65xx_leds_remove(struct platform_device *pdev)
{
	int i;
	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (!g_leds_data[i])
			continue;
		led_classdev_unregister(&g_leds_data[i]->cdev);
		cancel_work_sync(&g_leds_data[i]->work);
		kfree(g_leds_data[i]);
		g_leds_data[i] = NULL;
	}

	return 0;
}

/*
static int mt65xx_leds_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}
*/
#if defined CONFIG_ARCH_MT6589
static void mt65xx_leds_shutdown(struct platform_device *pdev)
{
	int i;
    struct nled_setting led_tmp_setting = {NLED_OFF,0,0};
    
    LEDS_DEBUG("[LED]%s\n", __func__);
    printk("[LED]mt65xx_leds_shutdown: turn off backlight\n");
    
	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (!g_leds_data[i])
			continue;
		switch (g_leds_data[i]->cust.mode) {
			
		    case MT65XX_LED_MODE_PWM:
			    if(strcmp(g_leds_data[i]->cust.name,"lcd-backlight") == 0)
			    {
					//mt_set_pwm_disable(g_leds_data[i]->cust.data);
					//mt_pwm_power_off (g_leds_data[i]->cust.data);
					mt_pwm_disable(g_leds_data[i]->cust.data, g_leds_data[i]->cust.config_data.pmic_pad);
			    }else
			    {
				    led_set_pwm(g_leds_data[i]->cust.data,&led_tmp_setting);
			    }
                break;
                
		   // case MT65XX_LED_MODE_GPIO:
			//    brightness_set_gpio(g_leds_data[i]->cust.data, 0);
            //    break;
                
		    case MT65XX_LED_MODE_PMIC:
			    brightness_set_pmic(g_leds_data[i]->cust.data, 0, 0);
                break;
		    case MT65XX_LED_MODE_CUST_LCM:
				printk("[LED]backlight control through LCM!!1\n");
			    ((cust_brightness_set)(g_leds_data[i]->cust.data))(0, bl_div);
                break;
            case MT65XX_LED_MODE_CUST_BLS_PWM:
				printk("[LED]backlight control through BLS!!1\n");
			    ((cust_set_brightness)(g_leds_data[i]->cust.data))(0);
                break;    
		    case MT65XX_LED_MODE_NONE:
		    default:
			    break;
          }
	}

}
#elif defined (CONFIG_ARCH_MT6575) || defined (CONFIG_ARCH_MT6575T)|| defined (CONFIG_ARCH_MT6577)
static void mt65xx_leds_shutdown(struct platform_device *pdev)
{
	int i;
    struct nled_setting led_tmp_setting = {NLED_OFF,0,0};
    
    LEDS_DEBUG("[LED]%s\n", __func__);
    printk("[LED]mt65xx_leds_shutdown: turn off backlight\n");
    
	for (i = 0; i < MT65XX_LED_TYPE_TOTAL; i++) {
		if (!g_leds_data[i])
			continue;
		switch (g_leds_data[i]->cust.mode) {
		    case MT65XX_LED_MODE_PWM:
			    if(strcmp(g_leds_data[i]->cust.name,"lcd-backlight") == 0)
			    {
					mt_set_pwm_disable(g_leds_data[i]->cust.data);
					mt_pwm_power_off (g_leds_data[i]->cust.data);
			    }else
			    {
				    led_set_pwm(g_leds_data[i]->cust.data,&led_tmp_setting);
			    }
                break;
                
		    case MT65XX_LED_MODE_GPIO:
			    brightness_set_gpio(g_leds_data[i]->cust.data, 0);
                break;
                
		    case MT65XX_LED_MODE_PMIC:
			    brightness_set_pmic(g_leds_data[i]->cust.data, 0, 0);
                break;
                
		    case MT65XX_LED_MODE_CUST:
			    ((cust_brightness_set)(g_leds_data[i]->cust.data))(0, bl_div);
                break;
                
		    case MT65XX_LED_MODE_NONE:
		    default:
			    break;
          }
	}

}
#endif


static struct platform_driver mt65xx_leds_driver = {
	.driver		= {
		.name	= "leds-mt65xx",
		.owner	= THIS_MODULE,
	},
	.probe		= mt65xx_leds_probe,
	.remove		= mt65xx_leds_remove,
	//.suspend	= mt65xx_leds_suspend,
	.shutdown   = mt65xx_leds_shutdown,
};

#if 0
static struct platform_device mt65xx_leds_device = {
	.name = "leds-mt65xx",
	.id = -1
};

#endif

static int __init mt65xx_leds_init(void)
{
	int ret;

	LEDS_DEBUG("[LED]%s\n", __func__);
	
//<2013/1/17-20471-jessicatseng, [Pelican] Integrate backlight IC LM3533 for PRE-MP SW
#if defined(BACKLIGHT_IC_LM3533)
	i2c_register_board_info(3, &i2c_lm3533, 1);
	
	if(i2c_add_driver(&lm3533_i2c_driver))
	{
		printk("LED]lm3533 add I2C driver error\n");
		return -1;
	}		
#endif	
//>2013/1/17-20471-jessicatseng

#if 0
	ret = platform_device_register(&mt65xx_leds_device);
	if (ret)
		printk("[LED]mt65xx_leds_init:dev:E%d\n", ret);
#endif
	ret = platform_driver_register(&mt65xx_leds_driver);

	if (ret)
	{
		printk("[LED]mt65xx_leds_init:drv:E%d\n", ret);
//		platform_device_unregister(&mt65xx_leds_device);
		return ret;
	}

	wake_lock_init(&leds_suspend_lock, WAKE_LOCK_SUSPEND, "leds wakelock");

	return ret;
}

static void __exit mt65xx_leds_exit(void)
{
	platform_driver_unregister(&mt65xx_leds_driver);
//	platform_device_unregister(&mt65xx_leds_device);
}

module_param(debug_enable, int,0644);

module_init(mt65xx_leds_init);
module_exit(mt65xx_leds_exit);

MODULE_AUTHOR("MediaTek Inc.");
MODULE_DESCRIPTION("LED driver for MediaTek MT65xx chip");
MODULE_LICENSE("GPL");
MODULE_ALIAS("leds-mt65xx");

