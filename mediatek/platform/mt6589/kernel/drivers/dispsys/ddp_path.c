

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/module.h>
#include <linux/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/param.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <linux/xlog.h>
#include <asm/io.h>

#include <mach/irqs.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_irq.h>
#include <mach/irqs.h>
#include <mach/mt_clkmgr.h> // ????
#include <mach/mt_irq.h>
#include <mach/sync_write.h>

#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_path.h"
#include "ddp_debug.h"

#include "ddp_rdma.h"
#include "ddp_wdma.h"
#include "ddp_ovl.h"

unsigned int gMutexID = 0;
unsigned int gTdshpStatus[OVL_LAYER_NUM] = {0};
static DEFINE_MUTEX(DpEngineMutexLock);

extern void DpEngine_COLORonConfig(unsigned int srcWidth,unsigned int srcHeight);
extern    void DpEngine_COLORonInit(void);
extern void DpEngine_SHARPonConfig(unsigned int srcWidth,unsigned int srcHeight);
extern    void DpEngine_SHARPonInit(void);

extern unsigned char pq_debug_flag;
static DECLARE_WAIT_QUEUE_HEAD(g_disp_mutex_wq);
static unsigned int g_disp_mutex_reg_update = 0;

//50us -> 1G needs 50000times
#define USE_TIME_OUT
#ifdef USE_TIME_OUT
int disp_wait_timeout(bool flag, unsigned int timeout)
{
    unsigned int cnt=0;

    while(cnt<timeout)
    {
        if(flag)
        {
            return 0;
        }
        cnt++;
    }
    return -1;
}
#endif

unsigned int g_ddp_timeout_flag = 0;
#define DDP_WAIT_TIMEOUT(flag, timeout) \
{ \
    unsigned int cnt=0; \
    while(cnt<timeout) \
    {\
        if(flag)\
        {\
            g_ddp_timeout_flag = 0;\
            break; \
        }\
        cnt++;\
    }\
    g_ddp_timeout_flag = 1;\
}

static void _disp_path_mutex_reg_update_cb(unsigned int param)
{
    if (param & (1 << gMutexID))
    {
        g_disp_mutex_reg_update = 1;
        wake_up_interruptible(&g_disp_mutex_wq);
    }
}

unsigned int disp_mutex_lock_cnt = 0;
unsigned int disp_mutex_unlock_cnt = 0;
int disp_path_get_mutex()
{
    if(pq_debug_flag == 3)
    {
        return 0;
    }
    else
    {
        disp_register_irq(DISP_MODULE_MUTEX, _disp_path_mutex_reg_update_cb);
        return disp_path_get_mutex_(gMutexID);
    }
}

int disp_path_get_mutex_(int mutexID)
{
    unsigned int cnt=0;

    DISP_DBG("disp_path_get_mutex %d \n", disp_mutex_lock_cnt++);
    mutex_lock(&DpEngineMutexLock);
    MMProfileLog(DDP_MMP_Events.Mutex[mutexID], MMProfileFlagStart);
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX(mutexID), 1);
    DISP_REG_SET_FIELD(REG_FLD(1, mutexID), DISP_REG_CONFIG_MUTEX_INTSTA, 0);

    while(((DISP_REG_GET(DISP_REG_CONFIG_MUTEX(mutexID))& DISP_INT_MUTEX_BIT_MASK) != DISP_INT_MUTEX_BIT_MASK))
    {
        cnt++;
        if(cnt>10000)
        {
            DISP_ERR("disp_path_get_mutex() timeout! mutexID=%d \n", mutexID);
            MMProfileLogEx(DDP_MMP_Events.Mutex[mutexID], MMProfileFlagPulse, 0, 0);
            disp_dump_reg(DISP_MODULE_MUTEX0+mutexID);
            break;
        }
    }

    return 0;
}
int disp_path_release_mutex()
{
    if(pq_debug_flag == 3)
    {
        return 0;
    }
    else
    {
        g_disp_mutex_reg_update = 0;
        return disp_path_release_mutex_(gMutexID);
    }
}

// check engines' clock bit and enable bit before unlock mutex
#define DDP_SMI_LARB2_POWER_BIT     0x1
#define DDP_OVL_POWER_BIT     0x30
#define DDP_RDMA0_POWER_BIT   0xe000
#define DDP_WDMA1_POWER_BIT   0x1800
int disp_check_engine_status(int mutexID)
{
    int result = 0;
    unsigned int engine = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(mutexID)); 
    
    if((DISP_REG_GET(DISP_REG_CONFIG_CG_CON0)&DDP_SMI_LARB2_POWER_BIT) != 0)
    {
        result = -1;
        DISP_ERR("smi clk if off before release mutex, clk=0x%x \n", DISP_REG_GET(DISP_REG_CONFIG_CG_CON0));

        DISP_MSG("force on smi clock\n");
        DISP_REG_SET(DISP_REG_CONFIG_CG_CON0, DISP_REG_GET(DISP_REG_CONFIG_CG_CON0)|DDP_SMI_LARB2_POWER_BIT);
    }
    
    // mutex intr must enable before release
    //DISP_ERR("release mutex inten=0x%x \n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN));
    if(DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN)!= DDP_MUTEX_INTR_ENABLE_BIT)
    {
        DISP_ERR("before release mutex inten=0x%x \n", DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN));
        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_INTEN, DDP_MUTEX_INTR_ENABLE_BIT);
    }

    if(engine&(1<<0)) //ROT
    {

    }
    if(engine&(1<<1)) //SCL
    {

    }
    if(engine&(1<<2)) //OVL
    {
        if(DISP_REG_GET(DISP_REG_OVL_EN)==0 || 
           (DISP_REG_GET(DISP_REG_CONFIG_CG_CON0)&DDP_OVL_POWER_BIT) != 0)
        {
            result = -1;
            DISP_ERR("ovl abnormal, en=%d, clk=0x%x \n", DISP_REG_GET(DISP_REG_OVL_EN), DISP_REG_GET(DISP_REG_CONFIG_CG_CON0));
        }
    }
    if(engine&(1<<3)) //COLOR
    {

    }
    if(engine&(1<<4)) //TDSHP
    {

    }
    if(engine&(1<<5)) //WDMA0
    {

    }
    if(engine&(1<<6)) //WDMA1
    {
        if(DISP_REG_GET(DISP_REG_WDMA_EN+0x1000)==0 || 
           (DISP_REG_GET(DISP_REG_CONFIG_CG_CON0)&DDP_WDMA1_POWER_BIT) != 0)
        {
            result = -1;
            DISP_ERR("wdma1 abnormal, en=%d, clk=0x%x \n", DISP_REG_GET(DISP_REG_WDMA_EN+0x1000), DISP_REG_GET(DISP_REG_CONFIG_CG_CON0));
        }
    }
    if(engine&(1<<7)) //RDMA0
    {
        if((DISP_REG_GET(DISP_REG_RDMA_GLOBAL_CON)&0x1) ==0 || 
           (DISP_REG_GET(DISP_REG_CONFIG_CG_CON0)&DDP_RDMA0_POWER_BIT) != 0)
        {
            result = -1;
            DISP_ERR("rdma0 abnormal, en=%d, clk=0x%x \n", DISP_REG_GET(DISP_REG_RDMA_GLOBAL_CON), DISP_REG_GET(DISP_REG_CONFIG_CG_CON0));
        }
    }
    if(engine&(1<<8)) //RDMA1
    {

    }    
    if(engine&(1<<9)) //BLS
    {

    }
    if(engine&(1<<10)) //GAMMA
    {

    }  

    if(result!=0)
    {
        DISP_ERR("engine status error before release mutex, engine=0x%x, mutexID=%d \n", engine, mutexID);
    }

    return result;

}


int disp_path_release_mutex_(int mutexID)
{
//  unsigned int reg = 0;
//    unsigned int cnt = 0;    
    disp_check_engine_status(mutexID);
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX(mutexID), 0);
    
#if 0    // can not polling mutex update done status, because after ECO, polling will delay at lease 12ms
    while(((DISP_REG_GET(DISP_REG_CONFIG_MUTEX(mutexID))& DISP_INT_MUTEX_BIT_MASK) != 0))
    {
        cnt++;
        if(cnt>10000)
        {
            DISP_ERR("disp_path_release_mutex() timeout! \n");
            MMProfileLogEx(DDP_MMP_Events.Mutex[mutexID], MMProfileFlagPulse, 1, 0);
            break;
        }
    }
#endif
    
#if 0
    while(((DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA) & (1<<mutexID)) != (1<<mutexID)))
    {
        if((DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA) & (1<<(mutexID+6))) == (1<<(mutexID+6)))
        {
            DISP_ERR("disp_path_release_mutex() timeout! \n");
            disp_dump_reg(DISP_MODULE_CONFIG);
            //print error engine
            reg = DISP_REG_GET(DISP_REG_CONFIG_REG_COMMIT);
            if(reg!=0)
            {
                  if(reg&(1<<0))  { DISP_MSG(" ROT update reg timeout! \n"); disp_dump_reg(DISP_MODULE_ROT); }
                  if(reg&(1<<1))  { DISP_MSG(" SCL update reg timeout! \n"); disp_dump_reg(DISP_MODULE_SCL); }
                  if(reg&(1<<2))  { DISP_MSG(" OVL update reg timeout! \n"); disp_dump_reg(DISP_MODULE_OVL); }
                  if(reg&(1<<3))  { DISP_MSG(" COLOR update reg timeout! \n"); disp_dump_reg(DISP_MODULE_COLOR); }
                  if(reg&(1<<4))  { DISP_MSG(" 2D_SHARP update reg timeout! \n"); disp_dump_reg(DISP_MODULE_TDSHP); }
                  if(reg&(1<<5))  { DISP_MSG(" WDMA0 update reg timeout! \n"); disp_dump_reg(DISP_MODULE_WDMA0); }
                  if(reg&(1<<6))  { DISP_MSG(" WDMA1 update reg timeout! \n"); disp_dump_reg(DISP_MODULE_WDMA1); }
                  if(reg&(1<<7))  { DISP_MSG(" RDMA0 update reg timeout! \n"); disp_dump_reg(DISP_MODULE_RDMA0); }
                  if(reg&(1<<8))  { DISP_MSG(" RDMA1 update reg timeout! \n"); disp_dump_reg(DISP_MODULE_RDMA1); }
                  if(reg&(1<<9))  { DISP_MSG(" BLS update reg timeout! \n"); disp_dump_reg(DISP_MODULE_BLS); }
                  if(reg&(1<<10)) { DISP_MSG(" GAMMA update reg timeout! \n"); disp_dump_reg(DISP_MODULE_GAMMA); }
            }  
                     
            //reset mutex
            DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(mutexID), 1);
            DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(mutexID), 0);
            DISP_MSG("mutex reset done! \n");
            MMProfileLogEx(DDP_MMP_Events.Mutex0, MMProfileFlagPulse, mutexID, 1);
            break;
        }

        cnt++;
        if(cnt>1000)
        {
            DISP_ERR("disp_path_release_mutex() timeout! \n");
            MMProfileLogEx(DDP_MMP_Events.Mutex0, MMProfileFlagPulse, mutexID, 2);
            break;
        }
    }

    // clear status
    reg = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTSTA);
    reg &= ~(1<<mutexID);
    reg &= ~(1<<(mutexID+6));
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_INTSTA, reg);
#endif
    MMProfileLog(DDP_MMP_Events.Mutex[mutexID], MMProfileFlagEnd);
    mutex_unlock(&DpEngineMutexLock);
    DISP_DBG("disp_path_release_mutex %d \n", disp_mutex_unlock_cnt++);

    return 0;
}

int disp_path_wait_reg_update(void)
{
    wait_event_interruptible_timeout(
                    g_disp_mutex_wq,
                    g_disp_mutex_reg_update,
                    HZ/10);
    return 0;
}

int disp_path_change_tdshp_status(unsigned int layer, unsigned int enable)
{
	  ASSERT(layer<DDP_OVL_LAYER_MUN);
	  DISP_MSG("disp_path_change_tdshp_status(), layer=%d, enable=%d", layer, enable);
	  gTdshpStatus[layer] = enable;
	  return 0;
}

int disp_path_config_layer(OVL_CONFIG_STRUCT* pOvlConfig)
{
//    unsigned int reg_addr;

    DISP_DBG("[DDP] config_layer(), layer=%d, en=%d, source=%d, fmt=%d, addr=0x%x, (%d, %d, %d, %d), pitch=%d, keyEn=%d, key=%d, aen=%d, alpha=%d, isTdshp=%d \n ", 
    pOvlConfig->layer,   // layer
    pOvlConfig->layer_en,   
    pOvlConfig->source,   // data source (0=memory)
    pOvlConfig->fmt, 
    pOvlConfig->addr, // addr 
    pOvlConfig->src_x,  // x
    pOvlConfig->src_y,  // y
    pOvlConfig->src_pitch, //pitch, pixel number
    pOvlConfig->dst_x,  // x
    pOvlConfig->dst_y,  // y
    pOvlConfig->dst_w, // width
    pOvlConfig->dst_h, // height
    pOvlConfig->keyEn,  //color key
    pOvlConfig->key,  //color key
    pOvlConfig->aen, // alpha enable
    pOvlConfig->alpha,
    pOvlConfig->isTdshp);    
    
    // config overlay
    MMProfileLogEx(DDP_MMP_Events.Debug, MMProfileFlagPulse, pOvlConfig->layer, pOvlConfig->layer_en);
    OVLLayerSwitch(pOvlConfig->layer, pOvlConfig->layer_en);
    if(pOvlConfig->layer_en!=0)
    {
        OVLLayerConfig(pOvlConfig->layer,   // layer
                       pOvlConfig->source,   // data source (0=memory)
                       pOvlConfig->fmt, 
                       pOvlConfig->addr, // addr 
                       pOvlConfig->src_x,  // x
                       pOvlConfig->src_y,  // y
                       pOvlConfig->src_pitch, //pitch, pixel number
                       pOvlConfig->dst_x,  // x
                       pOvlConfig->dst_y,  // y
                       pOvlConfig->dst_w, // width
                       pOvlConfig->dst_h, // height
                       pOvlConfig->keyEn,  //color key
                       pOvlConfig->key,  //color key
                       pOvlConfig->aen, // alpha enable
                       pOvlConfig->alpha); // alpha
    } 
    if(pOvlConfig->isTdshp==0)
    {
        gTdshpStatus[pOvlConfig->layer] = 0;
    }
    else
    {
        int i=0;
        for(i=0;i<OVL_LAYER_NUM;i++)
        {
            if(gTdshpStatus[i]==1 && i!=pOvlConfig->layer)  //other layer has already enable tdshp
            {
                DISP_ERR("enable layer=%d tdshp, but layer=%d has already enable tdshp \n", i, pOvlConfig->layer);
                return -1;
            }
            gTdshpStatus[pOvlConfig->layer] = 1;
        }
    }
    //OVLLayerTdshpEn(pOvlConfig->layer, pOvlConfig->isTdshp);
    OVLLayerTdshpEn(pOvlConfig->layer, 0); //Cvs: de-couple
    return 0;
}

int disp_path_config_layer_addr(unsigned int layer, unsigned int addr)
{
    unsigned int reg_addr;

    DISP_DBG("[DDP]disp_path_config_layer_addr(), layer=%d, addr=0x%x\n ", layer, addr);

    switch(layer)
    {
        case 0:
            DISP_REG_SET(DISP_REG_OVL_L0_ADDR, addr);
            reg_addr = DISP_REG_OVL_L0_ADDR;
            break;
        case 1:
            DISP_REG_SET(DISP_REG_OVL_L1_ADDR, addr);
            reg_addr = DISP_REG_OVL_L1_ADDR;
            break;
        case 2:
            DISP_REG_SET(DISP_REG_OVL_L2_ADDR, addr);
            reg_addr = DISP_REG_OVL_L2_ADDR;
            break;
        case 3:
            DISP_REG_SET(DISP_REG_OVL_L3_ADDR, addr);
            reg_addr = DISP_REG_OVL_L3_ADDR;
            break;
        default:
            DISP_ERR("unknow layer=%d \n", layer);
    }
   
    return 0;
}

DECLARE_WAIT_QUEUE_HEAD(mem_out_wq);
static unsigned int mem_out_done = 0;
void _disp_path_wdma_callback(unsigned int param)
{
    mem_out_done = 1;
    wake_up_interruptible(&mem_out_wq);
}

void disp_path_wait_mem_out_done(void)
{
    wait_event_interruptible(mem_out_wq, mem_out_done);
    mem_out_done = 0;
}

// add wdma1 into the path
// should call get_mutex() / release_mutex for this func
int disp_path_config_mem_out(struct disp_path_config_mem_out_struct* pConfig)
{
    unsigned int reg;
    static unsigned int bMemOutEnabled = 0;
#if 0    
    DISP_DBG(" disp_path_config_mem_out(), enable = %d, outFormat=%d, dstAddr=0x%x, ROI(%d,%d,%d,%d) \n",
            pConfig->enable,
            pConfig->outFormat,            
            pConfig->dstAddr,  
            pConfig->srcROI.x, 
            pConfig->srcROI.y, 
            pConfig->srcROI.width, 
            pConfig->srcROI.height);
#endif
    if(pConfig->enable==1 && pConfig->dstAddr==0)
    {
          DISP_ERR("pConfig->dstAddr==0! \n");
    }

    if(pConfig->enable==1)
    {
        mem_out_done = 0;
        disp_register_irq(DISP_MODULE_WDMA1, _disp_path_wdma_callback);

        // config wdma1
        if (!bMemOutEnabled)
        WDMAReset(1);
        WDMAConfig(1, 
                   WDMA_INPUT_FORMAT_ARGB, 
                   pConfig->srcROI.width,
                   pConfig->srcROI.height,
                   0,
                   0,
                   pConfig->srcROI.width, 
                   pConfig->srcROI.height, 
                   pConfig->outFormat, 
                   pConfig->dstAddr, 
                   pConfig->srcROI.width,
                   1, 
                   0);      
        if (!bMemOutEnabled)
        {
        WDMAStart(1);
        // mutex
        reg = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(gMutexID));
        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_MOD(gMutexID), reg|0x40); //wdma1=6
        
        // ovl mout
        reg = DISP_REG_GET(DISP_REG_CONFIG_OVL_MOUT_EN);
        DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, reg|0x1);   // ovl_mout output to bls
        }
		bMemOutEnabled = 1;
        //disp_dump_reg(DISP_MODULE_WDMA1);
    }
    else
    {
        // mutex
        reg = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(gMutexID));
        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_MOD(gMutexID), reg&(~0x40)); //wdma1=6
        
        // ovl mout
        reg = DISP_REG_GET(DISP_REG_CONFIG_OVL_MOUT_EN);
        DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, reg&(~0x1));   // ovl_mout output to bls
        
        // config wdma1
        //WDMAReset(1);
        disp_unregister_irq(DISP_MODULE_WDMA1, _disp_path_wdma_callback);
        bMemOutEnabled = 0;
    }

    return 0;
}

// just mem->ovl->wdma1->mem, used in suspend mode screen capture
// have to call this function set pConfig->enable=0 to reset configuration
// should call clock_on()/clock_off() if use this function in suspend mode
int disp_path_config_mem_out_without_lcd(struct disp_path_config_mem_out_struct* pConfig)
{
    static unsigned int reg_mutex_mod;
    static unsigned int reg_mutex_sof;
    static unsigned int reg_mout;
    
    DISP_DBG(" disp_path_config_mem_out(), enable = %d, outFormat=%d, dstAddr=0x%x, ROI(%d,%d,%d,%d) \n",
            pConfig->enable,
            pConfig->outFormat,            
            pConfig->dstAddr,  
            pConfig->srcROI.x, 
            pConfig->srcROI.y, 
            pConfig->srcROI.width, 
            pConfig->srcROI.height);
            
    if(pConfig->enable==1 && pConfig->dstAddr==0)
    {
          DISP_ERR("pConfig->dstAddr==0! \n");
    }

    if(pConfig->enable==1)
    {
        mem_out_done = 0;
        disp_register_irq(DISP_MODULE_WDMA1, _disp_path_wdma_callback);

        // config wdma1
        WDMAReset(1);
        WDMAConfig(1, 
                   WDMA_INPUT_FORMAT_ARGB, 
                   pConfig->srcROI.width,
                   pConfig->srcROI.height,
                   0,
                   0,
                   pConfig->srcROI.width, 
                   pConfig->srcROI.height, 
                   pConfig->outFormat, 
                   pConfig->dstAddr, 
                   pConfig->srcROI.width,
                   1, 
                   0);      
        WDMAStart(1);

        // mutex module
        reg_mutex_mod = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_MOD(gMutexID));
        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_MOD(gMutexID), 0x44); //ovl, wdma1

        // mutex sof
        reg_mutex_sof = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_SOF(gMutexID));
        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_SOF(gMutexID), 0); //single mode
                
        // ovl mout
        reg_mout = DISP_REG_GET(DISP_REG_CONFIG_OVL_MOUT_EN);
        DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, 1<<0);   // ovl_mout output to wdma1
        
        //disp_dump_reg(DISP_MODULE_WDMA1);
    }
    else
    {
        // mutex
        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_MOD(gMutexID), reg_mutex_mod);
        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_SOF(gMutexID), reg_mutex_sof);         
        // ovl mout
        DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, reg_mout);        

        disp_unregister_irq(DISP_MODULE_WDMA1, _disp_path_wdma_callback);
    }

    return 0;
}

UINT32 fb_width = 0;
UINT32 fb_height = 0;
int disp_path_config(struct disp_path_config_struct* pConfig)
{
	fb_width = pConfig->srcROI.width;
	fb_height = pConfig->srcROI.height;
    return disp_path_config_(pConfig, gMutexID);
}

DISP_MODULE_ENUM g_dst_module;
int disp_path_config_(struct disp_path_config_struct* pConfig, int mutexId)
{
    unsigned int mutexMode;
    unsigned int mutexValue;

    DISP_DBG("[DDP] disp_path_config(), srcModule=%d, addr=0x%x, inFormat=%d, \n\
            pitch=%d, bgROI(%d,%d,%d,%d), bgColor=%d, outFormat=%d, dstModule=%d, dstAddr=0x%x, dstPitch=%d, mutexId=%d \n",
            pConfig->srcModule,
            pConfig->addr,
            pConfig->inFormat,
            pConfig->pitch,
            pConfig->bgROI.x,
            pConfig->bgROI.y,
            pConfig->bgROI.width,
            pConfig->bgROI.height,
            pConfig->bgColor,
            pConfig->outFormat,
            pConfig->dstModule,
            pConfig->dstAddr,
            pConfig->dstPitch,
            mutexId);

    g_dst_module = pConfig->dstModule;
    
    if(pConfig->srcModule==DISP_MODULE_RDMA0 && pConfig->dstModule==DISP_MODULE_WDMA1)
    {
        DISP_ERR("rdma0 wdma1 can not enable together! \n");
        return -1;
    }

#ifdef DDP_USE_CLOCK_API

#else
        // TODO: clock manager sholud manager the clock ,not here
        DISP_REG_SET(DISP_REG_CONFIG_CG_CLR0 , 0xFFFFFFFF);
        DISP_REG_SET(DISP_REG_CONFIG_CG_CLR1 , 0xFFFFFFFF);   
#endif

        switch(pConfig->dstModule)
        {
            case DISP_MODULE_DSI_VDO:
                mutexMode = 1;
            break;

            case DISP_MODULE_DPI0:
                mutexMode = 2;
            break;

            case DISP_MODULE_DPI1:
            case DISP_MODULE_WDMA0: //FIXME: for hdmi temp
                mutexMode = 3;
                if(pConfig->srcModule==DISP_MODULE_SCL)
                {
                    mutexMode = 0;
                }
            break;

            case DISP_MODULE_DBI:
            case DISP_MODULE_DSI_CMD:
            case DISP_MODULE_WDMA1:
                mutexMode = 0;
            break;

            default:
                mutexMode = 0;
               DISP_ERR("unknown dstModule=%d \n", pConfig->dstModule); 
               return -1;
        }

       
        if(pConfig->srcModule==DISP_MODULE_RDMA0)
        {
            mutexValue = 1<<7; //rdma0=7
        }
        else if(pConfig->srcModule==DISP_MODULE_RDMA1)
        {
            mutexValue = 1<<8; //rdma1=8
        }
        else if(pConfig->srcModule==DISP_MODULE_SCL && pConfig->dstModule==DISP_MODULE_WDMA0)
        {
            mutexValue = 1 | 1<<1 | 1<<5; //rot=0, scl=1, wdma0=5
        }
        else
        {
	        if(pConfig->dstModule==DISP_MODULE_WDMA1)
	        {
	            mutexValue = 1<<2 | 1<<6; //ovl=2, wdma1=6
	        }
	        else
	        {
#if defined(MTK_AAL_SUPPORT)
                    //mutexValue = 1<<2 | 1<<3 | 1<<4 | 1<<7 | 1<<9; //ovl=2, rdma0=7, Color=3, TDSHP=4, BLS=9
                    // Cvs: de-couple TDSHP from OVL stream
                    mutexValue = 1<<2 | 1<<3 | 1<<7 | 1<<9; //ovl=2, rdma0=7, Color=3, TDSHP=4, BLS=9
#else
	            // Elsa: de-couple BLS from OVL stream
                    mutexValue = 1<<2 | 1<<3 | 1<<7; //ovl=2, rdma0=7, Color=3, TDSHP=4,
#endif
	        }
        }
        DISP_DBG("[DDP] %p mutex value : %x (mode : %d) (id : %d)\n", (void*)DISP_REG_CONFIG_MUTEX_MOD(mutexId), mutexValue, mutexMode, mutexId);

        if(pConfig->dstModule != DISP_MODULE_DSI_VDO && pConfig->dstModule != DISP_MODULE_DPI0)
        {
            DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(mutexId), 1);
            DISP_REG_SET(DISP_REG_CONFIG_MUTEX_RST(mutexId), 0);
        }

        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_MOD(mutexId), mutexValue);
        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_SOF(mutexId), mutexMode);

        DISP_REG_SET(DISP_REG_CONFIG_MUTEX_INTEN , 0x3cf);  //bit 0.1.2.3. 6.7.8.9

        // disp_path_get_mutex();

        ///> config config reg
        switch(pConfig->dstModule)
        {
            case DISP_MODULE_DSI:
            case DISP_MODULE_DSI_VDO:
            case DISP_MODULE_DSI_CMD:
            	
                DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, 0x4);   // ovl_mout output to COLOR
                DISP_REG_SET(DISP_REG_CONFIG_COLOR_MOUT_EN, 0x8); // color_mout output to BLS

                DISP_REG_SET(DISP_REG_CONFIG_COLOR_SEL, 1);         // color_sel from ovl
                DISP_REG_SET(DISP_REG_CONFIG_BLS_SEL, 1);         // bls_sel from COLOR   
                
                /*
                DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, 0x4);   // ovl_mout output to Color
                DISP_REG_SET(DISP_REG_CONFIG_TDSHP_MOUT_EN, 0x10); // tdshp_mout output to OVL directlink
                DISP_REG_SET(DISP_REG_CONFIG_COLOR_MOUT_EN, 0x8); // color_mout output to BLS

                DISP_REG_SET(DISP_REG_CONFIG_TDSHP_SEL, 0);         // tdshp_sel from overlay before blending 
                DISP_REG_SET(DISP_REG_CONFIG_COLOR_SEL, 1);         // color_sel from overla after blending
                DISP_REG_SET(DISP_REG_CONFIG_BLS_SEL, 1);         // bls_sel from COLOR               
                */
                DISP_REG_SET(DISP_REG_CONFIG_RDMA0_OUT_SEL, 0);  // rdma0_mout to dsi0
            break;

            case DISP_MODULE_DPI0:
                if(pConfig->srcModule == DISP_MODULE_RDMA1)
                {
                    DISP_REG_SET(DISP_REG_CONFIG_RDMA1_OUT_SEL, 0x1); // rdma1_mout to dpi0
                    DISP_REG_SET(DISP_REG_CONFIG_DPI0_SEL, 1);        // dpi0_sel from rdma1
                    DISP_REG_SET(DISP_REG_CONFIG_MISC, 0x1);	      // set DPI IO for DPI usage
                }
                else
                {
                    DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, 0x4);   // ovl_mout output to COLOR
                    DISP_REG_SET(DISP_REG_CONFIG_COLOR_MOUT_EN, 0x8); // color_mout output to BLS

                    DISP_REG_SET(DISP_REG_CONFIG_COLOR_SEL, 1);         // color_sel from ovl
                    DISP_REG_SET(DISP_REG_CONFIG_BLS_SEL, 1);         // bls_sel from COLOR   

                    /*
                    DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, 0x4);   // ovl_mout output to Color
                    DISP_REG_SET(DISP_REG_CONFIG_TDSHP_MOUT_EN, 0x10); // tdshp_mout output to OVL directlink
                    DISP_REG_SET(DISP_REG_CONFIG_COLOR_MOUT_EN, 0x8); // color_mout output to BLS

                    DISP_REG_SET(DISP_REG_CONFIG_TDSHP_SEL, 0);         // tdshp_sel from overlay before blending 
                    DISP_REG_SET(DISP_REG_CONFIG_COLOR_SEL, 1);         // color_sel from overla after blending
                    DISP_REG_SET(DISP_REG_CONFIG_BLS_SEL, 1);         // bls_sel from COLOR               
                    */

                    DISP_REG_SET(DISP_REG_CONFIG_RDMA0_OUT_SEL, 0x2); // rdma0_mout to dpi0
                    DISP_REG_SET(DISP_REG_CONFIG_DPI0_SEL, 0);        // dpi0_sel from rdma0
                }
            break;

            case DISP_MODULE_DPI1:
                DISP_REG_SET(DISP_REG_CONFIG_RDMA1_OUT_SEL, 0x2); // 2 for DPI1
            break;

            case DISP_MODULE_DBI:
                DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, 0x4);   // ovl_mout output to COLOR
                DISP_REG_SET(DISP_REG_CONFIG_COLOR_MOUT_EN, 0x8); // color_mout output to BLS

                DISP_REG_SET(DISP_REG_CONFIG_COLOR_SEL, 1);         // color_sel from ovl
                DISP_REG_SET(DISP_REG_CONFIG_BLS_SEL, 1);         // bls_sel from COLOR   
                
                /*
                DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, 0x4);   // ovl_mout output to Color
                DISP_REG_SET(DISP_REG_CONFIG_TDSHP_MOUT_EN, 0x10); // tdshp_mout output to OVL directlink
                DISP_REG_SET(DISP_REG_CONFIG_COLOR_MOUT_EN, 0x8); // color_mout output to BLS

                DISP_REG_SET(DISP_REG_CONFIG_TDSHP_SEL, 0);         // tdshp_sel from overlay before blending 
                DISP_REG_SET(DISP_REG_CONFIG_COLOR_SEL, 1);         // color_sel from overla after blending
                DISP_REG_SET(DISP_REG_CONFIG_BLS_SEL, 1);         // bls_sel from COLOR               
                */  

                DISP_REG_SET(DISP_REG_CONFIG_RDMA0_OUT_SEL, 0x1); // rdma0_mout to dbi    
                DISP_REG_SET(DISP_REG_CONFIG_DBI_SEL, 0);         // dbi_sel from rdma0        
                
            break;
            case DISP_MODULE_WDMA0:
                if(pConfig->srcModule == DISP_MODULE_SCL)
                {
                    DISP_REG_SET(DISP_REG_CONFIG_WDMA0_SEL, 0);   // 0 for SCL
                    DISP_REG_SET(DISP_REG_CONFIG_SCL_MOUT_EN, 1 << 0);   // 0 for WDMA0
                }
            break;
            case DISP_MODULE_WDMA1:
                DISP_REG_SET(DISP_REG_CONFIG_OVL_MOUT_EN, 0x1);   // ovl_mout output to wdma1
            break;
            default:
               DISP_ERR("unknown dstModule=%d \n", pConfig->dstModule); 
        }    
        
        ///> config engines
        if(pConfig->srcModule == DISP_MODULE_OVL)
        {            // config OVL

            OVLROI(pConfig->bgROI.width, // width
                   pConfig->bgROI.height, // height
                   pConfig->bgColor);// background B

            if(pConfig->dstModule!=DISP_MODULE_DSI_VDO && pConfig->dstModule!=DISP_MODULE_DPI0)
            {
                OVLStop();
                // OVLReset();
            }
            if(pConfig->ovl_config.layer<4)
            {

            OVLLayerSwitch(pConfig->ovl_config.layer, pConfig->ovl_config.layer_en);
            if(pConfig->ovl_config.layer_en!=0)
            {
                if(pConfig->ovl_config.addr==0 ||
                   pConfig->ovl_config.dst_w==0    ||
                   pConfig->ovl_config.dst_h==0    )
                {
                    DISP_ERR("ovl parameter invalidate, addr=0x%x, w=%d, h=%d \n",
                           pConfig->ovl_config.addr, 
                           pConfig->ovl_config.dst_w,
                           pConfig->ovl_config.dst_h);
                    return -1;
                }

                OVLLayerConfig(pConfig->ovl_config.layer,   // layer
                               pConfig->ovl_config.source,   // data source (0=memory)
                               pConfig->ovl_config.fmt, 
                               pConfig->ovl_config.addr, // addr 
                               pConfig->ovl_config.src_x,  // x
                               pConfig->ovl_config.src_y,  // y
                               pConfig->ovl_config.src_pitch, //pitch, pixel number
                               pConfig->ovl_config.dst_x,  // x
                               pConfig->ovl_config.dst_y,  // y
                               pConfig->ovl_config.dst_w, // width
                               pConfig->ovl_config.dst_h, // height
                               pConfig->ovl_config.keyEn,  //color key
                               pConfig->ovl_config.key,  //color key
                               pConfig->ovl_config.aen, // alpha enable
                               pConfig->ovl_config.alpha); // alpha
            }

            }
            else
            {
                DISP_ERR("layer ID undefined! %d \n", pConfig->ovl_config.layer);
            }
            OVLStart();

            if(pConfig->dstModule==DISP_MODULE_WDMA1)  //1. mem->ovl->wdma1->mem
            {
                WDMAReset(1);
                if(pConfig->dstAddr==0 ||
                   pConfig->srcROI.width==0    ||
                   pConfig->srcROI.height==0    )
                {
                    DISP_ERR("wdma parameter invalidate, addr=0x%x, w=%d, h=%d \n",
                           pConfig->dstAddr, 
                           pConfig->srcROI.width,
                           pConfig->srcROI.height);
                    return -1;
                }

                WDMAConfig(1, 
                           WDMA_INPUT_FORMAT_ARGB, 
                           pConfig->srcROI.width, 
                           pConfig->srcROI.height, 
                           0, 
                           0, 
                           pConfig->srcROI.width, 
                           pConfig->srcROI.height, 
                           pConfig->outFormat, 
                           pConfig->dstAddr, 
                           pConfig->srcROI.width, 
                           1, 
                           0);      
                WDMAStart(1);
            }
            else    //2. ovl->bls->rdma0->lcd
            {
                
#if defined(MTK_AAL_SUPPORT)
                disp_bls_init(pConfig->srcROI.width, pConfig->srcROI.height);
#endif
               //=============================config PQ start==================================				 
					
                DpEngine_SHARPonInit();
                DpEngine_SHARPonConfig(pConfig->srcROI.width,  //width
                                                     pConfig->srcROI.height); //height
                                                    

                DpEngine_COLORonInit();
                DpEngine_COLORonConfig(pConfig->srcROI.width,  //width
                                                     pConfig->srcROI.height); //height

								
                //=============================config PQ end==================================
                ///config RDMA
                if(pConfig->dstModule!=DISP_MODULE_DSI_VDO && pConfig->dstModule!=DISP_MODULE_DPI0)
                {
                    RDMAStop(0);
                    RDMAReset(0);
                }
                if(pConfig->srcROI.width==0    ||
                   pConfig->srcROI.height==0    )
                {
                    DISP_ERR("rdma parameter invalidate, w=%d, h=%d \n",
                           pConfig->srcROI.width,
                           pConfig->srcROI.height);
                    return -1;
                }
                RDMAConfig(0, 
                           RDMA_MODE_DIRECT_LINK,       ///direct link mode
                           RDMA_INPUT_FORMAT_RGB888,    // inputFormat
                           0,                        // address
                           pConfig->outFormat,          // output format
                           pConfig->pitch,              // pitch
                           pConfig->srcROI.width,       // width
                           pConfig->srcROI.height,      // height
                           0,                           //byte swap
                           0);                          // is RGB swap        
                           
                RDMAStart(0);
            }
        }
        else if(pConfig->srcModule == DISP_MODULE_RDMA0 || pConfig->srcModule == DISP_MODULE_RDMA1)
        {
            int index = pConfig->srcModule == DISP_MODULE_RDMA0 ? 0 : 1;
            
            ///config RDMA
            if(pConfig->dstModule!=DISP_MODULE_DSI_VDO && pConfig->dstModule!=DISP_MODULE_DPI0)
            {
            	RDMAStop(index);
              RDMAReset(index);
            }
            if(pConfig->addr==0 ||
               pConfig->srcWidth==0    ||
               pConfig->srcHeight==0    )
            {
                DISP_ERR("rdma parameter invalidate, addr=0x%x, w=%d, h=%d \n",
                           pConfig->addr, 
                           pConfig->srcWidth,
                           pConfig->srcHeight);
                return -1;
            }
            RDMAConfig(index,
                       RDMA_MODE_MEMORY,       ///direct link mode
                       pConfig->inFormat,      // inputFormat
                       pConfig->addr,          // address
                       pConfig->outFormat,     // output format
                       pConfig->pitch,          //                                         
                       pConfig->srcWidth,
                       pConfig->srcHeight,
                       0,                       //byte swap    
                       0);                      // is RGB swap          
            RDMAStart(index);
            disp_dump_reg(pConfig->srcModule);
        }
        else if(pConfig->srcModule == DISP_MODULE_SCL)
        {
            unsigned int memAddr[3] = { pConfig->addr, 0, 0 };
            DISP_COLOR_FORMAT outFormat;

            // ROT
            ROTStop();
            ROTReset();
            ROTConfig(0,
                      DISP_INTERLACE_FORMAT_NONE,
                      0,
                      pConfig->inFormat,
                      memAddr,
                      pConfig->srcWidth,
                      pConfig->srcHeight,
                      pConfig->pitch,
                      pConfig->srcROI,
                      &outFormat);
            ROTStart();
            //disp_dump_reg(DISP_MODULE_ROT);

            // SCL
            SCLStop();
            SCLReset();
            SCLConfig(DISP_INTERLACE_FORMAT_NONE,
                      0,
                      pConfig->srcWidth,
                      pConfig->srcHeight,
                      pConfig->dstWidth,
                      pConfig->dstHeight,
                      0);
            SCLStart();

            //disp_dump_reg(DISP_MODULE_SCL);

            // ROT->SCL->WDMA0
            if(pConfig->dstModule == DISP_MODULE_WDMA0)
            {
                WDMAReset(0);
                WDMAConfig(0,
                           WDMA_INPUT_FORMAT_YUV444, // from SCL
                           pConfig->dstWidth, 
                           pConfig->dstHeight,
                           0,
                           0,
                           pConfig->dstWidth,
                           pConfig->dstHeight,
                           pConfig->outFormat,
                           pConfig->dstAddr,
                           pConfig->dstWidth,
                           1,
                           0);
                WDMAStart(0);
                //disp_dump_reg(DISP_MODULE_WDMA0);
            }

            
        }

/*************************************************/
// Ultra config
    // ovl ultra 0x40402020
    DISP_REG_SET(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING, 0x40402020);
    DISP_REG_SET(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING, 0x40402020);
    DISP_REG_SET(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING, 0x40402020);
    DISP_REG_SET(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING, 0x40402020);
    // disp_rdma0 ultra
    DISP_REG_SET(DISP_REG_RDMA_MEM_GMC_SETTING_0, 0x20402040);
    // disp_rdma1 ultra
    DISP_REG_SET(DISP_REG_RDMA_MEM_GMC_SETTING_0+0x1000, 0x20402040);
    // disp_wdma0 ultra
    //DISP_REG_SET(DISP_REG_WDMA_BUF_CON1, 0x10000000);
    //DISP_REG_SET(DISP_REG_WDMA_BUF_CON2, 0x20402020);
    // disp_wdma1 ultra
    DISP_REG_SET(DISP_REG_WDMA_BUF_CON1+0x1000, 0x800800ff);    

    DISP_REG_SET(DISP_REG_WDMA_BUF_CON2+0x1000, 0x20200808);
/*************************************************/
       // TDOD: add debug cmd in display to dump register 
//        disp_dump_reg(DISP_MODULE_OVL);
//        disp_dump_reg(DISP_MODULE_WDMA1);
//        disp_dump_reg(DISP_MODULE_DPI0);
//        disp_dump_reg(DISP_MODULE_RDMA0);
//        disp_dump_reg(DISP_MODULE_CONFIG);

//		disp_path_release_mutex();
		
        return 0;
}

#ifdef DDP_USE_CLOCK_API
extern unsigned int* pRegBackup;
unsigned int reg_offset = 0;
// #define DDP_RECORD_REG_BACKUP_RESTORE  // print the reg value before backup and after restore
void reg_backup(unsigned int reg_addr)
{
   *(pRegBackup+reg_offset) = DISP_REG_GET(reg_addr);
#ifdef DDP_RECORD_REG_BACKUP_RESTORE
      printk("0x%08x(0x%08x), ", reg_addr, *(pRegBackup+reg_offset));
      if((reg_offset+1)%8==0)
          printk("\n");
#endif      
      reg_offset++;
      if(reg_offset>=DDP_BACKUP_REG_NUM)
      {
          DISP_ERR("reg_backup fail, reg_offset=%d, regBackupSize=%d \n", reg_offset, DDP_BACKUP_REG_NUM);        
      }
}

void reg_restore(unsigned int reg_addr)
{
      DISP_REG_SET(reg_addr, *(pRegBackup+reg_offset));
#ifdef DDP_RECORD_REG_BACKUP_RESTORE
      printk("0x%08x(0x%08x), ", reg_addr, DISP_REG_GET(reg_addr));
      if((reg_offset+1)%8==0)
          printk("\n");
#endif                
      reg_offset++;
      
      if(reg_offset>=DDP_BACKUP_REG_NUM)
      {
          DISP_ERR("reg_backup fail, reg_offset=%d, regBackupSize=%d \n", reg_offset, DDP_BACKUP_REG_NUM);        
      }
}

int disp_reg_backup()
{
  reg_offset = 0;
  DISP_MSG("disp_reg_backup() start, *pRegBackup=0x%x, reg_offset=%d  \n", *pRegBackup, reg_offset);
    MMProfileLogEx(DDP_MMP_Events.BackupReg, MMProfileFlagStart, 0, 0);
    // Config
    //reg_backup(DISP_REG_CONFIG_SCL_MOUT_EN      );
    reg_backup(DISP_REG_CONFIG_OVL_MOUT_EN      );
    reg_backup(DISP_REG_CONFIG_COLOR_MOUT_EN    );
    reg_backup(DISP_REG_CONFIG_TDSHP_MOUT_EN    );
    reg_backup(DISP_REG_CONFIG_MOUT_RST         );
    reg_backup(DISP_REG_CONFIG_RDMA0_OUT_SEL    );
    reg_backup(DISP_REG_CONFIG_RDMA1_OUT_SEL    );
    reg_backup(DISP_REG_CONFIG_OVL_PQ_OUT_SEL   );
    //reg_backup(DISP_REG_CONFIG_WDMA0_SEL        );
    reg_backup(DISP_REG_CONFIG_OVL_SEL          );
    reg_backup(DISP_REG_CONFIG_OVL_PQ_IN_SEL    );
    reg_backup(DISP_REG_CONFIG_COLOR_SEL        );
    reg_backup(DISP_REG_CONFIG_TDSHP_SEL        );
    reg_backup(DISP_REG_CONFIG_BLS_SEL          );
    reg_backup(DISP_REG_CONFIG_DBI_SEL          );
    reg_backup(DISP_REG_CONFIG_DPI0_SEL         );
    reg_backup(DISP_REG_CONFIG_MISC             );
    reg_backup(DISP_REG_CONFIG_PATH_DEBUG0      );
    reg_backup(DISP_REG_CONFIG_PATH_DEBUG1      );
    reg_backup(DISP_REG_CONFIG_PATH_DEBUG2      );
    reg_backup(DISP_REG_CONFIG_PATH_DEBUG3      );
    reg_backup(DISP_REG_CONFIG_PATH_DEBUG4      );
//    reg_backup(DISP_REG_CONFIG_CG_CON0          );
//    reg_backup(DISP_REG_CONFIG_CG_SET0          );
//    reg_backup(DISP_REG_CONFIG_CG_CLR0          );
//    reg_backup(DISP_REG_CONFIG_CG_CON1          );
//    reg_backup(DISP_REG_CONFIG_CG_SET1          );
//    reg_backup(DISP_REG_CONFIG_CG_CLR1          );
    reg_backup(DISP_REG_CONFIG_HW_DCM_EN0       );
    reg_backup(DISP_REG_CONFIG_HW_DCM_EN_SET0   );
    reg_backup(DISP_REG_CONFIG_HW_DCM_EN_CLR0   );
    reg_backup(DISP_REG_CONFIG_HW_DCM_EN1       );
    reg_backup(DISP_REG_CONFIG_HW_DCM_EN_SET1   );
    reg_backup(DISP_REG_CONFIG_HW_DCM_EN_CLR1   );
    reg_backup(DISP_REG_CONFIG_MBIST_DONE0      );
    reg_backup(DISP_REG_CONFIG_MBIST_DONE1      );
    reg_backup(DISP_REG_CONFIG_MBIST_FAIL0      );
    reg_backup(DISP_REG_CONFIG_MBIST_FAIL1      );
    reg_backup(DISP_REG_CONFIG_MBIST_FAIL2      );
    reg_backup(DISP_REG_CONFIG_MBIST_HOLDB0     );
    reg_backup(DISP_REG_CONFIG_MBIST_HOLDB1     );
    reg_backup(DISP_REG_CONFIG_MBIST_MODE0      );
    reg_backup(DISP_REG_CONFIG_MBIST_MODE1      );
    reg_backup(DISP_REG_CONFIG_MBIST_BSEL0      );
    reg_backup(DISP_REG_CONFIG_MBIST_BSEL1      );
    reg_backup(DISP_REG_CONFIG_MBIST_BSEL2      );
    reg_backup(DISP_REG_CONFIG_MBIST_BSEL3      );
    reg_backup(DISP_REG_CONFIG_MBIST_CON        );
    reg_backup(DISP_REG_CONFIG_DEBUG_OUT_SEL    );
    reg_backup(DISP_REG_CONFIG_TEST_CLK_SEL     );
    reg_backup(DISP_REG_CONFIG_DUMMY            );
    reg_backup(DISP_REG_CONFIG_MUTEX_INTEN      );
    reg_backup(DISP_REG_CONFIG_MUTEX_INTSTA     );
    reg_backup(DISP_REG_CONFIG_REG_UPD_TIMEOUT  );
    reg_backup(DISP_REG_CONFIG_REG_COMMIT       );
    reg_backup(DISP_REG_CONFIG_MUTEX0_EN        );
    reg_backup(DISP_REG_CONFIG_MUTEX0           );
    reg_backup(DISP_REG_CONFIG_MUTEX0_RST       );
    reg_backup(DISP_REG_CONFIG_MUTEX0_MOD       );
    reg_backup(DISP_REG_CONFIG_MUTEX0_SOF       );
    reg_backup(DISP_REG_CONFIG_MUTEX1_EN        );
    reg_backup(DISP_REG_CONFIG_MUTEX1           );
    reg_backup(DISP_REG_CONFIG_MUTEX1_RST       );
    reg_backup(DISP_REG_CONFIG_MUTEX1_MOD       );
    reg_backup(DISP_REG_CONFIG_MUTEX1_SOF       );
    reg_backup(DISP_REG_CONFIG_MUTEX2_EN        );
    reg_backup(DISP_REG_CONFIG_MUTEX2           );
    reg_backup(DISP_REG_CONFIG_MUTEX2_RST       );
    reg_backup(DISP_REG_CONFIG_MUTEX2_MOD       );
    reg_backup(DISP_REG_CONFIG_MUTEX2_SOF       );
    reg_backup(DISP_REG_CONFIG_MUTEX3_EN        );
    reg_backup(DISP_REG_CONFIG_MUTEX3           );
    reg_backup(DISP_REG_CONFIG_MUTEX3_RST       );
    reg_backup(DISP_REG_CONFIG_MUTEX3_MOD       );
    reg_backup(DISP_REG_CONFIG_MUTEX3_SOF       );
    reg_backup(DISP_REG_CONFIG_MUTEX4_EN        );
    reg_backup(DISP_REG_CONFIG_MUTEX4           );
    reg_backup(DISP_REG_CONFIG_MUTEX4_RST       );
    reg_backup(DISP_REG_CONFIG_MUTEX4_MOD       );
    reg_backup(DISP_REG_CONFIG_MUTEX4_SOF       );
    reg_backup(DISP_REG_CONFIG_MUTEX5_EN        );
    reg_backup(DISP_REG_CONFIG_MUTEX5           );
    reg_backup(DISP_REG_CONFIG_MUTEX5_RST       );
    reg_backup(DISP_REG_CONFIG_MUTEX5_MOD       );
    reg_backup(DISP_REG_CONFIG_MUTEX5_SOF       );
    reg_backup(DISP_REG_CONFIG_MUTEX_DEBUG_OUT_SEL); 
    
    // OVL
    reg_backup(DISP_REG_OVL_STA                       );
    reg_backup(DISP_REG_OVL_INTEN                     );
    reg_backup(DISP_REG_OVL_INTSTA                    );
    reg_backup(DISP_REG_OVL_EN                        );
    reg_backup(DISP_REG_OVL_TRIG                      );
    reg_backup(DISP_REG_OVL_RST                       );
    reg_backup(DISP_REG_OVL_ROI_SIZE                  );
    reg_backup(DISP_REG_OVL_DATAPATH_CON              );
    reg_backup(DISP_REG_OVL_ROI_BGCLR                 );
    reg_backup(DISP_REG_OVL_SRC_CON                   );
    reg_backup(DISP_REG_OVL_L0_CON                    );
    reg_backup(DISP_REG_OVL_L0_SRCKEY                 );
    reg_backup(DISP_REG_OVL_L0_SRC_SIZE               );
    reg_backup(DISP_REG_OVL_L0_OFFSET                 );
    reg_backup(DISP_REG_OVL_L0_ADDR                   );
    reg_backup(DISP_REG_OVL_L0_PITCH                  );
    reg_backup(DISP_REG_OVL_L1_CON                    );
    reg_backup(DISP_REG_OVL_L1_SRCKEY                 );
    reg_backup(DISP_REG_OVL_L1_SRC_SIZE               );
    reg_backup(DISP_REG_OVL_L1_OFFSET                 );
    reg_backup(DISP_REG_OVL_L1_ADDR                   );
    reg_backup(DISP_REG_OVL_L1_PITCH                  );
    reg_backup(DISP_REG_OVL_L2_CON                    );
    reg_backup(DISP_REG_OVL_L2_SRCKEY                 );
    reg_backup(DISP_REG_OVL_L2_SRC_SIZE               );
    reg_backup(DISP_REG_OVL_L2_OFFSET                 );
    reg_backup(DISP_REG_OVL_L2_ADDR                   );
    reg_backup(DISP_REG_OVL_L2_PITCH                  );
    reg_backup(DISP_REG_OVL_L3_CON                    );
    reg_backup(DISP_REG_OVL_L3_SRCKEY                 );
    reg_backup(DISP_REG_OVL_L3_SRC_SIZE               );
    reg_backup(DISP_REG_OVL_L3_OFFSET                 );
    reg_backup(DISP_REG_OVL_L3_ADDR                   );
    reg_backup(DISP_REG_OVL_L3_PITCH                  );
    reg_backup(DISP_REG_OVL_RDMA0_CTRL                );
    reg_backup(DISP_REG_OVL_RDMA0_MEM_START_TRIG      );
    reg_backup(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING     );
    reg_backup(DISP_REG_OVL_RDMA0_MEM_SLOW_CON        );
    reg_backup(DISP_REG_OVL_RDMA0_FIFO_CTRL           );
    reg_backup(DISP_REG_OVL_RDMA1_CTRL                );
    reg_backup(DISP_REG_OVL_RDMA1_MEM_START_TRIG      );
    reg_backup(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING     );
    reg_backup(DISP_REG_OVL_RDMA1_MEM_SLOW_CON        );
    reg_backup(DISP_REG_OVL_RDMA1_FIFO_CTRL           );
    reg_backup(DISP_REG_OVL_RDMA2_CTRL                );
    reg_backup(DISP_REG_OVL_RDMA2_MEM_START_TRIG      );
    reg_backup(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING     );
    reg_backup(DISP_REG_OVL_RDMA2_MEM_SLOW_CON        );
    reg_backup(DISP_REG_OVL_RDMA2_FIFO_CTRL           );
    reg_backup(DISP_REG_OVL_RDMA3_CTRL                );
    reg_backup(DISP_REG_OVL_RDMA3_MEM_START_TRIG      );
    reg_backup(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING     );
    reg_backup(DISP_REG_OVL_RDMA3_MEM_SLOW_CON        );
    reg_backup(DISP_REG_OVL_RDMA3_FIFO_CTRL           );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_R0            );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_R1            );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_G0            );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_G1            );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_B0            );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_B1            );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_YUV_A_0       );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_YUV_A_1       );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_RGB_A_0       );
    reg_backup(DISP_REG_OVL_L0_Y2R_PARA_RGB_A_1       );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_R0            );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_R1            );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_G0            );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_G1            );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_B0            );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_B1            );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_YUV_A_0       );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_YUV_A_1       );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_RGB_A_0       );
    reg_backup(DISP_REG_OVL_L1_Y2R_PARA_RGB_A_1       );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_R0            );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_R1            );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_G0            );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_G1            );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_B0            );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_B1            );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_YUV_A_0       );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_YUV_A_1       );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_RGB_A_0       );
    reg_backup(DISP_REG_OVL_L2_Y2R_PARA_RGB_A_1       );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_R0            );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_R1            );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_G0            );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_G1            );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_B0            );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_B1            );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_YUV_A_0       );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_YUV_A_1       );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_RGB_A_0       );
    reg_backup(DISP_REG_OVL_L3_Y2R_PARA_RGB_A_1       );
    reg_backup(DISP_REG_OVL_DEBUG_MON_SEL             );
    reg_backup(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING2    );
    reg_backup(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING2    );
    reg_backup(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING2    );
    reg_backup(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING2    );
    reg_backup(DISP_REG_OVL_FLOW_CTRL_DBG             );
    reg_backup(DISP_REG_OVL_ADDCON_DBG                );
    reg_backup(DISP_REG_OVL_OUTMUX_DBG                );
    
    // RDMA0
    reg_backup(DISP_REG_RDMA_INT_ENABLE          );
    reg_backup(DISP_REG_RDMA_INT_STATUS          );
    reg_backup(DISP_REG_RDMA_GLOBAL_CON          );
    reg_backup(DISP_REG_RDMA_SIZE_CON_0          );
    reg_backup(DISP_REG_RDMA_SIZE_CON_1          );
    reg_backup(DISP_REG_RDMA_TARGET_LINE          );
    reg_backup(DISP_REG_RDMA_MEM_CON             );
    reg_backup(DISP_REG_RDMA_MEM_START_ADDR      );
    reg_backup(DISP_REG_RDMA_MEM_SRC_PITCH       );
    reg_backup(DISP_REG_RDMA_MEM_GMC_SETTING_0   );
    reg_backup(DISP_REG_RDMA_MEM_SLOW_CON        );
    reg_backup(DISP_REG_RDMA_MEM_GMC_SETTING_1   );
    reg_backup(DISP_REG_RDMA_FIFO_CON            );
    reg_backup(DISP_REG_RDMA_CF_00               );
    reg_backup(DISP_REG_RDMA_CF_01               );
    reg_backup(DISP_REG_RDMA_CF_02               );
    reg_backup(DISP_REG_RDMA_CF_10               );
    reg_backup(DISP_REG_RDMA_CF_11               );
    reg_backup(DISP_REG_RDMA_CF_12               );
    reg_backup(DISP_REG_RDMA_CF_20               );
    reg_backup(DISP_REG_RDMA_CF_21               );
    reg_backup(DISP_REG_RDMA_CF_22               );
    reg_backup(DISP_REG_RDMA_CF_PRE_ADD0         );
    reg_backup(DISP_REG_RDMA_CF_PRE_ADD1         );
    reg_backup(DISP_REG_RDMA_CF_PRE_ADD2         );
    reg_backup(DISP_REG_RDMA_CF_POST_ADD0        );
    reg_backup(DISP_REG_RDMA_CF_POST_ADD1        );
    reg_backup(DISP_REG_RDMA_CF_POST_ADD2        );
    reg_backup(DISP_REG_RDMA_DUMMY               );
    reg_backup(DISP_REG_RDMA_DEBUG_OUT_SEL       );
    
    // RDMA1
    reg_backup(DISP_REG_RDMA_INT_ENABLE          +0x1000 );
    reg_backup(DISP_REG_RDMA_INT_STATUS          +0x1000 );
    reg_backup(DISP_REG_RDMA_GLOBAL_CON          +0x1000 );
    reg_backup(DISP_REG_RDMA_SIZE_CON_0          +0x1000 );
    reg_backup(DISP_REG_RDMA_SIZE_CON_1          +0x1000 );
    reg_backup(DISP_REG_RDMA_TARGET_LINE         +0x1000 );
    reg_backup(DISP_REG_RDMA_MEM_CON             +0x1000 );
    reg_backup(DISP_REG_RDMA_MEM_START_ADDR      +0x1000 );
    reg_backup(DISP_REG_RDMA_MEM_SRC_PITCH       +0x1000 );
    reg_backup(DISP_REG_RDMA_MEM_GMC_SETTING_0   +0x1000 );
    reg_backup(DISP_REG_RDMA_MEM_SLOW_CON        +0x1000 );
    reg_backup(DISP_REG_RDMA_MEM_GMC_SETTING_1   +0x1000 );
    reg_backup(DISP_REG_RDMA_FIFO_CON            +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_00               +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_01               +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_02               +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_10               +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_11               +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_12               +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_20               +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_21               +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_22               +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_PRE_ADD0         +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_PRE_ADD1         +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_PRE_ADD2         +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_POST_ADD0        +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_POST_ADD1        +0x1000 );
    reg_backup(DISP_REG_RDMA_CF_POST_ADD2        +0x1000 );
    reg_backup(DISP_REG_RDMA_DUMMY               +0x1000 );
    reg_backup(DISP_REG_RDMA_DEBUG_OUT_SEL       +0x1000 ); 
#if 0                                                
    // ROTDMA                                    
    reg_backup(DISP_REG_ROT_EN                   );
    reg_backup(DISP_REG_ROT_RESET                );
    reg_backup(DISP_REG_ROT_INTERRUPT_ENABLE     );
    reg_backup(DISP_REG_ROT_INTERRUPT_STATUS     );
    reg_backup(DISP_REG_ROT_CON                  );
    reg_backup(DISP_REG_ROT_GMCIF_CON            );
    reg_backup(DISP_REG_ROT_SRC_CON              );
    reg_backup(DISP_REG_ROT_SRC_BASE_0           );
    reg_backup(DISP_REG_ROT_SRC_BASE_1           );
    reg_backup(DISP_REG_ROT_SRC_BASE_2           );
    reg_backup(DISP_REG_ROT_MF_BKGD_SIZE_IN_BYTE );
    reg_backup(DISP_REG_ROT_MF_SRC_SIZE          );
    reg_backup(DISP_REG_ROT_MF_CLIP_SIZE         );
    reg_backup(DISP_REG_ROT_MF_OFFSET_1          );
    reg_backup(DISP_REG_ROT_MF_PAR               );
    reg_backup(DISP_REG_ROT_SF_BKGD_SIZE_IN_BYTE );
    reg_backup(DISP_REG_ROT_SF_PAR               );
    reg_backup(DISP_REG_ROT_MB_DEPTH             );
    reg_backup(DISP_REG_ROT_MB_BASE              );
    reg_backup(DISP_REG_ROT_MB_CON               );
    reg_backup(DISP_REG_ROT_SB_DEPTH             );
    reg_backup(DISP_REG_ROT_SB_BASE              );
    reg_backup(DISP_REG_ROT_SB_CON               );
    reg_backup(DISP_REG_ROT_VC1_RANGE            );
    reg_backup(DISP_REG_ROT_TRANSFORM_0          );
    reg_backup(DISP_REG_ROT_TRANSFORM_1          );
    reg_backup(DISP_REG_ROT_TRANSFORM_2          );
    reg_backup(DISP_REG_ROT_TRANSFORM_3          );
    reg_backup(DISP_REG_ROT_TRANSFORM_4          );
    reg_backup(DISP_REG_ROT_TRANSFORM_5          );
    reg_backup(DISP_REG_ROT_TRANSFORM_6          );
    reg_backup(DISP_REG_ROT_TRANSFORM_7          );
    reg_backup(DISP_REG_ROT_RESV_DUMMY_0         );
                                                
    // SCL                                      
    reg_backup(DISP_REG_SCL_CTRL                 );
    reg_backup(DISP_REG_SCL_INTEN                );
    reg_backup(DISP_REG_SCL_INTSTA               );
    reg_backup(DISP_REG_SCL_STATUS               );
    reg_backup(DISP_REG_SCL_CFG                  );
    reg_backup(DISP_REG_SCL_INP_CHKSUM           );
    reg_backup(DISP_REG_SCL_OUTP_CHKSUM          );
    reg_backup(DISP_REG_SCL_HRZ_CFG              );
    reg_backup(DISP_REG_SCL_HRZ_SIZE             );
    reg_backup(DISP_REG_SCL_HRZ_FACTOR           );
    reg_backup(DISP_REG_SCL_HRZ_OFFSET           );
    reg_backup(DISP_REG_SCL_VRZ_CFG              );
    reg_backup(DISP_REG_SCL_VRZ_SIZE             );
    reg_backup(DISP_REG_SCL_VRZ_FACTOR           );
    reg_backup(DISP_REG_SCL_VRZ_OFFSET           );
    reg_backup(DISP_REG_SCL_EXT_COEF             );
    reg_backup(DISP_REG_SCL_PEAK_CFG             );
                                                
    // WDMA 0                                    
    reg_backup(DISP_REG_WDMA_INTEN               );
    reg_backup(DISP_REG_WDMA_INTSTA              );
    reg_backup(DISP_REG_WDMA_EN                  );
    reg_backup(DISP_REG_WDMA_RST                 );
    reg_backup(DISP_REG_WDMA_SMI_CON             );
    reg_backup(DISP_REG_WDMA_CFG                 );
    reg_backup(DISP_REG_WDMA_SRC_SIZE            );
    reg_backup(DISP_REG_WDMA_CLIP_SIZE           );
    reg_backup(DISP_REG_WDMA_CLIP_COORD          );
    reg_backup(DISP_REG_WDMA_DST_ADDR            );
    reg_backup(DISP_REG_WDMA_DST_W_IN_BYTE       );
    reg_backup(DISP_REG_WDMA_ALPHA               );
    reg_backup(DISP_REG_WDMA_BUF_ADDR            );
    reg_backup(DISP_REG_WDMA_STA                 );
    reg_backup(DISP_REG_WDMA_BUF_CON1            );
    reg_backup(DISP_REG_WDMA_BUF_CON2            );
    reg_backup(DISP_REG_WDMA_C00                 );
    reg_backup(DISP_REG_WDMA_C02                 );
    reg_backup(DISP_REG_WDMA_C10                 );
    reg_backup(DISP_REG_WDMA_C12                 );
    reg_backup(DISP_REG_WDMA_C20                 );
    reg_backup(DISP_REG_WDMA_C22                 );
    reg_backup(DISP_REG_WDMA_PRE_ADD0            );
    reg_backup(DISP_REG_WDMA_PRE_ADD2            );
    reg_backup(DISP_REG_WDMA_POST_ADD0           );
    reg_backup(DISP_REG_WDMA_POST_ADD2           );
    reg_backup(DISP_REG_WDMA_DST_U_ADDR          );
    reg_backup(DISP_REG_WDMA_DST_V_ADDR          );
    reg_backup(DISP_REG_WDMA_DST_UV_PITCH        );
    reg_backup(DISP_REG_WDMA_DITHER_CON          );
    reg_backup(DISP_REG_WDMA_FLOW_CTRL_DBG       );
    reg_backup(DISP_REG_WDMA_EXEC_DBG            );
    reg_backup(DISP_REG_WDMA_CLIP_DBG            );
#endif
    // WDMA1                                    
    reg_backup(DISP_REG_WDMA_INTEN         +0x1000 );
    reg_backup(DISP_REG_WDMA_INTSTA        +0x1000 );
    reg_backup(DISP_REG_WDMA_EN            +0x1000 );
    reg_backup(DISP_REG_WDMA_RST           +0x1000 );
    reg_backup(DISP_REG_WDMA_SMI_CON       +0x1000 );
    reg_backup(DISP_REG_WDMA_CFG           +0x1000 );
    reg_backup(DISP_REG_WDMA_SRC_SIZE      +0x1000 );
    reg_backup(DISP_REG_WDMA_CLIP_SIZE     +0x1000 );
    reg_backup(DISP_REG_WDMA_CLIP_COORD    +0x1000 );
    reg_backup(DISP_REG_WDMA_DST_ADDR      +0x1000 );
    reg_backup(DISP_REG_WDMA_DST_W_IN_BYTE +0x1000 );
    reg_backup(DISP_REG_WDMA_ALPHA         +0x1000 );
    reg_backup(DISP_REG_WDMA_BUF_ADDR      +0x1000 );
    reg_backup(DISP_REG_WDMA_STA           +0x1000 );
    reg_backup(DISP_REG_WDMA_BUF_CON1      +0x1000 );
    reg_backup(DISP_REG_WDMA_BUF_CON2      +0x1000 );
    reg_backup(DISP_REG_WDMA_C00           +0x1000 );
    reg_backup(DISP_REG_WDMA_C02           +0x1000 );
    reg_backup(DISP_REG_WDMA_C10           +0x1000 );
    reg_backup(DISP_REG_WDMA_C12           +0x1000 );
    reg_backup(DISP_REG_WDMA_C20           +0x1000 );
    reg_backup(DISP_REG_WDMA_C22           +0x1000 );
    reg_backup(DISP_REG_WDMA_PRE_ADD0      +0x1000 );
    reg_backup(DISP_REG_WDMA_PRE_ADD2      +0x1000 );
    reg_backup(DISP_REG_WDMA_POST_ADD0     +0x1000 );
    reg_backup(DISP_REG_WDMA_POST_ADD2     +0x1000 );
    reg_backup(DISP_REG_WDMA_DST_U_ADDR    +0x1000 );
    reg_backup(DISP_REG_WDMA_DST_V_ADDR    +0x1000 );
    reg_backup(DISP_REG_WDMA_DST_UV_PITCH  +0x1000 );
    reg_backup(DISP_REG_WDMA_DITHER_CON    +0x1000 );
    reg_backup(DISP_REG_WDMA_FLOW_CTRL_DBG +0x1000 );
    reg_backup(DISP_REG_WDMA_EXEC_DBG      +0x1000 );
    reg_backup(DISP_REG_WDMA_CLIP_DBG      +0x1000 );
                                                
    // BLS                                       
    reg_backup(DISP_REG_BLS_EN                   );
    reg_backup(DISP_REG_BLS_RST                  );
    reg_backup(DISP_REG_BLS_INTEN                );
    reg_backup(DISP_REG_BLS_INTSTA               );
    reg_backup(DISP_REG_BLS_SRC_SIZE             );
    reg_backup(DISP_REG_BLS_PWM_DUTY             );
    reg_backup(DISP_REG_BLS_PWM_DUTY_GAIN        );
    reg_backup(DISP_REG_BLS_PWM_CON              );
    reg_backup(DISP_REG_PWM_H_DURATION           );
    reg_backup(DISP_REG_PWM_L_DURATION           );
    reg_backup(DISP_REG_PWM_G_DURATION           );
    reg_backup(DISP_REG_PWM_SEND_DATA0           );
    reg_backup(DISP_REG_PWM_SEND_DATA1           );
    reg_backup(DISP_REG_PWM_WAVE_NUM             );
    reg_backup(DISP_REG_PWM_DATA_WIDTH           );
    reg_backup(DISP_REG_PWM_THRESH               );
    reg_backup(DISP_REG_PWM_SEND_WAVENUM         );

  DISP_MSG("disp_reg_backup() end, *pRegBackup=0x%x, reg_offset=%d \n", *pRegBackup, reg_offset);     
    MMProfileLogEx(DDP_MMP_Events.BackupReg, MMProfileFlagEnd, 0, 0);
  
  return 0; 
}

int disp_reg_restore()
{
    reg_offset = 0;
    DISP_MSG("disp_reg_restore(*) start, *pRegBackup=0x%x, reg_offset=%d  \n", *pRegBackup, reg_offset);

    MMProfileLogEx(DDP_MMP_Events.BackupReg, MMProfileFlagStart, 1, 0);
    //disp_path_get_mutex();            
    // Config
    //reg_restore(DISP_REG_CONFIG_SCL_MOUT_EN      );
    reg_restore(DISP_REG_CONFIG_OVL_MOUT_EN      );
    reg_restore(DISP_REG_CONFIG_COLOR_MOUT_EN    );
    reg_restore(DISP_REG_CONFIG_TDSHP_MOUT_EN    );
    reg_restore(DISP_REG_CONFIG_MOUT_RST         );
    reg_restore(DISP_REG_CONFIG_RDMA0_OUT_SEL    );
    reg_restore(DISP_REG_CONFIG_RDMA1_OUT_SEL    );
    reg_restore(DISP_REG_CONFIG_OVL_PQ_OUT_SEL   );
    //reg_restore(DISP_REG_CONFIG_WDMA0_SEL        );
    reg_restore(DISP_REG_CONFIG_OVL_SEL          );
    reg_restore(DISP_REG_CONFIG_OVL_PQ_IN_SEL    );
    reg_restore(DISP_REG_CONFIG_COLOR_SEL        );
    reg_restore(DISP_REG_CONFIG_TDSHP_SEL        );
    reg_restore(DISP_REG_CONFIG_BLS_SEL          );
    reg_restore(DISP_REG_CONFIG_DBI_SEL          );
    reg_restore(DISP_REG_CONFIG_DPI0_SEL         );
    reg_restore(DISP_REG_CONFIG_MISC             );
    reg_restore(DISP_REG_CONFIG_PATH_DEBUG0      );
    reg_restore(DISP_REG_CONFIG_PATH_DEBUG1      );
    reg_restore(DISP_REG_CONFIG_PATH_DEBUG2      );
    reg_restore(DISP_REG_CONFIG_PATH_DEBUG3      );
    reg_restore(DISP_REG_CONFIG_PATH_DEBUG4      );
//    reg_restore(DISP_REG_CONFIG_CG_CON0          );
//    reg_restore(DISP_REG_CONFIG_CG_SET0          );
//    reg_restore(DISP_REG_CONFIG_CG_CLR0          );
//    reg_restore(DISP_REG_CONFIG_CG_CON1          );
//    reg_restore(DISP_REG_CONFIG_CG_SET1          );
//    reg_restore(DISP_REG_CONFIG_CG_CLR1          );
    reg_restore(DISP_REG_CONFIG_HW_DCM_EN0       );
    reg_restore(DISP_REG_CONFIG_HW_DCM_EN_SET0   );
    reg_restore(DISP_REG_CONFIG_HW_DCM_EN_CLR0   );
    reg_restore(DISP_REG_CONFIG_HW_DCM_EN1       );
    reg_restore(DISP_REG_CONFIG_HW_DCM_EN_SET1   );
    reg_restore(DISP_REG_CONFIG_HW_DCM_EN_CLR1   );
    reg_restore(DISP_REG_CONFIG_MBIST_DONE0      );
    reg_restore(DISP_REG_CONFIG_MBIST_DONE1      );
    reg_restore(DISP_REG_CONFIG_MBIST_FAIL0      );
    reg_restore(DISP_REG_CONFIG_MBIST_FAIL1      );
    reg_restore(DISP_REG_CONFIG_MBIST_FAIL2      );
    reg_restore(DISP_REG_CONFIG_MBIST_HOLDB0     );
    reg_restore(DISP_REG_CONFIG_MBIST_HOLDB1     );
    reg_restore(DISP_REG_CONFIG_MBIST_MODE0      );
    reg_restore(DISP_REG_CONFIG_MBIST_MODE1      );
    reg_restore(DISP_REG_CONFIG_MBIST_BSEL0      );
    reg_restore(DISP_REG_CONFIG_MBIST_BSEL1      );
    reg_restore(DISP_REG_CONFIG_MBIST_BSEL2      );
    reg_restore(DISP_REG_CONFIG_MBIST_BSEL3      );
    reg_restore(DISP_REG_CONFIG_MBIST_CON        );
    reg_restore(DISP_REG_CONFIG_DEBUG_OUT_SEL    );
    reg_restore(DISP_REG_CONFIG_TEST_CLK_SEL     );
    reg_restore(DISP_REG_CONFIG_DUMMY            );
    reg_restore(DISP_REG_CONFIG_MUTEX_INTEN      );
    reg_restore(DISP_REG_CONFIG_MUTEX_INTSTA     );
    reg_restore(DISP_REG_CONFIG_REG_UPD_TIMEOUT  );
    reg_restore(DISP_REG_CONFIG_REG_COMMIT       );
    reg_restore(DISP_REG_CONFIG_MUTEX0_EN        );
    reg_restore(DISP_REG_CONFIG_MUTEX0           );
    reg_restore(DISP_REG_CONFIG_MUTEX0_RST       );
    reg_restore(DISP_REG_CONFIG_MUTEX0_MOD       );
    reg_restore(DISP_REG_CONFIG_MUTEX0_SOF       );
    reg_restore(DISP_REG_CONFIG_MUTEX1_EN        );
    reg_restore(DISP_REG_CONFIG_MUTEX1           );
    reg_restore(DISP_REG_CONFIG_MUTEX1_RST       );
    reg_restore(DISP_REG_CONFIG_MUTEX1_MOD       );
    reg_restore(DISP_REG_CONFIG_MUTEX1_SOF       );
    reg_restore(DISP_REG_CONFIG_MUTEX2_EN        );
    reg_restore(DISP_REG_CONFIG_MUTEX2           );
    reg_restore(DISP_REG_CONFIG_MUTEX2_RST       );
    reg_restore(DISP_REG_CONFIG_MUTEX2_MOD       );
    reg_restore(DISP_REG_CONFIG_MUTEX2_SOF       );
    reg_restore(DISP_REG_CONFIG_MUTEX3_EN        );
    reg_restore(DISP_REG_CONFIG_MUTEX3           );
    reg_restore(DISP_REG_CONFIG_MUTEX3_RST       );
    reg_restore(DISP_REG_CONFIG_MUTEX3_MOD       );
    reg_restore(DISP_REG_CONFIG_MUTEX3_SOF       );
    reg_restore(DISP_REG_CONFIG_MUTEX4_EN        );
    reg_restore(DISP_REG_CONFIG_MUTEX4           );
    reg_restore(DISP_REG_CONFIG_MUTEX4_RST       );
    reg_restore(DISP_REG_CONFIG_MUTEX4_MOD       );
    reg_restore(DISP_REG_CONFIG_MUTEX4_SOF       );
    reg_restore(DISP_REG_CONFIG_MUTEX5_EN        );
    reg_restore(DISP_REG_CONFIG_MUTEX5           );
    reg_restore(DISP_REG_CONFIG_MUTEX5_RST       );
    reg_restore(DISP_REG_CONFIG_MUTEX5_MOD       );
    reg_restore(DISP_REG_CONFIG_MUTEX5_SOF       );
    reg_restore(DISP_REG_CONFIG_MUTEX_DEBUG_OUT_SEL); 
    
    // OVL
    reg_restore(DISP_REG_OVL_STA                       );
    reg_restore(DISP_REG_OVL_INTEN                     );
    reg_restore(DISP_REG_OVL_INTSTA                    );
    reg_restore(DISP_REG_OVL_EN                        );
    reg_restore(DISP_REG_OVL_TRIG                      );
    reg_restore(DISP_REG_OVL_RST                       );
    reg_restore(DISP_REG_OVL_ROI_SIZE                  );
    reg_restore(DISP_REG_OVL_DATAPATH_CON              );
    reg_restore(DISP_REG_OVL_ROI_BGCLR                 );
    reg_restore(DISP_REG_OVL_SRC_CON                   );
    reg_restore(DISP_REG_OVL_L0_CON                    );
    reg_restore(DISP_REG_OVL_L0_SRCKEY                 );
    reg_restore(DISP_REG_OVL_L0_SRC_SIZE               );
    reg_restore(DISP_REG_OVL_L0_OFFSET                 );
    reg_restore(DISP_REG_OVL_L0_ADDR                   );
    reg_restore(DISP_REG_OVL_L0_PITCH                  );
    reg_restore(DISP_REG_OVL_L1_CON                    );
    reg_restore(DISP_REG_OVL_L1_SRCKEY                 );
    reg_restore(DISP_REG_OVL_L1_SRC_SIZE               );
    reg_restore(DISP_REG_OVL_L1_OFFSET                 );
    reg_restore(DISP_REG_OVL_L1_ADDR                   );
    reg_restore(DISP_REG_OVL_L1_PITCH                  );
    reg_restore(DISP_REG_OVL_L2_CON                    );
    reg_restore(DISP_REG_OVL_L2_SRCKEY                 );
    reg_restore(DISP_REG_OVL_L2_SRC_SIZE               );
    reg_restore(DISP_REG_OVL_L2_OFFSET                 );
    reg_restore(DISP_REG_OVL_L2_ADDR                   );
    reg_restore(DISP_REG_OVL_L2_PITCH                  );
    reg_restore(DISP_REG_OVL_L3_CON                    );
    reg_restore(DISP_REG_OVL_L3_SRCKEY                 );
    reg_restore(DISP_REG_OVL_L3_SRC_SIZE               );
    reg_restore(DISP_REG_OVL_L3_OFFSET                 );
    reg_restore(DISP_REG_OVL_L3_ADDR                   );
    reg_restore(DISP_REG_OVL_L3_PITCH                  );
    reg_restore(DISP_REG_OVL_RDMA0_CTRL                );
    reg_restore(DISP_REG_OVL_RDMA0_MEM_START_TRIG      );
    reg_restore(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING     );
    reg_restore(DISP_REG_OVL_RDMA0_MEM_SLOW_CON        );
    reg_restore(DISP_REG_OVL_RDMA0_FIFO_CTRL           );
    reg_restore(DISP_REG_OVL_RDMA1_CTRL                );
    reg_restore(DISP_REG_OVL_RDMA1_MEM_START_TRIG      );
    reg_restore(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING     );
    reg_restore(DISP_REG_OVL_RDMA1_MEM_SLOW_CON        );
    reg_restore(DISP_REG_OVL_RDMA1_FIFO_CTRL           );
    reg_restore(DISP_REG_OVL_RDMA2_CTRL                );
    reg_restore(DISP_REG_OVL_RDMA2_MEM_START_TRIG      );
    reg_restore(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING     );
    reg_restore(DISP_REG_OVL_RDMA2_MEM_SLOW_CON        );
    reg_restore(DISP_REG_OVL_RDMA2_FIFO_CTRL           );
    reg_restore(DISP_REG_OVL_RDMA3_CTRL                );
    reg_restore(DISP_REG_OVL_RDMA3_MEM_START_TRIG      );
    reg_restore(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING     );
    reg_restore(DISP_REG_OVL_RDMA3_MEM_SLOW_CON        );
    reg_restore(DISP_REG_OVL_RDMA3_FIFO_CTRL           );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_R0            );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_R1            );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_G0            );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_G1            );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_B0            );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_B1            );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_YUV_A_0       );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_YUV_A_1       );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_RGB_A_0       );
    reg_restore(DISP_REG_OVL_L0_Y2R_PARA_RGB_A_1       );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_R0            );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_R1            );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_G0            );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_G1            );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_B0            );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_B1            );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_YUV_A_0       );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_YUV_A_1       );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_RGB_A_0       );
    reg_restore(DISP_REG_OVL_L1_Y2R_PARA_RGB_A_1       );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_R0            );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_R1            );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_G0            );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_G1            );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_B0            );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_B1            );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_YUV_A_0       );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_YUV_A_1       );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_RGB_A_0       );
    reg_restore(DISP_REG_OVL_L2_Y2R_PARA_RGB_A_1       );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_R0            );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_R1            );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_G0            );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_G1            );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_B0            );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_B1            );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_YUV_A_0       );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_YUV_A_1       );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_RGB_A_0       );
    reg_restore(DISP_REG_OVL_L3_Y2R_PARA_RGB_A_1       );
    reg_restore(DISP_REG_OVL_DEBUG_MON_SEL             );
    reg_restore(DISP_REG_OVL_RDMA0_MEM_GMC_SETTING2    );
    reg_restore(DISP_REG_OVL_RDMA1_MEM_GMC_SETTING2    );
    reg_restore(DISP_REG_OVL_RDMA2_MEM_GMC_SETTING2    );
    reg_restore(DISP_REG_OVL_RDMA3_MEM_GMC_SETTING2    );
    reg_restore(DISP_REG_OVL_FLOW_CTRL_DBG             );
    reg_restore(DISP_REG_OVL_ADDCON_DBG                );
    reg_restore(DISP_REG_OVL_OUTMUX_DBG                );
    
    // RDMA0
    reg_restore(DISP_REG_RDMA_INT_ENABLE          );
    reg_restore(DISP_REG_RDMA_INT_STATUS          );
    reg_restore(DISP_REG_RDMA_GLOBAL_CON          );
    reg_restore(DISP_REG_RDMA_SIZE_CON_0          );
    reg_restore(DISP_REG_RDMA_SIZE_CON_1          );
    reg_restore(DISP_REG_RDMA_TARGET_LINE         );
    reg_restore(DISP_REG_RDMA_MEM_CON             );
    reg_restore(DISP_REG_RDMA_MEM_START_ADDR      );
    reg_restore(DISP_REG_RDMA_MEM_SRC_PITCH       );
    reg_restore(DISP_REG_RDMA_MEM_GMC_SETTING_0   );
    reg_restore(DISP_REG_RDMA_MEM_SLOW_CON        );
    reg_restore(DISP_REG_RDMA_MEM_GMC_SETTING_1   );
    reg_restore(DISP_REG_RDMA_FIFO_CON            );
    reg_restore(DISP_REG_RDMA_CF_00               );
    reg_restore(DISP_REG_RDMA_CF_01               );
    reg_restore(DISP_REG_RDMA_CF_02               );
    reg_restore(DISP_REG_RDMA_CF_10               );
    reg_restore(DISP_REG_RDMA_CF_11               );
    reg_restore(DISP_REG_RDMA_CF_12               );
    reg_restore(DISP_REG_RDMA_CF_20               );
    reg_restore(DISP_REG_RDMA_CF_21               );
    reg_restore(DISP_REG_RDMA_CF_22               );
    reg_restore(DISP_REG_RDMA_CF_PRE_ADD0         );
    reg_restore(DISP_REG_RDMA_CF_PRE_ADD1         );
    reg_restore(DISP_REG_RDMA_CF_PRE_ADD2         );
    reg_restore(DISP_REG_RDMA_CF_POST_ADD0        );
    reg_restore(DISP_REG_RDMA_CF_POST_ADD1        );
    reg_restore(DISP_REG_RDMA_CF_POST_ADD2        );
    reg_restore(DISP_REG_RDMA_DUMMY               );
    reg_restore(DISP_REG_RDMA_DEBUG_OUT_SEL       );
    
    // RDMA1
    reg_restore(DISP_REG_RDMA_INT_ENABLE          +0x1000 );
    reg_restore(DISP_REG_RDMA_INT_STATUS          +0x1000 );
    reg_restore(DISP_REG_RDMA_GLOBAL_CON          +0x1000 );
    reg_restore(DISP_REG_RDMA_SIZE_CON_0          +0x1000 );
    reg_restore(DISP_REG_RDMA_SIZE_CON_1          +0x1000 );
    reg_restore(DISP_REG_RDMA_TARGET_LINE          +0x1000 );
    reg_restore(DISP_REG_RDMA_MEM_CON             +0x1000 );
    reg_restore(DISP_REG_RDMA_MEM_START_ADDR      +0x1000 );
    reg_restore(DISP_REG_RDMA_MEM_SRC_PITCH       +0x1000 );
    reg_restore(DISP_REG_RDMA_MEM_GMC_SETTING_0   +0x1000 );
    reg_restore(DISP_REG_RDMA_MEM_SLOW_CON        +0x1000 );
    reg_restore(DISP_REG_RDMA_MEM_GMC_SETTING_1   +0x1000 );
    reg_restore(DISP_REG_RDMA_FIFO_CON            +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_00               +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_01               +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_02               +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_10               +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_11               +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_12               +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_20               +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_21               +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_22               +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_PRE_ADD0         +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_PRE_ADD1         +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_PRE_ADD2         +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_POST_ADD0        +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_POST_ADD1        +0x1000 );
    reg_restore(DISP_REG_RDMA_CF_POST_ADD2        +0x1000 );
    reg_restore(DISP_REG_RDMA_DUMMY               +0x1000 );
    reg_restore(DISP_REG_RDMA_DEBUG_OUT_SEL       +0x1000 ); 
#if 0
    // ROTDMA                                    
    reg_restore(DISP_REG_ROT_EN                   );
    reg_restore(DISP_REG_ROT_RESET                );
    reg_restore(DISP_REG_ROT_INTERRUPT_ENABLE     );
    reg_restore(DISP_REG_ROT_INTERRUPT_STATUS     );
    reg_restore(DISP_REG_ROT_CON                  );
    reg_restore(DISP_REG_ROT_GMCIF_CON            );
    reg_restore(DISP_REG_ROT_SRC_CON              );
    reg_restore(DISP_REG_ROT_SRC_BASE_0           );
    reg_restore(DISP_REG_ROT_SRC_BASE_1           );
    reg_restore(DISP_REG_ROT_SRC_BASE_2           );
    reg_restore(DISP_REG_ROT_MF_BKGD_SIZE_IN_BYTE );
    reg_restore(DISP_REG_ROT_MF_SRC_SIZE          );
    reg_restore(DISP_REG_ROT_MF_CLIP_SIZE         );
    reg_restore(DISP_REG_ROT_MF_OFFSET_1          );
    reg_restore(DISP_REG_ROT_MF_PAR               );
    reg_restore(DISP_REG_ROT_SF_BKGD_SIZE_IN_BYTE );
    reg_restore(DISP_REG_ROT_SF_PAR               );
    reg_restore(DISP_REG_ROT_MB_DEPTH             );
    reg_restore(DISP_REG_ROT_MB_BASE              );
    reg_restore(DISP_REG_ROT_MB_CON               );
    reg_restore(DISP_REG_ROT_SB_DEPTH             );
    reg_restore(DISP_REG_ROT_SB_BASE              );
    reg_restore(DISP_REG_ROT_SB_CON               );
    reg_restore(DISP_REG_ROT_VC1_RANGE            );
    reg_restore(DISP_REG_ROT_TRANSFORM_0          );
    reg_restore(DISP_REG_ROT_TRANSFORM_1          );
    reg_restore(DISP_REG_ROT_TRANSFORM_2          );
    reg_restore(DISP_REG_ROT_TRANSFORM_3          );
    reg_restore(DISP_REG_ROT_TRANSFORM_4          );
    reg_restore(DISP_REG_ROT_TRANSFORM_5          );
    reg_restore(DISP_REG_ROT_TRANSFORM_6          );
    reg_restore(DISP_REG_ROT_TRANSFORM_7          );
    reg_restore(DISP_REG_ROT_RESV_DUMMY_0         );
                                                
    // SCL                                      
    reg_restore(DISP_REG_SCL_CTRL                 );
    reg_restore(DISP_REG_SCL_INTEN                );
    reg_restore(DISP_REG_SCL_INTSTA               );
    reg_restore(DISP_REG_SCL_STATUS               );
    reg_restore(DISP_REG_SCL_CFG                  );
    reg_restore(DISP_REG_SCL_INP_CHKSUM           );
    reg_restore(DISP_REG_SCL_OUTP_CHKSUM          );
    reg_restore(DISP_REG_SCL_HRZ_CFG              );
    reg_restore(DISP_REG_SCL_HRZ_SIZE             );
    reg_restore(DISP_REG_SCL_HRZ_FACTOR           );
    reg_restore(DISP_REG_SCL_HRZ_OFFSET           );
    reg_restore(DISP_REG_SCL_VRZ_CFG              );
    reg_restore(DISP_REG_SCL_VRZ_SIZE             );
    reg_restore(DISP_REG_SCL_VRZ_FACTOR           );
    reg_restore(DISP_REG_SCL_VRZ_OFFSET           );
    reg_restore(DISP_REG_SCL_EXT_COEF             );
    reg_restore(DISP_REG_SCL_PEAK_CFG             );
                                                
    // WDMA 0                                    
    reg_restore(DISP_REG_WDMA_INTEN               );
    reg_restore(DISP_REG_WDMA_INTSTA              );
    reg_restore(DISP_REG_WDMA_EN                  );
    reg_restore(DISP_REG_WDMA_RST                 );
    reg_restore(DISP_REG_WDMA_SMI_CON             );
    reg_restore(DISP_REG_WDMA_CFG                 );
    reg_restore(DISP_REG_WDMA_SRC_SIZE            );
    reg_restore(DISP_REG_WDMA_CLIP_SIZE           );
    reg_restore(DISP_REG_WDMA_CLIP_COORD          );
    reg_restore(DISP_REG_WDMA_DST_ADDR            );
    reg_restore(DISP_REG_WDMA_DST_W_IN_BYTE       );
    reg_restore(DISP_REG_WDMA_ALPHA               );
    reg_restore(DISP_REG_WDMA_BUF_ADDR            );
    reg_restore(DISP_REG_WDMA_STA                 );
    reg_restore(DISP_REG_WDMA_BUF_CON1            );
    reg_restore(DISP_REG_WDMA_BUF_CON2            );
    reg_restore(DISP_REG_WDMA_C00                 );
    reg_restore(DISP_REG_WDMA_C02                 );
    reg_restore(DISP_REG_WDMA_C10                 );
    reg_restore(DISP_REG_WDMA_C12                 );
    reg_restore(DISP_REG_WDMA_C20                 );
    reg_restore(DISP_REG_WDMA_C22                 );
    reg_restore(DISP_REG_WDMA_PRE_ADD0            );
    reg_restore(DISP_REG_WDMA_PRE_ADD2            );
    reg_restore(DISP_REG_WDMA_POST_ADD0           );
    reg_restore(DISP_REG_WDMA_POST_ADD2           );
    reg_restore(DISP_REG_WDMA_DST_U_ADDR          );
    reg_restore(DISP_REG_WDMA_DST_V_ADDR          );
    reg_restore(DISP_REG_WDMA_DST_UV_PITCH        );
    reg_restore(DISP_REG_WDMA_DITHER_CON          );
    reg_restore(DISP_REG_WDMA_FLOW_CTRL_DBG       );
    reg_restore(DISP_REG_WDMA_EXEC_DBG            );
    reg_restore(DISP_REG_WDMA_CLIP_DBG            );
#endif
    // WDMA1                                    
    reg_restore(DISP_REG_WDMA_INTEN         +0x1000 );
    reg_restore(DISP_REG_WDMA_INTSTA        +0x1000 );
    reg_restore(DISP_REG_WDMA_EN            +0x1000 );
    reg_restore(DISP_REG_WDMA_RST           +0x1000 );
    reg_restore(DISP_REG_WDMA_SMI_CON       +0x1000 );
    reg_restore(DISP_REG_WDMA_CFG           +0x1000 );
    reg_restore(DISP_REG_WDMA_SRC_SIZE      +0x1000 );
    reg_restore(DISP_REG_WDMA_CLIP_SIZE     +0x1000 );
    reg_restore(DISP_REG_WDMA_CLIP_COORD    +0x1000 );
    reg_restore(DISP_REG_WDMA_DST_ADDR      +0x1000 );
    reg_restore(DISP_REG_WDMA_DST_W_IN_BYTE +0x1000 );
    reg_restore(DISP_REG_WDMA_ALPHA         +0x1000 );
    reg_restore(DISP_REG_WDMA_BUF_ADDR      +0x1000 );
    reg_restore(DISP_REG_WDMA_STA           +0x1000 );
    reg_restore(DISP_REG_WDMA_BUF_CON1      +0x1000 );
    reg_restore(DISP_REG_WDMA_BUF_CON2      +0x1000 );
    reg_restore(DISP_REG_WDMA_C00           +0x1000 );
    reg_restore(DISP_REG_WDMA_C02           +0x1000 );
    reg_restore(DISP_REG_WDMA_C10           +0x1000 );
    reg_restore(DISP_REG_WDMA_C12           +0x1000 );
    reg_restore(DISP_REG_WDMA_C20           +0x1000 );
    reg_restore(DISP_REG_WDMA_C22           +0x1000 );
    reg_restore(DISP_REG_WDMA_PRE_ADD0      +0x1000 );
    reg_restore(DISP_REG_WDMA_PRE_ADD2      +0x1000 );
    reg_restore(DISP_REG_WDMA_POST_ADD0     +0x1000 );
    reg_restore(DISP_REG_WDMA_POST_ADD2     +0x1000 );
    reg_restore(DISP_REG_WDMA_DST_U_ADDR    +0x1000 );
    reg_restore(DISP_REG_WDMA_DST_V_ADDR    +0x1000 );
    reg_restore(DISP_REG_WDMA_DST_UV_PITCH  +0x1000 );
    reg_restore(DISP_REG_WDMA_DITHER_CON    +0x1000 );
    reg_restore(DISP_REG_WDMA_FLOW_CTRL_DBG +0x1000 );
    reg_restore(DISP_REG_WDMA_EXEC_DBG      +0x1000 );
    reg_restore(DISP_REG_WDMA_CLIP_DBG      +0x1000 );

#if 0
    // BLS                                       
    reg_restore(DISP_REG_BLS_EN                   );
    reg_restore(DISP_REG_BLS_RST                  );
    reg_restore(DISP_REG_BLS_INTEN                );
    reg_restore(DISP_REG_BLS_INTSTA               );
    reg_restore(DISP_REG_BLS_SRC_SIZE             );
    reg_restore(DISP_REG_BLS_PWM_DUTY             );
    reg_restore(DISP_REG_BLS_PWM_DUTY_GAIN        );
    reg_restore(DISP_REG_BLS_PWM_CON              );
    reg_restore(DISP_REG_PWM_H_DURATION           );
    reg_restore(DISP_REG_PWM_L_DURATION           );
    reg_restore(DISP_REG_PWM_G_DURATION           );
    reg_restore(DISP_REG_PWM_SEND_DATA0           );
    reg_restore(DISP_REG_PWM_SEND_DATA1           );
    reg_restore(DISP_REG_PWM_WAVE_NUM             );
    reg_restore(DISP_REG_PWM_DATA_WIDTH           );
    reg_restore(DISP_REG_PWM_THRESH               );
    reg_restore(DISP_REG_PWM_SEND_WAVENUM         );
#endif
    //DISP_MSG("disp_reg_restore() release mutex \n");
    //disp_path_release_mutex();
    DISP_MSG("disp_reg_restore() done \n");

    DpEngine_COLORonInit();
    DpEngine_COLORonConfig(fb_width,fb_height);

    // backlight should be turn on last
#if defined(MTK_AAL_SUPPORT)
    disp_bls_init(fb_width, fb_height);
#endif

    MMProfileLogEx(DDP_MMP_Events.BackupReg, MMProfileFlagEnd, 1, 0);
    return 0;        
}

unsigned int disp_intr_status[DISP_MODULE_MAX] = {0};
int disp_intr_restore(void)
{
    // restore intr enable reg 
    //DISP_REG_SET(DISP_REG_ROT_INTERRUPT_ENABLE,   disp_intr_status[DISP_MODULE_ROT]  );  
    //DISP_REG_SET(DISP_REG_SCL_INTEN,              disp_intr_status[DISP_MODULE_SCL]  ); 
    DISP_REG_SET(DISP_REG_OVL_INTEN,              disp_intr_status[DISP_MODULE_OVL]  ); 
    //DISP_REG_SET(DISP_REG_WDMA_INTEN,             disp_intr_status[DISP_MODULE_WDMA0]); 
    DISP_REG_SET(DISP_REG_WDMA_INTEN+0x1000,      disp_intr_status[DISP_MODULE_WDMA1]); 
    DISP_REG_SET(DISP_REG_RDMA_INT_ENABLE,        disp_intr_status[DISP_MODULE_RDMA0]); 
    DISP_REG_SET(DISP_REG_RDMA_INT_ENABLE+0x1000, disp_intr_status[DISP_MODULE_RDMA1]); 
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_INTEN,     disp_intr_status[DISP_MODULE_MUTEX]); 

    return 0;
}

// TODO: color, tdshp, gamma, bls, cmdq intr management should add later
int disp_intr_disable_and_clear(void)
{
    // backup intr enable reg
    //disp_intr_status[DISP_MODULE_ROT] = DISP_REG_GET(DISP_REG_ROT_INTERRUPT_ENABLE);
    //disp_intr_status[DISP_MODULE_SCL] = DISP_REG_GET(DISP_REG_SCL_INTEN);
    disp_intr_status[DISP_MODULE_OVL] = DISP_REG_GET(DISP_REG_OVL_INTEN);
    //disp_intr_status[DISP_MODULE_WDMA0] = DISP_REG_GET(DISP_REG_WDMA_INTEN);
    disp_intr_status[DISP_MODULE_WDMA1] = DISP_REG_GET(DISP_REG_WDMA_INTEN+0x1000);
    disp_intr_status[DISP_MODULE_RDMA0] = DISP_REG_GET(DISP_REG_RDMA_INT_ENABLE);
    disp_intr_status[DISP_MODULE_RDMA1] = DISP_REG_GET(DISP_REG_RDMA_INT_ENABLE+0x1000);
    disp_intr_status[DISP_MODULE_MUTEX] = DISP_REG_GET(DISP_REG_CONFIG_MUTEX_INTEN);
    
    // disable intr
    //DISP_REG_SET(DISP_REG_ROT_INTERRUPT_ENABLE, 0);
    //DISP_REG_SET(DISP_REG_SCL_INTEN, 0);
    DISP_REG_SET(DISP_REG_OVL_INTEN, 0);
    //DISP_REG_SET(DISP_REG_WDMA_INTEN, 0);
    DISP_REG_SET(DISP_REG_WDMA_INTEN+0x1000, 0);
    DISP_REG_SET(DISP_REG_RDMA_INT_ENABLE, 0);
    DISP_REG_SET(DISP_REG_RDMA_INT_ENABLE+0x1000, 0);
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_INTEN, 0);
    
    // clear intr status
    //DISP_REG_SET(DISP_REG_ROT_INTERRUPT_STATUS, 0);  
    //DISP_REG_SET(DISP_REG_SCL_INTSTA, 0);               
    DISP_REG_SET(DISP_REG_OVL_INTSTA, 0);    
    //DISP_REG_SET(DISP_REG_WDMA_INTSTA, 0);    
    DISP_REG_SET(DISP_REG_WDMA_INTSTA+0x1000, 0);    
    DISP_REG_SET(DISP_REG_RDMA_INT_STATUS, 0);    
    DISP_REG_SET(DISP_REG_RDMA_INT_STATUS+0x1000, 0);            
    DISP_REG_SET(DISP_REG_CONFIG_MUTEX_INTSTA, 0);

    return 0;	  
}

int disp_path_clock_on(char* name)
{
    if(name != NULL)
    {
        DISP_MSG("disp_path_power_on, caller:%s \n", name);
    }

    enable_clock(MT_CG_DISP0_LARB2_SMI   , "DDP");
//    enable_clock(MT_CG_DISP0_CMDQ_ENGINE , "DDP");
//    enable_clock(MT_CG_DISP0_CMDQ_SMI    , "DDP");
    
//    enable_clock(MT_CG_DISP0_ROT_ENGINE  , "DDP");    
//    enable_clock(MT_CG_DISP0_ROT_SMI     , "DDP");
//    enable_clock(MT_CG_DISP0_SCL         , "DDP");
//    enable_clock(MT_CG_DISP0_WDMA0_ENGINE, "DDP");
//    enable_clock(MT_CG_DISP0_WDMA0_SMI   , "DDP");

    enable_clock(MT_CG_DISP0_OVL_ENGINE  , "DDP");
    enable_clock(MT_CG_DISP0_OVL_SMI     , "DDP");
    enable_clock(MT_CG_DISP0_COLOR       , "DDP");
//    enable_clock(MT_CG_DISP0_2DSHP       , "DDP");
    enable_clock(MT_CG_DISP0_BLS         , "DDP");
    //enable_clock(MT_CG_DISP0_WDMA1_ENGINE, "DDP");
    //enable_clock(MT_CG_DISP0_WDMA1_SMI   , "DDP");
    enable_clock(MT_CG_DISP0_RDMA0_ENGINE, "DDP");
    enable_clock(MT_CG_DISP0_RDMA0_SMI   , "DDP");
    enable_clock(MT_CG_DISP0_RDMA0_OUTPUT, "DDP");
    
    //enable_clock(MT_CG_DISP0_RDMA1_ENGINE, "DDP");
    //enable_clock(MT_CG_DISP0_RDMA1_SMI   , "DDP");
    //enable_clock(MT_CG_DISP0_RDMA1_OUTPUT, "DDP");
    //enable_clock(MT_CG_DISP0_GAMMA_ENGINE, "DDP");
    //enable_clock(MT_CG_DISP0_GAMMA_PIXEL , "DDP");    

    //enable_clock(MT_CG_DISP0_G2D_ENGINE  , "DDP");
    //enable_clock(MT_CG_DISP0_G2D_SMI     , "DDP");
    
    // restore ddp related registers
    if (strncmp(name, "ipoh_mtkfb", 10))
    {
		if(*pRegBackup != DDP_UNBACKED_REG_MEM)
		{
			  disp_reg_restore();

			  // restore intr enable registers
			  disp_intr_restore();
		}
		else
		{
			  DISP_MSG("disp_path_clock_on(), dose not call disp_reg_restore, cause mem not inited! \n");
		}
    }
    
    
    return 0;
}


int disp_path_clock_off(char* name)
{
    if(name != NULL)
    {
        DISP_MSG("disp_path_power_off, caller:%s \n", name);
    }
    
    // disable intr and clear intr status
    disp_intr_disable_and_clear();
    
    // backup ddp related registers
    disp_reg_backup();
    
//    disable_clock(MT_CG_DISP0_CMDQ_ENGINE , "DDP");
//    disable_clock(MT_CG_DISP0_CMDQ_SMI    , "DDP");
    
//    disable_clock(MT_CG_DISP0_ROT_ENGINE  , "DDP");    
//    disable_clock(MT_CG_DISP0_ROT_SMI     , "DDP");
//    disable_clock(MT_CG_DISP0_SCL         , "DDP");
//    disable_clock(MT_CG_DISP0_WDMA0_ENGINE, "DDP");
//    disable_clock(MT_CG_DISP0_WDMA0_SMI   , "DDP");

    // Better to reset DMA engine before disable their clock
    RDMAStop(0);
    RDMAReset(0);

    WDMAStop(1);
    WDMAReset(1);

    OVLStop();
    OVLReset();    

    disable_clock(MT_CG_DISP0_OVL_ENGINE  , "DDP");
    disable_clock(MT_CG_DISP0_OVL_SMI     , "DDP");
    disable_clock(MT_CG_DISP0_COLOR       , "DDP");
//    disable_clock(MT_CG_DISP0_2DSHP       , "DDP");
    disable_clock(MT_CG_DISP0_BLS         , "DDP");
    //disable_clock(MT_CG_DISP0_WDMA1_ENGINE, "DDP");
    //disable_clock(MT_CG_DISP0_WDMA1_SMI   , "DDP");
    disable_clock(MT_CG_DISP0_RDMA0_ENGINE, "DDP");
    disable_clock(MT_CG_DISP0_RDMA0_SMI   , "DDP");
    disable_clock(MT_CG_DISP0_RDMA0_OUTPUT, "DDP");
    
    //disable_clock(MT_CG_DISP0_RDMA1_ENGINE, "DDP");
    //disable_clock(MT_CG_DISP0_RDMA1_SMI   , "DDP");
    //disable_clock(MT_CG_DISP0_RDMA1_OUTPUT, "DDP");
    //disable_clock(MT_CG_DISP0_GAMMA_ENGINE, "DDP");
    //disable_clock(MT_CG_DISP0_GAMMA_PIXEL , "DDP");    

    
    // DPI can not suspend issue fixed, so remove this workaround
    if(0) //if(g_dst_module==DISP_MODULE_DPI0 || g_dst_module==DISP_MODULE_DPI1)
    {
        DISP_MSG("warning: do not power off MT_CG_DISP0_LARB2_SMI for DPI resume issue\n");
    }
    else
    {
        disable_clock(MT_CG_DISP0_LARB2_SMI   , "DDP");
    } 
    //disable_clock(MT_CG_DISP0_G2D_ENGINE  , "DDP");
    //disable_clock(MT_CG_DISP0_G2D_SMI     , "DDP");

    return 0;
}
#endif

