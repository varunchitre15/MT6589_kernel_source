

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <asm/uaccess.h>
#include <mach/mtk_memcfg.h>

#define MTK_MEMCFG_SIMPLE_BUFFER_LEN 16
#define MTK_MEMCFG_LARGE_BUFFER_LEN (1200) /* 1200 bytes, it should not be larger than 1 page */

struct mtk_memcfg_info_buf {
    unsigned long max_len;
    unsigned long curr_pos;
    char buf[MTK_MEMCFG_LARGE_BUFFER_LEN];
};

static struct mtk_memcfg_info_buf mtk_memcfg_layout_buf = {
    .buf = { [0 ... (MTK_MEMCFG_LARGE_BUFFER_LEN - 1)] = 0, },
    .max_len = MTK_MEMCFG_LARGE_BUFFER_LEN,
    .curr_pos = 0,
};
static unsigned long force_inode_gfp_lowmem = 0;
#ifdef CONFIG_SLUB_DEBUG
static unsigned long bypass_slub_debug = 0;
#endif 

/* inode GFP control */

static int mtk_memcfg_inode_read(char *buf, char **start, off_t off, 
        int count, int *eof, void *data) 
{
    int len = 0;
    len += sprintf(buf, "default inode GFP: %s\n", 
            force_inode_gfp_lowmem? "Low Memory": "High User Movable");

    BUG_ON(len > count);    // check if we overwrite page buffer

    return len;
}

static ssize_t mtk_memcfg_inode_write(struct file *file, const char *buffer, 
        unsigned long count, void *data)
{
    int flag, err, len;
    char buf[MTK_MEMCFG_SIMPLE_BUFFER_LEN];

    len = count > MTK_MEMCFG_SIMPLE_BUFFER_LEN? 
        MTK_MEMCFG_SIMPLE_BUFFER_LEN: count;
    err = copy_from_user(buf, buffer, len);

    if (err) {
        printk(KERN_ERR"[%s][%d] failed\n", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    buf[len] = '\0';

    if (sscanf(buf, "%d", &flag)) {
        force_inode_gfp_lowmem = flag;
    }
    
    return len;
}

unsigned long mtk_memcfg_get_force_inode_gfp_lowmem(void)
{
    return force_inode_gfp_lowmem;
}

unsigned long mtk_memcfg_set_force_inode_gfp_lowmem(unsigned long flag)
{
    force_inode_gfp_lowmem = flag;
    return force_inode_gfp_lowmem;
}

void mtk_memcfg_write_memory_layout_buf(char *fmt, ...)
{
    va_list ap;
    struct mtk_memcfg_info_buf *layout_buf = &mtk_memcfg_layout_buf;
    if (layout_buf->curr_pos <= layout_buf->max_len) {
        va_start(ap, fmt);
        layout_buf->curr_pos += 
            vsnprintf((layout_buf->buf + layout_buf->curr_pos), 
                    (layout_buf->max_len - layout_buf->curr_pos), fmt, ap);
        va_end(ap);
    }
}

/* end of inode GFP control */

/* kenerl memory information */

static int mtk_memcfg_memory_layout_read(char *buf, char **start, off_t off, 
        int count, int *eof, void *data) 
{
    int len = 0;

    len += sprintf(buf + len, mtk_memcfg_layout_buf.buf);
    len += sprintf(buf + len, "buffer usage: %lu/%lu\n", 
            (mtk_memcfg_layout_buf.curr_pos <= mtk_memcfg_layout_buf.max_len?
            mtk_memcfg_layout_buf.curr_pos: mtk_memcfg_layout_buf.max_len), 
            mtk_memcfg_layout_buf.max_len);

    /* debug buffer overflow */
    //print_hex_dump(KERN_ALERT, "layout ", DUMP_PREFIX_ADDRESS, 16, 1,
    //        mtk_memcfg_layout_buf.buf, 512, 1);
    BUG_ON(len > count);    // check if we overwrite page buffer

    return len;
}

/* end of kenerl memory information */

#ifdef CONFIG_SLUB_DEBUG
/* bypass slub debug control */

static int mtk_memcfg_slub_debug_read(char *buf, char **start, off_t off, 
        int count, int *eof, void *data) 
{
    int len = 0;
    len += sprintf(buf, "slub debug mode: %s\n", 
            bypass_slub_debug? "bypass": "normal");

    BUG_ON(len > count);    // check if we overwrite page buffer

    return len;
}

static ssize_t mtk_memcfg_slub_debug_write(struct file *file, const char *buffer, 
        unsigned long count, void *data)
{
    int flag, err, len;
    char buf[MTK_MEMCFG_SIMPLE_BUFFER_LEN];

    len = count > MTK_MEMCFG_SIMPLE_BUFFER_LEN? 
        MTK_MEMCFG_SIMPLE_BUFFER_LEN: count;
    err = copy_from_user(buf, buffer, len);

    if (err) {
        printk(KERN_ERR"[%s][%d] failed\n", __FUNCTION__, __LINE__);
        return -EINVAL;
    }

    buf[len] = '\0';

    if (sscanf(buf, "%d", &flag)) {
        mtk_memcfg_set_bypass_slub_debug_flag((unsigned long)flag);
    }
    
    return len;
}

unsigned long mtk_memcfg_get_bypass_slub_debug_flag(void)
{
    return bypass_slub_debug;
}

unsigned long mtk_memcfg_set_bypass_slub_debug_flag(unsigned long flag)
{
    /* 
     * Do not re-enable slub debug after disabling it.
     * We do not trust slub debug data after it is disabled.
     */
    if (bypass_slub_debug && !flag) {
        printk(KERN_ERR"===== slub debug is re-enabled, "
                "ignore this operation =====\n");
        goto out;
    } else if (!bypass_slub_debug && flag) {
        printk(KERN_ALERT"===== bypass slub debug =====\n");
        bypass_slub_debug = flag;
    }
out:
    return bypass_slub_debug;
}

#endif 

/* end of bypass slub debug control */

static int __init mtk_memcfg_init(void)
{
    return 0;
}

static void __exit mtk_memcfg_exit(void)
{
}

static int __init mtk_memcfg_late_init(void)
{
    struct proc_dir_entry *entry = NULL;
    struct proc_dir_entry *mtk_memcfg_dir = NULL;

    printk(KERN_INFO"[%s] start\n", __FUNCTION__);

    mtk_memcfg_dir = proc_mkdir("mtk_memcfg", NULL);

    if (!mtk_memcfg_dir) {

        printk(KERN_ERR"[%s]: mkdir /proc/mtk_memcfg failed\n",
                __FUNCTION__);

    } else {

        /* inode gfp conifg */
        entry = create_proc_entry("force_inode_gfp_lowmem", 
                S_IRUGO | S_IWUSR, mtk_memcfg_dir);

        if (entry) {
            entry->read_proc = mtk_memcfg_inode_read;
            entry->write_proc = mtk_memcfg_inode_write;
        } else {
            printk(KERN_ERR"create force_inode_gfp_lowmem proc entry failed\n");
        }

        /* display kernel memory layout */
        entry = create_proc_entry("memory_layout", 
                S_IRUGO | S_IWUSR, mtk_memcfg_dir);

        if (entry) {
            entry->read_proc = mtk_memcfg_memory_layout_read;
        } else {
            printk(KERN_ERR"create memory_layout proc entry failed\n");
        }

#ifdef CONFIG_SLUB_DEBUG
        /* bypass slub debug control */
        entry = create_proc_entry("bypass_slub_debug", 
                S_IRUGO | S_IWUSR, mtk_memcfg_dir);

        if (entry) {
            entry->read_proc = mtk_memcfg_slub_debug_read;
            entry->write_proc = mtk_memcfg_slub_debug_write;
        } else {
            printk(KERN_ERR"create bypass_slub_debug proc entry failed\n");
        }
#endif 

    }
    
    return 0;
}

module_init(mtk_memcfg_init);
module_exit(mtk_memcfg_exit);

late_initcall(mtk_memcfg_late_init);
