#ifndef CUST_MATV_H
#define CUST_MATV_H

#include "kal_release.h"
#include "matvctrl.h"

/*
MATV default country
- TV_AFGHANISTAN,
- TV_ARGENTINA,
- TV_AUSTRALIA,
- TV_BRAZIL,
- TV_BURMA,
- TV_CAMBODIA,
- TV_CANADA,
- TV_CHILE,
- TV_CHINA,
- TV_CHINA_HONGKONG,
- TV_CHINA_SHENZHEN,
- TV_EUROPE_EASTERN,
- TV_EUROPE_WESTERN,
- TV_FRANCE,
- TV_FRENCH_COLONIE,
- TV_INDIA,
- TV_INDONESIA,
- TV_IRAN,
- TV_ITALY,
- TV_JAPAN,
- TV_KOREA,
- TV_LAOS,
- TV_MALAYSIA,
- TV_MEXICO,
- TV_NEWZEALAND,
- TV_PAKISTAN,
- TV_PARAGUAY,
- TV_PHILIPPINES,
- TV_PORTUGAL,
- TV_RUSSIA,
- TV_SINGAPORE,
- TV_SOUTHAFRICA,
- TV_SPAIN,
- TV_TAIWAN,
- TV_THAILAND,
- TV_TURKEY,
- TV_UNITED_ARAB_EMIRATES,
- TV_UNITED_KINGDOM,
- TV_USA,
- TV_URUGUAY,
- TV_VENEZUELA,
- TV_VIETNAM,
- TV_IRELAND,
- TV_MOROCCO,
*/

#define ANALOG_AUDIO

#if 1
/*
 * MATV default channel number
 * (If value = 0, default country and channel select UI are used.)
 * (If value > 0, customized country and channel select UI are used.)
*/
#define MATV_TOATL_CH  0x06

//typedef struct 
//{
//	kal_uint32	freq; //khz
//	kal_uint8	sndsys;	/* reference sv_const.h, TV_AUD_SYS_T ...*/
//	kal_uint8	colsys;	/* reference sv_const.h, SV_CS_PAL_N, SV_CS_PAL,SV_CS_NTSC358...*/
//	kal_uint8	flag;
//} matv_ch_entry;
matv_ch_entry MATV_CH_TABLE[]=
{
    //China 4/5/10/12/44/47
    {77250, SV_PAL_DK_FMMONO, SV_CS_PAL , 1},
    {85250, SV_PAL_DK_FMMONO, SV_CS_PAL , 1},
    {200250, SV_PAL_DK_FMMONO, SV_CS_PAL , 1},
    {216250, SV_PAL_DK_FMMONO, SV_CS_PAL , 1},    
    {759250, SV_PAL_DK_FMMONO, SV_CS_PAL , 1},
    {783250, SV_PAL_DK_FMMONO, SV_CS_PAL , 1},
    {-1, NULL, NULL, NULL}
};
#else
/*
 * MATV default channel number
 * (If value = 0, default country and channel select UI are used.)
 * (If value > 0, customized country and channel select UI are used.)
*/
#define MATV_TOATL_CH  0x03

//typedef struct 
//{
//	kal_uint32	freq; //khz
//	kal_uint8	sndsys;	/* reference sv_const.h, TV_AUD_SYS_T ...*/
//	kal_uint8	colsys;	/* reference sv_const.h, SV_CS_PAL_N, SV_CS_PAL,SV_CS_NTSC358...*/
//	kal_uint8	flag;
//} matv_ch_entry;
matv_ch_entry MATV_CH_TABLE[]=
{
    //Taiwan Ch42/44/46
    {639250, SV_MTS, SV_CS_NTSC358 , 1},
    {651250, SV_MTS, SV_CS_NTSC358 , 1},
    {663250, SV_MTS, SV_CS_NTSC358 , 1},      
    {-1, NULL, NULL, NULL}
};
#endif
#endif /* CUST_FM_H */

