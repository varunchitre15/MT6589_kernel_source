


obj-$(CONFIG_MTK_PMIC)          += pmic_mt6320.o upmu_common.o

ifeq ($(MTK_BQ24196_SUPPORT),yes) 
    obj-$(CONFIG_MTK_SMART_BATTERY) += mt_battery_bq24196.o mt6320_fuel_gauge_bq24196.o bq24196.o
    ifeq ($(MTK_BQ27541_SUPPORT),yes)
      obj-$(CONFIG_MTK_SMART_BATTERY) += bq27541.o
    endif
else
  ifeq ($(MTK_NCP1851_SUPPORT),yes) 
    obj-$(CONFIG_MTK_SMART_BATTERY) += mt_battery_ncp1851.o mt6320_fuel_gauge_ncp1851.o ncp1851.o
#          obj-$(CONFIG_MTK_SMART_BATTERY) += mt6320_battery.o mt6320_fuel_gauge.o
  else
      ifeq ($(MTK_FAN5405_SUPPORT),yes)
          obj-$(CONFIG_MTK_SMART_BATTERY) += mt_battery_fan5405.o mt6320_fuel_gauge.o fan5405.o
      else
#//<2013/1/21-20645-jessicatseng, [Pelican] Intrgrate charging IC BQ24157 for PRE-MP SW
     	 ifeq ($(MTK_BQ24157_SUPPORT),yes)
          obj-$(CONFIG_MTK_SMART_BATTERY) += mt_battery_bq24157.o mt6320_fuel_gauge.o bq24157.o
     	 else
          ifeq ($(MTK_BQ24158_SUPPORT),yes)
                obj-$(CONFIG_MTK_SMART_BATTERY) += mt_battery_bq24158.o mt6320_fuel_gauge.o bq24158.o
           else
				ifeq ($(MTK_BQ24156_SUPPORT),yes)
  					obj-$(CONFIG_MTK_SMART_BATTERY) += mt_battery_bq24156.o mt6320_fuel_gauge_bq24156.o bq24156.o
				else
                obj-$(CONFIG_MTK_SMART_BATTERY) += mt6320_battery.o mt6320_fuel_gauge.o
#               obj-$(CONFIG_MTK_SMART_BATTERY) += mt6320_battery.o 
				endif
         endif           
	 endif
#//>2013/1/21-20645-jessicatseng
      endif
  endif
endif
