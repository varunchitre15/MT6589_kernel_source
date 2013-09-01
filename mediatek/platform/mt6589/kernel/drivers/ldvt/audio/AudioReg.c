#include "AudioAfe.h"

/******************************************************************************
* Function      : vWriteREGBits
* Description   : Write value[(len - 1):0] to (*addr)[(bits+len-1):bits]
* Parameter     :
* Return        : None
******************************************************************************/
void vRegWriteBits(INT32 addr,INT32 value,INT8 bits,INT8 len)
{
    UINT32 u4TargetBitField = ((0x1 << len) - 1) << bits;
    UINT32 u4TargetValue = (value << bits) & u4TargetBitField;
    UINT32 u4CurrValue;
    u4CurrValue = ReadREG(addr);
    WriteREG(addr, ((u4CurrValue & (~u4TargetBitField)) | u4TargetValue));
    return;
}


/******************************************************************************
* Function      : vWriteREGBits
* Description   : Read (*addr)[(bits+len-1):bits] save to value[(len - 1):0]
* Parameter     :
* Return        : Return register value
******************************************************************************/
UINT32 u4RegReadBits(INT32 addr,INT8 bits,INT8 len)
{
    UINT32 u4TargetBitField = ((0x1 << len) - 1) << bits;
    UINT32 u4CurrValue = ReadREG(addr);
    return (u4CurrValue & u4TargetBitField)>>bits;
}


/******************************************************************************
* Function      : vSetRegBit
* Description   : Set one bit
* Parameter     :
* Return        : None
******************************************************************************/
void vRegSetBit(INT32 addr,INT32 bit)
{
    UINT32 u4CurrValue,u4Mask;
    u4Mask = 1<<bit;
    u4CurrValue = ReadREG(addr);
    WriteREG(addr, ((u4CurrValue & (~u4Mask)) | u4Mask));
    return;
}


/******************************************************************************
* Function      : vResetRegBit
* Description   : Reset one bit
* Parameter     :
* Return        : None
******************************************************************************/
void vRegResetBit(INT32 addr,INT32 bit)
{
    UINT32 u4CurrValue,u4Mask;
    u4Mask = 1<<bit;
    u4CurrValue = ReadREG(addr);
    WriteREG(addr, (u4CurrValue & (~u4Mask)));
    return;
}


