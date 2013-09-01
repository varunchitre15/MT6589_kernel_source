

#ifndef CUST_FTM_MATV_COMM_H
#define CUST_FTM_MATV_COMM_H


///#define DISABLE_AUTO_LOCK_FREQ_TEST
/// this define is used to disable lock freqency test
#define MATV_AUTO_TOATL_CH  0x03
typedef struct
{
	int freq; ///lcok freqency,  unit: khz
	int country; ///country id: the definition referes to matvctrl.h
}matv_autotest_ch_entry;

matv_autotest_ch_entry MATV_AUTOTEST_CH_TABLE[]= 
{
	{168250, 	TV_CHINA},
	{184250,		TV_CHINA},
	{680000,		TV_CHINA},
	{-1, 		NULL}
};



#endif /* CUST_FM_H */

