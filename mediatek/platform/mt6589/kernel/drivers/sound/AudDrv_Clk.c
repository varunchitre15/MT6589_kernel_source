/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*******************************************************************************
 *
 * Filename:
 * ---------
 *   AudDrv_Clk.c
 *
 * Project:
 * --------
 *   MT6583  Audio Driver clock control implement
 *
 * Description:
 * ------------
 *   Audio register
 *
 * Author:
 * -------
 * Chipeng Chang (MTK02308)
 *
 *------------------------------------------------------------------------------
 * $Revision: #1 $
 * $Modtime:$
 * $Log:$
 *
 *
 *******************************************************************************/


/*****************************************************************************
 *                     C O M P I L E R   F L A G S
 *****************************************************************************/


/*****************************************************************************
 *                E X T E R N A L   R E F E R E N C E S
 *****************************************************************************/
#include <mach/mt_clkmgr.h>
#include <mach/mt_pm_ldo.h>
#include <mach/pmic_mt6320_sw.h>
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>

#include "AudDrv_Common.h"
#include "AudDrv_Clk.h"
#include "AudDrv_Afe.h"
#include "AudDrv_Ana.h"
#include <linux/spinlock.h>
#include <linux/delay.h>



/*****************************************************************************
 *                         D A T A   T Y P E S
 *****************************************************************************/

int 	    Aud_Core_Clk_cntr   = 0;
int        Aud_AFE_Clk_cntr    = 0;
int        Aud_ADC_Clk_cntr    = 0;
int        Aud_I2S_Clk_cntr    = 0;
int        Aud_ANA_Clk_cntr    = 0;
int        Aud_LineIn_Clk_cntr =0;
int        Aud_HDMI_Clk_cntr =0;
int        Afe_Mem_Pwr_on =0;

static DEFINE_SPINLOCK(auddrv_Clk_lock);
 // amp mutex lock
 static DEFINE_MUTEX(auddrv_pmic_mutex);


 /*****************************************************************************
  * FUNCTION
  *  AudDrv_Clk_On / AudDrv_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable PLL(26M clock) \ AFE clock
  *
  *****************************************************************************
  */
 void AudDrv_Clk_On(void)
 {
     unsigned long flags;
     spin_lock_irqsave(&auddrv_Clk_lock, flags);
     if(Aud_AFE_Clk_cntr == 0 )
     {
         PRINTK_AUD_CLK("+AudDrv_Clk_On, Aud_AFE_Clk_cntr:%d \n", Aud_AFE_Clk_cntr);
         #ifdef PM_MANAGER_API
         Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00004000, 0x00004000);  // bit2: afe power on
         if(enable_clock(MT_CG_AUDIO_AFE,"AUDIO")){
             xlog_printk(ANDROID_LOG_ERROR, "Sound","Aud enable_clock MT_CG_AUDIO_AFE fail !!!\n");
         }
         #else
         Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00004000, 0xffffffff);  // bit2: afe power on
         #endif
     }
     Aud_AFE_Clk_cntr++;
     spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
     //PRINTK_AUD_CLK("-AudDrv_Clk_On, Aud_AFE_Clk_cntr:%d \n",Aud_AFE_Clk_cntr);
}

 void AudDrv_Clk_Off(void)
 {
     unsigned long flags;
     //PRINTK_AUD_CLK("+!! AudDrv_Clk_Off, Aud_AFE_Clk_cntr:%d \n",Aud_AFE_Clk_cntr);
     spin_lock_irqsave(&auddrv_Clk_lock, flags);

     Aud_AFE_Clk_cntr--;
     if(Aud_AFE_Clk_cntr == 0)
     {
         PRINTK_AUD_CLK("+ AudDrv_Clk_Off, Aud_AFE_Clk_cntr:%d \n", Aud_AFE_Clk_cntr);
         {
             // Disable AFE clock
             #ifdef PM_MANAGER_API
             Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00004004, 0x00004000);	// bit2: power down afe
             if(disable_clock(MT_CG_AUDIO_AFE,"AUDIO")){
                 xlog_printk(ANDROID_LOG_ERROR, "Sound","disable_clock MT_CG_AUDIO_AFE fail");
             }
             #else
             Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00000000, 0x00004043);  // bit2: power on
             #endif
         }
     }
     else if(Aud_AFE_Clk_cntr < 0){
	   PRINTK_AUD_ERROR("!! AudDrv_Clk_Off, Aud_AFE_Clk_cntr<0 (%d) \n",Aud_AFE_Clk_cntr);
	   AUDIO_ASSERT(true);
	   Aud_AFE_Clk_cntr =0;
     }

     spin_unlock_irqrestore(&auddrv_Clk_lock, flags);
     //PRINTK_AUD_CLK("-!! AudDrv_Clk_Off, Aud_AFE_Clk_cntr:%d \n",Aud_AFE_Clk_cntr);
 }

 /*****************************************************************************
 * FUNCTION
 *  AudDrv_Suspend_Clk_Off / AudDrv_Suspend_Clk_On
 *
 * DESCRIPTION
 *  Enable/Disable AFE clock for suspend
 *
 *****************************************************************************
 */
void AudDrv_Suspend_Clk_On(void)
{
   spin_lock_bh(&auddrv_Clk_lock);

   if(Aud_AFE_Clk_cntr>0)
   {
      PRINTK_AUD_CLK("AudDrv_Suspend_Clk_On Aud_AFE_Clk_cntr:%d ANA_Clk(%d) \n",Aud_AFE_Clk_cntr,Aud_ANA_Clk_cntr);
      #ifdef PM_MANAGER_API
      Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00004000, 0x00004000); // bit2: afe power on, bit6: I2S power on
      //Enable AFE clock
      if(enable_clock(MT_CG_AUDIO_AFE,"AUDIO")){
      	PRINTK_AUD_CLK("AudDrv_Suspend_Clk_On() Aud enable_clock() MT_CG_AUDIO_AFE fail");
      }
      else
      {
        PRINTK_AUD_CLK("AudDrv_Suspend_Clk_On() Aud enable_clock() MT_CG_AUDIO_AFE success");
      }

      if(Aud_I2S_Clk_cntr>0)
      {
      	///Enable I2S clock
         if(enable_clock(MT_CG_AUDIO_I2S,"AUDIO"))
            xlog_printk(ANDROID_LOG_ERROR, "Sound","AudDrv_Suspend_Clk_On() Aud enable_clock() MT_CG_AUDIO_I2S fail");
      }
      Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00004000, 0x00004044); // bit2: afe power on, bit6: I2S power on
      #else
      Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00004000, 0x00004044); // bit2: afe power on, bit6: I2S power on
      #endif
   }
   spin_unlock_bh(&auddrv_Clk_lock);
   if(Aud_ANA_Clk_cntr >0)
   {
       PRINTK_AUD_CLK("AudDrv_Suspend_Clk_On Aud_AFE_Clk_cntr:%d ANA_Clk(%d) \n",Aud_AFE_Clk_cntr,Aud_ANA_Clk_cntr);
       upmu_set_rg_clksq_en(1);
   }
   //PRINTK_AUD_CLK("-AudDrv_Suspend_Clk_On Aud_AFE_Clk_cntr:%d ANA_Clk(%d) \n",Aud_AFE_Clk_cntr,Aud_ANA_Clk_cntr);
}

void AudDrv_Suspend_Clk_Off(void)
{
   spin_lock_bh(&auddrv_Clk_lock);
   if(Aud_AFE_Clk_cntr>0)
   {
      PRINTK_AUD_CLK("AudDrv_Suspend_Clk_Off Aud_AFE_Clk_cntr:%d ANA_Clk(%d)\n",Aud_AFE_Clk_cntr,Aud_ANA_Clk_cntr);
      #ifdef PM_MANAGER_API
      //Disable AFE clock and I2S clock
      Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00004000, 0x00004000); // bit2: afe power off, bit6: I2S power off

      if(disable_clock(MT_CG_AUDIO_AFE,"AUDIO"))
          xlog_printk(ANDROID_LOG_ERROR, "Sound","AudDrv_Suspend_Clk_Off() disable_clock MT_CG_AUDIO_AFE fail");

      if(Aud_I2S_Clk_cntr>0)
      {
         if(disable_clock(MT_CG_AUDIO_I2S,"AUDIO"))
             xlog_printk(ANDROID_LOG_ERROR, "Sound","AudDrv_Suspend_Clk_Off() disable_clock MT_CG_AUDIO_I2S fail");
      }
      #else
      Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00004044, 0x00004044);  // bit2: afe power off, bit6: I2S power off
      #endif
   }
   spin_unlock_bh(&auddrv_Clk_lock);
   if(Aud_ANA_Clk_cntr > 0)
   {
       PRINTK_AUD_CLK("AudDrv_Suspend_Clk_On Aud_AFE_Clk_cntr:%d ANA_Clk(%d) \n",Aud_AFE_Clk_cntr,Aud_ANA_Clk_cntr);
       upmu_set_rg_clksq_en(0);
   }
}



/*****************************************************************************
 * FUNCTION
 *	AudDrv_ANA_Top_On / AudDrv_ANA_Top_Off
 *
 * DESCRIPTION
 *	Enable/Disable analog part clock
 *
 *****************************************************************************/

void AudDrv_ANA_Top_On(void)
{
    Ana_Set_Reg(TOP_CKPDN,0x0000 ,0x00000003);
}

void AudDrv_ANA_Top_Off(void)
{
    Ana_Set_Reg(TOP_CKPDN,0x0003 ,0x00000003);
}


 /*****************************************************************************
  * FUNCTION
  *  AudDrv_ANA_Clk_On / AudDrv_ANA_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable analog part clock
  *
  *****************************************************************************/
 void AudDrv_ANA_Clk_On(void)
 {
     mutex_lock(&auddrv_pmic_mutex);
     if(Aud_ANA_Clk_cntr == 0 )
     {
         PRINTK_AUD_CLK("+AudDrv_ANA_Clk_On, Aud_ANA_Clk_cntr:%d \n", Aud_ANA_Clk_cntr);
         upmu_set_rg_clksq_en(1);
         AudDrv_ANA_Top_On();
     }
     Aud_ANA_Clk_cntr++;
     mutex_unlock(&auddrv_pmic_mutex);
     //PRINTK_AUD_CLK("-AudDrv_ANA_Clk_Off, Aud_ANA_Clk_cntr:%d \n",Aud_ANA_Clk_cntr);
 }

 void AudDrv_ANA_Clk_Off(void)
 {
     //PRINTK_AUD_CLK("+AudDrv_ANA_Clk_Off, Aud_ADC_Clk_cntr:%d \n",  Aud_ANA_Clk_cntr);
     mutex_lock(&auddrv_pmic_mutex);
     Aud_ANA_Clk_cntr--;
     if(Aud_ANA_Clk_cntr == 0)
     {
         PRINTK_AUD_CLK("+AudDrv_ANA_Clk_Off disable_clock Ana clk(%x)\n",Aud_ANA_Clk_cntr);
         // Disable ADC clock
         #ifdef PM_MANAGER_API
         upmu_set_rg_clksq_en(0);
         AudDrv_ANA_Top_Off();
         #else
         // TODO:: open ADC clock....
         #endif
     }
     else if(Aud_ANA_Clk_cntr < 0){
         PRINTK_AUD_ERROR("!! AudDrv_ANA_Clk_Off, Aud_ADC_Clk_cntr<0 (%d) \n", Aud_ANA_Clk_cntr);
         AUDIO_ASSERT(true);
         Aud_ANA_Clk_cntr =0;
     }
     mutex_unlock(&auddrv_pmic_mutex);
     //PRINTK_AUD_CLK("-AudDrv_ANA_Clk_Off, Aud_ADC_Clk_cntr:%d \n", Aud_ANA_Clk_cntr);
 }

 /*****************************************************************************
  * FUNCTION
   *  AudDrv_ADC_Clk_On / AudDrv_ADC_Clk_Off
   *
   * DESCRIPTION
   *  Enable/Disable analog part clock
   *
   *****************************************************************************/

 void AudDrv_ADC_Clk_On(void)
 {
     //PRINTK_AUDDRV("+AudDrv_ADC_Clk_On, Aud_ADC_Clk_cntr:%d \n", Aud_ADC_Clk_cntr);
     mutex_lock(&auddrv_pmic_mutex);

     if(Aud_ADC_Clk_cntr == 0 )
     {
         PRINTK_AUDDRV("+AudDrv_ADC_Clk_On enable_clock ADC clk(%x)\n",Aud_ADC_Clk_cntr);
         #ifdef PM_MANAGER_API
         //hwPowerOn(MT65XX_POWER_LDO_VA28,VOL_2800 , "AUDIO");
         #endif
     }
     Aud_ADC_Clk_cntr++;
     mutex_unlock(&auddrv_pmic_mutex);
 }

void AudDrv_ADC_Clk_Off(void)
{
    //PRINTK_AUDDRV("+AudDrv_ADC_Clk_Off, Aud_ADC_Clk_cntr:%d \n", Aud_ADC_Clk_cntr);
    mutex_lock(&auddrv_pmic_mutex);
    Aud_ADC_Clk_cntr--;
    if(Aud_ADC_Clk_cntr == 0)
    {
        PRINTK_AUDDRV("+AudDrv_ADC_Clk_On disable_clock ADC clk(%x)\n",Aud_ADC_Clk_cntr);
        #ifdef PM_MANAGER_API
        //hwPowerDown(MT65XX_POWER_LDO_VA28, "AUDIO");
        #endif
    }
    if(Aud_ADC_Clk_cntr < 0){
        PRINTK_AUDDRV("!! AudDrv_ADC_Clk_Off, Aud_ADC_Clk_cntr<0 (%d) \n", Aud_ADC_Clk_cntr);
        Aud_ADC_Clk_cntr =0;
    }
    mutex_unlock(&auddrv_pmic_mutex);
    //PRINTK_AUDDRV("-AudDrv_ADC_Clk_Off, Aud_ADC_Clk_cntr:%d \n", Aud_ADC_Clk_cntr);
}

/*****************************************************************************
  * FUNCTION
  *  AudDrv_I2S_Clk_On / AudDrv_I2S_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable analog part clock
  *
  *****************************************************************************/
 void AudDrv_I2S_Clk_On(void)
 {
     unsigned long flags;
     //PRINTK_AUD_CLK("+AudDrv_I2S_Clk_On, Aud_I2S_Clk_cntr:%d \n", Aud_I2S_Clk_cntr);
     spin_lock_irqsave(&auddrv_Clk_lock,flags);
     if(Aud_I2S_Clk_cntr == 0 )
     {
         #ifdef PM_MANAGER_API
         if(enable_clock(MT_CG_AUDIO_I2S, "AUDIO")){
             PRINTK_AUD_ERROR("Aud enable_clock MT65XX_PDN_AUDIO_I2S fail !!!\n");
         }
         #else
         Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00000040, 0x00000040);  //power on I2S clock
         #endif
     }
     Aud_I2S_Clk_cntr++;
     spin_unlock_irqrestore(&auddrv_Clk_lock,flags);
 }

 void AudDrv_I2S_Clk_Off(void)
 {
     unsigned long flags;
     //PRINTK_AUD_CLK("+AudDrv_I2S_Clk_Off, Aud_I2S_Clk_cntr:%d \n", Aud_I2S_Clk_cntr);
     spin_lock_irqsave(&auddrv_Clk_lock,flags);
     Aud_I2S_Clk_cntr--;
     if(Aud_I2S_Clk_cntr == 0)
     {
         #ifdef PM_MANAGER_API
         if(disable_clock(MT_CG_AUDIO_I2S, "AUDIO"))
         {
             PRINTK_AUD_ERROR("disable_clock MT_CG_AUDIO_I2S fail");
         }
         #else
             Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00000000, 0x00000040);  //power off I2S clock
         #endif
     }
     else if(Aud_I2S_Clk_cntr < 0){
         PRINTK_AUD_ERROR("!! AudDrv_I2S_Clk_Off, Aud_I2S_Clk_cntr<0 (%d) \n", Aud_I2S_Clk_cntr);
         AUDIO_ASSERT(true);
         Aud_I2S_Clk_cntr =0;
     }
     spin_unlock_irqrestore(&auddrv_Clk_lock,flags);
     //PRINTK_AUD_CLK("-AudDrv_I2S_Clk_Off, Aud_I2S_Clk_cntr:%d \n",Aud_I2S_Clk_cntr);
 }

 /*****************************************************************************
   * FUNCTION
   *  AudDrv_Core_Clk_On / AudDrv_Core_Clk_Off
   *
   * DESCRIPTION
   *  Enable/Disable analog part clock
   *
   *****************************************************************************/

 void AudDrv_Core_Clk_On(void)
 {
     //PRINTK_AUD_CLK("+AudDrv_Core_Clk_On, Aud_Core_Clk_cntr:%d \n", Aud_Core_Clk_cntr);
     spin_lock(&auddrv_Clk_lock);
     if(Aud_Core_Clk_cntr == 0 )
     {
         #ifdef PM_MANAGER_API
         if(enable_clock(MT_CG_AUDIO_AFE, "AUDIO"))
         {
             PRINTK_AUD_ERROR("AudDrv_Core_Clk_On Aud enable_clock MT_CG_AUDIO_AFE fail !!!\n");
         }
         #endif
     }
     Aud_Core_Clk_cntr++;
     spin_unlock(&auddrv_Clk_lock);
     //PRINTK_AUD_CLK("-AudDrv_Core_Clk_On, Aud_Core_Clk_cntr:%d \n", Aud_Core_Clk_cntr);
 }


 void AudDrv_Core_Clk_Off(void)
 {
     //PRINTK_AUD_CLK("+AudDrv_Core_Clk_On, Aud_Core_Clk_cntr:%d \n", Aud_Core_Clk_cntr);
     spin_lock(&auddrv_Clk_lock);
     if(Aud_Core_Clk_cntr == 0 )
    {
        #ifdef PM_MANAGER_API
        if(disable_clock(MT_CG_AUDIO_AFE, "AUDIO"))
        {
            PRINTK_AUD_ERROR("AudDrv_Core_Clk_On Aud disable_clock MT_CG_AUDIO_AFE fail !!!\n");
        }
        #endif
    }
    Aud_Core_Clk_cntr++;
    spin_unlock(&auddrv_Clk_lock);
    //PRINTK_AUD_CLK("-AudDrv_Core_Clk_On, Aud_Core_Clk_cntr:%d \n", Aud_Core_Clk_cntr);
 }


/*****************************************************************************
  * FUNCTION
  *  AudDrv_Linein_Clk_On / AudDrv_Linein_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable analog part clock
  *
  *****************************************************************************/
 void AudDrv_Linein_Clk_On(void)
 {
     PRINTK_AUD_CLK("+AudDrv_Linein_Clk_On, Aud_I2S_Clk_cntr:%d \n", Aud_LineIn_Clk_cntr);
     if(Aud_LineIn_Clk_cntr == 0 )
     {
         #ifdef PM_MANAGER_API
         AudDrv_ANA_Clk_On();
         AudDrv_Clk_On();
         #else
         Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00000040, 0x00000040);  //power on I2S clock
         #endif
     }
     Aud_LineIn_Clk_cntr++;
 }

 void AudDrv_Linein_Clk_Off(void)
 {
     PRINTK_AUD_CLK("+AudDrv_Linein_Clk_Off, Aud_I2S_Clk_cntr:%d \n", Aud_LineIn_Clk_cntr);
     Aud_LineIn_Clk_cntr--;
     if(Aud_LineIn_Clk_cntr == 0)
     {
         #ifdef PM_MANAGER_API
         AudDrv_ANA_Clk_On();
         AudDrv_Clk_On();
         #else
         Afe_Set_Reg(AUDIO_AFE_TOP_CON0, 0x00000000, 0x00000040);  //power off I2S clock
         #endif
     }
     else if(Aud_LineIn_Clk_cntr < 0){
         PRINTK_AUD_ERROR("!! AudDrv_Linein_Clk_Off, Aud_I2S_Clk_cntr<0 (%d) \n", Aud_LineIn_Clk_cntr);
         AUDIO_ASSERT(true);
         Aud_LineIn_Clk_cntr =0;
     }
     PRINTK_AUD_CLK("-AudDrv_I2S_Clk_Off, Aud_I2S_Clk_cntr:%d \n",Aud_LineIn_Clk_cntr);
 }

/*****************************************************************************
  * FUNCTION
  *  AudDrv_HDMI_Clk_On / AudDrv_HDMI_Clk_Off
  *
  * DESCRIPTION
  *  Enable/Disable analog part clock
  *
  *****************************************************************************/

void AudDrv_HDMI_Clk_On(void)
{
     PRINTK_AUD_CLK("+AudDrv_Linein_Clk_On, Aud_I2S_Clk_cntr:%d \n", Aud_HDMI_Clk_cntr);
     if(Aud_HDMI_Clk_cntr == 0 )
     {
         AudDrv_ANA_Clk_On();
         AudDrv_Clk_On();
     }
     Aud_HDMI_Clk_cntr++;
}

void AudDrv_HDMI_Clk_Off(void)
{
     PRINTK_AUD_CLK("+AudDrv_Linein_Clk_Off, Aud_I2S_Clk_cntr:%d \n", Aud_HDMI_Clk_cntr);
     Aud_HDMI_Clk_cntr--;
     if(Aud_HDMI_Clk_cntr == 0)
     {
         AudDrv_ANA_Clk_Off();
         AudDrv_Clk_Off();
     }
     else if(Aud_HDMI_Clk_cntr < 0){
         PRINTK_AUD_ERROR("!! AudDrv_Linein_Clk_Off, Aud_I2S_Clk_cntr<0 (%d) \n", Aud_HDMI_Clk_cntr);
         AUDIO_ASSERT(true);
         Aud_HDMI_Clk_cntr =0;
     }
     PRINTK_AUD_CLK("-AudDrv_I2S_Clk_Off, Aud_I2S_Clk_cntr:%d \n",Aud_HDMI_Clk_cntr);
}

// export symbol for other module use
EXPORT_SYMBOL(AudDrv_Clk_On);
EXPORT_SYMBOL(AudDrv_Clk_Off);
EXPORT_SYMBOL(AudDrv_ANA_Clk_On);
EXPORT_SYMBOL(AudDrv_ANA_Clk_Off);
EXPORT_SYMBOL(AudDrv_I2S_Clk_On);
EXPORT_SYMBOL(AudDrv_I2S_Clk_Off);

