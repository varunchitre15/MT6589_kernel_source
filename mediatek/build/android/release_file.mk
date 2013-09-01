
ifneq ($(DO_GET_ARTIFACTS),yes)
  PROTECT_DIRS := mediatek/protect
  _release_policy := $(shell find $(PROTECT_DIRS) -name "Android.mk" | sed "s/\/Android.mk$$//") 
endif
