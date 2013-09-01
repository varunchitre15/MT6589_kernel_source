

/** 
 * @file 
 *   val_log.h 
 *
 * @par Project:
 *   MFlexVideo 
 *
 * @par Description:
 *   Log System
 *
 * @par Author:
 *   Jackal Chen (mtk02532)
 *
 * @par $Revision: #5 $
 * @par $Modtime:$
 * @par $Log:$
 *
 */

#ifndef _VAL_LOG_H_
#define _VAL_LOG_H_

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "MFV_COMMON"
#include <utils/Log.h>
#include <cutils/xlog.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MFV_LOG_ERROR   //error
#ifdef MFV_LOG_ERROR
#define MFV_LOGE(...) xlog_printf(ANDROID_LOG_ERROR, "VDO_LOG", __VA_ARGS__);
#define VDO_LOGE(...) xlog_printf(ANDROID_LOG_ERROR, "VDO_LOG", __VA_ARGS__);
#else
#define MFV_LOGE(...)
#define VDO_LOGE(...)
#endif

#define MFV_LOG_WARNING //warning
#ifdef MFV_LOG_WARNING
#define MFV_LOGW(...) xlog_printf(ANDROID_LOG_WARN, "VDO_LOG", __VA_ARGS__);
#define VDO_LOGW(...) xlog_printf(ANDROID_LOG_WARN, "VDO_LOG", __VA_ARGS__);
#else
#define MFV_LOGW(...)
#define VDO_LOGW(...)
#endif

//#define MFV_LOG_DEBUG   //debug information
#ifdef MFV_LOG_DEBUG
#define MFV_LOGD(...) xlog_printf(ANDROID_LOG_DEBUG, "VDO_LOG", __VA_ARGS__);
#define VDO_LOGD(...) xlog_printf(ANDROID_LOG_DEBUG, "VDO_LOG", __VA_ARGS__);
#else
#define MFV_LOGD(...)
#define VDO_LOGD(...)
#endif

#define MFV_LOG_INFO   //information
#ifdef MFV_LOG_INFO
#define MFV_LOGI(...) xlog_printf(ANDROID_LOG_INFO, "VDO_LOG", __VA_ARGS__);
#define VDO_LOGI(...) xlog_printf(ANDROID_LOG_INFO, "VDO_LOG", __VA_ARGS__);
#else
#define MFV_LOGI(...)
#define VDO_LOGI(...)
#endif

#ifdef __cplusplus
}
#endif

#endif // #ifndef _VAL_LOG_H_
