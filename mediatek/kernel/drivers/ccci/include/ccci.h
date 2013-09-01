#ifndef __CCCI_H__
#define __CCCI_H__

/*
#ifdef CCCI_DEBUG_ON
#define CCCI_DEBUG(format,arg...)   printk("CCCI:%s(),%d:" format,\
	__FUNCTION__,__LINE__,##arg)
#define CCCI_DEBUG_MSG(format, ...) printk("CCCI :" format, __VA_ARGS__)
#define CCCI_DEBUG_FUNC printk(KERN_ERR "%s\n",__FUNCTION__)
#else
#define CCCI_DEBUG_MSG(format, ...)
#define CCCI_DEBUG_FUNC 
#define CCCI_DEBUG(format,...) 
#endif
*/
#include <linux/kernel.h>
#include <ccci_layer.h>
#include <ccci_chrdev.h>
#include <ccci_md.h>
#include <ccci_fs.h>
#include <ccci_ipc.h>
#include <ccci_tty.h>
#include <ccci_pmic.h>
#include <ccci_platform.h>
#include <linux/xlog.h>
#include <linux/version.h>

/*********************************************************************************/
/**     Feature options                                                         **/
/*********************************************************************************/
//#define USING_PRINTK_LOG


/*********************************************************************************/
/**     debug log define                                                        **/
/*********************************************************************************/
/*---------------------------alway on log-----------------------------------*/
#ifdef USING_PRINTK_LOG

#define CCCI_MSG(fmt, args...)	printk("[ccci]" fmt, ##args)
#define CCCI_RAW		printk
//#define CCCI_DEBUG_MSG(format, ...) printk("CCCI :" format, __VA_ARGS__)

#else

#define CCCI_MSG(fmt, args...)		xlog_printk(ANDROID_LOG_ERROR, "ccci/com", fmt, ##args)
#define CCCI_MSG_INF(tag, fmt, args...)	xlog_printk(ANDROID_LOG_ERROR, "ccci/"tag, fmt, ##args)
#define CCCI_MSG_DBG(tag, fmt, args...)	xlog_printk(ANDROID_LOG_VERBOSE, "ccci/"tag, fmt, ##args)
//#define CCCI_MSG(fmt, args...)		xlog_printk(ANDROID_LOG_VERBOSE, "ccci/com", fmt, ##args)
//#define CCCI_MSG_INF(tag, fmt, args...)	xlog_printk(ANDROID_LOG_VERBOSE, "ccci/"tag, fmt, ##args)

#endif

/*---------------------------Switchable log--------------------------------*/
/* Debug message switch */
#define CCCI_DBG_NONE		(0x00000000)    /* No debug log */
#define CCCI_DBG_CTL		(0x00000001)    /* Control log */
#define CCCI_DBG_TTY		(0x00000002)    /* TTY channel log */
#define CCCI_DBG_FS		(0x00000004)    /* FS channel log */
#define CCCI_DBG_RPC		(0x00000008)    /* RPC channel log */
#define CCCI_DBG_IPC		(0x00000010)    /* IPC channel log */
#define CCCI_DBG_PMIC		(0x00000020)    /* PMIC channel log */
#define CCCI_DBG_CCMNI		(0x00000040)    /* CCMIN channel log */
#define CCCI_DBG_FUNC		(0x00000080)    /* Functiong entry log */
#define CCCI_DBG_MISC		(0x00000100)    /* Misc log */
#define CCCI_DBG_CHR		(0x00000200)	/* Char dev log */
#define CCCI_DBG_CCIF		(0x00000400)	/* Ccif log */
#define CCCI_DBG_ALL		(0xffffffff)

/*---------------------------------------------------------------------------*/
/* Switchable messages */
extern unsigned int ccci_msg_mask;

#ifdef USING_PRINTK_LOG

#define CCCI_DBG_MSG(mask, fmt, args...) \
do {	\
	if ((CCCI_DBG_##mask) & ccci_msg_mask ) \
            printk("[ccci]" fmt , ##args); \
} while(0)

#define CCCI_CTL_MSG(fmt, args...)	CCCI_DBG_MSG(CTL, "<ctl>"fmt, ##args)
#define CCCI_TTY_MSG(fmt, args...)	CCCI_DBG_MSG(TTY, "<tty>"fmt, ##args)
#define CCCI_FS_MSG(fmt, args...)	CCCI_DBG_MSG(FS, "<fs>"fmt, ##args)
#define CCCI_RPC_MSG(fmt, args...)	CCCI_DBG_MSG(RPC, "<rpc>"fmt, ##args)
#define CCCI_IPC_MSG(fmt, args...)	CCCI_DBG_MSG(IPC, "<ipc>"fmt, ##args)
#define CCCI_PMIC_MSG(fmt, args...)	CCCI_DBG_MSG(PMIC, "<pmic>"fmt, ##args)
#define CCCI_FUNC_ENTRY(f)		CCCI_DBG_MSG(FUNC, "%s\n", __FUNCTION__)
#define CCCI_MISC_MSG(fmt, args...)	CCCI_DBG_MSG(MISC, fmt, ##args)
#define CCCI_CHR_MSG(fmt, args...)	CCCI_DBG_MSG(CHR, "<chr>"fmt, ##args)
#define CCCI_CCIF_MSG(fmt, args...)	CCCI_DBG_MSG(CCIF, "<chr>"fmt, ##args)
#define CCCI_CCMNI_MSG(fmt, args...)	CCCI_DBG_MSG(CCMNI, "<ccmni>"fmt, ##args)


#else


#define CCCI_DBG_MSG(mask, tag, fmt, args...) \
do {	\
	if ((CCCI_DBG_##mask) & ccci_msg_mask ) \
            xlog_printk(ANDROID_LOG_ERROR, "ccci"tag, fmt , ##args); \
} while(0)

#define CCCI_CTL_MSG(fmt, args...)	CCCI_DBG_MSG(CTL, "/ctl", fmt, ##args)
#define CCCI_TTY_MSG(fmt, args...)	CCCI_DBG_MSG(TTY, "/tty", fmt, ##args)
#define CCCI_FS_MSG(fmt, args...)	CCCI_DBG_MSG(FS, "/fs ", fmt, ##args)
#define CCCI_RPC_MSG(fmt, args...)	CCCI_DBG_MSG(RPC, "/rpc", fmt, ##args)
#define CCCI_IPC_MSG(fmt, args...)	CCCI_DBG_MSG(IPC, "/ipc", fmt, ##args)
#define CCCI_PMIC_MSG(fmt, args...)	CCCI_DBG_MSG(PMIC, "/pmc", fmt, ##args)
#define CCCI_FUNC_ENTRY(f)		CCCI_DBG_MSG(FUNC, "/fun", "%s\n", __FUNCTION__)
#define CCCI_MISC_MSG(fmt, args...)	CCCI_DBG_MSG(MISC, "/mis", fmt, ##args)
#define CCCI_CHR_MSG(fmt, args...)	CCCI_DBG_MSG(CHR, "/chr", fmt, ##args)
#define CCCI_CCIF_MSG(fmt, args...)	CCCI_DBG_MSG(CCIF, "/cci", fmt, ##args)
#define CCCI_CCMNI_MSG(fmt, args...)	CCCI_DBG_MSG(CCMNI, "/net", fmt, ##args)
#endif


/******************************************************************************/
/** AEE function and macro define                                                                                **/
/******************************************************************************/
#ifdef CONFIG_MTK_AEE_FEATURE
extern void aed_md_exception1(int *log, int log_size, int *phy, int phy_size, 
		char* assert_type, char* filename, unsigned int line, unsigned int fata1, unsigned int fata2);
#endif

#define CCCI_AED_DUMP_EX_MEM		(0x00000001)
#define CCCI_AED_DUMP_MD_IMG_MEM	(0x00000002)
void ccci_aed(unsigned int flag, char* aed_str);
typedef void (*ccci_aed_cb_t)(unsigned int flag, char* aed_str);
struct init_mod {
	int (*init)(void);
	void (*exit)(void);
};

/******************************************************************************/
/** mdlogger mode define                                                                                           **/
/******************************************************************************/
typedef enum {
	MODE_UNKNOWN = -1, 	// -1
	MODE_IDLE,			    // 0
	MODE_USB,			      // 1
	MODE_SD,			      // 2
	MODE_POLLING,		    // 3
	MODE_WAITSD,		    // 4
}LOGGING_MODE;

/******************************************************************************/
/** irq handler error code                                                                                          **/
/******************************************************************************/
typedef enum {
	CLIENT_FAIL = 0,		//logical channel struct not create
	KFIFO_ALLOC_FAIL = 1,	        // allocate RX kfifo fail
	KFIFO_FULL = 2,			// kfifo is full for RX
	ERR_CODE_NUM,
}ERR_CODE;


#endif