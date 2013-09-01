

#include "tpd.h"
#include "tpd_adc.h"

extern int tpd_sample_mode;

void tpd_adc_init(void);
u16 tpd_read(int position, int raw_data_offset) ;
u16 tpd_read_adc(u16 pos);


void tpd_set_debounce_time_DEBT0(int debounce_time) 
{
    __raw_writew(debounce_time<<5, AUXADC_TP_DEBT0);
}

void tpd_set_debounce_time_DEBT1(int debounce_time) 
{
    __raw_writew(debounce_time<<5, AUXADC_TP_DEBT1);
}

unsigned int tpd_get_debounce_time_DEBT0(void) 
{
    return (__raw_readw(AUXADC_TP_DEBT0)&0x3fff)>>5;
}

unsigned int tpd_get_debounce_time_DEBT1(void) 
{
    return (__raw_readw(AUXADC_TP_DEBT1)&0x3fff)>>5;
}

void tpd_set_spl_number(int spl_num) 
{
	if(spl_num > 0)
    	__raw_writew(spl_num|0x100, AUXADC_TP_CON2);// enable and set spl num.
}

unsigned int tpd_get_spl_number(void)
{
	return __raw_readw(AUXADC_TP_CON2)&0xff;
}

static u16 tpd_read_rawdata(unsigned int base, int raw_data_offset)
{
	unsigned value = 0;

	value = (__raw_readw(base + raw_data_offset*4)&0xfff);
	return value;
}

u16 tpd_read(int position, int raw_data_offset) 
{
	if(tpd_sample_mode&TPD_SW) {
	    switch(position) {
	        default:
	        case TPD_X:  return tpd_read_adc(TP_CMD_ADDR_X);
	        case TPD_Y:  return tpd_read_adc(TP_CMD_ADDR_Y);
	        case TPD_Z1: return tpd_read_adc(TP_CMD_ADDR_Z1);
	        case TPD_Z2: return tpd_read_adc(TP_CMD_ADDR_Z2);
	    } 
		return 0;
	}

	if(tpd_sample_mode&FAV_MODE_HW || tpd_sample_mode&FAV_MODE_SW) {
		switch(position) {
			default:
			case TPD_X:  return __raw_readw(AUXADC_TP_DATA0)&0xffff;
			case TPD_Y:  return __raw_readw(AUXADC_TP_DATA1)&0xffff;
			case TPD_Z1: return __raw_readw(AUXADC_TP_DATA2)&0xffff;
			case TPD_Z2: return __raw_readw(AUXADC_TP_DATA3)&0xffff;
		} 
		return 0;
	}

	if(tpd_sample_mode&RAW_DATA_MODE) {
		switch(position) {
			default:
			case TPD_X:  return tpd_read_rawdata(AUXADC_TP_RAW_X_DAT0, raw_data_offset);
			case TPD_Y:  return tpd_read_rawdata(AUXADC_TP_RAW_Y_DAT0, raw_data_offset);
			case TPD_Z1: return tpd_read_rawdata(AUXADC_TP_RAW_Z1_DAT0, raw_data_offset);
			case TPD_Z2: return tpd_read_rawdata(AUXADC_TP_RAW_Z2_DAT0, raw_data_offset);
		} 
		return 0;
	}
	return -1;
}

/* pass command, return sampled data */
u16 tpd_read_adc(u16 pos) {
   __raw_writew(pos, AUXADC_TP_ADDR);
   __raw_writew(TP_CON_SPL_TRIGGER, AUXADC_TP_CON0);
   while(TP_CON_SPL_MASK & __raw_readw(AUXADC_TP_CON0)) { ; } //wait for write finish
   return __raw_readw(AUXADC_TP_DATA0); 
}

u16 tpd_read_status(void) 
{
    return __raw_readw(AUXADC_TP_CON0) & 2;
}

void tpd_fav_switch(int on_off)
{
	if(tpd_sample_mode&FAV_MODE_HW) {
		unsigned int value =  __raw_readw(AUXADC_TP_CON1);

		if(on_off == 1)//enable
		{
			value |= ((1<<FAV_SEL)); // hw auto-trigger
		}
		else // disable
		{
			/* disable auto trigger */
			value &= (~(1<<FAV_SEL));	
		}
		__raw_writew(value, AUXADC_TP_CON1);
	}

	if(tpd_sample_mode&FAV_MODE_SW) {
		unsigned int value =  __raw_readw(AUXADC_TP_CON1);

		if(on_off == 1)//enable
		{
			value |= (1 <<FAV_EN_BIT);
		}
		else // disable
		{
			/* disable auto trigger */
			value &= (~(1<<FAV_SEL));	
		}
		__raw_writew(value, AUXADC_TP_CON1);
	}

	if(tpd_sample_mode&RAW_DATA_MODE) {
		unsigned int value =  __raw_readw(AUXADC_TP_RAW_CON);

		if(on_off == 1)
		{	
			value |= (1<<2);
		}
		else
		{
			value &= (~(1<<2)); 
		}
		__raw_writew(value, AUXADC_TP_RAW_CON);
	}		
}

/* only for sw workaround, 6577E1 can NOT clear invalid flag in auto-timer tigger mode */
void tpd_clear_invalid_flag(void)
{
	unsigned int value =  __raw_readw(AUXADC_TP_CON1);

	value |= (1 << FAV_EN_BIT);

	__raw_writew(value, AUXADC_TP_CON1);
}

void tpd_fav_config(int coord, int cnt, int asamp, int adel)
{
	unsigned int value = 0;
	if(cnt == 1)
		cnt = 0x0;
	else if(cnt == 4)
		cnt = 0x1;
	else if(cnt == 8)
		cnt = 0x2;
	else if(cnt == 16)
		cnt = 0x3;
	else
		cnt = 0x1; //default 8 times
		
	//unsigned int value = 0x2|(1<<3)|(0x1<<5)|(0x1<<8);	
	value = cnt|(asamp<<FAV_ASAMP)|(coord<<FAV_COORDSEL)|(adel<<FAV_ADEL_BIT);
	__raw_writew(value, AUXADC_TP_CON1);
}

unsigned int tpd_get_sample_cnt(void)
{
	unsigned int value = (__raw_readw(AUXADC_TP_CON1)&0x3);
	
	if(value == 0)
		return 1;
	else
		return (2*(1<<value));
}

unsigned int tpd_get_asamp(void)
{
	unsigned int value = (__raw_readw(AUXADC_TP_CON1)&(0x1<<FAV_ASAMP));
	
	return value>>FAV_ASAMP;
}


void tpd_fav_set_auto_interval(unsigned int ms)
{
	int value = (0xa0/5)*ms;
	__raw_writew(value, AUXADC_TP_AUTO_TIME_INTVL);
}
unsigned int tpd_fav_get_auto_interval(void)
{
	unsigned int value = __raw_readw(AUXADC_TP_AUTO_TIME_INTVL);;
	return value/(0xa0/5);
}


void tpd_adc_init(void) 
{
	unsigned int value = __raw_readw(AUXADC_MISC);
	
    __raw_writew(TP_SAMPLE_SETTING, AUXADC_TP_CMD);
  	__raw_writew(1*32, AUXADC_TP_DEBT0); //when setting >2*32. Rtouch can not work with thermal sensor
  	__raw_writew(TP_DEBOUNCE_TIME, AUXADC_TP_DEBT1);
  	__raw_writew(value|(1<<15), AUXADC_MISC); // non-stop
    /* COORSEL: b1,x-y-z1-z2; LCNT: b11, 16 times; ASAMP: b1,enable; ADEL: 255 */
  	tpd_fav_config(1, 16, 1, 255);
  	tpd_fav_set_auto_interval(10);// 10ms
  	//__raw_writew(TP_CMD_ADDR_X, AUXADC_TP_ADDR);// for test
}


