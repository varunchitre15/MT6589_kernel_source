

custom_subdir_makefiles := \
        $(shell build/tools/findleaves.py --prune=out --prune=.repo --prune=.git $(MTK_PATH_CUSTOM) Android.mk)

include $(custom_subdir_makefiles)
