
# custom.mk - add supports for custom folder generation

#internal functions supporting custom folder generation
define .mtk.custom.delete-rule
$(1): $(2).delete
$(2).delete: 
	@echo "[DELETE] $(2)"
	@rm -rf $(2)
endef

define .mtk.custom.generate-rule
$(1): $(2)
$(2): $(3)
	@echo "[CUSTOM] copy $(3)"
	@echo "           to $(2)"
	@mkdir -p $(dir $(2))
	@cp -f $(3) $(2)
endef

define .mtk.custom.split-project
$(if $(filter-out .,$(1)),$(subst /,.,$(1)) $(call .mtk.custom.split-project,$(patsubst %/,%,$(dir $(1)))),)
endef

define .mtk.custom.map-module
$(strip \
  $(eval _custom._ := $(call path.split,$1)) \
  $(eval _custom.d := $(call path.split,$(firstword $(_custom._)))) \
  $(eval _custom.f := $(lastword $(_custom._))) \
  $(eval _custom.n := CUSTOM_$(call uc,$(subst /,_,$(firstword $(_custom.d))))) \
  $(eval _custom.v := $(lastword $(_custom.d))) \
  $(if $(filter $($(_custom.n)),$(_custom.v)),
    $1 $(firstword $(_custom.d))/$(_custom.f),
    $(if $(call seq,undefined,$(origin $(_custom.n))),$1 $1,\
      $(if $(filter inc,$(_custom.v)),$1 $1,\
        $(if $(filter src,$(_custom.v)),$1 $(firstword $(_custom.d))/$(_custom.f),)\
      )
  )) \
)
endef
#  $(if $(filter $($(_custom.n)),$(_custom.v)),
#    $1 $(firstword $(_custom.d))/$(_custom.f),
#    $(if $(filter inc src,$(lastword $(_custom.d))),$1 $1,\
#      $(if $($(_custom.n)),,$1 $1) \
#  )) \

define .mtk.custom.generate-folder-list
$(strip $(eval _mtk_project_ :=$(subst ],,$(subst [, ,$(FULL_PROJECT)))) \
$(eval _flvlist_     := $(strip $(subst +, ,$(word 2,$(_mtk_project_))))) \
$(eval _prjlist_     := $(call .mtk.custom.split-project,$(subst .,/,$(word 1,$(_mtk_project_))))) \
$(eval _fp_list_     := $(foreach p,$(_prjlist_),$(foreach f,$(_flvlist_),$(p)[$(f)])) $(_prjlist_)) \
$(eval _cust_list_   := $(if $(CUSTOMER),$(CUSTOMER))) \
$(_fp_list_) $(_cust_list_) $(call lc,$(MTK_PLATFORM)) common)
endef

define .if-cfg-on
$(if $(filter-out NO NONE FALSE,$(call uc,$(strip $($(1))))),$(2),$(3))
endef
define mtk.custom.generate-macros
$(strip $(foreach t,$(AUTO_ADD_GLOBAL_DEFINE_BY_NAME),$(call .if-cfg-on,$(t),-D$(call uc,$(t))))
$(foreach t,$(AUTO_ADD_GLOBAL_DEFINE_BY_VALUE),$(call .if-cfg-on,$(t),$(foreach v, $(call uc,$($(t))),-D$(v))))
$(foreach t,$(AUTO_ADD_GLOBAL_DEFINE_BY_NAME_VALUE),$(call .if-cfg-on,$(t),-D$(call uc,$(t))=\"$($(t))\")))
endef

# * mtk.custom.generate-rules - generate rules for customization folder generation
# example usage
# a. associate custom files with target "all"
#    $(call mtk.custom.generate-rules,all)
# b. associate custom files under */kernel and */uboot with target "vmlinux"
#    $(call mtk.custom.generate-rules,vmlinux,kernel uboot)
# c. associate auto-generated files (only support files generated under $(MTK_ROOT_CUSTOM)/$(MTK_PROJECT)
#    $(call mtk.custom.generate-rules,depend,preloader,$(MTK_ROOT_CUSTOM)/$(MTK_PROJECT)/preloader/inc/custom_emi.h
define mtk.custom.generate-rules
$(if $(MTK_ROOT_CUSTOM),$(strip \
  $(eval _custflist_ :=) $(eval _custfmap_  :=) $(eval _custfgen :=) \
  $(foreach d,$(addprefix $(MTK_ROOT_CUSTOM)/,$(MTK_CUSTOM_FOLDERS)),\
    $(eval _dirs := $(call wildcard2,$(if $(2),$(addprefix $(d)/,$(2)),$(d)))) \
    $(if $(_dirs),\
      $(eval _files := $(filter-out $(_custflist_), \
        $(patsubst $(d)/%,%,$(shell find -L $(_dirs) -type d -name ".svn" -prune -o \( ! -name .\* \) -type f -print)))) \
      $(foreach f,$(_files), \
        $(eval _      := $(call .mtk.custom.map-module,$f)) \
        $(if $_, \
          $(eval _src_       := $(firstword $_)) \
          $(eval _des_       := $(lastword $_)) \
          $(eval _des_       := $(if $(CUSTOM_MODEM),$(subst $(CUSTOM_MODEM)/,,$(_des_)),$(_des_))) \
          $(eval _custflist_ += $(_src_)) \
          $(eval _custfmap_  += $(MTK_ROOT_CUSTOM_OUT)/$(_des_):$(d)/$(_src_)) \
        ,) \
      ) \
    ,) \
  ) \
  $(eval # TODO pre-generated files should be enforced directly by architecture ) \
  $(if $(3),$(foreach f,$(filter-out $(_custflist_),$(3)), \
    $(eval _g_       := $(MTK_ROOT_CUSTOM_OUT)/$(patsubst $(MTK_ROOT_CUSTOM)/$(MTK_PROJECT)/%,%,$(f))) \
    $(if $(filter $(addprefix $(MTK_ROOT_CUSTOM_OUT)/,$(_custflist_)),$(_g_)),, \
      $(eval _custfgen  += $(_g_)) $(eval _custflist_ += $(f)) $(eval _custfmap_  += $(_g_):$(f)) \
  )),) \
  $(if $(call wildcard2,$(MTK_ROOT_CUSTOM_OUT)),\
    $(foreach f,$(filter-out $(_custfgen) $(foreach f,$(_custfmap_),$(word 1,$(subst :, ,$f))),\
      $(shell find $(if $(2),$(addprefix $(MTK_ROOT_CUSTOM_OUT)/,$(2)),$(MTK_ROOT_CUSTOM_OUT)) \
        -type d -name ".svn" -prune -o \
        ! \( -name '*.[oas]' -o -name '*.ko' -o -name '.*' -o -name '*.mod.c' -o -name '*.gcno' \
          -o -name 'modules.order' \) -type f -print 2> /dev/null)),\
      $(eval $(call .mtk.custom.delete-rule,$(1),$(f)))) \
  ,) \
  $(foreach f,$(_custfmap_),\
    $(eval $(call .mtk.custom.generate-rule,$(1),$(word 1,$(subst :, ,$(f))),$(word 2,$(subst :, ,$(f))))) \
    $(word 1,$(subst :, ,$(f))) \
   ) \
  $(eval # dump custgen dependency info. ) \
  $(if $(filter true, $(DUMP_COMP_BUILD_INFO)), \
    $(eval _depinfo := $(strip $(_custfmap_))) \
    $(eval _depfile := $(MTK_ROOT_CUSTOM_OUT)/custgen.P) \
    $(shell if [ ! -d $(dir $(_depfile)) ]; then mkdir -p $(dir $(_depfile)); fi) \
    $(call dump-words-to-file.mtk, $(strip $(_depinfo)), $(_depfile)) \
  ,) \
),)
endef

# Load Project Configuration
#   project configuration is defined in mediatek/config/<project>/ProjectConfig.mk
#   they will be include in reversed MTK_CUSTOM_FOLDER order, e.g.,
#     common/ProjectConfig.mk mtxxxx/ProjectConfig.mk prj/ProjectConfig.mk prj[flv]/ProjectConfig.mk
# pre-do include and export Project configurations for initializing some basic variables (MTK_PLATFORM)
MTK_CUSTOM_FOLDERS  := $(call .mtk.custom.generate-folder-list)
MTK_PROJECT_CONFIGS := $(call wildcard2,$(foreach c,$(call reverse,$(addsuffix /ProjectConfig.mk,\
    $(addprefix ../../config/,$(MTK_CUSTOM_FOLDERS)))),$(call relative-path,$(c))))
$(foreach p,$(MTK_PROJECT_CONFIGS),$(eval include $p))

# update custom folder with platform
# include and export Project configurations after updating MTK_CUSTOM_FOLDERS
MTK_CUSTOM_FOLDERS  := $(call .mtk.custom.generate-folder-list)
MTK_PROJECT_CONFIGS := $(call wildcard2,$(foreach c,$(call reverse,$(addsuffix /ProjectConfig.mk,\
    $(addprefix ../../config/,$(MTK_CUSTOM_FOLDERS)))),$(call relative-path,$(c))))
$(foreach p,$(MTK_PROJECT_CONFIGS),$(eval include $p))
# export all project defined variables
# it is necessary to have MTK_PROJECT here, to prevent empty project file
export_var :=
$(foreach p,$(MTK_PROJECT_CONFIGS),$(foreach f,$(strip $(shell cat $p | \
    grep -v -P "^\s*#" | sed 's/\s*=\s*.*//g')),$(eval export_var+=$f)))
export_var+= MTK_PROJECT
$(foreach i,$(export_var),$(eval $i=$(strip $($i))))
$(eval export $(export_var))

# MTK_CUSTOM_FOLDER   : decomposed project name list. for example, for rp.v2[lca+multitouch] it will be
#                       rp.v2[lca] rp.v2[multitouch] rp[lca] rp[multitouch] rp.v2 rp mtxxxx common
# MTK_PROJECT_CONFIGS : pathmap of all used configurations
export MTK_CUSTOM_FOLDERS MTK_PROJECT_CONFIGS
