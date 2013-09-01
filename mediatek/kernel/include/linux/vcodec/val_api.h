#ifndef _VAL_API_H_
#define _VAL_API_H_
#include "val_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
 *                             Function Declaration
 *===========================================================================*/
VAL_UINT32_T eVideoInitMVA(VAL_VOID_T** a_pvHandle);
VAL_UINT32_T eVideoAllocMVA(VAL_VOID_T* a_pvHandle, VAL_UINT32_T a_u4Va, VAL_UINT32_T* ap_u4Pa, VAL_UINT32_T a_u4Size, VAL_VCODEC_M4U_BUFFER_CONFIG_T *a_pvM4uConfig);
VAL_UINT32_T eVideoFreeMVA(VAL_VOID_T* a_pvHandle, VAL_UINT32_T a_u4Va, VAL_UINT32_T a_u4Pa, VAL_UINT32_T a_u4Size, VAL_VCODEC_M4U_BUFFER_CONFIG_T *a_pvM4uConfig);
VAL_UINT32_T eVideoDeInitMVA(VAL_VOID_T* a_pvHandle);

VAL_RESULT_T eValInit(VAL_HANDLE_T *a_phHalHandle);
VAL_RESULT_T eValDeInit(VAL_HANDLE_T *a_phHalHandle);
VAL_RESULT_T eVideoMemAlloc(VAL_MEMORY_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoMemFree(VAL_MEMORY_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoMemSet( VAL_MEMORY_T *a_prParam, VAL_UINT32_T a_u4ParamSize, VAL_INT32_T a_u4Value, VAL_UINT32_T a_u4Size);
VAL_RESULT_T eVideoMemCpy(VAL_MEMORY_T *a_prParamDst, VAL_UINT32_T a_u4ParamDstSize, VAL_MEMORY_T *a_prParamSrc, VAL_UINT32_T a_u4ParamSrcSize, VAL_UINT32_T a_u4Size);
VAL_RESULT_T eVideoMemCmp(VAL_MEMORY_T *a_prParamSrc1, VAL_UINT32_T a_u4ParamSrc1Size, VAL_MEMORY_T *a_prParamSrc2, VAL_UINT32_T a_u4ParamSrc2Size, VAL_UINT32_T a_u4Size);
VAL_RESULT_T eVideoIntMemAlloc(VAL_INTMEM_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoIntMemFree(VAL_INTMEM_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoCreateEvent(VAL_EVENT_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoSetEvent(VAL_EVENT_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoCloseEvent(VAL_EVENT_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoWaitEvent(VAL_EVENT_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoCreateMutex(VAL_MUTEX_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoCloseMutex(VAL_MUTEX_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoWaitMutex(VAL_MUTEX_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoReleaseMutex(VAL_MUTEX_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoGetTimeOfDay(VAL_TIME_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoStrStr(VAL_STRSTR_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoAtoi(VAL_ATOI_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eHalEMICtrlForRecordSize(VAL_RECORD_SIZE_T *a_prDrvRecordSize);

// for hybrid HW
VAL_RESULT_T eVideoFlushCache(VAL_MEMORY_T *a_prParam, VAL_UINT32_T a_u4ParamSize, VAL_UINT32_T optype);
VAL_RESULT_T eVideoMMAP(VAL_MMAP_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoUnMMAP(VAL_MMAP_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T WaitISR(VAL_ISR_T *a_prParam, VAL_UINT32_T a_u4ParamSize);
VAL_RESULT_T eVideoLockHW(VAL_HW_LOCK_T *a_prParam, VAL_UINT32_T  a_u4ParamSize);
VAL_RESULT_T eVideoUnLockHW(VAL_HW_LOCK_T *a_prParam, VAL_UINT32_T  a_u4ParamSize);
VAL_RESULT_T eVideoInitLockHW( VAL_VCODEC_OAL_HW_REGISTER_T *prParam, int size);
VAL_RESULT_T eVideoDeInitLockHW( VAL_VCODEC_OAL_HW_REGISTER_T *prParam, int size);
VAL_RESULT_T eVideoVcodecSetThreadID (VAL_VCODEC_THREAD_ID_T *a_prThreadID);
VAL_RESULT_T eVideoVCodecCoreLoading(int CPUid, int *Loading);
VAL_RESULT_T eVideoVCodecCoreNumber(int *CPUNums);

// for MCI
VAL_RESULT_T eVideoConfigMCIPort(VAL_UINT32_T u4PortConfig, VAL_UINT32_T *pu4PortResult, VAL_MEM_CODEC_T eMemCodec);
#ifdef __cplusplus
}
#endif

#endif // #ifndef _VAL_API_H_
