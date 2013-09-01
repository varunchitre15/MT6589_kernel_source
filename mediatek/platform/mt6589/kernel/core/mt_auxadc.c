/*****************************************************************************
 *
 * Filename:
 * ---------
 *    mt6589_auxadc.c
 *
 * Project:
 * --------
 *   Android_Software
 *
 * Description:
 * ------------
 *   This Module defines functions of mt6589 AUXADC
 *
 * Author:
 * -------
 * Zhong Wang
 *
 ****************************************************************************/
 
#include <linux/init.h>        /* For init/exit macros */
#include <linux/module.h>      /* For MODULE_ marcros  */
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <mach/mt_auxadc_sw.h>
#include <mach/mt_auxadc_hw.h>

#include <mach/hardware.h>
#include <mach/mt_gpt.h>
#include <mach/mt_clkmgr.h>
#include <mach/sync_write.h>
#include <cust_adc.h> // generate by DCT Tool

//#define AUXADC_BATTERY_VOLTAGE_CHANNEL     0
//#define AUXADC_REF_CURRENT_CHANNEL     1
//#define AUXADC_CHARGER_VOLTAGE_CHANNEL     2
//#define AUXADC_TEMPERATURE_CHANNEL     3


//#include <cust_battery.h>

//#include "pmu6577_sw.h"
//#include "upmu_sw.h"


// Define
//typedef unsigned short  kal_uint16;

//#define DRV_Reg(addr)               (*(volatile kal_uint16 *)(addr))
//#define DRV_WriteReg(addr,data)     ((*(volatile kal_uint16 *)(addr)) = (kal_uint16)(data))

#define MT65XX_PDN_PERI_AUXADC MT_CG_PERI1_AUXADC

#define DRV_ClearBits(addr,data)     {\
   kal_uint16 temp;\
   temp = DRV_Reg(addr);\
   temp &=~(data);\
   mt65xx_reg_sync_writew(temp, addr);\
}

#define DRV_SetBits(addr,data)     {\
   kal_uint16 temp;\
   temp = DRV_Reg(addr);\
   temp |= (data);\
   mt65xx_reg_sync_writew(temp, addr);\
}

#define DRV_SetData(addr, bitmask, value)     {\
   kal_uint16 temp;\
   temp = (~(bitmask)) & DRV_Reg(addr);\
   temp |= (value);\
   mt65xx_reg_sync_writew(temp, addr);\
}

#define AUXADC_DRV_ClearBits16(addr, data)           DRV_ClearBits(addr,data)
#define AUXADC_DRV_SetBits16(addr, data)             DRV_SetBits(addr,data)
#define AUXADC_DRV_WriteReg16(addr, data)            mt65xx_reg_sync_writew(data, addr)
#define AUXADC_DRV_ReadReg16(addr)                   DRV_Reg(addr)
#define AUXADC_DRV_SetData16(addr, bitmask, value)   DRV_SetData(addr, bitmask, value)

#define AUXADC_DVT_DELAYMACRO(u4Num)                                     \
{                                                                        \
    unsigned int u4Count = 0 ;                                           \
    for (u4Count = 0; u4Count < u4Num; u4Count++ );                      \
}

#define AUXADC_CLR_BITS(BS,REG)     {\
   kal_uint32 temp;\
   temp = DRV_Reg32(REG);\
   temp &=~(BS);\
   mt65xx_reg_sync_writel(temp, REG);\
}

#define AUXADC_SET_BITS(BS,REG)     {\
   kal_uint32 temp;\
   temp = DRV_Reg32(REG);\
   temp |= (BS);\
   mt65xx_reg_sync_writel(temp, REG);\
}

//#define AUXADC_SET_BITS(BS,REG)       ((*(volatile u32*)(REG)) |= (u32)(BS))
//#define AUXADC_CLR_BITS(BS,REG)       ((*(volatile u32*)(REG)) &= ~((u32)(BS)))

#define VOLTAGE_FULL_RANGE  1500 // VA voltage
#define AUXADC_PRECISE      4096 // 12 bits

/*****************************************************************************
 * Integrate with NVRAM 
****************************************************************************/
#define AUXADC_CALI_DEVNAME    "mtk-adc-cali"
#define AUXADC_CALI_MAJOR      0

#define TEST_ADC_CALI_PRINT _IO('k', 0)
#define SET_ADC_CALI_Slop   _IOW('k', 1, int)
#define SET_ADC_CALI_Offset _IOW('k', 2, int)
#define SET_ADC_CALI_Cal    _IOW('k', 3, int)
#define ADC_CHANNEL_READ    _IOW('k', 4, int)

#define R_BAT_SENSE 2		// times of voltage
#define R_I_SENSE 2			// times of voltage
#define R_CHARGER_1 330
#define R_CHARGER_2 39

static DEFINE_MUTEX(auxadc_mutex);
static DEFINE_MUTEX(mutex_get_cali_value);

int auxadc_cali_slop[16]   = {1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000};
int auxadc_cali_offset[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int auxadc_cali_cal[1]     = {0};

int auxadc_in_data[2]  = {1,1};
int auxadc_out_data[2] = {1,1};

static int adc_auto_set =0;
kal_bool g_AUXADC_Cali = KAL_FALSE;

static dev_t auxadc_cali_devno;
static int auxadc_cali_major = 0;
static struct cdev *auxadc_cali_cdev;
static struct class *auxadc_cali_class = NULL;


//use efuse cali
static kal_int32 g_adc_ge = 0;
static kal_int32 g_adc_oe = 0;
static kal_int32 g_o_vts = 0;
static kal_int32 g_o_vbg = 0;
static kal_int32 g_degc_cali = 0;
static kal_int32 g_adc_cali_en = 0;
static kal_int32 g_o_vts_abb = 0;
static kal_int32 g_o_slope = 0;
static kal_int32 g_o_slope_sign = 0;
static kal_int32 g_id = 0;
static kal_int32 g_y_vbg = 0;//defaul 1967 fi cali_en=0




#if 0
extern void upmu_adc_measure_vbat_enable(upmu_chr_list_enum chr, kal_bool enable);
extern void upmu_adc_measure_vsen_enable(upmu_chr_list_enum chr, kal_bool enable);
extern void upmu_adc_measure_vchr_enable(upmu_chr_list_enum chr, kal_bool enable);
#endif

int auxadc_debug = 0;
module_param(auxadc_debug, int, 00664);

///////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
//// Internal API
inline static void mt6577_ADC_2G_power_up(void)
{
    //2010/07/27: mt6577, the ADC 2G power on is controlled by APMCU_CG_CLR0
    #define PDN_CLR0 (0xF7026308)  
    unsigned int poweron = 1 << 4;
    AUXADC_SET_BITS(poweron, PDN_CLR0);
}

inline static void mt6577_ADC_2G_power_down(void)
{
    //2010/07/27: mt6577, the ADC 2G power on is controlled by APMCU_CG_SET0
    #define PDN_SET0 (0xF7026304)  
    unsigned int poweroff = 1 << 4;
    AUXADC_SET_BITS(poweroff, PDN_SET0);
}

inline static void mt6577_ADC_3G_power_up(void)
{
    //2010/07/27: mt6577, the ADC 3G power on is controlled by APMCU_CG_CLR0
    #define PDN_CLR0 (0xF7026308)  
    unsigned int poweron = 1 << 13;
    AUXADC_SET_BITS(poweron, PDN_CLR0);
}

inline static void mt6577_ADC_3G_power_down(void)
{
    //2010/07/27: mt6577, the ADC 3G power on is controlled by APMCU_CG_SET0
    #define PDN_SET0 (0xF7026304)  
    unsigned int poweroff = 1 << 13;
    AUXADC_SET_BITS(poweroff, PDN_SET0);
}
///////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////
//// Common API
static int g_adc_init_flag =0;

#define ADC_CHANNEL_MAX 16
//#define  CUST_ADC_MD_CHANNEL 1
typedef struct adc_info
{
   char channel_name[64];
   int channel_number;
   int reserve1;
   int reserve2;
   int reserve3;
}ADC_INFO;

ADC_INFO g_adc_info[ADC_CHANNEL_MAX];



int IMM_get_adc_channel_num(char *channel_name, int len)
{
  unsigned int i;

  printk("[ADC] name = %s\n", channel_name);
  printk("[ADC] name_len = %d\n", len);
  for(i=0; i<ADC_CHANNEL_MAX; i++)
  {
    if (!strncmp(channel_name, g_adc_info[i].channel_name, len))
    {
      return g_adc_info[i].channel_number;
    }
  }
  printk("[ADC] find channel number failed\n");
  return -1;
}


int adc_channel_info_init(void)
{
   // unsigned int ap_domain = 0;
	unsigned int used_channel_counter = 0;
	used_channel_counter = 0;
	#ifdef AUXADC_TEMPERATURE_CHANNEL
    //ap_domain &= ~(1<<CUST_ADC_MD_CHANNEL);
    sprintf(g_adc_info[used_channel_counter].channel_name, "ADC_RFTMP");
    g_adc_info[used_channel_counter].channel_number = AUXADC_TEMPERATURE_CHANNEL;
	printk("[ADC] channel_name = %s channel num=%d\n", g_adc_info[used_channel_counter].channel_name
		,g_adc_info[used_channel_counter].channel_number);
    used_channel_counter++;
	#endif

	#ifdef AUXADC_ADC_FDD_RF_PARAMS_DYNAMIC_CUSTOM_CH_CHANNEL
	sprintf(g_adc_info[used_channel_counter].channel_name, "ADC_FDD_Rf_Params_Dynamic_Custom");
    g_adc_info[used_channel_counter].channel_number = AUXADC_ADC_FDD_RF_PARAMS_DYNAMIC_CUSTOM_CH_CHANNEL;
	printk("[ADC] channel_name = %s channel num=%d\n", g_adc_info[used_channel_counter].channel_name
		,g_adc_info[used_channel_counter].channel_number);
    used_channel_counter++;
	#endif

	#ifdef AUXADC_HF_MIC_CHANNEL
	sprintf(g_adc_info[used_channel_counter].channel_name, "ADC_MIC");
    g_adc_info[used_channel_counter].channel_number = AUXADC_HF_MIC_CHANNEL;
	printk("[ADC] channel_name = %s channel num=%d\n", g_adc_info[used_channel_counter].channel_name
		,g_adc_info[used_channel_counter].channel_number);
    used_channel_counter++;
	#endif
	
	return 0;

}

int IMM_IsAdcInitReady(void)
{
  return g_adc_init_flag;
}



//step1 check con2 if auxadc is busy
//step2 clear bit
//step3  read channle and make sure old ready bit ==0
//step4 set bit  to trigger sample
//step5  read channle and make sure  ready bit ==1
//step6 read data

int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata)
{
   unsigned int channel[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
   int idle_count =0;
   int data_ready_count=0;
   
   mutex_lock(&mutex_get_cali_value);
#if 0   
    if(enable_clock(MT65XX_PDN_PERI_AUXADC,"AUXADC"))
   {
	    //printk("hwEnableClock AUXADC !!!.");
	    if(enable_clock(MT65XX_PDN_PERI_AUXADC,"AUXADC"))
	    {printk("hwEnableClock AUXADC failed.");}
        
   }
#endif	
   //step1 check con2 if auxadc is busy
   while ((*(volatile u16 *)AUXADC_CON2) & 0x01) 
   {
       printk("[adc_api]: wait for module idle\n");
       msleep(100);
	   idle_count++;
	   if(idle_count>30)
	   {
	      //wait for idle time out
	      printk("[adc_api]: wait for aux/adc idle time out\n");
		mutex_unlock(&mutex_get_cali_value);
	      return -1;
	   }
   } 
   // step2 clear bit
   if(0 == adc_auto_set)
   {
	   //clear bit
	   AUXADC_DRV_ClearBits16(AUXADC_CON1, (1 << dwChannel));
	   //*(volatile u16 *)AUXADC_CON1 = *(volatile u16 *)AUXADC_CON1 & (~(1 << dwChannel));
   }
   

   //step3  read channle and make sure old ready bit ==0
   while ((*(volatile u16 *)(AUXADC_DAT0 + dwChannel * 0x04)) & (1<<12)) 
   {
       printk("[adc_api]: wait for channel[%d] ready bit clear\n",dwChannel);
       msleep(10);
	   data_ready_count++;
	   if(data_ready_count>30)
	   {
	      //wait for idle time out
	      printk("[adc_api]: wait for channel[%d] ready bit clear time out\n",dwChannel);
		mutex_unlock(&mutex_get_cali_value);
	      return -2;
	   }
   }
  
   //step4 set bit  to trigger sample
   if(0==adc_auto_set)
   {  
   	  AUXADC_DRV_SetBits16(AUXADC_CON1, (1 << dwChannel));
	  //*(volatile u16 *)AUXADC_CON1 = *(volatile u16 *)AUXADC_CON1 | (1 << dwChannel);
   }
   //step5  read channle and make sure  ready bit ==1
   mdelay(1);//we must dealay here for hw sample cahnnel data
   while (0==((*(volatile u16 *)(AUXADC_DAT0 + dwChannel * 0x04)) & (1<<12))) 
   {
       printk("[adc_api]: wait for channel[%d] ready bit ==1\n",dwChannel);
       msleep(10);
	 data_ready_count++;

	 if(data_ready_count>30)
	 {
	      //wait for idle time out
	      printk("[adc_api]: wait for channel[%d] data ready time out\n",dwChannel);
		mutex_unlock(&mutex_get_cali_value);
	      return -3;
	 }
   }
   ////step6 read data
   
   channel[dwChannel] = (*(volatile u16 *)(AUXADC_DAT0 + dwChannel * 0x04)) & 0x0FFF;
   if(NULL != rawdata)
   {
      *rawdata = channel[dwChannel];
   }
   //printk("[adc_api: imm mode raw data => channel[%d] = %d\n",dwChannel, channel[dwChannel]);
   //printk("[adc_api]: imm mode => channel[%d] = %d.%d\n", dwChannel, (channel[dwChannel] * 150 / 4096 / 100), ((channel[dwChannel] * 150 / 4096) % 100));
   data[0] = (channel[dwChannel] * 150 / 4096 / 100);
   data[1] = ((channel[dwChannel] * 150 / 4096) % 100);
   
#if 0
   if(disable_clock(MT65XX_PDN_PERI_AUXADC,"AUXADC"))
   {
        printk("hwEnableClock AUXADC failed.");
   }
#endif   
    mutex_unlock(&mutex_get_cali_value);
   
   return 0;
   
}

// 1v == 1000000 uv
// this function voltage Unit is uv
int IMM_GetOneChannelValue_Cali(int Channel, int*voltage)
{
     int ret = 0, data[4], rawvalue;
     long a =0;
     // long b =0;
     long slop = 0;
     long offset =0;
     
     ret = IMM_GetOneChannelValue( Channel,  data, &rawvalue);
     if(ret)
     {
         ret = IMM_GetOneChannelValue( Channel,  data, &rawvalue);
	   if(ret)
	   {
	        printk("[adc_api]:IMM_GetOneChannelValue_Cali  get raw value error %d \n",ret);
		  return -1;
	   }
     }

     a = (1000000+g_adc_ge)*(g_y_vbg-g_adc_oe)/((g_o_vbg+1800)-g_adc_oe);
     // b = g_adc_oe;
     slop = ((1500000/4096)*1000000)/a;
     offset = 1500000*g_adc_oe/a; 

     *voltage = rawvalue*slop + offset;
	     
      //printk("[adc_api]:IMM_GetOneChannelValue_Cali  voltage= %d uv \n",*voltage);

      return 0;
     
}


static int IMM_auxadc_get_evrage_data(int times, int Channel)
{
	int ret = 0, data[4], i, ret_value = 0, ret_temp = 0;

	i = times;
	while (i--)
	{
		ret_value = IMM_GetOneChannelValue(Channel, data, &ret_temp);
		ret += ret_temp;
		printk("[auxadc_get_data(channel%d)]: ret_temp=%d\n",Channel,ret_temp);        
//		msleep(10);
	}

	ret = ret / times;
	return ret;
}


static void auxadc_cal_prepare(void)
{
	kal_uint32 temp = 0;
	
	temp = DRV_Reg32(0xF1019048);
	g_adc_ge = (temp & 0x000000FF);
	printk("[auxadc]temp = 0x%x, g_adc_ge = 0x%x\n", temp, g_adc_ge);
	
	temp = DRV_Reg32(0xF1019044);
	g_adc_oe = (temp & 0x000000FF);
	printk("[auxadc]temp = 0x%x, g_adc_oe = 0x%x\n", temp, g_adc_oe);
	
	temp = DRV_Reg32(0xF1019040);
	//g_o_vts_abb   = ((temp & 0xFC000000) >> 26);
	g_o_vts       = ((temp & 0x03FE0000) >> 17);
	g_o_vbg       = ((temp & 0x0001FF00) >> 8);
	//g_degc_cali   = ((temp & 0x000000FE) >> 1);
	g_degc_cali   = ((temp & 0x0000007E) >> 1);
	g_adc_cali_en = ((temp & 0x00000001) >> 0);

	g_o_slope     = ((temp & 0xFC000000) >> 26);
	g_o_slope_sign= ((temp & 0x00000080) >> 7);    

      //get y_vbg
      mt65xx_reg_sync_writel(0x0002, 0xf0007804);//TS_CON1
	mt65xx_reg_sync_writel(0x0200, 0xf0007808);//TS_CON2
	msleep(10);
	g_y_vbg = IMM_auxadc_get_evrage_data(20,5);

	temp = DRV_Reg32(0xF1019100);
	g_id = ((temp & 0x80000000) >> 31);

	if(g_id==0)
	{
		g_o_slope = 0;
	}
	
	if(g_adc_cali_en == 1)
	{
		//get y_vbg      
	}
	else
	{
		g_adc_ge = 128;
		g_adc_oe = 128;
		g_o_vts = 292;
		g_o_vbg = 167;
		g_degc_cali = 40;
		g_o_slope = 0;
		g_o_slope_sign = 0;
		g_y_vbg = 1967;
	}
	
	printk("[auxadc]temp = 0x%x, g_y_vbg=%d, g_o_vts = 0x%x, g_o_vbg = 0x%x, g_degc_cali = 0x%x, g_adc_cali_en = 0x%x, g_o_vts_abb = 0x%x, g_o_slope = 0x%x, g_o_slope_sign = 0x%x, g_id = 0x%x\n", 
		temp, g_y_vbg, g_o_vts, g_o_vbg, g_degc_cali, g_adc_cali_en, g_o_vts_abb, g_o_slope, g_o_slope_sign, g_id);

}


///////////////////////////////////////////////////////////////////////////////////////////
//// fop API 
///////////////////////////////////////////////////////////////////////////////////////////
static long auxadc_cali_unlocked_ioctl(struct file *file, unsigned int cmd,unsigned long arg)
{
    int i = 0, ret = 0;
    int *user_data_addr;
    int *nvram_data_addr;
    
    mutex_lock(&auxadc_mutex);

    switch(cmd)
    {
        case TEST_ADC_CALI_PRINT :
            g_AUXADC_Cali = KAL_FALSE;
            break;
        
        case SET_ADC_CALI_Slop:            
            nvram_data_addr = (int *)arg;
            ret = copy_from_user(auxadc_cali_slop, nvram_data_addr, 36);
            g_AUXADC_Cali = KAL_FALSE;           
            /* Protection */
            for (i = 0; i < 16; i++) 
            { 
                if ((*(auxadc_cali_slop + i) == 0) || (*(auxadc_cali_slop + i) == 1)) {
                    *(auxadc_cali_slop + i) = 1000;
                }
            }
            for (i = 0; i < 16; i++) printk("auxadc_cali_slop[%d] = %d\n", i, *(auxadc_cali_slop+i));
            printk("**** MT6589 auxadc_cali ioctl : SET_ADC_CALI_Slop Done!\n");
            break;    
            
        case SET_ADC_CALI_Offset:            
            nvram_data_addr = (int *)arg;
            ret = copy_from_user(auxadc_cali_offset, nvram_data_addr, 36);
            g_AUXADC_Cali = KAL_FALSE;
            for (i = 0; i < 16; i++) printk("auxadc_cali_offset[%d] = %d\n", i, *(auxadc_cali_offset+i));
            printk("**** MT6589 auxadc_cali ioctl : SET_ADC_CALI_Offset Done!\n");            
            break;
            
        case SET_ADC_CALI_Cal :            
            nvram_data_addr = (int *)arg;
            ret = copy_from_user(auxadc_cali_cal, nvram_data_addr, 4);
            g_AUXADC_Cali = KAL_TRUE; /* enable calibration after setting AUXADC_CALI_Cal */
            if (auxadc_cali_cal[0] == 1) {
                g_AUXADC_Cali = KAL_TRUE;
            } else {
                g_AUXADC_Cali = KAL_FALSE;
            }            
            for (i = 0; i < 1; i++) printk("auxadc_cali_cal[%d] = %d\n", i, *(auxadc_cali_cal + i));
            printk("**** MT6589 auxadc_cali ioctl : SET_ADC_CALI_Cal Done!\n");            
            break;    

        case ADC_CHANNEL_READ:       
            g_AUXADC_Cali = KAL_FALSE; /* 20100508 Infinity */
            user_data_addr = (int *)arg;
            ret = copy_from_user(auxadc_in_data, user_data_addr, 8); /* 2*int = 2*4 */
            
            /*ChannelNUm, Counts*/
            //auxadc_out_data[0] = GetOneChannelValue(auxadc_in_data[0], auxadc_in_data[1]);  
            #if 0          
            upmu_adc_measure_vbat_enable(CHR, KAL_TRUE);
            upmu_adc_measure_vsen_enable(CHR, KAL_TRUE);
            upmu_adc_measure_vchr_enable(CHR, KAL_TRUE);
            #endif
			#if 0
            if(auxadc_in_data[0] == 0) // I_SENSE
            {
                auxadc_out_data[0] = IMM_GetOneChannelValue(AUXADC_REF_CURRENT_CHANNEL, auxadc_in_data[1]) * R_BAT_SENSE * auxadc_in_data[1];
            }
            else if( auxadc_in_data[0] == 1 ) // BAT_SENSE
            {
                auxadc_out_data[0] = IMM_GetOneChannelValue(AUXADC_BATTERY_VOLTAGE_CHANNEL, auxadc_in_data[1]) * R_I_SENSE * auxadc_in_data[1];
            }
            else if( auxadc_in_data[0] == 3 ) // V_Charger
            {
                auxadc_out_data[0] = IMM_GetOneChannelValue(AUXADC_CHARGER_VOLTAGE_CHANNEL, auxadc_in_data[1]) * (((R_CHARGER_1 + R_CHARGER_2) * 100) / R_CHARGER_2) * auxadc_in_data[1];			
                auxadc_out_data[0] = auxadc_out_data[0] / 100;
            }	
            else
            {
                auxadc_out_data[0] = IMM_GetOneChannelValue(auxadc_in_data[0], auxadc_in_data[1]) * auxadc_in_data[1];
            }

            if (auxadc_out_data[0] < 0)
                auxadc_out_data[1] = 1; /* failed */
            else
                auxadc_out_data[1] = 0; /* success */

			#endif
			// this ioctl is removed
			printk("this api is removed !! \n");
            ret = copy_to_user(user_data_addr, auxadc_out_data, 8);
            printk("**** ioctl : AUXADC Channel %d * %d times = %d\n", auxadc_in_data[0], auxadc_in_data[1], auxadc_out_data[0]);            
            break;
	
        default:
            g_AUXADC_Cali = KAL_FALSE;
            break;
    }

    mutex_unlock(&auxadc_mutex);
    
    return 0;
}

static int auxadc_cali_open(struct inode *inode, struct file *file)
{ 
    return 0;
}

static int auxadc_cali_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations auxadc_cali_fops = {
    .owner      = THIS_MODULE,
    .unlocked_ioctl  = auxadc_cali_unlocked_ioctl,
    .open       = auxadc_cali_open,
    .release    = auxadc_cali_release,    
};

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_0_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_0_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 0));
    printk("[EM] AUXADC_Channel_0_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_0_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_0_Slope, 0664, show_AUXADC_Channel_0_Slope, store_AUXADC_Channel_0_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_1_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_1_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 1));
    printk("[EM] AUXADC_Channel_1_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_1_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_1_Slope, 0664, show_AUXADC_Channel_1_Slope, store_AUXADC_Channel_1_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_2_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_2_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 2));
    printk("[EM] AUXADC_Channel_2_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_2_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_2_Slope, 0664, show_AUXADC_Channel_2_Slope, store_AUXADC_Channel_2_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_3_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_3_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 3));
    printk("[EM] AUXADC_Channel_3_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_3_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_3_Slope, 0664, show_AUXADC_Channel_3_Slope, store_AUXADC_Channel_3_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_4_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_4_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 4));
    printk("[EM] AUXADC_Channel_4_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_4_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_4_Slope, 0664, show_AUXADC_Channel_4_Slope, store_AUXADC_Channel_4_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_5_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_5_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 5));
    printk("[EM] AUXADC_Channel_5_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_5_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_5_Slope, 0664, show_AUXADC_Channel_5_Slope, store_AUXADC_Channel_5_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_6_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_6_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 6));
    printk("[EM] AUXADC_Channel_6_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_6_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_6_Slope, 0664, show_AUXADC_Channel_6_Slope, store_AUXADC_Channel_6_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_7_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_7_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 7));
    printk("[EM] AUXADC_Channel_7_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_7_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_7_Slope, 0664, show_AUXADC_Channel_7_Slope, store_AUXADC_Channel_7_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_8_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_8_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 8));
    printk("[EM] AUXADC_Channel_8_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_8_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_8_Slope, 0664, show_AUXADC_Channel_8_Slope, store_AUXADC_Channel_8_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_9_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_9_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 9));
    printk("[EM] AUXADC_Channel_9_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_9_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_9_Slope, 0664, show_AUXADC_Channel_9_Slope, store_AUXADC_Channel_9_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_10_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_10_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 10));
    printk("[EM] AUXADC_Channel_10_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_10_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_10_Slope, 0664, show_AUXADC_Channel_10_Slope, store_AUXADC_Channel_10_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_11_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_11_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 11));
    printk("[EM] AUXADC_Channel_11_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_11_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_11_Slope, 0664, show_AUXADC_Channel_11_Slope, store_AUXADC_Channel_11_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_12_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_12_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 12));
    printk("[EM] AUXADC_Channel_12_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_12_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_12_Slope, 0664, show_AUXADC_Channel_12_Slope, store_AUXADC_Channel_12_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_13_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_13_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 13));
    printk("[EM] AUXADC_Channel_13_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_13_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_13_Slope, 0664, show_AUXADC_Channel_13_Slope, store_AUXADC_Channel_13_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_14_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_14_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 14));
    printk("[EM] AUXADC_Channel_14_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_14_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_14_Slope, 0664, show_AUXADC_Channel_14_Slope, store_AUXADC_Channel_14_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_15_Slope
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_15_Slope(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_slop + 15));
    printk("[EM] AUXADC_Channel_15_Slope : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_15_Slope(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_15_Slope, 0664, show_AUXADC_Channel_15_Slope, store_AUXADC_Channel_15_Slope);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_0_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_0_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 0));
    printk("[EM] AUXADC_Channel_0_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_0_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_0_Offset, 0664, show_AUXADC_Channel_0_Offset, store_AUXADC_Channel_0_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_1_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_1_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 1));
    printk("[EM] AUXADC_Channel_1_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_1_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_1_Offset, 0664, show_AUXADC_Channel_1_Offset, store_AUXADC_Channel_1_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_2_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_2_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 2));
    printk("[EM] AUXADC_Channel_2_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_2_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_2_Offset, 0664, show_AUXADC_Channel_2_Offset, store_AUXADC_Channel_2_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_3_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_3_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 3));
    printk("[EM] AUXADC_Channel_3_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_3_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_3_Offset, 0664, show_AUXADC_Channel_3_Offset, store_AUXADC_Channel_3_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_4_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_4_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 4));
    printk("[EM] AUXADC_Channel_4_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_4_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_4_Offset, 0664, show_AUXADC_Channel_4_Offset, store_AUXADC_Channel_4_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_5_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_5_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 5));
    printk("[EM] AUXADC_Channel_5_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_5_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_5_Offset, 0664, show_AUXADC_Channel_5_Offset, store_AUXADC_Channel_5_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_6_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_6_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 6));
    printk("[EM] AUXADC_Channel_6_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_6_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_6_Offset, 0664, show_AUXADC_Channel_6_Offset, store_AUXADC_Channel_6_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_7_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_7_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 7));
    printk("[EM] AUXADC_Channel_7_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_7_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_7_Offset, 0664, show_AUXADC_Channel_7_Offset, store_AUXADC_Channel_7_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_8_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_8_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 8));
    printk("[EM] AUXADC_Channel_8_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_8_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_8_Offset, 0664, show_AUXADC_Channel_8_Offset, store_AUXADC_Channel_8_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_9_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_9_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 9));
    printk("[EM] AUXADC_Channel_9_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_9_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_9_Offset, 0664, show_AUXADC_Channel_9_Offset, store_AUXADC_Channel_9_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_10_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_10_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 10));
    printk("[EM] AUXADC_Channel_10_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_10_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	printk("[EM] Not Support Write Function\n");	
	return size;
}
static DEVICE_ATTR(AUXADC_Channel_10_Offset, 0664, show_AUXADC_Channel_10_Offset, store_AUXADC_Channel_10_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_11_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_11_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 11));
    printk("[EM] AUXADC_Channel_11_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_11_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_11_Offset, 0664, show_AUXADC_Channel_11_Offset, store_AUXADC_Channel_11_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_12_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_12_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 12));
    printk("[EM] AUXADC_Channel_12_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_12_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_12_Offset, 0664, show_AUXADC_Channel_12_Offset, store_AUXADC_Channel_12_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_13_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_13_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 13));
    printk("[EM] AUXADC_Channel_13_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_13_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_13_Offset, 0664, show_AUXADC_Channel_13_Offset, store_AUXADC_Channel_13_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_14_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_14_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 14));
    printk("[EM] AUXADC_Channel_14_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_14_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_14_Offset, 0664, show_AUXADC_Channel_14_Offset, store_AUXADC_Channel_14_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_15_Offset
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_15_Offset(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = (*(auxadc_cali_offset + 15));
    printk("[EM] AUXADC_Channel_15_Offset : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_15_Offset(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_15_Offset, 0664, show_AUXADC_Channel_15_Offset, store_AUXADC_Channel_15_Offset);

///////////////////////////////////////////////////////////////////////////////////////////
//// Create File For EM : AUXADC_Channel_Is_Calibration
///////////////////////////////////////////////////////////////////////////////////////////
static ssize_t show_AUXADC_Channel_Is_Calibration(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 2;
    ret_value = g_AUXADC_Cali;
    printk("[EM] AUXADC_Channel_Is_Calibration : %d\n", ret_value);
    return sprintf(buf, "%u\n", ret_value);
}
static ssize_t store_AUXADC_Channel_Is_Calibration(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support Write Function\n");	
    return size;
}
static DEVICE_ATTR(AUXADC_Channel_Is_Calibration, 0664, show_AUXADC_Channel_Is_Calibration, store_AUXADC_Channel_Is_Calibration);

static ssize_t show_AUXADC_register(struct device *dev,struct device_attribute *attr, char *buf)
{
    
	printk("[adc_udvt]: AUXADC_CON0=%x\n",*(volatile u16 *)AUXADC_CON0);
    printk("[adc_udvt]: AUXADC_CON1=%x\n",*(volatile u16 *)AUXADC_CON1);
    printk("[adc_udvt]: AUXADC_CON2=%x\n",*(volatile u16 *)AUXADC_CON2);
    //printk("[adc_udvt]: AUXADC_CON3=%x\n",*(volatile u16 *)AUXADC_CON3);
#if 0
	if(enable_clock(MT65XX_PDN_PERI_AUXADC,"AUXADC"))
        printk("hwEnableClock AUXADC failed.");
#endif
    return sprintf(buf, "AUXADC_CON0:%x\n AUXADC_CON1:%x\n AUXADC_CON2:%x\n"
		, *(volatile u16 *)AUXADC_CON0,*(volatile u16 *)AUXADC_CON1,*(volatile u16 *)AUXADC_CON2);
}

static ssize_t store_AUXADC_register(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[EM] Not Support store_AUXADC_register\n");	
    return size;
}

static DEVICE_ATTR(AUXADC_register, 0664, show_AUXADC_register, store_AUXADC_register);


static ssize_t show_AUXADC_chanel(struct device *dev,struct device_attribute *attr, char *buf)
{
    //read data
    int i = 0, data[4] = {0,0,0,0};
	char buf_temp[960];
	int res =0;
    for (i = 0; i < 5; i++) 
    {
        //printk("[adc_driver]: i=%d\n",i);
		res = IMM_GetOneChannelValue(i,data,NULL);
		if(res < 0)
		{ 
			   printk("[adc_driver]: get data error\n");
			   break;
			   
		}
		else
		{
		       printk("[adc_driver]: channel[%d]=%d.%d \n",i,data[0],data[1]);
			   sprintf(buf_temp,"channel[%d]=%d.%d \n",i,data[0],data[1]);
			   strcat(buf,buf_temp);
			  // sprintf(buf,"channel[%d]=%d.%d \n",i,data[0],data[1]);
		}
			
    } 
	printk("[adc_driver]: AUXADC_CON0=%x\n",*(volatile u16 *)AUXADC_CON0);
    printk("[adc_driver]: AUXADC_CON1=%x\n",*(volatile u16 *)AUXADC_CON1);
    printk("[adc_driver]: AUXADC_CON2=%x\n",*(volatile u16 *)AUXADC_CON2);
    //printk("[adc_driver]: AUXADC_CON3=%x\n",*(volatile u16 *)AUXADC_CON3);

    sprintf(buf_temp, "AUXADC_CON0:%x\n AUXADC_CON1:%x\n AUXADC_CON2:%x\n"
		, *(volatile u16 *)AUXADC_CON0,*(volatile u16 *)AUXADC_CON1,*(volatile u16 *)AUXADC_CON2);
	strcat(buf,buf_temp);
	
    return strlen(buf);
}

static struct task_struct *thread = NULL;
static int g_start_debug_thread =0;


static int dbug_thread(void *unused) 
{
   int i = 0, data[4] = {0,0,0,0};
   int res =0;
   int rawdata=0;
   int cali_voltage =0;
   
   while(g_start_debug_thread)
   {

      //channel12~channel15 used for TP 
      for (i = 0; i < 16; i++) 
      {
        //printk("[adc_driver]: i=%d\n",i);
       
		res = IMM_GetOneChannelValue(i,data,&rawdata);
		if(res < 0)
		{ 
			   printk("[adc_driver]: get data error\n");
			   break;
			   
		}
		else
		{
		       printk("[adc_driver]: channel[%d]raw =%d\n",i,rawdata);
		       printk("[adc_driver]: channel[%d]=%d.%.02d \n",i,data[0],data[1]);
			  
		}
		res = IMM_GetOneChannelValue_Cali(i,&cali_voltage );
		if(res < 0)
		{ 
			   printk("[adc_driver]: get cali voltage error\n");
			   break;
			   
		}
		else
		{
		       printk("[adc_driver]: channel[%d] cali_voltage =%d\n",i,cali_voltage);
  
		}
	  msleep(500);
			
      } 
	  msleep(500);

   }
   return 0;
}


static ssize_t store_AUXADC_channel(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    //printk("[EM] Not Support store_AUXADC_channel\n");	
	
	unsigned int start_flag;
	int error;
	
		if (sscanf(buf, "%u", &start_flag) != 1) {
			printk("[adc_driver]: Invalid values\n");
			return -EINVAL;
		}
	
		printk("[adc_driver] start flag =%d \n",start_flag);
	    g_start_debug_thread = start_flag;
		if(1 == start_flag)
		{
		   thread = kthread_run(dbug_thread, 0, "AUXADC");
		   
		   if (IS_ERR(thread)) 
		   { 
			  error = PTR_ERR(thread);
			  printk( "[adc_driver] failed to create kernel thread: %d\n", error);
		   }
		}
	
    return size;
}

static DEVICE_ATTR(AUXADC_read_channel, 0664, show_AUXADC_chanel, store_AUXADC_channel);


// platform_driver API 
static int mt6589_auxadc_probe(struct platform_device *dev)	
{    
    int ret = 0;
   // kal_int32 slope,offset;	
    //int i= 0;
    struct class_device *class_dev = NULL;
    
    printk("******** MT6589 AUXADC driver probe!! ********\n");
	adc_channel_info_init();

	if(enable_clock(MT65XX_PDN_PERI_AUXADC,"AUXADC"))
        printk("hwEnableClock AUXADC failed.");
    
    /* Integrate with NVRAM */
    ret = alloc_chrdev_region(&auxadc_cali_devno, 0, 1, AUXADC_CALI_DEVNAME);
    if (ret) 
        printk("Error: Can't Get Major number for auxadc_cali\n");
        
    auxadc_cali_cdev = cdev_alloc();
    auxadc_cali_cdev->owner = THIS_MODULE;
    auxadc_cali_cdev->ops = &auxadc_cali_fops;
    ret = cdev_add(auxadc_cali_cdev, auxadc_cali_devno, 1);
    if(ret)
        printk("auxadc_cali Error: cdev_add\n");
        
    auxadc_cali_major = MAJOR(auxadc_cali_devno);
    auxadc_cali_class = class_create(THIS_MODULE, AUXADC_CALI_DEVNAME);
    class_dev = (struct class_device *)device_create(auxadc_cali_class, 
                NULL, auxadc_cali_devno, NULL, AUXADC_CALI_DEVNAME);
    printk("[MT6589 AUXADC_probe] NVRAM prepare : done !!\n");
    
    /* For EM */
	if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_register)) != 0) goto exit;
	if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_read_channel)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_0_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_1_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_2_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_3_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_4_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_5_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_6_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_7_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_8_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_9_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_10_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_11_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_12_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_13_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_14_Slope)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_15_Slope)) != 0) goto exit;
    
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_0_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_1_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_2_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_3_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_4_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_5_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_6_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_7_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_8_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_9_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_10_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_11_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_12_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_13_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_14_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_15_Offset)) != 0) goto exit;
    if ((ret = device_create_file(&(dev->dev), &dev_attr_AUXADC_Channel_Is_Calibration)) != 0) goto exit;

     g_adc_init_flag =1;
     //read calibration data from EFUSE
    auxadc_cal_prepare();
#if 0	 
    if(disable_clock(MT65XX_PDN_PERI_AUXADC,"AUXADC"))
        printk("hwDisableClock AUXADC failed in probe.");
#endif	
exit:
    return ret;
}

static int mt6589_auxadc_remove(struct platform_device *dev)	
{
    printk("******** MT6589 auxadc driver remove!! ********\n" );    
    return 0;
}

static void mt6589_auxadc_shutdown(struct platform_device *dev)	
{
    printk("******** MT6589 auxadc driver shutdown!! ********\n" );
}

static int mt6589_auxadc_suspend(struct platform_device *dev, pm_message_t state)	
{
    //printk("******** MT6589 auxadc driver suspend!! ********\n" );
	/*
	if(disable_clock(MT65XX_PDN_PERI_AUXADC,"AUXADC"))
        printk("hwEnableClock AUXADC failed.");
	*/
    return 0;
}

static int mt6589_auxadc_resume(struct platform_device *dev)
{
    //printk("******** MT6589 auxadc driver resume!! ********\n" );
	/*
	if(enable_clock(MT65XX_PDN_PERI_AUXADC,"AUXADC"))
	{
	    printk("hwEnableClock AUXADC again!!!.");
	    if(enable_clock(MT65XX_PDN_PERI_AUXADC,"AUXADC"))
	    {printk("hwEnableClock AUXADC failed.");}
        
	}
	*/
    return 0;
}
/*
struct platform_device MT6589_auxadc_device = {
    .name   = "mt6589-auxadc",
    .id     = -1,
};
*/
static struct platform_driver mt6589_auxadc_driver = {
    .probe      = mt6589_auxadc_probe,
    .remove     = mt6589_auxadc_remove,
    .shutdown   = mt6589_auxadc_shutdown,
    #ifdef CONFIG_PM
        .suspend = mt6589_auxadc_suspend,
        .resume	 = mt6589_auxadc_resume,
    #endif
    .driver     = {
    .name       = "mt-auxadc",
    },
};
 
static int __init mt6589_auxadc_init(void)
{
    int ret;
    /*
    ret = platform_device_register(&MT6589_auxadc_device);
    if (ret) {
        printk("****[mt6589_auxadc_driver] Unable to device register(%d)\n", ret);
        return ret;
    }
    */
    ret = platform_driver_register(&mt6589_auxadc_driver);
    if (ret) {
        printk("****[mt6589_auxadc_driver] Unable to register driver (%d)\n", ret);
        return ret;
    }
    printk("****[mt6589_auxadc_driver] Initialization : DONE \n");
    return 0;
}

static void __exit mt6589_auxadc_exit (void)
{
}

module_init(mt6589_auxadc_init);
module_exit(mt6589_auxadc_exit);

MODULE_AUTHOR("MTK");
MODULE_DESCRIPTION("MT689 AUXADC Device Driver");
MODULE_LICENSE("GPL");

