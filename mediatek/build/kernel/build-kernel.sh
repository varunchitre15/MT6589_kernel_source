#!/bin/sh

source ../mediatek/build/shell.sh ../ kernel

# Default settings
verfile="android.ver"
defcfg="${MTK_PATH_GEN_CONFIG}/kernel.config"
curcfg=".config"
custcfg=
release="n"
rebuild="n"
clean="n"
makeflags="-w"
makedefs="V=0"
makejobs=""
curdir=`pwd`

usage() {
    echo "Usage: $0 {release|rebuild|clean|silent|verbose|single} [config-xxx]"
    echo "  config file will be generated if build with TARGET_PRODUCT"
    exit 1
}

make_clean() {
    echo "**** Cleaning ****"
    nice make ${makeflags} ${makedefs} distclean
}


# Main starts here
while test -n "$1"; do
    case "$1" in
    release)
        release="y"
    ;;
    rebuild)
        rebuild="y"
    ;;
    clean)
        clean="y"
    ;;
    silent)
        makeflags="-ws"
        makedefs="V=0"
    ;;
    verbose)
        makeflags="-w"
        makedefs="V=1"
    ;;
    single)
        makejobs=""
    ;;
    config-*)
        if test ! -f "$1"; then
            die "$1 does not exist!"
        fi
        if test -f "${curcfg}" && ! rm -f "${curcfg}"; then
            die "Unable to remove ${curcfg}!"
        fi
        custcfg="$1"
    ;;
    *)
        usage
    ;;
    esac
    shift
done

# clean if it is necessary
if [ "${clean}" == "y" ];   then make_clean; exit 0; fi
if [ "${rebuild}" == "y" ]; then make_clean; fi

echo "**** Configuring / ` [ -z $custcfg ] || ! echo $custcfg && echo $defcfg ` / ****"
# select correct configuration file
if [ -z ${custcfg} ];       then make mediatek-configs; fi
if [ ! -z ${custcfg} ];     then cat ${custcfg} > ${curcfg}; fi

# update configuration
nice make ${makeflags} ${makedefs} silentoldconfig

echo "**** Building ****"
make ${makeflags} ${makejobs} ${makedefs}

if [ $? -ne 0 ]; then exit 1; fi

echo "**** Successfully built kernel ****"

mkimg="${MTK_ROOT_BUILD}/tools/mkimage"
kernel_img="${curdir}/arch/arm/boot/Image"
kernel_zimg="${curdir}/arch/arm/boot/zImage"
rootfs_img="${curdir}/trace32/rootfs_`echo ${MTK_PLATFORM} | tr A-Z a-z`.gz"

echo "**** Generate download images ****"

if [ ! -x ${mkimg} ]; then chmod a+x ${mkimg}; fi

${mkimg} ${kernel_zimg} KERNEL > kernel_${MTK_PROJECT}.bin
${mkimg} ${rootfs_img} ROOTFS > rootfs_${MTK_PROJECT}.bin

copy_to_legacy_download_sdcard_folder  ${kernel_img} ${rootfs_img}
copy_to_legacy_download_flash_folder   kernel_${MTK_PROJECT}.bin rootfs_${MTK_PROJECT}.bin
