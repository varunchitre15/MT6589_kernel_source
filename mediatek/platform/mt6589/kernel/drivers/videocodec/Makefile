vcodec_kernel_driver-objs :=        \
    videocodec_kernel_driver.o      \
    common/val/src/val.o			\
    common/drv/src/drv_base.o


obj-$(CONFIG_MTK_VIDEOCODEC_DRIVER) += vcodec_kernel_driver.o


ccflags-y += \
    -I$(MTK_PATH_PLATFORM)/drivers/videocodec/inc					\
    -I$(MTK_PATH_PLATFORM)/drivers/videocodec/common/val/inc		\
    -I$(MTK_PATH_PLATFORM)/drivers/videocodec/common/hal/inc		\
    -I$(MTK_PATH_PLATFORM)/drivers/videocodec/common/drv/inc		\
    -I$(MTK_PATH_PLATFORM)/../../../kernel/include/linux/vcodec			\
    -I$(MTK_PATH_PLATFORM)/../../../kernel/include/linux/vcodec/kernel			\
    -I$(MTK_PATH_PLATFORM)/../../mt6589/kernel/core/include/mach	\
    -I$(MTK_PATH_PLATFORM)/../../mt6589/kernel/core/include			\
