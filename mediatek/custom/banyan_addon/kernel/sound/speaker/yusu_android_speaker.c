/*******************************************************************************
 *
 * Filename:
 * ---------
 * Yusu_android_speaker.c
 *
 * Project:
 * --------
 *   Yusu
 *
 * Description:
 * ------------
 *   seaker setting
 *
 * Author:
 * -------
 *   ChiPeng Chang (mtk02308)
 *
 *
 *------------------------------------------------------------------------------
 * $Revision$
 * $Modtime:$
 * $Log:$
 *
 * 12 14 2011 weiguo.li
 * [ALPS00102848] [Need Patch] [Volunteer Patch] build waring in yusu_android_speaker.h
 * .
 *
 * 11 10 2011 weiguo.li
 * [ALPS00091610] [Need Patch] [Volunteer Patch]chang yusu_android_speaker.c function name and modules use it
 * .
 *
 * 09 28 2011 weiguo.li
 * [ALPS00076254] [Need Patch] [Volunteer Patch]LGE audio driver using Voicebuffer for incall
 * .
 *
 * 07 08 2011 weiguo.li
 * [ALPS00059378] poring lge code to alps(audio)
 * .
 *
 * 07 03 2010 chipeng.chang
 * [ALPS00002838][Need Patch] [Volunteer Patch] for speech volume step 
 * modify for headset customization.
 *
 *******************************************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <mach/mt6516_gpio.h>
#include <linux/semaphore.h>
#include <asm/uaccess.h>
#include <mach/mt6516_typedefs.h>
#include <linux/pmic6326_sw.h>
#include <linux/delay.h>
#include <mach/mt6516_pll.h>
#include "yusu_android_speaker.h"

//#define CONFIG_DEBUG_MSG
#ifdef CONFIG_DEBUG_MSG
#define PRINTK(format, args...) printk( KERN_EMERG format,##args )
#else
#define PRINTK(format, args...)
#endif

static int Speaker_Volume=0;

extern void Yusu_Sound_AMP_Switch(BOOL enable);
extern void pmic_asw_bsel(asw_bsel_enum sel);
extern void pmic_spkr_enable(kal_bool enable);
extern void pmic_spkl_enable(kal_bool enable);
extern void pmic_spkl_vol(kal_uint8 val);
extern void pmic_spkr_vol(kal_uint8 val);

bool Speaker_Init(void)
{
    printk("Speaker Init Success");
    return true;
}

bool Speaker_DeInit(void)
{
	return false;
}

void Sound_SpeakerL_SetVolLevel(int level)
{
    int hw_gain ,i=0;
    int  HW_Value[] = {6, 9, 12, 15, 18, 21, 24, 27};
    PRINTK(" Sound_SpeakerL_SetVolLevel  level = %d\n",level);
    if(level > 27)
        PRINTK("Sound_Speaker_Setlevel with level undefined  = %d\n",level);

    hw_gain = HW_Value[7] - level;
    if(hw_gain < HW_Value[0])
	   hw_gain = HW_Value[0];
    for(i = 0 ; i < 7 ; i++){
        if( HW_Value[i] >= hw_gain )
            break;
    }
    pmic_spkl_vol(i);
    PRINTK("Sound_SpeakerL_SetVolLevel  pmic_spkl_vol[%d]\n",i);

}

void Sound_SpeakerR_SetVolLevel(int level)
{
    int hw_gain ,i=0;
    int  HW_Value[] = {6, 9, 12, 15, 18, 21, 24, 27};
    PRINTK(" Sound_SpeakerR_SetVolLevel  level = %d\n",level);
    if(level > 27)
        PRINTK("Sound_Speaker_Setlevel with level undefined  = %d\n",level);

    hw_gain = HW_Value[7] - level;
    if(hw_gain < HW_Value[0])
	   hw_gain = HW_Value[0];
    for(i = 0 ; i < 7 ; i++){
        if( HW_Value[i] >= hw_gain )
            break;
    }
    pmic_spkr_vol(i);
    PRINTK("Sound_SpeakerR_SetVolLevel  pmic_spkl_vol[%d]\n",i);
}

void Sound_Speaker_Turnon(int channel)
{
    PRINTK("Sound_Speaker_Turnon channel = %d\n",channel);

#ifdef CONFIG_MT6516_GEMINI_BOARD
    PRINTK("CONFIG_MT6516_GEMINI_BOARD YUSU_SET_SPEAKER set HI_Z\n");
    msleep(5);
    pmic_asw_bsel(HI_Z);
#endif
    PRINTK(" Sound_Speaker_Turnon  Speaker_Volume = %d\n",Speaker_Volume);

    if(channel == Channel_None){
        return;
    }
    else if (channel == Channel_Right){
        Sound_SpeakerR_SetVolLevel(Speaker_Volume);
        pmic_spkr_enable(true);
    }
    else if (channel == Channel_Left){
        Sound_SpeakerL_SetVolLevel(Speaker_Volume);
        pmic_spkl_enable(true);
    }
    else if (channel == Channel_Stereo){
        Sound_SpeakerL_SetVolLevel(Speaker_Volume);
        pmic_spkl_enable(true);
        Sound_SpeakerR_SetVolLevel(Speaker_Volume);
        pmic_spkr_enable(true);
    }
    else{
        PRINTK("Sound_Speaker_Turnon with no define channel = %d\n",channel);
    }
}

void Sound_Speaker_Turnoff(int channel)
{
    PRINTK("Sound_Speaker_Turnoff channel = %d\n",channel);
    if(channel == Channel_None){
        return;
    }
    else if (channel == Channel_Right){
        pmic_spkr_enable(false);
    }
    else if (channel == Channel_Left){
        pmic_spkl_enable(false);
    }
    else if (channel == Channel_Stereo){
        pmic_spkl_enable(false);
        pmic_spkr_enable(false);
    }
    else{
        PRINTK("Sound_Speaker_Turnoff with no define channel = %d\n",channel);
    }

#ifdef CONFIG_MT6516_GEMINI_BOARD
    msleep(5);
    pmic_asw_bsel(RECEIVER);
    PRINTK("CONFIG_MT6516_GEMINI_BOARD YUSU_SET_SPEAKER set RECEIVER\n");
#endif
}

void Sound_Speaker_SetVolLevel(int level)
{
    int hw_gain ,i=0;
    int  HW_Value[] = {6, 9, 12, 15, 18, 21, 24, 27};
    if(level > 27)
        PRINTK("Sound_Speaker_Setlevel with level undefined  = %d\n",level);

    hw_gain = HW_Value[7] - level;
    if(hw_gain < HW_Value[0])
	   hw_gain = HW_Value[0];
    for(i = 0 ; i < 7 ; i++){
        if( HW_Value[i] >= hw_gain )
            break;
    }
    pmic_spkl_vol(i);
    pmic_spkr_vol(i);
    Speaker_Volume =level;
    PRINTK("Sound_Speaker_SetVolLevel  pmic_spkl_vol[%d]\n",Speaker_Volume);
}


void Sound_Headset_Turnon(void)
{

}
void Sound_Headset_Turnoff(void)
{

}

//kernal use
void AudioAMPDevice_Suspend(void)
{
	PRINTK("AudioDevice_Suspend\n");
}
void AudioAMPDevice_Resume(void)
{
	PRINTK("AudioDevice_Resume\n");
}
void AudioAMPDevice_SpeakerLouderOpen(void)
{
	PRINTK("AudioDevice_SpeakerLouderOpen\n");
	return ;

}
void AudioAMPDevice_SpeakerLouderClose(void)
{
	PRINTK("AudioDevice_SpeakerLouderClose\n");
}
void AudioAMPDevice_mute(void)
{
	PRINTK("AudioDevice_mute\n");
}

int Audio_eamp_command(unsigned int type, unsigned long args, unsigned int count)
{
	return 0;
}

static char *ExtFunArray[] =
{
    "InfoMATVAudioStart",
    "InfoMATVAudioStop",
    "End",
};

kal_int32 Sound_ExtFunction(const char* name, void* param, int param_size)
{
	int i = 0;
	int funNum = -1;

	//Search the supported function defined in ExtFunArray
	while(strcmp("End",ExtFunArray[i]) != 0 ) {		//while function not equal to "End"
		
	    if (strcmp(name,ExtFunArray[i]) == 0 ) {		//When function name equal to table, break
	    	funNum = i;
	    	break;
	    }
	    i++;
	}

	switch (funNum) {
	    case 0:			//InfoMATVAudioStart
	        printk("RunExtFunction InfoMATVAudioStart \n");
	        break;

	    case 1:			//InfoMATVAudioStop
	        printk("RunExtFunction InfoMATVAudioStop \n");
	        break;

	    default:
	    	 break;
	}

	return 1;
}


