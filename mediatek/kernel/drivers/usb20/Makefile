#
# for USB OTG silicon based on Mentor Graphics INVENTRA designs
#
obj-$(CONFIG_USB_MTK_HDRC)	:= musb_hdrc.o
musb_hdrc-objs += musb_core.o
ifeq ($(CONFIG_USB_MTK_HDRC_GADGET),y)
	musb_hdrc-objs		+= musb_gadget_ep0.o musb_gadget.o
endif

ifeq ($(CONFIG_USB_MTK_HDRC_HCD),y)
	musb_hdrc-objs		+= musb_virthub.o musb_host.o musb_otg_if.o
endif

ifeq ($(CONFIG_USB_MTK_DEBUG_FS),y)
	musb_hdrc-objs		+= musb_debugfs.o
endif

# the kconfig must guarantee that only one of the
# possible I/O schemes will be enabled at a time ...
# PIO only, or DMA (several potential schemes).
# though PIO is always there to back up DMA, and for ep0

musb_hdrc-objs		+= musbhsdma.o
#musb_hdrc-objs		+= mtk_usb.o


################################################################################

# Debugging

ifeq ($(CONFIG_USB_MTK_DEBUG),y)
	EXTRA_CFLAGS += -DDEBUG
ccflags-y += -I$(MTK_PATH_PLATFORM)/drivers/power/	
endif
