#if !defined(__AEE_IPANIC_H__)
#define __AEE_IPANIC_H__

#include <linux/autoconf.h>
#include <linux/kallsyms.h>
#include <linux/xlog.h>
//#include "partition_define.h"
#include "../../../../../../kernel/drivers/staging/android/logger.h"

#define AEE_IPANIC_PLABEL "expdb"

#define IPANIC_MODULE_TAG "KERNEL-PANIC"

#define IPANIC_LOG_TAG "aee/ipanic"

#define AEE_IPANIC_MAGIC 0xaee0dead
#define AEE_IPANIC_PHDR_VERSION   0x04
#define AEE_IPANIC_DATALENGTH_MAX (512 * 1024)

struct ipanic_header {
	/* The magic/version field cannot be moved or resize */
	u32 magic;
	u32 version;

	u32 oops_header_offset;
	u32 oops_header_length;

	u32 oops_detail_offset;
	u32 oops_detail_length;

	u32 console_offset;
	u32 console_length;

	u32 android_main_offset;
	u32 android_main_length;
	
	u32 android_event_offset;
	u32 android_event_length;
	
	u32 android_radio_offset;
	u32 android_radio_length;
	
	u32 android_system_offset;
	u32 android_system_length;

	u32 userspace_info_offset;
	u32 userspace_info_length;

	u32 mmprofile_offset;
	u32 mmprofile_length;
};

#define IPANIC_OOPS_HEADER_PROCESS_NAME_LENGTH 256
#define IPANIC_OOPS_HEADER_BACKTRACE_LENGTH 3840
#define IPANIC_OOPS_MMPROFILE_LENGTH_LIMIT 3000000

struct ipanic_oops_header 
{
	char process_path[IPANIC_OOPS_HEADER_PROCESS_NAME_LENGTH];
	char backtrace[IPANIC_OOPS_HEADER_BACKTRACE_LENGTH];
};

struct ipanic_ops {

	struct aee_oops *(*oops_copy)(void);
  
	void (*oops_free)(struct aee_oops *oops, int erase);
};

void register_ipanic_ops(struct ipanic_ops *op);

extern int log_buf_copy2(char *dest, int dest_len, int log_copy_start, int log_copy_end);

struct aee_oops *ipanic_oops_copy(void);

void ipanic_oops_free(struct aee_oops *oops, int erase);

extern unsigned log_start;

extern unsigned log_end;

extern unsigned ipanic_detail_start;

extern unsigned ipanic_detail_end;

extern struct ipanic_oops_header oops_header;

/*
 * Check if valid header is legitimate
 * return
 *  0: contain good panic data 
 *  1: no panic data
 *  2: contain bad panic data
 */
int ipanic_header_check(const struct ipanic_header *hdr);

void ipanic_header_dump(const struct ipanic_header *header);

void ipanic_block_scramble(u8 *buf, int buflen);

void ipanic_oops_start(void);

void ipanic_oops_end(void);

/* User space process support functions */

#define MAX_NATIVEINFO  32*1024
#define MAX_NATIVEHEAP  2048

extern char NativeInfo[MAX_NATIVEINFO]; //check that 32k is enought??

extern unsigned long User_Stack[MAX_NATIVEHEAP];//8K Heap

int DumpNativeInfo(void);

/* External ipanic support functions */

//int card_dump_func_read(unsigned char* buf, unsigned int len, unsigned int offset, unsigned int dev);

//int card_dump_func_write(unsigned char* buf, unsigned int len, unsigned int offset, unsigned int dev);

int panic_dump_android_log(char *buf, size_t size, int type);

/*
*  to check expdb parition size bigger than the buffer size.
*  layout
     -------------------------------------------------------------------------------------------
     | oops_header  | ipanic_detail | kernel buffer | usespace info| android buf|ipanic_header |
*/

#define __LOG_BUF_LEN	(1 << CONFIG_LOG_BUF_SHIFT)
#define BLOCK_DEVICE_SECTOR_ALIGN     512
/*
* Since ipanic_detail and usersapce info size is not known
* until run time, we do a guess here
*/
#define IPANIC_DETAIL_USERSPACE_SIZE    (100 * 1024)

#define  PANIC_INFO_SIZE        \
sizeof(struct ipanic_oops_header) + __LOG_BUF_LEN +  \
        __MAIN_BUF_SIZE +  IPANIC_DETAIL_USERSPACE_SIZE + \
            __RADIO_BUF_SIZE + __SYSTEM_BUF_SIZE +  \
            sizeof(struct ipanic_header) + (6 * BLOCK_DEVICE_SECTOR_ALIGN)

/*
 * the trick is if PART_SIZE_EXPDB is less than PANIC_INFO_SIZE, 
 * compiler error will occure
 */
//typedef unsigned int ipanic_check_expdb[PART_SIZE_EXPDB - PANIC_INFO_SIZE];

/* for WDT timeout case : dump timer/schedule/irq/softirq etc... debug information */
extern void aee_wdt_dump_info(void);

#endif
