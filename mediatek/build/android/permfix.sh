#!/bin/bash


MTK_PROJECT=$1

function change_mode
{
	local files=`echo $1 | sed 's/\[\(.*\)\]/\\\[\1\\\]/g'`
	for i in $files; do
		if [[ -e $i ]]; then
			chmod -R $2 $i
		fi
	done
}

files_to_chmod_x="
prebuilt/linux-x86/sdl/bin/*
prebuilt/linux-x86/flex/*
prebuilt/linux-x86/toolchain/arm-eabi-4.2.1/bin/*
prebuilt/linux-x86/toolchain/arm-eabi-4.2.1/arm-eabi/bin/*
prebuilt/linux-x86/toolchain/arm-eabi-4.2.1/libexec/gcc/arm-eabi/4.2.1/install-tools/*
prebuilt/linux-x86/toolchain/arm-eabi-4.2.1/libexec/gcc/arm-eabi/4.2.1/*
prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/bin/*
prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/arm-eabi/bin/*
prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/libexec/gcc/arm-eabi/4.4.0/install-tools/*
prebuilt/linux-x86/toolchain/arm-eabi-4.4.0/libexec/gcc/arm-eabi/4.4.0/*
build/core/find-jdk-tools-jar.sh
build/tools/*.sh
build/tools/*.py
external/iptables/extensions/create_initext
external/webkit/WebCore/css/*.pl
external/webkit/WebCore/*.sh
external/dhcpcd/dhcpcd-run-hooks
kernel/build.sh
kernel/scripts/*.sh
kernel/scripts/*.pl
kernel/trace32/*.sh
kernel/trace32/tools/mkimage
frameworks/base/cmds/am/am
frameworks/base/cmds/dumpinfo/dumpinfo.sh
frameworks/base/cmds/ime/ime
frameworks/base/cmds/input/input
frameworks/base/cmds/pm/pm
frameworks/base/cmds/svc/svc
development/cmds/monkey/monkey
prebuilt/android-arm/gdbserver/gdbserver
mediatek/build/android/*.sh
mediatek/build/tools/mkimage
external/qemu/hxtool
external/qemu/feature_to_c.sh
external/qemu/android/tools/gen-hw-config.py
cts/tools/dx-tests/etc/compileall
build/tools/releasetools/ota_from_target_files
mediatek/build/tools/*
bootable/bootloader/preloader/build.sh
mediatek/source/preloader/build.sh
bootable/bootloader/uboot/build.sh
mediatek/source/dct/DrvGen
mediatek/custom/common/uboot/logo/update
mediatek/custom/common/uboot/logo/tool/*
mediatek/build/Makefile
mediatek/build/shell.sh
mediatek/build/tools/image_signning_tool/exe/*
"

files_to_chmod_w="
frameworks/base/api/current.xml
vendor/mediatek/*/artifacts/*.txt
mediatek/custom/${MTK_PROJECT}/preloader/custom_emi.c
mediatek/custom/${MTK_PROJECT}/preloader/MTK_Loader_Info_v4.tag
mediatek/custom/${MTK_PROJECT}/preloader/inc/custom_emi.h
mediatek/custom/${MTK_PROJECT}/secro/android-sec
mediatek/custom/common/secro/android-sec
mediatek/source/frameworks/featureoption/java/com/mediatek/featureoption/FeatureOption.java
kernel/drivers/net/wireless/mt592x/wlan/
mediatek/custom/common/uboot/logo/boot_logo
mediatek/source/external/bluetooth/blueangel/_bt_scripts/
mediatek/source/external/bluetooth/database/BTCatacherDB
"

for i in $files_to_chmod_x; do
	change_mode $i a+x
done
for i in $files_to_chmod_w; do
	change_mode $i a+w
done

files_to_touch="
external/webkit/WebCore/css/tokenizer.flex
"

for i in $files_to_touch; do
	touch $i
done
