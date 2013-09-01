#ifndef __DISP_DRV_LOG_H__
#define __DISP_DRV_LOG_H__

#ifndef BUILD_UBOOT

    ///for kernel
    #include <linux/xlog.h>

    #define DISP_LOG_PRINT(level, sub_module, fmt, arg...)      \
        do {                                                    \
            xlog_printk(level, "DISP/"sub_module, fmt, ##arg);  \
        }while(0)
        
    #define LOG_PRINT(level, module, fmt, arg...)               \
        do {                                                    \
            xlog_printk(level, module, fmt, ##arg);             \
        }while(0)
        
#else

    ///for UBOOT
    #include <common.h>

    #define DISP_LOG_PRINT(level, sub_module, fmt, arg...)      \
        do {                                                    \
            printf("[DISP/"sub_module"]"fmt, ##arg);            \
        }while(0)

    #define LOG_PRINT(level, module, fmt, arg...)          \
        do {                                                    \
            printf("["module"]"fmt, ##arg);                     \
        }while(0)

        
#endif

#endif // __DISP_DRV_LOG_H__
