

#ifndef _CAMERA_INFO_MT9D115MIPIRAW_H
#define _CAMERA_INFO_MT9D115MIPIRAW_H

/*******************************************************************************
*   Configuration
********************************************************************************/
#define SENSOR_ID                           MT9D115_SENSOR_ID
#define SENSOR_DRVNAME                      SENSOR_DRVNAME_MT9D115_MIPI_RAW
#define INCLUDE_FILENAME_ISP_REGS_PARAM     "camera_isp_regs_mt9d115mipiraw.h"
#define INCLUDE_FILENAME_ISP_PCA_PARAM      "camera_isp_pca_mt9d115mipiraw.h"
#define INCLUDE_FILENAME_ISP_LSC_PARAM      "camera_isp_lsc_mt9d115mipiraw.h"
/*******************************************************************************
*
********************************************************************************/

#if defined(ISP_SUPPORT)

#define MT9D115MIPIRAW_CAMERA_AUTO_DSC CAM_AUTO_DSC
#define MT9D115MIPIRAW_CAMERA_PORTRAIT CAM_PORTRAIT
#define MT9D115MIPIRAW_CAMERA_LANDSCAPE CAM_LANDSCAPE
#define MT9D115MIPIRAW_CAMERA_SPORT CAM_SPORT
#define MT9D115MIPIRAW_CAMERA_FLOWER CAM_FLOWER
#define MT9D115MIPIRAW_CAMERA_NIGHTSCENE CAM_NIGHTSCENE
#define MT9D115MIPIRAW_CAMERA_DOCUMENT CAM_DOCUMENT
#define MT9D115MIPIRAW_CAMERA_ISO_ANTI_HAND_SHAKE CAM_ISO_ANTI_HAND_SHAKE
#define MT9D115MIPIRAW_CAMERA_ISO100 CAM_ISO100
#define MT9D115MIPIRAW_CAMERA_ISO200 CAM_ISO200
#define MT9D115MIPIRAW_CAMERA_ISO400 CAM_ISO400
#define MT9D115MIPIRAW_CAMERA_ISO800 CAM_ISO800
#define MT9D115MIPIRAW_CAMERA_ISO1600 CAM_ISO1600
#define MT9D115MIPIRAW_CAMERA_VIDEO_AUTO CAM_VIDEO_AUTO
#define MT9D115MIPIRAW_CAMERA_VIDEO_NIGHT CAM_VIDEO_NIGHT
#define MT9D115MIPIRAW_CAMERA_NO_OF_SCENE_MODE CAM_NO_OF_SCENE_MODE

#endif
#endif
