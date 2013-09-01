#ifndef CUSTOM_VT_VIDEO_ENC_TYPE_H
#define CUSTOM_VT_VIDEO_ENC_TYPE_H

#ifdef __cplusplus
extern "C"
    {
#endif


/*****************************************************************************
*
*****************************************************************************/
typedef enum
{
	CUSTOM_VT_SENSOR_AUTO_MODE = 0,
	CUSTOM_VT_SENSOR_NIGHT_MODE,
	CUSTOM_VT_SENSOR_UNKNOWN_MODE = 0x1FFFFFFF
}CUSTOM_VT_SENSOR_MODE_T;



/*****************************************************************************
*
*****************************************************************************/
typedef enum
{ 
	CUSTOM_VENC_RESOLUTION_176x144 = 0,             // QCIF
    CUSTOM_VENC_RESOLUTION_128x96,          // SQCIF
    CUSTOM_VENC_RESOLUTION_UNKNOWN = 0x1FFFFFFF // Unknown
} CUSTOM_VT_VENC_RESOLUTION_T;


/*****************************************************************************
*
*****************************************************************************/
typedef enum
{
    CUSTOM_VENC_QUALITY_GOOD = 0,
	CUSTOM_VENC_QUALITY_NORMAL,
    CUSTOM_VENC_QUALITY_LOW,   //fluency

    CUSTOM_VENC_QUALITY_FINE,
    CUSTOM_VENC_QUALITY_ALL,
    CUSTOM_VENC_QUALITY_UNKNOWN = 0x1FFFFFFF        // Keep it at end.
} CUSTOM_VT_VENC_QUALITY_T;



/*****************************************************************************
*
*****************************************************************************/
typedef enum
{
   	CUSTOM_VENC_CODEC_H263 = 0,
   	CUSTOM_VENC_CODEC_MPEG4,
    CUSTOM_VENC_CODEC_H264,
     CUSTOM_VENC_CODEC_ALL,
    CUSTOM_VENC_CODEC_UNKNOWN = 0x1FFFFFFF          // Keep it at end.
} CUSTOM_VT_VENC_CODEC_T;


/******************************************************************************
* This data structure is used by MMI.
******************************************************************************/
	


typedef struct
{
    CUSTOM_VT_VENC_RESOLUTION_T eResolution;
	CUSTOM_VT_SENSOR_MODE_T eSensorMode;
    CUSTOM_VT_VENC_QUALITY_T eQuality;
    CUSTOM_VT_VENC_CODEC_T eCodec;
} CUSTOM_VT_VENC_SCENARIO_TABLE_T;


/*****************************************************************************
*
*****************************************************************************/
	
typedef struct
{
	int iFrameRate;
	int iInitQp;
	int iMinQp;
	int iMaxQp;
	int iIntraVopRate;
	int iAlgorithmSelect;
	int iBitRate;
	int iDynamicRangeReduction;
	int iIsCustomerSetTable;
	signed short  *pDynamicRangeTable;

}CUSTOM_VT_VENC_PARAM_TABLE_T;


/*****************************************************************************
*
*****************************************************************************/
typedef struct
{
    CUSTOM_VT_VENC_SCENARIO_TABLE_T tVT_Scenario_T;
	CUSTOM_VT_VENC_PARAM_TABLE_T tVT_VEnc_Param_T;
  
} CUSTOM_VT_VENC_QUALITY_CTRL_TABLE_T;


/*****************************************************************************
*
*****************************************************************************/
typedef struct
{	
	float iGoodQualityFrameRate; //default 7.5fps
	float iNormalQualityFrameRate; // default 10fps
	float iLowQualityFrameRate;  // default 12fps
}CUSTOM_VT_VENC_QUALITY_LEVEL;



/******************************************************************************
* The functions to get the customer setting 
******************************************************************************/

bool _Custom_VT_GetFrameRates(int iCodec,int iSensorMode,void* pFrameRates);

bool _Custom_VT_GetCodecParam(int iCodec,int iSensorMode,int iEncLevel,CUSTOM_VT_VENC_PARAM_TABLE_T* pCodecParam);

signed short* _Custom_VT_GetDynamicRangeTable();

#ifdef __cplusplus
    }
#endif

#endif  //CUSTOM_VT_VIDEO_ENC_TYPE_H

