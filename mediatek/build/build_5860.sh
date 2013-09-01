#!/bin/bash

################################################################
### Author : Sandy Tan                         	             ###
### Program: ./build_5860.sh [DP/PDP/AP/RED]                 ###
###          [user/eng/userdebug] [CU/ROW/GEN]               ###
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
### 20130529: Modify for S1 remake and add error log remove  ###
### 20130531: Add RED version to select (but no fun to build)###
### 20130606: Add red_sign_sbc.sh for Build BROWN KEY        ###
### 20130607: Add clean function and compile RED             ###
################################################################


############ Configure ############
BUILD_MODE=""
ERROR_LOG_PATH="out/target/product"
ERROR_LOG_LIST=""
ERROR_LOG_RECORD=e_log
OUT="out"
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
   echo "\"RED\""
   exit 1
}

function usage3(){
   echo "Usage: "
   echo ""
   echo "new make: $0 [HW version] [Compile mode] [Country]"
   echo ""
   echo "re-make: non-S1: $0 [Compile mode]"
   echo "         S1: $0 [Compile mode] AP "
   echo "             or $0 AP [Compile mode]"
   echo ""
   echo "clean: $0 -c or $0 clean"
   echo ""
   echo ""
   echo "HW version: DP, PDP, AP, DP_512M, RED"
   echo ""
   echo "Compile mode: user, eng, userdebug"
   echo ""
   echo "Country: CU, ROW, GEN(not work!)"
   echo ""
   exit 1
}

function usage4(){
   echo "Unknown action: $1."
   echo "Please use $0 -h, or $0 help, if you need help!"
   exit 1
}

function usage_country(){
   echo "Please choose build country: "
   echo "\"CU\""
   echo "\"ROW\""
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
   ./arima_scripts/red_sign_sbc.sh $1
}

function remake_S1(){
   echo "#### Do update-api ####"
   ./mk -t arima89_we_s_jb2 update-api

   if [ "$1" == "" ]; then
        echo "#### Re-compile eng mode ####"
        ./mk -t arima89_we_s_jb2 r
   elif [ "$1" == "user" ] || [ "$1" == "eng" ] || [ "$1" == "userdebug" ]; then
        echo "#### Re-compile $1 mode ####"
        ./mk -t -o=TARGET_BUILD_VARIANT=$1 arima89_we_s_jb2 r
        ./mk -t -o=TARGET_BUILD_VARIANT=$1 arima89_we_s_jb2 sign-image

   fi
}

function check_error_log(){
echo "### Check MTK error log file exist ###"
cd ${ERROR_LOG_PATH}
for i in `ls`; do
  if [ ${i##*.} == "log_err" ]; then
      echo "#################################################"
      echo "Error exist!"
      echo "Error log file = $i"
      echo "#################################################"
      #exit 1
      echo $i >> ${ERROR_LOG_RECORD}
  fi
done
cd ../../../
}

function rm_old_error_log(){
   echo "### Remove old error log files ###"
   pwd
   #cd ${ERROR_LOG_PATH}
   ERROR_LOG_LIST=`grep "log_err" ${ERROR_LOG_RECORD}`
   for j in ${ERROR_LOG_LIST}; do
       echo "### Remove $j ###"
       rm -rf $j
   done
   rm -rf ${ERROR_LOG_RECORD}
   #cd ../../../
}

function compile_S1_red(){
   echo "#### Clear environment ####"
   ./mk -t arima89_we_s_jb2 c
   echo "#### Do update-api ####"
   ./mk -t arima89_we_s_jb2 update-api
   echo "#### Compile $1 mode ####"
   ./mk -t -o=TARGET_BUILD_VARIANT=$1 arima89_we_s_jb2 n pl 
}

function clean(){
   echo "#### Clear environment ####"
   ./mk -t arima89_we_s_jb2 c
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
			usage4;
		      ;;
		esac
	fi	
	;;
    "DP" | "PDP" | "AP" | "DP_512M" | "RED")
	if [ -d $OUT ]; then
	check_error_log;
	cd ${ERROR_LOG_PATH}
	if [ -e ${ERROR_LOG_RECORD} ]; then
		rm_old_error_log;
	fi
	cd ../../../
	fi

	echo "### Configure HW version: HW_$1 ###"
	./arima_scripts/change_hw_version.sh HW_$1

	if [ "$2" == "user" ] || [ "$2" == "eng" ] || [ "$2" == "userdebug" ]; then

		BUILD_MODE=$2

        	if [ "$3" == "CU" ] || [ "$3" == "ROW" ]; then
                	echo "### Configure Build Country: $3 ###"
                	./arima_scripts/change_build_country.sh $3
              ###Begin_Arima_HCSW7_20130531_ChrisHe - Add GEN script
        	elif  [ "$3" == "GEN" ]; then
                        echo "Now not workable......"
                        exit 1
                ##        echo "### Configure Build Country: $3 ###"
                ##        ./arima_scripts/prepare_cdf_apk.sh CU  ## gen CDF_CU_apk.zip
                ##        ./arima_scripts/prepare_cdf_apk.sh ROW ## gen CDF_ROW_apk.zip
                ##        ./arima_scripts/change_build_country.sh $3 ## Set GEN property
              ###End_Arima_HCSW7_20130531_ChrisHe - Add GEN script
        	elif  [ "$3" == "" ]; then
			if [ "$1" == "AP" ]; then
			   remake_S1 $2; ##./build_5860.sh AP [user/eng/userdebug]
			fi
        	else
                	usage_country;
        	fi

    	      ###Begin_Arima_HCSW7_20130527_ChrisHe - Add HW_AP - S1 compile
                if [ "$1" == "AP" ]; then
                        echo "### S1: BROWN KEY - Key select: 3 ###"
          	        ./key_select.sh 3 ## for S1 build only           

                        echo "### HW_AP - Build S1 ###"
                        compile_S1 ${BUILD_MODE}; ## for S1 build only  
                  
                elif [ "$1" == "DP" ] || [ "$1" == "PDP" ] || [ "$1" == "DP_512M" ]; then
                        echo "### none-S1 build ###"
                        compile ${BUILD_MODE}; 
                elif [ "$1" == "RED" ]; then
                        echo "### S1: RED KEY - Key select: 6 ###"
                        ./key_select.sh 6
                        compile_S1_red ${BUILD_MODE};
                fi
              ###End_Arima_HCSW7_20130527_ChrisHe - Add HW_AP - S1 compile

	else
		usage;
	fi
	;;
    "help" | "-h" )
	usage3;
	;;
    "clean" | "-c" )
	clean;
	rm -rf $OUT
        exit 1
	;;
    *)
	if [ "$1" == "user" ] || [ "$1" == "eng" ] || [ "$1" == "userdebug" ]; then
		if [ "$2" == "" ]; then
			remake $1;
		elif [ "$2" == "AP" ]; then
			remake_S1 $1;
		fi			
	else
		usage4 $1;
	fi
	;;
esac

check_error_log;
cd ${ERROR_LOG_PATH}
if [ -e ${ERROR_LOG_RECORD} ]; then
   echo "### Remove ${ERROR_LOG_RECORD} ###"
   rm -rf ${ERROR_LOG_RECORD}
   echo "### Error exist! Exit the script... ###"
   exit 1
fi
cd ../../../

echo "### Gather and compress output files ###"
./hc_copyImage.sh $@

