#
# Makefile for the cmmb drivers.
#
#obj-m    	:= Inno.o
ifeq ($(HAVE_CMMB_FEATURE),yes)

obj-$(CONFIG_MTK_CMMB)	:= cmmbdrv.o
cmmbdrv-objs := inno_core.o inno_lgxchar.o inno_spi.o mmis_cmdset.o platform.o

# check the cmmb chip ,then use the matched firmware
ifeq ($(MTK_CMMB_CHIP),Innofidei_IF228_20M)
ccflags-y += -DFLAG_CHIP_IF228_20M
endif
ifeq ($(MTK_CMMB_CHIP),Innofidei_IF228_26M)
ccflags-y += -DFLAG_CHIP_IF228_26M
endif
ifeq ($(MTK_CMMB_CHIP),Innofidei_IF238_20M)
ccflags-y += -DFLAG_CHIP_IF238_20M
endif
ifeq ($(MTK_CMMB_CHIP),Innofidei_IF238_26M)
ccflags-y += -DFLAG_CHIP_IF238_26M
endif
ifeq ($(MTK_CMMB_CHIP),Innofidei_IF258_20M)
ccflags-y += -DFLAG_CHIP_IF258_20M
endif
ifeq ($(MTK_CMMB_CHIP),Innofidei_IF258_26M)
ccflags-y += -DFLAG_CHIP_IF258_26M
endif

ifeq ($(MTK_PLATFORM),MT6589)
ccflags-y += -DMT6589_CMMB_SPI_CONFIG
endif

endif
