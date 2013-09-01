#include "camera_custom_hdr.h"
#include <math.h>
#include <cstdio>
#include <cstdlib>
#include <cutils/properties.h>
#include <cutils/xlog.h>	// For XLOG?().
#include <utils/Errors.h>



/**************************************************************************
 *                      D E F I N E S / M A C R O S                       *
 **************************************************************************/
#define MAX_HDR_GAIN_ARRAY_ELEM		11	// Maximun HDR GainArray element number.

/**************************************************************************
 *     E N U M / S T R U C T / T Y P E D E F    D E C L A R A T I O N     *
 **************************************************************************/

/**************************************************************************
 *                 E X T E R N A L    R E F E R E N C E S                 *
 **************************************************************************/

/**************************************************************************
 *                         G L O B A L    D A T A                         *
 **************************************************************************/
static MUINT32 au4HdrGainArray[MAX_HDR_GAIN_ARRAY_ELEM] =
{
	CUST_HDR_GAIN_00,
	CUST_HDR_GAIN_01,
	CUST_HDR_GAIN_02,
	CUST_HDR_GAIN_03,
	CUST_HDR_GAIN_04,
	CUST_HDR_GAIN_05,
	CUST_HDR_GAIN_06,
	CUST_HDR_GAIN_07,
	CUST_HDR_GAIN_08,
	CUST_HDR_GAIN_09,
	CUST_HDR_GAIN_10,
};

/**************************************************************************
 *       P R I V A T E    F U N C T I O N    D E C L A R A T I O N        *
 **************************************************************************/


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for Core Number.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomHdrCoreNumberGet(void)
{
	return CUST_HDR_CORE_NUMBER;
}

///////////////////////////////////////////////////////////////////////////
/// @brief Get prolonged VD number.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomHdrProlongedVdGet(void)
{
	return CUST_HDR_PROLONGED_VD;
}

///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for BRatio.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomHdrBRatioGet(void)
{
	return CUST_HDR_BRATIO;
}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for Gain.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomHdrGainArrayGet(MUINT32 u4ArrayIndex)
{
	if (u4ArrayIndex > MAX_HDR_GAIN_ARRAY_ELEM-1)
		u4ArrayIndex = MAX_HDR_GAIN_ARRAY_ELEM-1;

	return au4HdrGainArray[u4ArrayIndex];
}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for BottomFRatio.
///////////////////////////////////////////////////////////////////////////
double CustomHdrBottomFRatioGet(void)
{
	return CUST_HDR_BOTTOM_FRATIO;
}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for TopFRatio.
///////////////////////////////////////////////////////////////////////////
double CustomHdrTopFRatioGet(void)
{
	return CUST_HDR_TOP_FRATIO;
}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for BottomFBound.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomHdrBottomFBoundGet(void)
{
	return CUST_HDR_BOTTOM_FBOUND;
}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for TopFBound.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomHdrTopFBoundGet(void)
{
	return CUST_HDR_TOP_FBOUND;
}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for ThHigh.
///////////////////////////////////////////////////////////////////////////
MINT32 CustomHdrThHighGet(void)
{
	return CUST_HDR_TH_HIGH;
}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for ThLow.
///////////////////////////////////////////////////////////////////////////
MINT32 CustomHdrThLowGet(void)
{
	return CUST_HDR_TH_LOW;
}


///////////////////////////////////////////////////////////////////////////
/// @brief Get the customer-set value for TargetLevelSub.
///////////////////////////////////////////////////////////////////////////
MUINT32 CustomHdrTargetLevelSubGet(void)
{
	return CUST_HDR_TARGET_LEVEL_SUB;
}

/*******************************************************************************
* HDR exposure setting
*******************************************************************************/
#define MAX_LOG_BUF_SIZE	5000
static unsigned char GS_ucLogBuf[MAX_LOG_BUF_SIZE];	// Buffer to put log message. Will be outputed to file.
static char* pucLogBufPosition = NULL;	// A pointer pointing to some position in the GS_ucLogBuf[].
static unsigned int S_u4RunningNumber = 0;	// Record execution counts.

static unsigned int DumpToFile(
    char *fname,
    unsigned char *pbuf,
    unsigned int size
)
{
    int nw, cnt = 0;
    unsigned int written = 0;


    XLOGD("[DumpToFile] S_u4RunningNumber: %d.\n", S_u4RunningNumber);
    XLOGD("[DumpToFile] opening file [%s]\n", fname);
	FILE* pFp = fopen(fname, "a+t");	// a+: Opens for reading and appending. t: Open in text (translated) mode.
    if (pFp < 0) {
        XLOGE("[DumpToFile] failed to create file [%s]: %s", fname, strerror(errno));
        return 0x80000000;
    }

    XLOGD("[DumpToFile] writing %d bytes to file [%s]\n", size, fname);
    while (written < size) {
        nw = fwrite(pbuf + written, sizeof( char ), size - written, pFp);
        if (nw < 0) {
            XLOGE("[DumpToFile] failed to write to file [%s]: %s", fname, strerror(errno));
            break;
        }
        written += nw;
        cnt++;
    }
    XLOGD("[DumpToFile] done writing %d bytes to file [%s] in %d passes\n", size, fname, cnt);
    fclose(pFp);

    return 0;

}

/*
HDRFlag = 0;  // original version, always capture 3 frames
HDRFlag = 1;  // adaptive version, if original version use -2EV less, we only capture 2 frames (0EV and +2EV). If original version use -2EV a lot, we still capture 3 frames.
HDRFlag = 2;  // performance priority version, always capture 2 frames. The EV settings are decided adapively.
HDR_NEOverExp_Percent = 15; // this is a customer tuning parameter. When HDRFlag==1, it means if there is less than HDR_NEOverExp_Percent/1000 pixels over saturation in 0EV, we capture 2 frames instead.
*/

MVOID getHDRExpSetting(const HDRExpSettingInputParam_T& rInput, HDRExpSettingOutputParam_T& rOutput)
{
    // Tuning parameters
    MUINT32 HDRFlag = CUST_HDR_CAPTURE_ALGORITHM;
	MUINT32	HDR_NEOverExp_Percent = CUST_HDR_NEOverExp_Percent;
    MUINT32 u4MaxHDRExpTimeInUS = 200000; // Manually set, no longer than 0.5s (unit: us)
    MUINT32 u4MaxSafeHDRExpTimeInUS = 31250; // Manually set, no longer than 0.5s (unit: us)
    MUINT32 u4MaxHDRSensorGain = 4848; //Manually set, no larger than max gain in normal capture
    MUINT32 u4TimeMode = 1; // 0:Depend on default AE parameters; 1: Manually set
    MUINT32 u4GainMode = 1; // 0:Depend on default AE parameters; 1: Manually set
    double dfTargetTopEV =  1.5; // Target EV of long exposure image
    double dfSafeTargetTopEV = 0.5; // Target EV of long exposure image
    double dfTargetBottomEV = -2; // Target EV of short exposure image
    double dfTopEVBound = 0.2; // Long exposure image should be at least "dfTopEVBound" EV or would be discarded (2 frame case)
//  MBOOL bGainEnable = MTRUE; // True: Enable long exposure image to increase sensor gain; False: Disable
    MBOOL bGain0EVLimit = MFALSE;  // True: Limit the gain of 0EV and short exposure image; False: Keep it
	double dfFlag2TopEV = +0.5;
	double dfFlag2BottomEV = -1.5;
	double dfFlag2TopGain = pow(2, dfFlag2TopEV);
	double dfFlag2BottomGain = pow(2, dfFlag2BottomEV);
    //Add by ChingYi
    // Tuning parameters
// MUINT32 TargetToneMaxExposureTime = 100000; //Decide the exposure time start to decrease target tone
// MUINT32 TargetToneCurve = 10000; //Decide the curve to decide target tone
// MUINT32 TargetToneIn = 150; //Decide the curve to decide target tone

    // Temporary parameters
    MUINT32 u4MaxExpTimeInUS;
    MUINT32 u4MaxSensorGain;
	MUINT32 i;
	double dfRemainGain[3];
	double dfGainDiff[2];
	double dfTopGain     = pow(2, dfTargetTopEV);
	double dfSafeTopGain = pow(2, dfSafeTargetTopEV);
	double dfBottomGain  = pow(2, dfTargetBottomEV);

	if (u4TimeMode == 0) {
		u4MaxExpTimeInUS = rInput.u4MaxAEExpTimeInUS; // Depend on default AE parameters
    }
	else {
		u4MaxExpTimeInUS = u4MaxHDRExpTimeInUS; // Manually set
    }

	if (u4GainMode == 0) {
		u4MaxSensorGain = rInput.u4MaxAESensorGain; // Depend on default AE parameters
	}
	else {
		u4MaxSensorGain = u4MaxHDRSensorGain; // Manually set

	    if (u4MaxSensorGain > rInput.u4MaxSensorAnalogGain) {
			u4MaxSensorGain = rInput.u4MaxSensorAnalogGain;
		}
	}

    // Final output without any limitation
	if(rInput.u4SensorGain0EV > u4MaxHDRSensorGain)
		rOutput.u4ExpTimeInUS[2] = static_cast<MUINT32>(rInput.u4ExpTimeInUS0EV * dfSafeTopGain + 0.5);   //High ISO case: Long exposurerInput.u4ExpTimeInUS0EV;
	else
		rOutput.u4ExpTimeInUS[2] = static_cast<MUINT32>(rInput.u4ExpTimeInUS0EV * dfTopGain + 0.5);   //Low ISO case: Long exposurerInput.u4ExpTimeInUS0EV;
    rOutput.u4ExpTimeInUS[1] = rInput.u4ExpTimeInUS0EV; // 0EV
    rOutput.u4ExpTimeInUS[0] = static_cast<MUINT32>(rInput.u4ExpTimeInUS0EV * dfBottomGain + 0.5); // Short exposure
	rOutput.u4SensorGain[2] = rInput.u4SensorGain0EV;
	rOutput.u4SensorGain[1] = rInput.u4SensorGain0EV;
	rOutput.u4SensorGain[0] = rInput.u4SensorGain0EV;

	if (bGain0EVLimit == MTRUE) {// Limit 0EV or even short exposure image's gain but keep the brightness
		for (MUINT32 i = 0; i < 2; i++) {
			dfRemainGain[i] = 1;

			if (rOutput.u4SensorGain[i] > u4MaxHDRSensorGain) {
				dfRemainGain[i] = static_cast<double>(rOutput.u4SensorGain[i]) / u4MaxHDRSensorGain;
				rOutput.u4SensorGain[i] = u4MaxHDRSensorGain;
			}

			if (dfRemainGain[i] > 1) {
				rOutput.u4ExpTimeInUS[i] = static_cast<MUINT32>(rOutput.u4ExpTimeInUS[i] * dfRemainGain[i] + 0.5);

				if (rOutput.u4ExpTimeInUS[i] > u4MaxHDRExpTimeInUS) { //It means that we cannot keep original brightness, so we decide to keep original setting
					rOutput.u4ExpTimeInUS[1] = rInput.u4ExpTimeInUS0EV; // 0EV
					rOutput.u4ExpTimeInUS[0] = static_cast<MUINT32>(rInput.u4ExpTimeInUS0EV * dfBottomGain + 0.5); // Short exposure
					rOutput.u4SensorGain[1]   = rInput.u4SensorGain0EV;
					rOutput.u4SensorGain[0]   = rInput.u4SensorGain0EV;
				}
			}
		}
	}

//	if (bGainEnable == MFALSE) {
//		u4MaxSensorGain  = rOutput.u4SensorGain[1]; // MaxSensor gain should be equal to 0EV
//	}

	dfRemainGain[2] = 1;
	if(rInput.u4ExpTimeInUS0EV>u4MaxSafeHDRExpTimeInUS)
       u4MaxSafeHDRExpTimeInUS = rInput.u4ExpTimeInUS0EV;

	if (rOutput.u4ExpTimeInUS[2] > u4MaxSafeHDRExpTimeInUS) {
		dfRemainGain[2] = static_cast<double>(rOutput.u4ExpTimeInUS[2]) / u4MaxSafeHDRExpTimeInUS;
		rOutput.u4ExpTimeInUS[2] =  u4MaxSafeHDRExpTimeInUS;
	}

	if (dfRemainGain[2] > 1) {
		rOutput.u4SensorGain[2] = static_cast<MUINT32>(rOutput.u4SensorGain[2] * dfRemainGain[2] + 0.5);
		if (rOutput.u4SensorGain[2] > u4MaxSensorGain) {
			dfRemainGain[2] = static_cast<double>(rOutput.u4SensorGain[2]) / u4MaxSensorGain;
			rOutput.u4SensorGain[2] = u4MaxSensorGain;
		}
		else
			dfRemainGain[2] = 1;
	}

	if (dfRemainGain[2] > 1) {
		rOutput.u4ExpTimeInUS[2] = static_cast<MUINT32>(rOutput.u4ExpTimeInUS[2] * dfRemainGain[2] + 0.5);

		if (rOutput.u4ExpTimeInUS[2] >u4MaxExpTimeInUS)
		{
			dfRemainGain[2] = static_cast<double>(rOutput.u4ExpTimeInUS[2]) / u4MaxExpTimeInUS;
			rOutput.u4ExpTimeInUS[2]  = u4MaxExpTimeInUS;
		}
	}

	dfGainDiff[0] = static_cast<double>(rOutput.u4SensorGain[0]*rOutput.u4ExpTimeInUS[0]) / rOutput.u4SensorGain[1] / rOutput.u4ExpTimeInUS[1];
	dfGainDiff[1] = static_cast<double>(rOutput.u4SensorGain[2]*rOutput.u4ExpTimeInUS[2]) / rOutput.u4SensorGain[1] / rOutput.u4ExpTimeInUS[1];

	rOutput.u1FlareOffset[1] = rInput.u1FlareOffset0EV;
	if(rOutput.u1FlareOffset[1]>4)
       rOutput.u1FlareOffset[1] = 4;

	rOutput.u1FlareOffset[0] = static_cast<MUINT8>(rOutput.u1FlareOffset[1] * dfGainDiff[0] + 0.5);
	rOutput.u1FlareOffset[2] = static_cast<MUINT8>(rOutput.u1FlareOffset[1] * dfGainDiff[1] + 0.5);

	if(rOutput.u1FlareOffset[2]>63)
         rOutput.u1FlareOffset[2] = 63; //hardware limitation --> may lead to alignment issue

	double dfTopGainBound = pow(2, dfTopEVBound);
	if (dfTopGainBound < dfGainDiff[1]) {
		rOutput.u4OutputFrameNum = 3;
    }
	else {
		rOutput.u4OutputFrameNum = 2;
	}

	//Add by ChingYi
//	if(rOutput.u4ExpTimeInUS[2] > TargetToneMaxExposureTime && rOutput.u4OutputFrameNum == 3)
//		rOutput.u4TargetTone = TargetToneIn - (rOutput.u4ExpTimeInUS[2]-TargetToneMaxExposureTime)/TargetToneCurve;
//	else
//		rOutput.u4TargetTone = TargetToneIn;
	rOutput.u4TargetTone = 150;

	rOutput.u4FinalGainDiff[0] = static_cast<MUINT32>(1024 / dfGainDiff[0] + 0.5);
	rOutput.u4FinalGainDiff[1] = static_cast<MUINT32>(1024 / dfGainDiff[1] + 0.5);

	MUINT32	HISsum = 0;
	if( HDRFlag == 1)   // adaptive version, if original version use -2EV less, we only capture 2 frames (0EV and +2EV). If original version use -2EV a lot, we still capture 3 frames.
	{
		//calculate total bin count of AE histogram
		HISsum = 0;
		for(i=0 ; i<128 ; i++)
			HISsum = HISsum + rInput.u4Histogram[i];

		if( (int)((rInput.u4Histogram[126] + rInput.u4Histogram[127]) * 1000 / HISsum + 0.5) < HDR_NEOverExp_Percent) //calculate the percentage of bin[126]+bin[127]
		{
			rOutput.u4OutputFrameNum   = 2;      //capture 2 frames NE(0EV) and LE(+2EV)
			rOutput.u4ExpTimeInUS[0]   = rOutput.u4ExpTimeInUS[2];
			rOutput.u4SensorGain[0]    = rOutput.u4SensorGain[2];
			rOutput.u1FlareOffset[0]   = rOutput.u1FlareOffset[2];
			rOutput.u4FinalGainDiff[0] = rOutput.u4FinalGainDiff[1];
		}
		else
		{
			rOutput.u4OutputFrameNum = 3;
		}
	}

	if( HDRFlag == 2)  // performance priority version, always capture 2 frames. The EV settings are decided adapively.
	{
		rOutput.u4OutputFrameNum = 2;

		//calculate total bin count of AE histogram
		HISsum = 0;
		for(i=0 ; i<128 ; i++)
			HISsum = HISsum + rInput.u4Histogram[i];

		if( (int)((rInput.u4Histogram[126] + rInput.u4Histogram[127]) * 1000 / HISsum + 0.5) < HDR_NEOverExp_Percent) //calculate the percentage of bin[126]+bin[127]
		{
			rOutput.u4ExpTimeInUS[0]   = rOutput.u4ExpTimeInUS[2];     //capture 2 frames NE(0EV) and LE(+2EV)
			rOutput.u4SensorGain[0]    = rOutput.u4SensorGain[2];
			rOutput.u1FlareOffset[0]   = rOutput.u1FlareOffset[2];
			rOutput.u4FinalGainDiff[0] = rOutput.u4FinalGainDiff[1];
		}
		else
		{   //capture 2 frames NE(-0.5 EV) and LE(+1.5 EV)
			rOutput.u4ExpTimeInUS[0] = static_cast<MUINT32>(rInput.u4ExpTimeInUS0EV * dfFlag2BottomGain + 0.5); // long exposure
			rOutput.u4ExpTimeInUS[1] = static_cast<MUINT32>(rInput.u4ExpTimeInUS0EV * dfFlag2TopGain + 0.5); // normal exposure
			rOutput.u4SensorGain[0] = rInput.u4SensorGain0EV;
			rOutput.u4SensorGain[1] = rInput.u4SensorGain0EV;
			dfGainDiff[0] = static_cast<double>(rOutput.u4SensorGain[0]*rOutput.u4ExpTimeInUS[0]) / rOutput.u4SensorGain[1] / rOutput.u4ExpTimeInUS[1];
			rOutput.u4FinalGainDiff[0] = static_cast<MUINT32>(1024 / dfGainDiff[0] + 0.5);
			rOutput.u1FlareOffset[1] = rInput.u1FlareOffset0EV;
			if(rOutput.u1FlareOffset[1]>4)
				rOutput.u1FlareOffset[1] = 4;
			rOutput.u1FlareOffset[0] = static_cast<MUINT8>(rOutput.u1FlareOffset[1] * dfGainDiff[0] + 0.5);
		}
	}


#if 0 // for debug only: adb logcat HdrCustome:V *:S
#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "HdrCustome"

    XLOGD("System Paramters\n");
    XLOGD("0EV Exposure Time = %d\n0EV Sensor Gain = %d\n", rInput.u4ExpTimeInUS0EV, rInput.u4SensorGain0EV);
	XLOGD("Max Exposure Time 0EV = %d\nMaxSensor Gain 0EV = %d\n", rInput.u4MaxAEExpTimeInUS, rInput.u4MaxAESensorGain);
	XLOGD("Max Exposure Time Sensor = %d\nMaxSensor Gain Sensor = %d\n", u4MaxHDRExpTimeInUS, u4MaxHDRSensorGain);
    XLOGD("Final Max Exposure Time = %d\nFinal MaxSensor Gain = %d\n", u4MaxExpTimeInUS, u4MaxSensorGain);

    XLOGD("\nTuning Paramters\n");
	XLOGD("Target Top EV = %f\nTarget Bottom EV = %f\n", dfTargetTopEV, dfTargetBottomEV);
	XLOGD("dfTopGainBound = %f\n",dfTopGainBound);
   XLOGD("bGain0EVLimit = %s\n", (bGain0EVLimit ? "true" : "false"));

	XLOGD("\nOutput Paramters\n");
	for (i = 0; i < 3; i++) {
		XLOGD("Final Frame %d ExposureTime = %d\nSensorGain = %d\n", i, rOutput.u4ExpTimeInUS[i], rOutput.u4SensorGain[i]);
    }
    XLOGD("Final EVdiff[0] = %d\nFinal EVdiff[1] = %d\n", rOutput.u4FinalGainDiff[0], rOutput.u4FinalGainDiff[1]);
	XLOGD("OutputFrameNum = %d\n", rOutput.u4OutputFrameNum);
	XLOGD("Final FlareOffsetOut[0]= %d\nFinal FlareOffsetOut[1]= %d\nFinal FlareOffsetOut[2]= %d\n", rOutput.u1FlareOffset[0], rOutput.u1FlareOffset[1], rOutput.u1FlareOffset[2]);
    XLOGD("Final TargetTone1 = %d\n", rOutput.u4TargetTone);
#endif	// for debug only: adb logcat HdrCustome:V *:S

//#if (ENABLE_HDR_AE_DEBUG_INFO)
// Save HDR AE debug info to a file.
    char value[32] = {'\0'};
    property_get("mediatek.hdr.debug", value, "0");
    int hdr_debug_mode = atoi(value) || CUST_HDR_DEBUG;
	if(hdr_debug_mode) {
		// Increase 4-digit running number (range: 1 ~ 9999).
		if (S_u4RunningNumber >= 9999)
			S_u4RunningNumber = 1;
		else
			S_u4RunningNumber++;

		pucLogBufPosition = (char*)GS_ucLogBuf;
	    ::sprintf(pucLogBufPosition, "< No.%04d > ----------------------------------------------------------------------\n", S_u4RunningNumber);
	    pucLogBufPosition += strlen(pucLogBufPosition);
	    ::sprintf(pucLogBufPosition, "[System Paramters]\n");
	    pucLogBufPosition += strlen(pucLogBufPosition);
	    ::sprintf(pucLogBufPosition, "0EV Exposure Time = %d\n0EV Sensor Gain = %d\n", rInput.u4ExpTimeInUS0EV, rInput.u4SensorGain0EV);
	    pucLogBufPosition += strlen(pucLogBufPosition);
	    ::sprintf(pucLogBufPosition, "Max Exposure Time Sensor= %d\nMaxSensor Gain Sensor= %d\n", rInput.u4MaxAEExpTimeInUS, rInput.u4MaxAESensorGain);
	    pucLogBufPosition += strlen(pucLogBufPosition);
	    ::sprintf(pucLogBufPosition, "Max Exposure Time Manual= %d\nMaxSensor Gain Manual= %d\n", u4MaxHDRExpTimeInUS, u4MaxHDRSensorGain);
	    pucLogBufPosition += strlen(pucLogBufPosition);
	    ::sprintf(pucLogBufPosition, "Max Exposure Time = %d\nMaxSensor Gain = %d\n", u4MaxExpTimeInUS, u4MaxSensorGain);
	    pucLogBufPosition += strlen(pucLogBufPosition);

	    ::sprintf(pucLogBufPosition, "\n[Tuning Paramters]\n");
	    pucLogBufPosition += strlen(pucLogBufPosition);
		::sprintf(pucLogBufPosition, "Target Top EV = %f\nTarget Bottom EV = %f\n", dfTargetTopEV, dfTargetBottomEV);
	    pucLogBufPosition += strlen(pucLogBufPosition);
		::sprintf(pucLogBufPosition, "dfTopGainBound = %f\n",dfTopGainBound);
	    pucLogBufPosition += strlen(pucLogBufPosition);
		::sprintf(pucLogBufPosition, "bGain0EVLimit = %s\n", (bGain0EVLimit ? "true" : "false"));
	    pucLogBufPosition += strlen(pucLogBufPosition);

		::sprintf(pucLogBufPosition, "\n[Output Paramters]\n");
	    pucLogBufPosition += strlen(pucLogBufPosition);
		for (i = 0; i < 3; i++) {
			::sprintf(pucLogBufPosition, "Final Frame %d ExposureTime = %d\nSensorGain = %d\n", i, rOutput.u4ExpTimeInUS[i], rOutput.u4SensorGain[i]);
		    pucLogBufPosition += strlen(pucLogBufPosition);
	    }
	    ::sprintf(pucLogBufPosition, "Final EVdiff[0] = %d\nFinal EVdiff[1] = %d\n", rOutput.u4FinalGainDiff[0], rOutput.u4FinalGainDiff[1]);
	    pucLogBufPosition += strlen(pucLogBufPosition);
		::sprintf(pucLogBufPosition, "OutputFrameNum = %d\n", rOutput.u4OutputFrameNum);
	    pucLogBufPosition += strlen(pucLogBufPosition);
		::sprintf(pucLogBufPosition, "Final FlareOffsetOut[0]= %d\nFinal FlareOffsetOut[1]= %d\nFinal FlareOffsetOut[2]= %d\n", rOutput.u1FlareOffset[0], rOutput.u1FlareOffset[1], rOutput.u1FlareOffset[2]);
	    pucLogBufPosition += strlen(pucLogBufPosition);
		::sprintf(pucLogBufPosition, "Final TargetTone= %d\n", rOutput.u4TargetTone);
	    pucLogBufPosition += strlen(pucLogBufPosition);

		char szFileName[100];
		//::sprintf(szFileName, "sdcard/Photo/%04d_HDR_ExposureSetting.txt", S_u4RunningNumber);	// For ALPS.GB2.
		::sprintf(szFileName, HDR_DEBUG_OUTPUT_FOLDER"%04d_HDR_ExposureSetting.txt", S_u4RunningNumber);	// For ALPS.ICS.
		DumpToFile(szFileName, (unsigned char *)GS_ucLogBuf, MAX_LOG_BUF_SIZE);

	}
//#endif	// ENABLE_HDR_AE_DEBUG_INFO

}

/*******************************************************************************
*
*******************************************************************************/
