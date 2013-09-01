#
# Makefile for the Silicon Image 835x MHL TX device driver
#

DRIVER_NAME := sii8338drv_rgb

#AMF_SRC:= yes

ifneq ($(KERNELRELEASE),)

ccflags-y := -I$(src)/include
ccflags-y += -I$(src)/platform/hal


ifeq ($(AMF_SRC),yes)
ccflags-y += -I$(src)/platform/amf
ccflags-y += -I$(src)/platform/amf/AMF_LIB_Src
ccflags-y += -I$(src)/platform/amf/AMF_LIB_Src/HAL
ccflags-y += -I$(src)/platform/amf/AMF_LIB_Src/DEBUG
ccflags-y += -I$(src)/platform/amf/AMF_LIB_Src/CEC
endif

ccflags-y += -I$(src)/../component/mhl_tx
ccflags-y += -I$(src)/../driver/cra_drv
ccflags-y += -I$(src)/../driver/mhl_tx_drv
ccflags-y += -I$(src)/../driver/sii8338_drv
ccflags-y += -I$(src)/linux_kernel/kernel_module

ccflags-y += -DMAKE_8338_DRIVER

ccflags-y += -DRGB_BOARD #

ifeq ($(AMF_SRC),yes)
ccflags-y += -DAMF_SRC
endif

ccflags-$(CONFIG_DEBUG_DRIVER) += -DDEBUG

obj-y += $(DRIVER_NAME).o

#----------------- App layer --------------------------------------
$(DRIVER_NAME)-objs  += linux_kernel/kernel_module/mhl_linuxdrv_main.o
$(DRIVER_NAME)-objs  += linux_kernel/public/mhl_linuxdrv_ioctl.o
$(DRIVER_NAME)-objs  += linux_kernel/public/mhl_linuxdrv_open_close.o
#----------------- Component layer --------------------------------
$(DRIVER_NAME)-objs  += ../component/mhl_tx/si_mhl_tx.o
$(DRIVER_NAME)-objs  += ../component/mhl_tx/si_mhl_tx_rcp_table.o
#----------------- Platform layer ---------------------------------
$(DRIVER_NAME)-objs  += platform/osal/sii_osal_linux_timer.o
$(DRIVER_NAME)-objs  += platform/osal/si_osdebug.o
$(DRIVER_NAME)-objs  += platform/hal/sii_hal_linux_init.o
$(DRIVER_NAME)-objs  += platform/hal/sii_hal_linux_i2c.o
$(DRIVER_NAME)-objs  += platform/hal/sii_hal_linux_isr.o
$(DRIVER_NAME)-objs  += platform/hal/sii_hal_linux_timer.o
$(DRIVER_NAME)-objs  += platform/hal/sii_hal_linux_gpio.o
#----------------- Driver layer --------------------------------
$(DRIVER_NAME)-objs  += ../driver/cra_drv/si_cra.o
$(DRIVER_NAME)-objs  += ../driver/sii8338_drv/si_cra_cfg.o
#----------------- MHL layer --------------------------------
$(DRIVER_NAME)-objs  += ../driver/mhl_tx_drv/si_drv_mhl_tx_edid.o
#$(DRIVER_NAME)-objs  += ../driver/mhl_tx_drv/si_drv_mhl_rgb.o
#$(DRIVER_NAME)-objs  += ../driver/mhl_tx_drv/si_drv_ext.o
$(DRIVER_NAME)-objs  += ../driver/mhl_tx_drv/si_drv_mhl_tx.o
$(DRIVER_NAME)-objs  += hdmi_drv.o

ifeq ($(AMF_SRC),yes)	
#----------------- AMF RGB  board Control layer ---------------------------------
$(DRIVER_NAME)-objs  += platform/amf/AMF_LIB_Src/amf_timers.o
$(DRIVER_NAME)-objs  += platform/amf/AMF_LIB_Src/amf_regio.o
$(DRIVER_NAME)-objs  += platform/amf/AMF_LIB_Src/amf_interrupt.o
$(DRIVER_NAME)-objs  += platform/amf/AMF_LIB_Src/amf_EDID.o
$(DRIVER_NAME)-objs  += platform/amf/AMF_LIB_Src/HAL/amf_hal_cp9233.o
$(DRIVER_NAME)-objs  += platform/amf/AMF_LIB_Src/amf_main.o
endif

else
# Normal Makefile
WORK_DIR := /opt/WorkDir/sii8338
KERNELDIR := /usr/src/linux-2.6.39

COMPILE_TOOLS:=/usr/local/angstrom/arm/bin
CROSS_COMPILE :=arm-angstrom-linux-gnueabi-

PWD := $(shell pwd)
PATH := $(COMPILE_TOOLS):$(PATH)

CFLAGS := ARCH=arm CROSS_COMPILE=arm-angstrom-linux-gnueabi- \
-C $(KERNELDIR) M=$(PWD) CONFIG_SII835X_MHL_TX=m 

.PHONY: sii8338_clean

sii8338_release:
	make $(CFLAGS) modules
	$(CROSS_COMPILE)strip --strip-debug $(DRIVER_NAME).ko
	cp -rvf $(DRIVER_NAME).ko $(WORK_DIR)
sii8338_debug:
	make $(CFLAGS) CONFIG_DEBUG_DRIVER=y modules
	cp -rvf $(DRIVER_NAME).ko $(WORK_DIR)
sii8338_clean:
	find .. \( -name '*.o' -o -name '*.ko' -o -name '*.cmd' -o -name '*.mod.c' -o -name '*.depend' -o -name '*.d' \) -exec rm -f {} \;
endif
