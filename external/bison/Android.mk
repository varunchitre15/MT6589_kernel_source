# 
# Copyright 2006 The Android Open Source Project
#
# Android Asset Packaging Tool
#

ifeq ($(HOST_OS),linux)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	lib/abitset.c \
	lib/argmatch.c \
	lib/basename.c \
	lib/bitset.c \
	lib/bitset_stats.c \
	lib/bitsetv-print.c \
	lib/bitsetv.c \
	lib/dirname.c \
	lib/dup-safer.c \
	lib/ebitset.c \
	lib/exitfail.c \
	lib/fd-safer.c \
	lib/fopen-safer.c \
	lib/get-errno.c \
	lib/hard-locale.c \
	lib/hash.c \
	lib/lbitset.c \
	lib/mbswidth.c \
	lib/pipe-safer.c \
	lib/quote.c \
	lib/quotearg.c \
	lib/stripslash.c \
	lib/subpipe.c \
	lib/timevar.c \
	lib/vbitset.c \
	lib/xalloc-die.c \
	lib/xmalloc.c \
	lib/xstrndup.c \
	\
	src/LR0.c \
	src/assoc.c \
	src/closure.c \
	src/complain.c \
	src/conflicts.c \
	src/derives.c \
	src/files.c \
	src/getargs.c \
	src/gram.c \
	src/lalr.c \
	src/location.c \
	src/main.c \
	src/muscle_tab.c \
	src/nullable.c \
	src/output.c \
	src/parse-gram.c \
	src/print.c \
	src/print_graph.c \
	src/reader.c \
	src/reduce.c \
	src/relation.c \
	src/scan-gram-c.c \
	src/scan-skel-c.c \
	src/state.c \
	src/symlist.c \
	src/symtab.c \
	src/tables.c \
	src/uniqstr.c \
	src/vcg.c

LOCAL_MODULE := bison
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
intermediates := $(call local-intermediates-dir)

LOCAL_CFLAGS := -DHAVE_CONFIG_H -DPKGDATADIR=\"$(LOCAL_PATH)/data\"

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/lib

include $(BUILD_HOST_EXECUTABLE)

endif
