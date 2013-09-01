#include "camera_custom_eis.h"

void get_EIS_CustomizeData(EIS_Customize_PARA_STRUCT *a_pDataOut)
{    
    a_pDataOut->sensitivity   = CUSTOMER_EIS_SENSI_LEVEL_NORMAL;
    a_pDataOut->spring_offset = 0;
    a_pDataOut->spring_gain1  = 0;
    a_pDataOut->spring_gain2  = 0;
    a_pDataOut->new_trust_threshold = 0;
    a_pDataOut->new_trust_threshold_stnr = 0;
    a_pDataOut->votes_thershold   = 0;
    a_pDataOut->min_sad_threshold = 0;
    a_pDataOut->vector_threshold  = 0;
}


