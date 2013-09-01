/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Header file of Sensor driver
 *
 *
 * Author:
 * -------
 *   PC Huang (MTK02204)
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
/* SENSOR FULL SIZE */
#ifndef __SENSOR_H
#define __SENSOR_H

typedef enum S5K4ECGX_CAMCO_MODE
{
  S5K4ECGX_CAM_PREVIEW=0,//Camera Preview

  S5K4ECGX_CAM_CAPTURE,//Camera Capture

  S5K4ECGX_VIDEO_MPEG4,//Video Mode
  S5K4ECGX_VIDEO_MJPEG,

  S5K4ECGX_WEBCAM_CAPTURE,//WebCam

  S5K4ECGX_VIDEO_MAX
} S5K4ECGX_Camco_MODE;


struct S5K4ECGX_sensor_struct
{
        kal_uint16 sensor_id;

        kal_uint16 Dummy_Pixels;
        kal_uint16 Dummy_Lines;
        kal_uint32 Preview_PClk;

        kal_uint32 Preview_Lines_In_Frame;
        kal_uint32 Capture_Lines_In_Frame;

        kal_uint32 Preview_Pixels_In_Line;
        kal_uint32 Capture_Pixels_In_Line;
        kal_uint16 Preview_Shutter;
        kal_uint16 Capture_Shutter;

        kal_uint16 StartX;
        kal_uint16 StartY;
        kal_uint16 iGrabWidth;
        kal_uint16 iGrabheight;

        kal_uint16 Capture_Size_Width;
        kal_uint16 Capture_Size_Height;
        kal_uint32 Digital_Zoom_Factor;

        kal_uint16 Max_Zoom_Factor;

        kal_uint32 Min_Frame_Rate;
        kal_uint32 Max_Frame_Rate;
        kal_uint32 Fixed_Frame_Rate;
        //kal_bool Night_Mode;
        S5K4ECGX_Camco_MODE Camco_mode;
        AE_FLICKER_MODE_T Banding;

        kal_bool Night_Mode;
};


//Daniel


#define S5K4ECGX_WRITE_ID                     0x78//0x7A//0x78//0x5A//0xAC (0x78)
#define S5K4ECGX_READ_ID                      0x79//0x7B//0x79//0x5B//0xAD

#define S5K4ECGX2_WRITE_ID                      0x78
#define S5K4ECGX2_READ_ID                       0x79

#define S5K4ECGX_SENSOR_ID 					0x4EC0

/* SENSOR FULL/PV SIZE */
#define S5K4ECGX_IMAGE_SENSOR_FULL_WIDTH_DRV   2560//1280
#define S5K4ECGX_IMAGE_SENSOR_FULL_HEIGHT_DRV  1920//960
#define S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV     1280//1280 //1280//1280 //1280
#define S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT_DRV    960//960 //960//960  //960 
#define S5K4ECGX_IMAGE_SENSOR_PV_WIDTH         (S5K4ECGX_IMAGE_SENSOR_PV_WIDTH_DRV)
#define S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT        (S5K4ECGX_IMAGE_SENSOR_PV_HEIGHT_DRV) /* -2 for frame ready done */

/* SENSOR START/END POSITION */
#define S5K4ECGX_PV_X_START                     0    // 2
#define S5K4ECGX_PV_Y_START                     1    // 2
#define S5K4ECGX_FULL_X_START                  0
#define S5K4ECGX_FULL_Y_START                  1

/* Flicker factor to calculate tha minimal shutter width step for 50Hz and 60Hz  */
#define MACRO_50HZ                                                      (100)
#define MACRO_60HZ                                                      (120)

#define FACTOR_50HZ                                                     (MACRO_50HZ * 1000)
#define FACTOR_60HZ                                                     (MACRO_60HZ * 1000)



#endif /* __SENSOR_H */
