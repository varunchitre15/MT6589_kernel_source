#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

//#include "msdk_nvram_camera_exp.h"
//#include "msdk_lens_exp.h"
#include "camera_custom_nvram.h"
#include "camera_custom_lens.h"


const NVRAM_LENS_PARA_STRUCT OV8825AF_LENS_PARA_DEFAULT_VALUE =
{
    //Version
    NVRAM_CAMERA_LENS_FILE_VERSION,

    // Focus Range NVRAM
    {0, 1023},

    // AF NVRAM
    {
        {{
	    {// Exact Search
	    6,		// i4NormalNum
	    8,		// i4MarcoNum
    	{
           0,  138,  202,  274, 355 ,436, 506, 570, 0, 0, 0, 
           0, 0, 0, 0,   0,   0,   0,   0,   0,
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0
              
	    }
    	}
	    }},

        {100},          //i4ZoomTable[0]
        30,             // i4AF_THRES_MAIN
        20,             // i4AF_THRES_SUB
        10000,          // i4AF_THRES_OFFSET
        5,              // THRES OFFSET GAIN for LOW LIGHT
        30,             // i4LV_THRES
        5,              // i4MATRIX_AF_DOF
        3,              // i4MATRIX_AF_WIN_NUM
        15000,          // i4AFC_THRES_OFFSET;
        10,              // i4AFC_STEPSIZE
        4,              // i4AFC_SPEED
        15,             // i4SCENE_CHANGE_THRES
        10,             // i4SCENE_CHANGE_CNT

        15,             // i4SPOT_PERCENT_X
        15,             // i4SPOT_PERCENT_Y
        15,             // i4MATRIX_PERCENT_X
        15,             // i4MATRIX_PERCENT_Y
        5,              // i4MATRIX_LOC_OFFSET

        20000,          // i4TUNE_PARA1 : THRES OFFSET for Matrix
        0,              // i4TUNE_PARA2 : Infinity position for normal
        0               // i4TUNE_PARA3 : Infinity position for macro
    },

    {0}
};


UINT32 OV8825AF_getDefaultData(VOID *pDataBuf, UINT32 size)
{
    UINT32 dataSize = sizeof(NVRAM_LENS_PARA_STRUCT);

    if ((pDataBuf == NULL) || (size < dataSize))
    {
        return 1;
    }

    // copy from Buff to global struct
    memcpy(pDataBuf, &OV8825AF_LENS_PARA_DEFAULT_VALUE, dataSize);

    return 0;
}

PFUNC_GETLENSDEFAULT pOV8825AF_getDefaultData = OV8825AF_getDefaultData;


