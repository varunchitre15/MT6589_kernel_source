/*
 * Copyright (C) 2011 MediaTek, Inc.
 *
 * Author: Holmes Chiou <holmes.chiou@mediatek.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __MT_IO_LOGGER_H__
#define __MT_IO_LOGGER_H__

#ifndef USER_BUILD_KERNEL//engineering mode

#define IO_LOGGER_ENABLE 1
#else 
#define IO_LOGGER_ENABLE 0
#endif

#define IO_LOGGER_TRACE_T 50000000
#define IO_DUMP_LOGGER_T 5000000000

#define ILTAG "[IOLogger]"
#define ILog_MSG(fmt, args...) \
do {    \
		printk(KERN_INFO ILTAG""fmt" <- %s(): L<%d>  PID<%s><%d>\n", \
            	##args , __FUNCTION__, __LINE__, current->comm, current->pid); \
} while(0);

#define ITrace_MSG(fmt, args...) \
do {    \
		printk(KERN_INFO ILTAG""fmt" in PID<%s><%d>\n", \
            	##args, current->comm, current->pid); \
} while(0);

struct _loggerFuncName_
{
	int     FuncID;
	char* 	FuncName;
};

struct _loggerMsgFormat_
{
	int		MsgID;
	char* 	MsgFmt;
	char* 	DispFmt;
};

struct io_logger_file {
	char *buf;
	size_t size;
	size_t from;
	size_t count;
	loff_t index;
	loff_t read_pos;
	u64 version;
	struct mutex lock;
	void *private;
};

enum IO_Logger_MsgID
{
	IO_LOGGER_MSG_FIRST_ONE = 0,
	IO_LOGGER_MSG_VFS_INTFS = IO_LOGGER_MSG_FIRST_ONE,
	IO_LOGGER_MSG_VFS_INTFS_END,
	IO_LOGGER_MSG_VFS_OPEN_INTFS,
	IO_LOGGER_MSG_VFS_OPEN_INTFS_END,
	IO_LOGGER_MSG_VFS_NO_ARG,
	IO_LOGGER_MSG_VFS_NO_ARG_END,
	IO_LOGGER_MSG_NFI_R_W,
	IO_LOGGER_MSG_NFI_R_W_END,
	
	IO_LOGGER_MSG_LAST_ONE, // MUST BE THE LAST IO MSG ID

  
    
	IO_LOGGER_MSG_ID_MAX = IO_LOGGER_MSG_LAST_ONE// THE WHOLE ENUM LAST ONE
};

/*========IO PART========*/
enum IO_LogAPI
{

	IO_LOG_API___do_sys_open__func=0,
	IO_LOG_API___vfs_read__func,
	IO_LOG_API___vfs_write__func,
	IO_LOG_API___vfs_unlink__func,
	IO_LOG_API___vfs_sync__func,
	IO_LOG_API___do_truncate__func,
	IO_LOG_API___nand_read__func,
	IO_LOG_API___nand_write__func,

	IO_LOG_API___MAX__func
};

enum logger_type_i
{
	IO_LOGGER_TYPE_START = 0,
	LOGGER_TYPE_IO = IO_LOGGER_TYPE_START,
	IO_LOGGER_TYPE_END
};

enum logger_config
{
	ENABLE_IOLOGGER = 0,
	ENABLE_INMEM,
	ENABLE_PRINTK,
	ENABLE_RESERVER,
	ENABLE_PRONAME,
	IO_LOGGER_BUFSIZE
};
#ifndef USER_BUILD_KERNEL//engineering mode

#define CREATE_PROC_ENTRY_I(proc,x,y,z) proc = create_proc_entry(x,y,z)

#else

#define CREATE_PROC_ENTRY_I(proc,x,y,z)

#endif

extern void add_io_trace(enum logger_type_i type, unsigned int msg_id,
			unsigned int line_cnt, unsigned int func_id, ...);
extern void dump_trace_log(const char *func_name, unsigned long long timeoffset);

extern bool en_IOLogger(void);

extern void mt_disable_io_logger(void);

#define AddIOTrace(msg_id,name, ...) \
	add_io_trace(LOGGER_TYPE_IO, msg_id, __LINE__, \
		IO_LOG_API___##name##__func, __VA_ARGS__);
#define DumpIOTrace(timeoffset) \
	dump_trace_log(__FUNCTION__,timeoffset);
#define BEYOND_TRACE_LOG_TIME(timeoffset) (timeoffset>(unsigned long long)IO_LOGGER_TRACE_T)
#define BEYOND_DUMP_LOG_TIME(timeoffset) (unlikely(timeoffset>(unsigned long long)IO_DUMP_LOGGER_T))
#endif/* !__MT_IO_LOGGER_H__ */
