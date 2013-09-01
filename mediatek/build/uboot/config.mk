include ../../../mediatek/build/Makefile
$(call codebase-path,uboot)

# mkconfig ARCH CPU BOARD VENDOR SOC

UBOOT_CHIP_CONFIG    := $(call lc,$(MTK_PLATFORM))
UBOOT_BOARD_CONFIG   := $(MTK_PROJECT)
# move it to global config file
UBOOT_STORAGE_CONFIG := "CFG_NAND_BOOT"
ifeq ($(MTK_EMMC_SUPPORT),yes)
 UBOOT_STORAGE_CONFIG := "CFG_MMC_BOOT"
endif

UBOOT_CPU_CONFIG     :=$(MTK_CPU)

.PHONY: $(MTK_PROJECT)_config

$(MTK_PROJECT)_config: $(UBOOT_CHIP_CONFIG)_config
$(UBOOT_CHIP_CONFIG)_config : unconfig	
	@chmod a+x $(MKCONFIG)
	@$(MKCONFIG) $(UBOOT_BOARD_CONFIG) arm $(UBOOT_CPU_CONFIG) $(UBOOT_BOARD_CONFIG) NULL $(UBOOT_CHIP_CONFIG)
	echo "#define ${UBOOT_STORAGE_CONFIG}" >> include/config.h		
	echo "#define CFG_BOARD \"${UBOOT_BOARD_CONFIG}\"" >> include/config.h

MEDIATEK_CLEAN := -name '*.o' -o -name '.depend' -o -name '*.a' -o -name 'core' -o -name '*.bak' -o -name '*~' -o -name '*.exe' -o -name '*.srec' -o -name '*.bin' | grep -v "prebuilt/"
clean: mediatek-clean
mediatek-clean:
	@if [ -d $(MTK_PATH_SOURCE) ];   then find $(MTK_PATH_SOURCE)   $(MEDIATEK_CLEAN) | xargs rm -f; fi
	@if [ -d $(MTK_PATH_PLATFORM) ]; then find $(MTK_PATH_PLATFORM) $(MEDIATEK_CLEAN) | xargs rm -f; fi
	@if [ -d $(MTK_PATH_CUSTOM) ];   then find $(MTK_PATH_CUSTOM)   $(MEDIATEK_CLEAN) | xargs rm -f; fi
