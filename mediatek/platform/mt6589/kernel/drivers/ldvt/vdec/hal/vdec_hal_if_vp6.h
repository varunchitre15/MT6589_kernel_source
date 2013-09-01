
#ifndef _VDEC_HAL_IF_VP6_H_
#define _VDEC_HAL_IF_VP6_H_

#include "../include/vdec_info_vp6.h"
#include "../include/vdec_info_common.h"

/*! \name Video Decoder HAL VP6 Interface
* @{
*/

/// Initialize video decoder hardware
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_VP6_InitVDecHW(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_VP6_VFIFO_PRM_T *prVp6VFifoInitPrm      ///< [IN] Pointer to video fifo address information
);


/// Read Barrel Shifter after shifting
/// \return Value of barrel shifter input window after shifting
UINT32 u4VDEC_HAL_VP6_ShiftGetBitStream(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                  ///< [IN] Shift bits number
);


/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_VP6_GetBitStreamShift(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);



/// Initialize barrel shifter with byte alignment
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_VP6_InitBarrelShifter(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                     ///< [IN] Video decoder hardware ID
    VDEC_INFO_VP6_BS_INIT_PRM_T *prVp6BSInitPrm
);


/// Read current read pointer
/// \return Current read pointer with byte alignment
UINT32 u4VDEC_HAL_VP6_ReadRdPtr(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4VFIFOSa,
    UINT32 *pu4Bits                                     ///< [OUT] Read pointer with remained bits
);


/// Align read pointer to byte,word or double word
/// \return None
void vVDEC_HAL_VP6_AlignRdPtr(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4VFIFOSa,
    UINT32 u4AlignType                                  ///< [IN] Align type
);


/// Read barrel shifter bitcount after initializing 
/// \return Bitcount counted by HAL
UINT32 u4VDEC_HAL_VP6_GetBitcount(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter ID
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_VP6_DecStart(
    UINT32 u4VDecID,
    VDEC_INFO_DEC_PRM_T *prDecPrm            ///< [IN] Pointer to VP6 decode Information
);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_VP6_VPStart(
    UINT32 u4VDecID,
    VDEC_INFO_DEC_PRM_T *prDecPrm            ///< [IN] Pointer to VP6 decode Information
);


/// Read current decoded mbx and mby
/// \return None
void vVDEC_HAL_VP6_GetMbxMby(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 *pu4Mbx,                                      ///< [OUT] Pointer to current decoded macroblock in x axis
    UINT32 *pu4Mby                                       ///< [OUT] Pointer to current decoded macroblock in y axis
);


/// Read error count after decoding end
/// \return None
void vVDEC_HAL_VP6_GetErrInfo(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_VP6_ERR_INFO_T *prVp6ErrInfo          ///< [OUT] Pointer to error information
);


/// Read VP6 error type after decoding end
/// \return VP6 decode error type
UINT32 u4VDEC_HAL_VP6_GetErrType(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);


/// VP6 Dec end process, only for verification
/// \return None
void vVDEC_HAL_VP6_DecEndProcess(
    UINT32 u4VDecID
);

void vVDEC_HAL_VP6_VDec_DumpReg(UINT32 u4VDecID, BOOL fgBefore);


/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_VP6_GetBoolCoderShift(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);

/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_VP6_InitBoolCoder(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                 ///< [IN] Shift bits number
);

/// Read DDR3 Decode Finish Flag
/// \return 
INT32 i4VDEC_HAL_VP6_DDR3_DecFinish(
    UINT32 u4VDecID
);


UINT32 u4VDEC_HAL_VP6_Default_Models_Init(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP6_Parse_Mb_Type_Models(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP6_Load_QMatrix(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP6_Load_Filter_Coef(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4Select);

UINT32 u4VDEC_HAL_VP6_Read_QMatrix(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP6_Read_SRAMData1(UINT32 u4BSID, UINT32 u4VDecID);
UINT32 u4VDEC_HAL_VP6_Write_SRAMData1(UINT32 u4BSID, UINT32 u4VDecID);


#if CONFIG_DRV_VERIFY_SUPPORT
void vVDEC_HAL_VP6_VDec_ReadCheckSum(UINT32 u4VDecID, UINT32 *pu4DecCheckSum);

BOOL fgVDEC_HAL_VP6_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum);

UINT32 u4VDEC_HAL_VP6_VDec_ReadFinishFlag(UINT32 u4VDecID);
#endif

//MULTI-STREAM PANDA
UINT32 u4VDEC_HAL_VP6_VDec_SetByteCount(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4Ofst);
INT32 i4VDEC_HAL_VP6_InitBarrelShifter2(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_VP6_BS_INIT_PRM_T *prVp6BSInitPrm, VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr);
//~MULTI-STREAM PANDA

UINT32 u4VDEC_HAL_VP6_VDec_SetWorkspace(UINT32 u4VDecID, UINT32 u4VLDWrapperWorkspace, UINT32 u4PPWrapperWorkspace);

// ALPHA
UINT32 u4VDec_HAL_VP6_VDec_BackupSram(UINT32 u4VDecID, VDEC_INFO_VP6_DEC_PRM_T *prAlphaParam);
UINT32 u4VDec_HAL_VP6_VDec_RestoreSram(UINT32 u4VDecID, VDEC_INFO_VP6_DEC_PRM_T *prAlphaParam);

//
/*! @} */


#endif //#ifndef _HAL_VDEC_VP6_IF_H_

