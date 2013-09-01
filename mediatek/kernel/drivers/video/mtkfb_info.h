#ifndef __MTKFB_INFO_H__
#define __MTKFB_INFO_H__

#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    DISPIF_TYPE_DBI = 0,
    DISPIF_TYPE_DPI,
    DISPIF_TYPE_DSI
} MTKFB_DISPIF_TYPE;

typedef enum
{
    MTKFB_DISPIF_PRIMARY_LCD = 0,
    MTKFB_DISPIF_HDMI,
    MTKFB_MAX_DISPLAY_COUNT
} MTKFB_DISPIF_DEVICE_TYPE;

typedef enum
{
    DISPIF_FORMAT_RGB565 = 0,
    DISPIF_FORMAT_RGB666,
    DISPIF_FORMAT_RGB888 
} MTKFB_DISPIF_FORMAT;


typedef enum
{
    DISPIF_MODE_VIDEO = 0,
    DISPIF_MODE_COMMAND
} MTKFB_DISPIF_MODE;

typedef struct mtk_dispif_info {
	unsigned int display_id;
	unsigned int isHwVsyncAvailable;
	MTKFB_DISPIF_TYPE displayType;
	unsigned int displayWidth;
	unsigned int displayHeight;
	unsigned int displayFormat;
	MTKFB_DISPIF_MODE displayMode;
	unsigned int vsyncFPS;
	unsigned int xDPI;
	unsigned int yDPI;
	unsigned int isConnected;
} mtk_dispif_info_t;

#ifdef __cplusplus
}
#endif

#endif // __DISP_DRV_H__

