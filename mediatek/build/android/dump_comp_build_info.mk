#generate the component build info. file
#make sure the release.dep is newest,
#we need to take release.dep into clean target or every time make a new one manually
ifndef PRODUCT_COMP_BLD_INFO_OUT
  PRODUCT_COMP_BLD_INFO_OUT := $(PRODUCT_OUT)/release
endif

ifndef COMP_INDEX_TABLE
  COMP_INDEX_TABLE := $(PRODUCT_COMP_BLD_INFO_OUT)/path_module_maptable
  COMP_INDEX_TABLE_PLUS := $(PRODUCT_COMP_BLD_INFO_OUT)/module_installedpath_maptable
endif

COMP_BUILD_INFO_FILE := $(PRODUCT_COMP_BLD_INFO_OUT)/$(module_id).dep

ifeq (,$(wildcard $(COMP_BUILD_INFO_FILE)))
  $(shell mkdir -p $(PRODUCT_COMP_BLD_INFO_OUT))
  $(shell touch $(COMP_BUILD_INFO_FILE) 2>/dev/null)
endif

ifeq (,$(wildcard $(COMP_INDEX_TABLE)))
  $(shell mkdir -p $(PRODUCT_COMP_BLD_INFO_OUT))
  $(shell touch $(COMP_INDEX_TABLE) 2>/dev/null)
endif

ifeq (,$(wildcard $(COMP_INDEX_TABLE_PLUS)))
  $(shell mkdir -p $(PRODUCT_COMP_BLD_INFO_OUT))
  $(shell touch $(COMP_INDEX_TABLE_PLUS) 2>/dev/null)
endif

# all build related info. of each component
COMP_BUILD_INFO :=

# the mapping table of ID & path for all components
ifndef COMP_INDEX_INFO
  COMP_INDEX_INFO :=
else
  COMP_INDEX_INFO := $(strip $(COMP_INDEX_INFO))
endif

ifndef COMP_INDEX_INFO_PLUS
  COMP_INDEX_INFO_PLUS :=
else
  COMP_INDEX_INFO_PLUS := $(strip $(COMP_INDEX_INFO_PLUS))
endif

ifneq (,$(LOCAL_PATH))
  COMP_BUILD_INFO += $(strip LOCAL_PATH=$(LOCAL_PATH))
else
  $(error the LOCAL_PATH variable is null)
endif

# md_file_ is the makefile that build this module, refer the function $(call my-dir)
COMP_BUILD_INFO += $(strip LOCAL_MAKEFILE=$(LOCAL_MODULE_MAKEFILE))

# add LOCAL_MODULE_TAGS
COMP_BUILD_INFO += $(strip LOCAL_MODULE_TAGS=$(LOCAL_MODULE_TAGS))

ifneq (,$(LOCAL_MODULE))
  COMP_BUILD_INFO += $(strip LOCAL_MODULE=$(LOCAL_MODULE))
else
  $(error the LOCAL_MODULE variable is null)
endif

ifneq (,$(LOCAL_MODULE_CLASS))
  COMP_BUILD_INFO += $(strip LOCAL_MODULE_CLASS=$(LOCAL_MODULE_CLASS))
else
  $(error the LOCAL_MODULE_CLASS variable is null)
endif

ifneq (,$(LOCAL_BUILT_MODULE))
  COMP_BUILD_INFO += $(strip LOCAL_BUILT_MODULE=$(LOCAL_BUILT_MODULE))
else
  $(error the LOCAL_BUILT_MODULE variable is null)
endif

ifneq (,$(LOCAL_INSTALLED_MODULE))
  COMP_BUILD_INFO += $(strip LOCAL_INSTALLED_MODULE=$(LOCAL_INSTALLED_MODULE))
endif

COMP_INDEX_INFO += $(strip $(LOCAL_PATH)->$(module_id))

ifneq (,$(LOCAL_INSTALLED_MODULE))
  COMP_INDEX_INFO_PLUS += $(strip $(module_id)->$(LOCAL_INSTALLED_MODULE))
endif

# set the variable list which need $(LOCAL_PATH) prefixed
LOCAL_VARIABLE_LIST_WITH_PATH_PREFIX := LOCAL_ADDITIONAL_JAVA_DIR \
                                        LOCAL_ASSET_FILES \
                                        LOCAL_JAVA_RESOURCE_DIRS \
                                        LOCAL_PACKAGE_RESOURCES \
                                        LOCAL_SRC_FILES \
                                        LOCAL_DROIDDOC_SOURCE_PATH \
                                        LOCAL_DROIDDOC_HTML_DIR \
                                        LOCAL_COPY_HEADERS \
                                        LOCAL_INTERMEDIATE_SOURCES \
                                        LOCAL_JARJAR_RULES \
                                        LOCAL_JAR_MANIFEST \
                                        LOCAL_MANIFEST_FILE \
                                        LOCAL_PREBUILT_OBJ_FILES

$(foreach var,$(LOCAL_VARIABLE_LIST_WITH_PATH_PREFIX), \
    $(if $($(var)), \
        $(eval COMP_BUILD_INFO += $(call form-dep-file,$(var))) \
     ) \
 )

# set the variable list which does not need $(LOCAL_PATH) prefixed
LOCAL_VARIABLE_LIST_WITHOUT_PATH_PREFIX := LOCAL_GENERATED_SOURCES \
                                           LOCAL_ASSET_DIR \
                                           LOCAL_RESOURCE_DIR \
                                           LOCAL_JAVA_RESOURCE_FILES \
                                           LOCAL_JAVA_LIBRARIES \
                                           LOCAL_STATIC_JAVA_LIBRARIES \
                                           LOCAL_STATIC_LIBRARIES \
                                           LOCAL_WHOLE_STATIC_LIBRARIES \
                                           LOCAL_SHARED_LIBRARIES \
                                           LOCAL_JNI_SHARED_LIBRARIES


$(foreach var,$(LOCAL_VARIABLE_LIST_WITHOUT_PATH_PREFIX), \
    $(if $($(var)), \
        $(eval COMP_BUILD_INFO += $(call form-dep-file-no-path,$(var))) \
     ) \
 )

# collect the component's dependent libraries
ifneq (,$(LOCAL_STATIC_LIBRARIES))
built_static_libraries := \
    $(foreach lib,$(LOCAL_STATIC_LIBRARIES), \
      $(call intermediates-dir-for, \
        STATIC_LIBRARIES,$(lib),$(LOCAL_IS_HOST_MODULE))/$(lib)$(a_suffix))

COMP_BUILD_INFO += \
    $(foreach lib,$(built_static_libraries), \
        $(strip LOCAL_DEP_BUILT_FILES+=$(strip $(lib))) \
     )
endif

ifneq (,$(LOCAL_WHOLE_STATIC_LIBRARIES))
built_whole_libraries := \
    $(foreach lib,$(LOCAL_WHOLE_STATIC_LIBRARIES), \
      $(call intermediates-dir-for, \
        STATIC_LIBRARIES,$(lib),$(LOCAL_IS_HOST_MODULE))/$(lib)$(a_suffix))

COMP_BUILD_INFO += \
    $(foreach lib,$(built_whole_libraries), \
        $(strip LOCAL_DEP_BUILT_FILES+=$(strip $(lib))) \
     )
endif

ifneq (,$(LOCAL_JNI_SHARED_LIBRARIES)$(LOCAL_SHARED_LIBRARIES))
  $(foreach item,$(LOCAL_JNI_SHARED_LIBRARIES) $(LOCAL_SHARED_LIBRARIES), \
      $(eval COMP_BUILD_INFO += LOCAL_DEP_BUILT_FILES+=$(addprefix $($(my_prefix)OUT_INTERMEDIATE_LIBRARIES)/, \
          $(addsuffix $(so_suffix), $(item))) \
       ) \
   )

  $(foreach item,$(LOCAL_JNI_SHARED_LIBRARIES) $(LOCAL_SHARED_LIBRARIES), \
      $(eval COMP_BUILD_INFO += LOCAL_DEP_BUILT_FILES+=$(addprefix $($(my_prefix)OUT_SHARED_LIBRARIES)/, \
          $(notdir $(addprefix $($(my_prefix)OUT_INTERMEDIATE_LIBRARIES)/, \
              $(addsuffix $(so_suffix), $(item))))) \
       ) \
   )
endif


# set the variable list which config the dependent libraries
LOCAL_DEP_LIB_VARIABLE_LIST := LOCAL_JAVA_LIBRARIES \
                               LOCAL_STATIC_JAVA_LIBRARIES \
                               LOCAL_STATIC_LIBRARIES \
                               LOCAL_WHOLE_STATIC_LIBRARIES \
                               LOCAL_SHARED_LIBRARIES \
                               LOCAL_JNI_SHARED_LIBRARIES

$(foreach var,$(LOCAL_VARIABLE_LIST_WITH_PATH_PREFIX),\
    $(if $(filter $(var),$(LOCAL_DEP_LIB_VARIABLE_LIST)), \
      , \
        $(if $(var), \
            $(eval COMP_BUILD_INFO += $(call form-dep-file-list,$(var))) \
         ) \
     ) \
 )

$(foreach var,$(LOCAL_VARIABLE_LIST_WITHOUT_PATH_PREFIX),\
    $(if $(filter $(var),$(LOCAL_DEP_LIB_VARIABLE_LIST)), \
      , \
        $(if $(var), \
            $(eval COMP_BUILD_INFO += $(call form-dep-file-no-path-list,$(var))) \
         ) \
     ) \
 )

# dump component build related info.
$(call dump-words-to-file.mtk, \
    $(strip $(COMP_BUILD_INFO)), \
    $(strip $(COMP_BUILD_INFO_FILE)) \
 )

# Todo:
# Any dumplicated info.???
# if none, disable sort function to improve build performance
$(shell sort -u $(COMP_BUILD_INFO_FILE) -o $(COMP_BUILD_INFO_FILE))


#
# functions to manipulate the component build related info.
#
define form-dep-file
$(strip \
    $(eval _result := \
       $(foreach elem, $(strip $($(1))), \
          $(addprefix $(1)+=$(LOCAL_PATH)/,$(strip $(elem))) \
        ) \
     ) \
    $(eval COMP_BUILD_INFO += $(strip $(_result)))
    $(eval _result :=) \
 )
endef

define form-dep-file-no-path
$(strip \
    $(eval _result := \
       $(foreach elem,$(strip $($(1))), \
          $(addprefix $(1)+=,$(strip $(elem))) \
        ) \
     ) \
    $(eval COMP_BUILD_INFO += $(strip $(_result)))
    $(eval _result :=) \
 )
endef

define form-dep-file-list
$(strip \
    $(eval _result := \
       $(foreach elem, $(strip $($(1))), \
          $(addprefix LOCAL_DEP_VAR_LIST+=$(LOCAL_PATH)/,$(strip $(elem))) \
        ) \
     ) \
    $(eval COMP_BUILD_INFO += $(strip $(_result)))
    $(eval _result :=) \
 )
endef

define form-dep-file-no-path-list
$(strip \
    $(eval _result := \
       $(foreach elem,$(strip $($(1))), \
          $(addprefix LOCAL_DEP_VAR_LIST+=,$(strip $(elem))) \
        ) \
     ) \
    $(eval COMP_BUILD_INFO += $(strip $(_result)))
    $(eval _result :=) \
 )
endef

