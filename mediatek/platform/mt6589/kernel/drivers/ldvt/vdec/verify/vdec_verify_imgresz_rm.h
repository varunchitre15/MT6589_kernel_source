#include "vdec_verify_mpv_prov.h"
#include "../hal/vdec_hal_if_common.h"
#include "../hal/vdec_hal_if_wmv.h"
#include "vdec_verify_keydef.h"
//#include "vdec_drv_wmv_info.h"
//#include <string.h>
//#include <stdio.h>
//#include <math.h>
#include "vdec_verify_file_common.h"
//#include "vdec_verify_vparser_wmv.h"

#include "../hal/vdec_hw_common.h"
//#include "x_hal_1176.h"

#include "vdec_verify_general.h"
//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>

#define IMG_RESZ_REG_OFFSET0   0x4400

#define IMG_RESZ_START            0
#define IMG_RESZ_TYPE             1
#define IMG_RESZ_JPG_MODE         2
#define RZ_MEM_IF_MODE            3
#define RZ_SRC_BUF_LEN            4
#define RZ_JPEG_INT_SWITCH        5
#define RZ_TG_BUF_LEN             6
#define RZ_SRC_Y_OW_ADDR_BASE1    7
#define RZ_SRC_Y_OW_ADDR_BASE2    8
#define RZ_SRC_CB_OW_ADDR_BASE1   9
#define RZ_SRC_CB_OW_ADDR_BASE2   10
#define RZ_SRC_CR_OW_ADDR_BASE1   11
#define RZ_SRC_CR_OW_ADDR_BASE2   12
#define RZ_TG_Y_OW_ADDR_BASE      13
#define RZ_TG_C_OW_ADDR_BASE      14
#define RZ_SRC_SIZE_Y             16
#define RZ_SRC_SIZE_CB            17
#define RZ_SRC_SIZE_CR            18
#define RZ_TG_SIZE_Y              19
#define RZ_TG_SIZE_C              20
#define RZ_SRC_OFFSET_Y           21
#define RZ_SRC_OFFSET_C           22
#define RZ_SRC_OFFSET_CR          23
#define RZ_TG_OFFSET_Y            24
#define RZ_H8TAPS_SCL_Y           25
#define RZ_H8TAPS_SCL_CB          26
#define RZ_H8TAPS_SCL_CR          27
#define RZ_HEC_SCL_Y              28
#define RZ_HEC_SCL_CB             29
#define RZ_HEC_SCL_CR             30
#define RZ_V_SCL_Y                31
#define RZ_V_SCL_CB               32
#define RZ_V_SCL_CR               33
#define RZ_V4TAPS_SCL_Y           34
#define RZ_V4TAPS_SCL_CB          35
#define RZ_V4TAPS_SCL_CR          36
#define RZ_TMP_Y_OW_ADDR_BASE     37
#define RZ_PLD_Y_OW_ADDR_BASE     38
#define RZ_PLD_C_OW_ADDR_BASE     39
#define RZ_CPU_ASSIGN             40
#define RZ_SRC_CNT_Y              41
#define RZ_SRC_CNT_CB             42
#define RZ_SRC_CNT_CR             43
#define RZ_V_WR_OFFSET_Y          44
#define RZ_V_WR_OFFSET_CB         45
#define RZ_V_WR_OFFSET_CR         46
#define RZ_V_NEXT_C_Y             47
#define RZ_V_NEXT_C_CB            48
#define RZ_V_NEXT_C_CR            49
#define RZ_H8TAPS_OFT_Y           50
#define RZ_H8TAPS_OFT_CB          51
#define RZ_H8TAPS_OFT_CR          52
#define RZ_V4TAPS_OFT_Y           53
#define RZ_V4TAPS_OFT_CB          54
#define RZ_V4TAPS_OFT_CR          55
#define RZ_SPARE_REG1             56
#define RZ_SPARE_REG2             57
#define RZ_CHK_SUM                58 // read only
#define RZ_MONITOR_INT            59 // read only
#define RZ_MONITOR_FSM            60 // read only
#define RZ_MONITOR_STA            61 // read only
#define RZ_MONITOR_DATA           62 // read only
#define IMG_RESZ_DONE             63 // read only
#define IMG_RESZ_OSD_MODE         64
#define RZ_OSD_CTRL               65
#define RZ_OSD_A_TABLE            66
#define RZ_OSD_COLOR_TRANS        67
#define RZ_OSD_COLOR_TRANS1       68
#define RZ_OSD_COLOR_TRANS2       69
#define RZ_OSD_COLOR_TRANS3       70
#define RZ_OSD_CPT_ADDR           71
#define RZ_OSD_CPT_DATA           72
#define RZ_H_COEF0                73
#define RZ_H_COEF1                74
#define RZ_H_COEF2                75
#define RZ_H_COEF3                76
#define RZ_H_COEF4                77
#define RZ_H_COEF5                78
#define RZ_H_COEF6                79
#define RZ_H_COEF7                80
#define RZ_H_COEF8                81
#define RZ_H_COEF9                82
#define RZ_H_COEF10               83
#define RZ_H_COEF11               84
#define RZ_H_COEF12               85
#define RZ_H_COEF13               86
#define RZ_H_COEF14               87
#define RZ_H_COEF15               88
#define RZ_H_COEF16               89
#define RZ_H_COEF17               90
#define RZ_V_COEF0                91
#define RZ_V_COEF1                92
#define RZ_V_COEF2                93
#define RZ_V_COEF3                94
#define RZ_V_COEF4                95
#define RZ_V_COEF5                96
#define RZ_V_COEF6                97
#define RZ_V_COEF7                98
#define RZ_V_COEF8                99
#define RZ_DITHER                 100
#define RZ_CSC                    101
#define RZ_CSC_COEFF_11           102
#define RZ_CSC_COEFF_12           103
#define RZ_CSC_COEFF_13           104
#define RZ_CSC_COEFF_21           105
#define RZ_CSC_COEFF_22           106
#define RZ_CSC_COEFF_23           107
#define RZ_CSC_COEFF_31           108
#define RZ_CSC_COEFF_32           109
#define RZ_CSC_COEFF_33           110
#define RZ_LUMA_KEY               111                         
#define RZ_COLOR_SEL              112                         
#define RZ_JPG_V_Y                113
#define RZ_JPG_V_CB               114
#define RZ_JPG_V_CR               115
#define RZ_RPR                    116
#define RZ_DRAMQ_CFG              117
#define RZ_DRAMQ_STAD             118
#define RZ_DRAMQ_LEN              119
#define RZ_DRAMQ_RDPTR            120  
#define RZ_DRAMQ_EXPTR            121  // read only  
#define RZ_DUMMY                  122
#define RZ_CMDQDEBUG0             123
#define RZ_CMDQDEBUG1             124
#define RZ_CMDQDEBUG2             125
#define RZ_CMDQDEBUG3             126
#define RZ_ECO                    127





EXTERN void vRprImgResz(
  UINT32 u4InstID,
  UINT32 u4PrevPicDecWidth, UINT32 u4PrevPicDecHeight,
  UINT32 u4PrevPicDispWidth, UINT32 u4PrevPicDispHeight,
  UINT32 u4CurrPicDecWidth, UINT32 u4CurrPicDecHeight,
  UINT32 u4CurrPicDispWidth, UINT32 u4CurrPicDispHeight
);

