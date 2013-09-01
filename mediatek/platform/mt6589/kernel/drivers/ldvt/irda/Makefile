include $(MTK_ROOT_BUILD)/Makefile
KBUILD_EXTRA_SYMBOLS := net/irda/Module.symvers
CONFIG_MTK_LDVT_IRDA :=m
obj-$(CONFIG_MTK_LDVT_IRDA) += mt_irda.o
mt_irda-y += irda_$(call lc,$(MTK_PLATFORM)).o   irda_framer.o
#EXTRA_CFLAGS := "-DMT6573 "    #-DDEBUG
#obj-m := mt_irda.o
#mt_irda-y := irda_mt6573.o irda_framer.o
#ccflags-y := -DDEBUG