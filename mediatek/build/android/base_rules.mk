
export_includes_suffix := _intermediates/export_includes
ifneq (,$(DO_PUT_ARTIFACTS))
  policy_path := $(call policy-path,$(LOCAL_PATH))
  LOCAL_RELEASE_POLICY :=


  $(foreach item,$(strip $(LOCAL_JNI_SHARED_LIBRARIES) $(LOCAL_SHARED_LIBRARIES)),\
    $(eval _item := $(addprefix $($(my_prefix)OUT_INTERMEDIATE_LIBRARIES)/, \
      $(addsuffix $(so_suffix), $(item)))) \
    $(eval ARTIFACT.$(item).FILES := $(ARTIFACT.$(item).FILES) $(_item):$(patsubst $(OUT_DIR)/%,$(ARTIFACT_DIR)/out/%,$(_item))) \
  )

  $(foreach item,$(strip $(LOCAL_STATIC_JAVA_LIBRARIES)),\
    $(eval _item := $($(my_prefix)OUT_COMMON_INTERMEDIATES)/JAVA_LIBRARIES/$(item)_intermediates/javalib.jar) \
    $(eval ARTIFACT.$(item).FILES := $(ARTIFACT.$(item).FILES) $(_item):$(patsubst $(OUT_DIR)/%,$(ARTIFACT_DIR)/out/%,$(_item))) \
  )

  $(foreach item,$(strip $(LOCAL_STATIC_LIBRARIES) $(LOCAL_WHOLE_STATIC_LIBRARIES)),\
    $(eval _item :=  $(addprefix $($(my_prefix)OUT_INTERMEDIATES)/STATIC_LIBRARIES/,$(addsuffix $(export_includes_suffix),$(item))))    \
    $(eval ARTIFACT.$(item).FILES := $(ARTIFACT.$(item).FILES) $(_item):$(patsubst $(OUT_DIR)/%,$(ARTIFACT_DIR)/out/%,$(_item))) \
  )

  $(foreach item,$(strip $(LOCAL_SYSTEM_SHARED_LIBRARIES) $(LOCAL_SHARED_LIBRARIES)),\
    $(eval _item :=  $(addprefix $($(my_prefix)OUT_INTERMEDIATES)/SHARED_LIBRARIES/,$(addsuffix $(export_includes_suffix),$(item))))    \
    $(eval ARTIFACT.$(item).FILES := $(ARTIFACT.$(item).FILES) $(_item):$(patsubst $(OUT_DIR)/%,$(ARTIFACT_DIR)/out/%,$(_item))) \
  )

  ifdef $(policy_path).RELEASE_POLICY
    LOCAL_RELEASE_POLICY := $($(policy_path).RELEASE_POLICY)
  endif
  ifeq (,$(LOCAL_RELEASE_POLICY))
  LOCAL_RELEASE_POLICY := $(VALID_RELEASE_POLICY)
  endif
  ifeq (,$(filter $(VALID_RELEASE_POLICY),$(LOCAL_RELEASE_POLICY)))
    $(error local release policy for $(LOCAL_MODULE) should in $(VALID_RELEASE_POLICY), not $(LOCAL_RELEASE_POLICY))
  endif
  ifeq (,$(filter $(LOCAL_RELEASE_POLICY),$(RELEASE_POLICY)))
    ARTIFACT_RELEASE :=
    ifeq (APPS, $(LOCAL_MODULE_CLASS))
      # use product makefile for APP modules
      ifeq (optional,$(LOCAL_MODULE_TAGS))
        ifneq (,$(filter $(LOCAL_MODULE),$(PRODUCTS.$(INTERNAL_PRODUCT).PRODUCT_PACKAGES)))
           ARTIFACT_RELEASE := true
        endif
      endif
    else
      # use tags for non-APP modules
      # filter "tags_to_install" modules & static libraries tagged as "optional"
      ifneq (,$(filter optional $(tags_to_install),$(LOCAL_MODULE_TAGS)))
         ARTIFACT_RELEASE := true
      endif
    endif

    ifeq ($(ARTIFACT_RELEASE),true)
      ifneq (,$(notice_target))
        ARTIFACT_TARGET += $(notice_target):$(installed_notice_file)
      endif
      _artifacts := 
      ifneq (,$(installed_notice_file))
        _artifacts += $(installed_notice_file):$(patsubst $(OUT_DIR)/%,$(ARTIFACT_DIR)/out/%, $(installed_notice_file))
      endif
      ifneq (,$(LOCAL_INSTALLED_MODULE))
        _artifacts += $(LOCAL_INSTALLED_MODULE):$(patsubst $(OUT_DIR)/%,$(ARTIFACT_DIR)/out/%, $(LOCAL_INSTALLED_MODULE))
      else
        ifneq (,$(LOCAL_BUILT_MODULE))
          _artifacts += $(LOCAL_BUILT_MODULE):$(patsubst $(OUT_DIR)/%,$(ARTIFACT_DIR)/out/%, $(LOCAL_BUILT_MODULE))
        endif
      endif # LOCAL_INSTALLED_MODULE
      #module-installed-files
      _install_module := $(strip $(call module-installed-files,$(LOCAL_MODULE)))
      ifeq (,$(_install_module))
        _install_module := $(strip $(call module-built-files,$(LOCAL_MODULE)))
      endif
      ARTIFACT_MODULE += $(_install_module)
      ARTIFACT.$(_install_module).SRC := $(LOCAL_MODULE)

      $(foreach item,$(LOCAL_JNI_SHARED_LIBRARIES) $(LOCAL_SHARED_LIBRARIES),\
          $(eval ARTIFACT_TARGET += $(LOCAL_BUILT_MODULE):$(addprefix $($(my_prefix)OUT_INTERMEDIATE_LIBRARIES)/,\
              $(addsuffix $(so_suffix), $(item))))\
       )

      $(foreach item,$(LOCAL_JNI_SHARED_LIBRARIES) $(LOCAL_SHARED_LIBRARIES),\
          $(eval ARTIFACT_TARGET += $(LOCAL_BUILT_MODULE):$(addprefix $($(my_prefix)OUT_SHARED_LIBRARIES)/, \
              $(notdir  $(addprefix $($(my_prefix)OUT_INTERMEDIATE_LIBRARIES)/,\
                  $(addsuffix $(so_suffix), $(item))))))\
       )

      ifeq (,$(ARTIFACT.$(LOCAL_MODULE).FILES))
        ARTIFACT.$(LOCAL_MODULE).FILES := 
      endif

      ARTIFACT.$(LOCAL_MODULE).FILES := $(ARTIFACT.$(LOCAL_MODULE).FILES) $(_artifacts)

    endif # ARTIFACT_RELEASE 
  endif # LOCAL_RELEASE_POLICY
endif #DO_PUT_ARTIFACTS

ALL_ARTIFACT_LOCAL_MODULES := $(ALL_ARTIFACT_LOCAL_MODULE) $(LOCAL_MODULE)
ifneq (,$(DO_GET_ARTIFACTS))
  ifneq (,$(ARTIFACT.$(LOCAL_MODULE).JARS))
$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_STATIC_JAVA_LIBRARIES += \
                               $(foreach item,$(subst |, ,$(ARTIFACT.$(LOCAL_MODULE).JARS)), \
                                   $(ARTIFACT_DIR)/jar/$(item)  \
                                )
  endif
endif #DO_GET_ARTIFACTS

ifeq ($(PARTIAL_BUILD),true)
  ifneq (,$(ARTIFACT.$(LOCAL_MODULE).JARS))
$(LOCAL_INTERMEDIATE_TARGETS): PRIVATE_STATIC_JAVA_LIBRARIES += \
                               $(foreach item,$(subst |, ,$(ARTIFACT.$(LOCAL_MODULE).JARS)), \
                                   $(ARTIFACT_DIR)/jar/$(LOCAL_MODULE)/$(item)  \
                                )
  endif
endif

ifneq (,$(LOCAL_GENERATE_CUSTOM_FOLDER))
  ifeq (,$(filter $(mtk-custom-folder),$(LOCAL_PATH)))
    mtk-custom-folder += $(LOCAL_PATH)

    $(foreach t,$(LOCAL_GENERATE_CUSTOM_FOLDER),\
        $(eval _t := $(subst :, ,$(if $(filter all,$(t)),custom:,$(t)))) \
        $(eval _1 := $(LOCAL_PATH)/$(word 1,$(_t))) \
        $(eval _2 := $(call to-root,$(LOCAL_PATH))/$(MTK_PATH_CUSTOM)/$(word 2,$(_t))) \
        $(eval $(call generate-custom-folder,$(_1),$(_2))) \
     )

  endif
endif

            
# Check every module that do not use source file or other resource in protect folder
define protect-err
$(error $(1): Please do not use this protect source $(2))
endef

Check_Item := $(LOCAL_SRC_FILES) $(LOCAL_C_INCLUDE)
Check_Path := mediatek/protect/ \
              mediatek/protect-bsp/ \
              mediatek/protect-app/
PROTECT_FILES :=
$(foreach path,$(Check_Path), \
   $(if $(filter $(path)/%,$(LOCAL_PATH)),, \
      $(foreach item,$(Check_Item),\
         $(if $(findstring $(path),$(item)),\
             $(eval PROTECT_FILES += $(item)) \
         ) \
      ) \
   ) \
)
# Add exception case for gemini for workaround
ERROR_FILES :=
$(foreach item,$(PROTECT_FILES),  \
  $(if $(filter ../../mediatek/protect/frameworks/base/telephony/java/com/android/internal/telephony/gemini/%,$(PRODUCT_FILES)),  \
    $(eval ERROR_FILES += $(item))  \
  ) \
)
ifneq ($(ERROR_FILES),)
  $(call protect-err,$(LOCAL_PATH),$(PROTECT_FILES))
endif

ifeq ($(DUMP_COMP_BUILD_INFO),true)
-include $(BUILD_SYSTEM_MTK_EXTENSION)/dump_comp_build_info.mk
endif

