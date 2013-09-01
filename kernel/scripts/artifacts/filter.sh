if [[ $1 == "other" ]]; then
  rm -rf arch/arm/mach-mt3351
  rm -rf drivers/actuator
  rm -rf drivers/meta
  rm -rf drivers/monitor
  rm -f  arch/arm/mach-mt6516/include/mach/mt6516_IDP.h
  rm -f  arch/arm/mach-mt6516/include/mach/mt6516_ISP.h
  rm -f  arch/arm/mach-mt6516/include/mach/mt6516_auxadc_hw.h
  rm -f  arch/arm/mach-mt6516/include/mach/mt6516_auxadc_sw.h
  rm -f  arch/arm/mach-mt6516/include/mach/mt6516_gpt_sw.h
  rm -f  arch/arm/mach-mt6516/include/mach/mt6516_wdt.h
  rm -f  arch/arm/mach-mt6516/include/mach/mt6516_busmonitor.h

  rm -f arch/arm/mach-mt6516/core.c
  rm -f arch/arm/mach-mt6516/pwm.c
  rm -f arch/arm/mach-mt6516/dma.c
  rm -f arch/arm/mach-mt6516/mt6516_pll.c
  rm -f arch/arm/mach-mt6516/mt6516_wdt.c
  rm -f arch/arm/mach-mt6516/mt6516_timer.c
  rm -f arch/arm/mach-mt6516/gpt.c
  rm -f arch/arm/mach-mt6516/mt6516_IDP.c
  rm -f arch/arm/mach-mt6516/mt6516_ISP.c
  rm -f arch/arm/mach-mt6516/system.c
  rm -f arch/arm/mach-mt6516/mt6516_busmonitor.c
  rm -f arch/arm/mach-mt6516/mt6516_devs.c
  rm -f drivers/char/sampletrigger.c
  rm -f drivers/mmc/host/mt6516_sd.c
  rm -f drivers/power/smart_battery_mt6516.c
fi

rm -f arch/arm/mach-mt6516/MT6516_sleep.c
rm -f arch/arm/mach-mt6516/mt6516_intr.c
rm -f arch/arm/mach-mt6516/MT6516_PM_api.c
rm -f arch/arm/mach-mt6516/ccci*c
rm -f drivers/net/wireless/mt592x/*.c

rm -f arch/arm/mach-mt6516/include/mach/ccci*.h
rm -f arch/arm/mach-mt6516/include/mach/ccif.h
