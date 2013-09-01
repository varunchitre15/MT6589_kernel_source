#ifndef __MTKFB_H
#define __MTKFB_H


#define MTK_FB_OVERLAY_SUPPORT

/* IOCTL commands. */

#define MTK_IOW(num, dtype)     _IOW('O', num, dtype)
#define MTK_IOR(num, dtype)     _IOR('O', num, dtype)
#define MTK_IOWR(num, dtype)    _IOWR('O', num, dtype)
#define MTK_IO(num)             _IO('O', num)

#define MTKFB_FILLRECT          MTK_IOW(0, struct fb_fillrect)
#define MTKFB_COPYAREA          MTK_IOW(1, struct fb_copyarea)
#define MTKFB_IMAGEBLIT         MTK_IOW(2, struct fb_image)

#define MTKFB_TRANSPARENT_BLIT  MTK_IOW(30, struct fb_image)
#define MTKFB_MIRROR            MTK_IOW(31, int)
#define MTKFB_SCALE             MTK_IOW(32, struct fb_scale)
#define MTKFB_SELECT_VIS_FRAME  MTK_IOW(33, int)
#define MTKFB_SELECT_SRC_FRAME  MTK_IOW(34, int)
#define MTKFB_SELECT_DST_FRAME  MTK_IOW(35, int)
#define MTKFB_GET_FRAME_OFFSET  MTK_IOWR(36, struct fb_frame_offset)
#define MTKFB_SYNC_GFX          MTK_IO(37)
//#define MTKFB_VSYNC             MTK_IO(38)
#define MTKFB_LATE_ACTIVATE     MTK_IO(39)
#define MTKFB_SET_UPDATE_MODE   MTK_IOW(40, enum fb_update_mode)
#define MTKFB_UPDATE_WINDOW     MTK_IOW(41, struct fb_update_window)
#define MTKFB_GET_CAPS          MTK_IOR(42, unsigned long)
#define MTKFB_GET_UPDATE_MODE   MTK_IOW(43, enum fb_update_mode)

#define MTKFB_GETVFRAMEPHYSICAL MTK_IOW(44, unsigned long)

#define MTKFB_CAPTURE_FRAMEBUFFER    MTK_IOW(52, unsigned long)
#define MTKFB_CONFIG_IMMEDIATE_UPDATE   MTK_IOW(53, unsigned long)

#ifdef MTK_FB_OVERLAY_SUPPORT
#define MTKFB_SET_OVERLAY_LAYER   MTK_IOW(45, struct fb_overlay_layer)
#define MTKFB_TRIG_OVERLAY_OUT    MTK_IO(46)
#define MTKFB_ENABLE_OVERLAY      MTK_IOW(47, unsigned long)
#define MTKFB_DISABLE_OVERLAY     MTK_IOW(48, unsigned long)
#define MTKFB_WAIT_OVERLAY_READY  MTK_IO(49)
#define MTKFB_GET_OVERLAY_LAYER_COUNT   MTK_IOR(50, unsigned long)
#define MTKFB_SET_VIDEO_LAYERS    MTK_IOW(51, struct fb_overlay_layer)
#define MTKFB_REGISTER_OVERLAYBUFFER   MTK_IOW(55, struct fb_overlay_buffer_info)
#define MTKFB_UNREGISTER_OVERLAYBUFFER  MTK_IOW(56, unsigned int)
#define MTKFB_GET_VIDEOLAYER_SIZE   MTK_IOR(67, struct fb_overlay_layer)
#define MTKFB_CAPTURE_VIDEOBUFFER MTK_IOW(68, unsigned long)
#endif // MTK_FB_OVERLAY_SUPPORT

#define MTKFB_FBLAYER_ENABLE MTK_IOW(79,unsigned int)
// --------------------------------------------------------------------------
// Begin of TVOut IOCTL code

#define MTKFB_SET_ORIENTATION     MTK_IOW(62, unsigned long)
// Video Playback Mode
#define MTKFB_TV_POST_VIDEO_BUFFER         MTK_IOW(63, unsigned long)
#define MTKFB_TV_LEAVE_VIDEO_PLAYBACK_MODE MTK_IOW(64, unsigned long)
// For Factory Mode
#define MTKFB_IS_TV_CABLE_PLUG_IN MTK_IOW(65, unsigned long)


// End of TVOut
// --------------------------------------------------------------------------

#define MTKFB_SET_COMPOSING3D     MTK_IOW(66, unsigned long)
#define MTKFB_SET_S3D_FTM		  MTK_IOW(69, unsigned long)

#define MTKFB_LOCK_FRONT_BUFFER   MTK_IO(70)
#define MTKFB_UNLOCK_FRONT_BUFFER MTK_IO(71)
#define MTKFB_POWERON				MTK_IO(72)
#define MTKFB_POWEROFF				MTK_IO(73)
#define MTKFB_BOOTANIMATION			MTK_IO(74)
#define MTKFB_GETFPS			    MTK_IOW(77, unsigned long)

#define MTKFB_META_RESTORE_SCREEN MTK_IOW(101, unsigned long)
#define MTKFB_VSYNC MTK_IO(102)

#define FBCAPS_GENERIC_MASK     0x00000fff
#define FBCAPS_LCDC_MASK        0x00fff000
#define FBCAPS_PANEL_MASK       0xff000000

#define FBCAPS_MANUAL_UPDATE    0x00001000
#define FBCAPS_SET_BACKLIGHT    0x01000000


#define MAKE_MTK_FB_FORMAT_ID(id, bpp)  (((id) << 8) | (bpp))
//----------------------------------------------------------------------FM De-sense for EM and Normal mode
#define MTKFB_GET_DEFAULT_UPDATESPEED    MTK_IOR(72, unsigned long)
#define MTKFB_GET_CURR_UPDATESPEED       MTK_IOR(73, unsigned long)
#define MTKFB_CHANGE_UPDATESPEED    MTK_IOW(74, unsigned long)   //for EM, not called change writecycle because DPI change pll ckl
#define MTKFB_FM_NOTIFY_FREQ        MTK_IOW(75, unsigned long)  //for Normal mode
#define MTKFB_RESET_UPDATESPEED     MTK_IO(76)
#define MTKFB_GET_INTERFACE_TYPE    MTK_IOR(77, unsigned long)  ///0 DBI, 1 DPI, 2 MIPI
#define MTKFB_GET_POWERSTATE		MTK_IOR(78, unsigned long)  ///0: power off  1: power on
#define MTKFB_SET_UI_LAYER_ALPHA    MTK_IOW(88, unsigned long)
#define MTKFB_SET_UI_LAYER_SRCKEY    MTK_IOW(89, unsigned long)

#define MTKFB_GET_DISPLAY_IF_INFORMATION   MTK_IOR(90, mtk_dispif_info_t)
#define MTKFB_GET_MAX_DISPLAY_COUNT		MTK_IOR(91, unsigned int)
#define MTKFB_AEE_LAYER_EXIST      MTK_IOR(92, unsigned long)
#define MTKFB_GET_OVERLAY_LAYER_INFO    MTK_IOR(93, struct fb_overlay_layer_info)
//----------------------------------------------------------------------
typedef enum
{
    MTK_FB_FORMAT_UNKNOWN = 0,
        
    MTK_FB_FORMAT_RGB565   = MAKE_MTK_FB_FORMAT_ID(1, 2),
	MTK_FB_FORMAT_RGB888   = MAKE_MTK_FB_FORMAT_ID(2, 3),
    MTK_FB_FORMAT_BGR888   = MAKE_MTK_FB_FORMAT_ID(3, 3),
    MTK_FB_FORMAT_ARGB8888 = MAKE_MTK_FB_FORMAT_ID(4, 4),
    MTK_FB_FORMAT_ABGR8888 = MAKE_MTK_FB_FORMAT_ID(5, 4),
	MTK_FB_FORMAT_YUV422   = MAKE_MTK_FB_FORMAT_ID(6, 2),
	MTK_FB_FORMAT_XRGB8888 = MAKE_MTK_FB_FORMAT_ID(7, 4),
	MTK_FB_FORMAT_XBGR8888 = MAKE_MTK_FB_FORMAT_ID(8, 4),
    MTK_FB_FORMAT_BPP_MASK = 0xFF,
} MTK_FB_FORMAT;

#define GET_MTK_FB_FORMAT_BPP(f)    ((f) & MTK_FB_FORMAT_BPP_MASK)

// --------------------------------------------------------------------------

typedef enum
{
    MTK_FB_ORIENTATION_0   = 0,
    MTK_FB_ORIENTATION_90  = 1,
    MTK_FB_ORIENTATION_180 = 2,
    MTK_FB_ORIENTATION_270 = 3,
} MTK_FB_ORIENTATION;


typedef enum
{
    MTK_FB_TV_SYSTEM_NTSC = 0,
    MTK_FB_TV_SYSTEM_PAL  = 1,
} MTK_FB_TV_SYSTEM;


typedef enum
{
    MTK_FB_TV_FMT_RGB565     = 0,
    MTK_FB_TV_FMT_YUV420_SEQ = 1,
    MTK_FB_TV_FMT_UYUV422 =    2,
    MTK_FB_TV_FMT_YUV420_BLK = 3,
} MTK_FB_TV_SRC_FORMAT;

// --------------------------------------------------------------------------

struct fb_scale {
    unsigned int xscale, yscale;
};

struct fb_frame_offset {
    unsigned int idx;
    unsigned long offset;
};

struct fb_update_window {
    unsigned int x, y;
    unsigned int width, height;
};

typedef enum
{
	LAYER_2D 			= 0,
	LAYER_3D_SBS_0 		= 0x1,
	LAYER_3D_SBS_90 	= 0x2,
	LAYER_3D_SBS_180 	= 0x3,
	LAYER_3D_SBS_270 	= 0x4,
	LAYER_3D_TAB_0 		= 0x10,
	LAYER_3D_TAB_90 	= 0x20,
	LAYER_3D_TAB_180 	= 0x30,
	LAYER_3D_TAB_270 	= 0x40,
} MTK_FB_LAYER_TYPE;

struct fb_overlay_layer {
    unsigned int layer_id;
    unsigned int layer_enable;

    void* src_base_addr;
    void* src_phy_addr;
    unsigned int  src_direct_link;
    MTK_FB_FORMAT src_fmt;
    unsigned int  src_use_color_key;
    unsigned int  src_color_key;
    unsigned int  src_pitch;
    unsigned int  src_offset_x, src_offset_y;
    unsigned int  src_width, src_height;

    unsigned int  tgt_offset_x, tgt_offset_y;
    unsigned int  tgt_width, tgt_height;
	MTK_FB_ORIENTATION layer_rotation;
	MTK_FB_LAYER_TYPE	layer_type;
	MTK_FB_ORIENTATION video_rotation;

	unsigned int isTdshp;  // set to 1, will go through tdshp first, then layer blending, then to color

    int next_buff_idx;
    int identity;
    int connected_type;
    unsigned int security;
};

struct fb_overlay_buffer_info{
	unsigned int src_vir_addr;
	unsigned int size;
};

struct fb_overlay_layer_info {
    unsigned int layer_id;
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
};

// --------------------------------------------------------------------------

struct fb_post_video_buffer {
    void*                phy_addr;
    void*                vir_addr;
    MTK_FB_TV_SRC_FORMAT format;
    unsigned int         width, height;
};

// --------------------------------------------------------------------------
#ifndef MT65XX_NEW_DISP
#define HW_OVERLAY_COUNT     (6)
// Top layer is assigned to Debug Layer
// Second layer is assigned to UI
// Third layer is assigned to FD
#define RESERVED_LAYER_COUNT (3)
#define VIDEO_LAYER_COUNT    (HW_OVERLAY_COUNT - RESERVED_LAYER_COUNT)
#define FACE_DETECTION_LAYER_ID  (3)
#else
#define HW_OVERLAY_COUNT     (4)
// Top layer is assigned to Debug Layer
// Second layer is assigned to UI
#define RESERVED_LAYER_COUNT (2)
#define VIDEO_LAYER_COUNT    (HW_OVERLAY_COUNT - RESERVED_LAYER_COUNT)
#endif

#ifdef __KERNEL__

#include <linux/completion.h>
#include <linux/interrupt.h>

#define MTKFB_DRIVER "mtkfb"

enum mtkfb_state {
    MTKFB_DISABLED  = 0,
    MTKFB_SUSPENDED = 99,
    MTKFB_ACTIVE    = 100
};

typedef enum {
    MTKFB_LAYER_ENABLE_DIRTY = (1 << 0),
    MTKFB_LAYER_FORMAT_DIRTY = (1 << 1),
    MTKFB_LAYER_SET_DIRTY    = (1 << 2),
} MTKFB_LAYER_CONFIG_DIRTY;

struct mtkfb_device {
    int             state;
    void           *fb_va_base;             /* MPU virtual address */
    dma_addr_t      fb_pa_base;             /* Bus physical address */
    unsigned long   fb_size_in_byte;

    unsigned long   layer_enable;
    MTK_FB_FORMAT   layer_format[HW_OVERLAY_COUNT];
    unsigned int    layer_config_dirty;

    int             xscale, yscale, mirror; /* transformations.
                                               rotate is stored in fb_info->var */
    u32             pseudo_palette[17];

    struct fb_info *fb_info;                /* Linux fbdev framework data */
    struct device  *dev;
};

#endif /* __KERNEL__ */

#endif /* __MTKFB_H */
