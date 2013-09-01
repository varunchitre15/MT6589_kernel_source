# by kirby.wu@mediatek.com (2009/10/21)
# partial-release object file manager

obj_to_install :=
MTK_PROJECT_FLAVOR := $(MTK_PROJECT)
ifeq (,$(wildcard ../mediatek/build/android/full/config.mk))
ifneq (,$(FLAVOR))
MTK_PROJECT_FLAVOR := $(MTK_PROJECT)[$(FLAVOR)]
endif
-include ../vendor/mediatek/$(MTK_PROJECT_FLAVOR)/artifacts/kernel/objects.mk
endif

define word-colon
$(word $(1),$(subst :,$(space),$(2)))
endef


define copy-one-file
$(2): $(1)
	@echo "  COPY    $$@  <---  $$<"
	@cp -fp $$< $$@
endef

$(foreach tuple,$(obj_to_install), \
  $(eval _des := $(call word-colon,1,$(tuple))) \
  $(eval _src := $(call word-colon,2,$(tuple))) \
  $(eval $(call copy-one-file,../vendor/mediatek/$(MTK_PROJECT_FLAVOR)/artifacts/kernel/obj/$(_src).artifacts,$(_des)/$(_src))) \
)

$(foreach tuple,$(file_to_touch), \
  $(eval $(shell mkdir -p $(dir $(tuple)))) \
  $(eval $(shell touch $(tuple))) \
)

