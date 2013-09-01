#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <mach/mt_cirq.h>
#include <mach/mt_reg_base.h>
#include <asm/system.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <mach/mt_sleep.h>
#include "mach/sync_write.h"
#include "mach/irqs.h"
#include <asm/mach/irq.h>
#include <asm/hardware/gic.h>


#define CIRQ_DEBUG   0
#if(CIRQ_DEBUG == 1)
#ifdef CTP
#define dbgmsg dbg_print
#else
#define dbgmsg printk
#define print_func() do { \
    printk("in %s\n",__func__); \
} while(0)
#endif
#else
#define dbgmsg(...)
#define print_func() do { }while(0)
#endif

#define LDVT
#define INT_POL_CTL0 (MCUSYS_CFGREG_BASE + 0x100)

struct mt_cirq_driver{
    struct device_driver driver;
    const struct platform_device_id *id_table;
};

static struct mt_cirq_driver mt_cirq_drv = {
    .driver = {
        .name = "cirq",
        .bus = &platform_bus_type,
        .owner = THIS_MODULE,
    },
    .id_table= NULL,
};

unsigned int mt_cirq_get_mask(unsigned int cirq_num)
{
    return 0;
}

void mt_cirq_mask_all(void)
{
int i;

for(i = 0; i < 6; i++)
    mt65xx_reg_sync_writel(0xFFFFFFFF, CIRQ_MASK_SET0 + i * 4);

}

void mt_cirq_unmask_all(void)
{
int i;

for(i = 0; i < 6; i++)
    mt65xx_reg_sync_writel(0xFFFFFFFF, CIRQ_MASK_CLR0 + i * 4);

}

void mt_cirq_ack_all(void)
{
int i;
        for(i = 0; i < 6; i++)
            mt65xx_reg_sync_writel(0xFFFFFFFF, CIRQ_ACK0 + i * 4);

}

void mt_cirq_mask(unsigned int cirq_num)
{
	unsigned int base;
	unsigned int bit = 1 << (cirq_num % 32);
        print_func();

        base = (cirq_num / 32) * 4 + CIRQ_ACK0;
        mt65xx_reg_sync_writel(bit, base);

        base = (cirq_num / 32) * 4 + CIRQ_MASK_SET0;
	mt65xx_reg_sync_writel(bit, base);

	//dbgmsg("[CIRQ] mask addr:%x = %x\n", base, bit);

}
void mt_cirq_unmask(unsigned int cirq_num)
{
        unsigned int base;
	unsigned int bit = 1 << (cirq_num % 32);

        print_func();
        base = (cirq_num / 32) * 4 + CIRQ_ACK0;
        mt65xx_reg_sync_writel(bit, base);
        //dbgmsg("[CIRQ] ack :%x, bit: %x\n",  base, bit);

        base = (cirq_num / 32) * 4 + CIRQ_MASK_CLR0;
	mt65xx_reg_sync_writel(bit, base);

	//dbgmsg("[CIRQ] unmask addr:%x = %x\n", base, bit);

}
static void mt_cirq_set_pol(unsigned int cirq_num, unsigned int pol)
{
	unsigned int base;
	unsigned int bit = 1 << (cirq_num % 32);
	

        print_func();
	if (pol == MT_CIRQ_POL_NEG) {
                base = (cirq_num / 32) * 4 + CIRQ_POL_CLR0;
	} else {
                base = (cirq_num / 32) * 4 + CIRQ_POL_SET0;
	}
	mt65xx_reg_sync_writel(bit, base);
	dbgmsg("[CIRQ] set pol,%d :%x, bit: %x\n",pol, base, bit);
	
}
unsigned int mt_cirq_set_sens(unsigned int cirq_num, unsigned int sens)
{
	unsigned int base;
	unsigned int bit = 1 << (cirq_num % 32);

        print_func();
	if (sens == MT65xx_EDGE_SENSITIVE) {
            base = (cirq_num / 32) * 4 + CIRQ_SENS_CLR0;
	} else if (sens == MT65xx_LEVEL_SENSITIVE) {
            base = (cirq_num / 32) * 4 + CIRQ_SENS_SET0;
	} else {
		dbgmsg("%s invalid sensitivity value\n", __func__);
		return 0;
	}
	mt65xx_reg_sync_writel(bit, base);
	dbgmsg("[CIRQ] %s,sens:%d :%x, bit: %x\n", __func__,sens, base, bit);
	return 0;

}
unsigned int mt_cirq_get_sens(unsigned int cirq_num)
{

        print_func();
        return 0;
}
unsigned int mt_cirq_ack(unsigned int cirq_num)
{

        print_func();
        return 0;
}

unsigned int mt_cirq_read_status(unsigned int cirq_num)
{

        print_func();
        return 0;
}
void mt_cirq_enable(){

        print_func();
    *(volatile unsigned int*) CIRQ_CON |= 0x1;  
    dsb();
}
void mt_cirq_disable(){

        print_func();
    *(volatile unsigned int*) CIRQ_CON &= (~0x1);  
    dsb();
}
void mt_cirq_flush(){

    unsigned int irq;
    unsigned int val;
    print_func();
    for (irq = 64; irq < (MT_NR_SPI + 32); irq+=32)
    {
        val = readl(((irq-64) / 32) * 4 + CIRQ_STA0);
        //printk("irq:%d,pending bit:%x\n",irq,val);
         *(volatile u32 *)(GIC_DIST_BASE + GIC_DIST_PENDING_SET + irq / 32 * 4) = val;
        dsb();
        //printk("irq:%d,pending bit:%x,%x\n",irq,val,readl(GIC_DIST_BASE + GIC_DIST_PENDING_SET + irq / 32 * 4));
    }
    mt_cirq_ack_all();
    //*(volatile unsigned int*) CIRQ_CON |= 0x4;  
    dsb();
}
void mt_cirq_clone_pol(void)
{
    unsigned int irq,irq_bit,irq_offset;
    unsigned int value;
    unsigned int value_cirq;
    int ix;
    print_func();
    for (irq = 64; irq < (MT_NR_SPI + 32); irq+=32)
    {
        value = readl(INT_POL_CTL0 + ((irq-32) / 32 * 4));
        irq_offset = (irq-64) / 32;
        for (ix = 0; ix < 32; ix++)
        {
            dbgmsg("irq:%d,pol:%8x,value:0x%08x\n",irq,INT_POL_CTL0 + (irq / 32 * 4),value);
            irq_bit = (irq_offset+ix) % 32;
            if (value & (0x1)) //high trigger 
                mt_cirq_set_pol(irq+ix-64,MT_CIRQ_POL_NEG);
                //*(volatile unsigned int*)((unsigned int)CIRQ_POL_SET0 + irq_offset*4) |= (0x1 << irq_bit);
            else//low trigger
                mt_cirq_set_pol(irq+ix-64,MT_CIRQ_POL_POS);
                //*(volatile unsigned int*)((unsigned int)CIRQ_POL_CLR0 + irq_offset*4) |= (0x1 << irq_bit);
            value >>= 1;
        }
        value_cirq = *(volatile unsigned int*)((unsigned int)CIRQ_POL0 + irq_offset*4);
        dbgmsg("irq:%d,cirq_value:0x%08x\n",irq,value_cirq);
    }
}

void mt_cirq_clone_sens(void)
{
    unsigned int irq,irq_bit,irq_offset;
    unsigned int value;
    unsigned int value_cirq;
    int ix;
    print_func();
    for (irq = 64; irq < (MT_NR_SPI + 16); irq+=16)
    {   
        value = readl(GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4);
        dbgmsg("irq:%d,sens:%08x,value:0x%08x\n",irq,GIC_DIST_BASE + GIC_DIST_CONFIG + (irq / 16) * 4,value);
        irq_offset = (irq-64) / 32;
        for (ix = 0; ix < 16; ix++)
        {
            irq_bit = (irq+ix) % 32;
            if (value & (0x2)) //edge trigger 
                mt_cirq_set_sens(irq-64+ix,MT65xx_EDGE_SENSITIVE);
                //*(volatile unsigned int*)((unsigned int)CIRQ_SENS_SET0 + irq_offset*4) |= (0x1 << irq_bit);
            else//level trigger
                mt_cirq_set_sens(irq-64+ix,MT65xx_LEVEL_SENSITIVE);
                //*(volatile unsigned int*)((unsigned int)CIRQ_SENS_CLR0 + irq_offset*4) |= (0x1 << irq_bit);
            value >>= 2;
        }
        value_cirq = *(volatile unsigned int*)((unsigned int)CIRQ_SENS0 + irq_offset*4);
        dbgmsg("irq:%d,cirq_value:0x%08x\n",irq,value_cirq);
    }
}

void mt_cirq_clone_mask(void)
{
    unsigned int irq,irq_bit,irq_offset;
    unsigned int value;
    unsigned int value_cirq;
    int ix;
    print_func();
    for (irq = 64; irq < (MT_NR_SPI + 32); irq+=32)
    {
        //value = *(volatile u32 *)(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4) ;
        value = readl(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4);
        dbgmsg("irq:%d,mask:%08x,value:0x%08x\n",irq,(GIC_DIST_BASE + GIC_DIST_ENABLE_SET + irq / 32 * 4) ,value);
        irq_offset = (irq-64) / 32;
        for (ix = 0; ix < 32; ix++)
        {
            irq_bit = (irq+ix) % 32;
            if (value & (0x1)) //enable  
                mt_cirq_unmask(irq+ix-64);
                //*(volatile unsigned int*)((unsigned int)CIRQ_MASK_SET0 + irq_offset*4) |= (0x1 << irq_bit);
            else//disable
                mt_cirq_mask(irq+ix-64);
                //*(volatile unsigned int*)((unsigned int)CIRQ_MASK_CLR0 + irq_offset*4) |= (0x1 << irq_bit);
            value >>= 1;
        }
        value_cirq = *(volatile unsigned int*)((unsigned int)CIRQ_MASK0 + irq_offset*4);
        dbgmsg("irq:%d,cirq_value:0x%08x\n",irq,value_cirq);
    }
}
void mt_cirq_clone_gic()
{    
    mt_cirq_clone_pol();
    mt_cirq_clone_sens();
    mt_cirq_clone_mask();
}

void mt_cirq_wfi_func()
{
    mt_cirq_mask_all();
    mt_cirq_ack_all();
    mt_cirq_set_pol(MT_MD_WDT1_IRQ_ID - 64, MT_CIRQ_POL_NEG);
    mt_cirq_set_sens(MT_MD_WDT1_IRQ_ID - 64, MT65xx_EDGE_SENSITIVE);
    mt_cirq_unmask(MT_MD_WDT1_IRQ_ID - 64);
}


#if defined(LDVT)
/*
 * cirq_dvt_show: To show usage.
 */
static ssize_t cirq_dvt_show(struct device_driver *driver, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "CIRQ dvt test\n");
}

/*
 * mci_dvt_store: To select mci test case.
 */
static ssize_t cirq_dvt_store(struct device_driver *driver, const char *buf,
			      size_t count)
{
	char *p = (char *)buf;
	unsigned int num;

	num = simple_strtoul(p, &p, 10);
        switch(num){
            case 1:
                mt_cirq_clone_gic();
                break;
            case 2:
                break;
            case 3:
                break;
            default:
                break;
        }

	return count;
}
DRIVER_ATTR(cirq_dvt, 0664, cirq_dvt_show, cirq_dvt_store);
#endif //!LDVT

/*
 * CIRQ interrupt service routine.
 */
static irqreturn_t cirq_irq_handler(int irq, void *dev_id)
{
    printk("CIRQ_Handler\n");
    mt_cirq_ack_all();
    return IRQ_HANDLED;
}

/*
 * always return 0
 * */
int mt_cirq_init(void){
        int ret;
        printk("CIRQ init...\n");
#if 1
        if (request_irq(MT_CIRQ_IRQ_ID, cirq_irq_handler, IRQF_TRIGGER_LOW, "CIRQ",  NULL)) {
            printk(KERN_ERR"CIRQ IRQ LINE NOT AVAILABLE!!\n");
        }else
        {
            printk("CIRQ handler init success.");
        }
#endif
        ret = driver_register(&mt_cirq_drv.driver);
#ifdef LDVT
	ret = driver_create_file(&mt_cirq_drv.driver, &driver_attr_cirq_dvt);
#endif
        if (ret == 0)
        printk("CIRQ init done...\n");
	return 0;

}
int mt_cirq_exit(void){

        return 0;

}
arch_initcall(mt_cirq_init);
EXPORT_SYMBOL(mt_cirq_enable);
EXPORT_SYMBOL(mt_cirq_disable);
EXPORT_SYMBOL(mt_cirq_clone_gic);
EXPORT_SYMBOL(mt_cirq_flush);

