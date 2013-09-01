#ifndef _MTK_ADC_SW_H
#define _MTK_ADC_SW_H

//-----------------------------------------------------------------------------
//  IMM and TRI modes
typedef enum 
{
    NotLegalMode = -1,
    ModeNON   = 0x00,
    ModeIMM   = 0x01, //Immediate mode
    ModeTRI   = 0x10, //Time-trigger mode
    ModeChaos = 0x11
} AdcModeType;

typedef enum
{
    ADC_IDLE = 0,
    ADC_BUSY = 1,
} ADC_STATUS;

#endif   /*_MTK_ADC_SW_H*/

