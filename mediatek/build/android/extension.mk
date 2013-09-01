

force: ;
$(INSTALLED_BUILD_PROP_TARGET): force
droid: make-platform
make-platform: droidcore
	@mediatek/build/android/make-platform.sh $(PRODUCT_OUT) $(TARGET_PRODUCT)
#	@mediatek/build/tools/mkimage $(PRODUCT_OUT)/ramdisk.img ROOTFS > $(PRODUCT_OUT)/android_rootfs.bin
#	@mediatek/build/tools/mkimage $(PRODUCT_OUT)/ramdisk-recovery.img RECOVERY > $(PRODUCT_OUT)/android_rootfs_recovery.bin
#	@mediatek/build/tools/mkimage $(PRODUCT_OUT)/ramdisk-factory.img ROOTFS > $(PRODUCT_OUT)/factory.bin

##############################################################################
# FOR RELEASE POLICY                                                         #
##############################################################################
# GENERATE ARTIFACTS                                                         #
##############################################################################
ifneq (,$(DO_PUT_ARTIFACTS))

ARTIFACT_TARGET := $(sort $(strip $(ARTIFACT_TARGET)))
ARTIFACT_MODULE := $(sort $(strip $(ARTIFACT_MODULE)))

# write targets for installation to a file
$(shell if [ -e $(ARTIFACT_TARGET_FILE) ]; then rm -f $(ARTIFACT_TARGET_FILE); fi)
$(foreach item,$(ARTIFACT_TARGET),\
  $(eval $(call add-install-target,$(ARTIFACT_TARGET_FILE),$(item))) \
)

# don't know the impact to mark this line to "other"... need try
#ARTIFACT_MODULE := $(filter $(ALL_DEFAULT_INSTALLED_MODULES),$(ARTIFACT_MODULE))
ARTIFACT_MODULE := $(foreach item,$(ARTIFACT_MODULE),$(ARTIFACT.$(item).SRC))
ARTIFACT_FILES  := $(strip $(sort $(foreach item,$(ARTIFACT_MODULE),$(ARTIFACT.$(item).FILES))))
#$(info ---$(ARTIFACT_FILES)---)

# write copy for installation to a file
$(shell if [ -e $(ARTIFACT_COPY_FILE) ]; then rm -f $(ARTIFACT_COPY_FILE); fi)
$(foreach item,$(ARTIFACT_FILES),\
  $(eval $(call add-install-target,$(ARTIFACT_COPY_FILE),$(item))) \
)

#ALL_DEFAULT_INSTALLED_MODULES += $(ARTIFACT_COPY_FILE)
#ALL_DEFAULT_INSTALLED_MODULES += $(ARTIFACT_TARGET_FILE)
#ALL_DEFAULT_INSTALLED_MODULES += $(ARTIFACT_DEFAULT_INSTALLED_HEADERS)


# generate artifacts
$(foreach artifacts,$(ARTIFACT_FILES), \
  $(eval _src := $(call word-colon,1,$(artifacts))) \
  $(eval _des := $(call word-colon,2,$(artifacts))) \
  $(eval $(call copy-one-file,$(_src),$(_des)))    \
  $(eval ALL_DEFAULT_INSTALLED_MODULES += $(_des)) \
)

# generate the switch artifacts when add RELEASE_POLICY
include $(BUILD_SYSTEM_MTK_EXTENSION)/switch.mk

endif

##############################################################################
# INSTALL VIA ARTIFACTS                                                      #
##############################################################################
ifneq (,$(DO_GET_ARTIFACTS))

# some target in artifacts_target has been in artifacts_copy 
artifacts_target := $(sort $(artifacts_target))
$(foreach item,$(artifacts_target), \
  $(eval _src := $(call word-colon,1,$(item))) \
  $(eval _des := $(call word-colon,2,$(item))) \
  $(eval $(call add-dependency,$(_src),$(_des))) \
)
artifacts_copy := $(sort $(artifacts_copy))
$(foreach item,$(artifacts_copy), \
  $(eval _src := $(call word-colon,2,$(item))) \
  $(eval _des := $(call word-colon,1,$(item))) \
  $(eval $(call copy-one-file,$(_src),$(_des))) \
  $(eval ALL_DEFAULT_INSTALLED_MODULES += $(_des)) \
)
# switch artifacts
ifneq (generic,$(TARGET_PRODUCT))
include $(BUILD_SYSTEM_MTK_EXTENSION)/switch.mk
$(foreach item,$(SWITCH_DIRECTORY), \
   $(eval _src := $(call word-colon,2,$(item))) \
   $(eval _des := $(call word-colon,1,$(item))) \
   $(eval $(shell if [ -e $(_des) ];then rm -rf $(_des);fi)) \
   $(eval _result := $(shell cp -a $(_src) $(_des) > /dev/null 2>&1;echo $$?)) \
   $(eval $(if $(filter 0,$(_result)),,$(error the switch cp has a wrong!))) \
 )
endif #TARGET_PRODUCT
endif

$(ARTIFACT_DIR)/cls:
	@mkdir -p $(ARTIFACT_DIR)/cls

.PHONY: FORCE
FORCE:
