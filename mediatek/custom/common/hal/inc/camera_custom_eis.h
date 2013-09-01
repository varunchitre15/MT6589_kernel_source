#ifndef _EIS_CONFIG_H
#define _EIS_CONFIG_H

typedef enum
{
    CUSTOMER_EIS_SENSI_LEVEL_HIGH = 0,
    CUSTOMER_EIS_SENSI_LEVEL_NORMAL = 1,
    CUSTOMER_EIS_SENSI_LEVEL_ADVTUNE = 2
}Customize_EIS_SENSI;

typedef struct
{
    Customize_EIS_SENSI sensitivity;
    unsigned int spring_offset;
    unsigned int spring_gain1;
    unsigned int spring_gain2;

    unsigned int new_trust_threshold;
    unsigned int new_trust_threshold_stnr;

    unsigned int votes_thershold;
    unsigned int min_sad_threshold;
    unsigned int vector_threshold;

}EIS_Customize_PARA_STRUCT;


void get_EIS_CustomizeData(EIS_Customize_PARA_STRUCT *a_pDataOut);
	
#endif /* _EIS_CONFIG_H */

