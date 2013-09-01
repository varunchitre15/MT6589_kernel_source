

#ifndef _CAMERA_INFO_S5K3H7YRAW_H
#define _CAMERA_INFO_S5K3H7YRAW_H

/*******************************************************************************
*   Configuration
********************************************************************************/
#define SENSOR_ID                           S5K3H7Y_SENSOR_ID
#define SENSOR_DRVNAME                      SENSOR_DRVNAME_S5K3H7Y_MIPI_RAW
#define INCLUDE_FILENAME_ISP_REGS_PARAM     "camera_isp_regs_s5k3h7yraw.h"
#define INCLUDE_FILENAME_ISP_PCA_PARAM      "camera_isp_pca_s5k3h7yraw.h"
#define INCLUDE_FILENAME_ISP_LSC_PARAM      "camera_isp_lsc_s5k3h7yraw.h"
/*******************************************************************************
*
********************************************************************************/

#if defined(ISP_SUPPORT)

#define S5K3H7YRAW_CAMERA_AUTO_DSC CAM_AUTO_DSC
#define S5K3H7YRAW_CAMERA_PORTRAIT CAM_PORTRAIT
#define S5K3H7YRAW_CAMERA_LANDSCAPE CAM_LANDSCAPE
#define S5K3H7YRAW_CAMERA_SPORT CAM_SPORT
#define S5K3H7YRAW_CAMERA_FLOWER CAM_FLOWER
#define S5K3H7YRAW_CAMERA_NIGHTSCENE CAM_NIGHTSCENE
#define S5K3H7YRAW_CAMERA_DOCUMENT CAM_DOCUMENT
#define S5K3H7YRAW_CAMERA_ISO_ANTI_HAND_SHAKE CAM_ISO_ANTI_HAND_SHAKE
#define S5K3H7YRAW_CAMERA_ISO100 CAM_ISO100
#define S5K3H7YRAW_CAMERA_ISO200 CAM_ISO200
#define S5K3H7YRAW_CAMERA_ISO400 CAM_ISO400
#define S5K3H7YRAW_CAMERA_ISO800 CAM_ISO800
#define S5K3H7YRAW_CAMERA_ISO1600 CAM_ISO1600
#define S5K3H7YRAW_CAMERA_VIDEO_AUTO CAM_VIDEO_AUTO
#define S5K3H7YRAW_CAMERA_VIDEO_NIGHT CAM_VIDEO_NIGHT
#define S5K3H7YRAW_CAMERA_NO_OF_SCENE_MODE CAM_NO_OF_SCENE_MODE

#endif
#endif
