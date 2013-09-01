/* 
 * (C) Copyright 2010
 * MediaTek <www.MediaTek.com>
 *
 * Android Exception Device
 *
 */
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hardirq.h>
#include <linux/init.h>
#include <linux/kallsyms.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/disp_assert_layer.h>
#include <mach/system.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include "aed.h"
#include "../ipanic/ipanic.h"
/*
 *  may be accessed from irq
*/
static spinlock_t aed_device_lock;

static struct proc_dir_entry *aed_proc_dir;
static struct proc_dir_entry *aed_proc_current_ke_console_file;
static struct proc_dir_entry *aed_proc_current_ee_coredump_file;

static struct proc_dir_entry *aed_proc_current_ke_android_main_file;
static struct proc_dir_entry *aed_proc_current_ke_android_radio_file;
static struct proc_dir_entry *aed_proc_current_ke_android_system_file;
static struct proc_dir_entry *aed_proc_current_ke_userspaceInfo_file;
static struct proc_dir_entry *aed_proc_current_ke_mmprofile_file;

/******************************************************************************
 * DEBUG UTILITIES
 *****************************************************************************/

void msg_show(const char *prefix, AE_Msg *msg)
{
	const char *cmd_type = NULL;
	const char *cmd_id = NULL;

	if (msg == NULL) {
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s: EMPTY msg\n", prefix);
		return;
	}

	switch(msg->cmdType) {
	case AE_REQ:
		cmd_type = "REQ";
		break;
	case AE_RSP:
		cmd_type = "RESPONSE";
		break;
	case AE_IND:
		cmd_type = "IND";
		break;
	default:
		cmd_type = "UNKNOWN";
		break;
	}

	switch (msg->cmdId) {
	case AE_REQ_IDX:
		cmd_id = "IDX";
		break;
	case AE_REQ_CLASS:
		cmd_id = "CLASS";
		break;
	case AE_REQ_TYPE:
		cmd_id = "TYPE";
		break;
	case AE_REQ_MODULE:
		cmd_id = "MODULE";
		break;
	case AE_REQ_PROCESS:
		cmd_id = "PROCESS";
		break;
	case AE_REQ_DETAIL:
		cmd_id = "DETAIL";
		break;
	case AE_REQ_BACKTRACE:
		cmd_id = "BACKTRACE";
		break;
	case AE_REQ_COREDUMP:
		cmd_id = "COREDUMP";
		break;
	default:
		cmd_id = "UNKNOWN";
		break;
	}

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s: cmdType=%s[%d] cmdId=%s[%d] seq=%d arg=%x len=%d\n", prefix, cmd_type, msg->cmdType, cmd_id, msg->cmdId,
		  msg->seq, msg->arg, msg->len);
}


/******************************************************************************
 * CONSTANT DEFINITIONS
 *****************************************************************************/
#define CURRENT_KE_CONSOLE "current-ke-console"
#define CURRENT_EE_COREDUMP "current-ee-coredump"

#define CURRENT_KE_ANDROID_MAIN "current-ke-android_main"
#define CURRENT_KE_ANDROID_RADIO "current-ke-android_radio"
#define CURRENT_KE_ANDROID_SYSTEM "current-ke-android_system"
#define CURRENT_KE_USERSPACE_INFO "current-ke-userspace_info"

#define CURRENT_KE_MMPROFILE "current-ke-mmprofile"

#define MAX_EE_COREDUMP 0x800000

/******************************************************************************
 * STRUCTURE DEFINITIONS
 *****************************************************************************/

struct aed_eerec { // external exception record
	char assert_type[32];
	char exp_filename[512];
	unsigned int exp_linenum;
	unsigned int fatal1;
	unsigned int fatal2;

	int *ee_log;
	int  ee_log_size;
	int *ee_phy;
	int  ee_phy_size;
	char *msg;
};

struct aed_kerec { // TODO: kernel exception record
        char *msg;
	struct aee_oops *lastlog;
};

struct aed_dev {
	struct aed_eerec  eerec;
	wait_queue_head_t eewait;

	struct aed_kerec  kerec;
	wait_queue_head_t kewait;
};


/******************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/
static long aed_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


/******************************************************************************
 * GLOBAL DATA
 *****************************************************************************/
static struct aed_dev aed_dev;

/******************************************************************************
 * Message Utilities
 *****************************************************************************/

inline void msg_destroy(char **ppmsg)
{
	if (*ppmsg != NULL) {
		kfree(*ppmsg);
		*ppmsg = NULL;
	}
}

inline AE_Msg *msg_create(char **ppmsg, int extra_size)
{
	int size;

	msg_destroy(ppmsg);
	size = sizeof(AE_Msg) + extra_size;

	*ppmsg = kzalloc(size, GFP_ATOMIC);
	if (*ppmsg == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "%s : kzalloc() fail\n", __func__);
		return NULL;
	}

	((AE_Msg*)(*ppmsg))->len = extra_size;

	return (AE_Msg*)*ppmsg;
}

static ssize_t msg_copy_to_user(const char *prefix, const char *msg, char __user *buf, size_t count, loff_t *f_pos)
{
	ssize_t ret = 0;
	int len;

	msg_show(prefix, (AE_Msg*)msg);

	if (msg == NULL)
		return 0;

	len = ((AE_Msg*)msg)->len + sizeof(AE_Msg);

	if (*f_pos >= len) {
		ret = 0;
		goto out;
	}

	// TODO: semaphore
	if ((*f_pos + count) > len) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "read size overflow, count=%d, *f_pos=%d\n", count, (int)*f_pos);
		count = len - *f_pos;
		ret = -EFAULT;
		goto out;
	}

	if (copy_to_user(buf, msg + *f_pos, count)) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "copy_to_user failed\n");
		ret = -EFAULT;
		goto out;
	}
	*f_pos += count;
	ret = count;
out:
	return ret;
}

/******************************************************************************
 * Kernel message handlers
 *****************************************************************************/
static void ke_gen_notavail_msg(void)
{
	AE_Msg *rep_msg;
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, 0);
    if(rep_msg == NULL)
        return;

	rep_msg->cmdType = AE_RSP;
	rep_msg->arg = AE_NOT_AVAILABLE;
	rep_msg->len = 0;
}

static void ke_gen_class_msg(void)
{
#define KE_CLASS_STR "Kernel (KE)"
#define KE_CLASS_SIZE 12
	AE_Msg *rep_msg;
	char *data;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, KE_CLASS_SIZE);
    if(rep_msg == NULL)
        return;

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_CLASS;
	rep_msg->len = KE_CLASS_SIZE;
	strncpy(data, KE_CLASS_STR, KE_CLASS_SIZE);
}

static void ke_gen_type_msg(void)
{
#define KE_TYPE_STR "PANIC"
#define KE_TYPE_SIZE 6
	AE_Msg *rep_msg;
	char *data;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	rep_msg = msg_create(&aed_dev.kerec.msg, KE_TYPE_SIZE);
    if(rep_msg == NULL)
        return;

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_TYPE;
	rep_msg->len = KE_TYPE_SIZE;
	strncpy(data, KE_TYPE_STR, KE_TYPE_SIZE);
}

static void ke_gen_module_msg(void)
{
	AE_Msg *rep_msg;
	char *data;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);
	rep_msg = msg_create(&aed_dev.kerec.msg, strlen(aed_dev.kerec.lastlog->module) + 1);
    if(rep_msg == NULL)
        return;

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_MODULE;
	rep_msg->len = strlen(aed_dev.kerec.lastlog->module) + 1;
	strlcpy(data, aed_dev.kerec.lastlog->module, sizeof(aed_dev.kerec.lastlog->module));
}

static void ke_gen_detail_msg(const AE_Msg *req_msg)
{
	AE_Msg *rep_msg;
	char *data;
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "ke_gen_detail_msg is called\n");
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s req_msg arg:%d\n", __func__, req_msg->arg);

    rep_msg = msg_create(&aed_dev.kerec.msg, aed_dev.kerec.lastlog->detail_len + 1);
    if(rep_msg == NULL)
        return;	

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_DETAIL;
	rep_msg->len = aed_dev.kerec.lastlog->detail_len + 1;
    if(aed_dev.kerec.lastlog->detail != NULL)
    {
        strlcpy(data, aed_dev.kerec.lastlog->detail, aed_dev.kerec.lastlog->detail_len);
    }
	data[aed_dev.kerec.lastlog->detail_len] = 0;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "ke_gen_detail_msg is return: %s\n", data);
}

static void ke_gen_process_msg(void)
{
	AE_Msg *rep_msg;
	char *data;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);
    rep_msg = msg_create(&aed_dev.kerec.msg, AEE_PROCESS_NAME_LENGTH);
    if(rep_msg == NULL)
        return;	

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_PROCESS;

	strncpy(data, aed_dev.kerec.lastlog->process_path, AEE_PROCESS_NAME_LENGTH);
	/* Count into the NUL byte at end of string */
	rep_msg->len = strlen(data) + 1;
}

static void ke_gen_backtrace_msg(void)
{
	AE_Msg *rep_msg;
	char *data;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);
	rep_msg = msg_create(&aed_dev.kerec.msg, AEE_BACKTRACE_LENGTH);
    if(rep_msg == NULL)
        return;	

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_BACKTRACE;

	strcpy(data, aed_dev.kerec.lastlog->backtrace);
	/* Count into the NUL byte at end of string */
	rep_msg->len = strlen(data) + 1;
}

static void ke_gen_ind_msg(struct aee_oops *oops)
{
    unsigned long flags = 0;

    xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s oops %p\n", __func__, oops);
    if (oops == NULL) {
        return;
    }	
    
    spin_lock_irqsave(&aed_device_lock, flags); 
    if (aed_dev.kerec.lastlog == NULL) {
        aed_dev.kerec.lastlog = oops;
    }
    else {
        /*
         *  waaa..   Two ke api at the same time 
         *  or ke api during aed process is still busy at ke
         *  discard the new oops!
         */

        xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "%s: More than on kernel message queued\n", __func__);
        aee_oops_free(oops);
        spin_unlock_irqrestore(&aed_device_lock, flags);

        return;
    }
    spin_unlock_irqrestore(&aed_device_lock, flags);

    if (aed_dev.kerec.lastlog != NULL) {
        AE_Msg *rep_msg;
        rep_msg = msg_create(&aed_dev.kerec.msg, 0);
        if(rep_msg == NULL)
            return;	

        rep_msg->cmdType = AE_IND;
        switch (oops->attr) {
            case AE_DEFECT_REMINDING:
                rep_msg->cmdId = AE_IND_REM_RAISED;
                break;
            case AE_DEFECT_WARNING:
                rep_msg->cmdId = AE_IND_WRN_RAISED;
                break;
            case AE_DEFECT_EXCEPTION:
                rep_msg->cmdId = AE_IND_EXP_RAISED;
                break;
            case AE_DEFECT_FATAL:
                rep_msg->cmdId = AE_IND_FATAL_RAISED;
                break;
            default:
                /* Huh... something wrong, just go to exception */
                rep_msg->cmdId = AE_IND_EXP_RAISED;
                break;
        }

        rep_msg->arg = oops->clazz;
        rep_msg->len = 0;
        rep_msg->dbOption = oops->dump_option;

        wake_up(&aed_dev.kewait);
    }

}

static void ke_destroy_log(void)
{
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);
	msg_destroy(&aed_dev.kerec.msg);

	if(aed_dev.kerec.lastlog)
	{
		if (strncmp(aed_dev.kerec.lastlog->module, IPANIC_MODULE_TAG, strlen(IPANIC_MODULE_TAG)) == 0) {
			ipanic_oops_free(aed_dev.kerec.lastlog, 0);
		}
		else {
			aee_oops_free(aed_dev.kerec.lastlog);
		}

		aed_dev.kerec.lastlog = NULL;
	}
}

static int ke_log_avail(void)
{
    if (aed_dev.kerec.lastlog != NULL) {
        xlog_printk(ANDROID_LOG_INFO, AEK_LOG_TAG, "panic log avaiable\n");
        return 1;
    }

    return 0;
}

/******************************************************************************
 * EE message handlers
 *****************************************************************************/
static void ee_gen_notavail_msg(void)
{
	AE_Msg *rep_msg;
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	rep_msg = msg_create(&aed_dev.eerec.msg, 0);
    if(rep_msg == NULL)
        return;	

	rep_msg->cmdType = AE_RSP;
	rep_msg->arg = AE_NOT_AVAILABLE;
	rep_msg->len = 0;
}

static void ee_gen_class_msg(void)
{
#define EX_CLASS_EE_STR "External (EE)"
#define EX_CLASS_EE_SIZE 14
	AE_Msg *rep_msg;
	char *data;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	rep_msg = msg_create(&aed_dev.eerec.msg, EX_CLASS_EE_SIZE);
    if(rep_msg == NULL)
        return;	

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_CLASS;
	rep_msg->len = EX_CLASS_EE_SIZE;
	strncpy(data, EX_CLASS_EE_STR, EX_CLASS_EE_SIZE);
}

static void ee_gen_type_msg(void)
{
	AE_Msg *rep_msg;
	char *data;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	rep_msg = msg_create(&aed_dev.eerec.msg, strlen((char const *)&aed_dev.eerec.assert_type)+1);
    if(rep_msg == NULL)
        return;	

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_TYPE;
	rep_msg->len = strlen((char const *)&aed_dev.eerec.assert_type) + 1;
	strncpy(data, (char const *)&aed_dev.eerec.assert_type, strlen((char const *)&aed_dev.eerec.assert_type));
}

static void ee_gen_process_msg(void)
{
#define PROCESS_STRLEN 512
	
	int n = 0;
	AE_Msg *rep_msg;
	char *data;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	rep_msg = msg_create(&aed_dev.eerec.msg, PROCESS_STRLEN);
    if(rep_msg == NULL)
        return;	

	data = (char*)rep_msg + sizeof(AE_Msg);

    if (aed_dev.eerec.exp_linenum != 0) {
    	// for old aed_md_exception1()
    	n = sprintf(data, "%s", aed_dev.eerec.assert_type);
    	if (aed_dev.eerec.exp_filename[0] != 0) {
    		n += sprintf(data + n, ", filename=%s,line=%d", aed_dev.eerec.exp_filename, aed_dev.eerec.exp_linenum);
    	}
    	else if (aed_dev.eerec.fatal1 != 0 && aed_dev.eerec.fatal2 != 0) {
    		n += sprintf(data + n, ", err1=%d,err2=%d", aed_dev.eerec.fatal1, aed_dev.eerec.fatal2);
    	}
	} else {
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "ee_gen_process_msg else\n") ;
		n = sprintf (data, "%s", aed_dev.eerec.exp_filename) ;
	}

	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_PROCESS;
	rep_msg->len = n + 1;
}

static void ee_gen_detail_msg(void)
{
#define DETAIL_STRLEN 16384 // TODO: check if enough?
	int i, n = 0;
	AE_Msg *rep_msg;
	char *data;
	int *mem;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

    rep_msg = msg_create(&aed_dev.eerec.msg, DETAIL_STRLEN);
	if (rep_msg == NULL)
		return;	

	data = (char*)rep_msg + sizeof(AE_Msg);

	mem = (int*)aed_dev.eerec.ee_log;
	n += sprintf(data + n, "== EXTERNAL EXCEPTION LOG ==\n");
	for (i = 0; i < aed_dev.eerec.ee_log_size / 4; i += 4) {
		n += sprintf(data + n, "0x%08X 0x%08X 0x%08X 0x%08X\n", 
				mem[i], mem[i+1], mem[i+2], mem[i+3]);
	}
	n += sprintf(data + n, "== MEM DUMP(%d) ==\n", aed_dev.eerec.ee_phy_size);

	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_DETAIL;
	rep_msg->arg = AE_PASS_BY_MEM;
	rep_msg->len = n+1;
}

static void ee_gen_coredump_msg(void)
{
	AE_Msg *rep_msg;
	char *data;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	rep_msg = msg_create(&aed_dev.eerec.msg, 256);
    if(rep_msg == NULL)
        return;	

	data = (char*)rep_msg + sizeof(AE_Msg);
	rep_msg->cmdType = AE_RSP;
	rep_msg->cmdId = AE_REQ_COREDUMP;
	rep_msg->arg = 0;
	sprintf(data, "/proc/aed/%s", CURRENT_EE_COREDUMP);
	rep_msg->len = strlen(data) + 1;
}

static void ee_gen_ind_msg(void)
{
	AE_Msg *rep_msg;

	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	rep_msg = msg_create(&aed_dev.eerec.msg, 0);
    if(rep_msg == NULL)
        return;	

	rep_msg->cmdType = AE_IND;
	rep_msg->cmdId = AE_IND_EXP_RAISED;
	rep_msg->arg = AE_EE;
	rep_msg->len = 0;
}

static void ee_destroy_log(void)
{
	struct aed_eerec *pmdrec = &aed_dev.eerec;
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	msg_destroy(&aed_dev.eerec.msg);

	if (pmdrec->ee_phy != NULL) {
		vfree(pmdrec->ee_phy);
		pmdrec->ee_phy = NULL;
	}
	pmdrec->ee_log_size = 0;
	pmdrec->ee_phy_size = 0;

    if (pmdrec->ee_log != NULL) {
        kfree(pmdrec->ee_log);
        /*after this, another ee can enter*/
        pmdrec->ee_log = NULL;
    }
}

static int ee_log_avail(void)
{
	return (aed_dev.eerec.ee_log != NULL);
}

/******************************************************************************
 * AED EE File operations
 *****************************************************************************/
static int aed_ee_open(struct inode *inode, struct file *filp)
{
    ee_destroy_log(); //Destroy last log record
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static int aed_ee_release(struct inode *inode, struct file *filp)
{
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s:%d:%d\n", __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static unsigned int aed_ee_poll(struct file *file, struct poll_table_struct *ptable)
{
	// xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);
	if (ee_log_avail()) {
		return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
	}
	else {
		poll_wait(file, &aed_dev.eewait, ptable);
	}
	return 0;
}

static ssize_t aed_ee_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	return msg_copy_to_user(__func__, aed_dev.eerec.msg, buf, count, f_pos);
}

static ssize_t aed_ee_write(struct file *filp, const char __user *buf, size_t count,
			    loff_t *f_pos)
{
	AE_Msg msg;
	int rsize;

	// recevied a new request means the previous response is unavilable
	// 1. set position to be zero
	// 2. destroy the previous response message
	*f_pos = 0;

	msg_destroy(&aed_dev.eerec.msg);

	// the request must be an *AE_Msg buffer
	if (count != sizeof(AE_Msg)) {
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s: ERR, aed_wirte count=%d\n", __func__, count);
		return -1;
	}

	rsize = copy_from_user(&msg, buf, count);
	if (rsize != 0) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "%s: ERR, copy_from_user rsize=%d\n", __func__, rsize);
		return -1;
	}

	msg_show(__func__, &msg);

	if (msg.cmdType == AE_REQ) {
		if (!ee_log_avail()) {
			ee_gen_notavail_msg();
			return count;
		}
		switch(msg.cmdId) {
			case AE_REQ_CLASS:
				ee_gen_class_msg();
				break;
			case AE_REQ_TYPE:
				ee_gen_type_msg();
				break;
			case AE_REQ_DETAIL:
				ee_gen_detail_msg();
				break;
			case AE_REQ_PROCESS:
				ee_gen_process_msg();
				break;
			case AE_REQ_BACKTRACE:
				ee_gen_notavail_msg();
				break;
			case AE_REQ_COREDUMP:
				ee_gen_coredump_msg();
				break;
			default:
				xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "Unknown command id %d\n", msg.cmdId);
				ee_gen_notavail_msg();
				break;
		}
	}
	else if (msg.cmdType == AE_IND) {
		switch(msg.cmdId) {
			case AE_IND_LOG_CLOSE:
				ee_destroy_log();
				break;
			default:
				// IGNORE
				break;
		}
	}
	else if (msg.cmdType == AE_RSP) { // IGNORE
	}

	return count;
}

static int aed_proc_current_ee_coredump(char *page, char **start,
					off_t off, int count,
					int *eof, void *data)
{
	int len = 0;
	// xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s phy %p size %d off %d count %d\n", __func__, aed_dev.eerec.ee_phy, aed_dev.eerec.ee_phy_size, (int)off, count);

	if (aed_dev.eerec.ee_phy != NULL) {
		if (off >= aed_dev.eerec.ee_phy_size) {
		  return 0;
		}

		len = count;
		if (len > aed_dev.eerec.ee_phy_size - off) {
		  len = aed_dev.eerec.ee_phy_size - off;
		}
		memcpy(page, (char *)aed_dev.eerec.ee_phy + off, len);
		*start = (char *)len;
		if (off + len == aed_dev.eerec.ee_phy_size)
		  *eof = 1;
	}
	else {
		len = sprintf(page, "No current modem exception memory dump\n");
		*eof = 1;
	}
	return len;
}

/******************************************************************************
 * AED KE File operations
 *****************************************************************************/
static int aed_ke_open(struct inode *inode, struct file *filp)
{
	struct aee_oops *oops_open = NULL;

    ke_destroy_log();//Destroy last log record
    xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s:%d:%d\n", 
                __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));

    oops_open = ipanic_oops_copy();
    if (oops_open == NULL){
        return 0;
    }
   	/* The panic log only occur on system startup, so check it now */
	ke_gen_ind_msg(oops_open);
	return 0;
}

static int aed_ke_release(struct inode *inode, struct file *filp)
{
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s:%d:%d\n",
                __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static unsigned int aed_ke_poll(struct file *file, struct poll_table_struct *ptable)
{
	//xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "%s\n", __func__);

	if (ke_log_avail()) {
		return POLLIN | POLLRDNORM | POLLOUT | POLLWRNORM;
	}
	else {
		poll_wait(file, &aed_dev.kewait, ptable);
	}
	return 0;
}

static int aed_proc_current_ke_console(char *page, char **start,
				       off_t off, int count,
				       int *eof, void *data)
{
	int len = 0;
	if (aed_dev.kerec.lastlog) {
		struct aee_oops *oops = aed_dev.kerec.lastlog;
		if (off > oops->console_len) {
		  return 0;
		}

		len = count;
		if (len > oops->console_len - off) {
		  len = oops->console_len - off;
		}
		memcpy(page, oops->console + off, len);
		*start = (char *)len;
		if (off + len == oops->console_len)
		  *eof = 1;
	}
	else {
		len = sprintf(page, "No current kernel exception console\n");
		*eof = 1;
	}
	return len;
}

static int aed_proc_current_ke_userspaceInfo(char *page, char **start,
				       off_t off, int count,
				       int *eof, void *data)
{
	int len = 0;
	if (aed_dev.kerec.lastlog) {
		struct aee_oops *oops = aed_dev.kerec.lastlog;
		if (off > oops->userspace_info_len) {
		  return 0;
		}

		len = count;
		if (len > oops->userspace_info_len - off) {
		  len = oops->userspace_info_len - off;
		}
		memcpy(page, oops->userspace_info + off, len);
		*start = (char *)len;
		if (off + len == oops->userspace_info_len)
		  *eof = 1;
	}
	else {
		len = sprintf(page, "No current kernel aed_proc_current_ke_userspaceInfo\n");
		*eof = 1;
	}
	return len;
}

static int aed_proc_current_ke_android_main(char *page, char **start,
				       off_t off, int count,
				       int *eof, void *data)
{
	int len = 0;
	if (aed_dev.kerec.lastlog) {
		struct aee_oops *oops = aed_dev.kerec.lastlog;
		if (off > oops->android_main_len) {
		  return 0;
		}

		len = count;
		if (len > oops->android_main_len - off) {
		  len = oops->android_main_len - off;
		}
		memcpy(page, oops->android_main + off, len);
		*start = (char *)len;
		if (off + len == oops->android_main_len)
		  *eof = 1;
	}
	else {
		len = sprintf(page, "No current kernel exception android main\n");
		*eof = 1;
	}
	return len;
}

static int aed_proc_current_ke_android_radio(char *page, char **start,
				       off_t off, int count,
				       int *eof, void *data)
{
	int len = 0;
	if (aed_dev.kerec.lastlog) {
		struct aee_oops *oops = aed_dev.kerec.lastlog;
		if (off > oops->android_radio_len) {
		  return 0;
		}

		len = count;
		if (len > oops->android_radio_len - off) {
		  len = oops->android_radio_len - off;
		}
		memcpy(page, oops->android_radio + off, len);
		*start = (char *)len;
		if (off + len == oops->android_radio_len)
		  *eof = 1;
	}
	else {
		len = sprintf(page, "No current kernel exception android radio\n");
		*eof = 1;
	}
	return len;
}
static int aed_proc_current_ke_android_system(char *page, char **start,
				       off_t off, int count,
				       int *eof, void *data)
{
	int len = 0;
	if (aed_dev.kerec.lastlog) {
		struct aee_oops *oops = aed_dev.kerec.lastlog;
		if (off > oops->android_system_len) {
		  return 0;
		}

		len = count;
		if (len > oops->android_system_len - off) {
		  len = oops->android_system_len - off;
		}
		memcpy(page, oops->android_system + off, len);
		*start = (char *)len;
		if (off + len == oops->android_system_len)
		  *eof = 1;
	}
	else {
		len = sprintf(page, "No current kernel exception android system\n");
		*eof = 1;
	}
	return len;
}

static int aed_proc_current_ke_mmprofile(char *page, char **start,
				       off_t off, int count,
				       int *eof, void *data)
{
	int len = 0;
	if (aed_dev.kerec.lastlog) {
		struct aee_oops *oops = aed_dev.kerec.lastlog;
		if (off > oops->mmprofile_len) {
		  return 0;
		}

		len = count;
		if (len > oops->mmprofile_len - off) {
		  len = oops->mmprofile_len - off;
		}
		memcpy(page, oops->mmprofile + off, len);
		*start = (char *)len;
		if (off + len == oops->mmprofile_len)
		  *eof = 1;
	}
	else {
		len = sprintf(page, "No current kernel exception mmprofile\n");
		*eof = 1;
	}
	return len;
}

static ssize_t aed_ke_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	return msg_copy_to_user(__func__, aed_dev.kerec.msg, buf, count, f_pos);
}

static ssize_t aed_ke_write(struct file *filp, const char __user *buf, size_t count,
			    loff_t *f_pos)
{
	AE_Msg msg;
    int rsize;

	// recevied a new request means the previous response is unavilable
	// 1. set position to be zero
	// 2. destroy the previous response message
	*f_pos = 0;
    msg_destroy(&aed_dev.kerec.msg);

    // the request must be an *AE_Msg buffer
	if (count != sizeof(AE_Msg)) {
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "ERR: aed_wirte count=%d\n", count);
		return -1;
	}

	rsize = copy_from_user(&msg, buf, count);
	if (rsize != 0) {
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "copy_from_user rsize=%d\n", rsize);
		return -1;
	}

	msg_show(__func__, &msg);
    
    if (msg.cmdType == AE_REQ) {
		if (!ke_log_avail()) {
			ke_gen_notavail_msg();

            return count;
		}

		switch(msg.cmdId) {
		case AE_REQ_CLASS:
			ke_gen_class_msg();
			break;
		case AE_REQ_TYPE:
			ke_gen_type_msg();
			break;
		case AE_REQ_MODULE:
			ke_gen_module_msg();
			break;
		case AE_REQ_DETAIL:
			ke_gen_detail_msg(&msg);
			break;
		case AE_REQ_PROCESS:
			ke_gen_process_msg();
			break;
		case AE_REQ_BACKTRACE:
			ke_gen_backtrace_msg();
			break;
		default:
			ke_gen_notavail_msg();
			break;
		}
	}
	else if (msg.cmdType == AE_IND) {
		switch(msg.cmdId) {
			case AE_IND_LOG_CLOSE:
				ke_destroy_log();
				break;
			default:
				// IGNORE
				break;
		}
	}
	else if (msg.cmdType == AE_RSP) { // IGNORE
	}

    return count;
}

/*
 * aed process daemon and other command line may access me 
 * concurrently
 */
DEFINE_SEMAPHORE(aed_dal_sem);
static long aed_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	
	if(down_interruptible(&aed_dal_sem) < 0 ) {
		return -ERESTARTSYS;
	}

	switch (cmd) {
	case AEEIOCTL_DAL_SHOW:
	{

        /*It's troublesome to allocate more than 1KB size on stack*/
		struct aee_dal_show *dal_show = kzalloc(sizeof(struct aee_dal_show),  
                                                GFP_KERNEL);
        if(dal_show == NULL) {
            ret = -EFAULT;
			goto EXIT;
        }

		if (copy_from_user(dal_show, (struct aee_dal_show __user *)arg,
				   sizeof(struct aee_dal_show))) {
			ret = -EFAULT;
			goto OUT;
		}
		
		/* Try to prevent overrun */
		dal_show->msg[sizeof(dal_show->msg) - 1] = 0;
		DAL_Printf("%s", dal_show->msg);

   OUT: 
        kfree(dal_show);
        dal_show = NULL;
		goto EXIT;
	}
	
	case AEEIOCTL_DAL_CLEAN:
	{
		/* set default bgcolor to red, it will be used in DAL_Clean */
		struct aee_dal_setcolor dal_setcolor;
		dal_setcolor.foreground = 0x00ff00;			/*green*/
		dal_setcolor.background = 0xff0000;			/*red*/
		DAL_SetColor(dal_setcolor.foreground, dal_setcolor.background);
		
		DAL_Clean();
		break;
	}
	
	case AEEIOCTL_SETCOLOR:
	{

		struct aee_dal_setcolor dal_setcolor;
		
		if (copy_from_user(&dal_setcolor, (struct aee_dal_setcolor __user *)arg,
				   sizeof(struct aee_dal_setcolor))) {
			ret = -EFAULT;
			goto EXIT;
		}
		
		DAL_SetColor(dal_setcolor.foreground, dal_setcolor.background);
		break;
	}
	
	case AEEIOCTL_GET_PROCESS_BT:
	{
		struct aee_process_bt *bt;

		printk("%s: get process backtrace ioctl\n", __func__);

		bt = kzalloc(sizeof(struct aee_process_bt), GFP_KERNEL);
		if (bt == NULL) {
			ret = -ENOMEM;
            goto EXIT;
		}

		if (copy_from_user(bt, (struct aee_process_bt __user *)arg, sizeof(struct aee_process_bt))) {
			kfree(bt);
			ret = -EFAULT;
			goto EXIT;
		}

		ret = aed_get_process_bt(bt);
		if (ret == 0) {
			if (copy_to_user((struct aee_process_bt __user *)arg, bt, sizeof(struct aee_process_bt))) {
				kfree(bt);
				ret = -EFAULT;
				goto EXIT;
			}
		}
		kfree(bt);

		break;
	}
    case AEEIOCTL_GET_SMP_INFO:
	{
        int smp_info;
        
#ifdef CONFIG_SMP
        smp_info =1;
#else
        smp_info =0;
#endif
        if (copy_to_user((void __user *)arg, &smp_info, sizeof(smp_info))) {
			ret = -EFAULT;
			goto EXIT;
		}     
		break;
    }
        default:
		ret = -EINVAL;
	}
	
EXIT:
	up(&aed_dal_sem);
	return ret;
}

static void kernel_reportAPI(const AE_DEFECT_ATTR attr,const int db_opt, const char *module, const char *msg)
{
	struct aee_oops *oops;

	oops = aee_oops_create(attr, AE_KERNEL_PROBLEM_REPORT, module);
    if(NULL !=oops){
        oops->detail = (char *)msg;
    	oops->detail_len = strlen(msg) + 1;
    	oops->dump_option = db_opt;
        
    	xlog_printk(ANDROID_LOG_INFO, AEK_LOG_TAG, "%s,%s,%s,0x%x", __func__, module, msg, db_opt);
    	ke_gen_ind_msg(oops);
    }
}

void aee_kernel_dal_api(const char *file, const int line, const char *msg)
{
	if (down_interruptible(&aed_dal_sem) < 0 ) {
		xlog_printk(ANDROID_LOG_INFO, AEK_LOG_TAG, "ERROR : aee_kernel_dal_api() get aed_dal_sem fail ");
		return;
	}
	xlog_printk(ANDROID_LOG_INFO, AEK_LOG_TAG, "aee_kernel_dal_api : <%s:%d> %s ", file, line, msg);
	if (msg != NULL) {
		struct aee_dal_setcolor dal_setcolor;
		struct aee_dal_show *dal_show = kzalloc(sizeof(struct aee_dal_show), GFP_KERNEL);
		if (dal_show == NULL) {
			xlog_printk(ANDROID_LOG_INFO, AEK_LOG_TAG, "ERROR : aee_kernel_dal_api() kzalloc fail ");
			up(&aed_dal_sem);
			return;
		}
		dal_setcolor.foreground = 0xff00ff; // fg: purple
		dal_setcolor.background = 0x00ff00; // bg: green
		DAL_SetColor(dal_setcolor.foreground, dal_setcolor.background);
		strncpy(dal_show->msg, msg, sizeof(struct aee_dal_show));
		dal_show->msg[sizeof(dal_show->msg) - 1] = 0;
		DAL_Printf("%s", dal_show->msg);
		kfree(dal_show);
	}	
	up(&aed_dal_sem);
}
EXPORT_SYMBOL(aee_kernel_dal_api);

static void external_exception(const char *assert_type, const int *log, int log_size, const int *phy, int phy_size, const char *detail)
{
    int *ee_log = NULL;
    unsigned long flags = 0;

    xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, 
                "%s : [%s] log size %d phy ptr %p size %d\n", __func__,
                assert_type, log_size, phy, phy_size);

    if ((log_size > 0) && (log != NULL)) {
		aed_dev.eerec.ee_log_size = log_size;
		ee_log = (int*)kmalloc(log_size, GFP_ATOMIC);
		if(NULL != ee_log)
            memcpy(ee_log, log, log_size);
	} else {
		aed_dev.eerec.ee_log_size = 16;
		ee_log = (int*)kzalloc(aed_dev.eerec.ee_log_size, GFP_ATOMIC);
	}

    if(NULL == ee_log){
        xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "%s : memory alloc() fail\n", __func__);
		return;
    }
    
    /*
       Don't lock the whole function for the time is uncertain.
       we rely on the fact that ee_log is not null if race here!
      */
    spin_lock_irqsave(&aed_device_lock, flags);
    
    if (aed_dev.eerec.ee_log == NULL) {
        aed_dev.eerec.ee_log = ee_log;
    } else {
        /*no EE before aee_ee_write destroy the ee log*/
        kfree(ee_log);
        spin_unlock_irqrestore(&aed_device_lock, flags);
        xlog_printk(ANDROID_LOG_WARN, AEK_LOG_TAG, "%s: More than one EE message queued\n", __func__);
        return;
    }
    spin_unlock_irqrestore(&aed_device_lock, flags);

	memset(aed_dev.eerec.assert_type, 0, sizeof(aed_dev.eerec.assert_type));
	strncpy(aed_dev.eerec.assert_type, assert_type, sizeof(aed_dev.eerec.assert_type)-1);
	memset(aed_dev.eerec.exp_filename, 0, sizeof(aed_dev.eerec.exp_filename));
	strncpy(aed_dev.eerec.exp_filename, detail, sizeof(aed_dev.eerec.exp_filename)-1);
    xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "EE [%s] \n", aed_dev.eerec.assert_type);

	aed_dev.eerec.exp_linenum = 0;
	aed_dev.eerec.fatal1 = 0;
	aed_dev.eerec.fatal2 = 0;	

	/* Check if we can dump memory */
	if (in_interrupt()) {
		/* kernel vamlloc cannot be used in interrupt context */
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "Modem exception occur in interrupt context, no modem coredump");
		phy_size = 0;
	} 
	else if ((phy < 0) || (phy_size > MAX_EE_COREDUMP)) {
		xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "Modem Physical memory size(%d) too large or invalid", phy_size);
		phy_size = 0;
	}

	if (phy_size > 0) {
		aed_dev.eerec.ee_phy = (int*)vmalloc_user(phy_size);
		if (aed_dev.eerec.ee_phy != NULL) {
			memcpy(aed_dev.eerec.ee_phy, phy, phy_size);
			aed_dev.eerec.ee_phy_size = phy_size;
		}
		else {
			xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "Losing ee phy mem due to vmalloc return NULL");
			aed_dev.eerec.ee_phy_size = 0;
		}
	}
	else {
		aed_dev.eerec.ee_phy = NULL;
		aed_dev.eerec.ee_phy_size = 0;
	}
	ee_gen_ind_msg();
	xlog_printk(ANDROID_LOG_DEBUG, AEK_LOG_TAG, "external_exception out\n") ;
	wake_up(&aed_dev.eewait);
}


static struct aee_kernel_api kernel_api = {
	.kernel_reportAPI 	= kernel_reportAPI,
	.md_exception       = external_exception,
	.combo_exception    = external_exception
};
extern int aee_register_api(struct aee_kernel_api *aee_api);

static int aed_proc_init(void) 
{
	aed_proc_dir = proc_mkdir("aed", NULL);
	if(aed_proc_dir == NULL) {
	  xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed proc_mkdir failed\n");
	  return -ENOMEM;
	}

	aed_proc_current_ke_console_file = create_proc_read_entry(CURRENT_KE_CONSOLE, 
								  0444, aed_proc_dir, 
								  aed_proc_current_ke_console,
								  NULL);
	if (aed_proc_current_ke_console_file == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at %s\n", CURRENT_KE_CONSOLE);
		return -ENOMEM;
	}

	aed_proc_current_ke_userspaceInfo_file = create_proc_read_entry(CURRENT_KE_USERSPACE_INFO, 
								  0444, aed_proc_dir, 
								  aed_proc_current_ke_userspaceInfo,
								  NULL);
	if (aed_proc_current_ke_userspaceInfo_file == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed aed_proc_current_ke_userspaceInfo_file failed at %s\n", CURRENT_KE_USERSPACE_INFO);
		return -ENOMEM;
	}
	
	aed_proc_current_ke_android_main_file = create_proc_read_entry(CURRENT_KE_ANDROID_MAIN, 
								  0444, aed_proc_dir, 
								  aed_proc_current_ke_android_main,
								  NULL);
	if (aed_proc_current_ke_android_main_file == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at %s\n", CURRENT_KE_ANDROID_MAIN);
		return -ENOMEM;
	}
	
	aed_proc_current_ke_android_radio_file = create_proc_read_entry(CURRENT_KE_ANDROID_RADIO, 
								  0444, aed_proc_dir, 
								  aed_proc_current_ke_android_radio,
								  NULL);
	if (aed_proc_current_ke_android_radio_file == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at %s\n", CURRENT_KE_ANDROID_RADIO);
		return -ENOMEM;
	}	
	
	aed_proc_current_ke_android_system_file = create_proc_read_entry(CURRENT_KE_ANDROID_SYSTEM, 
								  0444, aed_proc_dir, 
								  aed_proc_current_ke_android_system,
								  NULL);
	if (aed_proc_current_ke_android_system_file == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at %s\n", CURRENT_KE_ANDROID_SYSTEM);
		return -ENOMEM;
	}
	
	aed_proc_current_ke_mmprofile_file = create_proc_read_entry(CURRENT_KE_MMPROFILE, 
								  0444, aed_proc_dir, 
								  aed_proc_current_ke_mmprofile,
								  NULL);
	if (aed_proc_current_ke_mmprofile_file == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at %s\n", CURRENT_KE_MMPROFILE);
		return -ENOMEM;
	}	

	aed_proc_current_ee_coredump_file = create_proc_read_entry(CURRENT_EE_COREDUMP, 
								   0444, aed_proc_dir, 
								   aed_proc_current_ee_coredump,
								   NULL);
	if (aed_proc_current_ee_coredump_file == NULL) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aed create_proc_read_entry failed at %s\n", CURRENT_EE_COREDUMP);
		return -ENOMEM;
	}

	aee_rr_proc_init(aed_proc_dir);

	aed_proc_debug_init(aed_proc_dir);

	return 0;
}

static int aed_proc_done(void)
{
	remove_proc_entry(CURRENT_KE_CONSOLE, aed_proc_dir);
	remove_proc_entry(CURRENT_EE_COREDUMP, aed_proc_dir);

	aed_proc_debug_done(aed_proc_dir);

	remove_proc_entry("aed", NULL);
	return 0;
}

/******************************************************************************
 * Module related
 *****************************************************************************/
static struct file_operations aed_ee_fops = {
	.owner   = THIS_MODULE,
	.open    = aed_ee_open,
	.release = aed_ee_release,
	.poll    = aed_ee_poll,
	.read    = aed_ee_read,
	.write   = aed_ee_write,
	.unlocked_ioctl   = aed_ioctl,
};

static struct file_operations aed_ke_fops = {
	.owner   = THIS_MODULE,
	.open    = aed_ke_open,
	.release = aed_ke_release,
	.poll    = aed_ke_poll,
	.read    = aed_ke_read,
	.write   = aed_ke_write,
	.unlocked_ioctl   = aed_ioctl,
};

static struct miscdevice aed_ee_dev = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = "aed0",
    .fops    = &aed_ee_fops,
};

static struct miscdevice aed_ke_dev = {
    .minor   = MISC_DYNAMIC_MINOR,
    .name    = "aed1",
    .fops    = &aed_ke_fops,
};

static int __init aed_init(void)
{
	int err = 0;
	err = aed_proc_init();
	if (err != 0)
		return err;

	memset(&aed_dev.eerec, 0, sizeof(struct aed_eerec));
	init_waitqueue_head(&aed_dev.eewait);
	memset(&aed_dev.kerec, 0, sizeof(struct aed_kerec));
	init_waitqueue_head(&aed_dev.kewait);

	aee_register_api(&kernel_api);

    spin_lock_init(&aed_device_lock);
	err = misc_register(&aed_ee_dev);
	if (unlikely(err)) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aee: failed to register aed0(ee) device!\n");
		return err;
	}

	err = misc_register(&aed_ke_dev);
	if(unlikely(err)) {
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "aee: failed to register aed1(ke) device!\n");
		return err;
	}

	return err;
}

static void __exit aed_exit(void)
{
	int err;

	err = misc_deregister(&aed_ee_dev);
	if (unlikely(err))
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "xLog: failed to unregister aed(ee) device!\n");
	err = misc_deregister(&aed_ke_dev);
	if (unlikely(err))
		xlog_printk(ANDROID_LOG_ERROR, AEK_LOG_TAG, "xLog: failed to unregister aed(ke) device!\n");

	ee_destroy_log();
	ke_destroy_log();

	aed_proc_done();
}

module_init(aed_init);
module_exit(aed_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek AED Driver");
MODULE_AUTHOR("MediaTek Inc.");
