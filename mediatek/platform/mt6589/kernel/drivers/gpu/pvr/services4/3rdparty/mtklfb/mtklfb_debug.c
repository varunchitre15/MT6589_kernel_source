#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/debugfs.h>
#include <linux/spinlock.h>

#include "img_defs.h"
#include "servicesext.h"
#include "kerneldisplay.h"
#include "mtklfb.h"

#ifdef MTK_DEBUG_LFB

static char USAGE[] =
    "\n"
    "USAGE\n"
    "        echo [ACTION]... > mtklfb\n"
    "\n"
    "ACTION\n";

#define MAX_COUNT 64
#define MAX_LENGTH 128
static char g_szDebugInfo[MAX_COUNT][MAX_LENGTH];
static int g_iIndex = 0;
static spinlock_t g_kLock;

IMG_EXPORT
PVRSRV_ERROR MTKInsertDebugInfoKM(const char* szInfo)
{
	int iLen = strlen(szInfo);
	iLen = min(MAX_LENGTH - 1, iLen);
	spin_lock(&g_kLock);
	strncpy(g_szDebugInfo[g_iIndex], szInfo, iLen);
	g_szDebugInfo[g_iIndex][iLen]='\0';
	g_iIndex = (g_iIndex + 1)%MAX_COUNT;
	sprintf(g_szDebugInfo[g_iIndex], "---");
	spin_unlock(&g_kLock);
	return PVRSRV_OK;
}

IMG_EXPORT
PVRSRV_ERROR MTKPrintDebugInfoKM(IMG_UINT32 iTemp)
{
	int i;
	spin_lock(&g_kLock);
	for (i = 0; i < MAX_COUNT; ++i)
	{
		xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, "[%d]%s\n", i, g_szDebugInfo[i]);
	}
	spin_unlock(&g_kLock);
	return PVRSRV_OK;
}

static void do_command(const char *input)
{
	if (0 == strncmp(input, "trace", 5))
	{
		MTKPrintDebugInfoKM(0);
	}
	else
	{
    xlog_printk(ANDROID_LOG_ERROR, DRIVER_PREFIX, DRIVER_PREFIX ": invalid debug command...\n");
}
}

static void process_commands(char *input)
{
    char *ptr = NULL;

    xlog_printk(ANDROID_LOG_DEBUG, DRIVER_PREFIX, DRIVER_PREFIX ": %s\n", input);

    while ((ptr = strsep(&input, " ")) != NULL)
    {
        do_command(ptr);
    }
}


////////////////////////////////////////////////////////////////////////////////////

struct dentry *mtklfb_debugfs = NULL;
static char debug_buffer[2048];

static ssize_t debug_open(struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}

static ssize_t debug_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    const int debug_bufmax = sizeof(debug_buffer) - 1;
    int n = scnprintf(debug_buffer, debug_bufmax, USAGE);
    debug_buffer[n++] = 0;
    return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}

static ssize_t debug_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    const int debug_bufmax = sizeof(debug_buffer) - 1;

    if (count > debug_bufmax)
        count = debug_bufmax;

    if (copy_from_user(&debug_buffer, ubuf, count))
        return -EFAULT;

    debug_buffer[count] = 0;

    process_commands(debug_buffer);

    return count;
}

static struct file_operations debug_fops = 
{
    .read  = debug_read,
    .write = debug_write,
    .open  = debug_open,
};

void dbg_init(void)
{
    mtklfb_debugfs = debugfs_create_file("mtklfb", S_IFREG|S_IRUGO, NULL, (void *)0, &debug_fops);
}

void dbg_exit(void)
{
    debugfs_remove(mtklfb_debugfs);
}

#endif
