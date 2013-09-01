

.PHONY: checkmtkapi

# eval this to define a rule that runs apicheck.
#
# Args:
#    $(1)  target
#    $(2)  stable api xml file
#    $(3)  api xml file to be tested
#    $(4)  arguments for apicheck
#    $(5)  command to run if apicheck failed

define check-mtk-api
$(TARGET_OUT_COMMON_INTERMEDIATES)/PACKAGING/$(strip $(1))-timestamp: $(2) $(3) $(APICHECK)
	@echo "Checking MediaTek API:" $(1)
	$(hide) ( $(APICHECK_COMMAND) $(4) $(2) $(3) || ( $(5) ; exit 38 ) )
	$(hide) mkdir -p $$(dir $$@)
	$(hide) touch $$@
checkmtkapi: $(TARGET_OUT_COMMON_INTERMEDIATES)/PACKAGING/$(strip $(1))-timestamp
endef

checkapi: checkmtkapi

# Get MTK SDK API XML of newest level.
last_released_mtk_sdk_version := $(lastword $(call numerically_sort,\
	$(patsubst $(SRC_MTK_API_DIR)/%.txt,%, \
	$(wildcard $(SRC_MTK_API_DIR)/*.txt))))

# Check that the API we're building hasn't broken the last-released SDK version.
# When fails, build will breaks with message from "apicheck_msg_last" text file shown.
ifneq ($(strip $(PARTIAL_BUILD)),true)
$(eval $(call check-mtk-api, \
	checkmtkapi-last, \
	$(SRC_MTK_API_DIR)/$(last_released_mtk_sdk_version).txt, \
	$(MTK_INTERNAL_PLATFORM_API_FILE), \
	-error 2 -error 3 -error 4 -error 5 -error 6 -error 7 -error 8 -error 9 -error 10 \
	-error 11 -error 12 -error 13 -error 14 -error 15 -error 16 -error 17 -error 18 \
	-error 19 -error 20 -error 21 -error 23 -error 24 , \
	cat $(BUILD_SYSTEM_MTK_EXTENSION)/apicheck_msg_last.txt \
	))
endif
