#include <linux/init.h>
#include <linux/module.h> 
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>

#include <asm/uaccess.h>
#include <mach/irqs.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt6575_sc.h>

#include "adc.h"

#define ADC_DEBUG   1

#define ADC_NAME    "dvt-adc"

#if ADC_DEBUG
    #define adc_print(fmt, arg...)  printk("[adc_udvt]: " fmt, ##arg)
#else
    #define adc_print(fmt, arg...)  do {} while (0)
#endif

struct udvt_cmd {
    int cmd;
    int value;
};

static int adc_run = 0;
static int adc_mode = 0;
static int adc_log_en = 0;
static int adc_3g_tx_en = 0;
static int adc_2g_tx_en = 0;
static int adc_bg_detect_en = 0;

struct timer_list adc_udvt_timer;
struct tasklet_struct adc_udvt_tasklet;


/* handle adc timer trigger */
void adc_udvt_tasklet_fn(unsigned long unused) 
{
    unsigned int channel[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    unsigned int i = 0, data = 0;
    unsigned int poweron, poweroff;
    
    
    /* Initialize ADC control register */
    *(volatile u16 *)AUXADC_CON0 = 0;
    *(volatile u16 *)AUXADC_CON1 = 0;
    *(volatile u16 *)AUXADC_CON2 = 0;
    *(volatile u16 *)AUXADC_CON3 = 0;

#if 0 // Mt6575 do not test 
    if (adc_3g_tx_en) 
    {        
        // 3G power on is controlled by APMCU_CG_CLR0
        *(volatile u16 *)APMCU_CG_CLR0 = 1 << 13;

        for (i = 0; i < 9; i++)
        {
            *(volatile u16 *)AUXADC_TXPWR_CH = 0;
            
            *(volatile u16 *)AUXADC_TXPWR_CH = i;
			
            data = *(volatile u16 *)AUXADC_TXPWR_CH;
            *(volatile u16 *)AUXADC_TXPWR_CH = 0x8000 | data;			
    
            data = *(volatile u16 *)AUXADC_TXPWR_CH;
            *(volatile u16 *)AUXADC_TXPWR_CH = 0x8000 | 0x4000 | data;
            
            data = *(volatile u16 *)AUXADC_TXPWR_CH;
            *(volatile u16 *)AUXADC_TXPWR_CH = 0x8000 | data;//0x4000 | data;
            
            // Delay for avoid reading invalid status bit
            mdelay(1000);  
            
            while ((*(volatile u16 *)AUXADC_CON3) & 0x01) {
                printk("[adc_udvt]: wait for module idle 1\n");
                mdelay(300);
        	  }  
            
            while(((*(volatile u16 *)(AUXADC_DAT0 + 4 * i)) & 0x1000) == 0) {
                printk("[adc_udvt]: wait for module idle 2\n");
                *(volatile u16 *)AUXADC_TXPWR_CH = 0;
            
                *(volatile u16 *)AUXADC_TXPWR_CH = i;
        
            data = *(volatile u16 *)AUXADC_TXPWR_CH;
            *(volatile u16 *)AUXADC_TXPWR_CH = 0x8000 | data;			
    
            data = *(volatile u16 *)AUXADC_TXPWR_CH;
            *(volatile u16 *)AUXADC_TXPWR_CH = 0x8000 | 0x4000 | data;
            
            data = *(volatile u16 *)AUXADC_TXPWR_CH;
            *(volatile u16 *)AUXADC_TXPWR_CH = 0x8000 | data;//0x4000 | data;
                
                // Delay for avoid reading invalid status bit
                mdelay(1000);  
                
                while ((*(volatile u16 *)AUXADC_CON3) & 0x01) {
                    printk("[adc_udvt]: wait for module idle 1\n");
                    mdelay(300);
                }  
        	  }  
   		  
            data = *(volatile u16 *) (AUXADC_DAT0 + 4 * i);
            
            printk("[adc_udvt]: 3G Tx Power => channel[%d] = %d.%d\n", i, (data * 250 / 4096 / 100), ((data * 250 / 4096) % 100));
       	}
       	
       	// 3G power on is controlled by APMCU_CG_SET0
        *(volatile u16 *)APMCU_CG_SET0 = 1 << 13;
    }
    
    if (adc_2g_tx_en) 
    {
        // 2G power on is controlled by APMCU_CG_CLR0
        *(volatile u16 *)APMCU_CG_CLR0 = 1 << 4;
        //*(volatile u16 *)APMCU_CG_CLR0 = 1 << 6;
        
        //*(volatile u16 *)MDMCU_CG_CON0 = ~(0x1 << 7); 
    
        *(volatile u16 *)AUXADC_2GTX_CH = 0;
        
        *(volatile u16 *)AUXADC_2GTX_CH = 6;

        data = *(volatile u16 *)AUXADC_2GTX_CH;
        *(volatile u16 *)AUXADC_2GTX_CH = 0x8000 | data;

        data = *(volatile u16 *)AUXADC_2GTX_CH;
        *(volatile u16 *)AUXADC_2GTX_CH = 0xC000 | data;// 0x4000 | data;
        
        // Delay for avoid reading invalid status bit
        mdelay(1000);  
        
        while ((*(volatile u16 *)AUXADC_CON3) & 0x01) {
            printk("[adc_udvt]: wait for module idle\n");
            mdelay(300);
    	  }  
        
        data = *(volatile u16 *)AUXADC_2GTX_DAT0 & 0x0FFF;
        printk("[adc_udvt]: 2G Tx Power => channel[6] = %d.%d, ", (data * 250 / 4096 / 100), ((data * 250 / 4096) % 100));
        
        data = *(volatile u16 *)AUXADC_2GTX_DAT0 & 0x1000;
        printk("pended = %d\n", (data >> 12));
        
        // 2G power on is controlled by APMCU_CG_SET0
        *(volatile u16 *)APMCU_CG_SET0 = 1 << 4;
        //*(volatile u16 *)APMCU_CG_SET0 = 1 << 6;
        
        //*(volatile u16 *)MDMCU_CG_CON0 = (0x1 << 7); 
    }
         
#endif          
    if (!adc_mode) {
        *(volatile u16 *)AUXADC_CON1 = 0x1FFF; 

        mdelay(1000);

        // Polling until bit STA = 0
        while ((*(volatile u16 *)AUXADC_CON3) & 0x01) {
            printk("[adc_udvt]: wait for module idle\n");
            mdelay(300);
    	  }         
        
        for (i = 0; i < 14; i++) {
            channel[i] = (*(volatile u16 *)(AUXADC_DAT0 + i * 0x04)) & 0x0FFF;
	    			printk("[adc_udvt: imm mode raw data => channel[%d] = %d\n",i, channel[i]);
            printk("[adc_udvt]: imm mode => channel[%d] = %d.%d\n", i, (channel[i] * 250 / 4096 / 100), ((data * 250 / 4096) % 100));
        } 
    } else {
#if 0 // timer-trigger have been canceled on Mt6575   	
        *(volatile u16 *)TDMA_AUXEV1 = 0x0050;
        
        *(volatile u16 *)AUXADC_CON3 = 0x0200;
        *(volatile u16 *)AUXADC_CON0 = 0x0100;
        
        for (i = 0; i < 1000; i++ );    

        mdelay(1000);
        
        // Polling until bit STA = 0       
        while ((*(volatile u16 *)AUXADC_CON0) & 0x0100) {
            printk("[adc_udvt]: wait for module idle\n");
            mdelay(1000);
        }
        	
        for (i = 8; i < 9; i++) {
            channel[i] = (*(volatile u16 *)(AUXADC_DAT0 + i * 0x04)) & 0x0FFF;
	    printk("[adc_udvt]: syn mode raw data => channel[%d]=%d\n",i, channel[i]);
            printk("[adc_udvt]: syn mode => channel[%d] = %d.%d\n", i, (channel[i] * 250 / 4096 / 100), ((data * 250 / 4096) % 100));
        }  
        
        *(volatile u16 *)TDMA_AUXEV0 = 0x0000;  
#endif         
    }
    
    if (!adc_mode) {
        *(volatile u16 *)AUXADC_CON1 = 0x0000;
    } else {
        *(volatile u16 *)AUXADC_CON0 = 0x0000;
    }
    
    mod_timer(&adc_udvt_timer, jiffies + 500);
   
    return;
}

/* timer keep polling touch panel status */
void adc_udvt_timer_fn(unsigned long arg) 
{
    if (!adc_run) {
        mod_timer(&adc_udvt_timer, jiffies + 500);
        return;
    }
    
    if (adc_log_en) printk("[adc_udvt]: timer trigger\n");
    
    if (adc_udvt_tasklet.state != TASKLET_STATE_RUN)
            tasklet_hi_schedule(&adc_udvt_tasklet);
}

/* handle interrupt from adc */
irqreturn_t adc_udvt_handler(int irq, void *dev_id)
{
    if (adc_log_en && adc_bg_detect_en) printk("[adc_udvt]: interrupt trigger\n");
    
    return IRQ_HANDLED;
} 

static long adc_udvt_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *uarg = (void __user *)arg;
    
    struct udvt_cmd *pcmd = (struct udvt_cmd *)arg;
    
    //printk("cmd:%d, value:0x%x\n", pcmd->cmd, pcmd->value);

    switch(pcmd->cmd) {
        case SET_AUXADC_CON0: 
            *(volatile u16 *)AUXADC_CON0 = (u16)(pcmd->value & 0xFFFF);
            break;           
        case SET_AUXADC_CON1:    
            *(volatile u16 *)AUXADC_CON1 = (u16)(pcmd->value & 0xFFFF);      
            break;
        case SET_AUXADC_CON2:              
            *(volatile u16 *)AUXADC_CON2 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_CON3:             
            *(volatile u16 *)AUXADC_CON3 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT0:                              
            *(volatile u16 *)AUXADC_DAT0 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT1:                               
            *(volatile u16 *)AUXADC_DAT1 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT2:                            
            *(volatile u16 *)AUXADC_DAT2 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT3:                                
            *(volatile u16 *)AUXADC_DAT3 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT4:                                
            *(volatile u16 *)AUXADC_DAT4 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT5:                               
            *(volatile u16 *)AUXADC_DAT5 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT6:                               
            *(volatile u16 *)AUXADC_DAT6 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT7:                               
            *(volatile u16 *)AUXADC_DAT7 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT8:                               
            *(volatile u16 *)AUXADC_DAT8 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT9:                               
            *(volatile u16 *)AUXADC_DAT9 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT10:                               
            *(volatile u16 *)AUXADC_DAT10 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT11:                               
            *(volatile u16 *)AUXADC_DAT11 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT12:                               
            *(volatile u16 *)AUXADC_DAT12 = (u16)(pcmd->value & 0xFFFF);
            break;
        case SET_AUXADC_DAT13:                               
            *(volatile u16 *)AUXADC_DAT13 = (u16)(pcmd->value & 0xFFFF);
            break;
        case GET_AUXADC_CON0:  
            pcmd->value = ((*(volatile u16 *)AUXADC_CON0) & 0xFFFF);
            break;          
        case GET_AUXADC_CON1:    
            pcmd->value = ((*(volatile u16 *)AUXADC_CON1) & 0xFFFF);       
            break;
        case GET_AUXADC_CON2:              
            pcmd->value = ((*(volatile u16 *)AUXADC_CON2) & 0xFFFF);
            break;
        case GET_AUXADC_CON3:            
            pcmd->value = ((*(volatile u16 *)AUXADC_CON3) & 0xFFFF);
            break;
        case GET_AUXADC_DAT0:                              
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT0) & 0xFFFF);
            break;
        case GET_AUXADC_DAT1:                               
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT1) & 0xFFFF);
            break;
        case GET_AUXADC_DAT2:                               
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT2) & 0xFFFF);
            break;
        case GET_AUXADC_DAT3:                             
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT3) & 0xFFFF);
            break;
        case GET_AUXADC_DAT4:                              
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT4) & 0xFFFF);
            break;
        case GET_AUXADC_DAT5:      
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT5) & 0xFFFF);            
            break;
        case GET_AUXADC_DAT6:
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT6) & 0xFFFF);     
            break;
        case GET_AUXADC_DAT7:
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT7) & 0xFFFF);     
            break; 
        case GET_AUXADC_DAT8:
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT8) & 0xFFFF);     
            break; 
        case GET_AUXADC_DAT9:
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT9) & 0xFFFF);     
            break; 
        case GET_AUXADC_DAT10:
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT10) & 0xFFFF);     
            break; 
        case GET_AUXADC_DAT11:
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT11) & 0xFFFF);     
            break; 
        case GET_AUXADC_DAT12:
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT12) & 0xFFFF);     
            break; 
        case GET_AUXADC_DAT13:
            pcmd->value = ((*(volatile u16 *)AUXADC_DAT13) & 0xFFFF);     
            break;
        case ENABLE_SYN_MODE:
            adc_mode = 1;
            break;
        case DISABLE_SYN_MODE:
            adc_mode = 0;
            break; 
        case ENABLE_ADC_RUN:
            adc_run = 1;
            break;
        case DISABLE_ADC_RUN:
            adc_run = 0;
            break;    
        case ENABLE_ADC_LOG:
            adc_log_en = 1;
            break;
        case DISABLE_ADC_LOG:
            adc_log_en = 0;
            break;
        case ENABLE_BG_DETECT:
            adc_bg_detect_en = 1;
            break;
        case DISABLE_BG_DETECT:
            adc_bg_detect_en = 0;
            break;
        case ENABLE_3G_TX:
            adc_3g_tx_en = 1;
            break;
        case DISABLE_3G_TX:
            adc_3g_tx_en = 0;
            break;            
        case ENABLE_2G_TX:
            adc_2g_tx_en = 1;
            break;
        case DISABLE_2G_TX:
            adc_2g_tx_en = 0;
            break; 
				case SET_ADC_WAKE_SRC:
	    		sc_set_wake_src(WAKE_SRC_LOW_BAT, MT65XX_IRQ_LOWBAT_LINE);		
	    		break;
	      case SET_DET_VOLT:
	      	(*(volatile u16 *)AUXADC_DET_VOLT) = pcmd->value;
	      	printk("AUXADC_DET_VOLT: 0x%x\n", (*(volatile u16 *)AUXADC_DET_VOLT));
	      	break;
	      case SET_DET_PERIOD:
	      	(*(volatile u16 *)AUXADC_DET_PERIOD) = pcmd->value;
	      	printk("AUXADC_DET_PERIOD: 0x%x\n", (*(volatile u16 *)AUXADC_DET_PERIOD));	      	
	      	break;
	      case SET_DET_DEBT:
	      	(*(volatile u16 *)AUXADC_DET_DEBT) = pcmd->value;
	      	printk("AUXADC_DET_DEBT: 0x%x\n", (*(volatile u16 *)AUXADC_DET_DEBT));	      	
	      	break;	      		      	
        default:
            return -EINVAL;										
    }
    return 0;
}

static int adc_udvt_dev_open(struct inode *inode, struct file *file)
{
    //if(hwEnableClock(MT65XX_PDN_PERI_TP,"Touch")==FALSE)
	 //printk("hwEnableClock TP failed.\n");   
    mod_timer(&adc_udvt_timer, jiffies + 500);
    return 0;
}

static struct file_operations adc_udvt_dev_fops = {
    .owner          = THIS_MODULE,
    .unlocked_ioctl = adc_udvt_dev_ioctl,
    .open           = adc_udvt_dev_open,
};

static struct miscdevice adc_udvt_dev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = ADC_NAME,
    .fops   = &adc_udvt_dev_fops,
};

int adc_udvt_local_init(void) 
{
    init_timer(&adc_udvt_timer);
    adc_udvt_timer.function = adc_udvt_timer_fn;
    tasklet_init(&adc_udvt_tasklet, adc_udvt_tasklet_fn, 0);  
    mt65xx_irq_set_sens(MT65XX_IRQ_LOWBAT_LINE, MT65xx_EDGE_SENSITIVE);	  
	  mt65xx_irq_set_polarity(MT65XX_IRQ_LOWBAT_LINE, MT65xx_POLARITY_LOW);
    if (request_irq(MT65XX_IRQ_LOWBAT_LINE, adc_udvt_handler, 0, "_ADC_UDVT", NULL))
        printk("[adc_udvt]: request_irq failed.\n");   
   
    // Change the period to 0 to disable background detection before we change the configuration
    *(volatile u16 *)AUXADC_DET_PERIOD = 0;
    
    // Set the Debounce Time
    *(volatile u16 *)AUXADC_DET_DEBT = 0x20;
	 
    // set the voltage for background detection(bit 15 set higher or lower)
    //*(volatile u16 *)AUXADC_DET_VOLT = 0x000F;
    *(volatile u16 *)AUXADC_DET_VOLT = 0x83FF;
  
    // Select the Channel
    //*(volatile u16 *)AUXADC_DET_SEL =0x6;// 0x8;
     *(volatile u16 *)AUXADC_DET_SEL =0x0;// 0x8;
    
    // Change the Period
    *(volatile u16 *)AUXADC_DET_PERIOD = 0xA0;
	   
    return 0;
}

static int __init adc_udvt_mod_init(void)
{
    int ret;
    
    ret = misc_register(&adc_udvt_dev);
    if (ret) {
        printk("[adc_udvt]: register driver failed (%d)\n", ret);
        return ret;
    }
    
    adc_udvt_local_init();
    
    printk("[adc_udvt]: adc_udvt_local_init initialization\n");
    return 0;
}

static void __exit adc_udvt_mod_exit(void)
{
    int ret;
    
    ret = misc_deregister(&adc_udvt_dev);
    if(ret){
        printk("[ts_udvt]: unregister driver failed\n");
    }
}

module_init(adc_udvt_mod_init);
module_exit(adc_udvt_mod_exit);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("MT6573 TS Driver for UDVT");
MODULE_LICENSE("GPL");


