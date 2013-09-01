


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   cfg_bwcs_file.h
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
 *   Saker Hsia(MTK02327)
 *
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 06 30 2010 saker.hsia
 * [ALPS00002764][Need Patch] [Volunteer Patch] ALPS.10X.W10.28 Volunteer patch for BWCS NVRAM customization 
 * .
 *
 * 05 05 2010 saker.hsia
 * [ALPS00001913][BWCS] BT WiFi Single Antenna Feature check in Android 2.1 
 * Check in BT/WiFi Single Antenna Feature
 *******************************************************************************/



#ifndef _CFG_BWCS_FILE_H
#define _CFG_BWCS_FILE_H


// the record structure define of bt nvram file
typedef struct
{
    unsigned int rt_rssi_th[3];
    unsigned int nrt_rssi_th[3];
    unsigned int ant_path_comp;
    unsigned int ant_switch_prot_time;
    unsigned int wifi_tx_flow[2];
    unsigned int bt_rx_range[2];
    unsigned int bt_tx_power[3];
    unsigned int reserved[5];
} ap_nvram_bwcs_config_struct;


//the record size and number of bt nvram file
#define CFG_FILE_BWCS_CONFIG_SIZE    sizeof(ap_nvram_bwcs_config_struct)
#define CFG_FILE_BWCS_CONFIG_TOTAL   1

#endif /* _CFG_BWCS_FILE_H */
