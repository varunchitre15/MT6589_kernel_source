
include mediatek/build/Makefile
$(call codebase-path)
$(call relative-include,custom.mk)
$(shell PROJECT=$(MTK_PROJECT) FLAVOR=$(FLAVOR) make -f mediatek/build/custgen.mk > /dev/null)
AEE_FEATURE_POLICY_PATH := $(MTK_PATH_SOURCE)/external/aee/policy
-include $(AEE_FEATURE_POLICY_PATH)/engine.mk

# MediaTek's standard source directories.
SRC_MTK_API_DIR := $(MTK_PATH_SOURCE)/frameworks/api

include $(MTK_PATH_SOURCE)/frameworks-ext/base/pathmap.mk
include $(MTK_PATH_SOURCE)/frameworks/base/mpathmap.mk


PREBUILT_PACKAGE:= $(BUILD_SYSTEM_MTK_EXTENSION)/prebuilt-package.mk
BOARD_CONFIG_DIR ?= $(MTK_ROOT_CONFIG)/$(TARGET_DEVICE)/
ARTIFACT_MODULE :=
ARTIFACT_TARGET := 

ifneq ($(FLAVOR),)
ARTIFACT_DIR := vendor/mediatek/$(TARGET_PRODUCT)[$(FLAVOR)]/artifacts
else
ARTIFACT_DIR := vendor/mediatek/$(TARGET_PRODUCT)/artifacts
endif

ARTIFACT_TARGET_FILE := $(ARTIFACT_DIR)/target.txt
ARTIFACT_COPY_FILE := $(ARTIFACT_DIR)/copy.txt
ARTIFACT_DEFAULT_INSTALLED_HEADERS := 
PARTIAL_BUILD :=

define make-private-dependency
$(eval LOCAL_ADDITIONAL_DEPENDENCIES:=$(call exist-config,$(1)))
endef
define exist-config
$(foreach x, $(1), $(findstring $x,$(call wildcard2,$(BOARD_CONFIG_DIR)/configs/*.mk)))
endef
ifneq (,$(RELEASE_POLICY))
  DO_PUT_ARTIFACTS := yes
endif

#ifneq (,$(call wildcard2,$(ARTIFACT_DIR)/Android.mk))
# full/full.mk is not released for partial source tree, to recognize if we should use artifacts
ifeq (,$(call wildcard2,$(BUILD_SYSTEM_MTK_EXTENSION)/full/config.mk))
ifeq (,$(DO_PUT_ARTIFACTS))
  DO_GET_ARTIFACTS := yes
endif
endif

# define partial build flag
ifeq (,$(call wildcard2,$(BUILD_SYSTEM_MTK_EXTENSION)/full/config.mk.custrel))
PARTIAL_BUILD := true
endif

ifeq ($(PARTIAL_BUILD),true)
NOTICE-HOST-%: ;
NOTICE-TARGET-%: ;
endif

ifneq (,$(DO_PUT_ARTIFACTS))

# 'else' block in policy-path function to deal with 
# "a/b" <---> "a/b/c" case
# "a/b" is configured as a release policy
# "a/b/c" is the current used "LOCAL_PATH"
define policy-path
$(strip \
  $(eval _policy_path := $(call remove-redundant-path-delimeter,$(1))) \
  $(if $(filter $(DIR_WITH_RELEASE_POLICY), $(_policy_path)), \
    $(_policy_path), \
    $(strip \
      $(subst \
        $(word 1, \
               $(filter-out $(_policy_path), \
                 $(foreach item,$(DIR_WITH_RELEASE_POLICY), \
                           $(subst $(item),,$(_policy_path)) \
                  ) \
                ) \
         ) \
         ,, \
         $(_policy_path) \
       ) \
     ) \
   ) \
 )
endef


define remove-redundant-path-delimeter
$(strip \
  $(eval _path := $(subst /, ,$(1)))
  $(eval _full_path :=)
  $(foreach item, $(strip $(_path)), \
    $(eval _full_path += $(item)/) \
   )
  $(patsubst %/,%, \
    $(subst $(space),,$(_full_path)) \
   ) \
 )
endef


#define policy-path
#$(strip $(subst \
#  $(word 1,$(filter-out $(1),$(foreach item,$(DIR_WITH_RELEASE_POLICY),$(subst $(item),,$(1))))),,\
#  $(1) \
#)) \
#endef
define copy-artifact-file
$(2):
	$(hide)if [ -e $(1) ];then \
	mkdir -p $(dir $(2)); \
	cp $(1) $(2);fi
endef

define force-depends-on
$(1): $(2)
endef

define add-install-target
$(shell \
  if [ ! -d $(dir $(1)) ]; then mkdir -p $(dir $(1)); fi; \
  if [ ! -e $(1) ]; then touch $(1); fi; \
  echo $(2) >> $(1) \
 )
endef

ifeq (,$(RELEASE_POLICY))
  RELEASE_POLICY := headquarter
endif

ifneq (1,$(words $(RELEASE_POLICY)))
  $(error Release policy should be one of $(VALID_RELEASE_POLICY), not $(RELEASE_POLICY))
endif

VALID_RELEASE_POLICY := headquarter oversea tier1 other
ifeq (,$(filter $(VALID_RELEASE_POLICY),$(RELEASE_POLICY)))
  $(error Release policy should be one of $(VALID_RELEASE_POLICY), not $(RELEASE_POLICY))
endif

endif # DO_PUT_ARTIFACTS

ifneq (,$(DO_GET_ARTIFACTS))
artifacts_target := $(shell cat $(ARTIFACT_TARGET_FILE) 2>/dev/null)

artifacts_copy := $(shell cat $(ARTIFACT_COPY_FILE) 2>/dev/null)

PV_TOP := external/opencore
PV_INCLUDES := \
        $(PV_TOP)/codecs_v2/audio/gsm_amr/amr_nb/enc/include/ \
        $(PV_TOP)/oscl/oscl/osclbase/src/ \
        $(PV_TOP)/oscl/oscl/config/shared/ \
        $(PV_TOP)/oscl/oscl/config/android/
endif
include $(BUILD_SYSTEM_MTK_EXTENSION)/policy.mk

MTK_INC += $(MTK_PATH_CUSTOM)/cgen/cfgfileinc \
           $(MTK_PATH_CUSTOM)/cgen/cfgdefault \
           $(MTK_PATH_CUSTOM)/cgen/inc \
           $(MTK_PATH_CUSTOM)/hal/inc \
           $(MTK_PATH_CUSTOM)/hal/audioflinger
MTK_CDEFS := $(call mtk.custom.generate-macros)
COMMON_GLOBAL_CFLAGS += $(MTK_CFLAGS) $(MTK_CDEFS)
COMMON_GLOBAL_CPPFLAGS += $(MTK_CPPFLAGS) $(MTK_CDEFS)
SRC_HEADERS += $(MTK_INC)

#.PHONY: mtk-config-files
#*: mtk-config-folder
#mtk-config-folder: mtk-config-files
#mtk-config-files := $(strip $(call mtk.config.generate-rules,mtk-config-files))

# clean spec - some config files are needed during clean. for now we keep generate 
#              custom folder data, and then delete - at least this won't be wrong.

clean: mtk-clean

mtk_dirs_to_clean := \
        $(MTK_ROOT_CUSTOM_OUT) \
        $(MTK_ROOT_CONFIG_OUT)

mtk-clean:
	@for dir in $(mtk_dirs_to_clean) ; do \
	    echo "Cleaning $$dir..."; \
	    rm -rf $$dir; \
	done
dump-comp-build-info:
	@echo Dump componenet level build info.

MTK_INTERNAL_PLATFORM_API_FILE := $(TARGET_OUT_COMMON_INTERMEDIATES)/PACKAGING/mediatek_public_api.txt
JPE_TOOL := $(HOST_OUT_JAVA_LIBRARIES)/jpe_tool.jar
