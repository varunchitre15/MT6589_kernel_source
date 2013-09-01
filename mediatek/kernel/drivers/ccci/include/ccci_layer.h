/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ccci_layer.h
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   MT65XX CCCI header file
 *
 ****************************************************************************/

#ifndef __CCCI_LAYER_H__
#define __CCCI_LAYER_H__

#define CCCI_LOGE(args...) printk(KERN_ERR args)
//20090901, EINT information share
#define AP_MD_EINT_SHARE_DATA     1

/*
 * define constants
 */


#include <linux/kfifo.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/kfifo.h>
#include <linux/io.h>
#include <asm/atomic.h>
#include <linux/rwlock.h>
#include <asm/bitops.h>
#include <linux/sched.h>

#define CCCI_DEV_NAME "ccci"
//#define CCCI_DEV_MAJOR 184 move to platform.h
#define CCCI_SYSFS_INFO "info"
#define CCCI_FIFO_MAX_LEN 8 /* 8 = max of physical channel */
#define CCCI_LOG_MAX_LEN 10
#define CCCI_LOG_TX 0
#define CCCI_LOG_RX 1

/*
 * define data structures
 */

/* CCCI API return value */
typedef enum
{
    CCCI_SUCCESS = 0,
    CCCI_FAIL = -EIO,
    CCCI_IN_USE = -EEXIST,
    CCCI_NOT_OWNER = -EPERM,
    CCCI_INVALID_PARAM = -EINVAL,
    CCCI_NO_PHY_CHANNEL = -ENXIO,
    CCCI_IN_INTERRUPT = -EACCES,
    CCCI_IN_IRQ = -EINTR,
    CCCI_MD_NOT_READY = -EBUSY,
    CCCI_MD_IN_RESET = -ESRCH,
    CCCI_RESET_NOT_READY = -ENODEV
}CCCI_RETURNVAL_T;

typedef enum
{
#define X_DEF_CH
#include "ccci_ch.h"
#undef X_DEF_CH
    CCCI_MAX_CHANNEL=100,
    CCCI_FORCE_RESET_MODEM_CHANNEL = 20090215,
} CCCI_CHANNEL_T;

/* CCCI mailbox channel structure */
typedef struct
{
    unsigned int magic;   /* 0xFFFFFFFF */
    unsigned int id;
} CCCI_MAILBOX_T;

/* CCCI stream channel structure */
typedef struct
{
    unsigned int addr;
    unsigned int len;
} CCCI_STREAM_T;

/* CCCI channel buffer structure */
typedef struct
{
    unsigned int data[2];
    unsigned int channel;
    unsigned int reserved;
} CCCI_BUFF_T;

/* CCCI callback function prototype */
typedef void (*CCCI_CALLBACK)(CCCI_BUFF_T *buff, void *private_data);

/* CCCI status */
/*
typedef enum
{
    CCCI_IDLE = 0x00,
    CCCI_ACTIVE_READ = 0x01,ror: expected specifier-qualifier-list before 'CCCI_BUFF_T'

    CCCI_ACTIVE_WRITE = 0x02,
    CCCI_ACTIVE_ISR = 0x04
} CCCI_STATE_T;
*/
/* CCCI control structure */

typedef enum {
	CCCI_ENABLED=0x0,
	CCCI_RUNNING=0x1,

} CCCI_STATE_T;
typedef struct _CCCI_LOG_T
{
    unsigned long sec;
    unsigned long nanosec;
    CCCI_BUFF_T buff;
    short rx;
} CCCI_LOG_T;

typedef struct dump_debug_info
{
	unsigned int type;
	char *name;
	union {
		struct {
			
			char file_name[30];
			int line_num;
			unsigned int parameters[3];
		} assert;	
		struct {
			int err_code1;
    			int err_code2;
		
		}fatal_error;
		CCCI_BUFF_T data;
		struct {
			unsigned char execution_unit[9]; // 8+1
			char file_name[30];
			int line_num;
			unsigned int parameters[3];
		}dsp_assert;
		struct {
			unsigned char execution_unit[9];
			unsigned int  code1;
		}dsp_exception;
		struct {
			unsigned char execution_unit[9];
			unsigned int  err_code[2];
		}dsp_fatal_err;
	};
	int *ext_mem;
	size_t ext_size;
	int *md_image;
	size_t md_size;
	void *platform_data;
	void (*platform_call)(void *data);
}DEBUG_INFO_T ;


struct lc_stats 
{
	unsigned long rx_num;
	unsigned long tx_num;
	unsigned long rx_drop_num;
	unsigned long tx_drop_num;
#ifdef __CCCI_LOG__
	int log_idx;
	CCCI_LOG_T log[CCCI_LOG_MAX_LEN];
#endif

} ;


struct logical_channel 
{
	spinlock_t lock;
	char *name;
	char owner[TASK_COMM_LEN];
	int buf_num;
	int have_fifo;
	struct kfifo fifo;
	void *private_data;
   	CCCI_CALLBACK callback;
	struct lc_stats  lc_statics;
	void (*dtor)(struct logical_channel *lg_ch);
} ;

struct logical_layer
{
	rwlock_t lock;
	atomic_t user;
	struct physical_channel *pc_channel;
	int lc_num;
	struct logical_channel **lg_array;
//	struct logical_channel *(*find)(struct logical_layer *lg_layer,int num);
	int (*add_client)(struct logical_layer *lg_layer,int num,int buf_num,struct logical_channel **);
	int (*remove_client)(struct logical_layer *lg_layer,int num);
	int (*process_data)(struct logical_layer *,CCCI_BUFF_T *,int in,int drop);
	void (*dump)(struct logical_layer *lg_layer,unsigned int nr);
	struct tasklet_struct tasklet;
};

struct physical_channel
{
	
	spinlock_t r_lock;
	unsigned int r_errors;
	unsigned int rx_idx;
	spinlock_t s_lock;
	unsigned int s_errors;
	unsigned int tx_idx;
	unsigned long flags;
	atomic_t irq_cnt;
	const int           max_pc_num;
	const unsigned long pc_base_addr;
	const int           irq_line;
	const int arb;
	struct logical_layer  lg_layer;
	int (*pc_init)(struct physical_channel *);
	int (*pc_destroy)(struct physical_channel *);
	int (*pc_xmit)(struct physical_channel *,CCCI_BUFF_T *,int force);
	int (*manual_triger)(struct physical_channel *,CCCI_BUFF_T *);
	void (*mask)(struct physical_channel *);
	void (*unmask)(struct physical_channel *);
	void (*disable)(struct physical_channel *,int);
	void (*enable)(struct physical_channel *);
	void (*reset)(struct physical_channel *);
	void (*dump)(struct physical_channel *,DEBUG_INFO_T *);
} ;



extern struct physical_channel ccif_ch;

extern int ccci_logical_layer_init(struct physical_channel *,struct logical_layer *lg_layer,int ch_num);
extern int ccci_logical_layer_destroy(struct logical_layer *lg_layer);
extern int pc_register(struct physical_channel *pc_ch,int ch_num, char *name,int buf_num,
		CCCI_CALLBACK call_back,void *private_data);
extern int pc_unregister(struct physical_channel *pc_ch,int ch_num);
/*
 * define macros
 */


/* initialize a CCCI mailbox buffer */
#define CCCI_INIT_MAILBOX(buff, mailbox_id) \
        do {    \
            ((CCCI_MAILBOX_T *)((buff)->data))->magic = 0xFFFFFFFF; \
            ((CCCI_MAILBOX_T *)((buff)->data))->id = (mailbox_id);  \
            (buff)->channel = CCCI_MAX_CHANNEL;  \
            (buff)->reserved = 0;    \
        } while (0)

/* initialize a CCCI mailbox buffer  with reserved value and logical channel*/
#define CCCI_INIT_FULL_MAILBOX(buff, mailbox_id, reserv) \
		do {	\
			((CCCI_MAILBOX_T *)((buff)->data))->magic = 0xFFFFFFFF; \
			((CCCI_MAILBOX_T *)((buff)->data))->id = (mailbox_id);	\
			(buff)->channel = CCCI_MAX_CHANNEL;  \
			(buff)->reserved = reserv;	 \
		} while (0)


/* initialize a CCCI stream buffer */
#define CCCI_INIT_STREAM(buff, stream_addr, stream_len) \
        do {    \
            ((CCCI_STREAM_T *)((buff)->data))->addr = (stream_addr); \
            ((CCCI_STREAM_T *)((buff)->data))->len = (stream_len);  \
            (buff)->channel = CCCI_MAX_CHANNEL;  \
            (buff)->reserved = 0;    \
        } while (0)

/* check the CCCI buffer type */
#define CCCI_IS_MAILBOX(buff)   ((((CCCI_MAILBOX_T *)((buff)->data))->magic == 0xFFFFFFFF)? 1: 0)

/* get the id of the CCCI mailbox buffer */
#define CCCI_MAILBOX_ID(buff)   (((CCCI_MAILBOX_T *)((buff)->data))->id)

/* get the addr of the CCCI stream buffer */
#define CCCI_STREAM_ADDR(buff)   (((CCCI_STREAM_T *)((buff)->data))->addr)

/* get the len of the CCCI stream buffer */
#define CCCI_STREAM_LEN(buff)   (((CCCI_STREAM_T *)((buff)->data))->len)

/* log CCCI transaction */
#define CCCI_LOG(c, act, b)    \
        do {    \
            struct timespec ts = current_kernel_time(); \
            ccci_log[(c)].sec[ccci_log[(c)].index] = ts.tv_sec;  \
            ccci_log[(c)].nanosec[ccci_log[(c)].index] = ts.tv_nsec;  \
            ccci_log[(c)].action[ccci_log[(c)].index] = (act);    \
            memcpy((void *)&(ccci_log[(c)].buff[ccci_log[(c)].index]), (void *)(b), sizeof(CCCI_BUFF_T));\
            ccci_log[(c)].index = (ccci_log[(c)].index + 1) % CCCI_LOG_MAX_LEN;    \
        } while (0)


#define ccci_register(ch,cb,para)    ({ \
		int __ret;	\
		__ret=pc_register(&ccif_ch,ch,#ch "(Auto Name)",CCCI_FIFO_MAX_LEN,cb,para); \
		__ret;  })

#define ccci_register_with_name_size(ch,nm,size,cb,para)    ({ \
		int __ret;	\
		int __size=(size)?:CCCI_FIFO_MAX_LEN; \
		__ret=pc_register(&ccif_ch,ch,nm,__size,cb,para); \
		__ret;  })
#define ccci_buff_t_assign(l,r)	({ \
		(l)->data[0]=(r)->data[0];\
		(l)->data[1]=(r)->data[1];\
		(l)->channel=(r)->channel;\
		(l)->reserved=(r)->reserved;})

#define ccci_unregister(ch)	({ \
	int __ret;	\
	__ret=pc_unregister(&ccif_ch,ch);\
	__ret;	})

#define ccci_manual_triger(data)     ({ \
		int __ret=ccif_ch.manual_triger(&ccif_ch,data); \
		(__ret==sizeof(*(data)))?CCCI_SUCCESS:__ret;})

#define ccci_system_message(buff)   \
	do {	\
		(buff)->channel=CCCI_SYSTEM_RX; \
		ccci_manual_triger(buff);\
	}while(0)

#define __ccci_write(ch_num,data,force)		({ \
	int __ret;\
	mb();\
	(data)->channel=ch_num;\
	__ret=ccif_ch.pc_xmit(&ccif_ch,data,force);	\
	(__ret==sizeof(*(data)))?CCCI_SUCCESS:__ret;  })

#define  ccci_write(ch_num,data)  __ccci_write(ch_num,data,0)
#define  ccci_write_force(ch_num,data)  __ccci_write(ch_num,data,1)



#define ccci_write_mailbox(ch,id)    ({ \
	CCCI_BUFF_T buff; \
	memset(&buff,0,sizeof(CCCI_BUFF_T));\
	CCCI_INIT_MAILBOX(&buff,id);\
	ccci_write(ch,&buff); })

#define ccci_write_mailbox_with_resv(ch,id,resv)    ({ \
	CCCI_BUFF_T buff; \
	memset(&buff,0,sizeof(CCCI_BUFF_T));\
	CCCI_INIT_FULL_MAILBOX(&buff,id,resv);\
	ccci_write(ch,&buff); })


#define ccci_write_stream(ch,addr,len)   ({ \
	CCCI_BUFF_T data; \
	CCCI_INIT_STREAM(&data,addr,len);  \
	ccci_write(ch,&data);	})


#define ccci_mask()    ccif_ch.mask(&ccif_ch)
#define ccci_unmask()    ccif_ch.unmask(&ccif_ch)
#define ccci_disable() ccif_ch.disable(&ccif_ch,1)
#define ccci_disable_nosync() ccif_ch.disable(&ccif_ch,0)
#define ccci_enable()  ccif_ch.enable(&ccif_ch)
#define ccci_reset()  ccif_ch.reset(&ccif_ch)

#define ccci_dump_debug_info(debug_info)   ccif_ch.dump(&ccif_ch,debug_info)
#define ccci_channel_status(n)	({\
	ccif_ch.lg_layer.dump(&ccif_ch.lg_layer,n);})



//typedef int (*is_md_boot_func)(void);
//typedef int (*reset_md_func)(void);
//typedef int (*ccci_base_req_func)(void*,int*);
//extern is_md_boot_func is_md_boot_funcp;
//extern reset_md_func reset_md_funcp;
//extern ccci_base_req_func ccci_pcm_base_req_funcp;
//extern ccci_base_req_func ccci_log_base_req_funcp;
//extern void ccci_register_mdfunc(is_md_boot_func func1, reset_md_func func2,
//		 ccci_base_req_func pcm_func,ccci_base_req_func log_func);
extern int __init ccif_module_init(void);
extern void __exit ccif_module_exit(void);

void my_default_end(void);
void my_mem_dump(int *mem, int size,void (*end)(void),char *fmt,...);

/*
 * define IOCTL commands
 */

#define CCCI_IOC_MAGIC 'C'
#define CCCI_IOC_MD_RESET				_IO(CCCI_IOC_MAGIC, 0)
#define CCCI_IOC_PCM_BASE_ADDR			_IOR(CCCI_IOC_MAGIC, 2, unsigned int)
#define CCCI_IOC_PCM_LEN				_IOR(CCCI_IOC_MAGIC, 3, unsigned int)
#define CCCI_IOC_FORCE_MD_ASSERT		_IO(CCCI_IOC_MAGIC, 4)
#define CCCI_IOC_ALLOC_MD_LOG_MEM		_IO(CCCI_IOC_MAGIC, 5)
#define CCCI_IOC_DO_MD_RST				_IO(CCCI_IOC_MAGIC, 6)
#define CCCI_IOC_SEND_RUN_TIME_DATA		_IO(CCCI_IOC_MAGIC, 7)
#define CCCI_IOC_GET_MD_INFO			_IOR(CCCI_IOC_MAGIC, 8, unsigned int)
#define CCCI_IOC_GET_MD_EX_TYPE			_IOR(CCCI_IOC_MAGIC, 9, unsigned int)
#define CCCI_IOC_SEND_STOP_MD_REQUEST	_IO(CCCI_IOC_MAGIC, 10)
#define CCCI_IOC_SEND_START_MD_REQUEST	_IO(CCCI_IOC_MAGIC, 11)
#define CCCI_IOC_DO_STOP_MD				_IO(CCCI_IOC_MAGIC, 12)
#define CCCI_IOC_DO_START_MD			_IO(CCCI_IOC_MAGIC, 13)

#endif  /* !__CCCI_LAYER_H__ */

