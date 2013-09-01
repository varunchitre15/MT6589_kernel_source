
//#include "vdec_hw_common.h"
//#include "vdec_hw_mpeg.h"
//#include <mach/mt6575_typedefs.h>

#include "vdec_hw_common.h"

#include "../include/vdec_info_mpeg.h"

#include "vdec_hw_mpeg.h"

#include "../include/vdec_info_h264.h"

#include "vdec_hw_h264.h"
#include "../include/drv_common.h"


//#include "x_hal_1176.h"
//#include "x_hal_ic.h"
//#include "x_debug.h"

#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_verify_mpv_prov.h"
#include "vdec_hal_if_mpeg.h"

#include <linux/string.h>
#if (!CONFIG_DRV_LINUX)
#include <string.h>
#include <stdio.h>
#endif

#if VMMU_SUPPORT
#include "vdec_hal_if_common.h"
#endif


extern void vVDecOutputDebugString(const CHAR * format, ...);
extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
extern void vVDecOutputDebugString(const CHAR * format, ...);
#else

#include "vdec_drv_mpeg4_info.h"

#ifdef VDEC_MPEG4_SUPPORT_RESYNC_MARKER

extern UINT32 _u4ResyncMarkMbx[2][2];
extern UINT32 _u4ResyncMarkMby[2][2];
extern UINT32 _u4ResyncMarkerCnt[2];

#endif

#endif


void vVLDDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
void vVLDDoDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgBPic,
                      BOOL fgB21FrmPic);
void vVLDDecPrmProc(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
void vVLDSetPicType(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
void vVLDSetPicSz(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgB21FrmPic);

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
void vVLDMp4SetPicSz(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgB21FrmPic);//mtk40343 9/7/2010
#endif

UINT32 u4VLDPicH(UCHAR ucMpegVer, UINT32 u4PicH);
void vVLDSetMBRowPrm(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgBPic, BOOL fgFrmPic);
void vVLDM4vDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgBVop);
void vVLDM4vDecPrmProc(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
void vVLDMp4TriggerDec(UINT32 u4VDecID);
void vVLDMp4DecPrmProc(UINT32 u4VDecID, VDEC_INFO_MPEG4_DEC_PRM_T *prMp4Prm,
                                UINT32 u4MBx,
                                UCHAR ucIntraDcVlcThr,
                                UCHAR ucQuarterSample,
                                UCHAR ucDataPartitioned);
void vVLDBarl2Vdec(UINT32 u4VDecID);
void vVLDSetDcacAddr(UINT32 u4VDecID, UINT32 dAddr);
void vVLDDx3Dec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
void vVLDDx3DecPrmProc(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
void vSetDecFld(UINT32 u4VDecID,VDEC_INFO_DEC_PRM_T *prDecPrm);
void vMCSetDecFld(UINT32 u4VDecID, UINT32 u4DecFld, UINT32 u42ndFldSw);
void vSetMcDecBuf(UINT32 u4VDecID,VDEC_INFO_DEC_PRM_T *prDecPrm);
void vMCWriteToDigBuf(UINT32 u4VDecID, BOOL fgSwitch);
void vVLDSetBRef(UINT32 u4VDecID, BOOL fgPVop);
void vVLDSetBRefCoLocI(UINT32 u4VDecID, BOOL fgIVop);
void vMCSetBRef(UINT32 u4VDecID,BOOL fgIVop);
void VDecDumpMP4Register(UINT32 u4VDecID);
void VDecDumpMpegRegister(UINT32 u4VDecID,BOOL fgTriggerAB);

void vVDecWriteMP4PP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val);
UINT32 u4VDecReadMP4PP(UINT32 u4VDecID, UINT32 u4Addr);

#define u4Div2Slash(v1, v2) (((v1)+(v2)/2)/(v2))

#ifdef DUMP_REG
void VDec_DumpReg(UCHAR ucMpvId);
#endif

// *********************************************************************
// Function :void vVLDDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
// Description : Decode Picture
// Parameter : prDecPrm: Decode Parameters Set
// Return    : None
// *********************************************************************
void vVLDDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    VDEC_INFO_MPEG2_PIC_PRM_T *prMp2;
    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif
    
    
    vVLDDecPrmProc(u4VDecID, prDecPrm);
    vVLDSetPicType(u4VDecID, prDecPrm);
  
    prMp2 = &(prMpegDecPrm->rPicLayer.rMp2PicPrm);
  
    if(prMp2->ucPicStruct != FRM_PIC)
    {
        //vVDecWriteVLD(u4VDecID, RW_VLD_SCALE, 0);
        printk("<vdec> skip vVDecWriteVLD(u4VDecID, 0x9c, 0)\n");
    }
  
    vVLDDoDec(u4VDecID, prDecPrm, (prMp2->ucPicCdTp == B_TYPE),
                (prMpegDecPrm->fgB21Mode && (prMp2->ucPicStruct == FRM_PIC)));
}

void VDec_DumpMpeg4Reg(UINT32 u4VDecID)
{
    UINT32 u4Val;
    int reg;

    printk("VLD Settings\n");

    for (reg = 33; reg <= 39; reg ++)
    {
        u4Val = u4VDecReadVLD(u4VDecID, (reg << 2));
        printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
    }

    for (reg = 42; reg <= 70; reg ++)
    {
        u4Val = u4VDecReadVLD(u4VDecID, (reg << 2));
        printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
    }

    for (reg = 112; reg <= 130; reg ++)
    {
        u4Val = u4VDecReadVLD(u4VDecID, (reg << 2));
        printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
    }

    for (reg = 189; reg <= 255; reg ++)
    {
        u4Val = u4VDecReadVLD(u4VDecID, (reg << 2));
        printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
    }
    
    printk("MC Settings\n");
    
    for(reg = 0; reg <= 511; reg++)
    {          
        u4Val = u4VDecReadMC(u4VDecID, (reg << 2));
        printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
    }
}

//extern BOOL _VDecNeedDumpRegister(UINT32 u4VDecID);

// *********************************************************************
// Function :void vVLDDoDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgBPic, BOOL fgB21FrmPic)
// Description : Decode Picture
//               For Mpeg1/2, this is the last function to call to start decoding
//               For DivX3/Mpeg4, we have to call vVLDMp4TriggerDec() after this.
// Parameter : prDecPrm: Decode Parameters Set
//             fgBPic: This picture is a B-Pic
//             fgB21FrmPic: B_1/2 mode and frame pic
// Return    : None
// *********************************************************************
void vVLDDoDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgBPic, BOOL fgB21FrmPic)
{
    UINT32 u4Tmp;
    UINT32 u4Proc = 0,u4MBend = 0;
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    UINT32 u4PicW = 0,u4PicH = 0;
    #endif

	
    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif
    
    vVDecWriteVLD(u4VDecID, RW_VLD_VDOUFM, VLD_VDOUFM + VLD_ERR_MONITOR);

#if (CONFIG_DRV_FPGA_BOARD)
    vVDecWriteMC(u4VDecID, RW_MC_MODE_CTL, MC_QIU_BANK4|MC_QIU_BANK8|MC_DRAM_REQ_DELAY_1T|MC_DRAM_REQ_MERGE_OFF|MC_MV_MERGE_OFF);
  #endif  
    u4Proc |= VLD_PDHW;
    if (fgBPic)
    {
        if (prMpegDecPrm->fgIgnoreVdo)
        {
            u4Proc|= VLD_IGBCL;
        }
    #ifdef HW_BUG_VLDPROC
        u4Proc|= VLD_IGBCL;
    #endif
        
        if (fgB21FrmPic)
        {
            u4Proc |= VLD_B21EN;
            if (prMpegDecPrm->ucDecFld == MC_TOP_FLD)
            {
                u4Proc |= VLD_DECTOP;
            }
            else
            {
                u4Proc |= VLD_DECBTM;
            }
        }
        // check Partial Decode
        if (prMpegDecPrm->u4DecXOff != 0)
        {
            u4Tmp = ((prMpegDecPrm->u4DecXOff >> 4) << 8) +
                    ((prMpegDecPrm->u4DecXOff + prMpegDecPrm->u4DecW) >> 4);
            if (((prMpegDecPrm->u4DecXOff + prMpegDecPrm->u4DecW) & 0xF) != 0)
            {
                u4Tmp += 1;
            }
	     u4MBend = u4Tmp;
        }
        else
        {
            u4Tmp = (prMpegDecPrm->u4DecW >> 4);
            if ((prMpegDecPrm->u4DecW & 0xF) != 0)
            {
                u4Tmp += 1;
            }
	     u4MBend = u4Tmp;
        }
	#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	#else
        u4Proc |= u4Tmp;
	#endif
    }
    else
    {
        // I, P shall decode whole picture
       #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	u4MBend = prDecPrm->u4PicW >> 4;
	#else
        u4Proc |= (prDecPrm->u4PicW >> 4);
	#endif
    }
  
    if (prMpegDecPrm->fgRetErr)
    {
        u4Proc |= VLD_RTERR;
    }

  #ifdef RET_IF_ERR
    u4Proc |= VLD_RTERR;
  #endif
  
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) 
	u4PicW = ((prDecPrm->u4PicW +15)>>4)<<4;
	if(prDecPrm->ucPicStruct == FRM_PIC)
	{
		u4PicH = ((prDecPrm->u4PicH + 15)>>4)<<4;
	}
	else
	{
		u4PicH = (((prDecPrm->u4PicH + 15)>>4)<<4)>>1;
	}

//	vVDecWriteVLD(u4VDecID, RW_VLD_PIC_SIZE, u4PicH<<16|u4PicW);
	
       vVDecWriteVLDTOP(u4VDecID,RW_TOPVLD_WMV_PICSIZE,u4PicH<<16|u4PicW);
	vVDecWriteVLD(u4VDecID, RW_VLD_DIGMBSA, u4MBend);
	vVDecWriteMC(u4VDecID,RW_MC_MVDCAC_SEL,u4VDecReadMC(u4VDecID,RW_MC_MVDCAC_SEL) | (1<<20));
#endif

    // prevent hw gen interrupt before last frame write in dram mtk40343
    #if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8560)
    vVDecWriteMC(u4VDecID, 0x834, u4VDecReadMC(u4VDecID, 0x834)&(~(0x1<<4)));
    #endif



#if (CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8520)
    if ((prDecPrm->rDownScalerPrm.fgDSCLEn) || (prMpegDecPrm->rMpegPpInfo.fgPpEnable))
    {
        // turn off error concealment because VDSCL and PP Wrapper can not work with it
        u4Proc |= VLD_PERRCON;
    }
#else
    if (prMpegDecPrm->rMpegPpInfo.fgPpEnable)
    {
        // turn off error concealment because PP Wrapper can not work with it        
        //u4Proc |= VLD_PERRCON;
        //printk("MPV keep error concealment on when PP enable\n");   
        //after mt8530, if  enable PP, must enable error concealment to decode an error bitstream 
        //fix CQ BDP00124291
    }
#if !CONFIG_DRV_VERIFY_SUPPORT && !CONFIG_DRV_FPGA_BOARD
    {
        VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
        
        if (VDEC_MPEG1 == prMpegDecPrm->ucMpegVer)
        {
            //mpeg1 does not support error concealment
            UINT32 u4Reg = u4VDecReadVLD(u4VDecID, 156 * 4);
            u4Reg |= 0x1400D2;
            vVDecWriteVLD(u4VDecID, 156 * 4, u4Reg);
        }
    }
#endif

#endif

//    if (_VDecNeedDumpRegister(u4VDecID))
    {
//        printk("MPV Dump register before decode start\n");
//        VDecDumpMpegRegister(u4VDecID);
//        printk("\n MPV Dump register end \n");
    }

#ifdef DUMP_REG
    VDec_DumpReg(u4VDecID);
#endif
    #if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && (CONFIG_CHIP_VER_CURR <  CONFIG_CHIP_VER_MT8580))
    vVDecWriteMC(u4VDecID,RW_MC_DDR3_EN,(u4VDecReadMC(u4VDecID, RW_MC_DDR3_EN) & (~(1<<4))));
    #endif
    

    //mtk40110 Qing.Li 2010/11/25, to fix mpeg4 DCAC Pred bug
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)
    if (2 == prDecPrm->ucMpegSpecType)
    {
        //6589NEW (3)
        vVDecWriteVLD(u4VDecID, RW_VLD_DCACWK, 0x21);
#if (!MPEG4_6589_SUPPORT)
        vVDecWriteVLD(u4VDecID, RW_VLD_DCMVSEL, 0xb03008f6);
#else //MPEG4_6589_SUPPORT
        u4Tmp = u4VDecReadVLD(u4VDecID, RW_VLD_DCACWK);
        if (_u4CodecVer[u4VDecID] == VDEC_MPEG4 && _fgVerShortVideoHeader[u4VDecID] != TRUE)
        {
            u4Tmp |= (0x1 << 4);
        }
        else
        {
            u4Tmp &= ~(0x1 << 4);
        }
        vVDecWriteVLD(u4VDecID, RW_VLD_DCACWK, u4Tmp);
#endif
    }
    else if (3 == prDecPrm->ucMpegSpecType)
    {
        //6589NEW (3) DIVX3
        vVDecWriteVLD(u4VDecID, RW_VLD_DCACWK, 0x21);
#if (!MPEG4_6589_SUPPORT)
        vVDecWriteVLD(u4VDecID, RW_VLD_DCMVSEL, 0x903008f2);
#else
        u4Tmp = u4VDecReadVLD(u4VDecID, RW_VLD_DCACWK);
        u4Tmp &= ~(0x1 << 4);
        vVDecWriteVLD(u4VDecID, RW_VLD_DCACWK, u4Tmp);
#endif
    }
#endif

    //printk("<vdec> Input window is 0x%x (%s, %d)\n", u4VDecReadVLD(u4VDecID, 0x00), __FUNCTION__, __LINE__);
    // Process Decode
    if (1 == prDecPrm->ucMpegSpecType)
    {
        #if VDEC_8320_SUPPORT // MPEG2 err bitstream debug
        vVDecWriteMC(u4VDecID, 0x904, (u4VDecReadMC(u4VDecID, 0x904) | (0x3 << 4)));
        #endif
        vVDecWriteVLD(u4VDecID, RW_VLD_PROC, u4Proc);
    }
}


// *********************************************************************
// Function :void vVLDDecPrmProc(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
// Description : Process on decode related parameters
// Parameter : prDecPrm: decode related parameters
// Return    : None
// *********************************************************************
void vVLDDecPrmProc(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    UINT32 u4RegVal1;
    VDEC_INFO_MPEG2_PIC_PRM_T *prMp2Prm;
    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif

#if(CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	 if (BSP_GetIcVersion() == IC_8555)
        {
     if(u4VDecID == 1)
     {
         vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
     }
     else
     {
        vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
     }
        }
#endif
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_FLG, 0);
#if(CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	if (BSP_GetIcVersion() == IC_8555)
	{
     if(u4VDecID == 1)
     {
        vWriteReg(0x32000,(u4ReadReg(0x32000) & (~0x1)));
     }
     else
     {
        vWriteReg(0x2B000,(u4ReadReg(0x2B000) & (~0x1)));
     }
       }
#endif
     //vVDecWriteVLD(u4VDecID, RW_VLD_MP4_FLG, 0);
  
    prMp2Prm = &(prMpegDecPrm->rPicLayer.rMp2PicPrm);
    u4RegVal1 = (prMp2Prm->ucPicStruct << 30) +
               (prMp2Prm->ucFrmPredFrmDct << 29) +
               (prMp2Prm->ucConcMotVec << 28) +
               (prMp2Prm->ucQScaleType << 27) +
               (prMp2Prm->ucTopFldFirst << 26) +
               (prMp2Prm->ucFullPelFordVec << 25) +
               (prMp2Prm->ucFullPelBackVec << 24) +
               (prMp2Prm->ucPicCdTp << 21) +
               (prMp2Prm->ucIntraVlcFmt << 20) +
               (prMp2Prm->ucIntraDcPre << 18) +
               (prMp2Prm->ucAltScan << 17);
  
    if (prMpegDecPrm->ucMpegVer == 1)
    {
        u4RegVal1 = u4RegVal1 +
                 (prMp2Prm->ucFordFCode << 12) +
                 (prMp2Prm->ucFordFCode << 8) +
                 (prMp2Prm->ucBackFCode << 4) +
                 (prMp2Prm->ucBackFCode);
    }
    else
    {
        u4RegVal1 = u4RegVal1 +
                   (prMp2Prm->pucfcode[0][0] << 12) +
                   (prMp2Prm->pucfcode[0][1] << 8) +
                   (prMp2Prm->pucfcode[1][0] << 4) +
                   (prMp2Prm->pucfcode[1][1]);
    }
    vVDecWriteVLD(u4VDecID, RW_VLD_PARA, u4RegVal1);
  
    //setting of the dwMaxMblMod
    vVLDSetPicSz(u4VDecID, prDecPrm, ((prMp2Prm->ucPicCdTp == B_TYPE) &&
                            (prMp2Prm->ucPicStruct == FRM_PIC) &&
                             prMpegDecPrm->fgB21Mode));
    vVLDSetMBRowPrm(u4VDecID, prDecPrm, (prMp2Prm->ucPicCdTp == B_TYPE),
                              (prMp2Prm->ucPicStruct == FRM_PIC));
  
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, ((prDecPrm->u4PicW + 15) >> 4) << 4);
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, ((prDecPrm->u4PicH + 15) >> 4) << 4);                         
}


void vVLDSetPicType(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    UINT32 u4Value = 0;
    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif

    switch (prMpegDecPrm->ucMpegVer)
    {
        case VDEC_MPEG1:
        case VDEC_MPEG2:
            u4Value = 1 << 4;
            break;
        case VDEC_DIVX3:
            u4Value = 3 << 5;
            break;
        case VDEC_MPEG4:
            u4Value = 1 << 6;
            break;
        default:
            return;
    }
    if ((VDEC_MPEG1 == prMpegDecPrm->ucMpegVer) || (VDEC_MPEG2 == prMpegDecPrm->ucMpegVer))
    {
        VDEC_INFO_MPEG2_PIC_PRM_T *prMp2Prm = &(prMpegDecPrm->rPicLayer.rMp2PicPrm);
        if (I_TYPE == prMp2Prm->ucPicCdTp)
            u4Value |= (1 << 0);
        else if (P_TYPE == prMp2Prm->ucPicCdTp)
            u4Value |= (1 << 1);
        else if (B_TYPE == prMp2Prm->ucPicCdTp)
            u4Value |= (1 << 2);
    }
    else if ((VDEC_DIVX3 == prMpegDecPrm->ucMpegVer) || (VDEC_MPEG4 == prMpegDecPrm->ucMpegVer))
    {
        VDEC_INFO_MPEG4_DEC_PRM_T *prMp4Prm = &(prMpegDecPrm->rPicLayer.rMp4DecPrm);
    
        if (0 == prMp4Prm->ucVopCdTp)
            u4Value |= (1 << 0);
        else if (1 == prMp4Prm->ucVopCdTp)
            u4Value |= (1 << 1);
        else if (2 == prMp4Prm->ucVopCdTp)
            u4Value |= (1 << 2);
        else if (3 == prMp4Prm->ucVopCdTp)
            u4Value |= (1 << 3);
    
        if (VDEC_MPEG4 == prMpegDecPrm->ucMpegVer)
        {
            VDEC_INFO_MPEG4_VOL_PRM_T *prVolPrm = prMpegDecPrm->rPicLayer.rMp4DecPrm.rDep.rM4vDecPrm.prVol;
            if (prVolPrm->ucShortVideoHeader)
                u4Value |= (1 << 7);
        }
    }
    vVDecWriteVLD(u4VDecID, RW_VLD_VOP_TYPE, u4Value);
}


// *********************************************************************
// Function :void vVLDSetPicSz(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgB21FrmPic)
// Description : Set RW_VLD_PICSZ register
// Parameter : prDecPrm: pointer to decoder relate variables
//             fgB21FrmPic: B_1/2 mode and frame pic
// Return    : None
// *********************************************************************
void vVLDSetPicSz(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgB21FrmPic)
{
    UINT32 u4MaxMblMod;
    UINT32 u4WidthMb;
    UINT32 u4RegVal2;
    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif

    if (fgB21FrmPic)
    {
        u4MaxMblMod = prMpegDecPrm->u4MaxMbl * 2 + 1;
    }
    else
    {
        u4MaxMblMod = prMpegDecPrm->u4MaxMbl;
    }
    u4WidthMb = (prDecPrm->u4PicW + 15) >> 4;
	
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
     u4RegVal2 = ((prMpegDecPrm->ucMpegVer == VDEC_MPEG1) << 31) + ((prMpegDecPrm->ucMpegVer == VDEC_MPEG2) << 30) + ((prDecPrm->u4PicH) << 16) +
                (u4MaxMblMod & 0xFF);
    #else
    u4RegVal2 = ((prMpegDecPrm->ucMpegVer == VDEC_MPEG1) << 31) + ((prDecPrm->u4PicH + 15) << 16) +
                ((u4MaxMblMod & 0xFF) << 8) + u4WidthMb;
    #endif
	
    vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, u4RegVal2 | 0x1ff);
    //vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, 0x1ff);
}


// *********************************************************************
// Function :void vVLDSetPicSz(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgB21FrmPic)
// Description : Set RW_VLD_PICSZ register
// Parameter : prDecPrm: pointer to decoder relate variables
//             fgB21FrmPic: B_1/2 mode and frame pic
// Return    : None
// *********************************************************************
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
void vVLDMp4SetPicSz(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgB21FrmPic)
{
	UINT32 u4MaxMblMod;
	UINT32 u4WidthMbM1,u4HeightMbM1,u4Width,u4Height;
	UINT32 u4RegVal2;
		//VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
		VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
		 VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif
	
		if (fgB21FrmPic)
		{
			u4MaxMblMod = prMpegDecPrm->u4MaxMbl * 2 + 1;
		}
		else
		{
			u4MaxMblMod = prMpegDecPrm->u4MaxMbl;
		}
		
		u4WidthMbM1 = ((prDecPrm->u4PicW + 15) >> 4) - 1;
		u4HeightMbM1 = ((prDecPrm->u4PicH + 15) >> 4) - 1;
		u4Width = prDecPrm->u4PicW + 15;
		u4Height = prDecPrm->u4PicH + 15;

		u4RegVal2 = ((prMpegDecPrm->ucMpegVer == VDEC_MPEG1) << 31) + (u4MaxMblMod & 0x1FF);
		
        vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, u4RegVal2 | 0x1ff);
        //vVDecWriteVLD(u4VDecID, RW_VLD_PICSZ, 0x1ff);
//		vVDecWriteVLD(u4VDecID, RW_VLD_MB_SIZE, u4HeightMbM1<<16 | u4WidthMbM1);
//		vVDecWriteVLD(u4VDecID, RW_VLD_PIC_SIZE, u4Height<<16 | u4Width);
		vVDecWriteVLDTOP(u4VDecID,RW_TOPVLD_WMV_PICSIZE_MB,u4HeightMbM1<<16 | u4WidthMbM1);
		vVDecWriteVLDTOP(u4VDecID,RW_TOPVLD_WMV_PICSIZE,u4Height<<16 | u4Width);

}
#endif


// *********************************************************************
// Function : UINT32 u4VLDPicH(UCHAR ucMpegVer, UINT32 u4PicH)
// Description : Add 15 to picture height or not according to mpeg version
// Parameter : prDecPrm: pointer to decoder relate variables
//             u4PicH: original height
// Return    : new height
// *********************************************************************
UINT32 u4VLDPicH(UCHAR ucMpegVer, UINT32 u4PicH)
{
    if ((ucMpegVer == 1) || (ucMpegVer == 2))
    {
        return (u4PicH);
    }
    else
    {
        return (u4PicH + 15);
    }
}


// *********************************************************************
// Function :void vVLDSetMBRowPrm(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgBPic, BOOL fgFrmPic)
// Description : Set RW_VLD_MBROWPRM register
// Parameter : prDecPrm: pointer to decoder relate variables
//             fgBPic: B-picture
//             fgFrmPic: frame picture
// Return    : None
// *********************************************************************
void vVLDSetMBRowPrm(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgBPic, BOOL fgFrmPic)
{
    UINT32 u4RegVal3;
    UINT32 u4StartX, u4StartY, u4EndX, u4EndY;
    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif

    //Partial decode parameters calculation
    u4StartX = prMpegDecPrm->u4DecXOff >> 4;
    if ((prMpegDecPrm->u4DecXOff & 0xf) != 0)
    {
        u4StartX += 1;
    }
  
    u4EndX = (prMpegDecPrm->u4DecXOff + prMpegDecPrm->u4DecW - 1) >> 4;
    //UNUSED(u4EndX);
    u4StartY = prMpegDecPrm->u4DecYOff >> 4;
    if (prMpegDecPrm->u4DecYOff == 0)
    {
        u4StartY = 0;
    }
    else if ((u4StartY & 0x1) != 0)
    {
        u4StartY += 1;
    }
    u4EndY = (prMpegDecPrm->u4DecYOff + u4VLDPicH(prMpegDecPrm->ucMpegVer, prMpegDecPrm->u4DecH) - 1) >> 4;
  
    if ((prMpegDecPrm->u4DecH == prDecPrm->u4PicH) &&
       (prMpegDecPrm->u4DecYOff == 0))
    {
    #if 1 //CONFIG_MT8520_VDEC_EMU
	 #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	 u4EndY = 0x1ff;
	 #else
        u4EndY = 127;
	 #endif
    #else
        u4EndY = 63;
    #endif    
    }
    else if ((u4EndY & 0x1)==0)
    {
        if(!(prMpegDecPrm->ucMpegVer == 1 || prMpegDecPrm->ucMpegVer == 2))
        {
           u4EndY -= 1;
        }
    }
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    if (fgBPic)
    {
       if (fgFrmPic)
       {
           u4RegVal3 = (u4StartY << 16) + u4EndY;
       }
       else
       {
           u4RegVal3 = ((u4StartY / 2)  << 16) +
                      ((u4EndY - 1) / 2) ;
       }
    }
    else
    {
        u4RegVal3 = 0x1ff;
    }
    #else
    if (fgBPic)
    {
       if (fgFrmPic)
       {
           u4RegVal3 = (u4StartY << 24) + (u4EndY << 16) +
                      (prMpegDecPrm->u4BBufStart << 8);
       }
       else
       {
           u4RegVal3 = ((u4StartY / 2)  << 24) +
                      (((u4EndY - 1) / 2) << 16) +
                      (prMpegDecPrm->u4BBufStart << 8);
       }
    }
    else
    {
        u4RegVal3 = (127 << 16) + (prMpegDecPrm->u4BBufStart << 8);  
    }
    #endif
	
    //vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM, u4RegVal3);
    vVDecWriteVLD(u4VDecID, RW_VLD_MBROWPRM,  0x1ff );
}


// *********************************************************************
// Function :void vVLDM4vDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgBVop)
// Description : Decode M4v Picture
// Parameter : prDecPrm: Decode Parameters Set
//             fgBVop: B-VOP
// Return    : None
// *********************************************************************
void vVLDM4vDec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm, BOOL fgBVop)
{
    vVLDM4vDecPrmProc(u4VDecID, prDecPrm);
    vVLDSetPicType(u4VDecID, prDecPrm);
    vVLDDoDec(u4VDecID, prDecPrm, fgBVop, FALSE);
    vVLDMp4TriggerDec(u4VDecID);
}


// *********************************************************************
// Function :void vVLDM4vDecPrmProc(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
// Description : Process M4v decode related parameters
// Parameter : prDecPrm: decode related parameters
// Return    : None
// *********************************************************************
void vVLDM4vDecPrmProc(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    UINT32 u4MBx;
    UINT32 u4RegVal1;
    UINT32 u4Mp4Hdr;
    UINT32 u4DcScalarY;
    UINT32 u4DcScalarC;
    INT32 i4RegHi;
    INT32 i4RegLo;
    INT32 i4Trb;
    INT32 i4Trd;
    INT32 i4Trbi;
    INT32 i4Trdi;
    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif
    VDEC_INFO_MPEG4_DEC_PRM_T *prMp4Prm = &(prMpegDecPrm->rPicLayer.rMp4DecPrm);
    VDEC_INFO_MPEG4_VOL_PRM_T *prVolPrm = prMp4Prm->rDep.rM4vDecPrm.prVol;
    VDEC_INFO_MPEG4_VOP_PRM_T *prVopPrm = prMp4Prm->rDep.rM4vDecPrm.prVop;
    VDEC_INFO_MPEG_DIR_MODE_T *prDirMdPrm = prVopPrm->prDirMd;
    VDEC_INFO_MPEG_GMC_PRM_T *prGmcPrm = prVopPrm->prGmcPrm;

 #ifdef VDEC_SIM_DUMP
    printk("Pic %d - type %d\n", _u4FileCnt[u4VDecID], prMp4Prm->ucVopCdTp);
 #endif
 #if 1 //mtk40088 add for MPEG4 quant_scale
    UINT32 u4RegValue = 0;
    u4RegValue = u4VDecReadVLD(u4VDecID, RW_VLD_BREF);
    //6589NEW 2.2
 #if (MPEG4_6589_SUPPORT)
    u4RegValue &= ~(0x3);
    u4RegValue |= (prMp4Prm->ucVopCdTp == VCT_B && 
                   (prVopPrm->ucBRefCdTp == VCT_I || prVopPrm->ucBRefCdTp == VCT_S)) 
                   ? 0x1 : 0x0;
    u4RegValue |= (prMp4Prm->ucVopCdTp == VCT_B && 
                   prVopPrm->ucBRefCdTp == VCT_I) 
                   ? (0x1 << 1) : (0x0 << 1);    
 #endif
    vVDecWriteVLD(u4VDecID, RW_VLD_BREF, u4RegValue | 0x100);
 #endif
 #if(CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	 if (BSP_GetIcVersion() == IC_8555)
        {
	    if(u4VDecID == 1)
    {
        vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
    }
    else
    {
        vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
    }
        }
#endif
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_FLG, 1);
    //6589NEW 2.1
#if (MPEG4_6589_SUPPORT)
    u4RegVal1 = u4VDecReadVLD(u4VDecID, RW_VLD_MP4_FLG) & ~(0x1 << 9);
    u4RegVal1 |= (0x1 << 16) | (0x1 << 9);
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_FLG, u4RegVal1);
#endif

#if(CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	if (BSP_GetIcVersion() == IC_8555)
	{
    if(u4VDecID == 1)
    {
        vWriteReg(0x32000,(u4ReadReg(0x32000) & (~0x1)));
    }
    else
    {
        vWriteReg(0x2B000,(u4ReadReg(0x2B000) & (~0x1)));
    }
       }
#endif
  
    u4MBx = (prDecPrm->u4PicW + 15) / 16;
  
    u4RegVal1 = (prVopPrm->fgTopFldFirst << 26) +
                (prMp4Prm->ucVopCdTp << 21) +
                ((prVopPrm->fgAlternateVerticalScanFlag & 0x1) << 17) +
                (prVopPrm->ucFordFCode << 12) +
                (prVopPrm->ucBackFCode << 4);
    vVDecWriteVLD(u4VDecID, RW_VLD_PARA, u4RegVal1);
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVLDMp4SetPicSz(u4VDecID, prDecPrm, FALSE);
    #else
    vVLDSetPicSz(u4VDecID, prDecPrm, FALSE);
    #endif
    vVLDSetMBRowPrm(u4VDecID, prDecPrm, prMp4Prm->ucVopCdTp == VCT_B, TRUE);
    vVLDMp4DecPrmProc(u4VDecID, prMp4Prm, u4MBx, prVopPrm->ucIntraDcVlcThr,
                                      prVolPrm->ucQuarterSample,
                                      prVolPrm->ucDataPartitioned);


    u4RegVal1 = u4VDecReadVLD(u4VDecID, RW_VLD_MP4_HDR) & ~(0x0778FFFF);
    // GSLin's
    u4Mp4Hdr = (prVolPrm->ucResyncMarkerDisable << 26) +
               (prVolPrm->ucQuarterSample << 25) +               
               (prVolPrm->ucSorenson << 23 ) +  //PANDA H263
               (prVolPrm->ucQuantType << 24) +
               (prVolPrm->ucInterlaced << 22) +
               (prVolPrm->ucDataPartitioned << 21) +
               (prVolPrm->ucReversibleVlc << 20) +
               (prVolPrm->ucShortVideoHeader << 19) +
               (prVolPrm->u2VopTimeIncrementResolution);
    //6589NEW (2)
#if (MPEG4_6589_SUPPORT)
    if (prMp4Prm->ucVopCdTp == VCT_B)
    {
        u4Mp4Hdr &= ~(0x1 << 20);
    }
    u4Mp4Hdr = u4Mp4Hdr | u4RegVal1;
#endif 
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_HDR, u4Mp4Hdr);
    vVDecWriteVLD(u4VDecID, RW_VLD_M4SH, prVolPrm->ucSourceFormat);

    if (prMp4Prm->ucVopCdTp == VCT_B)
    {
        //6589NEW 4.2
        vVLDSetBRef(u4VDecID, prVopPrm->ucBRefCdTp == VCT_P);
        #if (MPEG4_6589_SUPPORT)
        vVLDSetBRefCoLocI(u4VDecID, prVopPrm->ucBRefCdTp == VCT_I);
        #endif
        vMCSetBRef(u4VDecID, prVopPrm->ucBRefCdTp == VCT_I);
    }
    else
    {
        //6589NEW 4.2
        vVLDSetBRef(u4VDecID, 1);
        #if (MPEG4_6589_SUPPORT)        
        vVLDSetBRefCoLocI(u4VDecID, 0);
        #endif
    }

    //6589NEW 5
#if (!MPEG4_6589_SUPPORT)  
    // HHKuo's
    vVDecWriteVLD(u4VDecID, RW_VLD_DIRE_MD, prDirMdPrm->u4Trb | (prDirMdPrm->u4Trd << 16));

    if (prVolPrm->ucInterlaced)
    {
        vVDecWriteVLD(u4VDecID, RW_VLD_DIRE_MD_IL, prDirMdPrm->u4Trbi | (prDirMdPrm->u4Trdi << 16));
    }
#endif

#if (MPEG4_6589_SUPPORT)
    //6589NEW 3.1.2
    u4RegVal1 = u4VDecReadMP4PP(u4VDecID, RW_PP_DIVX3_DFT_PRED) & 0xFFFF0000;
    if (prMp4Prm->ucVopQuant > 24)
    {
        u4DcScalarY = prMp4Prm->ucVopQuant * 2 - 16;
        u4DcScalarC = prMp4Prm->ucVopQuant - 6;    
    } 
    else if (prMp4Prm->ucVopQuant > 8)
    {
        u4DcScalarY = prMp4Prm->ucVopQuant + 8;
        u4DcScalarC = (prMp4Prm->ucVopQuant + 13) / 2;    
    } 
    else if (prMp4Prm->ucVopQuant > 4)
    {
        u4DcScalarY = prMp4Prm->ucVopQuant * 2;
        u4DcScalarC = (prMp4Prm->ucVopQuant + 13) / 2;
    } 
    else
    {
        u4DcScalarY = 8;
        u4DcScalarC = 8;
    }
    u4RegVal1 |= u4Div2Slash(1024, u4DcScalarY) | (u4Div2Slash(1024, u4DcScalarC) << 8);
    vVDecWriteMP4PP(u4VDecID, RW_PP_DIVX3_DFT_PRED, u4RegVal1);
    

    //6589NEW 4.5 - 4.12
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD, (prDirMdPrm->u4Trb << 16) | (prDirMdPrm->u4Trd & 0xFFFF));
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_I, (prDirMdPrm->u4Trbi << 16) | (prDirMdPrm->u4Trdi & 0xFFFF));
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_I_PLUS_1, ((prDirMdPrm->u4Trbi + 1) << 16) | ((prDirMdPrm->u4Trdi + 1) & 0xFFFF));
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_I_MINUS_1, ((prDirMdPrm->u4Trbi - 1) << 16) | ((prDirMdPrm->u4Trdi - 1) & 0xFFFF));
    i4Trb = prDirMdPrm->u4Trb;
    i4Trd = prDirMdPrm->u4Trd;
    i4Trbi = prDirMdPrm->u4Trbi;
    i4Trdi = prDirMdPrm->u4Trdi;
    if (i4Trd == 0)
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_RATIO, 0);
    }
    else
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_RATIO, (((i4Trb << 14) / i4Trd) << 16) | ((((i4Trd - i4Trb) << 14) / i4Trd) & 0xFFFF));
    }
    if (i4Trdi == 0)
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_I_RATIO, 0);
    }
    else
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_I_RATIO, (((i4Trbi << 14) / i4Trdi) << 16) | ((((i4Trdi - i4Trbi) << 14) / i4Trdi) & 0xFFFF));
    }
    if (i4Trdi == -1)
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_I_RATIO_2, 0);
    }
    else
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_I_RATIO_2, ((((i4Trbi + 1) << 14) / (i4Trdi + 1)) << 16) | ((((i4Trdi - i4Trbi) << 14) / (i4Trdi + 1)) & 0xFFFF));
    }
    if (1 == i4Trdi)
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_I_RATIO_3, 0);        
    }
    else
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_VTRBD_I_RATIO_3, ((((i4Trbi-1) << 14) / (i4Trdi - 1)) << 16) | ((((i4Trdi - i4Trbi) << 14) / (i4Trdi - 1)) & 0xFFFF));
    }
#endif
    // CCJu's
    vVDecWriteMC(u4VDecID, RW_MC_WARP_POINT_NUM, prGmcPrm->ucEffectiveWarpingPoints);
    vVDecWriteMC(u4VDecID, RW_MC_GMC_Y_MV, (prGmcPrm->i4GmcYMvX << 16) | (prGmcPrm->i4GmcYMvY & 0xffff));
    // Cheng-Jung 20120320 vcon-asp14L1.m4v fix. Old way should work too, but..
    //vVDecWriteMC(u4VDecID, RW_MC_GMC_C_MV, ((prGmcPrm->i4GmcYMvX << 15) & 0xFFFF0000) | ((prGmcPrm->i4GmcYMvY >> 1) & 0xffff)); // DIVX mode
    vVDecWriteMC(u4VDecID, RW_MC_GMC_C_MV, (prGmcPrm->i4GmcCMvX << 16) | (prGmcPrm->i4GmcCMvY & 0xffff)); // MPEG4 mode
    

#if (MPEG4_6589_SUPPORT)
    //6589NEW 4.2
    u4RegVal1 = u4VDecReadAVCMV(u4VDecID, RW_AMV_MP4_COL_IVOP) & (~0x1);
    if (prMp4Prm->ucVopCdTp == VCT_B)
    {
        u4RegVal1 |= prVopPrm->ucBRefCdTp == VCT_I ? 0x1 : 0x0;
    }
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_COL_IVOP, u4RegVal1);

    //6589NEW 4.3
    u4RegVal1 = u4VDecReadAVCMV(u4VDecID, RW_AMV_MP4_TOP_FIELD_QUART_SAMPLE) & (~0x3);
    u4RegVal1 |= ((prVopPrm->fgTopFldFirst & 0x1) << 1) |
                  (prVolPrm->ucQuarterSample & 0x1);
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_TOP_FIELD_QUART_SAMPLE, u4RegVal1);

    //6589NEW 4.4
    u4RegVal1 = u4VDecReadAVCMV(u4VDecID, RW_AMV_MP4_GMC_Y_MV);
    if (1 == (prVolPrm->ucQuarterSample & 0x1))
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_GMC_Y_MV, (prGmcPrm->i4GmcYMvX << 16) | (prGmcPrm->i4GmcYMvY & 0xffff));
    }
    else
    {
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_GMC_Y_MV, ((prGmcPrm->i4GmcYMvX >> 1) << 16) | ((prGmcPrm->i4GmcYMvY >> 1) & 0xffff));
    }

    //6589NEW (9)

#endif  
    // DivX3 only registers...
    // Maybe it's not necessary but clear them anyway
    vVDecWriteVLD(u4VDecID, RW_VLD_DX3, 0);
}

//#define TEST_SMALL_PIC //Cheng-Jung 20120305 Small pic all in SRAM mode (DCACMV, NOT CODED, LOOP)
// *********************************************************************
// Function :void vVLDMp4TriggerDec(UINT32 u4VDecID)
// Description : Start decode for Mpeg4 (only used in Mpeg4 or DivX3)
// Parameter : None
// Return    : None
// *********************************************************************
void vVLDMp4TriggerDec(UINT32 u4VDecID)
{
    UINT32 u4Reg;
#ifdef TEST_SMALL_PIC
    UINT32 u4Mbx;
    UINT32 u4Mby;
    UINT32 u4DcacMVSz;
    UINT32 u4NotCodedSz;
    UINT32 u4Addr_1;
    UINT32 u4Addr_2;
#endif    
    //vVDecWriteVLD(u4VDecID, RW_VLD_DCMVSEL, 0x903000f2);
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    UINT32 vld138;
    vld138 = u4VDecReadVLD(u4VDecID, RW_VLD_MODE);
    vld138 |= (1<<28);
    vVDecWriteVLD(u4VDecID, RW_VLD_MODE, vld138);
#endif

    //mtk40110 Qing.Li 2010/11/25, to fix mpeg4 DCAC Pred bug
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8560)
    vVDecWriteVLD(u4VDecID, RW_VLD_DCMVSEL, 0x903008f2);
#endif

#ifdef TEST_SMALL_PIC
    u4Mbx = _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW >> 4;
    u4Mby = _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecH >> 4;
    u4DcacMVSz = (8*u4Mbx*128) >> 3;
    u4NotCodedSz = ((((u4Mbx*u4Mby)+127) >> 7) << 7) >> 3;

    printk("<SMALL PIC> DecW %d DecH %d, Mbx %d, Mby %d\n", _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW, _tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecH, u4Mbx, u4Mby);    
    printk("<SMALL PIC> u4DcacMVSz %d u4NotCodedSz %d\n", u4DcacMVSz, u4NotCodedSz);    

    u4Addr_1 = (u4DcacMVSz >> 4) & 0xFFF;
    u4Addr_2 = (u4Addr_1 + (u4NotCodedSz >> 4)) & 0xFFF;

    vVDecWriteMC(u4VDecID, 0x990, u4VDecReadMC(u4VDecID, 0x990) | 0x1);
    vVDecWriteMC(u4VDecID, 0x81C, (u4VDecReadMC(u4VDecID, 0x81C) & ~(0x0FFF0000)));
    vVDecWriteMC(u4VDecID, 0x820, (u4VDecReadMC(u4VDecID, 0x820) & ~(0x0FFF0FFF)) | u4Addr_1 | (u4Addr_2 << 16));
    vVDecWriteMC(u4VDecID, 0x990, u4VDecReadMC(u4VDecID, 0x990) | (0x1 << 20));
#endif

    //200808 YN
    //To fix error bistream hang issue.
    //vVDecWriteVLD(u4VDecID, RW_VLD_DRAMMBSEL, 0x910002);
    vVDecWriteVLD(u4VDecID, RW_VLD_DRAMMBSEL, 0x1910002);

	//Close mp4 error detection
	//Cheng-Jung 20120329 [try to close it now, if there is soemthing wrong, open it back to turn off this function	
//	vVDecWriteVLD(u4VDecID,RW_VLD_MP4_ERR_MASK,(u4VDecReadVLD(u4VDecID,RW_VLD_MP4_ERR_MASK) | (0xff << 16)));
    // ]
//	vVDecWriteVLD(u4VDecID,RW_VLD_MP4_ERR_DET1,0xffffffff);
//	vVDecWriteVLD(u4VDecID,RW_VLD_MP4_ERR_DET2,0xffffffff);
	//L buffer off
	vVDecWriteCRC(u4VDecID, 60*4,u4VDecReadCRC(u4VDecID,60*4)|0x01);
	//pre-fetch priority
	vVDecWriteCRC(u4VDecID, 88*4,u4VDecReadCRC(u4VDecID,88*4)|0x01);
    //6589NEW (9) DCAC/MV predictor WCP dram burst mode
    //vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_MODE,(u4VDecReadVLDTOP(u4VDecID,RW_VLD_TOP_PRED_MODE)&(~(0x3 << 4))) | (0x2 << 4));
	#ifndef VDEC_SRAM
	//bcoded dram mode
	//6589NEW (9) bypass vld_wrapper
	vVDecWriteVLDTOP(u4VDecID,RW_VLD_TOP_PRED_SRAM_CFG,u4VDecReadVLDTOP(u4VDecID,RW_VLD_TOP_PRED_SRAM_CFG)|(1<<30));
	#endif
	//6589NEW (9) bypass VLD prefetch 
	//vVDecWriteMC(u4VDecID,RW_MC_PREFETCH,u4VDecReadMC(u4VDecID,RW_MC_PREFETCH)|(1<<28));
	//bypass MC prefetch
	//vVDecWriteMC(u4VDecID,554*4,u4VDecReadMC(u4VDecID,554*4)|(1<<16));

	//MT6589 hit029 debug
	//hw_hang_if_Err
	//vVDecWriteVLD(u4VDecID, RW_VLD_RESET_COMMOM_SRAM, u4VDecReadVLD(u4VDecID, RW_VLD_RESET_COMMOM_SRAM)|(1<<3));
	//dp read burst
	//vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION, u4VDecReadVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION)|(1<<2));
	//local bvalid cnt
	//vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION, u4VDecReadVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION)&~(1<<6));

    //6589NEW (9) If vld_wrapper bypass, PP_512[8] must set to 1'b1 to turn on pred_wr wait dram bvalid
    //vVDecWriteMP4PP(u4VDecID, 512*4, u4VDecReadMP4PP(u4VDecID,512*4) | (0x1 << 8));

    #if MPEG4_6582_SUPPORT
    if (_tVerMpvDecPrm[u4VDecID].u4PicW < 176)
    {
        vVDecWriteMP4PP(u4VDecID, 0x800, u4VDecReadMP4PP(u4VDecID, 0x800) | (0x1 << 8));
        vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_SRAM_CFG, u4VDecReadVLDTOP(u4VDecID, RW_VLD_TOP_PRED_SRAM_CFG)|(1<<30));
        vVDecWriteMC(u4VDecID, RW_MC_PREFETCH, u4VDecReadMC(u4VDecID, RW_MC_PREFETCH)|(1<<28));
        vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_MODE, (u4VDecReadVLDTOP(u4VDecID, RW_VLD_TOP_PRED_MODE) & (~(0x1<<4)))|(2<<4));
        vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION, u4VDecReadVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION)|(0x1<<2)|(0x1<<7));
        vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DCAC_NOT_CODED_SZ, (u4VDecReadVLD(u4VDecID, RW_VLD_MP4_DCAC_NOT_CODED_SZ) & (~(0xFFFF << 16))) | ((6*((_tVerMpvDecPrm[u4VDecID].u4PicW + 15) >> 4)) << 16));
    }
    else
    {
        //6582 test L2$
#if MT6582_L2_EMULATION == 2
        vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_MV_DC_PRED_END_ADDR, 0x00200000 + (6*(((_tVerMpvDecPrm[u4VDecID].u4PicW + 15) >> 4) << 4)) - 16);
#else
        vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_MV_DC_PRED_END_ADDR, u4AbsDramANc(_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rMpeg4WorkBufSa.u4DcacSa) + (6*(((_tVerMpvDecPrm[u4VDecID].u4PicW + 15) >> 4) << 4)) - 16);
#endif
        vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_SRAM_CFG, (u4VDecReadVLDTOP(u4VDecID, RW_VLD_TOP_PRED_SRAM_CFG) & (~(0x1<<30))));
        vVDecWriteMC(u4VDecID, RW_MC_PREFETCH, u4VDecReadMC(u4VDecID, RW_MC_PREFETCH) & (~(0x1<<28)));
        //vVDecWriteMC(u4VDecID, RW_MC_PREFETCH, u4VDecReadMC(u4VDecID, RW_MC_PREFETCH)|(1<<28));
        vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_MODE, (u4VDecReadVLDTOP(u4VDecID, RW_VLD_TOP_PRED_MODE) & (~(0x3<<4))));
        vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION, (u4VDecReadVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION) & (~(0x1<<2))) | (0x1<<7));
        vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DCAC_NOT_CODED_SZ, (u4VDecReadVLD(u4VDecID, RW_VLD_MP4_DCAC_NOT_CODED_SZ) & (~(0xFFFF << 16))) | ((6*((_tVerMpvDecPrm[u4VDecID].u4PicW + 15) >> 4)) << 16));
        // Need setting VLD_191 if VLD_180[7] = 0, but 6582 does not support this yet..
        //vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DCAC_NOT_CODED_LOOP_SZ, (_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW >> 4) | (((_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW >> 4) * 5) << 16));

        vVDecWriteMP4PP(u4VDecID, 0x804, u4VDecReadMP4PP(u4VDecID, 0x804) & ~(0x1 << 4));
        vVDecWriteMP4PP(u4VDecID, 0x754, u4VDecReadMP4PP(u4VDecID, 0x754) | 0x1);        
        vVDecWriteVLD(u4VDecID, RW_VLD_MP4_ERR_MASK, u4VDecReadVLD(u4VDecID, RW_VLD_MP4_ERR_MASK)|(0x1 << 1));
#if MPEG4_6582_NORMAL_DEBUG_MODE == 1
        vVDecWriteMC(u4VDecID, RW_MC_PREFETCH, u4VDecReadMC(u4VDecID, RW_MC_PREFETCH) | (0x1<<28));
        //vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION, (u4VDecReadVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION) & (~(0x1<<7))));
        //vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DCAC_NOT_CODED_LOOP_SZ, (_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW >> 4) | (((_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW >> 4) * 5) << 16));
#elif MPEG4_6582_NORMAL_DEBUG_MODE == 2
        vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_SRAM_CFG, (u4VDecReadVLDTOP(u4VDecID, RW_VLD_TOP_PRED_SRAM_CFG) | (0x3<<26)));
        //vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION, (u4VDecReadVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION) & (~(0x1<<7))));
        //vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DCAC_NOT_CODED_LOOP_SZ, (_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW >> 4) | (((_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.u4DecW >> 4) * 5) << 16));
#endif
    }
    #endif
    

    //6589NEW (4) Error detection and concealment
    //#ifdef MPEG4_6589_ERROR_CONCEAL
    vVDecWriteVLD(u4VDecID, RW_VLD_TIMEOUT, 0x100000); // if doom_data_partition_full_frame.m4v decoding fail at first pic, try increase this number for its large data partition
    vVDecWriteVLD(u4VDecID, RW_VLD_VDOUFM, u4VDecReadVLD(u4VDecID, RW_VLD_VDOUFM) | (0x3 << 9));
    //#endif

    #if MPEG4_6582_SUPPORT 
    // Verification only, not for driver [
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_ERR_MASK, u4VDecReadVLD(u4VDecID, RW_VLD_MP4_ERR_MASK) & (~(0x7 << 11)));
    //vVDecWriteVLD(u4VDecID, RW_VLD_MP4_ERR_MASK, u4VDecReadVLD(u4VDecID, RW_VLD_MP4_ERR_MASK)|(0x7 << 11));
    // ]
    
    u4Reg = u4VDecReadVLD(u4VDecID, RW_VLD_MP4_ERR_DET1) & (~(0x1 << 20));
    u4Reg |= (_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rDep.rM4vDecPrm.prVol->ucSorenson & 1) << 20;
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_ERR_DET1, u4Reg);
    u4Reg = u4VDecReadVLD(u4VDecID, RW_VLD_MP4_ERR_DET2) | ((_tVerMpvDecPrm[u4VDecID].SpecDecPrm.rVDecMPEGDecPrm.rPicLayer.rMp4DecPrm.rDep.rM4vDecPrm.prVol->ucShortVideoHeader & 1) << 11);
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_ERR_DET2, u4Reg);    
    #endif

    // Error detection off 
    /*
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_ERR_DET1, 0xFFFFFFFF);
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_ERR_DET2, 0xFFFFFFFF);
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_ERR_MASK, u4VDecReadVLD(u4VDecID, RW_VLD_MP4_ERR_MASK)|(0xFF << 16));
    vVDecWriteVLD(u4VDecID, RW_VLD_VDOUFM, u4VDecReadVLD(u4VDecID, RW_VLD_VDOUFM) & (~(0x3 << 9)));
    */
    
    //40/12 random error try, write out last mb row to SRAM
    //vVDecWriteMP4PP(u4VDecID, 0x804, u4VDecReadMP4PP(u4VDecID, 0x804) & ~(0x1 << 4));
    //vVDecWriteMP4PP(u4VDecID, 0x754, u4VDecReadMP4PP(u4VDecID, 0x754) | 0x1);

    //vVDecWriteMP4PP(u4VDecID, 0x804, u4VDecReadMP4PP(u4VDecID, 0x804) | (0x1 << 5));
    //vVDecWriteMP4PP(u4VDecID, 0x754, u4VDecReadMP4PP(u4VDecID, 0x754) | (0x1 << 1));

	#if (CONFIG_DRV_FPGA_BOARD)
	//vVDecWriteAVCVLD(u4VDecID,RW_VLD_BS_SPEEDUP,2000);
	#endif

    #if 0
    // Debug test - latency buf bypass on
    vVDecWriteDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS, u4VDecReadDV(u4VDecID, VDEC_DV_LAT_BUF_BYPASS) |0x1);

    // Debug - stop at pic1
    if (_u4FileCnt[u4VDecID] == 0)
    {
        vVDecWriteMC(u4VDecID, 0x664, (u4VDecReadMC(u4VDecID, 0x664) & (~0x1FFFFFF)) | 0x1);
    }
    #endif
	
	
#if CONFIG_DRV_VERIFY_SUPPORT 
    #if (DUMP_ERROR == 0)
    if (_u4FileCnt[u4VDecID] == _u4DumpRegPicNum[u4VDecID]) 
    #endif
    {
       //vVDEC_HAL_MPEG_VDec_DumpReg(u4VDecID, TRUE);
       #if 0
       {
       		vVDecWriteMC(u4VDecID,409*4,(5 << 16)|(10 << 4) | 0x01);
       }
	   #endif
       VDecDumpMpegRegister(u4VDecID,0);
    }
#endif

#if 0
    UINT32 u4Val;
    BOOL fgLogReg = FALSE;
    int reg;
    int vldstart = 42;
    int vldsize = 400;
    int mcstart = 0;
    int mcsize = 400;

    if (fgLogReg == TRUE)
    {
	 printk("VLD Before Settings\n");
        for(reg= vldstart; reg < (vldstart+vldsize); reg++)
        {          
            u4Val = u4VDecReadVLD(u4VDecID, (reg << 2));
            printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
        }
        
	 printk("MC Before Settings\n");
        for(reg= mcstart; reg < (mcstart+mcsize); reg++)
        {          
            u4Val = u4VDecReadMC(u4VDecID, (reg << 2));
            printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
        }
    }
#endif

    // For Mpeg4, to start decode we have to write this register as follows
    #ifdef REG_LOG_NEW
    #ifndef MPEG4_6589_ERROR_CONCEAL
    #if (DUMP_ERROR == 0)
    if (_u4FileCnt[u4VDecID] == _u4DumpRegPicNum[u4VDecID])
    #endif
    {
        _u4RegisterLogLen[u4VDecID] = 0;
        _fgRegLogConsole[u4VDecID] = FALSE;
        VDecDumpMP4Register(u4VDecID);
        _fgRegLogConsole[u4VDecID] = TRUE;
    }
    #endif
    #endif
    vVDecWriteVLD(u4VDecID, RW_VLD_DECSTART, 1);
    vVDecWriteVLD(u4VDecID, RW_VLD_DECSTART, 0);
}


// *********************************************************************
//void vVLDMp4DecPrmProc(UINT32 u4VDecID, VDEC_INFO_MPEG4_DEC_PRM_T *prMp4Prm,
//                       UINT32 u4MBx,
//                       UCHAR ucIntraDcVlcThr,
//                       UCHAR ucQuarterSample,
//                       UCHAR ucDataPartitioned)
// Description : write Mpeg4/DivX3 common registers
// Parameter : prMp4Prm: mp4 decode parameters
//             u4MBx: width in MB
//             ucIntraDcVlcThr: intra_dc_vlc_thr
//             ucQuarterSample: quarter_sample
//             ucDataPartitioned: data_partitioned
// Return    : None
// *********************************************************************
void vVLDMp4DecPrmProc(UINT32 u4VDecID, VDEC_INFO_MPEG4_DEC_PRM_T *prMp4Prm,
                                UINT32 u4MBx,
                                UCHAR ucIntraDcVlcThr,
                                UCHAR ucQuarterSample,
                                UCHAR ucDataPartitioned)
{
    UINT32 u4DcacPitch;
    UINT32 u4NonPBit;
    UINT32 u4Temp = 0;
    UINT32 u4Mbstart_Dcac_Switch = 0;
  #ifdef VDEC_MPEG4_SUPPORT_RESYNC_MARKER 
    UINT32 u4ResyncMarkMB = 0;
  #endif    
  
    // GSLin's
    vVLDBarl2Vdec(u4VDecID);
    vVDecWriteVLD(u4VDecID,RW_VLD_LDSH,0x0);
    vVDecWriteVLD(u4VDecID, RW_VLD_QUANT, (ucIntraDcVlcThr << 8) + prMp4Prm->ucVopQuant);

    //6589NEW 1.1, 5
    // HHKuo's
    //6582 test L2$
#if MT6582_L2_EMULATION == 2
    vVLDSetDcacAddr(u4VDecID, 0x0020000);    
#else
    vVLDSetDcacAddr(u4VDecID, u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4DcacSa));
#endif


  #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) 
    u4DcacPitch =       (prMp4Prm->rMpeg4WorkBufSize.u4DcacSize >> 4) |
				    (8 << 24) |  // bit per pixel
                                (0 << 28) |  // only write 1 in DivX 3.11 1st picture
                                (ucQuarterSample << 29) |
  #ifdef WRITE_FULL_DCAC_DATA
                                (1 << 30) |   // write full dcac data to DRAM
  #else
                                (0 << 30) |   // only write 1 line data to DRAM
  #endif
                                (ucDataPartitioned << 31);
   //6589NEW 5
  #if (!MPEG4_6589_SUPPORT)
   vVDecWriteVLD(u4VDecID, RW_VLD_DCACPITCH, u4DcacPitch);
   vVDecWriteVLD(u4VDecID, RW_VLD_MVDRAM, (u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4MvecSa) >> 2));
   vVDecWriteVLD(u4VDecID, RW_VLD_MB1SA, (u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4Bmb1Sa) >> 2));	
   vVDecWriteVLD(u4VDecID, RW_VLD_MB2SA, (u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4Bmb2Sa) >> 2));	
   vVDecWriteVLD(u4VDecID, RW_VLD_COSA, (u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4BcodeSa) >> 2));
   u4Temp = (u4MBx * 4) |((u4MBx * 4*3) << 16);
   vVDecWriteVLD(u4VDecID, RW_VLD_ADDREXTEND, u4Temp);
   #if 0
   vVDecWriteVLD(u4VDecID, RW_VLD_MVSA, (u4MBx * 4));
   vVDecWriteVLD(u4VDecID, RW_VLD_BMB1, (u4MBx * 4));
   vVDecWriteVLD(u4VDecID, RW_VLD_BMB2, (u4MBx * 4));
   #else
   vVDecWriteVLD(u4VDecID, RW_VLD_MVSA, ((prMp4Prm->rMpeg4WorkBufSize.u4MVSize>> 4)<<16) | (u4MBx * 4));
   vVDecWriteVLD(u4VDecID, RW_VLD_BMB1, ((prMp4Prm->rMpeg4WorkBufSize.u4MB1Size>> 4)<<16) | (u4MBx * 4));
   vVDecWriteVLD(u4VDecID, RW_VLD_BMB2, ((prMp4Prm->rMpeg4WorkBufSize.u4MB2Size>> 4)<<16) | (u4MBx * 4));
   #endif
   vVDecWriteVLD(u4VDecID, RW_VLD_BCODE_SA, ((prMp4Prm->rMpeg4WorkBufSize.u4BcodeSize >> 4)<<16) | 0 << 1);//????
   vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER, PHYSICAL(prMp4Prm->rMpeg4WorkBufSa.u4VldWrapperSa));
   vVDecWriteMC(u4VDecID, RW_MC_PP_WRAPPER, PHYSICAL(prMp4Prm->rMpeg4WorkBufSa.u4PPWrapperSa));
//   vVDecWriteVLD(u4VDecID, RW_VLD_BCODE_SA,  0 << 1);//????
  #endif // !MPEG4_6589_SUPPORT
  //6582NEW PP wrapper
  #if (MPEG4_6582_SUPPORT)
   //6582 test L2$
  #if MT6582_L2_EMULATION == 2
    vVDecWriteMC(u4VDecID, RW_MC_PP_WRAPPER, 0x00205A00);
  #else
    vVDecWriteMC(u4VDecID, RW_MC_PP_WRAPPER, PHYSICAL(prMp4Prm->rMpeg4WorkBufSa.u4PPWrapperSa));
  #endif
   
  #endif
   
  #else
    u4DcacPitch = (u4MBx * 4) |  // MBx * 4
                                ((u4MBx * 4 * 3) << 12) |  // MBx * 4 * 3
                                (8 << 24) |  // bit per pixel
                                (0 << 28) |  // only write 1 in DivX 3.11 1st picture
                                (ucQuarterSample << 29) |
  #ifdef WRITE_FULL_DCAC_DATA
                                (1 << 30) |   // write full dcac data to DRAM
  #else
                                (0 << 30) |   // only write 1 line data to DRAM
  #endif
                 (ucDataPartitioned << 31);
    vVDecWriteVLD(u4VDecID, RW_VLD_DCACPITCH, u4DcacPitch);
    //vVDecWriteVLD(u4VDecID, RW_VLD_MVSA, (PHYSICAL(prMp4Prm->rMpeg4WorkBufSa.u4MvecSa) >> 2) | ((u4MBx * 4) << 22));
    vVDecWriteVLD(u4VDecID, RW_VLD_MVSA, ((u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4MvecSa) >> 2) & 0x3FFFFF) | ((u4MBx * 4) << 22)); 
    u4Temp = (((u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4MvecSa) >> 2) >> 22) & 0x3F);
    //vVDecWriteVLD(u4VDecID, RW_VLD_ADDREXTEND, (u4VDecReadVLD(u4VDecID, RW_VLD_ADDREXTEND) & 0xFFFFFFC0) |
    //(((PHYSICAL(prMp4Prm->rMpeg4WorkBufSa.u4MvecSa) >> 2) >> 22) & 0x3F)); 
    //vVDecWriteVLD(u4VDecID, RW_VLD_BMB1, (PHYSICAL(prMp4Prm->rMpeg4WorkBufSa.u4Bmb1Sa) >> 2) | ((u4MBx * 4) << 22));
    vVDecWriteVLD(u4VDecID, RW_VLD_BMB1, ((u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4Bmb1Sa) >> 2) & 0x3FFFFF) | ((u4MBx * 4) << 22)); 
    u4Temp |= ((((u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4Bmb1Sa) >> 2) >> 22) & 0x3F) << 8);
    //vVDecWriteVLD(u4VDecID, RW_VLD_ADDREXTEND, (u4VDecReadVLD(u4VDecID, RW_VLD_ADDREXTEND) & 0xFFFFC0FF) |
    //((((PHYSICAL(prMp4Prm->rMpeg4WorkBufSa.u4Bmb1Sa) >> 2) >> 22) & 0x3F) << 8)); 
    //vVDecWriteVLD(u4VDecID, RW_VLD_BMB2, (PHYSICAL(prMp4Prm->rMpeg4WorkBufSa.u4Bmb2Sa) >> 2) | ((u4MBx * 4) << 22));
    vVDecWriteVLD(u4VDecID, RW_VLD_BMB2, ((u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4Bmb2Sa) >> 2) & 0x3FFFFF) | ((u4MBx * 4) << 22));
    u4Temp |= ((((u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4Bmb2Sa) >> 2) >> 22) & 0x3F) << 16);
    vVDecWriteVLD(u4VDecID, RW_VLD_ADDREXTEND, u4Temp);
    //vVDecWriteVLD(u4VDecID, RW_VLD_ADDREXTEND, (u4VDecReadVLD(u4VDecID, RW_VLD_ADDREXTEND) & 0xFFC0FFFF) |
    //((((PHYSICAL(prMp4Prm->rMpeg4WorkBufSa.u4Bmb2Sa) >> 2) >> 22) & 0x3F) << 16)); 
  
    // Modified by C.K. Hu 20040617. Porting from 1389_5039.
  
    //if (dwSetDDR == 0) // for SDR
    //{
      // bit 25: wait for wdle, required for 8105 DDR, and OK for 1389
      vVDecWriteVLD(u4VDecID, RW_VLD_BCODE_SA, ((u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4BcodeSa) >> 2) & 0xFFFFFF) | (0 << 25)
      | (((u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4BcodeSa) >> 2) & 0xF000000) << 4));
#endif // (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) 

    //6589NEW 5
    #if (!MPEG4_6589_SUPPORT)
      if (0 == prMp4Prm->ucVopCdTp)// for 8520
          u4NonPBit = 1 << 18;
      else if ((1 == prMp4Prm->ucVopCdTp) || (2 == prMp4Prm->ucVopCdTp))
          u4NonPBit = 0 << 18;
      else
          u4NonPBit = u4VDecReadVLD(u4VDecID, RW_VLD_DCMVSEL) & (1 << 18);
        
      u4DcacPitch = (0 << 0)  |  //turn on reset direction_busy
             (1 << 1)  |  //turn on idctbusy + VWRDRAMDCACST[0]
             (0 << 6)  |  //dcacbusy wait for wdle (required for 8105 DDR)
             (0 << 7)  |  //dcacbusy4is wait for wdle (required for 8105 DDR)
             (1<<11)   |
             (0 << 16) |  //mv_dram reset by 2D reset
             (0 << 17) |  //turn on mv_modb1
             u4NonPBit  |  //none_P_bitstream mode
             (0 << 19) |  //turn on sram
             (0 << 20) |  //mvbusy wait for wdle (required for 8105 DDR)
             (0 << 21);   //turn 3t period without mem_pause;
      if (1) // for data-partitioned, but can turn on in general case
      {
          u4DcacPitch |= (1 << 4) |  //turn on dcacbusy + VWRDRAMDCACST[2]
                  (1 << 5);   //turn on dcacbusy4is + VWRDRAMDCACST[2]
      }
      u4DcacPitch |= (((UINT32)1) << 31); // for 8520
      vVDecWriteVLD(u4VDecID, RW_VLD_DCMVSEL, u4DcacPitch);
      u4DcacPitch = (0 << 0) | //vmbbusy is busy until data write2DRAM, not Write2WRFIFO.
             (0 << 1) | //vmbbusy wait for wdle (required for 8105 DDR)
             (0 << 16)| //stop vld_state by DCACFULL
             (0 << 17)| //trun off reset dramctrl when mc_abort
             (1 << 18)| //stop vld_state by VDramWrVFull
             (0 << 20); //check wr3fifo full when Wdle (required for 8105 DDR)
      u4DcacPitch |= (1 << 23); // for 8520
      vVDecWriteVLD(u4VDecID, RW_VLD_DRAMMBSEL, u4DcacPitch);
    #endif
      u4Mbstart_Dcac_Switch = u4VDecReadVLD(u4VDecID, RW_MBSTART_DCAC_SWITCH);
      u4Mbstart_Dcac_Switch |= (1<<30);

#ifdef VDEC_MPEG4_SUPPORT_RESYNC_MARKER 
      //Enable resync mark ECO.
      if(_u4ResyncMarkerCnt[u4VDecID] != 0)
      {
          u4Mbstart_Dcac_Switch |= (VLD_RESYNC_MARK_ECO);
      }
      

      //Set resync mark Mbx, Mby.
      if (_u4ResyncMarkerCnt[u4VDecID] == 1)
      {
         u4ResyncMarkMB |= ((_u4ResyncMarkMbx[u4VDecID][0]) << 16);
         u4ResyncMarkMB |= ((_u4ResyncMarkMby[u4VDecID][0]) );

         vVDecWriteVLD(u4VDecID, RW_VLD_RESYNC_MARK, u4ResyncMarkMB);
      }
      else
      if (_u4ResyncMarkerCnt[u4VDecID] == 2)      
      {
         u4ResyncMarkMB |= ((_u4ResyncMarkMbx[u4VDecID][0]) << 16);
         u4ResyncMarkMB |= ((_u4ResyncMarkMby[u4VDecID][0]));
         
         u4ResyncMarkMB |= ((_u4ResyncMarkMbx[u4VDecID][1]) << 24);
         u4ResyncMarkMB |= ((_u4ResyncMarkMby[u4VDecID][1]) << 8);      
         
         vVDecWriteVLD(u4VDecID, RW_VLD_RESYNC_MARK, u4ResyncMarkMB);
      }
      
#endif
      
      vVDecWriteVLD(u4VDecID, RW_MBSTART_DCAC_SWITCH, u4Mbstart_Dcac_Switch);

#if (MPEG4_6589_SUPPORT)
      //6589NEW 2.3-2.6
      u4Temp = u4VDecReadVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION_SZ) & 0xFFF00000;
      u4Temp |= ((prMp4Prm->rMpeg4WorkBufSize.u4DataPartitionSize >> 4) & 0x000FFFFF);
      vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION_SZ, u4Temp);      
      vVDecWriteVLD(u4VDecID, RW_VLD_MP4_NOT_CODED_SA, u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4NotCodedSa));
      vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DATA_PARTITION_SA, u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4DataPartitionSa));
      vVDecWriteVLD(u4VDecID, RW_VLD_MP4_DCAC_NOT_CODED_SZ, (((prMp4Prm->rMpeg4WorkBufSize.u4NotCodedSize >> 4) & 0xFFFF) | ((prMp4Prm->rMpeg4WorkBufSize.u4DcacSize >> 4) << 16)));            

      //6589NEW 4.1
      vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_MV_DIRECT_SA, ((u4AbsDramANc(prMp4Prm->rMpeg4WorkBufSa.u4MvDirectSa) >> 4) & 0x0FFFFFFF));
#endif

    //} // Mark DDR setting by C.K. Hu. 20050509
        // Because HW will decode error for DDR setting even DRAM is DDR, so use SDR setting for DDR DRAM
    //else // for DDR
    //{
      // bit 25: wait for wdle, required for 8105 DDR, and OK for 1389
    //  vWriteVLD(RW_VLD_BCODE_SA, (dwAbsDramANc(BCODE_SA) >> 2) | (1 << 25));
    //  u4NonPBit = dReadVLD(RW_VLD_DCMVSEL) & (1 << 18);
    //  u4DcacPitch = (0 << 0)  |  //turn on reset direction_busy
    //         (1 << 1)  |  //turn on idctbusy + VWRDRAMDCACST[0]
    //         (1 << 6)  |  //dcacbusy wait for wdle (required for 8105 DDR)
    //         (1 << 7)  |  //dcacbusy4is wait for wdle (required for 8105 DDR)
    //         (0 << 16) |  //mv_dram reset by 2D reset
    //         (0 << 17) |  //turn on mv_modb1
    //         u4NonPBit  |  //none_P_bitstream mode
    //         (0 << 19) |  //turn on sram
    //         (1 << 20) |  //mvbusy wait for wdle (required for 8105 DDR)
    //         (0 << 21);   //turn 3t period without mem_pause;
    //  if(1) // for data-partitioned, but can turn on in general case
    //  {
    //    u4DcacPitch |= (1 << 4) |  //turn on dcacbusy + VWRDRAMDCACST[2]
    //            (1 << 5);   //turn on dcacbusy4is + VWRDRAMDCACST[2]
    //  }
    //  vWriteVLD(RW_VLD_DCMVSEL, u4DcacPitch);
    //  u4DcacPitch = (0 << 0) | //vmbbusy is busy until data write2DRAM, not Write2WRFIFO.
    //         (1 << 1) | //vmbbusy wait for wdle (required for 8105 DDR)
    //         (0 << 16)| //stop vld_state by DCACFULL
    //         (0 << 17)| //trun off reset dramctrl when mc_abort
    //         (1 << 18)| //stop vld_state by VDramWrVFull
    //         (1 << 20); //check wr3fifo full when Wdle (required for 8105 DDR)
    //  vWriteVLD(RW_VLD_DRAMMBSEL, u4DcacPitch);
    //}
  
    // CCJu's
  //  vWriteMC(RW_MC_QPEL_TYPE, TYPE_14496);
    vVDecWriteMC(u4VDecID, RW_MC_QPEL_TYPE, prMp4Prm->u4QPelType);
    //6582 MPEG4 mode sync C-model fix
    //vVDecWriteMC(u4VDecID, RW_MC_QPEL_TYPE, TYPE_MOMUSYS);
    
    vVDecWriteMC(u4VDecID, RW_MC_QUARTER_SAMPLE, ucQuarterSample);
    vVDecWriteMC(u4VDecID, RW_MC_ROUNDING_CTRL, prMp4Prm->ucVopRoundingType);
  
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, prMp4Prm->u4UmvPicW);
    vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, prMp4Prm->u4UmvPicH);
  
    vVDecWriteMC(u4VDecID, RW_MC_CBCR_MV_TYPE, prMp4Prm->u4CMvType);
  
    // Modified by C.K. Hu 20040617. Porting from 1389_5039
    u4DcacPitch = u4VDecReadMC(u4VDecID, RW_MC_BREF);
    u4DcacPitch |= 0x10;
    vVDecWriteMC(u4VDecID, RW_MC_BREF, u4DcacPitch);    
}


// *********************************************************************
// Function : void vVLDBarl2Vdec(UINT32 u4VDecID)
// Description : Load sum from barrel shifter to VDec HW
// Parameter : None
// Return    : None
// *********************************************************************
void vVLDBarl2Vdec(UINT32 u4VDecID)
{
    // hw workaround
    // wait hw stable before load sum to mpeg4 mode
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	u4VDecReadVLD(u4VDecID,RO_VLD_BARL);
    //wait input window stable
	while(!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&0x1));
	while((u4VDecReadVLD(u4VDecID, RO_VLD_SUM + (0 << 10)) & 0x20));
#else
    u4VDecReadVLD(u4VDecID,RO_VLD_BARL);
    while (!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL + (0 << 10))&0x10000));
    while(!(u4VDecReadVLD(u4VDecID, RO_VLD_FETCHOK + (0 << 10)) & VLD_DRAM_REQ_FIN));
    while((u4VDecReadVLD(u4VDecID, RO_VLD_SUM + (0 << 10)) & 0x20));
#endif    
    vVDecWriteVLD(u4VDecID, RW_VLD_LDSR, 1);
    vVDecWriteVLD(u4VDecID, RW_VLD_LDSR, 0);
}


// *********************************************************************
// Function :void vVLDSetDcacAddr(UINT32 u4VDecID, UINT32 dAddr)
// Description : Set DCAC area start address
// Parameter : None
// Return    : None
// *********************************************************************
void vVLDSetDcacAddr(UINT32 u4VDecID, UINT32 dAddr)
{
#if (MPEG4_6589_SUPPORT)
    vVDecWriteVLDTOP(u4VDecID, RW_VLD_TOP_PRED_ADDR, dAddr);
#else   
    vVDecWriteVLD(u4VDecID, RW_VLD_DCACSA, (dAddr >> 2));
#endif
}


// *********************************************************************
// Function :void vVLDDx3Dec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
// Description : Decode DivX3 Picture
// Parameter : prDecPrm: Decode Parameters Set
// Return    : None
// *********************************************************************
void vVLDDx3Dec(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    vVLDDx3DecPrmProc(u4VDecID, prDecPrm);
    vVLDSetPicType(u4VDecID, prDecPrm);
    vVLDDoDec(u4VDecID, prDecPrm, FALSE, FALSE);
    vVLDMp4TriggerDec(u4VDecID);
}


// *********************************************************************
// Function :void vVLDDx3DecPrmProc(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDx3Prm)
// Description : process DivX3 decode parameters
// Parameter : prDx3Prm: DivX3 decode related parameters
// Return    : None
// *********************************************************************
void vVLDDx3DecPrmProc(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    UINT32 u4RegVal1;
    UINT32 u4DcScalarY;
    UINT32 u4DcScalarC;

    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif
    VDEC_INFO_MPEG4_DEC_PRM_T *prMp4Prm  = &(prMpegDecPrm->rPicLayer.rMp4DecPrm);
    VDEC_INFO_DIVX3_PIC_PRM_T *prDx3Prm = &(prMp4Prm->rDep.rDx3DecPrm);
    UINT32 u4Dx3Hdr;

#ifdef VDEC_SIM_DUMP
   printk("Pic (divx3) %d\n", _u4FileCnt[u4VDecID]);
#endif
#if 1 //mtk40088 add for MPEG4 quant_scale
   UINT32 u4RegValue = 0;
   u4RegValue = u4VDecReadVLD(u4VDecID, RW_VLD_BREF);
   //6589NEW 2.2 DIVX3
#if (MPEG4_6589_SUPPORT)
   u4RegValue &= ~(0x3);
#endif
   vVDecWriteVLD(u4VDecID, RW_VLD_BREF, u4RegValue);
#endif

#if(CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)  
	 if (BSP_GetIcVersion() == IC_8555)
        {
	    if(u4VDecID == 1)
    {
        vWriteReg(0x32000,(u4ReadReg(0x32000) | 0x1));
    }
    else
    {
        vWriteReg(0x2B000,(u4ReadReg(0x2B000) | 0x1));
    }
        }
#endif
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_FLG, 0x101);
    //6589NEW 2.1 DIVX3
#if (MPEG4_6589_SUPPORT)
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_FLG, u4VDecReadVLD(u4VDecID, RW_VLD_MP4_FLG) | (0x1 << 16));
#endif

#if(CONFIG_CHIP_VER_CURR == CONFIG_CHIP_VER_MT8555)
	if (BSP_GetIcVersion() == IC_8555)
	{
    if(u4VDecID == 1)
    {
        vWriteReg(0x32000,(u4ReadReg(0x32000) & (~0x1)));
    }
    else
    {
        vWriteReg(0x2B000,(u4ReadReg(0x2B000) & (~0x1)));
    }
       }
#endif
    vVDecWriteVLD(u4VDecID, RW_VLD_PARA, (prMp4Prm->ucVopCdTp << 21));
    #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
    vVLDMp4SetPicSz(u4VDecID, prDecPrm, FALSE);
    #else
    vVLDSetPicSz(u4VDecID, prDecPrm, FALSE);
    #endif
    vVLDSetMBRowPrm(u4VDecID, prDecPrm, FALSE, TRUE);
    vVLDMp4DecPrmProc(u4VDecID, prMp4Prm, (prDecPrm->u4PicW + 15) / 16, 0, 0, 0);
  
    u4Dx3Hdr = (prDx3Prm->ucAltIAcChromDctIdx) |
              (prDx3Prm->ucAltIAcChromDct << 1) |
              (prDx3Prm->ucAltIAcLumDctIdx << 2) |
              (prDx3Prm->ucAltIAcLumDct << 3) |
              (prDx3Prm->ucAltIDcDct << 4) |
              (prDx3Prm->ucAltPAcDctIdx << 8) |
              (prDx3Prm->ucAltPAcDct << 9) |
              (prDx3Prm->ucAltPDcDct << 10) |
              (prDx3Prm->ucAltMv << 11) |
              (prDx3Prm->ucHasSkip << 16) |
              (prDx3Prm->ucFrameMode << 20);
    vVDecWriteVLD(u4VDecID, RW_VLD_DX3, u4Dx3Hdr);
    vVDecWriteVLD(u4VDecID, RW_VLD_DVX_BOUND_MODE,0);
    vVDecWriteVLD(u4VDecID, RW_VLD_DVX_BOUND_R1,(prDx3Prm->ucSliceBoundary[0] | (prDx3Prm->ucSliceBoundary[1] << 16)));
    vVDecWriteVLD(u4VDecID, RW_VLD_DVX_BOUND_R2,(prDx3Prm->ucSliceBoundary[2] | (prDx3Prm->ucSliceBoundary[3] << 16)));
    vVDecWriteVLD(u4VDecID, RW_VLD_DVX_BOUND_R3,prDx3Prm->ucSliceBoundary[4]);
    // Non-DivX3 (MPEG4-only) registers...
    // Maybe it's not necessary but clear them anyway
    // GSLin's
    vVDecWriteVLD(u4VDecID, RW_VLD_MP4_HDR, 0);
    vVDecWriteVLD(u4VDecID, RW_VLD_M4SH, 0);
    // HHKuo's
    vVDecWriteVLD(u4VDecID, RW_VLD_DIRE_MD, 0);
  //  vWriteVLD(RW_VLD_BMB1, 0);
  //  vWriteVLD(RW_VLD_BMB2, 0);
  //  vWriteVLD(RW_VLD_BCODE_SA, 0);
    vVDecWriteVLD(u4VDecID, RW_VLD_DIRE_MD_IL, 0);
    // CCJu's
    vVDecWriteMC(u4VDecID, RW_MC_WARP_POINT_NUM, 0);
    vVDecWriteMC(u4VDecID, RW_MC_GMC_Y_MV, 0);
    vVDecWriteMC(u4VDecID, RW_MC_GMC_C_MV, 0);

#if (MPEG4_6589_SUPPORT)
    //6589NEW 3.1.2 DIVX3
    u4RegVal1 = u4VDecReadMP4PP(u4VDecID, RW_PP_DIVX3_DFT_PRED) & 0xFFFF0000;
    if (prMp4Prm->ucVopQuant > 24)
    {
        u4DcScalarY = prMp4Prm->ucVopQuant * 2 - 16;
        u4DcScalarC = prMp4Prm->ucVopQuant - 6;    
    } 
    else if (prMp4Prm->ucVopQuant > 8)
    {
        u4DcScalarY = prMp4Prm->ucVopQuant + 8;
        u4DcScalarC = (prMp4Prm->ucVopQuant + 13) / 2;    
    } 
    else if (prMp4Prm->ucVopQuant > 4)
    {
        u4DcScalarY = prMp4Prm->ucVopQuant * 2;
        u4DcScalarC = (prMp4Prm->ucVopQuant + 13) / 2;
    } 
    else
    {
        u4DcScalarY = 8;
        u4DcScalarC = 8;
    }
    u4RegVal1 |= u4Div2Slash(1024, u4DcScalarY) | (u4Div2Slash(1024, u4DcScalarC) << 8);
    vVDecWriteMP4PP(u4VDecID, RW_PP_DIVX3_DFT_PRED, u4RegVal1);    

    //6589NEW 4.2 DIVX3
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_COL_IVOP, u4VDecReadAVCMV(u4VDecID, RW_AMV_MP4_COL_IVOP) & (~0x1));

    //6589NEW 4.3 DIVX3
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_TOP_FIELD_QUART_SAMPLE, u4VDecReadAVCMV(u4VDecID, RW_AMV_MP4_TOP_FIELD_QUART_SAMPLE) & (~0x3));

    //6589NEW 4.4 DIVX3
    vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_GMC_Y_MV, 0);
#endif  

}


void vVDecMpegCommSetting(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
//VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;

#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif
     
    vVDecWriteVLD(u4VDecID, RW_VLD_PWRSAVE, 0);  
    //-------------------------------------------------
    // global setting un@}l]@Yi
    //-------------------------------------------------
    
    #if VMMU_SUPPORT
    {
    UINT32 u4Page_addr = 0;
    
    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa)/(4*1024))*4);
    printk("[MPEG] vVDecMpegCommSetting, 1, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa), PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa) + PIC_Y_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R1Y, (prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa) >> 9); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa)/(4*1024))*4);
    printk("[MPEG] vVDecMpegCommSetting, 2, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa), PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa) + PIC_C_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R1C, (prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa) >> 8); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa)/(4*1024))*4);
    printk("[MPEG] vVDecMpegCommSetting, 3, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa), PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa) + PIC_Y_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, (prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa) >> 9); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa)/(4*1024))*4);
    printk("[MPEG] vVDecMpegCommSetting, 4, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa), PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa) + PIC_C_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_R2C, (prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa) >> 8); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa)/(4*1024))*4);
    printk("[MPEG] vVDecMpegCommSetting, 5, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa), PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa) + PIC_Y_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_BY, (prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa) >> 8); 
    vVDecWriteMC(u4VDecID, RW_MC_BY1, (prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa) >> 9); 

    u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + (((prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa)/(4*1024))*4);
    printk("[MPEG] vVDecMpegCommSetting, 6, Page Addr = 0x%x\n", u4Page_addr);    
    vPage_Table(u4VDecID, u4Page_addr, PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa), PHYSICAL(prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa) + PIC_C_SZ);
    vVDecWriteMC(u4VDecID, RW_MC_BC, (prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa) >> 7); 
    vVDecWriteMC(u4VDecID, RW_MC_BC1, (prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa) >> 8); 
    
    printk("[MPEG] vVDecMpegCommSetting, VMMUTable:[0x%x, 0x%x]\n", ((UINT32)_pucVMMUTable[u4VDecID]), PHYSICAL((UINT32)_pucVMMUTable[u4VDecID]));         
    vVDecVMMUEnable(PHYSICAL((UINT32)_pucVMMUTable[u4VDecID]));
    }
    #else
    vVDecWriteMC(u4VDecID, RW_MC_R1Y, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R1C, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_R2Y, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa)) >> 9); // div 512
    vVDecWriteMC(u4VDecID, RW_MC_R2C, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BY,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa)) >> 8); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa)) >> 7); // div 128
    vVDecWriteMC(u4VDecID, RW_MC_BY1,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa)) >> 9); // div 256
    vVDecWriteMC(u4VDecID, RW_MC_BC1,  (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa)) >> 8); // div 128
    #endif 


    
#if 0//def VDEC_SIM_DUMP
         if(_u4PicCdTp[u4VDecID] == I_TYPE)
         {
            printk("//Dec0_Y_0x%x.bin_Size_0x%x\n",PHYSICAL((UINT32)(_u4WorkBuffer[_u4DecBufIdx[u4VDecID]*2])),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<8);
        printk("//Dec0_C_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4DecBufIdx[u4VDecID]*2 + 1]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<7);
         }
         else if(_u4PicCdTp[u4VDecID] == P_TYPE)
         {
            printk("//Dec0_Y_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4DecBufIdx[u4VDecID]*2]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<8);
        printk("//Dec0_C_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4DecBufIdx[u4VDecID]*2+ 1]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<7);
        printk("//Ref0_Y_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4FRefBufIdx[u4VDecID]*2]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<8);
        printk("//Ref0_C_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4FRefBufIdx[u4VDecID]*2 + 1]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<7);
         }
         else
         {
            printk("//Dec0_Y_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4DecBufIdx[u4VDecID]*2]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<8);
        printk("//Dec0_C_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4DecBufIdx[u4VDecID] *2+ 1]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<7);
        printk("//Ref0_Y_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4FRefBufIdx[u4VDecID]*2]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<8);
        printk("//Ref0_C_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4FRefBufIdx[u4VDecID]*2 + 1]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<7);    
        printk("//Ref1_Y_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4BRefBufIdx[u4VDecID]*2]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<8);
        printk("//Ref1_C_0x%x.bin_Size_0x%x\n",PHYSICAL(_u4WorkBuffer[_u4BRefBufIdx[u4VDecID] *2+ 1]),((prDecPrm->u4PicH + 15) >> 4)*((prDecPrm->u4PicW+ 15) >> 4)<<7);
         }
#endif


    vSetMcDecBuf(u4VDecID, prDecPrm);
    vSetDecFld(u4VDecID, prDecPrm);
    // addr swap mode
    vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prDecPrm->ucAddrSwapMode);

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
     //Set NBM address swap mode
     vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL, 
               ((u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL)  & 0xFFFFFFF8) |prDecPrm->ucAddrSwapMode));
     
#if VDEC_MC_NBM_OFF
     //Turn off NBM address swap mode
     vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL, 
               (u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL) | (RW_MC_NBM_OFF)  ));
#endif

#if VDEC_DDR3_SUPPORT
     //Turn on DDR3 mode
     vVDecWriteMC(u4VDecID, RW_MC_DDR_CTRL0, 
               ((u4VDecReadMC(u4VDecID, RW_MC_DDR_CTRL0)  & 0xFFFFFFFE) |0x1));

     vVDecWriteMC(u4VDecID, RW_MC_DDR_CTRL1, 
               ((u4VDecReadMC(u4VDecID, RW_MC_DDR_CTRL1)  & 0xFFFFFFFE) |0x1));
#endif

#endif

#if VDEC_FIELD_COMPACT
    //printk( "[MPEG4] Enable Field Compact Mode\n");
    vVDecWriteMP4PP(u4VDecID, 0x3C, (u4VDecReadMP4PP(u4VDecID, 0x3C) & 0xEFFFFFFF));
    vVDecWriteMC(u4VDecID, 0x920, (u4VDecReadMC(u4VDecID, 0x920)  & 0xFEFFFFFF)); 
#endif //VDEC_FIELD_COMPACT

#if (CONFIG_DRV_FPGA_BOARD)
    vVDecWriteMC(u4VDecID, RW_MC_MODE_CTL, MC_QIU_BANK4|MC_QIU_BANK8|MC_DRAM_REQ_DELAY_1T|MC_DRAM_REQ_MERGE_OFF|MC_MV_MERGE_OFF);
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    #if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8550)
       vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
    #else
      #if VDEC_DDR3_SUPPORT   
	   #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	   vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB,((prMpegDecPrm->u4DecW + 63)>>6)<<2);
	   #else
          vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prMpegDecPrm->u4DecW + 63)>>6)<<2);  
	   #endif
      #else
	   #if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
	   vVDecWriteMC(u4VDecID, RW_MC_DDR3_EN, (u4VDecReadMC(u4VDecID, RW_MC_DDR3_EN)  & 0xFFFFFFFE));
	   vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB,((prMpegDecPrm->u4DecW + 15)>> 4));
      #else
          vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prMpegDecPrm->u4DecW + 15)>> 4));   
      #endif
    #endif
#endif
#endif
  //Mars,[2004/06/08],turn on for P-scan 32-pulldown detection
  #ifdef PSCAN_SUPPORT
  //max set Motion Plane address
  //  if(fgIsPScanTv())
  
    // Don't turn ON Motion Plane HW in MPEG4, HW not support in DIVX
    if (_fgPScanOn && fgIsMpeg2())
    {
        vSetPSMP();//]wmotion plane @Ag@
        vSetPSCtrl();//]wPSacn MC control register
    }
    //max
  #endif
    if (prMpegDecPrm->rMpegPpInfo.fgPpEnable)
    {   // Qing Li fix here for 8550 pp reg set
        UINT32 u4MBqp = 0;
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 1);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, u4AbsDramANc(prMpegDecPrm->rMpegPpInfo.u4PpYBufSa) >> 9);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, u4AbsDramANc(prMpegDecPrm->rMpegPpInfo.u4PpCBufSa) >> 8);
        vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, (prDecPrm->u4PicW + 15) >> 4);
        printk("//<vdec> PP Y 0x%x\n", prMpegDecPrm->rMpegPpInfo.u4PpYBufSa);
        printk("//<vdec> PP C 0x%x\n", prMpegDecPrm->rMpegPpInfo.u4PpCBufSa);
        printk("//<vdec> PP MB width %d\n", (prDecPrm->u4PicW + 15) >> 4);

        u4MBqp = (prMpegDecPrm->rMpegPpInfo.au1MBqp[0] & 0x1F) | ((UINT32)(prMpegDecPrm->rMpegPpInfo.au1MBqp[1] & 0x1F) << 8) \
                       | ((UINT32)(prMpegDecPrm->rMpegPpInfo.au1MBqp[2] & 0x1F) << 16) | ((UINT32)(prMpegDecPrm->rMpegPpInfo.au1MBqp[3] & 0x1F) << 24);
        
        vVDecWriteMC(u4VDecID, RW_MC_PP_QP_TYPE, u4MBqp);
        printk("//<vdec> PP QP type %d\n", u4MBqp);        
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, DBLK_Y+DBLK_C);
        printk("//<vdec> PP deblocking mode %d\n", DBLK_Y+DBLK_C);
        vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0); // wirte MC out and PP out
        if (prMpegDecPrm->rMpegPpInfo.fgPpDemoEn)
        {
            vVDecWriteMC(u4VDecID, 0x658, ((u4VDecReadMC(u4VDecID, 0x658)&0xFFFFFFFE)|0x1)); // partial deblocking
            vVDecWriteMC(u4VDecID, 0x65C, ((((prDecPrm->u4PicH + 15) >> 4) - 1) << 24) | ((((prDecPrm->u4PicW + 15) >> 5) - 1) << 8)); // XY end MB
        }
        else
        {
            vVDecWriteMC(u4VDecID, 0x658, (u4VDecReadMC(u4VDecID, 0x658)&0xFFFFFFFE));
        }
#else
        vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, DBLK_Y+DBLK_C);
        //vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0); // wirte MC out and PP out
#endif
        vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, ((prDecPrm->u4PicW + 15) >> 4) - 1);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, (((prDecPrm->u4PicH + 15) >> 4) >> (prDecPrm->ucPicStruct != 3)) - 1);
        printk("//<vdec> PP x range %d\n", ((prDecPrm->u4PicW + 15) >> 4) - 1);
        printk("//<vdec> PP y range %d\n", (((prDecPrm->u4PicH + 15) >> 4) >> (prDecPrm->ucPicStruct != 3)) - 1);        
        //vVDecWriteAVCVLD(u4VDecID, RW_AVLD_SHDR_2, 0x6E00);
        //vVDecWriteMC(u4VDecID, RW_MC_PP_MODE, H264_MODE);
    }
    else
    {
        #if (VDEC_DDR3_SUPPORT && (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560)) // DDR3+PPOUT setting mtk40343
		if((prDecPrm->u4PicW > 16) && (prDecPrm->u4PicH > 16))//when width or height is less than 16,should be set MC out @mtk40343
		{
		vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 1);
		vVDecWriteMC(u4VDecID, 0x834, u4VDecReadMC(u4VDecID, 0x834)&(~(0x1)));// pp out buffer enable
		vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, 0);//turn off de-blocking
		vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 1); // wirte PP out only
	 		if(prDecPrm->ucDecFBufIdx == 0)
			{
			   vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0YSa))>>9);
			   vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic0CSa)) >> 8);
			}
			else if(prDecPrm->ucDecFBufIdx == 1)
			{
			   vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic1YSa))>>9);
			   vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic1CSa)) >> 8);
			}
			else if(prDecPrm->ucDecFBufIdx == 2)
			{
				vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2YSa))>>9);
				vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, (u4AbsDramANc(prMpegDecPrm->rMpegFrameBufSa.u4Pic2CSa)) >> 8);
			}
			vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, ((prDecPrm->u4PicW + 63) >> 6)<<2);
			vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, ((prDecPrm->u4PicW + 15) >> 4) - 1);
		       vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, (((prDecPrm->u4PicH + 15) >> 4) >> (prDecPrm->ucPicStruct != 3)) - 1);
		}
		else
		{
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 0);
    }
	 #else
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 0);
	#endif
    }

#ifdef VDEC_SARNOFF_ON//Qing Li add here for testing sarnoff
    vVDecWriteVLD(u4VDecID, 0x270, (u4VDecReadVLD(u4VDecID, 0x270) | (0x1 << 18) | (0x1 << 20)));
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
    vVDecWriteMC(0, 0x5E4, (u4VDecReadMC(0, 0x5E4) |(0x1 <<12)) );
	//mt8320 need mark 0x660 setting
    //vVDecWriteMC(0, 0x660, (u4VDecReadMC(0, 0x660) |(0x80000000)) );    

   #ifndef VDEC_PIP_WITH_ONE_HW
    vVDecWriteMC(1, 0x5E4, (u4VDecReadMC(1, 0x5E4) |(0x1 <<12)) );
    //vVDecWriteMC(1, 0x660, (u4VDecReadMC(1, 0x660) |(0x80000000)) );    
   #endif
#endif
}


// *********************************************************************
// Function : void vSetDecFld(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
// Description : Set Decoding Field to Register
// Parameter : None
// Return    : None
// *********************************************************************
void vSetDecFld(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    UINT32 u4DecFld;
    UINT32 u42ndFld;
    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif

    if (prDecPrm->ucPicStruct == FRM_PIC) //(fgIsFrmPic(dwIdx))
    {
        u4DecFld = MC_TOP_FLD;
        u42ndFld = MC_2ND_FLD_OFF;
    }
    else
    {
        if (prDecPrm->ucPicType != B_TYPE) //(fgIsRefPic(dwIdx))
        {
            if (prMpegDecPrm->fgDec2ndFld) //(fgIsDecFlagSet(DEC_FLG_2ND_FLD_PIC))
            {
                u4DecFld = (prDecPrm->ucPicStruct == TWO_FLDPIC_TOPFIRST) ?
                           MC_BOTTOM_FLD : MC_TOP_FLD;
                u42ndFld = MC_2ND_FLD_ON;
            }
            else
            {
                u4DecFld = (prDecPrm->ucPicStruct == TWO_FLDPIC_TOPFIRST) ?
                           MC_TOP_FLD : MC_BOTTOM_FLD;
                u42ndFld = MC_2ND_FLD_OFF;
            }
        }
        else  // B Picture
        {
            u4DecFld = (prDecPrm->ucPicStruct == TOP_FLD_PIC) ?
                       MC_TOP_FLD : MC_BOTTOM_FLD;
            u42ndFld = MC_2ND_FLD_OFF;
        }
    }
  
    prMpegDecPrm->ucDecFld = BYTE0(u4DecFld);
    vMCSetDecFld(u4VDecID, u4DecFld, u42ndFld);
}


// *********************************************************************
// Function : void vMCSetDecFld(UINT32 u4VDecID, UINT32 u4DecFld, UINT32 u42ndFldSw)
// Description : MC Set Decode Field
// Parameter : dDecFld: MC_TOP_FLD/MC_BOTTOM_FLD
//             d2ndFldSw: MC_2ND_FLD_ON/MC_2ND_FLD_OFF
// Return    : None
// *********************************************************************
void vMCSetDecFld(UINT32 u4VDecID, UINT32 u4DecFld, UINT32 u42ndFldSw)
{
    if (u4DecFld == MC_TOP_FLD)
    {
        vVDecWriteMC(u4VDecID, RW_MC_DECFLD, 0);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_DECFLD, 1);
    }
    if (u42ndFldSw == MC_2ND_FLD_ON)
    {
        vVDecWriteMC(u4VDecID, RW_MC_2FLDMD, 1);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_2FLDMD, 0);
    }
}


// *********************************************************************
// Function : void vSetMcDecBuf(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
// Description : Write Decoding Buffer into HW Register
// Parameter : None
// Return    : None
// *********************************************************************
void vSetMcDecBuf(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
{
    //VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#if ((CONFIG_DRV_VERIFY_SUPPORT) ||(CONFIG_DRV_FPGA_BOARD) && (!VDEC_DRV_PARSER))
    VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecMPEGDecPrm);
#else
     VDEC_INFO_MPEG_DEC_PRM_T *prMpegDecPrm = (VDEC_INFO_MPEG_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif

    switch (prDecPrm->ucPicType)//(prGetFrmInfo(_dwDecBufIdx)->bPicCdTp)
    {
        case I_TYPE:
        case P_TYPE:
            vMCSetOutputBuf(u4VDecID, prDecPrm->ucDecFBufIdx, prMpegDecPrm->u4FRefBufIdx);
            vMCWriteToDigBuf(u4VDecID, OFF);
            break;
        case B_TYPE:
            /*if(fgIsDecCfgSet(DEC_CFG_DIGEST))
            {
              if(fgVDecDigNeed2StepVP()) {
                vMCSetOutputBuf(MC_DIG_BUF, _dwFRefBufIdx);
                vMCWriteToDigBuf(ON);
              } else
              {
                vMCSetOutputBuf(MC_DIG_BUF, _dwFRefBufIdx);
                vMCWriteToDigBuf(ON);
              }
            }
            else */
            {
              vMCSetOutputBuf(u4VDecID, prDecPrm->ucDecFBufIdx + 2, prMpegDecPrm->u4FRefBufIdx);
              vMCWriteToDigBuf(u4VDecID, OFF);
            }
          break;
    }
}


// *********************************************************************
// Function : void vMCWriteToDigBuf(UINT32 u4VDecID, BOOL fgSwitch)
// Description : MC Write to DIG buffer switch
//               write to decoded buffer and digest buffer at the same time
// Parameter : fgSwitch: ON/OFF
// Return    : None
// *********************************************************************
void vMCWriteToDigBuf(UINT32 u4VDecID, BOOL fgSwitch)
{
    if (fgSwitch)
    {
        vVDecWriteMC(u4VDecID, RW_MC_WMBDIG, 1);
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_WMBDIG, 0);
    }
}


// *********************************************************************
// Function :void vVLDSetBRef(UINT32 u4VDecID, BOOL fgPVop)
// Description : Set RW_VLD_BREF[0]
// Parameter : fgPVop: indicate the backward reference picture is an P-VOP
// Return    : None
// *********************************************************************
void vVLDSetBRef(UINT32 u4VDecID, BOOL fgPVop)
{
    UINT32 u4RegValue = u4VDecReadVLD(u4VDecID, RW_VLD_BREF);
    if (fgPVop)
    {
        vVDecWriteVLD(u4VDecID, RW_VLD_BREF, u4RegValue & ~0x1);
    }
    else
    {
        vVDecWriteVLD(u4VDecID, RW_VLD_BREF, u4RegValue | 0x1);
    }
}


// *********************************************************************
// Function :void vVLDSetBRefCoLocI(UINT32 u4VDecID, BOOL fgIVop)
// Description : Set RW_VLD_BREF[1]
// Parameter : fgIVop: indicate the backward reference picture is an I-VOP
// Return    : None
// *********************************************************************
void vVLDSetBRefCoLocI(UINT32 u4VDecID, BOOL fgIVop)
{
    UINT32 u4RegValue;
    if (fgIVop)
    {
        u4RegValue = u4VDecReadAVCMV(u4VDecID, RW_AMV_MP4_COL_IVOP);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_COL_IVOP, u4RegValue | 1);
    }
    else
    {
        u4RegValue = u4VDecReadAVCMV(u4VDecID, RW_AMV_MP4_COL_IVOP);
        vVDecWriteAVCMV(u4VDecID, RW_AMV_MP4_COL_IVOP, u4RegValue & ~0x1);
    }
}


// *********************************************************************
// Function :void vMCSetBRef(UINT32 u4VDecID, BOOL fgIVop)
// Description : Set RW_MC_BREF
// Parameter : fgIVop: indicate the backward reference picture is an I-VOP
// Return    : None
// *********************************************************************
void vMCSetBRef(UINT32 u4VDecID, BOOL fgIVop)
{
    UINT32 u4VldDcMvSel = u4VDecReadVLD(u4VDecID, RW_VLD_DCMVSEL);
    UINT32 u4McBRef = u4VDecReadMC(u4VDecID, RW_MC_BREF);
    // Note for 1389Lite:
    // Before v5024, this function is not 1389Lite compatible.
    // If an earlier code needs to run in 1389Lite,
    // its vMCSetBRef() should be replaced by this newer one in v5024.
    if (fgIVop)
    {
        vVDecWriteMC(u4VDecID, RW_MC_BREF, u4McBRef | NML_BREF_I_VOP); // MT1389 normal
        vVDecWriteVLD(u4VDecID, RW_VLD_DCMVSEL, u4VldDcMvSel | LITE_BREF_I_VOP); // MT1389 lite
    }
    else
    {
        vVDecWriteMC(u4VDecID, RW_MC_BREF, u4McBRef & (~NML_BREF_I_VOP));  // MT1389 normal
        vVDecWriteVLD(u4VDecID, RW_VLD_DCMVSEL, u4VldDcMvSel & (~LITE_BREF_I_VOP)); // MT1389 lite
    }
}


void VDecDumpMP4Register(UINT32 u4VDecID)
{
    UINT32 u4Val;
    INT32 i;
    #ifdef REG_LOG_NEW
    //6589NEW (4) [ for error concealment only
    #ifdef MPEG4_6589_ERROR_CONCEAL // 0
    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// <DUMP_REG> bitstream: %s\n", _bFileStr1[u4VDecID][1]);
    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// <DUMP_REG> Start dumping pic %d\n", _u4FileCnt[u4VDecID]);
    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// VLD\n");
    for (i=113; i<130; i++)
    {
        u4Val = u4VDecReadVLD(0, i*4);
    }
    for (i=173; i<249; i++)
    {
        u4Val = u4VDecReadVLD(0, i*4);
    }
    #else
    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// <DUMP_REG> Start dumping pic %d\n", _u4FileCnt[u4VDecID]);
    #endif
    // ]
    #ifndef MPEG4_6589_ERROR_CONCEAL // 1
    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// VDEC_TOP\n");
    for (i=0; i<101; i++)
    {
        u4Val = u4VDecReadDV(0, i*4);
    }

    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// VLD_TOP\n");
    for (i=0; i<101; i++)
    {
        u4Val = u4VDecReadVLDTOP(0, i*4);
    }

    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// VLD\n");
    for (i=33; i<256; i++)
    {
        u4Val = u4VDecReadVLD(0, i*4);
    }


    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// MC\n");
    for (i=0; i<701; i++)
    {
        u4Val = u4VDecReadMC(0, i*4);
    }

    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// AVC_MV\n");
    for (i=0; i<256; i++)
    {
        u4Val = u4VDecReadAVCMV(0, i*4);
    }


    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// IS\n");
    for (i=0; i<65; i++)
    {
        u4Val = u4VDecReadMP4PP(0, 0x200 + i*4);
    }

    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// DCAC\n");    
    for (i=0; i<65; i++)
    {
        u4Val = u4VDecReadMP4PP(0, 0x700 + i*4);
    }

    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// IQ\n");
    for (i=0; i<65; i++)
    {
        u4Val = u4VDecReadMP4PP(0, 0x500 + i*4);
    }

    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// IT\n");
    for (i=0; i<65; i++)
    {
        u4Val = u4VDecReadMP4PP(0, 0x900 + i*4);
    }

    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "// ORED_WR\n");
    for (i=0; i<65; i++)
    {
        u4Val = u4VDecReadMP4PP(0, 0x800 + i*4);
    }
    
    _u4RegisterLogLen[u4VDecID] += sprintf(_pucRegisterLog[u4VDecID]+_u4RegisterLogLen[u4VDecID], "//  <DUMP_REG> End dump\n");
    #endif
    #endif
}


void VDecDumpMpegRegister(UINT32 u4VDecID,BOOL fgTriggerAB)
{
                
     UINT32 u4Val;
     int reg;
     int vldstart;
     //    int vldstart = 42;
     //    int vldsize = 200;
     int mcstart = 0;
     int mcsize = 700;
     #ifndef REG_LOG_NEW
     if(fgTriggerAB)
     {
     	u4VDecID = 1;//after decode reg
     }
	 else
	 {
	 	u4VDecID = 0;//before decode reg
	 }
     #if (DUMP_ERROR == 0)
     printk("VLD base Settings\n");
     #endif
     for(vldstart = 33; vldstart < 40; vldstart++)
     {
        u4Val = u4VDecReadVLD(u4VDecID, (vldstart << 2));
	#if DUMP_ERROR 
        ((UINT32 *)(_pucRegister[u4VDecID]))[vldstart] = u4Val;
	#else
         printk("%d (0x%x)  = 0x%4x\n",vldstart, (vldstart<<2),u4Val);
	#endif
     }
     for(vldstart = 42; vldstart < 71; vldstart++)
     {
         u4Val = u4VDecReadVLD(u4VDecID, (vldstart << 2));
	 #if DUMP_ERROR 
         ((UINT32 *)(_pucRegister[u4VDecID]))[vldstart] = u4Val;
	 #else
         printk("%d (0x%x)  = 0x%4x\n",vldstart, (vldstart<<2),u4Val);
	 #endif
     }
     for(vldstart = 112; vldstart < 131; vldstart++)
     {
         u4Val = u4VDecReadVLD(u4VDecID, (vldstart << 2));
	 #if DUMP_ERROR 
         ((UINT32 *)(_pucRegister[u4VDecID]))[vldstart] = u4Val;
	 #else
         printk("%d (0x%x)  = 0x%4x\n",vldstart, (vldstart<<2),u4Val);
	 #endif
     }
	 for(vldstart = 131; vldstart < 192; vldstart++)
     {
         u4Val = u4VDecReadVLD(u4VDecID, (vldstart << 2));
	 #if DUMP_ERROR 
         ((UINT32 *)(_pucRegister[u4VDecID]))[vldstart] = u4Val;
	 #else
         printk("%d (0x%x)  = 0x%4x\n",vldstart, (vldstart<<2),u4Val);
	 #endif
     }
	 
     for(vldstart = 192; vldstart < 256; vldstart++)
     {
         u4Val = u4VDecReadVLD(u4VDecID, (vldstart << 2));
	#if DUMP_ERROR 
         ((UINT32 *)(_pucRegister[u4VDecID]))[vldstart] = u4Val;
	#else
         printk("%d (0x%x)  = 0x%4x\n",vldstart, (vldstart<<2),u4Val);
	#endif
     }
     #if (DUMP_ERROR == 0)       
     printk("MC Settings\n");
	 #endif
     for(reg= mcstart; reg < (mcstart+mcsize); reg++)
     {          
         u4Val = u4VDecReadMC(u4VDecID, (reg << 2));
       #if DUMP_ERROR 
         ((UINT32 *)(_pucRegister[u4VDecID]))[reg + 0x100] = u4Val;
	   #else
         printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
	   #endif
     }
	 #if (DUMP_ERROR == 0)
     printk("IS Settings\n");
	 #endif
     for(reg = 128; reg < 192; reg++)
     {
         u4Val = u4VDecReadMP4PP(u4VDecID, (reg << 2));
		 #if DUMP_ERROR 
         ((UINT32 *)(_pucRegister[u4VDecID]))[reg + 1000] = u4Val;
		 #else
         printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
		 #endif
     }
	 #if (DUMP_ERROR == 0)
     printk("IQ Settings\n");
	 #endif
     for(reg = 320; reg < 384; reg++)
     {
         u4Val = u4VDecReadMP4PP(u4VDecID, (reg << 2));
		 #if DUMP_ERROR 
         ((UINT32 *)(_pucRegister[u4VDecID]))[reg + 1100] = u4Val;
		 #else
         printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
		 #endif
     }
	 #if (DUMP_ERROR == 0)
     printk("IT Settings\n");
	 #endif
     for(reg = 576; reg < 640; reg++)
     {
        u4Val = u4VDecReadMP4PP(u4VDecID, (reg << 2));
		#if DUMP_ERROR 
         ((UINT32 *)(_pucRegister[u4VDecID]))[reg + 1200] = u4Val;
		#else
        printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
		#endif
     }
     #if (DUMP_ERROR == 0)
     printk("CRC register! \n");
	 #endif
     vVDecWriteCRC(u4VDecID,VDEC_CRC_REG_EN,0x01);//crc enable ; mc agent
     for(reg = 0; reg < 65; reg++)
     {
        u4Val = u4VDecReadCRC(u4VDecID, (reg << 2));
		#if DUMP_ERROR 
        ((UINT32 *)(_pucRegister[u4VDecID]))[reg + 2000] = u4Val;
		#else
        printk("%d (0x%x) = 0x%4x\n", reg, (reg<<2), u4Val);
		#endif
     }
     #endif // REG_LOG_NEW
}
void vVDecWriteMP4PP(UINT32 u4VDecID, UINT32 u4Addr, UINT32 u4Val)
{
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif

    if (u4VDecID == 0)
    {
        vWriteReg(RM_VDEC_PP_BASE + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
    }
    else
    {
        vWriteReg(RM_VDEC_PP_BASE + u4Addr, u4Val);
        vVDecSimDumpW(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
    }
}


UINT32 u4VDecReadMP4PP(UINT32 u4VDecID, UINT32 u4Addr)
{
    UINT32 u4Val;
#if (CONFIG_DRV_FPGA_BOARD)
     u4VDecID = 0;
#endif    
    if (u4VDecID == 0)
    {
        u4Val = u4ReadReg(RM_VDEC_PP_BASE + u4Addr);
        vVDecSimDumpR(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
        return u4Val;
    }
    else
    {
        u4Val = u4ReadReg(RM_VDEC_PP_BASE + u4Addr);
        vVDecSimDumpR(u4VDecID, RM_VDEC_PP_BASE, u4Addr, u4Val);
        return u4Val;
    }
}
#ifdef DUMP_REG
UINT32 _aru4DumpData[0x200];
// *********************************************************************
// Function :void VDec_DumpReg(UCHAR ucMpvId)
// Description : Set RW_MC_BREF
// Parameter : fgIVop: indicate the backward reference picture is an I-VOP
// Return    : None
// *********************************************************************
void VDec_DumpReg(UCHAR ucMpvId)
{
    UINT8  ucIdx;
    UINT32 u4DumpIdx;

    UINT32 *pu4DumpArea;

    //pu4DumpArea = (UINT32*)x_mem_alloc(0x200);
    pu4DumpArea = &_aru4DumpData;
    u4DumpIdx = 0;
    
    for (ucIdx = 34; ucIdx <= 39; ucIdx++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadVLD(ucMpvId, 4*ucIdx);
        u4DumpIdx ++;
    }

    for (ucIdx = 42; ucIdx <= 112; ucIdx++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadVLD(ucMpvId, 4*ucIdx);
        u4DumpIdx ++;
    }


    for (ucIdx = 0; ucIdx <= 43; ucIdx++)
    {
        pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*ucIdx);
        u4DumpIdx ++;
    }

    pu4DumpArea[u4DumpIdx] = u4VDecReadMC(ucMpvId, 4*114);
    u4DumpIdx ++;
    
    //x_mem_free(pu4DumpArea);

}
  #endif
