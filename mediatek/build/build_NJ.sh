#!/bin/bash

################################################################
### Author : Sandy Tan                         	             ###
### Program: ./build_5860.sh [DP/PDP/AP] [user/eng/userdebug]###
##           [CU/ROW/GEN]                                    ###
### History:                                                 ###
### 20130318: Use to compile MTK 5860 simply          	     ###
### 20130319: Modify for adding DP/PDP function to choose    ###
### 20130326: Add error log file check                       ###
### 20130416: Add Build Country selection                    ###
### 20130425: Add AP HW version to select                    ###
### 20130506: Add GEN Build Country to select                ###
### 20130508: Add remake function, DP_512M HW version        ###
### 20130509: Modify for remake error                        ###
### 20130517: Add S1 compile commnad                         ###
################################################################


############ Configure ############
BUILD_MODE=""
ERROR_LOG_PATH="out/target/product"
###################################

#if [ "$1" == "" ]; then 
function usage(){
   echo "Please type the compile mode: "
   echo "\"user\""
   echo "\"eng\""
   echo "\"userdebug\""
   exit
}
#fi

function usage2(){
   echo "Please choose HW version: "
   echo "\"DP\""
   echo "\"PDP\""
   echo "\"AP\""
   echo "\"DP_512M\""
   exit 1
}

function usage3(){
   echo "Usage: "
   echo "new make: $0 [HW version] [Compile mode] [Country]"
   echo "re-make: $0 [Compile mode]"
   echo ""
   echo "HW version: DP, PDP, AP, DP_512M"
   echo ""
   echo "Compile mode: user, eng, userdebug"
   echo ""
   echo "Country: CU, ROW, GEN"
   echo ""
   exit 1
}

function usage_country(){
   echo "Please choose build country: "
   echo "\"CU\""
   echo "\"ROW\""
   echo "\"GEN\""
   exit 1
}

function compile(){
   echo "#### Clear environment ####"
   ./mk -t arima89_we_s_jb2 c
   echo "#### Do update-api ####"
   ./mk -t arima89_we_s_jb2 update-api
   echo "#### Compile $1 mode ####"
   ./mk -t -o=TARGET_BUILD_VARIANT=$1 arima89_we_s_jb2 n
}

function remake(){
   echo "#### Do update-api ####"
   ./mk -t arima89_we_s_jb2 update-api

   if [ "$1" == "" ]; then
   	echo "#### Re-compile eng mode ####"
   	./mk -t arima89_we_s_jb2 r
   else
   	echo "#### Re-compile $1 mode ####"
   	./mk -t -o=TARGET_BUILD_VARIANT=$1 arima89_we_s_jb2 r
   fi
}

function compile_S1(){
   echo "#### Clear environment ####"
   ./mk -t arima89_we_s_jb2 c
   echo "#### Do update-api ####"
   ./mk -t arima89_we_s_jb2 update-api
   echo "#### Compile $1 mode ####"
   ./mk -t -o=TARGET_BUILD_VARIANT=$1 arima89_we_s_jb2 n
   ./mk -t -o=TARGET_BUILD_VARIANT=$1 arima89_we_s_jb2 sign-image
}

case $1 in
    "")
	if [ "$2" == "" ] && [ "$3" == "" ]; then
		#usage3;
		echo "Do you want to remake code (Y/N)?"
		read VAL
		case $VAL in
		    "" | "N" | "n")
			usage3;
		      ;;
		    "Y" | "y")
			echo "======================================================"
			echo "execute \"eng\" mode, if you want execute other mode, "
			echo "please run $0 [user/eng/userdebug]."
			echo "======================================================"			
			remake;
		      ;;
		    *)
			usage3;
		      ;;
		esac
	fi	
	;;
    "DP" | "PDP" | "AP" | "DP_512M")
	echo "### Configure HW version: HW_$1 ###"
	./arima_scripts/change_hw_version.sh HW_$1

	if [ "$2" == "user" ] || [ "$2" == "eng" ] || [ "$2" == "userdebug" ]; then

		BUILD_MODE=$2

        	if [ "$3" == "CU" ] || [ "$3" == "ROW" ]; then
                	echo "### Configure Build Country: $3 ###"
                	./arima_scripts/change_build_country.sh $3
			compile ${BUILD_MODE};
              ###Begin_Arima_HCSW7_20130506_ChrisHe - Add GEN script
        	elif  [ "$3" == "GEN" ]; then
                        echo "### Configure Build Country: $3 ###"
                        ./arima_scripts/prepare_cdf_apk.sh CU  ## gen CDF_CU_apk.zip
                        ./arima_scripts/prepare_cdf_apk.sh ROW ## gen CDF_ROW_apk.zip
                        ./key_select.sh 3 ## for GEN build
                        ./arima_scripts/change_build_country.sh $3 ## Set GEN property
              ###End_Arima_HCSW7_20130506_ChrisHe - Add GEN script
			compile_S1 ${BUILD_MODE};
        	else
                	usage_country;
        	fi

		#BUILD_MODE=$2
		#compile ${BUILD_MODE};
	else
		usage;
	fi
	;;
    *)
	if [ "$1" == "user" ] || [ "$1" == "eng" ] || [ "$1" == "userdebug" ]; then
		remake $1;
	else
		usage2;
	fi
	;;
esac


echo "### Check MTK error log file exist ###"
cd ${ERROR_LOG_PATH}
for i in `ls`; do
  if [ ${i##*.} == "log_err" ]; then
      echo "#################################################"
      echo "Error exist!"
      echo "Error log file = $i"
      echo "#################################################"
      exit 1
  fi
done
cd ../../../


echo "### Gather and compress output files ###"
./hc_copyImage.sh $@


