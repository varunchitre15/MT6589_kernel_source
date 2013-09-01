## Android Makefile for kernel module
## by Kirby.Wu, 20090729, mediatek.inc
##
## this android makefile is (currently) used to build all kernel modules 
## and put them into android platform output directory. 
##
## to build kernel modules into system.img,
##   config build/target/board/<target>/BoardConfig.mk:
##     KERNEL_CONFIG_FILE := <your desired config file> # use .config if omit
##     TARGET_MODULES     := true                       # do make modules
##

#ifeq ($(MTK_PROJECT), gw616)
#  KERNEL_CONFIG_FILE := config-mt6516-phone
#else
#  ifeq ($(MTK_PROJECT), ds269)
#    KERNEL_CONFIG_FILE := config-mt6516-gemini
#  else
#    ifeq ($(MTK_PROJECT), oppo)
#      KERNEL_CONFIG_FILE := config-mt6516-oppo
#    else
#      ifeq ($(MTK_PROJECT), mt6516_evb)
#        KERNEL_CONFIG_FILE := config-mt6516-evb
#      else
#        ifeq ($(MTK_PROJECT), mt6573_evb)
#          KERNEL_CONFIG_FILE := config-mt6573-evb
#        else
#          ifeq ($(MTK_PROJECT), zte73v1)
#            KERNEL_CONFIG_FILE := config-mt6573-zte73v1
#          else
#            KERNEL_CONFIG_FILE := config-mt6516-$(MTK_PROJECT)
#          endif
#        endif
#      endif
#    endif
#  endif
#endif

$(info using $(KERNEL_CONFIG_FILE) .... )
ifeq ($(TARGET_KMODULES),true)
ALL_PREBUILT += $(TARGET_OUT)/lib/modules/modules.order
$(BUILT_SYSTEMIMAGE): kernel_modules
$(TARGET_OUT)/lib/modules/modules.order: kernel_modules

ifeq ($(strip $(MTK_WLAN_SUPPORT)),yes)
$(BUILT_SYSTEMIMAGE): wlanLink
$(TARGET_OUT)/lib/modules/modules.order: wlanLink
endif

kernel_modules:
	@echo "building linux kernel modules..."
#ifneq (,$(KERNEL_CONFIG_FILE))
#	@cat kernel/$(KERNEL_CONFIG_FILE) > kernel/.config
#endif
ifeq ($(strip $(KBUILD_OUTPUT_SUPPORT)),yes)	
	make MTK_PROJECT=$(MTK_PROJECT) -C  kernel O=out modules
	INSTALL_MOD_STRIP=1 MTK_PROJECT=$(MTK_PROJECT) INSTALL_MOD_PATH=../../$(TARGET_OUT) INSTALL_MOD_DIR=../../$(TARGET_OUT) make -C kernel O=out android_modules_install
else
	make MTK_PROJECT=$(MTK_PROJECT) -C  kernel modules
	INSTALL_MOD_STRIP=1 MTK_PROJECT=$(MTK_PROJECT) INSTALL_MOD_PATH=../$(TARGET_OUT) INSTALL_MOD_DIR=../$(TARGET_OUT) make -C kernel android_modules_install
endif
#end of KBUILD_OUTPUT_SUPPORT

################
## For WLAN switch
################
LINK_WLAN_NAME := $(TARGET_OUT)/lib/modules/wlan
LINK_P2P_NAME := $(TARGET_OUT)/lib/modules/p2p

KO_POSTFIX := _$(shell echo $(strip $(MTK_WLAN_CHIP)) | tr A-Z a-z)

CUR_WLAN_KO_NAME := wlan$(KO_POSTFIX).ko
CUR_P2P_KO_NAME := p2p$(KO_POSTFIX).ko

CUR_WLAN_KO_PATH := $(TARGET_OUT)/lib/modules/$(CUR_WLAN_KO_NAME)
CUR_P2P_KO_PATH := $(TARGET_OUT)/lib/modules/$(CUR_P2P_KO_NAME)

$(CUR_WLAN_KO_PATH) : kernel_modules 
$(CUR_P2P_KO_PATH) : kernel_modules

wlanLink : $(CUR_WLAN_KO_PATH) $(CUR_P2P_KO_PATH)
	@echo "select wlan chip"
	#select the right chip and copy
	-@ln -sf $(CUR_WLAN_KO_NAME) $(LINK_WLAN_NAME).ko
	#-@ln -sf $(CUR_P2P_KO_NAME) $(LINK_P2P_NAME).ko

endif

