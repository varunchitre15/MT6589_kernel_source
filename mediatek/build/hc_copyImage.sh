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
### 20130517: Add S1 image pack                                      ###
### 20130527: Modify "AP" selection for S1 pack                      ###
### 20130531: Modify S1 remake pack                                  ###
########################################################################

### Config Start ###
PATH_OF_MODEM="mediatek/custom/common/modem"
MODEM=`grep "CUSTOM_MODEM" mediatek/config/arima89_we_s_jb2/ProjectConfig.mk | grep "arima89" | cut -d "=" -f 2 | awk '{print $1}' | sed 's/\r//g'`
MODEM_FILE="BPLGUInfoCustomApp*"

PATH_OF_APDB_MT6589="mediatek/cgen"
PATH_OF_OUTPUT_FILE="out/target/product/arima89_we_s_jb2"
SW_VERSION=`grep "export BUILD_ID" build/core/build_id.mk  | cut -d "=" -f2`
COMPRESS_FILE=${SW_VERSION}.zip
LOG_PATH="out/target/product"
HW_MODEM_DB="hw_modem_db"
RELEASE_FILES=""
### Config End   ###


echo "**** To generate Modem-info log file ****"
./mk arima89_we_s_jb2 modem-info

echo "**** To copy and gather images!! ****"
### Factory Need ###
mkdir -p ${PATH_OF_OUTPUT_FILE}/${HW_MODEM_DB}
cp ${PATH_OF_MODEM}/${MODEM}/${MODEM_FILE} ${PATH_OF_OUTPUT_FILE}/${HW_MODEM_DB}
cp ${PATH_OF_MODEM}/${MODEM}/catcher_filter.bin ${PATH_OF_OUTPUT_FILE}/${HW_MODEM_DB}
cp ${PATH_OF_APDB_MT6589}/APDB_MT6589_S01_MAIN2.1_W10.24 ${PATH_OF_OUTPUT_FILE}/${HW_MODEM_DB}
cp ${LOG_PATH}/arima89_we_s_jb2_modem-info.log ${PATH_OF_OUTPUT_FILE}/${HW_MODEM_DB}

echo "**** Go to \"${PATH_OF_OUTPUT_FILE}\" ****"
cd ${PATH_OF_OUTPUT_FILE}
 
echo "**** To Compress \"${COMPRESS_FILE}\" ****"
### Define output file list at 20130311 ###
rm -rf ${HW_MODEM_DB}.zip
zip -r ${HW_MODEM_DB}.zip ${HW_MODEM_DB}
rm -rf ${HW_MODEM_DB}

if [ $# != 0 ]; then
   if [ $# == 1 ]; then
	COMPRESS_FILE=${SW_VERSION}_$@.zip
   elif [ $# == 2 ]; then
	RELEASE_FILES=${SW_VERSION}_$1_$2
	COMPRESS_FILE=${RELEASE_FILES}.zip
   elif [ $# == 3 ]; then
	RELEASE_FILES=${SW_VERSION}_$1_$2_$3
	COMPRESS_FILE=${RELEASE_FILES}.zip
   fi
   echo "**** To Compress \"${COMPRESS_FILE}\" ****"
fi

#if [ "$3" == "GEN" ]; then
if [ "$1" == "AP" ] || [ "$2" == "AP" ]; then
   echo "**** To Generate S1 images ****"
   cd ../../../../
   ./S1_sinnify.sh ${RELEASE_FILES}
else
rm -rf ${COMPRESS_FILE}
mkdir -p ${RELEASE_FILES}
cp boot.img cache.img EBR1 EBR2 logo.bin MBR MT6589_Android_scatter_emmc.txt preloader_arima89_we_s_jb2.bin recovery.img secro.img system.img lk.bin userdata.img ${HW_MODEM_DB}.zip ${RELEASE_FILES}/
zip -r ${COMPRESS_FILE} ${RELEASE_FILES}/*
rm -rf ${RELEASE_FILES}/
cd ../../../../
fi

#cd ../../../../ 
echo "**** Finish!! ****"

