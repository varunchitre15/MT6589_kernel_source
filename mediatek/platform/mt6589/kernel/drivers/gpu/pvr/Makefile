OPTIM = -Os
VER:=eng

ccflags-y := \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/include4 \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/include \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/system/include \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/srvkm/hwdefs \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/srvkm/bridged \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/srvkm/bridged/sgx \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/srvkm/common \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/srvkm/devices/sgx \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/srvkm/env/linux \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/srvkm/include \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/3rdparty/mtklfb \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/3rdparty/mtkbc \
	-I$(MTK_PATH_PLATFORM)/drivers/gpu/pvr/services4/system/mt6589

ccflags-y += \
	-fno-strict-aliasing \
	-W -Wall \
	-Wno-missing-field-initializers \
	-Wmissing-format-attribute \
	-Wdeclaration-after-statement \
	-Wno-format-zero-length \
	-Wmissing-prototypes \
	-Wstrict-prototypes \
	-Wno-unused-parameter \
	-Wno-sign-compare \
	-Wno-type-limits

ccflags-y += \
	-D__linux__ $(OPTIM) -g

ifneq ($(strip $(TARGET_BUILD_VARIANT)),)
ifneq ($(strip $(TARGET_BUILD_VARIANT)),eng)
ifneq ($(strip $(MTK_3D_DEBUG_FOR_USER_VERSION)),yes)
VER:=user
endif
endif
endif

ccflags-$(CONFIG_PVR_BUILD_RELEASE_MT6589) += \
	-include $(MTK_PATH_PLATFORM)/drivers/gpu/pvr/include4/config_kernel_$(VER).h
ccflags-$(CONFIG_PVR_BUILD_DEBUG_MT6589) += \
	-include $(MTK_PATH_PLATFORM)/drivers/gpu/pvr/include4/config_kernel_$(VER).h

ifeq ($(MTK_FORCE_GPU_89M), yes)
ccflags-y += -DMTK_FORCE_M
endif

pvrsrvkm-y := \
	services4/srvkm/env/linux/osfunc.o \
	services4/srvkm/env/linux/mutils.o \
	services4/srvkm/env/linux/mmap.o \
	services4/srvkm/env/linux/module.o \
	services4/srvkm/env/linux/pdump.o \
	services4/srvkm/env/linux/proc.o \
	services4/srvkm/env/linux/pvr_bridge_k.o \
	services4/srvkm/env/linux/pvr_debug.o \
	services4/srvkm/env/linux/mm.o \
	services4/srvkm/env/linux/mutex.o \
	services4/srvkm/env/linux/event.o \
	services4/srvkm/env/linux/osperproc.o \
	services4/srvkm/common/buffer_manager.o \
	services4/srvkm/common/devicemem.o \
	services4/srvkm/common/deviceclass.o \
	services4/srvkm/common/handle.o \
	services4/srvkm/common/hash.o \
	services4/srvkm/common/metrics.o \
	services4/srvkm/common/pvrsrv.o \
	services4/srvkm/common/queue.o \
	services4/srvkm/common/ra.o \
	services4/srvkm/common/resman.o \
	services4/srvkm/common/power.o \
	services4/srvkm/common/mem.o \
	services4/srvkm/common/pdump_common.o \
	services4/srvkm/bridged/bridged_support.o \
	services4/srvkm/bridged/bridged_pvr_bridge.o \
	services4/srvkm/common/perproc.o \
	services4/srvkm/common/lists.o \
	services4/srvkm/common/mem_debug.o \
	services4/srvkm/common/osfunc_common.o \
	services4/srvkm/bridged/sgx/bridged_sgx_bridge.o \
	services4/srvkm/devices/sgx/sgxinit.o \
	services4/srvkm/devices/sgx/sgxpower.o \
	services4/srvkm/devices/sgx/sgxreset.o \
	services4/srvkm/devices/sgx/sgxutils.o \
	services4/srvkm/devices/sgx/sgxkick.o \
	services4/srvkm/devices/sgx/sgxtransfer.o \
	services4/srvkm/devices/sgx/mmu.o \
	services4/srvkm/devices/sgx/pb.o \
    services4/system/mt6589/sysconfig.o \
	services4/system/mt6589/sysutils.o \
    services4/srvkm/env/linux/mtk_debug.o \
	services4/srvkm/common/ttrace.o \
	services4/system/mt6589/mtk_sysfreq.o

pvrsrvkm-$(CONFIG_ION_MTK) += \
    services4/srvkm/env/linux/ion.o

ifeq ($(SUPPORT_ION),1)
pvrsrvkm-y += \
    services4/srvkm/env/linux/ion.o
endif

mtklfb-y := \
	services4/3rdparty/mtklfb/mtklfb_displayclass.o \
	services4/3rdparty/mtklfb/mtklfb_linux.o \
	services4/3rdparty/mtklfb/mtklfb_debug.o

obj-$(CONFIG_PVR_SGX_MT6589) += pvrsrvkm.o
obj-$(CONFIG_PVR_SGX_MT6589) += mtklfb.o


