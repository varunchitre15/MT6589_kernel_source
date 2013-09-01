include $(MTK_PATH_BUILD)/common.mk
#$(call all-subdir-src-or-makefile)

obj-y   +=  aee/
obj-y   +=  ccci/
obj-y   +=  dual_ccci/
obj-y   +=  fmradio/
obj-y   +=  gps/
obj-y   +=  hwmon/
obj-y   +=  m4u/
obj-y   +=  mhl/
obj-y   +=  mtprof/
obj-y   +=  sched/
obj-y   +=  usb11/
obj-y   +=  almk/
ifneq ($(MTK_PROJECT), mt6589fpga_ca7_ldvt)
obj-y   +=  video/
endif
obj-y   +=  bthid/
obj-y   +=  ccmni/
 
obj-y   +=  emd_ctl/

# gator-driver is not custom-release and will be removed
obj-y	+= $(call all-subdirs-with-makefile,gator-driver)

obj-y   +=  hdmitx/
obj-y   +=  leds/
obj-y   +=  mmp/
obj-y		+=  ram_console/
obj-y   +=  net/
obj-y   +=  thermal/
obj-y   +=  videocodec/
obj-y   +=  xlog/
obj-y   +=  wdk/
obj-y   +=  usb20/

ifeq ($(CONFIG_MTK_COMBO),m)
obj-y   +=  combo/
endif
obj-y   +=  ion_drv/
ifeq ($(MTK_NFC_SUPPORT),yes)
obj-y	+= nfc/
endif

#//<2013/3/4-22421-jessicatseng, [Pelican] Add Sony XSSM function
obj-y += sony_xssm/
#//>2013/3/4-22421-jessicatseng
