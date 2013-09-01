
#ifndef _EIS_CONFIG_H_
#define _EIS_CONFIG_H_

typedef unsigned int MUINT32;

typedef enum
{
    CUSTOMER_EIS_SENSI_LEVEL_HIGH = 0,
    CUSTOMER_EIS_SENSI_LEVEL_NORMAL = 1,
    CUSTOMER_EIS_SENSI_LEVEL_ADVTUNE = 2
}Customize_EIS_SENSI;

typedef enum
{
    ABSOLUTE_HIST,
    SMOOTH_HIST    
} Customize_EIS_VOTE_METHOD_ENUM;


typedef struct
{
    Customize_EIS_SENSI sensitivity;
    MUINT32 filter_small_motion;
    MUINT32 new_tru_th; // 0~100
    MUINT32 vot_th;      // 1~16
    MUINT32 votb_enlarge_size;  // 0~1280
    MUINT32 min_s_th; // 10~100
    MUINT32 vec_th;   // 0~11   should be even
    MUINT32 spr_offset; //0 ~ MarginX/2
    MUINT32 spr_gain1; // 0~127
    MUINT32 spr_gain2; // 0~127
    MUINT32 gmv_pan_array[4];           //0~5
    MUINT32 gmv_sm_array[4];            //0~5
    MUINT32 cmv_pan_array[4];           //0~5
    MUINT32 cmv_sm_array[4];            //0~5
    
    Customize_EIS_VOTE_METHOD_ENUM vot_his_method; //0 or 1
    MUINT32 smooth_his_step; // 2~6
    MUINT32 eis_debug;
}EIS_Customize_Para_t;

void get_EIS_CustomizeData(EIS_Customize_Para_t *a_pDataOut);
	
#endif /* _EIS_CONFIG_H */

