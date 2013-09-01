#ifndef __MT_PMIC_WRAP_H__
#define __MT_PMIC_WRAP_H__
//#include <mach/typedefs.h>
#include <linux/smp.h>
#include <mach/mt_typedefs.h>

#define PMIC_WRAP_DEBUG

#define PWRAPTAG                "[PWRAP] "
#ifdef PMIC_WRAP_DEBUG
  #define PWRAPDEB(fmt, arg...)   printk(PWRAPTAG "cpuid=%d," fmt,raw_smp_processor_id(), ##arg)
  //#define PWRAPLOG(fmt, arg...)	printk(PWRAPTAG fmt,##arg)
  #define PWRAPFUC(fmt, arg...)	printk(PWRAPTAG "cpuid=%d,%s\n", raw_smp_processor_id(), __FUNCTION__)
  //#define PWRAPFUC(fmt, arg...)   printk(PWRAPTAG "%s\n", __FUNCTION__)
#endif
//typedef unsigned int        U32;
//typedef signed int          S32;
#define PWRAPLOG(fmt, arg...)	printk(PWRAPTAG fmt,##arg)
#define PWRAPERR(fmt, arg...)   printk(KERN_ERR PWRAPTAG "ERROR,line=%d " fmt, __LINE__, ##arg)
#define PWRAPREG(fmt, arg...)   printk(PWRAPTAG fmt,##arg)

//---start ---external API for pmic_wrap user--------------------------------------------------
S32 pwrap_wacs2( U32  write, U32  adr, U32  wdata, U32 *rdata );
S32 pwrap_read( U32  adr, U32 *rdata );
S32 pwrap_write( U32  adr, U32  wdata );
S32 pwrap_init ( void );
//---end ---external API----------------------------------------------------


//---start ---internal API--------------------------------------------------
S32 _pwrap_wacs2_nochk( U32 write, U32 adr, U32 wdata, U32 *rdata );
S32 _pwrap_reset_spislv(void);
U32 pwrap_read_test(void);
U32 pwrap_write_test(void);
void pwrap_power_off(void);
void pwrap_power_on(void);
void pwrap_dump_all_register(void);
void pwrap_dump_ap_register(void);
U32 _pwrap_AlignCRC( void );

//---end ---internal API--------------------------------------------------

#define E_PWR_INVALID_ARG               1
#define E_PWR_INVALID_RW                2
#define E_PWR_INVALID_ADDR              3
#define E_PWR_INVALID_WDAT              4
#define E_PWR_INVALID_OP_MANUAL         5
#define E_PWR_NOT_IDLE_STATE            6
#define E_PWR_NOT_INIT_DONE             7
#define E_PWR_NOT_INIT_DONE_READ        8
#define E_PWR_WAIT_IDLE_TIMEOUT         9
#define E_PWR_WAIT_IDLE_TIMEOUT_READ    10
#define E_PWR_INIT_SIDLY_FAIL           11
#define E_PWR_RESET_TIMEOUT             12
#define E_PWR_TIMEOUT                   13

#define E_PWR_INIT_RESET_SPI            20
#define E_PWR_INIT_SIDLY                21
#define E_PWR_INIT_REG_CLOCK            22
#define E_PWR_INIT_ENABLE_PMIC          23
#define E_PWR_INIT_DIO                  24
#define E_PWR_INIT_CIPHER               25
#define E_PWR_INIT_WRITE_TEST           26
#define E_PWR_INIT_ENABLE_CRC           27
#define E_PWR_INIT_ENABLE_DEWRAP        28

#define E_PWR_READ_TEST_FAIL            30
#define E_PWR_WRITE_TEST_FAIL           31
#define E_PWR_SWITCH_DIO                32

//---start ---LDVT API------------------------------------------------------
#ifdef CONFIG_MTK_LDVT_PMIC_WRAP
#include <linux/interrupt.h>
static irqreturn_t pwrap_interrupt_for_ldvt(int irqno, void *dev_id);
static int mt_pwrap_ioctl(struct file *file, unsigned int cmd, unsigned long arg);
S32 pwrap_wacs0( U32 write, U32 adr, U32 wdata, U32 *rdata );
S32 pwrap_wacs1( U32  write, U32  adr, U32  wdata, U32 *rdata );
S32 pwrap_wacs3( U32  write, U32  adr, U32  wdata, U32 *rdata );
S32 pwrap_wacs4( U32  write, U32  adr, U32  wdata, U32 *rdata );
S32 _pwrap_manual_mode( U32  write, U32  op, U32  wdata, U32 *rdata );
S32 _pwrap_manual_modeAccess( U32  write, U32  adr, U32  wdata, U32 *rdata );
S32 _pwrap_disable_cipher( void );
S32 _pwrap_enable_cipher( void );
S32 _pwrap_switch_dio( U32 dio_en );
static S32 _pwrap_StaUpdTrig( S32 mode );
void _pwrap_switch_mux( U32 mux_sel_new );
#endif //CONFIG_MTK_LDVT_PMIC_WRAP
//---start ---LDVT API-----------------------------------------------------
//#define PWRAP_CONCURRENCE_TEST
#ifdef PWRAP_CONCURRENCE_TEST
extern U32 g_eint_pass_cpu0;
extern U32 g_eint_pass_cpu1;
extern U32 g_eint_pass_cpu2;
extern U32 g_eint_pass_cpu3;

extern U32 g_eint_fail_cpu0;
extern U32 g_eint_fail_cpu1;
extern U32 g_eint_fail_cpu2;
extern U32 g_eint_fail_cpu3;

extern U32 g_i2c0_pass_cpu0;
extern U32 g_i2c0_pass_cpu1;
extern U32 g_i2c0_pass_cpu2;
extern U32 g_i2c0_pass_cpu3;

extern U32 g_i2c0_fail_cpu0;
extern U32 g_i2c0_fail_cpu1;
extern U32 g_i2c0_fail_cpu2;
extern U32 g_i2c0_fail_cpu3;

extern U32 g_i2c1_pass_cpu0;
extern U32 g_i2c1_pass_cpu1;
extern U32 g_i2c1_pass_cpu2;
extern U32 g_i2c1_pass_cpu3;

extern U32 g_i2c1_fail_cpu0;
extern U32 g_i2c1_fail_cpu1;
extern U32 g_i2c1_fail_cpu2;
extern U32 g_i2c1_fail_cpu3;

extern U32 g_i2c2_pass_cpu0;
extern U32 g_i2c2_pass_cpu1;
extern U32 g_i2c2_pass_cpu2;
extern U32 g_i2c2_pass_cpu3;

extern U32 g_i2c2_fail_cpu0;
extern U32 g_i2c2_fail_cpu1;
extern U32 g_i2c2_fail_cpu2;
extern U32 g_i2c2_fail_cpu3;


extern U32 g_kpd_pass_cpu0;
extern U32 g_kpd_pass_cpu1;
extern U32 g_kpd_pass_cpu2;
extern U32 g_kpd_pass_cpu3;

extern U32 g_kpd_fail_cpu0;
extern U32 g_kpd_fail_cpu1;
extern U32 g_kpd_fail_cpu2;
extern U32 g_kpd_fail_cpu3;

extern U32 g_spm_pass_cpu0;
extern U32 g_spm_pass_cpu1;
extern U32 g_spm_pass_cpu2;
extern U32 g_spm_pass_cpu3;

extern U32 g_spm_fail_cpu0;
extern U32 g_spm_fail_cpu1;
extern U32 g_spm_fail_cpu2;
extern U32 g_spm_fail_cpu3;

extern U32 g_pwm_pass_cpu0;
extern U32 g_pwm_pass_cpu1;
extern U32 g_pwm_pass_cpu2;
extern U32 g_pwm_pass_cpu3;

extern U32 g_pwm_fail_cpu0;
extern U32 g_pwm_fail_cpu1;
extern U32 g_pwm_fail_cpu2;
extern U32 g_pwm_fail_cpu3;

extern U32 g_wacs0_pass_cpu0;
extern U32 g_wacs0_pass_cpu1;
extern U32 g_wacs0_pass_cpu2;
extern U32 g_wacs0_pass_cpu3;

extern U32 g_wacs0_fail_cpu0;
extern U32 g_wacs0_fail_cpu1;
extern U32 g_wacs0_fail_cpu2;
extern U32 g_wacs0_fail_cpu3;

extern U32 g_wacs1_pass_cpu0;
extern U32 g_wacs1_pass_cpu1;
extern U32 g_wacs1_pass_cpu2;
extern U32 g_wacs1_pass_cpu3;

extern U32 g_wacs1_fail_cpu0;
extern U32 g_wacs1_fail_cpu1;
extern U32 g_wacs1_fail_cpu2;
extern U32 g_wacs1_fail_cpu3;

extern U32 g_wacs2_pass_cpu0;
extern U32 g_wacs2_pass_cpu1;
extern U32 g_wacs2_pass_cpu2;
extern U32 g_wacs2_pass_cpu3;

extern U32 g_wacs2_fail_cpu0;
extern U32 g_wacs2_fail_cpu1;
extern U32 g_wacs2_fail_cpu2;
extern U32 g_wacs2_fail_cpu3;

extern U32 g_wacs3_pass_cpu0;
extern U32 g_wacs3_pass_cpu1;
extern U32 g_wacs3_pass_cpu2;
extern U32 g_wacs3_pass_cpu3;

extern U32 g_wacs3_fail_cpu0;
extern U32 g_wacs3_fail_cpu1;
extern U32 g_wacs3_fail_cpu2;
extern U32 g_wacs3_fail_cpu3;

extern U32 g_wacs4_pass_cpu0;
extern U32 g_wacs4_pass_cpu1;
extern U32 g_wacs4_pass_cpu2;
extern U32 g_wacs4_pass_cpu3;

extern U32 g_wacs4_fail_cpu0;
extern U32 g_wacs4_fail_cpu1;
extern U32 g_wacs4_fail_cpu2;
extern U32 g_wacs4_fail_cpu3;


#endif


#endif //__MT_PMIC_WRAP_H__
