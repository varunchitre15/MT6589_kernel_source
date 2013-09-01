#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/sched.h>   
#include <mach/irqs.h>
#include <asm/io.h>

#include <mach/mt_smi.h>
#include "smi_reg.h"
#include "smi_common.h"

#include <mach/mt_typedefs.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_irq.h>
#include <mach/m4u.h>
#include <mach/mt_device_apc.h>
#include <mach/sync_write.h>
#ifdef CONFIG_HIBERNATION
#include <mach/mtk_hibernate_dpm.h>
#endif



#define SPCMSG(string, args...)	xlog_printk(ANDROID_LOG_INFO, "SPC", "[pid=%d]"string,current->tgid,##args)
#define SPCTMP(string, args...)  xlog_printk(ANDROID_LOG_INFO, "SPC", "[pid=%d]"string,current->tgid,##args)
#define SPCERR(string, args...) do{\
	xlog_printk(ANDROID_LOG_ERROR, "SPC", "error: "string, ##args); \
	aee_kernel_warning("SPC", "error: "string, ##args);  \
}while(0)

static char spc_name[100];
#define spc_aee_print(string, args...) do{\
	snprintf(spc_name,100, "[SPC]"string, ##args); \
  aee_kernel_warning(spc_name, "[SPC] error:"string,##args);  \
}while(0)



#define SYSTEM_RAM_ADDR 0x1200C000
#define SYSTEM_RAM_SIZE (1024*80) //80k


void* spc_isr_dev = NULL;


int spc_test(int code) 
{
    
    return 0;    
}


void spc_config(MTK_SPC_CONFIG* pCfg)
{
    unsigned int regval;
    int j;

    SPCMSG("spc config prot=(%d,%d,%d,%d), start=0x%x, end=0x%x\n",
        pCfg->domain_0_prot,pCfg->domain_1_prot,
        pCfg->domain_2_prot,pCfg->domain_3_prot,
        pCfg->start,pCfg->end);
   
    //config monitor range 1:
    regval = F_SYSRAM_RANG_ADDR_SET(pCfg->start);
    COM_WriteReg32(REG_SMI_ISPSYS_SRAM_RANG(0), regval);
    
    regval = F_SYSRAM_RANG_ADDR_SET(pCfg->end);
    COM_WriteReg32(REG_SMI_ISPSYS_SRAM_RANG(1), regval);

    //config access type monitor of domains
    //0-no protect; 1-sec rw; 2-sec_rw nonsec_r; 3-no access
    regval =    (pCfg->domain_0_prot<<(0*2))|
                (pCfg->domain_1_prot<<(1*2))|
                (pCfg->domain_2_prot<<(2*2))|
                (pCfg->domain_3_prot<<(3*2));
    
    // we use only region 1 for simple protection
    COM_WriteReg32(REG_SMI_ISPSYS_SRNG_ACTL(1), regval);
    
}
    

EXPORT_SYMBOL(spc_config);

unsigned int spc_status_check()
{
    unsigned int regval;
    int j;
    int vio = 0;
    unsigned int vread, vwrite, vdomain, vport, vaddr;

    for(j=0; j<4; j++)
    {
        regval = COM_ReadReg32(REG_ISPSYS_D_VIO_STA(j));
        if(regval !=0)
        {
            SPCMSG("domain %d violation: reg=0x%x\n", j, regval);
            vio = 1; 
        }

    }

    //check debug info-- write violation; domain=0; port=?; addr=?;
    regval = COM_ReadReg32(REG_ISPSYS_VIO_DBG0);
    vread = F_SYSRAM_VIO_DBG0_RD(regval);
    vwrite = F_SYSRAM_VIO_DBG0_WR(regval);
    vdomain = F_SYSRAM_VIO_DBG0_DOMAIN(regval);
    vport = F_SYSRAM_VIO_DBG0_PORT(regval);
    vaddr = COM_ReadReg32(REG_ISPSYS_VIO_DBG1);

    if(vio)
    {
        spc_aee_print("violation: port=%d,rd=%d,wt=%d,domain=%d,addr=0x%x\n",
            vport, vread, vwrite, vdomain, vaddr);
    }
    SPCMSG("vio debug0=0x%x, debug1=0x%x\n", regval, vaddr);


    //reset
    for(j=0; j<4; j++)
    {
        regval = COM_ReadReg32(REG_ISPSYS_D_VIO_STA(j));
        COM_WriteReg32(REG_ISPSYS_D_VIO_STA(j), regval);
    }
    COM_WriteReg32(REG_ISPSYS_VIO_DBG0, F_SYSRAM_VIO_DBG0_CLR);
  
    return 0;
}


#define ABORT_SMI  0x00400000   
unsigned int spc_clear_irq()
{
    mt65xx_reg_sync_writel(readl(DEVAPC3_D0_VIO_STA) | ABORT_SMI , DEVAPC3_D0_VIO_STA);
   
    mt65xx_reg_sync_writel(0x8, DEVAPC0_DXS_VIO_STA);

    return 0;
}


unsigned int spc_dump_reg()
{
    SPCMSG("SPC Reg Dump Start !\n");
    SPCMSG("(+0x500)SMI_SEN        = 0x%x \n", COM_ReadReg32(REG_SMI_SEN        ));
    SPCMSG("(+0x520)SMI_SRAM_RANGE0= 0x%x \n", COM_ReadReg32(REG_SMI_ISPSYS_SRAM_RANG(0)));
    SPCMSG("(+0x524)SMI_SRAM_RANGE1= 0x%x \n", COM_ReadReg32(REG_SMI_ISPSYS_SRAM_RANG(1)));
    SPCMSG("(+0x528)SMI_SRAM_RANGE2= 0x%x \n", COM_ReadReg32(REG_SMI_ISPSYS_SRAM_RANG(2)));
    SPCMSG("(+0x52C)SMI_SRAM_RANGE3= 0x%x \n", COM_ReadReg32(REG_SMI_ISPSYS_SRAM_RANG(3)));
    SPCMSG("(+0x530)SMI_SRNG_ACTL0 = 0x%x \n", COM_ReadReg32(REG_SMI_ISPSYS_SRNG_ACTL(0)));
    SPCMSG("(+0x534)SMI_SRNG_ACTL1 = 0x%x \n", COM_ReadReg32(REG_SMI_ISPSYS_SRNG_ACTL(1) ));
    SPCMSG("(+0x538)SMI_SRNG_ACTL2 = 0x%x \n", COM_ReadReg32(REG_SMI_ISPSYS_SRNG_ACTL(2) ));
    SPCMSG("(+0x53C)SMI_SRNG_ACTL3 = 0x%x \n", COM_ReadReg32(REG_SMI_ISPSYS_SRNG_ACTL(3) ));
    SPCMSG("(+0x540)SMI_SRNG_ACTL4 = 0x%x \n", COM_ReadReg32(REG_SMI_ISPSYS_SRNG_ACTL(4) ));
    SPCMSG("(+0x550)SMI_D_VIO_CON0 = 0x%x \n", COM_ReadReg32(REG_ISPSYS_D_VIO_CON(0)));
    SPCMSG("(+0x554)SMI_D_VIO_CON1 = 0x%x \n", COM_ReadReg32(REG_ISPSYS_D_VIO_CON(1)));
    SPCMSG("(+0x558)SMI_D_VIO_CON2 = 0x%x \n", COM_ReadReg32(REG_ISPSYS_D_VIO_CON(2) ));
    SPCMSG("(+0x558)SMI_D_VIO_CON3 = 0x%x \n", COM_ReadReg32(REG_ISPSYS_D_VIO_CON(3) ));
    SPCMSG("(+0x560)SMI_D_VIO_STA0 = 0x%x \n", COM_ReadReg32(REG_ISPSYS_D_VIO_STA(0) ));
    SPCMSG("(+0x564)SMI_D_VIO_STA1 = 0x%x \n", COM_ReadReg32(REG_ISPSYS_D_VIO_STA(1) ));
    SPCMSG("(+0x568)SMI_D_VIO_STA2 = 0x%x \n", COM_ReadReg32(REG_ISPSYS_D_VIO_STA(2) ));
    SPCMSG("(+0x568)SMI_D_VIO_STA2 = 0x%x \n", COM_ReadReg32(REG_ISPSYS_D_VIO_STA(3) ));
    SPCMSG("(+0x570)SMI_VIO_DBG0   = 0x%x \n", COM_ReadReg32(REG_ISPSYS_VIO_DBG0   ));
    SPCMSG("(+0x570)SMI_VIO_DBG1   = 0x%x \n", COM_ReadReg32(REG_ISPSYS_VIO_DBG1   ));
    SPCMSG("(+0x5C0)SMI_SECUR_CON0 = 0x%x \n", COM_ReadReg32(REG_SMI_SECUR_CON(0) ));
    SPCMSG("(+0x5C4)SMI_SECUR_CON1 = 0x%x \n", COM_ReadReg32(REG_SMI_SECUR_CON(1) ));
    SPCMSG("(+0x5C8)SMI_SECUR_CON2 = 0x%x \n", COM_ReadReg32(REG_SMI_SECUR_CON(2) ));
    SPCMSG("(+0x5CC)SMI_SECUR_CON3 = 0x%x \n", COM_ReadReg32(REG_SMI_SECUR_CON(3) ));
    SPCMSG("(+0x5D0)SMI_SECUR_CON4 = 0x%x \n", COM_ReadReg32(REG_SMI_SECUR_CON(4) ));
    SPCMSG("(+0x5D4)SMI_SECUR_CON5 = 0x%x \n", COM_ReadReg32(REG_SMI_SECUR_CON(5) ));
    SPCMSG("(+0x5D8)SMI_SECUR_CON6 = 0x%x \n", COM_ReadReg32(REG_SMI_SECUR_CON(6) ));
    SPCMSG("(+0x5DC)SMI_SECUR_CON7 = 0x%x \n", COM_ReadReg32(REG_SMI_SECUR_CON(7) ));	
    SPCMSG("SPC Reg Dump End !\n");
    
    return 0;
}

EXPORT_SYMBOL(spc_dump_reg);

static irqreturn_t MTK_SPC_isr(int irq, void *dev_id)
{
    if (( readl(DEVAPC0_DXS_VIO_STA) != 0x8) && ( spc_isr_dev != dev_id))
    {
    	SPCMSG("NOT spc abort \n");
        return IRQ_NONE;
    }

    spc_status_check();    	

    spc_clear_irq();
    	
    return IRQ_HANDLED;
}


unsigned int spc_register_isr(void* dev)
{
    unsigned int ret;

    ret = request_irq(MT_APARM_DOMAIN_IRQ_ID, (irq_handler_t)MTK_SPC_isr, IRQF_TRIGGER_LOW | IRQF_SHARED, "spc", dev);
    if(ret != 0)
    {
        SPCMSG("request SPC IRQ line failed! ret=0x%x \n", ret);
        return ret;
    }
    else
    {
        spc_isr_dev = dev;
        SPCMSG("spc_register_isr success! \n");
    }

    return 0;
}

#ifdef CONFIG_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
int spc_pm_restore_noirq(struct device *device)
{
    mt_irq_set_sens(MT_APARM_DOMAIN_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT_APARM_DOMAIN_IRQ_ID, MT65xx_POLARITY_LOW);

    return 0;
}
#endif

int MTK_SPC_Init(void* dev)
{

    SPCMSG("MTK_SPC_init() \n");

    mt65xx_reg_sync_writel(readl(DEVAPC0_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC0_APC_CON);
    mt65xx_reg_sync_writel(readl(DEVAPC3_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC3_APC_CON);
    mt65xx_reg_sync_writel(readl(DEVAPC0_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC0_PD_APC_CON);
    mt65xx_reg_sync_writel(readl(DEVAPC3_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC3_PD_APC_CON);

    mt65xx_reg_sync_writel(0x0000007F, DEVAPC0_DXS_VIO_STA);
    //writel(0x00FF00FB, AP_DEVAPC0_DXS_VIO_MASK);  // 0xfb:MM, 0xfd:EMI, 0xf9:Both
    mt65xx_reg_sync_writel(readl(DEVAPC0_DXS_VIO_MASK)&(~0x8), DEVAPC0_DXS_VIO_MASK);  // 0xfb:MM, 0xfd:EMI, 0xf9:Both

    mt65xx_reg_sync_writel(readl(DEVAPC3_D0_VIO_STA) | ABORT_SMI , DEVAPC3_D0_VIO_STA);
    mt65xx_reg_sync_writel(readl(DEVAPC3_D0_VIO_MASK) & ~ABORT_SMI , DEVAPC3_D0_VIO_MASK);
    
    spc_register_isr(dev);

#ifdef CONFIG_MTK_HIBERNATION
    register_swsusp_restore_noirq_func(ID_M_SPC, spc_pm_restore_noirq, NULL);
#endif

    return 0;

}

EXPORT_SYMBOL(MTK_SPC_Init);


