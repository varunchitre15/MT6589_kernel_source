/*****************************************************************************
 *
 * Filename:
 * ---------
 *   a5141mipi_Sensor.h
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
 *   Guangye Yang (mtk70662)
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
#ifndef _A5141MIPI_SENSOR_H
#define _A5141MIPI_SENSOR_H

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
    SENSOR_REG_STRUCT	Reg[ENGINEER_END];
    SENSOR_REG_STRUCT	CCT[FACTORY_END_ADDR];
} SENSOR_DATA_STRUCT, *PSENSOR_DATA_STRUCT;



#define CURRENT_MAIN_SENSOR                A5141MIPI_MICRON
//if define RAW10, MIPI_INTERFACE must be defined
//if MIPI_INTERFACE is marked, RAW10 must be marked too
#define MIPI_INTERFACE
#define RAW10


#define A5141MIPI_WRITE_ID_1	0x20
#define A5141MIPI_READ_ID_1	0x21
#define A5141MIPI_WRITE_ID_2	0x30
#define A5141MIPI_READ_ID_2	0x31



#define A5141MIPI_IMAGE_SENSOR_FULL_HACTIVE		2592
#define A5141MIPI_IMAGE_SENSOR_FULL_VACTIVE		1944
#define A5141MIPI_IMAGE_SENSOR_PV_HACTIVE			1296
#define A5141MIPI_IMAGE_SENSOR_PV_VACTIVE			972



#define A5141MIPI_FULL_START_X					6
#define A5141MIPI_FULL_START_Y					6
#define A5141MIPI_IMAGE_SENSOR_FULL_WIDTH			(A5141MIPI_IMAGE_SENSOR_FULL_HACTIVE - 32)  //2592-32 2560
#define A5141MIPI_IMAGE_SENSOR_FULL_HEIGHT		    (A5141MIPI_IMAGE_SENSOR_FULL_VACTIVE - 24)  //1944 -24 1920

#define A5141MIPI_PV_START_X						2
#define A5141MIPI_PV_START_Y						2
#define A5141MIPI_IMAGE_SENSOR_PV_WIDTH			(A5141MIPI_IMAGE_SENSOR_PV_HACTIVE - 16)	//1296 -16 1280
#define A5141MIPI_IMAGE_SENSOR_PV_HEIGHT			(A5141MIPI_IMAGE_SENSOR_PV_VACTIVE - 12)	//972 -12 960

#ifdef MIPI_INTERFACE
	#define	A5141MIPI_IMAGE_SENSOR_FULL_HBLANKING		1102
#else
	#define A5141MIPI_IMAGE_SENSOR_FULL_HBLANKING 	200
#endif
#define A5141MIPI_IMAGE_SENSOR_FULL_VBLANKING		77


#ifdef MIPI_INTERFACE
	#define	A5141MIPI_IMAGE_SENSOR_PV_HBLANKING		1855
#else
	#define	A5141MIPI_IMAGE_SENSOR_PV_HBLANKING		279
#endif
#define A5141MIPI_IMAGE_SENSOR_PV_VBLANKING		128


#define A5141MIPI_FULL_PERIOD_PIXEL_NUMS			(A5141MIPI_IMAGE_SENSOR_FULL_HACTIVE + A5141MIPI_IMAGE_SENSOR_FULL_HBLANKING)  //2592+1102= 3694
#define A5141MIPI_FULL_PERIOD_LINE_NUMS			(A5141MIPI_IMAGE_SENSOR_FULL_VACTIVE + A5141MIPI_IMAGE_SENSOR_FULL_VBLANKING)  //1944+77 = 2021
#define A5141MIPI_PV_PERIOD_PIXEL_NUMS			    (A5141MIPI_IMAGE_SENSOR_PV_HACTIVE + A5141MIPI_IMAGE_SENSOR_PV_HBLANKING)	   //1296 +1855 =3151
#define A5141MIPI_PV_PERIOD_LINE_NUMS				(A5141MIPI_IMAGE_SENSOR_PV_VACTIVE + A5141MIPI_IMAGE_SENSOR_PV_VBLANKING)    //972 + 128 =1100



#define A5141MIPI_FRAME_RATE_UNIT		10
#define A5141MIPI_set_frame_rate(a) 	(a * A5141MIPI_FRAME_RATE_UNIT)
#define A5141MIPI_1X_ZOOM_IN_CAPTURE_FRAME	9



/* SENSOR PRIVATE STRUCT */
struct A5141MIPI_SENSOR_STRUCT
{
	kal_uint8 i2c_write_id;
	kal_uint8 i2c_read_id;
	kal_uint16 preview_vt_clk;
	kal_uint16 capture_vt_clk;
};


/* FRAME RATE */
#define A5141MIPI_FPS(x)                          ((kal_uint32)(10 * (x)))

#endif /* _A5141MIPI_SENSOR_H */

