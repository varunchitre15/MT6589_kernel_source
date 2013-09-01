#ifndef _CUST_BAT_H_
#define _CUST_BAT_H_

#include <platform/mt_typedefs.h>

typedef enum
{
	Cust_CC_1600MA = 0x0,
	Cust_CC_1500MA = 0x1,
	Cust_CC_1400MA = 0x2,
	Cust_CC_1300MA = 0x3,
	Cust_CC_1200MA = 0x4,
	Cust_CC_1100MA = 0x5,
	Cust_CC_1000MA = 0x6,
	Cust_CC_900MA  = 0x7,
	Cust_CC_800MA  = 0x8,
	Cust_CC_700MA  = 0x9,
	Cust_CC_650MA  = 0xA,
	Cust_CC_550MA  = 0xB,
	Cust_CC_450MA  = 0xC,
	Cust_CC_400MA  = 0xD,
	Cust_CC_200MA  = 0xE,
	Cust_CC_70MA   = 0xF,
	Cust_CC_0MA	   = 0xDD
}cust_charging_current_enum;

typedef struct{	
	unsigned int BattVolt;
	unsigned int BattPercent;
}VBAT_TO_PERCENT;

//<2013/1/30-21231-jessicatseng, [Pelican] Re-charging if battery's temperature recover to 10 ~ 45 degree

#define RECOVERY_CHARGING_FUNCTION

/* Battery Temperature Protection */
#define MAX_CHARGE_TEMPERATURE  55//50
//<2013/4/23-24179-jessicatseng, [5860] Modify the range of charging temperature
#define MIN_CHARGE_TEMPERATURE  5//0
//>2013/4/23-24179-jessicatseng
#define ERR_CHARGE_TEMPERATURE  0xFF

#define MAX_RECOVERY_CHARGE_TEMPERATURE  45
//<2013/4/24-24262-jessicatseng, [5860] Battery's voltage charge up to 4.0V if temperature is higher than 45 degree
#define MIX_RECOVERY_CHARGE_TEMPERATURE  5//10
//>2013/4/24-24262-jessicatseng
//>2013/1/30-21231-jessicatseng

/* Recharging Battery Voltage */
#define RECHARGING_VOLTAGE      4110

/* Charging Current Setting */
#define USB_CHARGER_CURRENT					Cust_CC_450MA
#define AC_CHARGER_CURRENT					Cust_CC_650MA	

/* Battery Meter Solution */
#define CONFIG_ADC_SOLUTION 	1

/* Battery Voltage and Percentage Mapping Table */
VBAT_TO_PERCENT Batt_VoltToPercent_Table[] = {
	/*BattVolt,BattPercent*/
	{3400,0},
	{3686,10},
	{3740,20},
	{3771,30},
	{3789,40},
	{3820,50},
	{3873,60},
	{3943,70},
	{4013,80},
	{4100,90},
	{4189,100},
};

/* Precise Tunning */
//#define BATTERY_AVERAGE_SIZE 	600
#define BATTERY_AVERAGE_SIZE 	60


#define CHARGING_IDLE_MODE	 1

#define CHARGING_PICTURE	 1

/* Common setting */
#define R_CURRENT_SENSE 2				// 0.2 Ohm
#define R_BAT_SENSE 4					// times of voltage
#define R_I_SENSE 4						// times of voltage
#define R_CHARGER_1 330
#define R_CHARGER_2 39
#define R_CHARGER_SENSE ((R_CHARGER_1+R_CHARGER_2)/R_CHARGER_2)	// times of voltage
#define V_CHARGER_MAX 6500				// 6.5 V
#define V_CHARGER_MIN 4400				// 4.4 V
#define V_CHARGER_ENABLE 0				// 1:ON , 0:OFF
#define BACKLIGHT_KEY 10					// camera key

/* Teperature related setting */
#define RBAT_PULL_UP_R             39000
#define RBAT_PULL_UP_VOLT          1800
//#define TBAT_OVER_CRITICAL_LOW     68237
//#define TBAT_OVER_CRITICAL_LOW     483954
#define TBAT_OVER_CRITICAL_LOW     67790
#define BAT_TEMP_PROTECT_ENABLE    1
#define BAT_NTC_10 0
#define BAT_NTC_47 0
//<2013/1/21-20649-jessicatseng, [Pelican] Modify the mapping table of temperature and NTC
//#define BAT_NTC_CG103JF103F
#define BAT_NTC_27 1
//>2013/1/21-20649-jessicatseng

/* The option of new charging animation */
#define ANIMATION_NEW

#endif /* _CUST_BAT_H_ */ 
