
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
ALL_PREBUILT += $(TARGET_OUT)/usr/share/rildata/oper.lis
$(TARGET_OUT)/usr/share/rildata/oper.lis : $(LOCAL_PATH)/oper.lis | $(ACP)
	$(transform-prebuilt-to-target)

