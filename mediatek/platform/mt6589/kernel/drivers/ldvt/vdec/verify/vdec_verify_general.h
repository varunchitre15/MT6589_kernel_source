#ifndef _VDEC_VERIFY_GENERAL_H_
#define _VDEC_VERIFY_GENERAL_H_

#include <mach/mt_typedefs.h>
#include "vdec_verify_keydef.h"
#include "vdec_verify_mm_map.h"
#include "../include/vdec_info_common.h"

//#include "drv_config.h"
//#include "chip_ver.h"
//#include "x_printf.h"

//Vdec Codec Config
//#define VDEC_VIDEOCODEC_RM                            //Define Vdec Verification Flow for RM Decoder

//Vdec Codec - Feature Config 
#ifdef VDEC_VIDEOCODEC_RM
//#define DOWN_SCALE_SUPPORT                      // DownScale  //Must Disable DDR3 Setting
//#define MT8550_DDR3_VDEC_ENABLE             //DDR3     //Disable when use No-wrap, No-NBM FPGA bitfile    //!!!! Enable DDR3, Must Enable ADDRSWAP !!!!
   #ifdef  MT8550_DDR3_VDEC_ENABLE
#define MT8550_ADDRSWAP_ENABLE              //Address Swap      //Disable when use No-wrap, No-NBM FPGA bitfile
   #endif
//#define RM_RINGVIFO_FLOW                           //Use Ring VFIFO for RM Only

//#define RM_ATSPEED_TEST_ENABLE        //Load All Bitstream to VFIFO, Trigger Decode ASAP
//#define RM_DUMP_CHECKSUM

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
#define RM_CRCCHECKFLOW_SUPPORT   //if CRCcheck ,open it
#define RM_CRCCHECK_ENABLE              //if CRCcheck ,open it
#endif
//#define RM_CRCCHECK_RANDOMCHECK     //Pixel by pixel Check

//#define RM_CRCCHECK_ENABLE_ONEPATH    //Only for Test Flow, Should be Disabled
//#define RM_CRCCHECK_HWOUT_GENERATE
//#define RM_CRCCHECK_TIMER                     //Check Process Time
//#define RM_DISABLE_HWCRCCHECK              //Only for MT8550,8550 don't support crc
#endif //VDEC_VIDEOCODEC_RM

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8555)
#if !VMMU_SUPPORT
#define WMV_CRCCHECK_ENABLE 
#endif
#endif

#ifdef WMV_CRCCHECK_ENABLE
// Need disable this config because of BD CRC comparing according to frame individually
#define WMV_CRC_COMPOSITE_CHECK_ENABLE
//#define WMV_PIX_COMP_IF_CRC_COMP_ERR
#endif

#define DRAM_BUSY_TEST 0
//FILE IO Config
//#define VDEC_EMUVER_FILEIO    //Enable This Define When System Don't Support FileIO Function

//#define INTERGRATION_WITH_DEMUX

#define SATA_HDD_READ_SUPPORT
#define SATA_HDD_FS_SUPPORT

#ifndef SATA_HDD_READ_SUPPORT
  #ifdef SATA_HDD_FS_SUPPORT
     #undef SATA_HDD_FS_SUPPORT
  #endif
#endif


//#define BARREL2_SUPPORT  
//#define BARREL2_THREAD_SUPPORT

#ifdef BARREL2_THREAD_SUPPORT
  #undef BARREL2_SUPPORT
#endif

#define ADDR_SWAP_MODE ADDRSWAP_OFF
#define VDEC_PP_ENABLE  FALSE
//#define VDEC_SARNOFF_ON // for mpeg2 sarnoff test
#define VDEC_MP2_COMPARE_CRC 0
#define VDEC_MP2_SAVE_PATTERN 1
#define VDEC_FIELD_COMPACT 0

//#define	VPMODE
#define FW_WRITE_QUANTIZATION
#define MPEG4_CRC_CMP
//#define MPEG4_FORCE_DUMP_OUTPUT
// if VDEC_SRAM is defined, MPEG4 VDEC MV_PRED and Not coded uses SRAM
#define VDEC_SRAM
#define DUMP_ERROR 1

#define REALCHIP_DVT //[Cheng-Jung] If we want to use it on real chip for DVT test

#define DIRECT_DEC
#define SW_RESET
#define MEM_PAUSE_SUPPORT
//#define VDEC_BREAK_EN     //[Jackal Chen] If we will run break test case, enable this!
//#define REDEC
//#define EXT_COMPARE
//#define GEN_HW_CHKSUM
//#define COMP_HW_CHKSUM
//#define DOWN_SCALE_SUPPORT
//#define FGT_SUPPORT
//#define HW_FINDSTARTCODE_SUPPORT

#define POWER_TEST_NONE 0
#define POWER_TEST_SW_RESET 1
#define POWER_TEST_MTCMOS 2
#define POWER_TEST_DCM 3
#if 1
#define POWER_TEST_CASE POWER_TEST_NONE
#define POWER_TEST_MANUAL_CHECK 0
#endif

#if 0
//#define POWER_TEST_CASE POWER_TEST_MTCMOS
#define POWER_TEST_CASE POWER_TEST_DCM
#define POWER_TEST_MANUAL_CHECK 1
#endif


//#define LETTERBOX_SUPPORT
#ifdef LETTERBOX_SUPPORT
#define LETTERBOX_DETECTION_ONLY
#endif

#ifdef DOWN_SCALE_SUPPORT
#define VERIFICATION_DOWN_SCALE
#endif
//#define VERIFICATION_FGT
//#define VDEC_DEBUG_USAGE

#ifdef VDEC_DEBUG_USAGE
//#define NO_COMPARE
#endif


#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) //14/9/2010 mtk40343
#define fgVDEC_H264_REDUCE_MV_BUFF(fgDirBuffReduce)  (fgDirBuffReduce == 1)
#define VDEC_H264_REDUCE_MV_BUFF 1

#else
#define VDEC_H264_REDUCE_MV_BUFF 0
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#ifndef DOWN_SCALE_SUPPORT
#define GOLDEN_128BIT_COMP
#endif
//#define VDEC_MPEG4_SUPPORT_RESYNC_MARKER
#define WMV_EC_IMPROVE_SUPPORT 1
#endif

#define VDEC_VP6_ERR_TEST 0
#define VDEC_VP8_ERR_TEST 0
#define VDEC_AVS_EMU 0
#define VDEC_AVS_ERR_TEST 0
#define VDEC_DRAM_BUSY_TEST 0

#if (CONFIG_DRV_FPGA_BOARD)
//#define IRQ_DISABLE
//#define FPGA_FOR_MPEG2
//#define FPGA_FOR_H264
//#define IDE_READ_SUPPORT
//#define IDE_WRITE_SUPPORT
//#define PCFILE_WRITE

#define RING_VFIFO_SUPPORT
#define DYNAMIC_MEMORY_ALLOCATE
#else
//#define RING_VFIFO_SUPPORT
#define DYNAMIC_MEMORY_ALLOCATE
//#define ReDecBitstream
#endif

#endif
