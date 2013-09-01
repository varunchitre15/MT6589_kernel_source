LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
TARGET_PROVIDES_INIT_RC := true

PRODUCT_COPY_FILES += $(LOCAL_PATH)/init.rc:root/init.rc

