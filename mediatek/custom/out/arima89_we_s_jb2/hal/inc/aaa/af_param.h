

/*
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef _AF_PARAM_H
#define _AF_PARAM_H

#define AF_WIN_NUM_SPOT  1
#define MAX_AF_HW_WIN   37

#define FD_WIN_NUM     15

#define PATH_LENGTH    100

typedef struct
{
    MINT32 i4X;
    MINT32 i4Y;
    MINT32 i4W;
    MINT32 i4H;
    MINT32 i4Info;
    
} AREA_T;

typedef struct 
{
	MINT32  i4Count;
	AREA_T  sRect[AF_WIN_NUM_SPOT];    
    
} AF_AREA_T;

typedef struct
{
    MINT32 i4SGG_GAIN;
    MINT32 i4SGG_GMR1;
    MINT32 i4SGG_GMR2;
    MINT32 i4SGG_GMR3;
    MINT32 AF_DECI_1;
    MINT32 AF_ZIGZAG;
    MINT32 AF_ODD;
    MINT32 AF_IIR_KEEP;
    MINT32 AF_FILT_F0;
    MINT32 AF_FILT1[12];
    MINT32 AF_TH[3];
    MINT32 AF_THEX[2];

} AF_CONFIG_T;

typedef struct
{
    MINT32 AF_WINX[6];
    MINT32 AF_WINY[6];
    MINT32 AF_XSIZE;
    MINT32 AF_YSIZE;
    MINT32 AF_WINXE;
    MINT32 AF_WINYE;
    MINT32 AF_SIZE_XE;
    MINT32 AF_SIZE_YE;

} AF_WIN_CONFIG_T;


typedef struct
{
    MUINT32 u4Stat24;
    MUINT32 u4Stat32;
    MUINT32 u4StatV;

} AF_HW_STAT_SINGLE_T;

typedef struct
{
    AF_HW_STAT_SINGLE_T sStat[MAX_AF_HW_WIN];

} AF_HW_STAT_T;

typedef struct
{
    MINT64 i8Stat24;
    MINT64 i8StatFL;
    MINT64 i8StatV;

} AF_STAT_T;

typedef struct
{
    MINT64 i8StatH[MAX_AF_HW_WIN-1];
    MINT64 i8StatV[MAX_AF_HW_WIN-1];

} AF_FULL_STAT_T;

typedef struct
{
	MINT32 i4CurrentPos;        //current position
	MINT32 i4MacroPos;          //macro position
	MINT32 i4InfPos;            //Infiniti position
	MINT32 bIsMotorMoving;      //Motor Status
	MINT32 bIsMotorOpen;        //Motor Open?
    MINT32 bIsSupportSR;

} LENS_INFO_T;

typedef struct
{
    MINT32      i4IsZSD;
    MINT32      i4IsAEStable;
    MINT64      i8GSum;
    MINT32      i4ISO; 
    MINT32      i4SceneLV;
    MINT32      i4FullScanStep;
    AF_AREA_T   sAFArea;
    AF_STAT_T   sAFStat;
    LENS_INFO_T sLensInfo;
    AREA_T      sEZoom;

} AF_INPUT_T;

typedef struct
{
    MINT32      i4IsAFDone;
    MINT32      i4IsFocused;
    MINT32      i4IsMonitorFV;
    MINT32      i4AFBestPos;
    MINT32      i4AFPos;    
    MINT64      i8AFValue;
    AF_CONFIG_T sAFStatConfig;
    AF_AREA_T   sAFArea;

} AF_OUTPUT_T;

typedef struct
{
    MINT32  i4AFS_STEP_MIN_ENABLE;
    MINT32  i4AFS_STEP_MIN_NORMAL;
    MINT32  i4AFS_STEP_MIN_MACRO;

    MINT32  i4AFS_MODE;    //0 : singleAF, 1:smoothAF
    MINT32  i4AFC_MODE;    //0 : singleAF, 1:smoothAF    
    MINT32  i4VAFC_MODE;   //0 : singleAF, 1:smoothAF    

    MINT32  i4ReadOTP;     //0 : disable, 1:enable     

    MINT32  i4FD_DETECT_CNT;    
    MINT32  i4FD_NONE_CNT;

    MINT32  i4FV_SHOCK_THRES;
    MINT32  i4FV_SHOCK_OFFSET;
    MINT32  i4FV_VALID_CNT;
    MINT32  i4FV_SHOCK_FRM_CNT;
    MINT32  i4FV_SHOCK_CNT;

} AF_PARAM_T;

typedef struct
{
    MINT32  i4Num;
    MINT32  i4Pos[PATH_LENGTH];

} AF_STEP_T;

typedef enum
{
	AF_MARK_NORMAL = 0,
	AF_MARK_OK,
	AF_MARK_FAIL,
	AF_MARK_NONE

} AF_MARK_T;

// AF info for ISP tuning
typedef struct
{
    MINT32 i4AFPos; // AF position
    
} AF_INFO_T;


#endif

