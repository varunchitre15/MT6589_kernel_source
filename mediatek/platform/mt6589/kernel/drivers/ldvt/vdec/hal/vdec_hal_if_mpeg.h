
#ifndef _VDEC_HAL_IF_MPEG_H_
#define _VDEC_HAL_IF_MPEG_H_

#include "../include/vdec_info_mpeg.h"
#include "../include/vdec_info_common.h"
//#include "drv_config.h"
//#include "vdec_hw_common.h"
//#include "vdec_verify_typedef.h"
//#include "vdec_verify_keydef.h"
//#include "typedef.h"


/*! \name Video Decoder HAL MPEG Interface
* @{
*/

/// Initialize video decoder hardware
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_MPEG_InitVDecHW(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_MPEG_VFIFO_PRM_T *prMpegVFifoPrm      ///< [IN] Pointer to video fifo address information
);


/// Read Barrel Shifter after shifting
/// \return Value of barrel shifter input window after shifting
UINT32 u4VDEC_HAL_MPEG_ShiftGetBitStream(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                  ///< [IN] Shift bits number
);


/// Read Barrel Shifter before shifting
/// \return Value of barrel shifter input window before shifting
UINT32 u4VDEC_HAL_MPEG_GetBitStreamShift(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4ShiftBits                                  ///< [IN] Shift bits number
);


/// Initialize barrel shifter with byte alignment
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_MPEG_InitBarrelShifter(
    UINT32 u4BSID,                                           ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                       ///< [IN] Video decoder hardware ID
    VDEC_INFO_MPEG_BS_INIT_PRM_T *prMpegBSInitPrm
);

INT32 i4VDEC_HAL_MPEG2_InitBarrelShifter(
    UINT32 u4BSID,                                           ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                       ///< [IN] Video decoder hardware ID
    VDEC_INFO_MPEG_BS_INIT_PRM_T *prMpegBSInitPrm
);


/// Load sum from VDec HW to barrel shifter
/// \return  None
void vVDEC_HAL_MPEG_VLDVdec2Barl(
    UINT32 u4VDecID
);


/// Read current read pointer
/// \return Current read pointer with byte alignment
UINT32 u4VDEC_HAL_MPEG_ReadRdPtr(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4VFIFOSa,                                  ///< [IN] Video FIFO start address
    UINT32 *pu4Bits                                      ///< [OUT] Read pointer with remained bits
);


/// Align read pointer to byte,word or double word
/// \return None
void vVDEC_HAL_MPEG_AlignRdPtr(
    UINT32 u4BSID,                                        ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 u4VFIFOSa,                                  ///< [IN] Video FIFO start address
    UINT32 u4AlignType                                  ///< [IN] Align type
);

#if 0   //VDEC_REMOVE_UNUSED_FUNC
/// Read barrel shifter bitcount after initializing 
/// \return  Bitcount counted by HAL
UINT32 u4VDEC_HAL_MPEG_GetBitcount(
    UINT32 u4BSID,                                      ///< [IN] Barrel shifter hardware ID of one video decoder
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);
#endif

/// Set video decoder hardware registers to load quantization, only for MPEG12
/// \return None
#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
void vVDEC_HAL_MPEG12_LoadQuantMatrix(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    BOOL fgIntra,                                           ///< [IN] flag of intra block or not
    VDEC_INFO_MPEG_QANTMATRIX_T *prQuantMatrix                    ///< [OUT] Pointer to MPEG12 quantization matrix array
);
#endif

/// Set video decoder hardware registers to load quantization, only for MPEG4
/// \return None
#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
BOOL fgVDEC_HAL_MPEG4_LoadQuantMatrix(
    UINT32 u4VDecID,                                     ///< [IN] Video decoder hardware ID
    BOOL fgIntra,                                           ///< [IN] flag of intra block or not
    UINT32 *pu4QuanMatrixLen,                       ///< [IN] Pointer of MPEG4 quantization matrix length
    VDEC_INFO_MPEG_BS_INIT_PRM_T *prMpegBSInitPrm,                ///< [IN] Pointer of MPEG initialize barrel shifter struct
    VDEC_INFO_MPEG_QANTMATRIX_T *prQuantMatrix                     ///< [OUT] Pointer to MPEG4 quantization matrix array
);
#endif

/// Load quantization, only for MPEG12
/// \return None
void vVDEC_HAL_MPEG12_FW_LoadQuantMatrix(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    BOOL fgIntra,                                           ///< [IN] flag of intra block or not
    UINT32 *pu4QuantMatrix                    ///< [OUT] Pointer to MPEG12 quantization matrix array
);


/// Load quantization, only for MPEG4
/// \return None
void vVDEC_HAL_MPEG4_FW_LoadQuantMatrix(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    BOOL fgIntra,                                           ///< [IN] flag of intra block or not
    UINT32 *pu4QuanMatrixLen,                       ///< [IN] Pointer of MPEG4 quantization matrix length
    UINT32 *pu4QuantMatrix                    ///< [OUT] Pointer to MPEG4 quantization matrix array
);


/// Set video decoder hardware registers to load quantization
/// \return None
void vVDEC_HAL_MPEG_ReLoadQuantMatrix(
    UINT32 u4VDecID,                                                                 ///< [IN] Video decoder hardware ID
    BOOL fgIntra                                                                         ///< [IN] flag of intra block or not
);

#if 0   //VDEC_REMOVE_UNUSED_FUNC
/// Write video decoder hardware quantization matrix
/// and load it
/// \return None
void vVDEC_HAL_MPEG_ReStoreQuantMatrix(
    UINT32 u4VDecID,                                                                 ///< [IN] Video decoder hardware ID
    BOOL fgIntra,                                                                       ///< [IN]  flag of intra block or not
    VDEC_INFO_MPEG_QANTMATRIX_T *prQuantMatrix                    ///< [IN] Pointer to MPEG quantization matrix array
);
#endif

//
void vVDEC_HAL_MPEG_ResetQuantMatrix(
    UINT32 u4VDecID                                                                  ///< [IN] Video decoder hardware ID
);


/// Set Mpeg4 register flag
/// \return None
#if (CONFIG_DRV_ONLY || CONFIG_DRV_VERIFY_SUPPORT)
void vVDEC_HAL_MPEG_SetMPEG4Flag(
    UINT32 u4VDecID,                                                                    ///< [IN] Video decoder hardware ID
    BOOL fgMp4                                                                          ///< [IN] Flag of MPEG4 or not
);
#endif

/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_MPEG12_DecStart(
    UINT32 u4VDecID,                                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_DEC_PRM_T *prDecPrm      ///< [IN] Pointer to MPEG12 decode Information
);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_DIVX3_DecStart(
    UINT32 u4VDecID,                                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_DEC_PRM_T *prDecPrm        ///< [IN] Pointer to DIVX3 decode Information
);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_MPEG4_DecStart(
    UINT32 u4VDecID,                                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_DEC_PRM_T *prDecPrm        ///< [IN] Pointer to MPEG4 decode Information
);


/// Set video decoder hardware registers to decode
/// \return If return value < 0, it's failed. Please reference hal_vdec_errcode.h.
INT32 i4VDEC_HAL_MPEG_VPStart(
    UINT32 u4VDecID,
    VDEC_INFO_DEC_PRM_T *prDecPrm            ///< [IN] Pointer to WMV decode Information
);


/// Read current decoded mbx and mby
/// \return None
void vVDEC_HAL_MPEG_GetMbxMby(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    UINT32 *pu4Mbx,                                      ///< [OUT] Pointer to current decoded macroblock in x axis
    UINT32 *pu4Mby                                       ///< [OUT] Pointer to current decoded macroblock in y axis
);


/// Read error count after decoding end
/// \return None
void vVDEC_HAL_MPEG_GetErrInfo(
    UINT32 u4VDecID,                                    ///< [IN] Video decoder hardware ID
    VDEC_INFO_MPEG_ERR_INFO_T *prMpegErrInfo         ///< [OUT] Pointer to error information
);

///Disable Motion Vector Overflow Dectetion
///\return If return value < 0, it's failed.
INT32 vVDEC_HAL_MPEG2_DisableMVOverflowDetection(
    UINT32 u4VDecID
);

/// Read Mpeg4 or Divx3 error type after decoding end
/// \return Mpeg4 or Divx3 decode error type
UINT32 u4VDEC_HAL_MPEG4_GetErrType(
    UINT32 u4VDecID                                    ///< [IN] Video decoder hardware ID
);

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)

#define VDEC_BCODE_SRAM_ADDR    4096
void vVDEC_HAL_MPEG_WriteSram(UINT32 u4InstID,UINT32 u4SramAddr,UINT32 u4SramValue);
UINT32 u4VDEC_HAL_MPEG_ReadSram(UINT32 u4InstID,UINT32 u4SramAddr);

#endif





#if CONFIG_DRV_VERIFY_SUPPORT
void vVDEC_HAL_MPEG_VDec_ReadCheckSum(UINT32 u4VDecID, UINT32 *pu4DecCheckSum);

BOOL fgVDEC_HAL_MPEG_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum);

UINT32 u4VDEC_HAL_MPEG_VDec_ReadFinishFlag(UINT32 u4VDecID, BOOL fgMpeg4);

UINT32 u4VDEC_HAL_MPEG_VDec_ReadErrorFlag(UINT32 u4VDecID);

void vVDEC_HAL_MPEG_VDec_DumpReg(UINT32 u4VDecID, BOOL fgBefore);
#endif
//
/*! @} */
#endif //#ifndef _HAL_VDEC_MPEG_IF_H_

