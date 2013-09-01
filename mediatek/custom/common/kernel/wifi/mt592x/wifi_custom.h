
/*******************************************************************************
 *
 * Filename:
 * ---------
 * wifi_custom.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 * This file is the header of wifi customization related function or definition.
 *
 * Author:
 * -------
 * Renbang
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 06 18 2010 renbang.jiang
 * [ALPS00008357][Need Patch] [Volunteer Patch] ALPS.10X.W10.11 Volunteer patch for support Rx/Tx indication led 
 * .
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#ifndef __WIFI_CUSTOM_H
#define __WIFI_CUSTOM_H

#define  BWCS_SINGLE_ANT            1
#define  COMODULE_DAISY_CHAIN       1

#define MT5921_DEFAULT_LED_SETTING  0   /* 0:NONE  1:TX  2:RX  3:TX_RX */
#define MT5921_DEFAULT_LED_ON_TIME  80  /* ms, range : 0 ~ 1020 */
#define MT5921_DEFAULT_LED_OFF_TIME 24  /* ms, range : 0 ~ 1020 */

#endif
