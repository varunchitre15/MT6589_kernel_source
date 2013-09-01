#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/xlog.h>
#include <linux/platform_device.h>


#include "mach/mt_reg_base.h"
#include "mach/mt_device_apc.h"
#include "mach/mt_typedefs.h"
#include "mach/sync_write.h"
#include "mach/irqs.h"
#ifdef CONFIG_MTK_HIBERNATION
#include "mach/mtk_hibernate_dpm.h"
#endif
#include "devapc.h"


static DEFINE_SPINLOCK(g_devapc_lock);
static unsigned long g_devapc_flags;
static BOOL g_usb_protected = FALSE;

static struct cdev* g_devapc_ctrl = NULL;


/*
 * set_module_apc: set module permission on device apc.
 * @module: the moudle to specify permission
 * @devapc_num: device apc index number (device apc 0 or 1)
 * @domain_num: domain index number (AP or MD domain)
 * @permission_control: specified permission
 * no return value.
 */
static void set_module_apc(unsigned int module, DEVAPC_NUM devapc_num, 
    E_MASK_DOM domain_num , APC_ATTR permission_control)
{

    if (E_DEVAPC0 == devapc_num )
        {
            if ( module < (MOD_NO_IN_1_DEVAPC) )
            {
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(0,0,0, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(0,1,0, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(0,2,0, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(0,3,0, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");
                }
            }
            else
            {
                module -= MOD_NO_IN_1_DEVAPC;
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(0,0,1, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(0,1,1, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(0,2,1, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(0,3,1, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");
                }
            }
        }
        else if (E_DEVAPC1 == devapc_num )
        {
            if ( module < (MOD_NO_IN_1_DEVAPC) )
            {
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(1,0,0, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(1,1,0, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(1,2,0, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(1,3,0, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");                  
                }
            }
            else
            {
                module -= MOD_NO_IN_1_DEVAPC;
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(1,0,1, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(1,1,1, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(1,2,1, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(1,3,1, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");                   
                }
            }
        }
        else if (E_DEVAPC2 == devapc_num )
        {
            if ( module < (MOD_NO_IN_1_DEVAPC) )
            {
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(2,0,0, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(2,1,0, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(2,2,0, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(2,3,0, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");                  
                }
            }
            else
            {
                module -= MOD_NO_IN_1_DEVAPC;
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(2,0,1, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(2,1,1, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(2,2,1, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(2,3,1, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");                   
                }
            }
        }
        else if (E_DEVAPC3 == devapc_num )
        {
            if ( module < (MOD_NO_IN_1_DEVAPC) )
            {
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(3,0,0, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(3,1,0, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(3,2,0, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(3,3,0, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");                  
                }
            }
            else
            {
                module -= MOD_NO_IN_1_DEVAPC;
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(3,0,1, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(3,1,1, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(3,2,1, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(3,3,1, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");                   
                }
            }
        }
        else if (E_DEVAPC4 == devapc_num )
        {
            if ( module < (MOD_NO_IN_1_DEVAPC) )
            {
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(4,0,0, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(4,1,0, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(4,2,0, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(4,3,0, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");                  
                }
            }
            else
            {
                module -= MOD_NO_IN_1_DEVAPC;
                if (E_AP_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(4,0,1, module, permission_control);
                }
                else if (E_MD1_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(4,1,1, module, permission_control);
                }
                else if (E_MD2_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(4,2,1, module, permission_control);
                }
                else if (E_MM_MCU == domain_num)
                {
                    SET_SINGLE_MODULE(4,3,1, module, permission_control);
                }
                else
                {
                    xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR, The setting is error, please check if domain master setting is correct or not !\n");                   
                }
            }
        }
}



/*
 * unmask_module_irq: unmask device apc irq for specified module.
 * @module: the moudle to unmask
 * @devapc_num: device apc index number (device apc 0 or 1)
 * @domain_num: domain index number (AP or MD domain)
 * no return value.
 */
static void unmask_module_irq(unsigned int module, DEVAPC_NUM devapc_num, E_MASK_DOM 
    domain_num)
{
    unsigned int module_index = (0x1 << module);

    if (E_DEVAPC0 == devapc_num )
	{
		if (E_AP_MCU == domain_num)
		{
		    UNMASK_SINGLE_MODULE_IRQ(0, 0, module_index);
		}
        else if (E_MD1_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(0, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(0, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(0, 3, module_index);
        }
	}
	else if (E_DEVAPC1 == devapc_num )
	{
	    if (E_AP_MCU == domain_num)
		{
		    UNMASK_SINGLE_MODULE_IRQ(1, 0, module_index);
		}
        else if (E_MD1_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(1, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(1, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(1, 3, module_index);
        }
	}
	else if (E_DEVAPC2 == devapc_num )
	{
	    if (E_AP_MCU == domain_num)
		{
		    UNMASK_SINGLE_MODULE_IRQ(2, 0, module_index);
		}
        else if (E_MD1_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(2, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(2, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(2, 3, module_index);
        }
	}
	else if (E_DEVAPC3 == devapc_num )
	{
	    if (E_AP_MCU == domain_num)
		{
		    UNMASK_SINGLE_MODULE_IRQ(3, 0, module_index);
		}
        else if (E_MD1_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(3, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(3, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(3, 3, module_index);
        }
	}
	else if (E_DEVAPC4 == devapc_num )
	{
	    if (E_AP_MCU == domain_num)
		{
		    UNMASK_SINGLE_MODULE_IRQ(4, 0, module_index);
		}
        else if (E_MD1_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(4, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(4, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            UNMASK_SINGLE_MODULE_IRQ(4, 3, module_index);
        }
	}
}



/*
 * clear_vio_status: clear violation status for each module.
 * @module: the moudle to clear violation status
 * @devapc_num: device apc index number (device apc 0 or 1)
 * @domain_num: domain index number (AP or MD domain)
 * no return value.
 */
static void clear_vio_status(unsigned int module, DEVAPC_NUM devapc_num, E_MASK_DOM 
    domain_num)
{
    unsigned int module_index = (0x1 << module);

    if (E_DEVAPC0 == devapc_num )
    {
        if (E_AP_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(0, 0, module_index);
        }
        else if (E_MD1_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(0, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(0, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(0, 3, module_index);
        }
    }
    else if (E_DEVAPC1 == devapc_num )
    {
        if (E_AP_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(1, 0, module_index);
        }
        else if (E_MD1_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(1, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(1, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(1, 3, module_index);
        }
    }
    else if (E_DEVAPC2 == devapc_num )
    {
        if (E_AP_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(2, 0, module_index);
        }
        else if (E_MD1_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(2, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(2, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(2, 3, module_index);
        }
    }
    else if (E_DEVAPC3 == devapc_num )
    {
        if (E_AP_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(3, 0, module_index);
        }
        else if (E_MD1_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(3, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(3, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(3, 3, module_index);
        }
    }
    else if (E_DEVAPC4 == devapc_num )
    {
        if (E_AP_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(4, 0, module_index);
        }
        else if (E_MD1_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(4, 1, module_index);
        }
        else if (E_MD2_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(4, 2, module_index);
        }
        else if (E_MM_MCU == domain_num)
        {
            CLEAR_SINGLE_VIO_STA(4, 3, module_index);
        }
    }
    else
    {
        xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] ERROR: clear_vio_status , The setting is error, please check if the setting is correct or not !\n");  
    }
}


static irqreturn_t devapc_violation_irq(int irq, void *dev_id)
{  
    unsigned int dbg0 = 0, dbg1 = 0;
    unsigned int master_ID;
    unsigned int domain_ID;
    unsigned int r_w_violation;
   //unsigned int timeout = 0;
    DEVAPC_NUM apc_index = E_MAX_DEVAPC;
    int module_index;
    
    if (( (readl(DEVAPC0_DXS_VIO_STA) & 0xF) == 0) && (&g_devapc_ctrl != dev_id) )
    {
        xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"[DEVAPC] ERROR AP_DEVAPC0_DXS_VIO_STA not device apc AP/MM violation!");
        return IRQ_NONE;
    }

    spin_lock_irqsave(&g_devapc_lock, g_devapc_flags);

    
    if ( (readl(DEVAPC0_DXS_VIO_STA) & 0x1) > 0)
    {
        dbg0 = readl(DEVAPC0_VIO_DBG0);
        dbg1 = readl(DEVAPC0_VIO_DBG1);
        apc_index = E_DEVAPC0;
        //xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"Apc 0 vio \n");
    }
    else if ( (readl(DEVAPC0_DXS_VIO_STA) & 0x2) > 0)
    {
        dbg0 = readl(DEVAPC1_VIO_DBG0);
        dbg1 = readl(DEVAPC1_VIO_DBG1);
        apc_index = E_DEVAPC1;
        //xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"Apc 1 vio \n");
    }
    else if ( (readl(DEVAPC0_DXS_VIO_STA) & 0x4) > 0)
    {
        dbg0 = readl(DEVAPC2_VIO_DBG0);
        dbg1 = readl(DEVAPC2_VIO_DBG1);
        apc_index = E_DEVAPC2;
        //xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"Apc 2 vio \n");
    }
    else if ( (readl(DEVAPC0_DXS_VIO_STA) & 0x8) > 0)
    {
        dbg0 = readl(DEVAPC3_VIO_DBG0);
        dbg1 = readl(DEVAPC3_VIO_DBG1);
        apc_index = E_DEVAPC3;
        //xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"Apc 3 vio \n");
    }
    else if ( (readl(DEVAPC0_DXS_VIO_STA) & 0x10) > 0)
    {
        dbg0 = readl(DEVAPC4_VIO_DBG0);
        dbg1 = readl(DEVAPC4_VIO_DBG1);
        apc_index = E_DEVAPC4;
        //xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,Apc 4 vio \n");
    }
      
    master_ID = dbg0 & 0x000007FF;
    domain_ID = (dbg0 >>12) & 0x00000003;
    r_w_violation = (dbg0 >> 28) & 0x00000003;
    
    //xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"Current Proc : \"%s \" (pid: %i) \n", current->comm, current->pid);


    if(r_w_violation == 1)
    {
      xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"Vio Addr:0x%x , Master ID:0x%x , Dom ID:0x%x, W\n", dbg1, master_ID, domain_ID);
    }
    else
    {
      xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"Vio Addr:0x%x , Master ID:0x%x , Dom ID:0x%x, R\n", dbg1, master_ID, domain_ID);
    }


    if (E_DEVAPC0 == apc_index)
    {
        for (module_index = 0; module_index< (sizeof(D_APC0_Devices)/sizeof(DEVICE_INFO)); module_index++)
        {
            if (NULL == D_APC0_Devices[module_index].device_name)
                break;
                   
            if (TRUE == D_APC0_Devices[module_index].forbidden)
                clear_vio_status(module_index, E_DEVAPC0, E_AP_MCU);
        }
        
        mt65xx_reg_sync_writel(0x1, DEVAPC0_DXS_VIO_STA);
        /*
        while((( readl(DEVAPC0_DXS_VIO_STA) & 0x1) != 0) && (timeout < MAX_TIMEOUT)){
            timeout++;
        }
        */
        
        mt65xx_reg_sync_writel(0x80000000 , DEVAPC0_VIO_DBG0);
        dbg0 = readl(DEVAPC0_VIO_DBG0);
        dbg1 = readl(DEVAPC0_VIO_DBG1);
    }
    else if (E_DEVAPC1 == apc_index)
    {
        for (module_index = 0; module_index< (sizeof(D_APC1_Devices)/sizeof(DEVICE_INFO)); module_index++)
        {
            if (NULL == D_APC1_Devices[module_index].device_name)
                break;
                   
            if (TRUE == D_APC1_Devices[module_index].forbidden)
                clear_vio_status(module_index, E_DEVAPC1, E_AP_MCU);
        }

        mt65xx_reg_sync_writel(0x2, DEVAPC0_DXS_VIO_STA);
        /*
        while((( readl(DEVAPC0_DXS_VIO_STA) & 0x2) != 0) && (timeout < MAX_TIMEOUT)){
            timeout++;
        }
        */

        mt65xx_reg_sync_writel(0x80000000 , DEVAPC1_VIO_DBG0);        
        dbg0 = readl(DEVAPC1_VIO_DBG0);
        dbg1 = readl(DEVAPC1_VIO_DBG1);
    }
    else if (E_DEVAPC2 == apc_index)
    {
        for (module_index = 0; module_index< (sizeof(D_APC2_Devices)/sizeof(DEVICE_INFO)); module_index++)
        {
            if (NULL == D_APC2_Devices[module_index].device_name)
                break;
                   
            if (TRUE == D_APC2_Devices[module_index].forbidden)
                clear_vio_status(module_index, E_DEVAPC2, E_AP_MCU);
        }

        mt65xx_reg_sync_writel(0x4, DEVAPC0_DXS_VIO_STA);
        /*
        while((( readl(DEVAPC0_DXS_VIO_STA) & 0x4) != 0) && (timeout < MAX_TIMEOUT)){
            timeout++;
        }
        */

        mt65xx_reg_sync_writel(0x80000000 , DEVAPC2_VIO_DBG0);        
        dbg0 = readl(DEVAPC2_VIO_DBG0);
        dbg1 = readl(DEVAPC2_VIO_DBG1);
    }
    else if (E_DEVAPC3 == apc_index)
    {
        for (module_index = 0; module_index< (sizeof(D_APC3_Devices)/sizeof(DEVICE_INFO)); module_index++)
        {
            if (NULL == D_APC3_Devices[module_index].device_name)
                break;
                   
            if (TRUE == D_APC3_Devices[module_index].forbidden)
                clear_vio_status(module_index, E_DEVAPC3, E_AP_MCU);
        }

        mt65xx_reg_sync_writel(0x8, DEVAPC0_DXS_VIO_STA);
        /*
        while((( readl(DEVAPC0_DXS_VIO_STA) & 0x8) != 0) && (timeout < MAX_TIMEOUT)){
            timeout++;
        }
        */

        mt65xx_reg_sync_writel(0x80000000 , DEVAPC3_VIO_DBG0);        
        dbg0 = readl(DEVAPC3_VIO_DBG0);
        dbg1 = readl(DEVAPC3_VIO_DBG1);
    }
    else if (E_DEVAPC4 == apc_index)
    {
        for (module_index = 0; module_index< (sizeof(D_APC4_Devices)/sizeof(DEVICE_INFO)); module_index++)
        {
            if (NULL == D_APC4_Devices[module_index].device_name)
                break;
                   
            if (TRUE == D_APC4_Devices[module_index].forbidden)
                clear_vio_status(module_index, E_DEVAPC4, E_AP_MCU);
        }

        mt65xx_reg_sync_writel(0x8, DEVAPC0_DXS_VIO_STA);
        /*
        while((( readl(DEVAPC0_DXS_VIO_STA) & 0x10) != 0) && (timeout < MAX_TIMEOUT)){
            timeout++;
        }
        */

        mt65xx_reg_sync_writel(0x80000000 , DEVAPC4_VIO_DBG0);        
        dbg0 = readl(DEVAPC4_VIO_DBG0);
        dbg1 = readl(DEVAPC4_VIO_DBG1);
    }

    if ((dbg0 !=0) || (dbg1 !=0)) 
    {
        xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] FAILED!\n");
        xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] DBG0 = %x, DBG1 = %x\n", dbg0, dbg1);
    }

    spin_unlock_irqrestore(&g_devapc_lock, g_devapc_flags);
  
    return IRQ_HANDLED;
}


static void init_devpac(void)
{

   
   mt65xx_reg_sync_writel(readl(0xF0001040) &  (0xFFFFFFFF ^ (1<<6)), 0xF0001040);    
   
   // clear the violation
   mt65xx_reg_sync_writel(0x80000000, DEVAPC0_VIO_DBG0); // clear apc0 dbg info if any
   mt65xx_reg_sync_writel(0x80000000, DEVAPC1_VIO_DBG0); // clear apc1 dbg info if any
   mt65xx_reg_sync_writel(0x80000000, DEVAPC2_VIO_DBG0); // clear apc2 dbg info if any
   mt65xx_reg_sync_writel(0x80000000, DEVAPC3_VIO_DBG0); // clear apc3 dbg info if any
   mt65xx_reg_sync_writel(0x80000000, DEVAPC4_VIO_DBG0); // clear apc4 dbg info if any
   
   mt65xx_reg_sync_writel(readl(DEVAPC0_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), DEVAPC0_APC_CON);
   mt65xx_reg_sync_writel(readl(DEVAPC1_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), DEVAPC1_APC_CON);
   mt65xx_reg_sync_writel(readl(DEVAPC2_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), DEVAPC2_APC_CON);
   mt65xx_reg_sync_writel(readl(DEVAPC3_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), DEVAPC3_APC_CON);
   mt65xx_reg_sync_writel(readl(DEVAPC4_APC_CON) &  (0xFFFFFFFF ^ (1<<2)), DEVAPC4_APC_CON);
   mt65xx_reg_sync_writel(readl(DEVAPC0_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC0_PD_APC_CON);
   mt65xx_reg_sync_writel(readl(DEVAPC1_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC1_PD_APC_CON);
   mt65xx_reg_sync_writel(readl(DEVAPC2_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC2_PD_APC_CON);
   mt65xx_reg_sync_writel(readl(DEVAPC3_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC3_PD_APC_CON);
   mt65xx_reg_sync_writel(readl(DEVAPC4_PD_APC_CON) & (0xFFFFFFFF ^ (1<<2)), DEVAPC4_PD_APC_CON);
   
   // clean violation status & unmask device apc 0 & 1 
   mt65xx_reg_sync_writel(0x0000007F, DEVAPC0_DXS_VIO_STA);
   mt65xx_reg_sync_writel(0x00FF00F0, DEVAPC0_DXS_VIO_MASK);
}

/*
 * start_devapc: start device apc for MD
 */
void start_devapc(void)
{

    int module_index = 0;

    init_devpac();

    for (module_index = 0; module_index<(sizeof(D_APC0_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
        if (NULL == D_APC0_Devices[module_index].device_name)
            break;
            
        if (TRUE == D_APC0_Devices[module_index].forbidden)
        {
            clear_vio_status(module_index, E_DEVAPC0, E_AP_MCU);
            unmask_module_irq(module_index, E_DEVAPC0 , E_AP_MCU);
            set_module_apc(module_index, E_DEVAPC0, E_MD1_MCU, E_L3);
            set_module_apc(module_index, E_DEVAPC0, E_MD2_MCU, E_L3);
        }
    }

    for (module_index = 0; module_index<(sizeof(D_APC1_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
        if (NULL == D_APC1_Devices[module_index].device_name)
            break;
            
        if (TRUE == D_APC1_Devices[module_index].forbidden)
        {
            clear_vio_status(module_index, E_DEVAPC1, E_AP_MCU);
            unmask_module_irq(module_index, E_DEVAPC1 , E_AP_MCU);
            set_module_apc(module_index, E_DEVAPC1, E_MD1_MCU, E_L3);
            set_module_apc(module_index, E_DEVAPC1, E_MD2_MCU, E_L3);
        }
    }

    for (module_index = 0; module_index<(sizeof(D_APC2_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
        if (NULL == D_APC2_Devices[module_index].device_name)
            break;
            
        if (TRUE == D_APC2_Devices[module_index].forbidden)
        {
            clear_vio_status(module_index, E_DEVAPC2, E_AP_MCU);
            unmask_module_irq(module_index, E_DEVAPC2 , E_AP_MCU);
            set_module_apc(module_index, E_DEVAPC2, E_MD1_MCU, E_L3);
            set_module_apc(module_index, E_DEVAPC2, E_MD2_MCU, E_L3);
        }
    }

    for (module_index = 0; module_index<(sizeof(D_APC3_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
        if (NULL == D_APC3_Devices[module_index].device_name)
            break;
            
        if (TRUE == D_APC3_Devices[module_index].forbidden)
        {
            clear_vio_status(module_index, E_DEVAPC3, E_AP_MCU);
            unmask_module_irq(module_index, E_DEVAPC3 , E_AP_MCU);
            set_module_apc(module_index, E_DEVAPC3, E_MD1_MCU, E_L3);
            set_module_apc(module_index, E_DEVAPC3, E_MD2_MCU, E_L3);
        }
    }

    for (module_index = 0; module_index< (sizeof(D_APC4_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
        if (NULL == D_APC4_Devices[module_index].device_name)
            break;
            
        if (TRUE == D_APC4_Devices[module_index].forbidden)
        {
            clear_vio_status(module_index, E_DEVAPC4, E_AP_MCU);
            unmask_module_irq(module_index, E_DEVAPC4 , E_AP_MCU);
            set_module_apc(module_index, E_DEVAPC4, E_MD1_MCU, E_L3);
            set_module_apc(module_index, E_DEVAPC4, E_MD2_MCU, E_L3);
        }
    }

    /* for EMI */
    mt65xx_reg_sync_writel(readl(DEVAPC1_D0_VIO_STA) | ABORT_EMI , DEVAPC1_D0_VIO_STA);
    mt65xx_reg_sync_writel(readl(DEVAPC1_D0_VIO_MASK) & ~ABORT_EMI , DEVAPC1_D0_VIO_MASK);
    mt65xx_reg_sync_writel(0x2, DEVAPC0_DXS_VIO_STA);
    
}

/*
 * start_usb_protection: start usb protection 
 */
void start_usb_protection(void)
{

    int module_index = 0;

    init_devpac();

    module_index = 22;
    
    clear_vio_status(module_index, E_DEVAPC2, E_AP_MCU);
    unmask_module_irq(module_index, E_DEVAPC2 , E_AP_MCU);
    set_module_apc(module_index, E_DEVAPC2, E_AP_MCU, E_L3);
    set_module_apc(module_index, E_DEVAPC2, E_MD1_MCU, E_L3);
    set_module_apc(module_index, E_DEVAPC2, E_MD2_MCU, E_L3);
    set_module_apc(module_index, E_DEVAPC2, E_MM_MCU, E_L3);

    module_index = 23;

    clear_vio_status(module_index, E_DEVAPC2, E_AP_MCU);
    unmask_module_irq(module_index, E_DEVAPC2 , E_AP_MCU);
    set_module_apc(module_index, E_DEVAPC2, E_AP_MCU, E_L3);
    set_module_apc(module_index, E_DEVAPC2, E_MD1_MCU, E_L3);
    set_module_apc(module_index, E_DEVAPC2, E_MD2_MCU, E_L3);
    set_module_apc(module_index, E_DEVAPC2, E_MM_MCU, E_L3);
        
    module_index = 29;
    
    clear_vio_status(module_index, E_DEVAPC2, E_AP_MCU);
    unmask_module_irq(module_index, E_DEVAPC2 , E_AP_MCU);
    set_module_apc(module_index, E_DEVAPC2, E_AP_MCU, E_L3);
    set_module_apc(module_index, E_DEVAPC2, E_MD1_MCU, E_L3);
    set_module_apc(module_index, E_DEVAPC2, E_MD2_MCU, E_L3);
    set_module_apc(module_index, E_DEVAPC2, E_MM_MCU, E_L3);

    g_usb_protected = TRUE;
   
}

/*
 * stop_usb_protection: start usb protection 
 */
void stop_usb_protection(void)
{

    int module_index = 0;

    module_index = 22;
    
    set_module_apc(module_index, E_DEVAPC2, E_AP_MCU, E_L0);
    set_module_apc(module_index, E_DEVAPC2, E_MD1_MCU, E_L0);
    set_module_apc(module_index, E_DEVAPC2, E_MD2_MCU, E_L0);
    set_module_apc(module_index, E_DEVAPC2, E_MM_MCU, E_L0);

    module_index = 23;

    set_module_apc(module_index, E_DEVAPC2, E_AP_MCU, E_L0);
    set_module_apc(module_index, E_DEVAPC2, E_MD1_MCU, E_L0);
    set_module_apc(module_index, E_DEVAPC2, E_MD2_MCU, E_L0);
    set_module_apc(module_index, E_DEVAPC2, E_MM_MCU, E_L0);
        
    module_index = 29;
    
    set_module_apc(module_index, E_DEVAPC2, E_AP_MCU, E_L0);
    set_module_apc(module_index, E_DEVAPC2, E_MD1_MCU, E_L0);
    set_module_apc(module_index, E_DEVAPC2, E_MD2_MCU, E_L0);
    set_module_apc(module_index, E_DEVAPC2, E_MM_MCU, E_L0);

    g_usb_protected = FALSE;

}

/*
 * test_devapc: test device apc mechanism
 */
void test_devapc(void)
{

    int module_index = 0;

    init_devpac();

    for (module_index = 0; module_index< (sizeof(D_APC0_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
      if (NULL == D_APC0_Devices[module_index].device_name)
          break;
          
      if (TRUE == D_APC0_Devices[module_index].forbidden)
      {
          clear_vio_status(module_index, E_DEVAPC0, E_AP_MCU);
          unmask_module_irq(module_index, E_DEVAPC0 , E_AP_MCU);
          set_module_apc(module_index, E_DEVAPC0, E_AP_MCU, E_L3);
      }
    }

    for (module_index = 0; module_index< (sizeof(D_APC1_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
      if (NULL == D_APC1_Devices[module_index].device_name)
          break;
          
      if (TRUE == D_APC1_Devices[module_index].forbidden)
      {
          clear_vio_status(module_index, E_DEVAPC1, E_AP_MCU);
          unmask_module_irq(module_index, E_DEVAPC1 , E_AP_MCU);
          set_module_apc(module_index, E_DEVAPC1, E_AP_MCU, E_L3);
      }
    }

    for (module_index = 0; module_index< (sizeof(D_APC2_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
      if (NULL == D_APC2_Devices[module_index].device_name)
          break;
          
      if (TRUE == D_APC2_Devices[module_index].forbidden)
      {
          clear_vio_status(module_index, E_DEVAPC2, E_AP_MCU);
          unmask_module_irq(module_index, E_DEVAPC2 , E_AP_MCU);
          set_module_apc(module_index, E_DEVAPC2, E_AP_MCU, E_L3);
      }
    }

    for (module_index = 0; module_index< (sizeof(D_APC3_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
      if (NULL == D_APC3_Devices[module_index].device_name)
          break;
          
      if (TRUE == D_APC3_Devices[module_index].forbidden)
      {
          clear_vio_status(module_index, E_DEVAPC3, E_AP_MCU);
          unmask_module_irq(module_index, E_DEVAPC3 , E_AP_MCU);
          set_module_apc(module_index, E_DEVAPC3, E_AP_MCU, E_L3);
      }
    }

    for (module_index = 0; module_index< (sizeof(D_APC4_Devices)/sizeof(DEVICE_INFO)); module_index++)
    {
      if (NULL == D_APC4_Devices[module_index].device_name)
          break;
          
      if (TRUE == D_APC4_Devices[module_index].forbidden)
      {
          clear_vio_status(module_index, E_DEVAPC4, E_AP_MCU);
          unmask_module_irq(module_index, E_DEVAPC4 , E_AP_MCU);
          set_module_apc(module_index, E_DEVAPC4, E_AP_MCU, E_L3);
      }
    }
}


static int devapc_probe(struct platform_device *dev)
{
    xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"[DEVAPC] module probe. \n");
    start_devapc();
    return 0;
}


static int devapc_remove(struct platform_device *dev)
{
    return 0;
}

static int devapc_suspend(struct platform_device *dev, pm_message_t state)
{
    return 0;
}

static int devapc_resume(struct platform_device *dev)
{
    //xlog_printk(ANDROID_LOG_DEBUG, DEVAPC_TAG ,"[DEVAPC] module resume. \n");
    start_devapc();

    if (g_usb_protected){
        start_usb_protection();
    }else{
        stop_usb_protection();
    }
    
    return 0;
}

#ifdef CONFIG_MTK_HIBERNATION
extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
int devapc_pm_restore_noirq(struct device *device)
{
    mt_irq_set_sens(MT_APARM_DOMAIN_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
    mt_irq_set_polarity(MT_APARM_DOMAIN_IRQ_ID, MT65xx_POLARITY_LOW);

    return 0;
}
#endif

struct platform_device devapc_device = {
    .name   = "devapc",
    .id        = -1,
};

static struct platform_driver devapc_driver = {
    .probe        = devapc_probe,
    .remove        = devapc_remove,
    .suspend    = devapc_suspend,
    .resume        = devapc_resume,
    .driver     = {
        .name = "devapc",
        .owner = THIS_MODULE,
    },
};



/*
 * devapc_init: module init function.
 */
static int __init devapc_init(void)
{
    int ret;

    xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG , "[DEVAPC] module init. \n");

    ret = platform_device_register(&devapc_device);
    if (ret) {
        xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG , "[DEVAPC] Unable to do device register(%d)\n", ret);
        return ret;
    }
    ret = platform_driver_register(&devapc_driver);
    if (ret) {
        xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] Unable to register driver (%d)\n", ret);
        return ret;
    }

    g_devapc_ctrl = cdev_alloc();
    g_devapc_ctrl->owner = THIS_MODULE;

    if(ret != 0)
    {
        xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] Failed to add devapc device! (%d)\n", ret);
        return ret;
    }

    /* 
     * NoteXXX: Interrupts of vilation (including SPC in SMI, or EMI MPU) are triggered by the device APC.
     *          Need to share the interrupt with the SPC driver. 
     */
    ret = request_irq(MT_APARM_DOMAIN_IRQ_ID, (irq_handler_t)devapc_violation_irq, IRQF_TRIGGER_LOW | IRQF_SHARED, 
        "mt6577_devapc", &g_devapc_ctrl);    
    disable_irq(MT_APARM_DOMAIN_IRQ_ID);
    enable_irq(MT_APARM_DOMAIN_IRQ_ID);
    
    if(ret != 0)
    {
        xlog_printk(ANDROID_LOG_ERROR, DEVAPC_TAG ,"[DEVAPC] Failed to request irq! (%d)\n", ret);
        return ret;
    }
 
#ifdef CONFIG_MTK_HIBERNATION
    register_swsusp_restore_noirq_func(ID_M_DEVAPC, devapc_pm_restore_noirq, NULL);
#endif

    return 0;
}

/*
 * devapc_exit: module exit function.
 */
static void __exit devapc_exit(void)
{
    xlog_printk(ANDROID_LOG_INFO, DEVAPC_TAG ,"[DEVAPC] DEVAPC module exit\n");

#ifdef CONFIG_MTK_HIBERNATION
    unregister_swsusp_restore_noirq_func(ID_M_DEVAPC);
#endif
}

module_init(devapc_init);
module_exit(devapc_exit);
MODULE_LICENSE("GPL");
EXPORT_SYMBOL(start_usb_protection);
EXPORT_SYMBOL(stop_usb_protection);
