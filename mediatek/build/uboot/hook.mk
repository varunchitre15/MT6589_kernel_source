include ../../../mediatek/build/Makefile
$(call codebase-path,uboot)

$(VERSION_FILE) version: setlocalversion_chmod
setlocalversion_chmod:
	chmod a+x $(TOPDIR)/tools/setlocalversion

emigen_files := $(MTK_ROOT_CUSTOM)/$(MTK_PROJECT)/uboot/inc/memory_info.h
.PHONY: custom_files
custom-files := $(strip $(call mtk.custom.generate-rules,$(obj)include/autoconf.mk,uboot kernel,$(emigen_files)))

$(emigen_files):
	cd $(to-root); ./makeMtk $(MTK_PROJECT) emigen; cd -;

ifdef SOC
LIBS := $(filter-out $(CPUDIR)/$(SOC)/lib$(SOC).a,$(LIBS))
LIBS += $(MTK_PATH_PLATFORM)/lib$(SOC).a
endif

LIBBOARD = $(MTK_PATH_CUSTOM)/lib$(BOARD).a
