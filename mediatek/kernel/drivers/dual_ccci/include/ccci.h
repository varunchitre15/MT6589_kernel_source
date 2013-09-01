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
#include <linux/version.h>

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