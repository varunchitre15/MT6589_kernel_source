#########################################################################
#########################################################################
############ Internal Signing ###########################################
#########################################################################
#########################################################################
mkdir -p ./out/target/product/arima89_we_s_jb2/S1_signbin 
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/S1SBL-sign.bin -o out/target/product/arima89_we_s_jb2/S1_signbin/S1SBL-sign.bin
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-ROOT -t MTKDAC -i out/target/product/arima89_we_s_jb2/preloader_arima89_we_s_jb2.bin -o out/target/product/arima89_we_s_jb2/S1_signbin/preloader_arima89_we_s_jb2.bin
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/MBR-sign -o out/target/product/arima89_we_s_jb2/S1_signbin/MBR-sign
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/EBR1-sign -o out/target/product/arima89_we_s_jb2/S1_signbin/EBR1-sign
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/EBR2-sign -o out/target/product/arima89_we_s_jb2/S1_signbin/EBR2-sign
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/lk-sign.bin -o out/target/product/arima89_we_s_jb2/S1_signbin/lk-sign.bin
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/boot-sign.img -o out/target/product/arima89_we_s_jb2/S1_signbin/boot-sign.img
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/recovery-sign.img -o out/target/product/arima89_we_s_jb2/S1_signbin/recovery-sign.img
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/secro-sign.img -o out/target/product/arima89_we_s_jb2/S1_signbin/secro-sign.img
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/sro-lock-sign.img -o out/target/product/arima89_we_s_jb2/S1_signbin/sro-lock-sign.img
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/sro-unlock-sign.img -o out/target/product/arima89_we_s_jb2/S1_signbin/sro-unlock-sign.img
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/logo-sign.bin -o out/target/product/arima89_we_s_jb2/S1_signbin/logo-sign.bin
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/system-sign.img -o out/target/product/arima89_we_s_jb2/S1_signbin/system-sign.img
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/cache-sign.img -o out/target/product/arima89_we_s_jb2/S1_signbin/cache-sign.img
./signatory -f cert_data.dat -c PLATFORM-MT6589-TEST-6308-0001-OEM0-KEY1 -t MTK -i out/target/product/arima89_we_s_jb2/signed_bin/userdata-sign.img -o out/target/product/arima89_we_s_jb2/S1_signbin/userdata-sign.img
#cp -Lv ./out/target/product/arima89_we_s_jb2/preloader_arima89_we_s_jb2.bin ./out/target/product/arima89_we_s_jb2/S1_signbin/preloader_arima89_we_s_jb2.bin
#########################################################################
#########################################################################
############ External Signing ###########################################
#########################################################################
#########################################################################
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MBR -t MTK -i out/target/product/arima89_we_s_jb2/MT6589_Android_scatter_emmc.txt -o out/target/product/arima89_we_s_jb2/S1_signbin/partition-image_S1-SW-TEST-B316-0001-MBR.sin
./signatory -f cert_data.dat -c S1-BOOT-TEST-B316-0001-MMC --sin-block-size 0x00080000 --pldf-address-offset 0x800 --sin-mtk-name PRELOADER -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/preloader_arima89_we_s_jb2.bin -n
./signatory -f cert_data.dat -c S1-BOOT-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name S1SBL -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/S1SBL-sign.bin -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name MBR -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/MBR-sign -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name EBR1 -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/EBR1-sign -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name UBOOT -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/lk-sign.bin -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name BOOTIMG -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/boot-sign.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name RECOVERY -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/recovery-sign.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name SEC_RO -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/secro-sign.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name LOGO -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/logo-sign.bin -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name EBR2 -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/EBR2-sign -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name ANDROID -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/system-sign.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name CACHE -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/cache-sign.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name USRDATA -t SIN out/target/product/arima89_we_s_jb2/S1_signbin/userdata-sign.img -n
cp -Lv ./out/target/product/arima89_we_s_jb2/S1_signbin/EBR1-sign_S1-SW-TEST-B316-0001-MMC ./out/target/product/arima89_we_s_jb2/EBR1-sign_S1-SW-TEST-B316-0001-MMC.sin
cp -Lv ./out/target/product/arima89_we_s_jb2/S1_signbin/EBR2-sign_S1-SW-TEST-B316-0001-MMC ./out/target/product/arima89_we_s_jb2/EBR2-sign_S1-SW-TEST-B316-0001-MMC.sin
cp -Lv ./out/target/product/arima89_we_s_jb2/S1_signbin/MBR-sign_S1-SW-TEST-B316-0001-MMC ./out/target/product/arima89_we_s_jb2/MBR-sign_S1-SW-TEST-B316-0001-MMC.sin
rm ./out/target/product/arima89_we_s_jb2/S1_signbin/*-MMC
mv ./out/target/product/arima89_we_s_jb2/EBR1-sign_S1-SW-TEST-B316-0001-MMC.sin ./out/target/product/arima89_we_s_jb2/S1_signbin/
mv ./out/target/product/arima89_we_s_jb2/EBR2-sign_S1-SW-TEST-B316-0001-MMC.sin ./out/target/product/arima89_we_s_jb2/S1_signbin/
mv ./out/target/product/arima89_we_s_jb2/MBR-sign_S1-SW-TEST-B316-0001-MMC.sin ./out/target/product/arima89_we_s_jb2/S1_signbin/
cd ./out/target/product/arima89_we_s_jb2/S1_signbin/
zip -r -MM ${1}.zip *.sin
cd ../../../../..
mv ./out/target/product/arima89_we_s_jb2/S1_signbin/${1}.zip ./out/target/product/arima89_we_s_jb2/
#rm -r ./out/target/product/arima89_we_s_jb2/S1_signbin