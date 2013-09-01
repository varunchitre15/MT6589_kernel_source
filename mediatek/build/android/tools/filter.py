#!/proj/map/bin/python/bin/python

import os

filter = [
  "external/besloudness",
  "external/fmtest",
  "external/libfm",
  "$(MTK_PATH_SOURCE)/external/camera",
  "$(MTK_PATH_SOURCE)/external/ccci_fsd",
  "$(MTK_PATH_SOURCE)/external/ccci_mdinit",
  "$(MTK_PATH_SOURCE)/external/mhal",
  "$(MTK_PATH_SOURCE)/external/mnl",
  "$(MTK_PATH_SOURCE)/external/Rachel_Load",
  "$(MTK_PATH_SOURCE)/external/RIL_ut_program",
  "external/opencore",
  "external/ppp",
  ["external/skia",
    ["external/skia/include",
     "external/skia/emoji",
     "external/skia/src/ports/SkImageRef_ashmem.h"]],
  ["frameworks/base/camera/libcameraservice",
    ["frameworks/base/camera/libcameraservice/CameraService.h"]],
  ["frameworks/base/include/private/media/",
    ["frameworks/base/include/private/media/VideoFrame.h"]],
  ["frameworks/base/libs/audioflinger",
    ["frameworks/base/libs/audioflinger/AudioFlinger.h",
     "frameworks/base/libs/audioflinger/AudioBufferProvider.h"]],
  ["frameworks/base/libs/surfaceflinger",
    ["frameworks/base/libs/surfaceflinger/SurfaceFlinger.h",
     "frameworks/base/libs/surfaceflinger/Barrier.h",
     "frameworks/base/libs/surfaceflinger/CPUGauge.h",
     "frameworks/base/libs/surfaceflinger/LayerBlur.h",
     "frameworks/base/libs/surfaceflinger/Layer.h",
     "frameworks/base/libs/surfaceflinger/VRamHeap.h",
     "frameworks/base/libs/surfaceflinger/BlurFilter.h",
     "frameworks/base/libs/surfaceflinger/LayerBase.h",
     "frameworks/base/libs/surfaceflinger/LayerBuffer.h",
     "frameworks/base/libs/surfaceflinger/LayerOrientationAnim.h",
     "frameworks/base/libs/surfaceflinger/Tokenizer.h",
     "frameworks/base/libs/surfaceflinger/clz.h",
     "frameworks/base/libs/surfaceflinger/LayerBitmap.h",
     "frameworks/base/libs/surfaceflinger/LayerDim.h",
     "frameworks/base/libs/surfaceflinger/OrientationAnimation.h",
     "frameworks/base/libs/surfaceflinger/Transform.h", ]],
  "frameworks/base/libs/ui",
  "frameworks/base/media/java",
  "frameworks/base/media/jni",
  "frameworks/base/media/libmedia",
  "frameworks/base/media/tests",
  "frameworks/base/opengl/conform",
  "frameworks/base/opengl/GLBenchmark",
  "frameworks/base/opengl/libagl",
  "frameworks/base/opengl/libagl_mtk",
  "frameworks/base/opengl/libs",
  "frameworks/base/services/java",
  "frameworks/base/telephony",
  "hardware/libhardware_legacy/gps",
  "hardware/libhardware_legacy/wifi",
  "$(MTK_PATH_SOURCE)/hardware/gsm0710muxd",
  ["$(MTK_PATH_SOURCE)/hardware/ril",
    ["$(MTK_PATH_SOURCE)/hardware/ril/mtk-ril/oper.lis"]],
  "hardware/ril/include",
  "hardware/ril/libril",
  "hardware/ril/reference-cdma-sms",
  "hardware/ril/reference-ril",
  "hardware/ril/rild",
  "packages/apps/Camera",
  "packages/apps/MediaTek/FileManager",
  "packages/apps/MediaTek/WebBench",
  "packages/apps/MediaTek/WebSpeed",
  "packages/apps/MyTube",
  "packages/apps/SoundRecorder",
  "packages/apps/VideoPlayer",
  ["system/bluetooth",[
    "system/bluetooth/bluedroid/include",
    "system/bluetooth/data",
    "system/bluetooth/bluez-clean-headers/bluetooth"]],
  "system/core/include/cutils/pmem.h",
  "system/core/init",
  "system/core/libcutils",
]

for item in filter:
  if type(item)==type(""): os.system("rm -rf %s"%item)
  else:
    os.system("rm -rf .tmp")
    os.system("mkdir -p .tmp")
    count=0
    for subitem in item[1]:
      count=count+1
      os.system("mv %s .tmp/%d"%(subitem,count))
    os.system("rm -rf %s"%item[0])
    count=0
    for subitem in item[1]:
      count=count+1
      head,tail=os.path.split(subitem)
      os.system("mkdir -p %s"%head)
      os.system("mv .tmp/%d %s"%(count,subitem))

