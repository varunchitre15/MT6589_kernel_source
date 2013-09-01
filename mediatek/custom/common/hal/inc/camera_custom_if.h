#ifndef _CAMERA_CUSTOM_IF_
#define _CAMERA_CUSTOM_IF_
//
#include "camera_custom_types.h"
#include "camera_custom_atv_para.h"

//
namespace NSCamCustom
{
//
//
enum EDevId
{
    eDevId_ImgSensor0, //main sensor
    eDevId_ImgSensor1, //sub sensor
    eDevId_ImgSensor2, //main2 sensor (for 3D)
};

/*******************************************************************************
* Sensor Input Data Bit Order
*   Return:
*       0   : raw data input [9:2]
*       1   : raw data input [7:0]
*       -1  : error
*******************************************************************************/
MINT32  getSensorInputDataBitOrder(EDevId const eDevId);

/*******************************************************************************
* Sensor Pixel Clock Inverse in PAD side.
*   Return:
*       0   : no inverse
*       1   : inverse
*       -1  : error
*******************************************************************************/
MINT32  getSensorPadPclkInv(EDevId const eDevId);

/*******************************************************************************
* Sensor Placement Facing Direction
*   Return:
*       0   : Back side  
*       1   : Front side (LCD side)
*       -1  : error
*******************************************************************************/
MINT32  getSensorFacingDirection(EDevId const eDevId);

/*******************************************************************************
* Image Sensor Orientation
*******************************************************************************/
typedef struct SensorOrientation_S
{
    MUINT32 u4Degree_0;     //  main sensor in degree (0, 90, 180, 270)
    MUINT32 u4Degree_1;     //  sub  sensor in degree (0, 90, 180, 270)
    MUINT32 u4Degree_2;     //  main2 sensor in degree (0, 90, 180, 270)
} SensorOrientation_T;

SensorOrientation_T const&  getSensorOrientation();

/*******************************************************************************
* Return fake orientation for front sensor in degree 0/180 or not
*******************************************************************************/
MBOOL isRetFakeSubOrientation();

/*******************************************************************************
* Auto flicker detection
*******************************************************************************/
typedef struct FlickerThresholdSetting_S
{
    MUINT32 u4FlickerPoss1;         // impossible flicker
    MUINT32 u4FlickerPoss2;         // maybe flicker exist
    MUINT32 u4FlickerFreq1;         // flicker frequency detect 
    MUINT32 u4FlickerFreq2;         // flicker frequency detect
    MUINT32 u4ConfidenceLevel1;   // flicker confidence level
    MUINT32 u4ConfidenceLevel2;   // flicker confidence level
    MUINT32 u4ConfidenceLevel3;   // flicker confidence level
}FlickerThresholdSetting_T;

FlickerThresholdSetting_T const&  getFlickerThresPara();

/*******************************************************************************
* MDP
*******************************************************************************/
typedef struct TuningParam_CRZ_S
{
    MUINT8  uUpScaleCoeff;  //  [5 bits; 1~19] Up sample coeff. choose > 12 may get undesirable result, '8' is recommended.
    MUINT8  uDnScaleCoeff;  //  [5 bits; 1~19] Down sample coeff. '15' is recommended.
} TuningParam_CRZ_T;

typedef struct TuningParam_PRZ_S
{
    MUINT8  uUpScaleCoeff;  //  [5 bits; 1~19] Up sample coeff. choose > 12 may get undesirable result, '8' is recommended.
    MUINT8  uDnScaleCoeff;  //  [5 bits; 1~19] Down sample coeff. '15' is recommended.
    MUINT8  uEEHCoeff;      //  [4 bits] The strength for horizontal edge.
    MUINT8  uEEVCoeff;      //  [4 bits] The strength for vertial edge.
} TuningParam_PRZ_T;

TuningParam_CRZ_T const&  getParam_CRZ_Video();
TuningParam_CRZ_T const&  getParam_CRZ_Preview();
TuningParam_CRZ_T const&  getParam_CRZ_Capture();
TuningParam_PRZ_T const&  getParam_PRZ_QuickView();

//
/*******************************************************************************
* Dynamic Frame Rate for Video
******************************************************************************/
typedef struct VdoDynamicFrameRate_S
{
    MUINT32 EVThresNormal;
    MUINT32 EVThresNight;
    MBOOL   isEnableDFps;
} VdoDynamicFrameRate_T;

VdoDynamicFrameRate_T const& getParamVdoDynamicFrameRate();


/*******************************************************************************
* Custom EXIF (Imgsensor-related)
*******************************************************************************/
typedef struct SensorExifInfo_S
{
    MUINT32 uFLengthNum;
    MUINT32 uFLengthDenom;
    
} SensorExifInfo_T;

SensorExifInfo_T const& getParamSensorExif();

//
/*******************************************************************************
* Custom EXIF
******************************************************************************/
#define SET_EXIF_TAG_STRING(tag,str) \
    if (strlen((const char*)str) <= 32) { \
        strcpy((char *)pexifApp1Info->tag, (const char*)str); }
        
typedef struct customExifInfo_s {
    unsigned char strMake[32];
    unsigned char strModel[32];
    unsigned char strSoftware[32];
} customExifInfo_t;

MINT32 custom_SetExif(void **ppCustomExifTag);
/*******************************************************************************
* Custom EXIF
******************************************************************************/
typedef struct customExif_s
{
    MBOOL   bEnCustom;
    MUINT32 u4ExpProgram;
    
} customExif_t;

customExif_t const& getCustomExif();

MUINT32 custom_GetFlashlightGain10X(void);  //cotta : added for high current solution
MUINT32 custom_BurstFlashlightGain10X(void);
double custom_GetYuvFlashlightThreshold(void);
MINT32 custom_GetYuvFlashlightFrameCnt(void);
MINT32 custom_GetYuvFlashlightDuty(void);
MINT32 custom_GetYuvFlashlightStep(void);
MINT32 custom_GetYuvFlashlightHighCurrentDuty(void);
MINT32 custom_GetYuvFlashlightHighCurrentTimeout(void);


/*******************************************************************************
* Get the LCM Physical Orientation, the LCM physical orientation 
* will be defined in ProjectConfig.mk 
*******************************************************************************/
MUINT32 getLCMPhysicalOrientation();
/*******************************************************************************
* ATV
*******************************************************************************/
MINT32 get_atv_input_data();

/*******************************************************************************
* FD Threshold
*******************************************************************************/
MINT8 get_fdvt_threshold();

/*******************************************************************************
* SD Threshold:  Default: 5 
*******************************************************************************/
MINT8 get_SD_threshold();

/*******************************************************************************
*  Get Face beautify blur level Default: 4   1~4
*******************************************************************************/
MINT8 get_FB_BlurLevel();

/*******************************************************************************
*  Get Face beautify NR cycle number Default: 4   2~6
*******************************************************************************/
MINT8 get_FB_NRTime();

/*******************************************************************************
*  Get Face beautify Color Target mode Default: 2   2:white  0:red
*******************************************************************************/
MINT8 get_FB_ColorTarget();

MINT32 get_atv_disp_delay(MINT32 mode);

/*******************************************************************************
* ASD Threshold
*******************************************************************************/

typedef struct ASDThreshold_S
{
	MINT16 s2IdxWeightBlAe;
     MINT16 s2IdxWeightBlScd;    
	MINT16 s2IdxWeightLsAe;        
  	MINT16 s2IdxWeightLsAwb;
  	MINT16 s2IdxWeightLsAf;    
     MINT16 s2IdxWeightLsScd;
     MUINT8 u1TimeWeightType;
     MUINT8 u1TimeWeightRange;
     MINT16 s2EvLoThrNight;
     MINT16 s2EvHiThrNight;
     MINT16 s2EvLoThrOutdoor;
     MINT16 s2EvHiThrOutdoor;
     MUINT8 u1ScoreThrNight;
     MUINT8 u1ScoreThrBacklit;
     MUINT8 u1ScoreThrPortrait;
     MUINT8 u1ScoreThrLandscape;
     MBOOL boolBacklitLockEnable;
     MINT16 s2BacklitLockEvDiff;  
}ASDThreshold_T;

ASDThreshold_T const&  get_ASD_threshold();

/*******************************************************************************
* PCA LUT for face beautifier
*******************************************************************************/
enum { PCA_BIN_NUM = 180 };
typedef struct {
    MUINT8  y_gain;
    MUINT8  sat_gain;
    MUINT8  hue_shift;
    MUINT8  reserved;
} FB_PCA_BIN_T;
//
//
typedef struct {
    FB_PCA_BIN_T lut[PCA_BIN_NUM];
} FB_PCA_LUT_T;

FB_PCA_LUT_T&  getFBPCALut();

/*******************************************************************************
* Refine capture ISP RAW gain
*******************************************************************************/
MVOID  refineCaptureISPRAWGain(MUINT32 u4SensorGain, MUINT32& u4RAWGain_R, MUINT32& u4RAWGain_Gr, MUINT32& u4RAWGain_Gb, MUINT32& u4RAWGain_B);

};  //NSCamCustom
#endif  //  _CAMERA_CUSTOM_IF_

