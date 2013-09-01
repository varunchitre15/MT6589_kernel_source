

#ifndef _CFG_BT_FILE_H
#define _CFG_BT_FILE_H


// the record structure define of bt nvram file
#ifdef MTK_COMBO_SUPPORT
typedef struct
{
    unsigned char addr[6];            // BT address
    unsigned char Voice[2];           // Voice setting for SCO connection
    unsigned char Codec[4];           // PCM codec setting
    unsigned char Radio[6];           // RF configuration
    unsigned char Sleep[7];           // Sleep mode configuration
    unsigned char BtFTR[2];           // Other feature setting
    unsigned char TxPWOffset[3];      // TX power channel offset compensation
} ap_nvram_btradio_mt6610_struct;
#else
// MT661x is phased out, current for MT662x
typedef struct
{
    unsigned char addr[6];            // BT address
    unsigned char CapId[1];           // Calibration setting
    unsigned char LinkKeyType[1];     // Link key type
    unsigned char UintKey[16];        // Unit key
    unsigned char Encryption[3];      // Encryption configuration
    unsigned char PinCodeType[1];     // Pin code type
    unsigned char Voice[2];           // Voice setting for SCO connection
    unsigned char Codec[4];           // PCM codec setting
    unsigned char Radio[6];           // RF configuration
    unsigned char Sleep[7];           // Sleep mode configuration
    unsigned char BtFTR[2];           // Other feature setting
    unsigned char ECLK_SEL[1];        // External clk select
    unsigned char Reserved1[1];       // Reserved
    unsigned char Reserved2[2];       // Reserved
    unsigned char Reserved3[4];       // Reserved
    unsigned char Reserved4[16];      // Reserved
} ap_nvram_btradio_mt6610_struct;
#endif

//the record size and number of bt nvram file
#define CFG_FILE_BT_ADDR_REC_SIZE    sizeof(ap_nvram_btradio_mt6610_struct)
#define CFG_FILE_BT_ADDR_REC_TOTAL   1

#endif


