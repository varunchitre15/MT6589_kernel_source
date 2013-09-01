#ifndef __MTK_FB_CONSOLE_H__
#define __MTK_FB_CONSOLE_H__

#ifdef __cplusplus
extern "C" {
#endif


#define MFC_CHECK_RET(expr)             \
    do {                                \
        MFC_STATUS ret = (expr);        \
        ASSERT(MFC_STATUS_OK == ret);   \
    } while (0)


typedef enum
{	
   MFC_STATUS_OK                = 0,

   MFC_STATUS_INVALID_ARGUMENT  = -1,
   MFC_STATUS_NOT_IMPLEMENTED   = -2,
   MFC_STATUS_OUT_OF_MEMORY     = -3,
   MFC_STATUS_LOCK_FAIL         = -4,
   MFC_STATUS_FATAL_ERROR       = -5,
} MFC_STATUS;


typedef void* MFC_HANDLE;


/* MTK Framebuffer Console API */

MFC_STATUS MFC_Open(MFC_HANDLE *handle,
                    void *fb_addr,
                    unsigned int fb_width,
                    unsigned int fb_height,
                    unsigned int fb_bpp,
                    unsigned int fg_color,
                    unsigned int bg_color);

MFC_STATUS MFC_Close(MFC_HANDLE handle);

MFC_STATUS MFC_SetColor(MFC_HANDLE handle,
                        unsigned int fg_color, 
                        unsigned int bg_color);

MFC_STATUS MFC_ResetCursor(MFC_HANDLE handle);

MFC_STATUS MFC_Print(MFC_HANDLE handle, const char *str);

MFC_STATUS MFC_LowMemory_Printf(MFC_HANDLE handle, const char *str, UINT32 fg_color, UINT32 bg_color);

MFC_STATUS MFC_SetMem(MFC_HANDLE handle, const char *str, UINT32 color);
#ifdef __cplusplus
}
#endif

#endif // __MTK_FB_CONSOLE_H__
