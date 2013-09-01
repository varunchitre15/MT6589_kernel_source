

#include "camera_custom_eis.h"

void get_EIS_CustomizeData(EIS_Customize_Para_t *a_pDataOut)
{    
    a_pDataOut->sensitivity   = CUSTOMER_EIS_SENSI_LEVEL_ADVTUNE;
    a_pDataOut->filter_small_motion = 0;
    a_pDataOut->new_tru_th = 25; // 0~100
    a_pDataOut->vot_th = 4;      // 1~16
    a_pDataOut->votb_enlarge_size = 0;  // 0~1280
    a_pDataOut->min_s_th = 40; // 10~100
    a_pDataOut->vec_th = 0;   // 0~11   should be even
    a_pDataOut->spr_offset = 0;//90; //0 ~ MarginX/2
    a_pDataOut->spr_gain1 = 0;//10; // 0~127
    a_pDataOut->spr_gain2 = 0; // 0~127
    a_pDataOut->gmv_pan_array[0] = 0;//0;   //0~5
    a_pDataOut->gmv_pan_array[1] = 0;//1;   //0~5
    a_pDataOut->gmv_pan_array[2] = 0;//2;   //0~5
    a_pDataOut->gmv_pan_array[3] = 1;//5;   //0~5
    
    a_pDataOut->gmv_sm_array[0] = 0;    //0~5
    a_pDataOut->gmv_sm_array[1] = 0;    //0~5
    a_pDataOut->gmv_sm_array[2] = 0;//1;    //0~5
    a_pDataOut->gmv_sm_array[3] = 1;//3;    //0~5

    a_pDataOut->cmv_pan_array[0] = 0;   //0~5
    a_pDataOut->cmv_pan_array[1] = 0;//1;   //0~5
    a_pDataOut->cmv_pan_array[2] = 0;//1;   //0~5
    a_pDataOut->cmv_pan_array[3] = 1;//6;   //0~5
    
    a_pDataOut->cmv_sm_array[0] = 0;    //0~5
    a_pDataOut->cmv_sm_array[1] = 1;    //0~5
    a_pDataOut->cmv_sm_array[2] = 2;    //0~5
    a_pDataOut->cmv_sm_array[3] = 4;    //0~5
    
    a_pDataOut->vot_his_method = ABSOLUTE_HIST; //0 or 1   //SMOOTH_HIST
    a_pDataOut->smooth_his_step = 3; // 2~6
    a_pDataOut->eis_debug = 0;
}

