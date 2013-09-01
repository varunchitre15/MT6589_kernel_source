/**
 * @file
 *   hal_types.h
 *
 * @par Project:
 *   MFlexVideo
 *
 * @par Description:
 *   Hardware Abstraction Layer Type Definitions
 *
 * @par Author:
 *   Jackal Chen (mtk02532)
 *
 * @par $Revision: #4 $
 * @par $Modtime:$
 * @par $Log:$
 *
 */

#ifndef _HAL_TYPES_H_
#define _HAL_TYPES_H_

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @par Structure
 *  HAL_POWER_T
 * @par Description
 *  This is a parameter for eVideoHwPowerCtrl()
 */
typedef struct _HAL_POWER_T
{
    VAL_VOID_T *pvHandle;                ///< [IN]     The video codec driver handle
    VAL_UINT32_T u4HandleSize;           ///< [IN]     The size of video codec driver handle
    VAL_DRIVER_TYPE_T eDriverType;       ///< [IN]     The driver type
    VAL_BOOL_T  fgEnable;                ///< [IN]     Enable or not.
    VAL_VOID_T *pvReserved;              ///< [IN/OUT] The reserved parameter
    VAL_UINT32_T u4ReservedSize;         ///< [IN]     The size of reserved parameter structure
} HAL_POWER_T;

/**
 * @par Structure
 *  HAL_ISR_T
 * @par Description
 *  This is a parameter for eVideoHwPowerCtrl()
 */
typedef struct _HAL_ISR_T
{
    VAL_VOID_T *pvHandle;                ///< [IN]     The video codec driver handle
    VAL_UINT32_T u4HandleSize;           ///< [IN]     The size of video codec driver handle
    VAL_DRIVER_TYPE_T eDriverType;       ///< [IN]     The driver type
    VAL_BOOL_T  fgRegister;              ///< [IN]     Register or un-register.
    VAL_VOID_T *pvReserved;              ///< [IN/OUT] The reserved parameter
    VAL_UINT32_T u4ReservedSize;         ///< [IN]     The size of reserved parameter structure
} HAL_ISR_T;

/**
 * @par Structure
 *  HAL_CLOCK_T
 * @par Description
 *  This is a parameter for eVideoHwEnableClock() and eVideoHwDisableClock()
 */
typedef struct _HAL_CLOCK_T
{
    VAL_VOID_T *pvHandle;                ///< [IN]     The video codec driver handle
    VAL_UINT32_T u4HandleSize;           ///< [IN]     The size of video codec driver handle
    VAL_DRIVER_TYPE_T eDriverType;       ///< [IN]     The driver type
    VAL_VOID_T *pvReserved;              ///< [IN/OUT] The reserved parameter
    VAL_UINT32_T u4ReservedSize;         ///< [IN]     The size of reserved parameter structure
} HAL_CLOCK_T;

#ifdef __cplusplus
}
#endif

#endif // #ifndef _HAL_TYPES_H_
