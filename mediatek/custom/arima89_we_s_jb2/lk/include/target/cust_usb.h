#ifndef __CUST_USB_H__
#define __CUST_USB_H__

#define CONFIG_USBD_LANG	"0409"

#ifdef SONY_S1_SUPPORT
#define USB_VENDORID		(0x0FCE)
#define USB_PRODUCTID		(0x0DDE)
#define USB_VERSIONID		(0x0100)
#define USB_MANUFACTURER	"Sony Mobile Communications AB"
#define USB_PRODUCT_NAME	"S1Boot Fastboot"
#else
#define USB_VENDORID		(0x0BB4)
#define USB_PRODUCTID		(0x0C01)
#define USB_VERSIONID		(0x0100)
#define USB_MANUFACTURER	"MediaTek"
#define USB_PRODUCT_NAME	"Android"
#endif // SONY added
#define FASTBOOT_DEVNAME	"arima89_we_s_jb2"
#define SN_BUF_LEN		19

#endif /* __CUST_USB_H__ */
