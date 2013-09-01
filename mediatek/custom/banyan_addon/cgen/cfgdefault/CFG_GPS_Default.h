/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_GPS_Default.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *    give the default GPS config data.
 *
 * Author:
 * -------
 *  Mike Chang(MTK02063) 
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
#ifndef _CFG_GPS_D_H
#define _CFG_GPS_D_H
ap_nvram_gps_config_struct stGPSConfigDefault =
{
    /* "/dev/ttyMT1" */
    {'/','d','e','v','/','t','t','y','M','T','1',0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0},
    /* 0:s/w, 1:none, 2:h/w */
    1,
    
    /* 16.368MHz */
    16368000,
    /* 500ppb */
#if defined MTK_GPS_MT6628
    2000,
#else
    500,
#endif
 
    /* 0:16.368MHz TCXO */
    0,
    
    /* 0:mixer-in, 1:internal-LNA */
    0,
    
    /* 0:none */
    0
};
#endif /* _CFG_GPS_D_H */
 
