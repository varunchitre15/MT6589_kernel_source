#
# arch/arm/boot/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies.
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1995-2002 Russell King
#

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/Makefile.boot
endif

# Note: the following conditions must always be true:
#   ZRELADDR == virt_to_phys(PAGE_OFFSET + TEXT_OFFSET)
#   PARAMS_PHYS must be within 4MB of ZRELADDR
#   INITRD_PHYS must be in RAM
ZRELADDR    := $(zreladdr-y)
PARAMS_PHYS := $(params_phys-y)
INITRD_PHYS := $(initrd_phys-y)

export ZRELADDR INITRD_PHYS PARAMS_PHYS

targets := Image zImage xipImage bootpImage uImage

# [Mediatek]
# Check if (sizeof(Image)+sizeof(zImage)-0x8000) > $(CONFIG_RAMDISK_OFFSET)
# Remember to align the value of CONFIG_RAMDISK_OFFSET with CFG_RAMDISK_LOAD_ADDR
# in mediatek/custom/<project>/uboot/inc/configs/ubconfigs.h
# Ex. CFG_RAMDISK_LOAD_ADDR is configured in ubconfigs.h as below
# '#define CFG_RAMDISK_LOAD_ADDR (RIL_SIZE + 0xD00000)'
# so CONFIG_RAMDISK_OFFSET should be configured as '0xD00000' in kconfig
define chk-kernel-img-size
  Image_size=`stat --format=%s $(obj)/Image`; \
  zImage_size=`stat --format=%s $(obj)/zImage`; \
  offset=$$(($$Image_size + $$zImage_size - 32768)); \
  ramdisk_offset=`printf %d $(CONFIG_RAMDISK_OFFSET)`; \
  shortage=$$(($$offset - $$ramdisk_offset)); \
  if [ $$offset -gt $$ramdisk_offset ]; then \
    echo \[Kernel Size Expired\] Kernel size is too big that might overwrite Ramdisk, please optimize Kernel size.; \
    echo \[Kernel Size Expired\] Current offset is \"`printf "0x%x" $$offset`\" \(Image + zImage - 0x8000\).; \
    echo \[Kernel Size Expired\] The offset has to be less than \"`printf "0x%x" $$ramdisk_offset`\" \(RAMDISK_LOAD_ADDR\).; \
    echo \[Kernel Size Expired\] The total size \(Image + zImage\) is over \"$$shortage\" bytes than expected.; \
    echo \[Kernel Size Expired\] If the code size is hard to be optimized, please contact memory expert for code review.; \
    exit 1; \
  fi
endef


ifeq ($(CONFIG_XIP_KERNEL),y)

$(obj)/xipImage: vmlinux FORCE
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready (physical address: $(CONFIG_XIP_PHYS_ADDR))'

$(obj)/Image $(obj)/zImage: FORCE
	@echo 'Kernel configured for XIP (CONFIG_XIP_KERNEL=y)'
	@echo 'Only the xipImage target is available in this case'
	@false

else

$(obj)/xipImage: FORCE
	@echo 'Kernel not configured for XIP (CONFIG_XIP_KERNEL!=y)'
	@false

$(obj)/Image: vmlinux FORCE
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready'

$(obj)/compressed/vmlinux: $(obj)/Image FORCE
	$(Q)$(MAKE) $(build)=$(obj)/compressed $@

$(obj)/zImage:	$(obj)/compressed/vmlinux FORCE
	$(call if_changed,objcopy)
	$(Q)$(chk-kernel-img-size)
	@echo '  Kernel: $@ is ready'

endif

targets += $(dtb-y)

# Rule to build device tree blobs
$(obj)/%.dtb: $(src)/dts/%.dts FORCE
	$(call if_changed_dep,dtc)

$(obj)/dtbs: $(addprefix $(obj)/, $(dtb-y))

clean-files := *.dtb

ifneq ($(LOADADDR),)
  UIMAGE_LOADADDR=$(LOADADDR)
else
  ifeq ($(CONFIG_ZBOOT_ROM),y)
    UIMAGE_LOADADDR=$(CONFIG_ZBOOT_ROM_TEXT)
  else
    UIMAGE_LOADADDR=$(ZRELADDR)
  endif
endif

check_for_multiple_loadaddr = \
if [ $(words $(UIMAGE_LOADADDR)) -gt 1 ]; then \
	echo 'multiple load addresses: $(UIMAGE_LOADADDR)'; \
	echo 'This is incompatible with uImages'; \
	echo 'Specify LOADADDR on the commandline to build an uImage'; \
	false; \
fi

$(obj)/uImage:	$(obj)/zImage FORCE
	@$(check_for_multiple_loadaddr)
	$(call if_changed,uimage)
	@echo '  Image $@ is ready'

$(obj)/bootp/bootp: $(obj)/zImage initrd FORCE
	$(Q)$(MAKE) $(build)=$(obj)/bootp $@
	@:

$(obj)/bootpImage: $(obj)/bootp/bootp FORCE
	$(call if_changed,objcopy)
	@echo '  Kernel: $@ is ready'

PHONY += initrd FORCE
initrd:
	@test "$(INITRD_PHYS)" != "" || \
	(echo This machine does not support INITRD; exit -1)
	@test "$(INITRD)" != "" || \
	(echo You must specify INITRD; exit -1)

install: $(obj)/Image
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image System.map "$(INSTALL_PATH)"

zinstall: $(obj)/zImage
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/zImage System.map "$(INSTALL_PATH)"

uinstall: $(obj)/uImage
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/uImage System.map "$(INSTALL_PATH)"

zi:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/zImage System.map "$(INSTALL_PATH)"

i:
	$(CONFIG_SHELL) $(srctree)/$(src)/install.sh $(KERNELRELEASE) \
	$(obj)/Image System.map "$(INSTALL_PATH)"

subdir-	    := bootp compressed
