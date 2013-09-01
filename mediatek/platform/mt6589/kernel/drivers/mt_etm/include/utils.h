#ifndef _UTILS_H_
#define _UTILS_H_

#include <linux/kernel.h> /* needed by printk */
#include "etm_trace_context.h"

/* buffer operations */
unsigned char buf_get_u8(const void *_buffer, unsigned int first,
				unsigned int num);
unsigned int buf_get_u32(const void *_buffer, unsigned int first,
				unsigned int num);

/* file operations */
struct file* open_file(const char *log_name);// open file for writing
void write_file(struct file *fp, const char *format, ...);
int close_file(struct file *fp);

// added by Mac Lee
struct file* open_file_R(const char *log_name); // open file for reading
int file_read(struct file* file, unsigned char* data, unsigned int size); // read raw data by size
int file_write(struct file* fp, unsigned char* data, unsigned int size); // write raw data by size
loff_t get_file_position(struct file *fp);
void put_file_position(struct file *fp, loff_t pos);
int does_reach_pre_position(struct file *fp, loff_t pos);

#ifdef __MT_PREEMPT
#define GET_PROCESSOR_ID raw_smp_processor_id()
#else
#define GET_PROCESSOR_ID smp_processor_id()
#endif

enum {
	LOG_LEVEL_NONE = -1,	/**< log nothing */
	LOG_LEVEL_ERROR = 3,	/**< log error and more critical infomation */
	/** log warning and more critical infomation */
	LOG_LEVEL_WARNING = 4,
	LOG_LEVEL_INFO = 6,	/**< log info and more critical infomation */
	/** log debug info and more critical infomation */
	LOG_LEVEL_DEBUG = 7,
};

/* log operations */
extern struct etm_trace_context_t tracer;
/** print infomation with log level ERROR */
#define p_error(arg...)\
do {\
	if (tracer.log_level >= LOG_LEVEL_ERROR) {\
		printk(KERN_ERR "[ETM decoder] error:[%s:%s:%d]", __FILE__,\
			__FUNCTION__, __LINE__);\
		printk(KERN_ERR "[ETM decoder] " arg);\
	}\
} while (0)
/** print infomation with log level WARNING */
#define p_warning(arg...)\
do {\
	if (tracer.log_level >= LOG_LEVEL_WARNING) {\
		printk(KERN_WARNING "[ETM decoder] warning:[%s:%s:%d]",\
			__FILE__, __FUNCTION__, __LINE__);\
		printk(KERN_WARNING "[ETM decoder] " arg);\
	}\
} while (0)
/** print infomation with log level INFO */
#define p_info(arg...) do {\
	if (tracer.log_level >= LOG_LEVEL_INFO) {\
		printk(KERN_INFO "[ETM decoder] " arg);\
	}\
} while (0)
/** print infomation with log level DEBUG */
#define p_debug(arg...) do {\
	if (tracer.log_level >= LOG_LEVEL_DEBUG) {\
		printk(KERN_DEBUG "[ETM decoder] " arg);\
	}\
} while (0)

#endif

