#
# for CA7 cpu stress test
#
obj-y	:= slt_maxpower.o slt_fp0_maxpower_asm.o slt_memcpyL2.o slt_fp3_memcpyL2_asm.o slt_vfp_func.o slt_fp4_vfp_func_asm.o slt_fdct.o slt_fp7_fdct_asm.o slt_imdct.o slt_fp8_imdct_asm.o slt_dhry.o slt_dhry_1.o slt_dhry_2.o
KBUILD_AFLAGS	:=$(KBUILD_AFLAGS:-msoft-float=-Wa,-mfpu=neon-vfpv4) 
