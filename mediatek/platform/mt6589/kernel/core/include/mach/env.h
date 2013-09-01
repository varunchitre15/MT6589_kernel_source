#ifndef __ENV_H__
#define __ENV_H__
#include "partition_define.h"
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>  /*proc*/
#include <linux/ioctl.h> /*ioctl*/
#include <linux/module.h>

#define CFG_ENV_SIZE 0x4000 //(16KB)
#define CFG_ENV_OFFSET 0x20000  //(128KB)

#define CFG_ENV_DATA_SIZE (CFG_ENV_SIZE-sizeof(g_env.checksum)-sizeof(g_env.sig)-sizeof(g_env.sig_1))
#define CFG_ENV_DATA_OFFSET (sizeof(g_env.sig))
#define CFG_ENV_SIG_1_OFFSET (CFG_ENV_SIZE - sizeof(g_env.checksum)-sizeof(g_env.sig_1))
#define CFG_ENV_CHECKSUM_OFFSET (CFG_ENV_SIZE - sizeof(g_env.checksum))

#define ENV_PART PART_MISC

#define ENV_SIG "ENV_v1"

#define DATA_FREE_SIZE_TH_DEFAULT (50*1024*1024)

#ifdef MTK_SHARED_SDCARD

#define LIMIT_SDCARD_SIZE

#endif

typedef struct env_struct
{
	char sig[8]; // "ENV_v1"
	char *env_data;
	char sig_1[8];  //"ENV_v1"
	int checksum; // checksum for env_data
}env_t;

#define ENV_MAGIC	 'e'
#define ENV_READ		_IOW(ENV_MAGIC, 1, int)
#define ENV_WRITE 		_IOW(ENV_MAGIC, 2, int)

struct env_ioctl
{
	char *name;
	int name_len;
	char *value;
	int value_len;	
};
extern void env_init(loff_t env_part_addr);
#ifdef MTK_EMMC_SUPPORT
extern int eMMC_rw_x(loff_t addr,u32  *buffer, int host_num, int iswrite,u32 totalsize, int transtype, Region part);
#endif
extern int set_env(char *name,char *value);
extern char *get_env(char *name);
extern long long data_free_size_th;
EXPORT_SYMBOL(data_free_size_th);
#endif
