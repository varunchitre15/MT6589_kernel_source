

/******************************************************************************
 * pmic_wrapper.c - Linux pmic_wrapper Driver
 *
 *
 * DESCRIPTION:
 *     This file provid the other drivers PMIC wrapper relative functions
 *
 ******************************************************************************/

#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/sched.h>

#include <mach/mt_reg_base.h>
#include <mach/mt_typedefs.h>
//#include <mach/mtk_timer.h>
#include <linux/timer.h>

//#include <mach/mt_reg_base.h>
//#include <mach/mt6585_typedefs.h>

#include "reg_pmic_wrap.h"
#include <mach/mt_pmic_wrap.h>

#include <linux/io.h>
#include "mach/irqs.h"
#include <linux/syscore_ops.h>


#ifdef CONFIG_MTK_LDVT_PMIC_WRAP
//#undef CONFIG_MTK_LDVT_PMIC_WRAP
#endif

#ifdef CONFIG_MTK_LDVT_PMIC_WRAP
  #include "tc_pwrap_ldvt.h"
#endif
//----------interral API ------------------------
static S32 _pwrap_init_dio( U32 dio_en );
static S32 _pwrap_init_cipher( void );
static S32 _pwrap_init_sidly( void );
static S32 _pwrap_init_reg_clock( U32 regck_sel );
static BOOL _pwrap_timeout_ns (U64 start_time_ns, U64 timeout_time_ns);
static U64 _pwrap_get_current_time(void);
static U64 _pwrap_time2ns (U64 time_us);
static S32 pwrap_read_nochk( U32  adr, U32 *rdata );
static S32 pwrap_write_nochk( U32  adr, U32  wdata );
static void pwrap_trace_wacs2(void);


//******************************************************************************
//--add log for pmic access-------------------------------------------------
//******************************************************************************
typedef enum
{
    PWRAP_________READ,
    PWRAP_________WRITE,
    PWRAP_________MAX
} PWRAP_ACTION_ENUM;

typedef struct
{
    U64 wacs_time;
    PWRAP_ACTION_ENUM operation;
    U32 result;//result=0:success
    U32 addr;
    U32 wdata;
    U32 rdata;
} PWRAP_DEBUG_DATA_T;//adr, U32  wdata, U32 *rdata

#define PWRAP_DEBUG_COUNT 100
static volatile PWRAP_DEBUG_DATA_T pwrap_debug_data[PWRAP_DEBUG_COUNT] = {{0}};
static volatile U32 pwrap_debug_index = 0;
void pwrap_trace(U64 wacs_time,U64 result,U32  write, U32 addr, U32 wdata, U32 rdata)
{
    U32 index;
    pwrap_debug_index++;
    pwrap_debug_index %= PWRAP_DEBUG_COUNT;
    index =pwrap_debug_index;
    if(write==0)
      pwrap_debug_data[index].operation = PWRAP_________READ;//read
    else
      pwrap_debug_data[index].operation = PWRAP_________WRITE;
    pwrap_debug_data[index].wacs_time = wacs_time;
    pwrap_debug_data[index].result = result;
    pwrap_debug_data[index].addr = addr;
    pwrap_debug_data[index].wdata = wdata;
    pwrap_debug_data[index].rdata = rdata;
}
/******************************************************************************
 wrapper timeout
******************************************************************************/
#define PWRAP_TIMEOUT
#ifdef PWRAP_TIMEOUT
static U64 _pwrap_get_current_time(void)
{
  return sched_clock();   ///TODO: fix me
}
U64 elapse_time=0;

static BOOL _pwrap_timeout_ns (U64 start_time_ns, U64 timeout_time_ns)
{
  U64 cur_time=0;
  //U64 elapse_time=0;

  // get current tick
  cur_time = sched_clock();//ns
  elapse_time=cur_time-start_time_ns;

  // check if timeout
  if (timeout_time_ns <= elapse_time)
  {
    // timeout
    return TRUE;
  }

  return FALSE;
}
static U64 _pwrap_time2ns (U64 time_us)
{
  return time_us*1000;
}

#else
static U64 _pwrap_get_current_time(void)
{
  return 0;
}
static BOOL _pwrap_timeout_ns (U64 start_time_ns, U64 elapse_time)//,U64 timeout_ns)
{
  return FALSE;
}
static U64 _pwrap_time2ns (U64 time_us)
{
  return 0;
}

#endif
//#####################################################################
//define macro and inline function (for do while loop)
//#####################################################################
typedef U32 (*loop_condition_fp)(U32);//define a function pointer

static inline U32 wait_for_fsm_idle(U32 x)
{
  return (GET_WACS0_FSM( x ) != WACS_FSM_IDLE );
}
static inline U32 wait_for_fsm_vldclr(U32 x)
{
  return (GET_WACS0_FSM( x ) != WACS_FSM_WFVLDCLR);
}
static inline U32 wait_for_sync(U32 x)
{
  return (GET_SYNC_IDLE0(x) != WACS_SYNC_IDLE);
}
static inline U32 wait_for_idle_and_sync(U32 x)
{
  return ((GET_WACS0_FSM(x) != WACS_FSM_IDLE) || (GET_SYNC_IDLE0(x) != WACS_SYNC_IDLE)) ;
}
static inline U32 wait_for_wrap_idle(U32 x)
{
  return ((GET_WRAP_FSM(x) != 0x0) || (GET_WRAP_CH_DLE_RESTCNT(x) != 0x0));
}
static inline U32 wait_for_wrap_state_idle(U32 x)
{
  return ( GET_WRAP_AG_DLE_RESTCNT( x ) != 0 ) ;
}
static inline U32 wait_for_man_idle_and_noreq(U32 x)
{
  return ( (GET_MAN_REQ(x) != MAN_FSM_NO_REQ ) || (GET_MAN_FSM(x) != MAN_FSM_IDLE) );
}
static inline U32 wait_for_man_vldclr(U32 x)
{
  return  (GET_MAN_FSM( x ) != MAN_FSM_WFVLDCLR) ;
}
static inline U32 wait_for_cipher_ready(U32 x)
{
  return (x!=1) ;
}
static inline U32 wait_for_stdupd_idle(U32 x)
{
  return ( GET_STAUPD_FSM(x) != 0x0) ;
}

static inline U32 wait_for_state_ready_init(loop_condition_fp fp,U32 timeout_us,U32 wacs_register,U32 *read_reg)
{

  U64 start_time_ns=0, timeout_ns=0;
  U32 reg_rdata=0x0;
  //U32 rdata=0;
  start_time_ns = _pwrap_get_current_time();
  timeout_ns = _pwrap_time2ns(timeout_us);
  do
  {
    reg_rdata = WRAP_RD32(wacs_register);

    if (_pwrap_timeout_ns(start_time_ns, timeout_ns))
    {
      PWRAPERR("timeout when waiting for idle\n");
      return E_PWR_TIMEOUT;
    }
  } while( fp(reg_rdata)); //IDLE State
  if(read_reg)
   *read_reg=reg_rdata;
  return 0;
}
static inline U32 wait_for_state_idle(loop_condition_fp fp,U32 timeout_us,U32 wacs_register,U32 wacs_vldclr_register,U32 *read_reg)
{

  U64 start_time_ns=0, timeout_ns=0;
  U32 reg_rdata;
  start_time_ns = _pwrap_get_current_time();
  timeout_ns = _pwrap_time2ns(timeout_us);
  do
  {
    reg_rdata = WRAP_RD32(wacs_register);
    if( GET_INIT_DONE0( reg_rdata ) != WACS_INIT_DONE)
    {
      PWRAPERR("initialization isn't finished \n");
      return E_PWR_NOT_INIT_DONE;
    }
    //if last read command timeout,clear vldclr bit
    //read command state machine:FSM_REQ-->wfdle-->WFVLDCLR;write:FSM_REQ-->idle
    switch ( GET_WACS0_FSM( reg_rdata ) )
    {
      case WACS_FSM_WFVLDCLR:
        WRAP_WR32(wacs_vldclr_register , 1);
        PWRAPERR("WACS_FSM = PMIC_WRAP_WACS_VLDCLR\n");
        break;
      case WACS_FSM_WFDLE:
        PWRAPERR("WACS_FSM = WACS_FSM_WFDLE\n");
        break;
      case WACS_FSM_REQ:
        PWRAPERR("WACS_FSM = WACS_FSM_REQ\n");
        break;
      default:
        break;
    }
    if (_pwrap_timeout_ns(start_time_ns, timeout_ns))
    {
      PWRAPERR("timeout when waiting for idle\n");
      pwrap_dump_ap_register();
      pwrap_trace_wacs2();
      return E_PWR_WAIT_IDLE_TIMEOUT;
    }
  }while( fp(reg_rdata)); //IDLE State
  if(read_reg)
   *read_reg=reg_rdata;
  return 0;
}
static inline U32 wait_for_state_ready(loop_condition_fp fp,U32 timeout_us,U32 wacs_register,U32 *read_reg)
{

  U64 start_time_ns=0, timeout_ns=0;
  U32 reg_rdata;
  start_time_ns = _pwrap_get_current_time();
  timeout_ns = _pwrap_time2ns(timeout_us);
  do
  {
    reg_rdata = WRAP_RD32(wacs_register);

    if( GET_INIT_DONE0( reg_rdata ) != WACS_INIT_DONE)
    {
      PWRAPERR("initialization isn't finished \n");
      return E_PWR_NOT_INIT_DONE;
    }
    if (_pwrap_timeout_ns(start_time_ns, timeout_ns))
    {
      PWRAPERR("timeout when waiting for idle\n");
      pwrap_dump_ap_register();
      pwrap_trace_wacs2();
      return E_PWR_WAIT_IDLE_TIMEOUT;
    }
  } while( fp(reg_rdata)); //IDLE State
  if(read_reg)
   *read_reg=reg_rdata;
  return 0;
}
//******************************************************************************
//--external API for pmic_wrap user-------------------------------------------------
//******************************************************************************
S32 pwrap_read( U32  adr, U32 *rdata )
{
  return pwrap_wacs2( 0, adr,0,rdata );
}

S32 pwrap_write( U32  adr, U32  wdata )
{
  return pwrap_wacs2( 1, adr,wdata,0 );
}
//--------------------------------------------------------
//    Function : pwrap_wacs2()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 pwrap_wacs2( U32  write, U32  adr, U32  wdata, U32 *rdata )
{
  U64 wrap_access_time=0x0;
  U32 reg_rdata=0;
  U32 wacs_write=0;
  U32 wacs_adr=0;
  U32 wacs_cmd=0;
  U32 return_value=0;
  unsigned long flags=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
  if (!pwrap_obj)
        PWRAPERR("NULL pointer\n");
  //PWRAPFUC();
  //#ifndef CONFIG_MTK_LDVT_PMIC_WRAP
  //PWRAPLOG("wrapper access,write=%x,add=%x,wdata=%x,rdata=%x\n",write,adr,wdata,rdata);
  //#endif
  // Check argument validation
  if( (write & ~(0x1))    != 0)  return E_PWR_INVALID_RW;
  if( (adr   & ~(0xffff)) != 0)  return E_PWR_INVALID_ADDR;
  if( (wdata & ~(0xffff)) != 0)  return E_PWR_INVALID_WDAT;

  spin_lock_irqsave(&pwrap_obj->spin_lock,flags);
  // Check IDLE & INIT_DONE in advance
  return_value=wait_for_state_idle(wait_for_fsm_idle,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS2_RDATA,PMIC_WRAP_WACS2_VLDCLR,0);
  if(return_value!=0)
  {
    PWRAPERR("wait_for_fsm_idle fail,return_value=%d\n",return_value);
    goto FAIL;
  }
  wacs_write  = write << 31;
  wacs_adr    = (adr >> 1) << 16;
  wacs_cmd= wacs_write | wacs_adr | wdata;

  WRAP_WR32(PMIC_WRAP_WACS2_CMD,wacs_cmd);
  if( write == 0 )
  {
    if (NULL == rdata)
    {
      PWRAPERR("rdata is a NULL pointer\n");
      return_value= E_PWR_INVALID_ARG;
      goto FAIL;
    }
    return_value=wait_for_state_ready(wait_for_fsm_vldclr,TIMEOUT_READ,PMIC_WRAP_WACS2_RDATA,&reg_rdata);
    if(return_value!=0)
    {
      PWRAPERR("wait_for_fsm_vldclr fail,return_value=%d\n",return_value);
      return_value+=1;//E_PWR_NOT_INIT_DONE_READ or E_PWR_WAIT_IDLE_TIMEOUT_READ
      goto FAIL;
    }
    *rdata = GET_WACS0_RDATA( reg_rdata );
    WRAP_WR32(PMIC_WRAP_WACS2_VLDCLR , 1);
  }
FAIL:
  spin_unlock_irqrestore(&pwrap_obj->spin_lock,flags);
  if(return_value!=0)
  {
    PWRAPERR("pwrap_wacs2 fail,return_value=%d\n",return_value);
    PWRAPERR("timeout:BUG_ON here\n");    
    //BUG_ON(1);      
  }
  wrap_access_time=sched_clock();
  pwrap_trace(wrap_access_time,return_value,write, adr, wdata,(U32)rdata);
  return return_value;
}
//******************************************************************************
//--internal API for pwrap_init-------------------------------------------------
//******************************************************************************
#if 0
static void pwrap_enable_clk(void)
{
  //enable_clock(MT65XX_PDN_MM_SPI, "pmic_wrap");
  return;
}

static void pwrap_disable_clk(void)
{
  //disable_clock(MT65XX_PDN_MM_SPI, "pmic_wrap");
  return;
}
#endif
//--------------------------------------------------------
//    Function : _pwrap_wacs2_nochk()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------

static S32 pwrap_read_nochk( U32  adr, U32 *rdata )
{
  return _pwrap_wacs2_nochk( 0, adr,  0, rdata );
}

static S32 pwrap_write_nochk( U32  adr, U32  wdata )
{
  return _pwrap_wacs2_nochk( 1, adr,wdata,0 );
}

S32 _pwrap_wacs2_nochk( U32 write, U32 adr, U32 wdata, U32 *rdata )
{
  U32 reg_rdata=0x0;
  U32 wacs_write=0x0;
  U32 wacs_adr=0x0;
  U32 wacs_cmd=0x0;
  U32 return_value=0x0;
  //PWRAPFUC();
  // Check argument validation
  if( (write & ~(0x1))    != 0)  return E_PWR_INVALID_RW;
  if( (adr   & ~(0xffff)) != 0)  return E_PWR_INVALID_ADDR;
  if( (wdata & ~(0xffff)) != 0)  return E_PWR_INVALID_WDAT;

  // Check IDLE
  return_value=wait_for_state_ready_init(wait_for_fsm_idle,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS2_RDATA,0);
  if(return_value!=0)
  {
    PWRAPERR("_pwrap_wacs2_nochk write command fail,return_value=%x\n", return_value);
    return return_value;
  }

  wacs_write  = write << 31;
  wacs_adr    = (adr >> 1) << 16;
  wacs_cmd = wacs_write | wacs_adr | wdata;
  WRAP_WR32(PMIC_WRAP_WACS2_CMD,wacs_cmd);

  if( write == 0 )
  {
    if (NULL == rdata)
      return E_PWR_INVALID_ARG;
    // wait for read data ready
    return_value=wait_for_state_ready_init(wait_for_fsm_vldclr,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS2_RDATA,&reg_rdata);
    if(return_value!=0)
    {
      PWRAPERR("_pwrap_wacs2_nochk read fail,return_value=%x\n", return_value);
      return return_value;
    }
    *rdata = GET_WACS0_RDATA( reg_rdata );
    WRAP_WR32(PMIC_WRAP_WACS2_VLDCLR , 1);
  }
  return 0;
}
//--------------------------------------------------------
//    Function : _pwrap_init_dio()
// Description :call it in pwrap_init,mustn't check init done
//   Parameter :
//      Return :
//--------------------------------------------------------
static S32 _pwrap_init_dio( U32 dio_en )
{
  U32 arb_en_backup=0x0;
  U32 rdata=0x0;
  U32 return_value=0;

  //PWRAPFUC();
  arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);
  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , 0x8); // only WACS2
  pwrap_write_nochk(DEW_DIO_EN, dio_en);

  // Check IDLE & INIT_DONE in advance
  return_value=wait_for_state_ready_init(wait_for_idle_and_sync,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS2_RDATA,0);
  if(return_value!=0)
  {
    PWRAPERR("_pwrap_init_dio fail,return_value=%x\n", return_value);
    return return_value;
  }
  WRAP_WR32(PMIC_WRAP_DIO_EN , dio_en);
  // Read Test
  pwrap_read_nochk(DEW_READ_TEST,&rdata);
  if( rdata != DEFAULT_VALUE_READ_TEST )
  {
    PWRAPERR("[Dio_mode][Read Test] fail,dio_en = %x, READ_TEST rdata=%x, exp=0x5aa5\n", dio_en, rdata);
    return E_PWR_READ_TEST_FAIL;
  }
  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , arb_en_backup);
  return 0;
}

//--------------------------------------------------------
//    Function : _pwrap_init_cipher()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
static S32 _pwrap_init_cipher( void )
{
  U32 arb_en_backup=0;
  U32 rdata=0;
  U32 return_value=0;
  U32 start_time_ns=0, timeout_ns=0;
  //PWRAPFUC();
  arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , 0x8); // only WACS0

  WRAP_WR32(PMIC_WRAP_CIPHER_SWRST , 1);
  WRAP_WR32(PMIC_WRAP_CIPHER_SWRST , 0);
  WRAP_WR32(PMIC_WRAP_CIPHER_KEY_SEL , 1);
  WRAP_WR32(PMIC_WRAP_CIPHER_IV_SEL  , 2);
  WRAP_WR32(PMIC_WRAP_CIPHER_LOAD    , 1);
  WRAP_WR32(PMIC_WRAP_CIPHER_START   , 1);

  //Config CIPHER @ PMIC
  pwrap_write_nochk(DEW_CIPHER_SWRST,   0x1);
  pwrap_write_nochk(DEW_CIPHER_SWRST,   0x0);
  pwrap_write_nochk(DEW_CIPHER_KEY_SEL, 0x1);
  pwrap_write_nochk(DEW_CIPHER_IV_SEL,  0x2);
  pwrap_write_nochk(DEW_CIPHER_LOAD,    0x1);
  pwrap_write_nochk(DEW_CIPHER_START,   0x1);

  //wait for cipher data ready@AP
  return_value=wait_for_state_ready_init(wait_for_cipher_ready,TIMEOUT_WAIT_IDLE,PMIC_WRAP_CIPHER_RDY,0);
  if(return_value!=0)
  {
    PWRAPERR("wait for cipher data ready@AP fail,return_value=%x\n", return_value);
    return return_value;
  }

  //wait for cipher data ready@PMIC
  start_time_ns = _pwrap_get_current_time();
  timeout_ns = _pwrap_time2ns(TIMEOUT_WAIT_IDLE);
  do
  {
    pwrap_read_nochk(DEW_CIPHER_RDY,&rdata);
    if (_pwrap_timeout_ns(start_time_ns, timeout_ns))
    {
      PWRAPERR("wait for cipher data ready:timeout when waiting for idle\n");
      return E_PWR_WAIT_IDLE_TIMEOUT;
    }
  } while( rdata != 0x1 ); //cipher_ready

  pwrap_write_nochk(DEW_CIPHER_MODE, 0x1);
  //wait for cipher mode idle
  return_value=wait_for_state_ready_init(wait_for_idle_and_sync,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS2_RDATA,0);
  if(return_value!=0)
  {
    PWRAPERR("wait for cipher mode idle fail,return_value=%x\n", return_value);
    return return_value;
  }
  WRAP_WR32(PMIC_WRAP_CIPHER_MODE , 1);

  // Read Test
  pwrap_read_nochk(DEW_READ_TEST,&rdata);
  if( rdata != DEFAULT_VALUE_READ_TEST )
  {
    PWRAPERR("_pwrap_init_cipher,read test error,error code=%x, rdata=%x\n", 1, rdata);
    return E_PWR_READ_TEST_FAIL;
  }

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , arb_en_backup);
  return 0;
}

//--------------------------------------------------------
//    Function : _pwrap_init_sidly()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
static S32 _pwrap_init_sidly( void )
{
  U32 arb_en_backup=0;
  U32 rdata=0;
  U32 ind=0;
  U32 result=0;
  U32 result_faulty=0;
  //PWRAPFUC();
  arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);
  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , 0x8); // only WACS2

  // Scan all SIDLY by Read Test
  result = 0;
  for( ind=0 ; ind<4 ; ind++)
  {
    WRAP_WR32(PMIC_WRAP_SIDLY , ind);
    pwrap_read_nochk(DEW_READ_TEST,&rdata);
    if( rdata == DEFAULT_VALUE_READ_TEST )
    {
      PWRAPLOG("_pwrap_init_sidly [Read Test] pass,SIDLY=%x rdata=%x\n", ind,rdata);
      result |= (0x1 << ind);
    }
    else
      PWRAPLOG("_pwrap_init_sidly [Read Test] fail,SIDLY=%x,rdata=%x\n", ind,rdata);
  }

  // Config SIDLY according to result
  switch( result )
  {
    // Only 1 pass, choose it
    case 0x1:
      WRAP_WR32(PMIC_WRAP_SIDLY , 0);
      break;
    case 0x2:
      WRAP_WR32(PMIC_WRAP_SIDLY , 1);
      break;
    case 0x4:
      WRAP_WR32(PMIC_WRAP_SIDLY , 2);
      break;
    case 0x8:
      WRAP_WR32(PMIC_WRAP_SIDLY , 3);
      break;

    // two pass, choose the one on SIDLY boundary
    case 0x3:
      WRAP_WR32(PMIC_WRAP_SIDLY , 0);
      break;
    case 0x6:
      WRAP_WR32(PMIC_WRAP_SIDLY , 1); //no boundary, choose smaller one
      break;
    case 0xc:
      WRAP_WR32(PMIC_WRAP_SIDLY , 3);
      break;

    // three pass, choose the middle one
    case 0x7:
      WRAP_WR32(PMIC_WRAP_SIDLY , 1);
      break;
    case 0xe:
      WRAP_WR32(PMIC_WRAP_SIDLY , 2);
      break;
    // four pass, choose the smaller middle one
    case 0xf:
      WRAP_WR32(PMIC_WRAP_SIDLY , 1);
      break;

    // pass range not continuous, should not happen
    default:
      WRAP_WR32(PMIC_WRAP_SIDLY , 0);
      result_faulty = 0x1;
      break;
  }

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , arb_en_backup);
  if( result_faulty == 0 )
    return 0;
  else
  {
    PWRAPERR("error,_pwrap_init_sidly fail,result=%x\n",result);
    return result_faulty;
  }
}

//--------------------------------------------------------
//    Function : _pwrap_reset_spislv()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------

S32 _pwrap_reset_spislv( void )
{
  U32 ret=0;
  U32 return_value=0;
  //PWRAPFUC();
  // This driver does not using _pwrap_switch_mux
  // because the remaining requests are expected to fail anyway

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , 0);
  WRAP_WR32(PMIC_WRAP_WRAP_EN , 0);
  WRAP_WR32(PMIC_WRAP_MUX_SEL , 1);
  WRAP_WR32(PMIC_WRAP_MAN_EN ,1);
  WRAP_WR32(PMIC_WRAP_DIO_EN , 0);

  WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_CSL  << 8));
  WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8)); //to reset counter
  WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_CSH  << 8));
  WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8));
  WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8));
  WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8));
  WRAP_WR32(PMIC_WRAP_MAN_CMD , (OP_WR << 13) | (OP_OUTS << 8));

  return_value=wait_for_state_ready_init(wait_for_sync,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS2_RDATA,0);
  if(return_value!=0)
  {
    PWRAPERR("_pwrap_reset_spislv fail,return_value=%x\n", return_value);
    ret=E_PWR_TIMEOUT;
    goto timeout;
  }

  WRAP_WR32(PMIC_WRAP_MAN_EN , 0);
  WRAP_WR32(PMIC_WRAP_MUX_SEL , 0);

timeout:
  WRAP_WR32(PMIC_WRAP_MAN_EN , 0);
  WRAP_WR32(PMIC_WRAP_MUX_SEL , 0);
  return ret;
}

static S32 _pwrap_init_reg_clock( U32 regck_sel )
{
  U32 wdata=0;
  U32 rdata=0;

  // Set reg clk freq
  pwrap_read_nochk(PMIC_TOP_CKCON2,&rdata);

  if( regck_sel == 1 )
    wdata = (rdata & (~(0x3<<10))) | (0x1<<10);
  else
    wdata = rdata & (~(0x3<<10));

  pwrap_write_nochk(PMIC_TOP_CKCON2, wdata);
  pwrap_read_nochk(PMIC_TOP_CKCON2, &rdata);
  if( rdata != wdata ) {
    PWRAPERR("_pwrap_init_reg_clock,PMIC_TOP_CKCON2 Write [15]=1 Fail, rdata=%x\n",rdata);
    return E_PWR_WRITE_TEST_FAIL;
  }

  // Config SPI Waveform according to reg clk
  if( regck_sel == 1 ) { //18MHz
    WRAP_WR32(PMIC_WRAP_CSHEXT, 0xc);
    WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE   , 0x4);
    WRAP_WR32(PMIC_WRAP_CSHEXT_READ    , 0xc);
    WRAP_WR32(PMIC_WRAP_CSLEXT_START   , 0x0);
    WRAP_WR32(PMIC_WRAP_CSLEXT_END     , 0x0);
  } else if( regck_sel == 2 ){ //36MHz
    WRAP_WR32(PMIC_WRAP_CSHEXT         , 0x4);
    WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE   , 0x0);
    WRAP_WR32(PMIC_WRAP_CSHEXT_READ    , 0x4);
    WRAP_WR32(PMIC_WRAP_CSLEXT_START   , 0x0);
    WRAP_WR32(PMIC_WRAP_CSLEXT_END     , 0x0);
  } else { //Safe mode
    WRAP_WR32(PMIC_WRAP_CSHEXT         , 0xf);
    WRAP_WR32(PMIC_WRAP_CSHEXT_WRITE   , 0xf);
    WRAP_WR32(PMIC_WRAP_CSHEXT_READ    , 0xf);
    WRAP_WR32(PMIC_WRAP_CSLEXT_START   , 0xf);
    WRAP_WR32(PMIC_WRAP_CSLEXT_END     , 0xf);
  }

  return 0;
}

/*Interrupt handler function*/
static irqreturn_t mt_pmic_wrap_interrupt(int irqno, void *dev_id)
{
  U32 i=0;
  unsigned long flags=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;

  PWRAPFUC();
  PWRAPREG("dump pwrap register\n");
  spin_lock_irqsave(&pwrap_obj->spin_lock_isr,flags);
  //*-----------------------------------------------------------------------
  pwrap_dump_all_register();
  //*-----------------------------------------------------------------------
  //print the latest access of pmic
  PWRAPREG("the latest 20 access of pmic is following.\n");
  if(pwrap_debug_index>=20)
  {
    for(i=pwrap_debug_index-20;i<=pwrap_debug_index;i++)
      PWRAPREG("index=%d,time=%llx,operation=%x,addr=%x,wdata=%x,rdata=%x\n",i,
        pwrap_debug_data[i].wacs_time,pwrap_debug_data[i].operation,pwrap_debug_data[i].addr,
      pwrap_debug_data[i].wdata,pwrap_debug_data[i].rdata);
  }
  else //PWRAP_DEBUG_COUNT=100
  {
    for(i=PWRAP_DEBUG_COUNT+pwrap_debug_index-20;i<PWRAP_DEBUG_COUNT;i++)
      PWRAPREG("index=%d,time=%llx,operation=%x,addr=%x,wdata=%x,rdata=%x\n",i,
        pwrap_debug_data[i].wacs_time,pwrap_debug_data[i].operation,pwrap_debug_data[i].addr,
        pwrap_debug_data[i].wdata,pwrap_debug_data[i].rdata);
    for(i=0;i<=pwrap_debug_index;i++)
      PWRAPREG("index=%d,time=%llx,option=%x,addr=%x,wdata=%x,rdata=%x\n",i,
        pwrap_debug_data[i].wacs_time,pwrap_debug_data[i].operation,pwrap_debug_data[i].addr,
        pwrap_debug_data[i].wdata,pwrap_debug_data[i].rdata);
  }
  //raise the priority of WACS2 for AP
  WRAP_WR32(PMIC_WRAP_HARB_HPRIO,1<<3);

  //*-----------------------------------------------------------------------
  //clear interrupt flag
  WRAP_WR32(PMIC_WRAP_INT_CLR, 0xffffffff);
  BUG_ON(1);
  spin_unlock_irqrestore(&pwrap_obj->spin_lock_isr,flags);
  return IRQ_HANDLED;
}
//--------------------------------------------------------
//    Function : _pwrap_AlignCRC()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
U32 _pwrap_AlignCRC( void )
{
  U32 arb_en_backup=0;
  U32 staupd_prd_backup=0;
  U32 return_value=0;
  //Backup Configuration & Set New Ones
  arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);
  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , 0x8); // only WACS2
  staupd_prd_backup = WRAP_RD32(PMIC_WRAP_STAUPD_PRD);
  WRAP_WR32(PMIC_WRAP_STAUPD_PRD , 0); //disable STAUPD

  // reset CRC
  return_value=pwrap_write(DEW_CRC_EN, 0);
  //PWRAPLOG("reset CR return_value=%d\n",return_value);
  WRAP_WR32(PMIC_WRAP_CRC_EN , 0);

  //Wait for FSM to be IDLE
  return_value=wait_for_state_ready_init(wait_for_wrap_state_idle,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WRAP_STA,0);
  if(return_value!=0)
  {
    PWRAPERR("AlignCRC wait for DLE fail=%d\n",return_value);
    return return_value;
  }

  // Enable CRC
  return_value=pwrap_write(DEW_CRC_EN, 1);
  //PWRAPLOG("Enable CRC return_value=%d\n",return_value);
  WRAP_WR32(PMIC_WRAP_CRC_EN , 1);

  //restore Configuration
  WRAP_WR32(PMIC_WRAP_STAUPD_PRD , staupd_prd_backup);
  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , arb_en_backup);
  return 0;
}

/*
*pmic_wrap init,init wrap interface
*
*/
S32 pwrap_init ( void )
{
  S32 sub_return=0;
  S32 sub_return1=0;
  U32 rdata=0x0;
  //U32 timeout=0;
  PWRAPFUC();
  //###############################
  // Reset related modules
  // PMIC_WRAP, PERI_PWRAP_BRIDGE, PWRAP_SPICTL
  // Subject to project
  //###############################
  WRAP_SET_BIT(0x80,INFRA_GLOBALCON_RST0);
  WRAP_SET_BIT(0x04,PERI_GLOBALCON_RST1);
  rdata=WRAP_RD32(WDT_SWSYSRST);
  WRAP_WR32(WDT_SWSYSRST,(0x88000000 | ((rdata | (0x1 << 11)) & 0x00ffffff )));

  WRAP_CLR_BIT(0x80,INFRA_GLOBALCON_RST0);
  WRAP_CLR_BIT(0x04,PERI_GLOBALCON_RST1);
  rdata=WRAP_RD32(WDT_SWSYSRST);
  WRAP_WR32(WDT_SWSYSRST,(0x88000000 | ((rdata & (~(0x1 << 11))) & 0x00ffffff )));


  //###############################
  //Enable DCM
  //###############################
   WRAP_WR32(PMIC_WRAP_DCM_EN , 1);
   WRAP_WR32(PMIC_WRAP_DCM_DBC_PRD ,0);

  //###############################
  //Reset SPISLV
  //###############################
  sub_return=_pwrap_reset_spislv();
  if( sub_return != 0 )
  {
    PWRAPERR("error,_pwrap_reset_spislv fail,sub_return=%x\n",sub_return);
    return E_PWR_INIT_RESET_SPI;
  }
  //###############################
  // Enable WACS2
  //###############################
  WRAP_WR32(PMIC_WRAP_WRAP_EN,1);//enable wrap
  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN,8); //Only WACS2
  WRAP_WR32(PMIC_WRAP_WACS2_EN,1);

  //###############################
  //TBD: Set SPI_CK freq = 66MHz
  //###############################
  //WRAP_WR32(CLK_CFG_8, 5);
  rdata = WRAP_RD32(CLK_CFG_8);
  WRAP_WR32(CLK_CFG_8, (rdata & ~0x7) | 0x5);

  //###############################
  // SIDLY setting
  //###############################
  sub_return = _pwrap_init_sidly();
  //sub_return = 0; //TBD
  if( sub_return != 0 )
  {
    PWRAPERR("error,_pwrap_init_sidly fail,sub_return=%x\n",sub_return);
    return E_PWR_INIT_SIDLY;
  }
  //###############################
  // SPI Waveform Configuration
  //###############################
  sub_return = _pwrap_init_reg_clock(2); //0:safe mode, 1:18MHz, 2:36MHz //2
  if( sub_return != 0)  {
    PWRAPERR("error,_pwrap_init_reg_clock fail,sub_return=%x\n",sub_return);
    return E_PWR_INIT_REG_CLOCK;
  }

  //###############################
  // Enable PMIC
  // (May not be necessary, depending on S/W partition)
  //###############################
  sub_return= pwrap_write_nochk(PMIC_WRP_CKPDN,   0);//set dewrap clock bit
  sub_return1=pwrap_write_nochk(PMIC_WRP_RST_CON, 0);//clear dewrap reset bit
  if(( sub_return != 0 )||( sub_return1 != 0 ))
  {
    PWRAPERR("Enable PMIC fail, sub_return=%x sub_return1=%x\n", sub_return,sub_return1);
    return E_PWR_INIT_ENABLE_PMIC;
  }
  //###############################
  // Enable DIO mode
  //###############################
  sub_return = _pwrap_init_dio(1);
  if( sub_return != 0 )
  {
    PWRAPERR("_pwrap_init_dio test error,error code=%x, sub_return=%x\n", 0x11, sub_return);
    return E_PWR_INIT_DIO;
  }

  //###############################
  // Enable Encryption
  //###############################
  sub_return = _pwrap_init_cipher();
  if( sub_return != 0 )
  {
    PWRAPERR("Enable Encryption fail, return=%x\n", sub_return);
    return E_PWR_INIT_CIPHER;
  }

  //###############################
  // Write test using WACS2
  //###############################
  sub_return = pwrap_write_nochk(DEW_WRITE_TEST, WRITE_TEST_VALUE);
  sub_return1 = pwrap_read_nochk(DEW_WRITE_TEST,&rdata);
  if(( rdata != WRITE_TEST_VALUE )||( sub_return != 0 )||( sub_return1 != 0 ))
  {
    PWRAPERR("write test error,rdata=0x%x,exp=0xa55a,sub_return=0x%x,sub_return1=0x%x\n", rdata,sub_return,sub_return1);
    return E_PWR_INIT_WRITE_TEST;
  }

  //###############################
  // Signature Checking - Using Write Test Register
  // should be the last to modify WRITE_TEST
  //###############################
  #if 0 //old mode
  _pwrap_wacs2_nochk(1, DEW_WRITE_TEST, 0x5678, &rdata);
  WRAP_WR32(PMIC_WRAP_SIG_ADR,DEW_WRITE_TEST);
  WRAP_WR32(PMIC_WRAP_SIG_VALUE,0x5678);
  WRAP_WR32(PMIC_WRAP_SIG_MODE, 0x1);
  #endif

  //###############################
  // Signature Checking - Using CRC
  // should be the last to modify WRITE_TEST
  //###############################
  sub_return=pwrap_write_nochk(DEW_CRC_EN, 0x1);
  if( sub_return != 0 )
  {
    PWRAPERR("enable CRC fail,sub_return=%x\n", sub_return);
    return E_PWR_INIT_ENABLE_CRC;
  }
  WRAP_WR32(PMIC_WRAP_CRC_EN ,0x1);
  WRAP_WR32(PMIC_WRAP_SIG_MODE, 0x0);
  WRAP_WR32(PMIC_WRAP_SIG_ADR , DEW_CRC_VAL);


  //###############################
  // PMIC_WRAP enables
  //###############################
  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN,0x1ff);
  WRAP_WR32(PMIC_WRAP_RRARB_EN ,0x7);
  WRAP_WR32(PMIC_WRAP_WACS0_EN,0x1);
  WRAP_WR32(PMIC_WRAP_WACS1_EN,0x1);
  WRAP_WR32(PMIC_WRAP_WACS2_EN,0x1);//already enabled
  WRAP_WR32(PMIC_WRAP_EVENT_IN_EN,0x1);
  WRAP_WR32(PMIC_WRAP_EVENT_DST_EN,0xffff);
  WRAP_WR32(PMIC_WRAP_STAUPD_PRD, 0x5);  //0x1:20us,for concurrence test,MP:0x5;  //100us
  WRAP_WR32(PMIC_WRAP_STAUPD_GRPEN,0xff);
  WRAP_WR32(PMIC_WRAP_WDT_UNIT,0xf);
  WRAP_WR32(PMIC_WRAP_WDT_SRC_EN,0xffffffff);
  WRAP_WR32(PMIC_WRAP_TIMER_EN,0x1);
  WRAP_WR32(PMIC_WRAP_INT_EN,0x7ffffffd); //except for [31] debug_int

  //###############################
  // PERI_PWRAP_BRIDGE enables
  //###############################
  WRAP_WR32(PERI_PWRAP_BRIDGE_IORD_ARB_EN, 0x7f);
  WRAP_WR32(PERI_PWRAP_BRIDGE_WACS3_EN , 0x1);
  WRAP_WR32(PERI_PWRAP_BRIDGE_WACS4_EN ,0x1);
  WRAP_WR32(PERI_PWRAP_BRIDGE_WDT_UNIT , 0xf);
  WRAP_WR32(PERI_PWRAP_BRIDGE_WDT_SRC_EN , 0xffff);
  WRAP_WR32(PERI_PWRAP_BRIDGE_TIMER_EN  , 0x1);
  WRAP_WR32(PERI_PWRAP_BRIDGE_INT_EN    , 0x7ff);

  //###############################
  // PMIC_DEWRAP enables
  //###############################
  sub_return  = pwrap_write_nochk(DEW_EVENT_OUT_EN, 0x1);
  sub_return1 = pwrap_write_nochk(DEW_EVENT_SRC_EN, 0xffff);
  if(( sub_return != 0 )||( sub_return1 != 0 ))
  {
    PWRAPERR("enable dewrap fail,sub_return=%d,sub_return1=%d\n", sub_return,sub_return1);
    return E_PWR_INIT_ENABLE_DEWRAP;
  }
  //###############################
  // Initialization Done
  //###############################
  WRAP_WR32(PMIC_WRAP_INIT_DONE2 , 0x1);

  //###############################
  //TBD: Should be configured by MD MCU
  //###############################
  #if 1  //def CONFIG_MTK_LDVT_PMIC_WRAP
    WRAP_WR32(PMIC_WRAP_INIT_DONE0 ,1);
    WRAP_WR32(PMIC_WRAP_INIT_DONE1 , 1);
    WRAP_WR32(PERI_PWRAP_BRIDGE_INIT_DONE3 , 1);
    WRAP_WR32(PERI_PWRAP_BRIDGE_INIT_DONE4 , 1);
  #endif
  return 0;
}
void pwrap_power_on(void)
{
  //U32 reg_addr=0;
  //U32 reg_value=0;
  WRAP_SET_BIT(0x1<<22,INFRA_GLOBALCON_PDN1);//SPI clock
  WRAP_SET_BIT(0x1<<23,INFRA_GLOBALCON_PDN1);//QAXI clock

  WRAP_SET_BIT(0x1<<1,PERI_GLOBALCON_PDN1_CLR);//PWRAP_BRIDGE QAXI clock
  WRAP_SET_BIT(0x1<<1,CLK_PDN_CLR);//SPIctl SPI clock
}
void pwrap_power_off(void)
{
  WRAP_SET_BIT(0x1<<22,INFRA_GLOBALCON_PDN0);//SPI clock
  WRAP_SET_BIT(0x1<<23,INFRA_GLOBALCON_PDN0);//QAXI clock

  WRAP_SET_BIT(0x1<<1,PERI_GLOBALCON_PDN1_SET);//PWRAP_BRIDGE QAXI clock
  WRAP_SET_BIT(0x1<<1,CLK_PDN_SET);//SPIctl SPI clock

}

/*-pwrap debug--------------------------------------------------------------------------*/
void pwrap_dump_all_register(void)
{
  U32 i=0;
  U32 reg_addr=0;
  U32 reg_value=0;
  PWRAPREG("dump pwrap register\n");
  for(i=0;i<=89;i++)
  {
    reg_addr=(PMIC_WRAP_BASE+i*4);
    reg_value=WRAP_RD32(reg_addr);
    PWRAPREG("0x%x=0x%x\n",reg_addr,reg_value);
  }
  PWRAPREG("dump peri_pwrap register\n");
  for(i=0;i<=24;i++)
  {
    reg_addr=(PERI_PWRAP_BRIDGE_BASE+i*4);
    reg_value=WRAP_RD32(reg_addr);
    PWRAPREG("0x%x=0x%x\n",reg_addr,reg_value);
  }
  PWRAPREG("dump dewrap register\n");
  for(i=0;i<=25;i++)
  {
    reg_addr=(DEW_BASE+i*4);
    reg_value=pwrap_read_nochk(reg_addr,&reg_value);
    PWRAPREG("0x%x=0x%x\n",reg_addr,reg_value);
  }
}
void pwrap_dump_ap_register(void)
{
  U32 i=0;
  U32 reg_addr=0;
  U32 reg_value=0;
  PWRAPREG("dump pwrap register\n");
  for(i=0;i<=89;i++)
  {
    reg_addr=(PMIC_WRAP_BASE+i*4);
    reg_value=WRAP_RD32(reg_addr);
    PWRAPREG("0x%x=0x%x\n",reg_addr,reg_value);
  }
  PWRAPREG("dump peri_pwrap register\n");
  for(i=0;i<=24;i++)
  {
    reg_addr=(PERI_PWRAP_BRIDGE_BASE+i*4);
    reg_value=WRAP_RD32(reg_addr);
    PWRAPREG("0x%x=0x%x\n",reg_addr,reg_value);
  }
  PWRAPREG("elapse_time=%llx(ns)\n",elapse_time);
}

static void pwrap_trace_wacs2(void)
{
  U32 i=0;
  //print the latest access of pmic
  PWRAPREG("the latest 20 access of pmic is following.\n");
  if(pwrap_debug_index>=20)
  {
    for(i=pwrap_debug_index-20;i<=pwrap_debug_index;i++)
      PWRAPREG("index=%d,time=%llx,operation=%x,result=%d,addr=%x,wdata=%x,rdata=%x\n",i,
        pwrap_debug_data[i].wacs_time,pwrap_debug_data[i].operation,pwrap_debug_data[i].result,pwrap_debug_data[i].addr,
      pwrap_debug_data[i].wdata,pwrap_debug_data[i].rdata);
  }
  else //PWRAP_DEBUG_COUNT=100
  {
    for(i=PWRAP_DEBUG_COUNT+pwrap_debug_index-20;i<PWRAP_DEBUG_COUNT;i++)
      PWRAPREG("index=%d,time=%llx,operation=%x,result=%d,addr=%x,wdata=%x,rdata=%x\n",i,
        pwrap_debug_data[i].wacs_time,pwrap_debug_data[i].operation,pwrap_debug_data[i].result,pwrap_debug_data[i].addr,
        pwrap_debug_data[i].wdata,pwrap_debug_data[i].rdata);
    for(i=0;i<=pwrap_debug_index;i++)
      PWRAPREG("index=%d,time=%llx,operation=%x,result=%d,addr=%x,wdata=%x,rdata=%x\n",i,
        pwrap_debug_data[i].wacs_time,pwrap_debug_data[i].operation,pwrap_debug_data[i].result,pwrap_debug_data[i].addr,
        pwrap_debug_data[i].wdata,pwrap_debug_data[i].rdata);
  }
}
static void pwrap_read_reg_on_ap(U32 reg_addr)
{
  U32 reg_value=0;
  reg_value=WRAP_RD32(reg_addr);
  PWRAPREG("0x%x=0x%x\n",reg_addr,reg_value);
}

static void pwrap_write_reg_on_ap(U32 reg_addr,U32 reg_value)
{
  PWRAPREG("write 0x%x to register 0x%x\n",reg_value,reg_addr);
  WRAP_WR32(reg_addr,reg_value);
  reg_value=WRAP_RD32(reg_addr);
  PWRAPREG("the result:0x%x=0x%x\n",reg_addr,reg_value);
}

static void pwrap_read_reg_on_pmic(U32 reg_addr)
{
  U32 reg_value=0;
  U32 return_value=0;
  //PWRAPFUC();
  return_value=pwrap_read(reg_addr, &reg_value);
  PWRAPREG("0x%x=0x%x,return_value=%x\n",reg_addr,reg_value,return_value);
}

static void pwrap_write_reg_on_pmic(U32 reg_addr,U32 reg_value)
{
  U32 return_value=0;
  PWRAPREG("write 0x%x to register 0x%x\n",reg_value,reg_addr);
  return_value=pwrap_write(reg_addr, reg_value);
  return_value=pwrap_read(reg_addr, &reg_value);
  //PWRAPFUC();
  PWRAPREG("the result:0x%x=0x%x,return_value=%x\n",reg_addr,reg_value,return_value);
}
U32 pwrap_read_test(void)
{
  U32 rdata=0;
  U32 return_value=0;
  // Read Test
  return_value=pwrap_read(DEW_READ_TEST,&rdata);
  if( rdata != DEFAULT_VALUE_READ_TEST )
  {
    PWRAPREG("Read Test fail,rdata=0x%x, exp=0x5aa5,return_value=0x%x\n", rdata,return_value);
    return E_PWR_READ_TEST_FAIL;
  }
  else
  {
    PWRAPREG("Read Test pass,return_value=%d\n",return_value);
    return 0;
}
}
U32 pwrap_write_test(void)
{
  U32 rdata=0;
  U32 sub_return=0;
  U32 sub_return1=0;
  //###############################
  // Write test using WACS2
  //###############################
  sub_return = pwrap_write(DEW_WRITE_TEST, WRITE_TEST_VALUE);
  PWRAPREG("after pwrap_write_nochk\n");
  sub_return1 = pwrap_read(DEW_WRITE_TEST,&rdata);
  if(( rdata != WRITE_TEST_VALUE )||( sub_return != 0 )||( sub_return1 != 0 ))
  {
    PWRAPREG("write test error,rdata=0x%x,exp=0xa55a,sub_return=0x%x,sub_return1=0x%x\n", rdata,sub_return,sub_return1);
    return E_PWR_INIT_WRITE_TEST;
  }
  else
  {
    PWRAPREG("write Test pass\n");
    return 0;
  }
}
#define WRAP_ACCESS_TEST_REG DEW_CIPHER_IV1
static void pwrap_wacs2_para_test(void)
{
  U32 return_value=0;
  U32 result=0;
  U32 rdata=0;
  //test 1st parameter--------------------------------------------
  return_value=pwrap_wacs2(3, WRAP_ACCESS_TEST_REG, 0x1234, &rdata);
  if( return_value != 0 )
  {
    PWRAPREG("pwrap_wacs2_para_test 1st para,return_value=%x\n", return_value);
    result+=1;
  }
  //test 2nd parameter--------------------------------------------
  return_value=pwrap_wacs2(0, 0xffff+0x10, 0x1234, &rdata);
  if( return_value != 0 )
  {
    PWRAPREG("pwrap_wacs2_para_test 2nd para,return_value=%x\n", return_value);
    result+=1;
  }
  //test 3rd parameter--------------------------------------------
  return_value=pwrap_wacs2(0, WRAP_ACCESS_TEST_REG, 0xffff+0x10, &rdata);
  if( return_value != 0 )
  {
    PWRAPREG("pwrap_wacs2_para_test 3rd para,return_value=%x\n", return_value);
    result+=1;
  }
  //test 4th parameter--------------------------------------------
  return_value=pwrap_wacs2(0, WRAP_ACCESS_TEST_REG, 0x1234, 0);
  if( return_value != 0 )
  {
    PWRAPREG("pwrap_wacs2_para_test 4th para,return_value=%x\n", return_value);
    result+=1;
  }
  if(result==4)
    PWRAPREG("pwrap_wacs2_para_test pass\n");
  else
    PWRAPREG("pwrap_wacs2_para_test fail\n");
}
static void pwrap_ut(U32 ut_test)
{
  switch(ut_test)
  {
  case 1:
    pwrap_wacs2_para_test();
    break;
  case 2:
    //pwrap_wacs2_para_test();
    break;

  default:
    PWRAPREG ( "default test.\n" );
    break;
  }
}


/*---------------------------------------------------------------------------*/
static ssize_t mt_pwrap_show(struct device* dev,
                                struct device_attribute *attr, char *buf)
{
    PWRAPFUC();
    return 0;
}
/*---------------------------------------------------------------------------*/
static ssize_t mt_pwrap_store(struct device* dev, struct device_attribute *attr,
                                 const char *buf, size_t count)
{
  U32 reg_value=0;
  U32 reg_addr=0;
  U32 return_value=0;
  U32 ut_test=0;
  if(!strncmp(buf, "-h", 2))
  {
    PWRAPREG("PWRAP debug: [-dump_reg][-trace_wacs2][-init][-rdap][-wrap][-rdpmic][-wrpmic][-readtest][-writetest]\n");
    PWRAPREG("PWRAP UT: [1][2]\n");
  }
  //--------------------------------------pwrap debug-------------------------------------------------------------
  else if(!strncmp(buf, "-dump_reg", 9))
  {
    pwrap_dump_all_register();
  }
  else if(!strncmp(buf, "-trace_wacs2", 12))
  {
    pwrap_trace_wacs2();
  }
  else if(!strncmp(buf, "-init", 5))
  {
    return_value=pwrap_init();
    if(return_value==0)
      PWRAPREG("pwrap_init pass,return_value=%d\n",return_value);
    else
      PWRAPREG("pwrap_init fail,return_value=%d\n",return_value);
  }
  else if (!strncmp(buf, "-rdap", 5) && (1 == sscanf(buf+5, "%x", &reg_addr)))
  {
    pwrap_read_reg_on_ap(reg_addr);
  }
  else if (!strncmp(buf, "-wrap", 5) && (2 == sscanf(buf+5, "%x %x", &reg_addr,&reg_value)))
  {
    pwrap_write_reg_on_ap(reg_addr,reg_value);
  }
  else if (!strncmp(buf, "-rdpmic", 7) && (1 == sscanf(buf+7, "%x", &reg_addr)))
  {
    pwrap_read_reg_on_pmic(reg_addr);
  }
  else if (!strncmp(buf, "-wrpmic", 7) && (2 == sscanf(buf+7, "%x %x", &reg_addr,&reg_value)))
  {
    pwrap_write_reg_on_pmic(reg_addr,reg_value);
  }
  else if(!strncmp(buf, "-readtest", 9))
  {
    pwrap_read_test();
  }
  else if(!strncmp(buf, "-writetest", 10))
  {
    pwrap_write_test();
  }
  //--------------------------------------pwrap UT-------------------------------------------------------------
  else if (!strncmp(buf, "-ut", 3) && (1 == sscanf(buf+3, "%d", &ut_test)))
  {
    pwrap_ut(ut_test);
  }
    return count;
}
/*---------------------------------------------------------------------------*/
static DEVICE_ATTR(pwrap, 0664, mt_pwrap_show,   mt_pwrap_store);
/*---------------------------------------------------------------------------*/
static struct device_attribute *pmic_wrap_attr_list[] = {
    &dev_attr_pwrap,
};
/*---------------------------------------------------------------------------*/
static int mt_pwrap_create_attr(struct device *dev)
{
    int idx=0, err = 0;
    int num = (int)(sizeof(pmic_wrap_attr_list)/sizeof(pmic_wrap_attr_list[0]));
    if (!dev)
        return -EINVAL;

    for (idx = 0; idx < num; idx++) {
        if ((err = device_create_file(dev, pmic_wrap_attr_list[idx])))
            break;
    }

    return err;
}
/*---------------------------------------------------------------------------*/
static int mt_pwrap_delete_attr(struct device *dev)
{
    int idx=0 ,err = 0;
    int num = (int)(sizeof(pmic_wrap_attr_list)/sizeof(pmic_wrap_attr_list[0]));

    if (!dev)
        return -EINVAL;

    for (idx = 0; idx < num; idx++)
        device_remove_file(dev, pmic_wrap_attr_list[idx]);

    return err;
}

/*****************************************************************************/
/* File operation                                                            */
/*****************************************************************************/
static int mt_pwrap_open(struct inode *inode, struct file *file)
{
    struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
    //PWRAPFUC();

    if (!pwrap_obj) {
        PWRAPERR("NULL pointer\n");
        return -EFAULT;
    }

    atomic_inc(&pwrap_obj->ref);
    file->private_data = pwrap_obj;
    return nonseekable_open(inode, file);
}
/*---------------------------------------------------------------------------*/
static int mt_pwrap_release(struct inode *inode, struct file *file)
{
    struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;

    //PWRAPFUC();

    if (!pwrap_obj) {
        PWRAPERR("NULL pointer\n");
        return -EFAULT;
    }

    atomic_dec(&pwrap_obj->ref);
    return 0;
}

/*---------------------------------------------------------------------------*/
static struct file_operations mt_pmic_wrap_fops =
{
    .owner=        THIS_MODULE,
    #ifdef CONFIG_MTK_LDVT_PMIC_WRAP
    .unlocked_ioctl=        mt_pwrap_ioctl,
    #endif
    .open=         mt_pwrap_open,
    .release=      mt_pwrap_release,
};
//#define PWRAP_INIT_RELEASE
#define PMIC_WRAP_DEVICE "pmic_wrap"
#define VERSION     "$Revision$"

/*----------------------------------------------------------------------------*/
static struct miscdevice mt_pmic_wrap_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "pmic_wrap",
    .fops = &mt_pmic_wrap_fops,
};
/*---------------------------------------------------------------------------*/
static int mt_pwrap_probe(struct platform_device *pdev)
{
    int err=0;
    struct miscdevice *pwrap_dev = &mt_pmic_wrap_device;
    U32 ret=0;

    PWRAPLOG("Registering PWRAP device\n");

    if (!g_pmic_wrap_obj)
    {
        PWRAPERR("pwrap_obj can't be NULL\n");
        return -EACCES;
    }
    g_pmic_wrap_obj->misc = pwrap_dev;

    if ((err = misc_register(pwrap_dev)))
        PWRAPERR("register PWRAP\n");

    if ((err = mt_pwrap_create_attr(pwrap_dev->this_device)))
        PWRAPERR("create attribute\n");

    dev_set_drvdata(pwrap_dev->this_device, g_pmic_wrap_obj);
    #ifndef CONFIG_MTK_LDVT_PMIC_WRAP
      ret = request_irq(MT6589_PMIC_WRAP_IRQ_ID, mt_pmic_wrap_interrupt, IRQF_TRIGGER_HIGH, PMIC_WRAP_DEVICE, NULL);
    #else
      ret = request_irq(MT6589_PMIC_WRAP_IRQ_ID, pwrap_interrupt_for_ldvt, IRQF_TRIGGER_HIGH, PMIC_WRAP_DEVICE, NULL);
    #endif
    if (ret) {
      PWRAPERR("register IRQ failed (%d)\n", ret);
      misc_deregister(pwrap_dev);
      return ret;
    }

    return err;
}
/*---------------------------------------------------------------------------*/
static int mt_pwrap_remove(struct platform_device *dev)
{
    struct pmic_wrap_obj *obj = platform_get_drvdata(dev);
    int err=0;

    if ((err = mt_pwrap_delete_attr(obj->misc->this_device)))
        PWRAPERR("delete attr\n");

    if ((err = misc_deregister(obj->misc)))
        PWRAPERR("deregister PWRAP\n");

    return err;
}
/*---------------------------------------------------------------------------*/
static void mt_pwrap_shutdown(struct platform_device *dev)
{
    PWRAPLOG("shutdown\n");
}
#if 0
/*---------------------------------------------------------------------------*/
static int mt_pwrap_suspend(struct platform_device *dev, pm_message_t state)
{
    PWRAPLOG("suspend: %d!\n", state.event);
    return 0;
}
/*---------------------------------------------------------------------------*/
static int mt_pwrap_resume(struct platform_device *dev)
{
  PWRAPLOG("PWRAP Resume !\n");
    return 0;
}
#endif
/*---------------------------------------------------------------------------*/
static struct platform_driver mt_pwrap_driver =
{
    .probe          = mt_pwrap_probe,
    .remove         = mt_pwrap_remove,
    .shutdown       = mt_pwrap_shutdown,
    //.suspend        = mt_pwrap_suspend,
    //.resume         = mt_pwrap_resume,
    .driver         = {
            .name = PMIC_WRAP_DEVICE,
        },
};
/*---------------------------------------------------------------------------*/
static int __init mt_pwrap_init(void)
{
  U32 ret = 0;
  U32 rdata=0;
  PWRAPLOG("MT6589 PWRAP: version %s\n", VERSION);

  ret = platform_driver_register(&mt_pwrap_driver);
  //gating eint/i2c/pwm/kpd clock@PMIC
  pwrap_read_nochk(PMIC_WRP_CKPDN,&rdata);
  //disable clock,except kpd(bit[4] kpd and bit[6] 32k);
  ret= pwrap_write_nochk(PMIC_WRP_CKPDN,  rdata | 0x2F);//set dewrap clock bit
  return ret;

}
/*---------------------------------------------------------------------------*/
static void __exit mt_pwrap_exit(void)
{
    platform_driver_unregister(&mt_pwrap_driver);
    return;
}
/*---------------------------------------------------------------------------*/
postcore_initcall(mt_pwrap_init);
module_exit(mt_pwrap_exit);
//#define PWRAP_EARLY_PORTING
/*-----suspend/resume for pmic_wrap-------------------------------------------*/
//infra power down while suspend,pmic_wrap will gate clock after suspend.
//so,need to init PPB when resume.
//only affect PWM and I2C

static int pwrap_suspend(void)
{
  //PWRAPLOG("pwrap_suspend\n");
  return 0;
}
static void pwrap_resume(void)
{
  //PWRAPLOG("pwrap_resume\n");
  //###############################
  // Reset related modules
  // PERI_PWRAP_BRIDGE
  // Subject to project
  //###############################
  WRAP_SET_BIT(0x04,PERI_GLOBALCON_RST1);
  WRAP_CLR_BIT(0x04,PERI_GLOBALCON_RST1);

  //###############################
  // PERI_PWRAP_BRIDGE enables
  //###############################
  WRAP_WR32(PERI_PWRAP_BRIDGE_IORD_ARB_EN , 0x7f);
  WRAP_WR32(PERI_PWRAP_BRIDGE_WACS3_EN  , 0x1);
  WRAP_WR32(PERI_PWRAP_BRIDGE_WACS4_EN  , 0x1);
  WRAP_WR32(PERI_PWRAP_BRIDGE_WDT_UNIT  , 0xf);
  WRAP_WR32(PERI_PWRAP_BRIDGE_WDT_SRC_EN  , 0xffff);
  WRAP_WR32(PERI_PWRAP_BRIDGE_TIMER_EN  , 0x1);
  WRAP_WR32(PERI_PWRAP_BRIDGE_INT_EN    , 0x7ff);
}

static struct syscore_ops pwrap_syscore_ops = {
  .resume   = pwrap_resume,
  .suspend  = pwrap_suspend,
};

static int __init pwrap_init_ops(void)
{
  PWRAPLOG("pwrap_init_ops\n");
  register_syscore_ops(&pwrap_syscore_ops);
  return 0;
}
device_initcall(pwrap_init_ops);
// Begin_Arima_HCSW7_20130424_JohnnyZheng - [MTK patch] ALPS00595492
static void __init pwrap_init_clock(void)
{
  U32 rdata=0;
  PWRAPFUC();
  //###############################
  //TBD: Set SPI_CK freq = 66MHz
  //###############################
  //WRAP_WR32(CLK_CFG_8, 5);
  rdata = WRAP_RD32(CLK_CFG_8);
  WRAP_WR32(CLK_CFG_8, (rdata & ~0x7) | 0x5);
}
postcore_initcall(pwrap_init_clock);
// End_Arima_HCSW7_20130424_JohnnyZheng - [MTK patch]
//#ifdef PWRAP_EARLY_PORTING
#ifdef CONFIG_MTK_LDVT_PMIC_WRAP

//--------------------------------------------------------
//    Function : _pwrap_event_Test()
// Description :only for early porting
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 _pwrap_event_test_porting( void )
{
  U32 i, j;
  U32 rdata;
  U32 sub_return;
  U32 ret=0;
  PWRAPFUC();
  WRAP_WR32(PMIC_WRAP_EVENT_STACLR , 0xffff);

  sub_return=pwrap_write(DEW_EVENT_TEST, 0x1);
  if( sub_return != 0 )
  {  /* TERR="Error: [DrvPWRAP_EventTest][DrvPWRAP_WACS2] fail, return=%x, exp=0x0", sub_return*/
     PWRAPERR("event test error,return=%x, exp=0x0", sub_return);
     return 0x02;
  }
  //pwrap_delay_us(10);////delay at least 2us
  for(i=0;i++;i<100);
  rdata = WRAP_RD32(PMIC_WRAP_EVENT_STA);
  if( (rdata & 0x8000) != 0x8000 )
  {
    PWRAPERR("event test error,ret=%x, rdata=%x\n", ret, rdata);
    return 1;//DEBUG_CTP
   }
  WRAP_WR32(PMIC_WRAP_EVENT_STACLR , 0xffff);
  return 0;
}

//--------------------------------------------------------
//    Function : _pwrap_status_update_test()
// Description :only for early porting
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 _pwrap_status_update_test_porting( void )
{
  U32 i, j;
  U32 rdata;
  volatile U32 delay=1000*1000*1;
  PWRAPFUC();
  //disable signature interrupt
  WRAP_WR32(PMIC_WRAP_INT_EN,0x0);
  pwrap_write(DEW_WRITE_TEST, WRITE_TEST_VALUE);
  WRAP_WR32(PMIC_WRAP_SIG_ADR,DEW_WRITE_TEST);
  WRAP_WR32(PMIC_WRAP_SIG_VALUE,0xAA55);
  WRAP_WR32(PMIC_WRAP_SIG_MODE, 0x1);

  //pwrap_delay_us(5000);//delay 5 seconds

  while(delay--);

  rdata=WRAP_RD32(PMIC_WRAP_SIG_ERRVAL);
  if( rdata != WRITE_TEST_VALUE )
  {
    PWRAPERR("_pwrap_status_update_test error,error code=%x, rdata=%x\n", 1, rdata);
    //return 1;
  }
  WRAP_WR32(PMIC_WRAP_SIG_VALUE,WRITE_TEST_VALUE);//tha same as write test
  //clear sig_error interrupt flag bit
  WRAP_WR32(PMIC_WRAP_INT_CLR,1<<1);

  //enable signature interrupt
  WRAP_WR32(PMIC_WRAP_INT_EN,0x7ffffffd);
  WRAP_WR32(PMIC_WRAP_SIG_MODE, 0x0);
  WRAP_WR32(PMIC_WRAP_SIG_ADR , DEW_CRC_VAL);
  return 0;
}

static int __init pwrap_init_for_early_porting(void)
{
    int ret = 0;
    U32 res=0;
  PWRAPFUC();
  ret=pwrap_init();
    if(ret==0)
    {
      PWRAPLOG("wrap_init test pass.\n");
    }
    else
    {
      PWRAPLOG("error:wrap_init test fail.\n");
    res+=1;
    }
  ret=_pwrap_event_test_porting();
    if(ret==0)
    {
      PWRAPLOG("wrapper EventTest pass.\n");
    }
    else
    {
      PWRAPLOG("error:wrapper_EventTest fail.\n");
    res+=1;
    }
  ret=_pwrap_status_update_test_porting();
    if(ret==0)
    {
      PWRAPLOG("wrapper_StatusUpdateTest pass.\n");
    }
    else
    {
      PWRAPLOG("error:wrapper_StatusUpdateTest fail.\n");
    res+=1;
    }

}
//postcore_initcall(pwrap_init_for_early_porting);
#endif //WRAP_EARLY_PORTING,,CONFIG_MTK_LDVT_PMIC_WRAP

//########################################################################################
#ifdef CONFIG_MTK_LDVT_PMIC_WRAP
/*----FPR ldvt-----------------------------------------------------------------------*/
#define WRAP_UVVF_INIT                  0x0600
#define WRAP_UVVF_WACS_TEST             0x0601
#define WRAP_UVVF_STATUS_UPDATE         0x0602
#define WRAP_UVVF_EVENT_TEST            0x0603
#define WRAP_UVVF_DUAL_IO               0x0604
#define WRAP_UVVF_REG_RW                0x0605
#define WRAP_UVVF_MUX_SWITCH            0x0606
#define WRAP_UVVF_RESET_PATTERN         0x0607
#define WRAP_UVVF_SOFT_RESET            0x0608
#define WRAP_UVVF_HIGH_PRI              0x0609
#define WRAP_UVVF_IN_ORDER_PRI          0x0610
#define WRAP_UVVF_SPI_ENCRYPTION_TEST   0x0612
#define WRAP_UVVF_WDT_TEST              0x0613
#define WRAP_UVVF_INT_TEST              0x0614
#define WRAP_UVVF_PERI_WDT_TEST         0x0615
#define WRAP_UVVF_PERI_INT_TEST         0x0616
#define WRAP_UVVF_CONCURRENCE_TEST      0x0617
#define WRAP_UVVF_CLOCK_GATING          0x0618
#define WRAP_UVVF_THROUGHPUT            0x0619

/*****************************************************************************/
/* driver only used in LDVT                                                            */
/*****************************************************************************/
/*Interrupt handler function*/
static irqreturn_t pwrap_interrupt_for_ldvt(int irqno, void *dev_id)
{
  U32 i=0;
  U32 reg_addr=0;
  U32 reg_value=0;
  U32 reg_backup=0;
  unsigned long flags=0;
  unsigned int mask = 1 << (MT6589_PMIC_WRAP_IRQ_ID % 32);
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
  PWRAPFUC();

  spin_lock_irqsave(&pwrap_obj->spin_lock_isr,flags);
  //*-----------------------------------------------------------------------
  pwrap_interrupt_on_ldvt();
  //temp solution for INT and WDT test
  //WRAP_WR32(PMIC_WRAP_WDT_SRC_EN, 0);
  //WRAP_WR32(PERI_PWRAP_BRIDGE_WDT_SRC_EN, 0);
  spin_unlock_irqrestore(&pwrap_obj->spin_lock_isr,flags);

  return IRQ_HANDLED;
}

//--------------------------------------------------------
//    Function : pwrap_wacs3()
// Description :pwrap wacs3  inorder
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 pwrap_wacs3( U32  write, U32  adr, U32  wdata, U32 *rdata )
{
  U32 reg_rdata=0;
  U32 wacs_write=0;
  U32 wacs_adr=0;
  U32 wacs_cmd=0;
  U32 return_value=0;
  unsigned long flags=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;

  if (NULL == pwrap_obj)
      return E_PWR_INVALID_ARG;
  //PWRAPFUC();
  // Check argument validation
  if( (write & ~(0x1))    != 0)  return E_PWR_INVALID_RW;
  if( (adr   & ~(0xffff)) != 0)  return E_PWR_INVALID_ADDR;
  if( (wdata & ~(0xffff)) != 0)  return E_PWR_INVALID_WDAT;

  spin_lock_irqsave(&pwrap_obj->spin_lock_wacs3,flags);
  //#if 0
  return_value=wait_for_state_idle(wait_for_fsm_idle,TIMEOUT_WAIT_IDLE,PERI_PWRAP_BRIDGE_WACS3_RDATA,PERI_PWRAP_BRIDGE_WACS3_VLDCLR,0);
  if(return_value!=0)
  {
    PWRAPERR("wait_for_fsm_idle fail,return_value=%d\n",return_value);
    goto FAIL;
  }
  //#endif
  wacs_write  = write << 31;
  wacs_adr    = (adr >> 1) << 16;
  wacs_cmd= wacs_write | wacs_adr | wdata;

  WRAP_WR32(PERI_PWRAP_BRIDGE_WACS3_CMD,wacs_cmd);
  if( write == 0 )
  {
    if (NULL == rdata)
    {
      PWRAPERR("rdata is a NULL pointer\n");
      return_value= E_PWR_INVALID_ARG;
      goto FAIL;
    }
    return_value=wait_for_state_ready(wait_for_fsm_vldclr,TIMEOUT_READ,PERI_PWRAP_BRIDGE_WACS3_RDATA,&reg_rdata);
    if(return_value!=0)
    {
      PWRAPERR("wait_for_fsm_vldclr fail,return_value=%d\n",return_value);
      return_value+=1;//E_PWR_NOT_INIT_DONE_READ or E_PWR_WAIT_IDLE_TIMEOUT_READ
      goto FAIL;
    }
    *rdata = GET_WACS0_RDATA( reg_rdata );
    WRAP_WR32(PERI_PWRAP_BRIDGE_WACS3_VLDCLR , 1);
  }
FAIL:
  spin_unlock_irqrestore(&pwrap_obj->spin_lock_wacs3,flags);
  return return_value;

}

//--------------------------------------------------------
//    Function : pwrap_wacs4()
// Description :pwrap   wacs4 inorder
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 pwrap_wacs4( U32  write, U32  adr, U32  wdata, U32 *rdata )
{
  U32 reg_rdata=0;
  U32 wacs_write=0;
  U32 wacs_adr=0;
  U32 wacs_cmd=0;
  U32 return_value=0;
  unsigned long flags=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;

  if (NULL == pwrap_obj)
      return E_PWR_INVALID_ARG;
  //PWRAPFUC();
  // Check argument validation
  if( (write & ~(0x1))    != 0)  return E_PWR_INVALID_RW;
  if( (adr   & ~(0xffff)) != 0)  return E_PWR_INVALID_ADDR;
  if( (wdata & ~(0xffff)) != 0)  return E_PWR_INVALID_WDAT;

  spin_lock_irqsave(&pwrap_obj->spin_lock_wacs4,flags);
  // Check IDLE & INIT_DONE in advance
  return_value=wait_for_state_idle(wait_for_fsm_idle,TIMEOUT_WAIT_IDLE,PERI_PWRAP_BRIDGE_WACS4_RDATA,PERI_PWRAP_BRIDGE_WACS4_VLDCLR,0);
  if(return_value!=0)
  {
    PWRAPERR("wait_for_fsm_idle fail,return_value=%d\n",return_value);
    goto FAIL;
  }

  wacs_write  = write << 31;
  wacs_adr    = (adr >> 1) << 16;
  wacs_cmd= wacs_write | wacs_adr | wdata;
  WRAP_WR32(PERI_PWRAP_BRIDGE_WACS4_CMD,wacs_cmd);

  if( write == 0 )
  {
    if (NULL == rdata)
    {
      PWRAPERR("rdata is a NULL pointer\n");
      return_value= E_PWR_INVALID_ARG;
      goto FAIL;
    }
    return_value=wait_for_state_ready(wait_for_fsm_vldclr,TIMEOUT_READ,PERI_PWRAP_BRIDGE_WACS4_RDATA,&reg_rdata);
    if(return_value!=0)
    {
      PWRAPERR("wait_for_fsm_vldclr fail,return_value=%d\n",return_value);
      return_value+=1;//E_PWR_NOT_INIT_DONE_READ or E_PWR_WAIT_IDLE_TIMEOUT_READ
      goto FAIL;
    }
    *rdata = GET_WACS0_RDATA( reg_rdata );
    WRAP_WR32(PERI_PWRAP_BRIDGE_WACS4_VLDCLR , 1);
  }
  FAIL:
  spin_unlock_irqrestore(&pwrap_obj->spin_lock_wacs4,flags);
  return return_value;

}

//--------------------------------------------------------
//    Function : pwrap_wacs0()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 pwrap_wacs0( U32 write, U32 adr, U32 wdata, U32 *rdata )
{
  U32 reg_rdata=0;
  U32 wacs_write=0;
  U32 wacs_adr=0;
  U32 wacs_cmd=0;
  U32 return_value=0;
  unsigned long flags=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
  if (!pwrap_obj)
        PWRAPERR("NULL pointer\n");
  //PWRAPFUC();
  //PWRAPLOG("wrapper access,write=%x,add=%x,wdata=%x,rdata=%x\n",write,adr,wdata,rdata);

  // Check argument validation
  if( (write & ~(0x1))    != 0)  return E_PWR_INVALID_RW;
  if( (adr   & ~(0xffff)) != 0)  return E_PWR_INVALID_ADDR;
  if( (wdata & ~(0xffff)) != 0)  return E_PWR_INVALID_WDAT;

  spin_lock_irqsave(&pwrap_obj->spin_lock_wacs0,flags);
  // Check IDLE & INIT_DONE in advance
  return_value=wait_for_state_idle(wait_for_fsm_idle,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS0_RDATA,PMIC_WRAP_WACS0_VLDCLR,0);
  if(return_value!=0)
  {
    PWRAPERR("wait_for_fsm_idle fail,return_value=%d\n",return_value);
    goto FAIL;
  }

  wacs_write  = write << 31;
  wacs_adr    = (adr >> 1) << 16;
  wacs_cmd= wacs_write | wacs_adr | wdata;

  WRAP_WR32(PMIC_WRAP_WACS0_CMD,wacs_cmd);
  if( write == 0 )
  {
    if (NULL == rdata)
    {
      PWRAPERR("rdata is a NULL pointer\n");
      return_value= E_PWR_INVALID_ARG;
      goto FAIL;
    }
    return_value=wait_for_state_ready(wait_for_fsm_vldclr,TIMEOUT_READ,PMIC_WRAP_WACS0_RDATA,&reg_rdata);
    if(return_value!=0)
    {
      PWRAPERR("wait_for_fsm_vldclr fail,return_value=%d\n",return_value);
      return_value+=1;//E_PWR_NOT_INIT_DONE_READ or E_PWR_WAIT_IDLE_TIMEOUT_READ
      goto FAIL;
    }
    *rdata = GET_WACS0_RDATA( reg_rdata );
    WRAP_WR32(PMIC_WRAP_WACS0_VLDCLR , 1);
  }
  FAIL:
  spin_unlock_irqrestore(&pwrap_obj->spin_lock_wacs0,flags);
  return return_value;
}

//--------------------------------------------------------
//    Function : pwrap_wacs1()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 pwrap_wacs1( U32  write, U32  adr, U32  wdata, U32 *rdata )
{
  U32 reg_rdata=0;
  U32 wacs_write=0;
  U32 wacs_adr=0;
  U32 wacs_cmd=0;
  U32 return_value=0;
  unsigned long flags=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;

  if (NULL == pwrap_obj)
      return E_PWR_INVALID_ARG;
  //PWRAPFUC();

  // Check argument validation
  if( (write & ~(0x1))    != 0)  return E_PWR_INVALID_RW;
  if( (adr   & ~(0xffff)) != 0)  return E_PWR_INVALID_ADDR;
  if( (wdata & ~(0xffff)) != 0)  return E_PWR_INVALID_WDAT;

  spin_lock_irqsave(&pwrap_obj->spin_lock_wacs1,flags);
  // Check IDLE & INIT_DONE in advance
  return_value=wait_for_state_idle(wait_for_fsm_idle,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS1_RDATA,PMIC_WRAP_WACS1_VLDCLR,0);
  if(return_value!=0)
  {
    PWRAPERR("wait_for_fsm_idle fail,return_value=%d\n",return_value);
    goto FAIL;
  }
  // Argument process
  wacs_write  = write << 31;
  wacs_adr    = (adr >> 1) << 16;
  wacs_cmd= wacs_write | wacs_adr | wdata;
  //send command
  WRAP_WR32(PMIC_WRAP_WACS1_CMD,wacs_cmd);
  if( write == 0 )
  {
    if (NULL == rdata)
    {
      PWRAPERR("rdata is a NULL pointer\n");
      return_value= E_PWR_INVALID_ARG;
      goto FAIL;
    }
    return_value=wait_for_state_ready(wait_for_fsm_vldclr,TIMEOUT_READ,PMIC_WRAP_WACS1_RDATA,&reg_rdata);
    if(return_value!=0)
    {
      PWRAPERR("wait_for_fsm_vldclr fail,return_value=%d\n",return_value);
      return_value+=1;//E_PWR_NOT_INIT_DONE_READ or E_PWR_WAIT_IDLE_TIMEOUT_READ
      goto FAIL;
    }
    *rdata = GET_WACS0_RDATA( reg_rdata );
    WRAP_WR32(PMIC_WRAP_WACS1_VLDCLR , 1);
  }
  FAIL:
  spin_unlock_irqrestore(&pwrap_obj->spin_lock_wacs1,flags);
  return return_value;

}

//    Function : _pwrap_switch_dio()
// Description :call it after pwrap_init, check init done
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 _pwrap_switch_dio( U32 dio_en )
{

  U32 arb_en_backup=0;
  U32 rdata=0;
  U32 sub_return=0;
  U32 return_value=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
  arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , 0x8); // only WACS0
  sub_return= pwrap_write(DEW_DIO_EN, dio_en);
  if( sub_return != 0 )
  {
     PWRAPERR("[_pwrap_switch_dio] enable DEW_DIO fail,return=%x", sub_return);
     return E_PWR_SWITCH_DIO;
  }
  // Wait WACS0_FSM==IDLE
  return_value=wait_for_state_ready(wait_for_idle_and_sync,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS2_RDATA,0);
  if(return_value!=0)
  return return_value;


  WRAP_WR32(PMIC_WRAP_DIO_EN , dio_en);
  // Read Test
  pwrap_read(DEW_READ_TEST, &rdata);
  if( rdata != DEFAULT_VALUE_READ_TEST )
  {
    PWRAPERR("[_pwrap_switch_dio][Read Test] fail,dio_en = %x, READ_TEST rdata=%x, exp=0x5aa5\n", dio_en, rdata);
    return E_PWR_READ_TEST_FAIL;
  }

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN ,arb_en_backup);
  return 0;
}
//--------------------------------------------------------
//    Function : DrvPWRAP_SwitchMux()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
void _pwrap_switch_mux( U32 mux_sel_new )
{
  U32 mux_sel_old=0;
  U32 rdata=0;
  U32 return_value=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
  // return if no change is necessary
  mux_sel_old = WRAP_RD32(PMIC_WRAP_MUX_SEL);
  if( mux_sel_new == mux_sel_old )
    return;

  // disable OLD, wait OLD finish
  // switch MUX, then enable NEW
  if( mux_sel_new == 1 )
  {
    WRAP_WR32(PMIC_WRAP_WRAP_EN ,0);
  // Wait for WRAP to be in idle state, // and no remaining rdata to be received
  return_value=wait_for_state_ready_init(wait_for_wrap_idle,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WRAP_STA,0);
  if(return_value!=0)
    return return_value;
    WRAP_WR32(PMIC_WRAP_MUX_SEL , 1);
    WRAP_WR32(PMIC_WRAP_MAN_EN , 1);
  }
  else
  {
    WRAP_WR32(PMIC_WRAP_MAN_EN , 0);
  // Wait for WRAP to be in idle state, // and no remaining rdata to be received
  return_value=wait_for_state_ready_init(wait_for_man_idle_and_noreq,TIMEOUT_WAIT_IDLE,PMIC_WRAP_MAN_RDATA,0);
  if(return_value!=0)
    return return_value;

    WRAP_WR32(PMIC_WRAP_MUX_SEL , 0);
    WRAP_WR32(PMIC_WRAP_WRAP_EN , 1);
  }

  return 0;
}



//--------------------------------------------------------
//    Function : _pwrap_enable_cipher()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 _pwrap_enable_cipher( void )
{
  U32 arb_en_backup=0;
  U32 rdata=0;
  U32 cipher_ready=0;
  U32 return_value=0;
  U32 start_time_ns=0, timeout_ns=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
  PWRAPFUC();
  arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN ,0x8); // only WACS0


  //Make sure CIPHER engine is idle
  pwrap_write(DEW_CIPHER_START,0x0);
  WRAP_WR32(PMIC_WRAP_CIPHER_START   ,0);

  WRAP_WR32(PMIC_WRAP_CIPHER_MODE    , 0);
  WRAP_WR32(PMIC_WRAP_CIPHER_SWRST   , 1);
  WRAP_WR32(PMIC_WRAP_CIPHER_SWRST   , 0);
  WRAP_WR32(PMIC_WRAP_CIPHER_KEY_SEL , 1);
  WRAP_WR32(PMIC_WRAP_CIPHER_IV_SEL  , 2);
  WRAP_WR32(PMIC_WRAP_CIPHER_LOAD    , 1);
  WRAP_WR32(PMIC_WRAP_CIPHER_START   , 1);

  //Config CIPHER @ PMIC
  pwrap_write(DEW_CIPHER_SWRST,   0x1);
  pwrap_write(DEW_CIPHER_SWRST,   0x0);
  pwrap_write(DEW_CIPHER_KEY_SEL, 0x1);
  pwrap_write(DEW_CIPHER_IV_SEL,  0x2);
  pwrap_write(DEW_CIPHER_LOAD,    0x1);
  pwrap_write(DEW_CIPHER_START,   0x1);
  //wait for cipher ready
  return_value=wait_for_state_ready_init(wait_for_cipher_ready,TIMEOUT_WAIT_IDLE,PMIC_WRAP_CIPHER_RDY,0);
  if(return_value!=0)
  return return_value;

  start_time_ns = _pwrap_get_current_time();
  do
  {
    pwrap_read(DEW_CIPHER_RDY, &rdata);
    if (_pwrap_timeout_ns(start_time_ns, timeout_ns))
    {
      PWRAPERR("timeout when waiting for idle\n");
      return E_PWR_WAIT_IDLE_TIMEOUT;
    }
  } while( rdata != 0x1 ); //cipher_ready

  pwrap_write(DEW_CIPHER_MODE, 0x1);
  //wait for wacs2 ready
  return_value=wait_for_state_ready_init(wait_for_idle_and_sync,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS2_RDATA,0);
  if(return_value!=0)
  return return_value;

  WRAP_WR32(PMIC_WRAP_CIPHER_MODE , 1);

  // Read Test
  pwrap_read(DEW_READ_TEST,&rdata);
  if( rdata != DEFAULT_VALUE_READ_TEST )
  {
    PWRAPERR("Enable Encryption [Read Test] fail, READ_TEST rdata=%x, exp=0x5aa5", rdata);
    return E_PWR_READ_TEST_FAIL;
  }

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , arb_en_backup);
  return 0;
}



//--------------------------------------------------------
//    Function : _pwrap_disable_cipher()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 _pwrap_disable_cipher( void )
{
  U32 arb_en_backup=0;
  U32 rdata=0;
  U32 return_value=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
  PWRAPFUC();
  arb_en_backup = WRAP_RD32(PMIC_WRAP_HIPRIO_ARB_EN);

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , 0x8); // only WACS0

  //[7:6]key_sel, [5:4]iv_sel, [3]swrst [2]load, [1]start, [0]mode
  pwrap_write(DEW_CIPHER_MODE, 0x0);

  //wait for wacs2 ready
  return_value=wait_for_state_ready_init(wait_for_idle_and_sync,TIMEOUT_WAIT_IDLE,PMIC_WRAP_WACS2_RDATA,0);
  if(return_value!=0)
  return return_value;

  WRAP_WR32(PMIC_WRAP_CIPHER_MODE , 0);
  WRAP_WR32(PMIC_WRAP_CIPHER_START , 0);
  WRAP_WR32(PMIC_WRAP_CIPHER_SWRST , 1);
  WRAP_WR32(PMIC_WRAP_CIPHER_SWRST , 0);

  // Read Test
  pwrap_read(DEW_READ_TEST,&rdata);
  if( rdata != DEFAULT_VALUE_READ_TEST )
  {
    PWRAPERR("disable Encryption [Read Test] fail, READ_TEST rdata=%x, exp=0x5aa5", rdata);
    return E_PWR_READ_TEST_FAIL;
  }

  WRAP_WR32(PMIC_WRAP_HIPRIO_ARB_EN , arb_en_backup);
  return 0;
}

//--------------------------------------------------------
//    Function : _pwrap_manual_mode()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 _pwrap_manual_mode( U32  write, U32  op, U32  wdata, U32 *rdata )
{
  U32 reg_rdata=0;
  U32 man_write=0;
  U32 man_op=0;
  U32 man_cmd=0;
  U32 return_value=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
  reg_rdata = WRAP_RD32(PMIC_WRAP_MAN_RDATA);
  if( GET_MAN_FSM( reg_rdata ) != 0) //IDLE State
    return E_PWR_NOT_IDLE_STATE;

  // check argument validation
  if( (write & ~(0x1))  != 0)  return E_PWR_INVALID_RW;
  if( (op    & ~(0x1f)) != 0)  return E_PWR_INVALID_OP_MANUAL;
  if( (wdata & ~(0xff)) != 0)  return E_PWR_INVALID_WDAT;

  man_write = write << 13;
  man_op    = op << 8;
  man_cmd = man_write | man_op | wdata;
  WRAP_WR32(PMIC_WRAP_MAN_CMD ,man_cmd);
  if( write == 0 )
  {
    //wait for wacs2 ready
    return_value=wait_for_state_ready_init(wait_for_man_vldclr,TIMEOUT_WAIT_IDLE,PMIC_WRAP_MAN_RDATA,&reg_rdata);
    if(return_value!=0)
    return return_value;

    *rdata = GET_MAN_RDATA( reg_rdata );
    WRAP_WR32(PMIC_WRAP_MAN_VLDCLR , 1);
  }
  return 0;
}

//--------------------------------------------------------
//    Function : _pwrap_manual_modeAccess()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
S32 _pwrap_manual_modeAccess( U32  write, U32  adr, U32  wdata, U32 *rdata )
{
  U32  man_wdata=0;
  U32 man_rdata=0;

  // check argument validation
  if( (write & ~(0x1))    != 0)  return E_PWR_INVALID_RW;
  if( (adr   & ~(0xffff)) != 0)  return E_PWR_INVALID_ADDR;
  if( (wdata & ~(0xffff)) != 0)  return E_PWR_INVALID_WDAT;

  _pwrap_switch_mux(1);
  _pwrap_manual_mode(OP_WR,  OP_CSH,  0, &man_rdata);
  _pwrap_manual_mode(OP_WR,  OP_CSL,  0, &man_rdata);
  man_wdata = adr >> 1;
  _pwrap_manual_mode(OP_WR,  OP_OUTD, (man_wdata & 0xff), &man_rdata);
  man_wdata = (adr >> 9) | (write << 7);
  _pwrap_manual_mode(OP_WR,  OP_OUTD, (man_wdata & 0xff), &man_rdata);
  _pwrap_manual_mode(OP_WR,  OP_CSH,  0, &man_rdata);

  _pwrap_manual_mode(OP_WR,  OP_CSL,  0, &man_rdata);
  if( write == 1 )
  {
    man_wdata = wdata;
    _pwrap_manual_mode(1,  OP_OUTD, (man_wdata & 0xff), &man_rdata);
    man_wdata = wdata >> 8;
    _pwrap_manual_mode(1,  OP_OUTD, (man_wdata & 0xff), &man_rdata);
  }
  else
  {
    _pwrap_manual_mode(0,  OP_IND, 0, &man_rdata);
    *rdata = GET_MAN_RDATA( man_rdata );
    _pwrap_manual_mode(0,  OP_IND, 0, &man_rdata);
    *rdata |= (GET_MAN_RDATA( man_rdata )<<8);
  }
  _pwrap_manual_mode(OP_WR,  OP_CSH,  0, &man_rdata);
  return 0;
}
//--------------------------------------------------------
//    Function : _pwrap_StaUpdTrig()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------
static S32 _pwrap_StaUpdTrig( S32 mode )
{
  U32 man_rdata=0;
  U32 reg_data=0;
  U32 return_value=0;
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;

  //Wait for FSM to be IDLE
  return_value=wait_for_state_ready_init(wait_for_stdupd_idle,TIMEOUT_WAIT_IDLE,PMIC_WRAP_STAUPD_STA,0);
  if(return_value!=0)
  return return_value;

  //Trigger FSM
  WRAP_WR32(PMIC_WRAP_STAUPD_MAN_TRIG ,0x1);
  reg_data=WRAP_RD32(PMIC_WRAP_STAUPD_STA);
  //Check if FSM is in REQ
  if( GET_STAUPD_FSM(reg_data) != 0x2)
    return E_PWR_NOT_IDLE_STATE;

  // if mode==1, only return after new status is updated.
  if( mode == 1)
  {
    while( GET_STAUPD_FSM(reg_data) != 0x0); //IDLE State
  }

  return 0;
}


//------------------
//--------------------------------------------------------
//    Function : mt_pwrap_ioctl()
// Description :
//   Parameter :
//      Return :
//--------------------------------------------------------

static int mt_pwrap_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
  struct pmic_wrap_obj *pwrap_obj = g_pmic_wrap_obj;
  S32 res=1;
  U32 pin=0;

  PWRAPFUC();
  if (!pwrap_obj)
  {
    PWRAPERR("NULL pointer\n");
    return -EFAULT;
  }

  switch ( arg )
  {
    case WRAP_UVVF_INIT:
      PWRAPLOG("WRAP_UVVF_INIT test start.\n");
      res=tc_wrap_init_test();
      break;
    case WRAP_UVVF_WACS_TEST:
      PWRAPLOG("WRAP_UVVF_WACS_TEST test.\n");
      tc_wrap_access_test();
      break;
    case WRAP_UVVF_STATUS_UPDATE:
      PWRAPLOG("WRAP_UVVF_STATUS_UPDATE test.\n");
      tc_status_update_test();
      break;
    case WRAP_UVVF_EVENT_TEST:
      PWRAPLOG("WRAP_UVVF_EVENT_TEST test.\n");
      tc_event_test();
      break;
    case WRAP_UVVF_DUAL_IO:
      PWRAPLOG("WRAP_UVVF_DUAL_IO test start.\n");
      tc_dual_io_test();
      break;
    case WRAP_UVVF_REG_RW:
      PWRAPLOG("WRAP_UVVF_REG_RW test.\n");
      tc_reg_rw_test();
      break;
    case WRAP_UVVF_MUX_SWITCH:
      PWRAPLOG("WRAP_UVVF_MUX_SWITCH test.\n");
      tc_mux_switch_test();
      break;
    case WRAP_UVVF_RESET_PATTERN:
      PWRAPLOG("WRAP_UVVF_RESET_PATTERN test.\n");
      tc_reset_pattern_test();
      break;
    case WRAP_UVVF_SOFT_RESET:
      PWRAPLOG("WRAP_UVVF_SOFT_RESET test.\n");
      tc_soft_reset_test();
      break;
    case WRAP_UVVF_HIGH_PRI:
      PWRAPLOG("WRAP_UVVF_HIGH_PRI test.\n");
      tc_high_pri_test();
      break;
    case WRAP_UVVF_IN_ORDER_PRI:
      PWRAPLOG("WRAP_UVVF_IN_ORDER_PRI test.\n");
      tc_in_order_pri_test();
      break;
    case WRAP_UVVF_SPI_ENCRYPTION_TEST:
      PWRAPLOG("WRAP_UVVF_SPI_ENCRYPTION_TEST test.\n");
      tc_spi_encryption_test();
      break;
    case WRAP_UVVF_WDT_TEST:
      PWRAPLOG("WRAP_UVVF_WDT_TEST test.\n");
      tc_wdt_test();
      break;
    case WRAP_UVVF_PERI_WDT_TEST:
      PWRAPLOG("WRAP_UVVF_WDT_TEST test.\n");
      tc_peri_wdt_test();
      break;
    case WRAP_UVVF_INT_TEST:
      PWRAPLOG("WRAP_UVVF_INT_TEST test.\n");
      tc_int_test();
      break;
    case WRAP_UVVF_PERI_INT_TEST:
      PWRAPLOG("WRAP_UVVF_INT_TEST test.\n");
      tc_peri_int_test();
      break;
    case WRAP_UVVF_CONCURRENCE_TEST:
      PWRAPLOG("WRAP_UVVF_CONCURRENCE_TEST test.\n");
      tc_concurrence_test();
      break;
    case WRAP_UVVF_CLOCK_GATING:
      PWRAPLOG("WRAP_UVVF_CONCURRENCE_TEST test.\n");
      tc_clock_gating_test();
      break;
    case WRAP_UVVF_THROUGHPUT:
      PWRAPLOG("tc_throughput_test test.\n");
      tc_throughput_test();
      break;
    default:
      //while(1)
      {
        PWRAPLOG ( "default test.\n" );
      }
      break;
  }
  return 0;
}
#endif //end of CONFIG_MTK_LDVT_PMIC_WRAP

MODULE_AUTHOR("sixian wang <sixian.wang@mediatek.com>");
MODULE_DESCRIPTION("MT6589 pmic_wrapper Driver  $Revision$");
MODULE_LICENSE("GPL");
