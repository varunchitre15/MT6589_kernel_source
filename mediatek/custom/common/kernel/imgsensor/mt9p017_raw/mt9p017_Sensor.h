/*****************************************************************************
 *
 * Filename:
 * ---------
 *   mt9p017_Sensor.h
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
#ifndef _MT9P017_SENSOR_H
#define _MT9P017_SENSOR_H

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



#define CURRENT_MAIN_SENSOR                MT9P017_MICRON



#define MT9P017_WRITE_ID_1	0x20
#define MT9P017_READ_ID_1	0x21
#define MT9P017_WRITE_ID_2	0x30
#define MT9P017_READ_ID_2	0x31



#define MT9P017_IMAGE_SENSOR_FULL_HACTIVE		2592
#define MT9P017_IMAGE_SENSOR_FULL_VACTIVE		1944
#define MT9P017_IMAGE_SENSOR_PV_HACTIVE			1296
#define MT9P017_IMAGE_SENSOR_PV_VACTIVE			972

#define MT9P017_FULL_START_X					2
#define MT9P017_FULL_START_Y					2
#define MT9P017_IMAGE_SENSOR_FULL_WIDTH			(MT9P017_IMAGE_SENSOR_FULL_HACTIVE - 32)  //2592-32 2560
#define MT9P017_IMAGE_SENSOR_FULL_HEIGHT		(MT9P017_IMAGE_SENSOR_FULL_VACTIVE - 24)  //1944 -24 1920

#define MT9P017_PV_START_X						2
#define MT9P017_PV_START_Y						2
#define MT9P017_IMAGE_SENSOR_PV_WIDTH			(MT9P017_IMAGE_SENSOR_PV_HACTIVE - 16)	//1296 -16 1280
#define MT9P017_IMAGE_SENSOR_PV_HEIGHT			(MT9P017_IMAGE_SENSOR_PV_VACTIVE - 12)	//972 -12 960

#define	MT9P017_IMAGE_SENSOR_FULL_HBLANKING		200
#define MT9P017_IMAGE_SENSOR_FULL_VBLANKING		150
#define	MT9P017_IMAGE_SENSOR_PV_HBLANKING		279
#define MT9P017_IMAGE_SENSOR_PV_VBLANKING		128

#define MT9P017_FULL_PERIOD_PIXEL_NUMS			(MT9P017_IMAGE_SENSOR_FULL_HACTIVE + MT9P017_IMAGE_SENSOR_FULL_HBLANKING)  //2592+200= 2792
#define MT9P017_FULL_PERIOD_LINE_NUMS			(MT9P017_IMAGE_SENSOR_FULL_VACTIVE + MT9P017_IMAGE_SENSOR_FULL_VBLANKING)  //1944+150 = 2094
#define MT9P017_PV_PERIOD_PIXEL_NUMS			(MT9P017_IMAGE_SENSOR_PV_HACTIVE + MT9P017_IMAGE_SENSOR_PV_HBLANKING)	 //1296 +279 =1575
#define MT9P017_PV_PERIOD_LINE_NUMS				(MT9P017_IMAGE_SENSOR_PV_VACTIVE + MT9P017_IMAGE_SENSOR_PV_VBLANKING)    //972 + 128 =1100



#define MT9P017_FRAME_RATE_UNIT		10
#define MT9P017_set_frame_rate(a) 	(a * MT9P017_FRAME_RATE_UNIT)
#define MT9P017_1X_ZOOM_IN_CAPTURE_FRAME	9



/* SENSOR PRIVATE STRUCT */
struct MT9P017_SENSOR_STRUCT
{
	kal_uint8 i2c_write_id;
	kal_uint8 i2c_read_id;
	kal_uint16 preview_vt_clk;
	kal_uint16 capture_vt_clk;
};


/* FRAME RATE */
#define MT9P017_FPS(x)                          ((kal_uint32)(10 * (x)))

#endif /* _MT9P017_SENSOR_H */

