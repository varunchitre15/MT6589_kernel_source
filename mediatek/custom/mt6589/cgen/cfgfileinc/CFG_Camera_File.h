
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   MSDK_NVRAM_CAMERA_exp.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *   Definition of the data structures of ISP drivers that will be stored into NRVAM
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
 * 11 16 2011 koli.lin
 * [ALPS00030473] [Camera]
 * [Camera] Add two parameters to NVRAM for CCT tuning.
 *
 * 05 17 2010 koli.lin
 * [ALPS00000143][Camera]
 * Synchronize the NVRAM structure and Code gen.
 *
 * 05 14 2010 koli.lin
 * [ALPS00000143][Camera]
 * Add one parameters for AE NVRAM used.
 *
 * Mar 21 2009 mtk80306
 * [DUMA00112158] fix the code convention.
 * fix the codeing convention.
 *
 * Mar 15 2009 mtk80306
 * [DUMA00111629] add camera nvram files
 * add camera nvram file
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#ifndef __MSDK_NVRAM_CAMERA_EXP_H
#define __MSDK_NVRAM_CAMERA_EXP_H

#include "CFG_Camera_File_Max_Size.h"

typedef unsigned char       UINT8;



typedef struct
{
   UINT8 Data[MAXIMUM_NVRAM_CAMERA_DEFECT_FILE_SIZE];
} NVRAM_CAMERA_DEFECT_STRUCT, *PNVRAM_CAMERA_DEFECT_STRUCT;

/*******************************************************************************
* shading
********************************************************************************/

typedef struct
{
   	UINT8 CameraData[MAXIMUM_NVRAM_CAMERA_SHADING_FILE_SIZE];
} NVRAM_CAMERA_SHADING_STRUCT, *PNVRAM_CAMERA_SHADING_STRUCT;

/*******************************************************************************
* 3A
********************************************************************************/

typedef struct
{
    UINT8 Data[MAXIMUM_NVRAM_CAMERA_3A_FILE_SIZE];
} NVRAM_CAMERA_3A_STRUCT, *PNVRAM_CAMERA_3A_STRUCT;

/*******************************************************************************
* ISP parameter
********************************************************************************/

typedef struct
{
    UINT8   Data[MAXIMUM_NVRAM_CAMERA_ISP_FILE_SIZE];
} NVRAM_CAMERA_ISP_PARAM_STRUCT, *PNVRAM_CAMERA_ISP_PARAM_STRUCT;


/*******************************************************************************
* Lens
********************************************************************************/

typedef struct
{
    UINT8 reserved[MAXIMUM_NVRAM_CAMERA_LENS_FILE_SIZE];
} NVRAM_LENS_PARA_STRUCT, *PNVRAM_LENS_PARA_STRUCT;

/* define the LID and total record for NVRAM interface */
#define CFG_FILE_CAMERA_PARA_REC_SIZE			MAXIMUM_NVRAM_CAMERA_ISP_FILE_SIZE
#define CFG_FILE_CAMERA_3A_REC_SIZE			    MAXIMUM_NVRAM_CAMERA_3A_FILE_SIZE
#define CFG_FILE_CAMERA_SHADING_REC_SIZE		MAXIMUM_NVRAM_CAMERA_SHADING_FILE_SIZE
#define CFG_FILE_CAMERA_DEFECT_REC_SIZE			MAXIMUM_NVRAM_CAMERA_DEFECT_FILE_SIZE
#define CFG_FILE_CAMERA_SENSOR_REC_SIZE			MAXIMUM_NVRAM_CAMERA_SENSOR_FILE_SIZE
#define CFG_FILE_CAMERA_LENS_REC_SIZE			MAXIMUM_NVRAM_CAMERA_LENS_FILE_SIZE

#define CFG_FILE_CAMERA_PARA_REC_TOTAL			3
#define CFG_FILE_CAMERA_3A_REC_TOTAL			3
#define CFG_FILE_CAMERA_SHADING_REC_TOTAL		3
#define CFG_FILE_CAMERA_DEFECT_REC_TOTAL		3
#define CFG_FILE_CAMERA_SENSOR_REC_TOTAL		3
#define CFG_FILE_CAMERA_LENS_REC_TOTAL		    3


#endif
