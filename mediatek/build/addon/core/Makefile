# ###############################################################
# Flow control makefile
# ###############################################################



TOPDIR :=
MTK_BUILD_SYSTEM   := $(TOPDIR)mediatek/build/addon/core
MTK_PROJECT_CONFIG := $(TOPDIR)mediatek/build/addon/project
MTK_BUILD_TOOLS    := $(TOPDIR)mediatek/build/addon/tools

# Bring in user-defined make functions
include $(MTK_BUILD_SYSTEM)/definitions.mak

# include configuration interface(to Android/Kernel/Uboot/Preloader)
# the following segment will also placed in Android/Kernel/Uboot/Preloader
# configuration makefile
ifndef CONFIG_INTERFACE
  CONFIG_INTERFACE := $(TOPDIR)mediatek/build/addon/interface/config_interface.mak
  $(shell export CONFIG_INTERFACE)
  include $(CONFIG_INTERFACE)
endif
# End

.PHONY: gen_kernel_config
gen_kernel_config:
	$(hide) echo Generating kernel configuration\(.config\)...
# Todo

.PHONY: gen_java_option
gen_java_option:
	$(hide) echo Generating java configuration options\(FeatureOption.java\)...
# Todo

.PHONY: test
test:
	$(hide) echo testing...

