#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/module.h>       /* needed by all modules */
#include <linux/init.h>         /* needed by module macros */
#include <linux/fs.h>		/* needed by file_operations* */
#include <linux/miscdevice.h>	/* needed by miscdevice* */
#include <linux/device.h>	/* needed by device_* */
#include <mach/hardware.h>	/* needed by __io_address */
#include <asm/io.h>		/* needed by ioremap * */
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/fs.h>		/* needed by file_operations* */
#include <mach/sync_write.h>


#ifdef CONFIG_SMP
#include <linux/smp.h>		/* needed by smp_call_function_single */
#endif

#include <mach/sync_write.h>
#include <asm/sections.h>
#include <mach/mt_reg_base.h>
#include <linux/aee.h>

#include "config.h"
#include "utils.h"
#include "etm_state.h"
#include "utils.h"
#include "etm_register.h"
#include "etb_register.h"
#include "config.h"


extern struct file *fp;
extern struct file *fp2;
unsigned long long last_ts[ETM_NO];
#define SIZE_4K	0x1000
#define TIMEOUT 100

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
#define DBGAPB_BASE   0xF0100000
#define CPUDBG0_BASE  DBGAPB_BASE+0x00070000
#define CPUPMU0_BASE  DBGAPB_BASE+0x00071000
#define CPUDBG1_BASE  DBGAPB_BASE+0x00072000
#define CPUPMU1_BASE  DBGAPB_BASE+0x00073000
#define CPUDBG2_BASE  DBGAPB_BASE+0x00074000
#define CPUPMU2_BASE  DBGAPB_BASE+0x00075000
#define CPUDBG3_BASE  DBGAPB_BASE+0x00076000
#define CPUPMU3_BASE  DBGAPB_BASE+0x00077000
#define CTI0_BASE     DBGAPB_BASE+0x00078000
#define CTI1_BASE     DBGAPB_BASE+0x00079000
#define CTI2_BASE     DBGAPB_BASE+0x0007A000
#define CTI3_BASE     DBGAPB_BASE+0x0007B000
#define PTM0_BASE     DBGAPB_BASE+0x0007C000
#define PTM1_BASE     DBGAPB_BASE+0x0007D000
#define PTM2_BASE     DBGAPB_BASE+0x0007E000
#define PTM3_BASE     DBGAPB_BASE+0x0007F000

//#define ETB_BASE      DBGAPB_BASE+0x00011000
/* ETR base */
#define ETB_BASE      DBGAPB_BASE+0x00013000

#define CTI_BASE      DBGAPB_BASE+0x00012000
#define TPIU_BASE     DBGAPB_BASE+0x00013000
#define FUNNEL_BASE   DBGAPB_BASE+0x00014000

#define STP_BASE      DBGAPB_BASE+0x00019000
#define DEM_BASE      DBGAPB_BASE+0x0001A000
#define SATATX_BASE   DBGAPB_BASE+0x0001B000

#define ETR_RAM_PHY_BASE 0x0010F800
#define ETR_RAM_VIR_BASE (INTER_SRAM + 0xF800)
#define ETR_RAM_SIZE 0x200

#define ETB_EXT_MODE       1
#define ETB_DDR            0
#define ETB_TIMESTAMP      1 
#define ETB_CYCLE_ACCURATE 0
  //----------------------------------------
#define CS_TP_PORTSIZE  16
/* T32 is 0x2001, we can apply 0x1 is fine */
//#define CS_FORMATMODE   0x12	// Enable Continuous formatter and FLUSHIN
#define CS_FORMATMODE   0x2001	// Enable Continuous formatter and FLUSHIN



//#define TRACE_RANGE_START  0x82200000    
//#define TRACE_RANGE_STOP   0x82658800    

//unsigned int TRACE_RANGE_START = 0xc0008180 ;
//unsigned int TRACE_RANGE_STOP = 0xc111e798;

//unsigned int TRACE_RANGE_START = 0x0;
//unsigned int TRACE_RANGE_STOP = 0xffffffff;

unsigned int TRACE_RANGE_START = 0xb0000000;
unsigned int TRACE_RANGE_STOP = 0xd0000000;

unsigned int data_trace_enable = 0;
unsigned int etm_0_enabled = 1;
unsigned int etm_1_enabled = 1;
unsigned int etm_2_enabled = 1;
unsigned int etm_3_enabled = 1;

/**
 * read from ETB register
 * @param ctx trace context
 * @param x register offset
 * @return value read from the register
 */
unsigned int etb_readl(struct etm_trace_context_t *ctx, int x)
{
	return __raw_readl(ctx->etb_regs + x);
}

/**
 * write to ETB register
 * @param ctx trace context
 * @param v value to be written to the register
 * @param x register offset
 * @return value written to the register
 */
void etb_writel(struct etm_trace_context_t *ctx, unsigned int v, int x)
{
	mt65xx_reg_sync_writel(v, ctx->etb_regs + x);
}

/**
 * check whether ETB supports lock
 * @param ctx trace context
 * @return 1:supports lock, 0:doesn't
 */
int etb_supports_lock(struct etm_trace_context_t *ctx)
{
	return etb_readl(ctx, ETBLS) & 0x1;
}

/**
 * check whether ETB registers are locked
 * @param ctx trace context
 * @return 1:locked, 0:aren't
 */
int etb_is_locked(struct etm_trace_context_t *ctx)
{
	return etb_readl(ctx, ETBLS) & 0x2;
}

/**
 * disable further write access to ETB registers
 * @param ctx trace context
 */
void etb_lock(struct etm_trace_context_t *ctx)
{
	if (etb_supports_lock(ctx)) {
		do {
			etb_writel(ctx, 0, ETBLA);
		} while (unlikely(!etb_is_locked(ctx)));
	} else {
		p_warning("etb does not support lock\n");
	}
}

/**
 * enable further write access to ETB registers
 * @param ctx trace context
 */
void etb_unlock(struct etm_trace_context_t *ctx)
{
	if (etb_supports_lock(ctx)) {
		do {
			etb_writel(ctx, ETBLA_UNLOCK_MAGIC, ETBLA);
		} while (unlikely(etb_is_locked(ctx)));
	} else {
		p_warning("etb does not support lock\n");
	}
}

int etb_get_data_length(struct etm_trace_context_t *t)
{
        unsigned int v;
        int rp, wp;

        v = etb_readl(t, ETBSTS);
        rp = etb_readl(t, ETBRRP);
        wp = etb_readl(t, ETBRWP);
        
        printk("v = %x, rp = %x, wp = %x\n", v, rp, wp);

        if (v & 1)  //bit 0 for RAM full
          return t->etb_total_buf_size;
        
        return (wp - rp);
}

static int etb_open(struct inode *inode, struct file *file)
{
	if (!tracer.etb_regs)
		return -ENODEV;

	file->private_data = &tracer;

	return nonseekable_open(inode, file);
}

static ssize_t etb_read(struct file *file, char __user *data,
		size_t len, loff_t *ppos)
{
  //int total, i;
  int i;
  int length;
  struct etm_trace_context_t *t = file->private_data;
  u32 first = 0;
  u32 *buf;
  unsigned long copy_size;
  unsigned int buffer_end = ETR_RAM_PHY_BASE + (ETR_RAM_SIZE*4);

  loff_t pos = *ppos;

  mutex_lock(&t->mutex);

  if(t->state == TRACE_STATE_TRACING)
  {
    length = 0;
    p_warning("[ETM] trace is running! could not dump etb, please stop trace first\n");
    goto out;
  }

  etb_unlock(t);
  length = etb_get_data_length(t);
  if(length <= 0)
  {
    printk("[ETM] The flow is not correct, etb length sould not be <= 0!\n ");
    goto out;
  }  
  
  ////debug////
  printk("[ETM] ETBRWP = %x\n", etb_readl(t, ETBRWP));
  printk("[ETM] ETBRRP = %x\n", etb_readl(t, ETBRRP));
  printk("[ETM] ETBRDP = %x\n\n", etb_readl(t, ETBRDP));
  printk("[ETM] length = %x\n", length);
  ////debug////
  
  if (length == t->etb_total_buf_size)
    first = etb_readl(t, ETBRWP);  
  
  //first += 4; /* move the the oldest ptr */

  printk("(1) first = %x\n\n", first);
  
  /* Error handling */
  if(first<ETR_RAM_PHY_BASE || first>=(buffer_end))
  {
    printk("[ETM] ETB rp, wp value are not correct!!! ETR_RAM_PHY_BASE = %x, ETR_RAM_PHY_END = %x\n",ETR_RAM_PHY_BASE, buffer_end);
    goto out;
  }
  
  etb_writel(t, first, ETBRRP);

  buf = vmalloc(length * 4);

#if 0
  //for (i=length; i>=0; i--) 
  for (i=0; i<length; i++) 
  {
    buf[i] = etb_readl(t, ETBRRD);
    printk("ETBRRD = %x, first = %x\n", buf[i], first);
    first += 4;
    if(first >= 0x00110000)
    {
      first = 0x0010dd00;
      etb_writel(t, first, ETBRRP);
    }
  }
#endif

  /* 0x00100000 is ISRAM's PHY addr */
  first = (first - 0x00100000) + INTER_SRAM;
  printk("(2) first = %x\n\n", first);

  for (i=0; i<length; i++) 
  {
    buf[i] = *((unsigned int*)(first));
    //printk("ETBRRD = %x, first = %x\n", buf[i], first);
    first += 4;
    if(first >= 0xF9010000) /* 0xF9010000 is the end of ISRAM */
    {
      first = ETR_RAM_VIR_BASE;  
    }
  }


  etb_lock(t);

  copy_size = copy_to_user((char *)data, (char *)buf, length*4);
  
  vfree(buf);
  *ppos = pos + length;
  
out:
  mutex_unlock(&t->mutex);

  return length;
}

/**
 * read from ETM register
 * @param ctx trace context
 * @param n ETM index
 * @param x register offset
 * @return value read from the register
 */
unsigned int etm_readl(struct etm_trace_context_t *ctx, int n, int x)
{
	return __raw_readl(ctx->etm_regs[n] + x);
}

/**
 * write to ETM register
 * @param ctx trace context
 * @param n ETM index
 * @param v value to be written to the register
 * @param x register offset
 * @return value written to the register
 */
unsigned int etm_writel(struct etm_trace_context_t *ctx, int n, unsigned int v,
			int x)
{
	return __raw_writel(v, ctx->etm_regs[n] + x);
}

static struct file_operations etm_file_ops = {
	.owner = THIS_MODULE,
   .read = etb_read,
   .open = etb_open,
};
#ifdef __ETM
static struct miscdevice etm_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "etm",
	.fops = &etm_file_ops
};
#else
static struct miscdevice etm_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ptm",
	.fops = &etm_file_ops
};
#endif

struct etm_trace_context_t tracer;

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
#define UINT32 unsigned int
#define UINT32P unsigned int*

void cs_cpu_write (UINT32 addr_base, UINT32 offset, UINT32 wdata)
{
  /* TINFO="Write addr %h, with data %h", addr_base+offset, wdata */
  //*((UINT32P) (addr_base+offset)) = wdata;
  mt65xx_reg_sync_writel(wdata, addr_base+offset);
}

UINT32 cs_cpu_read (UINT32 addr_base, UINT32 offset)
{
   UINT32 actual;
   
   actual = *((UINT32P) (addr_base+offset));
   /* TINFO="Read addr %h, with data %h", addr_base+offset, actual */
   return actual;
}

void cs_cpu_lock(UINT32 addr_base)
{
  UINT32 result;
  cs_cpu_write(addr_base, 0xFB0, 0);

  dsb();

  result = cs_cpu_read (addr_base, 0xFB4);

  if (result != 0x03) {
    aee_sram_fiq_log("[ETM] magic lock failed!\n");
    //while(1);
  } 

  /*OS lock lock*/
  cs_cpu_write(addr_base, 0x300, 0xC5ACCE55);
  dsb();

  return;
}

void cs_cpu_unlock (UINT32 addr_base) 
{
  UINT32 result;

  //dbg_print("\r\n --- cs_cpu_unlock 1 ---\r\n");

  cs_cpu_write(addr_base, 0xFB0, 0xC5ACCE55);

  //dbg_print("\r\n --- cs_cpu_unlock 2 ---\r\n");
  
  dsb();
  result = cs_cpu_read (addr_base, 0xFB4);

  //dbg_print("\r\n --- cs_cpu_unlock 3 ---\r\n");
  if (result != 0x01) {
    aee_sram_fiq_log("[ETM] magic unlock failed!\n");
    //while(1);
  }

  /*OS lock unlock*/
  cs_cpu_write(addr_base, 0x300, 0x0);
  dsb();
  //result = cs_cpu_read (addr_base, 0x304);
  

  //dbg_print("\r\n --- cs_cpu_unlock 4 ---\r\n");
  
    return;
}

void cs_cpu_ptm_powerup (UINT32 ptm_addr_base)
{
  UINT32 result;

  result = cs_cpu_read (ptm_addr_base, 0x000);
  result = result ^ 0x001;
  cs_cpu_write(ptm_addr_base, 0x000, result);
}

void cs_cpu_ptm_progbit (UINT32 ptm_addr_base)
{
  UINT32 result;
  UINT32 counter = 0;

  result = cs_cpu_read (ptm_addr_base, 0x000);
  // Set PTMControl.PTMProgramming ([10])
  cs_cpu_write(ptm_addr_base, 0x000, result | (1<<10));	
  result = 0;
  // Wait for PTMStatus.PTMProgramming ([1]) to be set
  while( !(result & ((1<<1))) ) {
      result = cs_cpu_read (ptm_addr_base, 0x010);
      counter++;
      if(counter >= TIMEOUT)
      {
        aee_sram_fiq_log("[ETM] set program bit timeout!\n");  
        break;
      }
  }
}

void cs_cpu_ptm_clear_progbit (UINT32 ptm_addr_base)
{
  UINT32 result;
  UINT32 counter = 0;

  result = cs_cpu_read (ptm_addr_base, 0x000);
  // Clear PTMControl.PTMProgramming ([10])
  cs_cpu_write(ptm_addr_base, 0x000, result ^ (1<<10));
  // Wait for PTMStatus.PTMProgramming ([1]) to be clear
  do{
    result = cs_cpu_read (ptm_addr_base, 0x010);
    counter++;
    if(counter >= TIMEOUT)
    {
      aee_sram_fiq_log("[ETM] clear program bit timeout!\n");          
      break;
    }
    //printk("[ETM clear prog_bit]result = %x\n", result);
  }while( (result & 0x2) );
  ///*"PTMStatus.PTMProgramming ([1]) is cleared" */
}

void cs_cpu_tpiu_setup (void)
{
  UINT32 result;  

  // Current Port Size - Port Size 16
  cs_cpu_write (TPIU_BASE, 0x004, 1 << (CS_TP_PORTSIZE - 1));
  result = cs_cpu_read (TPIU_BASE, 0x000);
  if (!(result & ((CS_TP_PORTSIZE - 1))) ) 
  {
      /* TERR="MAXPORTSIZE tie-off conflicts with port size" */
  }
  // Formatter and Flush Control Register - Enable Continuous formatter and FLUSHIN
  cs_cpu_write (TPIU_BASE, 0x304, CS_FORMATMODE); // enable formating
}

void cs_cpu_funnel_setup (void)
{
  UINT32 funnel_ports;

  funnel_ports = 0;
  
  if(etm_0_enabled == 1)
  {  
    funnel_ports = funnel_ports | (1<<0);	// PTM0 present
  }
  if(etm_1_enabled == 1)
  {  
    funnel_ports = funnel_ports | (1<<1); // PTM1 present
  }
  if(etm_2_enabled == 1)
  {  
    funnel_ports = funnel_ports | (1<<2); // PTM2 present
  }
  if(etm_3_enabled == 1)
  {  
    funnel_ports = funnel_ports | (1<<3); // PTM3 present
  }
  //funnel_ports = funnel_ports | (1<<7);	// flush master
  cs_cpu_write (FUNNEL_BASE, 0x000, funnel_ports);
  //Adjust priorities so that ITM has highest
  //cs_cpu_write (FUNNEL_BASE, 0x004, 0x00FAC0D1);
}

void cs_cpu_flushandstop (UINT32 device_addr_base)
{
  UINT32 result;
  UINT32 counter = 0;

  // Configure the device to stop on flush completion
  cs_cpu_write (device_addr_base, 0x304, CS_FORMATMODE | (1<<12));
  // Cause a manual flush
  cs_cpu_write (device_addr_base, 0x304, CS_FORMATMODE | (1<<6));
  // Now wait for the device to stop
  result = 0x02;
  while( result & 0x02 ) {
      result = cs_cpu_read (device_addr_base, 0x300);
      counter++;
      if(counter >= TIMEOUT)
      {
        aee_sram_fiq_log("[ETM] flush and stop timeout!\n");  
        break;
      }
  }
}

void cs_cpu_etb_setup (void)
{
  UINT32 result;  
  

  // Formatter and Flush Control Register - Enable Continuous formatter and FLUSHIN
  cs_cpu_write (ETB_BASE, 0x304, CS_FORMATMODE); // enable formating
  // Configure ETB control (set TraceCapture)
  cs_cpu_write (ETB_BASE, 0x020, 0x01);

  // Configure ETB to External Mode
#if ETB_EXT_MODE
  cs_cpu_write (ETB_BASE, 0xEFC, 0x01);
#else
  cs_cpu_write (ETB_BASE, 0xEFC, 0x00);
#endif
  // Configure ETB to External Address 
  result = cs_cpu_read (ETB_BASE, 0xEFC);
#if ETB_DDR
  cs_cpu_write (ETB_BASE, 0xEFC, result | (0x83C00000)); //DDR
#else
  cs_cpu_write (ETB_BASE, 0xEFC, result | (0x100000)); //onchip SRAM
#endif


}

void cs_cpu_test_common_ptm_setup (UINT32 ptm_addr_base)
{
  UINT32 result;

  // Set up to trace memory range defined by ARC1
  // SAC1&2 (ARC1)
  /* Comment to Record all */
  cs_cpu_write (ptm_addr_base, 0x040, TRACE_RANGE_START); 
  cs_cpu_write (ptm_addr_base, 0x044, TRACE_RANGE_STOP); 
  // TATR1&2
  cs_cpu_write (ptm_addr_base, 0x080, 0x01);
  cs_cpu_write (ptm_addr_base, 0x084, 0x01);
  // TraceEnable Event
  cs_cpu_write (ptm_addr_base, 0x020, 0x10);
  // TraceEnable Control
  cs_cpu_write (ptm_addr_base, 0x024, 0x01);	
  
  // Set up trigger event to never occur
  cs_cpu_write (ptm_addr_base, 0x008, 0x0000406F);
  //cs_cpu_write (ptm_addr_base, 0x008, 0x00000010);

  // Set up timestamp event to never occur
  cs_cpu_write (ptm_addr_base, 0x1F8, 0x0000406F);
  
  // Enable timestamp
  // Set PTMControl.Timestamping ([28])
#if ETB_TIMESTAMP
  result = cs_cpu_read (ptm_addr_base, 0x000);
  cs_cpu_write (ptm_addr_base, 0x000, result | (1<<28));
#endif
  // Enable cycle accurate 
  // Set PTMControl.CycleAccurate ([12])
#if ETB_CYCLE_ACCURATE
  result = cs_cpu_read (ptm_addr_base, 0x000);
  cs_cpu_write (ptm_addr_base, 0x000, result | (1<<12));
#endif
  // Set up Context ID tracing to all 4 bytes
  // Set PTMControl.ContextIDsize ([15:14]) to 3
  result = cs_cpu_read (ptm_addr_base, 0x000);
  cs_cpu_write (ptm_addr_base, 0x000, result | (3<<14));

  if(data_trace_enable == 0x1)
  {
    /* Set up Data access tracing (Trace both the address and the data of the access) */
    result = cs_cpu_read (ptm_addr_base, 0x000);
    cs_cpu_write (ptm_addr_base, 0x000, result | (3<<2));

    /* Enable ViewData enabling event */
    cs_cpu_write (ptm_addr_base, 0x30, 0x37EF);
    /* Exclude only */
    cs_cpu_write (ptm_addr_base, 0x3C, 0x10000);  
  } 
  else
  {
    /* Cancel data tracing */
    result = cs_cpu_read (ptm_addr_base, 0x000);
    cs_cpu_write (ptm_addr_base, 0x000, result & 0xFFFFFFF9);
  }
  
  // Set up synchronization frequency
  //cs_cpu_write (ptm_addr_base, 0x1E0, 0x00000400);  // 1024
  cs_cpu_write (ptm_addr_base, 0x1E0, 0x00000080);  // 128

  //Init ETM
  result = cs_cpu_read(ptm_addr_base, 0x000);
  cs_cpu_write (ptm_addr_base, 0x000, result | (1<<11));
}

void create_backup_file(struct etm_trace_context_t *t)
{
  struct file *fp = NULL;
  int i;
  unsigned int main_control_value;

  fp = open_file("/data/etm.backup");

  t->used_ETMs = 0b1111; /* duad core */
  atomic_set(&t->tracing_ETMs, 4); 
  
	file_write(fp, (unsigned char *)(&(t->used_ETMs)), 4);
	file_write(fp, (unsigned char *)(&(t->tracing_ETMs)), 4);
	for(i = 0; i < ETM_NO; i++)
	{
	    //struct etm_control_t *control = etm_get_control(t, i);
	    main_control_value = etm_readl(t, i, ETMCR);
	    file_write(fp, (unsigned char *)(&main_control_value), 4);
	    //etm_put_control(i);
	}

	file_write(fp, (unsigned char *)(&(t->etm_config)), sizeof(struct etm_config_t));
	close_file(fp);
}

#if 0
int switch_2_etr()
{
  int ret = 0;
  iounmap(tracer.etb_regs);
  tracer.etb_regs = NULL;

  tracer.etb_regs = ioremap_nocache(ETR_BASE_PHY, SIZE_4K);
  if (unlikely(!tracer.etb_regs)) 
  {
    p_error("ioremap ETR register failed\n");
    ret = -ENOMEM;
    goto remap_etb;
  }  

  /* AHBAP_EN to enable master port, then ETR could write the trace to bus */
  mt65xx_reg_sync_writel(ETMLAR_UNLOCK_MAGIC, DEM_Unlock);
  mt65xx_reg_sync_writel(0x1, DEM_BASE + 0x40);

  etb_unlock(&tracer);

  /* Set up ETR memory buffer address */  
  etb_writel(&tracer, 0x00109000, 0x118);
  /* Set up ETR memory buffer size */
  etb_writel(&tracer, 0x500, 0x4);

  return ret; 

remap_etb:
  tracer.etb_regs = ioremap_nocache(ETB_BASE_PHY, SIZE_4K);
  if (unlikely(!tracer.etb_regs)) 
  {
    p_error("ioremap ETB register failed\n");
    ret = -ENOMEM;
    return ret;
  }  
}
#endif

void trace_start(void)
{
  //unsigned int faxi_clk;
  
  if(tracer.state == TRACE_STATE_TRACING)
  {
    printk("[ETM] etm trace is now already running!\n");
    return;
  }

  mutex_lock(&tracer.mutex);
  
  /* AHBAP_EN to enable master port, then ETR could write the trace to bus */
  mt65xx_reg_sync_writel(ETMLAR_UNLOCK_MAGIC, DEM_Unlock);
  mt65xx_reg_sync_writel(0x1, DEM_BASE + 0x40);
  
  etb_unlock(&tracer);

  cs_cpu_unlock(PTM0_BASE);
  cs_cpu_unlock(PTM1_BASE);
  cs_cpu_unlock(PTM2_BASE);
  cs_cpu_unlock(PTM3_BASE);

  cs_cpu_unlock(TPIU_BASE);
  cs_cpu_unlock(FUNNEL_BASE);
  cs_cpu_unlock(ETB_BASE);
  
  cs_cpu_funnel_setup();
  cs_cpu_etb_setup();  
  
  // Power-up PTMs
  cs_cpu_ptm_powerup(PTM0_BASE);
  cs_cpu_ptm_powerup(PTM1_BASE);
  cs_cpu_ptm_powerup(PTM2_BASE);
  cs_cpu_ptm_powerup(PTM3_BASE);

  //Disable PTMs so that they can be set up safely
  cs_cpu_ptm_progbit(PTM0_BASE);
  cs_cpu_ptm_progbit(PTM1_BASE);
  cs_cpu_ptm_progbit(PTM2_BASE);
  cs_cpu_ptm_progbit(PTM3_BASE);

  //Set up PTMs
  cs_cpu_test_common_ptm_setup(PTM0_BASE);
  cs_cpu_test_common_ptm_setup(PTM1_BASE);
  cs_cpu_test_common_ptm_setup(PTM2_BASE);
  cs_cpu_test_common_ptm_setup(PTM3_BASE);
  
  //Set up CoreSightTraceID
  cs_cpu_write (PTM0_BASE, 0x200, 0x1);  
  //Set up CoreSightTraceID
  cs_cpu_write (PTM1_BASE, 0x200, 0x2);
  cs_cpu_write (PTM2_BASE, 0x200, 0x3);
  cs_cpu_write (PTM3_BASE, 0x200, 0x4);

  create_backup_file(&tracer);

  
  //Enable PTMs now everything has been set up
  cs_cpu_ptm_clear_progbit(PTM0_BASE);
  cs_cpu_ptm_clear_progbit(PTM1_BASE);
  cs_cpu_ptm_clear_progbit(PTM2_BASE);
  cs_cpu_ptm_clear_progbit(PTM3_BASE);

  
  /* Avoid DBG_sys being reset */
  mt65xx_reg_sync_writel(ETMLAR_UNLOCK_MAGIC, DEM_Unlock);
  mt65xx_reg_sync_writel(0x0, DEM_Reset);
  mt65xx_reg_sync_writel(0x1, 0xF011a02C);
  mt65xx_reg_sync_writel(0x1, 0xF011a030);
  
#if 0  
  /* downgrade CA7 to ETM clk */
  faxi_clk = readl(0xF0000140);
  faxi_clk &= 0xFFFFFFF0;
  faxi_clk |= 0x3;
  printk("[ETM] faxi_clk = %x\n", faxi_clk);
  mt65xx_reg_sync_writel(faxi_clk, 0xF0000140);
#endif  

  tracer.state = TRACE_STATE_TRACING;
  
  etb_lock(&tracer);

  mutex_unlock(&tracer.mutex);
}
  
void trace_stop(void)
{      
  if(tracer.state == TRACE_STATE_STOP)
  {
    printk("[ETM] etm trace is now already stop!\n");
    return;
  }
  
  mutex_lock(&tracer.mutex);
  
  etb_unlock(&tracer);
  
   /* "Trace program done" */
  /* "Disable trace components" */
  cs_cpu_ptm_progbit(PTM0_BASE);
  cs_cpu_ptm_progbit(PTM1_BASE);
  cs_cpu_ptm_progbit(PTM2_BASE);
  cs_cpu_ptm_progbit(PTM3_BASE);

  //cs_cpu_flushandstop (TPIU_BASE);
  
  //Disable ETB capture (ETB_CTL bit0 = 0x0)
  cs_cpu_write (ETB_BASE, 0x20, 0x0);
  //Reset ETB RAM Read Data Pointer (ETB_RRP = 0x0)
  /* no need to reset RRP*/
  //cs_cpu_write (ETB_BASE, 0x14, 0x0);

  cs_cpu_write (PTM0_BASE, 0x0, 0x1); //Power down
  cs_cpu_write (PTM1_BASE, 0x0, 0x1); //Power down
  cs_cpu_write (PTM2_BASE, 0x0, 0x1); //Power down
  cs_cpu_write (PTM3_BASE, 0x0, 0x1); //Power down
  dsb();

  tracer.state = TRACE_STATE_STOP;

  etb_lock(&tracer);

  mutex_unlock(&tracer.mutex);
}

/* For dormant driver */
void trace_start_dormant(void)
{
  //  if(tracer.state == TRACE_STATE_TRACING)
  //{
    //return;
  //}
  
  ////////////////////////////////////////
  ////////////////////////////////////////
  /* AHBAP_EN to enable master port, then ETR could write the trace to bus */
  //mt65xx_reg_sync_writel(ETMLAR_UNLOCK_MAGIC, DEM_Unlock);
  mt65xx_reg_sync_writel(0x1, DEM_BASE + 0x40);

  etb_unlock(&tracer);

  /* Set up ETR memory buffer address */  
  etb_writel(&tracer, ETR_RAM_PHY_BASE, 0x118);
  /* Set up ETR memory buffer size */
  etb_writel(&tracer, ETR_RAM_SIZE, 0x4);

  //Disable ETB capture (ETB_CTL bit0 = 0x0)
  /* For wdt reset */
  cs_cpu_write (ETB_BASE, 0x20, 0x0);

  cs_cpu_unlock(PTM0_BASE);
  cs_cpu_unlock(PTM1_BASE);
  cs_cpu_unlock(PTM2_BASE);
  cs_cpu_unlock(PTM3_BASE);

  cs_cpu_unlock(TPIU_BASE);
  cs_cpu_unlock(FUNNEL_BASE);
  cs_cpu_unlock(ETB_BASE);
  
  cs_cpu_funnel_setup();
  cs_cpu_etb_setup();  
  
  // Power-up PTMs
  cs_cpu_ptm_powerup(PTM0_BASE);
  cs_cpu_ptm_powerup(PTM1_BASE);
  cs_cpu_ptm_powerup(PTM2_BASE);
  cs_cpu_ptm_powerup(PTM3_BASE);

  //Disable PTMs so that they can be set up safely
  cs_cpu_ptm_progbit(PTM0_BASE);
  cs_cpu_ptm_progbit(PTM1_BASE);
  cs_cpu_ptm_progbit(PTM2_BASE);
  cs_cpu_ptm_progbit(PTM3_BASE);

  //Set up PTMs
  cs_cpu_test_common_ptm_setup(PTM0_BASE);
  cs_cpu_test_common_ptm_setup(PTM1_BASE);
  cs_cpu_test_common_ptm_setup(PTM2_BASE);
  cs_cpu_test_common_ptm_setup(PTM3_BASE);
  
  //Set up CoreSightTraceID
  cs_cpu_write (PTM0_BASE, 0x200, 0x1);  
  //Set up CoreSightTraceID
  cs_cpu_write (PTM1_BASE, 0x200, 0x2);
  cs_cpu_write (PTM2_BASE, 0x200, 0x3);
  cs_cpu_write (PTM3_BASE, 0x200, 0x4);  
  
  //Enable PTMs now everything has been set up
  cs_cpu_ptm_clear_progbit(PTM0_BASE);
  cs_cpu_ptm_clear_progbit(PTM1_BASE);
  cs_cpu_ptm_clear_progbit(PTM2_BASE);
  cs_cpu_ptm_clear_progbit(PTM3_BASE);

  
  /* Avoid DBG_sys being reset */
  mt65xx_reg_sync_writel(ETMLAR_UNLOCK_MAGIC, DEM_Unlock);
  mt65xx_reg_sync_writel(0x0, DEM_Reset);
  mt65xx_reg_sync_writel(0x1, 0xF011a02C);
  mt65xx_reg_sync_writel(0x1, 0xF011a030);

  etb_lock(&tracer);

  //tracer.state = TRACE_STATE_TRACING;
}

/* For dormant driver */
void trace_stop_dormant(void)
{      
  //if(tracer.state == TRACE_STATE_STOP)
  //{
//    return;
  //}      
  etb_unlock(&tracer);
   /* "Trace program done" */
  /* "Disable trace components" */
  cs_cpu_ptm_progbit(PTM0_BASE);
  cs_cpu_ptm_progbit(PTM1_BASE);
  cs_cpu_ptm_progbit(PTM2_BASE);
  cs_cpu_ptm_progbit(PTM3_BASE);

  //cs_cpu_flushandstop (TPIU_BASE);
  
  //Disable ETB capture (ETB_CTL bit0 = 0x0)
  cs_cpu_write (ETB_BASE, 0x20, 0x0);
  //Reset ETB RAM Read Data Pointer (ETB_RRP = 0x0)
  /* no need to reset RRP*/
  //cs_cpu_write (ETB_BASE, 0x14, 0x0);

  cs_cpu_write (PTM0_BASE, 0x0, 0x1); //Power down
  cs_cpu_write (PTM1_BASE, 0x0, 0x1); //Power down
  cs_cpu_write (PTM2_BASE, 0x0, 0x1); //Power down
  cs_cpu_write (PTM3_BASE, 0x0, 0x1); //Power down
  dsb();

  etb_lock(&tracer);

  //tracer.state = TRACE_STATE_STOP;
}

/* For debug tool */
void trace_start_unlock(void)
{
  if(tracer.state == TRACE_STATE_TRACING)
  {
    return;
  }
  
  /* AHBAP_EN to enable master port, then ETR could write the trace to bus */
  mt65xx_reg_sync_writel(ETMLAR_UNLOCK_MAGIC, DEM_Unlock);
  mt65xx_reg_sync_writel(0x1, DEM_BASE + 0x40);
  
  etb_unlock(&tracer);

  cs_cpu_unlock(PTM0_BASE);
  cs_cpu_unlock(PTM1_BASE);
  cs_cpu_unlock(PTM2_BASE);
  cs_cpu_unlock(PTM3_BASE);

  cs_cpu_unlock(TPIU_BASE);
  cs_cpu_unlock(FUNNEL_BASE);
  cs_cpu_unlock(ETB_BASE);
  
  cs_cpu_funnel_setup();
  cs_cpu_etb_setup();  
  
  // Power-up PTMs
  cs_cpu_ptm_powerup(PTM0_BASE);
  cs_cpu_ptm_powerup(PTM1_BASE);
  cs_cpu_ptm_powerup(PTM2_BASE);
  cs_cpu_ptm_powerup(PTM3_BASE);

  //Disable PTMs so that they can be set up safely
  cs_cpu_ptm_progbit(PTM0_BASE);
  cs_cpu_ptm_progbit(PTM1_BASE);
  cs_cpu_ptm_progbit(PTM2_BASE);
  cs_cpu_ptm_progbit(PTM3_BASE);

  //Set up PTMs
  cs_cpu_test_common_ptm_setup(PTM0_BASE);
  cs_cpu_test_common_ptm_setup(PTM1_BASE);
  cs_cpu_test_common_ptm_setup(PTM2_BASE);
  cs_cpu_test_common_ptm_setup(PTM3_BASE);
  
  //Set up CoreSightTraceID
  cs_cpu_write (PTM0_BASE, 0x200, 0x1);  
  //Set up CoreSightTraceID
  cs_cpu_write (PTM1_BASE, 0x200, 0x2);
  cs_cpu_write (PTM2_BASE, 0x200, 0x3);
  cs_cpu_write (PTM3_BASE, 0x200, 0x4);  
  
  //Enable PTMs now everything has been set up
  cs_cpu_ptm_clear_progbit(PTM0_BASE);
  cs_cpu_ptm_clear_progbit(PTM1_BASE);
  cs_cpu_ptm_clear_progbit(PTM2_BASE);
  cs_cpu_ptm_clear_progbit(PTM3_BASE);

  
  /* Avoid DBG_sys being reset */
  mt65xx_reg_sync_writel(ETMLAR_UNLOCK_MAGIC, DEM_Unlock);
  mt65xx_reg_sync_writel(0x0, DEM_Reset);
  mt65xx_reg_sync_writel(0x1, 0xF011a02C);
  mt65xx_reg_sync_writel(0x1, 0xF011a030);

  etb_lock(&tracer);

  tracer.state = TRACE_STATE_TRACING;
}

/* For debug tool */
void trace_stop_unlock(void)
{      
  if(tracer.state == TRACE_STATE_STOP)
  {
    return;
  }      
  
  etb_unlock(&tracer);
  
   /* "Trace program done" */
  /* "Disable trace components" */
  cs_cpu_ptm_progbit(PTM0_BASE);
  cs_cpu_ptm_progbit(PTM1_BASE);
  cs_cpu_ptm_progbit(PTM2_BASE);
  cs_cpu_ptm_progbit(PTM3_BASE);

  //cs_cpu_flushandstop (TPIU_BASE);
  
  //Disable ETB capture (ETB_CTL bit0 = 0x0)
  cs_cpu_write (ETB_BASE, 0x20, 0x0);
  //Reset ETB RAM Read Data Pointer (ETB_RRP = 0x0)
  /* no need to reset RRP*/
  //cs_cpu_write (ETB_BASE, 0x14, 0x0);

  cs_cpu_write (PTM0_BASE, 0x0, 0x1); //Power down
  cs_cpu_write (PTM1_BASE, 0x0, 0x1); //Power down
  cs_cpu_write (PTM2_BASE, 0x0, 0x1); //Power down
  cs_cpu_write (PTM3_BASE, 0x0, 0x1); //Power down
  dsb();
  
  etb_lock(&tracer);

  tracer.state = TRACE_STATE_STOP;
}
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

/**
 * read callback function of proc file system entry related to the decoder
 */
static inline ssize_t run_show(struct device *kobj,
				struct device_attribute *attr, char *buf)
{
        return snprintf(buf, PAGE_SIZE, "%x\n", tracer.state);
}

/**
 * write callback function of proc file system entry related to the decoder
 */
static ssize_t run_store(struct device *kobj, struct device_attribute *attr,
				const char *buf, size_t n)
{
    unsigned int value;

    if (unlikely(sscanf(buf, "%u", &value) != 1))
                return -EINVAL;

    if(value == 1)
      trace_start();
    else if(value == 0)
      trace_stop();
    else
      return -EINVAL;

    return n;
}
DEVICE_ATTR(run, 0644, run_show, run_store);

static inline ssize_t etb_length_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
  int etb_legnth;
  etb_legnth = etb_get_data_length(&tracer);
  return sprintf(buf, "%08x\n", etb_legnth);
}

static ssize_t etb_length_store(struct device *kobj, struct device_attribute *attr,	const char *buf, size_t n)
{
  /*do nothing*/
  return n;
}

DEVICE_ATTR(etb_length, 0644, etb_length_show, etb_length_store);

static inline ssize_t trace_data_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%08x\n", data_trace_enable);
}

static ssize_t trace_data_store(struct device *kobj, struct device_attribute *attr,	const char *buf, size_t n)
{
  unsigned int value;

  if (unlikely(sscanf(buf, "%u", &value) != 1))
    return -EINVAL;  

  if(tracer.state == TRACE_STATE_TRACING)
  {
    printk("[ETM] etm trace is now already running!Please stop etm first before change setting\n");
    return n;
  }

  mutex_lock(&tracer.mutex);

  if(value == 1)
  {    
    data_trace_enable = 1;
  }
  else
  {
    data_trace_enable = 0;
  }
  mutex_unlock(&tracer.mutex);
  
  return n;
}

DEVICE_ATTR(trace_data, 0644, trace_data_show, trace_data_store);


/*Set monitor range*/
static ssize_t trace_range_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%08x %08x\n",
			TRACE_RANGE_START, TRACE_RANGE_STOP);
}

static ssize_t trace_range_store(struct device *kobj, struct device_attribute *attr,	const char *buf, size_t n)
{
  unsigned int range_start, range_end;
  
  if (sscanf(buf, "%x %x", &range_start, &range_end) != 2)
		return -EINVAL;

  if(tracer.state == TRACE_STATE_TRACING)
  {
    printk("[ETM] etm trace is now already running!Please stop etm first before change setting\n");
    return n;
  }

  mutex_lock(&tracer.mutex);
  TRACE_RANGE_START = range_start;
  TRACE_RANGE_STOP = range_end;
  mutex_unlock(&tracer.mutex);

  return n;
}

DEVICE_ATTR(trace_range, 0644, trace_range_show, trace_range_store);

/* Enable/Disable ETM run time interface */
static ssize_t etm0_online_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", etm_0_enabled);
}

static ssize_t etm0_online_store(struct device *kobj, struct device_attribute *attr,	const char *buf, size_t n)
{
  unsigned int enable = 0;
  
  if (sscanf(buf, "%d", &enable) != 1)
		return -EINVAL;

  if(tracer.state == TRACE_STATE_TRACING)
  {
    printk("[ETM] etm trace is now already running!Please stop etm first before change setting\n");
    return n;
  }

  mutex_lock(&tracer.mutex);
  if(enable == 1)
  {
    etm_0_enabled = 1;
  }
  else if(enable == 0)
  {
    etm_0_enabled = 0;
  }  
  mutex_unlock(&tracer.mutex);

  return n;
}

DEVICE_ATTR(etm0_online, 0644, etm0_online_show, etm0_online_store);

static ssize_t etm1_online_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", etm_1_enabled);
}

static ssize_t etm1_online_store(struct device *kobj, struct device_attribute *attr,	const char *buf, size_t n)
{
  unsigned int enable = 0;
  
  if (sscanf(buf, "%d", &enable) != 1)
		return -EINVAL;

  if(tracer.state == TRACE_STATE_TRACING)
  {
    printk("[ETM] etm trace is now already running!Please stop etm first before change setting\n");
    return n;
  }

  mutex_lock(&tracer.mutex);
  if(enable == 1)
  {
    etm_1_enabled = 1;
  }
  else if(enable == 0)
  {
    etm_1_enabled = 0;
  }  
  mutex_unlock(&tracer.mutex);

  return n;
}

DEVICE_ATTR(etm1_online, 0644, etm1_online_show, etm1_online_store);

static ssize_t etm2_online_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", etm_2_enabled);
}

static ssize_t etm2_online_store(struct device *kobj, struct device_attribute *attr,	const char *buf, size_t n)
{
  unsigned int enable = 0;
  
  if (sscanf(buf, "%d", &enable) != 1)
		return -EINVAL;

  if(tracer.state == TRACE_STATE_TRACING)
  {
    printk("[ETM] etm trace is now already running!Please stop etm first before change setting\n");
    return n;
  }

  mutex_lock(&tracer.mutex);
  if(enable == 1)
  {
    etm_2_enabled = 1;
  }
  else if(enable == 0)
  {
    etm_2_enabled = 0;
  }  
  mutex_unlock(&tracer.mutex);

  return n;
}

DEVICE_ATTR(etm2_online, 0644, etm2_online_show, etm2_online_store);

static ssize_t etm3_online_show(struct device *kobj, struct device_attribute *attr, char *buf)
{
  return sprintf(buf, "%d\n", etm_3_enabled);
}

static ssize_t etm3_online_store(struct device *kobj, struct device_attribute *attr,	const char *buf, size_t n)
{
  unsigned int enable = 0;
  
  if (sscanf(buf, "%d", &enable) != 1)
		return -EINVAL;

  if(tracer.state == TRACE_STATE_TRACING)
  {
    printk("[ETM] etm trace is now already running!Please stop etm first before change setting\n");
    return n;
  }

  mutex_lock(&tracer.mutex);
  if(enable == 1)
  {
    etm_3_enabled = 1;
  }
  else if(enable == 0)
  {
    etm_3_enabled = 0;
  }  
  mutex_unlock(&tracer.mutex);

  return n;
}

DEVICE_ATTR(etm3_online, 0644, etm3_online_show, etm3_online_store);

/**
 * create proc file system entry related to the decoder
 */
static int create_files(void)
{
	int ret = device_create_file(etm_device.this_device, &dev_attr_run);
	if (unlikely(ret != 0))
		return ret;	
		
  ret = device_create_file(etm_device.this_device, &dev_attr_etb_length);
  if (unlikely(ret != 0))
    return ret;

  ret = device_create_file(etm_device.this_device, &dev_attr_trace_data);
  if (unlikely(ret != 0))
    return ret;

  ret = device_create_file(etm_device.this_device, &dev_attr_trace_range);
    if (unlikely(ret != 0))
      return ret;
 
  /* run-time enable/disable etm interface */
  ret = device_create_file(etm_device.this_device, &dev_attr_etm0_online);
    if (unlikely(ret != 0))
      return ret;
  ret = device_create_file(etm_device.this_device, &dev_attr_etm1_online);
    if (unlikely(ret != 0))
      return ret;
  ret = device_create_file(etm_device.this_device, &dev_attr_etm2_online);
    if (unlikely(ret != 0))
      return ret;
  ret = device_create_file(etm_device.this_device, &dev_attr_etm3_online);
    if (unlikely(ret != 0))
      return ret;            
  
  return 0;
}

/**
 * delete proc file system entry related to the decoder
 */
static int remove_files(void)
{
	device_remove_file(etm_device.this_device, &dev_attr_run);

	return 0;
}

/** \def ETM_REGS_BASE_CHECK
 * check ETM register base address
 */
/** \def ETM_REGS_BASE_INIT
 * init ETM register base address
 */

/**
 * driver initialization entry point
 */
static int __init etm_init(void)
{
	int ret = -1;
	int i = 0;
   //unsigned int faxi_clk;

	memset(&tracer, sizeof(struct etm_trace_context_t), 0);
	mutex_init(&tracer.mutex);
	mutex_lock(&tracer.mutex);
	tracer.set = 0;
	tracer.log_level = LOG_LEVEL_DEBUG;
	//tracer.log_level = LOG_LEVEL_INFO;
	//tracer.log_level = LOG_LEVEL_NONE;
	p_info("==========init==========\n");

	if (unlikely((ret = misc_register(&etm_device)) != 0)) {
		p_error("misc register failed\n");
		goto out;
	}

	if (unlikely((ret = create_files()) != 0)) {
		p_error("create files failed\n");
		goto deregister;
	}   
  

#define ETM_REGS_BASE_CHECK(n) \
do {\
	if (unlikely(!tracer.etm_regs[n])) {\
		p_error("etm %d ioremap register failed\n", n);\
		ret = -ENOMEM;\
		goto remove; /* TODO */\
	}\
	p_debug("etm %d registers after ioremap is %x\n", n,\
		(unsigned int)tracer.etm_regs[n]);\
} while(0)

#define ETM_REGS_BASE_INIT(n) \
do {\
	tracer.etm_regs[n] = ioremap_nocache(ETM ## n ##_BASE, SIZE_4K);\
	ETM_REGS_BASE_CHECK(n);\
} while (0)

  ETM_REGS_BASE_INIT(0);
  i++;
  ETM_REGS_BASE_INIT(1);
  i++;
  ETM_REGS_BASE_INIT(2);
  i++;
  ETM_REGS_BASE_INIT(3);
  i++;
	
	tracer.etb_regs = ioremap_nocache(ETB_BASE_PHY, SIZE_4K); 
	if (unlikely(!tracer.etb_regs)) {
		p_error("ioremap register failed\n");
		ret = -ENOMEM;
		goto unmap;
	}
	p_debug("etb registers after ioremap is %x\n", (unsigned int)tracer.etb_regs);

	tracer.funnel_regs = ioremap_nocache(FUNNEL_BASE_PHY, SIZE_4K); 
	if (unlikely(!tracer.funnel_regs)) {
		p_error("ioremap register failed\n");
		ret = -ENOMEM;
		goto unmap;
	}
	p_debug("funnel registers after ioremap is %x\n", 	(unsigned int)tracer.funnel_regs);

  #if 1
  ////////////////////////////////////////
  ////////////////////////////////////////
  /* AHBAP_EN to enable master port, then ETR could write the trace to bus */
  mt65xx_reg_sync_writel(ETMLAR_UNLOCK_MAGIC, DEM_Unlock);
  mt65xx_reg_sync_writel(0x1, DEM_BASE + 0x40);

  etb_unlock(&tracer);

  /* Set up ETR memory buffer address */  
  etb_writel(&tracer, ETR_RAM_PHY_BASE, 0x118);
  /* Set up ETR memory buffer size */
  etb_writel(&tracer, ETR_RAM_SIZE, 0x4);

  //Disable ETB capture (ETB_CTL bit0 = 0x0)
  /* For wdt reset */
  cs_cpu_write (ETB_BASE, 0x20, 0x0);

  /* downgrade CA7 to ETM clk */
  //faxi_clk = readl(0xF0000140);
  //faxi_clk &= 0xFFFFFFF0;
  //faxi_clk |= 0x3;
  //printk("[ETM] faxi_clk = %x\n", faxi_clk);
  //writel(faxi_clk, 0xF0000140);
  
  ////////////////////////////////////////
  ////////////////////////////////////////
#endif

  tracer.etb_total_buf_size = etb_readl(&tracer, ETBRDP);
  tracer.state = TRACE_STATE_STOP;

  
  mutex_unlock(&tracer.mutex);
  return ret;

unmap:
remove:
	for(; i>0; i--) {
		iounmap(tracer.etm_regs[i-1]);
		tracer.etm_regs[i-1] = NULL;
	}
  	iounmap(tracer.funnel_regs);
	tracer.funnel_regs = NULL;
  
	iounmap(tracer.etb_regs);
	tracer.etb_regs = NULL;

	remove_files();
deregister:
	misc_deregister(&etm_device);
	
out:
  mutex_unlock(&tracer.mutex);
  return ret;
}

/**
 * driver exit point
 */
static void __exit etm_exit(void)
{
	int ret = 0;
	int i;

	p_info("==========exit==========\n");

	if (unlikely((ret = remove_files()) != 0)) {
		p_warning("remove files failed\n");
	}

	misc_deregister(&etm_device);

	for(i=0; i<ETM_NO; i++) {
		iounmap(tracer.etm_regs[i]);
		tracer.etm_regs[i] = NULL;
	}
  
	iounmap(tracer.etb_regs);
	tracer.etb_regs = NULL;
	iounmap(tracer.funnel_regs);
	tracer.funnel_regs = NULL;
}

module_init(etm_init);
module_exit(etm_exit);

MODULE_AUTHOR("mac.lee");
MODULE_DESCRIPTION("ETM decorder");
MODULE_LICENSE("GPL");

