# please see binary/Makefile for usage


obj_to_install := \
 ../mediatek/platform/mt6516/kernel/core:MT6516_PM_api.o \
 ../mediatek/platform/mt6516/kernel/core:mt6516_intr.o \
 ../mediatek/platform/mt6516/kernel/core:MT6516_sleep.o

ifeq ($(MTK_WAPI_SUPPORT),yes)
obj_to_install +=  ../mediatek/source/kernel/drivers/net/mt592x/wlan:gl_sec.o \

endif

file_to_touch := \
  arch/arm/mach-mt3351/Kconfig \
  arch/arm/mach-mt3351/Makefile \

