/*******************************************************************************
* mt_pwm.c PWM Drvier                                                     
*                                                                                             
* Copyright (c) 2012, Media Teck.inc                                           
*                                                                             
* This program is free software; you can redistribute it and/or modify it     
* under the terms and conditions of the GNU General Public Licence,            
* version 2, as publish by the Free Software Foundation.                       
*                                                                              
* This program is distributed and in hope it will be useful, but WITHOUT       
* ANY WARRNTY; without even the implied warranty of MERCHANTABITLITY or        
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for     
* more details.                                                                
*                                                                              
*                                                                              
********************************************************************************
* Author : Changlei Gao (changlei.gao@mediatek.com)                              
********************************************************************************
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/autoconf.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <mach/mt_pwm.h>  
#include <mach/mt_typedefs.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_gpio.h>
#include <mach/irqs.h>
#include <mach/upmu_common.h>

#include <mach/sync_write.h>
#ifdef OUTREG32
  #undef OUTREG32
  #define OUTREG32(x, y) mt65xx_reg_sync_writel(y, x)
#endif

#ifdef PWM_DEBUG
	#define PWMDBG(fmt, args ...) printk(KERN_INFO "pwm %5d: " fmt, __LINE__,##args)
#else
	#define PWMDBG(fmt, args ...)
#endif

#define PWMMSG(fmt, args ...)  printk(KERN_INFO fmt, ##args)

#define PWM_DEVICE "mt-pwm"

#define BLOCK_CLK 52*1024*1024

enum {                                                                                     
	PWM_CON,
	PWM_HDURATION,
	PWM_LDURATION,
	PWM_GDURATION,
	PWM_BUF0_BASE_ADDR,
	PWM_BUF0_SIZE,
	PWM_BUF1_BASE_ADDR,
	PWM_BUF1_SIZE,
	PWM_SEND_DATA0,
	PWM_SEND_DATA1,
	PWM_WAVE_NUM,
	PWM_DATA_WIDTH,		//PWM3&4 SEND_WAVENUM?
	PWM_THRESH,				//PWM3&4 DATA_WIDTH
	PWM_SEND_WAVENUM,	//PWM3&4 THRESH
	PWM_VALID
}PWM_REG_OFF;

U32 PWM_register[PWM_NUM]={
	(PWM_BASE+0x0010),     //PWM1 REGISTER BASE,   15 registers
	(PWM_BASE+0x0050),     //PWM2 register base    15 registers
	(PWM_BASE+0x0090),     //PWM3 register base    15 registers
	(PWM_BASE+0x00d0),     //PWM4 register base    13 registers
	(PWM_BASE+0x0110),     //PWM5 register base    13 registers
	(PWM_BASE+0x0150),     //PWM6 register base    13 registers
	(PWM_BASE+0x0190)      //PWM7 register base    15 registers
};

struct pwm_device {
	const char      *name;
	atomic_t        ref;
	dev_t           devno;
	spinlock_t      lock;
	unsigned long 	power_flag;//bitwise, bit(8):map to MT_CG_PERI0_PWM
	struct device   dev;
};

static int pwm_power_id[] = {
	MT_CG_PERI0_PWM1,
	MT_CG_PERI0_PWM2,
	MT_CG_PERI0_PWM3,
	MT_CG_PERI0_PWM4,
	MT_CG_PERI0_PWM5,
	MT_CG_PERI0_PWM6,
	MT_CG_PERI0_PWM7,
};

static struct pwm_device pwm_dat = {
	.name = PWM_DEVICE,
	.ref = ATOMIC_INIT(0),
	.power_flag = 0,
	.lock = __SPIN_LOCK_UNLOCKED(pwm_dat.lock)
};

static struct pwm_device *pwm_dev = &pwm_dat;

void mt_pwm_power_on(U32 pwm_no, BOOL pmic_pad)
{
	if(0 == pwm_dev->power_flag){
		PWMDBG("enable_clock: main\n");//
		enable_clock(MT_CG_PERI0_PWM, "PWM");
		set_bit(7, &(pwm_dev->power_flag));
	}
	if (!test_bit(pwm_no, &(pwm_dev->power_flag))) {
		PWMDBG("enable_clock: %d\n", pwm_no);//
		if(!pmic_pad){//ap pad
			PWMDBG("ap pad\n");//
			enable_clock(pwm_power_id[pwm_no], "PWM");  //enable clock 
		} else {
			PWMDBG("pmic pad\n");//
			upmu_set_rg_wrp_pwm_pdn(false);
		}
		set_bit(pwm_no,&(pwm_dev->power_flag));
		PWMDBG("enable_clock PWM%d\n", pwm_no+1);
	}
}

void mt_pwm_power_off (U32 pwm_no, BOOL pmic_pad)
{
	if (test_bit(pwm_no, &(pwm_dev->power_flag))) {
		PWMDBG("disable_clock: %d\n", pwm_no);//
		if(!pmic_pad){//ap pad
			PWMDBG("ap pad\n");//
			disable_clock(pwm_power_id[pwm_no], "PWM");  //disable clock 
		} else {//pmic pad
			PWMDBG("pmic pad\n");//
			upmu_set_rg_wrp_pwm_pdn(true);
		}
		clear_bit(pwm_no,&(pwm_dev->power_flag));
		PWMDBG("disable_clock PWM%d\n", pwm_no+1);
	}
	if(0x80 == pwm_dev->power_flag){
		PWMDBG("disable_clock: main\n");//
		disable_clock(MT_CG_PERI0_PWM, "PWM");
		clear_bit(7, &(pwm_dev->power_flag));
	}
}

S32 mt_pwm_sel_pmic(U32 pwm_no)
{
	unsigned long flags;
	struct pwm_device *dev = pwm_dev;

	if ( pwm_no <= PWM4 ) {
		PWMDBG ( "PWM1~PWM4 not support pmic_pad\n" );
		return -EEXCESSPWMNO;
	} 

	spin_lock_irqsave ( &dev->lock,flags );
	SETREG32(PWM_SEL_PMIC, 1 << (pwm_no-PWM5));
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

S32 mt_pwm_sel_ap(U32 pwm_no)
{
	unsigned long flags;
	struct pwm_device *dev = pwm_dev;

	if ( pwm_no <= PWM4 ) {
		PWMDBG ( "PWM1~PWM4 not support pmic_pad\n" );
		return -EEXCESSPWMNO;
	} 

	spin_lock_irqsave ( &dev->lock,flags );
	CLRREG32(PWM_SEL_PMIC, 1 << (pwm_no-PWM5));
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/*******************************************************
*   Set PWM_ENABLE register bit to enable pwm1~pwm7
*
********************************************************/
S32 mt_set_pwm_enable(U32 pwm_no) 
{
	unsigned long flags;

	struct pwm_device *dev = pwm_dev;

	if ( !dev ) {
		PWMDBG("dev is not valid!\n");
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number is not between PWM1~PWM7\n" );
		return -EEXCESSPWMNO;
	} 

	spin_lock_irqsave ( &dev->lock,flags );
	SETREG32(PWM_ENABLE, 1 << pwm_no);
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}


/*******************************************************/
S32 mt_set_pwm_disable ( U32 pwm_no )  
{
	unsigned long flags;
	struct pwm_device *dev = pwm_dev;

	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number is not between PWM1~PWM7\n" );
		return -EEXCESSPWMNO;
	}

	spin_lock_irqsave ( &dev->lock, flags );	
	CLRREG32 ( PWM_ENABLE, 1 << pwm_no );
	spin_unlock_irqrestore ( &dev->lock, flags );

	mdelay(1);

	return RSUCCESS;
}

void mt_pwm_disable(U32 pwm_no, BOOL pmic_pad)
{
	mt_set_pwm_disable(pwm_no);
	mt_pwm_power_off(pwm_no, pmic_pad);
}

void mt_set_pwm_enable_seqmode(void)
{
	unsigned long flags;
	struct pwm_device *dev = pwm_dev;

	if ( !dev ) {
		PWMDBG ( "dev is not valid \n" );
		return ;
    }

	spin_lock_irqsave ( &dev->lock,flags );
	SETREG32 ( PWM_ENABLE, 1 << PWM_ENABLE_SEQ_OFFSET );
	spin_unlock_irqrestore ( &dev->lock, flags );
}

void mt_set_pwm_disable_seqmode(void)
{
	unsigned long flags;
	struct pwm_device *dev = pwm_dev;

	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return ;
	}

	spin_lock_irqsave ( &dev->lock, flags );
	CLRREG32 ( PWM_ENABLE,1 << PWM_ENABLE_SEQ_OFFSET );
	spin_unlock_irqrestore ( &dev->lock, flags );
}

S32 mt_set_pwm_test_sel(U32 val)  //val as 0 or 1
{
	unsigned long flags;
	struct pwm_device *dev = pwm_dev;

	if ( !dev ) {
		PWMDBG ( "dev is not pwm_dev \n" );
		return -EINVALID;
	}

	spin_lock_irqsave ( &dev->lock, flags );
	if (val == TEST_SEL_TRUE)
		SETREG32 ( PWM_ENABLE, 1 << PWM_ENABLE_TEST_SEL_OFFSET );
	else if ( val == TEST_SEL_FALSE )
		CLRREG32 ( PWM_ENABLE, 1 << PWM_ENABLE_TEST_SEL_OFFSET );
	else
		goto err;	
	spin_unlock_irqrestore ( &dev->lock, flags );
	return RSUCCESS;

err:
	spin_unlock_irqrestore ( &dev->lock, flags );
	return -EPARMNOSUPPORT;
}

S32 mt_set_pwm_clk ( U32 pwm_no, U32 clksrc, U32 div )
{
	unsigned long flags;
	U32 reg_con;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}

	if ( div >= CLK_DIV_MAX ) {
		PWMDBG ("division excesses CLK_DIV_MAX\n");
		return -EPARMNOSUPPORT;
	}

	if (clksrc > CLK_BLOCK_BY_1625_OR_32K) {
		PWMDBG("clksrc excesses CLK_BLOCK_BY_1625_OR_32K\n");
		return -EPARMNOSUPPORT;
	}

	reg_con = PWM_register [pwm_no] + 4* PWM_CON;

	spin_lock_irqsave ( &dev->lock, flags );
	MASKREG32 ( reg_con, PWM_CON_CLKDIV_MASK, div );
	if (clksrc == CLK_BLOCK)
		CLRREG32 ( reg_con, 1 << PWM_CON_CLKSEL_OFFSET );
	else if (clksrc == CLK_BLOCK_BY_1625_OR_32K)
		SETREG32 ( reg_con, 1 << PWM_CON_CLKSEL_OFFSET );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

S32 mt_get_pwm_clk ( U32 pwm_no )
{
	unsigned long flags;
	S32 clk, clksrc, clkdiv;
	U32 reg_con, reg_val,reg_en;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ("dev is not valid \n");
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}

	reg_con = PWM_register[pwm_no] + 4*PWM_CON;

	spin_lock_irqsave ( &dev->lock, flags);
	reg_val = INREG32 (reg_con);
	reg_en = INREG32 (PWM_ENABLE);
	spin_unlock_irqrestore ( &dev->lock, flags );

	if ( ( ( reg_val & PWM_CON_CLKSEL_MASK ) >> PWM_CON_CLKSEL_OFFSET ) == 1 )
		if ( ((reg_en &PWM_CON_OLD_MODE_MASK) >> PWM_CON_OLD_MODE_OFFSET ) == 1)
			clksrc = 32*1024;
		else clksrc = BLOCK_CLK;
	else
		clksrc = BLOCK_CLK/1625;

	clkdiv = 2 << ( reg_val & PWM_CON_CLKDIV_MASK);
	if ( clkdiv <= 0 ) {
		PWMDBG ( "clkdiv less zero, not valid \n" );
		return -ERROR;
	}

	clk = clksrc/clkdiv;
	PWMDBG ( "CLK is :%d\n", clk );

	return clk;
}

/******************************************
* Set PWM_CON register data source
* pwm_no: pwm1~pwm7(0~6)
*val: 0 is fifo mode
*       1 is memory mode
*******************************************/

S32 mt_set_pwm_con_datasrc ( U32 pwm_no, U32 val )
{
	unsigned long flags;
	U32 reg_con;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG (" pwm deivce doesn't exist\n");
		return -EINVALID;
	}
		
	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ("pwm number excesses PWM_MAX \n");
		return -EEXCESSPWMNO;
	}
	
	reg_con = PWM_register[pwm_no] + 4*PWM_CON;

	spin_lock_irqsave ( &dev->lock, flags );
	if ( val == PWM_FIFO )
		CLRREG32 ( reg_con, 1 << PWM_CON_SRCSEL_OFFSET );
	else if ( val == MEMORY )
		SETREG32 ( reg_con, 1 << PWM_CON_SRCSEL_OFFSET );
	else 
		goto err;
	spin_unlock_irqrestore ( &dev->lock, flags );
	return RSUCCESS;

err:
	spin_unlock_irqrestore ( &dev->lock, flags );
	return -EPARMNOSUPPORT;
}


/************************************************
*  set the PWM_CON register
* pwm_no : pwm1~pwm7 (0~6)
* val: 0 is period mode
*        1 is random mode
*
***************************************************/
S32 mt_set_pwm_con_mode( U32 pwm_no, U32 val )
{
	unsigned long flags;
	U32 reg_con;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ("dev is not valid \n");
		return -EINVALID;
	}
	
	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}
	
	reg_con = PWM_register[pwm_no] + 4*PWM_CON;

	spin_lock_irqsave ( &dev->lock, flags );
	if ( val == PERIOD )
		CLRREG32 ( reg_con, 1 << PWM_CON_MODE_OFFSET );
	else if (val == RAND)
		SETREG32 ( reg_con, 1 << PWM_CON_MODE_OFFSET );
	else
		goto err;
	spin_unlock_irqrestore ( &dev->lock, flags );
	return RSUCCESS;

err:
	spin_unlock_irqrestore ( &dev->lock, flags );
	return -EPARMNOSUPPORT;
}

/***********************************************
*Set PWM_CON register, idle value bit 
* val: 0 means that  idle state is not put out.
*       1 means that idle state is put out
*
*      IDLE_FALSE: 0
*      IDLE_TRUE: 1
***********************************************/
S32 mt_set_pwm_con_idleval(U32 pwm_no, U16 val)
{
	unsigned long flags;
	U32 reg_con;

	struct pwm_device *dev = pwm_dev;
	if ( ! dev ) {
		PWMDBG ( "dev is not valid \n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}
	
	reg_con = PWM_register[pwm_no] + 4*PWM_CON;

	spin_lock_irqsave ( &dev->lock, flags );
	if ( val == IDLE_TRUE )
		SETREG32 ( reg_con,1 << PWM_CON_IDLE_VALUE_OFFSET );
	else if ( val == IDLE_FALSE )
		CLRREG32 ( reg_con, 1 << PWM_CON_IDLE_VALUE_OFFSET );
	else 
		goto err;
	
	spin_unlock_irqrestore ( &dev->lock, flags );
	return RSUCCESS;

err:
	spin_unlock_irqrestore ( &dev->lock, flags );
	return -EPARMNOSUPPORT;
}

/*********************************************
* Set PWM_CON register guardvalue bit
*  val: 0 means guard state is not put out.
*        1 mens guard state is put out.
*
*    GUARD_FALSE: 0
*    GUARD_TRUE: 1
**********************************************/
S32 mt_set_pwm_con_guardval(U32 pwm_no, U16 val)
{
	unsigned long flags;
	U32 reg_con;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ("pwm number excesses PWM_MAX \n");
		return -EEXCESSPWMNO;
	}
	
	reg_con = PWM_register[pwm_no] + 4*PWM_CON;

	spin_lock_irqsave ( &dev->lock, flags );
	if ( val == GUARD_TRUE )
		SETREG32 ( reg_con, 1 << PWM_CON_GUARD_VALUE_OFFSET );
	else if ( val == GUARD_FALSE )
		CLRREG32 ( reg_con, 1 << PWM_CON_GUARD_VALUE_OFFSET );
	else 
		goto err;
	
	spin_unlock_irqrestore ( &dev->lock, flags );
	return RSUCCESS;

err:
	spin_unlock_irqrestore ( &dev->lock, flags );
	return -EPARMNOSUPPORT;
}

/*************************************************
* Set PWM_CON register stopbits
*stop bits should be less then 0x3f
*
**************************************************/
S32 mt_set_pwm_con_stpbit(U32 pwm_no, U32 stpbit, U32 srcsel )
{
	unsigned long flags;
	U32 reg_con;
	
	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid \n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}

	reg_con = PWM_register[pwm_no] + 4*PWM_CON;

	if (srcsel == PWM_FIFO) {
		if ( stpbit > 0x3f ) {
			PWMDBG ( "stpbit execesses the most of 0x3f in fifo mode\n" );
			return -EPARMNOSUPPORT;
		}
	}else if (srcsel == MEMORY){
		if ( stpbit > 0x1f) {
			PWMDBG ("stpbit excesses the most of 0x1f in memory mode\n");
			return -EPARMNOSUPPORT;
		}
	}

	spin_lock_irqsave ( &dev->lock, flags );
	if ( srcsel == PWM_FIFO )
		MASKREG32 ( reg_con, PWM_CON_STOP_BITS_MASK, stpbit << PWM_CON_STOP_BITS_OFFSET);
	if ( srcsel == MEMORY )
		MASKREG32 ( reg_con, PWM_CON_STOP_BITS_MASK & (0x1f << PWM_CON_STOP_BITS_OFFSET), stpbit << PWM_CON_STOP_BITS_OFFSET);
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/*****************************************************
*Set PWM_CON register oldmode bit
* val: 0 means disable oldmode
*        1 means enable oldmode
*
*      OLDMODE_DISABLE: 0
*      OLDMODE_ENABLE: 1
******************************************************/

S32 mt_set_pwm_con_oldmode ( U32 pwm_no, U32 val )
{
	unsigned long flags;
	U32 reg_con;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid \n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ("pwm number excesses PWM_MAX \n");
		return -EEXCESSPWMNO;
	}

	reg_con = PWM_register[pwm_no] + 4*PWM_CON;

	spin_lock_irqsave ( &dev->lock, flags );
	if ( val == OLDMODE_DISABLE )
		CLRREG32 ( reg_con, 1 << PWM_CON_OLD_MODE_OFFSET );
	else if ( val == OLDMODE_ENABLE )
		SETREG32 ( reg_con, 1 << PWM_CON_OLD_MODE_OFFSET );
	else 
		goto err;
	
	spin_unlock_irqrestore ( &dev->lock, flags );
	return RSUCCESS;

err:
	spin_unlock_irqrestore ( &dev->lock, flags );
	return -EPARMNOSUPPORT;
}

/***********************************************************
* Set PWM_HIDURATION register
*
*************************************************************/

S32 mt_set_pwm_HiDur(U32 pwm_no, U16 DurVal)  //only low 16 bits are valid
{
	unsigned long flags;
	U32 reg_HiDur;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid \n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX\n" );
		return -EEXCESSPWMNO;
	}
	
	reg_HiDur = PWM_register[pwm_no]+4*PWM_HDURATION;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_HiDur, DurVal);	
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/************************************************
* Set PWM Low Duration register
*************************************************/
S32 mt_set_pwm_LowDur (U32 pwm_no, U16 DurVal)
{
	unsigned long flags;
	U32 reg_LowDur;
	
	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid \n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	reg_LowDur = PWM_register[pwm_no] + 4*PWM_LDURATION;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_LowDur, DurVal );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/***************************************************
* Set PWM_GUARDDURATION register
* pwm_no: PWM1~PWM7(0~6)
* DurVal:   the value of guard duration
****************************************************/
S32 mt_set_pwm_GuardDur ( U32 pwm_no, U16 DurVal )
{
	unsigned long flags;
	U32 reg_GuardDur;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ("pwm number excesses PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	reg_GuardDur = PWM_register[pwm_no] + 4*PWM_GDURATION;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_GuardDur, DurVal );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/*****************************************************
* Set pwm_buf0_addr register
* pwm_no: pwm1~pwm7 (0~6)
* addr: data address
*****************************************************
S32 mt_set_pwm_buf0_addr (U32 pwm_no, U32 addr )
{
	unsigned long flags;
	U32 reg_buff0_addr;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}

	reg_buff0_addr = PWM_register[pwm_no] + 4 * PWM_BUF0_BASE_ADDR;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_buff0_addr, addr );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}*/

/*****************************************************
* Set pwm_buf0_size register
* pwm_no: pwm1~pwm7 (0~6)
* size: size of data
*****************************************************
S32 mt_set_pwm_buf0_size ( U32 pwm_no, U16 size)
{
	unsigned long flags;
	U32 reg_buff0_size;
	
	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}

	reg_buff0_size = PWM_register[pwm_no] + 4* PWM_BUF0_SIZE;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_buff0_size, size );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;

}*/

/*****************************************************
* Set pwm_buf1_addr register
* pwm_no: pwm1~pwm7 (0~6)
* addr: data address
*****************************************************
S32 mt_set_pwm_buf1_addr (U32 pwm_no, U32 addr )
{
	unsigned long flags;
	U32 reg_buff1_addr;
	
	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid \n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}

	reg_buff1_addr = PWM_register[pwm_no] + 4 * PWM_BUF1_BASE_ADDR;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_buff1_addr, addr );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}*/

/*****************************************************
* Set pwm_buf1_size register
* pwm_no: pwm1~pwm7 (0~6)
* size: size of data
*****************************************************
S32 mt_set_pwm_buf1_size ( U32 pwm_no, U16 size)
{
	unsigned long flags;
	U32 reg_buff1_size;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}

	reg_buff1_size = PWM_register[pwm_no] + 4* PWM_BUF1_SIZE;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_buff1_size, size );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;

}*/

/*****************************************************
* Set pwm_send_data0 register
* pwm_no: pwm1~pwm7 (0~6)
* data: the data in the register
******************************************************/
S32 mt_set_pwm_send_data0 ( U32 pwm_no, U32 data )
{
	unsigned long flags;
	U32 reg_data0;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}

	reg_data0 = PWM_register[pwm_no] + 4 * PWM_SEND_DATA0;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_data0, data );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/*****************************************************
* Set pwm_send_data1 register
* pwm_no: pwm1~pwm7 (0~6)
* data: the data in the register
******************************************************/
S32 mt_set_pwm_send_data1 ( U32 pwm_no, U32 data )
{
	unsigned long flags;
	U32 reg_data1;

	struct pwm_device *dev = pwm_dev; 
	if ( !dev ) {
		PWMDBG ( "dev is not valid \n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX \n" );
		return -EEXCESSPWMNO;
	}

	reg_data1 = PWM_register[pwm_no] + 4 * PWM_SEND_DATA1;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_data1, data );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/*****************************************************
* Set pwm_wave_num register
* pwm_no: pwm1~pwm7 (0~6)
* num:the wave number
******************************************************/
S32 mt_set_pwm_wave_num ( U32 pwm_no, U16 num )
{
	unsigned long flags;
	U32 reg_wave_num;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ("dev is not valid\n");
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX\n" );
		return -EEXCESSPWMNO;
	}

	reg_wave_num = PWM_register[pwm_no] + 4 * PWM_WAVE_NUM;

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_wave_num, num );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/*****************************************************
* Set pwm_data_width register. 
* This is only for old mode
* pwm_no: pwm1~pwm7 (0~6)
* width: set the guard value in the old mode
******************************************************/
S32 mt_set_pwm_data_width ( U32 pwm_no, U16 width )
{
	unsigned long flags;
	U32 reg_data_width;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX\n" );
		return -EEXCESSPWMNO;
	}

	if (pwm_no == PWM3 || pwm_no == PWM4) {
		reg_data_width = PWM_register[pwm_no] + 4 * PWM_THRESH;
	} else {
		reg_data_width = PWM_register[pwm_no] + 4 * PWM_DATA_WIDTH;
	}

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_data_width, width );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/*****************************************************
* Set pwm_thresh register
* pwm_no: pwm1~pwm7 (0~6)
* thresh:  the thresh of the wave
******************************************************/
S32 mt_set_pwm_thresh ( U32 pwm_no, U16 thresh )
{
	unsigned long flags;
	U32 reg_thresh;

	struct pwm_device *dev = pwm_dev; 
	if ( !dev ) {
		PWMDBG ( "dev is not valid \n" );
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( " pwm number excesses PWM_MAX \n");
		return -EEXCESSPWMNO;
	}

	if (pwm_no == PWM3 || pwm_no == PWM4) {
		reg_thresh = PWM_register[pwm_no] + 4 * PWM_SEND_WAVENUM;
	} else {
		reg_thresh = PWM_register[pwm_no] + 4 * PWM_THRESH;
	}

	spin_lock_irqsave ( &dev->lock, flags );
	OUTREG32 ( reg_thresh, thresh );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

/*****************************************************
* Set pwm_send_wavenum register
* pwm_no: pwm1~pwm7 (0~6)
*
******************************************************/
S32 mt_get_pwm_send_wavenum ( U32 pwm_no )
{
	unsigned long flags;
	U32 reg_send_wavenum;
	S32 wave_num;

	struct pwm_device *dev = pwm_dev;
	if ( ! dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EBADADDR;
	}
	
	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX\n" );
		return -EEXCESSPWMNO;
	}

	if ( (pwm_no <=PWM2)||(pwm_no == PWM6) )
		reg_send_wavenum = PWM_register[pwm_no] + 4 * PWM_SEND_WAVENUM;
	else
		reg_send_wavenum = PWM_register[pwm_no] + 4 * (PWM_SEND_WAVENUM - 2);  //pwm4,pwm5,pwm6 has no data width and thresh register

	spin_lock_irqsave ( &dev->lock, flags );
	wave_num = INREG32 ( reg_send_wavenum );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return wave_num;
}

/*****************************************************
* Set pwm_send_data1 register
* pwm_no: pwm1~pwm7 (0~6)
* buf_valid_bit: 
* for buf0: bit0 and bit1 should be set 1. 
* for buf1: bit2 and bit3 should be set 1.
*****************************************************
S32 mt_set_pwm_valid ( U32 pwm_no, U32 buf_valid_bit )   //set 0  for BUF0 bit or set 1 for BUF1 bit
{
	unsigned long flags;
	U32 reg_valid;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ("dev is not valid\n");
		return -EINVALID;
	}

	if ( pwm_no >= PWM_MAX ) {
		PWMDBG ( "pwm number excesses PWM_MAX\n" );
		return -EEXCESSPWMNO;
	}

	if ( !buf_valid_bit>= BUF_EN_MAX) {
		PWMDBG ( "inavlid bit \n" );
		return -EPARMNOSUPPORT;
	}

	if ( (pwm_no <= PWM2)||(pwm_no == PWM6))
		reg_valid = PWM_register[pwm_no] + 4 * PWM_VALID;
	else
		reg_valid = PWM_register[pwm_no] + 4* (PWM_VALID -2);

	spin_lock_irqsave ( &dev->lock, flags );
	SETREG32 ( reg_valid, 1 << buf_valid_bit );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}*/

/*************************************************
*  set PWM4_delay when using SEQ mode
*
*************************************************
S32 mt_set_pwm_delay_duration(U32 pwm_delay_reg, U16 val)
{
	unsigned long flags;
	struct pwm_device *pwmdev = pwm_dev;

	if (!pwmdev) {
		PWMDBG( "device doesn't exist\n" );
		return -EBADADDR;
	}

	spin_lock_irqsave ( &pwmdev->lock, flags );	
	MASKREG32 ( pwm_delay_reg, PWM_DELAY_DURATION_MASK, val );
	spin_unlock_irqrestore ( &pwmdev->lock, flags );

	return RSUCCESS;
}*/

/*******************************************************
* Set pwm delay clock
* 
*
*******************************************************
S32 mt_set_pwm_delay_clock (U32 pwm_delay_reg, U32 clksrc)
{
	unsigned long flags;
	struct pwm_device *pwmdev = pwm_dev;
	if ( ! pwmdev ) {
		PWMDBG ( "device doesn't exist\n" );
		return -EBADADDR;
	}

	spin_lock_irqsave ( &pwmdev->lock, flags );
	MASKREG32 (pwm_delay_reg, PWM_DELAY_CLK_MASK, clksrc );
	spin_unlock_irqrestore (&pwmdev->lock, flags);

	return RSUCCESS;
}*/

/*******************************************
* Set intr enable register
* pwm_intr_enable_bit: the intr bit, 
*
*********************************************/
S32 mt_set_intr_enable(U32 pwm_intr_enable_bit)
{
	unsigned long flags;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}
	
	if (pwm_intr_enable_bit >= PWM_INT_ENABLE_BITS_MAX) {
		PWMDBG (" pwm inter enable bit is not right.\n"); 
		return -EEXCESSBITS; 
	}
	
	spin_lock_irqsave ( &dev->lock, flags );
	SETREG32 ( PWM_INT_ENABLE, 1 << pwm_intr_enable_bit );
	spin_lock_irqsave (&dev->lock, flags );

	return RSUCCESS;
}

/*****************************************************
* Set intr status register
* pwm_no: pwm1~pwm7 (0~6)
* pwm_intr_status_bit
******************************************************/
S32 mt_get_intr_status(U32 pwm_intr_status_bit)
{
	unsigned long flags;
	int ret;

	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ( "dev is not valid\n" );
		return -EINVALID;
	}

	if ( pwm_intr_status_bit >= PWM_INT_STATUS_BITS_MAX ) {
		PWMDBG ( "status bit excesses PWM_INT_STATUS_BITS_MAX\n" );
		return -EEXCESSBITS;
	}
	
	spin_lock_irqsave ( &dev->lock, flags );
	ret = INREG32 ( PWM_INT_STATUS );
	ret = ( ret >> pwm_intr_status_bit ) & 0x01;
	spin_unlock_irqrestore ( &dev->lock, flags );

	return ret;
}

/*****************************************************
* Set intr ack register
* pwm_no: pwm1~pwm7 (0~6)
* pwm_intr_ack_bit
******************************************************/
S32 mt_set_intr_ack ( U32 pwm_intr_ack_bit )
{
	unsigned long flags;
	struct pwm_device *dev = pwm_dev;
	if ( !dev ) {
		PWMDBG ("dev is not valid\n");
		return -EINVALID;
	}

	if ( pwm_intr_ack_bit >= PWM_INT_ACK_BITS_MAX ) {
		PWMDBG ( "ack bit excesses PWM_INT_ACK_BITS_MAX\n" ); 
		return -EEXCESSBITS;
	}

	spin_lock_irqsave ( &dev->lock, flags );
	SETREG32 ( PWM_INT_ACK, 1 << pwm_intr_ack_bit );
	spin_unlock_irqrestore ( &dev->lock, flags );

	return RSUCCESS;
}

S32 pwm_set_easy_config ( struct pwm_easy_config *conf)
{

	U32 duty = 0;
	U16 duration = 0;
	U32 data_AllH=0xffffffff;
	U32 data0 = 0;
	U32 data1 = 0;
	
	if ( conf->pwm_no >= PWM_MAX || conf->pwm_no < PWM_MIN ) {
		PWMDBG("pwm number excess PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

	if ((conf->clk_div >= CLK_DIV_MAX) || (conf->clk_div < CLK_DIV_MIN )) {
		PWMDBG ( "PWM clock division invalid\n" );
		return -EINVALID;
	}
	
	if ( ( conf ->clk_src >= PWM_CLK_SRC_INVALID) || (conf->clk_src < PWM_CLK_SRC_MIN) ) {
		PWMDBG ("PWM clock source invalid\n");
		return -EINVALID;
	}

	if  ( conf->duty < 0 ) {
		PWMDBG("duty parameter is invalid\n");
		return -EINVALID;
	}

	PWMDBG("pwm_set_easy_config\n");

	if ( conf->duty == 0 ) {
		mt_set_pwm_disable (conf->pwm_no);
		mt_pwm_power_off(conf->pwm_no, conf->pmic_pad);
		return RSUCCESS;
	}
	
	duty = conf->duty;
	duration = conf->duration;
	
	switch ( conf->clk_src ) {
		case PWM_CLK_OLD_MODE_BLOCK:
		case PWM_CLK_OLD_MODE_32K:
			if ( duration > 8191 || duration < 0 ) {
				PWMDBG ( "duration invalid parameter\n" );
				return -EPARMNOSUPPORT;
			}
			if ( duration < 10 ) 
				duration = 10;
			break;
			
		case PWM_CLK_NEW_MODE_BLOCK:
		case PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625:
			if ( duration > 65535 || duration < 0 ){
				PWMDBG ("invalid paramters\n");
				return -EPARMNOSUPPORT;
			}
			break;
		default:
			PWMDBG("invalid clock source\n");
			return -EPARMNOSUPPORT;
	}
	
	if ( duty > 100 ) 
		duty = 100;

	if ( duty > 50 ){
		data0 = data_AllH;
		data1 = data_AllH >> ((PWM_NEW_MODE_DUTY_TOTAL_BITS * (100 - duty ))/100 );
	}else {
		data0 = data_AllH >> ((PWM_NEW_MODE_DUTY_TOTAL_BITS * (50 - duty))/100);
		PWMDBG("DATA0 :0x%x\n",data0);
		data1 = 0;
	}

	mt_pwm_power_on(conf->pwm_no, conf->pmic_pad);
	if(conf->pmic_pad){
		mt_pwm_sel_pmic(conf->pwm_no);
	}
	mt_set_pwm_con_guardval(conf->pwm_no, GUARD_TRUE);

	switch ( conf->clk_src ) {
		case PWM_CLK_OLD_MODE_32K:
			mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_ENABLE);
			mt_set_pwm_clk ( conf->pwm_no, CLK_BLOCK_BY_1625_OR_32K, conf->clk_div);
			break;

		case PWM_CLK_OLD_MODE_BLOCK:
			mt_set_pwm_con_oldmode (conf->pwm_no, OLDMODE_ENABLE );
			mt_set_pwm_clk ( conf->pwm_no, CLK_BLOCK, conf->clk_div );
			break;

		case PWM_CLK_NEW_MODE_BLOCK:
			mt_set_pwm_con_oldmode (conf->pwm_no, OLDMODE_DISABLE );
			mt_set_pwm_clk ( conf->pwm_no, CLK_BLOCK , conf->clk_div );
			mt_set_pwm_con_datasrc( conf->pwm_no, PWM_FIFO);
			mt_set_pwm_con_stpbit ( conf->pwm_no, 0x3f, PWM_FIFO );
			break;

		case PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625:
			mt_set_pwm_con_oldmode (conf->pwm_no,  OLDMODE_DISABLE );
			mt_set_pwm_clk ( conf->pwm_no, CLK_BLOCK_BY_1625_OR_32K, conf->clk_div );
			mt_set_pwm_con_datasrc( conf->pwm_no, PWM_FIFO);
			mt_set_pwm_con_stpbit ( conf->pwm_no, 0x3f, PWM_FIFO );
			break;

		default:
			break;
		}
	PWMDBG("The duration is:%x\n", duration);
	PWMDBG("The data0 is:%x\n",data0);
	PWMDBG("The data1 is:%x\n",data1);
	mt_set_pwm_HiDur ( conf->pwm_no, duration );
	mt_set_pwm_LowDur (conf->pwm_no, duration );
	mt_set_pwm_GuardDur (conf->pwm_no, 0 );
//	mt_set_pwm_buf0_addr (conf->pwm_no, 0 );
//	mt_set_pwm_buf0_size( conf->pwm_no, 0 );
//	mt_set_pwm_buf1_addr (conf->pwm_no, 0 );
//	mt_set_pwm_buf1_size (conf->pwm_no, 0 );
	mt_set_pwm_send_data0 (conf->pwm_no, data0 );
	mt_set_pwm_send_data1 (conf->pwm_no, data1 );
	mt_set_pwm_wave_num (conf->pwm_no, 0 );

//	if ( conf->pwm_no <= PWM2 || conf->pwm_no == PWM6)
//	{
	mt_set_pwm_data_width (conf->pwm_no, duration );
	mt_set_pwm_thresh ( conf->pwm_no, (( duration * conf->duty)/100));
//		mt_set_pwm_valid (conf->pwm_no, BUF0_EN_VALID );
//		mt_set_pwm_valid ( conf->pwm_no, BUF1_EN_VALID );
		
//	}

	mb();
	mt_set_pwm_enable ( conf->pwm_no );
	PWMDBG("mt_set_pwm_enable\n");

	return RSUCCESS;
	
}

EXPORT_SYMBOL(pwm_set_easy_config);
	
S32 pwm_set_spec_config(struct pwm_spec_config *conf)
{

	if ( conf->pwm_no >= PWM_MAX ) {
		PWMDBG("pwm number excess PWM_MAX\n");
		return -EEXCESSPWMNO;
	}

       if ( ( conf->mode >= PWM_MODE_INVALID )||(conf->mode < PWM_MODE_MIN )) {
	   	PWMDBG ( "PWM mode invalid \n" );
		return -EINVALID;
       }

	if ( ( conf ->clk_src >= PWM_CLK_SRC_INVALID) || (conf->clk_src < PWM_CLK_SRC_MIN) ) {
		PWMDBG ("PWM clock source invalid\n");
		return -EINVALID;
	}

	if ((conf->clk_div >= CLK_DIV_MAX) || (conf->clk_div < CLK_DIV_MIN )) {
		PWMDBG ( "PWM clock division invalid\n" );
		return -EINVALID;
	}

	if ( (conf->mode == PWM_MODE_OLD &&
			(conf->clk_src == PWM_CLK_NEW_MODE_BLOCK|| conf->clk_src == PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625)) 
		||(conf->mode != PWM_MODE_OLD &&
			(conf->clk_src == PWM_CLK_OLD_MODE_32K || conf->clk_src == PWM_CLK_OLD_MODE_BLOCK)) ) {

		PWMDBG ( "parameters match error\n" );
		return -ERROR;
	}

	mt_pwm_power_on(conf->pwm_no, conf->pmic_pad);
	if(conf->pmic_pad){
		mt_pwm_sel_pmic(conf->pwm_no);
	}

	switch (conf->mode ) {
		case PWM_MODE_OLD:
			PWMDBG("PWM_MODE_OLD\n");
			mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_ENABLE);
			mt_set_pwm_con_idleval(conf->pwm_no, conf->PWM_MODE_OLD_REGS.IDLE_VALUE);
			mt_set_pwm_con_guardval (conf->pwm_no, conf->PWM_MODE_OLD_REGS.GUARD_VALUE);
			mt_set_pwm_GuardDur (conf->pwm_no, conf->PWM_MODE_OLD_REGS.GDURATION);
			mt_set_pwm_wave_num(conf->pwm_no, conf->PWM_MODE_OLD_REGS.WAVE_NUM);
			mt_set_pwm_data_width(conf->pwm_no, conf->PWM_MODE_OLD_REGS.DATA_WIDTH);
			mt_set_pwm_thresh(conf->pwm_no, conf->PWM_MODE_OLD_REGS.THRESH);
			PWMDBG ("PWM set old mode finish\n");
			break;
		case PWM_MODE_FIFO:
			PWMDBG("PWM_MODE_FIFO\n");
			mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_DISABLE);
			mt_set_pwm_con_datasrc(conf->pwm_no, PWM_FIFO);
			mt_set_pwm_con_mode (conf->pwm_no, PERIOD);
			mt_set_pwm_con_idleval(conf->pwm_no, conf->PWM_MODE_FIFO_REGS.IDLE_VALUE);
			mt_set_pwm_con_guardval (conf->pwm_no, conf->PWM_MODE_FIFO_REGS.GUARD_VALUE);
			mt_set_pwm_HiDur (conf->pwm_no, conf->PWM_MODE_FIFO_REGS.HDURATION);
			mt_set_pwm_LowDur (conf->pwm_no, conf->PWM_MODE_FIFO_REGS.LDURATION);
			mt_set_pwm_GuardDur (conf->pwm_no, conf->PWM_MODE_FIFO_REGS.GDURATION);
			mt_set_pwm_send_data0 (conf->pwm_no, conf->PWM_MODE_FIFO_REGS.SEND_DATA0);
			mt_set_pwm_send_data1 (conf->pwm_no, conf->PWM_MODE_FIFO_REGS.SEND_DATA1);
			mt_set_pwm_wave_num(conf->pwm_no, conf->PWM_MODE_FIFO_REGS.WAVE_NUM);
			mt_set_pwm_con_stpbit(conf->pwm_no, conf->PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE,PWM_FIFO);
			break;
/*		case PWM_MODE_MEMORY:
			PWMDBG("PWM_MODE_MEMORY\n");
			mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_DISABLE);
			mt_set_pwm_con_datasrc(conf->pwm_no, MEMORY);
			mt_set_pwm_con_mode (conf->pwm_no, PERIOD);
			mt_set_pwm_con_idleval(conf->pwm_no, conf->PWM_MODE_MEMORY_REGS.IDLE_VALUE);
			mt_set_pwm_con_guardval (conf->pwm_no, conf->PWM_MODE_MEMORY_REGS.GUARD_VALUE);
			mt_set_pwm_HiDur (conf->pwm_no, conf->PWM_MODE_MEMORY_REGS.HDURATION);
			mt_set_pwm_LowDur (conf->pwm_no, conf->PWM_MODE_MEMORY_REGS.LDURATION);
			mt_set_pwm_GuardDur (conf->pwm_no, conf->PWM_MODE_MEMORY_REGS.GDURATION);
			mt_set_pwm_buf0_addr(conf->pwm_no, (U32)conf->PWM_MODE_MEMORY_REGS.BUF0_BASE_ADDR);
			mt_set_pwm_buf0_size (conf->pwm_no, conf->PWM_MODE_MEMORY_REGS.BUF0_SIZE);
			mt_set_pwm_wave_num(conf->pwm_no, conf->PWM_MODE_MEMORY_REGS.WAVE_NUM);
			mt_set_pwm_con_stpbit(conf->pwm_no, conf->PWM_MODE_MEMORY_REGS.STOP_BITPOS_VALUE,MEMORY);
			
			break;
		case PWM_MODE_RANDOM:
			PWMDBG("PWM_MODE_RANDOM\n");
			mt_set_pwm_disable(conf->pwm_no);
			mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_DISABLE);
			mt_set_pwm_con_datasrc(conf->pwm_no, MEMORY);
			mt_set_pwm_con_mode (conf->pwm_no, RAND);
			mt_set_pwm_con_idleval(conf->pwm_no, conf->PWM_MODE_RANDOM_REGS.IDLE_VALUE);
			mt_set_pwm_con_guardval (conf->pwm_no, conf->PWM_MODE_RANDOM_REGS.GUARD_VALUE);
			mt_set_pwm_HiDur (conf->pwm_no, conf->PWM_MODE_RANDOM_REGS.HDURATION);
			mt_set_pwm_LowDur (conf->pwm_no, conf->PWM_MODE_RANDOM_REGS.LDURATION);
			mt_set_pwm_GuardDur (conf->pwm_no, conf->PWM_MODE_RANDOM_REGS.GDURATION);
			mt_set_pwm_buf0_addr(conf->pwm_no, (U32 )conf->PWM_MODE_RANDOM_REGS.BUF0_BASE_ADDR);
			mt_set_pwm_buf0_size (conf->pwm_no, conf->PWM_MODE_RANDOM_REGS.BUF0_SIZE);
			mt_set_pwm_buf1_addr(conf->pwm_no, (U32 )conf->PWM_MODE_RANDOM_REGS.BUF1_BASE_ADDR);
			mt_set_pwm_buf1_size (conf->pwm_no, conf->PWM_MODE_RANDOM_REGS.BUF1_SIZE);
			mt_set_pwm_wave_num(conf->pwm_no, conf->PWM_MODE_RANDOM_REGS.WAVE_NUM);
			mt_set_pwm_con_stpbit(conf->pwm_no, conf->PWM_MODE_RANDOM_REGS.STOP_BITPOS_VALUE, MEMORY);
			mt_set_pwm_valid(conf->pwm_no, BUF0_EN_VALID);
			mt_set_pwm_valid(conf->pwm_no, BUF1_EN_VALID);
			break;

		case PWM_MODE_DELAY:
			PWMDBG("PWM_MODE_DELAY\n");
			mt_set_pwm_con_oldmode(conf->pwm_no, OLDMODE_DISABLE);
			mt_set_pwm_enable_seqmode();
			mt_set_pwm_disable(PWM2);
			mt_set_pwm_disable(PWM3);
			mt_set_pwm_disable(PWM4);
			mt_set_pwm_disable(PWM5);
			if ( conf->PWM_MODE_DELAY_REGS.PWM3_DELAY_DUR <0 ||conf->PWM_MODE_DELAY_REGS.PWM3_DELAY_DUR >= (1<<17) ||
				conf->PWM_MODE_DELAY_REGS.PWM4_DELAY_DUR < 0|| conf->PWM_MODE_DELAY_REGS.PWM4_DELAY_DUR >= (1<<17) ||
				conf->PWM_MODE_DELAY_REGS.PWM5_DELAY_DUR <0 || conf->PWM_MODE_DELAY_REGS.PWM5_DELAY_DUR >=(1<<17) ) {
				PWMDBG("Delay value invalid\n");
				return -EINVALID;
			}
			mt_set_pwm_delay_duration(PWM3_DELAY, conf->PWM_MODE_DELAY_REGS.PWM3_DELAY_DUR );
			mt_set_pwm_delay_clock(PWM3_DELAY, conf->PWM_MODE_DELAY_REGS.PWM3_DELAY_CLK);
			mt_set_pwm_delay_duration(PWM4_DELAY, conf->PWM_MODE_DELAY_REGS.PWM4_DELAY_DUR);
			mt_set_pwm_delay_clock(PWM4_DELAY, conf->PWM_MODE_DELAY_REGS.PWM4_DELAY_CLK);
			mt_set_pwm_delay_duration(PWM5_DELAY, conf->PWM_MODE_DELAY_REGS.PWM5_DELAY_DUR);
			mt_set_pwm_delay_clock(PWM5_DELAY, conf->PWM_MODE_DELAY_REGS.PWM5_DELAY_CLK);
			
			mt_set_pwm_enable(PWM2);
			mt_set_pwm_enable(PWM3);
			mt_set_pwm_enable(PWM4);
			mt_set_pwm_enable(PWM5);
			break;
*/			
		default:
			break;
		}

	switch (conf->clk_src) {
		case PWM_CLK_OLD_MODE_BLOCK:
			mt_set_pwm_clk (conf->pwm_no, CLK_BLOCK, conf->clk_div);
			PWMDBG("Enable oldmode and set clock block\n");
			break;
		case PWM_CLK_OLD_MODE_32K:
			mt_set_pwm_clk (conf->pwm_no, CLK_BLOCK_BY_1625_OR_32K, conf->clk_div);
			PWMDBG("Enable oldmode and set clock 32K\n");
			break;
		case PWM_CLK_NEW_MODE_BLOCK:
			mt_set_pwm_clk (conf->pwm_no, CLK_BLOCK, conf->clk_div);
			PWMDBG("Enable newmode and set clock block\n");
			break;
		case PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625:
			mt_set_pwm_clk (conf->pwm_no, CLK_BLOCK_BY_1625_OR_32K, conf->clk_div);
			PWMDBG("Enable newmode and set clock 32K\n");
			break;
		default:
			break;
		} 

	mb();
	mt_set_pwm_enable(conf->pwm_no); 
	PWMDBG("mt_set_pwm_enable\n");

	return RSUCCESS;
	
}	

EXPORT_SYMBOL(pwm_set_spec_config);

void mt_pwm_dump_regs(void)
{
	int i;
	U32 reg_val;
	reg_val = INREG32(PWM_ENABLE);
	PWMMSG("\r\n[PWM_ENABLE is:%x]\n\r ", reg_val );
	printk("<0>""pwm power_flag: 0x%x\n", (unsigned int)pwm_dev->power_flag); 
	printk("<0>""peri pdn0 clock: 0x%x\n", INREG32(PERI_PDN0_STA)); 

	for ( i = PWM1;  i <= PWM7; i ++ ) 
	{
		reg_val = INREG32(PWM_register[i] + 4* PWM_CON);
		PWMMSG("\r\n[PWM%d_CON is:%x]\r\n", i+1, reg_val);
		reg_val=INREG32(PWM_register[i]+4* PWM_HDURATION);
		PWMMSG("[PWM%d_HDURATION is:%x]\r\n",i+1,reg_val);
		reg_val = INREG32(PWM_register[i] + 4* PWM_LDURATION);
		PWMMSG("[PWM%d_LDURATION is:%x]\r\n", i+1, reg_val);
		reg_val=INREG32(PWM_register[i]+4* PWM_GDURATION);
		PWMMSG("[PWM%d_GDURATION is:%x]\r\n",i+1,reg_val);
/*
		reg_val = INREG32(PWM_register[i] + 4* PWM_BUF0_BASE_ADDR);
		PWMMSG("\r\n[PWM%d_BUF0_BASE_ADDR is:%x]\r\n", i, reg_val);
		reg_val=INREG32(PWM_register[i]+4* PWM_BUF0_SIZE);
		PWMMSG("\r\n[PWM%d_BUF0_SIZE is:%x]\r\n",i,reg_val);
		reg_val = INREG32(PWM_register[i] + 4* PWM_BUF1_BASE_ADDR);
		PWMMSG("\r\n[PWM%d_BUF1_BASE_ADDR is:%x]\r\n", i, reg_val);
		reg_val=INREG32(PWM_register[i]+4* PWM_BUF1_SIZE);
		PWMMSG("\r\n[PWM%d_BUF1_SIZE is:%x]\r\n",i+1,reg_val);
*/
		reg_val = INREG32(PWM_register[i] + 4* PWM_SEND_DATA0);
		PWMMSG("[PWM%d_SEND_DATA0 is:%x]\r\n", i+1, reg_val);
		reg_val=INREG32(PWM_register[i]+4* PWM_SEND_DATA1);
		PWMMSG("[PWM%d_PWM_SEND_DATA1 is:%x]\r\n",i+1,reg_val);
		reg_val = INREG32(PWM_register[i] + 4* PWM_WAVE_NUM);
		PWMMSG("[PWM%d_WAVE_NUM is:%x]\r\n", i+1, reg_val);
		if(i==PWM3||i==PWM4){
			reg_val=INREG32(PWM_register[i]+4* PWM_THRESH);
		}else{
			reg_val=INREG32(PWM_register[i]+4* PWM_DATA_WIDTH);
		}
		PWMMSG("[PWM%d_WIDTH is:%x]\r\n",i+1,reg_val);
		if(i==PWM3||i==PWM4){
			reg_val=INREG32(PWM_register[i]+4* PWM_SEND_WAVENUM);
		}else{
			reg_val=INREG32(PWM_register[i]+4* PWM_THRESH);
		}
		PWMMSG("[PWM%d_THRESH is:%x]\r\n",i+1,reg_val);
		if(i==PWM3||i==PWM4){
			reg_val=INREG32(PWM_register[i]+4* PWM_DATA_WIDTH);
		}else{
			reg_val=INREG32(PWM_register[i]+4* PWM_SEND_WAVENUM);
		}
		PWMMSG("[PWM%d_SEND_WAVENUM is:%x]\r\n",i+1,reg_val);
/*
		reg_val=INREG32(PWM_register[i]+4* PWM_VALID);
		PWMMSG("\r\n[PWM%d_SEND_VALID is:%x]\r\n",i+1,reg_val);
*/
	}		
}

EXPORT_SYMBOL(mt_pwm_dump_regs);

struct platform_device pwm_plat_dev={
	.name = "mt-pwm",
};

void mt_pmic_pwm1_test(void)
{
	struct pwm_spec_config conf;
	printk("<0>""=============mt_pmic_pwm1_test===============\n");

	mt_set_gpio_mode(GPIO73,GPIO_MODE_01); 
	conf.pwm_no = PWM1;
	conf.mode = PWM_MODE_FIFO;
	conf.clk_div = CLK_DIV1;
	conf.clk_src = PWM_CLK_NEW_MODE_BLOCK;
	//conf.intr = FALSE;
	conf.pmic_pad = false;
	conf.PWM_MODE_FIFO_REGS.IDLE_VALUE = IDLE_FALSE;
	conf.PWM_MODE_FIFO_REGS.GUARD_VALUE = GUARD_FALSE;
	conf.PWM_MODE_FIFO_REGS.STOP_BITPOS_VALUE = 63;
	conf.PWM_MODE_FIFO_REGS.HDURATION = 1;
	conf.PWM_MODE_FIFO_REGS.LDURATION = 1;
	conf.PWM_MODE_FIFO_REGS.GDURATION = 0;
	conf.PWM_MODE_FIFO_REGS.SEND_DATA0 = 0xf0f0f0f0;
	conf.PWM_MODE_FIFO_REGS.SEND_DATA1 = 0xf0f0f0f0;
	conf.PWM_MODE_FIFO_REGS.WAVE_NUM = 0;	
	printk(KERN_INFO "PWM: clk_div = %x, clk_src = %x, pwm_no = %x\n", conf.clk_div, conf.clk_src, conf.pwm_no);
	pwm_set_spec_config(&conf);
	//
	mt_set_gpio_mode(GPIOEXT34,GPIO_MODE_01); 
	conf.pwm_no = PWM7;
	conf.pmic_pad = true;
	pwm_set_spec_config(&conf);
}
void mt_pmic_pwm6_test(void)
{
	printk("<0>""=================mt_pmic_pwm6_test==================\n");
	
	mt_pwm_disable(PWM1, false);
	mt_pwm_disable(PWM7, true);
}

/****/
static ssize_t pwm_debug_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	//dump clock status
	printk("<0>""pwm power_flag: 0x%x\n", (unsigned int)pwm_dev->power_flag); 
	printk("<0>""peri pdn0 clock: 0x%x\n", INREG32(PERI_PDN0_STA)); 
	mt_pmic_pwm1_test();
	return count;
}

static ssize_t pwm_debug_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	mt_pwm_dump_regs();
	//
	mt_pmic_pwm6_test();
	//
	return sprintf(buf, "%s\n",buf);
}

static DEVICE_ATTR(pwm_debug, 0644, pwm_debug_show, pwm_debug_store);


int mt_pwm_probe ( struct platform_device *pdev)
{
	int ret;

	platform_set_drvdata (pdev, pwm_dev);
	
	ret = device_create_file(&pdev->dev, &dev_attr_pwm_debug);
	if (ret)
		PWMDBG("error creating sysfs files: pwm_debug\n");

//	request_irq(69, mt_pwm_irq, IRQF_TRIGGER_LOW, "mt6589_pwm", NULL);
	
	return RSUCCESS;
}

int  mt_pwm_remove(struct platform_device *pdev)
{
	if ( ! pdev ) {
		PWMDBG ("The plaform device is not exist\n");
		return -EBADADDR;
	}
	device_remove_file(&pdev->dev, &dev_attr_pwm_debug);

	PWMDBG ( "mt_pwm_remove\n" );
	return RSUCCESS;
}

void mt_pwm_shutdown(struct platform_device *pdev)
{
	printk("mt_pwm_shutdown\n");
	return;
}

struct platform_driver pwm_plat_driver={
	.probe = mt_pwm_probe,
	.remove = mt_pwm_remove,
	.shutdown = mt_pwm_shutdown,
	.driver = {
		.name="mt-pwm"
	},
};

static int __init mt_pwm_init(void)
{
	int ret;
	ret = platform_device_register ( &pwm_plat_dev );
	if (ret < 0 ){
		PWMDBG ("platform_device_register error\n");
		goto out;
	}
	ret = platform_driver_register ( &pwm_plat_driver );
	if ( ret < 0 ) {
		PWMDBG ("platform_driver_register error\n");
		goto out;
	}

out:
	return ret;
}

static void __exit mt_pwm_exit(void)
{
	platform_device_unregister ( &pwm_plat_dev );
	platform_driver_unregister ( &pwm_plat_driver );
}

module_init(mt_pwm_init);
module_exit(mt_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("How wang <How.wang@mediatek.com>");
MODULE_DESCRIPTION(" This module is for mtk chip of mediatek");

