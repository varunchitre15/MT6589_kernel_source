
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_file_info.c
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *   Configuration File List
 *
 *
 * Author:
 * -------
 *   Nick Huang (mtk02183)
 *
 ****************************************************************************/

#ifndef __CFG_FILE_INFO_H__
#define __CFG_FILE_INFO_H__

#ifdef __cplusplus
extern "C"
{
#endif
/*
#include "CFG_file_public.h"
#include "CFG_file_lid.h"
#include "CFG_module_file.h"
#include "CFG_module_default.h"

const TCFG_FILE g_akCFG_File[]=
{
    //nvram version information
	{ "/nvram/APCFG/APRDCL/FILE_VER",		VER(AP_CFG_FILE_VER_INFO_LID), 		4,								
	CFG_FILE_VER_FILE_REC_TOTAL,			DEFAULT_ZERO,						0},

	{ "/nvram/APCFG/APRDEB/BT_Addr",		VER(AP_CFG_RDEB_FILE_BT_ADDR_LID),	CFG_FILE_BT_ADDR_REC_SIZE,		
	CFG_FILE_BT_ADDR_REC_TOTAL,				SIGNLE_DEFUALT_REC,					(char *)&stBtDefault},
	
	{ "/nvram/APCFG/APRDEB/WIFI",	    	VER(AP_CFG_RDEB_FILE_WIFI_LID),		    CFG_FILE_WIFI_REC_SIZE,	
	CFG_FILE_WIFI_REC_TOTAL,		    	SIGNLE_DEFUALT_REC,				    (char *)&stWifiCfgDefault},
	
	{ "/nvram/APCFG/APRDCL/AUXADC",			VER(AP_CFG_RDCL_FILE_AUXADC_LID),	CFG_FILE_AUXADC_REC_SIZE,		
	CFG_FILE_AUXADC_REC_TOTAL,				SIGNLE_DEFUALT_REC,					(char *)&stADCDefualt},

    { "/nvram/APCFG/APRDCL/CAMERA_Para",	VER(AP_CFG_RDCL_CAMERA_PARA_LID),   CFG_FILE_CAMERA_PARA_REC_SIZE,	
	CFG_FILE_CAMERA_PARA_REC_TOTAL,			DEFAULT_ZERO,						0},

    { "/nvram/APCFG/APRDCL/CAMERA_3A",	   	VER(AP_CFG_RDCL_CAMERA_3A_LID),     CFG_FILE_CAMERA_3A_REC_SIZE,	
	CFG_FILE_CAMERA_3A_REC_TOTAL,			    DEFAULT_ZERO,						0},

    { "/nvram/APCFG/APRDCL/CAMERA_SHADING",	VER(AP_CFG_RDCL_CAMERA_SHADING_LID),CFG_FILE_CAMERA_SHADING_REC_SIZE,	
	CFG_FILE_CAMERA_SHADING_REC_TOTAL,			DEFAULT_ZERO,					    0},

    { "/nvram/APCFG/APRDCL/CAMERA_DEFECT",	VER(AP_CFG_RDCL_CAMERA_DEFECT_LID), CFG_FILE_CAMERA_DEFECT_REC_SIZE,	
	CFG_FILE_CAMERA_DEFECT_REC_TOTAL,			DEFAULT_ZERO,					    0},

    { "/nvram/APCFG/APRDCL/CAMERA_SENSOR",	VER(AP_CFG_RDCL_CAMERA_SENSOR_LID), CFG_FILE_CAMERA_SENSOR_REC_SIZE,	
	CFG_FILE_CAMERA_SENSOR_REC_TOTAL,			DEFAULT_ZERO,					    0},
	
    { "/nvram/APCFG/APRDCL/CAMERA_LENS",	VER(AP_CFG_RDCL_CAMERA_LENS_LID),   CFG_FILE_CAMERA_LENS_REC_SIZE,	
	CFG_FILE_CAMERA_LENS_REC_TOTAL,			        DEFAULT_ZERO,				0},
		
    { "/nvram/APCFG/APRDCL/UART",			VER(AP_CFG_RDCL_UART_LID), 			CFG_FILE_UART_CONFIG_SIZE,	
	CFG_FILE_UART_CONFIG_TOTAL,					SIGNLE_DEFUALT_REC,				(char *)&stUARTConfigDefault},

    { "/nvram/APCFG/APRDCL/FACTORY",		VER(AP_CFG_RDCL_FACTORY_LID), 		CFG_FILE_FACTORY_REC_SIZE,	
	CFG_FILE_FACTORY_REC_TOTAL,				DEFAULT_ZERO,			    	    0},    
	
    { "/nvram/APCFG/APRDCL/BWCS",			VER(AP_CFG_RDCL_BWCS_LID), 	        CFG_FILE_BWCS_CONFIG_SIZE,	
	CFG_FILE_BWCS_CONFIG_TOTAL,				SIGNLE_DEFUALT_REC,					(char *)&stBWCSConfigDefault},	

    { "/nvram/APCFG/APRDCL/HWMON_ACC",		VER(AP_CFG_RDCL_HWMON_ACC_LID), 	CFG_FILE_HWMON_ACC_REC_SIZE,	
	CFG_FILE_HWMON_ACC_REC_TOTAL,		    DEFAULT_ZERO,					    0},	
	
    { "/nvram/APCFG/APRDEB/WIFI_CUSTOM",	VER(AP_CFG_RDEB_WIFI_CUSTOM_LID),	CFG_FILE_WIFI_CUSTOM_REC_SIZE,	
	CFG_FILE_WIFI_CUSTOM_REC_TOTAL,		    SIGNLE_DEFUALT_REC,				    (char *)&stWifiCustomDefault},
};
const int g_i4CFG_File_Count = sizeof(g_akCFG_File)/sizeof(TCFG_FILE);

extern const TCFG_FILE g_akCFG_File[];

extern const int g_i4CFG_File_Count;
*/
const TCFG_FILE g_akCFG_File[];
#ifdef __cplusplus
}
#endif

#endif
