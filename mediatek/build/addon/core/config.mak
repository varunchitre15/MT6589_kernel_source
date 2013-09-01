# include project feature configuration makefile
# ############################################################
# maybe enable in the future
# ############################################################
#include $(MTK_PROJECT_CONFIG)/$(PROJECT).mak

include mediatek/build/addon/core/definitions.mak
MTK_BUILD_SYSTEM := mediatek/build/addon/core
# ###############################################################
# Build System internal files
# ###############################################################

COMMON_DEFS_CONFIG           := $(MTK_BUILD_SYSTEM)/common_defs.mak
COMMON_DEP_RULE_CONFIG       := $(MTK_BUILD_SYSTEM)/common_dep_rule.mak
KERNEL_COMMON_DEFS_CONFIG    := $(MTK_BUILD_SYSTEM)/kernel_defs.mak
KERNEL_DEP_RULE_CONFIG       := $(MTK_BUILD_SYSTEM)/kernel_dep_rule.mak
ANDROID_COMMON_DEFS_CONFIG   := $(MTK_BUILD_SYSTEM)/android_defs.mak
ANDROID_DEP_RULE_CONFIG      := $(MTK_BUILD_SYSTEM)/android_dep_rule.mak
UBOOT_COMMON_DEFS_CONFIG     := $(MTK_BUILD_SYSTEM)/uboot_defs.mak
UBOOT_DEP_RULE_CONFIG        := $(MTK_BUILD_SYSTEM)/uboot_dep_rule.mak
PRELOADER_COMMON_DEFS_CONFIG := $(MTK_BUILD_SYSTEM)/preloader_defs.mak
PRELOADER_DEP_RULE_CONFIG    := $(MTK_BUILD_SYSTEM)/preloader_dep_rule.mak


INCLUDE_MAKEFILES := COMMON_DEP_RULE_CONFIG \
                     KERNEL_DEP_RULE_CONFIG \
                     ANDROID_DEP_RULE_CONFIG \
                     UBOOT_DEP_RULE_CONFIG \
                     PRELOADER_DEP_RULE_CONFIG

ifneq (,$(CUR_MODULE))
INCLUDE_MAKEFILES := $(filter $(call lower-to-upper,$(CUR_MODULE))_%_CONFIG COMMON_%_CONFIG,$(INCLUDE_MAKEFILES))
endif

INCLUDE_MAKEFILES := $(foreach file,$(INCLUDE_MAKEFILES),$($(file)))

include $(INCLUDE_MAKEFILES)

# ###############################################################
# include defs,include path, dep. rule configuration files
# ###############################################################

include $(COMMON_DEFS_CONFIG)
#include $(COMMON_DEP_RULE_CONFIG)
include $(KERNEL_COMMON_DEFS_CONFIG)
#include $(KERNEL_DEP_RULE_CONFIG)
include $(ANDROID_COMMON_DEFS_CONFIG)
#include $(ANDROID_DEP_RULE_CONFIG)
include $(UBOOT_COMMON_DEFS_CONFIG)
#include $(UBOOT_DEP_RULE_CONFIG)
include $(PRELOADER_COMMON_DEFS_CONFIG)
#include $(PRELOADER_DEP_RULE_CONFIG)


