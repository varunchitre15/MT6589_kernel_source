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
#include <linux/slab.h>     //kmalloc
#include <linux/proc_fs.h>  //For proc filesystem
#include <linux/vmalloc.h>  //For vmalloc


#include <linux/xlog.h>  //Prevent include by user space (no good)
//Arch dependent files
#include <mach/sync_write.h>
//#include <mach/mt_pll.h>
#include <mach/irqs.h>
//#include <asm/tcm.h> /*MT6577 do not have tcm*/

#include "ddp_drv.h"
#include "ddp_dpfd.h"

#define DDPKBITBLIT_TIMEOUT     ( 3000 )

//--------------------------------------------Global Variable (Log Switch)---------------------------------------//
int b_ddp_show_info       = 0;
int b_ddp_show_warning    = 1;
int b_ddp_show_error      = 1;   //Always kepp this on
int b_ddp_show_isrinfo    = 0;
int b_ddp_show_powerinfo  = 0;
int b_ddp_show_zoominfo   = 0;
int b_ddp_show_fpsinfo    = 0;
int b_ddp_show_bbinfo     = 0;
int b_ddp_show_zsdinfo    = 0;

typedef struct 
{
    spinlock_t          lock;               /*void spin_lock_init(spinlock_t *lock);*/
    wait_queue_head_t   request_wait_queue; /*Wait by daemon*/
    wait_queue_head_t   done_wait_queue;    /*Wait by kernel request client;*/
    unsigned long       request_flag;
    int                 b_src_addr_dirty[ DDPKBITBLIT_CHNL_COUNT ];   /*if b_addr_dirty , need to remap*/
    int                 b_dst_addr_dirty[ DDPKBITBLIT_CHNL_COUNT ];   /*if b_addr_dirty , need to remap*/
    DdpkBitbltConfig    config[ DDPKBITBLIT_CHNL_COUNT ];
    int                 result[ DDPKBITBLIT_CHNL_COUNT ];
    
} DdpkBitbltChannels;


static DdpkBitbltChannels g_ddpk_bitblt_chnls;

static bool is_init = false;


unsigned long DDPK_Util_MsToJiffies(unsigned long u4ms)
{
    return ((u4ms*HZ + 512) >> 10);
}

int DDPK_Bitblt_Init( void )
{
    int i = 0;       

    if (is_init == true)
        return 0;

    DDPK_DEBUG("===  DDPK_Bitblt_Init  ==="); 

    spin_lock_init( &g_ddpk_bitblt_chnls.lock );
    init_waitqueue_head( &g_ddpk_bitblt_chnls.request_wait_queue );
    init_waitqueue_head( &g_ddpk_bitblt_chnls.done_wait_queue );
    g_ddpk_bitblt_chnls.request_flag = 0;

    for( i = 0; i < DDPKBITBLIT_CHNL_COUNT; i++ )
    {
         g_ddpk_bitblt_chnls.b_src_addr_dirty[i] = 0;
         g_ddpk_bitblt_chnls.b_dst_addr_dirty[i] = 0;
    }

    is_init = true;

    return 0;
}



/*int Mt6577_mHalBitblt(void *a_pInBuffer) mt6577 prefix should not add, this should be platform independent implement*/
int DDPK_Bitblt_Config( int channel, DdpkBitbltConfig* pParam )
{
    unsigned long irq_state;
    
    DDPK_Bitblt_Init();    

    if( ( channel >= DDPKBITBLIT_CHNL_COUNT) || (channel < 0) )    {
        DDPK_ERROR("ddpk bitblt channel illegal(=%d), max = %d\n", channel, DDPKBITBLIT_CHNL_COUNT-1 );
        return -1;
    }

    if( pParam == NULL )    {
        DDPK_ERROR("ddpk bitblt channel config parameter is NULL\n");
        return -1;
    }

    /*Check if this channel busy*/
    /*Critical Section*/ 
    spin_lock_irqsave(&g_ddpk_bitblt_chnls.lock, irq_state );
    {
        if( (g_ddpk_bitblt_chnls.request_flag & ( 0x1 << channel )) != 0 )
        {
            DDPK_ERROR("DDPK DDPK_Bitblt_Config Bitblt Ch:%d (0x%08X) is busy. Config fail\n", channel, (unsigned int)g_ddpk_bitblt_chnls.request_flag );
            spin_unlock_irqrestore(&g_ddpk_bitblt_chnls.lock, irq_state );
            return -1;
        }
    }
    spin_unlock_irqrestore(&g_ddpk_bitblt_chnls.lock, irq_state );
    /*~Critical Section*/    
    

    /*Critical Section*/    
    spin_lock_irqsave(&g_ddpk_bitblt_chnls.lock, irq_state );
    {
        g_ddpk_bitblt_chnls.b_src_addr_dirty[channel] = 1;
        g_ddpk_bitblt_chnls.b_dst_addr_dirty[channel] = 1;
        memcpy( &(g_ddpk_bitblt_chnls.config[channel]), pParam, sizeof(DdpkBitbltConfig)  );
    }
    spin_unlock_irqrestore(&g_ddpk_bitblt_chnls.lock, irq_state );
    /*~Critical Section*/    


    return 0;
    
}



int DDPK_Bitblt( int channel )
{    
    const unsigned long kTIMEOUT = 3000;    //ms
    unsigned long   irq_state;
    int             ret_of_wait;

    DDPK_Bitblt_Init();

    if( ( channel >= DDPKBITBLIT_CHNL_COUNT) || (channel < 0) )    {
        DDPK_ERROR("ddpk bitblt channel illegal(=%d), max = %d\n", channel, DDPKBITBLIT_CHNL_COUNT-1 );
        return -1;
    }
    
    /*Critical Section*/    
    spin_lock_irqsave(&g_ddpk_bitblt_chnls.lock, irq_state );
    {
        if( (g_ddpk_bitblt_chnls.request_flag & ( 0x1 << channel )) != 0 )
        {
            DDPK_ERROR("DDPK DDPK_Bitblt Bitblt Ch:%d (0x%08X) is busy. Config fail\n", channel, (unsigned int)g_ddpk_bitblt_chnls.request_flag );
            spin_unlock_irqrestore(&g_ddpk_bitblt_chnls.lock, irq_state );
            return -1;
        }

        /*Set request flag*/
        g_ddpk_bitblt_chnls.request_flag |= ( 0x1 << channel );

        DDPK_BBINFO("DDPK Bitblt Request Ch:%d (0x%08X)\n", channel, (unsigned int)g_ddpk_bitblt_chnls.request_flag );
    }
    spin_unlock_irqrestore(&g_ddpk_bitblt_chnls.lock, irq_state );
    /*~Critical Section*/    

    

    /*Wake-up Request Wait Queue for daemon*/
    mb();   /*Add memory barrier before the other CPU (may) wakeup*/
    wake_up_interruptible( &(g_ddpk_bitblt_chnls.request_wait_queue) );

    /*Wait Done Wait Queue from daemon*/
    ret_of_wait = wait_event_interruptible_timeout( g_ddpk_bitblt_chnls.done_wait_queue,
                                                    ( g_ddpk_bitblt_chnls.request_flag & ( 0x1 << channel ) ) == 0,
                                                    DDPK_Util_MsToJiffies(kTIMEOUT) );
    if( ret_of_wait == 0 )
    {
        DDPK_ERROR("DDPK Timeout. Ch:%d (0x%08X) timeout = %dms\n", channel, (unsigned int)g_ddpk_bitblt_chnls.request_flag, (int)kTIMEOUT );
        return -1;
    }

    return g_ddpk_bitblt_chnls.result[ channel ];
    
}


unsigned int ddp_bitblt_ioctl_wait_reequest( unsigned long ioctl_user_param  )
{
    int                         ret_of_wait;
    DDPIOCTL_DdpkBitbltConfig   param;
    int                         ch;
    unsigned long               irq_state;

    DDPK_Bitblt_Init();
       
    /*wait until request flag is raised*/
    ret_of_wait = wait_event_interruptible( g_ddpk_bitblt_chnls.request_wait_queue , 
                                            g_ddpk_bitblt_chnls.request_flag != 0  );

    if( ret_of_wait != 0 ){
        return -ERESTARTSYS;
    }

    
    /*Critical Section*/    
    spin_lock_irqsave(&g_ddpk_bitblt_chnls.lock, irq_state );
    {
        int b_found = 0;
        /*find the first request flag*/
        for( ch = 0; ch < DDPKBITBLIT_CHNL_COUNT; ch++ ){
            if( ( g_ddpk_bitblt_chnls.request_flag & ( 0x1 << ch ) ) != 0 ){
                b_found = 1;
                break;
            }
        }

        if( b_found == 0 ){
            DDPK_ERROR("DDPK Bitblt cannot found request flag\n");
            spin_unlock_irqrestore(&g_ddpk_bitblt_chnls.lock, irq_state );
            return -ERESTARTSYS;
        }

        DDPK_BBINFO("Process DDPK Bitblt Channel : %d ( request flag = 0x%08X )\n", ch, (unsigned int)g_ddpk_bitblt_chnls.request_flag );

        param.out_channel          = ch;
        param.out_b_src_addr_dirty = g_ddpk_bitblt_chnls.b_src_addr_dirty[ch];
        param.out_b_dst_addr_dirty = g_ddpk_bitblt_chnls.b_dst_addr_dirty[ch];
        memcpy( &param.out_config, &(g_ddpk_bitblt_chnls.config[ch]), sizeof(DdpkBitbltConfig)  );

        /*Clear address dirty bit,once user retrieve config parameter*/
        g_ddpk_bitblt_chnls.b_src_addr_dirty[ch] = 0;
        g_ddpk_bitblt_chnls.b_dst_addr_dirty[ch] = 0;
    }
    spin_unlock_irqrestore(&g_ddpk_bitblt_chnls.lock, irq_state );
    /*~Critical Section*/    


    if(copy_to_user((void __user *) ioctl_user_param , (void*)&param , sizeof(DDPIOCTL_DdpkBitbltConfig)))
    {
        DDPK_ERROR("DDPK_Bitblt_IOCTL_Config_Get() copy to user failed!!\n");
        return -EFAULT;
    }

    return 0;
}


  

unsigned int ddp_bitblt_ioctl_inform_done( unsigned long ioctl_user_param  )
{
    int                             channel;
    int                             result;
    unsigned long                   irq_state;
    DDPIOCTL_DdpkBitbltInformDone   param;

    DDPK_Bitblt_Init();
            
    if(copy_from_user(&param, (const void __user *)ioctl_user_param , sizeof(DDPIOCTL_DdpkBitbltInformDone)))
    {
        DDPK_ERROR("DDPK_Bitblt_IOCTL_Inform_Done(), Copy from user failed !!\n");
        return -EFAULT;
    }

    channel = param.in_channel;
    result  = param.in_ret_val;

    
    if( ( channel >= DDPKBITBLIT_CHNL_COUNT) || (channel < 0) )    {
        DDPK_ERROR("ddpk bitblt channel illegal(=%d), max = %d\n", channel, DDPKBITBLIT_CHNL_COUNT-1 );
        return -1;
    }

    /*Clear Request Flag and get return value*/
    /*Critical Section*/ 
    spin_lock_irqsave(&g_ddpk_bitblt_chnls.lock, irq_state );
    {
        if( (g_ddpk_bitblt_chnls.request_flag & ( 0x1 << channel )) == 0 )
        {
            DDPK_ERROR("DDPK Bitblt Ch:%d (0x%08X) is free, but a done signal received.\n", channel, (unsigned int)g_ddpk_bitblt_chnls.request_flag );
            spin_unlock_irqrestore(&g_ddpk_bitblt_chnls.lock, irq_state );
            return -1;
        }

        /*Store return value*/
        g_ddpk_bitblt_chnls.result[ channel ] = result;

        /*Clear request flag*/
        g_ddpk_bitblt_chnls.request_flag &= ~( 0x1 << channel );

    }
    spin_unlock_irqrestore(&g_ddpk_bitblt_chnls.lock, irq_state );
    /*~Critical Section*/    


    /*Wake-up Done Wait Queue for Kernel*/
    mb();   /*Add memory barrier before the other CPU (may) wakeup*/
    wake_up_interruptible( &(g_ddpk_bitblt_chnls.done_wait_queue) );

    return 0;
    
}


#if 0

int DDPK_Util_Map_Vmalloc_to_User( struct vm_area_struct* p_vma )
{
    int             ret;
    unsigned long   pfn;
    void*           vmalloc_addr    = (void*)( p_vma->vm_pgoff * PAGE_SIZE );
    unsigned long   start           = p_vma->vm_start;
    long            size            = (long)(p_vma->vm_end - p_vma->vm_start);

    
    DDPK_BBINFO("Map Kernel Vmalloc (0x%08X) to User Space (0x%08X) (szie=0x%08X)-------\n", 
                (unsigned int)vmalloc_addr, (unsigned int)start, (unsigned int)size );

    
    while( size > 0 )
    {
        pfn = vmalloc_to_pfn( vmalloc_addr );

        /*Debug Info*/
        DDPK_BBINFO("kernel vm=0x%08X pfn==0x%08X\n", (unsigned int)vmalloc_addr, (unsigned int)pfn);

        if( (ret = remap_pfn_range( p_vma, start, pfn, PAGE_SIZE, PAGE_SHARED )) < 0 )
        {
            return ret;
        }

        start           += PAGE_SIZE;
        vmalloc_addr    += PAGE_SIZE;
        size            -= PAGE_SIZE;
        
    }

    return 0;

}
#endif

EXPORT_SYMBOL(DDPK_Bitblt);
EXPORT_SYMBOL(DDPK_Bitblt_Config);





/*-----------------------------------------------------------------------------
    Test Harness
  -----------------------------------------------------------------------------*/

void dump_pixel(int type, char* buf, int width, int i, int j)
{
    int offset = 4*(j*width+i);
//    char* p = buf + j*width+i;
    
    if (type == 0)
    {
        DDPK_DEBUG("src(%d, %d)=[%d, %d, %d, %d]\n", i, j, 
//            *p, *(p+1), *(p+2), *(p+3));
            buf[offset], buf[offset+1], buf[offset+2], buf[offset+3]);
    }
    else
    {
        DDPK_DEBUG("dst(%d, %d)=[%d, %d, %d, %d]\n", i, j, 
//            *p, *(p+1), *(p+2), *(p+3));
            buf[offset], buf[offset+1], buf[offset+2], buf[offset+3]);
    }
}

#define TEST_OUT_WIDTH      100
#define TEST_OUT_HEIGHT     100
void ddpk_testfunc_1( unsigned long channel)
{
    static void* src_buffer[DDPKBITBLIT_CHNL_COUNT] = { 0 };
    unsigned long src_w = 100;
    unsigned long src_h = 100;
    unsigned long src_size = src_w * src_h * 4;

    
    static void* dst_buffer[DDPKBITBLIT_CHNL_COUNT] = { 0 };
    unsigned long dst_w = TEST_OUT_WIDTH;
    unsigned long dst_h = TEST_OUT_HEIGHT;
    unsigned long dst_size = dst_w * dst_h * 4;

    DdpkBitbltConfig    config;
    char* psrc = (char*)src_buffer[channel];
    char* pdst = (char*)dst_buffer[channel];

    int i = 0;
    int j = 0;

    DDPK_DEBUG("ddpk_testfunc_1() +++++++++\n");

    if(  src_buffer[channel] != NULL )   vfree( src_buffer[channel] );
    src_buffer[channel] = vmalloc( src_size );
    DDPK_DEBUG("src_buffer[%lu] = 0x%08X\n", channel, (unsigned int)src_buffer[channel] );

    if(  dst_buffer[channel] != NULL )   vfree( dst_buffer[channel] );
    dst_buffer[channel] = vmalloc( dst_size );
    DDPK_DEBUG("dst_buffer[%lu] = 0x%08X\n", channel, (unsigned int)dst_buffer[channel] );

    for (j = 0; j < 100; j++)
    {
        for (i = 0; i < 100; i++)
        {
#if 0
            if (j < 50)
            {
                *psrc = 255;
                psrc++;
                *psrc = 0;
                psrc++;
                *psrc = 0;
                psrc++;
                *psrc = 255;
                psrc++;
            }
            else
            {
                *psrc = 0;
                psrc++;
                *psrc = 255;
                psrc++;
                *psrc = 0;
                psrc++;
                *psrc = 255;
                psrc++;
            }
#else
            if (j < 50)
            {
                psrc[4*(j*100+i)+0] = 255;
                psrc[4*(j*100+i)+1] = 0;
                psrc[4*(j*100+i)+2] = 0;
                psrc[4*(j*100+i)+3] = 255;
            }
            else
            {
                psrc[4*(j*100+i)+0] = 0;
                psrc[4*(j*100+i)+1] = 255;
                psrc[4*(j*100+i)+2] = 0;
                psrc[4*(j*100+i)+3] = 255;
            }
#endif
        }
    }

    for (j = 0; j < TEST_OUT_HEIGHT; j++)
    {
        for (i = 0; i < TEST_OUT_WIDTH; i++)
        {
/*
            *pdst = 0;
            pdst++;
            *pdst = 0;
            pdst++;
            *pdst = 0;
            pdst++;
            *pdst = 0;
            pdst++;
*/
            pdst[4*(j*TEST_OUT_WIDTH+i)+0] = 0;
            pdst[4*(j*TEST_OUT_WIDTH+i)+1] = 0;
            pdst[4*(j*TEST_OUT_WIDTH+i)+2] = 0;
            pdst[4*(j*TEST_OUT_WIDTH+i)+3] = 0;
        }
    }

    dump_pixel(0, (char*)src_buffer[channel], TEST_OUT_WIDTH, 0, 0);
    dump_pixel(0, (char*)src_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH/2-1, TEST_OUT_HEIGHT/4-1);
    dump_pixel(0, (char*)src_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH-1, TEST_OUT_HEIGHT/2-1);
    dump_pixel(0, (char*)src_buffer[channel], TEST_OUT_WIDTH, 0, TEST_OUT_HEIGHT/2);
    dump_pixel(0, (char*)src_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH/2-1, TEST_OUT_HEIGHT*3/4-1);
    dump_pixel(0, (char*)src_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH-1, TEST_OUT_HEIGHT-1);

    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, 0, 0);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH/2-1, TEST_OUT_HEIGHT/4-1);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH-1, TEST_OUT_HEIGHT/2-1);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, 0, TEST_OUT_HEIGHT/2);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH/2-1, TEST_OUT_HEIGHT*3/4-1);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH-1, TEST_OUT_HEIGHT-1);

    
    {
        DDPK_DEBUG("DDPK_Bitblt_Config()\n");

        config.srcX = 0;
        config.srcY = 0;
        config.srcW = src_w;
        config.srcWStride = src_w;
        config.srcH = src_h;
        config.srcHStride = src_h;
        config.srcAddr[0] = (unsigned int)src_buffer[channel];
        config.srcAddr[1] = 0;
        config.srcAddr[2] = 0;
        config.srcFormat = eARGB8888_K;
        config.srcBufferSize[0] = src_size;
        config.srcBufferSize[1] = 0;
        config.srcBufferSize[2] = 0;
        config.srcPlaneNum = 1;
//        config.srcMemType = DDPK_MEMTYPE_VMALLOC;

        config.dstX = 0;
        config.dstY = 0;
        config.dstW = dst_w;
        config.dstWStride = dst_w;
        config.dstH = dst_h;
        config.dstHStride = dst_h;
        config.dstAddr[0] = (unsigned int)dst_buffer[channel];
        config.dstAddr[1] = 0;
        config.dstAddr[2] = 0;
        config.dstFormat = eABGR8888_K;
        config.pitch = dst_w;
        config.dstBufferSize[0] = dst_size; 
        config.dstBufferSize[1] = 0; 
        config.dstBufferSize[2] = 0; 
        config.dstPlaneNum = 1;
//        config.dstMemType = DDPK_MEMTYPE_VMALLOC;

        config.orientation = 0;

//        config.u4SrcOffsetXFloat = 0;//0x100 stands for 1, 0x40 stands for 0.25 , etc...
//        config.u4SrcOffsetYFloat = 0;//0x100 stands for 1, 0x40 stands for 0.25 , etc...
        
        DDPK_Bitblt_Config( channel, &config );
    }


    {
        int ret = 0;

        DDPK_DEBUG("DDPK_Bitblt()\n");

        ret = DDPK_Bitblt( channel );

        DDPK_DEBUG("DDPK_Bitblt() return %d\n", ret );
        
    }
    
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, 0, 0);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH/2-1, TEST_OUT_HEIGHT/4-1);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH-1, TEST_OUT_HEIGHT/2-1);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, 0, TEST_OUT_HEIGHT/2);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH/2-1, TEST_OUT_HEIGHT*3/4-1);
    dump_pixel(1, (char*)dst_buffer[channel], TEST_OUT_WIDTH, TEST_OUT_WIDTH-1, TEST_OUT_HEIGHT-1);


    DDPK_DEBUG("ddpk_testfunc_1() ------------\n");
}

extern unsigned long    max_pfn;
#define MAX_PFN         ((max_pfn << PAGE_SHIFT) + PHYS_OFFSET)

#define PMEM_MM_START   (MAX_PFN)
#define PMEM_MM_SIZE    (CONFIG_RESERVED_MEM_SIZE_FOR_PMEM)

//#define FB_START       (PMEM_MM_START + PMEM_MM_SIZE)
#define FB_START        PMEM_MM_START
#define FB_SIZE         (RESERVED_MEM_SIZE_FOR_FB)


void ddpk_testfunc_2( unsigned long channel)
{

    static void* src_buffer[DDPKBITBLIT_CHNL_COUNT] = { 0 };
    unsigned long src_w = 100;
    unsigned long src_h = 100;
    unsigned long src_size = src_w * src_h * 4;

    
    static void* dst_buffer[DDPKBITBLIT_CHNL_COUNT] = { 0 };
    unsigned long dst_w = 200;
    unsigned long dst_h = 200;
    unsigned long dst_size = dst_w * dst_h * 4;

    DdpkBitbltConfig    config;

    unsigned long src_pa, dst_pa;


    if( src_buffer[channel] != NULL ){
        iounmap( src_buffer[channel] );
    }
    src_pa = FB_START;
    src_buffer[channel] = ioremap( src_pa, src_size );
    DDPK_DEBUG("src_buffer[%lu] = 0x%08X(PA) ==> 0x%08X\n", channel, (unsigned int)src_pa, (unsigned int)src_buffer[channel] );

    if( dst_buffer[channel] != NULL ){
        iounmap( dst_buffer[channel] );
    }
    dst_pa = ( (FB_START + src_size) +  0xFFF ) & (~0xFFF);
    dst_buffer[channel] = ioremap( dst_pa, dst_size );
    DDPK_DEBUG("dst_buffer[%lu] = 0x%08X(PA) ==> 0x%08X\n", channel, (unsigned int)dst_pa, (unsigned int)dst_buffer[channel] );


    
    {
        DDPK_DEBUG("DDPK_Bitblt_Config()\n");

        config.srcX = 0;
        config.srcY = 0;
        config.srcW = src_w;
        config.srcWStride = src_w;
        config.srcH = src_h;
        config.srcHStride = src_h;
        config.srcAddr[0] = (unsigned int)src_buffer[channel];
        config.srcFormat = eARGB8888_K;
        config.srcBufferSize[0] = src_size;
        config.srcPlaneNum = 1;
//        config.srcMemType = DDPK_MEMTYPE_PHYMEM;

        config.dstW = dst_w;
        config.dstH = dst_h;
        config.dstAddr[0] = (unsigned int)dst_buffer[channel];
        config.dstFormat = eARGB8888_K;
        config.pitch = dst_w;
        config.dstBufferSize[0] = dst_size; 
        config.dstPlaneNum = 1;
//        config.dstMemType = DDPK_MEMTYPE_PHYMEM;

        config.orientation      = 2;
//        config.doImageProcess   = 1;

//        config.u4SrcOffsetXFloat = 0;//0x100 stands for 1, 0x40 stands for 0.25 , etc...
//        config.u4SrcOffsetYFloat = 0;//0x100 stands for 1, 0x40 stands for 0.25 , etc...
        
        DDPK_Bitblt_Config( channel, &config );
    }


    {
        int ret = 0;

        DDPK_DEBUG("DDPK_Bitblt()\n");

        ret = DDPK_Bitblt( channel );

        DDPK_DEBUG("DDPK_Bitblt() return %d\n", ret );
        
    }
    


    
}


