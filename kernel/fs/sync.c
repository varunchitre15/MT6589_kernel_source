/*
 * High-level sync()-related operations
 */

#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/namei.h>
#include <linux/sched.h>
#include <linux/writeback.h>
#include <linux/syscalls.h>
#include <linux/linkage.h>
#include <linux/pagemap.h>
#include <linux/quotaops.h>
#include <linux/backing-dev.h>
#include "internal.h"

#define FEATURE_PRINT_FSYNC_PID
#ifdef USER_BUILD_KERNEL
#undef FEATURE_PRINT_FSYNC_PID
#endif
#include <linux/xlog.h>
#include <mach/mt_io_logger.h>

#define VALID_FLAGS (SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE| \
			SYNC_FILE_RANGE_WAIT_AFTER)

/*
 * Do the filesystem syncing work. For simple filesystems
 * writeback_inodes_sb(sb) just dirties buffers with inodes so we have to
 * submit IO for these buffers via __sync_blockdev(). This also speeds up the
 * wait == 1 case since in that case write_inode() functions do
 * sync_dirty_buffer() and thus effectively write one block at a time.
 */


#ifdef FEATURE_PRINT_FSYNC_PID
#define SYNC_PRT_TIME_PERIOD	2000000000
#define ID_CNT 20
static unsigned char f_idx=0;
static unsigned char fs_idx=0;
static unsigned char fs1_idx=0;
static unsigned long long fsync_last_t=0;
static unsigned long long fs_sync_last_t=0;
static unsigned long long fs1_sync_last_t=0;
static DEFINE_MUTEX(fsync_mutex);
static DEFINE_MUTEX(fs_sync_mutex);
static DEFINE_MUTEX(fs1_sync_mutex);
struct
{
	pid_t pid;
	unsigned int cnt; 
}fsync[ID_CNT], fs_sync[ID_CNT], fs1_sync[ID_CNT];
static char xlog_buf[ID_CNT*10+50]={0};
static char xlog_buf2[ID_CNT*10+50]={0};
static char xlog_buf3[ID_CNT*10+50]={0};



static void fs_sync_mmcblk0_log(void)
{
	pid_t curr_pid;
	unsigned int i;
	unsigned long long time1=0;
	bool ptr_flag=false;


	time1 = sched_clock();
	mutex_lock(&fs_sync_mutex);
	if(fs_sync_last_t == 0)
	{
			fs_sync_last_t = time1;
	}
	if (time1 - fs_sync_last_t >= (unsigned long long)SYNC_PRT_TIME_PERIOD)
	{
		sprintf(xlog_buf2, "MMCBLK0_FS_Sync [(PID):cnt] -- ");		
		for(i=0;i<ID_CNT;i++)
		{
			if(fs_sync[i].pid==0)
				break;
			else
			{
				sprintf(xlog_buf2+31+i*9, "(%4d):%d ", fs_sync[i].pid, fs_sync[i].cnt);	//31=strlen("MMCBLK1_FS_Sync [(PID):cnt] -- "), 9=strlen("(%4d):%d ")
				ptr_flag = true;
			}
		}	
		if(ptr_flag)
		{		
			xlog_printk(ANDROID_LOG_DEBUG, "BLOCK_TAG", "MMCBLK0_FS_Sync statistic in timeline %lld\n", fs_sync_last_t); 
			xlog_printk(ANDROID_LOG_DEBUG, "BLOCK_TAG", "%s\n", xlog_buf2);			
		}		
		for (i=0;i<ID_CNT;i++)	//clear
		{
			fs_sync[i].pid=0;
			fs_sync[i].cnt=0;
		}
		fs_sync_last_t = time1;
	}
	curr_pid = task_pid_nr(current);
	do{
		if(fs_sync[0].pid ==0)
		{
			fs_sync[0].pid= curr_pid;
			fs_sync[0].cnt ++;
			fs_idx=0;
			break;
		}

		if(curr_pid == fs_sync[fs_idx].pid)
		{
			fs_sync[fs_idx].cnt++;
			break;
		}

		for(i=0;i<ID_CNT;i++)
		{
			if(curr_pid == fs_sync[i].pid)		//found
			{
				fs_sync[i].cnt++;
				fs_idx = i;
				break;
			}
			if((fs_sync[i].pid ==0) || (i==ID_CNT-1) )		//found empty space or (full and NOT found)
			{
				fs_sync[i].pid = curr_pid;
				fs_sync[i].cnt=1;
				fs_idx=i;			
				break;
			}
		}
	}while(0);
	mutex_unlock(&fs_sync_mutex);
}

static void fs_sync_mmcblk1_log(void)
{
	pid_t curr_pid;
	unsigned int i;
	unsigned long long time1=0;
	bool ptr_flag=false;

	time1 = sched_clock();
	mutex_lock(&fs1_sync_mutex);
	if(fs1_sync_last_t == 0)
	{
			fs1_sync_last_t = time1;
	}
	if (time1 - fs1_sync_last_t >= (unsigned long long)SYNC_PRT_TIME_PERIOD)
	{
		sprintf(xlog_buf3, "MMCBLK1_FS_Sync [(PID):cnt] -- ");		
		for(i=0;i<ID_CNT;i++)
		{
			if(fs1_sync[i].pid==0)
				break;
			else
			{
				sprintf(xlog_buf3+31+i*9, "(%4d):%d ", fs1_sync[i].pid, fs1_sync[i].cnt);	//31=strlen("MMCBLK1_FS_Sync [(PID):cnt] -- "), 9=strlen("(%4d):%d ")
				ptr_flag = true;
			}
		}	
		if(ptr_flag)
		{		
			xlog_printk(ANDROID_LOG_DEBUG, "BLOCK_TAG", "MMCBLK1_FS_Sync statistic in timeline %lld\n", fs1_sync_last_t); 
			xlog_printk(ANDROID_LOG_DEBUG, "BLOCK_TAG", "%s\n", xlog_buf3);			
		}		
		for (i=0;i<ID_CNT;i++)	//clear
		{
			fs1_sync[i].pid=0;
			fs1_sync[i].cnt=0;
		}
		fs1_sync_last_t = time1;
	}
	curr_pid = task_pid_nr(current);
	do{
		if(fs1_sync[0].pid ==0)
		{
			fs1_sync[0].pid= curr_pid;
			fs1_sync[0].cnt ++;
			fs1_idx=0;
			break;
		}

		if(curr_pid == fs1_sync[fs1_idx].pid)
		{
			fs1_sync[fs1_idx].cnt++;
			break;
		}

		for(i=0;i<ID_CNT;i++)
		{
			if(curr_pid == fs1_sync[i].pid)		//found
			{
				fs1_sync[i].cnt++;
				fs1_idx = i;
				break;
			}
			if((fs1_sync[i].pid ==0) || (i==ID_CNT-1) )		//found empty space or (full and NOT found)
			{
				fs1_sync[i].pid = curr_pid;
				fs1_sync[i].cnt=1;
				fs1_idx=i;			
				break;
			}
		}
	}while(0);
	mutex_unlock(&fs1_sync_mutex);
}
#endif	



static int __sync_filesystem(struct super_block *sb, int wait)
{

#ifdef FEATURE_PRINT_FSYNC_PID
	char b[BDEVNAME_SIZE];
#endif 
	/*
	 * This should be safe, as we require bdi backing to actually
	 * write out data in the first place
	 */
	if (sb->s_bdi == &noop_backing_dev_info)
		return 0;

	if (sb->s_qcop && sb->s_qcop->quota_sync)
		sb->s_qcop->quota_sync(sb, -1, wait);

	if (wait)
		sync_inodes_sb(sb);
	else
		writeback_inodes_sb(sb, WB_REASON_SYNC);

	if (sb->s_op->sync_fs)
		sb->s_op->sync_fs(sb, wait);

#ifdef FEATURE_PRINT_FSYNC_PID
if(sb->s_bdev != NULL)
{
	if((!memcmp(bdevname(sb->s_bdev, b),"mmcblk0",7)))
		fs_sync_mmcblk0_log();
	else if((!memcmp(bdevname(sb->s_bdev, b),"mmcblk1",7)))
		fs_sync_mmcblk1_log();
}
#endif

	return __sync_blockdev(sb->s_bdev, wait);
}

/*
 * Write out and wait upon all dirty data associated with this
 * superblock.  Filesystem data as well as the underlying block
 * device.  Takes the superblock lock.
 */
int sync_filesystem(struct super_block *sb)
{
	int ret;

	/*
	 * We need to be protected against the filesystem going from
	 * r/o to r/w or vice versa.
	 */
	WARN_ON(!rwsem_is_locked(&sb->s_umount));

	/*
	 * No point in syncing out anything if the filesystem is read-only.
	 */
	if (sb->s_flags & MS_RDONLY)
		return 0;

	ret = __sync_filesystem(sb, 0);
	if (ret < 0)
		return ret;
	return __sync_filesystem(sb, 1);
}
EXPORT_SYMBOL_GPL(sync_filesystem);

static void sync_one_sb(struct super_block *sb, void *arg)
{
	if (!(sb->s_flags & MS_RDONLY))
		__sync_filesystem(sb, *(int *)arg);
}
/*
 * Sync all the data for all the filesystems (called by sys_sync() and
 * emergency sync)
 */
static void sync_filesystems(int wait)
{
	iterate_supers(sync_one_sb, &wait);
}

/*
 * sync everything.  Start out by waking pdflush, because that writes back
 * all queues in parallel.
 */
SYSCALL_DEFINE0(sync)
{
#if IO_LOGGER_ENABLE
	unsigned long long time1 = 0,timeoffset = 0;
if(unlikely(en_IOLogger())){
	time1 = sched_clock();
	AddIOTrace(IO_LOGGER_MSG_VFS_NO_ARG,vfs_sync,0);
}
#endif
	wakeup_flusher_threads(0, WB_REASON_SYNC);
	sync_filesystems(0);
	sync_filesystems(1);
	if (unlikely(laptop_mode))
		laptop_sync_completion();
#if IO_LOGGER_ENABLE
if(unlikely(en_IOLogger())){	
	timeoffset = sched_clock()-time1;
	if(BEYOND_TRACE_LOG_TIME(timeoffset))
	{
		 AddIOTrace(IO_LOGGER_MSG_VFS_NO_ARG_END,vfs_sync,timeoffset);	
		 if(BEYOND_DUMP_LOG_TIME(timeoffset))
			DumpIOTrace(timeoffset);
		
	}
}
#endif
	return 0;
}

static void do_sync_work(struct work_struct *work)
{
	/*
	 * Sync twice to reduce the possibility we skipped some inodes / pages
	 * because they were temporarily locked
	 */
	sync_filesystems(0);
	sync_filesystems(0);
	printk("Emergency Sync complete\n");
	kfree(work);
}

void emergency_sync(void)
{
	struct work_struct *work;

	work = kmalloc(sizeof(*work), GFP_ATOMIC);
	if (work) {
		INIT_WORK(work, do_sync_work);
		schedule_work(work);
	}
}

/*
 * sync a single super
 */
SYSCALL_DEFINE1(syncfs, int, fd)
{
	struct file *file;
	struct super_block *sb;
	int ret;
	int fput_needed;

	file = fget_light(fd, &fput_needed);
	if (!file)
		return -EBADF;
	sb = file->f_dentry->d_sb;

	down_read(&sb->s_umount);
	ret = sync_filesystem(sb);
	up_read(&sb->s_umount);

	fput_light(file, fput_needed);
	return ret;
}

/**
 * vfs_fsync_range - helper to sync a range of data & metadata to disk
 * @file:		file to sync
 * @start:		offset in bytes of the beginning of data range to sync
 * @end:		offset in bytes of the end of data range (inclusive)
 * @datasync:		perform only datasync
 *
 * Write back data in range @start..@end and metadata for @file to disk.  If
 * @datasync is set only metadata needed to access modified file data is
 * written.
 */
int vfs_fsync_range(struct file *file, loff_t start, loff_t end, int datasync)
{
#ifdef FEATURE_PRINT_FSYNC_PID
	pid_t curr_pid;
	unsigned int i;
	unsigned long long time1=0;
	bool ptr_flag=false;
#endif	


	if (!file->f_op || !file->f_op->fsync)
		return -EINVAL;
#ifdef FEATURE_PRINT_FSYNC_PID
	time1 = sched_clock();
mutex_lock(&fsync_mutex);
	if(fsync_last_t == 0)
	{
			fsync_last_t = time1;
	}
	if (time1 - fsync_last_t >= (unsigned long long)SYNC_PRT_TIME_PERIOD)
	{
		sprintf(xlog_buf, "Fsync [(PID):cnt] -- ");		
		for(i=0;i<ID_CNT;i++)
		{
			if(fsync[i].pid==0)
				break;
			else
			{
				sprintf(xlog_buf+21+i*9, "(%4d):%d ", fsync[i].pid, fsync[i].cnt);	//21=strlen("Fsync [(PID):cnt] -- "), 9=strlen("(%4d):%d ")
				ptr_flag = true;
			}
		}	
		if(ptr_flag)
		{		
			xlog_printk(ANDROID_LOG_DEBUG, "BLOCK_TAG", "Fsync statistic in timeline %lld\n", fsync_last_t); 
			xlog_printk(ANDROID_LOG_DEBUG, "BLOCK_TAG", "%s\n", xlog_buf);			
		}		
		for (i=0;i<ID_CNT;i++)	//clear
		{
			fsync[i].pid=0;
			fsync[i].cnt=0;
		}
		fsync_last_t = time1;
	}
	curr_pid = task_pid_nr(current);	
	do{
		if(fsync[0].pid ==0)
		{
			fsync[0].pid= curr_pid;
			fsync[0].cnt ++;
			f_idx=0;
			break;
		}

		if(curr_pid == fsync[f_idx].pid)
		{
			fsync[f_idx].cnt++;
			break;
		}

		for(i=0;i<ID_CNT;i++)
		{
			if(curr_pid == fsync[i].pid)		//found
			{
				fsync[i].cnt++;
				f_idx = i;
				break;
			}
			if((fsync[i].pid ==0) || (i==ID_CNT-1) )		//found empty space or (full and NOT found)
			{
				fsync[i].pid = curr_pid;
				fsync[i].cnt=1;
				f_idx=i;			
				break;
			}
		}
	}while(0);
mutex_unlock(&fsync_mutex);
#endif	

	return file->f_op->fsync(file, start, end, datasync);
}
EXPORT_SYMBOL(vfs_fsync_range);

/**
 * vfs_fsync - perform a fsync or fdatasync on a file
 * @file:		file to sync
 * @datasync:		only perform a fdatasync operation
 *
 * Write back data and metadata for @file to disk.  If @datasync is
 * set only metadata needed to access modified file data is written.
 */
int vfs_fsync(struct file *file, int datasync)
{
	return vfs_fsync_range(file, 0, LLONG_MAX, datasync);
}
EXPORT_SYMBOL(vfs_fsync);

static int do_fsync(unsigned int fd, int datasync)
{
	struct file *file;
	int ret = -EBADF;

	file = fget(fd);
	if (file) {
		ret = vfs_fsync(file, datasync);
		fput(file);
	}
	return ret;
}

SYSCALL_DEFINE1(fsync, unsigned int, fd)
{
	return do_fsync(fd, 0);
}

SYSCALL_DEFINE1(fdatasync, unsigned int, fd)
{
	return do_fsync(fd, 1);
}

/**
 * generic_write_sync - perform syncing after a write if file / inode is sync
 * @file:	file to which the write happened
 * @pos:	offset where the write started
 * @count:	length of the write
 *
 * This is just a simple wrapper about our general syncing function.
 */
int generic_write_sync(struct file *file, loff_t pos, loff_t count)
{
	if (!(file->f_flags & O_DSYNC) && !IS_SYNC(file->f_mapping->host))
		return 0;
	return vfs_fsync_range(file, pos, pos + count - 1,
			       (file->f_flags & __O_SYNC) ? 0 : 1);
}
EXPORT_SYMBOL(generic_write_sync);

/*
 * sys_sync_file_range() permits finely controlled syncing over a segment of
 * a file in the range offset .. (offset+nbytes-1) inclusive.  If nbytes is
 * zero then sys_sync_file_range() will operate from offset out to EOF.
 *
 * The flag bits are:
 *
 * SYNC_FILE_RANGE_WAIT_BEFORE: wait upon writeout of all pages in the range
 * before performing the write.
 *
 * SYNC_FILE_RANGE_WRITE: initiate writeout of all those dirty pages in the
 * range which are not presently under writeback. Note that this may block for
 * significant periods due to exhaustion of disk request structures.
 *
 * SYNC_FILE_RANGE_WAIT_AFTER: wait upon writeout of all pages in the range
 * after performing the write.
 *
 * Useful combinations of the flag bits are:
 *
 * SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE: ensures that all pages
 * in the range which were dirty on entry to sys_sync_file_range() are placed
 * under writeout.  This is a start-write-for-data-integrity operation.
 *
 * SYNC_FILE_RANGE_WRITE: start writeout of all dirty pages in the range which
 * are not presently under writeout.  This is an asynchronous flush-to-disk
 * operation.  Not suitable for data integrity operations.
 *
 * SYNC_FILE_RANGE_WAIT_BEFORE (or SYNC_FILE_RANGE_WAIT_AFTER): wait for
 * completion of writeout of all pages in the range.  This will be used after an
 * earlier SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE operation to wait
 * for that operation to complete and to return the result.
 *
 * SYNC_FILE_RANGE_WAIT_BEFORE|SYNC_FILE_RANGE_WRITE|SYNC_FILE_RANGE_WAIT_AFTER:
 * a traditional sync() operation.  This is a write-for-data-integrity operation
 * which will ensure that all pages in the range which were dirty on entry to
 * sys_sync_file_range() are committed to disk.
 *
 *
 * SYNC_FILE_RANGE_WAIT_BEFORE and SYNC_FILE_RANGE_WAIT_AFTER will detect any
 * I/O errors or ENOSPC conditions and will return those to the caller, after
 * clearing the EIO and ENOSPC flags in the address_space.
 *
 * It should be noted that none of these operations write out the file's
 * metadata.  So unless the application is strictly performing overwrites of
 * already-instantiated disk blocks, there are no guarantees here that the data
 * will be available after a crash.
 */
SYSCALL_DEFINE(sync_file_range)(int fd, loff_t offset, loff_t nbytes,
				unsigned int flags)
{
	int ret;
	struct file *file;
	struct address_space *mapping;
	loff_t endbyte;			/* inclusive */
	int fput_needed;
	umode_t i_mode;

	ret = -EINVAL;
	if (flags & ~VALID_FLAGS)
		goto out;

	endbyte = offset + nbytes;

	if ((s64)offset < 0)
		goto out;
	if ((s64)endbyte < 0)
		goto out;
	if (endbyte < offset)
		goto out;

	if (sizeof(pgoff_t) == 4) {
		if (offset >= (0x100000000ULL << PAGE_CACHE_SHIFT)) {
			/*
			 * The range starts outside a 32 bit machine's
			 * pagecache addressing capabilities.  Let it "succeed"
			 */
			ret = 0;
			goto out;
		}
		if (endbyte >= (0x100000000ULL << PAGE_CACHE_SHIFT)) {
			/*
			 * Out to EOF
			 */
			nbytes = 0;
		}
	}

	if (nbytes == 0)
		endbyte = LLONG_MAX;
	else
		endbyte--;		/* inclusive */

	ret = -EBADF;
	file = fget_light(fd, &fput_needed);
	if (!file)
		goto out;

	i_mode = file->f_path.dentry->d_inode->i_mode;
	ret = -ESPIPE;
	if (!S_ISREG(i_mode) && !S_ISBLK(i_mode) && !S_ISDIR(i_mode) &&
			!S_ISLNK(i_mode))
		goto out_put;

	mapping = file->f_mapping;
	if (!mapping) {
		ret = -EINVAL;
		goto out_put;
	}

	ret = 0;
	if (flags & SYNC_FILE_RANGE_WAIT_BEFORE) {
		ret = filemap_fdatawait_range(mapping, offset, endbyte);
		if (ret < 0)
			goto out_put;
	}

	if (flags & SYNC_FILE_RANGE_WRITE) {
		ret = filemap_fdatawrite_range(mapping, offset, endbyte);
		if (ret < 0)
			goto out_put;
	}

	if (flags & SYNC_FILE_RANGE_WAIT_AFTER)
		ret = filemap_fdatawait_range(mapping, offset, endbyte);

out_put:
	fput_light(file, fput_needed);
out:
	return ret;
}
#ifdef CONFIG_HAVE_SYSCALL_WRAPPERS
asmlinkage long SyS_sync_file_range(long fd, loff_t offset, loff_t nbytes,
				    long flags)
{
	return SYSC_sync_file_range((int) fd, offset, nbytes,
				    (unsigned int) flags);
}
SYSCALL_ALIAS(sys_sync_file_range, SyS_sync_file_range);
#endif

/* It would be nice if people remember that not all the world's an i386
   when they introduce new system calls */
SYSCALL_DEFINE(sync_file_range2)(int fd, unsigned int flags,
				 loff_t offset, loff_t nbytes)
{
	return sys_sync_file_range(fd, offset, nbytes, flags);
}
#ifdef CONFIG_HAVE_SYSCALL_WRAPPERS
asmlinkage long SyS_sync_file_range2(long fd, long flags,
				     loff_t offset, loff_t nbytes)
{
	return SYSC_sync_file_range2((int) fd, (unsigned int) flags,
				     offset, nbytes);
}
SYSCALL_ALIAS(sys_sync_file_range2, SyS_sync_file_range2);
#endif
