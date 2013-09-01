#include "utils.h"
#include <linux/fs.h>		/* needed by file, vfs_write */
#include <linux/kernel.h>	/* needed by vsprintf */
#include <asm/uaccess.h>	/* needed by set_fs, ... */
//#include "etm_internal.h"
#include <asm/page.h>		// neede by PAGE_SIZE
/**
 * read from buffer (8 bits buffer)
 * @param _buffer the buffer to be read
 * @param first the bit offset we want to read from
 * @param num the size of data we want to read, in bits
 * @return the number read from the buffer
 */
unsigned char buf_get_u8(const void *_buffer, unsigned int first,
				unsigned int num)
{
	unsigned char *buffer = (unsigned char *)_buffer;

	if ((first == 0) && (num == 8)) {
		return buffer[0];
	} else {
		unsigned char result = 0;
		int i;
		for (i = first; i < first + num; i++) {
			if (((buffer[0] >> i) & 1) == 1)
				result |= 1 << (i - first);
		}
		return result;
	}
}

/**
 * read from buffer (32 bits buffer)
 * @param _buffer the buffer to be read
 * @param first the bit offset we want to read from
 * @param num the size of data we want to read, in bits
 * @return the number read from the buffer
 */
unsigned int buf_get_u32(const void *_buffer, unsigned int first,
				unsigned int num)
{
	unsigned char *buffer = (unsigned char *)_buffer;

	if ((first == 0) && (num == 32)) {
		return (((unsigned int)buffer[3]) << 24) |
			(((unsigned int)buffer[2]) << 16) |
			(((unsigned int)buffer[1]) << 8) |
			(((unsigned int)buffer[0]) << 0);
	} else {
		unsigned int result = 0;
		unsigned int i;
		for (i = first; i < first + num; i++) {
			if (((buffer[i / 8] >> (i % 8)) & 1) == 1)
				result |= 1 << (i - first);
		}
		return result;
	}
}

static char tmp[1000] = {0}; /*TODO*/

/**
 * open file
 * @param name path to open
 * @return file pointer
 */
struct file* open_file(const char *name)
{
	struct file *fp = NULL;

	fp = filp_open(name, O_WRONLY|O_TRUNC|O_CREAT, 0644);
	if (unlikely(fp == NULL)) {
		p_error("can not open result file");
		return NULL;
	}
	return fp;
}

struct file* open_file_R(const char *name)
{
	struct file *fp = NULL;

	fp = filp_open(name, O_RDONLY, 0);
	if (unlikely(fp == NULL)) {
		p_error("can not open result file");
		return NULL;
	}
	return fp;
}

int file_read(struct file* fp, unsigned char* data, unsigned int size) 
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    ret = vfs_read(fp, data, size, &(fp->f_pos));

    set_fs(oldfs);
    return ret;
}

int file_write(struct file* fp, unsigned char* data, unsigned int size) 
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(KERNEL_DS);

    ret = vfs_write(fp, data, size, &(fp->f_pos));

    set_fs(oldfs);
    return ret;
}

/**
 * write to file
 * @param fp file pointer
 * @param format format string
 * @param ... variable-length subsequent arguments
 */
void write_file(struct file *fp, const char *format, ...)
{
	va_list va;
	mm_segment_t fs = get_fs();

	va_start(va, format);
	vsnprintf(tmp, PAGE_SIZE, format, va);
	set_fs(KERNEL_DS);
	vfs_write(fp, tmp, strlen(tmp), &(fp->f_pos));
	set_fs(fs);
	va_end(va);
}


/**
 * close file
 * @param fp file pointer
 * @return exit code
 */
int close_file(struct file *fp)
{
	if (likely(fp != NULL)) {
		filp_close(fp, NULL);
		fp = NULL;
		return 0;
	} else {
		p_warning("cannot close file pointer:%p\n", fp);
		return -1;
	}
}

/**
 * get file position
 * @param fp file pointer
 * @return file position
 */
loff_t get_file_position(struct file *fp)
{
  return fp->f_pos;
}

/**
 * put file position
 * @param fp file pointer
 * @param pos file position
 */
void put_file_position(struct file *fp, loff_t pos)
{
  fp->f_pos = pos;
}

/**
 * check if file position reach older one
 * @param fp file pointer
 * @param pos older file position
 * @return exit code
 */
int does_reach_pre_position(struct file *fp, loff_t pos)
{
  if(fp->f_pos == pos)
    return 1;
  else
    return 0;
}