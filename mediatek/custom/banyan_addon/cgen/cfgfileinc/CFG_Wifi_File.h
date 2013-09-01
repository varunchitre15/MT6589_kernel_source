/*******************************************************************************
 *
 * Filename:
 * ---------
 *   cfg_wifi_file.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *    header file of main function
 *
 * Author:
 * -------
 *   Ning.F (MTK08139) 09/11/2008
 *
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * 09 22 2011 cp.wu
 * [ALPS00070736] MT6573W Wifi wƻݨD
 * add fields for Wi-Fi regularity domain control.
 *
 * 05 27 2011 cp.wu
 * [ALPS00050349] [Need Patch] [Volunteer Patch][MT6620 Wi-Fi][Driver] Add band edge tx power control to Wi-Fi NVRAM
 * change size of reserved fields.
 *
 * 05 26 2011 cp.wu
 * [ALPS00050349] [Need Patch] [Volunteer Patch][MT6620 Wi-Fi][Driver] Add band edge tx power control to Wi-Fi NVRAM
 * update Wi-Fi NVRAM definition for band edge tx power control.
 *
 * 04 19 2011 cp.wu
 * [ALPS00041285] [Need Patch] [Volunteer Patch][MT6620 Wi-Fi] Merge MT6620 Wi-Fi into mt6575_evb project
 * 1. update init.rc for normal boot/meta/factory for MT6620 Wi-Fi related part.
 * 2. update NVRAM structure definition and default value for MT6620 Wi-Fi
 *
 * 11 05 2010 renbang.jiang
 * [ALPS00134025] [Wi-Fi] move Wi-Fi NVRAM definition source file to project folder from common folder
 * .
 *
 * 11 05 2010 renbang.jiang
 * [ALPS00134025] [Wi-Fi] move Wi-Fi NVRAM definition source file to project folder from common folder
 * .
 *
 * 07 10 2010 renbang.jiang
 * [ALPS00121785][Need Patch] [Volunteer Patch] use NVRAM to save Wi-Fi custom data 
 * .
 *
 * Jul 9 2009 mtk80306
 * [DUMA00122953] optimize nvram and change meta clean boot flag.
 * modify wifi str
 *
 * Mar 21 2009 mtk80306
 * [DUMA00112158] fix the code convention.
 * fix the codeing convention.
 *
 * Mar 9 2009 mtk80306
 * [DUMA00111088] nvram customization
 * change wifi cmd structure.
 *
 * Dec 17 2008 mbj08139
 * [DUMA00105099] create meta code
 *
 *
 * Dec 8 2008 mbj08139
 * [DUMA00105099] create meta code
 *
 *
 * Nov 24 2008 mbj08139
 * [DUMA00105099] create meta code
 *
 *
 * Oct 29 2008 mbj08139
 * [DUMA00105099] create meta code
 *
 *
 *
 *
 *******************************************************************************/



#ifndef _CFG_WIFI_FILE_H
#define _CFG_WIFI_FILE_H

// the record structure define of wifi nvram file
/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                    E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/

/*******************************************************************************
*                              C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                             D A T A   T Y P E S
********************************************************************************
*/
typedef signed char             INT_8, *PINT_8, **PPINT_8;
typedef unsigned char           UINT_8, *PUINT_8, **PPUINT_8, *P_UINT_8;
typedef unsigned short          UINT_16, *PUINT_16, **PPUINT_16;
typedef unsigned long           ULONG, UINT_32, *PUINT_32, **PPUINT_32;


/*******************************************************************************
*                            P U B L I C   D A T A
********************************************************************************
*/
// duplicated from nic_cmd_event.h to avoid header dependency
typedef struct _TX_PWR_PARAM_T {
    INT_8       cTxPwr2G4Cck;		/* signed, in unit of 0.5dBm */
#if defined(MT6628) || defined(MT5931)
    INT_8       cTxPwr2G4Dsss;      /* signed, in unit of 0.5dBm */
    INT_8       acReserved[2];
#else /*For MT6620*/
    INT_8       acReserved[3];
#endif

    INT_8       cTxPwr2G4OFDM_BPSK;
    INT_8       cTxPwr2G4OFDM_QPSK;
    INT_8       cTxPwr2G4OFDM_16QAM;
    INT_8       cTxPwr2G4OFDM_Reserved;
    INT_8       cTxPwr2G4OFDM_48Mbps;
    INT_8       cTxPwr2G4OFDM_54Mbps;

    INT_8       cTxPwr2G4HT20_BPSK;
    INT_8       cTxPwr2G4HT20_QPSK;
    INT_8       cTxPwr2G4HT20_16QAM;
    INT_8       cTxPwr2G4HT20_MCS5;
    INT_8       cTxPwr2G4HT20_MCS6;
    INT_8       cTxPwr2G4HT20_MCS7;

    INT_8       cTxPwr2G4HT40_BPSK;
    INT_8       cTxPwr2G4HT40_QPSK;
    INT_8       cTxPwr2G4HT40_16QAM;
    INT_8       cTxPwr2G4HT40_MCS5;
    INT_8       cTxPwr2G4HT40_MCS6;
    INT_8       cTxPwr2G4HT40_MCS7;

    INT_8       cTxPwr5GOFDM_BPSK;
    INT_8       cTxPwr5GOFDM_QPSK;
    INT_8       cTxPwr5GOFDM_16QAM;
    INT_8       cTxPwr5GOFDM_Reserved;
    INT_8       cTxPwr5GOFDM_48Mbps;
    INT_8       cTxPwr5GOFDM_54Mbps;

    INT_8       cTxPwr5GHT20_BPSK;
    INT_8       cTxPwr5GHT20_QPSK;
    INT_8       cTxPwr5GHT20_16QAM;
    INT_8       cTxPwr5GHT20_MCS5;
    INT_8       cTxPwr5GHT20_MCS6;
    INT_8       cTxPwr5GHT20_MCS7;

    INT_8       cTxPwr5GHT40_BPSK;
    INT_8       cTxPwr5GHT40_QPSK;
    INT_8       cTxPwr5GHT40_16QAM;
    INT_8       cTxPwr5GHT40_MCS5;
    INT_8       cTxPwr5GHT40_MCS6;
    INT_8       cTxPwr5GHT40_MCS7;
} TX_PWR_PARAM_T, *P_TX_PWR_PARAM_T;

typedef struct _PWR_5G_OFFSET_T {
    INT_8       cOffsetBand0;       /* 4.915-4.980G */
    INT_8       cOffsetBand1;       /* 5.000-5.080G */
    INT_8       cOffsetBand2;       /* 5.160-5.180G */
    INT_8       cOffsetBand3;       /* 5.200-5.280G */
    INT_8       cOffsetBand4;       /* 5.300-5.340G */
    INT_8       cOffsetBand5;       /* 5.500-5.580G */
    INT_8       cOffsetBand6;       /* 5.600-5.680G */
    INT_8       cOffsetBand7;       /* 5.700-5.825G */
} PWR_5G_OFFSET_T, *P_PWR_5G_OFFSET_T;

typedef struct _PWR_PARAM_T {
    UINT_32     au4Data[28];
    UINT_32     u4RefValue1;
    UINT_32     u4RefValue2;
} PWR_PARAM_T, *P_PWR_PARAM_T;

typedef struct _MT6620_CFG_PARAM_STRUCT {
    /* 256 bytes of MP data */
    UINT_16             u2Part1OwnVersion;
    UINT_16             u2Part1PeerVersion;
    UINT_8              aucMacAddress[6];
    UINT_8              aucCountryCode[2];
    TX_PWR_PARAM_T      rTxPwr;
    UINT_8              aucEFUSE[144];
    UINT_8              ucTxPwrValid;
    UINT_8              ucSupport5GBand;
    UINT_8              fg2G4BandEdgePwrUsed;
    INT_8               cBandEdgeMaxPwrCCK;
    INT_8               cBandEdgeMaxPwrOFDM20; 
    INT_8               cBandEdgeMaxPwrOFDM40;

    UINT_8              ucRegChannelListMap;
    UINT_8              ucRegChannelListIndex;
    UINT_8              aucRegSubbandInfo[36];

    UINT_8              aucReserved2[256-240];

    /* 256 bytes of function data */
    UINT_16             u2Part2OwnVersion;
    UINT_16             u2Part2PeerVersion;
    UINT_8              uc2G4BwFixed20M;
    UINT_8              uc5GBwFixed20M;
    UINT_8              ucEnable5GBand;
    UINT_8              aucPreTailReserved;
    UINT_8              aucTailReserved[256-8];
} MT6620_CFG_PARAM_STRUCT, *P_MT6620_CFG_PARAM_STRUCT, 
    WIFI_CFG_PARAM_STRUCT, *P_WIFI_CFG_PARAM_STRUCT;

typedef struct _WIFI_CUSTOM_PARAM_STRUCT
{
    UINT_32             u4Resv;         /* Reserved */
} WIFI_CUSTOM_PARAM_STRUCT;



/*******************************************************************************
*                           P R I V A T E   D A T A
********************************************************************************
*/

/*******************************************************************************
*                                 M A C R O S
********************************************************************************
*/
#define CFG_FILE_WIFI_REC_SIZE           sizeof(WIFI_CFG_PARAM_STRUCT)
#define CFG_FILE_WIFI_REC_TOTAL		       1

#define CFG_FILE_WIFI_CUSTOM_REC_SIZE    sizeof(WIFI_CUSTOM_PARAM_STRUCT)
#define CFG_FILE_WIFI_CUSTOM_REC_TOTAL   1

/*******************************************************************************
*                  F U N C T I O N   D E C L A R A T I O N S
********************************************************************************
*/
	
/*******************************************************************************
*                              F U N C T I O N S
********************************************************************************
*/


#endif


