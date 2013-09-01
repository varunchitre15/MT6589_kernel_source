# Used for UBOOT
#DST_FOLDER = bootable/bootloader/uboot/include/asm-arm/arch-arm926ejs/custom
DST_FOLDER = bootable/bootloader/uboot/arch/arm/include/asm/arch-mt6516/custom
#ifeq ($(MTK_PLATFORM), MT6516)
#  DST_FOLDER = bootable/bootloader/uboot/arch/arm/include/asm/arch-mt6516/custom
#  else
#    ifeq ($(MTK_PLATFORM), MT6573)
#      DST_FOLDER = bootable/bootloader/uboot/arch/arm/include/asm/arch-mt6573/custom
#    else
#      DST_FOLDER = bootable/bootloader/uboot/arch/arm/include/asm/arch-$(MTK_PLATFORM)/custom
#    endif
#  endif
#endif
#ANDROID_MK_DIR = external/mediatek/mhal/src/custom
