#ifndef FTM_CUST_H
#define FTM_CUST_H

#define FEATURE_FTM_AUDIO
//#define FEATURE_DUMMY_AUDIO
#define FEATURE_FTM_BATTERY
#define FEATURE_FTM_PMIC_632X
#define BATTERY_TYPE_Z3
#define FEATURE_FTM_SWCHR_I_68mohm
#define FEATURE_FTM_BT
#define FEATURE_FTM_FM
//#define FEATURE_FTM_FMTX
#define FEATURE_FTM_GPS
#if defined(MTK_WLAN_SUPPORT)
#define FEATURE_FTM_WIFI
#endif
#define FEATURE_FTM_MAIN_CAMERA
#define FEATURE_FTM_SUB_CAMERA
#ifdef MTK_EMMC_SUPPORT
#define FEATURE_FTM_EMMC
#define FEATURE_FTM_CLEAREMMC
#else
#define FEATURE_FTM_FLASH
#define FEATURE_FTM_CLEARFLASH
#endif
#define FEATURE_FTM_KEYS
#define FEATURE_FTM_LCD
#define FEATURE_FTM_LED
#define FEATURE_FTM_MEMCARD
#define FEATURE_FTM_RTC
#define FEATURE_FTM_TOUCH
#define FEATURE_FTM_VIBRATOR
#define FEATURE_FTM_IDLE
//#define FEATURE_FTM_CLEARFLASH
#define FEATURE_FTM_ACCDET
#define FEATURE_FTM_HEADSET
#define HEADSET_BUTTON_DETECTION
#ifdef HAVE_MATV_FEATURE
#define FEATURE_FTM_MATV
#endif
//#define FEATURE_FTM_FONT_10x18
#define FEATURE_FTM_OTG
#define FEATURE_FTM_SIM
#define FEATURE_FTM_SPK_OC
#define FEATURE_FTM_WAVE_PLAYBACK

#include "cust_font.h"		/* common part */
#include "cust_keys.h"		/* custom part */
#include "cust_lcd.h"		/* custom part */
#include "cust_led.h"		/* custom part */
#include "cust_touch.h"         /* custom part */

#endif /* FTM_CUST_H */
