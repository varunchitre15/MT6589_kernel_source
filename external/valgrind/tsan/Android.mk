# Copyright (C) 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)

ifneq ($(filter arm x86,$(TARGET_ARCH)),)

common_cflags := \
	-Wall -Wno-deprecated -fno-exceptions -fno-stack-protector \
	-DTS_VALGRIND=1 \
	-DTS_VERSION=\"exported\" \
	-DVGA_$(TARGET_ARCH)=1 \
	-DVGO_linux=1 \
	-DVGP_$(TARGET_ARCH)_linux=1 \
	-DVG_PLATFORM=\"$(TARGET_ARCH)-linux\" \
	-D_STLP_NO_IOSTREAMS=1 \
	-U_FORTIFY_SOURCE

common_includes := \
	external/valgrind/main \
	external/valgrind/main/include \
	external/valgrind/main/VEX/pub \
	external/valgrind/dynamic_annotations

ifeq ($(TARGET_ARCH),arm)
tool_ldflags := -static -Wl,--build-id=none,-Ttext=0x38000000 -nodefaultlibs -nostartfiles -u _start -e_start
else
tool_ldflags := -static -Wl,-Ttext=0x38000000 -nodefaultlibs -nostartfiles -u _start -e_start
endif

preload_ldflags := -nodefaultlibs -Wl,-z,interpose,-z,initfirst
# Remove this when the all toolchains are GCC 4.4
ifeq ($(TARGET_ARCH),arm)
  preload_ldflags += -Wl,--icf=none
endif

# TODO(eugenis): Add ts_event_names.h generation step

# Build tsan-$(TARGET_ARCH)-linux
include $(CLEAR_VARS)

LOCAL_MODULE := tsan-$(TARGET_ARCH)-linux
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/valgrind
LOCAL_ARM_MODE := arm
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_NO_CRT := true
LOCAL_SYSTEM_SHARED_LIBRARIES :=
LOCAL_CPP_EXTENSION := .cc
LOCAL_SRC_FILES := \
	thread_sanitizer.cc \
	ts_valgrind.cc \
	ts_valgrind_libc.cc \
	ts_util.cc \
	suppressions.cc \
	ignore.cc \
	common_util.cc \
	ts_race_verifier.cc
LOCAL_C_INCLUDES := \
        bionic \
        external/stlport/stlport \
	$(common_includes)
LOCAL_LDFLAGS := $(tool_ldflags)
LOCAL_CFLAGS := $(common_cflags)
LOCAL_CXXFLAGS := $(common_cxxflags)
LOCAL_RTTI_FLAG := -fno-rtti
LOCAL_STATIC_LIBRARIES := libcoregrind-$(TARGET_ARCH)-linux libvex-$(TARGET_ARCH)-linux

include $(BUILD_EXECUTABLE)


# Build vgpreload_tsan-$(TARGET_ARCH)-linux.so
include $(CLEAR_VARS)

LOCAL_MODULE := vgpreload_tsan-$(TARGET_ARCH)-linux
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/valgrind
LOCAL_ARM_MODE := arm
LOCAL_STRIP_MODULE := false
LOCAL_NO_CRT := true
LOCAL_PRELINK_MODULE := false
LOCAL_SRC_FILES := \
	ts_valgrind_intercepts.c
LOCAL_C_INCLUDES := $(common_includes)
LOCAL_LDFLAGS := $(preload_ldflags)
LOCAL_CFLAGS := $(common_cflags)
LOCAL_RTTI_FLAG := -fno-rtti

include $(BUILD_SHARED_LIBRARY)

endif
