
# default TARGET_CPU_ABI
TARGET_CPU_ABI := armeabi
board_config_mk := $(MTK_ROOT_CONFIG_OUT)/BoardConfig.mk

# try to include by-project BoardConfig.
# inclusion failure will auto trigger auto-generation & rebuild
include $(MTK_ROOT_CONFIG_OUT)/BoardConfig.mk
