#ifndef TOUCHPANEL_H
#define TOUCHPANEL_H
//#include <mt6516_kpd.h>

/* turn on only one of them */
//#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE

#define TPD_DELAY               HZ/50 //(1*HZ/200)
//#define TPD_RES_X               480
//#define TPD_RES_Y               800
#define TPD_PRESSURE_MAX        22000 //11000
#define TPD_PRESSURE_MIN        500 //1000
#define TPD_PRESSURE_NICE       15000 //9000
#define TPD_COUNT_TOLERANCE     0
//#define TPD_CALIBRATION_MATRIX  {-1183,6,1127068,-21,1969,-75047, 0, 0};
//#define TPD_CALIBRATION_MATRIX  {630,4,-228605,83,-961,3502366,28,-51};
#define TPD_CALIBRATION_MATRIX  {532,0,-106496,0,-880,3440640};

#define TPD_HAVE_TREMBLE_ELIMINATION

#define TPD_HAVE_CALIBRATION

  //#define TPD_HAVE_DRIFT_ELIMINATION

//#define TPD_HAVE_BUTTON
//#define TPD_BUTTON_HEIGHT       410

/*#define TPD_HAVE_VIRTUAL_KEY
#define TPD_KEY_COUNT           3
#define TPD_KEYS                {KEY_MENU, KEY_HOME, KEY_BACK}
#define TPD_KEYS_DIM            {"40:425:80:50","120:425:80:50","200:425:80:50"}*/


  //#define TPD_HAVE_ADV_DRIFT_ELIMINATION
#define TPD_ADE_P1 1800
#define TPD_ADE_P2 2000

//#define TPD_HAVE_TRACK_EXTENSION

#endif
