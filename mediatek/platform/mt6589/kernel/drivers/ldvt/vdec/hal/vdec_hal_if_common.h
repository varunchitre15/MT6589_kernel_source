
#ifndef _VDEC_HAL_IF_COMMON_H_
#define _VDEC_HAL_IF_COMMON_H_

#include "vdec_hal_if_mpeg.h"
#include "vdec_hal_errcode.h"
#include "vdec_hw_common.h"
//#include "drv_config.h"
//#include "chip_ver.h"
#include "../include/vdec_info_common.h"

//#include <mach/mt6575_typedefs.h>
//#include "vdec_verify_info_common.h"
//#include "vdec_verify_hw_common.h"
//#include "vdec_verify_hal_errcode.h"

/*! \name Video Decoder HAL Common Private Function
* @{
*/

typedef void  (* vDecEndCallback)(void *pvUserPrivData);

//
/*! @} */

/*! \name VDEC HAL init/uninit
* @{
*/

/// This function turns on video decoder HAL
/// \return If return value < 0, it's failed. Please reference drv_vdec_errcode.h.
INT32 i4VDEC_HAL_Common_Init(
    UINT32 u4ChipID            ///< [IN] Chip ID
);

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
void vVDEC_HAL_CLK_Set(UINT32 u4CodeType);
#endif

/// This function turns off video decoder HAL
/// \return If return value < 0, it's failed. Please reference drv_vdec_errcode.h.
#if VDEC_REMOVE_UNUSED_FUNC
INT32 i4VDEC_HAL_Common_Uninit(void);
#endif


void vVDec_HAL_CRC_Enable(UINT32 u4VDecID, BOOL fgCRCType);

//
/*! @} */

/*! \name Video Decoder HAL Common Interface
* @{
*/


/// Get video decoder hardware resource
/// \return None
void u4VDEC_HAL_Common_GetHWResourceNum(
    UINT32 *pu4BSNum,                                ///< [OUT] Pointer to barrel shifter number of every VLD
    UINT32 *pu4VLDNum                               ///< [OUT] Pointer to VLD number
);


/// Turn on or turn off VLD power
/// \return None
void vDEC_HAL_COMMON_SetVLDPower(
    UINT32 u4VDecID,                                            ///< [IN] Video decoder hardware ID
    BOOL fgOn                                                      ///< [IN] Turn on or off VLD
);


#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
INT32 vDEC_HAL_COMMON_ReadLBDResult(
    UINT32 ucMpvId, 
    UINT32* u4YUpbound, 
    UINT32* u4YLowbound, 
    UINT32* u4CUpbound, 
    UINT32* u4CLowbound
);
#endif


void  vDEC_HAL_COMMON_PowerOn (void );

void vDEC_HAL_COMMON_PowerOff (void );

//
/*! @} */

INT32 i4VDEC_HAL_Dram_Busy (
    UINT32 u4ChipID,            ///< [IN] Chip ID
    UINT32 u4StartAddr, UINT32 u4Offset
);

INT32 i4VDEC_HAL_Dram_Busy_Off(
    UINT32 u4ChipID,            ///< [IN] Chip ID
    UINT32 u4StartAddr, UINT32 u4Offset
);

INT32 i4VDEC_HAL_Common_Gcon_Enable(void);

#ifdef CAPTURE_ESA_LOG
UINT32 u4VDEC_HAL_Read_ESA(UINT32 u4InstID , UINT32 u4Temp);
#endif
#ifdef VDEC_BREAK_EN
BOOL fgBreakVDec(UINT32 u4InstID);
#endif

#if VMMU_SUPPORT
void vPage_Table(UINT32 u4InstID, UINT32  page_addr, UINT32 start, UINT32 end); // page_addr = table_base + (start/4KB)*4
void vVDecVMMUEnable(UINT32 table_base);
#endif

#endif //#ifndef _HAL_VDEC_COMMON_IF_H_

