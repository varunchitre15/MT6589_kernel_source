
define add-install-target
$(shell if [ ! -e $(1) ]; then touch $(1); fi; \
  echo $(2) >> $(1) \
 )
endef

ARTIFACT_SWITCH_FILE := $(ARTIFACT_DIR)/switch.txt
ARTIFACT_DIR_CLS := $(ARTIFACT_DIR)/cls
ARTIFACT_DIR_JAR := $(ARTIFACT_DIR)/jar
SWITCH_DIRECTORY :=

#config the switch flag
# ----------------------------------------
SWITCH_GEMINI_FLAG := no


# ----------------------------------------

ifeq ($(SWITCH_GEMINI_FLAG),yes)
# #########################################
# gemini switch
# #########################################
GEMINI_SWITCH_DIR := $(ARTIFACT_DIR)
GEMINI_CLS_SSIM := cls_ssim
GEMINI_CLS_DSIM := cls_dsim
GEMINI_JAR_INI := $(ARTIFACT_DIR_JAR)
GEMINI_JAR_SSIM := jar_ssim
GEMINI_JAR_DSIM := jar_dsim
ifeq ($(GEMINI),yes)
SWITCH_DIRECTORY += $(ARTIFACT_DIR_CLS):$(GEMINI_SWITCH_DIR)/$(GEMINI_CLS_DSIM)
SWITCH_DIRECTORY += $(ARTIFACT_DIR_JAR):$(GEMINI_SWITCH_DIR)/$(GEMINI_JAR_DSIM)
else
SWITCH_DIRECTORY += $(ARTIFACT_DIR_CLS):$(GEMINI_SWITCH_DIR)/$(GEMINI_CLS_SSIM)
SWITCH_DIRECTORY += $(ARTIFACT_DIR_JAR):$(GEMINI_SWITCH_DIR)/$(GEMINI_JAR_SSIM)
endif # GEMINI
###########################################
endif # SWITCH_GEMINI_FLAG

#add the swith info into switch.txt
$(shell if [ -e $(ARTIFACT_SWITCH_FILE) ]; then rm -f $(ARTIFACT_SWITCH_FILE); fi)
$(foreach item,$(SWITCH_DIRECTORY),\
  $(eval $(call add-install-target,$(ARTIFACT_SWITCH_FILE),$(item))) \
 )


