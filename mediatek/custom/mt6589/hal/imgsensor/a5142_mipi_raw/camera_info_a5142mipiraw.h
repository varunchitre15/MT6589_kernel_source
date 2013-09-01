

#ifndef _CAMERA_INFO_A5142MIPIRAW_H
#define _CAMERA_INFO_A5142MIPIRAW_H

/*******************************************************************************
*   Configuration
********************************************************************************/
#define SENSOR_ID                           A5142MIPI_SENSOR_ID
#define SENSOR_DRVNAME                      SENSOR_DRVNAME_A5142_MIPI_RAW
#define INCLUDE_FILENAME_ISP_REGS_PARAM     "camera_isp_regs_a5142mipiraw.h"
#define INCLUDE_FILENAME_ISP_PCA_PARAM      "camera_isp_pca_a5142mipiraw.h"
#define INCLUDE_FILENAME_ISP_LSC_PARAM      "camera_isp_lsc_a5142mipiraw.h"
/*******************************************************************************
*
********************************************************************************/

#if defined(ISP_SUPPORT)

#define A5142MIPIRAW_CAMERA_AUTO_DSC CAM_AUTO_DSC
#define A5142MIPIRAW_CAMERA_PORTRAIT CAM_PORTRAIT
#define A5142MIPIRAW_CAMERA_LANDSCAPE CAM_LANDSCAPE
#define A5142MIPIRAW_CAMERA_SPORT CAM_SPORT
#define A5142MIPIRAW_CAMERA_FLOWER CAM_FLOWER
#define A5142MIPIRAW_CAMERA_NIGHTSCENE CAM_NIGHTSCENE
#define A5142MIPIRAW_CAMERA_DOCUMENT CAM_DOCUMENT
#define A5142MIPIRAW_CAMERA_ISO_ANTI_HAND_SHAKE CAM_ISO_ANTI_HAND_SHAKE
#define A5142MIPIRAW_CAMERA_ISO100 CAM_ISO100
#define A5142MIPIRAW_CAMERA_ISO200 CAM_ISO200
#define A5142MIPIRAW_CAMERA_ISO400 CAM_ISO400
#define A5142MIPIRAW_CAMERA_ISO800 CAM_ISO800
#define A5142MIPIRAW_CAMERA_ISO1600 CAM_ISO1600
#define A5142MIPIRAW_CAMERA_VIDEO_AUTO CAM_VIDEO_AUTO
#define A5142MIPIRAW_CAMERA_VIDEO_NIGHT CAM_VIDEO_NIGHT
#define A5142MIPIRAW_CAMERA_NO_OF_SCENE_MODE CAM_NO_OF_SCENE_MODE

#endif
#endif
