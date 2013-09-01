/*******************************************************************************
 *
 * Filename:
 * ---------
 *   cfg_gps_file.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *    header file of main function
 *
 * Author:
 * -------
 *   Mike Chang(MTK02063)
 *
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 06 24 2010 yunchang.chang
 * [ALPS00002677][Need Patch] [Volunteer Patch] ALPS.10X.W10.26 Volunteer patch for GPS customization use NVRam 
 * .
 *
 *******************************************************************************/



#ifndef _CFG_GPS_FILE_H
#define _CFG_GPS_FILE_H


// the record structure define of bt nvram file
typedef struct
{
    char dsp_dev[20];
    unsigned char gps_if_type;
    
    unsigned int gps_tcxo_hz;
    unsigned int gps_tcxo_ppb;
    unsigned char gps_tcxo_type;
    
    unsigned char gps_lna_mode;
    
    unsigned char gps_sbas_mode;
} ap_nvram_gps_config_struct;


//the record size and number of bt nvram file
#define CFG_FILE_GPS_CONFIG_SIZE    sizeof(ap_nvram_gps_config_struct)
#define CFG_FILE_GPS_CONFIG_TOTAL   1

#endif /* _CFG_GPS_FILE_H */
