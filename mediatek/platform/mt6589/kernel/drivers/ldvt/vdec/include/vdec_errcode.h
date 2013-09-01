

#ifndef _VDEC_ERROR_CODE_H
#define _VDEC_ERROR_CODE_H
//#include "drv_config.h"

#define VDEC_NONE_ERROR          0x00000000
#define SEQH_LOC_ERR        0x00000001
#define GOPH_LOC_ERR        0x00000002
#define PICH_LOC_ERR        0x00000003
#define SEQ_ERR_CODE        0x00000004
#define HORI_SIZE_ERR       0x00000005 
#define VERT_SIZE_ERR       0x00000006 
#define MPEG_VER_ERR        0x00000007 

#define ASP_RAT_RES         0x00000000

#define FRAME_RATE_ERR      0x00000008  
#define BIT_RATE_RES        0x00000009  
#define I_Q_MAT_ERR         0x0000000b 
#define NI_Q_MAT_ERR        0x0000000c

#define SEQE_LOC_ERR        0x0000000d
#define PROF_ID_RES         0x0000000e
#define LEV_ID_RES          0x0000000f
#define CH_FORMAT_RES       0x00000010


#define SEQH_MKB_ERR        0x0
#define SEQE_MKB_ERR        0x0
#define COLOR_PRI_ERR       0x0
#define TRA_CHA_ERR         0x0
#define MAT_COE_ERR         0x0
#define SEQDE_MKB_ERR       0x0
#define GOP_MKB_ERR         0x0
#define PICDE_MKB1_ERR      0x0
#define PICDE_MKB2_ERR      0x0
#define PICCE_MKB1_ERR      0x0
#define PICCE_MKB2_ERR      0x0
#define PICCE_MKB3_ERR      0x0


#define PIC_COD_TYP_ERR     0x00000017
#define F_CODE_00_ERR       0x00000018
#define F_CODE_01_ERR       0x00000019
#define F_CODE_10_ERR       0x0000001a
#define F_CODE_11_ERR       0x0000001b
#define PIC_STR_ERR         0x0000001c

#define PICCE_RES_ERR       0x0000001f

#define EXT_ST_COD_ERR      0x00000023
#define USR_DAT_BG_ERR      0x00000024
//#define CC_DAT_MKB_ERR      0x00000025

#define PIC_DIM_ERR         0x00000030
#define END_OF_FILE         0x00000031

#define VDEC_PRS_INQ_COMPLETE         0x00000040
#define VDEC_DRIP_FRM_PRS_COMPLETE         0x00000041

#define NO_START_C_ERR1     0x00000251
#define NO_START_C_ERR2     0x00000252
#define NO_START_C_ERR3     0x00000253
#define NO_START_C_ERR4     0x00000254
#define NO_START_C_ERR5     0x00000255
#define NO_START_C_ERR6     0x00000256
#define NO_START_C_ERR7     0x00000257
#define NO_START_C_ERR8     0x00000258
#define NO_START_C_ERR9     0x00000259
#define NO_START_C_ERR10    0x0000025A
#define NO_START_C_ERR11    0x0000025B
#define NO_START_C_ERR12    0x0000025C
#define NO_START_C_ERR13    0x0000025D
#define NO_START_C_ERR14    0x0000025E
#define NO_START_C_ERR15    0x0000025F
#define NO_START_C_ERR16    0x00000260
#define NO_EXT_START_CODE  0x00000261  

#define EXT_DATA_IDX_ERR	0x00000026
#define BAR_SHIFTER_ERR	0x00000029
#define VLD_RDPTR_ERR       	0x00000030
#define FB_ID_UNKNOWN		0x00000031
#define NO_DECODE			0x00000032
#define DECODE_PREVIOUS_I	0x00000033
#define FORBIDEN_ERR        	0x00000034
#define NO_PIC_SIZE_INFO	0x00000035
#define BROKEN_LINK_B_SKIP  0x00000036
#define JUMPMODE_NO_DECODE 0x00000037
#define NO_ENOUGH_FBUF       0x00000038

#define P_REFERNCE_INVALID	0x00009001
#define B_REFERNCE_INVALID	0x00009002
#define FIELD_ERR	    		0x00009004
#define REFERNCE_MISSING	0x00009005
#define FBG_ID_UNKNOWN 	0x00009006
#define B_WAIT_INTRA_PIC	0x00009007
#define NEED_EXTRACT_CC 	0x00009000

#define H264_SET_INVALID 	0x02640001

#define VDEC_INV_CONN_INFO          ((INT32) -256)

// *********************************************************************
// return value defined for MPEG4
// *********************************************************************
#define VOP_CODED_0           0x00000403
#define VOP_DUMMY_CODED  0x00000404
/* Short Header */
#define SRC_FMT_ERR         0x00000201

// *********************************************************************
// return value defined for WMV
// *********************************************************************
#define WMV_XINTRA8_ERR      0x00000405

// *********************************************************************
// return value defined for over hardware spec
// *********************************************************************
#define VDEC_OVER_HW_SPEC_ERR                       0x00000501
#define VDEC_CHIP_BOUNDING_NOT_SUPPORT     0x00000502


// *********************************************************************
// return value define of vdec relate function
// *********************************************************************
#define VDEC_ERR                   0               
#define VDEC_SUCCESS               1
// no picture index to parse
#define VDEC_NO_PIC                0x10
// no free frame buffer to decode
#define VDEC_NO_FBUF               0x11
// wait until correct PTS to decode
#define VDEC_WAIT_AVSYNC           0x12
#define VDEC_SKIP_PIC                0x13
#define VDEC_REPARSE                 0x14
// stream syntext error in seq/gop/picture layer
#define VDEC_HDR_ERR               0xE0
// DTS Time out
#define VDEC_DTS_TIMEOUT           0xE1
// decode Time out,  video decoder fail to send decode end interrupt
#define VDEC_DEC_TIMEOUT           0xE2
// decode picture error, video decoder return Error Code
#define VDEC_DEC_ERR               0xE3

#define VDEC_HDR_SUCCESS              0x0

//#if(CONFIG_DRV_VERIFY_SUPPORT )
/* Visual Object layer */
#define VIS_OBJ_TYPE_ERR    0x00000001
#define M4V_NO_START_C_ERR1 0x00000002

/* VOL layer */
#define VOL_TOP_ERR         0x00000101
#define UNKNOWN_SHAPE_ERR   0x00000102
#define UNKNOWN_SPRITE_ERR  0x00000103
#define GMC_BR_CHG_ERR      0x00000104
#define NOT_8_BIT_ERR       0x00000105
#define COMP_EST_ERR        0x00000106
#define NEWPRED_ERR         0x00000107
#define SCALABILITY_ERR     0x00000108
#define INTRA_Q_BARSH_ERR   0x00000109
#define NINTRA_Q_BARSH_ERR  0x0000010a
#define DATA_PARTITION_ERR  0x0000010b

/* Short Header */
#define SRC_FMT_ERR         0x00000201

/* GOV layer */
#define GOV_MKB_ERR         0x00000301

/* VOP layer */
#define VOP_SC_ERR          0x00000401
#define WARPING_PT_ERR      0x00000402
#define VOP_CODED_0         0x00000403
//#endif

#endif


