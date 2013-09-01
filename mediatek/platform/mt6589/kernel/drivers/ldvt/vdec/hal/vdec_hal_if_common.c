
#include "vdec_hal_if_common.h"
#include "vdec_hw_common.h"
#include <mach/mt_typedefs.h>
//#include "x_ckgen.h"
//#include "drv_config.h"
//#include "x_debug.h"

#if (CONFIG_DRV_LINUX_DATA_CONSISTENCY)
//#include <config/arch/chip_ver.h>
#endif
#include "../verify/vdec_verify_mpv_prov.h"

#if CONFIG_SECTION_BUILD_LINUX_KO
#include <mach/irqs.h>
//#include <mach/mt8530.h>



#define MUSB_BASE3                        (IO_VIRT + 0xF000)
#define MUSB_ANAPHYBASE                    (0x700)

#define M_REG_AUTOPOWER                      0x80
#define M_REG_AUTOPOWER_DRAMCLK         0x01
#define M_REG_AUTOPOWER_ON                     0x02
#define M_REG_AUTOPOWER_PHYCLK             0x04
#define MUSB_BASE                       (IO_VIRT + 0xE000)
#define MUSB_PHYBASE                           (0x400)
#define MUSB_MISCBASE                     (0x600)

#define MGC_PHY_Read32(_pBase, r)      \
    *((volatile UINT32 *)(((UINT32)_pBase) + (MUSB_PHYBASE)+ (r)))
#define MGC_PHY_Write32(_pBase, r, v)  \
    (*((volatile UINT32 *)((((UINT32)_pBase) + MUSB_PHYBASE)+ (r))) = v)
#define MGC_MISC_Read32(_pBase, r)     \
    *((volatile UINT32 *)(((UINT32)_pBase) + (MUSB_MISCBASE)+ (r)))
#define MGC_MISC_Write32(_pBase, r, v) \
    (*((volatile UINT32 *)(((UINT32)_pBase) + (MUSB_MISCBASE)+ (r))) = v)
#define MGC_AnaPhy_Read32(_pBase, _offset) \
    (*((volatile UINT32 *)(((UINT32)_pBase) + MUSB_ANAPHYBASE + _offset)))
#define MGC_AnaPhy_Write32(_pBase, _offset, _data) \
    (*((volatile UINT32 *)(((UINT32)_pBase) + MUSB_ANAPHYBASE + _offset)) = _data)
#endif

#define VDEC_VLD_USE_USB  0
// **************************************************************************
// Function : INT32 i4VDEC_HAL_Common_Init(UINT32 u4ChipID);
// Description : Turns on video decoder HAL
// Parameter : u4ChipID
// Return      : >0: init OK.
//                  <0: init failed
// **************************************************************************
INT32 i4VDEC_HAL_Common_Init(UINT32 u4ChipID)
{
#if (!CONFIG_DRV_FPGA_BOARD)

#if CONFIG_SECTION_BUILD_LINUX_KO
    //In Linux environemnt, enable USB PLL for VDEC, if USB driver compile.
#if (VDEC_VLD_USE_USB  || (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8530))
    UINT32 pBase =  MUSB_BASE;
    UINT32 u4Reg = 0;
#endif

#endif
///TODO: Clock Setting
/*
   #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
    //set vdec clock
    CKGEN_WRITE32(REG_RW_CLK_SEL_0, (CKGEN_READ32(REG_RW_CLK_SEL_0)&(~(CLK_SEL_0_VDEC_MASK|CLK_SEL_0_MC_MASK)))
    |((CLK_SEL_0_MC_DMPLL_1_4 << 12) | (CLK_SEL_0_MC_DMPLL_1_4 << 16)));
    #else

    #if (CONFIG_DRV_VERIFY_SUPPORT)    
    if (_u4CodecVer[0] == VDEC_H264 || _u4CodecVer[0] == VDEC_AVS || _u4CodecVer[0] == VDEC_MPEG2 || _u4CodecVer[0] == VDEC_RM)
    	{
   #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
    	CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CFG1_VDEC_SEL_SYSPLL1_1_2);
   #else
    	CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CLK_VDEC_SEL_SYSPLL2_1_2);
   #endif
    	}
    else if (_u4CodecVer[0] == VDEC_MPEG4 || _u4CodecVer[0] == VDEC_WMV)
    	{
   #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
    	CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CFG1_VDEC_SEL_SYSPLL1_1_2);
   #else
    	CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CLK_VDEC_SEL_SYSPLL1_1_2);
   #endif
    	}    	
    else
    	{
   #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
    	CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CFG1_VDEC_SEL_SYSPLL1_1_2);
   #else
    	CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CLK_VDEC_SEL_SYSPLL2_1_2);
   #endif
    	}
   #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
    	CKGEN_AgtSelClk(e_CLK_MC,   CLK_CFG1_MC_SEL_SYSPLL2_1_2);
   #else
    	CKGEN_AgtSelClk(e_CLK_MC,   CLK_CLK_MC_SEL_SYSPLL2_1_2);
   #endif
    #else
#if (CONFIG_CHIP_VER_CURR  < CONFIG_CHIP_VER_MT8580)
#if VDEC_VLD_USE_USB    
    CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CLK_VDEC_SEL_USBPLL);
#else
   #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
    	CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CFG1_VDEC_SEL_27M);
        CKGEN_WRITE32(REG_RW_CLK_CFG8, (CKGEN_REG32(REG_RW_CLK_CFG8)&0xFFFFFFF8) | 0x07);
   #else
    CKGEN_AgtSelClk(e_CLK_VDEC, CLK_CLK_VDEC_SEL_SYSPLL1_1_2);
   #endif
#endif
   #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8580)
//    	CKGEN_AgtSelClk(e_CLK_MC,   CLK_CFG1_MC_SEL_SYSPLL2_1_2);
   #else
    CKGEN_AgtSelClk(e_CLK_MC,   CLK_CLK_MC_SEL_SYSPLL2_1_2);
   #endif
    #endif
    #endif
    #endif
*/
    // reset common SRAM
    vVDecWriteVLD(0, RW_VLD_RESET_COMMOM_SRAM, 0x00000000);
    vVDecWriteVLD(0, RW_VLD_RESET_COMMOM_SRAM, 0x00000100);
    vVDecWriteVLD(0, RW_VLD_RESET_COMMOM_SRAM, 0x00000000);
    vVDecWriteVLD(1, RW_VLD_RESET_COMMOM_SRAM, 0x00000000);
    vVDecWriteVLD(1, RW_VLD_RESET_COMMOM_SRAM, 0x00000100);
    vVDecWriteVLD(1, RW_VLD_RESET_COMMOM_SRAM, 0x00000000);
#endif

    return HAL_HANDLE_OK;
}
#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)  && (!CONFIG_DRV_FPGA_BOARD))
void vVDEC_HAL_CLK_Set(UINT32 u4CodeType)
{
    vVDecSetVldMcClk(0,u4CodeType);
}
#endif

#if VDEC_REMOVE_UNUSED_FUNC
// **************************************************************************
// Function : INT32 i4VDEC_HAL_Common_Uninit(void);
// Description : Turns off video decoder HAL
// Parameter : void
// Return      : >0: uninit OK.
//                  <0: uninit failed
// **************************************************************************
INT32 i4VDEC_HAL_Common_Uninit(void)
{
    return HAL_HANDLE_OK;
}
#endif

#if 0
/******************************************************************************
* Global Function
******************************************************************************/
void MPV_PllInit(void)
{
       UINT32 u4DramClk;
       UINT16 u2MS, u2NS, u2Counter;
       UINT8 u1Band;
       UINT8 ucDelay;

       u4DramClk = BSP_GetClock(CAL_SRC_DMPLL, &u1Band, &u2MS, &u2NS,
            &u2Counter);

       printk("DRAM CLK: %d\n", u4DramClk);
/*
	// set vpll test_gnd = 1, bs = 4, bw = 2, gs = 2, cpi = 8 (default)
	// 148.5MHz
	*((UINT32*)0x2000D030) = (UINT32)0x00082528;
*/
	ucDelay = 0;
	*((UINT32*)0x2000D03c) = (*((UINT32*)0x2000D03c) & 0xC0FFC0FF) | ((ucDelay<<24)|(ucDelay<<8));
	*((UINT32*)0x2000D040) = (*((UINT32*)0x2000D040) & 0xFFFFC0FF) | (ucDelay<<8);

	// set mdec_ck = ck_vpll (default)
	*((UINT32*)0x2000D048) = (*((UINT32*)0x2000D048) & 0xFF00FFFF) | 0x00010000;
	*((UINT32*)0x20007054) = (*((UINT32*)0x20007054) & 0xFFFF000F) | 0x0000FFF0;


}
#endif

#if VDEC_REMOVE_UNUSED_FUNC
// **************************************************************************
// Function : UINT32 u4VDEC_HAL_Common_GetHWResourceNum(UINT32 *pu4BSNum, UINT32 *pu4VLDNum);
// Description :Get hardware resource number
// Parameter : pu4BSNum : Pointer to barrel shifter number of every VLD
//                 pu4VLDNum : Pointer to VLD number
// Return      : None
// **************************************************************************
void vVDEC_HAL_Common_GetHWResourceNum(UINT32 *pu4BSNum, UINT32 *pu4VLDNum)
{
    *pu4BSNum = 2;
    *pu4VLDNum = 2;
  
    return;
}
#endif

// **************************************************************************
// Function : void vDEC_HAL_COMMON_SetVLDPower(UINT32 u4VDecID, BOOL fgOn);
// Description :Turn on or turn off VLD power
// Parameter : u4VDecID : video decoder hardware ID
//                 fgOn : Flag to vld power on or off
// Return      : None
// **************************************************************************
void vDEC_HAL_COMMON_SetVLDPower(UINT32 u4VDecID, BOOL fgOn)
{
    if (fgOn)
    {
        vVDecWriteVLD(u4VDecID, RW_VLD_PWRSAVE, 0);
    }
    else
    {
        vVDecWriteVLD(u4VDecID, RW_VLD_PWRSAVE, 1);
    }
    return;
}


// **************************************************************************
// Function : void vDEC_HAL_COMMON_PowerOn(UINT32 u4VDecID, BOOL fgOn);
// Description :Turn on or turn off VLD power
// Parameter : u4VDecID : video decoder hardware ID
//                 fgOn : Flag to vld power on or off
// Return      : None
// **************************************************************************
void vDEC_HAL_COMMON_PowerOn(void)
{    
#if 0
    CKGEN_AgtOnClk(e_CLK_VDEC);
    CKGEN_AgtOnClk(e_CLK_MC);
#endif    
    return;
}

// **************************************************************************
// Function : void vDEC_HAL_COMMON_PowerOff(UINT32 u4VDecID, BOOL fgOn);
// Description :Turn on or turn off VLD power
// Parameter : u4VDecID : video decoder hardware ID
//                 fgOn : Flag to vld power on or off
// Return      : None
// **************************************************************************
void vDEC_HAL_COMMON_PowerOff(void)
{    
#if 0
    CKGEN_AgtOffClk(e_CLK_VDEC);
    CKGEN_AgtOffClk(e_CLK_MC);
#endif    
    return;
}



INT32 i4VDEC_HAL_Dram_Busy( UINT32 u4ChipID, UINT32 u4StartAddr, UINT32 u4Offset)
{

    vWriteReg(0x7210, (u4StartAddr << 4));
    vWriteReg(0x7214, (u4Offset << 4));
    vWriteReg(0x7104, 0x0);    
    vWriteReg(0x7218, 0x8e0f110d);    
    return 0;
}

INT32 i4VDEC_HAL_Dram_Busy_Off( UINT32 u4ChipID, UINT32 u4StartAddr, UINT32 u4Offset)
{

    vWriteReg(0x7210, (u4StartAddr << 4));
    vWriteReg(0x7214, (u4Offset << 4));
    //vWriteReg(0x7104, 0x0);    
    vWriteReg(0x7218, 0x860f110d);    
    return 0;
}

INT32 i4VDEC_HAL_Common_Gcon_Enable(void)
{
    vWriteGconReg(0, 0x1);
}

#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
INT32 vDEC_HAL_COMMON_ReadLBDResult(UINT32 ucMpvId, UINT32* u4YUpbound, 
    UINT32* u4YLowbound, UINT32* u4CUpbound, UINT32* u4CLowbound)
{
    UINT32 u4YResult, u4CResult;

    
    vVDECReadLetetrBoxDetResult(ucMpvId, &u4YResult, &u4CResult);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    *u4YUpbound = (u4YResult&0xFFF);
    *u4YLowbound = ((u4YResult >> 16)&0xFFF);
    *u4CUpbound = (u4CResult&0xFFF);
    *u4CLowbound = ((u4CResult >> 16)&0xFFF);
#else
    *u4YUpbound = (u4YResult&0x7FF);
    *u4YLowbound = ((u4YResult >> 16)&0x7FF);
    *u4CUpbound = (u4CResult&0x7FF);
    *u4CLowbound = ((u4CResult >> 16)&0x7FF);
#endif
    return 0;
}
#endif

#ifdef CAPTURE_ESA_LOG

UINT32 u4VDEC_HAL_Read_ESA(UINT32 u4InstID , UINT32 u4Temp)
{
    UINT32 u4Temp1;

   _u4ESAValue[u4InstID][0] = u4VDecReadMC(u4InstID, 0x770); //MC NBM_DLE_NUM
   _u4ESAValue[u4InstID][1] = u4VDecReadMC(u4InstID, 0x8B8); //MC ESA_REQ_DATA_NUM
   _u4ESAValue[u4InstID][2] = u4VDecReadMC(u4InstID, 0xA28);     //MC MC_REQ_DATA_NUM
   _u4ESAValue[u4InstID][3] = u4VDecReadMC(u4InstID, 0x28);     //MC MC_MBX
   _u4ESAValue[u4InstID][4] = u4VDecReadMC(u4InstID, 0x2C);     //MC MC_MBY
   _u4ESAValue[u4InstID][5] = u4VDecReadMC(u4InstID, 0x9E0);    //MC CYC_SYS 
   _u4ESAValue[u4InstID][6] = u4VDecReadMC(u4InstID, 0x9E4);     //MC INTRA_CNT
   _u4ESAValue[u4InstID][7] = u4VDecReadDV(u4InstID, 0xF0) & 0x1;    //VDEC_TOP LAT_BUF_BYPASS
    u4Temp1= u4Temp + sprintf (_pucESALog[u4InstID]+u4Temp, "%d,%d,%d,%d,%d,%d,%d,%d,\n", 
    	_u4ESAValue[u4InstID][0] ,
    	_u4ESAValue[u4InstID][1] ,
    	_u4ESAValue[u4InstID][2] ,
    	_u4ESAValue[u4InstID][3] ,
    	_u4ESAValue[u4InstID][4] ,
    	_u4ESAValue[u4InstID][5] ,
    	_u4ESAValue[u4InstID][6] ,
    	_u4ESAValue[u4InstID][7]);
    //printk("u4Temp = %d\n", u4Temp1);
    return u4Temp1;
}

#endif

#ifdef VDEC_BREAK_EN
#include <linux/random.h>

BOOL fgBreakVDec(UINT32 u4InstID)
{	
   UINT32 u4Cnt=0;
   UINT32 u4Mbx,u4Mby;
   char cid[3];
       get_random_bytes(cid, sizeof(cid));
       if (_u4FileCnt[u4InstID] % 3 == 0)
            u4Cnt = (cid[0]) | (cid[1] << 8) | (cid[2] << 16);
       else if (_u4FileCnt[u4InstID] % 3 == 1)
            u4Cnt = (cid[0]) | (cid[1] << 8);
       else 
            u4Cnt = (cid[0]);
       	
  //     printk("u4Cnt = 0x%x, cid[0] = 0x%x, cid[1] = 0x%x, cid[2] = 0x%x\n", u4Cnt, cid[0], cid[1], cid[2]);
       if (!(u4VDecReadDV(u4InstID, VDEC_DV_DEC_BREAK)&0x01))
       {
         while(u4Cnt--);
         vVDecWriteDV(u4InstID, VDEC_DV_DEC_BREAK, u4VDecReadDV(u4InstID, VDEC_DV_DEC_BREAK) |VDEC_DV_DEC_BREAK_EN);
         u4Cnt=0;
         while (!((u4VDecReadDV(u4InstID, VDEC_DV_DEC_BREAK_STATUS)&VDEC_BREAK_OK_0) && (u4VDecReadDV(u4InstID, VDEC_DV_DEC_BREAK_STATUS) & VDEC_BREAK_OK_1)))
           {
             u4Cnt++;
             if (u4Cnt >= 0x5000)
             {
               printk("not break\n");
               break;
             }
           }
       }
       else
       {
         printk("[1] fgBreakVDec Fail\n");
         return FALSE;
       }
       	
       u4Mbx= u4VDecReadMC(u4InstID, RO_MC_MBX);
       u4Mby = u4VDecReadMC(u4InstID, RO_MC_MBY);

 //      printk("u4Mbx = 0x%x, u4Mby = 0x%x\n", u4Mbx, u4Mby);

    if (u4Cnt == 0x5000)
    {
      printk("[2] fgBreakVDec Fail\n");
      return FALSE;
    }
//    printk("Break done, Count = 0x%x\n", u4Cnt);
    printk("fgBreakVDec Pass\n");
    return TRUE;
}
#endif


#if VMMU_SUPPORT
void vPage_Table(UINT32 u4InstID, UINT32  page_addr, UINT32 start, UINT32 end) // page_addr = table_base + (start/4KB)*4
{
  int page_num;
  int temp;
  int i;

  temp = (end-start)%(0x1000);
  page_num = (end-start)/(0x1000); 
  if (temp > 0)
  	page_num++;

  for (i = 0; i < page_num; i++)  	{
  *(UINT32P)(page_addr +4*i) = start + 0x02 + (0x1000) * (i+1); //PA = offset VA by 4KB
  	}
 }

void enable_vmmu0(UINT32 table_base)
{
//  vVDecWriteMC(u4InstID, 0xF2098000, table_base);
//  vVDecWriteMC(u4InstID, 0xF2098080, table_base);
//  vVDecWriteMC(u4InstID, 0xF2098300, table_base);
  *(UINT32P)(0xF2098000) = table_base; // set page table base
  *(UINT32P)(0xF2098080) = 0x0; // set prefetch distance = 0
  *(UINT32P)(0xF2098300) = 0x00000001; //enable vmmu0
  *(UINT32P)(0xF2098210) = 0x00000001; //enable vmmu0
}

void enable_vmmu1(UINT32 table_base)
{
  *(UINT32P)(0xF2099000) = table_base; // set page table base
  *(UINT32P)(0xF2099080) = 0x0; // set prefetch distance = 0
  *(UINT32P)(0xF2099300) = 0x00000001; //enable vmmu1
  *(UINT32P)(0xF2099210) = 0x00000001; //enable vmmu1
}

void vVDecVMMUEnable(UINT32 table_base)
{
enable_vmmu0(table_base);
enable_vmmu1(table_base);
}

#endif
