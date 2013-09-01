SECURE_LIB := auth/ToolLib.a
CFLAGS += -I auth/inc
EXTRA_CLEAN := $(COMMON_LIB)
SEC_WRAPPER_IMPL := $(OBJ_DIR)/sec_auth.o

extra: SignLib

$(SEC_WRAPPER_IMPL): auth/sec_auth.c
	gcc -m32 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c auth/sec_auth.c -o $(SEC_WRAPPER_IMPL) $(CFLAGS)

SignLib: $(OBJ_DIR)/lib_sign.o $(OBJ_DIR)/sign_util.o
	ar rcs $(COMMON_LIB) $(OBJ_DIR)/lib_sign.o $(OBJ_DIR)/sign_util.o

$(OBJ_DIR)/lib_sign.o: auth/lib_sign.c
	gcc -m32 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c auth/lib_sign.c -o $(OBJ_DIR)/lib_sign.o $(CFLAGS)

$(OBJ_DIR)/sign_util.o: auth/sign_util.c
	gcc -m32 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -c auth/sign_util.c -o $(OBJ_DIR)/sign_util.o $(CFLAGS)
