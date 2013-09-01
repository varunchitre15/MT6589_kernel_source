
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>

#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/tcm.h>
#include <mach/irqs.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt6575_sc.h>

#include "adc_ts.h"

extern int tpd_mode_select(int mode);
extern void tpd_enable_em_log(int enable);
extern void tpd_fav_coord_sel(int coord);

#define TS_DEBUG   1

#define TS_NAME    "dvt-ts"

#if TS_DEBUG
    #define ts_print(fmt, arg...)  printk("[ts_udvt]: " fmt, ##arg)
#else
    #define ts_print(fmt, arg...)  do {} while (0)
#endif

struct udvt_cmd {
    int cmd;
    int value;
};

static int touch_log_en = 0;
static int touch_interrupt = 0;  // hw status
static int fav_mode_en = 0;

struct timer_list ts_udvt_timer;
struct tasklet_struct ts_udvt_tasklet;
struct tasklet_struct ts_udvt_auto_sample_tasklet;
struct tasklet_struct ts_fav_mode_tasklet;
static inline u32 POS_XYZ(unsigned int v) 
{
	*(volatile u32 *)AUXADC_TS_ADDR = v; 
	return ((*(volatile u16 *)AUXADC_TS_DATA0) & 0xFFFF);
}



static void dump_AUXADC_regs(void)
{
	int i = 0;
	
	for(i = 0; i <= 0xa0/4; i++)
	{
		printk("[addr:value] 0x%8x:0x%8x\n", (AUXADC_BASE + 4*i), *(volatile u32 *)(AUXADC_BASE + 4*i));
	}
    printk("[addr:value] 0x%8x:0x%8x\n", AUXADC_TS_RAW_CON, *(volatile u32 *)(AUXADC_TS_RAW_CON));
	printk("[addr:value] 0x%8x:0x%8x\n", AUXADC_TS_AUTO_TIME_INTVL, *(volatile u32 *)(AUXADC_TS_AUTO_TIME_INTVL));
}

static void set_touch_resistance(int value)
{
	if(value == 1)
		*(volatile u32 *)AUXADC_TS_CON3 = (1<<15); //90 OKM
	if(value == 0)
		*(volatile u32 *)AUXADC_TS_CON3 = 0x00; //55 OKM
}

/*
* mode: 0: 12-bit, 1: 10-bit
* sedf: 0: different, 1: single-end
* PD: 0x00: turn on Y-_driver signal and PDN_sh_ref
         0x01: turn on PDN_IRQ and PDN_sh_ref
         0x02: reserved
         0x03: turn on PDN_IRQ
*/
static void set_ts_cmd_reg(int mode, int sedf, int pd)
{
	unsigned int value = *(volatile u32 *)AUXADC_TS_CMD;

	if(mode == 1)//10 bit-mode
		value |= (1<<3);
	else
		value &= (~(1<<3));
	if(sedf == 1)//single-end
		value |= (1<<2);
	else
		value &= (~(1<<2));
	value |= pd;
	*(volatile u32 *)AUXADC_TS_CMD = value;

	printk("AUXADC_TS_CMD: 0x%x\n", *(volatile u32 *)AUXADC_TS_CMD);
}

static void set_ts_raw_reg(int en, int start, int abort)
{
	unsigned int value = *(volatile u32 *)AUXADC_TS_RAW_CON;

	if(en == 1)
		value |= (1<<2);
	else
		value &= (~(1<<2));
	if(start == 1)
		value |= (1<<1);
	else
		value &= (~(1<<1));
	value |= abort;
	*(volatile u32 *)AUXADC_TS_RAW_CON = value;

}

unsigned int ts_udvt_read_status() 
{
    return (*(volatile u16 *)AUXADC_TS_CON3) & 2;
}

unsigned int ts_udvt_read(int position) 
{
    //*(volatile u32 *)AUXADC_TS_CMD = 0x8;
    switch(position) {
        case AUXADC_TS_POSITION_X:  
            *(volatile u32 *)AUXADC_TS_ADDR = AUXADC_TS_CMD_ADDR_X ; 
            break;
        case AUXADC_TS_POSITION_Y:
            *(volatile u32 *)AUXADC_TS_ADDR = AUXADC_TS_CMD_ADDR_Y; 
            break;
        case AUXADC_TS_POSITION_Z1:
            *(volatile u32 *)AUXADC_TS_ADDR = AUXADC_TS_CMD_ADDR_Z1; 
            break;
        case AUXADC_TS_POSITION_Z2:
		    *(volatile u32 *)AUXADC_TS_ADDR = AUXADC_TS_CMD_ADDR_Z2; 
           // *(volatile u16 *)AUXADC_TS_CMD = AUXADC_TS_CMD_ADDR_Z2 | AUXADC_TS_SAMPLE_SETTING; 
            break;
        default:
            return 0;
    } 
    
    *(volatile u16 *)AUXADC_TS_CON0 = AUXADC_TS_CON_SPL_TRIGGER;
    
   while (AUXADC_TS_CON_SPL_MASK & (*(volatile u16 *)AUXADC_TS_CON0));
   
   return ((*(volatile u16 *)AUXADC_TS_DATA0) & 0xFFFF);
}


irqreturn_t ts_udvt_FAV_fn(int irq, void *dev_id) 
{
	unsigned int tmp_value = 0;
	unsigned int value = *(volatile u32 *)AUXADC_TS_CON1;
	unsigned int x, y, z1, z2;

	printk("[ts_udvt] AUXADC_TS_CON1:0x%x\n", value);

	//(*(volatile u32 *)AUXADC_TS_CON1) = (((*(volatile u32 *)AUXADC_TS_CON1))|(1<<FAV_EN_BIT));
	//while(((*(volatile u32 *)AUXADC_TS_CON1)&(1<<FAV_EN_BIT)) == 0)break;

    if ((*(volatile u16 *)AUXADC_TS_RAW_CON) & 0x0004) {
       mt65xx_irq_mask(MT65XX_TOUCH_BATCH_LINE);
        if(ts_udvt_auto_sample_tasklet.state != TASKLET_STATE_RUN)
            tasklet_hi_schedule(&ts_udvt_auto_sample_tasklet);
        mt65xx_irq_unmask(MT65XX_TOUCH_BATCH_LINE);
        return IRQ_HANDLED;
    }

	tmp_value = ((value>>5)&0x03);
	if(tmp_value == 0x00)
	{
		x = ts_udvt_read(AUXADC_TS_POSITION_X);
		y = ts_udvt_read(AUXADC_TS_POSITION_Y);
		z1 = ts_udvt_read(AUXADC_TS_POSITION_Z1);
		z2 = ts_udvt_read(AUXADC_TS_POSITION_Z2);
	}
	else
	{
		x = ((*(volatile u16 *)AUXADC_TS_DATA0) & 0xFFFF);
		y = ((*(volatile u16 *)AUXADC_TS_DATA1) & 0xFFFF);
		z1= ((*(volatile u16 *)AUXADC_TS_DATA2) & 0xFFFF);
		z2 =((*(volatile u16 *)AUXADC_TS_DATA3) & 0xFFFF);
	}
	printk("[ts_udvt] FAV mode:0x%x, x:%4d, y:%4d, z1:%4d, z2:%4d\n", tmp_value, x, y, z1, z2);

	tmp_value = value&0x03;
	if(tmp_value != 0)
		printk("[ts_udvt] FAV mode: repeat times: %2d \n", 2*(1<<tmp_value));
	else
		printk("[ts_udvt] FAV mode: repeat times: 1 \n");
	return IRQ_HANDLED;

	
}


/* handle touch panel interrupt */
void ts_udvt_auto_trigger_fn(unsigned long unused) 
{
    int i, j;
    int rx[7], ry[7], rz1[4], rz2[4], x = 0, y = 0, z1 = 0, z2 = 0;
    
    printk("[ts_udvt]: start ts auto trigger\n");
   
    *(volatile u32 *)AUXADC_TS_CON0 = 0x0006;
   
    for (i = 0; i < 8000; i++);
	 
    while ((*(volatile u16 *)AUXADC_CON3) & 0x01) {
        printk("[ts_udvt]: wait for module idle\n");
        mdelay(300);
	  }
   
    printk("[ts_udvt]: auto sampling completed, start to read data\n");
 
    for (j = 0; j < 4; j++) {
        for (i = 0; i < 7; i++) {
            rx[i] = *(volatile u16 *)(AUXADC_TS_RAW_X_DAT0 + 0x04 * i);
            ry[i] = *(volatile u16 *)(AUXADC_TS_RAW_Y_DAT0 + 0x04 * i);
            
            printk("[ts_udvt]: AUXADC_TS_RAW_X_DAT0  = %4d, addr = 0x%04x\n", rx[i], (AUXADC_TS_RAW_X_DAT0 + 0x04 * i));
            printk("[ts_udvt]: AUXADC_TS_RAW_Y_DAT0  = %4d, addr = 0x%04x\n", ry[i], (AUXADC_TS_RAW_Y_DAT0 + 0x04 * i));            
        }
        rz1[j] = *(volatile u16 *)(AUXADC_TS_RAW_Z1_DAT0 + 0x04 * j);
        rz2[j] = *(volatile u16 *)(AUXADC_TS_RAW_Z2_DAT0 + 0x04 * j);
        
        printk("[ts_udvt]: AUXADC_TS_RAW_Z1_DAT0 = %4d, addr = 0x%04x\n", rz1[j], (AUXADC_TS_RAW_Z1_DAT0 + 0x04 * j));
        printk("[ts_udvt]: AUXADC_TS_RAW_Z2_DAT0 = %4d, addr = 0x%04x\n", rz2[j], (AUXADC_TS_RAW_Z2_DAT0 + 0x04 * j));       
    }
    
    //*(volatile u16 *)AUXADC_TS_RAW_CON = 0x0004;// RAW_BATCH_EN
    
    for (i = 0; i < 7; i++) {
        x = x + rx[i];
        y = y + ry[i];
    }
    
    for (i = 0; i < 4; i++) {
        z1 = z1 + rz1[i];
        z2 = z2 + rz2[i];
    }
    
    x = x / 7; y = y / 7; z1 = z1 / 4; z2 = z2 / 4;
    
    printk("[ts_udvt]: x = %4d, y = %4d, z1 = %4d, z2 = %4d\n", x, y, z1, z2);
    
    if (touch_interrupt && x != 0 && y != 4095) {   
        mod_timer(&ts_udvt_timer, jiffies + 20);
    } else {
        touch_interrupt = 0;
        //mod_timer(&ts_udvt_timer, jiffies + 1000);
    }
    
}

/* timer keep polling touch panel status */
void ts_udvt_timer_fn(unsigned long arg) 
{
    if (touch_log_en) printk("[ts_udvt]: timer trigger\n");
    
    if ((*(volatile u16 *)AUXADC_TS_RAW_CON) & 0x0004) {
        if(ts_udvt_auto_sample_tasklet.state != TASKLET_STATE_RUN)
            tasklet_hi_schedule(&ts_udvt_auto_sample_tasklet);
    } else{
        if (ts_udvt_tasklet.state != TASKLET_STATE_RUN)
            tasklet_hi_schedule(&ts_udvt_tasklet);
    }
}

/* handle interrupt from touch panel */
irqreturn_t ts_udvt_handler(int irq, void *dev_id)
{
    if (touch_log_en) printk("[ts_udvt]: interrupt trigger\n");
    
    touch_interrupt = 1;
       
    if ((*(volatile u16 *)AUXADC_TS_RAW_CON) & 0x0004) {
#if 0     	
        mt65xx_irq_mask(MT65XX_TOUCH_IRQ_LINE);
        if(ts_udvt_auto_sample_tasklet.state != TASKLET_STATE_RUN)
            tasklet_hi_schedule(&ts_udvt_auto_sample_tasklet);
        mt65xx_irq_unmask(MT65XX_TOUCH_IRQ_LINE);
#endif        
    }

	else {    
        if(ts_udvt_tasklet.state != TASKLET_STATE_RUN)
            tasklet_hi_schedule(&ts_udvt_tasklet);
    }
    return IRQ_HANDLED;
} 
  
/* handle touch panel interrupt */
void ts_udvt_tasklet_fn(unsigned long unused) 
{
    int x, y, z1, z2;
    
    if (!touch_interrupt) return;
     
    x  = ts_udvt_read(AUXADC_TS_POSITION_X);
    y  = ts_udvt_read(AUXADC_TS_POSITION_Y);
    z1 = ts_udvt_read(AUXADC_TS_POSITION_Z1);
    z2 = ts_udvt_read(AUXADC_TS_POSITION_Z2);    
     
    if (touch_log_en) printk("[ts_udvt]: x = %4d, y = %4d, z1 = %4d, z2 = %4d\n", x, y, z1, z2); 
    
    if (touch_interrupt && x != 0 && y != 4095) {   
        mod_timer(&ts_udvt_timer, jiffies + 5);
    } else {
        touch_interrupt = 0;
        //mod_timer(&ts_udvt_timer, jiffies + 1000);
    }
     
    return;
}

static long ts_udvt_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *uarg = (void __user *)arg;
    
    struct udvt_cmd *pcmd = (struct udvt_cmd *)arg;
    
    //printk("cmd:%d, value:0x%x\n", pcmd->cmd, pcmd->value);
    
    switch(pcmd->cmd) {
        case SET_AUXADC_TS_DEBT0: 
            *(volatile u32 *)AUXADC_TS_DEBT0 = (u32)(pcmd->value & 0xFFFF);
            break;           
		case SET_AUXADC_TS_DEBT1: 
			*(volatile u32 *)AUXADC_TS_DEBT1 = (u32)(pcmd->value & 0xFFFF);
			break;	   
        case SET_AUXADC_TS_CMD:    
            *(volatile u32 *)AUXADC_TS_CMD = (u32)(pcmd->value & 0xFFFF);      
            break;
        case SET_AUXADC_TS_CON:              
            *(volatile u32 *)AUXADC_CON3 = (u32)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_TS_DAT0:             
            *(volatile u32 *)AUXADC_TS_DATA0 = (u32)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_TS_AUTO_CON:                              
            *(volatile u32 *)AUXADC_TS_RAW_CON = (u32)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_TS_AUTO_COUNT:                               
            *(volatile u32 *)AUXADC_TS_AUTO_TIME_INTVL = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_TS_AUTO_X_DAT0:                            
            *(volatile u32 *)AUXADC_TS_RAW_X_DAT0 = (u32)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_TS_AUTO_Y_DAT0:                                
            *(volatile u32 *)AUXADC_TS_RAW_Y_DAT0 = (u32)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_TS_AUTO_Z1_DAT0:                                
            *(volatile u32 *)AUXADC_TS_RAW_Z1_DAT0 = (u32)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_TS_AUTO_Z2_DAT0:                               
            *(volatile u32 *)AUXADC_TS_RAW_Z2_DAT0 = (u32)(pcmd->value & 0xFFFF);
            break;
        case GET_AUXADC_TS_DEBT0:  
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_DEBT0) & 0xFFFF);
            break;          
        case GET_AUXADC_TS_DEBT1:    
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_DEBT1) & 0xFFFF);       
            break;
        case GET_AUXADC_TS_CMD:    
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_CMD) & 0xFFFF);       
            break;		
        case GET_AUXADC_TS_CON:              
            pcmd->value = ((*(volatile u32 *)AUXADC_CON3) & 0xFFFF);
            break;
        case GET_AUXADC_TS_DAT0:            
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_DATA0) & 0xFFFF);
            break;
        case GET_AUXADC_TS_AUTO_CON:                              
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_RAW_CON) & 0xFFFF);
            break;
        case GET_AUXADC_TS_AUTO_COUNT:                               
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_AUTO_TIME_INTVL) & 0xFFFF);
            break;
        case GET_AUXADC_TS_AUTO_X_DAT0:                               
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_RAW_X_DAT0) & 0xFFFF);
            break;
        case GET_AUXADC_TS_AUTO_Y_DAT0:                             
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_RAW_Y_DAT0) & 0xFFFF);
            break;
        case GET_AUXADC_TS_AUTO_Z1_DAT0:                              
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_RAW_Z1_DAT0) & 0xFFFF);
            break;
        case GET_AUXADC_TS_AUTO_Z2_DAT0:      
            pcmd->value = ((*(volatile u32 *)AUXADC_TS_RAW_Z2_DAT0) & 0xFFFF);            
            break;
		case GET_REG_DEFAULT:
			dump_AUXADC_regs();
			break;
		case SET_RESISTANCE:
			set_touch_resistance(pcmd->value);
			break;
		case SET_S_D_MODE:
			if(pcmd->value == 0)
				set_ts_cmd_reg(0, 0, 0);
			else
				set_ts_cmd_reg(1, 0, 0);
			break;
		case SET_BIT_MODE:
			if(pcmd->value == 0)
				set_ts_cmd_reg(0, 1, 0);
			else
				set_ts_cmd_reg(1, 1, 0);
			break;	
		case SET_RAW_ABORT:
			if(pcmd->value == 0) //start
				(*(volatile u32 *)AUXADC_TS_RAW_CON) = (((*(volatile u32 *)AUXADC_TS_RAW_CON))&0xfffe);
			else //abort
				(*(volatile u32 *)AUXADC_TS_RAW_CON) = (((*(volatile u32 *)AUXADC_TS_RAW_CON))|0x1);
			break;
		case SET_FAV_NOISE:
			if(pcmd->value == 1) //enable
				(*(volatile u32 *)AUXADC_TS_CON1) = (((*(volatile u32 *)AUXADC_TS_CON1))|(1<<FAV_ASAMP));
			else //disable
				(*(volatile u32 *)AUXADC_TS_CON1) = (((*(volatile u32 *)AUXADC_TS_CON1))&(~(1<<FAV_ASAMP)));
			break;
		case SET_FAV_ACC_MODE:
			if(pcmd->value == 1)// 1 times
			{
				(*(volatile u32 *)AUXADC_TS_CON1) = ((((*(volatile u32 *)AUXADC_TS_CON1))&0xfffc));
			}
			else if(pcmd->value == 4) // 4 times
			{
				(*(volatile u32 *)AUXADC_TS_CON1) = ((((*(volatile u32 *)AUXADC_TS_CON1))&0xfffc)|0x01);
			}
			else if(pcmd->value == 8) // 8 times
			{
				(*(volatile u32 *)AUXADC_TS_CON1) = ((((*(volatile u32 *)AUXADC_TS_CON1))&0xfffc)|0x02);
			}
			else if(pcmd->value == 16) // 16 times
			{
				(*(volatile u32 *)AUXADC_TS_CON1) = ((((*(volatile u32 *)AUXADC_TS_CON1))&0xfffc)|0x03);
			}
			break;
		case SET_FAV_LATENCY:
			(*(volatile u32 *)AUXADC_TS_CON1) = (((*(volatile u32 *)AUXADC_TS_CON1)&0x00FF)|(pcmd->value<<FAV_ADEL_BIT));
			break;
		case SET_FAV_INTR_MODE:
			if(pcmd->value == 0) {//software mode
				(*(volatile u32 *)AUXADC_TS_CON1) = (((*(volatile u32 *)AUXADC_TS_CON1))&(~(1<<FAV_SEL)));
			}
			if(pcmd->value == 1) {
				(*(volatile u32 *)AUXADC_TS_CON1) = (((*(volatile u32 *)AUXADC_TS_CON1))|((1<<FAV_SEL)));
				(*(volatile u32 *)AUXADC_MISC) = (((*(volatile u32 *)AUXADC_MISC))&(~(1<<8)));	// disale Misc
			}
			break;
		case SET_FAV_MODE:		
			if(pcmd->value == 1) // enable
					(*(volatile u32 *)AUXADC_MISC) = (((*(volatile u32 *)AUXADC_MISC))|((1<<8)));
			else
				(*(volatile u32 *)AUXADC_MISC) = (((*(volatile u32 *)AUXADC_MISC))&(~(1<<8)));		
			break;
		case SET_FAV_SAMPLING_MODE:
			tpd_fav_coord_sel(pcmd->value);
			break;
        case ENABLE_TOUCH_LOG:
            touch_log_en = 1;
            tpd_enable_em_log(1);
            break;
        case DISABLE_TOUCH_LOG:
            touch_log_en = 0;
            tpd_enable_em_log(0);
            break;
		case SET_TS_WAKE_SRC:
#ifdef CONFIG_MTK_LDVT
    		sc_set_wake_src(WAKE_SRC_TS,MT65XX_TOUCH_IRQ_LINE);		
#else
			printk("sc_set_wake_src is only available in LDVT load\n");
#endif
    		break;
    	case SET_SAMPLE_ADJUST:
	        if(pcmd->value == 0)
	    			(*(volatile u32 *)AUXADC_TS_CON2) = 0x01;
	    		else
	    			(*(volatile u32 *)AUXADC_TS_CON2) = (0x100|pcmd->value);	
    		printk("AUXADC_TS_CON2: 0x%x\n", *(volatile u32 *)AUXADC_TS_CON2);	
    		break;
    	case SEL_MODE_SW:
    		tpd_mode_select(0x01);
    		break;
    	case SEL_MODE_FAV_SW:
    		tpd_mode_select(0x02);
    		break;
    	case SEL_MODE_FAV_HW:
    		tpd_mode_select(0x04);
    		break;
    	case SEL_MODE_RAW_DATA:
    		tpd_mode_select(0x08);
    		break;
        default:
            return -EINVAL;										
    }
    return 0;
}

static int ts_udvt_dev_open(struct inode *inode, struct file *file)
{
	//if(hwEnableClock(MT65XX_PDN_PERI_TP,"Touch")==FALSE)
	//     printk("hwEnableClock TP failed.\n");
    return 0;
}

static struct file_operations ts_udvt_dev_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = ts_udvt_dev_ioctl,
    .open           = ts_udvt_dev_open,
};

static struct miscdevice ts_udvt_dev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = TS_NAME,
    .fops   = &ts_udvt_dev_fops,
};

int ts_udvt_local_init(void) 
{
    init_timer(&ts_udvt_timer);
    ts_udvt_timer.function = ts_udvt_timer_fn;
    tasklet_init(&ts_udvt_tasklet, ts_udvt_tasklet_fn, 0);
    tasklet_init(&ts_udvt_auto_sample_tasklet, ts_udvt_auto_trigger_fn, 0);
	tasklet_init(&ts_fav_mode_tasklet, ts_udvt_FAV_fn, 0);
    mt65xx_irq_set_sens(MT65XX_TOUCH_IRQ_LINE, MT65xx_EDGE_SENSITIVE);    
	mt65xx_irq_set_polarity(MT65XX_TOUCH_IRQ_LINE, MT65xx_POLARITY_LOW);	
    if (request_irq(MT65XX_TOUCH_IRQ_LINE, ts_udvt_handler, 0, "_TS_UDVT", NULL))
        printk("[ts_udvt]: request_irq failed.\n");

	mt65xx_irq_set_sens(MT65XX_TOUCH_BATCH_LINE, MT65xx_EDGE_SENSITIVE);	  
	mt65xx_irq_set_polarity(MT65XX_TOUCH_BATCH_LINE, MT65xx_POLARITY_LOW);	
	if (request_irq(MT65XX_TOUCH_BATCH_LINE, ts_udvt_FAV_fn, 0, "_TS_UDVT", NULL))
		printk("[ts_udvt]: request_irq failed.\n");

    return 0;
}

static int __init ts_udvt_mod_init(void)
{
    int ret;
    ret = misc_register(&ts_udvt_dev);
    if (ret) {
        printk("[ts_udvt]: register driver failed (%d)\n", ret);
        return ret;
    }
    /* free Product driver irq */
	//free_irq(MT65XX_TOUCH_IRQ_LINE, NULL);
    //ts_udvt_local_init();
    
    printk("[ts_udvt]: ts_udvt_local_init initialization\n");
    return 0;
}

/* should never be called */
static void __exit ts_udvt_mod_exit(void)
{
    int ret;
    
    ret = misc_deregister(&ts_udvt_dev);
    if(ret){
        printk("[ts_udvt]: unregister driver failed\n");
    }
}

module_init(ts_udvt_mod_init);
module_exit(ts_udvt_mod_exit);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("MT6573 TS Driver for UDVT");
MODULE_LICENSE("GPL");
