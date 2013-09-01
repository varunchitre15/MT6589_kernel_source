

#ifndef _CAMERA_INFO_AR0833MIPIRAW_H
#define _CAMERA_INFO_AR0833MIPIRAW_H

/*******************************************************************************
*   Configuration
********************************************************************************/
#define SENSOR_ID                           AR0833_SENSOR_ID
#define SENSOR_DRVNAME                      SENSOR_DRVNAME_AR0833_MIPI_RAW
#define INCLUDE_FILENAME_ISP_REGS_PARAM     "camera_isp_regs_ar0833mipiraw.h"
#define INCLUDE_FILENAME_ISP_PCA_PARAM      "camera_isp_pca_ar0833mipiraw.h"
#define INCLUDE_FILENAME_ISP_LSC_PARAM      "camera_isp_lsc_ar0833mipiraw.h"
/*******************************************************************************
*
********************************************************************************/

#if defined(ISP_SUPPORT)

#define AR0833MIPIRAW_CAMERA_AUTO_DSC CAM_AUTO_DSC
#define AR0833MIPIRAW_CAMERA_PORTRAIT CAM_PORTRAIT
#define AR0833MIPIRAW_CAMERA_LANDSCAPE CAM_LANDSCAPE
#define AR0833MIPIRAW_CAMERA_SPORT CAM_SPORT
#define AR0833MIPIRAW_CAMERA_FLOWER CAM_FLOWER
#define AR0833MIPIRAW_CAMERA_NIGHTSCENE CAM_NIGHTSCENE
#define AR0833MIPIRAW_CAMERA_DOCUMENT CAM_DOCUMENT
#define AR0833MIPIRAW_CAMERA_ISO_ANTI_HAND_SHAKE CAM_ISO_ANTI_HAND_SHAKE
#define AR0833MIPIRAW_CAMERA_ISO100 CAM_ISO100
#define AR0833MIPIRAW_CAMERA_ISO200 CAM_ISO200
#define AR0833MIPIRAW_CAMERA_ISO400 CAM_ISO400
#define AR0833MIPIRAW_CAMERA_ISO800 CAM_ISO800
#define AR0833MIPIRAW_CAMERA_ISO1600 CAM_ISO1600
#define AR0833MIPIRAW_CAMERA_VIDEO_AUTO CAM_VIDEO_AUTO
#define AR0833MIPIRAW_CAMERA_VIDEO_NIGHT CAM_VIDEO_NIGHT
#define AR0833MIPIRAW_CAMERA_NO_OF_SCENE_MODE CAM_NO_OF_SCENE_MODE

#endif
#endif
