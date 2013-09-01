#include <linux/string.h>
#include <linux/uaccess.h>

#include <linux/debugfs.h>

#include <linux/aee.h>
#include <linux/disp_assert_layer.h>
//#include <linux/unistd.h>
//#include <linux/fcntl.h>

#include <linux/dma-mapping.h>
#include "ddp_debug.h"
#include "ddp_reg.h"
#include "ddp_bls.h"
#include "ddp_color.h"
#include "ddp_drv.h"
#include "ddp_dpfd.h"
#include "ddp_rot.h"
#include "ddp_scl.h"
#include "ddp_wdma.h"
#include "ddp_path.h"

#include "disp_drv.h"
#include "ddp_dpfd.h"
#include "../m4u/m4u.h"
// ---------------------------------------------------------------------------
//  External variable declarations
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
//  Debug Options
// ---------------------------------------------------------------------------

struct DDP_MMP_Events_t DDP_MMP_Events;

struct dentry *debugfs = NULL;

static const long int DEFAULT_LOG_FPS_WND_SIZE = 30;


unsigned char pq_debug_flag=0;
unsigned char aal_debug_flag=0;


static char STR_HELP[] =
    "\n"
    "USAGE\n"
    "        echo [ACTION]... > mtkaal\n"
    "\n"
    "ACTION\n"
    "       regr:addr\n"
    "\n"
    "       regw:addr,value\n"
    "\n"
    "       dbg_log:0|1\n"
    "\n"
    "       irq_log:0|1\n"
    "\n"    
    "       backlight:level\n"
    "\n"
    "       dump_aal:arg\n"
    "\n"
    "       mmp\n"
    "\n"    
    "       dump_reg:moduleID\n"
    "\n"    
    "       dpfd_ut1:channel\n"
    ;

void init_ddp_mmp_events(void)
{
    if (DDP_MMP_Events.DDP == 0)
    {
        DDP_MMP_Events.DDP = MMProfileRegisterEvent(MMP_RootEvent, "DDP");
        DDP_MMP_Events.MutexParent = MMProfileRegisterEvent(DDP_MMP_Events.DDP, "Mutex");
        DDP_MMP_Events.Mutex[0] = MMProfileRegisterEvent(DDP_MMP_Events.MutexParent, "Mutex0");
        DDP_MMP_Events.Mutex[1] = MMProfileRegisterEvent(DDP_MMP_Events.MutexParent, "Mutex1");
        DDP_MMP_Events.Mutex[2] = MMProfileRegisterEvent(DDP_MMP_Events.MutexParent, "Mutex2");
        DDP_MMP_Events.Mutex[3] = MMProfileRegisterEvent(DDP_MMP_Events.MutexParent, "Mutex3");
        DDP_MMP_Events.Mutex[4] = MMProfileRegisterEvent(DDP_MMP_Events.MutexParent, "Mutex4");
        DDP_MMP_Events.Mutex[5] = MMProfileRegisterEvent(DDP_MMP_Events.MutexParent, "Mutex5");
        DDP_MMP_Events.BackupReg = MMProfileRegisterEvent(DDP_MMP_Events.DDP, "BackupReg");
        DDP_MMP_Events.DDP_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP, "DDP_IRQ");
        DDP_MMP_Events.SCL_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "SCL_IRQ");
        DDP_MMP_Events.ROT_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "ROT_IRQ");
        DDP_MMP_Events.OVL_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "OVL_IRQ");
        DDP_MMP_Events.WDMA0_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "WDMA0_IRQ");
        DDP_MMP_Events.WDMA1_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "WDMA1_IRQ");
        DDP_MMP_Events.RDMA0_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "RDMA0_IRQ");
        DDP_MMP_Events.RDMA1_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "RDMA1_IRQ");
        DDP_MMP_Events.COLOR_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "COLOR_IRQ");
        DDP_MMP_Events.BLS_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "BLS_IRQ");
        DDP_MMP_Events.TDSHP_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "TDSHP_IRQ");
        DDP_MMP_Events.CMDQ_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "CMDQ_IRQ");
        DDP_MMP_Events.Mutex_IRQ = MMProfileRegisterEvent(DDP_MMP_Events.DDP_IRQ, "Mutex_IRQ");
        DDP_MMP_Events.WAIT_INTR = MMProfileRegisterEvent(DDP_MMP_Events.DDP, "WAIT_IRQ");
        DDP_MMP_Events.Debug = MMProfileRegisterEvent(DDP_MMP_Events.DDP, "Debug");

        MMProfileEnableEventRecursive(DDP_MMP_Events.DDP, 1);
        //MMProfileEnableEventRecursive(DDP_MMP_Events.BackupReg, 1);
        MMProfileEnableEventRecursive(DDP_MMP_Events.DDP_IRQ, 0);
        //MMProfileEnableEventRecursive(DDP_MMP_Events.WAIT_INTR, 1);
    }
}

// ---------------------------------------------------------------------------
//  Command Processor
// ---------------------------------------------------------------------------
static char dbg_buf[2048];

static void process_dbg_opt(const char *opt)
{
    char *buf = dbg_buf + strlen(dbg_buf);
    if (0 == strncmp(opt, "regr:", 5))
    {
        char *p = (char *)opt + 5;
        unsigned int addr = (unsigned int) simple_strtoul(p, &p, 16);

        if (addr) 
        {
            unsigned int regVal = DISP_REG_GET(addr);
            DISP_MSG("regr: 0x%08X = 0x%08X\n", addr, regVal);
            sprintf(buf, "regr: 0x%08X = 0x%08X\n", addr, regVal);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "regw:", 5))
    {
        char *p = (char *)opt + 5;
        unsigned int addr = (unsigned int) simple_strtoul(p, &p, 16);
        unsigned int val = (unsigned int) simple_strtoul(p + 1, &p, 16);
        if (addr) 
        {
            unsigned int regVal;
            DISP_REG_SET(addr, val);
            regVal = DISP_REG_GET(addr);
            DISP_DBG("regw: 0x%08X, 0x%08X = 0x%08X\n", addr, val, regVal);
            sprintf(buf, "regw: 0x%08X, 0x%08X = 0x%08X\n", addr, val, regVal);
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "dbg_log:", 8))
    {
        char *p = (char *)opt + 8;
        unsigned int enable = (unsigned int) simple_strtoul(p, &p, 10);
        if (enable)
            dbg_log = 1;
        else
            dbg_log = 0;

        sprintf(buf, "dbg_log: %d\n", dbg_log);
    }
    else if (0 == strncmp(opt, "irq_log:", 8))
    {
        char *p = (char *)opt + 8;
        unsigned int enable = (unsigned int) simple_strtoul(p, &p, 10);
        if (enable)
            irq_log = 1;
        else
            irq_log = 0;
        
        sprintf(buf, "irq_log: %d\n", irq_log);        
    }
    else if (0 == strncmp(opt, "backlight:", 10))
    {
        char *p = (char *)opt + 10;
        unsigned int level = (unsigned int) simple_strtoul(p, &p, 10);

        if (level) 
        {
            disp_bls_set_backlight(level);            
            sprintf(buf, "backlight: %d\n", level); 
        } else {
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "dump_reg:", 9))
    {
        char *p = (char *)opt + 9;
        unsigned int module = (unsigned int) simple_strtoul(p, &p, 10);
        DISP_MSG("process_dbg_opt, module=%d \n", module);
        if (module<DISP_MODULE_MAX) 
        {
            disp_dump_reg(module);            
            sprintf(buf, "dump_reg: %d\n", module); 
        } else {
            DISP_MSG("process_dbg_opt2, module=%d \n", module);
            goto Error;
        }
    }
    else if (0 == strncmp(opt, "dump_aal:", 9))
    {        
        char *p = (char *)opt + 9;
        unsigned int arg = (unsigned int) simple_strtoul(p, &p, 10);
        if (arg == 0)
        {
            int i;
            unsigned int hist[LUMA_HIST_BIN];
            disp_get_hist(hist);
            for (i = 0; i < LUMA_HIST_BIN; i++)
            {
                DISP_DBG("LUMA_HIST_%02d: %d\n", i, hist[i]);
                sprintf(dbg_buf + strlen(dbg_buf), "LUMA_HIST_%2d: %d\n", i, hist[i]);
            }
        }
        else if (arg == 1)
        {
            int i;
            DISP_AAL_PARAM param;
            
            GetUpdateMutex();
            memcpy(&param, get_aal_config(), sizeof(DISP_AAL_PARAM));
            ReleaseUpdateMutex();

            DISP_DBG("pwmDuty: %lu\n", param.pwmDuty);
            sprintf(dbg_buf + strlen(dbg_buf), "pwmDuty: %lu\n", param.pwmDuty);
            for (i = 0; i < LUMA_CURVE_POINT; i++)
            {
                DISP_DBG("lumaCurve[%02d]: %lu\n", i, param.lumaCurve[i]);
                sprintf(dbg_buf + strlen(dbg_buf), "lumaCurve[%02d]: %lu\n", i, param.lumaCurve[i]);
            }
        }
    }
    else if (0 == strncmp(opt, "debug:", 6))
    {
        char *p = (char *)opt + 6;
        unsigned int enable = (unsigned int) simple_strtoul(p, &p, 10);
        if(enable==1)
        {
            printk("[DDP] debug=1, trigger AEE\n");
            aee_kernel_exception("DDP-TEST-ASSERT", "[DDP] DDP-TEST-ASSERT");
        }
        else if(enable==2)
        {
           ddp_mem_test();
        }
        else if(enable==3)
        {
           ddp_mem_test2();
        }
    }
    else if (0 == strncmp(opt, "mmp", 3))
    {
        init_ddp_mmp_events();
    }
    else if (0 == strncmp(opt, "dpfd_ut1:", 9))
    {
        char *p = (char *)opt + 9;
        unsigned int channel = (unsigned int) simple_strtoul(p, &p, 10);        
        ddpk_testfunc_1(channel);
    }
    else if (0 == strncmp(opt, "dpfd_ut2:", 9))
    {
        char *p = (char *)opt + 9;
        unsigned int channel = (unsigned int) simple_strtoul(p, &p, 10);        
        ddpk_testfunc_2(channel);
    }
    else if (0 == strncmp(opt, "dpfd:log", 8))
    {
    }
    else if (0 == strncmp(opt, "pqon", 4))
    {
        pq_debug_flag=0;
        sprintf(buf, "Turn on PQ %d\n", pq_debug_flag);
    }
    else if (0 == strncmp(opt, "pqoff", 5))
    {
        pq_debug_flag=1;
        sprintf(buf, "Turn off PQ %d\n", pq_debug_flag);        
    }
    else if (0 == strncmp(opt, "pqdemo", 6))
    {
        pq_debug_flag=2;
        sprintf(buf, "Turn on PQ (demo) %d\n", pq_debug_flag);    
    }
    else if (0 == strncmp(opt, "pqstop", 6))
    {
        pq_debug_flag=3;
        sprintf(buf, "Stop mutex update %d\n", pq_debug_flag);    
    }
    else if (0 == strncmp(opt, "aalon", 5))
    {
        aal_debug_flag=0;
        sprintf(buf, "resume aal update %d\n", aal_debug_flag);    
    }
    else if (0 == strncmp(opt, "aaloff", 6))
    {
        aal_debug_flag=1;
        sprintf(buf, "suspend aal update %d\n", aal_debug_flag);    
    }
    else if (0 == strncmp(opt, "color_win:", 10))
    {
        char *p = (char *)opt + 10;
        unsigned int sat_upper, sat_lower, hue_upper, hue_lower;
        sat_upper = (unsigned int) simple_strtoul(p, &p, 10);
        p++;
        sat_lower = (unsigned int) simple_strtoul(p, &p, 10);
        p++;
        hue_upper = (unsigned int) simple_strtoul(p, &p, 10);
        p++;
        hue_lower = (unsigned int) simple_strtoul(p, &p, 10);
        DISP_MSG("Set color_win: %u, %u, %u, %u\n", sat_upper, sat_lower, hue_upper, hue_lower);
        disp_color_set_window(sat_upper, sat_lower, hue_upper, hue_lower);
    }
    else
    {
	    goto Error;
    }

    return;

Error:
    DISP_ERR("parse command error!\n%s\n\n%s", opt, STR_HELP);
}


static void process_dbg_cmd(char *cmd)
{
    char *tok;
    
    DISP_DBG("cmd: %s\n", cmd);
    memset(dbg_buf, 0, sizeof(dbg_buf));
    while ((tok = strsep(&cmd, " ")) != NULL)
    {
        process_dbg_opt(tok);
    }
}


// ---------------------------------------------------------------------------
//  Debug FileSystem Routines
// ---------------------------------------------------------------------------

static ssize_t debug_open(struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}


static char cmd_buf[512];

static ssize_t debug_read(struct file *file,
                          char __user *ubuf, size_t count, loff_t *ppos)
{
    if (strlen(dbg_buf))
        return simple_read_from_buffer(ubuf, count, ppos, dbg_buf, strlen(dbg_buf));
    else
        return simple_read_from_buffer(ubuf, count, ppos, STR_HELP, strlen(STR_HELP));
        
}


static ssize_t debug_write(struct file *file,
                           const char __user *ubuf, size_t count, loff_t *ppos)
{
    const int debug_bufmax = sizeof(cmd_buf) - 1;
	size_t ret;

	ret = count;

	if (count > debug_bufmax) 
        count = debug_bufmax;

	if (copy_from_user(&cmd_buf, ubuf, count))
		return -EFAULT;

	cmd_buf[count] = 0;

    process_dbg_cmd(cmd_buf);

    return ret;
}


static struct file_operations debug_fops = {
	.read  = debug_read,
    .write = debug_write,
	.open  = debug_open,
};


void ddp_debug_init(void)
{
    debugfs = debugfs_create_file("dispsys",
        S_IFREG|S_IRUGO, NULL, (void *)0, &debug_fops);
}


void ddp_debug_exit(void)
{
    debugfs_remove(debugfs);
}

#include <linux/vmalloc.h>
#define DDP_TEST_WIDTH 64
#define DDP_TEST_HEIGHT 64
#define DDP_TEST_BPP 3
#define DDP_MUTEX_FOR_ROT_SCL_WDMA 1
extern unsigned char data_rgb888_64x64[12288];
extern unsigned char data_rgb888_64x64_golden[12288];
int ddp_mem_test2(void)
{  
    unsigned int* pSrc;
    unsigned int* pDst;
    int result = 0;
    DdpkBitbltConfig pddp;
    
    pSrc= vmalloc(DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP);
    if(pSrc==0)
    {
        printk("[DDP] error: dma_alloc_coherent error!  dma memory not available.\n");
        return 0;
    }
    else
    {
        printk("[ddp] pSrc=0x%x \n", (unsigned int)pSrc);
    }
    memcpy((void*)pSrc, data_rgb888_64x64, DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP);
    
    pDst= vmalloc(DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP);
    if(pDst==0)
    {
        printk("[DDP] error: dma_alloc_coherent error!  dma memory not available.\n");
        return 0;
    }
    else
    {
        printk("[ddp] pDst=0x%x\n", (unsigned int)pDst);
    }
    memset((void*)pDst, 0, DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP);

    /*
    disp_power_on(DISP_MODULE_ROT);
    disp_power_on(DISP_MODULE_SCL);
    disp_power_on(DISP_MODULE_WDMA0); 
   */
    // config port to virtual
    {
        M4U_PORT_STRUCT sPort;
        sPort.ePortID = M4U_PORT_ROT_EXT;
        sPort.Virtuality = 1; 					   
        sPort.Security = 0;
        sPort.Distance = 1;
        sPort.Direction = 0;
        m4u_config_port(&sPort);
        
        sPort.ePortID = M4U_PORT_WDMA0;
        sPort.Virtuality = 1; 					   
        sPort.Security = 0;
        sPort.Distance = 1;
        sPort.Direction = 0;
        m4u_config_port(&sPort);
	  }
	      
    //config

    pddp.srcX = 0;
    pddp.srcY = 0;
    pddp.srcW = DDP_TEST_WIDTH;
    pddp.srcWStride = DDP_TEST_WIDTH;
    pddp.srcH = DDP_TEST_HEIGHT;
    pddp.srcHStride = DDP_TEST_HEIGHT;
    pddp.srcAddr[0] = (unsigned int)pSrc;
    pddp.srcFormat = eRGB888_K;
    pddp.srcPlaneNum = 1;
    pddp.srcBufferSize[0] = DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP;
    
    pddp.dstX = 0;
    pddp.dstY = 0;
    pddp.dstW = DDP_TEST_WIDTH;
    pddp.dstWStride = DDP_TEST_WIDTH;
    pddp.dstH = DDP_TEST_HEIGHT;
    pddp.dstHStride = DDP_TEST_HEIGHT;
    pddp.dstAddr[0] = (unsigned int)pDst;
    pddp.dstFormat = eRGB888_K;
    pddp.pitch = DDP_TEST_WIDTH;
    pddp.dstPlaneNum = 1;
    pddp.dstBufferSize[0] = DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP;
    pddp.orientation = 0;   
    result = DDPK_Bitblt_Config( DDPK_CH_HDMI_0, &pddp );
    if(result)
    {
        printk("[DDP] error: DDPK_Bitblt_Config fail!, ret=%d\n", result);
    }

    printk("DDP, DDPK_Bitblt module setting: \n");
    disp_dump_reg(DISP_MODULE_ROT);
    disp_dump_reg(DISP_MODULE_SCL);
    disp_dump_reg(DISP_MODULE_WDMA0);
    disp_dump_reg(DISP_MODULE_CONFIG);
    
    result = DDPK_Bitblt( DDPK_CH_HDMI_0);
    if(result)
    {
        printk("[DDP] error: DDPK_Bitblt() fail, result=%d \n", result);
    }

        
    // result verify
    {
        unsigned int diff_cnt = 0;
        unsigned int t=0;
        unsigned int size = DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP;
        for(t=0;t<size;t++)
        {
            if( *((unsigned char*)pSrc+t) != *((unsigned char*)data_rgb888_64x64+t) )
            {
                diff_cnt++;
                printk("t=%d, diff_cnt=%d, dst=0x%x, gold=0x%x \n", 
                t, 
                diff_cnt, 
                *((unsigned char*)pSrc+t), 
                *((unsigned char*)data_rgb888_64x64+t) );
            }
    
        }
        if(diff_cnt == 0)
            printk("ddp_mem_test src compare result: success \n");
        else
        {
            printk("[DDP] error: ddp_mem_test src compare result: fail \n");
            printk("detail, %d, %d, %%%d \n", diff_cnt, size, diff_cnt*100/size);  
            result = -1;
        }              
    }
    
    {
        unsigned int diff_cnt = 0;
        unsigned int t=0;
        unsigned int size = DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP;
        for(t=0;t<size;t++)
        {
            if( *((unsigned char*)pDst+t) != *((unsigned char*)data_rgb888_64x64_golden+t) )
            {
                diff_cnt++;
                printk("t=%d, diff_cnt=%d, dst=0x%x, gold=0x%x \n", 
                t, 
                diff_cnt, 
                *((unsigned char*)pDst+t), 
                *((unsigned char*)data_rgb888_64x64_golden+t) );
            }
    
        }
        if(diff_cnt == 0)
            printk("ddp_mem_test result: success \n");
        else
        {
            printk("[DDP] error: ddp_mem_test result: fail \n");
            printk("detail, %d, %d, %%%d \n", diff_cnt, size, diff_cnt*100/size); 
            result = -1;
        }              
    }
    /*
    disp_power_off(DISP_MODULE_ROT);
    disp_power_off(DISP_MODULE_SCL);
    disp_power_off(DISP_MODULE_WDMA0);
    */
    //dealloc memory
    vfree(pSrc);
    vfree(pDst);   

    return result;

}

int ddp_mem_test(void)
{    
    struct disp_path_config_struct config;
    unsigned int* pSrc;
    unsigned char* pSrcPa;
    unsigned int* pDst;
    unsigned char* pDstPa;
    int result = 0;
    
    pSrc= dma_alloc_coherent(NULL, DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP, (dma_addr_t *)&pSrcPa, GFP_KERNEL);
    if(pSrc==0 || pSrcPa==0)
    {
        printk("dma_alloc_coherent error!  dma memory not available.\n");
        return 0;
    }
    else
    {
        printk("[ddp] pSrc=0x%x, pSrcPa=0x%x \n", (unsigned int)pSrc, (unsigned int)pSrcPa);
    }
    memcpy((void*)pSrc, data_rgb888_64x64, DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP);
    
    pDst= dma_alloc_coherent(NULL, DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP, (dma_addr_t *)&pDstPa, GFP_KERNEL);
    if(pDst==0 || pDstPa==0)
    {
        printk("dma_alloc_coherent error!  dma memory not available.\n");
        return 0;
    }
    else
    {
        printk("[ddp] pDst=0x%x, pDstPa=0x%x \n",(unsigned int) pDst, (unsigned int)pDstPa);
    }
    memset((void*)pDst, 0, DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP);


    // config port to physical
    {
        M4U_PORT_STRUCT sPort;
        sPort.ePortID = M4U_PORT_ROT_EXT;
        sPort.Virtuality = 0; 					   
        sPort.Security = 0;
        sPort.Distance = 1;
        sPort.Direction = 0;
        m4u_config_port(&sPort);
        
        sPort.ePortID = M4U_PORT_WDMA0;
        sPort.Virtuality = 0; 					   
        sPort.Security = 0;
        sPort.Distance = 1;
        sPort.Direction = 0;
        m4u_config_port(&sPort);
	  }
	  
    config.srcModule = DISP_MODULE_SCL;
    config.addr = (unsigned int)pSrcPa; 
    config.inFormat = DISP_COLOR_FORMAT_RGB888; 
    config.pitch = DDP_TEST_WIDTH;
    config.srcROI.x = 0;
    config.srcROI.y = 0; 
    config.srcROI.width = DDP_TEST_WIDTH; 
    config.srcROI.height = DDP_TEST_HEIGHT; 
    config.srcWidth = DDP_TEST_WIDTH;
    config.srcHeight = DDP_TEST_HEIGHT;
    config.dstModule = DISP_MODULE_WDMA0;
    config.outFormat = WDMA_OUTPUT_FORMAT_RGB888; 
    config.dstAddr = (unsigned int)pDstPa; 
    config.dstWidth = DDP_TEST_WIDTH; 
    config.dstHeight = DDP_TEST_HEIGHT;
    config.dstPitch = DDP_TEST_WIDTH;
    /*
    disp_power_on(DISP_MODULE_ROT);
    disp_power_on(DISP_MODULE_SCL);
    disp_power_on(DISP_MODULE_WDMA0);
    */
    disp_path_get_mutex_(DDP_MUTEX_FOR_ROT_SCL_WDMA);
    disp_path_config_(&config, DDP_MUTEX_FOR_ROT_SCL_WDMA);
    
    printk("*after ddp test config start: -------------------\n");
    disp_dump_reg(DISP_MODULE_ROT);
    disp_dump_reg(DISP_MODULE_SCL);
    disp_dump_reg(DISP_MODULE_WDMA0);
    disp_dump_reg(DISP_MODULE_CONFIG);
    printk("*after ddp test config end: ---------------------\n");
    
    disp_path_release_mutex_(DDP_MUTEX_FOR_ROT_SCL_WDMA);
    if(*(volatile unsigned int*)DISP_REG_CONFIG_MUTEX1 != 0)
    {
        *(volatile unsigned int*)DISP_REG_CONFIG_MUTEX1 = 0;
    }
    
    printk("ddp_mem_test wdma wait done... \n"); 
    WDMAWait(0);
    printk("ddp_mem_test wdma done! \n");            
    
    if(0) //compare source
    {
        unsigned int diff_cnt = 0;
        unsigned int t=0;
        unsigned int size = DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP;
        for(t=0;t<size;t++)
        {
            if( *((unsigned char*)pSrc+t) != *((unsigned char*)data_rgb888_64x64+t) )
            {
                diff_cnt++;
                printk("t=%d, diff_cnt=%d, dst=0x%x, gold=0x%x \n", 
                t, 
                diff_cnt, 
                *((unsigned char*)pSrc+t), 
                *((unsigned char*)data_rgb888_64x64+t) );
            }
    
        }
        if(diff_cnt == 0)
            printk("ddp_mem_test src compare result: success \n");
        else
        {
            printk("ddp_mem_test src compare result: fail \n");
            printk("detail, %d, %d, %%%d \n", diff_cnt, size, diff_cnt*100/size);  
            result = -1;
        }              
    }
    
    if(1)  //compare dst
    {
        unsigned int diff_cnt = 0;
        unsigned int t=0;
        unsigned int size = DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP;
        for(t=0;t<size;t++)
        {
            if( *((unsigned char*)pDst+t) != *((unsigned char*)data_rgb888_64x64_golden+t) )
            {
                diff_cnt++;
                printk("t=%d, diff_cnt=%d, dst=0x%x, gold=0x%x \n", 
                t, 
                diff_cnt, 
                *((unsigned char*)pDst+t), 
                *((unsigned char*)data_rgb888_64x64_golden+t) );
            }
    
        }
        if(diff_cnt == 0)
            printk("ddp_mem_test result: success \n");
        else
        {
            printk("ddp_mem_test result: fail \n");
            printk("detail, %d, %d, %%%d \n", diff_cnt, size, diff_cnt*100/size); 
            result = -1;
        }              
    }
    
    // print out dst buffer to save as golden
    if(0)
    {
  	      unsigned int t=0;
	        unsigned int size = DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP;
	        for(t=0;t<size;t++)
	        {
		    	  printk("0x%x, ", *((unsigned char*)pDst+t));
		    	  if((t+1)%12==0)
		    	  {
		    	  	  printk("\n%05d: ", (t+1)/12);
		    	  }
	        }
    }
    /*
    disp_power_off(DISP_MODULE_ROT);
    disp_power_off(DISP_MODULE_SCL);
    disp_power_off(DISP_MODULE_WDMA0);
    */
    //dealloc memory
    dma_free_coherent(NULL, DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP, pSrc, (dma_addr_t)&pSrcPa);
    dma_free_coherent(NULL, DDP_TEST_WIDTH*DDP_TEST_HEIGHT*DDP_TEST_BPP, pDst, (dma_addr_t)&pDstPa);   

    return result;

}

