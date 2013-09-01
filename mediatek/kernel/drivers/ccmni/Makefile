ifneq ($(CONFIG_MTK_CCCI_EXT), y)
include $(MTK_ROOT_BUILD)/Makefile
#CCCI_PLATFORM_DRIVER_DIR :=  $(wildcard $(obj)/$(call to-root,$(obj))mediatek/platform/$(call lc,$(MTK_PLATFORM))/kernel/drivers/ccci)
CCCI_PLATFORM_DRIVER_DIR :=  $(srctree)/mediatek/platform/$(call lc,$(MTK_PLATFORM))/kernel/drivers/ccci
ifeq ($(CCCI_PLATFORM_DRIVER_DIR),)
$(error Invalid ccci platform path, please check)
endif
ifeq (y,$(CONFIG_MTK_NET_CCMNI))
obj-m	+= ccmni.o
ccmni-$(CONFIG_MTK_NET_CCMNI) := ccmni_pfp.o ccmni_net.o
ccflags-y += -I$(MTK_PATH_SOURCE)/drivers/ccci/include -I$(CCCI_PLATFORM_DRIVER_DIR)\
		 -I$(CCCI_PLATFORM_DRIVER_DIR)/include
else
obj-n += dummy.o
endif

else 
obj- :=dummy.o
endif

