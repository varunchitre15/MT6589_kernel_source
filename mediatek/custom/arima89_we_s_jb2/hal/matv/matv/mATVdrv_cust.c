
/**
 *   @file mATVdrv_cust.c
 *		
 *   @author ky.lin 
 */
#if defined(__MTK_TARGET__) 

#include <fcntl.h>
#include "kal_release.h"
#include "hostlib.h"
#include "matvctrl.h"
///#include "cust_matv_comm.h"

enum {
	BG_LIKE=0,
	DK_LIKE,
	I_LIKE,
	L_LIKE,
	L1_LIKE,
	M_LIKE,
}; // assosiate  to abScanRoadMap
 
#define Freq2DR(freqkhz)		(UINT16)((freqkhz*2)/125) //(freqkhz*16)/1000
#define CUSTOM_AVC_ON 1

 
struct UniSystemChMap{
	UINT8	start_ch;
	UINT8	end_ch;
	UINT16  start_DR;
	UINT16  end_DR;
	UINT16  bandwidth_DR;
	UINT8 sndsys;
	UINT8 colsys;
};
 
#define UNICHMAP(start_ch,end_ch,start_freq,end_freq,bandwidth_freq) \
	 { start_ch, end_ch, Freq2DR(start_freq),Freq2DR(end_freq),Freq2DR(bandwidth_freq),0,0}
 
#define ADHOCCHMAP(start_ch,end_ch,start_freq,end_freq,bandwidth_freq,sndsys,colsys) \
	 { start_ch, end_ch, Freq2DR(start_freq),Freq2DR(end_freq),Freq2DR(bandwidth_freq),sndsys,colsys}
	 
struct chscan_method {
	const char * description;
	UINT8 BSystem;
	UINT8 SndSystem;
	UINT8 ColorSystem;
	UINT8 UniSystem;
	const struct UniSystemChMap * UniChTab; 
};

static int is_factory_boot(void)
{
	int fd;
	size_t s;
	char boot_mode;
	fd = open("/sys/class/BOOT/BOOT/boot/boot_mode", O_RDWR);
	if (fd < 0) {		  
		kal_prompt_trace(MOD_MATV, "fail to open: %s\n", "/sys/class/BOOT/BOOT/boot/boot_mode");		 
	 	return 0;	  
	}	  
	s = read(fd, (void *)&boot_mode, sizeof(boot_mode)); 
	close(fd);    
	if (s <= 0 || boot_mode != '4')		   
		return 0;	  
	kal_prompt_trace(MOD_MATV, "Factory Mode Booting.....\n");    
	return 1;
}


 /*****************************************************************************
 * FUNCTION
 *	matv_module_power_off
 * DESCRIPTION
 *	power off sequency
 * PARAMETERS
 *	 void
 * RETURNS
 *	
 *****************************************************************************/ 
void  matv_module_power_off(void)
{
}

 /*****************************************************************************
 * FUNCTION
 *matv_module_power_on
 * DESCRIPTION
 *	config power on seq
 * PARAMETERS
 * RETURNS
 * NONE
 *****************************************************************************/
void  matv_module_power_on(void)
{
	if(is_factory_boot())
	{
		kal_prompt_trace(MOD_MATV, "Factory Mode\n");	  
	}
	else
	{		
		kal_prompt_trace(MOD_MATV, "Normal Mode\n");	  
	}
	DrvSetChipDep(MTK_TVD_CamIFMode,0);
	DrvSetChipDep(MTK_TVD_CamIFRefMCLK,0);
#if CUSTOM_AVC_ON	
	DrvSetChipDep(MTK_AUD_AVC,0x30000);
#else
	DrvSetChipDep(MTK_AUD_AVC,0x0);
#endif	

#if 0
///ifdef CAMERA_IO_DRV_1800
  DrvSetChipDep(MTK_PAD_DRIVING,0x02);
  DrvSetChipDep(MTK_PAD_DRIVING,0x12);
  DrvSetChipDep(MTK_PAD_DRIVING,0x22);
  DrvSetChipDep(MTK_PAD_DRIVING,0x32);
  DrvSetChipDep(MTK_PAD_DRIVING,0x42);
  DrvSetChipDep(MTK_PAD_DRIVING,0x52);
  DrvSetChipDep(MTK_PAD_DRIVING,0x62);
  DrvSetChipDep(MTK_PAD_DRIVING,0x72);
  DrvSetChipDep(MTK_PAD_DRIVING,0x82);
  DrvSetChipDep(MTK_PAD_DRIVING,0x92);
  DrvSetChipDep(MTK_PAD_DRIVING,0xa2);
  DrvSetChipDep(MTK_PAD_DRIVING,0xb2);
#else
    kal_prompt_trace(MOD_MATV, "Factory Mode max\n");
    ///0x08  --> 0xb8
    DrvSetChipDep(MTK_PAD_DRIVING,0x0f);
	DrvSetChipDep(MTK_PAD_DRIVING,0x1f);
	DrvSetChipDep(MTK_PAD_DRIVING,0x2f);
	DrvSetChipDep(MTK_PAD_DRIVING,0x3f);
	DrvSetChipDep(MTK_PAD_DRIVING,0x4f);
	DrvSetChipDep(MTK_PAD_DRIVING,0x5f);
	DrvSetChipDep(MTK_PAD_DRIVING,0x6f);
	DrvSetChipDep(MTK_PAD_DRIVING,0x7f);
	DrvSetChipDep(MTK_PAD_DRIVING,0x8f);
	DrvSetChipDep(MTK_PAD_DRIVING,0x9f);
	DrvSetChipDep(MTK_PAD_DRIVING,0xaf);
	DrvSetChipDep(MTK_PAD_DRIVING,0xbf);

#endif
	
}

  /*****************************************************************************
 * FUNCTION
 *matv_fm_module_power_on
 * DESCRIPTION
 *	config power on seq
 * PARAMETERS
 * RETURNS
 * NONE
 *****************************************************************************/
void  matv_fm_module_power_on(void)
{
	if(is_factory_boot())
	{
		kal_prompt_trace(MOD_MATV, "Factory Mode\n");	  
	}
	else
	{		
		kal_prompt_trace(MOD_MATV, "Normal Mode\n");	  
	}

#if CUSTOM_AVC_ON	
	DrvSetChipDep(MTK_AUD_AVC,0x30000);
#else
	DrvSetChipDep(MTK_AUD_AVC,0x0);
#endif
}


const struct UniSystemChMap NA_AIR[]=
	{	//NA_AIR
		UNICHMAP(2,4,55250,67250,6000), //VHF
		UNICHMAP(5,6,77250,83250,6000), //VHF
		UNICHMAP(7,13,175250,211250,6000), //VHF
		UNICHMAP(14,69,471250,801250,6000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
const struct UniSystemChMap JP_AIR[]=
	{	//JP_AIR
		UNICHMAP(1,3,91250,103250,6000), //VHF
		UNICHMAP(4,7,171250,189250,6000), //VHF
		UNICHMAP(8,12,193250,217250,6000), //VHF
		UNICHMAP(13,62,471250,765250,6000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};

/* Russia & Eastern Europe*/
const struct UniSystemChMap RU_AIR[]=
	{	//RU_AIR
		UNICHMAP(1,2,49750,59250,9500), //VHF
		UNICHMAP(3,5,77250,93250,8000), //VHF
		UNICHMAP(6,12,175250,223250,8000), //VHF
		UNICHMAP(21,69,471250,855250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
	
/* Western Europe */
const struct UniSystemChMap WE_AIR[]=
	{	//WE_AIR	
		UNICHMAP(2,4,48250,62250,7000), //VHF
		UNICHMAP(5,12,175250,224250,7000), //VHF
		UNICHMAP(21,69,471250,855250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};

/* UK */
const struct UniSystemChMap UK_AIR[]=
	{	//UK_AIR
		UNICHMAP(2,4,46250,62250,8000), //VHF
		UNICHMAP(5,13,175250,239250,8000), //VHF
		UNICHMAP(21,69,471250,855250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* China */
const struct UniSystemChMap CN_AIR[]=
	{	//CN_AIR
		UNICHMAP(1,3,49750,65750,8000), //VHF
		UNICHMAP(4,5,77250,85250,8000), //VHF
		UNICHMAP(6,12,168250,216250,8000), //VHF
		UNICHMAP(13,24,471250,559250,8000), //UHF
		UNICHMAP(25,59,607250,879250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* Vietnam */
const struct UniSystemChMap VN_AIR[]=
	{	//VN_AIR
		UNICHMAP(2,3,49750,59750,10000), //VHF
		UNICHMAP(4,5,77250,85250,8000), //VHF
		UNICHMAP(6,12,175250,223250,8000), //VHF
		UNICHMAP(21,69,471250,855250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
#if 0 /* just use WE */
/* Indonesia */
const struct UniSystemChMap ID_AIR[]=
	{	//ID_AIR
		UNICHMAP(2,3,55250,62250,7000), //VHF
		UNICHMAP(4,11,175250,224250,7000), //VHF
		UNICHMAP(21,81,471250,951250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
#endif
/* Italy*/
const struct UniSystemChMap IT_AIR[]=
	{	//IT_AIR - 7MHz channel
		UNICHMAP(1,2,53750,62250,8500), //VHF -sparse
		UNICHMAP(3,4,82250,175250,93000), //VHF -sparse
		UNICHMAP(5,6,183750,192250,8500), //VHF -sparse
		UNICHMAP(7,8,201250,210250,9000), //VHF -sparse
		UNICHMAP(9,10,217250,224250,7000), //VHF -sparse
		UNICHMAP(21,62,471250,799250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/*Australia*/
const struct UniSystemChMap AU_AIR[]=
	{	//AU_AIR - 7MHz channel
		UNICHMAP(0,1,46250,57250,11000), //VHF -sparse
		UNICHMAP(2,3,64250,86250,22000), //VHF -sparse
		UNICHMAP(4,5,95250,102250,7000), //VHF -sparse
		UNICHMAP(6,6,138250,138250,7000), //VHF
		UNICHMAP(7,10,175250,196250,7000), //VHF
		UNICHMAP(11,13,209250,223250,7000), //VHF
		UNICHMAP(28,69,527250,814250,7000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* New Zealand */
const struct UniSystemChMap NZ_AIR[]=
	{	//NZ_AIR
		UNICHMAP(1,1,45250,45250,7000), //VHF
		UNICHMAP(2,3,55250,62250,7000), //VHF
		UNICHMAP(4,11,175250,224250,7000), //VHF
		UNICHMAP(28,69,527250,814250,7000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* Morocco */
const struct UniSystemChMap MA_AIR[]=
	{	//MA_AIR
		UNICHMAP(4,10,163250,211250,8000), //VHF
		UNICHMAP(21,69,471250,855250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* South Africa */
const struct UniSystemChMap ZA_AIR[]=
	{	//ZA_AIR
		//UNICHMAP(1,1,43250,43250,8000), //VHF
		UNICHMAP(2,4,46250,62250,8000), //VHF
		UNICHMAP(5,13,175250,239250,8000), //VHF
		UNICHMAP(21,69,471250,855250,8000), //UHF
		//UNICHMAP(21,81,471250,951250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* France */
const struct UniSystemChMap FR_AIR[]=
	{	//FR_AIR
		UNICHMAP(2,3,55750,60500,4750), //VHF
		UNICHMAP(4,4,63750,63750,8000), //VHF
		UNICHMAP(5,10,176000,216000,8000), //VHF
		UNICHMAP(21,69,471250,855250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* French oversea,French African colonies */
const struct UniSystemChMap FR2_AIR[]=
	{	//FR2_AIR
		UNICHMAP(2,3,52250,60250,8000), //VHF
		UNICHMAP(4,9,175250,215250,8000), //VHF
		UNICHMAP(21,69,471250,855250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* USA Cabe IRC */
const struct UniSystemChMap NA_IRC[]=
	{	//NA_IRC
//		UNICHMAP(1,1,73250,73250,6000), //VHF
		UNICHMAP(2,4,55250,67250,6000), //VHF
		UNICHMAP(5,6,77250,83250,6000), //VHF
		UNICHMAP(7,13,175250,211250,6000), //VHF
		UNICHMAP(14,22,121250,169250,6000), //VHF
		UNICHMAP(23,94,217250,643250,6000), //VHF,UHF
//		UNICHMAP(95,99,91250,115250,6000), //VHF
		UNICHMAP(100,158,649250,997250,6000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* USA Cabe HRC */
const struct UniSystemChMap NA_HRC[]=
	{	//NA_HRC
//		UNICHMAP(1,1,72000,72000,6000), //VHF
		UNICHMAP(2,4,54000,66000,6000), //VHF
		UNICHMAP(5,6,76000,82000,6000), //VHF
		UNICHMAP(7,13,174000,210000,6000), //VHF
		UNICHMAP(14,22,120000,168000,6000), //VHF
		UNICHMAP(23,94,216000,642000,6000), //VHF,UHF
//		UNICHMAP(95,99,90000,114000,6000), //VHF
		UNICHMAP(100,158,648000,996000,6000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* Ireland */
const struct UniSystemChMap IE_AIR[]=
	{	//IE_AIR
		UNICHMAP(1,3,45750,61750,8000), //VHF
		UNICHMAP(4,13,175250,247250,8000), //VHF
		UNICHMAP(21,69,471250,855250,8000), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
/* Lab signal */
const struct UniSystemChMap LAB_AIR[]=
	{	//IE_AIR
		ADHOCCHMAP(1,1,77250,77250,8000,SV_PAL_DK,SV_CS_PAL), //VHF DK
		ADHOCCHMAP(2,2,120000,120000,8000,SV_SECAM_L,SV_CS_SECAM), //VHF		
		ADHOCCHMAP(4,13,175250,247250,8000,SV_PAL_BG,SV_CS_SECAM), //VHF BG
		UNICHMAP(14,14,256250,256250,8000), //VHF BG
		UNICHMAP(15,15,448250,448250,8000), //VHF BG
		ADHOCCHMAP(16,16,495250,495250,7000,SV_PAL_I,SV_CS_PAL), //UHF
		ADHOCCHMAP(17,17,543250,543250,7000,SV_PAL_I,SV_CS_PAL), //UHF
		ADHOCCHMAP(18,18,808250,808250,7000,SV_PAL_I,SV_CS_PAL), //UHF
		UNICHMAP(0,0,0,0,0) //end of table
	};
const struct chscan_method ChCountryTab[]=
{
	{"West EU(PAL-BG)",BG_LIKE,SV_A2_BG,SV_CS_PAL,1,WE_AIR},
	{"West EU(PAL-DK)",DK_LIKE,SV_A2_DK1,SV_CS_PAL,1,WE_AIR},
	{"East EU(PAL-DK)",DK_LIKE,SV_A2_DK1,SV_CS_PAL,1,RU_AIR},	
	{"Russia(SECAM-DK)",DK_LIKE,SV_PAL_DK,SV_CS_SECAM,1,RU_AIR},
	{"UK(PAL-I)",I_LIKE,SV_PAL_I,SV_CS_PAL,1,UK_AIR},
	{"Italy(PAL-BG)",BG_LIKE,SV_A2_BG,SV_CS_PAL,1,IT_AIR},
	{"France(SECAM-L)",L_LIKE,SV_SECAM_L,SV_CS_SECAM,1,FR_AIR},
	{"FrenchOversea(SECAM-DK)",DK_LIKE,SV_SECAM_DK,SV_CS_SECAM,1,FR2_AIR},
	{"US(NTSC-M)",M_LIKE,SV_MTS,SV_CS_NTSC358,1,NA_AIR},
	{"Korea(NTSC-M)",M_LIKE,SV_MTS,SV_CS_NTSC358,1,NA_AIR},
	{"Japan(NTSC-M)",M_LIKE,SV_MTS,SV_CS_NTSC358,1,JP_AIR},
	{"China(PAL-DK)",DK_LIKE,SV_PAL_DK_FMMONO,SV_CS_PAL,1,CN_AIR},
	{"Brazil(PAL-M)",M_LIKE,SV_MTS,SV_CS_PAL_M,1,NA_AIR},
	{"Uruguay(PAL-N)",M_LIKE,SV_MTS,SV_CS_PAL_N,1,NA_AIR},
	{"Argentina(PAL-Nc)",M_LIKE,SV_MTS,SV_CS_PAL_N,1,NA_AIR},
	{"SouthernAfrica(PAL-I)",I_LIKE,SV_PAL_I,SV_CS_PAL,1,ZA_AIR},
	{"Australia(PAL-BG)",BG_LIKE,SV_A2_BG,SV_CS_PAL,1,AU_AIR},
	{"NewZealand(PAL-BG)",BG_LIKE,SV_PAL_BG,SV_CS_PAL,1,NZ_AIR},
	{"IRC(NTSC-M)",M_LIKE,SV_MTS,SV_CS_NTSC358,1,NA_IRC},
	{"HRC(NTSC-M)",M_LIKE,SV_MTS,SV_CS_NTSC358,1,NA_HRC},
	{"Vietnam(PAL-DK)",DK_LIKE,SV_A2_DK1,SV_CS_PAL,1,VN_AIR},
	{"Iran(SECAM-BG)",BG_LIKE,SV_A2_BG,SV_CS_SECAM,1,WE_AIR},
	{"Ireland(PAL-I)",I_LIKE,SV_PAL_I,SV_CS_PAL,1,IE_AIR},
	{"Morocco(SECAM-BG)",BG_LIKE,SV_A2_BG,SV_CS_SECAM,1,MA_AIR},
	{"SZ,HK(PAL-I)",I_LIKE,SV_PAL_I_FMMONO,SV_CS_PAL,1,UK_AIR},
	{"West EU(PAL-BG)(NICAM)",BG_LIKE,SV_PAL_BG,SV_CS_PAL,1,WE_AIR},
	{"Lab signal(PAL-BG)",BG_LIKE,SV_PAL_BG,SV_CS_PAL,1,LAB_AIR}
	//{"Germery(BG+L)",EU_LIKE,SV_A2_BG,SV_CS_PAL,0,WE_AIR},
	//{"GA",GA_LIKE,SV_A2_BG,SV_CS_AUTO,0,NA_AIR},
};
#define CHSCAN_METHODTAB_SIZE (sizeof(ChCountryTab)/sizeof(struct chscan_method))
UINT16 ChCountryTab_Size=CHSCAN_METHODTAB_SIZE;
struct UniSystemChMap * ChScanUniChTab=(struct UniSystemChMap *)WE_AIR;//ChCountryTab[CHSCAN_COUNTRY_DEFAULT].UniChTab;

const kal_uint8 countrymap[TV_COUNTRY_MAX]=
{
	0,14,16,12,8,
	0,8,8,11,24,
	24,2,0,6,7,
	0,25,21,5,10,
	9,0,0,8,17,
	0,13,8,0,3,
	25,15,25,8,0,
	0,0,4,8,13,
	8,20,22,23
};
/////////////////////////////////////////////////////////////////////////////
//TTS table
/////////////////////////////////////////////////////////////////////////////
typedef struct
{
	UINT32 SpurFreq;
	UINT16 CVBSNoiseEnter_EQOn;
	UINT16 CVBSNoiseBack_EQOn;
	UINT16 CVBSNoiseEnter_EQOff;
	UINT16 CVBSNoiseBack_EQOff;
} TTS_Script_T;

TTS_Script_T const TTS_Table[] =
{
//Common Table
{ 78000,	6000,	2000,	5000,	1000},
{182000,	6000,	2000,	5000,	1000},
{208000,	6000,	2000,	5000,	1000},
{520000,	6000,	2000,	5000,	1000},
{624000,	6000,	2000,	5000,	1000},
{676000,	6000,	2000,	5000,	1000},
{702000,	6000,	2000,	5000,	1000},
{728000,	6000,	2000,	5000,	1000},
{780000,	6000,	2000,	5000,	1000},
{832000,	6000,	2000,	5000,	1000},
{     0,	   0,	   0,      0,	   0}
};

#endif /* __MTK_TARGET__ */
