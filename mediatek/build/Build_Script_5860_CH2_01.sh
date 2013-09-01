echo -e "\n project1=$1 \n project2=$2 \n project3=$3 \n project4=$4 \n svn=$5\n type=$6\n SW_Prefix=$7\n SW_SubVersion=$8 \n buildLabel=$9 PW=${10} sign=${11} BuildType=${12} FLEX=${13} AUTO_VERSION=${14} SW_Version_Delta=${15} SW_Version_Delta_Day=${16} SW_Version_Increase_Day=${17} Security_key=${18} HW_Version=${19} \n"
export set project1=$1
export set project2=$2
export set project3=$3
export set project4=$4
export set Build_TOP=$(pwd)
export set TOP=$(pwd)
export set svn=$5
export set type=$6
export set Build_Label=$9
export set password=${10}
export set signimage=${11}
export set BuildType=${12}
export set FLEX=${13}
export set AUTO_VERSION=${14}
export set SW_Version_Delta=${15}
export set SW_Version_Delta_Day=${16}
export set SW_Version_Increase_Day=${17}
export set key=${18}
export set HW_Verion=${19}
export set USERCMD='-o=TARGET_BUILD_VARIANT=user'
export set USERDBGCMD='-o=TARGET_BUILD_VARIANT=userdebug'
export set week=$(date +%U)
export set day=$(date +%u)
export set year=$(date +%Y)


if [ "$AUTO_VERSION" == ON ]; then
echo "Year=${year}"
if [ ${day} -ge ${SW_Version_Increase_Day} ]; then
echo "Day=${day}"
export set SWSub=$(((${week})-(${SW_Version_Delta})+((52)*(${year}%(4)))+(${year}%(4))))0
echo "SWSub=${SWSub}"
export set SW_SubVersion=$8${SWSub}
else
echo "Day=${day}"
export set SWSub=$(((${week})-(${SW_Version_Delta}-(${SW_Version_Delta_Day}))+((52)*(${year}%(4)))+(${year}%(4))))0
echo "SWSub=${SWSub}"
export set SW_SubVersion=$8${SWSub}
fi
else
export set SW_SubVersion=$8
fi



cd ..
export set Build_TOPUP1=$(pwd)
#echo $password | sudo -S chmod a+x+w -R $Build_TOP
#echo $password | sudo -S rm -rf Images*
#echo $password | sudo -S rm Images*.*
#echo $password | sudo -S rm SourceCode*.*

sudo chmod a+x+w -R $Build_TOP
sudo rm -rf Images*
sudo rm Images*.*
sudo rm -rf Symbols*
sudo rm Symbols*.*
sudo rm SourceCode*.*

cd $Build_TOP
export set SW_Prefix=$7
export set SW_SubVersion=$8${SWSub}
export set buildLabel=$9

export set SW_Version1=$1_$svn
echo The SW version is [$SW_Version1]
case $type in
  "OFL")
	echo "Your choice is Officl type build"
	export set Build_Type=OFL
	;;
	"DB")
	echo "Your choice is Daily Build type build"
	export set Build_Type=DB
	;;
	"INT")
	echo "Your choice is INT type build"
	export set Build_Type=INT
	;;
	"DRV")
	echo "Your choice is DRV type build"
	export set Build_Type=DRV
	;;
	*)
	echo "Usage $type not match {OFL|DB|ROA|INT|DRV}"
	;;
esac	
#read -p "Please input [BLUR_Main_VERSION] for this build: " Blur_Main
#echo The BLUR_Main_VERSION is [$Blur_Main]
#./change_blur_main_version.sh $Blur_Main
#read -p "Please input [BLUR_Delta_VERSION] for this build: " Blur_Delta
#echo The BLUR_Delta_VERSION is [$Blur_Delta]
#./change_blur_delta_version.sh $Blur_Delta
#./Set_Carrier.sh
export set Build_Date=$(date +%G%m%d.%H%M%S)
export set Build_Info1=$SW_Version1.$Build_Type.$buildLabel
echo -e "change_sw_version_name.sh===> \n [svn]=$svn \n [Build_Info]=$Build_Info1 \n [Build_Type]=$Build_Type \n [Build_Date_TAG]=${Build_Date} \n [Build_Label]=$Build_Label \n [SW_Prefix]=$SW_Prefix"

./change_sw_version.sh ${SW_SubVersion} ${Build_Label} ${project1}

./sec_tp2.sh $signimage
./key_select.sh ${key}
./arima_scripts/change_hw_version.sh ${HW_Verion}

#project 1 section
if [ "$project1" == "NULL" ]; then
echo "skip build project1, becuase the NULL"
else 
if [ "$BuildType" == "ENG" ]; then
echo -e "[process] ==> ./mk -t ${project1} clean"
./mk -t ${project1} clean
echo -e "[process] ==> ./mk -t ${project1} update-api"
./mk -t ${project1} update-api
echo -e "[process] ==> ./mk -t ${project1} new"
./mk -t,j32 ${project1} new

if [ -f $Build_TOP/out/target/product/${project1}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project1}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project1}"
export set ImageBuildPass=fail
fi

if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e,j32 "[process] ==> ./mk -t ${project1} n kernel bootimagew"
./mk -t ${project1} n kernel bootimage
echo -e "[process] ==> ./mk -t ${project1} sign-image"
./mk -t ${project1} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e,j32 "[process] ==> ./mk -t ${project1} n kernel bootimagew"
./mk -t ${project1} n kernel bootimage
echo -e "[process] ==> ./mk -t ${project1} sign-image"
./mk -t ${project1} sign-image
echo -e "[process] ==> ./S1_sinnify.sh Images_$SW_Prefix$buildLabel"
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi
fi
fi

if [ "$BuildType" == "USER" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${project1} clean"
./mk -t ${USERCMD} ${project1} clean
echo -e "[process] ==> ./mk -t ${USERCMD} ${project1} update-api"
./mk -t ${USERCMD} ${project1} update-api
echo -e "[process] ==> ./mk -t ${USERCMD} ${project1} new"
./mk -t,j32 ${USERCMD} ${project1} new

if [ -f $Build_TOP/out/target/product/${project1}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project1}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project1}"
export set ImageBuildPass=fail
fi

if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${project1} n kernel bootimage"
./mk -t,j32 ${USERCMD} ${project1} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERCMD} ${project1} sign-image"
./mk -t ${USERCMD} ${project1} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${project1} n kernel bootimage"
./mk -t,j32 ${USERCMD} ${project1} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERCMD} ${project1} sign-image"
./mk -t ${USERCMD} ${project1} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi

if [ "$BuildType" == "USERDBG" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project1} clean"
./mk -t ${USERDBGCMD} ${project1} clean
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project1} update-api"
./mk -t ${USERDBGCMD} ${project1} update-api
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project1} new"
./mk -t,j32 ${USERDBGCMD} ${project1} new

if [ -f $Build_TOP/out/target/product/${project1}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project1}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project1}"
export set ImageBuildPass=fail
fi

if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project1} n kernel bootimage"
./mk -t,j32 ${USERDBGCMD} ${project1} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project1} sign-image"
./mk -t ${USERDBGCMD} ${project1} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project1} n kernel bootimage"
./mk -t,j32 ${USERDBGCMD} ${project1} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project1} sign-image"
./mk -t ${USERDBGCMD} ${project1} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi

if [ "$ImageBuildPass" == "true" ]; then
./collect-loadmodules.sh ${project1} ${signimage} Images_$SW_Prefix$buildLabel
cd $Build_TOPUP1
mkdir Images_${BuildType}_${Build_Info1}
mkdir Images_${BuildType}_${Build_Info1}/images
mkdir Symbols_${BuildType}_${Build_Info1}
mkdir Symbols_${BuildType}_${Build_Info1}/symbols

if [ "$signimage" == "NO" ]; then
cp -LR $Build_TOP/images/${project1}/ ${Build_TOPUP1}/Images_${BuildType}_${Build_Info1}/images
fi

if [ "$signimage" == "YES" ]; then
cp -LR $Build_TOP/images/${project1}_sign/ ${Build_TOPUP1}/Images_${BuildType}_${Build_Info1}/images
fi

if [ "$signimage" == "S1" ]; then
cp -LR $Build_TOP/images/${project1}_S1/ ${Build_TOPUP1}/Images_${BuildType}_${Build_Info1}/images
fi

cp -LR $Build_TOP/symbols/${project1}/ $Build_TOPUP1/Symbols_${BuildType}_${Build_Info1}/symbols
cp $Build_TOP/0ChangeNotes/5860_SW.txt $Build_TOPUP1/Images_${BuildType}_${Build_Info1}/5860_SW.xls
if [ "$FLEX" == "NON-FLEX" ]; then
mkdir Images_${BuildType}_${Build_Info1}_${FLEX}
mkdir Images_${BuildType}_${Build_Info1}_${FLEX}/images
cp -LR ${Build_TOP}/images/${project1}/ ${Build_TOPUP1}/Images_${BuildType}_${Build_Info1}_${FLEX}/images
cp ${Build_TOP}/0ChangeNotes/5860_SW.txt ${Build_TOPUP1}/Images_${BuildType}_${Build_Info1}_${FLEX}/5860_SW.xls
sudo rm ${Build_TOPUP1}/Images_${BuildType}_${Build_Info1}_${FLEX}/images/${project1}/XT*.*
fi
cd $Build_TOPUP1
tar -cf - Images_${BuildType}_${Build_Info1} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Images_${BuildType}_${Build_Info1}.tar.7z
tar -cf - Symbols_${BuildType}_${Build_Info1} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Symbols_${BuildType}_${Build_Info1}.tar.7z

if [ "$FLEX" == "NON-FLEX" ]; then
tar -cf - Images_${BuildType}_${Build_Info1}_${FLEX} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Images_${BuildType}_${Build_Info1}_${FLEX}.tar.7z
fi

export set BuildPass=true
fi
fi

#project 2 section
cd $Build_TOP

if [ "$project2" == "NULL" ]; then
echo "skip build project2, becuase the NULL"
else
export set SW_Version2=$2_$svn
echo The SW version is [$SW_Version2]
export set Build_Info2=$SW_Version2.$Build_Type.$buildLabel
echo -e "change_sw_version_name.sh===> \n [svn]=$svn \n [Build_Info]=$Build_Info2 \n [Build_Type]=$Build_Type \n [Build_Date_TAG]=${Build_Date} \n [Build_Label]=$Build_Label \n [SW_Prefix]=$SW_Prefix"
./change_sw_version.sh ${SW_SubVersion} ${Build_Info2} ${project2}
sudo rm -rf out
sudo rm -rf ./kernel/out
if [ "$BuildType" == "ENG" ]; then
echo -e "[process] ==> ./mk -t ${project1} clean"
./mk -t ${project1} clean
echo -e "[process] ==> ./mk -t ${project2} update-api"
./mk -t ${project2} update-api
echo -e "[process] ==> ./mk -t ${project2} new"
./mk -t,j32 ${project2} new

if [ -f $Build_TOP/out/target/product/${project2}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project2}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project2}"
export set ImageBuildPass=fail
fi


if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${project2} n kernel bootimagew"
./mk -t,j32 ${project2} n kernel bootimage
echo -e "[process] ==> ./mk -t ${project2} sign-image"
./mk -t ${project2} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${project2} n kernel bootimagew"
./mk -t,j32 ${project2} n kernel bootimage
echo -e "[process] ==> ./mk -t ${project2} sign-image"
./mk -t ${project2} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi

if [ "$BuildType" == "USER" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${project1} clean"
./mk -t ${USERCMD} ${project1} clean
echo -e "[process] ==> ./mk -t ${USERCMD} ${project2} update-api"
./mk -t ${USERCMD} ${project2} update-api
echo -e "[process] ==> ./mk -t ${USERCMD} ${project2} new"
./mk -t,j32 ${USERCMD} ${project2} new

if [ -f $Build_TOP/out/target/product/${project2}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project2}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project2}"
export set ImageBuildPass=fail
fi


if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${project2} n kernel bootimage"
./mk -t,j32 ${USERCMD} $projec2 n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERCMD} ${project2} sign-image"
./mk -t ${USERCMD} ${project2} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${project2} n kernel bootimage"
./mk -t,j32 ${USERCMD} $projec2 n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERCMD} ${project2} sign-image"
./mk -t ${USERCMD} ${project2} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi


if [ "$BuildType" == "USERDBG" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project1} clean"
./mk -t ${USERDBGCMD} ${project1} clean
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project2} update-api"
./mk -t ${USERDBGCMD} ${project2} update-api
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project2} new"
./mk -t,j32 ${USERDBGCMD} ${project2} new

if [ -f $Build_TOP/out/target/product/${project2}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project2}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project2}"
export set ImageBuildPass=fail
fi


if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project2} n kernel bootimage"
./mk -t,j32 ${USERDBGCMD} ${projec2} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project2} sign-image"
./mk -t ${USERDBGCMD} ${project2} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project2} n kernel bootimage"
./mk -t,j32 ${USERDBGCMD} ${projec2} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project2} sign-image"
./mk -t ${USERDBGCMD} ${project2} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi

if [ "$ImageBuildPass" == "true" ]; then
./collect-loadmodules.sh ${project2} ${signimage} Images_$SW_Prefix$buildLabel
cd $Build_TOPUP1
mkdir Images_${BuildType}_${Build_Info2}
mkdir Images_${BuildType}_${Build_Info2}/images
mkdir Symbols_${BuildType}_${Build_Info2}
mkdir Symbols_${BuildType}_${Build_Info2}/symbols

if [ "$signimage" == "NO" ]; then
cp -LR $Build_TOP/images/${project2}/ ${Build_TOPUP2}/Images_${BuildType}_${Build_Info2}/images
fi

if [ "$signimage" == "YES" ]; then
cp -LR $Build_TOP/images/${project2}_sign/ ${Build_TOPUP2}/Images_${BuildType}_${Build_Info2}/images
fi

if [ "$signimage" == "S1" ]; then
cp -LR $Build_TOP/images/${project2}_S1/ ${Build_TOPUP2}/Images_${BuildType}_${Build_Info2}/images
fi

cp -LR ${Build_TOP}/symbols/${project2}/ ${Build_TOPUP1}/Symbols_${BuildType}_${Build_Info2}/symbols
cp $Build_TOP/0ChangeNotes/5860_SW.txt ${Build_TOPUP1}/Images_${BuildType}_${Build_Info2}/5860_SW.xls

if [ "$FLEX" == "NON-FLEX" ]; then
mkdir Images_${BuildType}_${Build_Info2}_${FLEX}
mkdir Images_${BuildType}_${Build_Info2}_${FLEX}/images
cp -LR ${Build_TOP}/images/${project2}/ ${Build_TOPUP1}/Images_${BuildType}_${Build_Info2}_${FLEX}/images
cp ${Build_TOP}/0ChangeNotes/5860_SW.txt ${Build_TOPUP1}/Images_${BuildType}_${Build_Info2}_${FLEX}/5860_SW.xls
sudo rm ${Build_TOPUP1}/Images_${BuildType}_${Build_Info2}_${FLEX}/images/${project2}/XT*.*
fi

cd $Build_TOPUP1
tar -cf - Images_${BuildType}_${Build_Info2} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Images_${BuildType}_${Build_Info2}.tar.7z
tar -cf - Symbols_${BuildType}_${Build_Info2} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Symbols_${BuildType}_${Build_Info2}.tar.7z

if [ "$FLEX" == "NON-FLEX" ]; then
tar -cf - Images_${BuildType}_${Build_Info2}_${FLEX} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Images_${BuildType}_${Build_Info2}_${FLEX}.tar.7z
fi

cd $Build_TOP
export set BuildPass=true
fi
fi



#project 3 section.
cd $Build_TOP

if [ "$project3" == "NULL" ]; then
echo "skip build project3, becuase the NULL"
else
export set SW_Version3=$3_$svn
echo The SW version is [$SW_Version3]
export set Build_Info3=$SW_Version3.$Build_Type.$buildLabel
echo -e "change_sw_version_name.sh===> \n [svn]=$svn \n [Build_Info]=$Build_Info3 \n [Build_Type]=$Build_Type \n [Build_Date_TAG]=${Build_Date} \n [Build_Label]=$Build_Label \n [SW_Prefix]=$SW_Prefix"
./change_sw_version.sh ${SW_SubVersion} ${Build_Info3} ${project3}
sudo rm -rf out
sudo rm -rf ./kernel/out
if [ "$BuildType" == "ENG" ]; then
./mk -t ${project2} clean
./mk -t ${project3} update-api
./mk -t,j32 ${project3} new

if [ -f $Build_TOP/out/target/product/${project3}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project3}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project3}"
export set ImageBuildPass=fail
fi


if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${project3} n kernel bootimagew"
./mk -t,j32 ${project3} n kernel bootimage
echo -e "[process] ==> ./mk -t ${project3} sign-image"
./mk -t ${project3} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${project3} n kernel bootimagew"
./mk -t,j32 ${project3} n kernel bootimage
echo -e "[process] ==> ./mk -t ${project3} sign-image"
./mk -t ${project3} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi

if [ "$BuildType" == "USER" ]; then
./mk -t ${USERCMD} ${project2} clean
./mk -t ${USERCMD} ${project3} update-api
./mk -t,j32 ${USERCMD} ${project3} new

if [ -f $Build_TOP/out/target/product/${project3}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project3}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project3}"
export set ImageBuildPass=fail
fi


if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${projec3} n kernel bootimagew"
./mk -t,j32 ${USERCMD} ${project3} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERCMD} ${projec3} sign-image"
./mk -t ${USERCMD} ${project3} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${projec3} n kernel bootimagew"
./mk -t,j32 ${USERCMD} ${project3} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERCMD} ${projec3} sign-image"
./mk -t ${USERCMD} ${project3} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi

if [ "$BuildType" == "USERDBG" ]; then
./mk -t ${USERDBGCMD} ${project2} clean
./mk -t ${USERDBGCMD} ${project3} update-api
./mk -t,j32 ${USERDBGCMD} ${project3} new

if [ -f $Build_TOP/out/target/product/${project3}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project3}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project3}"
export set ImageBuildPass=fail
fi


if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} $projec3 n kernel bootimagew"
./mk -t,j32 ${USERDBGCMD} ${project3} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERDBGCMD} $projec3 sign-image"
./mk -t ${USERDBGCMD} ${project3} $sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} $projec3 n kernel bootimagew"
./mk -t,j32 ${USERDBGCMD} ${project3} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERDBGCMD} $projec3 sign-image"
./mk -t ${USERDBGCMD} ${project3} $sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi

if [ "$ImageBuildPass" == "true" ]; then
./collect-loadmodules.sh ${project3} ${signimage} Images_$SW_Prefix$buildLabel
cd $Build_TOPUP1
mkdir Images_${BuildType}_${Build_Info3}
mkdir Images_${BuildType}_${Build_Info3}/images
mkdir Symbols_${BuildType}_${Build_Info3}
mkdir Symbols_${BuildType}_${Build_Info3}/symbols

if [ "$signimage" == "NO" ]; then
cp -LR $Build_TOP/images/${project3}/ ${Build_TOPUP3}/Images_${BuildType}_${Build_Info3}/images
fi

if [ "$signimage" == "YES" ]; then
cp -LR $Build_TOP/images/${project3}_sign/ ${Build_TOPUP3}/Images_${BuildType}_${Build_Info3}/images
fi

if [ "$signimage" == "S1" ]; then
cp -LR $Build_TOP/images/${project3}_S1/ ${Build_TOPUP3}/Images_${BuildType}_${Build_Info3}/images
fi

cp -LR ${Build_TOP}/symbols/${project3}/ ${Build_TOPUP1}/Symbols_${BuildType}_${Build_Info3}/symbols
cp ${Build_TOP}/0ChangeNotes/5860_SW.txt ${Build_TOPUP1}/Images_${BuildType}_${Build_Info3}/5860_SW.xls

if [ "$FLEX" == "NON-FLEX" ]; then
mkdir Images_${BuildType}_${Build_Info3}_${FLEX}
mkdir Images_${BuildType}_${Build_Info3}_${FLEX}/images
cp -LR ${Build_TOP}/images/${project3}/ ${Build_TOPUP1}/Images_${BuildType}_${Build_Info3}_${FLEX}/images
cp ${Build_TOP}/0ChangeNotes/5860_SW.txt ${Build_TOPUP1}/Images_${BuildType}_${Build_Info3}_${FLEX}/5860_SW.xls
sudo rm ${Build_TOPUP1}/Images_${BuildType}_${Build_Info3}_${FLEX}/images/${project3}/XT*.*
fi

cd $Build_TOPUP1
tar -cf - Images_${BuildType}_${Build_Info3} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Images_${BuildType}_${Build_Info3}.tar.7z
tar -cf - Symbols_${BuildType}_${Build_Info3} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Symbols_${BuildType}_${Build_Info3}.tar.7z

if [ "$FLEX" == "NON-FLEX" ]; then
tar -cf - Images_${BuildType}_${Build_Info3}_${FLEX} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Images_${BuildType}_${Build_Info3}_${FLEX}.tar.7z
fi

cd $Build_TOP
export set BuildPass=true
fi
fi


#project 4 section.
cd $Build_TOP

if [ "$project4" == "NULL" ]; then
echo "skip build project4, becuase the NULL"
else
export set SW_Version4=$4_$svn
echo The SW version is [$SW_Version4]
export set Build_Info4=$SW_Version4.$Build_Type.$buildLabel
echo -e "change_sw_version_name.sh===> \n [svn]=$svn \n [Build_Info]=$Build_Info4 \n [Build_Type]=$Build_Type \n [Build_Date_TAG]=${Build_Date} \n [Build_Label]=$Build_Label \n [SW_Prefix]=$SW_Prefix"
./change_sw_version.sh ${SW_SubVersion} ${Build_Info4} ${project4}
sudo rm -rf out
sudo rm -rf ./kernel/out
if [ "$BuildType" == "ENG" ]; then
./mk -t ${project3} clean
./mk -t ${project4} update-api
./mk -t,j32 ${project4} new

if [ -f $Build_TOP/out/target/product/${project4}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project4}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project4}"
export set ImageBuildPass=fail
fi


if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${project4} n kernel bootimagew"
./mk -t,j32 ${project4} n kernel bootimage
echo -e "[process] ==> ./mk -t ${project4} sign-image"
./mk -t ${project4} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${project4} n kernel bootimagew"
./mk -t,j32 ${project4} n kernel bootimage
echo -e "[process] ==> ./mk -t ${project4} sign-image"
./mk -t ${project4} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi


if [ "$BuildType" == "USER" ]; then
./mk -t ${USERCMD} ${project3} clean
./mk -t ${USERCMD} ${project4} update-api
./mk -t,j32 ${USERCMD} ${project4} new

if [ -f $Build_TOP/out/target/product/${project4}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project4}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project4}"
export set ImageBuildPass=fail
fi


if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${project4} n kernel bootimagew"
./mk -t ${USERCMD} ${project4} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERCMD} ${project4} sign-image"
./mk -t ${USERCMD} ${project4} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${USERCMD} ${project4} n kernel bootimagew"
./mk -t ${USERCMD} ${project4} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERCMD} ${project4} sign-image"
./mk -t ${USERCMD} ${project4} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi

if [ "$BuildType" == "USERDBG" ]; then
./mk -t ${USERDBGCMD} ${project3} clean
./mk -t ${USERDBGCMD} ${project4} update-api
./mk -t,j32 ${USERDBGCMD} ${project4} new

if [ -f $Build_TOP/out/target/product/${project4}/system.img ]; then
echo -e "Image build PASS!!!!!==>${project4}"
export set ImageBuildPass=true
else
echo "Image build FAIL!!!!!==>${project4}"
export set ImageBuildPass=fail
fi


if [ "$ImageBuildPass" == "true" ]; then
if [ "$signimage" == "YES" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project4} n kernel bootimagew"
./mk -t,j32 ${USERDBGCMD} ${project4} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project4} sign-image"
./mk -t ${USERDBGCMD} ${project4} sign-image
fi

if [ "$signimage" == "S1" ]; then
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project4} n kernel bootimagew"
./mk -t,j32 ${USERDBGCMD} ${project4} n kernel bootimage
echo -e "[process] ==> ./mk -t ${USERDBGCMD} ${project4} sign-image"
./mk -t ${USERDBGCMD} ${project4} sign-image
./S1_sinnify.sh Images_$SW_Prefix$buildLabel
fi

fi
fi

if [ "$ImageBuildPass" == "true" ]; then
./collect-loadmodules.sh ${project4} ${signimage} Images_$SW_Prefix$buildLabel
cd $Build_TOPUP1
mkdir Images_${BuildType}_${Build_Info4}
mkdir Images_${BuildType}_${Build_Info4}/images
mkdir Symbols_${BuildType}_${Build_Info4}
mkdir Symbols_${BuildType}_${Build_Info4}/symbols

if [ "$signimage" == "NO" ]; then
cp -LR $Build_TOP/images/${project4}/ ${Build_TOPUP4}/Images_${BuildType}_${Build_Info4}/images
fi

if [ "$signimage" == "YES" ]; then
cp -LR $Build_TOP/images/${project4}_sign/ ${Build_TOPUP4}/Images_${BuildType}_${Build_Info4}/images
fi

if [ "$signimage" == "S1" ]; then
cp -LR $Build_TOP/images/${project4}_S1/ ${Build_TOPUP4}/Images_${BuildType}_${Build_Info4}/images
fi

cp -LR ${Build_TOP}/symbols/${project4}/ ${Build_TOPUP1}/Symbols_${BuildType}_${Build_Info4}/symbols
cp ${Build_TOP}/0ChangeNotes/5860_SW.txt ${Build_TOPUP1}/Images_${BuildType}_${Build_Info4}/5860_SW.xls

if [ "$FLEX" == "NON-FLEX" ]; then
mkdir Images_${BuildType}_${Build_Info4}_${FLEX}
mkdir Images_${BuildType}_${Build_Info4}_${FLEX}/images
cp -LR ${Build_TOP}/images/${project4}/ ${Build_TOPUP1}/Images_${BuildType}_${Build_Info4}_${FLEX}/images
cp ${Build_TOP}/0ChangeNotes/5860_SW.txt ${Build_TOPUP1}/Images_${BuildType}_${Build_Info4}_${FLEX}/5860_SW.xls
sudo rm ${Build_TOPUP1}/Images_${BuildType}_${Build_Info4}_${FLEX}/images/${project4}/XT*.*
fi

cd $Build_TOPUP1
tar -cf - Images_${BuildType}_${Build_Info4} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Images_${BuildType}_${Build_Info4}.tar.7z
tar -cf - Symbols_${BuildType}_${Build_Info4} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Symbols_${BuildType}_${Build_Info4}.tar.7z

if [ "$FLEX" == "NON-FLEX" ]; then
tar -cf - Images_${BuildType}_${Build_Info4}_${FLEX} | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si Images_${BuildType}_${Build_Info4}_${FLEX}.tar.7z
fi

cd $Build_TOP
export set BuildPass=true
fi
fi

#project sourcecode
#./offical_img_upload.sh
if [ "$BuildPass" == "true" ]; then 
./mk -t clean
#echo $password | sudo -S rm -rf out
#echo $password | sudo -S rm -rf images
sudo rm -rf out
sudo rm -rf ./kernel/out
sudo rm -rf images
sudo rm -rf symbols
cd $Build_TOPUP1
#echo $password | sudo -S chown -hR administrator $Build_TOP
#echo $password | sudo -S chgrp -hR administrator $Build_TOP
sudo chown -hR administrator $Build_TOP
sudo chgrp -hR administrator $Build_TOP
tar -cf - 5860 | 7za a -t7z -m0=lzma2 -mx=9 -mfb=64 -md=64m -mmt=6 -ms=on -si SourceCode_$SW_Prefix$buildLabel.tar.7z
sudo chown -hR smithhuang $Build_TOP
sudo chgrp -hR ccusers_8300 $Build_TOP
sudo rm -rf ${SW_Prefix}Backup
mv 5860 ${SW_Prefix}Backup
cd $Build_TOP
export set BuildPass=default

fi
