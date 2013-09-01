LOCAL_DIR := $(GET_LOCAL_DIR)

# user defined macros, will be generated into build-xxx/config.h
DEFINES += \


# header include path

INCLUDES += -I$(LOCAL_DIR)/include

INCLUDES += -I$(LOCAL_DIR)/lcm/inc \
            -I$(LOCAL_DIR)/inc \
            -Icustom/$(FULL_PROJECT)/common


# object files list
# sample

OBJS += \
	$(patsubst %.c,%.o,$(call wildcard2, $(LOCAL_DIR)/lcm/*.c))\
	$(LOCAL_DIR)/partition.o \
	$(LOCAL_DIR)/init.o \
	$(LOCAL_DIR)/cust_msdc.o\
	$(LOCAL_DIR)/cust_display.o\
	$(LOCAL_DIR)/cust_leds.o\
	$(LOCAL_DIR)/power_off.o\
	$(LOCAL_DIR)/fastboot_oem_commands.o\

include $(LOCAL_DIR)/logo/rules.mk
