source ../../../mediatek/build/shell.sh ../../.. uboot $1

export UBOOT_CHIP_CONFIG=`echo ${MTK_PLATFORM} | tr [A-Z] [a-z]`
export UBOOT_BOARD_CONFIG=${UBOOT_CHIP_CONFIG}_`echo ${MTK_PROJECT} | sed -e 's/^mt.\+_//g'`
export UBOOT_STORAGE_CONFIG="CFG_NAND_BOOT"
export UBOOT_IMAGE_NAME="u-boot-${UBOOT_CHIP_CONFIG}-${MTK_PROJECT}-nand-boot.bin"
export UBOOT_LOGO_PATH=${MTK_PATH_SOURCE}/logo/${CUSTOM_UBOOT_LOGO}

CUR_DIR=`pwd`
SCT_DIR="scatter"
MKIMG="${CUR_DIR}/../../../build/mediatek/mkimage"
if [ ! -f "${MKIMG}" ]; then
    MKIMG="${CUR_DIR}/tools/mkimage"
fi
IMG_DIR="../../../kernel/Download"
FLASH_DIR="${IMG_DIR}/flash"
UBOOT_IMG="${CUR_DIR}/${UBOOT_IMAGE_NAME}"
UBOOT_DOWNLOAD_IMG="${FLASH_DIR}/${UBOOT_IMAGE_NAME}"    
TARGET=${UBOOT_CHIP_CONFIG}_${UBOOT_BOARD_CONFIG}_${UBOOT_STORAGE_CONFIG}

##############################################################
# Board configuration definition

usage() {
    echo "Usage: build.sh -[c] <target>"
    echo "   -c: clean before building"
}

##############################################################
# Get options flag

while getopts "c?" option
do
    case $option in
      c) CLEAN="yes";;
      c) HELP="yes";;
      [?]) usage; exit 0;;
    esac
done

##############################################################
# Decrements the argument pointer so it points to next argument
# $1 now references the first non option item supplied on the command line
# if one exists.

shift $(($OPTIND - 1))

if [ "$HELP" == "yes" ]; then
    usage; exit 1;
fi

##############################################################
# Remove .a to make sure the library will be recreated

#make remove_library

##############################################################
# Build u-boot image

if [ ! -x mkconfig ]; then
    chmod a+x mkconfig
fi

if [ "$CLEAN" == "yes" ]; then
    make distclean;
fi

if [ -f "${UBOOT_IMG}" ]; then
    rm "${UBOOT_IMG}"
fi

if [ -f "${CUR_DIR}/u-boot.bin" ]; then
    rm "${CUR_DIR}/u-boot.bin"
fi

make "$UBOOT_CHIP_CONFIG"_config
make

##############################################################
# Check compile result

if [ ! -f "${CUR_DIR}/u-boot.bin" ]; then
    echo "COMPILE FAIL !!!!!!!!!!!!!!!!"
    echo "COMPILE FAIL !!!!!!!!!!!!!!!!"
    echo "COMPILE FAIL !!!!!!!!!!!!!!!!"
    echo "COMPILE FAIL !!!!!!!!!!!!!!!!"
    echo "COMPILE FAIL !!!!!!!!!!!!!!!!"
    echo "COMPILE FAIL !!!!!!!!!!!!!!!!"
    exit 1
fi


##############################################################
# Save image to image folder



##############################################################
# Generate logo image

	
if [ ! -x ${MKIMG} ]; then
    chmod a+x ${MKIMG}
fi

if [ ${UBOOT_STORAGE_CONFIG} == "CFG_NAND_BOOT" ]; then
	
    if [ ! -e ${UBOOT_LOGO_PATH} ]; then
        echo "${UBOOT_LOGO_PATH} can't be found"; 
        exit 1;
    fi 
	
    ${MKIMG} ${UBOOT_LOGO_PATH} LOGO > ${FLASH_DIR}/logo.bin
    ${MKIMG} ${UBOOT_LOGO_PATH} LOGO > ${CUR_DIR}/logo.bin

fi

##############################################################
# Copy u-boot image

${MKIMG} ${CUR_DIR}/u-boot.bin UBOOT > ${CUR_DIR}/u-boot-${UBOOT_CHIP_CONFIG}.bin
rm ${CUR_DIR}/u-boot.bin

cp ${CUR_DIR}/u-boot-${UBOOT_CHIP_CONFIG}.bin ${CUR_DIR}/${UBOOT_IMAGE_NAME}
    
if [ -e ${UBOOT_IMG} ]; then
    if [ ! -d ${IMG_DIR} ]; then
        mkdir ${IMG_DIR}
    fi
    if [ ! -d ${FLASH_DIR} ]; then
        mkdir ${FLASH_DIR}
    fi
    cp ${SCT_DIR}/* ${FLASH_DIR}/
    ln -sf ${UBOOT_IMG} ${UBOOT_DOWNLOAD_IMG} # ${FLASH_DIR}/u-boot-${UBOOT_CHIP_CONFIG}.bin # Jau modify mt3351.bin
fi


##############################################################
# Jauping add at 20090804 for MT6516 {
# Check object file size

if [ -f "${UBOOT_IMG}" ]; then

    UBOOT_SIZE=$(stat -c%s "$UBOOT_IMG")
    

    echo "===================== building warning ========================"
    echo "the following guy's code size is a bit large, ( > 2500 bytes )"
    echo "please note it and reduce it if possible !"
    echo "for more detail, please refer to <u-boot-code-size-report.txt>"
    echo "---------------------------------------------------------------"
    echo ${TARGET} image size is `stat --printf="%s bytes\n" ${UBOOT_IMG}`
    echo "---------------------------------------------------------------"
    echo "                      CODE SIZE REPORT                         "
    echo "---------------------------------------------------------------"
    echo "size(bytes)     file  ( size > 2500 bytes )"
    echo "---------------------------------------------------------------"
    size `ls board/${UBOOT_CHIP_CONFIG}/*.o` >u-boot-code-size-report.txt
    awk '$1 ~ /^[0-9]/ { if ($4>2500) print $4 "\t\t" $6}' < u-boot-code-size-report.txt | sort -rn
    echo "==============================================================="

    echo "UBoot-2010.06 is built successfully!!"
fi
# Jauping add at 20090804 for MT6516 }
