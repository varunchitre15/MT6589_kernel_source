# drivers/video/mtk/Makefile
#
# Makefile for MT6516 video driver.
#

ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/hdmitx/  \
             -I$(MTK_PATH_SOURCE)/drivers/hdmitx/    \
             -I$(MTK_PATH_PLATFORM)/drivers/video/   \
             -I$(MTK_PATH_SOURCE)/drivers/video/     \
             -I$(MTK_PATH_PLATFORM)/drivers/hdmitx/  \
             -I$(MTK_PATH_SOURCE)/drivers/hdmitx/    \
             -I$(MTK_PATH_PLATFORM)/drivers/dispsys/

ifeq ($(CUSTOM_KERNEL_HDMI),nxp_tda19989)
	ccflags-y += 	-DHDMI_TDA19989
endif
obj-y	:= hdmitx.o debug.o wfd.o hdmitx_test.o




