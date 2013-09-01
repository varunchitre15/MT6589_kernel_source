
#ifndef _LOGGER_H_
#define _LOGGER_H_

#ifdef ENABLE_USB_LOGGER

#include <mach/mt_storage_logger.h>

#define USB_LOGGER(msg_id, func_name, args...) \
	do { \
		if(unlikely(is_dump_usb_gadget())) { \
			ADD_USB_TRACE(msg_id,func_name, args); \
		} \
	}while(0)

#else

#define USB_LOGGER(msg_id,func_name, args...) do{} while(0)

#endif

#endif /* _LOGGER_H_*/

