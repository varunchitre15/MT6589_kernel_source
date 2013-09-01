#ifdef MTK_MT8193_HDMI_SUPPORT

#include "mt8193_ctrl.h"
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/div64.h>

#ifdef MT6575
#include <mach/mt6575_devs.h>
#include <mach/mt6575_typedefs.h>
#include <mach/mt6575_gpio.h>
#include <mach/mt6575_pm_ldo.h>
#endif
#ifdef MT6589
#include <mach/devs.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>
#endif



//example to set mt8193 clk
void vSetClk(void)
{
    //mt8193_i2c_write();
    //mt8193_i2c_write();
    //mt8193_i2c_write();
}

u8  fgMT8193DDCByteWrite(u8 ui1Device, u8 ui1Data_Addr, u8  u1Data)
{
  u8 fgResult = 0; 
  
  //fgResult = fgTxDataWrite(ui1Device/2, ui1Data_Addr, 1, &u1Data);
    
  
  if (fgResult== TRUE)
  {
    return TRUE;
  }	
  else
  {
    return FALSE;
  }	
}

u8  fgMT8193DDCDataWrite(u8 ui1Device, u8 ui1Data_Addr, u8 u1Count, const u8  *pr_u1Data)
{
	u8 fgResult = 0; 
	
	//fgResult = fgTxDataWrite(ui1Device/2, ui1Data_Addr, 1, &u1Data);
	  
	
	if (fgResult== TRUE)
	{
	  return TRUE;
	} 
	else
	{
	  return FALSE;
	} 

}

u8 fgMT8193DDCByteRead(u8 ui1Device, u8 ui1Data_Addr, u8 * pu1Data)
{
  u8 fgResult = 0;
  
 
  //fgResult= fgTxDataRead(ui1Device/2, ui1Data_Addr, 1, pu1Data);
  
  
  if (fgResult== TRUE)
  {
    return TRUE;
  }	
  else
  {
    return FALSE;
  }	
}

u8 fgMT8193DDCDataRead(u8 ui1Device, u8 ui1Data_Addr, u8 u1Count, u8 * pu1Data)
{
  u8 fgResult = 0;
  
  
  //fgResult= fgTxDataRead(ui1Device/2, ui1Data_Addr, 1, pu1Data);
  
  
  if (fgResult== TRUE)
  {
    return TRUE;
  } 
  else
  {
    return FALSE;
  } 
}

#endif
