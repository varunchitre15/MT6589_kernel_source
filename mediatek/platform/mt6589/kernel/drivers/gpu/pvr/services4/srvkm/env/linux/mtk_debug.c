#include <linux/sched.h>
#include "img_types.h"
#include <linux/string.h>	
#include "mtk_debug.h"
#include "servicesext.h"
#include "mutex.h"
#include "services.h"
#include "osfunc.h"

#ifdef MTK_DEBUG

#ifdef MTK_DEBUG_PROC_PRINT
#include <linux/vmalloc.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "proc.h"
#include "pvr_debug.h"
#include "services_headers.h"
#include "srvkm.h"
#endif

static IMG_UINT32 g_PID;
static PVRSRV_LINUX_MUTEX g_sDebugMutex;
static IMG_CHAR g_acMsgBuffer[MTK_DEBUG_MSG_LENGTH];
static IMG_BOOL g_bEnable3DMemInfo;

#ifdef MTK_DEBUG_PROC_PRINT

static struct proc_dir_entry *g_MTKPP_proc;
static MTK_PROC_PRINT_DATA *g_MTKPPdata[MTK_PP_R_SIZE];

static spinlock_t g_MTKPP_4_SGXOSTimer_lock;
static unsigned long g_MTKPP_4_SGXOSTimer_irqflags;

static void *g_MTKPP_4_SGXOSTimer_current;

static void MTKPPInitLock(MTK_PROC_PRINT_DATA *data)
{
	spin_lock_init(&data->lock);
}

static void MTKPPLock(MTK_PROC_PRINT_DATA *data)
{
	spin_lock_irqsave(&data->lock, data->irqflags);
}
static void MTKPPUnLock(MTK_PROC_PRINT_DATA *data)
{
	spin_unlock_irqrestore(&data->lock, data->irqflags);
}

static void MTKPPPrintQueueBuffer(MTK_PROC_PRINT_DATA *data, const char *fmt, ...) IMG_FORMAT_PRINTF(2,3);
static void MTKPPPrintQueueBuffer2(MTK_PROC_PRINT_DATA *data, const char *fmt, ...) IMG_FORMAT_PRINTF(2,3);
static void MTKPPPrintRingBuffer(MTK_PROC_PRINT_DATA *data, const char *fmt, ...) IMG_FORMAT_PRINTF(2,3);

static int MTKPPPrintTime(char *buf, int n)
{
	/* copy & modify from ./kernel/printk.c */
	unsigned long long t;
	unsigned long nanosec_rem;

	t = cpu_clock(smp_processor_id());
	nanosec_rem = do_div(t, 1000000000);
	
	return snprintf(buf, n, "[%5lu.%06lu] ", (unsigned long) t, nanosec_rem / 1000);
}

static void MTKPPPrintQueueBuffer(MTK_PROC_PRINT_DATA *data, const char *fmt, ...)
{
	va_list args;
	char *buf;
	int len;

	MTKPPLock(data);
	
	if ((data->l >= data->line_array_size)
		|| (data->p >= (data->data_array_size - 128)))
	{
		// out of buffer, ignore input
		MTKPPUnLock(data);
		return;
	}

	/* Move to next line */
	buf = data->line[data->l++] = data->data + data->p;
	
	/* Print string */
	va_start(args, fmt);
	len = vsnprintf(buf, (data->data_array_size - data->p), fmt, args);
	va_end(args);
	
	data->p += len + 1;
	
	MTKPPUnLock(data);
}

static void MTKPPPrintQueueBuffer2(MTK_PROC_PRINT_DATA *data, const char *fmt, ...)
{
	va_list args;
	char *buf;
	int len;

	MTKPPLock(data);
	
	if ((data->l >= data->line_array_size)
		|| (data->p >= (data->data_array_size - 128)))
	{
		// out of buffer, ignore input
		MTKPPUnLock(data);
		return;
	}

	/* Move to next line */
	buf = data->line[data->l++] = data->data + data->p;

	/* Add the current time stamp */
	len = MTKPPPrintTime(buf, (data->data_array_size - data->p));
	buf += len;
	data->p += len;

	/* Print string */
	va_start(args, fmt);
	len = vsnprintf(buf, (data->data_array_size - data->p), fmt, args);
	va_end(args);
	
	data->p += len + 1 ;
	
	MTKPPUnLock(data);
}

static void MTKPPPrintRingBuffer(MTK_PROC_PRINT_DATA *data, const char *fmt, ...)
{
	va_list args;
	char *buf;
	int len, s;

	MTKPPLock(data);
	
	if ((data->l >= data->line_array_size)
		|| (data->p >= (data->data_array_size - 128)))
	{
		data->l = 0;
		data->p = 0;
	}

	/* Move to next line */
	buf = data->line[data->l++] = data->data + data->p;

	/* Add the current time stamp */
	len = MTKPPPrintTime(buf, (data->data_array_size - data->p));
	buf += len;
	data->p += len;

	/* Print string */
	va_start(args, fmt);
	len = vsnprintf(buf, (data->data_array_size - data->p), fmt, args);
	va_end(args);
	
	data->p += len + 1 ;

	/* Clear overflow data */
	buf += len; s = data->l;
	while (s < data->line_array_size 
		&& data->line[s] != NULL 
		&& data->line[s] <= buf)
	{
		data->line[s++] = NULL;
	}
	
	MTKPPUnLock(data);
}

static MTK_PROC_PRINT_DATA *MTKPPAllocStruct(int flags)
{
	MTK_PROC_PRINT_DATA *data;

	data = vmalloc(sizeof(MTK_PROC_PRINT_DATA));
	if (data == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s: vmalloc fail", __func__));
		goto err_out;
	}

	data->data = NULL;
	data->line = NULL;
	data->data_array_size = 0;
	data->line_array_size = 0;
	data->p = 0;
	data->l = 0;
	data->flags = flags;

	switch (flags)
	{
		case MTK_PP_B_QUEUEBUFFER:
			data->pfn_print = MTKPPPrintQueueBuffer;
			break;
		case MTK_PP_B_RINGBUFFER:
			data->pfn_print = MTKPPPrintRingBuffer;
			break;
		default:
			// something wrong
			PVR_DPF((PVR_DBG_ERROR, "%s: unknow flags: %d", __func__, flags));
			goto err_out2;
			break;
	}
	
	MTKPPInitLock(data);

	return data;
		
err_out2:
	vfree(data);
err_out:	
	return NULL;

}

static void MTKPPFreeStruct(MTK_PROC_PRINT_DATA **data)
{
	vfree(*data);
	*data = NULL;
}

static void MTKPPAllocData(MTK_PROC_PRINT_DATA *data, int data_size, int max_line)
{
	MTKPPLock(data);
		
	data->data = (char *)kmalloc(sizeof(char)*data_size, GFP_ATOMIC);
	if (data->data == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s, vmalloc data fail, size = %d", 
			__func__, data_size));
		goto err_alloc_struct;
	}
	data->line = (char **)kmalloc(sizeof(char*)*max_line, GFP_ATOMIC);
	if (data->line == NULL)
	{
		PVR_DPF((PVR_DBG_ERROR, "%s, vmalloc line fail, size = %d", 
			__func__, data_size));
		goto err_alloc_data;
	}

	data->data_array_size = data_size;
	data->line_array_size = max_line;
	
	MTKPPUnLock(data);

	return;
	
err_alloc_data:
	kfree(data->data);
err_alloc_struct:	
	MTKPPUnLock(data);
	return;
	
}

static void MTKPPFreeData(MTK_PROC_PRINT_DATA *data)
{
	MTKPPLock(data);
	
	kfree(data->line);
	kfree(data->data);
	
	data->line = NULL;
	data->data = NULL;
	data->data_array_size = 0;
	data->line_array_size = 0;
	data->p = 0;
	data->l = 0;
		
	MTKPPUnLock(data);
}

static void MTKPPCleanData(MTK_PROC_PRINT_DATA *data)
{
	MTKPPLock(data);

	memset(data->line, 0, sizeof(char*)*data->line_array_size);
	data->p = 0;
	data->l = 0;
	
	MTKPPUnLock(data);
}

static void* MTKPPSeqStart(struct seq_file *s, loff_t *pos)
{
	loff_t *spos;
	
	spos = kmalloc(sizeof(loff_t), GFP_KERNEL);
	
	spin_lock_irqsave(&g_MTKPP_4_SGXOSTimer_lock, g_MTKPP_4_SGXOSTimer_irqflags);

	if (*pos >= MTK_PP_R_SIZE)	
	{
		// lono@2013/1/7
		// seq_file cycle: ^(A(CD)+B)+AB$
		// A: start, B: stop, C: show, D: next
		MTKPPFreeData(g_MTKPPdata[MTK_PP_R_SGXOSTimer]);
		return NULL;
	}

	if (spos == NULL)
	{	
		return NULL;
	}

	*spos = *pos;
	return spos;
}

static void* MTKPPSeqNext(struct seq_file *s, void *v, loff_t *pos)
{
	loff_t *spos = (loff_t *) v;
	*pos = ++(*spos);
	
	return (*pos < MTK_PP_R_SIZE) ? spos : NULL;
}

static void MTKPPSeqStop(struct seq_file *s, void *v)
{
	spin_unlock_irqrestore(&g_MTKPP_4_SGXOSTimer_lock, g_MTKPP_4_SGXOSTimer_irqflags);
	
	kfree(v);
}

static int MTKPPSeqShow(struct seq_file *sfile, void *v)
{
	MTK_PROC_PRINT_DATA *data;
	int off, i;
	loff_t *spos = (loff_t *) v;

    off = *spos;	
	data = g_MTKPPdata[off];
		
	seq_printf(sfile, "\n" "===== buffer_id = %d =====\n", off);

	MTKPPLock(data);
	
	switch (data->flags)
	{
		case MTK_PP_B_QUEUEBUFFER:
			seq_printf(sfile, "data_size = %d/%d\n", data->p, data->data_array_size);
			seq_printf(sfile, "data_line = %d/%d\n", data->l, data->line_array_size);
			for (i = 0; i < data->l; ++i)
			{
				seq_printf(sfile, "%s\n", data->line[i]);
			}
			break;
		case MTK_PP_B_RINGBUFFER:
			seq_printf(sfile, "data_size = %d\n", data->data_array_size);
			seq_printf(sfile, "data_line = %d\n", data->line_array_size);
			for (i = data->l; i < data->line_array_size; ++i)
			{
				if (data->line[i] != NULL)
				{
					seq_printf(sfile, "%s\n", data->line[i]);
				}
			}
			for (i = 0; i < data->l; ++i)
			{
				if (data->line[i] != NULL)
				{
					seq_printf(sfile, "%s\n", data->line[i]);
				}
			}
			break;
		default:
			// FIXME: assert here
			break;
	}
	
	MTKPPUnLock(data);
	
	return 0;
}

static struct seq_operations g_MTKPP_seq_ops = {
    .start = MTKPPSeqStart,
    .next  = MTKPPSeqNext,
    .stop  = MTKPPSeqStop,
    .show  = MTKPPSeqShow
};

static int MTKPPProcOpen(struct inode *inode, struct file *file)
{
    return seq_open(file, &g_MTKPP_seq_ops);
}

static struct file_operations g_MTKPP_proc_ops = {
    .open    = MTKPPProcOpen,
    .read    = seq_read,    // system
    .llseek  = seq_lseek,   // system
    .release = seq_release  // system
};

static IMG_VOID MTKPPInit(IMG_VOID)
{
	int i;
	struct {
		int uid;
		int flags;
		int data_size;
		int max_line;
	} mtk_pp_register_tabls[] =
	{	
		{MTK_PP_R_SGXOSTimer,	MTK_PP_B_QUEUEBUFFER,	0,					0},
		{MTK_PP_R_DEVMEM,		MTK_PP_B_RINGBUFFER,	1024 * 1024 * 1,	512},
	};

	for (i = 0; i < MTK_PP_R_SIZE; ++i)
	{
		if (i != mtk_pp_register_tabls[i].uid)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: index(%d) != tabel_uid(%d)", 
				__func__, i, mtk_pp_register_tabls[i].uid));
			goto err_out;
		}
		
		g_MTKPPdata[i] = MTKPPAllocStruct(mtk_pp_register_tabls[i].flags);

		if (g_MTKPPdata[i] == NULL)
		{
			PVR_DPF((PVR_DBG_ERROR, "%s: alloc struct fail: flags = %d", 
				__func__, mtk_pp_register_tabls[i].flags));
			goto err_out;
		}

		if (mtk_pp_register_tabls[i].data_size > 0)
		{
			MTKPPAllocData(
				g_MTKPPdata[i],
				mtk_pp_register_tabls[i].data_size * 2,
				mtk_pp_register_tabls[i].max_line * 2
				);
			
			MTKPPCleanData(g_MTKPPdata[i]);
		}
	}
	
	g_MTKPP_proc = create_proc_entry("gpulog", 0, NULL);
	g_MTKPP_proc->proc_fops = &g_MTKPP_proc_ops;

	g_MTKPP_4_SGXOSTimer_current = NULL;
	spin_lock_init(&g_MTKPP_4_SGXOSTimer_lock);

	return;
	
err_out:	
	return;
}

static IMG_VOID MTKPPDeinit(IMG_VOID)
{
	int i;
	
	remove_proc_entry("gpulog", NULL);
	
	for (i = (MTK_PP_R_SIZE - 1); i >= 0; --i)
	{
		MTKPPFreeData(g_MTKPPdata[i]);
		MTKPPFreeStruct(&g_MTKPPdata[i]);
	}
}

MTK_PROC_PRINT_DATA *MTKPPGetObjById(int id)
{
	return (id >= 0 && id < MTK_PP_R_SIZE) ? 
		g_MTKPPdata[id] : NULL;
}

// for SGXOSTimer()
MTK_PROC_PRINT_DATA *MTK_PP_4_SGXOSTimer_getregister()
{
	return (g_MTKPP_4_SGXOSTimer_current == (void *)current) ? 
		g_MTKPPdata[MTK_PP_R_SGXOSTimer] : NULL;
}

int MTK_PP_4_SGXOSTimer_register(void)
{
	spin_lock_irqsave(&g_MTKPP_4_SGXOSTimer_lock, g_MTKPP_4_SGXOSTimer_irqflags);

	if (g_MTKPPdata[MTK_PP_R_SGXOSTimer]->data == NULL)
	{
		MTKPPAllocData(
			g_MTKPPdata[MTK_PP_R_SGXOSTimer], 
			1024 * 1024 * 4,
			1024 * 64
			);
		MTKPPCleanData(g_MTKPPdata[MTK_PP_R_SGXOSTimer]);
	}

	MTKPPPrintQueueBuffer2(g_MTKPPdata[MTK_PP_R_SGXOSTimer], "=== start ===");
	
	g_MTKPP_4_SGXOSTimer_current = (void *)current;
	return 1;
}

void MTK_PP_4_SGXOSTimer_unregister(void)
{
	g_MTKPP_4_SGXOSTimer_current = NULL;
	
	spin_unlock_irqrestore(&g_MTKPP_4_SGXOSTimer_lock, g_MTKPP_4_SGXOSTimer_irqflags);
}

#endif // MTK_DEBUG_PROC_PRINT

IMG_VOID MTKDebugInit(IMG_VOID)
{
	LinuxInitMutex(&g_sDebugMutex);
    g_bEnable3DMemInfo = IMG_FALSE;
    g_PID = 0;
	g_acMsgBuffer[0] = '\0';
	
#ifdef MTK_DEBUG_PROC_PRINT
	MTKPPInit();
#endif
}

IMG_VOID MTKDebugDeinit(IMG_VOID)
{
#ifdef MTK_DEBUG_PROC_PRINT
	MTKPPDeinit();
#endif
}

IMG_VOID MTKDebugSetInfo(
	const IMG_CHAR* pszInfo,
    IMG_INT32       i32Size)
{
    if (i32Size > MTK_DEBUG_MSG_LENGTH)
    {
        i32Size = MTK_DEBUG_MSG_LENGTH;
    }
	LinuxLockMutex(&g_sDebugMutex);
    if (pszInfo && i32Size > 0)
    {
    	g_PID = OSGetCurrentProcessIDKM();
        memcpy(g_acMsgBuffer, pszInfo, i32Size);
    	g_acMsgBuffer[i32Size - 1] = '\0';
    }
    else
    {
        g_PID = 0;
        g_acMsgBuffer[0] = '\0';
    }
	LinuxUnLockMutex(&g_sDebugMutex);
}

IMG_VOID MTKDebugGetInfo(
    IMG_CHAR* acDebugMsg,
    IMG_INT32 i32Size)
{
    if (i32Size > MTK_DEBUG_MSG_LENGTH)
    {
        i32Size = MTK_DEBUG_MSG_LENGTH;
    }
	LinuxLockMutex(&g_sDebugMutex);
	if ((g_PID == OSGetCurrentProcessIDKM()) || (g_acMsgBuffer[0] == '\0'))
	{
		memcpy(acDebugMsg, g_acMsgBuffer, i32Size);
        acDebugMsg[i32Size - 1] = '\0';
	}
	else
	{
		sprintf(acDebugMsg, "{None}");
	}
	LinuxUnLockMutex(&g_sDebugMutex);
}

IMG_IMPORT IMG_VOID IMG_CALLCONV MTKDebugEnable3DMemInfo(IMG_BOOL bEnable)
{
	LinuxLockMutex(&g_sDebugMutex);
    g_bEnable3DMemInfo = bEnable;
	LinuxUnLockMutex(&g_sDebugMutex);
}

IMG_IMPORT IMG_BOOL IMG_CALLCONV MTKDebugIsEnable3DMemInfo(IMG_VOID)
{
    return g_bEnable3DMemInfo;
}

#endif
