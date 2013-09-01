
cp -R vendor/mediatek/yusu/artifacts/obj/hardware/libhardware_legacy/gps hardware/libhardware_legacy
cp -R vendor/mediatek/yusu/artifacts/obj/hardware/libhardware_legacy/wifi hardware/libhardware_legacy
mkdir -p $(MTK_PATH_SOURCE)/hardware/ril/mtk-ril
cp -R vendor/mediatek/yusu/artifacts/obj/$(MTK_PATH_SOURCE)/hardware/ril/mtk-ril $(MTK_PATH_SOURCE)/hardware/ril/
cp -R vendor/mediatek/yusu/artifacts/obj/external/opencore external/
