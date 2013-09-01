

include $(_prefix_)/mediatek/build/Makefile
$(call codebase-path,$(_postfix_),$(_prefix_))

# export variables to shell environments
$(eval ADDITIONAL_LIST := $(strip $(foreach p,$(MTK_PROJECT_CONFIGS),\
    $(shell cat $p | grep -v "^\s*#" | sed 's/\s*=\s*.*//g'))))

# TODO this should be done automatically to avoid issues
VARIABLE_LIST := \
    TO_ROOT MTK_PROJECT MTK_ROOT MTK_ROOT_CUSTOM_OUT MKT_ROOT_CONFIG_OUT MTK_ROOT_PLATFORM MTK_ROOT_CONFIG \
    MTK_ROOT_BUILD MTK_ROOT_SOURCE MTK_ROOT_CUSTOM MTK_PATH_PLATFORM \
    MTK_PATH_BUILD MTK_PATH_SOURCE MTK_PATH_CUSTOM MTK_ROOT_GEN_CONFIG \
    MTK_CUSTOM_FOLDERS MTK_PROJECT_CONFIGS FULL_PROJECT \
    $(ADDITIONAL_LIST)

$(foreach v,$(VARIABLE_LIST), $(info export $(v)="$($v)"))
all:
	@echo "export MTK_LOAD_CONFIG=1"
