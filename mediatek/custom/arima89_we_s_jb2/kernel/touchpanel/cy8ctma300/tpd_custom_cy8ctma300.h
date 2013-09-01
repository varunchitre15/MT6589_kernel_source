#ifndef TOUCHPANEL_H__
#define TOUCHPANEL_H__

/* Pre-defined definition */
#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE
#define TPD_POWER_SOURCE         MT6575_POWER_VGP2
#define TPD_I2C_NUMBER           0
#define TPD_WAKEUP_TRIAL         60
#define TPD_WAKEUP_DELAY         100

#define TPD_DELAY                (2*HZ/100)
//#define TPD_RES_X                480
//#define TPD_RES_Y                800
#define TPD_CALIBRATION_MATRIX  {962,0,0,0,1600,0,0,0};

#define TPD_HAVE_CALIBRATION
//#define TPD_HAVE_BUTTON
#define TPD_HAVE_TREMBLE_ELIMINATION

#define TPD_NO_GPIO
//#define TPD_RESET_PIN_ADDR   (PERICFG_BASE + 0xC000)
#define TPD_GPIO_GPO_ADDR (0xc100c000)
#define TPD_GPIO_OE_ADDR (0xc100c008)

#define PRESSURE_FACTOR	10

#endif /* TOUCHPANEL_H__ */
