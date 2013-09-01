
# *************************************************************************
# Set shell align with Android build system
# *************************************************************************
SHELL        := /bin/bash

include mediatek/build/Makefile
$(call codebase-path)
#mtk-config-files := $(strip $(call mtk.config.generate-rules,mtk-config-files))
#mtk-custom-files := $(strip $(call mtk.custom.generate-rules,mtk-custom-files))
$(shell $(foreach a,$(CMD_ARGU),$(if $(filter 2,$(words $(subst =, ,$(a)))),$(a))) make -f mediatek/build/custgen.mk > /dev/null)
PRJ_MF       := $(MTK_ROOT_CONFIG_OUT)/ProjectConfig.mk
include $(MTK_ROOT_CONFIG_OUT)/ProjectConfig.mk
ifdef OPTR_SPEC_SEG_DEF
  ifneq (NONE,$(OPTR_SPEC_SEG_DEF))
    include $(MTK_ROOT_SOURCE_OPERATOR)/OperatorInfo.mk
  endif
endif

remake update-api $(ALLJAVAOPTFILES) drvgen emigen nandgen: custgen
#ifneq ($(PROJECT),)
# include $(PRJ_MF)
#endif
# *************************************************************************
# Set PHONY
# *************************************************************************
.PHONY : new newall remake remakeall clean cleanall \
         preloader uboot kernel android \
         check-modem update-modem sign-image encrypt-image sign-modem check-dep \
         dump-memusage gen-relkey check-appres \
         codegen btcodegen javaoptgen clean-javaoptgen emigen nandgen custgen drvgen ptgen \
         update-api modem-info bindergen clean-modem

#MKTOPDIR      =  $(shell pwd)
LOGDIR        =  $(MKTOPDIR)/out/target/product
S_MODULE_LOG  =  out/target/product/$(PROJECT)_$(CUR_MODULE).log
S_CODEGEN_LOG =  out/target/product/$(PROJECT)_codegen.log
CODEGEN_LOG   =  $(LOGDIR)/$(PROJECT)_codegen.log
MODULE_LOG    =  $(LOGDIR)/$(PROJECT)_$(CUR_MODULE).log
S_LOG =  out/target/product/$(PROJECT)_
LOG   =  $(LOGDIR)/$(PROJECT)_
CUSTOM_MEMORY_HDR = mediatek/custom/$(PROJECT)/preloader/inc/custom_MemoryDevice.h
CUSTOM_NAND_HDR = mediatek/custom/$(PROJECT)/common/nand_device_list.h

ifeq ($(MTK_TABLET_DRAM),yes)
MEMORY_DEVICE_XLS = mediatek/build/tools/TabletEmiList/$(MTK_PLATFORM)/TabletMemoryDeviceList_$(MTK_PLATFORM).xls
else
MEMORY_DEVICE_XLS = mediatek/build/tools/emigen/$(MTK_PLATFORM)/MemoryDeviceList_$(MTK_PLATFORM).xls
endif

USERID        =  $(shell whoami)
PRELOADER_WD  =  mediatek/preloader
UBOOT_WD      =  bootable/bootloader/uboot
LK_WD         =  bootable/bootloader/lk
KERNEL_WD     =  kernel
ANDROID_WD    =  .
ALL_MODULES   =  
MAKE_DEBUG    =  --no-print-directory
hide         :=  @
CMD_ARGU2    :=  $(filter-out -j%, $(CMD_ARGU))
REMAKECMD    :=  make -fmediatek/build/makemtk.mk CMD_ARGU=$(CMD_ARGU) $(CMD_ARGU2) $(MAKE_DEBUG)
CPUCORES     :=  $(shell cat /proc/cpuinfo | grep processor | wc -l)
MAKEJOBS     :=  -j$(CPUCORES)

# Memory partition auto-gen related variable initilization
MEM_PARTITION_GENERATOR   := mediatek/build/tools/ptgen/$(MTK_PLATFORM)/ptgen.pl
MEM_PARTITION_TABLE       := mediatek/build/tools/ptgen/$(MTK_PLATFORM)/partition_table_$(MTK_PLATFORM).xls
PARTITION_HEADER_LOCATION := mediatek/custom/$(PROJECT)/common
BOARDCONFIG_LOCATION 	  := mediatek/config/$(PROJECT)/configs/EMMC_partition_size.mk
OTA_SCATTER_GENERATOR     := mediatek/build/tools/ptgen/ota_scatter.pl

#ifeq ($(ACTION),update-api)
#   MAKEJOBS := 
#endif
MAKECMD      :=  make $(MAKEJOBS) $(CMD_ARGU) $(MAKE_DEBUG)

SHOWTIMECMD   =  date "+%Y/%m/%d %H:%M:%S"
SHOWRSLT      =  /usr/bin/perl $(MKTOPDIR)/mediatek/build/tools/showRslt.pl

PRELOADER_IMAGES := $(PRELOADER_WD)/bin/preloader_$(PROJECT).bin
UBOOT_IMAGES     := $(UBOOT_WD)/uboot_$(PROJECT).bin \
                    $(UBOOT_WD)/logo.bin

LK_IMAGES     := $(LK_WD)/build-$(PROJECT)/lk.bin

ifeq ($(strip $(KBUILD_OUTPUT_SUPPORT)),yes)
  KERNEL_IMAGES    := $(KERNEL_WD)/out/kernel_$(PROJECT).bin
else
  KERNEL_IMAGES    := $(KERNEL_WD)/kernel_$(PROJECT).bin
endif
ANDROID_IMAGES   := $(LOGDIR)/$(PROJECT)/system.img \
                    $(LOGDIR)/$(PROJECT)/boot.img \
                    $(LOGDIR)/$(PROJECT)/recovery.img \
                    $(LOGDIR)/$(PROJECT)/secro.img \
                    $(LOGDIR)/$(PROJECT)/userdata.img
ifeq (true,$(BUILD_TINY_ANDROID))
 ANDROID_IMAGES := $(filter-out %recovery.img,$(ANDROID_IMAGES))
endif
ifneq ($(ACTION),)
ANDROID_TARGET_IMAGES :=$(filter %/$(patsubst %image,%.img,$(ACTION)),$(ANDROID_IMAGES))
ifeq (${ACTION},otapackage)
ANDROID_TARGET_IMAGES :=$(ANDROID_IMAGES) 
endif
ifeq (${ACTION},snod)
ANDROID_TARGET_IMAGES :=$(filter %/system.img,$(ANDROID_IMAGES))
endif
endif
ifeq (MT6573, $(MTK_PLATFORM))
  ifeq (android, $(CUR_MODULE))
    ANDROID_IMAGES += $(LOGDIR)/$(PROJECT)/DSP_BL
  endif
endif

ifeq ($(MTK_LCA_SUPPORT),yes)
  SCATTER_FILE := mediatek/misc/$(MTK_PLATFORM)_Android_scatter_LCA.txt
else
  SCATTER_FILE := mediatek/misc/$(MTK_PLATFORM)_Android_scatter.txt
  ifeq ($(strip $(MTK_EMMC_SUPPORT)),yes)
    SCATTER_FILE := mediatek/misc/$(MTK_PLATFORM)_Android_scatter_emmc.txt
  endif
endif
#wschen
OTA_SCATTER_FILE := mediatek/misc/ota_scatter.txt

export TARGET_PRODUCT=$(PROJECT)
export FLAVOR=$(FLAVOR)

ifneq ($(ACTION), )
  SHOWBUILD     =  $(ACTION)
else
  SHOWBUILD     =  build
endif
SHOWTIME      =  $(shell $(SHOWTIMECMD))
ifeq ($(ENABLE_TEE), TRUE)
  DEAL_STDOUT := 2>&1 | tee -a $(MODULE_LOG)
  DEAL_STDOUT_CODEGEN := 2>&1 | tee -a $(CODEGEN_LOG)
  DEAL_STDOUT_BTCODEGEN := 2>&1 | tee -a $(LOG)btcodegen.log
  DEAL_STDOUT_CUSTGEN := 2>&1 | tee -a $(LOG)custgen.log
  DEAL_STDOUT_EMIGEN := 2>&1 | tee -a $(LOG)emigen.log
  DEAL_STDOUT_NANDGEN := 2>&1 | tee -a $(LOG)nandgen.log
  DEAL_STDOUT_JAVAOPTGEN := 2>&1 | tee -a $(LOG)javaoptgen.log
  DEAL_STDOUT_IMEJAVAOPTGEN := 2>&1 | tee -a $(LOG)imejavaoptgen.log
  DEAL_STDOUT_SIGN_IMAGE := 2>&1 | tee -a $(LOG)sign-image.log
  DEAL_STDOUT_ENCRYPT_IMAGE := 2>&1 | tee -a $(LOG)encrypt-image.log
  DEAL_STDOUT_DRVGEN := 2>&1 | tee -a $(LOG)drvgen.log
  DEAL_STDOUT_SIGN_MODEM := 2>&1 | tee -a $(LOG)sign-modem.log
  DEAL_STDOUT_CHECK_MODEM := 2>&1 | tee -a $(LOG)check-modem.log
  DEAL_STDOUT_MODEM_INFO := 2>&1 | tee -a $(LOG)modem-info.log
  DEAL_STDOUT_UPDATE_MD := 2>&1 | tee -a $(LOG)update-modem.log
  DEAL_STDOUT_DUMP_MEMUSAGE := 2>&1 | tee -a $(LOG)dump-memusage.log
  DEAL_STDOUT_PTGEN := 2>&1 | tee -a $(LOG)ptgen.log
  DEAL_STDOUT_MM := 2>&1 | tee -a $(LOG)mm.log
  DEAL_STDOUT_CUSTREL := 2>&1 | tee -a $(LOG)rel-cust.log
  DEAL_STDOUT_CHK_APPRES := 2>&1 | tee -a $(LOG)check-appres.log
  DEAL_STDOUT_BINDERGEN := 2>&1 | tee -a $(LOG)bindergen.log
else
  DEAL_STDOUT  := >> $(MODULE_LOG) 2>&1
  DEAL_STDOUT_CODEGEN  := > $(CODEGEN_LOG) 2>&1
  DEAL_STDOUT_BTCODEGEN  := > $(LOG)btcodegen.log 2>&1
  DEAL_STDOUT_CUSTGEN := > $(LOG)custgen.log 2>&1
  DEAL_STDOUT_EMIGEN := > $(LOG)emigen.log 2>&1
  DEAL_STDOUT_NANDGEN := > $(LOG)nandgen.log 2>&1
  DEAL_STDOUT_JAVAOPTGEN := > $(LOG)javaoptgen.log 2>&1
  DEAL_STDOUT_IMEJAVAOPTGEN := > $(LOG)imejavaoptgen.log 2>&1
  DEAL_STDOUT_SIGN_IMAGE := > $(LOG)sign-image.log 2>&1
  DEAL_STDOUT_ENCRYPT_IMAGE := > $(LOG)encrypt-image.log 2>&1
  DEAL_STDOUT_SIGN_MODEM := > $(LOG)sign-modem.log 2>&1
  DEAL_STDOUT_CHECK_MODEM := > $(LOG)check-modem.log 2>&1
  DEAL_STDOUT_MODEM_INFO := > $(LOG)modem-info.log 2>&1
  DEAL_STDOUT_DRVGEN := > $(LOG)drvgen.log 2>&1
  DEAL_STDOUT_UPDATE_MD := > $(LOG)update-modem.log 2>&1
  DEAL_STDOUT_DUMP_MEMUSAGE := > $(LOG)dump-memusage.log 2>&1
  DEAL_STDOUT_PTGEN := > $(LOG)ptgen.log 2>&1
  DEAL_STDOUT_MM := > $(LOG)mm.log 2>&1
  DEAL_STDOUT_CUSTREL := > $(LOG)rel-cust.log 2>&1
  DEAL_STDOUT_CHK_APPRES := >> $(LOG)check-appres.log 2>&1
  DEAL_STDOUT_BINDERGEN := > $(LOG)bindergen.log 2>&1
endif

ifneq ($(PROJECT),generic)
  MAKECMD    +=  TARGET_PRODUCT=$(PROJECT) GEMINI=$(GEMINI) EVB=$(EVB) FLAVOR=$(FLAVOR)
else
  MAKECMD    +=  TARGET_PRODUCT=generic GEMINI=$(GEMINI) EVB=$(EVB) FLAVOR=$(FLAVOR)
endif

ifeq ($(BUILD_PRELOADER),yes)
  ALL_MODULES += preloader
endif

ifeq ($(BUILD_LK),yes)
  ALL_MODULES += lk
endif

ifeq ($(BUILD_UBOOT),yes)
  ALL_MODULES += uboot
endif

ifeq ($(BUILD_KERNEL),yes)
  ALL_MODULES += kernel
  KERNEL_ARG = kernel_$(PROJECT).config
endif

ALL_MODULES += android

-include mediatek/build/tools/preprocess/preprocess.mk

ifneq ($(MTK_PTGEN_SUPPORT),no)
newall: cleanall emigen nandgen ptgen custgen codegen remakeall
else
newall: cleanall emigen nandgen custgen codegen remakeall
endif

check-dep: custgen
	$(eval include mediatek/build/addon/core/config.mak)
	$(if $(filter error,$(DEP_ERR_CNT)),\
                  $(error Dependency Check FAILED!!))
#	$(hide) echo " Dependency Check Successfully!!"
#	$(hide) echo "*******************************"

new: clean codegen remake

remakeall:

cleanall remakeall:
ifeq ($(filter -k, $(CMD_ARGU)),)
	$(hide) for i in $(ALL_MODULES); do \
	  $(REMAKECMD) CUR_MODULE=$$i $(subst all,,$@); \
	  if [ $${PIPESTATUS[0]} != 0 ]; then exit 1; fi; \
      done
else
	$(hide) let count=0; for i in $(ALL_MODULES); do \
	$(REMAKECMD) CUR_MODULE=$$i $(subst all,,$@); \
	last_return_code=$${PIPESTATUS[0]}; \
	if [ $$last_return_code != 0 ]; then let count=$$count+$$last_return_code; fi; \
	done; \
	exit $$count
endif


ANDROID_NATIVE_TARGETS := \
         update-api \
         cts sdk win_sdk otapackage banyan_addon dist updatepackage \
         snod bootimage systemimage recoveryimage secroimage target-files-package \
         factoryimage userdataimage userdataimage-nodeps
ANDROID_NATIVE_TARGETS += dump-comp-build-info
.PHONY: $(ANDROID_NATIVE_TARGETS)

systemimage: check-modem

$(ANDROID_NATIVE_TARGETS):
	$(hide) \
        $(if $(filter update-api,$@),\
          $(if $(filter true,$(strip $(BUILD_TINY_ANDROID))), \
            echo SKIP $@... \
            , \
            $(if $(filter snod userdataimage-nodeps,$@), \
              , \
              /usr/bin/perl mediatek/build/tools/mtkBegin.pl $(PROJECT) && \
             ) \
            $(REMAKECMD) ACTION=$@ CUR_MODULE=$@ android \
           ) \
          , \
          $(if $(filter snod userdataimage-nodeps,$@), \
            , \
            /usr/bin/perl mediatek/build/tools/mtkBegin.pl $(PROJECT) && \
           ) \
          $(if $(filter banyan_addon,$@), \
            $(REMAKECMD) ACTION=sdk_addon CUR_MODULE=sdk_addon android \
            , \
            $(REMAKECMD) ACTION=$@ CUR_MODULE=$@ android \
           ) \
         )

update-api: $(ALLJAVAOPTFILES)
banyan_addon: $(ALLJAVAOPTFILES)
win_sdk: $(ALLJAVAOPTFILES)

ifeq ($(TARGET_PRODUCT),emulator)
   TARGET_PRODUCT := generic
endif
.PHONY: mm
ifeq ($(HAVE_PREPROCESS_FLOW),true)
mm: run-preprocess
endif
mm:
	$(hide) echo $(SHOWTIME) $@ing...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) (source build/envsetup.sh;cd $(MM_PATH);TARGET_PRODUCT=$(TARGET_PRODUCT) FLAVOR=$(FLAVOR) mm $(SNOD) $(DEAL_STDOUT_MM);exit $${PIPESTATUS[0]})  && \
          $(SHOWRSLT) $$? $(LOG)$@.log || \
          $(SHOWRSLT) $$? $(LOG)$@.log

.PHONY: rel-cust
ifeq ($(DUMP),true)
rel-cust: dump_option := -d
endif
rel-cust: 
	$(hide) echo $(SHOWTIME) $@ing...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) python mediatek/build/tools/customRelease.py $(dump_option) ./ $(RELEASE_DEST) $(TARGET_PRODUCT) $(MTK_RELEASE_PACKAGE).xml $(DEAL_STDOUT_CUSTREL) && \
         $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
         $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log

clean:
	$(hide) $(REMAKECMD) ACTION=$@ $(CUR_MODULE)

mrproper:
	$(hide) $(REMAKECMD) ACTION=$@ $(CUR_MODULE)

remake:
	$(hide) /usr/bin/perl mediatek/build/tools/mtkBegin.pl $(PROJECT)
	$(hide) $(REMAKECMD) ACTION= $(CUR_MODULE)

#### Remove old modem files under mediatek/custom/out/$project/modem ####
clean-modem:
	$(hide) rm -rf $(strip $(MTK_ROOT_CUSTOM_OUT))/modem
	$(hide) rm -rf $(strip $(LOGDIR)/$(PROJECT))/system/etc/extmddb
	$(hide) rm -rf $(strip $(LOGDIR)/$(PROJECT))/system/etc/mddb

update-modem: clean-modem custgen check-modem sign-modem
	$(hide) echo $(SHOWTIME) $@ing...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) ./makeMtk $(FULL_PROJECT) mm build/target/board/ snod $(DEAL_STDOUT_UPDATE_MD) && \
         $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
         $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log


drvgen:PRIVATE_CUSTOM_KERNEL_DCT:= $(if $(CUSTOM_KERNEL_DCT),$(CUSTOM_KERNEL_DCT),dct)
drvgen:
#//<2013/03/07-22571-stevenchen, Fix drvgen issue.
#//<2013/03/06-22514-stevenchen, Fix the issue of adding HW feature.
#//<2013/03/04-22431-stevenchen, Add feature and dws file for different HW versions.
ifneq ($(PROJECT),generic)
ifneq ($(strip $(ARIMA_HW_VERSION)),NONE)
	cp mediatek/custom/$(PROJECT)/kernel/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/$(ARIMA_HW_VERSION).dws mediatek/custom/$(PROJECT)/kernel/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws
endif
	$(hide) echo $(SHOWTIME) $@ing...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) mediatek/dct/DrvGen mediatek/custom/$(PROJECT)/kernel/dct/$(PRIVATE_CUSTOM_KERNEL_DCT)/codegen.dws $(DEAL_STDOUT_DRVGEN) && \
	 $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
	 $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log
endif
#//>2013/03/04-22431-stevenchen
#//>2013/03/06-22514-stevenchen
#//>2013/03/07-22571-stevenchen
ifneq ($(MTK_DRVGEN_SUPPORT),no)
codegen: drvgen
endif
ifneq ($(MTK_BTCODEGEN_SUPPORT),no)
codegen: btcodegen
endif
codegen:
ifneq ($(PROJECT),generic)
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_CODEGEN_LOG)
	$(hide) perl mediatek/build/tools/codegen.pl $(LOGDIR) $(DEAL_STDOUT_CODEGEN) && \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(CODEGEN_LOG) || \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(CODEGEN_LOG)
endif

ifneq ($(PROJECT),generic)
  ifeq ($(MTK_BT_SUPPORT), yes)
btcodegen: BT_DB_AUTO_GEN_SCRIPTS_ENTRY := mediatek/build/tools/BTCodegen.pl
btcodegen: BT_DB_AUTO_GEN_SCRIPTS_PATH := mediatek/protect/external/bluetooth/blueangel/_bt_scripts
btcodegen: CGEN_EXECUTABLE := mediatek/cgen/Cgen
btcodegen: CGEN_HOST_CFG := mediatek/cgen/cgencfg/pc_cnf
btcodegen: CGEN_TARGET_CFG := mediatek/cgen/cgencfg/tgt_cnf
btcodegen:
    # Todo: use partial source flag to wrap here
    ifneq ($(wildcard mediatek/protect/external/bluetooth/blueangel/_bt_scripts/BTCodegen.pl),)
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl $(BT_DB_AUTO_GEN_SCRIPTS_ENTRY) \
                     $(BT_DB_AUTO_GEN_SCRIPTS_PATH) \
                     $(CGEN_EXECUTABLE) \
                     $(CGEN_HOST_CFG) \
                     $(CGEN_TARGET_CFG) \
                     $(PROJECT) $(DEAL_STDOUT_BTCODEGEN) && \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log
    else # partial source building
btcodegen:
	$(hide) echo BT database auto-gen process disabled due to BT_DB_AUTO_GEN_SCRIPTS_PATH is not exist.
    endif
  else
btcodegen:
	$(hide) echo BT database auto-gen process disabled due to Bluetooth is turned off.
  endif
else
btcodegen: ;
endif

custgen:
	$(hide) echo $(SHOWTIME) $@ing...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) make -f mediatek/build/custgen.mk $(DEAL_STDOUT_CUSTGEN) && \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log
#	$(hide) echo $(SHOWTIME) $@ing ...
#	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
#	$(hide) perl mediatek/build/tools/mtkCustom.pl $(PRJ_MF) $(DEAL_STDOUT_CUSTGEN) && \
#	  $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
#	  $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log

JAVAOPTFILEPATH := mediatek/frameworks/common/src/com/mediatek/common/featureoption
JAVAOPTFILE := $(JAVAOPTFILEPATH)/FeatureOption.java

$(JAVAOPTFILE): mediatek/build/tools/javaoptgen.pl $(PRJ_MF) $(OPTR_MF) mediatek/build/tools/javaoption.pm
	$(hide) echo $(SHOWTIME) gen $@ ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)javaoptgen.log
	$(hide) perl mediatek/build/tools/javaoptgen.pl $(PRJ_MF) $(OPTR_MF) $(DEAL_STDOUT_JAVAOPTGEN)

JAVAIMEOPTFILE := $(JAVAOPTFILEPATH)/IMEFeatureOption.java
$(JAVAIMEOPTFILE): mediatek/build/tools/gen_java_ime_definition.pl $(PRJ_MF)
	$(hide) echo $(SHOWTIME) gen $@ ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)imejavaoptgen.log
	$(hide) perl mediatek/build/tools/gen_java_ime_definition.pl $(PRJ_MF) $(DEAL_STDOUT_IMEJAVAOPTGEN)

ALLJAVAOPTFILES := $(JAVAIMEOPTFILE) $(JAVAOPTFILE)
clean-javaoptgen:
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo clean $(ALLJAVAOPTFILES)
	$(hide) rm -rf $(ALLJAVAOPTFILES)

javaoptgen: $(ALLJAVAOPTFILES)
	$(hide) echo Done java optgen

#javaoptgen:
#	$(hide) echo $(SHOWTIME) $@ing ...
#	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
#	$(hide) perl mediatek/build/tools/javaoptgen.pl $(PRJ_MF) $(OPTR_MF) $(DEAL_STDOUT_JAVAOPTGEN) && \
#	perl mediatek/build/tools/gen_java_ime_definition.pl $(PRJ_MF) $(DEAL_STDOUT_IMEJAVAOPTGEN) && \
#	  $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
#          $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log

sign-image:
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl mediatek/build/tools/SignTool/SignTool.pl $(PROJECT) $(FULL_PROJECT) $(MTK_SEC_SECRO_AC_SUPPORT) $(MTK_NAND_PAGE_SIZE) $(DEAL_STDOUT_SIGN_IMAGE) && \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
          $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log

encrypt-image:
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl mediatek/build/tools/encrypt_image.pl $(PROJECT) $(DEAL_STDOUT_ENCRYPT_IMAGE) && \
          $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
          $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log

#//<20130603-25620-Eric Lin, Sign modem support live.
ifeq ($(filter generic banyan_addon,$(PROJECT)),)
sign-modem: custgen
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl mediatek/build/tools/sign_modem.pl \
			$(MTK_SEC_MP_KEY) \
                     $(FULL_PROJECT) \
                     $(MTK_SEC_MODEM_ENCODE) \
                     $(MTK_SEC_MODEM_AUTH) \
                     $(PROJECT) \
                     $(MTK_SEC_SECRO_AC_SUPPORT) \
                     $(DEAL_STDOUT_SIGN_MODEM) && \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log
else # TARGET no modem
sign-modem: ;
endif
# //>20130603-25620-Eric Lin

MODEM_INFO_FLAG := $(foreach f, $(CUSTOM_MODEM), $(wildcard $(MTK_ROOT_CUSTOM)/common/modem/$(f)/modem*.info))

modem-info: clean-modem custgen
modem-info: PRIVATE_MODEM_PATH := $(strip $(MTK_ROOT_CUSTOM_OUT))/modem
modem-info: PRIVATE_CHK_MD_TOOL := mediatek/build/tools/checkMD.pl
modem-info:
	$(hide) echo MODEM_INFO_FLAG = $(MODEM_INFO_FLAG)
ifneq ($(strip $(MODEM_INFO_FLAG)),)
   ifeq ($(strip $(MTK_ENABLE_MD1)),yes)
	$(hide) echo ==== Modem info. of MD1 ===
	$(hide) cat $(PRIVATE_MODEM_PATH)/modem.info
	$(hide) echo ""
    endif
    ifeq ($(strip $(MTK_ENABLE_MD2)),yes)
	$(hide) echo ==== Modem info. of MD2 ===
	$(hide) cat $(PRIVATE_MODEM_PATH)/modem_sys2.info
	$(hide) echo ""
    endif
else
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl $(PRIVATE_CHK_MD_TOOL) \
                     PROJECT=$(PROJECT) \
                     PRIVATE_MODEM_PATH=$(PRIVATE_MODEM_PATH) \
                     MTK_PLATFORM=$(MTK_PLATFORM) \
                     MTK_MODEM_SUPPORT=$(MTK_MODEM_SUPPORT) \
                     MTK_MD2_SUPPORT=$(MTK_MD2_SUPPORT) \
                     MTK_GET_BIN_INFO=$@ \
                     $(DEAL_STDOUT_MODEM_INFO) && \
          $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
          $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log
endif #$(MODEMINFO_Exist_Flag)

ifeq ($(filter generic banyan_addon,$(PROJECT)),)
  ifneq ($(MTK_PLATFORM),MT8320)
check-modem: clean-modem custgen
check-modem: PRIVATE_CHK_MD_TOOL := mediatek/build/tools/checkMD.pl
check-modem: PRIVATE_MODEM_PATH := $(strip $(MTK_ROOT_CUSTOM_OUT))/modem
check-modem:
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl $(PRIVATE_CHK_MD_TOOL) \
                     PROJECT=$(PROJECT) \
                     PRIVATE_MODEM_PATH=$(PRIVATE_MODEM_PATH) \
                     MTK_PLATFORM=$(MTK_PLATFORM) \
                     MTK_MODEM_SUPPORT=$(MTK_MODEM_SUPPORT) \
                     MTK_MD2_SUPPORT=$(MTK_MD2_SUPPORT) \
                     $(DEAL_STDOUT_CHECK_MODEM) && \
          $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
          $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log
  else # no modem on MT8320 now
check-modem: ;
  endif
else # TARGET no modem
check-modem: ;
endif

emigen:
ifeq (,$(filter MT8320 MT6575 MT6577 MT6589,$(MTK_PLATFORM)))
ifneq ($(PROJECT), generic)
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl mediatek/build/tools/emigen/$(MTK_PLATFORM)/emigen.pl $(CUSTOM_MEMORY_HDR) \
                     $(MEMORY_DEVICE_XLS) $(MTK_PLATFORM) $(PROJECT) $(DEAL_STDOUT_EMIGEN) && \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log
endif
endif

nandgen:
ifneq ($(PROJECT), generic)
ifneq ($(strip $(MTK_EMMC_SUPPORT)),yes)
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl mediatek/build/tools/emigen/$(MTK_PLATFORM)/nandgen.pl \
                     $(CUSTOM_NAND_HDR) \
                     $(MEMORY_DEVICE_XLS) \
                     $(MTK_PLATFORM) \
                     $(PROJECT) \
                     $(MTK_NAND_PAGE_SIZE) \
                     $(DEAL_STDOUT_NANDGEN) && \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log
endif
endif

dump-memusage: MEM_USAGE_LABEL := $(if $(LABEL),$(LABEL),$(shell date +%Y-%m-%d_%H:%M:%S))
dump-memusage: MEM_USAGE_GENERATOR := mediatek/build/tools/memmon/rommon.pl
dump-memusage: PRIVATE_PROJECT := $(if $(filter emulator, $(PROJECT)),generic,$(PROJECT))
dump-memusage: MEM_USAGE_DATA_LOCATION := mediatek/build/tools/memmon/data
dump-memusage: IMAGE_LOCATION := out/target/product/$(PRIVATE_PROJECT)
dump-memusage:
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl $(MEM_USAGE_GENERATOR) \
                     $(MEM_USAGE_LABEL) \
                     $(PRIVATE_PROJECT) \
                     $(FLAVOR) \
                     $(MEM_USAGE_DATA_LOCATION) \
                     $(IMAGE_LOCATION) \
                     $(DEAL_STDOUT_DUMP_MEMUSAGE) && \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
                $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log

ptgen:
ifneq ($(PROJECT), generic)
	$(hide) echo $(SHOWTIME) $@ing ...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) perl $(MEM_PARTITION_GENERATOR) \
                     MTK_PLATFORM=$(MTK_PLATFORM) \
                     PROJECT=$(PROJECT) \
                     FULL_PROJECT=$(FULL_PROJECT) \
                     MTK_LCA_SUPPORT=$(MTK_LCA_SUPPORT) \
                     MTK_NAND_PAGE_SIZE=$(MTK_NAND_PAGE_SIZE) \
                     MTK_EMMC_SUPPORT=$(MTK_EMMC_SUPPORT) \
                     EMMC_CHIP=$(EMMC_CHIP) \
                     MTK_LDVT_SUPPORT=$(MTK_LDVT_SUPPORT) \
                     TARGET_BUILD_VARIANT=$(TARGET_BUILD_VARIANT) \
                     MTK_EMMC_OTP_SUPPORT=$(MTK_EMMC_SUPPORT_OTP) \
                     $(DEAL_STDOUT_PTGEN) && \
                     $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
                     $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log

	$(hide) perl $(OTA_SCATTER_GENERATOR) $(SCATTER_FILE) $(OTA_SCATTER_FILE)
endif

gen-relkey: PRIVATE_KEY_GENERATOR := development/tools/make_key
gen-relkey: PRIVATE_KEY_LOCATION := build/target/product/security/$(TARGET_PRODUCT)
gen-relkey: PRIVATE_KEY_LIST := releasekey media shared platform
gen-relkey: PRIVATE_SIGNATURE_SUBJECT := $(strip $(SIGNATURE_SUBJECT))
gen-relkey:
	$(hide) echo "Generating release key/certificate..."
	$(hide) if [ ! -d $(PRIVATE_KEY_LOCATION) ]; then \
                  mkdir $(PRIVATE_KEY_LOCATION); \
                fi
	$(hide) for key in $(PRIVATE_KEY_LIST); do \
                  $(PRIVATE_KEY_GENERATOR) $(strip $(PRIVATE_KEY_LOCATION))/$$key '$(PRIVATE_SIGNATURE_SUBJECT)' < /dev/null; \
                done

# check unused application resource
check-appres: PRIVATE_SCANNING_FOLDERS := packages/apps
check-appres: PRIVATE_CHECK_TOOL := mediatek/build/tools/FindDummyRes.py
check-appres:
	$(hide) echo $(SHOWTIME) $(SHOWBUILD)ing $@...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) rm -rf $(LOG)$@.log*
	$(hide) for d in $(PRIVATE_SCANNING_FOLDERS); do \
                  $(PRIVATE_CHECK_TOOL) -d $$d \
                  $(DEAL_STDOUT_CHK_APPRES); \
                done
	$(hide) $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log

preloader:
ifeq ($(BUILD_PRELOADER),yes)
	$(hide) echo $(SHOWTIME) $(SHOWBUILD)ing $@...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_MODULE_LOG)
ifneq ($(ACTION), )
	$(hide) cd $(PRELOADER_WD) && \
	  (make clean $(DEAL_STDOUT) && \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION) || \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION)) && cd $(MKTOPDIR)
else
ifneq (,$(filter MT6516 MT6575 MT6577 MT6573 MT6589,$(MTK_PLATFORM)))
	$(hide) perl mediatek/build/tools/emigen/$(MTK_PLATFORM)/emigen.pl $(CUSTOM_MEMORY_HDR) \
                $(MEMORY_DEVICE_XLS) $(MTK_PLATFORM) $(PROJECT) $(DEAL_STDOUT)
endif
ifneq ($(strip $(MTK_EMMC_SUPPORT)),yes)
	$(hide) perl mediatek/build/tools/emigen/$(MTK_PLATFORM)/nandgen.pl \
                     $(CUSTOM_NAND_HDR) \
                     $(MEMORY_DEVICE_XLS) \
                     $(MTK_PLATFORM) \
                     $(PROJECT) \
                     $(MTK_NAND_PAGE_SIZE) \
                     $(DEAL_STDOUT)
endif
	$(hide) perl $(MEM_PARTITION_GENERATOR) \
                     MTK_PLATFORM=$(MTK_PLATFORM) \
                     PROJECT=$(PROJECT) \
                     FULL_PROJECT=$(FULL_PROJECT) \
                     MTK_LCA_SUPPORT=$(MTK_LCA_SUPPORT) \
                     MTK_NAND_PAGE_SIZE=$(MTK_NAND_PAGE_SIZE) \
                     MTK_EMMC_SUPPORT=$(MTK_EMMC_SUPPORT) \
                     EMMC_CHIP=$(EMMC_CHIP) \
                     MTK_LDVT_SUPPORT=$(MTK_LDVT_SUPPORT) \
                     TARGET_BUILD_VARIANT=$(TARGET_BUILD_VARIANT) \
                     MTK_EMMC_OTP_SUPPORT=$(MTK_EMMC_SUPPORT_OTP) \
                     $(DEAL_STDOUT_PTGEN)

	$(hide) perl $(OTA_SCATTER_GENERATOR) $(SCATTER_FILE) $(OTA_SCATTER_FILE)

	$(hide) cd $(PRELOADER_WD) && \
	  (./build.sh $(PROJECT) $(ACTION) $(DEAL_STDOUT) && \
	  cd $(MKTOPDIR) && \
          $(call chkImgSize,$(ACTION),$(PROJECT),$(SCATTER_FILE),$(PRELOADER_IMAGES),$(DEAL_STDOUT),&&) \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) || \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG))
endif
else
	$(hide) echo Not support $@.
endif

lk:
ifeq ($(BUILD_LK),yes)
	$(hide) echo $(SHOWTIME) $(SHOWBUILD)ing $@...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_MODULE_LOG)
ifneq ($(ACTION), )
	$(hide) cd $(LK_WD) && \
	  (PROJECT=$(PROJECT) make clean $(DEAL_STDOUT) && \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION) || \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION)) && cd $(MKTOPDIR)
else
ifneq (,$(filter MT6516 MT6575 MT6577 MT6573,$(MTK_PLATFORM)))
	$(hide) perl mediatek/build/tools/emigen/$(MTK_PLATFORM)/emigen.pl $(CUSTOM_MEMORY_HDR) \
                $(MEMORY_DEVICE_XLS) $(MTK_PLATFORM) $(PROJECT) $(DEAL_STDOUT)
endif
ifneq ($(strip $(MTK_EMMC_SUPPORT)),yes)
	$(hide) perl mediatek/build/tools/emigen/$(MTK_PLATFORM)/nandgen.pl \
                     $(CUSTOM_NAND_HDR) \
                     $(MEMORY_DEVICE_XLS) \
                     $(MTK_PLATFORM) \
                     $(PROJECT) \
                     $(MTK_NAND_PAGE_SIZE) \
                     $(DEAL_STDOUT)
endif
	$(hide) perl $(MEM_PARTITION_GENERATOR) \
                     MTK_PLATFORM=$(MTK_PLATFORM) \
                     PROJECT=$(PROJECT) \
                     FULL_PROJECT=$(FULL_PROJECT) \
                     MTK_LCA_SUPPORT=$(MTK_LCA_SUPPORT) \
                     MTK_NAND_PAGE_SIZE=$(MTK_NAND_PAGE_SIZE) \
                     MTK_EMMC_SUPPORT=$(MTK_EMMC_SUPPORT) \
                     EMMC_CHIP=$(EMMC_CHIP) \
                     MTK_LDVT_SUPPORT=$(MTK_LDVT_SUPPORT) \
                     TARGET_BUILD_VARIANT=$(TARGET_BUILD_VARIANT) \
                    MTK_EMMC_OTP_SUPPORT=$(MTK_EMMC_SUPPORT_OTP) \
                     $(DEAL_STDOUT_PTGEN)

# disable ota scatter generation
#	$(hide) perl $(OTA_SCATTER_GENERATOR) $(SCATTER_FILE) $(OTA_SCATTER_FILE)

	 $(hide) cd $(LK_WD) && \
	  (FULL_PROJECT=$(FULL_PROJECT) make $(MAKEJOBS) $(PROJECT) $(ACTION) $(DEAL_STDOUT) && \
	  cd $(MKTOPDIR) && \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) || \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG))


#	  cd $(MKTOPDIR) && \
#	   $(call chkImgSize,$(ACTION),$(PROJECT),$(SCATTER_FILE),$(LK_IMAGES),$(DEAL_STDOUT) &&) \

endif
else
	$(hide) echo Not support $@.
endif

uboot:
ifeq ($(BUILD_UBOOT),yes)
	$(hide) echo $(SHOWTIME) $(SHOWBUILD)ing $@...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_MODULE_LOG)
ifneq ($(ACTION), )
	$(hide) cd $(UBOOT_WD) && \
	  (make distclean $(DEAL_STDOUT) && \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION) || \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION)) && cd $(MKTOPDIR)
else
ifneq (,$(filter MT6516 MT6575 MT6577 MT6573,$(MTK_PLATFORM)))
	$(hide) perl mediatek/build/tools/emigen/$(MTK_PLATFORM)/emigen.pl $(CUSTOM_MEMORY_HDR) \
                $(MEMORY_DEVICE_XLS) $(MTK_PLATFORM) $(PROJECT) $(DEAL_STDOUT)
endif
ifneq ($(strip $(MTK_EMMC_SUPPORT)),yes)
	$(hide) perl mediatek/build/tools/emigen/$(MTK_PLATFORM)/nandgen.pl \
                     $(CUSTOM_NAND_HDR) \
                     $(MEMORY_DEVICE_XLS) \
                     $(MTK_PLATFORM) \
                     $(PROJECT) \
                     $(MTK_NAND_PAGE_SIZE) \
                     $(DEAL_STDOUT)
endif
	$(hide) perl $(MEM_PARTITION_GENERATOR) \
                     MTK_PLATFORM=$(MTK_PLATFORM) \
                     PROJECT=$(PROJECT) \
                     FULL_PROJECT=$(FULL_PROJECT) \
                     MTK_LCA_SUPPORT=$(MTK_LCA_SUPPORT) \
                     MTK_NAND_PAGE_SIZE=$(MTK_NAND_PAGE_SIZE) \
                     MTK_EMMC_SUPPORT=$(MTK_EMMC_SUPPORT) \
                     EMMC_CHIP=$(EMMC_CHIP) \
                     MTK_LDVT_SUPPORT=$(MTK_LDVT_SUPPORT) \
                     TARGET_BUILD_VARIANT=$(TARGET_BUILD_VARIANT) \
                     MTK_EMMC_OTP_SUPPORT=$(MTK_EMMC_SUPPORT_OTP) \
                     $(DEAL_STDOUT_PTGEN)

	$(hide) perl $(OTA_SCATTER_GENERATOR) $(SCATTER_FILE) $(OTA_SCATTER_FILE)

	$(hide) cd $(UBOOT_WD) && \
	  (MAKEJOBS=$(MAKEJOBS) ./build.sh $(ACTION) $(DEAL_STDOUT) && \
	  cd $(MKTOPDIR) && \
	   $(call chkImgSize,$(ACTION),$(PROJECT),$(SCATTER_FILE),$(UBOOT_IMAGES),$(DEAL_STDOUT) &&) \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) || \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG))
endif
else
	$(hide) echo Not support $@.
endif

kernel: nandgen

kernel:
ifeq ($(BUILD_KERNEL),yes)
  ifeq ($(ACTION),)
	$(hide) perl $(MEM_PARTITION_GENERATOR) \
                     MTK_PLATFORM=$(MTK_PLATFORM) \
                     PROJECT=$(PROJECT) \
                     FULL_PROJECT=$(FULL_PROJECT) \
                     MTK_LCA_SUPPORT=$(MTK_LCA_SUPPORT) \
                     MTK_NAND_PAGE_SIZE=$(MTK_NAND_PAGE_SIZE) \
                     MTK_EMMC_SUPPORT=$(MTK_EMMC_SUPPORT) \
                     EMMC_CHIP=$(EMMC_CHIP) \
                     MTK_LDVT_SUPPORT=$(MTK_LDVT_SUPPORT) \
                     TARGET_BUILD_VARIANT=$(TARGET_BUILD_VARIANT) \
                     MTK_EMMC_OTP_SUPPORT=$(MTK_EMMC_SUPPORT_OTP) \
                     $(DEAL_STDOUT_PTGEN)
	$(hide) perl $(OTA_SCATTER_GENERATOR) $(SCATTER_FILE) $(OTA_SCATTER_FILE)
  endif
  ifneq ($(KMOD_PATH),)
	$(hide)	echo building kernel module KMOD_PATH=$(KMOD_PATH)
	$(hide) cd $(KERNEL_WD) && \
	(KMOD_PATH=$(KMOD_PATH) ./build.sh $(ACTION) $(KERNEL_ARG) ) && cd $(MKTOPDIR)
  else
	$(hide) echo $(SHOWTIME) $(SHOWBUILD)ing $@...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_MODULE_LOG)
	$(hide) cd $(KERNEL_WD) && \
	  (MAKEJOBS=$(MAKEJOBS) ./build.sh $(ACTION) $(PROJECT) $(DEAL_STDOUT) && \
	   cd $(MKTOPDIR) && \
	   $(call chkImgSize,$(ACTION),$(PROJECT),$(SCATTER_FILE),$(if $(strip $(ACTION)),,$(KERNEL_IMAGES)),$(DEAL_STDOUT),&&) \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION) || \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION))
#	$(hide) $(SHOWTIMECMD) 
  endif
else
	$(hide) echo Not support $@.
endif


ifneq ($(ACTION),clean)
  ifneq ($(MTK_SIGNMODEM_SUPPORT),no)
android: check-modem sign-modem
  else
android: check-modem
  endif
android: $(ALLJAVAOPTFILES)
else
android: clean-javaoptgen
endif
ifeq ($(HAVE_PREPROCESS_FLOW),true)
  ifeq ($(ACTION),clean)
android: clean-preprocessed
  else
android: run-preprocess
  endif
endif
android: CHECK_IMAGE := $(ANDROID_TARGET_IMAGES)
android:
ifeq ($(ACTION), )
	$(hide) /usr/bin/perl mediatek/build/tools/mtkBegin.pl $(PROJECT)
endif

ifneq ($(DR_MODULE),)
   ifneq ($(ACTION), clean)
	$(hide) echo building android module MODULE=$(DR_MODULE)
#	$(hide) perl mediatek/build/tools/javaoptgen.pl $(PRJ_MF) $(OPTR_MF)
	$(MAKECMD) $(DR_MODULE)
   else
	$(hide) echo cleaning android module MODULE=$(DR_MODULE)
	$(hide) $(MAKECMD) clean-$(DR_MODULE) 
   endif
else 
	$(hide) echo $(SHOWTIME) $(SHOWBUILD)ing $@...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_MODULE_LOG)
#ifeq ($(SHOWBUILD), build)
#	$(hide) perl mediatek/build/tools/javaoptgen.pl $(PRJ_MF) $(OPTR_MF) $(DEAL_STDOUT)
#endif
	$(hide) ($(MAKECMD) $(ACTION) $(DEAL_STDOUT);exit $${PIPESTATUS[0]}) && \
	  $(call chkImgSize,$(ACTION),$(PROJECT),$(SCATTER_FILE),$(if $(strip $(ACTION)),$(CHECK_IMAGE),$(ANDROID_IMAGES)),$(DEAL_STDOUT),&&) \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION) || \
	  $(SHOWRSLT) $${PIPESTATUS[0]} $(MODULE_LOG) $(ACTION)

ifeq ($(ACTION), )
	$(hide) /usr/bin/perl mediatek/build/tools/mtkFinalize.pl $(PROJECT) $(MTK_PLATFORM) $(MTK_EMMC_SUPPORT)
endif
endif
st:
	


define chkImgSize 
$(if $(filter no,$(MTK_CHKIMGSIZE_SUPPORT)), \
     echo "Check Img size process disabled due to MTK_CHKIMGSIZE_SUPPORT is set to no" $(5) $(6),\
     $(call chkImgSize1,$(1),$(2),$(3),$(4),$(5),$(6)) \
)
endef
##############################################################
# function:  chkImgSize1
# arguments: $(ACTION) $(PROJECT) $(SCATTER_FILE) $(IMAGES) $(DEAL_STDOUT) &&
#############################################################
define chkImgSize1
$(if $(strip $(1)), \
     $(if $(strip $(4)), \
          $(if $(filter generic, $(2)),, \
               perl mediatek/build/tools/chkImgSize.pl $(3) $(2) $(4) $(5) $(6) \
           ) \
      ), \
     $(if $(filter generic, $(2)),, \
         perl mediatek/build/tools/chkImgSize.pl $(3) $(2) $(4) $(5) $(6) \
      ) \
 ) 
endef


bindergen: 
	$(hide) echo $(SHOWTIME) $@ing...
	$(hide) echo -e \\t\\t\\t\\b\\b\\b\\bLOG: $(S_LOG)$@.log
	$(hide) mediatek/build/tools/bindergen/bindergen.pl $(DEAL_STDOUT_BINDERGEN) && \
	 $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log || \
	 $(SHOWRSLT) $${PIPESTATUS[0]} $(LOG)$@.log
