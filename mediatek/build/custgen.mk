
include mediatek/build/Makefile
$(call codebase-path)
all: mtk-config-files mtk-custom-files
	@echo "done"
mtk-config-files := $(strip $(call mtk.config.generate-rules,mtk-config-files))
mtk-custom-files := $(strip $(call mtk.custom.generate-rules,mtk-custom-files))
