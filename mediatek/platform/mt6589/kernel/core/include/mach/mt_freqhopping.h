
#ifndef __MT_FREQHOPPING_H__
#define __MT_FREQHOPPING_H__

#define MT_FHPLL_MAX 6
#define MT_SSC_NR_PREDEFINE_SETTING 10 //TODO: is 10 a good number ?

#define FHTAG "[FH]"

#define VERBOSE_DEBUG 0

#if VERBOSE_DEBUG
#define FH_MSG(fmt, args...) \
do {    \
		printk( KERN_DEBUG FHTAG""fmt" <- %s(): L<%d>  PID<%s><%d>\n", \
            	##args ,__FUNCTION__,__LINE__,current->comm,current->pid); \
} while(0);
#else
#define FH_MSG(fmt, args...) \
do {    \
		printk( KERN_DEBUG FHTAG""fmt" \n", \
            	##args ); \
} while(0);

#endif

enum FH_FH_STATUS{
 FH_FH_DISABLE = 0,
 FH_FH_ENABLE_SSC,	
 FH_FH_ENABLE_DFH
};

enum FH_PLL_STATUS{
 FH_PLL_DISABLE = 0,
 FH_PLL_ENABLE = 1
};

//TODO: FREQ_MODIFIED should not be here
// FH_PLL_STATUS_FREQ_MODIFIED = 3


enum FH_CMD{
 FH_CMD_ENABLE = 1,
 FH_CMD_DISABLE,
 FH_CMD_ENABLE_USR_DEFINED,
 FH_CMD_DISABLE_USR_DEFINED,
 FH_CMD_INTERNAL_MAX_CMD,
/* TODO:  do we need these cmds ?
 FH_CMD_PLL_ENABLE,
 FH_CMD_PLL_DISABLE,
 FH_CMD_EXT_ALL_FULL_RANGE_CMD,
 FH_CMD_EXT_ALL_HALF_RANGE_CMD,
 FH_CMD_EXT_DISABLE_ALL_CMD,
 FH_CMD_EXT_DESIGNATED_PLL_FULL_RANGE_CMD,
 FH_CMD_EXT_DESIGNATED_PLL_AND_SETTING_CMD
*/ 
};

/*
enum FH_OPCODE{
 FH_OPCODE_ENABLE_WITH_ID = 1,
 FH_OPCODE_ENABLE_WITHOUT_ID,
 FH_OPCODE_DISABLE,
};
*/

enum FH_PLL_ID {
 MT658X_FH_MINIMUMM_PLL = 0,		
 MT658X_FH_ARM_PLL	= MT658X_FH_MINIMUMM_PLL,
 MT658X_FH_MAIN_PLL	= 1,
 MT658X_FH_MEM_PLL	= 2,
 MT658X_FH_MSDC_PLL	= 3,
 MT658X_FH_TVD_PLL	= 4,
 MT658X_FH_LVDS_PLL	= 5,
 MT658X_FH_MAXIMUMM_PLL = MT658X_FH_LVDS_PLL,
 MT658X_FH_PLL_TOTAL_NUM
};

//keep track the status of each PLL 
//TODO: do we need another "uint mode" for Dynamic FH
typedef struct{
	unsigned int	fh_status;
	unsigned int	pll_status;
	unsigned int	setting_id;
	unsigned int	curr_freq;
	unsigned int	user_defined;
}fh_pll_t;


//Record the owner of enable freq hopping <==TBD
struct freqhopping_pll{
	union {
		int mt_pll[MT_FHPLL_MAX];
		struct {
		int mt_arm_fhpll;
		int mt_main_fhpll;
		int mt_mem_fhpll;
		int mt_msdc_fhpll;
		int mt_tvd_fhpll;
		int mt_lvds_fhpll;
		};
	};
};

struct freqhopping_ssc {
	unsigned int	 freq;
	unsigned int	 dt;
	unsigned int	 df;
	unsigned int	 upbnd;
	unsigned int 	 lowbnd;
	unsigned int	 dds;
};

struct freqhopping_ioctl {
	unsigned int  pll_id;
	struct freqhopping_ssc ssc_setting; //used only when user-define
	int  result;
};

int freqhopping_config(unsigned int pll_id, unsigned long vco_freq, unsigned int enable);
void mt_freqhopping_init(void);
void mt_freqhopping_pll_init(void);
int mt_h2l_mempll(void);
int mt_l2h_mempll(void);
void mt_fh_popod_save(void);
void mt_fh_popod_restore(void);
int mt_fh_dram_overclock(int clk);
int mt_fh_get_dramc(void);

#endif/* !__MT_FREQHOPPING_H__ */

