

# include release policy
RELEASE_FILE := $(BUILD_SYSTEM_MTK_EXTENSION)/release_file.mk
include $(RELEASE_FILE)

# word-colon not defined here; use our version
define _word-colon
$(word $(1),$(subst :,$(space),$(2)))
endef

$(foreach item,$(_release_policy), \
    $(eval _dir := $(call _word-colon,1,$(item))) \
    $(eval _policy := $(subst ;, ,$(call _word-colon,2,$(item)))) \
    $(eval $(_dir).RELEASE_POLICY := headquarter $(_policy)) \
)

DIR_WITH_RELEASE_POLICY := $(foreach item,$(_release_policy),$(call _word-colon,1,$(item)))

# <LOCAL_MODULE>:<SEMI-COLON SEPERATED LIBRARIES_LIST>
# (with be mapped to vendor/mediatek/yusu/jar/...)
_patch_additional_file :=

ifeq ($(PARTIAL_BUILD),true)
_patch_additional_file := $(subst /,:,$(subst $(ARTIFACT_DIR)/jar/,,$(shell find $(ARTIFACT_DIR)/jar -name "policy.jar")))
endif

$(foreach item,$(_patch_additional_file), \
    $(eval _module := $(call _word-colon,1,$(item))) \
    $(eval ARTIFACT.$(_module).JARS := $(call _word-colon,2,$(item))) \
)


# temporarily removed dependencies 
#  $(MTK_PATH_SOURCE)/external/mhal/inc:oversea \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/inc:oversea \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/lib3a: \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/libidp: \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/libisp: \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/libmexif: \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/libmhal: \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/libmjpeg:oversea;tier1 \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/libsensor: \
#  $(MTK_PATH_SOURCE)/external/mhal/src/test/camtest: \
#  $(MTK_PATH_SOURCE)/external/mhal/src/test/libcunit: \
#  $(MTK_PATH_SOURCE)/external/mhal/src/test/sensorUnitTest: \
#  $(MTK_PATH_SOURCE)/external/mhal/doc:oversea;tier1 \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/libDetection: \
#  $(MTK_PATH_SOURCE)/external/mhal/src/lib/libmcu:oversea;tier1 \
