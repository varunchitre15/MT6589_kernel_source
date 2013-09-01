
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
/*
#ifndef __CFG_FILE_INFO_H__
#define __CFG_FILE_INFO_H__

#ifdef __cplusplus
extern "C"
{
#endif
*/
#include "CFG_file_public.h"
#include "libnvram.h"
#include "inc/CFG_file_lid.h"
#include "inc/CFG_module_file.h"
#include "inc/CFG_module_default.h"
#include "inc/CFG_file_info.h"
#include "CFG_file_info_custom.h"
#include <stdio.h>
#define MAX_FILENAMELEN 128;
#define RESERVE_PATH	"Reserved"
#define RESERVE_VER	"000"
const TCFG_FILE g_akCFG_File[]=
{
    //nvram version information
	{ "/data/nvram/APCFG/APRDCL/FILE_VER",		VER(AP_CFG_FILE_VER_INFO_LID), 		4,								
	CFG_FILE_VER_FILE_REC_TOTAL,			DEFAULT_ZERO,						0,  DataReset , NULL },

#ifdef MTK_COMBO_SUPPORT
	{ "/data/nvram/APCFG/APRDEB/BT_Addr",		VER(AP_CFG_RDEB_FILE_BT_ADDR_LID),	CFG_FILE_BT_ADDR_REC_SIZE,		
	CFG_FILE_BT_ADDR_REC_TOTAL,			DEFAULT_ZERO,				0,  DataReset , NULL},
#else
	{ "/data/nvram/APCFG/APRDEB/BT_Addr",		VER(AP_CFG_RDEB_FILE_BT_ADDR_LID),	CFG_FILE_BT_ADDR_REC_SIZE,		
	CFG_FILE_BT_ADDR_REC_TOTAL,			SIGNLE_DEFUALT_REC,			(char *)&stBtDefault,  DataReset , NULL},
#endif
#if 0
	{ "/data/nvram/APCFG/APRDEB/WIFI",	    	VER(AP_CFG_RDEB_FILE_WIFI_LID),		    CFG_FILE_WIFI_REC_SIZE,	
	CFG_FILE_WIFI_REC_TOTAL,		    	SIGNLE_DEFUALT_REC,				    (char *)&stWifiCfgDefault, DataReset , NULL},
#endif
	{ "/data/nvram/APCFG/APRDCL/AUXADC",			VER(AP_CFG_RDCL_FILE_AUXADC_LID),	CFG_FILE_AUXADC_REC_SIZE,		
	CFG_FILE_AUXADC_REC_TOTAL,				SIGNLE_DEFUALT_REC,					(char *)&stADCDefualt, DataReset , NULL},

    { "/data/nvram/APCFG/APRDCL/CAMERA_Para",	VER(AP_CFG_RDCL_CAMERA_PARA_LID),   CFG_FILE_CAMERA_PARA_REC_SIZE,	
	CFG_FILE_CAMERA_PARA_REC_TOTAL,			DEFAULT_ZERO,						0, DataReset , NULL},

    { "/data/nvram/APCFG/APRDCL/CAMERA_3A",	   	VER(AP_CFG_RDCL_CAMERA_3A_LID),     CFG_FILE_CAMERA_3A_REC_SIZE,	
	CFG_FILE_CAMERA_3A_REC_TOTAL,			    DEFAULT_ZERO,						0, DataReset , NULL},

    { "/data/nvram/APCFG/APRDCL/CAMERA_SHADING",	VER(AP_CFG_RDCL_CAMERA_SHADING_LID),CFG_FILE_CAMERA_SHADING_REC_SIZE,	
	CFG_FILE_CAMERA_SHADING_REC_TOTAL,			DEFAULT_ZERO,					    0, DataReset , NULL},

    { "/data/nvram/APCFG/APRDCL/CAMERA_DEFECT",	VER(AP_CFG_RDCL_CAMERA_DEFECT_LID), CFG_FILE_CAMERA_DEFECT_REC_SIZE,	
	CFG_FILE_CAMERA_DEFECT_REC_TOTAL,			DEFAULT_ZERO,					    0, DataReset , NULL},

    { "/data/nvram/APCFG/APRDCL/CAMERA_SENSOR",	VER(AP_CFG_RDCL_CAMERA_SENSOR_LID), CFG_FILE_CAMERA_SENSOR_REC_SIZE,	
	CFG_FILE_CAMERA_SENSOR_REC_TOTAL,			DEFAULT_ZERO,					    0, DataReset , NULL},
	
    { "/data/nvram/APCFG/APRDCL/CAMERA_LENS",	VER(AP_CFG_RDCL_CAMERA_LENS_LID),   CFG_FILE_CAMERA_LENS_REC_SIZE,	
	CFG_FILE_CAMERA_LENS_REC_TOTAL,			        DEFAULT_ZERO,				0, DataReset , NULL},
		
    { "/data/nvram/APCFG/APRDCL/UART",			VER(AP_CFG_RDCL_UART_LID), 			CFG_FILE_UART_CONFIG_SIZE,	
	CFG_FILE_UART_CONFIG_TOTAL,					SIGNLE_DEFUALT_REC,				(char *)&stUARTConfigDefault, DataReset , NULL},

    { "/data/nvram/APCFG/APRDCL/FACTORY",		VER(AP_CFG_RDCL_FACTORY_LID), 		CFG_FILE_FACTORY_REC_SIZE,	
	CFG_FILE_FACTORY_REC_TOTAL,				DEFAULT_ZERO,			    	    0, DataReset , NULL},    
	
    { "/data/nvram/APCFG/APRDCL/BWCS",			VER(AP_CFG_RDCL_BWCS_LID), 	        CFG_FILE_BWCS_CONFIG_SIZE,	
	CFG_FILE_BWCS_CONFIG_TOTAL,				SIGNLE_DEFUALT_REC,					(char *)&stBWCSConfigDefault, DataReset , NULL},	

    { "/data/nvram/APCFG/APRDCL/HWMON_ACC",		VER(AP_CFG_RDCL_HWMON_ACC_LID), 	CFG_FILE_HWMON_ACC_REC_SIZE,	
	CFG_FILE_HWMON_ACC_REC_TOTAL,		    DEFAULT_ZERO,					    0, DataReset , NULL},	
	{ "/data/nvram/APCFG/APRDCL/HWMON_GYRO",	VER(AP_CFG_RDCL_HWMON_GYRO_LID), 	CFG_FILE_HWMON_GYRO_REC_SIZE,	
	CFG_FILE_HWMON_GYRO_REC_TOTAL,		    DEFAULT_ZERO,					    0, DataReset, NULL},
#if 0
    { "/data/nvram/APCFG/APRDEB/WIFI_CUSTOM",	VER(AP_CFG_RDEB_WIFI_CUSTOM_LID),	CFG_FILE_WIFI_CUSTOM_REC_SIZE,	
	CFG_FILE_WIFI_CUSTOM_REC_TOTAL,		    SIGNLE_DEFUALT_REC,				    (char *)&stWifiCustomDefault, DataReset , NULL},
#endif
    { "/data/nvram/APCFG/APRDEB/OMADM_USB",		VER(AP_CFG_RDEB_OMADM_USB_LID),		CFG_FILE_OMADMUSB_REC_SIZE,	
	CFG_FILE_OMADMUSB_REC_TOTAL,		    SIGNLE_DEFUALT_REC,				    (char *)&stOMADMUSBDefualt,DataReset , NULL},
   // { "/nvram/APCFG/APRDCL/ADDED",	VER(AP_CFG_RDCL_ADDED_LID),	CFG_FILE_added_CONFIG_SIZE,	
   //	CFG_FILE_added_CONFIG_TOTAL,		    SIGNLE_DEFUALT_REC,				    (char *)&stAddedDefault},
    { "/data/nvram/APCFG/APRDCL/Voice_Recognize_Param",   VER(AP_CFG_RDCL_FILE_VOICE_RECOGNIZE_PARAM_LID), CFG_FILE_VOICE_RECOGNIZE_PAR_SIZE,
	CFG_FILE_VOICE_RECOGNIZE_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&Voice_Recognize_Par_default, DataReset , NULL},
//Reserved ten item
    { "/data/nvram/APCFG/APRDCL/Audio_AudEnh_Control_Opt",   VER(AP_CFG_RDCL_FILE_AUDIO_AUDENH_CONTROL_OPTION_PAR_LID), CFG_FILE_AUDIO_AUDENH_CONTROL_OPTION_PAR_SIZE,
            CFG_FILE_AUDIO_AUDENH_CONTROL_OPTION_PAR_TOTAL, SIGNLE_DEFUALT_REC  ,    (char *)&AUDENH_Control_Option_Par_default, DataReset , NULL},
    { RESERVE_PATH,	RESERVE_VER,	0,	0,	SIGNLE_DEFUALT_REC,	NULL ,	DataReset , NULL},
    { RESERVE_PATH,	RESERVE_VER,	0,	0,	SIGNLE_DEFUALT_REC,	NULL ,	DataReset , NULL},
    { RESERVE_PATH,	RESERVE_VER,	0,	0,	SIGNLE_DEFUALT_REC,	NULL ,	DataReset , NULL},
    { RESERVE_PATH,	RESERVE_VER,	0,	0,	SIGNLE_DEFUALT_REC,	NULL ,	DataReset , NULL},
    { RESERVE_PATH,	RESERVE_VER,	0,	0,	SIGNLE_DEFUALT_REC,	NULL ,	DataReset , NULL},
    { RESERVE_PATH,	RESERVE_VER,	0,	0,	SIGNLE_DEFUALT_REC,	NULL ,	DataReset , NULL},
    { RESERVE_PATH,	RESERVE_VER,	0,	0,	SIGNLE_DEFUALT_REC,	NULL ,	DataReset , NULL},
    { RESERVE_PATH,	RESERVE_VER,	0,	0,	SIGNLE_DEFUALT_REC,	NULL ,	DataReset , NULL}, 
//Reserved   
};
int iCustomBeginLID=AP_CFG_CUSTOM_BEGIN_LID;
extern int iCustomBeginLID;
int iFileVerInfoLID=AP_CFG_FILE_VER_INFO_LID;
extern int iFileVerInfoLID;
int iFileBTAddrLID=AP_CFG_RDEB_FILE_BT_ADDR_LID;
extern int iFileBTAddrLID;
int iFileAuxADCLID=AP_CFG_RDCL_FILE_AUXADC_LID;
extern int iFileAuxADCLID;
int iFileOMADMUSBLID=AP_CFG_RDEB_OMADM_USB_LID;
extern int iFileOMADMUSBLID;
const unsigned int g_i4CFG_File_Count = sizeof(g_akCFG_File)/sizeof(TCFG_FILE);

extern const TCFG_FILE g_akCFG_File[];

extern const unsigned int g_i4CFG_File_Count;
typedef struct
{
	char	cFileName[128];
	unsigned int iLID;
}FileName;

FileName aBackupToBinRegion[]=
{
	{"FILE_VER",AP_CFG_FILE_VER_INFO_LID},
	{"BT_Addr",AP_CFG_RDEB_FILE_BT_ADDR_LID},
	{"WIFI",AP_CFG_RDEB_FILE_WIFI_LID},
	{"AUXADC",AP_CFG_RDCL_FILE_AUXADC_LID},
	{"FACTORY",AP_CFG_RDCL_FACTORY_LID},
	{"HWMON_ACC",AP_CFG_RDCL_HWMON_ACC_LID},
	{"HWMON_GYRO",AP_CFG_RDCL_HWMON_GYRO_LID},
	{"WIFI_CUSTOM",AP_CFG_RDEB_WIFI_CUSTOM_LID},
#ifndef MTK_PRODUCT_INFO_SUPPORT
	{"PRODUCT_INFO",AP_CFG_REEB_PRODUCT_INFO_LID},
#endif
#ifdef	MTK_SDIORETRY_SUPPORT
	{"SDIO_RETRY",AP_CFG_RDEB_SDIO_RETRY_LID},
#endif
#ifdef	MTK_MT8193_HDCP_SUPPORT
    {"Hdcp_Key_Table", AP_CFG_RDCL_FILE_HDCP_KEY_LID},
#endif
};
FileName aPerformance[]=
{
	{"CAMERA_Para",AP_CFG_RDCL_CAMERA_PARA_LID},
	{"CAMERA_3A",AP_CFG_RDCL_CAMERA_3A_LID},
	{"CAMERA_SHADING",AP_CFG_RDCL_CAMERA_SHADING_LID},
	{"CAMERA_DEFECT",AP_CFG_RDCL_CAMERA_DEFECT_LID},
	{"CAMERA_SENSOR",AP_CFG_RDCL_CAMERA_SENSOR_LID},
	{"CAMERA_LENS",AP_CFG_RDCL_CAMERA_LENS_LID}
};
extern FileName aBackupToBinRegion[];
extern FileName aPerformance[];
const unsigned int g_Backup_File_Count = sizeof(aBackupToBinRegion)/(sizeof(FileName));
const unsigned int g_Performance_File_Count = sizeof(aPerformance)/(sizeof(FileName));

extern const unsigned int g_Backup_File_Count;
extern const unsigned int g_Performance_File_Count;
pfConvertFunc aNvRamConvertFuncTable[]=
{
  NULL,//AP_CFG_FILE_VER_INFO_LID
  NULL,//AP_CFG_RDEB_FILE_BT_ADDR_LID
  NULL,//AP_CFG_RDCL_FILE_AUXADC_LID
  NULL,//AP_CFG_RDCL_CAMERA_PARA_LID
  NULL,//AP_CFG_RDCL_CAMERA_3A_LID
  NULL,//AP_CFG_RDCL_CAMERA_SHADING_LID
  NULL,//AP_CFG_RDCL_CAMERA_DEFECT_LID
  NULL,//AP_CFG_RDCL_CAMERA_SENSOR_LID
  NULL,//AP_CFG_RDCL_CAMERA_LENS_LID
  NULL,//AP_CFG_RDCL_UART_LID
  NULL,//AP_CFG_RDCL_FACTORY_LID
  NULL,//AP_CFG_RDCL_BWCS_LID
  NULL,//AP_CFG_RDCL_HWMON_ACC_LID
  NULL,//AP_CFG_RDCL_HWMON_GYRO_LID
  NULL,//AP_CFG_RDCL_FILE_AUDIO_LID
  NULL,//AP_CFG_RDCL_FILE_AUDIO_COMPFLT_LID
  NULL,//AP_CFG_RDCL_FILE_AUDIO_EFFECT_LID
  NULL,//AP_CFG_CUSTOM_FILE_GPS_LID
  NULL,//AP_CFG_RDEB_FILE_WIFI_LID
  NULL,//AP_CFG_RDEB_WIFI_CUSTOM_LID
  NULL,//AP_CFG_RDEB_OMADM_USB_LID
  NULL,//AP_CFG_REEB_PRODUCT_INFO_LID
#ifdef	MTK_SDIORETRY_SUPPORT
  NULL,//AP_CFG_REEB_SDIO_RETRY_LID
#endif
};
extern pfConvertFunc aNvRamConvertFuncTable[];

//Add for new nvram partition feature
pfCallbackForDaemon callback_for_nvram_daemon = NULL;
extern pfCallbackForDaemon callback_for_nvram_daemon;

#ifdef MTK_PRODUCT_INFO_SUPPORT
extern bool nvram_new_partition_support()
{
	return true;
}
const TABLE_FOR_SPECIAL_LID g_new_nvram_lid[] = 
{
	{ AP_CFG_REEB_PRODUCT_INFO_LID, 0, 1024 * 1024 },
};
const unsigned int g_new_nvram_lid_count = sizeof(g_new_nvram_lid)/sizeof(TABLE_FOR_SPECIAL_LID);
const char *nvram_new_partition_name = "/dev/pro_info";
extern const char *nvram_new_partition_name;
extern const TABLE_FOR_SPECIAL_LID g_new_nvram_lid[];
extern const unsigned int g_new_nvram_lid_count;
#else
extern bool nvram_new_partition_support()
{
	return false;
}
const TABLE_FOR_SPECIAL_LID g_new_nvram_lid[] = {0 , 0, 0}; 
const unsigned int g_new_nvram_lid_count = 0;
const char *nvram_new_partition_name = NULL;
extern const char *nvram_new_partition_name;
extern const TABLE_FOR_SPECIAL_LID g_new_nvram_lid[];
extern const unsigned int g_new_nvram_lid_count;
#endif
//end new nvram partition feature
/*
#ifdef __cplusplus
}
#endif

#endif
*/
