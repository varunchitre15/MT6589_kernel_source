mkdir -p ./out/target/product/arima89_we_s_jb2/external_sign_bin 
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MBR -t MTK -i out/target/product/arima89_we_s_jb2/MT6589_Android_scatter_emmc.txt -o out/target/product/arima89_we_s_jb2/external_sign_bin/partition-image_S1-SW-TEST-B316-0001-MBR.sin
./signatory -f cert_data.dat -c S1-BOOT-TEST-B316-0001-MMC --sin-block-size 0x00080000 --pldf-address-offset 0x800 --sin-mtk-name PRELOADER -t SIN out/target/product/arima89_we_s_jb2/preloader_arima89_we_s_jb2.bin -n
./signatory -f cert_data.dat -c S1-BOOT-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name S1SBL -t SIN vendor/semc/s1_release/S1_img_delivery/S1SBL/S1SBL.bin -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name MBR -t SIN out/target/product/arima89_we_s_jb2/MBR -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name EBR1 -t SIN out/target/product/arima89_we_s_jb2/EBR1 -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name UBOOT -t SIN out/target/product/arima89_we_s_jb2/lk.bin -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name BOOTIMG -t SIN out/target/product/arima89_we_s_jb2/boot.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name RECOVERY -t SIN out/target/product/arima89_we_s_jb2/recovery.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name SEC_RO -t SIN out/target/product/arima89_we_s_jb2/secro.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name LOGO -t SIN out/target/product/arima89_we_s_jb2/logo.bin -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name EBR2 -t SIN out/target/product/arima89_we_s_jb2/EBR2 -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name ANDROID -t SIN out/target/product/arima89_we_s_jb2/system.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name CACHE -t SIN out/target/product/arima89_we_s_jb2/cache.img -n
./signatory -f cert_data.dat -c S1-SW-TEST-B316-0001-MMC --sin-block-size 0x00080000 --sin-mtk-name USRDATA -t SIN out/target/product/arima89_we_s_jb2/userdata.img -n
mv ./vendor/semc/s1_release/S1_img_delivery/S1SBL/*.sin ./out/target/product/arima89_we_s_jb2/external_sign_bin 
cp -Lv ./out/target/product/arima89_we_s_jb2/EBR1_S1-SW-TEST-B316-0001-MMC ./out/target/product/arima89_we_s_jb2/external_sign_bin/EBR1_S1-SW-TEST-B316-0001-MMC.sin
cp -Lv ./out/target/product/arima89_we_s_jb2/EBR2_S1-SW-TEST-B316-0001-MMC ./out/target/product/arima89_we_s_jb2/external_sign_bin/EBR2_S1-SW-TEST-B316-0001-MMC.sin
cp -Lv ./out/target/product/arima89_we_s_jb2/MBR_S1-SW-TEST-B316-0001-MMC ./out/target/product/arima89_we_s_jb2/external_sign_bin/MBR_S1-SW-TEST-B316-0001-MMC.sin
rm ./out/target/product/arima89_we_s_jb2/*-MMC
mv ./out/target/product/arima89_we_s_jb2/*.sin ./out/target/product/arima89_we_s_jb2/external_sign_bin 