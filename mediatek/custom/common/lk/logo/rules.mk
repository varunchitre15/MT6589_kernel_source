LOCAL_DIR := $(GET_LOCAL_DIR)
BOOT_LOGO_DIR := $(LOCAL_DIR)
#$(info lk/logo/dir=$(LOCAL_DIR),builddir=$(BUILDDIR))

BMP_TO_RAW := $(BOOT_LOGO_DIR)/tool/bmp_to_raw
ZPIPE := $(BOOT_LOGO_DIR)/tool/zpipe


BOOT_LOGO_RESOURCE := $(BUILDDIR)/$(BOOT_LOGO_DIR)/$(BOOT_LOGO).raw
LOGO_IMAGE := $(BUILDDIR)/logo.bin
BOOT_LOGO_IMAGE := $(LOCAL_DIR)/../../../../common/lk/logo/boot_logo
RESOURCE_OBJ_LIST :=   \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_uboot.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_battery.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_low_battery.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_charger_ov.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_0.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_1.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_2.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_3.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_4.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_5.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_6.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_7.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_8.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_9.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_num_percent.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_01.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_02.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_03.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_04.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_05.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_06.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_07.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_08.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_09.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_animation_10.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_01.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_02.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_03.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_04.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_05.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_06.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_07.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_08.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_09.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_10_10.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_bg.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_img.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_bat_100.raw \
            $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_kernel.raw

GENERATED += \
            $(BOOT_LOGO_RESOURCE) \
            $(LOGO_IMAGE) \
            $(BOOT_LOGO_IMAGE) \
            $(addprefix $(BUILDDIR)/,$(RESOURCE_OBJ_LIST))


all:: $(LOGO_IMAGE) $(BOOT_LOGO_IMAGE)

$(LOGO_IMAGE):$(MKIMG) $(BOOT_LOGO_RESOURCE)
	$(NOECHO) if [ ! -x $(MKIMG) ]; then chmod a+x $(MKIMG); fi
	@echo "MKING $(LOGO_IMAGE)"
	$(MKIMG) $(BOOT_LOGO_RESOURCE) LOGO > $(LOGO_IMAGE)

$(BOOT_LOGO_RESOURCE): $(addprefix $(BUILDDIR)/,$(RESOURCE_OBJ_LIST)) $(ZPIPE)
	@$(MKDIR)
	$(NOECHO) if [ ! -x $(ZPIPE) ]; then chmod a+x $(ZPIPE); fi
	@echo "zpiping "
	$(ZPIPE) -l 9 $@ $(addprefix $(BUILDDIR)/,$(RESOURCE_OBJ_LIST))


$(BUILDDIR)/%.raw: %.bmp $(BMP_TO_RAW)
	@$(MKDIR)
	$(NOECHO) if [ ! -x $(BMP_TO_RAW) ]; then chmod a+x $(BMP_TO_RAW); fi
	@echo "Compiling_BMP_TO_RAW $<"
	$(BMP_TO_RAW) $@ $<

$(BOOT_LOGO_IMAGE): $(BMP_TO_RAW)
	@$(MKDIR)
	$(NOECHO) if [ ! -x $(BMP_TO_RAW) ]; then chmod a+x $(BMP_TO_RAW); fi
	@echo "Compiling_BMP_TO_RAW_BOOT_LOGO"
	$(BMP_TO_RAW) $(BOOT_LOGO_IMAGE) $(BOOT_LOGO_DIR)/$(BOOT_LOGO)/$(BOOT_LOGO)_kernel.bmp
