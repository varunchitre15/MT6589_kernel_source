# drivers/video/mtk/Makefile
#
# Makefile for MT6516 video driver.
#

obj-$(CONFIG_MTK_FB) += disp_drv_dbi.o              \
         disp_drv_dpi.o                             \
				 disp_drv_dsi.o															\
         disp_drv.o                                 \
         mtkfb_console.o                            \
         disp_assert_layer.o                        \
         mtkfb.o                                    \
		 mtkfb_vsync.o								\
         debug.o                                    \
         font_8x16.o                                \
         tv_out.o                                   \
         mtkfb_proc.o

ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/video/  \
             -I$(MTK_PATH_SOURCE)/drivers/video/    \
			 -I$(MTK_PATH_PLATFORM)/drivers/dispsys/

ifeq ($(MTK_DITHERING_SUPPORT), yes)
ccflags-y += -DDITHERING_SUPPORT
endif

ifeq ($(MTK_GPU_SUPPORT), yes)
ccflags-y += -DHWGPU_SUPPORT
endif
