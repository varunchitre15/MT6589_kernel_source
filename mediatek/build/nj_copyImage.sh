#!/bin/bash

########################################################################
### Author : Sandy Tan                                               ###
### Program: ./hc_copyImage.sh                                       ###
### History:                                                         ###
### 20130312: Use to pack output files!                              ###
### 20130313: Modify for more clear.                                 ###
### 20130322: Modify the path of modem,                              ###
###           re-define the path that auto get from ProjectConfig.mk ###
### 20130325: Clear all mark symble.                                 ###
### 20130418: Generate and cpoy modem-info log for MTK debug         ###
### 20130508: Change zip file name                                   ###
### 20130509: Modify for remake image format issue                   ###
########################################################################

### Config Start ###
PATH_OF_MODEM="mediatek/custom/common/modem"
MODEM=`grep "CUSTOM_MODEM" mediatek/config/arima89_we_s_jb2/ProjectConfig.mk | grep "arima89" | cut -d "=" -f 2 | awk '{print $1}' | sed 's/\r//g'`
MODEM_FILE="BPLGUInfoCustomApp*"

PATH_OF_APDB_MT6589="mediatek/cgen"
PATH_OF_OUTPUT_FILE="out/target/product/arima89_we_s_jb2"
SW_VERSION=`grep "export BUILD_ID" build/core/build_id.mk  | cut -d "=" -f2`
COMPRESS_FILE=image.zip
LOG_PATH="out/target/product"
### Config End   ###


echo "**** To generate Modem-info log file ****"
./mk arima89_we_s_jb2 modem-info

echo "**** To copy and gather images!! ****"
### Factory Need ###
cp ${PATH_OF_MODEM}/${MODEM}/${MODEM_FILE} ${PATH_OF_OUTPUT_FILE}
cp ${PATH_OF_MODEM}/${MODEM}/catcher_filter.bin ${PATH_OF_OUTPUT_FILE}
cp ${PATH_OF_APDB_MT6589}/APDB_MT6589_S01_MAIN2.1_W10.24 ${PATH_OF_OUTPUT_FILE}
cp ${LOG_PATH}/arima89_we_s_jb2_modem-info.log ${PATH_OF_OUTPUT_FILE}

echo "**** Go to \"${PATH_OF_OUTPUT_FILE}\" ****"
cd ${PATH_OF_OUTPUT_FILE}
 
echo "**** To Compress \"${COMPRESS_FILE}\" ****"
### Define output file list at 20130311 ###
rm -rf ${COMPRESS_FILE}
if [ $# != 0 ]; then
COMPRESS_FILE=image.zip
echo "**** To Compress \"${COMPRESS_FILE}\" ****"
fi
zip -r ${COMPRESS_FILE} boot.img cache.img EBR1 EBR2 logo.bin MBR MT6589_Android_scatter_emmc.txt preloader_arima89_we_s_jb2.bin recovery.img secro.img system.img lk.bin userdata.img catcher_filter.bin APDB_MT6589* ${MODEM_FILE} arima89_we_s_jb2_modem-info.log

cd ../../../../ 
echo "**** Finish!! ****"

