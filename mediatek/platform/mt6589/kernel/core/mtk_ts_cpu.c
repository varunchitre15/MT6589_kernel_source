

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/thermal.h>
#include <linux/platform_device.h>
#include <linux/aee.h>
#include <linux/xlog.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>

#include<mach/sync_write.h>
#include <mach/mt_irq.h>
#include "mach/mtk_thermal_monitor.h"
#include <mach/system.h>
//#include "mach/mtk_cpu_management.h"

#include "mach/mt_typedefs.h"
#include "mach/mt_thermal.h"
#include "mach/mt_gpufreq.h"

static unsigned int interval = 0; /* seconds, 0 : no auto polling */
static unsigned int trip_temp[10] = {120000,110000,100000,90000,80000,70000,65000,60000,55000,50000};

static unsigned int *cl_dev_state;
static unsigned int cl_dev_sysrst_state=0;
static struct thermal_zone_device *thz_dev;

static struct thermal_cooling_device **cl_dev;
static struct thermal_cooling_device *cl_dev_sysrst;

static int mtktscpu_debug_log = 0;
static int kernelmode = 0;
static int g_THERMAL_TRIP[10] = {0,0,0,0,0,0,0,0,0,0};

static int num_trip=10;
int MA_len=5;
int MA_len_temp=0;
static int proc_write_flag=0;
static char *cooler_name;

static char g_bind0[20]="mtktscpu-sysrst";
static char g_bind1[20]="2300";
static char g_bind2[20]="2100";
static char g_bind3[20]="1900";
static char g_bind4[20]="1700";
static char g_bind5[20]="1500";
static char g_bind6[20]="1300";
static char g_bind7[20]="1100";
static char g_bind8[20]="900";
static char g_bind9[20]="700";


#define MTKTSCPU_TEMP_CRIT 120000 /* 120.000 degree Celsius */

#define mtktscpu_dprintk(fmt, args...)   \
do {                                    \
	if (mtktscpu_debug_log) {                \
		xlog_printk(ANDROID_LOG_INFO, "Power/CPU_Thermal", fmt, ##args); \
	}                                   \
} while(0)

//extern int IMM_GetOneChannelValue(int dwChannel, int data[4], int* rawdata);
//extern int IMM_IsAdcInitReady(void);
extern void mt_cpufreq_thermal_protect(unsigned int limited_power);
extern void mt_gpufreq_thermal_protect(unsigned int limited_power);
static kal_int32 temperature_to_raw_abb(kal_uint32 ret);
//static int last_cpu_t=0;
int last_abb_t=0;
//int	last_pa_t=0;

static kal_int32 g_adc_ge = 0;
static kal_int32 g_adc_oe = 0;
static kal_int32 g_corner_TS = 0;
static kal_int32 g_o_vtsmcu1 = 0;
static kal_int32 g_o_vtsmcu2 = 0;
static kal_int32 g_o_vtsmcu3 = 0;
static kal_int32 g_o_vtsabb = 0;
static kal_int32 g_degc_cali = 0;
static kal_int32 g_adc_cali_en = 0;
static kal_int32 g_o_slope = 0;
static kal_int32 g_o_slope_sign = 0;
static kal_int32 g_id = 0;

static kal_int32 g_ge = 0;
static kal_int32 g_oe = 0;
static kal_int32 g_gain = 0;

//static kal_int32 g_x_roomt1 = 0;
//static kal_int32 g_x_roomt2 = 0;
//static kal_int32 g_x_roomt3 = 0;
static kal_int32 g_x_roomtabb = 0;
static int Num_of_OPP=0;
static int Num_of_GPU_OPP=0;
//static int curr_high=0, curr_low=0;

#define y_curr_repeat_times 1
#define THERMAL_NAME    "mtk-thermal"
//#define GPU_Default_POWER	456

struct mtk_cpu_power_info
{
	unsigned int cpufreq_khz;
	unsigned int cpufreq_ncpu;
	unsigned int cpufreq_power;
};
struct mtk_gpu_power_info
{
	unsigned int gpufreq_khz;
	unsigned int gpufreq_power;
};
static struct mtk_cpu_power_info *mtk_cpu_power;
static struct mtk_gpu_power_info *mtk_gpu_power;

static bool talking_flag=false;
void set_taklking_flag(bool flag)
{
	talking_flag = flag;
	printk("Power/CPU_Thermal: talking_flag=%d", talking_flag);
	return;
}


void get_thermal_slope_intercept(struct TS_PTPOD *ts_info)
{
	unsigned int temp0, temp1, temp2;
	struct TS_PTPOD ts_ptpod;

	//temp0 = (10000*100000/4096/g_gain)*15/18;
	temp0 = (10000*100000/g_gain)*15/18;
//	printk("temp0=%d\n", temp0);
	if(g_o_slope_sign==0)
	{
		temp1 = temp0/(165+g_o_slope);
	}
	else
	{
		temp1 = temp0/(165-g_o_slope);
	}
//	printk("temp1=%d\n", temp1);
	//ts_ptpod.ts_MTS = temp1 - (2*temp1) + 2048;
	ts_ptpod.ts_MTS = temp1;

	temp0 = (g_degc_cali *10 / 2);
	temp1 = ((10000*100000/4096/g_gain)*g_oe + g_x_roomtabb*10)*15/18;
//	printk("temp1=%d\n", temp1);
	if(g_o_slope_sign==0)
	{
		temp2 = temp1*10/(165+g_o_slope);
	}
	else
	{
		temp2 = temp1*10/(165-g_o_slope);
	}
//	printk("temp2=%d\n", temp2);
	ts_ptpod.ts_BTS = (temp0+temp2-250)*4/10;

	//ts_info = &ts_ptpod;
	ts_info->ts_MTS = ts_ptpod.ts_MTS;
	ts_info->ts_BTS = ts_ptpod.ts_BTS;
	printk("ts_MTS=%d, ts_BTS=%d\n",ts_ptpod.ts_MTS, ts_ptpod.ts_BTS);

	return;
}
EXPORT_SYMBOL(get_thermal_slope_intercept);


/*
static void set_high_low_threshold(int high, int low)
{
	int temp=0;
	int raw_low=0, raw_high=0, raw_high_offset=0, raw_hot_normal=0, raw_low_offset=0;

	if( (curr_high==high) && (curr_low==low) )
		return;

	curr_high = high;
	curr_low = low;
//	mtktscpu_dprintk("Set_high_low_threshold: curr_high=%d, curr_low=%d\n", curr_high, curr_low);

	raw_low = temperature_to_raw_abb(low);	//bigger
	raw_high = temperature_to_raw_abb(high);//smaller

	mtktscpu_dprintk("[set_high_low_threshold]: raw_low=%d, raw_high=%d\n",raw_low, raw_high);
	//calculate high offset threshold, hot to normal threshold, low offset threshold
	temp = (raw_low - raw_high)/4;
	raw_high_offset = raw_high + temp;
	raw_hot_normal = raw_high_offset + temp;
	raw_low_offset = raw_hot_normal + temp;
//	mtktscpu_dprintk("set_high_low_threshold, raw_high_offset=%d, raw_hot_normal=%d, raw_low_offset=%d\n", raw_high_offset, raw_hot_normal, raw_low_offset);

//	mt65xx_reg_sync_writel(0x0, TEMPMONCTL0);           // disable periodoc temperature sensing point 0
	temp = DRV_Reg32(TEMPMONINT);
	mt65xx_reg_sync_writel(temp & 0xFFFFFFFC, TEMPMONINT);			// disable interrupt

	mt65xx_reg_sync_writel(raw_high, TEMPHTHRE);			// set hot threshold
	mt65xx_reg_sync_writel(raw_high_offset, TEMPOFFSETH);	// set high offset threshold
	mt65xx_reg_sync_writel(raw_hot_normal, TEMPH2NTHRE);	// set hot to normal threshold
	mt65xx_reg_sync_writel(raw_low_offset, TEMPOFFSETL);	// set low offset threshold
	mt65xx_reg_sync_writel(raw_low, TEMPCTHRE);				// set cold threshold

//	mt65xx_reg_sync_writel(0x1, TEMPMONCTL0);           // enable periodoc temperature sensing point 0
	temp = temp | 3;
	mt65xx_reg_sync_writel(temp, TEMPMONINT);		// enable cold threshold, and high threshold interrupt
//	mt65xx_reg_sync_writel(temp | 0x00ffff, TEMPMONINT);
//	mtktscpu_dprintk("set_high_low_threshold end, temp=0x%x", temp);

}
*/
static irqreturn_t thermal_interrupt_handler(int irq, void *dev_id)
{
	kal_uint32 ret = 0;
//	int i=0;
	ret = DRV_Reg32(TEMPMONINTSTS);

	xlog_printk(ANDROID_LOG_DEBUG, "[Power/CPU_Thermal]", "thermal_isr: [Interrupt trigger]: status = 0x%x\n", ret);
	if (ret & THERMAL_MON_CINTSTS0)
	{
		xlog_printk(ANDROID_LOG_DEBUG, "[Power/CPU_Thermal]", "thermal_isr: thermal sensor point 0 - cold interrupt trigger\n");
		//call thermal monitor interrupt, mode=0
/*		mtk_thermal_zone_bind_trigger_trip(thz_dev,  curr_low, 0);
		for(i=0; i<num_trip; i++)
		{
			if(curr_low == trip_temp[i])
				break;
		}
		if(i==0)
		{
			xlog_printk(ANDROID_LOG_DEBUG, "[Power/CPU_Thermal]", "thermal_isr: cold interrupt error\n");
		}
		else if(i==num_trip)
			set_high_low_threshold(trip_temp[i-1], 10000);
		else
			set_high_low_threshold(trip_temp[i], trip_temp[i+1]);
*/
	}
	if (ret & THERMAL_MON_HINTSTS0)
	{
		xlog_printk(ANDROID_LOG_DEBUG, "[Power/CPU_Thermal]", "thermal_isr: thermal sensor point 0 - hot interrupt trigger\n");
		//call thermal monitor interrupt, mode=1
/*		mtk_thermal_zone_bind_trigger_trip(thz_dev, curr_high, 1);
		for(i=0; i<num_trip; i++)
		{
			if(curr_high == trip_temp[i])
				break;
		}
		if(i==0)
		{
			//do nothing
		}
		else if(i==num_trip)
			xlog_printk(ANDROID_LOG_DEBUG, "[Power/CPU_Thermal]", "thermal_isr: hot interrupt error\n");
		else
			set_high_low_threshold(trip_temp[i-1], trip_temp[i]);
*/
	}

	if(ret & THERMAL_tri_SPM_State0)
		xlog_printk(ANDROID_LOG_DEBUG, "[Power/CPU_Thermal]", "thermal_isr: Thermal state0 to trigger SPM state0 \n");
	if(ret & THERMAL_tri_SPM_State1)
		xlog_printk(ANDROID_LOG_DEBUG, "[Power/CPU_Thermal]", "thermal_isr: Thermal state1 to trigger SPM state1\n");
	if(ret & THERMAL_tri_SPM_State2)
		xlog_printk(ANDROID_LOG_DEBUG, "[Power/CPU_Thermal]", "thermal_isr: Thermal state2 to trigger SPM state2\n");

	return IRQ_HANDLED;
}


static void thermal_reset_and_initial(void)
{
	UINT32 temp = 0;

	mtktscpu_dprintk("[Reset and init thermal controller]\n");

	//reset thremal ctrl
	temp = DRV_Reg32(PERI_GLOBALCON_RST0);
	temp |= 0x00010000;
	mt65xx_reg_sync_writel(temp, PERI_GLOBALCON_RST0);

	temp = DRV_Reg32(PERI_GLOBALCON_RST0);
	temp &= 0xFFFEFFFF;
	mt65xx_reg_sync_writel(temp, PERI_GLOBALCON_RST0);

	// AuxADC Initialization
	temp = DRV_Reg32(AUXADC_CON0_V);
	temp &= 0xFFFFF7FF;
	mt65xx_reg_sync_writel(temp, AUXADC_CON0_V);        // disable auxadc channel 11 synchronous mode

	mt65xx_reg_sync_writel(0x800, AUXADC_CON1_CLR_V);    // disable auxadc channel 11 immediate mode


	mt65xx_reg_sync_writel(0x000003FF, TEMPMONCTL1);    // counting unit is 1024 * 20ns


    mt65xx_reg_sync_writel(0x03FF0000, TEMPMONCTL2);	// sensing interval is 1024 * 20ns = 20ms


	//mt65xx_reg_sync_writel(0x0000000F, TEMPAHBPOLL);    // polling interval to check if temperature sense is ready, 32k*16
	mt65xx_reg_sync_writel(0x00FFFFFF, TEMPAHBPOLL);		//total update time = 0x1000000x20ns +  20ms
	mt65xx_reg_sync_writel(0xFFFFFFFF, TEMPAHBTO);      // exceed this polling time, IRQ would be inserted

	mt65xx_reg_sync_writel(0x00000000, TEMPMONIDET0);   // times for interrupt occurrance
//	mt65xx_reg_sync_writel(0x00000000, TEMPMONIDET1);   // times for interrupt occurrance

//	mt65xx_reg_sync_writel(0x0000008FC, TEMPHTHRE);     // set hot threshold
//	mt65xx_reg_sync_writel(0x00000960, TEMPOFFSETH);    // set high offset threshold
//	mt65xx_reg_sync_writel(0x00000A8C, TEMPH2NTHRE);    // set hot to normal threshold
//	mt65xx_reg_sync_writel(0x00000C80, TEMPOFFSETL);    // set low offset threshold
//	mt65xx_reg_sync_writel(0x00000CE4, TEMPCTHRE);      // set cold threshold

	mt65xx_reg_sync_writel(0x0000000, TEMPMSRCTL0);     // temperature sampling control, one sample

	mt65xx_reg_sync_writel(0x800, AUXADC_CON1_SET_V);    // enable auxadc channel 11 immediate mode

	//mt65xx_reg_sync_writel(0x0, TEMPADCPNP0);                       // this value will be stored to TEMPPNPMUXADDR (TEMPSPARE0) automatically by hw
	//mt65xx_reg_sync_writel((UINT32) TEMPSPARE0, TEMPPNPMUXADDR);    // AHB address for pnp sensor mux selection

	mt65xx_reg_sync_writel(0x800, TEMPADCMUX);                          // this value will be stored to TEMPPNPMUXADDR (TEMPSPARE0) automatically by hw
	mt65xx_reg_sync_writel((UINT32) AUXADC_CON1_CLR_P, TEMPADCMUXADDR); // AHB address for auxadc mux selection

	mt65xx_reg_sync_writel(0x800, TEMPADCEN);                           // AHB value for auxadc enable
	mt65xx_reg_sync_writel((UINT32) AUXADC_CON1_SET_P, TEMPADCENADDR);  // AHB address for auxadc enable (channel 0 immediate mode selected)

	mt65xx_reg_sync_writel((UINT32) AUXADC_DAT11_P, TEMPADCVALIDADDR);   // AHB address for auxadc valid bit
	mt65xx_reg_sync_writel((UINT32) AUXADC_DAT11_P, TEMPADCVOLTADDR);    // AHB address for auxadc voltage output
	mt65xx_reg_sync_writel(0x0, TEMPRDCTRL);                            // read valid & voltage are at the same register
	mt65xx_reg_sync_writel(0x0000002C, TEMPADCVALIDMASK);               // indicate where the valid bit is (the 12th bit is valid bit and 1 is valid)
	mt65xx_reg_sync_writel(0x0, TEMPADCVOLTAGESHIFT);                   // do not need to shift
	mt65xx_reg_sync_writel(0x2, TEMPADCWRITECTRL);                      // enable auxadc mux write transaction

	//mt65xx_reg_sync_writel(0x0000FFFF, TEMPMONINT);                     // enable all interrupt
	mt65xx_reg_sync_writel(0x00000001, TEMPMONCTL0);                    // enable periodoc temperature sensing point 0
	mt65xx_reg_sync_writel(0x40, TS_CON0);	//read abb need
}


static void set_thermal_ctrl_trigger_SPM(int temperature)
{
	int temp = 0;
	int raw_high, raw_middle, raw_low;

	mtktscpu_dprintk("[Set_thermal_ctrl_trigger_SPM]: temperature=%d\n", temperature);

	//temperature to trigger SPM state2
	raw_high = temperature_to_raw_abb(temperature);
	raw_middle = temperature_to_raw_abb(70000);
	raw_low = temperature_to_raw_abb(50000);

	temp = DRV_Reg32(TEMPMONINT);
	mt65xx_reg_sync_writel(temp & 0x8FFFFFFF, TEMPMONINT);	// enable trigger SPM interrupt

	mt65xx_reg_sync_writel(0x20000, TEMPPROTCTL);
	mt65xx_reg_sync_writel(raw_low, TEMPPROTTA);
	mt65xx_reg_sync_writel(raw_middle, TEMPPROTTB);
	mt65xx_reg_sync_writel(raw_high, TEMPPROTTC);

	mt65xx_reg_sync_writel(temp | 0xE0000000, TEMPMONINT);	// enable trigger SPM interrupt
}



int mtk_cpufreq_register(struct mtk_cpu_power_info *freqs, int num)
{
	int i=0;
	printk("[Power/CPU_Thermal] setup cpu power table\n");

	mtk_cpu_power = kzalloc((num) * sizeof(struct mtk_cpu_power_info), GFP_KERNEL);
	if (mtk_cpu_power==NULL)
		return -ENOMEM;

	for (i=0; i<num; i++)
	{
		mtk_cpu_power[i].cpufreq_khz = freqs[i].cpufreq_khz;
		mtk_cpu_power[i].cpufreq_ncpu = freqs[i].cpufreq_ncpu;
		mtk_cpu_power[i].cpufreq_power = freqs[i].cpufreq_power;

		printk("[Power/CPU_Thermal] freqs[%d].cpufreq_khz=%u, .cpufreq_ncpu=%u, .cpufreq_power=%u\n",
				i, freqs[i].cpufreq_khz, freqs[i].cpufreq_ncpu, freqs[i].cpufreq_power);
	}

/*	{
		num=19;//700~2500
		cl_dev_state = kzalloc((num) * sizeof(unsigned int), GFP_KERNEL);
		if(cl_dev_state==NULL)
			return -ENOMEM;

		cl_dev = (struct thermal_cooling_device **)kzalloc((num) * sizeof(struct thermal_cooling_device *), GFP_KERNEL);
		if(cl_dev==NULL)
			return -ENOMEM;

		cooler_name = kzalloc((num) * sizeof(char) * 20, GFP_KERNEL);
		if(cooler_name==NULL)
			return -ENOMEM;

		//for(i=0; i<num; i++)
		//	sprintf(cooler_name+(i*20), "%d", mtk_cpu_power[i].cpufreq_power);
		for(i=0; i<num; i++)
			sprintf(cooler_name+(i*20), "%d", ((7+i)*100));

		Num_of_OPP = num;
	}*/
	return 0;
}
EXPORT_SYMBOL(mtk_cpufreq_register);

static int init_cooler(void)
{
	int i;
	int num=19;//700~2500

	cl_dev_state = kzalloc((num) * sizeof(unsigned int), GFP_KERNEL);
	if(cl_dev_state==NULL)
		return -ENOMEM;

	cl_dev = (struct thermal_cooling_device **)kzalloc((num) * sizeof(struct thermal_cooling_device *), GFP_KERNEL);
	if(cl_dev==NULL)
		return -ENOMEM;

	cooler_name = kzalloc((num) * sizeof(char) * 20, GFP_KERNEL);
	if(cooler_name==NULL)
		return -ENOMEM;

	//for(i=0; i<num; i++)
	//	sprintf(cooler_name+(i*20), "%d", mtk_cpu_power[i].cpufreq_power);
	for(i=0; i<num; i++)
		sprintf(cooler_name+(i*20), "%d", (i*100+700));

	Num_of_OPP = num;
	return 0;
}


int mtk_gpufreq_register(struct mtk_gpu_power_info *freqs, int num)
{
	int i=0;
	printk("[Power/CPU_Thermal] setup gpu power table\n");

	mtk_gpu_power = kzalloc((num) * sizeof(struct mtk_gpu_power_info), GFP_KERNEL);
	if (mtk_gpu_power==NULL)
		return -ENOMEM;

	for (i=0; i<num; i++)
	{
		mtk_gpu_power[i].gpufreq_khz = freqs[i].gpufreq_khz;
		mtk_gpu_power[i].gpufreq_power = freqs[i].gpufreq_power;

		printk("[Power/CPU_Thermal] freqs[%d].cpufreq_khz=%u, .cpufreq_power=%u\n",
				i, freqs[i].gpufreq_khz, freqs[i].gpufreq_power);
	}

	Num_of_GPU_OPP = num;
	return 0;
}
EXPORT_SYMBOL(mtk_gpufreq_register);


static void thermal_cal_prepare(void)
{
	kal_uint32 temp0, temp1, temp2 = 0;

	temp0 = DRV_Reg32(0xF0009100);
	temp1 = DRV_Reg32(0xF0009104);
	temp2 = DRV_Reg32(0xF0009108);
	printk("[Power/CPU_Thermal] [Thermal calibration] Reg(0xF0009100)=0x%x, Reg(0xF0009104)=0x%x, Reg(0xF0009108)=0x%x\n", temp0, temp1, temp2);

	g_adc_ge = ((temp1 & 0xFF000000)>>24) + (((temp2&0x000C0000)>>18)<<8);
	g_adc_oe = ((temp1 & 0x00FF0000)>>16) + (((temp2&0x00030000)>>16)<<8);
	g_corner_TS = (temp2 & 0x00100000)>>20;

	g_o_vtsmcu1 = (temp0 & 0x03FE0000)>>17;
	g_o_vtsmcu2 = (temp0 & 0x0001FF00)>>8;
	g_o_vtsmcu3 = ((temp2 & 0xFF000000)>>24)| ((temp1 & 0x00000001)<<8) ;
	g_o_vtsabb = (temp1 & 0x0000FF80)>>7 ;

	g_degc_cali = (temp0 & 0x0000007E)>>1;
	g_adc_cali_en = (temp0 & 0x00000001);
	g_o_slope = (temp0 & 0xFC000000)>>26;
	g_o_slope_sign = (temp0 & 0x00000080)>>7;

	g_id=0; //use TSMC now
	if(g_id==0)
	{
		g_o_slope=0;
	}

	if(g_adc_cali_en == 1)
	{
		//thermal_enable = true;
	}
	else
	{
		g_adc_ge = 128;
		g_adc_oe = 128;
		g_degc_cali = 40;
		g_o_slope = 0;
		g_o_slope_sign = 0;
		//g_o_vtsabb = 275;
		//g_o_vtsabb = 260;
		g_o_vtsabb = 272;
	}
	printk("[Power/CPU_Thermal] [Thermal calibration] g_adc_ge = 0x%x, g_adc_oe = 0x%x, g_degc_cali = 0x%x, g_adc_cali_en = 0x%x, g_o_slope = 0x%x, g_o_slope_sign = 0x%x, g_id = 0x%x\n",
		g_adc_ge, g_adc_oe, g_degc_cali, g_adc_cali_en, g_o_slope, g_o_slope_sign, g_id);
	printk("[Power/CPU_Thermal] [Thermal calibration] g_o_vtsmcu1 = 0x%x, g_o_vtsmcu2 = 0x%x, g_o_vtsmcu3 = 0x%x, g_o_vtsabb = 0x%x\n",
		g_o_vtsmcu1, g_o_vtsmcu2, g_o_vtsmcu3, g_o_vtsabb);
}

static void thermal_cal_prepare_2(kal_uint32 ret)
{
	kal_int32 format_1, format_2, format_3, format_abb= 0;

	if(g_corner_TS==1)
	{
		g_ge = ((g_adc_ge - 512) * 10000 ) / 4096; // ge * 10000
		g_oe = (g_adc_oe - 512);
	}
	else
	{
		g_ge = ((g_adc_ge - 128) * 10000 ) / 4096; // ge * 10000
		g_oe = (g_adc_oe - 128);
	}
	g_gain = (10000 + g_ge);

	format_1 = (g_o_vtsmcu1 + 3350 - g_oe);
	format_2 = (g_o_vtsmcu2 + 3350 - g_oe);
	format_3 = (g_o_vtsmcu3 + 3350 - g_oe);
	format_abb = (g_o_vtsabb + 3350 - g_oe);

//	g_x_roomt1 = (((format_1 * 10000) / 4096) * 10000) / g_gain; // x_roomt * 10000
//	g_x_roomt2 = (((format_2 * 10000) / 4096) * 10000) / g_gain; // x_roomt * 10000
//	g_x_roomt3 = (((format_3 * 10000) / 4096) * 10000) / g_gain; // x_roomt * 10000
	g_x_roomtabb = (((format_abb * 10000) / 4096) * 10000) / g_gain; // x_roomt * 10000

	printk("[Power/CPU_Thermal] [Thermal calibration] g_ge = %d, g_oe = %d, g_gain = %d, g_x_roomtabb = %d\n",
		g_ge, g_oe, g_gain, g_x_roomtabb);
}
/*
static kal_int32 thermal_cal_exec(kal_uint32 ret)
{
	kal_int32 t_current = 0;
	kal_int32 y_curr = ret;
	kal_int32 format_1 = 0;
	kal_int32 format_2 = 0;
	kal_int32 format_3 = 0;
	kal_int32 format_4 = 0;

	if(ret==0)
	{
		return 0;
	}

	format_1 = (g_degc_cali / 2);
	format_2 = (y_curr - g_oe);
	format_3 = (((((format_2) * 10000) / 4096) * 10000) / g_gain) - g_x_roomt1;
	format_3 = format_3 * 15/18;

	//format_4 = ((format_3 * 100) / 139); // uint = 0.1 deg
	if(g_o_slope_sign==0)
	{
		//format_4 = ((format_3 * 100) / (139+g_o_slope)); // uint = 0.1 deg
		format_4 = ((format_3 * 100) / (165+g_o_slope)); // uint = 0.1 deg
	}
	else
	{
		//format_4 = ((format_3 * 100) / (139-g_o_slope)); // uint = 0.1 deg
		format_4 = ((format_3 * 100) / (165-g_o_slope)); // uint = 0.1 deg
	}
	format_4 = format_4 - (2 * format_4);

	t_current = (format_1 * 10) + format_4; // uint = 0.1 deg

	return t_current;
}
*/

static kal_int32 temperature_to_raw_abb(kal_uint32 ret)
{
	// Ycurr = [(Tcurr - DEGC_cali/2)*(165+O_slope)*(18/15)*(1/10000)+X_roomtabb]*Gain*4096 + OE

	kal_int32 t_curr = ret;
//	kal_int32 y_curr = 0;
	kal_int32 format_1 = 0;
	kal_int32 format_2 = 0;
	kal_int32 format_3 = 0;
	kal_int32 format_4 = 0;

	if(g_o_slope_sign==0)
	{
		//format_1 = t_curr-(g_degc_cali/2)*1000;
		format_1 = t_curr-(g_degc_cali*1000/2);
		format_2 = format_1 * (165+g_o_slope) * 18/15;
		format_2 = format_2 - 2*format_2;
		format_3 = format_2/1000 + g_x_roomtabb*10;
		format_4 = (format_3*4096/10000*g_gain)/100000 + g_oe;
		mtktscpu_dprintk("[Temperature_to_raw_abb] format_1=%d, format_2=%d, format_3=%d, format_4=%d\n", format_1, format_2, format_3, format_4);
	}
	else
	{
		//format_1 = t_curr-(g_degc_cali/2)*1000;
		format_1 = t_curr-(g_degc_cali*1000/2);
		format_2 = format_1 * (165-g_o_slope) * 18/15;
		format_2 = format_2 - 2*format_2;
		format_3 = format_2/1000 + g_x_roomtabb*10;
		format_4 = (format_3*4096/10000*g_gain)/100000 + g_oe;
		mtktscpu_dprintk("[temperature_to_raw_abb] format_1=%d, format_2=%d, format_3=%d, format_4=%d\n", format_1, format_2, format_3, format_4);
	}

	mtktscpu_dprintk("[temperature_to_raw_abb] temperature=%d, raw=%d", ret, format_4);
	return format_4;
}


static kal_int32 raw_to_temperature_abb(kal_uint32 ret)
{
	kal_int32 t_current = 0;
	kal_int32 y_curr = ret;
	kal_int32 format_1 = 0;
	kal_int32 format_2 = 0;
	kal_int32 format_3 = 0;
	kal_int32 format_4 = 0;

	if(ret==0)
	{
		return 0;
	}

	//format_1 = (g_degc_cali / 2);
	format_1 = (g_degc_cali*10 / 2);
	format_2 = (y_curr - g_oe);
	format_3 = (((((format_2) * 10000) / 4096) * 10000) / g_gain) - g_x_roomtabb;
	format_3 = format_3 * 15/18;

	//format_4 = ((format_3 * 100) / 139); // uint = 0.1 deg
	if(g_o_slope_sign==0)
	{
		//format_4 = ((format_3 * 100) / (139+g_o_slope)); // uint = 0.1 deg
		format_4 = ((format_3 * 100) / (165+g_o_slope)); // uint = 0.1 deg
	}
	else
	{
		//format_4 = ((format_3 * 100) / (139-g_o_slope)); // uint = 0.1 deg
		format_4 = ((format_3 * 100) / (165-g_o_slope)); // uint = 0.1 deg
	}
	format_4 = format_4 - (2 * format_4);

	//t_current = (format_1 * 10) + format_4; // uint = 0.1 deg
	t_current = format_1 + format_4; // uint = 0.1 deg

	return t_current;
}
/*
static int thermal_auxadc_get_data(int times, int Channel)
{
	int ret = 0, data[4], i, ret_value = 0, ret_temp = 0;

	if( IMM_IsAdcInitReady() == 0 )
	{
        mtktscpu_dprintk("[thermal_auxadc_get_data]: AUXADC is not ready\n");
		return 0;
	}

	i = times;
	while (i--)
	{
		ret_value = IMM_GetOneChannelValue(Channel, data, &ret_temp);
		ret += ret_temp;
		mtktscpu_dprintk("[thermal_auxadc_get_data(ADCIN5)]: ret_temp=%d\n",ret_temp);
	}

	ret = ret / times;
	return ret;
}
*/
static void thermal_calibration(void)
{
	if (g_adc_cali_en == 0)
		printk("Not Calibration\n");
	thermal_cal_prepare_2(0);
}

static DEFINE_MUTEX(TS_lock);
static int MA_counter=0, MA_first_time=0;
/*
static int mtktscpu_get_hw_temp(void)
{
	int ret=0, len=0;
	int t_ret1=0;
	static int abb[60]={0};
	int	i=0;

	mutex_lock(&TS_lock);

	if(proc_write_flag==1)
	{
		MA_counter=0, MA_first_time=0;
		MA_len=MA_len_temp;
		proc_write_flag=0;
		mtktscpu_dprintk("[mtktscpu_get_hw_temp]:MA_len=%d",MA_len);
	}

	//get HW Abb temp(TS_abb)
	mt65xx_reg_sync_writel(TS_CON0, 0x40);	//abb
	msleep(1);
	ret = thermal_auxadc_get_data(y_curr_repeat_times, 11);
	mtktscpu_dprintk("[mtktsabb_get_hw_temp]: TSABB average %d times channel 11 (0x%x,0x%x) = %d\n",
					y_curr_repeat_times, DRV_Reg32(TS_CON0), DRV_Reg32(TS_CON1), ret);

	t_ret1 = raw_to_temperature_abb(ret);
	t_ret1 = t_ret1 * 100;
	abb[MA_counter] = t_ret1;
	mtktscpu_dprintk("[mtktscpu_get_hw_temp] T_ABB, %d\n", t_ret1);


	if(MA_counter==0 && MA_first_time==0 && MA_len!=1)
	{
		MA_counter++;

		//get HW Abb temp(TS_abb)
		mt65xx_reg_sync_writel(TS_CON0, 0x40);	//abb
		msleep(1);
		ret = thermal_auxadc_get_data(y_curr_repeat_times, 11);
		mtktscpu_dprintk("[mtktsabb_get_hw_temp]: TSABB average %d times channel 11 (0x%x,0x%x) = %d\n",
					y_curr_repeat_times, DRV_Reg32(TS_CON0), DRV_Reg32(TS_CON1), ret);

		t_ret1 = raw_to_temperature_abb(ret);
		t_ret1 = t_ret1 * 100;
		abb[MA_counter] = t_ret1;
		mtktscpu_dprintk("[mtktscpu_get_hw_temp] T_ABB, %d\n", t_ret1);

	}
	mt65xx_reg_sync_writel(TS_CON0, DRV_Reg32(TS_CON0) | 0x000000C0); // turn off the sensor buffer to save power


	MA_counter++;
	if(MA_first_time==0)
		len = MA_counter;
	else
		len = MA_len;

	t_ret1 = 0;
	for(i=0; i<len; i++)
	{
		t_ret1 += abb[i];
	}
	last_abb_t = t_ret1 / len;

	mtktscpu_dprintk("[mtktscpu_get_hw_temp] MA_ABB=%d, \n", last_abb_t);
	mtktscpu_dprintk("[mtktscpu_get_hw_temp] MA_counter=%d, MA_first_time=%d, MA_len=%d\n", MA_counter, MA_first_time, MA_len);

	if(MA_counter==MA_len )
	{
		MA_counter=0;
		MA_first_time=1;
	}

	mutex_unlock(&TS_lock);

	return last_abb_t;
}
*/

static int get_immediate_temp(void)
{
	int curr_raw, curr_temp;

	curr_raw = DRV_Reg32(TEMPMSR0);
	curr_raw = curr_raw & 0x0fff;
	curr_temp = raw_to_temperature_abb(curr_raw);
	curr_temp = curr_temp*100;

	mtktscpu_dprintk("[get_immediate_temp] temp=%d, raw=%d\n", curr_temp, curr_raw);
	return curr_temp;
}

static int mtktscpu_get_TC_temp(void)
{
//	int curr_raw, curr_temp;
	int /*ret=0,*/ len=0, i=0;
	int t_ret1=0;
	static int abb[60]={0};

	mutex_lock(&TS_lock);

	if(proc_write_flag==1)
	{
		MA_counter=0, MA_first_time=0;
		MA_len=MA_len_temp;
		proc_write_flag=0;
		mtktscpu_dprintk("[mtktscpu_get_hw_temp]:MA_len=%d",MA_len);
	}

	t_ret1 = get_immediate_temp();
	abb[MA_counter] = t_ret1;
//	mtktscpu_dprintk("[mtktscpu_get_temp] temp=%d, raw=%d\n", t_ret1, curr_raw);

	if(MA_counter==0 && MA_first_time==0 && MA_len!=1)
	{
		MA_counter++;

		t_ret1 = get_immediate_temp();
		abb[MA_counter] = t_ret1;
//		mtktscpu_dprintk("[mtktscpu_get_temp] temp=%d, raw=%d\n", t_ret1, curr_raw);
	}
	MA_counter++;
	if(MA_first_time==0)
		len = MA_counter;
	else
		len = MA_len;

	t_ret1 = 0;
	for(i=0; i<len; i++)
	{
		t_ret1 += abb[i];
	}
	last_abb_t = t_ret1 / len;

	mtktscpu_dprintk("[mtktscpu_get_hw_temp] MA_ABB=%d, \n", last_abb_t);
	mtktscpu_dprintk("[mtktscpu_get_hw_temp] MA_counter=%d, MA_first_time=%d, MA_len=%d\n", MA_counter, MA_first_time, MA_len);

	if(MA_counter==MA_len )
	{
		MA_counter=0;
		MA_first_time=1;
	}

	mutex_unlock(&TS_lock);
	return last_abb_t;
}

static int mtktscpu_get_temp(struct thermal_zone_device *thermal,
							unsigned long *t)
{
	int curr_temp;

	curr_temp = mtktscpu_get_TC_temp();

	if((curr_temp>100000) | (curr_temp<-30000))
		printk("[Power/CPU_Thermal] CPU T=%d\n",curr_temp);

	//curr_temp = mtktscpu_get_hw_temp();
	*t = (unsigned long) curr_temp;
	return 0;
}

int mtktscpu_get_cpu_temp(void)
{
	int curr_temp;

	curr_temp = mtktscpu_get_TC_temp();

	if((curr_temp>100000) | (curr_temp<-30000))
		printk("[Power/CPU_Thermal] CPU T=%d\n",curr_temp);


	return ((unsigned long) curr_temp);
}


static int mtktscpu_bind(struct thermal_zone_device *thermal,
						struct thermal_cooling_device *cdev)
{
	int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		set_thermal_ctrl_trigger_SPM(trip_temp[0]);
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktscpu_dprintk("[mtktscpu_bind] %s\n", cdev->type);
	}
	else
	{
		return 0;
	}

	if (mtk_thermal_zone_bind_cooling_device(thermal, table_val, cdev)) {
		mtktscpu_dprintk("[mtktscpu_bind] error binding cooling dev\n");
		return -EINVAL;
	} else {
		mtktscpu_dprintk("[mtktscpu_bind] binding OK, %d\n", table_val);
	}

	return 0;
}

static int mtktscpu_unbind(struct thermal_zone_device *thermal,
						struct thermal_cooling_device *cdev)
{
	int table_val=0;

	if(!strcmp(cdev->type, g_bind0))
	{
		table_val = 0;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind1))
	{
		table_val = 1;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind2))
	{
		table_val = 2;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind3))
	{
		table_val = 3;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind4))
	{
		table_val = 4;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind5))
	{
		table_val = 5;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind6))
	{
		table_val = 6;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind7))
	{
		table_val = 7;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind8))
	{
		table_val = 8;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else if(!strcmp(cdev->type, g_bind9))
	{
		table_val = 9;
		mtktscpu_dprintk("[mtktscpu_unbind] %s\n", cdev->type);
	}
	else
		return 0;


	if (thermal_zone_unbind_cooling_device(thermal, table_val, cdev)) {
		mtktscpu_dprintk("[mtktscpu_unbind] error unbinding cooling dev\n");
	return -EINVAL;
	} else {
		mtktscpu_dprintk("[mtktscpu_unbind] unbinding OK\n");
	}

	return 0;
}

static int mtktscpu_get_mode(struct thermal_zone_device *thermal,
							enum thermal_device_mode *mode)
{
	*mode = (kernelmode) ? THERMAL_DEVICE_ENABLED
		: THERMAL_DEVICE_DISABLED;
	return 0;
}

static int mtktscpu_set_mode(struct thermal_zone_device *thermal,
							enum thermal_device_mode mode)
{
	kernelmode = mode;
	return 0;
}

static int mtktscpu_get_trip_type(struct thermal_zone_device *thermal, int trip,
								enum thermal_trip_type *type)
{
	*type = g_THERMAL_TRIP[trip];
	return 0;
}

static int mtktscpu_get_trip_temp(struct thermal_zone_device *thermal, int trip,
								unsigned long *temp)
{
	*temp = trip_temp[trip];
	return 0;
}

static int mtktscpu_get_crit_temp(struct thermal_zone_device *thermal,
								unsigned long *temperature)
{
	*temperature = MTKTSCPU_TEMP_CRIT;
	return 0;
}

/* bind callback functions to thermalzone */
static struct thermal_zone_device_ops mtktscpu_dev_ops = {
	.bind = mtktscpu_bind,
	.unbind = mtktscpu_unbind,
	.get_temp = mtktscpu_get_temp,
	.get_mode = mtktscpu_get_mode,
	.set_mode = mtktscpu_set_mode,
	.get_trip_type = mtktscpu_get_trip_type,
	.get_trip_temp = mtktscpu_get_trip_temp,
	.get_crit_temp = mtktscpu_get_crit_temp,
};


static int previous_step=-1;
/*
static int step0_mask[11] = {1,1,1,1,1,1,1,1,1,1,1};
static int step1_mask[11] = {0,1,1,1,1,1,1,1,1,1,1};
static int step2_mask[11] = {0,0,1,1,1,1,1,1,1,1,1};
static int step3_mask[11] = {0,0,0,1,1,1,1,1,1,1,1};
static int step4_mask[11] = {0,0,0,0,1,1,1,1,1,1,1};
static int step5_mask[11] = {0,0,0,0,0,1,1,1,1,1,1};
static int step6_mask[11] = {0,0,0,0,0,0,1,1,1,1,1};
static int step7_mask[11] = {0,0,0,0,0,0,0,1,1,1,1};
static int step8_mask[11] = {0,0,0,0,0,0,0,0,1,1,1};
static int step9_mask[11] = {0,0,0,0,0,0,0,0,0,1,1};
static int step10_mask[11]= {0,0,0,0,0,0,0,0,0,0,1};
*/
static int _mtktscpu_set_power_consumption_state(void)
{
	int i=0;
	int power=0;
	for(i=0; i<Num_of_OPP; i++)
	{
		if(1==cl_dev_state[i])
		{
			if(i!=previous_step)
			{
				xlog_printk(ANDROID_LOG_INFO, "Power/CPU_Thermal", "previous_opp=%d, now_opp=%d\n", previous_step, i);
				previous_step=i;
				if(Num_of_GPU_OPP>2)
				{
					power = (i*100+700) - mtk_gpu_power[Num_of_GPU_OPP-2].gpufreq_power;
					mt_cpufreq_thermal_protect(power);
					mt_gpufreq_thermal_protect(mtk_gpu_power[Num_of_GPU_OPP-2].gpufreq_power);
				}
				else if(Num_of_GPU_OPP==2)
				{
					power = (i*100+700) - mtk_gpu_power[1].gpufreq_power;
					mt_cpufreq_thermal_protect(power);
					mt_gpufreq_thermal_protect(mtk_gpu_power[1].gpufreq_power);
				}
				else if(Num_of_GPU_OPP==1)
				{
					power = (i*100+700) - mtk_gpu_power[0].gpufreq_power;
					//power = (i*100+700) - GPU_Default_POWER;
					mt_cpufreq_thermal_protect(power);
				}
			}
			break;
		}
	}

	if(i==Num_of_OPP)
	{
		if(previous_step!=-1)
		{
			previous_step = -1;
			xlog_printk(ANDROID_LOG_INFO, "Power/CPU_Thermal", "Free all thermal limit, previous_opp=%d\n", previous_step);
			mt_cpufreq_thermal_protect(0);
			mt_gpufreq_thermal_protect(0);
		}
	}
	return 0;
}


static int cpufreq_F0x2_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	mtktscpu_dprintk("cpufreq_F0x2_get_max_state\n");
	*state = 1;
	return 0;
}
static int cpufreq_F0x2_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	int i=0;
	mtktscpu_dprintk("get_cur_state, %s\n", cdev->type);

	for(i=0; i<Num_of_OPP; i++)
	{
		if(!strcmp(cdev->type, &cooler_name[i*20]))
		{
			*state = cl_dev_state[i];
			mtktscpu_dprintk("get_cur_state: cl_dev_state[%d]=%d\n",i, cl_dev_state[i]);
		}
	}
	return 0;
}
static int cpufreq_F0x2_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	int i=0;
	mtktscpu_dprintk("set_cur_state, %s\n", cdev->type);

	for(i=0; i<Num_of_OPP; i++)
	{
		if(!strcmp(cdev->type, &cooler_name[i*20]))
		{
			cl_dev_state[i]=state;
			mtktscpu_dprintk("set_cur_state: cl_dev_state[%d]=%d\n",i, cl_dev_state[i]);
			_mtktscpu_set_power_consumption_state();
			break;
		}
	}
	return 0;
}



/*
 * cooling device callback functions (mtktscpu_cooling_sysrst_ops)
 * 1 : ON and 0 : OFF
 */
static int sysrst_get_max_state(struct thermal_cooling_device *cdev,
				unsigned long *state)
{
	mtktscpu_dprintk("sysrst_get_max_state\n");
	*state = 1;
	return 0;
}
static int sysrst_get_cur_state(struct thermal_cooling_device *cdev,
				unsigned long *state)
{
	mtktscpu_dprintk("sysrst_get_cur_state\n");
	*state = cl_dev_sysrst_state;
	return 0;
}
static int sysrst_set_cur_state(struct thermal_cooling_device *cdev,
				unsigned long state)
{
	mtktscpu_dprintk("sysrst_set_cur_state\n");
	cl_dev_sysrst_state = state;
	if(cl_dev_sysrst_state == 1)
	{
		printk("Power/CPU_Thermal: reset, reset, reset!!!");
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");
		printk("*****************************************");
		printk("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@");

		BUG();
		//arch_reset(0,NULL);
	}
	return 0;
}

/* bind fan callbacks to fan device */

static struct thermal_cooling_device_ops mtktscpu_cooling_F0x2_ops = {
	.get_max_state = cpufreq_F0x2_get_max_state,
	.get_cur_state = cpufreq_F0x2_get_cur_state,
	.set_cur_state = cpufreq_F0x2_set_cur_state,
};

static struct thermal_cooling_device_ops mtktscpu_cooling_sysrst_ops = {
	.get_max_state = sysrst_get_max_state,
	.get_cur_state = sysrst_get_cur_state,
	.set_cur_state = sysrst_set_cur_state,
};

static int mtktscpu_read_opp(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;

	int i=0;
	for(i=0; i<Num_of_OPP; i++)
	{
		if(1==cl_dev_state[i])
		{
			p += sprintf(p, "%s\n", &cooler_name[i*20]);
			break;
		}
	}

	// CTFang 2012/10/26: if no limit, still return a string to indicate there is no limit.
	if (i == Num_of_OPP)
	{
		p += sprintf(p, "no_limit\n");
	}

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;
}

static int mtktscpu_read_cal(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;

	p += sprintf(p, "mtktscpu cal:\nReg(0xF0009100)=0x%x, Reg(0xF0009104)=0x%x, Reg(0xF0009108)=0x%x\n",
	                DRV_Reg32(0xF0009100), DRV_Reg32(0xF0009104), DRV_Reg32(0xF0009108));

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;
}

static int mtktscpu_read_log(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;

	p += sprintf(p, "[ mtktscpu_read_log] log = %d\n",mtktscpu_debug_log);

	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;
}

static int mtktscpu_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
	int len = 0;
	char *p = buf;

	p += sprintf(p, "[ mtktscpu_read] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\n\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,\n\
g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\n\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n\
cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\n\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s,time_ms=%d\n",
				trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
				trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],
				g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
				g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9],
				g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9,
				interval*1000);


	*start = buf + off;

	len = p - buf;
	if (len > off)
		len -= off;
	else
		len = 0;

	return len < count ? len  : count;
}

static ssize_t mtktscpu_write_log(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char desc[32];
	int log_switch;
	int len = 0;

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d", &log_switch) == 1)
	{
		mtktscpu_debug_log = log_switch;
		mtktscpu_dprintk("[mtktscpu_write_log] mtktscpu_debug_log=%d\n", mtktscpu_debug_log);
		return count;
	}
	else
	{
		mtktscpu_dprintk("[mtktscpu_write_log] bad argument\n");
	}
	return -EINVAL;
}


int mtktscpu_register_thermal(void);
void mtktscpu_unregister_thermal(void);

static ssize_t mtktscpu_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len=0,time_msec=0;
	int trip[10]={0};
	int t_type[10]={0};
	int i;
	char bind0[20],bind1[20],bind2[20],bind3[20],bind4[20];
	char bind5[20],bind6[20],bind7[20],bind8[20],bind9[20];
	char desc[512];
//	int curr_temp;

//	return 0; //test

	len = (count < (sizeof(desc) - 1)) ? count : (sizeof(desc) - 1);
	if (copy_from_user(desc, buffer, len))
	{
		return 0;
	}
	desc[len] = '\0';

	if (sscanf(desc, "%d %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d %s %d %d",
				&num_trip, &trip[0],&t_type[0],bind0, &trip[1],&t_type[1],bind1,
				&trip[2],&t_type[2],bind2, &trip[3],&t_type[3],bind3,
				&trip[4],&t_type[4],bind4, &trip[5],&t_type[5],bind5,
				&trip[6],&t_type[6],bind6, &trip[7],&t_type[7],bind7,
				&trip[8],&t_type[8],bind8, &trip[9],&t_type[9],bind9,
				&time_msec, &MA_len_temp) == 33)
	{

		mtktscpu_dprintk("[mtktscpu_write] mtktscpu_unregister_thermal\n");
		mtktscpu_unregister_thermal();

		for(i=0; i<num_trip; i++)
			g_THERMAL_TRIP[i] = t_type[i];

		g_bind0[0]=g_bind1[0]=g_bind2[0]=g_bind3[0]=g_bind4[0]=g_bind5[0]=g_bind6[0]=g_bind7[0]=g_bind8[0]=g_bind9[0]='\0';

		for(i=0; i<20; i++)
		{
			g_bind0[i]=bind0[i];
			g_bind1[i]=bind1[i];
			g_bind2[i]=bind2[i];
			g_bind3[i]=bind3[i];
			g_bind4[i]=bind4[i];
			g_bind5[i]=bind5[i];
			g_bind6[i]=bind6[i];
			g_bind7[i]=bind7[i];
			g_bind8[i]=bind8[i];
			g_bind9[i]=bind9[i];
		}

		mtktscpu_dprintk("[mtktscpu_write] g_THERMAL_TRIP_0=%d,g_THERMAL_TRIP_1=%d,g_THERMAL_TRIP_2=%d,g_THERMAL_TRIP_3=%d,g_THERMAL_TRIP_4=%d,\
g_THERMAL_TRIP_5=%d,g_THERMAL_TRIP_6=%d,g_THERMAL_TRIP_7=%d,g_THERMAL_TRIP_8=%d,g_THERMAL_TRIP_9=%d,\n",
				g_THERMAL_TRIP[0],g_THERMAL_TRIP[1],g_THERMAL_TRIP[2],g_THERMAL_TRIP[3],g_THERMAL_TRIP[4],
				g_THERMAL_TRIP[5],g_THERMAL_TRIP[6],g_THERMAL_TRIP[7],g_THERMAL_TRIP[8],g_THERMAL_TRIP[9]);
		mtktscpu_dprintk("[mtktscpu_write] cooldev0=%s,cooldev1=%s,cooldev2=%s,cooldev3=%s,cooldev4=%s,\
cooldev5=%s,cooldev6=%s,cooldev7=%s,cooldev8=%s,cooldev9=%s\n",
				g_bind0,g_bind1,g_bind2,g_bind3,g_bind4,g_bind5,g_bind6,g_bind7,g_bind8,g_bind9);

		for(i=0; i<num_trip; i++)
		{
			trip_temp[i]=trip[i];
		}

		interval=time_msec / 1000;

		mtktscpu_dprintk("[mtktscpu_write] trip_0_temp=%d,trip_1_temp=%d,trip_2_temp=%d,trip_3_temp=%d,trip_4_temp=%d,\
trip_5_temp=%d,trip_6_temp=%d,trip_7_temp=%d,trip_8_temp=%d,trip_9_temp=%d,time_ms=%d, num_trip=%d\n",
				trip_temp[0],trip_temp[1],trip_temp[2],trip_temp[3],trip_temp[4],
				trip_temp[5],trip_temp[6],trip_temp[7],trip_temp[8],trip_temp[9],interval*1000, num_trip);


		//get temp, set high low threshold
/*		curr_temp = get_immediate_temp();
		for(i=0; i<num_trip; i++)
		{
			if(curr_temp>trip_temp[i])
				break;
		}
		if(i==0)
		{
			printk("Power/CPU_Thermal: [mtktscpu_write] setting error");
		}
		else if(i==num_trip)
			set_high_low_threshold(trip_temp[i-1], 10000);
		else
			set_high_low_threshold(trip_temp[i-1], trip_temp[i]);
*/
		mtktscpu_dprintk("[mtktscpu_write] mtktscpu_register_thermal\n");
		mtktscpu_register_thermal();
		proc_write_flag=1;

		return count;
	}
	else
	{
		mtktscpu_dprintk("[mtktscpu_write] bad argument\n");
	}

	return -EINVAL;
}

int mtktscpu_register_DVFS_hotplug_cooler(void)
{
	int i;

	mtktscpu_dprintk("[mtktscpu_register_DVFS_hotplug_cooler] \n");
	for(i=0; i<Num_of_OPP; i++)
	{
		cl_dev[i] = mtk_thermal_cooling_device_register(&cooler_name[i*20], NULL,
					 &mtktscpu_cooling_F0x2_ops);
	}
	cl_dev_sysrst = mtk_thermal_cooling_device_register("mtktscpu-sysrst", NULL,
					&mtktscpu_cooling_sysrst_ops);

	return 0;
}
int mtktscpu_register_thermal(void)
{
	mtktscpu_dprintk("[mtktscpu_register_thermal] \n");

	/* trips : trip 0~3 */
	thz_dev = mtk_thermal_zone_device_register("mtktscpu", num_trip, NULL,
				&mtktscpu_dev_ops, 0, 0, 0, interval*1000);
	return 0;
}

void mtktscpu_unregister_DVFS_hotplug_cooler(void)
{
	int i;
	for(i=0; i<Num_of_OPP; i++)
	{
		if(cl_dev[i])
		{
			mtk_thermal_cooling_device_unregister(cl_dev[i]);
			cl_dev[i] = NULL;
		}
	}
	if(cl_dev_sysrst) {
		mtk_thermal_cooling_device_unregister(cl_dev_sysrst);
		cl_dev_sysrst = NULL;
	}
}

void mtktscpu_unregister_thermal(void)
{
	mtktscpu_dprintk("[mtktscpu_unregister_thermal] \n");
	if(thz_dev) {
		mtk_thermal_zone_device_unregister(thz_dev);
		thz_dev = NULL;
	}
}

static int mtk_thermal_suspend(struct platform_device *dev, pm_message_t state)
{
	mtktscpu_dprintk("[mtk_thermal_suspend] \n");
	mt65xx_reg_sync_writel(DRV_Reg32(TS_CON0) | 0x000000C0, TS_CON0); // turn off the sensor buffer to save power

	return 0;
}

static int mtk_thermal_resume(struct platform_device *dev)
{
	mtktscpu_dprintk("[mtk_thermal_resume] \n");
	if(talking_flag==false)
	{
		thermal_reset_and_initial();
		set_thermal_ctrl_trigger_SPM(trip_temp[0]);
	}

	return 0;
}

static struct platform_driver mtk_thermal_driver = {
	.remove     = NULL,
	.shutdown   = NULL,
	.probe      = NULL,
	.suspend	= mtk_thermal_suspend,
	.resume		= mtk_thermal_resume,
	.driver     = {
		.name = THERMAL_NAME,
    },
};

static int __init mtktscpu_init(void)
{
	return 0;
}

//static int __init mtktscpu_init(void)
static int __init thermal_late_init(void)
{
	int err = 0;
	struct proc_dir_entry *entry = NULL;
	struct proc_dir_entry *mtktscpu_dir = NULL;
//	struct TS_PTPOD ts;

	mtktscpu_dprintk("[mtktscpu_init] \n");

	thermal_cal_prepare();
	thermal_calibration();

	mt65xx_reg_sync_writel(DRV_Reg32(TS_CON0) | 0x000000C0, TS_CON0); // turn off the sensor buffer to save power


	thermal_reset_and_initial();
//	set_high_low_threshold(20000, 10000);//test

	err = platform_driver_register(&mtk_thermal_driver);
	if (err)
		return err;

	err = init_cooler();
	if(err)
		return err;

	err = mtktscpu_register_DVFS_hotplug_cooler();
	if(err)
		return err;

	err = mtktscpu_register_thermal();
	if(err)
		goto err_unreg;

	err= request_irq(MT_PTP_THERM_IRQ_ID, thermal_interrupt_handler, IRQF_TRIGGER_LOW, THERMAL_NAME, NULL);
	if(err)
		mtktscpu_dprintk("[mtktscpu_init] IRQ register fail\n");

	mtktscpu_dir = proc_mkdir("mtktscpu", NULL);
	if (!mtktscpu_dir)
	{
		mtktscpu_dprintk("[mtktscpu_init]: mkdir /proc/mtktscpu failed\n");
	}
	else
	{
		entry = create_proc_entry("mtktscpu", S_IRUGO | S_IWUSR, mtktscpu_dir);
		if (entry)
		{
			entry->read_proc = mtktscpu_read;
			entry->write_proc = mtktscpu_write;
		}

		entry = create_proc_entry("mtktscpu_log", S_IRUGO | S_IWUSR, mtktscpu_dir);
		if (entry)
		{
			entry->read_proc = mtktscpu_read_log;
			entry->write_proc = mtktscpu_write_log;
		}

		entry = create_proc_entry("mtktscpu_opp", S_IRUGO, mtktscpu_dir);
		if (entry)
		{
			entry->read_proc = mtktscpu_read_opp;
			entry->write_proc = NULL;
		}

		entry = create_proc_entry("mtktscpu_cal", S_IRUGO, mtktscpu_dir);
		if (entry)
		{
			entry->read_proc = mtktscpu_read_cal;
			entry->write_proc = NULL;
		}
	}

//	get_thermal_slope_intercept(&ts);
//	printk("INIT: ts_MTS=%d, ts_BTS=%d \n", ts.ts_MTS, ts.ts_BTS);

	return 0;

err_unreg:
	mtktscpu_unregister_DVFS_hotplug_cooler();
	return err;
}

static void __exit mtktscpu_exit(void)
{
	mtktscpu_dprintk("[mtktscpu_exit] \n");
	mtktscpu_unregister_thermal();
	mtktscpu_unregister_DVFS_hotplug_cooler();
}

module_init(mtktscpu_init);
module_exit(mtktscpu_exit);

late_initcall(thermal_late_init);
