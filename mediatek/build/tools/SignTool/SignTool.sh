#!/bin/sh

##############################################################
# Program:
#	SIGN TOOL
#

CUR_DIR=`pwd`

if [ "$3" = "" ]; then
    echo "Usage: Sign Image .."
    echo "       ./SignTool [KEY] [CONFIG] [INPUT_IMAGE] [OUTPUT_IMAGE]"
    echo ""
    echo "Example:"
    echo "       ./SignTool.sh IMG_KEY.ini IMG_CFG.ini u-boot.bin signed-u-boot.bin"    
    echo ""
    exit 1;
fi

cd mediatek/build/tools/SignTool
make
cd $CUR_DIR

##############################################################
# Setup tool and nand page size
SignTool=${0%/*}/SignTool
Simg2imgTool=mediatek/build/tools/SignTool/simg2img
Yaffs2Covert=mediatek/build/tools/SignTool/yimg2img
if [ "$5" = "2K" ] ; then
    NAND_PAGE_SIZE=2048
elif [ "$5" = "4K" ] ; then
    NAND_PAGE_SIZE=4096
elif [ "$5" = "1K" ] ; then
    NAND_PAGE_SIZE=1024
else
    echo "[SignTool] NAND page size $5 not supported, set to default 2048!!"
    NAND_PAGE_SIZE=2048
fi

##############################################################
# Check if it is a sparse image or yaffs2 image
Sparse_Str=`hexdump -n 4 $3 | head -n 1 | awk '{print $2,$3}'`
Yaffs_Str=`hexdump $3 | head -n 1 | awk '{print $2,$3,$4,$5,$6,$7,$8,$9}'`
if [ "$Sparse_Str" = "ff3a ed26" ] ; then

    ./${Simg2imgTool} $3 UN_SPARSE_TEMP_IMG
    ./${SignTool} $1 $2 UN_SPARSE_TEMP_IMG SIGNATURE HEADER
    if [ $? -eq 0 ] ; then
        echo "SIGN PASS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Sparse S1 IMG)"
        cp -f HEADER EXT_HDR_SPARSE
        cat SIGNATURE >> EXT_HDR_SPARSE
    else
        echo "SIGN FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Sparse S1 IMG)"
        exit 1;
    fi

    rm -f SIGNATURE
    rm -f HEADER

    ./${SignTool} $1 $2 $3 SIGNATURE HEADER EXT_HDR_SPARSE
    if [ $? -eq 0 ] ; then
        echo "SIGN PASS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Sparse S2 IMG)"
        cp -f HEADER $4
        cat $3 >> $4
        cat SIGNATURE >> $4
        rm -f SIGNATURE
        rm -f HEADER
    else
        echo "SIGN FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Sparse S2 IMG)"
        exit 1;
    fi

    rm -rf UN_SPARSE_TEMP_IMG
    rm -rf EXT_HDR_SPARSE

elif [ "$Yaffs_Str" = "0003 0000 0001 0000 ffff 0000 0000 0000" ] ; then

    ./${Yaffs2Covert} -c $NAND_PAGE_SIZE $3 UN_YAFFS_TEMP_IMG
    echo "./${Yaffs2Covert} -c $NAND_PAGE_SIZE $3 UN_YAFFS_TEMP_IMG"
    ./${SignTool} $1 $2 UN_YAFFS_TEMP_IMG SIGNATURE HEADER
    if [ $? -eq 0 ] ; then
        echo "SIGN PASS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Yaffs S1 IMG)"
        cp -f HEADER EXT_HDR_SPARSE
        cat SIGNATURE >> EXT_HDR_SPARSE
    else
        echo "SIGN FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Yaffs S1 IMG)"
        exit 1;
    fi

    rm -f SIGNATURE
    rm -f HEADER

    ./${SignTool} $1 $2 $3 SIGNATURE HEADER EXT_HDR_SPARSE
    if [ $? -eq 0 ] ; then
        echo "SIGN PASS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Yaffs S2 IMG)"
        cp -f HEADER $4
        cat $3 >> $4
        cat SIGNATURE >> $4
        rm -f SIGNATURE
        rm -f HEADER
    else
        echo "SIGN FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Yaffs S2 IMG)"
        exit 1;
    fi

    rm -rf UN_YAFFS_TEMP_IMG
    rm -rf EXT_HDR_SPARSE
    
else

    ./${SignTool} $1 $2 $3 SIGNATURE HEADER
    if [ $? -eq 0 ] ; then
        echo "SIGN PASS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Normal IMG)"
        cp -f HEADER $4
        cat $3 >> $4
        cat SIGNATURE >> $4
    else
        echo "SIGN FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(Normal IMG)"
        exit 1;
    fi

    rm -f SIGNATURE
    rm -f HEADER
fi


# //<2013/04/19-24075-EricLin, [Pelican][S1] Remove the  FB_SIG switch for signtool.
##############################################################
# Check if need to generate fastboot signature
# if [ "$6" = "FB_SIG" ] ; then
#     ./${SignTool} $1 $2 $4 $3 FB_SIG
#     if [ $? -eq 0 ] ; then
#         echo "SIGN PASS !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(FB SIG)"
#     else
#         echo "SIGN FAIL !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!(FB SIG)"
#         exit 1;
#     fi
#     cp -f FB_SIG $4.csd
#     cat $4 >> $4.csd
#     rm -f FB_SIG
#     rm -f $4.sig
#     rm -f $4
#     mv -f $4.csd $4
# fi
# //>2013/04/19-24075-EricLin
