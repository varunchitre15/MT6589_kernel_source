#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

//#include "msdk_nvram_camera_exp.h"
#include "camera_custom_nvram.h"
#include "camera_custom_lens.h"
//#include "msdk_lens_exp.h"


const NVRAM_LENS_PARA_STRUCT DUMMY_LENS_PARA_DEFAULT_VALUE =
{
    //Version
    NVRAM_CAMERA_LENS_FILE_VERSION,

    // Focus Range NVRAM
    {0, 0},

    // AF NVRAM
	{
		{{
			{// Exact Search
			    0,		// i4NormalNum
			    0,		// i4MarcoNum
				{
			      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
			      0,   0,	0,	 0,   0,   0,	0,	 0,   0,   0
				}
			}
		}},

		{0},		//i4ZoomTable[0]
	    0,		// i4AF_THRES_MAIN
	    0,		// i4AF_THRES_SUB
	    0,	    // i4AF_THRES_OFFSET
	    0,		// i4AF_CURVE_SCORE
	    0,		// i4LV_THRES
	    0,		// i4MATRIX_AF_DOF
	    0,		// i4MATRIX_AF_WIN_NUM
	    0,      // i4AFC_THRES_OFFSET;
	    0,		// i4AFC_STEPSIZE
	    0,		// i4AFC_SPEED
	    0,		// i4SCENE_CHANGE_THRES
	    0,		// i4SCENE_CHANGE_CNT

		0,		// i4SPOT_PERCENT_X
		0,		// i4SPOT_PERCENT_Y
		0,		// i4MATRIX_PERCENT_X
		0,		// i4MATRIX_PERCENT_Y
		0,		// i4MATRIX_LOC_OFFSET

		0,		// i4TUNE_PARA1;
		0,		// i4TUNE_PARA2;
		0		// i4TUNE_PARA3; 			
	},

    {0}	
};


UINT32 Dummy_getDefaultData(VOID *pDataBuf, UINT32 size)
{
	UINT32 dataSize = sizeof(NVRAM_LENS_PARA_STRUCT);

    if ((pDataBuf == NULL) || (size < dataSize))
    {
        return 1;
    }

	// copy from Buff to global struct
    memcpy(pDataBuf, &DUMMY_LENS_PARA_DEFAULT_VALUE, dataSize);

    return 0;
}

PFUNC_GETLENSDEFAULT pDummy_getDefaultData = Dummy_getDefaultData;


