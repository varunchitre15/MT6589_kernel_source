
#ifndef _ISP_TUNING_H_
#define _ISP_TUNING_H_


namespace NSIspTuning
{


/*******************************************************************************
*
*******************************************************************************/
typedef enum MERROR_ENUM
{
    MERR_OK         = 0,
    MERR_UNKNOWN    = 0x80000000, // Unknown error
    MERR_UNSUPPORT,
    MERR_BAD_PARAM,
    MERR_BAD_CTRL_CODE,
    MERR_BAD_FORMAT,
    MERR_BAD_ISP_DRV,
    MERR_BAD_NVRAM_DRV,
    MERR_BAD_SENSOR_DRV,
    MERR_BAD_SYSRAM_DRV,
    MERR_SET_ISP_REG,
    MERR_NO_MEM,
    MERR_NO_SYSRAM_MEM,
    MERR_NO_RESOURCE,
    MERR_CUSTOM_DEFAULT_INDEX_NOT_FOUND,
    MERR_CUSTOM_NOT_READY,
    MERR_PREPARE_HW,
    MERR_APPLY_TO_HW
} MERROR_ENUM_T;


/*******************************************************************************
* Operation Mode
*******************************************************************************/
typedef enum
{
    EOperMode_Normal    = 0,
    EOperMode_PureRaw,
    EOperMode_Meta,
    EOperMode_EM
} EOperMode_T;

/*******************************************************************************
* Sensor Mode
*******************************************************************************/
typedef enum
{
    ESensorMode_Preview    = 0,
    ESensorMode_Video,
    ESensorMode_Capture
} ESensorMode_T;

/*******************************************************************************
*
*******************************************************************************/
typedef enum
{
    //  NORMAL
    EIspProfile_NormalPreview              = 0,
    EIspProfile_ZsdPreview_CC,
    EIspProfile_ZsdPreview_NCC,
    EIspProfile_NormalCapture,
    EIspProfile_VideoPreview,
    EIspProfile_VideoCapture,
    //  MF
    EIspProfile_MFCapPass1,
    EIspProfile_MFCapPass2,
    EIspProfile_NUM
} EIspProfile_T;


/*******************************************************************************
*
*******************************************************************************/
typedef enum
{
    ESensorDev_Main         = 0x01,
    ESensorDev_Sub          = 0x02,
	ESensorDev_Atv          = 0x04,
	ESensorDev_MainSecond   = 0x08,
    ESensorDev_Main3D       = 0x09,    
}   ESensorDev_T;

};  //  NSIspTuning

#endif //  _ISP_TUNING_H_

