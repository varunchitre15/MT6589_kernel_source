####################################################
# eeprom feature dependency

ifneq ($(strip $(CUSTOM_KERNEL_EEPROM)),$(strip $(CUSTOM_HAL_EEPROM)))
  $(call dep-err-seta-or-setb,CUSTOM_KERNEL_EEPROM,$(CUSTOM_HAL_EEPROM),CUSTOM_HAL_EEPROM,$(CUSTOM_KERNEL_EEPROM))
endif
