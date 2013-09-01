#
# for USB OTG silicon based on Mentor Graphics INVENTRA designs
#
obj-$(CONFIG_USB_MTK_HDRC)	:= usb20.o
ifeq ($(CONFIG_MTK_AEE_KDUMP),y)
obj-$(CONFIG_USB_MTK_HDRC) += usb_dump.o
endif
