


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_BWCS_Default.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *    give the default BT/WiFi co-existence config data.
 *
 * Author:
 * -------
 *  Saker Hsia(MTK02327) 
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
 * 05 16 2010 saker.hsia
 * [ALPS00001913][BWCS] BT WiFi Single Antenna Feature check in Android 2.1 
 * .
 *
 * 05 14 2010 saker.hsia
 * [ALPS00001913][BWCS] BT WiFi Single Antenna Feature check in Android 2.1 
 * .
 *
 * 05 05 2010 saker.hsia
 * [ALPS00001913][BWCS] BT WiFi Single Antenna Feature check in Android 2.1 
 * Check in BT/WiFi Single Antenna Feature
 *
 *******************************************************************************/
#ifndef _CFG_BWCS_D_H
#define _CFG_BWCS_D_H
#include "../inc/bwcs_custom.h"
ap_nvram_bwcs_config_struct stBWCSConfigDefault =
{
    /* Real Time RSSI Threshold : BT / WIFI1 / WIFI2 */
    {RT_RSSI_TH_BT, RT_RSSI_TH_WIFI1, RT_RSSI_TH_WIFI2}
    ,
    /* Non Real Time RSSI Threshold : BT / WIFI1 / WIFI2 */
    {NRT_RSSI_TH_BT, NRT_RSSI_TH_WIFI1, NRT_RSSI_TH_WIFI2}
    ,
    /* Antenna path compensation */
    ANT_PATH_COMP
    ,
    /* antenna path switch protection period, Unit is seconds */
    ANT_SWITCH_PROT_TIME
    ,
    /* TX Flow control : medium time / period time */
    WIFI_TX_FLOW_CTRL
    ,
    /* BT RX Range : Low / High */
    BT_RX_RANGE
    ,
    /* BT TX power : WIFI OFF / WIFI SCO / WIFI ACL */
    {BT_TX_PWR_WIFI_OFF, BT_TX_PWR_SCO, BT_TX_PWR_ACL}
    ,
    /* Reserved                     ,  5 */
    RESERVED
};
#endif /* _CFG_BWCS_D_H */
