
#ifndef _VDEC_HAL_IF_VP8_H_
#define _VDEC_HAL_IF_VP8_H_

#include "../include/vdec_info_vp8.h"
#include "../include/vdec_info_common.h"

/*! \name Video Decoder HAL VP8 Interface
* @{
*/

/// Initialize video decoder hardware
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_VP8_InitVDecHW(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_VP8_VFIFO_PRM_T *prVp8VFifoInitPrm      ///< [IN] Pointer to video fifo address information
);

/// Reset Vp8 HW
/// \return value of reset ok.
BOOL fgVDEC_Vp8DecReset(
	UINT32 u4InstID, 
	VDEC_INFO_VP8_FRM_HDR_T *pVp8DecInfo,
	BOOL fgInit);

void vVDEC_VP8_VldReset(UINT32 u4VldId);


/// Read Barrel Shifter after shifting
/// \return Value of barrel shifter input window after shifting
UINT32 u4VDEC_HAL_VP8_ShiftGetBitStream(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                  ///< [IN] Shift bits number
);


/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_VP8_GetBitStreamShift(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);



/// Initialize barrel shifter with byte alignment
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_VP8_InitBarrelShifter(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                     ///< [IN] Video decoder hardware ID
    VDEC_INFO_VP8_FRM_HDR_T *prVp8BSInitPrm
);


/// Read current read pointer
/// \return Current read pointer with byte alignment
UINT32 u4VDEC_HAL_VP8_ReadRdPtr(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4VFIFOSa,
    UINT32 *pu4Bits                                     ///< [OUT] Read pointer with remained bits
);


/// Align read pointer to byte,word or double word
/// \return None
void vVDEC_HAL_VP8_AlignRdPtr(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4VFIFOSa,
    UINT32 u4AlignType                                  ///< [IN] Align type
);


/// Read barrel shifter bitcount after initializing 
/// \return Bitcount counted by HAL
UINT32 u4VDEC_HAL_VP8_GetBitcount(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter ID
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
UINT32 i4VDEC_HAL_VP8_DecStart(
    UINT32 u4VDecID, 
    VDEC_INFO_VP8_FRM_HDR_T *pVp8DecInfo);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_VP8_VPStart(
    UINT32 u4VDecID,
    VDEC_INFO_DEC_PRM_T *prDecPrm            ///< [IN] Pointer to VP8 decode Information
);


/// Read current decoded mbx and mby
/// \return None
void vVDEC_HAL_VP8_GetMbxMby(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 *pu4Mbx,                                      ///< [OUT] Pointer to current decoded macroblock in x axis
    UINT32 *pu4Mby                                       ///< [OUT] Pointer to current decoded macroblock in y axis
);


/// Read error count after decoding end
/// \return None
void vVDEC_HAL_VP8_GetErrInfo(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_VP8_ERR_INFO_T *prVp8ErrInfo          ///< [OUT] Pointer to error information
);


/// Read VP8 error type after decoding end
/// \return VP8 decode error type
UINT32 u4VDEC_HAL_VP8_GetErrType(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// VP8 Dec end process, only for verification
/// \return None
void vVDEC_HAL_VP8_DecEndProcess(
    UINT32 u4VDecID
);

void vVDEC_HAL_VP8_VDec_DumpReg(UINT32 u4VDecID, BOOL fgBefore);


/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_VP8_GetBoolCoderShift(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);

/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_VP8_InitBoolCoder(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);

/// Read DDR3 Decode Finish Flag
/// \return 
INT32 i4VDEC_HAL_VP8_DDR3_DecFinish(
    UINT32 u4VDecID
);

UINT32 u4VDEC_HAL_VP8_Default_Models_Init(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP8_Parse_Mb_Type_Models(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP8_Load_QMatrix(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP8_Load_Filter_Coef(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4Select);

UINT32 u4VDEC_HAL_VP8_Read_QMatrix(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP8_Read_SRAMData1(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP8_Write_SRAMData1(UINT32 u4BSID, UINT32 u4VDecID);


#if CONFIG_DRV_VERIFY_SUPPORT
void vVDEC_HAL_VP8_VDec_ReadCheckSum(UINT32 u4VDecID, UINT32 *pu4DecCheckSum);

BOOL fgVDEC_HAL_VP8_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum);

UINT32 u4VDEC_HAL_VP8_VDec_ReadFinishFlag(UINT32 u4VDecID);
#endif

BOOL fgVDEC_Vp8IsDecFinish(VOID);


//
/*! @} */


#endif //#ifndef _HAL_VDEC_VP8_IF_H_

