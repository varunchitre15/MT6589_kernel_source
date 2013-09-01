

#ifndef __MT_PARTITION_H__
#define __MT_PARTITION_H__


#include <platform/part.h>
#include "partition_define.h"
#include <platform/mt_typedefs.h>

#define NAND_WRITE_SIZE	 2048

#define BIMG_HEADER_SZ				(0x800)
#define MKIMG_HEADER_SZ				(0x200)

#define BLK_BITS         (9)
#define BLK_SIZE         (1 << BLK_BITS)
#ifdef MTK_EMMC_SUPPORT
#define BLK_NUM(size)    ((unsigned long long)(size) / BLK_SIZE)
#else
#define BLK_NUM(size)    ((unsigned long)(size) / BLK_SIZE)
#endif
#define PART_KERNEL     "KERNEL"
#define PART_ROOTFS     "ROOTFS"

#define PART_BLKS_PRELOADER   BLK_NUM(PART_SIZE_PRELOADER)
#define PART_BLKS_MBR   BLK_NUM(PART_SIZE_MBR)
#define PART_BLKS_EBR1   BLK_NUM(PART_SIZE_EBR1)
#define PART_BLKS_PMT   BLK_NUM(PART_SIZE_PMT)
#define PART_BLKS_PRO_INFO   BLK_NUM(PART_SIZE_PRO_INFO)
#define PART_BLKS_NVRAM   BLK_NUM(PART_SIZE_NVRAM)
#define PART_BLKS_PROTECT_F   BLK_NUM(PART_SIZE_PROTECT_F)
#define PART_BLKS_PROTECT_S   BLK_NUM(PART_SIZE_PROTECT_S)
#define PART_BLKS_SECURE   BLK_NUM(PART_SIZE_SECCFG)
#define PART_BLKS_S1SBL   BLK_NUM(PART_SIZE_S1SBL)
#define PART_BLKS_TA   BLK_NUM(PART_SIZE_TA)
#define PART_BLKS_LTALABEL   BLK_NUM(PART_SIZE_LTALABEL)
#define PART_BLKS_UBOOT   BLK_NUM(PART_SIZE_UBOOT)
#define PART_BLKS_BOOTIMG   BLK_NUM(PART_SIZE_BOOTIMG)
#define PART_BLKS_RECOVERY   BLK_NUM(PART_SIZE_RECOVERY)
#define PART_BLKS_SECSTATIC   BLK_NUM(PART_SIZE_SEC_RO)
#define PART_BLKS_MISC   BLK_NUM(PART_SIZE_MISC)
#define PART_BLKS_LOGO   BLK_NUM(PART_SIZE_LOGO)
#define PART_BLKS_EBR2   BLK_NUM(PART_SIZE_EBR2)
#define PART_BLKS_APANIC   BLK_NUM(PART_SIZE_EXPDB)
#define PART_BLKS_NRSTDATA   BLK_NUM(PART_SIZE_NRSTDATA)
#define PART_BLKS_ANDSYSIMG   BLK_NUM(PART_SIZE_ANDROID)
#define PART_BLKS_CACHE   BLK_NUM(PART_SIZE_CACHE)
#define PART_BLKS_USER   BLK_NUM(PART_SIZE_USRDATA)
#define PART_BLKS_FAT   BLK_NUM(PART_SIZE_FAT)


#define PMT_END_NAME "FAT"

struct NAND_CMD{
	u32	u4ColAddr;
	u32 u4RowAddr;
	u32 u4OOBRowAddr;
	u8	au1OOB[64];
	u8*	pDataBuf;
};

typedef union {
    struct {    
        unsigned int magic;        /* partition magic */
        unsigned int dsize;        /* partition data size */
        char         name[32];     /* partition name */
    } info;
    unsigned char data[BLK_SIZE];
} part_hdr_t;

typedef struct {
    unsigned char *name;        /* partition name */
    unsigned long  blknum;      /* partition blks */
    unsigned long  flags;       /* partition flags */
    unsigned long  startblk;    /* partition start blk */
} part_t;

struct part_name_map{
	char fb_name[32]; 	/*partition name used by fastboot*/	
	char r_name[32];  	/*real partition name*/
	char *partition_type;	/*partition_type*/
	int partition_idx;	/*partition index*/
	int is_support_erase;	/*partition support erase in fastboot*/
	int is_support_dl;	/*partition support download in fastboot*/
};

typedef struct part_dev part_dev_t;

struct part_dev {
    int init;
    int id;
    block_dev_desc_t *blkdev;
    int (*init_dev) (int id);
#ifdef MTK_EMMC_SUPPORT
	int (*read)  (part_dev_t *dev, u64 src, uchar *dst, int size);
    int (*write) (part_dev_t *dev, uchar *src, u64 dst, int size);
#else
    int (*read)  (part_dev_t *dev, ulong src, uchar *dst, int size);
    int (*write) (part_dev_t *dev, uchar *src, ulong dst, int size);
#endif
};
extern struct part_name_map g_part_name_map[];
extern int mt_part_register_device(part_dev_t *dev);
extern part_t* mt_part_get_partition(char *name);
extern part_dev_t* mt_part_get_device(void);
extern void mt_part_init(unsigned long totalblks);
extern void mt_part_dump(void);
extern int partition_get_index(const char * name);
extern u64 partition_get_offset(int index);
extern u64 partition_get_size(int index);
extern int partition_get_type(int index, char **p_type);
extern int partition_get_name(int index, char **p_name);
extern int is_support_erase(int index);
extern int is_support_flash(int index);
extern u64 emmc_write(u64 offset, void *data, u64 size);
extern u64 emmc_read(u64 offset, void *data, u64 size);
extern int emmc_erase(u64 offset, u64 size);
extern unsigned long partition_reserve_size(void);
#endif /* __MT_PARTITION_H__ */

