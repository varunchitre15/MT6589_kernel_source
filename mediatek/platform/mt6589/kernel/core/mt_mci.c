#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <mach/mt_mci.h>
#include <mach/mt_reg_base.h>
#include <asm/system.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <mach/dma.h>
#include <linux/dma-mapping.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#define LDVT
#define MCI_DEBUG

#ifdef MCI_DEBUG
#define dbg_printk printk
#else
#define dbg_printk
#endif
extern void smp_inner_dcache_flush_all();
static void mci_snoop_en(unsigned int );
static void mci_write_skip_en(unsigned  int );
static void mci_write_snoop_early_resp_en(unsigned  int );
static void mci_write_snoop_outstanding_en( unsigned int );

struct MCI_CONF{
    int mci_bar;
    int mci_snoop;
    int mci_write_skip;
    int mci_write_snoop_early_resp;
    int mci_write_snoop_outstanding;
};
enum {
    MCI_DISABLE=0,
    MCI_ENABLE=1
};
static struct MCI_CONF mci_conf;
void mci_dump_structure(){
    dbg_printk("[MCI] mci_confi:%d,%d,%d,%d,%d\n",mci_conf.mci_write_skip,mci_conf.mci_write_snoop_early_resp,mci_conf.mci_write_snoop_outstanding,mci_conf.mci_bar,mci_conf.mci_snoop);

}
void mci_sleep(){
        dbg_printk("[MCI] in %s...\n",__func__);
        mci_dump_structure();
        mci_snoop_en(MCI_DISABLE);
        mci_write_skip_en(MCI_DISABLE);
        mci_write_snoop_early_resp_en(MCI_DISABLE);
        mci_write_snoop_outstanding_en(MCI_DISABLE);
        //asm volatile("LOOP_2:");
        //asm volatile ("mov r0,r0");
        ////asm volatile("b LOOP_2");
}

static void mci_setup(struct MCI_CONF conf){
        dbg_printk("[MCI] in %s...\n",__func__);
        smp_inner_dcache_flush_all();
        mci_conf.mci_write_skip = conf.mci_write_skip;
        mci_conf.mci_write_snoop_early_resp = conf.mci_write_snoop_early_resp;
        mci_conf.mci_write_snoop_outstanding = conf.mci_write_snoop_outstanding;
        mci_conf.mci_bar = conf.mci_bar;
        mci_conf.mci_snoop = conf.mci_snoop;

        mci_write_skip_en(conf.mci_write_skip);
        mci_write_snoop_early_resp_en(conf.mci_write_snoop_early_resp);
        mci_write_snoop_outstanding_en(conf.mci_write_snoop_outstanding);
        mci_snoop_en(conf.mci_snoop);
}
void mci_restore(){
        //dbg_printk("[MCI] in %s...\n",__func__);
        mci_write_skip_en(mci_conf.mci_write_skip);
        mci_write_snoop_early_resp_en(mci_conf.mci_write_snoop_early_resp);
        mci_write_snoop_outstanding_en(mci_conf.mci_write_snoop_outstanding);
        mci_snoop_en(mci_conf.mci_snoop);
}
void mci_snoop_sleep(){
    //    dbg_printk("[MCI] in %s...\n",__func__);
    mci_snoop_en(MCI_DISABLE);
}
void mci_snoop_restore(){
    //    dbg_printk("[MCI] in %s...\n",__func__);
    mci_snoop_en(mci_conf.mci_snoop);
}

// Snoop Support
// Enable snoop function
static void mci_snoop_en(unsigned int en)
{
if(en)
{
    *((unsigned int *) MCI_SCR_S0) |= 0x11; //enable slave0 (Periph/MM) port snoop
}
else
{
    *((unsigned int *) MCI_SCR_S0) &= ~(0x11); //disable slave0 port snoop
}
    dsb();
}

// Performance option
// Write Skip: issuing MakeInv instead of CleanInv when updating full cacheline no matter the dirty bit
static void mci_write_skip_en(unsigned int en)
{
if(en)
{
    *((unsigned int *) MCI_SCR_S0) |= 0x4;
}
else
{
    *((unsigned int *) MCI_SCR_S0) &= ~(0x4);
}
    dsb();
    //mci_conf.mci_write_skip_en = en;
}

// Write snoop early response enable: early response to snoop controller to reduce write latency
static void mci_write_snoop_early_resp_en(unsigned int en)
{
if(en)
{
    *((unsigned int *) MCI_SCR_S0) |= 0x8;
}
else
{
    *((unsigned int *) MCI_SCR_S0) &= ~(0x8);
}
    dsb();
    //mci_conf.mci_write_snoop_early_resp_en = en;
}

// Write outstanding enable: enable write snoop outstanding to EMI
static void mci_write_snoop_outstanding_en(unsigned int en)
{
if(en)
{
    *((unsigned int *) MCI_SCR_S0) |= 0x10;
}
else
{
    *((unsigned int *) MCI_SCR_S0) &= ~(0x10);
}
    dsb();
    //mci_conf.mci_write_snoop_outstanding_en = en;
}

static struct mt_mci_driver {
        struct device_driver driver;
        const struct platform_device_id *id_table;
};

static struct mt_mci_driver mt_mci_drv = {
    .driver = {
	.name = "mci",
	.bus = &platform_bus_type,
	.owner = THIS_MODULE,
    },
    .id_table= NULL,
};

#ifdef LDVT
int cohen;
int cacheable;
#define BUFF_LEN        320
#define PARTIAL_BUFF_LEN        280
#define PATTERN1 0x5A5A5A5A
#define PATTERN2 0xA5A5A5A5
#define PATTERN3 0xDEADDEAD
unsigned int *dst_array_v;
unsigned int *src_array_v;
unsigned int *dst_addr,*src_addr;
dma_addr_t dst_array_p;
dma_addr_t src_array_p;

unsigned int *dst_array_v2;
unsigned int *src_array_v2;
dma_addr_t dst_array_p2;
dma_addr_t src_array_p2;

struct mt65xx_gdma_conf dma_conf = {
    .count = BUFF_LEN,
    .iten = DMA_TRUE,
    .burst = DMA_CON_BURST_SINGLE,
    //.burst = DMA_CON_BURST_2BEAT,
    .dinc = DMA_TRUE,
    .sinc = DMA_TRUE,
    .cohen = DMA_TRUE, //enable coherence bus
    .sec = DMA_FALSE, // non-security channel
    .limiter = 0,
};

int channel, channel2;

int dma_test_exit(unsigned int cohen,unsigned cacheable)
{
    if (cacheable){
        printk("in dma_unmap_single\nn");
        dma_unmap_single(NULL,dst_array_p,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_TO_DEVICE);
        dma_unmap_single(NULL,src_array_p,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_FROM_DEVICE);
        //kfree(dst_array_v);
        //kfree(src_array_v);
    }
    return 0;
}
void irq_dma_handler(void * data)
{
    int i;
    int result = 0;
    mt65xx_stop_gdma(channel);
    dma_test_exit(cohen,cacheable);
    for(i = 0; i < BUFF_LEN/sizeof(unsigned int); i++)
        if(dst_array_v[i] != src_array_v[i]){
            printk("DMA ERROR at Address %x,%x != %x\n", &dst_array_v[i],dst_array_v[i],src_array_v[i]);
            result = -1;
        }
    if (result == 0)
        printk("Channel1 DMA TEST PASS\n");
    else
        printk("Channel1 DMA TEST failed\n");
    printk("in handler:MCI_EVENT0_CNT=%x,MCI_EVENT1_CNT:%x\n",*((unsigned int *) MCI_EVENT0_CNT),*((unsigned int *) MCI_EVENT1_CNT));
   
    mt65xx_dma_running_status();
    mt65xx_free_gdma(channel);
}

void irq_dma_handler2(void * data)
{
    int i;
    mt65xx_stop_gdma(channel2);
    for(i = 0; i < BUFF_LEN/sizeof(unsigned int); i++)
        if(dst_array_v2[i] != src_array_v2[i])
            printk("DMA ERROR at Address %x\n", &dst_array_v2[i]);
    
    printk("Channel2 DMA TEST PASS\n");
    mt65xx_dma_running_status();
    mt65xx_free_gdma(channel2);
    dma_test_exit(cohen,cacheable);
}

void irq_dma_handler3(void * data)
{
    int i;
    int result = 0;
    mt65xx_stop_gdma(channel);
    //because we have MCI, we don't need to invalidate cache
    //dma_test_exit(cohen,cacheable);
    for(i = 0; i < BUFF_LEN/sizeof(unsigned int); i++)
        if(dst_array_v[i] != PATTERN3){
            printk("DMA ERROR at Address %x,%x != %x, (%x=%x)\n", &dst_array_v[i],dst_array_v[i],PATTERN3,&src_array_v[i],src_array_v[i]);
            result = -1;
        }
    
    printk("in handler:MCI_EVENT0_CNT=%x,MCI_EVENT1_CNT:%x\n",*((unsigned int *) MCI_EVENT0_CNT),*((unsigned int *) MCI_EVENT1_CNT));
    if (result == 0)
        printk("Read snoop TEST PASS\n");
    else
        printk("Read snoop TEST failed\n");
    mt65xx_dma_running_status();
    mt65xx_free_gdma(channel);


}
void irq_dma_handler4(void * data)
{
    int i;
    int result = 0;
    mt65xx_stop_gdma(channel);
    //because we have MCI, we don't need to invalidate cache
    //dma_test_exit(cohen,cacheable);
    for(i = 0; i < PARTIAL_BUFF_LEN/sizeof(unsigned int); i++)
        if(dst_array_v[i] != i){
            printk("DMA ERROR at Address %x,%x != %x, (%x=%x)\n", &dst_array_v[i],dst_array_v[i],PATTERN3,&src_array_v[i],src_array_v[i]);
            result = -1;
        }
    for(; i < BUFF_LEN/sizeof(unsigned int); i++)
        if(dst_array_v[i] != PATTERN3){
            printk("DMA ERROR at Address %x,%x != %x, (%x=%x)\n", &dst_array_v[i],dst_array_v[i],PATTERN3,&src_array_v[i],src_array_v[i]);
            result = -1;
        }
    

    printk("in handler:MCI_EVENT0_CNT=%x,MCI_EVENT1_CNT:%x\n",*((unsigned int *) MCI_EVENT0_CNT),*((unsigned int *) MCI_EVENT1_CNT));
    if (result == 0)
        printk("Write snoop TEST PASS\n");
    else
        printk("Write snoop TEST failed\n");
    mt65xx_dma_running_status();
    mt65xx_free_gdma(channel);


}
static int dma_test_init(unsigned int cohen,unsigned cacheable)
{
    int i;
    
    channel = mt65xx_req_gdma(irq_dma_handler, NULL);
    printk("GDMA channel:%d\n",channel);
    if(channel < 0 ){
        printk("ERROR Register DMA\n");
        return -1;
    }
    
    mt_reset_dma(channel);
    if (cacheable){
        dst_array_v = kzalloc(sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), GFP_ATOMIC);
        src_array_v = kzalloc(sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), GFP_ATOMIC);
    }else{
        dst_array_v = dma_alloc_coherent(NULL, sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), &dst_array_p, GFP_KERNEL );
        src_array_v = dma_alloc_coherent(NULL, sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), &src_array_p, GFP_KERNEL );
    }
    
    for(i = 0; i < BUFF_LEN/sizeof(unsigned int); i++) {
        dst_array_v[i] = 0;
        src_array_v[i] = i;
    }
    
    if (cacheable){
        //after dma_map_single(), the memory should not be touched
        //it will dirty the cacheline
        dst_array_p = dma_map_single(NULL,dst_array_v,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_TO_DEVICE);
        src_array_p = dma_map_single(NULL,src_array_v,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_FROM_DEVICE);
        if (dma_mapping_error(NULL,dst_array_p) || dma_mapping_error(NULL,src_array_p)){
            printk("ERROR Couldn't map DMA\n");
            return -1;
        }
        printk("[DMA]dst_array_v:%x,src_array_v:%x,dst_array_p:%x,src_array_p:%x\n",dst_array_v,src_array_v,dst_array_p,src_array_p);
    }
    dma_conf.src = src_array_p;
    dma_conf.dst = dst_array_p;

    smp_inner_dcache_flush_all();
    /* setup coherence bus*/ 
    if (cohen)
        dma_conf.cohen = DMA_TRUE;
    else
        dma_conf.cohen = DMA_FALSE;

    if ( mt65xx_config_gdma(channel, &dma_conf, ALL) != 0) 
        printk("ERROR set DMA\n");
    
    
    return 0;
}
// --------------------------------------------------------
// do memory test
// --------------------------------------------------------

int
complex_mem_test ()
{
    unsigned int start;
    unsigned char *MEM8_BASE; 
    unsigned short *MEM16_BASE;
    unsigned int *MEM32_BASE; 
    unsigned int *MEM_BASE; 
    unsigned char pattern8;
    unsigned short pattern16;
    unsigned int i, j, size, pattern32;
    unsigned int value;
    unsigned len = SZ_4M;
    start = kmalloc(len, GFP_KERNEL);
    MEM8_BASE = (unsigned char *) start;
    MEM16_BASE = (unsigned short *) start;
    MEM32_BASE = (unsigned int *) start;
    MEM_BASE = (unsigned int *) start;

    printk("[mci][cpu%d]memory test: start:%x\n",raw_smp_processor_id(),start);
    size = len >> 2;

    /* === Verify the tied bits (tied high) === */
    for (i = 0; i < size; i++)
    {
        MEM32_BASE[i] = 0;
    }

    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0)
        {
            return -1;
        }
        else
        {
            MEM32_BASE[i] = 0xffffffff;
        }
    }

    /* === Verify the tied bits (tied low) === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xffffffff)
        {
            return -2;
        }
        else
            MEM32_BASE[i] = 0x00;
    }

    /* === Verify pattern 1 (0x00~0xff) === */
    pattern8 = 0x00;
    for (i = 0; i < len; i++)
        MEM8_BASE[i] = pattern8++;
    pattern8 = 0x00;
    for (i = 0; i < len; i++)
    {
        if (MEM8_BASE[i] != pattern8++)
        {
            return -3;
        }
    }

    /* === Verify pattern 2 (0x00~0xff) === */
    pattern8 = 0x00;
    for (i = j = 0; i < len; i += 2, j++)
    {
        if (MEM8_BASE[i] == pattern8)
            MEM16_BASE[j] = pattern8;
        if (MEM16_BASE[j] != pattern8)
        {
            return -4;
        }
        pattern8 += 2;
    }

    /* === Verify pattern 3 (0x00~0xffff) === */
    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++)
        MEM16_BASE[i] = pattern16++;
    pattern16 = 0x00;
    for (i = 0; i < (len >> 1); i++)
    {
        if (MEM16_BASE[i] != pattern16++)
        {
            return -5;
        }
    }

    /* === Verify pattern 4 (0x00~0xffffffff) === */
    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++)
        MEM32_BASE[i] = pattern32++;
    pattern32 = 0x00;
    for (i = 0; i < (len >> 2); i++)
    {
        if (MEM32_BASE[i] != pattern32++)
        {
            return -6;
        }
    }

    /* === Pattern 5: Filling memory range with 0x44332211 === */
    for (i = 0; i < size; i++)
        MEM32_BASE[i] = 0x44332211;

    /* === Read Check then Fill Memory with a5a5a5a5 Pattern === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0x44332211)
        {
            return -7;
        }
        else
        {
            MEM32_BASE[i] = 0xa5a5a5a5;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 0h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa5a5a5a5)
        {
            return -8;
        }
        else
        {
            MEM8_BASE[i * 4] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 2h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa5a5a500)
        {
            return -9;
        }
        else
        {
            MEM8_BASE[i * 4 + 2] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 1h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa500a500)
        {
            return -10;
        }
        else
        {
            MEM8_BASE[i * 4 + 1] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with 00 Byte Pattern at offset 3h === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xa5000000)
        {
            return -11;
        }
        else
        {
            MEM8_BASE[i * 4 + 3] = 0x00;
        }
    }

    /* === Read Check then Fill Memory with ffff Word Pattern at offset 1h == */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0x00000000)
        {
            return -12;
        }
        else
        {
            MEM16_BASE[i * 2 + 1] = 0xffff;
        }
    }


    /* === Read Check then Fill Memory with ffff Word Pattern at offset 0h == */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xffff0000)
        {
            return -13;
        }
        else
        {
            MEM16_BASE[i * 2] = 0xffff;
        }
    }


    /*===  Read Check === */
    for (i = 0; i < size; i++)
    {
        if (MEM32_BASE[i] != 0xffffffff)
        {
            return -14;
        }
    }


    /************************************************
    * Additional verification 
    ************************************************/
    /* === stage 1 => write 0 === */

    for (i = 0; i < size; i++)
    {
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 2 => read 0, write 0xF === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];

        if (value != PATTERN1)
        {
            return -15;
        }
        MEM_BASE[i] = PATTERN2;
    }


    /* === stage 3 => read 0xF, write 0 === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN2)
        {
            return -16;
        }
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 4 => read 0, write 0xF === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN1)
        {
            return -17;
        }
        MEM_BASE[i] = PATTERN2;
    }


    /* === stage 5 => read 0xF, write 0 === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN2)
        {
            return -18;
        }
        MEM_BASE[i] = PATTERN1;
    }


    /* === stage 6 => read 0 === */
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != PATTERN1)
        {
            return -19;
        }
    }


    /* === 1/2/4-byte combination test === */
    i = (unsigned int) MEM_BASE;

    while (i < (unsigned int) MEM_BASE + (size << 2))
    {
        *((unsigned char *) i) = 0x78;
        i += 1;
        *((unsigned char *) i) = 0x56;
        i += 1;
        *((unsigned short *) i) = 0x1234;
        i += 2;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
        *((unsigned short *) i) = 0x5678;
        i += 2;
        *((unsigned char *) i) = 0x34;
        i += 1;
        *((unsigned char *) i) = 0x12;
        i += 1;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
        *((unsigned char *) i) = 0x78;
        i += 1;
        *((unsigned char *) i) = 0x56;
        i += 1;
        *((unsigned short *) i) = 0x1234;
        i += 2;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
        *((unsigned short *) i) = 0x5678;
        i += 2;
        *((unsigned char *) i) = 0x34;
        i += 1;
        *((unsigned char *) i) = 0x12;
        i += 1;
        *((unsigned int *) i) = 0x12345678;
        i += 4;
    }
    for (i = 0; i < size; i++)
    {
        value = MEM_BASE[i];
        if (value != 0x12345678)
        {
            return -20;
        }
    }


    /* === Verify pattern 1 (0x00~0xff) === */
    pattern8 = 0x00;
    MEM8_BASE[0] = pattern8;
    for (i = 0; i < size * 4; i++)
    {
        unsigned char waddr8, raddr8;
        waddr8 = i + 1;
        raddr8 = i;
        if (i < size * 4 - 1)
            MEM8_BASE[waddr8] = pattern8 + 1;
        if (MEM8_BASE[raddr8] != pattern8)
        {
            return -21;
        }
        pattern8++;
    }


    /* === Verify pattern 2 (0x00~0xffff) === */
    pattern16 = 0x00;
    MEM16_BASE[0] = pattern16;
    for (i = 0; i < size * 2; i++)
    {
        if (i < size * 2 - 1)
            MEM16_BASE[i + 1] = pattern16 + 1;
        if (MEM16_BASE[i] != pattern16)
        {
            return -22;
        }
        pattern16++;
    }


    /* === Verify pattern 3 (0x00~0xffffffff) === */
    pattern32 = 0x00;
    MEM32_BASE[0] = pattern32;
    for (i = 0; i < size; i++)
    {
        if (i < size - 1)
            MEM32_BASE[i + 1] = pattern32 + 1;
        if (MEM32_BASE[i] != pattern32)
        {
            return -23;
        }
        pattern32++;
    }

    return 0;
}
static void ts_complex_mem_test(){

    int ret;

    ret = complex_mem_test();
    if (ret < 0)
        printk("[MCI][CPU%d]complex_mem_test failed\n",raw_smp_processor_id());
    else
        printk("[MCI][CPU%d]complex_mem_test passed\n",raw_smp_processor_id());
}

static int ts_MCU_EMI_cache_on(void){
    /* access normal memory with cache on */
    /* multi core access*/

    printk("in %s",__FUNCTION__);
    get_online_cpus();
        on_each_cpu(ts_complex_mem_test, NULL, false);
    put_online_cpus();


    return 0;
}
static int ts_ACELite_EMI(void){
    printk("in %s",__FUNCTION__);
    cacheable = 1;
    cohen = 0; // non shareable
    dma_test_init(cohen,cacheable);
#if 1
    printk("in testcases:MCI_EVENT0_CNT=%x,MCI_EVENT1_CNT:%x\n",*((unsigned int *) MCI_EVENT0_CNT),*((unsigned int *) MCI_EVENT1_CNT));
    *((unsigned int *) MCI_EVENT0_SEL) = 0x2; //AC_R snoop transaction count MCI->CPU
    //*((unsigned int *) MCI_EVENT1_SEL) = 0x4; //AC_R snoop hit
    *((unsigned int *) MCI_EVENT1_SEL) = 0x0; // DMA->MCI
    *((unsigned int *) MCI_EVENT0_CON) = 0x1; // enable counter 0 
    *((unsigned int *) MCI_EVENT1_CON) = 0x1; // enable counter 1
    *((unsigned int *) MCI_PMCR) |= 0x2; // retset perf counter
    *((unsigned int *) MCI_PMCR) &= ~0x2; // reset performance counter
#endif
 
    printk("Start %d\n",mt65xx_start_gdma(channel));
    mt65xx_dma_running_status();
    return 0;
}
static int make_cache_dirty(unsigned int dirty_cache_addr){
    
    unsigned int* write_array_pcpu;
    unsigned int cpu_id;
    unsigned int len_pcpu = BUFF_LEN/4;
    int i;
    cpu_id = smp_processor_id();
    printk("dirty_cache_addr:%x,&src_array_v[0]=%x\n",dirty_cache_addr,&src_array_v[0]); 
    printk("[CPU%d]in %s\n",cpu_id,__FUNCTION__);
    write_array_pcpu =  dirty_cache_addr + cpu_id * len_pcpu;
    for(i = 0; i < len_pcpu/sizeof(unsigned int); i++){
       *(write_array_pcpu+i) = 0xDEADDEAD; 
       printk("%x=%x\n",write_array_pcpu+i,*(write_array_pcpu+i));
    }

    return 0; 

}
static int ts_ACELite_MCU_snoop_read_cache_hit(void){
    /* access normal memory with dma  */
    /* multi core access*/
    /* performance monitor*/
    /* cohen = 1, the mci will handle, after dma transfer, we don't need to flush the cache, the result will pass */
    /* cohen = 0, the mci will not handle, the result will failed */
    int i;
    cohen = 1;

    cacheable = 1;
    printk("in %s",__FUNCTION__);
    /* setup dma*/
    channel = mt65xx_req_gdma(irq_dma_handler3, NULL);
    if(channel < 0 ){
        printk("ERROR Register DMA\n");
        return -1;
    }
    mt_reset_dma(channel);
    dst_array_v = kzalloc(sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), GFP_ATOMIC);
    src_array_v = kzalloc(sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), GFP_ATOMIC);
    dst_array_p = dma_map_single(NULL,dst_array_v,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_TO_DEVICE);
    src_array_p = dma_map_single(NULL,src_array_v,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_FROM_DEVICE);
    if (dma_mapping_error(NULL,dst_array_p) || dma_mapping_error(NULL,src_array_p)){
        printk("ERROR Couldn't map DMA\n");
        return -1;
    }
    printk("[DMA]dst_array_v:%x,src_array_v:%x,dst_array_p:%x,src_array_p:%x, size:%x\n",dst_array_v,src_array_v,dst_array_p,src_array_p,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int));
    /* make dma go through coherence bus*/
    if (cohen)
        dma_conf.cohen = DMA_TRUE;
    else
        dma_conf.cohen = DMA_FALSE;

    /* configure security chaneel*/
    dma_conf.sec = DMA_TRUE;

    /* configure physical address*/
    dma_conf.src = src_array_p;
    dma_conf.dst = dst_array_p;

    /* setup array default value*/
    for(i = 0; i < BUFF_LEN/sizeof(unsigned int); i++) {
        dst_array_v[i] = 0;
        src_array_v[i] = i;
    }

    if ( mt65xx_config_gdma(channel, &dma_conf, ALL) != 0)
        printk("ERROR set DMA\n");

#if 1
    /* flush all inner cache*/
    smp_inner_dcache_flush_all();
    
    /* make cache dirty according to CPU_ID*/
    get_online_cpus();
        // make src_array dirty for read snoopy test
        on_each_cpu(make_cache_dirty, src_array_v, true);

    put_online_cpus();
    // flush again, it will get a hit counter = 0
    /* flush all inner cache*/
    //smp_inner_dcache_flush_all();
#endif
#if 1
    printk("in testcases:MCI_EVENT0_CNT=%x,MCI_EVENT1_CNT:%x\n",*((unsigned int *) MCI_EVENT0_CNT),*((unsigned int *) MCI_EVENT1_CNT));
    *((unsigned int *) MCI_EVENT0_SEL) = 0x2; //AC_R snoop transaction count MCI->CPU
    *((unsigned int *) MCI_EVENT1_SEL) = 0x4; //AC_R snoop hit
    //*((unsigned int *) MCI_EVENT1_SEL) = 0x0; // DMA->MCI
    *((unsigned int *) MCI_EVENT0_CON) = 0x1; // enable counter 0 
    *((unsigned int *) MCI_EVENT1_CON) = 0x1; // enable counter 1
    *((unsigned int *) MCI_PMCR) |= 0x2; // retset perf counter
    *((unsigned int *) MCI_PMCR) &= ~0x2; // reset performance counter
#endif
    printk("Start %d\n",mt65xx_start_gdma(channel));
    mt65xx_dma_running_status();
    //dma_test_exit(cohen,cacheable);


    return 0;
}
static int ts_ACELite_MCU_snoop_read_cache_miss(void){
    /* access normal memory with dma  */
    /* multi core access*/
    /* performance monitor*/
    int i;
    cohen = 1;
    cacheable = 1;
    printk("in %s",__FUNCTION__);
    /* setup dma*/
    channel = mt65xx_req_gdma(irq_dma_handler3, NULL);
    if(channel < 0 ){
        printk("ERROR Register DMA\n");
        return -1;
    }
    mt_reset_dma(channel);
    dst_array_v = kzalloc(sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), GFP_ATOMIC);
    src_array_v = kzalloc(sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), GFP_ATOMIC);
    dst_array_p = dma_map_single(NULL,dst_array_v,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_TO_DEVICE);
    src_array_p = dma_map_single(NULL,src_array_v,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_FROM_DEVICE);
    if (dma_mapping_error(NULL,dst_array_p) || dma_mapping_error(NULL,src_array_p)){
        printk("ERROR Couldn't map DMA\n");
        return -1;
    }
    printk("[DMA]dst_array_v:%x,src_array_v:%x,dst_array_p:%x,src_array_p:%x, size:%x\n",dst_array_v,src_array_v,dst_array_p,src_array_p,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int));
    /* make dma go through coherence bus*/
    if (cohen)
        dma_conf.cohen = DMA_TRUE;
    else
        dma_conf.cohen = DMA_FALSE;

    /* configure security chaneel*/
    dma_conf.sec = DMA_TRUE;

    /* configure physical address*/
    dma_conf.src = src_array_p;
    dma_conf.dst = dst_array_p;

    /* setup array default value*/
    for(i = 0; i < BUFF_LEN/sizeof(unsigned int); i++) {
        dst_array_v[i] = 0;
        src_array_v[i] = i;
    }

    if ( mt65xx_config_gdma(channel, &dma_conf, ALL) != 0)
        printk("ERROR set DMA\n");

#if 1
    /* flush all inner cache*/
    smp_inner_dcache_flush_all();
    
    /* make cache dirty according to CPU_ID*/
    get_online_cpus();
        on_each_cpu(make_cache_dirty, src_array_v, true);
    put_online_cpus();

    /* flush all inner cache*/
    smp_inner_dcache_flush_all();

#endif
#if 1
    printk("in testcases:MCI_EVENT0_CNT=%x\n",*((unsigned int *) MCI_EVENT0_CNT));
    *((unsigned int *) MCI_EVENT0_SEL) = 0x2; //AC_R snoop transaction count
    *((unsigned int *) MCI_EVENT1_SEL) = 0x4; //AC_R snoop hit
    *((unsigned int *) MCI_EVENT0_CON) = 0x1; // enable counter 0 
    *((unsigned int *) MCI_EVENT1_CON) = 0x1; // enable counter 1
#endif
    printk("Start %d\n",mt65xx_start_gdma(channel));
    mt65xx_dma_running_status();
    //dma_test_exit(cohen,cacheable);
    return 0;
}
static int ts_ACELite_MCU_snoop_write_partial_line(void){
    /* access normal memory with dma  */
    /* multi core access*/
    int i;
    cohen = 1;
    cacheable = 1;
    printk("in %s",__FUNCTION__);
    /* setup dma*/
    channel = mt65xx_req_gdma(irq_dma_handler4, NULL);
    if(channel < 0 ){
        printk("ERROR Register DMA\n");
        return -1;
    }
    mt_reset_dma(channel);
    dst_array_v = kzalloc(sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), GFP_ATOMIC);
    src_array_v = kzalloc(sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int), GFP_ATOMIC);
    dst_array_p = dma_map_single(NULL,dst_array_v,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_TO_DEVICE);
    src_array_p = dma_map_single(NULL,src_array_v,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int),DMA_FROM_DEVICE);
    if (dma_mapping_error(NULL,dst_array_p) || dma_mapping_error(NULL,src_array_p)){
        printk("ERROR Couldn't map DMA\n");
        return -1;
    }
    printk("[DMA]dst_array_v:%x,src_array_v:%x,dst_array_p:%x,src_array_p:%x, size:%x\n",dst_array_v,src_array_v,dst_array_p,src_array_p,sizeof(unsigned int ) * BUFF_LEN/sizeof(unsigned int));
    /* make dma go through coherence bus*/
    if (cohen)
        dma_conf.cohen = DMA_TRUE;
    else
        dma_conf.cohen = DMA_FALSE;

    /* configure security chaneel*/
    dma_conf.sec = DMA_TRUE;

    /* configure physical address*/
    dma_conf.src = src_array_p;
    dma_conf.dst = dst_array_p;

    /* configure as partial dma transfer */
    dma_conf.count = PARTIAL_BUFF_LEN;

    /* setup array default value*/
    for(i = 0; i < BUFF_LEN/sizeof(unsigned int); i++) {
        dst_array_v[i] = 0;
        src_array_v[i] = i;
    }

    if ( mt65xx_config_gdma(channel, &dma_conf, ALL) != 0)
        printk("ERROR set DMA\n");

#if 1
    /* flush all inner cache*/
    smp_inner_dcache_flush_all();
    
    /* make cache dirty according to CPU_ID*/
    get_online_cpus();
        on_each_cpu(make_cache_dirty, dst_array_v, true);
    put_online_cpus();

    /* flush all inner cache*/
    //smp_inner_dcache_flush_all();

#endif
#if 1
    printk("in testcases:MCI_EVENT0_CNT=%x\n",*((unsigned int *) MCI_EVENT0_CNT));
    *((unsigned int *) MCI_EVENT0_SEL) = 0x3; //AC_W snoop transaction count
    *((unsigned int *) MCI_EVENT1_SEL) = 0x5; //AC_W snoop hit
    *((unsigned int *) MCI_EVENT0_CON) = 0x1; // enable counter 0 
    *((unsigned int *) MCI_EVENT1_CON) = 0x1; // enable counter 1
#endif
    printk("Start %d\n",mt65xx_start_gdma(channel));
    mt65xx_dma_running_status();

    return 0;
}
static int ts_ACELite_MCU_snoop_write_full_line(void){
    /* access normal memory with dma  */
    /* multi core access*/
    return 0;
}
static int ts_sleep_mci(void){
    return 0;
}
/*
 * mci_dvt_show: To show usage.
 */
static ssize_t mci_dvt_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "MCI dvt test\n");
}

/*
 * mci_dvt_store: To select mci test case.
 */
static ssize_t mci_dvt_store(struct device_driver *driver, const char *buf,
			      size_t count)
{
	char *p = (char *)buf;
	unsigned int num;

	num = simple_strtoul(p, &p, 10);
        switch(num){
            case 1:
                ts_MCU_EMI_cache_on();
                break;
            case 2:
                ts_ACELite_EMI();
                break;
            case 3:
                ts_ACELite_MCU_snoop_read_cache_hit();
                break;
            case 4:
                ts_ACELite_MCU_snoop_write_partial_line();
                break;
            default:
                break;
        }

	return count;
}
DRIVER_ATTR(mci_dvt, 0664, mci_dvt_show, mci_dvt_store);
#endif //!LDVT
int mt_mci_init(void){
        int ret;
        struct MCI_CONF conf;
        dbg_printk("[MCI] MCI init...\n");
        conf.mci_bar=MCI_ENABLE;
        conf.mci_snoop=MCI_ENABLE;
        conf.mci_write_skip=MCI_ENABLE;
        conf.mci_write_snoop_early_resp=MCI_ENABLE;
        conf.mci_write_snoop_outstanding=MCI_ENABLE;
        
#if defined(CONFIG_MTK_MCI)
        mci_setup(conf);
        mci_dump_structure();
#endif
        ret = driver_register(&mt_mci_drv.driver);
#ifdef LDVT
	ret = driver_create_file(&mt_mci_drv.driver, &driver_attr_mci_dvt);
#endif
        if (ret == 0)
        dbg_printk("MCI init done...\n");
	return ret;

}
int mt_mci_exit(void){

        return 0;

}
module_init(mt_mci_init);
module_exit(mt_mci_exit);

EXPORT_SYMBOL(mt_mci_init);
EXPORT_SYMBOL(mci_snoop_sleep);
EXPORT_SYMBOL(mci_snoop_restore);
EXPORT_SYMBOL(mci_sleep);
EXPORT_SYMBOL(mci_restore);
