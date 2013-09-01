#ifndef _VDEC_VERIFY_VPARSER_RM_H_
#define _VDEC_VERIFY_VPARSER_RM_H_

#include <mach/mt_typedefs.h>
#include "vdec_verify_general.h"

#ifdef MT8550_DDR3_VDEC_ENABLE    //vdec_verify_general.h
#define RM_DDR3MODE_ENABLE
//#define RM_DDR3MODE_DEBUG_ENABLE
#endif //MT8550_DDR3_VDEC_ENABLE

#ifdef MT8550_ADDRSWAP_ENABLE
#define RM_ADDRSWAP_ENABLE
//#define RM_ADDRSWAP_DSCL_ENABLE
//#define RM_RANDOM_ADDRSWAP_ENABLE
#endif //MT8550_ADDRSWAP_ENABLE

//#ifdef DOWN_SCALE_SUPPORT
//#define DSCL_TOLERATED_RANGE    2
#define DSCL_TOLERATED_RANGE    1 
//#define DSCL_LUMAKEY_ENABLE
//UINT32 u4DSCLLumaKey = 0x30;
//#define DSCL_LUMAKEY_VALUE    10
//#endif //DOWN_SCALE_SUPPORT

//#define NO_SCALE_TEST_ENABLE
//#define FIX_DSCL_PARAM_SETTING

//#define RM_MT8550_EMULATION

#ifdef DOWN_SCALE_SUPPORT
  #ifdef RM_MT8550_EMULATION
  //#define RM_MCOutPut_ENABLE    //Only for RV8 DSCL Testing
  //#define RM_DISABLE_NBM          //Should not Enable
  #endif //RM_MT8550_EMULATION
#endif //DOWN_SCALE_SUPPORT



#define RM_MAX_DSCL_WIDTH    960



//#define RM_INTERLACEFRAME_DSCL_TEST


#define RM_RPR_RACINGMODE_ENABLE
#define RPR_RAC_PREPARSING_CNT      20        // number of pictures to search in advance

//#define RM_FPGA_LIMITATION

#define RM_NEW_PPOUT_MODE     //Disable when use No-wrap, No-NBM FPGA bitfile


//#define RM_DUMP_MCPERFORMANCE_INFO
//#define RM_RISCCHECKFINISH_ENABLE



#ifdef RM_ADDRSWAP_ENABLE
extern UINT8 auAddrSwapMapTable_RM[8];
extern UINT8 auImgRszAddrSwapMapTable[8];
#endif //RM_ADDRSWAP_ENABLE


extern BOOL fgRMCheckCRCResult;


extern void vRM_VParserEx(UINT32 u4InstID);
extern void vRM_VParser(UINT32 u4InstID);
extern UINT32 u4RM_FindIPic(UINT32 u4InstID, UINT32 u4EndFrm);
extern UINT32 u4RM_PreParseIPic(UINT32 u4InstID, UINT32 u4EndFrm);


extern void vRM_TriggerDecode(UINT32 u4InstID, VDEC_INFO_RM_PICINFO_T *prParsingPic);
extern void vRM_VDecDecEndProc(UINT32 u4InstID);
extern void vRM_VerInitDec(UINT32 u4InstID);
extern void vDrmaBusyOff (UINT32  u4InstID);


#endif // _VDEC_VERIFY_VPARSER_RM_H_

