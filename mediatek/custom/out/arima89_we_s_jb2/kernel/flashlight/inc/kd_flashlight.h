#ifndef _KD_FLASHLIGHT_H
#define _KD_FLASHLIGHT_H

#include <linux/ioctl.h>

/*************************************************
*
**************************************************/
//In KERNEL mode,SHOULD be sync with mediatype.h
//CHECK before remove or modify
//#undef BOOL
//#define BOOL signed int
#ifndef _MEDIA_TYPES_H
typedef unsigned char   MUINT8;
typedef unsigned short  MUINT16;
typedef unsigned int    MUINT32;
typedef signed char     MINT8;
typedef signed short    MINT16;
typedef signed int      MINT32;
#endif

//<2013/1/18-20610-jessicatseng, [Pelican] Integrate flashlight IC LM3642 for PRE-MP SW
#define FLASHLIGHT_IC_LM3642
//>2013/1/18-20610-jessicatseng

/* cotta-- added for high current solution */
#define KD_STROBE_HIGH_CURRENT_WIDTH 0xFF

/* cotta-- time limit of strobe watch dog timer. unit : ms */
#define FLASH_LIGHT_WDT_TIMEOUT_MS 300



//FIXME: temp. solutoin for main/sub sensor mapping
#define KD_DEFAULT_FLASHLIGHT_INDEX    0
#define KD_CUSTOM_FLASHLIGHT_INDEX    1


#define KD_DEFAULT_FLASHLIGHT_ID    0
#define KD_DUMMY_FLASHLIGHT_ID      1
#define KD_PEAK_FLASHLIGHT_ID       2
#define KD_TORCH_FLASHLIGHT_ID      3
#define KD_CONSTANT_FLASHLIGHT_ID   4

typedef struct
{
    int (* flashlight_open)(void *pArg);
    int (* flashlight_release)(void *pArg);
    int (* flashlight_ioctl)(MUINT32 cmd, MUINT32 arg);
} FLASHLIGHT_FUNCTION_STRUCT, *PFLASHLIGHT_FUNCTION_STRUCT;

typedef struct
{
	MUINT32 flashlightId;
    MUINT32 (* flashlightInit)(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc);
} KD_FLASHLIGHT_INIT_FUNCTION_STRUCT, *pKD_FLASHLIGHT_INIT_FUNCTION_STRUCT;

typedef enum {
    FLASHLIGHTDRV_STATE_PREVIEW,
    FLASHLIGHTDRV_STATE_STILL,
}eFlashlightState;

//flash type enum
typedef enum
{
    FLASHLIGHT_NONE = 0,
    FLASHLIGHT_LED_ONOFF,           // LED always on/off
    FLASHLIGHT_LED_CONSTANT,        // CONSTANT type LED
    FLASHLIGHT_LED_PEAK,            // peak strobe type LED
    FLASHLIGHT_LED_TORCH,           // LED turn on when switch FLASH_ON
    FLASHLIGHT_XENON_SCR,           // SCR strobe type Xenon
    FLASHLIGHT_XENON_IGBT           // IGBT strobe type Xenon
}   FLASHLIGHT_TYPE_ENUM;


#define FLASHLIGHT_MAGIC 'S'
//S means "set through a ptr"
//T means "tell by a arg value"
//G means "get by a ptr"
//Q means "get by return a value"
//X means "switch G and S atomically"
//H means "switch T and Q atomically"

//FLASHLIGHTIOC_T_ENABLE : Tell FLASHLIGHT to turn ON/OFF
#define FLASHLIGHTIOC_T_ENABLE _IOW(FLASHLIGHT_MAGIC,5, unsigned long )

//set flashlight power level 0~31(weak~strong)
#define FLASHLIGHTIOC_T_LEVEL _IOW(FLASHLIGHT_MAGIC,10, unsigned long)

//set flashlight time us
#define FLASHLIGHTIOC_T_FLASHTIME _IOW(FLASHLIGHT_MAGIC,15, unsigned long)

//set flashlight state
#define FLASHLIGHTIOC_T_STATE _IOW(FLASHLIGHT_MAGIC,20, unsigned long)

//get flash type
#define FLASHLIGHTIOC_G_FLASHTYPE _IOR(FLASHLIGHT_MAGIC,25, int)

//set flashlight driver
#define FLASHLIGHTIOC_X_SET_DRIVER _IOWR(FLASHLIGHT_MAGIC,30,unsigned long)

/* cotta-- set capture delay of sensor */
#define FLASHLIGHTIOC_T_DELAY _IOW(FLASHLIGHT_MAGIC, 35, unsigned int)


#define FLASH_IOC_SET_TIME_OUT_TIME_MS  _IOR(FLASHLIGHT_MAGIC, 100, int)
#define FLASH_IOC_SET_STEP 		        _IOR(FLASHLIGHT_MAGIC, 105, int)
#define FLASH_IOC_SET_DUTY				_IOR(FLASHLIGHT_MAGIC, 110, int)
#define FLASH_IOC_SET_ONOFF           	_IOR(FLASHLIGHT_MAGIC, 115, int)




#endif

