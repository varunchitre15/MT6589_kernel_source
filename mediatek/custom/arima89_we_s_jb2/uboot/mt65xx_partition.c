/*
 * (C) Copyright 2008
 * MediaTek <www.mediatek.com>
 * Infinity Chen <infinity.chen@mediatek.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <exports.h>
#include <asm/errno.h>
#include "config.h"
#include "mt65xx_partition.h"
#include "pmt.h"

/* BUG BUG: temp */
#define DBGMSG(...)
#define ERRMSG(...)
#ifdef MTK_EMMC_SUPPORT
#define PMT 0
#else
#define PMT 1
#endif

static part_dev_t *mt6575_part_dev;
extern void part_init_pmt(unsigned long totalblks,part_dev_t *dev);

//static 
part_t mt6575_parts[] = {
#ifdef CFG_USE_MBL_PARTITION
	{
	    .name   = PART_PRELOADER,
	    .blknum = PART_BLKS_PRELOADER,
	    .flags  = PART_FLAG_NONE,  
	    .startblk = 0x0,	      
	},
#if 0
	{
		.name   = PART_DSP_DL,
		.blknum = PART_BLKS_DSP_BL,
		.flags  = PART_FLAG_NONE,       
	},
#endif
#endif
#ifdef CFG_USE_MBR_PARTITION
	{
		.name   = PART_MBR,
		.blknum = PART_BLKS_MBR,
		.flags  = PART_FLAG_NONE,       
	},
#endif

#ifdef CFG_USE_EBR1_PARTITION
	{
		.name   = PART_EBR1,
		.blknum = PART_BLKS_EBR1,
		.flags  = PART_FLAG_NONE,       
	},
#endif

#ifdef CFG_USE_PMT_PARTITION
	{
		.name   = PART_PMT,
		.blknum = PART_BLKS_PMT,
		.flags  = PART_FLAG_NONE,       
	},
#endif

#ifdef CFG_USE_NVRAM_PARTITION
    {
        .name   = PART_NVRAM,
        .blknum = PART_BLKS_NVRAM,
        .flags  = PART_FLAG_NONE,
    },
#endif

#ifdef CFG_USE_SECURE_PARTITION
    {
        .name     = PART_SECURE,
        .blknum   = PART_BLKS_SECURE,
        .flags    = PART_FLAG_NONE,
    },
#endif

#ifdef CFG_USE_UBOOT_PARTITION
    {
        .name     = PART_UBOOT,
        .blknum   = PART_BLKS_UBOOT,
        .flags    = PART_FLAG_NONE,
    },
#endif
  
//-------------------------------------------
//-------------------------------------------
// use Android Boot Image instead of kernel and rootfs
#ifdef CFG_USE_BOOTIMG_PARTITION
    {
        .name   = PART_BOOTIMG,
        .blknum = PART_BLKS_BOOTIMIG,
        .flags  = PART_FLAG_NONE,
    },
#endif

//-------------------------------------------
//-------------------------------------------

#ifdef CFG_USE_RECOVERY_PARTITION
    {
        .name   = PART_RECOVERY,
        .blknum = PART_BLKS_RECOVERY,
        .flags  = PART_FLAG_NONE,
    },
#endif

#ifdef CFG_USE_SECSTATIC_PARTITION
    {
        .name   = PART_SECSTATIC,
        .blknum = PART_BLKS_SECSTATIC,
        .flags  = PART_FLAG_NONE,
    },
#endif

#ifdef CFG_USE_MISC_PARTITION
    {
        .name   = PART_MISC,
        .blknum = PART_BLKS_MISC,
        .flags  = PART_FLAG_NONE,
    },
#endif
#ifdef CFG_USE_LOGO_PARTITION
    {
        .name   = PART_LOGO,
        .blknum = PART_BLKS_LOGO,
        .flags  = PART_FLAG_NONE,
    },
#endif
#ifdef CFG_USE_EXPDB_PARTITION
    {
        .name   = PART_APANIC,
        .blknum = PART_BLKS_APANIC,
        .flags  = PART_FLAG_NONE,
    },
#endif

#ifdef CFG_USE_EBR2_PARTITION
	{
		.name   = PART_EBR2,
		.blknum = PART_BLKS_EBR2,
		.flags  = PART_FLAG_NONE,       
	},
#endif
#ifdef CFG_USE_ANDROID_SYSIMG_PARTITION
    {
        .name   = PART_ANDSYSIMG,
        .blknum = PART_BLKS_SYSIMG,
        .flags  = PART_FLAG_NONE,
    },
#endif

#ifdef CFG_USE_CACHE_PARTITION
    {
        .name   = PART_CACHE,
        .blknum = PART_BLKS_CACHE,
        .flags  = PART_FLAG_NONE,
    },
#endif


#ifdef CFG_USE_USER_PARTITION
    {
        .name   = PART_USER,
        .blknum = PART_BLKS_USER,
        .flags  = PART_FLAG_NONE,
    },
#endif
    {
        .name   = NULL,
        .flags  = PART_FLAG_END,
    },
};

static uchar *mt6575_part_buf;

void mt6575_part_dump(void)
{
    part_t *part = &mt6575_parts[0];
    
    printf("\nPart Info from compiler.(1blk=%dB):\n", BLK_SIZE);
    printf("\nPart Info.(1blk=%dB):\n", BLK_SIZE);
    while (part->name) {
        printf("[0x%.8x-0x%.8x] (%.8ld blocks): \"%s\"\n", 
               part->startblk * BLK_SIZE, 
              (part->startblk + part->blknum) * BLK_SIZE - 1, 
			   part->blknum, part->name);
        part++;
    }
    printf("\n");
}

void mt6575_part_init(unsigned long totalblks)
{
    part_t *part = &mt6575_parts[0];
    unsigned long lastblk;

    mt6575_part_buf = (uchar*)malloc(BLK_SIZE * 2);
    
    if (!totalblks) return;

    /* updater the number of blks of first part. */
    if (totalblks <= part->blknum)
        part->blknum = totalblks;

    totalblks -= part->blknum;
    lastblk = part->startblk + part->blknum;

    while(totalblks) {
        part++;
        if (!part->name)
            break;

        if (part->flags & PART_FLAG_LEFT || totalblks <= part->blknum)
            part->blknum = totalblks;

        part->startblk = lastblk;
        totalblks -= part->blknum;
        lastblk = part->startblk + part->blknum;;
    }
}

#if PMT
part_t tempart;
pt_resident lastest_part[PART_MAX_COUNT];
#endif
part_t* mt6575_part_get_partition(char *name)
{
	int index=0;
	printf ("[%s] %s\n", __FUNCTION__, name);
    part_t *part = &mt6575_parts[0];
	
	while (part->name)
	{
		if (!strcmp (name, part->name))
		{
#if PMT
		tempart.name=part->name;
		//when download get partitin used new,orther wise used latest
		{
			tempart.startblk = BLK_NUM(lastest_part[index].offset);
			tempart.blknum=BLK_NUM(lastest_part[index].size);
		}
		tempart.flags=part->flags;
		printf ("[%s] %x\n", __FUNCTION__, tempart.startblk);
		return &tempart;
#endif
		return part;
		}
		index++;
		part++;
	}
    return NULL;
}

int mt6575_part_generic_read(part_dev_t *dev, ulong src, uchar *dst, int size)
{
    int dev_id = dev->id;
    uchar *buf = &mt6575_part_buf[0];
    block_dev_desc_t *blkdev = dev->blkdev;
	ulong end, part_start, part_end, part_len, aligned_start, aligned_end;
    ulong blknr, blkcnt;

	if (!blkdev) {
        ERRMSG("No block device registered\n");
        return -ENODEV;
	}

	if (size == 0) 
		return 0;

	end = src + size;
	
	part_start    = src &  (BLK_SIZE - 1);
	part_end      = end &  (BLK_SIZE - 1);
	aligned_start = src & ~(BLK_SIZE - 1);
	aligned_end   = end & ~(BLK_SIZE - 1);
 
	if (part_start) {
	    blknr = aligned_start >> BLK_BITS;	
		part_len = BLK_SIZE - part_start;
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf)) != 1)
			return -EIO;
		memcpy(dst, buf + part_start, part_len);
		dst += part_len;
        src += part_len;
    }
    
	aligned_start = src & ~(BLK_SIZE - 1);
	blknr  = aligned_start >> BLK_BITS;
	blkcnt = (aligned_end - aligned_start) >> BLK_BITS;
	
	if ((blkdev->block_read(dev_id, blknr, blkcnt, (unsigned long *)(dst))) != blkcnt)
		return -EIO;

    src += (blkcnt << BLK_BITS);
    dst += (blkcnt << BLK_BITS);

	if (part_end && src < end) {
	    blknr = aligned_end >> BLK_BITS;	
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf)) != 1)
			return -EIO;
		memcpy(dst, buf, part_end);
	}
	return size;
}

static int mt6575_part_generic_write(part_dev_t *dev, uchar *src, ulong dst, int size)
{
    int dev_id = dev->id;
    uchar *buf = &mt6575_part_buf[0];
    block_dev_desc_t *blkdev = dev->blkdev;
	ulong end, part_start, part_end, part_len, aligned_start, aligned_end;
    ulong blknr, blkcnt;

	if (!blkdev) {
        ERRMSG("No block device registered\n");
        return -ENODEV;
	}

	if (size == 0) 
		return 0;

	end = dst + size;
	
	part_start    = dst &  (BLK_SIZE - 1);
	part_end      = end &  (BLK_SIZE - 1);
	aligned_start = dst & ~(BLK_SIZE - 1);
	aligned_end   = end & ~(BLK_SIZE - 1);
 
	if (part_start) {
	    blknr = aligned_start >> BLK_BITS;	
		part_len = BLK_SIZE - part_start;
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf)) != 1)
			return -EIO;
		memcpy(buf + part_start, src, part_len);
    	if ((blkdev->block_write(dev_id, blknr, 1, (unsigned long*)buf)) != 1)
        	return -EIO;
		dst += part_len;
        src += part_len;
    }
    
	aligned_start = dst & ~(BLK_SIZE - 1);
	blknr  = aligned_start >> BLK_BITS;
	blkcnt = (aligned_end - aligned_start) >> BLK_BITS;

	if ((blkdev->block_write(dev_id, blknr, blkcnt, (unsigned long *)(dst))) != blkcnt)
		return -EIO;
    
    src += (blkcnt << BLK_BITS);
    dst += (blkcnt << BLK_BITS);

	if (part_end && dst < end) {
	    blknr = aligned_end >> BLK_BITS;	
		if ((blkdev->block_read(dev_id, blknr, 1, (unsigned long*)buf)) != 1) {
			return -EIO;
		}
		memcpy(buf, src, part_end);
    	if ((blkdev->block_write(dev_id, blknr, 1, (unsigned long*)buf)) != 1) {
            return -EIO;
    	}
	}
	return size;
}

int mt6575_part_register_device(part_dev_t *dev)
{
    if (!mt6575_part_dev) {
        if (!dev->read)
            dev->read = mt6575_part_generic_read;
        if (!dev->write)
            dev->write = mt6575_part_generic_write;
        mt6575_part_dev = dev;

        #ifdef CFG_NAND_BOOT
        if (dev->id == CFG_MT6575_NAND_ID)
	{
#ifdef PMT
		// printf("call mt6575_part_init_pmt");
		part_init_pmt(BLK_NUM(1 * GB),dev);
#else
            mt6575_part_init(BLK_NUM(1 * GB));
#endif
	}
        else
        {
#ifdef PMT
		// printf("call mt6516_part_init_pmt");
		part_init_pmt(BLK_NUM(1 * GB),dev);
#else
            mt6575_part_init((unsigned long)dev->blkdev->lba);
#endif /* CFG_NAND_BOOT */
        }
	#endif /* CFG_NAND_BOOT */
    }
    return 0;
}

part_dev_t *mt6575_part_get_device(void)
{
    if (mt6575_part_dev && !mt6575_part_dev->init && mt6575_part_dev->init_dev) 
    {
        mt6575_part_dev->init_dev(mt6575_part_dev->id);
        mt6575_part_dev->init = 1;
    }
    return mt6575_part_dev;
}

