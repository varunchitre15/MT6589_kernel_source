ifneq ($(STANDALONE_BUILD),yes)
include ../../../build/Makefile
include ../../../../$(MTK_ROOT_CONFIG_OUT)/ProjectConfig.mk
endif

export CFLAGS := -I inc
export SECURE_LIB := 
export COMMON_LIB := lib/SignLib.a
export EXTRA_CLEAN := 
export SEC_WRAPPER_IMPL := 
export EXTRA_TARGET :=
export OBJ_DIR := obj
OUTPUT_LINK := SignTool

all: precheck build_info extra SignTool

ifeq ($(CUSTOM_SEC_SIGNTOOL_SUPPORT),yes)
ifeq ($(STANDALONE_BUILD),yes)
    SIGN_TOOL_CUSTOM_FOLDER := auth/private	
else
    SIGN_TOOL_CUSTOM_FOLDER := ../../../../$(MTK_ROOT_CUSTOM_OUT)/security/signtool
endif
    OUTPUT_BIN := SignTool_Cust
    DEL_BIN := SignTool_MTK
    EXTRA_CLEAN :=
    $(shell rm -rf custom)
    $(shell \
        if [ -d $(SIGN_TOOL_CUSTOM_FOLDER) ]; then \
            ln -s $(SIGN_TOOL_CUSTOM_FOLDER) custom ; \
        fi; \
    )
    include custom/Makefile
else
    OUTPUT_BIN := SignTool_MTK
    DEL_BIN := SignTool_Cust
    include auth/Makefile
    $(shell \
        if [ -f $(COMMON_LIB) ]; then \
            chmod a+w $(COMMON_LIB) ; \
        fi; \
    )
endif



SignTool: $(OBJ_DIR)/main.o $(SEC_WRAPPER_IMPL) $(SECURE_LIB)
	gcc -m32 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 obj/main.o $(SEC_WRAPPER_IMPL) $(SECURE_LIB) $(COMMON_LIB) -o $(OUTPUT_BIN)
	ln -s $(OUTPUT_BIN) $(OUTPUT_LINK)

$(OBJ_DIR)/main.o: core/main.c
	gcc -m32 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c core/main.c -o $(OBJ_DIR)/main.o $(CFLAGS)

build_info:		
	@chmod a+w inc/build_info.h
	@echo //building time information > inc/build_info.h
	@echo '#'define BUILD_TIME '"'$(shell date)'"' >> inc/build_info.h

precheck: \
	$(shell \
	if [ -f $(DEL_BIN) ]; then \
		echo "clean" ; \
	else \
		echo ""; \
	fi; \
	) 
	@mkdir -p obj

clean:
	rm -rf $(OBJ_DIR)/*o $(EXTRA_CLEAN) $(OUTPUT_LINK) SignTool_MTK SignTool_Cust
