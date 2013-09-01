/*******************************************************************************
* mt6575_pwm.h PWM Drvier                                                     
*                                                                              
* Copyright (c) 2010, Media Teck.inc                                           
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
* Author : Chagnlei Gao (changlei.gao@mediatek.com)                            
********************************************************************************
*/

#ifndef __MT_PWM_H__
#define __MT_PWM_H__
#endif

#include <mach/mt_typedefs.h>
#include <mach/mt_reg_base.h>


/***********************************
* PWM register address             *
************************************/
#define PWM_ENABLE (PWM_BASE+0x0000)
                                                  
#define PWM3_DELAY (PWM_BASE+0x00004)                                                 
#define PWM4_DELAY (PWM_BASE+0x00008)                                                 
#define PWM5_DELAY (PWM_BASE+0x000c)
                                                  
#define PWM_INT_ENABLE (PWM_BASE+0x0200)                                            
#define PWM_INT_STATUS (PWM_BASE+0x0204)                                      
#define PWM_INT_ACK (PWM_BASE+0x0208)                                       
#define PWM_EN_STATUS (PWM_BASE+0x020c)

#define PWM_SEL_PMIC (PWM_BASE+0x02D4)

/*PWM3,PWM4,PWM5_DELAY registers*/
#define PWM_DELAY_DURATION_MASK 0x0000FFFF
#define PWM_DELAY_CLK_MASK 0x00010000

#define PWM_ENABLE_SEQ_OFFSET 16
#define PWM_ENABLE_TEST_SEL_OFFSET 17

/*PWM1~PWM7 control registers*/
#define PWM_CON_CLKDIV_MASK 0x00000007
#define PWM_CON_CLKDIV_OFFSET 0
#define PWM_CON_CLKSEL_MASK 0x00000008
#define PWM_CON_CLKSEL_OFFSET 3

#define PWM_CON_SRCSEL_MASK 0x00000020
#define PWM_CON_SRCSEL_OFFSET 5

#define PWM_CON_MODE_MASK 0x00000040
#define PWM_CON_MODE_OFFSET 6

#define PWM_CON_IDLE_VALUE_MASK 0x00000080
#define PWM_CON_IDLE_VALUE_OFFSET 7

#define PWM_CON_GUARD_VALUE_MASK 0x00000100
#define PWM_CON_GUARD_VALUE_OFFSET 8

#define PWM_CON_STOP_BITS_MASK 0x00007E00
#define PWM_CON_STOP_BITS_OFFSET 9
#define PWM_CON_OLD_MODE_MASK 0x00008000
#define PWM_CON_OLD_MODE_OFFSET 15

/*********************************
*  Define Error Number                        
**********************************/
#define RSUCCESS 0
#define EEXCESSPWMNO 1
#define EPARMNOSUPPORT 2
#define ERROR 3
#define EBADADDR 4
#define EEXCESSBITS 5
#define EINVALID 6

/**********************************
* Global enum data                

***********************************/


enum PWN_NO{ 
	PWM_MIN,
	PWM1 = PWM_MIN,
	PWM2,
	PWM3,
	PWM4,
	PWM5,
	PWM6,
	PWM7,
	PWM_NUM,
	PWM_MAX=PWM_NUM
};

enum TEST_SEL_BIT{
	TEST_SEL_FALSE,
	TEST_SEL_TRUE
};

enum PWM_CON_MODE_BIT{
	PERIOD,
	RAND
};

enum PWM_CON_SRCSEL_BIT{
	PWM_FIFO,
	MEMORY
};

enum PWM_CON_IDLE_BIT{
	IDLE_FALSE,
	IDLE_TRUE,
	IDLE_MAX
};

enum  PWM_CON_GUARD_BIT{
	GUARD_FALSE,
	GUARD_TRUE,
	GUARD_MAX
};

enum OLD_MODE_BIT{
	OLDMODE_DISABLE,
	OLDMODE_ENABLE
};

enum PWM_BUF_VALID_BIT{
	BUF0_VALID,
	BUF0_EN_VALID,
	BUF1_VALID,
	BUF1_EN_VALID,
	BUF_EN_MAX
};

enum CLOCK_SRC{
	CLK_BLOCK,
	CLK_BLOCK_BY_1625_OR_32K
};

enum PWM_CLK_DIV{
	CLK_DIV_MIN,
	CLK_DIV1 = CLK_DIV_MIN,
	CLK_DIV2,
	CLK_DIV4,
	CLK_DIV8,
	CLK_DIV16,
	CLK_DIV32,
	CLK_DIV64,
	CLK_DIV128,
	CLK_DIV_MAX
};

enum PWM_INT_ENABLE_BITS{
	PWM1_INT_FINISH_EN,
	PWM1_INT_UNDERFLOW_EN,
	PWM2_INT_FINISH_EN,
	PWM2_INT_UNDERFLOW_EN,
	PWM3_INT_FINISH_EN,
	PWM3_INT_UNDERFLOW_EN, 
	PWM4_INT_FINISH_EN,
	PWM4_INT_UNDERFLOW_EN,
	PWM5_INT_FINISH_EN,
	PWM5_INT_UNDERFLOW_EN,
	PWM6_INT_FINISH_EN,
	PWM6_INT_UNDERFLOW_EN,
	PWM7_INT_FINISH_EN,
	PWM7_INT_UNDERFLOW_EN,
	PWM_INT_ENABLE_BITS_MAX,
};

enum PWM_INT_STATUS_BITS{
	PWM1_INT_FINISH_ST,
	PWM1_INT_UNDERFLOW_ST,
	PWM2_INT_FINISH_ST,
	PWM2_INT_UNDERFLOW_ST,
	PWM3_INT_FINISH_ST,
	PWM3_INT_UNDERFLOW_ST, 
	PWM4_INT_FINISH_ST,
	PWM4_INT_UNDERFLOW_ST,
	PWM5_INT_FINISH_ST,
	PWM5_INT_UNDERFLOW_ST,
	PWM6_INT_FINISH_ST,
	PWM6_INT_UNDERFLOW_ST,
	PWM7_INT_FINISH_ST,
	PWM7_INT_UNDERFLOW_ST,
	PWM_INT_STATUS_BITS_MAX,
};
	   
enum PWM_INT_ACK_BITS{
	PWM1_INT_FINISH_ACK,
	PWM1_INT_UNDERFLOW_ACK,
	PWM2_INT_FINISH_ACK,
	PWM2_INT_UNDERFLOW_ACK,
	PWM3_INT_FINISH_ACK,
	PWM3_INT_UNDERFLOW_ACK, 
	PWM4_INT_FINISH_ACK,
	PWM4_INT_UNDERFLOW_ACK,
	PWM5_INT_FINISH_ACK,
	PWM5_INT_UNDERFLOW_ACK,
	PWM6_INT_FINISH_ACK,
	PWM6_INT_UNDERFLOW_ACK,
	PWM7_INT_FINISH_ACK,
	PWM7_INT_UNDERFLOW_ACK,
	PWM_INT_ACK_BITS_MAX,
};

enum PWM_CLOCK_SRC_ENUM{
	PWM_CLK_SRC_MIN,
	PWM_CLK_OLD_MODE_BLOCK = PWM_CLK_SRC_MIN,
	PWM_CLK_OLD_MODE_32K,
	PWM_CLK_NEW_MODE_BLOCK,
	PWM_CLK_NEW_MODE_BLOCK_DIV_BY_1625,
	PWM_CLK_SRC_NUM,
	PWM_CLK_SRC_INVALID,
};

enum PWM_MODE_ENUM{
	PWM_MODE_MIN,
	PWM_MODE_OLD = PWM_MODE_MIN,
	PWM_MODE_FIFO,
	PWM_MODE_MEMORY,
	PWM_MODE_RANDOM,
	PWM_MODE_DELAY,
	PWM_MODE_INVALID,
};

#define PWM_NEW_MODE_DUTY_TOTAL_BITS 64

struct pwm_easy_config {
	U32 pwm_no;
	U32 duty;
	U32 clk_src;
	U32 clk_div;
	U16 duration;
	BOOL pmic_pad;
};
struct pwm_spec_config {
	U32 pwm_no;
	U32 mode;
	U32  clk_div;
	U32 clk_src;
	BOOL intr;
	BOOL pmic_pad;

	union {
		//for old mode
		struct _PWM_OLDMODE_REGS {
			U16 IDLE_VALUE;
			U16 GUARD_VALUE;
			U16 GDURATION;
			U16 WAVE_NUM;
			U16 DATA_WIDTH;
			U16 THRESH;
		}PWM_MODE_OLD_REGS;

		//for fifo mode
		struct _PWM_MODE_FIFO_REGS {
			U32 IDLE_VALUE;
			U32 GUARD_VALUE;
			U32 STOP_BITPOS_VALUE;
			U16 HDURATION;
			U16 LDURATION;
			U32 GDURATION;
			U32 SEND_DATA0;
			U32 SEND_DATA1;
			U32 WAVE_NUM;
		}PWM_MODE_FIFO_REGS;
/*
		//for memory mode
		struct _PWM_MODE_MEMORY_REGS {
			U32 IDLE_VALUE;
			U32 GUARD_VALUE;
			U32 STOP_BITPOS_VALUE;
			U16 HDURATION;
			U16 LDURATION;
			U16 GDURATION;
			U32  * BUF0_BASE_ADDR;
			U32 BUF0_SIZE;
			U16 WAVE_NUM;
		}PWM_MODE_MEMORY_REGS;

		//for RANDOM mode
		struct _PWM_MODE_RANDOM_REGS {
			U16 IDLE_VALUE;
			U16 GUARD_VALUE;
			U32 STOP_BITPOS_VALUE;
			U16 HDURATION;
			U16 LDURATION;
			U16 GDURATION;
			U32  * BUF0_BASE_ADDR;
			U32 BUF0_SIZE;
			U32 *BUF1_BASE_ADDR;
			U32 BUF1_SIZE;
			U16 WAVE_NUM;
			U32 VALID;
		}PWM_MODE_RANDOM_REGS;

		//for seq mode
		struct _PWM_MODE_DELAY_REGS {
			//U32 ENABLE_DELAY_VALUE;
			U16 PWM3_DELAY_DUR;
			U32 PWM3_DELAY_CLK;   //0: block clock source, 1: block/1625 clock source
			U16 PWM4_DELAY_DUR;
			U32 PWM4_DELAY_CLK;   
			U16 PWM5_DELAY_DUR;
			U32 PWM5_DELAY_CLK;
		}PWM_MODE_DELAY_REGS;
*/
	};	
};

S32 pwm_set_easy_config ( struct pwm_easy_config *conf);
S32 pwm_set_spec_config(struct pwm_spec_config *conf);

void mt_pwm_dump_regs(void);
void mt_pwm_disable(U32 pwm_no, BOOL pmic_pad);

