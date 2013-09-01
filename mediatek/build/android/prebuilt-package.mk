
###########################################################
## Extend rules for installing a prebuilt application package.
##
## Additional inputs from base_rules.make:
## LOCAL_PACKAGE_NAME: The name of the package; the directory
##		will be called this.
##
## MODULE, MODULE_PATH, and MODULE_SUFFIX will
## be set for you.
###########################################################

LOCAL_PACKAGE_NAME := $(strip $(LOCAL_PACKAGE_NAME))
ifeq ($(LOCAL_PACKAGE_NAME),)
$(error $(LOCAL_PATH): Package modules must define LOCAL_PACKAGE_NAME)
endif

ifneq ($(strip $(LOCAL_MODULE_SUFFIX)),)
$(error $(LOCAL_PATH): Package modules may not define LOCAL_MODULE_SUFFIX)
endif
LOCAL_MODULE_SUFFIX := $(COMMON_ANDROID_PACKAGE_SUFFIX)

ifneq ($(strip $(LOCAL_MODULE)),)
$(error $(LOCAL_PATH): Package modules may not define LOCAL_MODULE)
endif
LOCAL_MODULE := $(LOCAL_PACKAGE_NAME)

# Android packages should use Android resources or assets.
ifneq (,$(LOCAL_JAVA_RESOURCE_DIRS))
$(error $(LOCAL_PATH): Package modules may not set LOCAL_JAVA_RESOURCE_DIRS)
endif
ifneq (,$(LOCAL_JAVA_RESOURCE_FILES))
$(error $(LOCAL_PATH): Package modules may not set LOCAL_JAVA_RESOURCE_FILES)
endif

LOCAL_MODULE_CLASS := APPS

# Package LOCAL_MODULE_TAGS default to optional
LOCAL_MODULE_TAGS := $(strip $(LOCAL_MODULE_TAGS))
ifeq ($(LOCAL_MODULE_TAGS),)
LOCAL_MODULE_TAGS := optional
endif

LOCAL_ASSET_DIR:=
LOCAL_RESOURCE_DIR:=
R_file_stamp :=

LOCAL_BUILT_MODULE_STEM := package.apk

#################################
include $(BUILD_SYSTEM)/base_rules.mk
#################################

# Pick a key to sign the package with.  If this package hasn't specified
# an explicit certificate, use the default.
# Secure release builds will have their packages signed after the fact,
# so it's ok for these private keys to be in the clear.
ifeq ($(LOCAL_CERTIFICATE),)
    LOCAL_CERTIFICATE := testkey
endif
# If this is not an absolute certificate, assign it to a generic one.
ifeq ($(dir $(strip $(LOCAL_CERTIFICATE))),./)
    LOCAL_CERTIFICATE := $(SRC_TARGET_DIR)/product/security/$(LOCAL_CERTIFICATE)
endif
private_key := $(LOCAL_CERTIFICATE).pk8
certificate := $(LOCAL_CERTIFICATE).x509.pem

$(LOCAL_BUILT_MODULE): $(private_key) $(certificate) $(SIGNAPK_JAR)
$(LOCAL_BUILT_MODULE): PRIVATE_PRIVATE_KEY := $(private_key)
$(LOCAL_BUILT_MODULE): PRIVATE_CERTIFICATE := $(certificate)
PACKAGES.$(LOCAL_PACKAGE_NAME).PRIVATE_KEY := $(private_key)
PACKAGES.$(LOCAL_PACKAGE_NAME).CERTIFICATE := $(certificate)


$(LOCAL_BUILT_MODULE): $(LOCAL_PREBUILT_PACKAGE)
	@mkdir -p $(dir $@)
	$(hide) cp -fp $< $@

# Save information about this package
PACKAGES.$(LOCAL_PACKAGE_NAME).OVERRIDES := $(strip $(LOCAL_OVERRIDES_PACKAGES))

PACKAGES := $(PACKAGES) $(LOCAL_PACKAGE_NAME)
