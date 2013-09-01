#
# Makefile for the linux kernel.
#

obj-y := core.o irq.o system.o timer.o clock.o mt_dbg.o
obj-y += mt_devs.o mt_gpt.o mt_boot.o board.o mt_gpio.o mt_pm_init.o
obj-y += ca7_timer.o dram_overclock.o
ifeq ($(CONFIG_SMP),y)
obj-y += mt-smp.o hotplug.o mt-headsmp.o mt_hotplug.o mt_hotplug_mechanism.o
endif
obj-y += mt_eint.o mt_freqhopping.o bus_fabric.o
obj-$(CONFIG_MT65XX_TRACER) += pmu_v7.o mon_interface.o mt_mon.o mt_emi_bm.o
obj-y += emi_bwl.o emi_mpu.o
obj-y += mt_auxadc.o
obj-y += camera_isp.o
obj-y += camera_sysram.o 
obj-y += camera_pipe_mgr.o
#obj-y += camera_eis.o
#obj-y += camera_fdvt.o
obj-y += mt_pm_ldo.o
obj-y += mt_cache_v7.o innercache.o mt_cache_dump.o
obj-y += mt_dma.o
ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/power \
             -I$(MTK_PATH_SOURCE)/drivers/wdk
obj-y += mt_spm.o mt_spm_sleep.o mt_spm_idle.o mt_spm_mtcmos.o
obj-y += mt_sleep.o
obj-y += mt_cpufreq.o
obj-y += mt_gpufreq.o
obj-y += mt_emifreq.o
obj-y += mt_cpu_ss.o
obj-y += mt_dormant.o cpu_dormant.o
obj-y += mt_dcm.o mt_idle.o
obj-y += mt_clkmgr.o
obj-y += mt_pwm.o
obj-y += mt_golden_setting.o
obj-y += sbchk_base.o
obj-y += sec_osal.o
obj-$(CONFIG_MTK_CIRQ) += mt_cirq.o
obj-$(CONFIG_MTK_MCI) += mt_mci.o
obj-y += mtk_ccci_helper.o
obj-$(CONFIG_SMP) += fiq_smp_call.o
obj-y += mssv.o
obj-y += mt_dsizer.o
obj-$(CONFIG_MTK_MEMCFG) += mtk_memcfg.o

ifneq ($(CONFIG_MTK_CCCI_DEVICES),y)
ccflags-y  += -D__USING_DUMMY_CCCI_API__
endif

ifeq ($(CONFIG_THERMAL),y)
    obj-y += mtk_ts_cpu.o
    obj-y += mtk_ts_pmic.o
    obj-y += mtk_ts_pa.o
	
    ifeq ($(MTK_ENABLE_MD2),yes)
    	ifeq ($(MTK_MD2_SUPPORT),modem2_3g)
	    	obj-y += mtk_ts_tdpa.o
	    endif
    endif
	
    ifeq ($(CONFIG_POWER_EXT), y)
    else
        ifeq ($(CONFIG_MTK_SMART_BATTERY), y)
            # Always align the Makefile in mediatek/platform/mt6589/kernel/drivers/power/
            ifeq ($(MTK_NCP1851_SUPPORT),yes)
                obj-y += mtk_ts_battery.o
            else
                ifeq ($(MTK_BQ24196_SUPPORT),yes) 
                    obj-y += mtk_ts_battery.o
                else
                    # FAN5405 or PMIC6320 pulse charging
                    obj-y += mtk_ts_battery.o
                endif
            endif
        endif
    endif
    ifeq ($(MTK_PCB_BATTERY_SENSOR), yes)
        obj-y += mtk_ts_battery2.o
    endif
endif

obj-y += mt_ptp.o

# Hibernation
AFLAGS_swsusp.o	:= -DTEXT_OFFSET=$(TEXT_OFFSET)
obj-$(CONFIG_HIBERNATION)	+= cpu_hibernate.o swsusp.o
obj-$(CONFIG_MTK_HIBERNATION) += mtk_hibernate_dpm.o mtk_hibernate_core.o

ifneq ($(MTK_TABLET_HARDWARE), )
ccflags-y += -DMTK_TABLET_PLATFORM=\"$(shell echo $(MTK_TABLET_HARDWARE) | tr a-z A-Z )\"
endif

KBUILD_AFLAGS	:=$(KBUILD_AFLAGS:-msoft-float=-Wa,-mfpu=neon)


