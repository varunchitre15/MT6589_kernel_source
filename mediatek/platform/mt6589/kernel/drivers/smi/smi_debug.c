#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/aee.h>
#include <linux/timer.h>
#include <asm/system.h>
#include <asm-generic/irq_regs.h>
#include <asm/mach/map.h>
#include <mach/sync_write.h>
#include <mach/irqs.h>
#include <asm/cacheflush.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/fb.h>
#include <linux/debugfs.h>
#include <mach/mt_typedefs.h>
#include <mach/m4u.h>
#include <mach/mt_smi.h>

#include "smi_common.h"

#include <linux/xlog.h>

#include "smi_reg.h"

#define SMI_LOG_TAG "smi"

static char debug_buffer[4096];


static char STR_HELP[] =
    "\n"
    "USAGE\n"
    "        echo [ACTION]... > mau_dbg\n"
    "\n"
    "ACTION\n"
    "        module1|module2?R/W/RW(startPhyAddr,endPhyAddr)@MAU_Enty_ID\n"
    "             MAU will monitor specified module whether R/W specified range of memory\n"
    "             example: echo tvc|lcd_r?R(0,0x1000)@1 > mau_dbg\n"
    "             you can use [all] to specify all modules\n"
    "             example: echo all?W(0x2000,0x9000)@2 > mau_dbg\n"
    "\n"
    "        module1|module2@MAU_Enty_ID:off\n"
    "             Turn off specified module on specified MAU Entry\n"
    "             example: echo tvc|lcd_r@1:off > mau_dbg\n"
    "\n"
    "\n"
    "        all:off\n"
    "             Turn off all of modules\n"
    "             example: echo all:off > mau_dbg\n"
    "\n"
    "        list modules\n"
    "             list all module names MAU could monitor\n"
    "\n"
    "        reg:[MPU|MAU1|MAU2]\n"
    "             dump hw register values\n"
    "\n"
    "        regw:addr=val\n"
    "             write hw register\n"
    "\n"
    "        regr:addr\n"
    "             read hw register\n"
    "\n"
    "        m4u_log:on\n"
    "             start to print m4u translate miss rate every second \n"
    "\n"
    "        m4u_log:off\n"
    "             stop to print m4u translate miss rate every second \n"
    "\n"
    "        m4u_debug:[command] \n"
    "             input a command, used for debug \n"
    "\n"
    "        m4u_monitor:on\n"
    "             start to print m4u translate miss rate every second \n"
    "\n"
    "        m4u_monitor:off\n"
    "             stop to print m4u translate miss rate every second \n";


static void process_dbg_opt(const char *opt)
{
    //m4u log
    if (0 == strncmp(opt, "m4u_log:", 8))
    {
        if (0 == strncmp(opt + 8, "on", 2)) 
            m4u_log_on();
        else if (0 == strncmp(opt + 8, "off", 3)) 
            m4u_log_off();
        else
            goto Error;
    }
    //m4u debug
    if (0 == strncmp(opt, "m4u_debug:", 10))
    {
        unsigned int command;
        char *p = (char *)opt + 10;
        command = (unsigned int) simple_strtoul(p, &p, 10);
        SMIMSG("m4u_debug_command, command=%d ", command);
        m4u_debug_command(command);
    }

    //mau dump
    if (0 == strncmp(opt, "mau_stat:", 9))
    {
		char *p = (char *)opt + 9;
        unsigned int larb=(unsigned int)simple_strtoul(p, &p, 16);
        if(larb>SMI_LARB_NR)
            SMIERR("debug error: larb=%d\n", larb);
        mau_dump_status(larb);
    }

    if (0 == strncmp(opt, "mau_config:", 11 ))
    {
        MTK_MAU_CONFIG MauConf;
		unsigned int larb,entry, rd, wt, vir, start, end, port_msk;
		char *p = (char *)opt + 11;
		larb = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		entry = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		rd = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		wt = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		vir = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		start = (unsigned int) simple_strtoul(p, &p, 16);
        p++;
		end = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		port_msk = (unsigned int) simple_strtoul(p, &p, 16);

		SMIMSG("larb=%d,entry=%d,rd=%d wt=%d vir=%d \n"
            "start=0x%x end=0x%x msk=0x%x \n",
            larb, entry, rd, wt, vir, start, end, port_msk);

        MauConf.larb = larb;
        MauConf.entry = entry;
        MauConf.monitor_read = rd;
        MauConf.monitor_write = wt;
        MauConf.virt = vir;
        MauConf.start = start;
        MauConf.end = end;
        MauConf.port_msk = port_msk;
        mau_config(&MauConf);

    }

    if (0 == strncmp(opt, "spc_config:", 11 ))
    {
        MTK_SPC_CONFIG pCfg;
		char *p = (char *)opt + 11;

        SMIMSG("%s", p);
        //0-no protect; 1-sec rw; 2-sec_rw nonsec_r; 3-no access
		pCfg.domain_0_prot = (unsigned int) simple_strtoul(p, &p, 16);

        SMIMSG("%d,%s", pCfg.domain_0_prot, p);
		p++;
		pCfg.domain_1_prot = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
        SMIMSG("%d", pCfg.domain_1_prot);
		pCfg.domain_2_prot = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
        SMIMSG("%d", pCfg.domain_2_prot);
		pCfg.domain_3_prot = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
        SMIMSG("%d", pCfg.domain_3_prot);
		pCfg.start = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
        SMIMSG("%d", pCfg.domain_0_prot);
		pCfg.end = (unsigned int) simple_strtoul(p, &p, 16);

		SMIMSG("prot=(%d,%d,%d,%d), start=0x%x, end=0x%x\n",
            pCfg.domain_0_prot,pCfg.domain_1_prot,
            pCfg.domain_2_prot,pCfg.domain_3_prot,
            pCfg.start,pCfg.end);

        spc_config(&pCfg);

    }

    if (0 == strncmp(opt, "spc_status", 10 ))
    {
        spc_status_check();
    }

    if (0 == strncmp(opt, "spc_dump_reg", 12 ))
    {
        spc_dump_reg();
    }

    if (0 == strncmp(opt, "touch_sysram", 10 ))
    {
        volatile unsigned int *va;
        unsigned int i;

        //va = ioremap_nocache(0x1200C000, 1024*80);
        va=(volatile unsigned int *)0xf2000000;

        for(i=0; i<1024*80/4; i++)
        {
            va[i] = i;
        }
        
        SMIMSG("cpu read sysram: 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x",
            va[0],va[1],va[2],va[3],va[100],va[222],va[444]);
            
    }

    if (0 == strncmp(opt, "set_reg:", 8 ))
    {
        unsigned int addr, val;
		char *p = (char *)opt + 8;

		addr = (unsigned int) simple_strtoul(p, &p, 16);
		p++;
		val = (unsigned int) simple_strtoul(p, &p, 16);

		SMIMSG("set register: 0x%x = 0x%x\n", addr, val);

        COM_WriteReg32(addr, val);
    }
    if (0 == strncmp(opt, "get_reg:", 8 ))
    {
        unsigned int addr;
		char *p = (char *)opt + 8;

		addr = (unsigned int) simple_strtoul(p, &p, 16);

		SMIMSG("get register: 0x%x = 0x%x \n", addr, COM_ReadReg32(addr));
    }
    

    
    return;
Error:
    SMIERR("parse command error!\n");
    SMIMSG("%s", STR_HELP);
}


static void process_dbg_cmd(char *cmd)
{
    char *tok;
    while ((tok = strsep(&cmd, " ")) != NULL)
    {
        process_dbg_opt(tok);
    }
}


// ---------------------------------------------------------------------------
//  Debug FileSystem Routines
// ---------------------------------------------------------------------------

struct dentry *smi_dbgfs = NULL;


static ssize_t debug_open(struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}

static ssize_t debug_read(struct file *file,
                          char __user *ubuf, size_t count, loff_t *ppos)
{
    int n = 0;
    return simple_read_from_buffer(ubuf, count, ppos, debug_buffer, n);
}


static ssize_t debug_write(struct file *file,
                           const char __user *ubuf, size_t count, loff_t *ppos)
{
    const int debug_bufmax = sizeof(debug_buffer) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax)
        count = debug_bufmax;

	if (copy_from_user(&debug_buffer, ubuf, count))
		return -EFAULT;

	debug_buffer[count] = 0;

    process_dbg_cmd(debug_buffer);

    return ret;
}


static struct file_operations debug_fops = {
	.read  = debug_read,
    .write = debug_write,
	.open  = debug_open,
};


void SMI_DBG_Init(void)
{
    smi_dbgfs = debugfs_create_file("smi",
        S_IFREG|S_IRUGO, NULL, (void *)0, &debug_fops);
}


void SMI_DBG_Deinit(void)
{
    debugfs_remove(smi_dbgfs);
}


