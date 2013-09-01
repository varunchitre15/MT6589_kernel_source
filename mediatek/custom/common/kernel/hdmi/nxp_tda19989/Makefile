# drivers/video/mtk/Makefile
#
# Makefile for MT6516 video driver.
#
ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/hdmitx/  \
             -I$(MTK_PATH_SOURCE)/drivers/hdmitx/	\
			 -I$(MTK_PATH_PLATFORM)/drivers/video/  \
             -I$(MTK_PATH_SOURCE)/drivers/video/	\
			-DTMFL_TDA19989 -DTMFL_NO_RTOS -DTMFL_LINUX_OS_KERNEL_DRIVER

obj-y += 						\
	hdmi_drv.o					\
	tmbslTDA9989_HDCP.o			\
	tmbslTDA9989_local.o		\
	tmbslTDA9989_State.o		\
	tmdlHdmiTx_local.o			\
	tmbslTDA9989_Edid.o			\
	tmbslTDA9989_InOut.o		\
	tmbslTDA9989_Misc.o			\
	tmdlHdmiTx.o				\
	tmdlHdmiTx_cfg.o			\
	I2C_api.o					\

