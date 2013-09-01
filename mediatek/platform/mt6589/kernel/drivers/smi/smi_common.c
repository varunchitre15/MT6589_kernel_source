#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <mach/mt_clkmgr.h>

#include <mach/m4u.h>
#include <mach/mt_smi.h>
#include <mach/mt_gpufreq.h>    /*Adjust GPU OD or not*/

#include "smi_reg.h"
#include "smi_common.h"


#define SMI_LOG_TAG "SMI"


unsigned int gLarbBaseAddr[SMI_LARB_NR] = 
    {LARB0_BASE, LARB1_BASE, LARB2_BASE, LARB3_BASE, LARB4_BASE}; 


char *smi_port_name[][15] = 
{
    {
        "venc_rcpu"             ,
        "venc_ref_luma"         ,
        "venc_ref_chroma"       ,
        "venc_db_read"          ,
        "venc_db_write"         ,
        "venc_cur_luma"         ,
        "venc_cur_chroma"       ,
        "venc_rd_comv"          ,
        "venc_sv_comv"          ,
        "venc_bsdma"            ,
    },
    {
        "vdec_mc"        ,
        "vdec_pp"        ,
        "vdec_avc_mv"    ,
        "vdec_pred_rd"   ,
        "vdec_pred_wr"   ,
        "vdec_vld"       ,
        "vdec_vld2"      ,
    },
    {
        "rot_ext"               ,
        "ovl_ch0"               ,
        "ovl_ch1"               ,
        "ovl_ch2"               ,
        "ovl_ch3"               ,
        "wdma0"                 ,
        "wdma1"                 ,
        "rdma0"                 ,
        "rdma1"                 ,
        "cmdq"                  ,
        "dbi"                   ,
        "g2d"                   ,
    },
    {
        "jpgdec_wdma"           ,
        "jpgenc_rdma"           ,
        "vipi"                  ,
        "imgi"                  ,
        "dispo"                 ,
        "dispco"                ,
        "dispvo"                ,
        "vido"                  ,
        "vidco"                 ,
        "vidvo"                 ,
        "vip2i"                 ,
        "gdma_smi_wr"           ,
        "jpgdec_bsdma"          ,
        "jpgenc_bsdma"          ,
        "vido_rot0"             ,
    },
    {
        "gdma_smi_rd"           ,
        "imgci"                 ,
        "imgo"                   ,
        "img2o"                  ,
        "lsci"                   ,
        "flki"                   ,
        "lcei"                   ,
        "lcso"                   ,
        "esfko"                  ,
        "aao"                    ,
        "vidco_rot0"             ,
        "fpc"                    ,
    },
};





int larb_clock_on(int larb_id) 
{

    char name[30];
    sprintf(name, "smi+%d", larb_id);  


    switch(larb_id)
    {
        case 0: 
           enable_clock(MT_CG_VENC_VEN, name);
           break;
        case 1:
           enable_clock(MT_CG_VDEC0_VDE, name);
           enable_clock(MT_CG_VDEC1_SMI, name);
           break;
        case 2: 
           enable_clock(MT_CG_DISP0_LARB2_SMI, name);
           break;
        case 3: 
           enable_clock(MT_CG_IMAGE_LARB3_SMI, name);
           break;
        case 4: 
           enable_clock(MT_CG_IMAGE_LARB4_SMI, name);
           break;
        case 5: 
           enable_clock(MT_CG_IMAGE_LARB4_SMI, name);
           break;

        default: 
            break;
    }

  return 0;
}

int larb_clock_off(int larb_id) 
{

    char name[30];
    sprintf(name, "smi+%d", larb_id);


    switch(larb_id)
    {
        case 0: 
           disable_clock(MT_CG_VENC_VEN, name);
           break;
        case 1:
           disable_clock(MT_CG_VDEC0_VDE, name);
           disable_clock(MT_CG_VDEC1_SMI, name);
           break;
        case 2: 
           disable_clock(MT_CG_DISP0_LARB2_SMI, name);
           break;
        case 3: 
           disable_clock(MT_CG_IMAGE_LARB3_SMI, name);
           break;
        case 4: 
           disable_clock(MT_CG_IMAGE_LARB4_SMI, name);
           break;

        default: 
            break;
    }

    return 0;

}


#define LARB_BACKUP_REG_SIZE 128
static unsigned int* pLarbRegBackUp[SMI_LARB_NR];

int larb_reg_backup(int larb)
{
    unsigned int* pReg = pLarbRegBackUp[larb];
    int i;
    unsigned int larb_base = gLarbBaseAddr[larb];
    
    *(pReg++) = M4U_ReadReg32(larb_base, SMI_LARB_CON);
    *(pReg++) = M4U_ReadReg32(larb_base, SMI_SHARE_EN);
    *(pReg++) = M4U_ReadReg32(larb_base, SMI_ROUTE_SEL);

    for(i=0; i<3; i++)
    {
        *(pReg++) = M4U_ReadReg32(larb_base, SMI_MAU_ENTR_START(i));
        *(pReg++) = M4U_ReadReg32(larb_base, SMI_MAU_ENTR_END(i));
        *(pReg++) = M4U_ReadReg32(larb_base, SMI_MAU_ENTR_GID(i));
    }

    return 0;
}

int larb_reg_restore(int larb)
{
    unsigned int* pReg = pLarbRegBackUp[larb];
    int i;
    unsigned int regval;
    unsigned int larb_base = gLarbBaseAddr[larb];

    
    //warning: larb_con is controlled by set/clr
    regval = *(pReg++);
    M4U_WriteReg32(larb_base, SMI_LARB_CON_CLR, ~(regval));
    M4U_WriteReg32(larb_base, SMI_LARB_CON_SET, (regval));
    
    M4U_WriteReg32(larb_base, SMI_SHARE_EN, *(pReg++) );
    M4U_WriteReg32(larb_base, SMI_ROUTE_SEL, *(pReg++) );

    for(i=0; i<3; i++)
    {
        M4U_WriteReg32(larb_base, SMI_MAU_ENTR_START(i), *(pReg++));
        M4U_WriteReg32(larb_base, SMI_MAU_ENTR_END(i), *(pReg++));
        M4U_WriteReg32(larb_base, SMI_MAU_ENTR_GID(i), *(pReg++));
    }

    return 0;
}


// callback after larb clock is enabled
void on_larb_power_on(struct larb_monitor *h, int larb_idx)
{
    //M4ULOG("on_larb_power_on(), larb_idx=%d \n", larb_idx);
    larb_reg_restore(larb_idx);
    
    return;
}
// callback before larb clock is disabled
void on_larb_power_off(struct larb_monitor *h, int larb_idx)
{
    //M4ULOG("on_larb_power_off(), larb_idx=%d \n", larb_idx);
    larb_reg_backup(larb_idx);
}


int smi_bwc_config( MTK_SMI_BWC_CONFIG* p_conf )
{
    int i;

    /*turn on larb clock*/    
    for(i=0; i<SMI_LARB_NR; i++){
        larb_clock_on(i);
    }

    /*Bandwidth Limiter*/
    switch( p_conf->scenario )
    {
    case SMI_BWC_SCEN_VRCAMERA1066:
        SMIMSG( "[SMI_PROFILE] : %s\n", "SMI_BWC_SCEN_VRCAMERA1066");
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB0, 0xb92 );   //larb0 venc
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB1, 0x0   );   //larb1 vdec:default
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB2, 0xa4b );   //larb2 disp
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB3, 0x96d );   //larb3 cdp
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB4, 0x9a7 );   //larb4 isp

        #if 1 /*Jackie custom*/
        //M4U_WriteReg32( 0x0, REG_SMI_BUS_SEL, 0x140 );
        //M4U_WriteReg32( 0x0, REG_SMI_READ_FIFO_TH, 0x1560 );
        M4U_WriteReg32( LARB0_BASE, 0x14, 0x400420 );
        M4U_WriteReg32( LARB1_BASE, 0x14, 0x400420 );
//        M4U_WriteReg32( LARB2_BASE, 0x14, 0x400420 );
        M4U_WriteReg32( LARB3_BASE, 0x14, 0x400420 );
        #endif

//        M4U_WriteReg32( 0x0 , REG_SMI_L1ARB6 ,  0);//
        break;
    
    case SMI_BWC_SCEN_VR1066:
        SMIMSG( "[SMI_PROFILE] : %s\n", "SMI_BWC_SCEN_VR1066");
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB0, 0xb92 );   //larb0 venc
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB1, 0x0   );   //larb1 vdec:default
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB2, 0xa4b );   //larb2 disp
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB3, 0x96d );   //larb3 cdp
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB4, 0x9a7 );   //larb4 isp

        #if 1/*Jackie custom*/
        //M4U_WriteReg32( 0x0, REG_SMI_BUS_SEL, 0x140 );
        //M4U_WriteReg32( 0x0, REG_SMI_READ_FIFO_TH, 0x1560 );
        M4U_WriteReg32( LARB0_BASE, 0x14, 0x400420 );
        M4U_WriteReg32( LARB1_BASE, 0x14, 0x400420 );
//        M4U_WriteReg32( LARB2_BASE, 0x14, 0x400420 );
        M4U_WriteReg32( LARB3_BASE, 0x14, 0x400420 );
        #endif

//        M4U_WriteReg32( 0x0 , REG_SMI_L1ARB6 ,  0x1c22);//
        break;

        
    case SMI_BWC_SCEN_VP1066:
        SMIMSG( "[SMI_PROFILE] : %s\n", "SMI_BWC_SCEN_VP1066");
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB0, 0x0   );   //larb0 venc:default
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB1, 0x9b1 );   //larb1 vdec
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB2, 0xaa8 );   //larb2 disp
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB3, 0x909 );   //larb3 cdp
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB4, 0x0   );   //larb4 isp:default

        #if 1/*Jackie custom*/
        //M4U_WriteReg32( 0x0, REG_SMI_BUS_SEL, 0x140 );
        //M4U_WriteReg32( 0x0, REG_SMI_READ_FIFO_TH, 0xD60 );
        M4U_WriteReg32( LARB0_BASE, 0x18, 0x420 );       
        M4U_WriteReg32( LARB1_BASE, 0x18, 0x420 );       
//        M4U_WriteReg32( LARB2_BASE, 0x18, 0x420 );       
        M4U_WriteReg32( LARB3_BASE, 0x18, 0x420 );    
        #endif

//        M4U_WriteReg32( 0x0 , REG_SMI_L1ARB6 ,  0);//
        break;
        
        
    case SMI_BWC_SCEN_NORMAL:
    default:
        SMIMSG( "[SMI_PROFILE] : %s\n", "SMI_BWC_SCEN_NORMAL");
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB0, 0x0   );   //larb0 venc:default
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB1, 0x0   );   //larb1 vdec:default
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB2, 0x0   );   //larb2 disp:default
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB3, 0x0   );   //larb3 cdp:default
        M4U_WriteReg32( 0x0, REG_SMI_L1ARB4, 0x0   );   //larb4 isp:default

        #if 1/*Jackie custom*/
        //M4U_WriteReg32( 0x0, REG_SMI_BUS_SEL, 0x140 );
        //M4U_WriteReg32( 0x0, REG_SMI_READ_FIFO_TH, 0xD60 );
        M4U_WriteReg32( LARB0_BASE, 0x18, 0x420 );       
        M4U_WriteReg32( LARB1_BASE, 0x18, 0x420 );       
//        M4U_WriteReg32( LARB2_BASE, 0x18, 0x420 );       
        M4U_WriteReg32( LARB3_BASE, 0x18, 0x420 );    
        #endif

//        M4U_WriteReg32( 0x0 , REG_SMI_L1ARB6 ,  0);//
        break;
    }

    /*reduce command buffer*/
    //if( p_conf->b_reduce_command_buffer )
    {
        /*SMI COMMON reduce command buffer*/
        M4U_WriteReg32( 0x0, REG_SMI_READ_FIFO_TH, 0x00001560 );
        
        /*SMI LARB reduce command buffer*/
        M4U_WriteReg32( LARB0_BASE, 0x14, 0x00400000 );
        M4U_WriteReg32( LARB1_BASE, 0x14, 0x00400000 );
        M4U_WriteReg32( LARB2_BASE, 0x14, 0x00400000 );
        M4U_WriteReg32( LARB3_BASE, 0x14, 0x00400000 );
        M4U_WriteReg32( LARB4_BASE, 0x14, 0x00400000 );
        
    }

    #if 0 /*dump message*/
    {
        #define _SMI_BC_DUMP_REG( _base_, _off_ ) \
            SMIMSG( "\t[SMI_REG] %s + %s = 0x%08X\n", #_base_, #_off_, M4U_ReadReg32( _base_, _off_ ) );

        SMIMSG( "[SMI_REG] : %s\n", "<Bandwidth Limiter>");
        /*Bandwidth Limiter*/
        _SMI_BC_DUMP_REG( 0x0, REG_SMI_L1ARB0 );   //larb0 venc
        _SMI_BC_DUMP_REG( 0x0, REG_SMI_L1ARB1 );   //larb1 vdec
        _SMI_BC_DUMP_REG( 0x0, REG_SMI_L1ARB2 );   //larb2 disp
        _SMI_BC_DUMP_REG( 0x0, REG_SMI_L1ARB3 );   //larb3 cdp
        _SMI_BC_DUMP_REG( 0x0, REG_SMI_L1ARB4 );   //larb4 isp

        SMIMSG( "[SMI_REG] : %s\n", "<Reduce Command Buffer- Common>");
        /*SMI COMMON reduce command buffer*/
        _SMI_BC_DUMP_REG( 0x0, REG_SMI_READ_FIFO_TH );

        SMIMSG( "[SMI_REG] : %s\n", "<Reduce Command Buffer - LARB>");
        /*SMI LARB reduce command buffer (RO register)*/
        _SMI_BC_DUMP_REG( LARB0_BASE, 0x10 );
        _SMI_BC_DUMP_REG( LARB1_BASE, 0x10 );
        _SMI_BC_DUMP_REG( LARB2_BASE, 0x10 );
        _SMI_BC_DUMP_REG( LARB3_BASE, 0x10 );
        _SMI_BC_DUMP_REG( LARB4_BASE, 0x10 );

        
    }
    #endif


    /*turn off larb clock*/    
    for(i=0; i<SMI_LARB_NR; i++){
        larb_clock_off(i);
    }

    /*.............................................................................
        GPU OD Adjust 
      .............................................................................*/
    if( p_conf->b_gpu_od ){
        mt_gpufreq_keep_frequency_non_OD (false);
        SMIMSG( "[SMI_BWC] : GPU OD ON!\n");
    } else {
        mt_gpufreq_keep_frequency_non_OD (true);
        SMIMSG( "[SMI_BWC] : GPU OD OFF!\n");
    }
    

    return 0;
    
}



struct larb_monitor larb_monitor_handler =
{
    .level = LARB_MONITOR_LEVEL_HIGH,
    .backup = on_larb_power_off,
    .restore = on_larb_power_on	
};



int smi_common_init(void)
{
    int i;

    for(i=0; i<SMI_LARB_NR; i++)
    {
        pLarbRegBackUp[i] = (unsigned int*)kmalloc(LARB_BACKUP_REG_SIZE, GFP_KERNEL|__GFP_ZERO);
        if(pLarbRegBackUp[i]==NULL)
        {
        	  SMIERR("pLarbRegBackUp kmalloc fail %d \n", i);
        }  
    }

    /** make sure all larb power is on before we register callback func.
        then, when larb power is first off, default register value will be backed up.
    **/     

    for(i=0; i<SMI_LARB_NR; i++)
    {
        larb_clock_on(i);
    }

    register_larb_monitor(&larb_monitor_handler);
    
    for(i=0; i<SMI_LARB_NR; i++)
    {
        larb_clock_off(i);
    }
    
    return 0;
}




static long smi_ioctl( struct file * pFile,
						 unsigned int cmd,
						 unsigned long param)
{
    int ret = 0;
    
    switch (cmd)
    {
        case MTK_CONFIG_MM_MAU:
        {
        	MTK_MAU_CONFIG b;
       		if(copy_from_user(&b, (void __user *)param, sizeof(b)))
        	{
            	SMIERR("copy_from_user failed!");
            	ret = -EFAULT;
        	} else {
                mau_config(&b);
			}
        	return ret;
    	}
        case MTK_IOC_SPC_CONFIG :
        {
            MTK_SPC_CONFIG cfg;
            ret = copy_from_user(&cfg, (void*)param , sizeof(MTK_SPC_CONFIG));
            if(ret)
            {
            	SMIMSG(" SPC_CONFIG, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            }  

            spc_config(&cfg);

        }
            break;

        case MTK_IOC_SPC_DUMP_REG :
            spc_dump_reg();
            break;


        case MTK_IOC_SPC_DUMP_STA :
            spc_status_check();    	
            break;

        case MTK_IOC_SPC_CMD :
            spc_test(param);
            break;

        case MTK_IOC_SMI_BWC_CONFIG:
            {
                MTK_SMI_BWC_CONFIG cfg;
                ret = copy_from_user(&cfg, (void*)param , sizeof(MTK_SMI_BWC_CONFIG));
                if(ret)
                {
                    SMIMSG(" SMI_BWC_CONFIG, copy_from_user failed: %d\n", ret);
                    return -EFAULT;
                }  

                smi_bwc_config( &cfg );
            
            }
            break;
        
        default:
            return -1;
    }

	return ret;
}


static const struct file_operations smiFops =
{
	.owner = THIS_MODULE,
	.unlocked_ioctl = smi_ioctl,
};

static struct cdev * pSmiDev = NULL;
static dev_t smiDevNo = MKDEV(MTK_SMI_MAJOR_NUMBER,0);
static inline int smi_register(void)
{
    if (alloc_chrdev_region(&smiDevNo, 0, 1,"MTK_SMI")){
        SMIERR("Allocate device No. failed");
        return -EAGAIN;
    }
    //Allocate driver
    pSmiDev = cdev_alloc();

    if (NULL == pSmiDev) {
        unregister_chrdev_region(smiDevNo, 1);
        SMIERR("Allocate mem for kobject failed");
        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(pSmiDev, &smiFops);
    pSmiDev->owner = THIS_MODULE;

    //Add to system
    if (cdev_add(pSmiDev, smiDevNo, 1)) {
        SMIERR("Attatch file operation failed");
        unregister_chrdev_region(smiDevNo, 1);
        return -EAGAIN;
    }
    return 0;
}


static struct class *pSmiClass = NULL;
static int smi_probe(struct platform_device *pdev)
{
    struct device* smiDevice = NULL;

    if (NULL == pdev) {
        SMIERR("platform data missed");
        return -ENXIO;
    }

    if (smi_register()) {
        dev_err(&pdev->dev,"register char failed\n");
        return -EAGAIN;
    }

    pSmiClass = class_create(THIS_MODULE, "MTK_SMI");
    if (IS_ERR(pSmiClass)) {
        int ret = PTR_ERR(pSmiClass);
        SMIERR("Unable to create class, err = %d", ret);
        return ret;
    }
    smiDevice = device_create(pSmiClass, NULL, smiDevNo, NULL, "MTK_SMI");

    smi_common_init();

    mau_init();

    MTK_SPC_Init(&(pdev->dev));

    SMI_DBG_Init();

    //init mau to monitor mva 0~0x2ffff & 0x40000000~0xffffffff
    {
        MTK_MAU_CONFIG cfg;
        int i;
        for(i=0; i<SMI_LARB_NR; i++)
        {
            cfg.larb = i;
            cfg.entry = 0;
            cfg.port_msk = 0xffffffff;
            cfg.virt = 1;
            cfg.monitor_read = 1;
            cfg.monitor_write = 1;
            cfg.start = 0;
            cfg.end = 0x2ffff;
            mau_config(&cfg);

            cfg.entry = 1;
            cfg.start = 0x40000000;
            cfg.end = 0xffffffff;
            mau_config(&cfg);
        }
    }
    
    return 0;
}



static int smi_remove(struct platform_device *pdev)
{
    cdev_del(pSmiDev);
    unregister_chrdev_region(smiDevNo, 1);
    device_destroy(pSmiClass, smiDevNo);
    class_destroy(pSmiClass);
    return 0;
}


static int smi_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int smi_resume(struct platform_device *pdev)
{
    return 0;
}

static struct platform_driver smiDrv = {
    .probe	= smi_probe,
    .remove	= smi_remove,
    .suspend= smi_suspend,
    .resume	= smi_resume,
    .driver	= {
    .name	= "MTK_SMI",
    .owner	= THIS_MODULE,
    }
};


static int __init smi_init(void)
{
    if(platform_driver_register(&smiDrv)){
        SMIERR("failed to register MAU driver");
        return -ENODEV;
    }
	return 0;
}

static void __exit smi_exit(void)
{
    platform_driver_unregister(&smiDrv);

}

module_init(smi_init);
module_exit(smi_exit);

MODULE_DESCRIPTION("MTK SMI driver");
MODULE_AUTHOR("K_zhang<k.zhang@mediatek.com>");
MODULE_LICENSE("GPL");

