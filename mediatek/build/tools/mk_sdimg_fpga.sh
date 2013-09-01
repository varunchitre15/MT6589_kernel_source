#!/bin/bash -e

ROOTFS=android_rootfs.bin
ANDROID=android_sd.tar.gz
PROJECT=mt6589_fpga

if [ "$1" != "" ]; then
	PROJECT=$1
fi
PROJECT_FOLDER=out/target/product/$PROJECT/

echo "Prepare SD image for $PROJECT ..."
rm -rf out/target/product/$PROJECT/system/bin/ccci_mdinit
rm -rf out/target/product/$PROJECT/root/system
rm -rf out/target/product/$PROJECT/root/data

if [ "$PROJECT" = "mt6589_fpga" ]; then
    echo "To shorten the booting time. Move unnecessary *.apk for $PROJECT ..."
    unnecessary_apks=("CalendarProvider.apk" "ContactsProvider.apk" "MediaProvider.apk" "SogouInput.apk" "Provision.apk" "OOBE.apk"
    "Contacts.apk" "SystemUI.apk" "LatinIME.apk" "UserDictionaryProvider.apk" "dm.apk" "SmsReg.apk" "ActivityNetwork.apk" "DownloadProvider.apk"
    "AtciService.apk" "DeskClock.apk" "Email.apk" "Exchange.apk" "GoogleOta.apk" "Log2Server.apk" "Mms.apk" "MobileLog.apk" "Omacp.apk" "MtkWeatherProvider.apk" "MtkWeatherWidget.apk" "ModemLog.apk" "DmvProvider.apk" "MtkBt.apk" "QQBrowser.apk" "QQGame.apk" "QQIM.apk" "MTKThemvalManager.apk"
    )
    mkdir -p out/target/product/$PROJECT/system/app_unnecessary
    for apk in ${unnecessary_apks[@]}
    do
        if [ -e $PROJECT_FOLDER/system/app/$apk ]; then
            echo "mv $PROJECT_FOLDER/system/app/$apk $PROJECT_FOLDER/system/app_unnecessary/$apk"
            mv $PROJECT_FOLDER/system/app/$apk $PROJECT_FOLDER/system/app_unnecessary/
        fi
    done
fi

cd ./out/target/product/$PROJECT/root
find . -print | cpio -H newc -o > ../$ROOTFS

cd ../
cp -r data system/
mkdir -p system/data/local
touch system/data/local/enable_menu_key
cd system
tar zcvf ../$ANDROID .

cp ../$ANDROID ~/tmp/

