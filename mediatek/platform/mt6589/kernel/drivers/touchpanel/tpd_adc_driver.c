

#include "tpd_custom_generic.h"
#include "tpd.h"
#include <linux/delay.h>
#include "tpd_adc.h"
#include "mach/mt_clkmgr.h"
#include <linux/interrupt.h>

//#define MT6585_FPGA
extern struct tpd_device *tpd;
extern int tpd_register_flag;

int tpd_intr_status = 0;
int tpd_suspend_flag = 0;

int tpd_event_status = 0; // default up

static int raw_x, raw_y, pre_x = 0, pre_y = 0;
static atomic_t count_down;
static struct timeval d_time;
static struct timeval first_report;
//static spinlock_t g_tpd_lock = SPIN_LOCK_UNLOCKED;
//static unsigned long g_tpd_flags;

int tpd_sample_mode = FAV_MODE_HW; // default sample mode

/* internal function definition */
static void tpd_timer_fn(unsigned long);
static void tpd_tasklet(unsigned long unused);
irqreturn_t tpd_handler_penirq(int, void *);

irqreturn_t tpd_handler_batchirq(int, void *);

static void tpd_down(int cx, int cy, int cd, int cp);
static void tpd_up(int cx, int cy, int cd, int cp);
static int tpd_mode_select(int mode);
//extern void mt6577_irq_mask(unsigned int line);
//extern void mt6577_irq_unmask(unsigned int line);
//extern bool hwEnableClock(MT65XX_CLOCK clockId, char *mode_name);
//extern bool hwDisableClock(MT65XX_CLOCK clockId, char *mode_name);

static void tpd_para_adjust(void)
{
	if(tpd_mode == 0)
		return;
	
	switch(tpd_mode) {
		case TPD_MODE_SW:
			tpd_mode_select(TPD_SW);
			break;
		case TPD_MODE_FAV_SW:
			tpd_mode_select(FAV_MODE_SW);
			break;
		case TPD_MODE_FAV_HW:
			tpd_mode_select(FAV_MODE_HW);
			break;
		case TPD_MODE_RAW_DATA:
			tpd_mode_select(RAW_DATA_MODE);
			break;
		default:
			tpd_mode_select(-1);
	}
		
    tpd_set_debounce_time_DEBT0(tpd_em_debounce_time0);
	TPD_DMESG("DEBT0: %d ms\n", tpd_get_debounce_time_DEBT0());
	
    tpd_set_debounce_time_DEBT1(tpd_em_debounce_time1);
	TPD_DMESG("DEBT1: %d ms\n", tpd_get_debounce_time_DEBT1());

    tpd_set_spl_number(tpd_em_spl_num);
	TPD_DMESG("spl number: %d\n", tpd_get_spl_number());

	tpd_fav_set_auto_interval(tpd_em_auto_time_interval);
	TPD_DMESG("auto_time_interval: %d ms\n", tpd_fav_get_auto_interval());

	tpd_fav_config(1, tpd_em_sample_cnt, tpd_em_asamp, 255);
	TPD_DMESG("tpd_em_sample_cnt: %d \n", tpd_get_sample_cnt());
	TPD_DMESG("tpd_get_asamp: %d \n", tpd_get_asamp());
	tpd_mode = 0;

}


/* invoke by tpd_local_init, initialize r-type touchpanel */
int tpd_driver_local_init(void) {
    // only for bring up mt6577
#if 0
    if(enable_clock(MT65XX_PDN_PERI_AUXADC,"Touch")==FALSE)
        TPD_DMESG("hwEnableClock TP failed.");
#endif
    tpd_adc_init();

	init_timer(&(tpd->timer));
	tpd->timer.function = tpd_timer_fn;

	atomic_set(&count_down, 0);
    tasklet_init(&(tpd->tasklet), tpd_tasklet, 0);
   // mt65xx_irq_set_sens(MT65XX_TOUCH_IRQ_LINE, MT65xx_EDGE_SENSITIVE);
    //mt65xx_irq_set_polarity(MT65XX_TOUCH_IRQ_LINE, MT65xx_POLARITY_LOW);
    if(request_irq(MT65XX_TOUCH_IRQ_LINE, (irq_handler_t)tpd_handler_penirq, IRQF_TRIGGER_FALLING, "mtk_tpd_penirq", NULL))
    {
      TPD_DMESG("request_irq failed.MT65XX_TOUCH_IRQ_LINE\n");
      return -1;
     }
    //mt65xx_irq_set_sens(MT65XX_TOUCH_BATCH_LINE, MT65xx_EDGE_SENSITIVE);	  
	//mt65xx_irq_set_polarity(MT65XX_TOUCH_BATCH_LINE, MT65xx_POLARITY_LOW);	
	if (request_irq(MT65XX_TOUCH_BATCH_LINE, tpd_handler_batchirq, IRQF_TRIGGER_FALLING, "mtk_tpd_favirq", NULL))
	{
		TPD_DMESG("request_irq failed.MT65XX_TOUCH_BATCH_LINE\n");
		return -1;
	}    
    return 0;
}

/* timer keep polling touch panel status */
void tpd_timer_fn(unsigned long arg) {
    TPD_EM_PRINT(0, 0, 0, 0, 0, TPD_TYPE_TIMER);
    if(tpd->tasklet.state != TASKLET_STATE_RUN)
        tasklet_hi_schedule(&(tpd->tasklet));
}

int tpd_mode_select(int mode)
{
	TPD_DMESG("mode=");
	switch(mode) {
		case TPD_SW:
			printk("TPD_SW\n");
			break;
		case FAV_MODE_SW:
			printk("FAV_MODE_SW\n");
			break;
		case FAV_MODE_HW:
			printk("FAV_MODE_HW\n");
			break;
		case RAW_DATA_MODE:
			printk("RAW_DATA_MODE\n");
			break;
		default:
			printk("invalid!!! tpd_sample_mode=%d\n", tpd_sample_mode);
			return -EINVAL;
	}
	tpd_sample_mode = mode;
	return 0;

}
EXPORT_SYMBOL(tpd_mode_select);

/* handle interrupt from touch panel (by dispatching to tasklet */
irqreturn_t tpd_handler_penirq(int irq, void *dev_id) 
{
	disable_irq_nosync(MT65XX_TOUCH_IRQ_LINE);
	if(__raw_readw(AUXADC_TP_CON0)&(1<<1)) // touch
	{	
		tpd_para_adjust();
	
		if(tpd_sample_mode&FAV_MODE_SW || tpd_sample_mode&FAV_MODE_HW || tpd_sample_mode&RAW_DATA_MODE) {
			tpd_fav_switch(1);
		}
		tpd_event_status = 1;
		pre_x = 0;
        pre_y = 0;	
        TPD_EM_PRINT(0, 0, 0, 0, 0, TPD_TYPE_INT_DOWN);
        
        tpd_intr_status = 1;
		if(tpd_sample_mode&TPD_SW) {
		    if(tpd->tasklet.state != TASKLET_STATE_RUN)
		        tasklet_hi_schedule(&(tpd->tasklet));
		}
    }
    else {
		tpd_event_status = 0;
	#if 0	
		tpd_fav_switch(0);
		if (atomic_read(&count_down) != 0) {
			atomic_set(&count_down, 0);
            tpd_up(pre_x, pre_y, 0, 0);   
			pre_x = 0;
        	pre_y = 0;	

		}
	#endif	
        TPD_EM_PRINT(0, 0, 0, 0, 0, TPD_TYPE_INT_UP);
    }    
	enable_irq(MT65XX_TOUCH_IRQ_LINE);
	
    return IRQ_HANDLED;
}

irqreturn_t tpd_handler_batchirq(int irq, void *dev_id) 
{ 
	//TPD_DMESG("%s\n", __FUNCTION__);
	disable_irq_nosync(MT65XX_TOUCH_BATCH_LINE);
	if(tpd_sample_mode&FAV_MODE_SW) {
		if(__raw_readw(AUXADC_TP_CON1)&(1<<4))
		{
			tpd_fav_switch(1);
			enable_irq(MT65XX_TOUCH_BATCH_LINE);
			TPD_DMESG("Invalid flag\n");
			return IRQ_HANDLED;
		}
		//tpd_fav_switch(1);
	}
	
	if(tpd_sample_mode&FAV_MODE_HW || tpd_sample_mode&RAW_DATA_MODE) {	
		if(__raw_readw(AUXADC_TP_CON1)&(1<<4))
		{
			/* removed due to hardware bug */
			//tpd_clear_invalid_flag();
			//mt6577_irq_unmask(MT65XX_TOUCH_BATCH_LINE);
			//TPD_DMESG("Invalid flag\n");
			//return IRQ_HANDLED;
		}
		tpd_fav_switch(0);
		//__raw_writew(__raw_readw(AUXADC_TP_CON1)&(~(1<<2)), AUXADC_TP_CON1); // disable SEL, auto interval
		if(__raw_readw(AUXADC_TP_CON1)&(1<<7))
		{
			/* should never run here */
			TPD_DMESG("FAV EN bit is 1\n");
			enable_irq(MT65XX_TOUCH_BATCH_LINE);
			return IRQ_HANDLED;
		}
	}

    if(tpd->tasklet.state != TASKLET_STATE_RUN)
    {
        tasklet_hi_schedule(&(tpd->tasklet));
		return IRQ_HANDLED;
    }
	else
	{
		enable_irq(MT65XX_TOUCH_BATCH_LINE);
        return IRQ_HANDLED;
	}
}


/**************************** touch down / up *******************************/
static void tpd_down(int cx, int cy, int cd, int cp) {
    /*
    #ifdef TPD_HAVE_BUTTON
    if (cy >= TPD_BUTTON_HEIGHT) {
        tpd_button(cx, cy, 1);
        return;
    }
    if (cy < TPD_BUTTON_HEIGHT && tpd->btn_state && cp < TPD_PRESSURE_MAX) 
        tpd_button(cx, cy, 0);
    #endif
    */
    int p;
    p = cp;
    cp = (255 * (TPD_PRESSURE_MAX - cp)) / (TPD_PRESSURE_MAX - TPD_PRESSURE_MIN);
//    if(cx<TPD_RES_X/100) cx=TPD_RES_X/100;
//    else if(cx>(TPD_RES_X-TPD_RES_X/100)) cx=TPD_RES_X-TPD_RES_X/100;
//    if(cy<TPD_RES_Y/100) cy=TPD_RES_Y/100;
//    else if(cy>(TPD_RES_Y-TPD_RES_Y/100)) cy=TPD_RES_Y-TPD_RES_Y/100; 
	if(tpd && tpd->dev && tpd_register_flag==1) {
    input_report_abs(tpd->dev, ABS_X, cx);
    input_report_abs(tpd->dev, ABS_Y, cy);
    input_report_abs(tpd->dev, ABS_PRESSURE, cp);
    input_report_key(tpd->dev, BTN_TOUCH, 1);
    input_sync(tpd->dev);
    
    TPD_DEBUG_PRINT_DOWN;
	//TPD_EM_PRINT(raw_x, raw_y, cx, cy, cp, 1);
    TPD_EM_PRINT(raw_x, raw_y, cx, cy, p, 1);
  }  
    //printk(KERN_ERR "tpd, raw_x=%d, raw_y=%d, cx=%d, cy=%d, cp=%d, down\n", raw_x, raw_y, cx, cy, cp);
}
  
static void tpd_up(int cx, int cy, int cd, int cp) {
    int pending = 0;

    /*
    #ifdef TPD_HAVE_BUTTON
    if(tpd->btn_state) tpd_button(cx,cy,0);
    #endif
    */
    int p;
    p = cp;  
//    if(cx<TPD_RES_X/100) cx=TPD_RES_X/100;
//    else if(cx>(TPD_RES_X-TPD_RES_X/100)) cx=TPD_RES_X-TPD_RES_X/100;
//    if(cy<TPD_RES_Y/100) cy=TPD_RES_Y/100;
//    else if(cy>(TPD_RES_Y-TPD_RES_Y/100)) cy=TPD_RES_Y-TPD_RES_Y/100;
	if(tpd && tpd->dev && tpd_register_flag==1) {
    input_report_abs(tpd->dev, ABS_X, cx);
    input_report_abs(tpd->dev, ABS_Y, cy);
    input_report_abs(tpd->dev, ABS_PRESSURE, 0);
    input_report_key(tpd->dev, BTN_TOUCH, 0);
    input_sync(tpd->dev);
    
    TPD_DEBUG_PRINT_UP;
    TPD_EM_PRINT(raw_x, raw_y, cx, cy, p, 0);
  }
    //printk(KERN_ERR "tpd, raw_x=%d, raw_y=%d, cx=%d, cy=%d, cp=%d, up\n", raw_x, raw_y, cx, cy, cp);
}

/*************************** main event handler ******************************/
static int fav_sw_sample_count;
static int fav_sw_buf[4];

void tpd_fav_coord_sel(int coord)
{
	unsigned tscon1 = __raw_readw(AUXADC_TP_CON1);
	if(coord == 0 || coord == 1 || coord == 2) {
		__raw_writew((tscon1&(~(3<<5)))|(coord<<5), AUXADC_TP_CON1);
	} else {
		__raw_writew((tscon1&(~(3<<5)))|(1<<5), AUXADC_TP_CON1);
	}
	
	if(coord == 0) {
		__raw_writew(TP_CMD_ADDR_X, AUXADC_TP_ADDR);
	}
	fav_sw_sample_count=0;
	fav_sw_buf[0]=0;
	fav_sw_buf[1]=0;
	fav_sw_buf[2]=0;
	fav_sw_buf[3]=0;
}
EXPORT_SYMBOL(tpd_fav_coord_sel);

int tpd_sampling(int *cx, int *cy, int *cp, int *cd) {
    int i, x = 0, y = 0, p = 0, rx = 0, ry = 0, rz1 = 0, rz2 = 0;

	if(tpd_sample_mode&TPD_SW || tpd_sample_mode&FAV_MODE_HW) {
	    /* ignore the 1st raw data (TPD_SW mode)*/
	    rx  = tpd_read(TPD_X, 0);
	    ry  = tpd_read(TPD_Y, 0);
	    rz1 = tpd_read(TPD_Z1,0);
	    rz2 = tpd_read(TPD_Z2,0);
	}

	if(tpd_sample_mode&FAV_MODE_SW) {
		unsigned tscon1 = __raw_readw(AUXADC_TP_CON1);
		unsigned value = (tscon1>>5)&0x03;
		if(value == 0) {
			switch(fav_sw_sample_count) {
				case 0:
					fav_sw_buf[0]  = tpd_read(TPD_X, 0);
					__raw_writew(TP_CMD_ADDR_Y, AUXADC_TP_ADDR);
					fav_sw_sample_count=1;
					tpd_fav_switch(1);
					return 1;
				case 1:
					fav_sw_buf[1]  = tpd_read(TPD_X, 0);
					__raw_writew(TP_CMD_ADDR_Z1, AUXADC_TP_ADDR);
					fav_sw_sample_count=2;
					tpd_fav_switch(1);
					return 2;
				case 2:
					fav_sw_buf[2]  = tpd_read(TPD_X, 0);
					__raw_writew(TP_CMD_ADDR_Z2, AUXADC_TP_ADDR);
					fav_sw_sample_count=3;
					tpd_fav_switch(1);
					return 3;
				case 3:
					fav_sw_buf[3]  = tpd_read(TPD_X, 0);
					__raw_writew(TP_CMD_ADDR_X, AUXADC_TP_ADDR);
					fav_sw_sample_count=0;
					rx = fav_sw_buf[0];
					ry = fav_sw_buf[1];
					rz1 = fav_sw_buf[2];
					rz2 = fav_sw_buf[3];
					break;
				default:
					TPD_DMESG("invalid FAV_SW count!!!\n");
					return -1;
					
			}
		} else if(value == 1) {
			rx  = tpd_read(TPD_X, 0);
		    ry  = tpd_read(TPD_Y, 0);
		    rz1 = tpd_read(TPD_Z1,0);
		    rz2 = tpd_read(TPD_Z2,0);
		} else if(value == 2) {
			__raw_writew((tscon1&(~(3<<5)))|(3<<5), AUXADC_TP_CON1);
			fav_sw_buf[0]  = tpd_read(TPD_X, 0);
		    fav_sw_buf[1]  = tpd_read(TPD_Y, 0);
		    tpd_fav_switch(1);
		    return 1;
		} else if(value == 3) {
			__raw_writew((tscon1&(~(3<<5)))|(2<<5), AUXADC_TP_CON1);
			fav_sw_buf[2]  = tpd_read(TPD_X, 0);
		    fav_sw_buf[3]  = tpd_read(TPD_Y, 0);
		    rx = fav_sw_buf[0];
			ry = fav_sw_buf[1];
			rz1 = fav_sw_buf[2];
			rz2 = fav_sw_buf[3];
		}
	}
	
 	if(tpd_sample_mode&TPD_SW || tpd_sample_mode&RAW_DATA_MODE) {
	    for (i = 0; i < 8; i++) { 
	        rx  = tpd_read(TPD_X, i );
	        ry  = tpd_read(TPD_Y, i );
	        rz1 = tpd_read(TPD_Z1, i );
	        rz2 = tpd_read(TPD_Z2, i );
	        //printk("raw[%5d %5d %5d %5d] \n", rx, ry, rz1, rz2);    
	        if (rx == 0 && ry == 4095) break;
	                
	        x += rx; y += ry;
	        p = p + ((rx + 1) * (rz2 - rz1) / (rz1 + 1));
	        
	        TPD_EM_PRINT(rx, ry, rz1, rz2, ((rx+1)*(rz2-rz1)/(rz1+1)), TPD_TYPE_RAW_DATA);
	        udelay(5);
	    } 
	    
	    if (i == 0)
	        x = 0, y = 0, p = 0;
	    else     
	        x /= i, y /= i, p /= i;
 	}

	if(tpd_sample_mode&FAV_MODE_SW || tpd_sample_mode&FAV_MODE_HW) {
		p = ((rx+1)*(rz2-rz1)/(rz1+1));
		TPD_EM_PRINT(rx, ry, rz1, rz2, p, TPD_TYPE_RAW_DATA);
		x = rx; 
		y = ry;
	}
  
    raw_x = x; raw_y = y;
     
    if(!rx && ry == 4095) 
        *cd = 0;
    else 
        *cd = 1;

    *cx = x, *cy = y, *cp = p;
  
   //printk("cx = %d, cy = %d, cp = %d, cd = %d\n", *cx, *cy, *cp, *cd);
   return 0;
}

/* handle touch panel interrupt for down / up event */
void tpd_tasklet(unsigned long unused) {
    int cx = 0, cy = 0, cp = 0, cd = 0;
    int debt0 = 0, debt1 = 0, spl_num = 0;
    static int down = 0;
    static int latency_us = 0;
    int ret;

    TPD_DEBUG_SET_TIME;
	if(tpd_sample_mode&FAV_MODE_HW || tpd_sample_mode&RAW_DATA_MODE) {
		if(tpd_event_status == 0) {
		   tpd_fav_switch(0);
		   if (atomic_read(&count_down) != 0) {
				atomic_set(&count_down, 0);
	            tpd_up(pre_x, pre_y, 0, 0);   
				pre_x = 0;
	        	pre_y = 0;	
			}
			goto tpd_end;
		}
	}

    //spin_lock_irqsave(&g_tpd_lock, g_tpd_flags);

	ret = tpd_sampling(&cx, &cy, &cp, &cd);
	//TPD_DMESG("tpd_sampling %d\n", ret);
    if(ret != 0) {
    	goto tpd_end;
    }

	if(tpd_sample_mode&TPD_SW) {
		// ignore unstable point from interrupt trigger    
		if (tpd_intr_status == 1) 
		{	
			do_gettimeofday(&d_time);        
			tpd_intr_status = 0;        
			mod_timer(&(tpd->timer), jiffies + TPD_DELAY * 2);       	
			TPD_EM_PRINT(0, 0, 0, 0, 0, TPD_TYPE_REJECT1);        
			goto tpd_end;  
		}
	}

    if(tpd_em_pressure_threshold >=TPD_PRESSURE_NICE) {
	    if (cp > tpd_em_pressure_threshold && cd != 0) {
	        debt0 = tpd_get_debounce_time_DEBT0();
	        debt1 = tpd_get_debounce_time_DEBT1();
	        spl_num = tpd_get_spl_number();
	        TPD_EM_PRINT(cp, tpd_em_pressure_threshold, debt0, debt1, spl_num, TPD_TYPE_REJECT2);
			if(tpd_sample_mode&FAV_MODE_SW || tpd_sample_mode&FAV_MODE_HW || tpd_sample_mode&RAW_DATA_MODE) {
				tpd_fav_switch(1);
			}
			if(tpd_sample_mode&TPD_SW) {
			  	mod_timer(&(tpd->timer), jiffies + TPD_DELAY * 2);
			 }
	        goto tpd_end; 
	    }
    } else {
	    if (cp > TPD_PRESSURE_NICE && cd != 0) {
		      debt0 = tpd_get_debounce_time_DEBT0();
		      debt1 = tpd_get_debounce_time_DEBT1();
		      spl_num = tpd_get_spl_number();
	          TPD_EM_PRINT(cp, TPD_PRESSURE_NICE, debt0, debt1, spl_num, TPD_TYPE_REJECT2);
			  if(tpd_sample_mode&FAV_MODE_SW || tpd_sample_mode&FAV_MODE_HW || tpd_sample_mode&RAW_DATA_MODE) {
					tpd_fav_switch(1);
				}
			  if(tpd_sample_mode&TPD_SW) {
			  	mod_timer(&(tpd->timer), jiffies + TPD_DELAY * 2);
			  }
	          goto tpd_end; 
	     }
    } 

	if(tpd_sample_mode&TPD_SW) {
		if(tpd_suspend_flag==1)		
			return; 
	}	

    if (cd == 1) {
        tpd_calibrate(&cx, &cy);
        
        if (cx < 0) cx = 0;
        if (cy < 0) cy = 0;
        if (cx > TPD_RES_X) cx = TPD_RES_X;
        if (cy > TPD_RES_Y) cy = TPD_RES_Y;
        
        //printk("pre_x = %d, pre_y = %d, cx = %d, cy = %d\n", pre_x, pre_y, cx, cy);
       
        if (pre_x != 0 || pre_y != 0) 
		{
		    if(atomic_read(&count_down) == 0) {
				do_gettimeofday(&first_report);
				latency_us =( (first_report.tv_sec & 0xFFF) * 1000000 + first_report.tv_usec)-( (d_time.tv_sec & 0xFFF) * 1000000 + d_time.tv_usec);
				TPD_EM_PRINT(latency_us, 0, 0, 0, 0, TPD_TYPE_FIST_LATENCY);
		    }
			down++;
	        cx = (pre_x + cx) / 2;
	        cy = (pre_y + cy) / 2;
		    if(cy==TPD_RES_Y) cy=TPD_RES_Y-1;
			//printk("[tpd_log]pre_x:0x%x, 0x%x\n", pre_x, pre_y);
			atomic_set(&count_down, 1);
	        tpd_down(cx, cy, cd, cp);			 
       }
 
       pre_x = cx;
       pre_y = cy;  
	   if(tpd_sample_mode&FAV_MODE_SW || tpd_sample_mode&FAV_MODE_HW || tpd_sample_mode&RAW_DATA_MODE) {
			tpd_fav_switch(1);
		}
	   if(tpd_sample_mode&TPD_SW) {
	   		mod_timer(&(tpd->timer), jiffies + TPD_DELAY);
	   }
    } else {    
        if (down&&(atomic_read(&count_down) != 0)) {
			atomic_set(&count_down, 0);
	    	if(cy==TPD_RES_Y) cy=TPD_RES_Y-1;
            tpd_up(pre_x, pre_y, cd, cp);    
            down = 0;
        }
        pre_x = 0;
        pre_y = 0;	
    }
	tpd_end:
		enable_irq(MT65XX_TOUCH_BATCH_LINE);
     //spin_lock_irqsave(&g_tpd_lock, g_tpd_flags);
    return;
}

//#ifdef CONFIG_HAS_EARLYSUSPEND
void tpd_driver_suspend(struct early_suspend *h) {
    disable_irq_nosync(MT65XX_TOUCH_IRQ_LINE);
    disable_irq_nosync(MT65XX_TOUCH_BATCH_LINE);
	tpd_fav_switch(0);
	tpd_intr_status = 0;
    tpd_suspend_flag = 1;
#if 0
    if(disable_clock(MT65XX_PDN_PERI_AUXADC,"Touch")==FALSE)
        TPD_DMESG("entering suspend mode - hwDisableClock failed.");
#endif
}
void tpd_driver_resume(struct early_suspend *h) {
#if 0
    if(enable_clock(MT65XX_PDN_PERI_AUXADC,"Touch")==FALSE)
        TPD_DMESG("resume from suspend mode - hwEnableClock TP failed.");
#endif	
	tpd_adc_init();
    enable_irq(MT65XX_TOUCH_IRQ_LINE);
   	enable_irq(MT65XX_TOUCH_BATCH_LINE);
    tpd_suspend_flag = 0;
}
//#endif




