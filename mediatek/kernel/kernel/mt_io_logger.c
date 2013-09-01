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


#include <stdarg.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <mach/mt_io_logger.h>
#include <linux/unistd.h> 
#include <linux/file.h>
#include <linux/vmalloc.h>

#if IO_LOGGER_ENABLE
#define IO_LOGGER_DEFAULT_ON
#else
#undef IO_LOGGER_DEFAULT_ON
#endif
/*
 *  Constant
 */


#define MAX_FILENAME_TOTAL_LENGTH (50)
#define MAX_SINGLE_LINE_SIZE 128
#define MAX_SINGLE_LINE_SIZE_INT	32
#define MAX_WHILE_LOOP 100

#define ACCESS_PERMISSION 0660	//666 means that all users can read/write but not execute

/*
 *  Macro
 */


/*
 *  Global variable
 */

static unsigned int io_logger_bufsize;

static char printkbuffer[MAX_SINGLE_LINE_SIZE];

static uint *io_logger_mem_pool = NULL;
//static char* seq_buf_ptr = NULL;
static int writeIndex = 0;
static int readIndex = 0;

static int lastwriteIndex = 0;
static bool iswrapped = false;
static atomic_t external_read_counter;


//static DEFINE_SPINLOCK(logger_lock);
static struct mutex logger_lock;

static bool enable_printk = false;
static bool enable_inMem = false;
static bool enable_ProcName = true;
static bool enable_IOLogger = false;
static bool enable_reverse = true;
static void en_io_logger(int i);
static void modify_io_logger_bufsize(unsigned long count);

static int printk_count = 20;

//static spinlock_t io_logger_lock;

/*========IO PART========*/
static struct _loggerMsgFormat_ ioLoggerFmt[]=
{

	{IO_LOGGER_MSG_VFS_INTFS,"%s%u","iFS_name:[%s],sz[%u]"},
	{IO_LOGGER_MSG_VFS_INTFS_END,"%s%X%Lu","iFS_name:[%s],ret[%X],dt[%Lu]-end"},
	{IO_LOGGER_MSG_VFS_OPEN_INTFS,"%s","iFS_name:[%s]"},
	{IO_LOGGER_MSG_VFS_OPEN_INTFS_END,"%s%Lu","iFS_name:[%s],dt[%Lu]-end"},
	{IO_LOGGER_MSG_VFS_NO_ARG,"",""},
	{IO_LOGGER_MSG_VFS_NO_ARG_END,"%Lu","dt[%Lu]-end"},
	{IO_LOGGER_MSG_NFI_R_W,"%x","page_addr:[%x]"},	
	{IO_LOGGER_MSG_NFI_R_W_END,"%x%d%Lu","page_addr:[%x],ret[%d],dt[%Lu]-end"},

	{IO_LOGGER_MSG_LAST_ONE,""}
};

static struct _loggerFuncName_ ioLoggerfunc[]=
{

	{IO_LOG_API___do_sys_open__func,"do_sys_open"},
	{IO_LOG_API___vfs_read__func,"vfs_read"},
	{IO_LOG_API___vfs_write__func,"vfs_write"},
	{IO_LOG_API___vfs_unlink__func,"vfs_unlink"},
	{IO_LOG_API___vfs_sync__func,"vfs_sync"},
	{IO_LOG_API___do_truncate__func,"do_truncate"},
	{IO_LOG_API___nand_read__func,"nand_read"},
	{IO_LOG_API___nand_write__func,"nand_write"},

	{IO_LOG_API___MAX__func,"Invalid function"}
};



struct logger_type_info {
	enum logger_type_i type;
	enum IO_Logger_MsgID msg_id_start;
	enum IO_Logger_MsgID msg_id_end;
	enum IO_LogAPI func_id_max;
	struct _loggerMsgFormat_ *logger_format;
	struct _loggerFuncName_ *logger_func_string;
};

static struct logger_type_info logger_info[]=
{
	{LOGGER_TYPE_IO, IO_LOGGER_MSG_FIRST_ONE, IO_LOGGER_MSG_LAST_ONE,
		IO_LOG_API___MAX__func, &ioLoggerFmt[0], &ioLoggerfunc[0]},

	

};

bool en_IOLogger()
{
	return enable_IOLogger;
}
static inline void __add_io_trace(enum logger_type_i type, unsigned int msg_id,
				unsigned int line_cnt, unsigned int func_id, va_list ap)
{
	int len = 0;
	int bin_len=0;
	int err_count=0;
	uint total_len = 0;
	const char* fmt = NULL;
	//unsigned long flags;
	unsigned long long curtime;
	unsigned long long threadtime;
	uint localbuffer[MAX_SINGLE_LINE_SIZE_INT];
	uint payload[MAX_SINGLE_LINE_SIZE_INT];
	//int temp_1;

	struct logger_type_info *info = &logger_info[type];

	/*Disable the IO Logger. Do nothing and just return*/
	if(!enable_IOLogger ||(!io_logger_mem_pool))
		return;

	if(msg_id < info->msg_id_start && msg_id > info->msg_id_end)
		msg_id = info->msg_id_end;

	fmt = info->logger_format[msg_id-info->msg_id_start].MsgFmt;

	/* Get current system clock */
	curtime = sched_clock();
	threadtime = current->se.sum_exec_runtime;

	//spin_lock_irq(&io_logger_lock);
	localbuffer[len++] = msg_id;

	/* store the current->comm into 2 int */
	if (enable_ProcName)
	{
		/* Max lenth of process name is 8 chars */
		//scnprintf( (char*)(localbuffer+len),8,"%8.8s", current->comm);
		memcpy((char*)(localbuffer+len),current->comm,8);
		len+=2; /* Shift 2 integer */
	}

	localbuffer[len++] =  task_pid_nr(current);
	func_id = min(func_id, (unsigned int)info->func_id_max);

	localbuffer[len++] = func_id;
	localbuffer[len++] = line_cnt;
	localbuffer[len++] = curtime&0xFFFFFFFF;
	localbuffer[len++] = (curtime>>32)&0xFFFFFFFF;
	localbuffer[len++] = threadtime&0xFFFFFFFF;
	localbuffer[len++] = (threadtime>>32)&0xFFFFFFFF;

/*printk(KERN_ERR "MsgID: %d, PID: %d, Func: %s, Line: %d, len:%d - %s",
				   msg_id,task_pid_nr(current), info->logger_func_string[func_id].FuncName, \
				   line_cnt, len, info->logger_format[msg_id-info->msg_id_start].DispFmt);*/
	/* Enabel in RT printk io logger */
	if (unlikely(enable_printk))
	{		
		vsnprintf(printkbuffer, MAX_SINGLE_LINE_SIZE, \
				info->logger_format[msg_id-info->msg_id_start].DispFmt, ap);

		ITrace_MSG("MsgID: %d, PID: %d, Func: %s, Line: %d, len:%d time:%Lu(%Lu)- %s",
				   msg_id,task_pid_nr(current), info->logger_func_string[func_id].FuncName, \
				   line_cnt, len, curtime, threadtime,printkbuffer);
	}

	/* Enabel in memory io logger */
	if (likely(enable_inMem))
	{	
#ifdef CONFIG_BINARY_PRINTF
		bin_len = vbin_printf(payload,MAX_SINGLE_LINE_SIZE_INT,fmt,ap);
#endif
		localbuffer[len++] = bin_len;
		//printk(KERN_ERR "sss--W :%d, r:%d is_wrapper: %d %d localbuffer %p\n",writeIndex,readIndex,iswrapped,io_logger_bufsize>>1,(char *)localbuffer);


		/* Lock section */
		//spin_lock_irqsave(&logger_lock, flags);
		mutex_lock(&logger_lock);
		//printk(KERN_ERR "sss--W :%d, r:%d is_wrapper: %d len %d bin_len %d buffer %p\n",writeIndex,readIndex,iswrapped,len,bin_len,(char *)localbuffer);

		/* Buffer is overflow!!!=>Reset writeindex = 0; */
		if ( (len + bin_len + writeIndex) >= (io_logger_bufsize>>2)  )
		{
			iswrapped = true;
			lastwriteIndex = writeIndex; /* Keep the last */
			writeIndex = 0;
			readIndex = 0;
		}		
		
		if (iswrapped)
		{
			err_count = 0;
			while ((len+bin_len) >= (readIndex-writeIndex)) {
				/* Check read index over boundary!!! */
				if (writeIndex >=  ((io_logger_bufsize>>2) - (io_logger_bufsize>>6) ) ) {
						readIndex = 0;
						iswrapped = false;
						break;						
				}

				if (unlikely(readIndex >= lastwriteIndex)) {
					readIndex = 0;
					iswrapped = false;
					break;					
				}

				if ((readIndex == 0)  &&  (writeIndex != 0)) {
					readIndex = 0;
					iswrapped = false;
					break;
				}
				if(unlikely(err_count > MAX_WHILE_LOOP)) {
					readIndex = 0;	
					iswrapped = false;
					break;
				}
				BUG_ON(readIndex > (io_logger_bufsize>>2));
				total_len = (io_logger_mem_pool[readIndex] >>24)&0xFF;
				BUG_ON(total_len == 0);
				readIndex += total_len;
				err_count++;
			}
		}


		total_len = (len+bin_len + 1)&0xFF;
		//localbuffer[total_len-1] = total_len;
		localbuffer[0] = localbuffer[0] |(total_len<<24);
	//	printk(KERN_ERR "eee---W :%d, r:%d is_wrapper: %d len:%d localbuffer:%p\n",writeIndex,readIndex,iswrapped,len,(char *)localbuffer);
		
		memcpy(io_logger_mem_pool + writeIndex, localbuffer, sizeof(uint)*len);
		writeIndex += len;
		memcpy(io_logger_mem_pool + writeIndex, payload, sizeof(uint)*bin_len);
		writeIndex += bin_len;
		io_logger_mem_pool[writeIndex] = (total_len <<24)|0xabc;
		writeIndex ++;
		//printk(KERN_ERR "eee---W :%d, r:%d is_wrapper: %d len:%d iobuffer:%p\n",writeIndex,readIndex,iswrapped,len,(char *)io_logger_mem_pool);
		
		mutex_unlock(&logger_lock);
		//spin_unlock_irqrestore(&logger_lock, flags);

		

	}
}

void add_io_trace(enum logger_type_i type, unsigned int msg_id,
			unsigned int line_cnt, unsigned int func_id, ...)
{
	va_list ap;
	va_start(ap, func_id);
	__add_io_trace(type, msg_id, line_cnt, func_id, ap);
	va_end(ap);
}



static int io_logger_read_config(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = page;
	int len = 0;
	p += sprintf(p, "\r\n[io_logger config flag]\r\n");
	p += sprintf(p, "=========================================\r\n" );
	p += sprintf(p, "enable_IOLogger = %d\r\n", enable_IOLogger);
	p += sprintf(p, "readIndex = %d\r\n", readIndex);
	p += sprintf(p, "writeIndex = %d\r\n", writeIndex);
	p += sprintf(p, "enable_inMem = %d\r\n", enable_inMem);
    p += sprintf(p, "enable_printk = %d\r\n", enable_printk);
    p += sprintf(p, "enable_reverse = %d\r\n", enable_reverse);
    p += sprintf(p, "enable_ProcName = %d\r\n", enable_ProcName);
	p += sprintf(p, "io_logger_bufsize = %d, %dMB\n", io_logger_bufsize, io_logger_bufsize/(1024*1024));

	*start = page + off;

	len = p - page;

	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
}

static char acBuf[MAX_SINGLE_LINE_SIZE];

static int io_logger_write_config(struct file *file, const char *buffer, unsigned long count, void *data)
{
	unsigned long CopySize = 0;
	unsigned int  tmp1,tmp2;

	CopySize = (count < (sizeof(acBuf) - 1)) ? count : (sizeof(acBuf) - 1);
	if(copy_from_user(acBuf, buffer, CopySize))
		return 0;
	acBuf[CopySize] = '\0';

	sscanf(acBuf,"%d %d",&tmp1,&tmp2);
	
	if(tmp1 != ENABLE_IOLOGGER){
		if(enable_IOLogger){
				printk(KERN_ERR "please disable io logger first\n");
				return count;
		}

	}
	switch(tmp1){
		case ENABLE_IOLOGGER:
			en_io_logger(tmp2);
			break;
		case ENABLE_INMEM:
			enable_inMem = (tmp2>0)? true:false;
			break;
		case ENABLE_PRINTK:
			enable_printk = (tmp2>0)? true:false;
			break;
		case ENABLE_RESERVER:
			enable_reverse = (tmp2>0)? true:false;
			break;
		case ENABLE_PRONAME:
			enable_ProcName = (tmp2>0)? true:false;
			break;
		case IO_LOGGER_BUFSIZE:
			modify_io_logger_bufsize(tmp2);
			break;
		default:
			break;
			

	}

	return count;
}

static void en_io_logger(int i)
{
	
	if(atomic_read(&external_read_counter) > 0){
		printk(KERN_ERR "some process open dump file,it can not enable IO Logger\n");
		return;
	}
	
	enable_IOLogger = (i>0)? true:false;
	
	if(enable_IOLogger) {
		iswrapped = false;
		writeIndex = 0;
		readIndex = 0;
		lastwriteIndex = 0;
	}


}

void mt_disable_io_logger(void)
{
	en_io_logger(0);
}

static int io_logger_help(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = page;
	int len = 0;
	
	p += sprintf(p, "=================help========================\r\n" );
	p += sprintf(p, "[io_logger_display]--- display io logger\n");	
	p += sprintf(p, "[io_logger_config] --- config io logger, you can read it before config\n\n");
	p += sprintf(p, "How to config io logger?? -->\n ");
	p += sprintf(p, "\t echo '[option] [value]' > /proc/driver/io_logger/io_logger_config\n");
	p += sprintf(p, "[option]:\n");
	p += sprintf(p, "%d: enable_IOLogger. 1: enable, 0:disable. \r\n", ENABLE_IOLOGGER);
	p += sprintf(p, "%d: enable_inMem. 1: enable, 0:disable.\r\n", ENABLE_INMEM);
    p += sprintf(p, "%d: enable_printk. 1: enable, 0:disable.\r\n", ENABLE_PRINTK);
    p += sprintf(p, "%d: enable_reverse. 1: enable, 0:disable.\r\n", ENABLE_RESERVER);
    p += sprintf(p, "%d: enable_ProcName. 1: enable, 0:disable.\r\n", ENABLE_PRONAME);
	p += sprintf(p, "%d: io_logger_bufsize.unit is KB\n", IO_LOGGER_BUFSIZE);

	*start = page + off;

	len = p - page;

	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len : count;
} 
static void modify_io_logger_bufsize(unsigned long count)
{
	
	unsigned int  local_logger_bs = count*1024;

	if(0 != local_logger_bs) {
		if((local_logger_bs != io_logger_bufsize)) {
			//Free first, then vmalloc
			ILog_MSG("io_logger_mem_pool_addr:0x%p", io_logger_mem_pool);

			if(enable_IOLogger){
				printk(KERN_ERR "please disable io logger first\n");
				return;
			}
			if(NULL !=io_logger_mem_pool)
			    vfree(io_logger_mem_pool);

			io_logger_mem_pool = vmalloc(local_logger_bs);
			ILog_MSG("io_logger_mem_pool_addr:0x%p, local_logger_bs:%d," \
				"io_logger_bufsize:%d", io_logger_mem_pool, local_logger_bs, \
				io_logger_bufsize);
		}
		io_logger_bufsize = local_logger_bs;

		if(NULL == io_logger_mem_pool)
			io_logger_bufsize = 0;
	}

}


static inline struct logger_type_info *find_type_info(unsigned int msg_id)
{
	unsigned int tmp = (unsigned int)IO_LOGGER_TYPE_START;
	for(;tmp < IO_LOGGER_TYPE_END; tmp++) {
		if(logger_info[tmp].msg_id_start <= msg_id && logger_info[tmp].msg_id_end >= msg_id)
			return &logger_info[tmp];
	}

	/* Cant find match type for this msg id, give the io type. */
	return &logger_info[IO_LOGGER_TYPE_START];
}

static void parse_log(char *buf, int count)
{
		
		char* fmt;
		char procname[16];
		unsigned long long curtime_high, curtime_low,threadtime_low,threadtime_high;
		
		uint localbuffer[MAX_SINGLE_LINE_SIZE_INT];
		unsigned int msg_id, tmp_pid, func_id, line_cnt, payload_len;
		struct logger_type_info *info = NULL;
		u32 *bufp = (u32 *)buf;
		int total_size;
		if(count == 0)
			return;
		printk("[parse_log]count %d\n",count);
		while(count > 0){


			/*Parser the data from the log memory*/
			msg_id = *bufp & 0x00ffffff;
			total_size = ((*bufp)>>24) &0xff;
			msg_id = min(msg_id, (unsigned int)IO_LOGGER_MSG_ID_MAX);
			bufp++;
			if(enable_ProcName) {
				memcpy(procname,(char*)bufp,8);
				procname[8]='\0';
				bufp += 2;
			}
	
			tmp_pid =  *bufp;
			bufp++;
			func_id = *bufp;
			bufp++;
			line_cnt = *bufp;
			bufp++;
			curtime_low = *bufp;
			bufp++;
			curtime_high = *bufp;
			bufp++;
			curtime_low |= (curtime_high<<32)&(0xFFFFFFFF00000000);
			threadtime_low = *bufp;
			bufp++;
			threadtime_high = *bufp;
			bufp++;
			threadtime_low |= (threadtime_high<<32)&(0xFFFFFFFF00000000);
			payload_len = *bufp;
			bufp++;
	
			info = find_type_info(msg_id);
			func_id = min(func_id, info->func_id_max);
	
			/* Output the relative info into the display buffer */
	
				printk(KERN_DEBUG "MsgID:%d,PID:[%s|%d],%s_%d,[%Lu|%Lu]-", \
						msg_id, procname,tmp_pid, info->logger_func_string[func_id].FuncName, \
						line_cnt, curtime_low,threadtime_low);
			
	
			/* Decode the binary buffer */
			fmt = info->logger_format[msg_id-(info->msg_id_start)].DispFmt;
	
	#ifdef CONFIG_BINARY_PRINTF
			bstr_printf((char *)localbuffer, MAX_SINGLE_LINE_SIZE, fmt, (u32 *)bufp);
	#endif
			/* Output the decoded info to the display buffer */
			printk(KERN_DEBUG " %s\n",(char *)localbuffer);
			bufp += payload_len;
			bufp ++;
			count -= total_size;

	}
	
}

static void parse_log_reverse(char *buf, int count)
{
		char* fmt;
		char procname[16];
		unsigned long long curtime_high, curtime_low,threadtime_low,threadtime_high;
		
		uint localbuffer[MAX_SINGLE_LINE_SIZE_INT];
		unsigned int msg_id, tmp_pid, func_id, line_cnt, payload_len;
		struct logger_type_info *info = NULL;
		u32 *bufp = (u32 *)buf + count;
		int total_length = 0;
		int sig;
		if(count == 0)
			return;
		printk("[parse_log_reverse]count %d\n",count);
		while((count>0) && (printk_count>0)){

			bufp = (u32 *)buf + count;

			total_length = ((*(bufp-1))>>24) & 0xff;

			sig = (*(bufp-1)) &0x0fff;
			if(sig != 0xabc){
				printk("[parse_log_reverse]total_length=%d,sig=%x\n",total_length,sig);
				return;
			}
			count -= total_length;
			
			bufp -= total_length;
			/*Parser the data from the log memory*/
			msg_id = *bufp & 0x0fff;
			msg_id = min(msg_id, (unsigned int)IO_LOGGER_MSG_ID_MAX);
			bufp++;
			if(enable_ProcName) {
				memcpy(procname,(char*)bufp,8);
				procname[8]='\0';
				bufp += 2;
			}
	
			tmp_pid = *bufp;
			bufp++;
			func_id =*bufp;
			bufp++;
			line_cnt = *bufp;
			bufp++;
			curtime_low = *bufp;
			bufp++;
			curtime_high = *bufp;
			bufp++;
			curtime_low += (curtime_high<<32);
			threadtime_low = *bufp;
			bufp++;
			threadtime_high = *bufp;
			bufp++;
			threadtime_low |= (threadtime_high<<32)&(0xFFFFFFFF00000000);
			payload_len = *bufp;
			bufp++;
	
			info = find_type_info(msg_id);
			func_id = min(func_id, info->func_id_max);
	
			/* Output the relative info into the display buffer */
	
				printk(KERN_DEBUG "MsgID:%d,PID:[%s|%d],%s_%d,[%Lu|%Lu]-", \
						msg_id, procname,tmp_pid, info->logger_func_string[func_id].FuncName, \
						line_cnt, curtime_low,threadtime_low);
			
	
			/* Decode the binary buffer */
			fmt = info->logger_format[msg_id-(info->msg_id_start)].DispFmt;
	
	#ifdef CONFIG_BINARY_PRINTF
			bstr_printf((char *)localbuffer, MAX_SINGLE_LINE_SIZE, fmt, (u32 *)bufp);
	#endif
			/* Output the decoded info to the display buffer */
			printk(KERN_DEBUG " %s\n",(char *)localbuffer);
			 /*Payload is decoded!!!!*/
			printk_count--;

	}

}


void dump_trace_log(const char *func_name, unsigned long long timeoffset)
{
	char *buf;
	int count = 0;
	//unsigned long flags;
	printk("++++++++++ [%s,%lld] trace timeout ++++dump trace log++++++\n",func_name,timeoffset);
	/* Force disable the io logger */
	mutex_lock(&logger_lock);
	if (enable_IOLogger) 	{
		enable_IOLogger = false;
		/* Boundary value check */
		BUG_ON( readIndex>(io_logger_bufsize >> 2) );
		BUG_ON( writeIndex>(io_logger_bufsize >> 2) );
		BUG_ON( lastwriteIndex>(io_logger_bufsize >> 2 ));
		BUG_ON( readIndex>lastwriteIndex );

		/* Take a sleep to wait the io logger stopping */
		msleep(10);
	}
	
	//spin_lock_irqsave(&logger_lock, flags);
if(enable_reverse){
	
	if( writeIndex < readIndex) {
		
		buf = (char*) (&io_logger_mem_pool[0]);
		count = writeIndex;
		parse_log_reverse(buf,count);

	}

	if (!iswrapped)
		lastwriteIndex = writeIndex;

	buf = (char*) (&io_logger_mem_pool[readIndex]);
	count =( lastwriteIndex-readIndex);
	parse_log_reverse(buf,count);

}else
{
	if (!iswrapped)
		lastwriteIndex = writeIndex;
	
	buf = (char*) (&io_logger_mem_pool[readIndex]);
	count =( lastwriteIndex-readIndex);
	parse_log(buf,count);


	if( writeIndex < readIndex) {
		
		buf = (char*) (&io_logger_mem_pool[0]);
		count = writeIndex ;
		parse_log(buf,count);

	}
}
	
	en_io_logger(1);
	printk_count = 20;
	mutex_unlock(&logger_lock);
	//spin_unlock_irqrestore(&logger_lock, flags);
	printk("++++++++++ [%s] trace timeout ++++dump trace log++++++\n",func_name);
}

ssize_t  io_logger_proc_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	struct io_logger_file *m = file->private_data;
	size_t copied = 0;	
	size_t n;
	int err = 0;
	
	/* Force disable the io logger */
	if (enable_IOLogger) 	{
		enable_IOLogger = false;
		/* Boundary value check */
		BUG_ON( readIndex>(io_logger_bufsize >> 2) );
		BUG_ON( writeIndex>(io_logger_bufsize >> 2) );
		BUG_ON( lastwriteIndex>(io_logger_bufsize >> 2) );
		BUG_ON( readIndex>lastwriteIndex );

		/* Take a sleep to wait the io logger stopping */
		msleep(10);
	}

	//ILog_MSG("io_logger_proc_read: %Lu %d  %d %d\n",*ppos,readIndex,writeIndex,lastwriteIndex);

	mutex_lock(&m->lock);

	if( *ppos == 0) {
		//seq_buf_ptr = m->buf;

		if (!iswrapped)
			lastwriteIndex = writeIndex;

		m->buf = (char*) (&io_logger_mem_pool[readIndex]);
		m->count =( lastwriteIndex-readIndex) << 2;
		m->size = io_logger_bufsize;
		m->index = 0;
	}
	

	m->version = file->f_version;
	/* grab buffer if we didn't have one */

	if (m->count) {
		n = min(m->count, size);
		err = copy_to_user(buf, m->buf + m->from, n);
		//ILog_MSG("copy_to_user: %d from:%d m->index:%lld\n",n,m->from,m->index);
		

		if (err)
			goto Done;
		m->count -= n;
		m->from += n;
		size -= n;
		buf += n;
		copied += n;
		if (!size)
			goto Done;
		if(!iswrapped)
			goto Done;					
	}

	if (m->index)
		goto Done;	

	/* we put the bottom record into buffer */
	if( writeIndex < readIndex) {
		m->index++;
		m->buf = (char*) (&io_logger_mem_pool[0]);
		m->count = writeIndex << 2;
		m->size = io_logger_bufsize;
		m->from = 0;
	}
	//ILog_MSG("second part: %Lu  %d\n",m->index,m->count);

Done:
	if (!copied)
		copied = err;
	else {
		*ppos += copied;
		m->read_pos += copied;
	}
	file->f_version = m->version;
	mutex_unlock(&m->lock);
	//ILog_MSG("return: %d  %d\n",copied,m->count);
	return copied;
}



int __io_logger_proc_release(struct inode *inode, struct file *file)
{
	struct io_logger_file *m = file->private_data;

	kfree(m);
	return 0;
}


int io_logger_proc_release(struct inode *inode, struct file *file)
{
	//unsigned long flags;
	
	int res = __io_logger_proc_release(inode, file);
	

	//spin_lock_irqsave(&logger_lock, flags);	
	atomic_dec(&external_read_counter);
	//spin_unlock_irqrestore(&logger_lock, flags);

	return res;
}

static int io_logger_proc_single_open(struct file *file, void *data)
{

	struct io_logger_file *p = file->private_data;

	if (!p) {
		p = kmalloc(sizeof(*p), GFP_KERNEL);
		if (!p)
			return -ENOMEM;
		file->private_data = p;
	}
	memset(p, 0, sizeof(*p));
	mutex_init(&p->lock);
	p->private = data;
	file->f_version = 0;
	return 0;
}

static int io_logger_proc_open(struct inode *inode, struct file *file)
{
	//unsigned long flags;
	//spin_lock_irqsave(&logger_lock, flags);
	atomic_inc(&external_read_counter);
	//spin_unlock_irqrestore(&logger_lock, flags);

	return io_logger_proc_single_open(file, NULL);
}


static const struct file_operations io_logger_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= io_logger_proc_open,
	.read		= io_logger_proc_read,
	//.llseek		= io_logger_proc_lseek,
	.release	= io_logger_proc_release,
};

static int __init io_logger_init(void)
{
	int ret = 0;
	struct proc_dir_entry *procEntry = NULL;
	struct proc_dir_entry *procDir = proc_mkdir("driver/io_logger",NULL);

	if(!procDir){
		ILog_MSG("Create /proc/driver/io_logger folder fail\n");
		return -1;
	}

	CREATE_PROC_ENTRY_I(procEntry,"help", ACCESS_PERMISSION, procDir);
	if (procEntry) {
		procEntry->read_proc = io_logger_help;
		procEntry->write_proc = NULL;
		procEntry->gid = 1000;
	} else
		ILog_MSG("add /proc/driver/io_logger entry fail");

	CREATE_PROC_ENTRY_I(procEntry,"io_logger_config", ACCESS_PERMISSION, procDir);
	if (procEntry) {
		procEntry->read_proc = io_logger_read_config;
		procEntry->write_proc = io_logger_write_config;
		procEntry->gid = 1000;
	} else
		ILog_MSG("add /proc/driver/io_logger_config entry fail");

	

	CREATE_PROC_ENTRY_I(procEntry, "io_logger_display", ACCESS_PERMISSION, procDir);
	if (procEntry) {
		procEntry->proc_fops = &io_logger_proc_fops;
		procEntry->gid = 1000;
	} else
		ILog_MSG("add /proc/driver/io_logger entry fail");


	//Initialize the variable used in this module
	writeIndex = 0;
	readIndex = 0;	
	lastwriteIndex = 0;

	iswrapped = false;
	enable_printk = false;
	atomic_set(&external_read_counter, 0);
#if defined( IO_LOGGER_DEFAULT_ON ) && defined (CONFIG_BINARY_PRINTF)
	enable_inMem = true;
	
	enable_ProcName = true;

	/* IO logger buffer pool 10MB */
	io_logger_bufsize = 2*1024*1024;
	io_logger_mem_pool = vmalloc(io_logger_bufsize);

	enable_IOLogger = true;
#else
	enable_inMem = false;	
	enable_ProcName = false;
	enable_IOLogger = false;
	io_logger_mem_pool = NULL;
	io_logger_bufsize = 0;
#endif
	
        // Add for File system directory file check

	mutex_init(&logger_lock);
	ILog_MSG("io_logger_init success\n");
	return ret;
}

static void __exit io_logger_exit(void)
{

  if(NULL != io_logger_mem_pool)
  {
    vfree(io_logger_mem_pool);
    io_logger_mem_pool = NULL;
  }
}

module_init(io_logger_init);
module_exit(io_logger_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("IO Logger driver");
