LOCAL_PATH:= $(call my-dir)

# Common C++ flags to build this library.
# Note that we need to access private Bionic headers
# and define ANDROID_SMP accordingly.
libstdc++_cflags := -Ibionic/libc/
ifeq ($(TARGET_CPU_SMP),true)
    libstdc++_cflags += -DANDROID_SMP=1
else
    libstdc++_cflags += -DANDROID_SMP=0
endif

ifeq ($(TARGET_BUILD_VARIANT),eng)
  ifeq ($(filter banyan_addon banyan_addon_x86,$(TARGET_PRODUCT)),)
    ifeq ($(MTK_INTERNAL),yes)
      libstdc++_cflags += \
		-D_MTK_ENG_ \
		-fno-omit-frame-pointer \
		-mapcs
    endif
  endif
endif

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
	src/one_time_construction.cpp \
	src/new.cpp \
	src/pure_virtual.cpp \
	src/typeinfo.cpp

LOCAL_MODULE:= libstdc++
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_CFLAGS := $(libstdc++_cflags)

LOCAL_SYSTEM_SHARED_LIBRARIES := libc

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
	src/one_time_construction.cpp \
	src/new.cpp \
	src/pure_virtual.cpp \
	src/typeinfo.cpp

LOCAL_CFLAGS := $(libstdc++_cflags)

LOCAL_MODULE:= libstdc++
LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

LOCAL_SYSTEM_SHARED_LIBRARIES := libc

include $(BUILD_STATIC_LIBRARY)
