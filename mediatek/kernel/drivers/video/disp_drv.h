#ifndef __DISP_DRV_H__
#define __DISP_DRV_H__

#ifdef BUILD_UBOOT
    #include <asm/arch/mt65xx_typedefs.h>
#else
///TODO: remove mt65xx_typedefs.h for next chip
    #include <mach/mt_typedefs.h>
#endif


#include "disp_drv_log.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------

#define DISP_CHECK_RET(expr)                                                \
    do {                                                                    \
        DISP_STATUS ret = (expr);                                           \
        if (DISP_STATUS_OK != ret) {                                        \
            DISP_LOG_PRINT(ANDROID_LOG_ERROR, "COMMON", "[ERROR][mtkfb] DISP API return error code: 0x%x\n"      \
                   "  file : %s, line : %d\n"                               \
                   "  expr : %s\n", ret, __FILE__, __LINE__, #expr);        \
        }                                                                   \
    } while (0)


// ---------------------------------------------------------------------------

typedef enum
{	
   DISP_STATUS_OK = 0,

   DISP_STATUS_NOT_IMPLEMENTED,
   DISP_STATUS_ALREADY_SET,
   DISP_STATUS_ERROR,
} DISP_STATUS;


typedef enum {
   DISP_STATE_IDLE = 0,
   DISP_STATE_BUSY,
} DISP_STATE;


#define MAKE_PANEL_COLOR_FORMAT(R, G, B) ((R << 16) | (G << 8) | B)
#define PANEL_COLOR_FORMAT_TO_BPP(x) ((x&0xff) + ((x>>8)&0xff) + ((x>>16)&0xff))

typedef enum {
    PANEL_COLOR_FORMAT_RGB332 = MAKE_PANEL_COLOR_FORMAT(3, 3, 2),
    PANEL_COLOR_FORMAT_RGB444 = MAKE_PANEL_COLOR_FORMAT(4, 4, 4),
    PANEL_COLOR_FORMAT_RGB565 = MAKE_PANEL_COLOR_FORMAT(5, 6, 5),
    PANEL_COLOR_FORMAT_RGB666 = MAKE_PANEL_COLOR_FORMAT(6, 6, 6),
    PANEL_COLOR_FORMAT_RGB888 = MAKE_PANEL_COLOR_FORMAT(8, 8, 8),
} PANEL_COLOR_FORMAT;

typedef enum {
    DISP_LCD_INTERRUPT_EVENTS_START     = 0x01,
    DISP_LCD_TRANSFER_COMPLETE_INT      = 0x01,
    DISP_LCD_REG_COMPLETE_INT           = 0x02,
    DISP_LCD_CDMQ_COMPLETE_INT          = 0x03,
    DISP_LCD_HTT_INT                    = 0x04,
    DISP_LCD_SYNC_INT                   = 0x05,
    DISP_LCD_INTERRUPT_EVENTS_END       = 0x05,

    
    DISP_DSI_INTERRUPT_EVENTS_START     = 0x11,
    DISP_DSI_READ_RDY_INT               = 0x11,
    DISP_DSI_CMD_DONE_INT               = 0x12,
    DISP_DSI_VSYNC_INT                  = 0x13,
    DISP_DSI_TARGET_LINE_INT            = 0x14,
    DISP_DSI_REG_UPDATE_INT             = 0x15,
    DISP_DSI_VMDONE_INT                 = 0x16,
    DISP_DSI_INTERRUPT_EVENTS_END       = 0x16,

    DISP_DPI_INTERRUPT_EVENTS_START     = 0x21,
    DISP_DPI_FIFO_EMPTY_INT             = 0x21,
    DISP_DPI_FIFO_FULL_INT              = 0x22,
    DISP_DPI_OUT_EMPTY_INT              = 0x23,
    DISP_DPI_CNT_OVERFLOW_INT           = 0x24,
    DISP_DPI_LINE_ERR_INT               = 0x25,
    DISP_DPI_VSYNC_INT                  = 0x26,
    DISP_DPI_TARGET_LINE_INT            = 0x27,
    DISP_DPI_REG_UPDATE_INT             = 0x28,
    DISP_DPI_INTERRUPT_EVENTS_END       = 0x28,
} DISP_INTERRUPT_EVENTS;

#define DISP_LCD_INTERRUPT_EVENTS_NUMBER (DISP_LCD_INTERRUPT_EVENTS_END - DISP_LCD_INTERRUPT_EVENTS_START + 1)
#define DISP_DSI_INTERRUPT_EVENTS_NUMBER (DISP_DSI_INTERRUPT_EVENTS_END - DISP_DSI_INTERRUPT_EVENTS_START + 1)
#define DISP_DPI_INTERRUPT_EVENTS_NUMBER (DISP_DPI_INTERRUPT_EVENTS_END - DISP_DPI_INTERRUPT_EVENTS_START + 1)

typedef void (*DISP_INTERRUPT_CALLBACK_PTR)(void *params);

typedef struct{
    DISP_INTERRUPT_CALLBACK_PTR pFunc;
    void *pParam;
}DISP_INTERRUPT_CALLBACK_STRUCT;

typedef struct{
    unsigned int id;
    unsigned int curr_en;
    unsigned int next_en;
    unsigned int hw_en;
    int curr_idx;
    int next_idx;
    int hw_idx;
    int curr_identity;
    int next_identity;
    int hw_identity;
    int curr_conn_type;
    int next_conn_type;
    int hw_conn_type;
}DISP_LAYER_INFO;

// ---------------------------------------------------------------------------
// Public Functions
// ---------------------------------------------------------------------------

DISP_STATUS DISP_Init(UINT32 fbVA, UINT32 fbPA, BOOL isLcmInited);
DISP_STATUS DISP_Deinit(void);
DISP_STATUS DISP_PowerEnable(BOOL enable);
DISP_STATUS DISP_PanelEnable(BOOL enable);
//<2013/02/10-21805-stevenchen, Add ADB commands to turn on/off LCM.
DISP_STATUS DISP_PanelOnOff(BOOL on_off);
//>2013/02/10-21805-stevenchen
DISP_STATUS DISP_LCDPowerEnable(BOOL enable);   ///only used to power on LCD for memory out
DISP_STATUS DISP_SetFrameBufferAddr(UINT32 fbPhysAddr);
DISP_STATUS DISP_EnterOverlayMode(void);
DISP_STATUS DISP_LeaveOverlayMode(void);
DISP_STATUS DISP_EnableDirectLinkMode(UINT32 layer);
DISP_STATUS DISP_DisableDirectLinkMode(UINT32 layer);
DISP_STATUS DISP_UpdateScreen(UINT32 x, UINT32 y, UINT32 width, UINT32 height);
DISP_STATUS DISP_SetInterruptCallback(DISP_INTERRUPT_EVENTS eventID, DISP_INTERRUPT_CALLBACK_STRUCT *pCBStruct);
DISP_STATUS DISP_WaitForLCDNotBusy(void);
DISP_STATUS DISP_PrepareSuspend(void);
DISP_STATUS DISP_GetLayerInfo(DISP_LAYER_INFO *pLayer);

//Register extra trigger source 
typedef int (*DISP_EXTRA_CHECKUPDATE_PTR)(int);
typedef int (*DISP_EXTRA_CONFIG_PTR)(int);
int DISP_RegisterExTriggerSource(DISP_EXTRA_CHECKUPDATE_PTR pCheckUpdateFunc , DISP_EXTRA_CONFIG_PTR pConfFunc);
void DISP_UnRegisterExTriggerSource(int u4ID);
void GetUpdateMutex(void);
void ReleaseUpdateMutex(void);

///TODO: implement it and replace LCD_LayerXXX for mtkfb.c
typedef enum
{
    DISP_SET_LAYER_ENABLE = 1,          ///type: BOOL
    DISP_GET_LAYER_ENABLE,
    DISP_SET_LAYER_ADDRESS,             ///type: UINT32
    DISP_GET_LAYER_ADDRESS,
    DISP_SET_LAYER_FORMAT,              ///type: 
    DISP_GET_LAYER_FORMAT,
    DISP_SET_LAYER_ALPHA_BLENDING,
    DISP_GET_LAYER_ALPHA_BLENDING,
    DISP_SET_LAYER_SIZE,
    DISP_GET_LAYER_SIZE,
    DISP_SET_LAYER_PITCH,
    DISP_GET_LAYER_PITCH,
    DISP_SET_LAYER_OFFSET,
    DISP_GET_LAYER_OFFSET,
    DISP_SET_LAYER_ROTATION,
    DISP_GET_LAYER_ROTATION,
    DISP_SET_LAYER_SOURCE_KEY,
    DISP_GET_LAYER_SOURCE_KEY,
    DISP_SET_LAYER_3D,
    DISP_GET_LAYER_3D,
    DISP_SET_LAYER_DITHER_EN,
    DISP_GET_LAYER_DITHER_EN,
    DISP_SET_LAYER_DITHER_CONFIG,
    DISP_GET_LAYER_DITHER_CONFIG,    
}DISP_LAYER_CONTROL_CODE_ENUM;
DISP_STATUS DISP_LayerControl(DISP_LAYER_CONTROL_CODE_ENUM code, UINT32 layer_id, void *param, UINT32 *param_len);

DISP_STATUS DISP_ConfigDither(int lrs, int lgs, int lbs, int dbr, int dbg, int dbb);


// Retrieve Information
BOOL   DISP_IsVideoMode(void);
UINT32 DISP_GetScreenWidth(void);
UINT32 DISP_GetScreenHeight(void);
UINT32 DISP_GetActiveWidth(void);
UINT32 DISP_GetActiveHeight(void);
UINT32 DISP_GetScreenBpp(void);
UINT32 DISP_GetPages(void);
DISP_STATUS DISP_SetScreenBpp(UINT32);   ///config how many bits for each pixel of framebuffer
DISP_STATUS DISP_SetPages(UINT32);         ///config how many framebuffer will be used
///above information is used to determine the vRAM size

BOOL   DISP_IsDirectLinkMode(void);
BOOL   DISP_IsInOverlayMode(void);
UINT32 DISP_GetFBRamSize(void);         ///get FB buffer size
UINT32 DISP_GetVRamSize(void);          /// get total RAM size (FB+working buffer+DAL buffer)
PANEL_COLOR_FORMAT DISP_GetPanelColorFormat(void);
UINT32 DISP_GetPanelBPP(void);
BOOL DISP_IsLcmFound(void);
BOOL DISP_IsImmediateUpdate(void);
DISP_STATUS DISP_ConfigImmediateUpdate(BOOL enable);

DISP_STATUS DISP_SetBacklight(UINT32 level);
DISP_STATUS DISP_SetPWM(UINT32 divider);
DISP_STATUS DISP_GetPWM(UINT32 divider, unsigned int *freq);
DISP_STATUS DISP_SetBacklight_mode(UINT32 mode);

DISP_STATUS DISP_Set3DPWM(BOOL enable, BOOL landscape);

// FM De-sense
DISP_STATUS DISP_FMDesense_Query(void);
DISP_STATUS DISP_FM_Desense(unsigned long);
DISP_STATUS DISP_Reset_Update(void);
DISP_STATUS DISP_Get_Default_UpdateSpeed(unsigned int *speed);
DISP_STATUS DISP_Get_Current_UpdateSpeed(unsigned int *speed);
DISP_STATUS DISP_Change_Update(unsigned int);
///////////////

DISP_STATUS DISP_InitM4U(void);
DISP_STATUS DISP_ConfigAssertLayerMva(void);
DISP_STATUS DISP_AllocUILayerMva(unsigned int pa, unsigned int *mva, unsigned int size);
DISP_STATUS DISP_AllocOverlayMva(unsigned int va, unsigned int *mva, unsigned int size);
DISP_STATUS DISP_DeallocMva(unsigned int va, unsigned int mva, unsigned int size);
DISP_STATUS DISP_DumpM4U(void);

// ---------------------------------------------------------------------------
// Private Functions
// ---------------------------------------------------------------------------

typedef struct
{
    DISP_STATUS (*init)(UINT32 fbVA, UINT32 fbPA, BOOL isLcmInited);
    DISP_STATUS (*enable_power)(BOOL enable);
    DISP_STATUS (*update_screen)(BOOL isMuextLocked);

    UINT32 (*get_working_buffer_size)(void);
    UINT32 (*get_working_buffer_bpp)(void);
    PANEL_COLOR_FORMAT (*get_panel_color_format)(void);
    void (*init_te_control)(void);
	UINT32 (*get_dithering_bpp)(void);

	DISP_STATUS (*capture_framebuffer)(unsigned int pvbuf, unsigned int bpp);

    void (*esd_reset)(void);
	BOOL (*esd_check)(void);
} DISP_DRIVER;


const DISP_DRIVER *DISP_GetDriverDBI(void);
const DISP_DRIVER *DISP_GetDriverDPI(void);
const DISP_DRIVER *DISP_GetDriverDSI(void);


BOOL DISP_SelectDevice(const char* lcm_name);
BOOL DISP_DetectDevice(void);
BOOL DISP_SelectDeviceBoot(const char* lcm_name);
UINT32 DISP_GetVRamSizeBoot(char *cmdline);
DISP_STATUS DISP_Capture_Framebuffer(unsigned int pvbuf, unsigned int bpp, unsigned int is_early_suspended);
BOOL DISP_IsContextInited(void);

DISP_STATUS DISP_Capture_Videobuffer(unsigned int pvbuf, unsigned int bpp, unsigned int video_rotation);
UINT32 DISP_GetOutputBPPforDithering(void);
BOOL DISP_IsLCDBusy(void);
DISP_STATUS DISP_ChangeLCDWriteCycle(void);
DISP_STATUS DISP_M4U_On(BOOL enable);
const char* DISP_GetLCMId(void);

BOOL DISP_EsdRecoverCapbility(void);
BOOL DISP_EsdCheck(void);
BOOL DISP_EsdRecover(void);
void DISP_WaitVSYNC(void);
void DISP_InitVSYNC(unsigned int vsync_interval);
DISP_STATUS DISP_PauseVsync(BOOL enable);
DISP_STATUS DISP_Config_Overlay_to_Memory(unsigned int mva, int enable);
void DISP_StartConfigUpdate(void);

unsigned long DISP_GetLCMIndex(void);
// ---------------------------------------------------------------------------



#ifdef __cplusplus
}
#endif

#endif // __DISP_DRV_H__
