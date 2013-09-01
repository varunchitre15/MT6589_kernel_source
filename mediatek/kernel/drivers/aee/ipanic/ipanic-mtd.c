/* drivers/misc/apanic.c
 *
 * Copyright (C) 2009 Google, Inc.
 * Author: San Mehat <san@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 */

#ifndef MTK_EMMC_SUPPORT

#include <linux/autoconf.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/wakelock.h>
#include <linux/uaccess.h>
#include <linux/mtd/mtd.h>
#include <linux/notifier.h>
#include <linux/mtd/mtd.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/preempt.h>
#include <linux/aee.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include "ipanic.h"

/*
* actual number of blocks on MTD NAND. 
* <= entries of blk_offset
*/
static int IPANIC_OOPS_BLOCK_COUNT = 0;
struct mtd_ipanic_data {
	struct mtd_info	*mtd;
	struct ipanic_header curr;
	void *bounce;
	u32 blk_offset[512];

	struct proc_dir_entry *oops;
};

static struct mtd_ipanic_data mtd_drv_ctx;

static int mtd_ipanic_block_scan(struct mtd_ipanic_data *ctx) 
{
	int index = 0, offset;
	
    /*calcuate total number of non-bad blocks on NAND device, 
    *and record it's offset
    */
	for (offset = 0; offset < ctx->mtd->size; offset += ctx->mtd->writesize) {
		
        if (!ctx->mtd->_block_isbad(ctx->mtd, offset)) {

            /*index can't surpass array size*/
            if(index >= (sizeof(ctx->blk_offset)/ sizeof(u32))) {
                break;
            }

			ctx->blk_offset[index++] = offset;
		}
	}

    IPANIC_OOPS_BLOCK_COUNT = index;

#if 0
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: blocks: ");
	for (index = 0; index < IPANIC_OOPS_BLOCK_COUNT; index++) {
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, " %d", ctx->blk_offset[index]);
	}
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "\n");
#endif
	return 1;
}

static int mtd_ipanic_block_read_single(struct mtd_ipanic_data *ctx, loff_t offset)
{
	int rc, len;
	int index = offset >> ctx->mtd->writesize_shift;

	if ((index < 0) || (index >= IPANIC_OOPS_BLOCK_COUNT)) {
		return -EINVAL;
	}

	rc = ctx->mtd->_read(ctx->mtd, ctx->blk_offset[index], ctx->mtd->writesize, &len, ctx->bounce);
	ipanic_block_scramble(ctx->bounce, ctx->mtd->writesize);
#if 0
	if (rc == -EBADMSG) {
		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "Check sum error (ignore)\n");
		rc = 0;
	}
#endif
	if (rc == -EUCLEAN) {
		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "ECC Check sum error corrected %lld\n", offset);
		rc = 0;
	}
	if ((rc == 0) && (len != ctx->mtd->writesize)) {
		xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG, "aee-ipanic: read size mismatch %d\n", len);
		return -EINVAL;
	}
	return rc;
}

static int mtd_ipanic_block_write(struct mtd_ipanic_data *ctx, loff_t to, int bounce_len)
{
	int rc;
	size_t wlen;
	int panic = in_interrupt() | in_atomic();
	int index = to >> ctx->mtd->writesize_shift;

	if ((index < 0) || (index >= IPANIC_OOPS_BLOCK_COUNT)) {
		return -EINVAL;
	}

	if (bounce_len > ctx->mtd->writesize) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "aee-ipanic(%s) len too large\n", __func__);
		return -EINVAL;
	}
	if (panic && !ctx->mtd->_panic_write) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: No panic_write available\n", __func__);
		return 0;
	} else if (!panic && !ctx->mtd->_write) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: No write available\n", __func__);
		return 0;
	}

	if (bounce_len < ctx->mtd->writesize)
		memset(ctx->bounce + bounce_len, 0, ctx->mtd->writesize - bounce_len);
	ipanic_block_scramble(ctx->bounce, ctx->mtd->writesize);

	if (panic)
		rc = ctx->mtd->_panic_write(ctx->mtd, ctx->blk_offset[index], ctx->mtd->writesize, &wlen, ctx->bounce);
	else
		rc = ctx->mtd->_write(ctx->mtd, ctx->blk_offset[index], ctx->mtd->writesize, &wlen, ctx->bounce);

	if (rc) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
		       "%s: Error writing data to flash (%d)\n",
		       __func__, rc);
		return rc;
	}

	return wlen;
}

static int mtd_ipanic_block_read(struct mtd_ipanic_data *ctx, off_t file_offset, int file_length, void *buf)
{
#if 0
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "%s: ctx:%p file_offset:%d file_length:%lu\n", __func__, ctx, file_offset, file_length);
#endif
	while (file_length > 0) {
		unsigned int page_no;
		off_t page_offset;
		int rc;
		size_t count = file_length;

		/* We only support reading a maximum of a flash page */
		if (count > ctx->mtd->writesize)
			count = ctx->mtd->writesize;
		page_no = file_offset / ctx->mtd->writesize;
		page_offset = file_offset % ctx->mtd->writesize;

		rc = mtd_ipanic_block_read_single(ctx, page_no * ctx->mtd->writesize);
		if (rc < 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "aee-ipanic(%s): mtd read error page_no(%d) error(%d)\n", __func__, page_no, rc);
			goto error_return;
		}
		if (page_offset)
			count -= page_offset;
		memcpy(buf, ctx->bounce + page_offset, count);

		file_length -= count;
		buf += count;
		file_offset += count;
	}
	return 0;
error_return:
	return -1;
}

static void mtd_ipanic_block_erase_callback(struct erase_info *done)
{
	wait_queue_head_t *wait_q = (wait_queue_head_t *) done->priv;
	wake_up(wait_q);
}

static void mtd_ipanic_block_erase(void)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	struct erase_info erase;
	DECLARE_WAITQUEUE(wait, current);
	wait_queue_head_t wait_q;
	int rc, i;

	init_waitqueue_head(&wait_q);
	erase.mtd = ctx->mtd;
	erase.callback = mtd_ipanic_block_erase_callback;
	erase.len = ctx->mtd->erasesize;
	erase.priv = (u_long)&wait_q;
	for (i = 0; i < ctx->mtd->size; i += ctx->mtd->erasesize) {
		erase.addr = i;
		set_current_state(TASK_INTERRUPTIBLE);
		add_wait_queue(&wait_q, &wait);

		rc = ctx->mtd->_block_isbad(ctx->mtd, erase.addr);
		if (rc < 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
			       "aee-ipanic: Bad block check "
			       "failed (%d)\n", rc);
			goto out;
		}
		if (rc) {
			xlog_printk(ANDROID_LOG_WARN, IPANIC_LOG_TAG,
			       "aee-ipanic: Skipping erase of bad "
			       "block @%llx\n", erase.addr);
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			continue;
		}

		rc = ctx->mtd->_erase(ctx->mtd, &erase);
		if (rc) {
			set_current_state(TASK_RUNNING);
			remove_wait_queue(&wait_q, &wait);
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
			       "aee-ipanic: Erase of 0x%llx, 0x%llx failed\n",
			       (unsigned long long) erase.addr,
			       (unsigned long long) erase.len);
			if (rc == -EIO) {
				if (ctx->mtd->_block_markbad(ctx->mtd,
							    erase.addr)) {
					xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
					       "aee-ipanic: Err marking blk bad\n");
					goto out;
				}
				xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG,
				       "aee-ipanic: Marked a bad block"
				       " @%llx\n", erase.addr);
				continue;
			}
			goto out;
		}
		schedule();
		remove_wait_queue(&wait_q, &wait);
	}
	xlog_printk(ANDROID_LOG_DEBUG, IPANIC_LOG_TAG, "aee-ipanic: %s partition erased\n",
	       AEE_IPANIC_PLABEL);
out:
	return;
}

static int mtd_ipanic_proc_oops(char *page, char **start,
				off_t off, int count,
				int *eof, void *data)
{
	int len;
	struct aee_oops *oops = ipanic_oops_copy();
	len = sprintf(page, "aee-ipanic Oops\n");
	if (oops) {
		len += sprintf(page + len, "Module: %s\nProcess\n%s\nBacktrace\n%s", oops->module, oops->process_path, oops->backtrace);
		ipanic_oops_free(oops, 0);
	}
	else {
		len += sprintf(page + len, "No available\n");
	}
	return len;
}

static void mtd_panic_notify_add(struct mtd_info *mtd)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	struct ipanic_header *hdr = ctx->bounce;
	int rc;

	if (strcmp(mtd->name, AEE_IPANIC_PLABEL))
		return;

	ctx->mtd = mtd;

	if (!mtd_ipanic_block_scan(ctx))
		goto out_err;

	rc = mtd_ipanic_block_read_single(ctx, 0);
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "aee-ipanic: Error reading block 0 (%d)\n", rc);
		mtd_ipanic_block_erase();
		goto out_err;
	}

	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: Bound to mtd partition '%s', write size(%d), write size shift(%d)\n", mtd->name, mtd->writesize, mtd->writesize_shift);
	switch (ipanic_header_check(hdr)) {
	case 0:
		break;
	case 1:
		return;
	case 2:
		mtd_ipanic_block_erase();
		return;
	}

	memcpy(&ctx->curr, hdr, sizeof(struct ipanic_header));
	ipanic_header_dump(hdr);

	ctx->oops = create_proc_read_entry("aee_ipanic_oops", 
					   0444, NULL, 
					   mtd_ipanic_proc_oops,
					   NULL);
	if (ctx->oops == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, " %s: failed to create proc file apanic_oops\n", __func__);
	}

	/*it will free the ctx->curr assigne during bootup. then 
     open the ke device will return NULL. so lose db
    */
    //mtd_ipanic_oops_dump();
	
    return;

out_err:
	ctx->mtd = NULL;
}

static void mtd_panic_notify_remove(struct mtd_info *mtd)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	if (mtd == ctx->mtd) {
		ctx->mtd = NULL;
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: Unbound from %s\n", mtd->name);
	}
}

static struct mtd_notifier mtd_panic_notifier = {
	.add	= mtd_panic_notify_add,
	.remove	= mtd_panic_notify_remove,
};

static int in_panic = 0;

/*
 * Writes the contents of the console to the specified offset in flash.
 * Returns number of bytes written
 */
static int ipanic_write_log_buf(struct mtd_info *mtd, unsigned int off, int log_copy_start, int log_copy_end)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	int saved_oip;
	int rc, rc2;
	unsigned int last_chunk = 0, copy_count = 0;

	while (!last_chunk) {
		saved_oip = oops_in_progress;
		oops_in_progress = 1;
		rc = log_buf_copy2(ctx->bounce, mtd->writesize, log_copy_start, log_copy_end);
		BUG_ON(rc < 0);
		log_copy_start += rc;
		copy_count += rc;
		if (rc != mtd->writesize)
			last_chunk = rc;

		oops_in_progress = saved_oip;
		if (rc <= 0)
			break;

		rc2 = mtd_ipanic_block_write(ctx, off, rc);
		if (rc2 <= 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
			       "aee-ipanic: Flash write failed (%d)\n", rc2);
			return rc2;
		}
		off += rc2;
	}
	return copy_count;
}

static int ipanic_write_android_buf(struct mtd_info *mtd, unsigned int off, int type)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	int saved_oip;
	int rc, rc2;
	unsigned int last_chunk = 0, copy_count = 0;

	while (!last_chunk) {
		saved_oip = oops_in_progress;
		oops_in_progress = 1;
		rc = panic_dump_android_log(ctx->bounce, mtd->writesize, type);
		BUG_ON(rc < 0);
		copy_count += rc;
		if (rc == 0)
			last_chunk = rc;

		oops_in_progress = saved_oip;
		if (rc <= 0)
			break;

		rc2 = mtd_ipanic_block_write(ctx, off, rc);
		if (rc2 <= 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
			       "aee-ipanic: Flash write failed (%d)\n", rc2);
			return rc2;
		}
		off += rc2;
	}
	//xlog_printk(ANDROID_LOG_DEBUG, IPANIC_LOG_TAG, "dump droid log type %d, count %d\n", type, copy_count);
	return copy_count;
}

/*
 * Writes the contents of the console to the specified offset in flash.
 * Returns number of bytes written
 */
static int mtd_ipanic_write_console(struct mtd_info *mtd, unsigned int off)
{
    #define __LOG_BUF_LEN	(1 << CONFIG_LOG_BUF_SHIFT)
    if (log_end > __LOG_BUF_LEN)
	    return ipanic_write_log_buf(mtd, off, log_end-__LOG_BUF_LEN, log_end);
	else
	    return ipanic_write_log_buf(mtd, off, 0, log_end);
}

static int mtd_ipanic_write_oops_header(struct mtd_info *mtd, unsigned int off)
{
	int wlen = 0, rc;
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	u8 *raw_oops_header = (u8 *)&oops_header;
	while (wlen < sizeof(struct ipanic_oops_header)) {
		int writesize = sizeof(struct ipanic_oops_header) - wlen;
		if (writesize > mtd->writesize)
			writesize = mtd->writesize;

		memcpy(ctx->bounce, raw_oops_header + wlen, writesize);
		rc = mtd_ipanic_block_write(ctx, off + wlen, writesize);
		if (rc < 0) {
			return rc;
		}
		wlen += writesize;
	}
	return wlen;
}

static int ipanic_write_oops_detail(struct mtd_info *mtd, unsigned int off)
{
	return ipanic_write_log_buf(mtd, off, ipanic_detail_start, ipanic_detail_end);
}

static struct aee_oops *mtd_ipanic_oops_copy(void)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	struct aee_oops *oops;

	if ((ctx->curr.magic != AEE_IPANIC_MAGIC) || (ctx->curr.version != AEE_IPANIC_PHDR_VERSION)) {
		return NULL;
	}

	oops = aee_oops_create(AE_DEFECT_FATAL, AE_KE, IPANIC_MODULE_TAG);
	if (oops != NULL) {
		struct ipanic_oops_header *oops_header = kzalloc(sizeof(struct ipanic_oops_header), GFP_KERNEL);
		if (oops_header == NULL)
			goto error_return;

		if (mtd_ipanic_block_read(ctx, ctx->curr.oops_header_offset, ctx->curr.oops_header_length, oops_header) != 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read header failed\n", __FUNCTION__);
			kfree(oops_header);
			goto error_return;
		}

		aee_oops_set_process_path(oops, oops_header->process_path);
		aee_oops_set_backtrace(oops, oops_header->backtrace);
		kfree(oops_header);
        if(ctx->curr.oops_detail_length != 0)
        {
            oops->detail = kmalloc(ctx->curr.oops_detail_length, GFP_KERNEL);
            oops->detail_len = ctx->curr.oops_detail_length;
            if (oops->detail != NULL) {
                if (mtd_ipanic_block_read(ctx, ctx->curr.oops_detail_offset, ctx->curr.oops_detail_length, oops->detail) != 0) {
                    xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read detail failed\n", __FUNCTION__);
                    kfree(oops->detail);
                    goto error_return;
                }
            }
        }else {
           #define TMPDETAILSTR  "panic detail is empty"
            oops->detail = kstrdup(TMPDETAILSTR,GFP_KERNEL);
            oops->detail_len = sizeof TMPDETAILSTR; 
        }

        if(oops->detail == NULL)
        {
            xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at detail\n", __FUNCTION__);
            kfree(oops);
            return NULL;
        }

		oops->console = kmalloc(ctx->curr.console_length, GFP_KERNEL);
		oops->console_len = ctx->curr.console_length;
		if (oops->console != NULL) {
			if (mtd_ipanic_block_read(ctx, ctx->curr.console_offset, ctx->curr.console_length, oops->console) != 0) {
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read console failed\n", __FUNCTION__);
				kfree(oops->detail);
				goto error_return;
			}
		}
		else {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at detail\n", __FUNCTION__);
			kfree(oops);
			return NULL;
		}
		
		/* Android log */
		oops->android_main = kmalloc(ctx->curr.android_main_length, GFP_KERNEL);
		oops->android_main_len = ctx->curr.android_main_length;
		if (oops->android_main)	{
			if (mtd_ipanic_block_read(ctx, ctx->curr.android_main_offset, ctx->curr.android_main_length, oops->android_main) != 0) {
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read android_main failed\n", __FUNCTION__);
				kfree(oops->detail);
				kfree(oops->console);
				goto error_return;
			}
		}
		else {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at android_main\n", __FUNCTION__);
			aee_oops_free(oops);
			return NULL;
		}

		oops->android_radio = kmalloc(ctx->curr.android_radio_length, GFP_KERNEL);
		oops->android_radio_len = ctx->curr.android_radio_length;
		if (oops->android_radio) {
			if (mtd_ipanic_block_read(ctx, ctx->curr.android_radio_offset, ctx->curr.android_radio_length, oops->android_radio) != 0) {
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read android_radio failed\n", __FUNCTION__);
				kfree(oops->detail);
				kfree(oops->console);
				kfree(oops->android_main);
				goto error_return;
			}		    
		}
		else {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at android_radio\n", __FUNCTION__);
			aee_oops_free(oops);
			return NULL;
		}
		
		oops->android_system = kmalloc(ctx->curr.android_system_length, GFP_KERNEL);
		oops->android_system_len = ctx->curr.android_system_length;
		if (oops->android_system) {
			if (mtd_ipanic_block_read(ctx, ctx->curr.android_system_offset, ctx->curr.android_system_length, oops->android_system) != 0) {
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read android_system failed\n", __FUNCTION__);
				kfree(oops->detail);
				kfree(oops->console);
				kfree(oops->android_main);
				kfree(oops->android_radio);
				goto error_return;
			}		    
		}
		else {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at android_system\n", __FUNCTION__);
			aee_oops_free(oops);
			return NULL;
		}

#if 0		
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "android log length, 0x%x, 0x%x, 0x%x, 0x%x\n",
			    oops->android_main_len,oops->android_event_len,oops->android_radio_len,oops->android_system_len);
#endif
		/* Process dump */
		oops->userspace_info = kmalloc(ctx->curr.userspace_info_length, GFP_KERNEL);
		oops->userspace_info_len = ctx->curr.userspace_info_length;
		if (oops->userspace_info) {
			if (mtd_ipanic_block_read(ctx, ctx->curr.userspace_info_offset, ctx->curr.userspace_info_length, oops->userspace_info) != 0) {
				xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: mtd read usrespace info failed\n", __FUNCTION__);
				kfree(oops->detail);
				kfree(oops->console);
				kfree(oops->android_main);
				kfree(oops->android_radio);
				kfree(oops->userspace_info);
				goto error_return;
			}		    
		}
		else {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at userspace info failed\n", __FUNCTION__);
			aee_oops_free(oops);
			return NULL;
		}
		
		xlog_printk(ANDROID_LOG_DEBUG, IPANIC_LOG_TAG, "ipanic_oops_copy return OK\n");
		return oops;
	}
	else {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "%s: kmalloc failed at header\n", __FUNCTION__);
		return NULL;
	}
error_return:
	kfree(oops);
	memset(&ctx->curr, 0, sizeof(struct ipanic_header));
	mtd_ipanic_block_erase();
	return NULL;
}

/*erase is no longer used. erase from user space*/
static void mtd_ipanic_oops_free(struct aee_oops *oops, int erase)
{
	if (oops) {
		struct mtd_ipanic_data *ctx = &mtd_drv_ctx;

		aee_oops_free(oops);
	
       /*After this, open ke device returns null*/
       memset(&ctx->curr, 0, sizeof(struct ipanic_header));
		
	}
}

static int panic_dump_user_info(char *buf, size_t size, unsigned copy_count)
{
	size_t distance = 0;
	size_t realsize = 0;
	
	distance = (strlen(NativeInfo) - copy_count);
	if(distance > size)
		realsize = size;
	else
		realsize = distance;
	memcpy(buf, NativeInfo + copy_count, realsize);
	return realsize;
}

static int ipanic_write_userspace(struct mtd_ipanic_data *ctx, unsigned int off)
{
	int saved_oip, rc, rc2;
	unsigned int copy_count = 0;

	memset(ctx->bounce, 0, PAGE_SIZE);
	DumpNativeInfo();

	while (1) {
		saved_oip = oops_in_progress;
		oops_in_progress = 1;
		rc = panic_dump_user_info(ctx->bounce, ctx->mtd->writesize, copy_count);
		oops_in_progress = saved_oip;

		if (rc <= 0)
			break;

		copy_count += rc;
		rc2 = mtd_ipanic_block_write(ctx, off, rc);
		if (rc2 <= 0) {
			xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,
				    "%s: Flash write failed (%d)\n", __func__, rc2);
			return rc2;
		}
		off += rc2;
	}
	xlog_printk(ANDROID_LOG_DEBUG, IPANIC_LOG_TAG, "%s: count %d, strlen(NativeInfo):%d, off:%d\n", __func__, copy_count, strlen(NativeInfo), off);
	return copy_count;
}

static int ipanic_write_all_android_buf(struct mtd_ipanic_data *ctx, int offset, struct ipanic_header *hdr)
{
	int rc;
	
	// main buffer:
	offset = ALIGN(offset, ctx->mtd->writesize);
	rc = ipanic_write_android_buf(ctx->mtd, offset, 1);
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing android log(1) to panic log (%d)\n", rc);
		rc = 0;
	}
	hdr->android_main_offset = offset;
	hdr->android_main_length = rc;
	offset += rc;

#if 0
	// event buffer:
	offset = ALIGN(offset, ctx->mtd->writesize);
	rc = ipanic_write_android_buf(ctx->mtd, offset, 2);
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing android log(2) to panic log (%d)\n", rc);
		rc = 0;
	}
	hdr->android_event_offset = offset;
	hdr->android_event_length = rc;
	offset += rc;
#endif

	// radio buffer:
	offset = ALIGN(offset, ctx->mtd->writesize);
	rc = ipanic_write_android_buf(ctx->mtd, offset, 3);
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing android log(3) to panic log (%d)\n", rc);
		rc = 0;
	}
	hdr->android_radio_offset = offset;
	hdr->android_radio_length = rc;
	offset += rc;

	// system buffer:
	offset = ALIGN(offset, ctx->mtd->writesize);
	rc = ipanic_write_android_buf(ctx->mtd, offset, 4);
	if (rc < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing android log(4) to panic log! (%d)\n", rc);
		rc = 0;
	}
	hdr->android_system_offset = offset;
	hdr->android_system_length = rc;
	offset += rc;

	return offset;
}
static spinlock_t ipanic_lock;
extern  void ipanic_save_current_tsk_info(void);
static int mtd_ipanic(struct notifier_block *this, unsigned long event,
		      void *ptr)
{
	struct mtd_ipanic_data *ctx = &mtd_drv_ctx;
	struct ipanic_header current_hdr;
    int rc;

    aee_wdt_dump_info();

    /*In case many core run here concurrently*/
    spin_lock(&ipanic_lock);
    if (in_panic)
		return NOTIFY_DONE;
	in_panic = 1;
    /*Save task backtrace in oops_header, which will be dump into expdb*/
    ipanic_save_current_tsk_info();

	memset(&current_hdr, 0, sizeof(struct ipanic_header));
	current_hdr.magic = AEE_IPANIC_MAGIC;
	current_hdr.version = AEE_IPANIC_PHDR_VERSION;

#ifdef CONFIG_PREEMPT
	/* Ensure that cond_resched() won't try to preempt anybody */
	add_preempt_count(PREEMPT_ACTIVE);
#endif

	if (!ctx->mtd)
		goto out;
#if 1
	if (ctx->curr.magic) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Crash partition in use!\n");
		goto out;
	}
#endif

	/*
	 * Write out the console
	 * Section 0 is reserved for ipanic header, we start at section 1
	 */
	current_hdr.oops_header_offset = ctx->mtd->writesize;
	current_hdr.oops_header_length = mtd_ipanic_write_oops_header(ctx->mtd, current_hdr.oops_header_offset);
	if (current_hdr.oops_header_length < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing oops header to panic log! (%d)\n",
		       current_hdr.oops_header_length);
		current_hdr.oops_header_length = 0;
	}

	current_hdr.oops_detail_offset = ALIGN(current_hdr.oops_header_offset + current_hdr.oops_header_length, ctx->mtd->writesize);
	current_hdr.oops_detail_length = ipanic_write_oops_detail(ctx->mtd, current_hdr.oops_detail_offset);
	if (current_hdr.oops_detail_length < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "Error writing detail to panic log! (%d)\n",
			    current_hdr.oops_detail_length);
		current_hdr.oops_detail_length = 0;
	}

	/* Kernel buffer log */
	current_hdr.console_offset = ALIGN(current_hdr.oops_detail_offset + current_hdr.oops_detail_length, ctx->mtd->writesize);
	current_hdr.console_length = mtd_ipanic_write_console(ctx->mtd, current_hdr.console_offset);
	if (current_hdr.console_length < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,  "Error writing console to panic log (%d)\n", current_hdr.console_length);
		current_hdr.console_length = 0;
	}
	
	current_hdr.userspace_info_offset = ALIGN(current_hdr.console_offset + current_hdr.console_length, ctx->mtd->writesize);
	current_hdr.userspace_info_length = ipanic_write_userspace(ctx, current_hdr.userspace_info_offset);
	if (current_hdr.userspace_info_length < 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,  "Error writing user space process to panic log (%d)\n", current_hdr.console_length);
		current_hdr.userspace_info_length = 0;
	}
	
	rc = ipanic_write_all_android_buf(ctx, current_hdr.userspace_info_offset + current_hdr.userspace_info_length, &current_hdr);

	/*
	 * Finally write the ipanic header
	 */
	memset(ctx->bounce, 0, PAGE_SIZE);
	memcpy(ctx->bounce, &current_hdr, sizeof(current_hdr));
	rc = mtd_ipanic_block_write(ctx, 0, sizeof(struct ipanic_header));
	if (rc <= 0) {
		xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG,  "aee-ipanic: Header write failed (%d)\n", rc);
		goto out;
	}
	
	ipanic_header_dump(&current_hdr);
	xlog_printk(ANDROID_LOG_ERROR, IPANIC_LOG_TAG, "aee-ipanic: Panic dump sucessfully written to flash\n");

out:
#ifdef CONFIG_PREEMPT
	sub_preempt_count(PREEMPT_ACTIVE);
#endif
	in_panic = 0;
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= mtd_ipanic,
};

static struct ipanic_ops mtd_ipanic_ops = {
	.oops_copy = mtd_ipanic_oops_copy,
	.oops_free = mtd_ipanic_oops_free,
};

int __init aee_mtd_ipanic_init(void)
{
    spin_lock_init(&ipanic_lock);
	memset(&mtd_drv_ctx, 0, sizeof(mtd_drv_ctx));
	mtd_drv_ctx.bounce = (void *) __get_free_page(GFP_KERNEL);
	
	register_mtd_user(&mtd_panic_notifier);
	atomic_notifier_chain_register(&panic_notifier_list, &panic_blk);
	register_ipanic_ops(&mtd_ipanic_ops);

	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-mtd-ipanic: startup, partition assgined %s\n", AEE_IPANIC_PLABEL);
	return 0;
}

module_init(aee_mtd_ipanic_init);

#endif
