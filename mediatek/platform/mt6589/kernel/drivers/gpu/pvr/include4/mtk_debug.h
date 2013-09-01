#ifndef __MTK_DEBUG_H__
#define __MTK_DEBUG_H__

#include "mtk_version.h"

#include "img_types.h"

#include <linux/workqueue.h>

#include "servicesext.h"
#include "mutex.h"

#if defined (__cplusplus)
extern "C" {
#endif

enum MTK_PP_REGISTER
{
    MTK_PP_R_SGXOSTimer,
    MTK_PP_R_DEVMEM,
    MTK_PP_R_SIZE
};

#ifdef MTK_DEBUG

IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugInit(IMG_VOID);
IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugDeinit(IMG_VOID);
IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugSetInfo(const IMG_CHAR* acDebugMsg, IMG_INT32 i32Size);
IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugGetInfo(IMG_CHAR* acDebugMsg, IMG_INT32 i32Size);
IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugEnable3DMemInfo(IMG_BOOL bEnable);
IMG_IMPORT IMG_BOOL IMG_CALLCONV MTKDebugIsEnable3DMemInfo(IMG_VOID);

#ifdef MTK_DEBUG_PROC_PRINT

enum MTK_PP_BUFFER
{
    MTK_PP_B_QUEUEBUFFER,
    MTK_PP_B_RINGBUFFER
};

typedef struct MTK_PROC_PRINT_DATA_TAG
{
	char *data;
	char **line;
	int data_array_size;
	int line_array_size;
	int p;	// counter of data
	int l;	// counter of line
	
	int flags;
	
	void (*pfn_print)(struct MTK_PROC_PRINT_DATA_TAG *data, const char *fmt, ...) IMG_FORMAT_PRINTF(2,3);
	
	spinlock_t lock;
	unsigned long irqflags;
	
} MTK_PROC_PRINT_DATA;

MTK_PROC_PRINT_DATA *MTKPPGetObjById(int id);

#define MTKPP_LOG(id, ...) do {														\
		MTK_PROC_PRINT_DATA *mtkpp_data = MTKPPGetObjById(id); 						\
		if (mtkpp_data != NULL) { mtkpp_data->pfn_print(mtkpp_data, __VA_ARGS__); }	\
	} while(0);

// for SGXOSTimer()
MTK_PROC_PRINT_DATA *MTK_PP_4_SGXOSTimer_getregister(void);
int MTK_PP_4_SGXOSTimer_register(void);
void MTK_PP_4_SGXOSTimer_unregister(void);

#define _PVR_DPF_MTKPP(level, ...) do {													\
		MTK_PROC_PRINT_DATA *mtkpp_data = MTK_PP_4_SGXOSTimer_getregister();			\
		if (mtkpp_data != NULL) { mtkpp_data->pfn_print(mtkpp_data, __VA_ARGS__); }		\
		else { PVR_DPF((level, __VA_ARGS__)); }											\
	} while(0)
#define _PVR_LOG_MTKPP(...) do {														\
		MTK_PROC_PRINT_DATA *mtkpp_data = MTK_PP_4_SGXOSTimer_getregister();			\
		if (mtkpp_data != NULL) { mtkpp_data->pfn_print(mtkpp_data, __VA_ARGS__); }		\
		else { PVR_LOG((__VA_ARGS__)) }													\
	} while(0);

#define MTKPP_REGISTER		do { if (MTK_PP_4_SGXOSTimer_register() == 1) {
#define PVR_DPF_MTKPP(x)	_PVR_DPF_MTKPP x
#define PVR_LOG_MTKPP(x)	_PVR_LOG_MTKPP x
#define MTKPP_UNREGISTER	MTK_PP_4_SGXOSTimer_unregister(); } } while(0);

// old interface
#define MDWP_REGISTER		MTKPP_REGISTER
#define PVR_LOG_MDWP(x) 	PVR_LOG_MTKPP(x)
#define PVR_DPF_MDWP(x) 	PVR_DPF_MTKPP(x)
#define MDWP_UNREGISTER		MTKPP_UNREGISTER

#endif // MTK_DEBUG_PROC_PRINT
#endif // MTK_DEBUG

#if !defined(MTK_DEBUG) || !defined(MTK_DEBUG_PROC_PRINT)
#define MTKPP_LOG(id, ...)

// for SGXOSTimer()
#define MTKPP_REGISTER
#define MTKPP_UNREGISTER
#define PVR_DPF_MTKPP(x)    PVR_DPF(x)
#define PVR_LOG_MTKPP(x)    PVR_LOG(x)

// old interface
#define MDWP_REGISTER
#define PVR_LOG_MDWP(x) PVR_LOG(x)
#define PVR_DPF_MDWP(x) PVR_DPF(x)
#define MDWP_UNREGISTER
#endif

#if defined (__cplusplus)
}
#endif

#endif	/* __MTK_DEBUG_H__ */

/******************************************************************************
 End of file (mtk_debug.h)
******************************************************************************/

