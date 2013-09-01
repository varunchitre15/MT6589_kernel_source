/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ar0833mipiraw_Sensor.h
 *
 * Project:
 * --------
 *   YUSU
 *
 * Description:
 * ------------
 *   Header file of Sensor driver
 *
 *
 * Author:
 * -------
 *   Kenny Chuang (mtk04214)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef _AR0833_MIPI_RAW_SENSOR_H_
#define _AR0833_MIPI_RAW_SENSOR_H_

typedef enum group_enum {
    PRE_GAIN=0,
    CMMCLK_CURRENT,
    FRAME_RATE_LIMITATION,
    REGISTER_EDITOR,
    GROUP_TOTAL_NUMS
} FACTORY_GROUP_ENUM;


#define ENGINEER_START_ADDR 10
#define FACTORY_START_ADDR 0


typedef enum register_index
{
    SENSOR_BASEGAIN=FACTORY_START_ADDR,
    PRE_GAIN_R_INDEX,
    PRE_GAIN_Gr_INDEX,
    PRE_GAIN_Gb_INDEX,
    PRE_GAIN_B_INDEX,
    FACTORY_END_ADDR
} FACTORY_REGISTER_INDEX;

typedef enum engineer_index
{
    CMMCLK_CURRENT_INDEX=ENGINEER_START_ADDR,
    ENGINEER_END
} FACTORY_ENGINEER_INDEX;



typedef struct
{
    SENSOR_REG_STRUCT   Reg[ENGINEER_END];
    SENSOR_REG_STRUCT   CCT[FACTORY_END_ADDR];
} SENSOR_DATA_STRUCT, *PSENSOR_DATA_STRUCT;


typedef enum {
    SENSOR_MODE_INIT = 0,
    SENSOR_MODE_PREVIEW,
    SENSOR_MODE_CAPTURE
} AR0833_SENSOR_MODE;


typedef struct
{
	kal_uint32 DummyPixels;
	kal_uint32 DummyLines;

	kal_uint32 pvShutter;
	kal_uint32 pvGain;

	kal_uint32 pvPclk;  // x10 480 for 48MHZ
	kal_uint32 capPclk; // x10

	kal_uint32 shutter;
	kal_uint32 maxExposureLines;

	kal_uint16 sensorGlobalGain;//sensor gain read from 0x350a 0x350b;
	kal_uint16 ispBaseGain;//64
	kal_uint16 realGain;//ispBaseGain as 1x

	kal_int16 imgMirror;

	AR0833_SENSOR_MODE sensorMode;

	kal_bool AR0833AutoFlickerMode;
	kal_bool AR0833VideoMode;

}AR0833_PARA_STRUCT,*PAR0833_PARA_STRUCT;



#define CURRENT_MAIN_SENSOR                AR0833_MICRON
//if define RAW10, MIPI_INTERFACE must be defined
//if MIPI_INTERFACE is marked, RAW10 must be marked too
#define MIPI_INTERFACE
#define RAW10


#define AR0833_IMAGE_SENSOR_FULL_HACTIVE    3264
#define AR0833_IMAGE_SENSOR_FULL_VACTIVE    2448


//#define AR0833_PV_PERIOD_PIXEL_NUMS                 0x0DBC  //3516
//#define AR0833_PV_PERIOD_LINE_NUMS                  0x51E   //1310


#if 0  ///kk test default:0
#define AR0833_IMAGE_SENSOR_PV_HACTIVE      3264
#define AR0833_IMAGE_SENSOR_PV_VACTIVE      2448
#else
#define AR0833_IMAGE_SENSOR_PV_HACTIVE      1616
#define AR0833_IMAGE_SENSOR_PV_VACTIVE      1212
#endif

#define AR0833_FULL_START_X                 2
#define AR0833_FULL_START_Y                 2
#define AR0833_IMAGE_SENSOR_FULL_WIDTH      (AR0833_IMAGE_SENSOR_FULL_HACTIVE - 32)  //2592-32 2560
#define AR0833_IMAGE_SENSOR_FULL_HEIGHT     (AR0833_IMAGE_SENSOR_FULL_VACTIVE - 24)  //1944 -24 1920

#define AR0833_PV_START_X                   0
#define AR0833_PV_START_Y                   0
#define AR0833_IMAGE_SENSOR_PV_WIDTH        (AR0833_IMAGE_SENSOR_PV_HACTIVE - 16)
#define AR0833_IMAGE_SENSOR_PV_HEIGHT       (AR0833_IMAGE_SENSOR_PV_VACTIVE - 12)

#ifdef MIPI_INTERFACE
    #define	AR0833_IMAGE_SENSOR_FULL_HBLANKING  1200
#else
    #define AR0833_IMAGE_SENSOR_FULL_HBLANKING 	200
#endif
#define AR0833_IMAGE_SENSOR_FULL_VBLANKING      150

#ifdef MIPI_INTERFACE
    #define	AR0833_IMAGE_SENSOR_PV_HBLANKING    1855
#else
    #define	AR0833_IMAGE_SENSOR_PV_HBLANKING    279
#endif
#define AR0833_IMAGE_SENSOR_PV_VBLANKING        128

#define AR0833_FULL_PERIOD_PIXEL_NUMS	    (AR0833_IMAGE_SENSOR_FULL_HACTIVE + AR0833_IMAGE_SENSOR_FULL_HBLANKING)  //2592+1200= 3792
#define AR0833_FULL_PERIOD_LINE_NUMS	    (AR0833_IMAGE_SENSOR_FULL_VACTIVE + AR0833_IMAGE_SENSOR_FULL_VBLANKING)  //1944+150 = 2094
#define AR0833_PV_PERIOD_PIXEL_NUMS	        (AR0833_IMAGE_SENSOR_PV_HACTIVE + AR0833_IMAGE_SENSOR_PV_HBLANKING)     //1296 +1855 =3151
#define AR0833_PV_PERIOD_LINE_NUMS	        (AR0833_IMAGE_SENSOR_PV_VACTIVE + AR0833_IMAGE_SENSOR_PV_VBLANKING)    //972 + 128 =1100



#define AR0833_FRAME_RATE_UNIT          10
#define AR0833_set_frame_rate(a)        (a * AR0833_FRAME_RATE_UNIT)
#define AR0833_1X_ZOOM_IN_CAPTURE_FRAME	9



/* SENSOR PRIVATE STRUCT */
struct AR0833_SENSOR_STRUCT
{
    kal_uint8 i2c_write_id;
    kal_uint8 i2c_read_id;
    kal_uint16 preview_vt_clk;
    kal_uint16 capture_vt_clk;
};


/* FRAME RATE */
#define AR0833_FPS(x)                          ((kal_uint32)(10 * (x)))

#endif /* _AR0833_MIPI_RAW_SENSOR_H_ */

