

#ifndef CUST_BLDR_H
#define CUST_BLDR_H

#include "boot_device.h"
#include "uart.h"

/*=======================================================================*/
/* Pre-Loader Features                                                   */
/*=======================================================================*/
#ifdef MTK_EMMC_SUPPORT
#define CFG_BOOT_DEV                (BOOTDEV_SDMMC)
#else
#define CFG_BOOT_DEV                (BOOTDEV_NAND)
#endif
#define CFG_FPGA_PLATFORM           (0)
//<2013/4/25-24321-jessicatseng, [5860] Don't check whether battery exist in preloader 
//<2013/4/19-24028-jessicatseng, [5860] Enable to check battery's temperature   
//<2013/1/21-20662-jessicatseng, [Pelican] Don't check if battery existing or not temporarily
#define CFG_BATTERY_DETECT          (0)//(1)
//>2013/1/21-20662-jessicatseng
//>2013/4/19-24028-jessicatseng
//>2013/4/25-24321-jessicatseng

#define CFG_UART_TOOL_HANDSHAKE     (1)
#define CFG_USB_TOOL_HANDSHAKE      (1)
#define CFG_USB_DOWNLOAD            (1)
#define CFG_PMT_SUPPORT             (1)

#define CFG_LOG_BAUDRATE            (921600)
#define CFG_META_BAUDRATE           (115200)
//<2013/01/18-20554-stevenchen,Switch pre-loader log from UART1 to UART4.
#define CFG_UART_LOG                (UART4) //(UART1)
//>2013/01/18-20554-stevenchen
#define CFG_UART_META               (UART1)

#define CFG_EMERGENCY_DL_SUPPORT    (1)
#define CFG_EMERGENCY_DL_TIMEOUT_MS (1000 * 30) /* 30 s */

/*=======================================================================*/
/* Misc Options                                                          */	
/*=======================================================================*/
#define FEATURE_MMC_ADDR_TRANS

#endif /* CUST_BLDR_H */
