#ifndef _VDEC_VERIFY_MM_MAP_H_
#define _VDEC_VERIFY_MM_MAP_H_

#define WORKING_AREA_CHANEL_A    1
#define VDSCL_CHANEL_A                    1
#define GOLD_CHANEL_A                     1

#define VDEC_VIDEOCODEC_RM_4MMAP    //Only for RM Decoder

#ifdef VDEC_VIDEOCODEC_RM_4MMAP
#define RING_VFIFO_SUPPORT_4MMAP
#endif //VDEC_VIDEOCODEC_RM_4MMAP


//Working Area Chanel A
#define DRAMA_NONCACH_BASE_ADDRESS          0xC0000000L
//Working Area Chanel B
#define DRAMA_NONCACH_BASE_ADDRESS2          0xD0000000L


#if (WORKING_AREA_CHANEL_A)
#define VDEC_WORK_BUF_SA DRAMA_NONCACH_BASE_ADDRESS + 0x01200000         //For Global Variable Memory Size
#else
#define VDEC_WORK_BUF_SA DRAMA_NONCACH_BASE_ADDRESS2 + 0x01200000       //For Global Variable Memory Size
#endif

#define VDEC_WORK_BUF_SZ 0x01000000

#define AVC_PRED_SA VDEC_WORK_BUF_SA + VDEC_WORK_BUF_SZ // 0x31020000
#define AVC_PRED_SZ 64*1024 // 32*1024 Cheng-Jung 2012/08/30 test H264 DRAM

#if (VDSCL_CHANEL_A)
//Chanel A
#define VDSCL_BUF_SA 0xC2100000L
#else
//Chanel B
#define VDSCL_BUF_SA 0xD2100000L
#endif

#define VDSCL_BUF_SZ 0x3FC000  //1920x1080x2


#define IDE_BUFFER_SA VDSCL_BUF_SA + VDSCL_BUF_SZ
#define IDE_BUFFER_SZ 0x10000

#define FGT_SA IDE_BUFFER_SA + IDE_BUFFER_SZ // 0x31400000
#define FGT_SZ 0x300000  //1920x1080x3/2


#define FGT_DATABASE_SA FGT_SA + FGT_SZ // 0x31700000
#define FGT_DATABASE_SZ 0xa9000  //0x317a9000

#define FGT_SEED_SA FGT_DATABASE_SA + FGT_DATABASE_SZ // 0x317a9000
#define FGT_SEED_SZ 0x400  //0x317a9400

#define FGT_SEI_SA FGT_SEED_SA + FGT_SEED_SZ // 0x317a9400
#define FGT_SEI_SZ 0x600  //0x317a9a00

#define VDSCL_WORK_SA FGT_SEI_SA + FGT_SEI_SZ // 0x317a9a00
#define VDSCL_WORK_SZ 0x8000  //0x317B1a00

#define VDSCL_SW_WORK1_SA VDSCL_WORK_SA + VDSCL_WORK_SZ // 0x317a9a00
#define VDSCL_SW_WORK1_SZ 0x1fe000  //0x3194fa00

#define VDSCL_SW_WORK2_SA VDSCL_SW_WORK1_SA + VDSCL_SW_WORK1_SZ // 0x317a9a00
#define VDSCL_SW_WORK2_SZ 0x1fe000  //0x31b4da00

#define VDSCL_SW_WORK3_SA VDSCL_SW_WORK2_SA + VDSCL_SW_WORK2_SZ // 0x317a9a00
#define VDSCL_SW_WORK3_SZ 0x1fe000  //0x31d4ba00

#define VDSCL_SW_WORK4_SA VDSCL_SW_WORK3_SA + VDSCL_SW_WORK3_SZ // 0x317a9a00
#define VDSCL_SW_WORK4_SZ 0x1fe000  //0x31f49a00

#if (GOLD_CHANEL_A)
#define GOLD_Y_SA  0xC3100000L
#else
#define GOLD_Y_SA  0xD3100000L
#endif
#define VDEC_VP8_WRAPPER_OFF 0                      //[Jackal Chen] If width or height > 2048, enable this!  
#define VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION 0     //[Jackal Chen] If we will decode Webp with ME2 integration, enable this!

#if VDEC_VP8_WRAPPER_OFF
#if VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION
#define GOLD_Y_SZ  0x1F4000 //(16000*128)
#define GOLD_C_SZ  0xFA000  //(16000*128)/2
#else
#define GOLD_Y_SZ  0x1A00000//(4096*4096)
#define GOLD_C_SZ  0xD00000 //(4096*4096)/2  //1920x1080
#endif
#else
#define GOLD_Y_SZ  0x400000
#define GOLD_C_SZ  0x1FE000  //1920x1080
#endif
#define GOLD_C_SA  GOLD_Y_SA + GOLD_Y_SZ  //0xC3500000


#define V_FIFO_SA GOLD_C_SA + GOLD_C_SZ   //0x36FE000

#if 1
#if VDEC_VP8_WRAPPER_OFF
#define V_FIFO_SZ 0x01900000 //0x0800000
#else
#define V_FIFO_SZ 0x01900000//0x05000000 //0x1000000
#endif

#define AVS_VLD_PRED_SZ 0x2F7600
#define AVS_VLD_MV_SZ 0x2F7600

#else

#ifdef RING_VFIFO_SUPPORT_4MMAP
  #ifdef VDEC_VIDEOCODEC_RM_4MMAP
  #define V_FIFO_SZ 0x100000                               //Only for RM Emulation
  #else //VDEC_VIDEOCODEC_RM_4MMAP
#define V_FIFO_SZ 0xA00000                             //10*1024*1024
  #endif //VDEC_VIDEOCODEC_RM_4MMAP
#else  //RING_VFIFO_SUPPORT_4MMAP
  #ifdef VDEC_VIDEOCODEC_RM_4MMAP
  #define V_FIFO_SZ 0x100000                               //Only for RM Emulation
  #else //VDEC_VIDEOCODEC_RM_4MMAP
#define V_FIFO_SZ 0x9600000                           //150*1024*1024
  #endif //VDEC_VIDEOCODEC_RM_4MMAP
#endif  //RING_VFIFO_SUPPORT_4MMAP
#endif
#if VDEC_VP8_WRAPPER_OFF
#define VLD_PRED_SZ 64*1024
#define V_PRED_SA V_FIFO_SA + V_FIFO_SZ

#define SEG_ID_SZ 256*1024
#define SEG_I_SA V_PRED_SA + VLD_PRED_SZ

#define PP_WRAPY_SZ 64*1024
#define PP_WRAPY_SA SEG_I_SA + SEG_ID_SZ 

#define PP_WRAPC_SZ 64*1024
#define PP_WRAPC_SA PP_WRAPY_SA + PP_WRAPY_SZ  

#define VLD_WRAP_SZ 15*4096
#define V_WRAP_SA PP_WRAPC_SA + PP_WRAPC_SZ

#define PP_WRAP_SZ 30*4096
#define PP_WRAP_SA V_WRAP_SA + VLD_WRAP_SZ 
#else
#define VLD_WRAP_SZ 15*4096
#define V_WRAP_SA V_FIFO_SA + V_FIFO_SZ

#define PP_WRAP_SZ 30*4096
#define PP_WRAP_SA V_WRAP_SA + VLD_WRAP_SZ 
#endif

//#define MV_BUF_SA AVC_PRED_SA + AVC_PRED_SZ
//#define MV_BUF_SZ 8670*1024

//#define DEBLOCK_AREA_SA MV_BUF_SA + MV_BUF_SZ
//#define DEBLOCK_AREA_SZ 8670*1024

//#define MV_BUF_EA 0x32000000L
//#define MV_BUF_EA IO_BASE_ADDR + 0x02000000L

#if VDEC_VP8_WRAPPER_OFF
#define DPB_SA    PP_WRAPC_SA + V_FIFO_SZ       // 0xCCCFE000
#else
#define DPB_SA    PP_WRAP_SA + V_FIFO_SZ       // 0xCCCFE000
#endif

#if 1//VDEC_MVC_SUPPORT
#define DPB_SZ    0x01C00000 //(12288*1024)
//#define MVBUF_SZ	4096*2088*2 + 4096*2088/4
#define MVBUF_SZ	1920*1088*2 + 1920*1088/4

#else
#define DPB_SZ    0x01200000 //(12288*1024)
//#define DPB_SZ    0x02C00000
#define MVBUF_SZ	4096*1088*2 + 4096*1088/4

#endif

#define FILE_LIST_SA DPB_SA + DPB_SZ        // 0xCDEFE000
#define FILE_LIST_SZ 1024*1024

#define GOLDEN_FILE_INFO_SZ 1024*1024

//#define DPB_MV_SZ    0x0080000 //(522240 for 1 Frame)

//#define DEC_WORK_SA    0x34000000L

#define ADDSWAP_BUF_SZ 0x3FC000  //1920x1080x2

//Buffer used by DV Decode
#define DVDEC_RLCB0_SIZE           0x2000
#define DVDEC_RLCB1_SIZE           0x2000
#define _pbRLCB0Addr	 (FILE_LIST_SA + FILE_LIST_SZ + 0x00000000)
#define _pbRLCB1Addr	 (FILE_LIST_SA + FILE_LIST_SZ + DVDEC_RLCB0_SIZE)

#define WMV_DEFINE
#define WMV_LARGE_FRAME


//RM MMap Info
#define RM_CHECKSUM_BUFFER_SZ    0x200000    // 2 x 1024 x 1024
#define RM_AULIKEBUF_SZ    0xF00000    // 15 x 1024 x 1024
#define RM_FRMINFO_SZ    0x200000
#define RM_MVHWBUF_SZ    0x1FE00
#define RM_VLDPRED_SZ      0xC000
#define RM_GOLDENDATA_SZ      0x400000
#define RM_RSZWORKBUF_SZ      0x1800
#define GOLD_Y_SZ_1  0x400000
#define GOLD_C_SZ_1  0x1FE000                           //1920x1080
#define VDSCL_BUF_SZ_1 0x400000 + 0x1FE000  //1920x1080x2
#define RM_MCOUT_Y_SZ    (2048*1088) *2
#define RM_MCOUT_C_SZ    (2048*1088 / 2) *2 //0xA00000 //(2048*1088 / 2) *2
#define RM_RINGFLOW_TEMPFIFO_SZ    0x100000
#define RM_CRCRESULT_SZ    4*1024*1024    //CRC Result Buffer


#ifdef WMV_DEFINE
//Define DCAC, MV, Bitplane for WMV
//#define DCAC_SA           (FILE_LIST_SA + FILE_LIST_SZ + 0x00000000)
#define DCAC_SA           (VDEC_WORK_BUF_SA)
#define Mv_1              (DCAC_SA + 0x10000)//0x27100
#define Mv_1_SZ         0x3fc00
#define Mv_2              (DCAC_SA + 0x4fc00)//0x4e200
#define Mv_2_SZ         0x3fc00
#define Bp_1              (DCAC_SA + 0x8f800)//0x75300
#define Bp_1_SZ         0x10000
#define Bp_2              (DCAC_SA + 0x9f800)//0x9c400
#define Bp_2_SZ         0x10000
#define Bp_3              (DCAC_SA + 0xaf800 )//0xc3500
#define Bp_3_SZ         0x10000
#define Bp_4              (DCAC_SA + 0xbf800)//0xea600
#define Bp_4_SZ         0x10000
#define Mv_3              (DCAC_SA + 0xcf800)//0x111700
#define Mv_3_SZ         0x10000
#define Mv_1_2            (DCAC_SA + 0xdf800)//0x138800
#define Mv_1_2_SZ      0x10000
#define DCAC_2            (DCAC_SA + 0xef800)//0x15f900

#define DCAC_NEW_SA         (VDEC_WORK_BUF_SA)
#define DCAC_NEW_SZ         (0x3480 * 2)                                     //DCAC Size = MB_W*7*16
#define MV_NEW_SA             (DCAC_NEW_SA + 0x4000)
#define MV_NEW_SZ             (0x1FE00 * 2)                               //MV Siz = MB_W*MB_H*16  sun for temp for 3D feature
#define BP_0_NEW_SA         (DCAC_NEW_SA + 0x25000)
#define BP_0_NEW_SZ          (0x440 * 4)                                     //BP0 Siz = MB_H*16
#define BP_1_NEW_SA          (DCAC_NEW_SA + 0x26000)
#define BP_1_NEW_SZ          (0x440 * 4)                                     //BP1 Siz = MB_H*16
#define BP_2_NEW_SA          (DCAC_NEW_SA + 0x27000)
#define BP_2_NEW_SZ           (0x440 * 4)                                     //BP2 Siz = MB_H*16

#ifdef WMV_LARGE_FRAME

#if VDEC_VP8_WRAPPER_OFF
#if VDEC_VP8_WEBP_SUPPORT_ME2_INTEGRATION
#define PIC_Y_SZ          0x1F4000 //(16000*128)
#define PIC_C_SZ          0xFA000  //(16000*128)/2
#else
#define PIC_Y_SZ          0x1A00000//(4096*4096) //(2048*1088) *2
#define PIC_C_SZ          0xD00000//(4096*4096)/2 // (2048*1088 / 2) *2
#endif
#else
#define PIC_Y_SZ          (2048*1088) *2
#define PIC_C_SZ          (2048*1088 / 2) *2
#endif
//#define PIC_BASE_SA       (DCAC_SA + 0x6acfc00)
#define PIC_BASE_SA       (FILE_LIST_SA + FILE_LIST_SZ + 0x100000)    // 0xCE030000
#define VDO_REF_AREA_SA   PIC_BASE_SA                                           // 0xCE030000
#define VDO_REF_AREA_SZ   ((PIC_Y_SZ + PIC_C_SZ) * 2)
#define PIC0Y_SA          VDO_REF_AREA_SA                                            // 0xCE030000
#define PIC0C_SA          (PIC0Y_SA + PIC_Y_SZ)                                     // 0xCE470000
#define PIC1Y_SA          (PIC0C_SA + PIC_C_SZ)                                    // 0xCE690000
#define PIC1C_SA          (PIC1Y_SA + PIC_Y_SZ)                                     // 0xCEAD0000
#define VDO_B_AREA_SA     (VDO_REF_AREA_SA + VDO_REF_AREA_SZ)  // 0xCECF0000
#define VDO_B_AREA_SZ     (PIC_Y_SZ + PIC_C_SZ)                              // 0xCF350000
#define PIC2Y_SA          VDO_B_AREA_SA                                               // 0xCECF0000
#define PIC2C_SA          (PIC2Y_SA + PIC_Y_SZ)                                     // 0xCF130000
// WMV PP Work Bufer
#define DEC_PP_SZ         ((2048/16)*9*8)*2
#define DEC_PP_SA         PIC2C_SA + PIC_C_SZ                                     // 0xCF350000
#define DEC_PP_1          DEC_PP_SA                                                       // 0xCF350000
#define DEC_PP_2          DEC_PP_SA + DEC_PP_SZ                                 // 0xCF354800
// ~WMV PP Work Bufer
#define DEC_PP_Y_SZ      (2048*1088) *2
#define DEC_PP_C_SZ      (2048*1088/2) *2
#define DEC_PP_Y_SA     DEC_PP_2 + DEC_PP_SZ                                 // 0xCF359000
#define DEC_PP_C_SA     DEC_PP_Y_SA + DEC_PP_Y_SZ                        // 0xCF799000


#else  // !WMV_LARGE_FRAME

#define PIC_Y_SZ          (1920*1088) *2
#define PIC_C_SZ          (1920*1088 / 2) *2
#define PIC_BASE_SA       (DCAC_SA + 0x6acfc00)
#define VDO_REF_AREA_SA   PIC_BASE_SA
#define VDO_REF_AREA_SZ   ((PIC_Y_SZ + PIC_C_SZ) * 2)
#define PIC0Y_SA          VDO_REF_AREA_SA
#define PIC0C_SA          (PIC0Y_SA + PIC_Y_SZ)
#define PIC1Y_SA          (PIC0C_SA + PIC_C_SZ)
#define PIC1C_SA          (PIC1Y_SA + PIC_Y_SZ)
#define VDO_B_AREA_SA     (VDO_REF_AREA_SA + VDO_REF_AREA_SZ)
#define VDO_B_AREA_SZ     (PIC_Y_SZ + PIC_C_SZ)
#define PIC2Y_SA          VDO_B_AREA_SA
#define PIC2C_SA          (PIC2Y_SA + PIC_Y_SZ)
// WMV PP Work Bufer
#define DEC_PP_SZ         ((1920/16)*9*8)*2
#define DEC_PP_SA         PIC2C_SA + PIC_C_SZ
#define DEC_PP_1          DEC_PP_SA
#define DEC_PP_2          DEC_PP_SA + DEC_PP_SZ
// ~WMV PP Work Bufer
#define DEC_PP_Y_SZ      (1920*1088) *2
#define DEC_PP_C_SZ      (1920*1088/2) *2
#define DEC_PP_Y_SA     DEC_PP_2 + DEC_PP_SZ
#define DEC_PP_C_SA     DEC_PP_Y_SA + DEC_PP_Y_SZ
#endif
#else //!WMV_DEFINE

#define PIC_Y_SZ          (1920*1088)
#define PIC_C_SZ          (1920*1088 / 2)
#define VDO_REF_AREA_SA   (V_FIFO_SA + V_FIFO_SZ)
#define VDO_REF_AREA_SZ   ((PIC_Y_SZ + PIC_C_SZ) * 2)
#define PIC0Y_SA          VDO_REF_AREA_SA
#define PIC0C_SA          (PIC0Y_SA + PIC_Y_SZ)
#define PIC1Y_SA          (PIC0C_SA + PIC_C_SZ)
#define PIC1C_SA          (PIC1Y_SA + PIC_Y_SZ)
#define VDO_B_AREA_SA     (VDO_REF_AREA_SA + VDO_REF_AREA_SZ)
#define VDO_B_AREA_SZ     (PIC_Y_SZ + PIC_C_SZ)
#define PIC2Y_SA          VDO_B_AREA_SA
#define PIC2C_SA          (PIC2Y_SA + PIC_Y_SZ)
// WMV PP Work Bufer
#define DEC_PP_SZ         ((1920/16)*9*8)*2
#define DEC_PP_SA         PIC2C_SA + PIC_C_SZ
#define DEC_PP_1          DEC_PP_SA
#define DEC_PP_2          DEC_PP_SA + DEC_PP_SZ
// ~WMV PP Work Bufer
#define DEC_PP_Y_SZ      (1920*1088) *2
#define DEC_PP_C_SZ      (1920*1088/2) *2
#define DEC_PP_Y_SA     DEC_PP_2 + DEC_PP_SZ
#define DEC_PP_C_SA     DEC_PP_Y_SA + DEC_PP_Y_SZ
#endif //WMV_DEFINE

//Robert: MP4 uses.
// HHKuo's DRAM size total: 13770 bytes
//#define MP4_DCAC_SA           (VDO_B_AREA_SA + VDO_B_AREA_SZ)
#define DCAC_2_SZ                0x100000
#define MP4_DCAC_SA           (DCAC_2 + DCAC_2_SZ)

//Ginny for HD emulation 061122
#ifdef WMV_DEFINE
#ifdef WMV_LARGE_FRAME
#ifdef WRITE_FULL_DCAC_DATA
#define DCAC_SZ           ((((2048 / 16) * 4) * ((1088 / 16) * 4)) * 4) // (MBx * 4) * (MBy * 4) = 25920
#else
//#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
#if 1 //mtk40343 8/9/2010 for MT8580
// 6589NEW 1.1
//#define DCAC_SZ           ((((4096 / 16) * 4) * (4)) * 4) // (MBx * 4) * (4) = 7680
#define DCAC_SZ           ((((1920 / 16) * 8) * 128) / 8) // (MBx * 8 * 128) bits = (MBx * 128) bytes = 15360
#else
#define DCAC_SZ           ((((2048 / 16) * 4) * (4)) * 4) // (MBx * 4) * (4) = 7680
#endif
#endif
#define VER_MVEC_SA           (MP4_DCAC_SA + DCAC_SZ)
#define VER_MVEC_SZ           (((4096 / 16) * 4 * (2048 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define VER_BMB1_SA           (VER_MVEC_SA + VER_MVEC_SZ)
#define VER_BMB1_SZ           (((4096 / 16) * 4 * (2048 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define VER_BMB2_SA           (VER_BMB1_SA + VER_BMB1_SZ)
#define VER_BMB2_SZ           (((4096 / 16) * 4 * (2048 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define BCODE_SA          (VER_BMB2_SA + VER_BMB2_SZ)
//#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
#if 1 //mtk40343 8/9/2010 for MT8580
#define BCODE_SZ          (((2048 / 16) * 8) * 4) // (MBx * 2) = 90
#else
#define BCODE_SZ          (((2048 / 16) * 2) * 4) // (MBx * 2) = 90
#endif
#else   // !WMV_LARGE_FRAME
#ifdef WRITE_FULL_DCAC_DATA
#define DCAC_SZ           ((((1920 / 16) * 4) * ((1088 / 16) * 4)) * 4) // (MBx * 4) * (MBy * 4) = 25920
#else
// 6589NEW 1.1
//#define DCAC_SZ           ((((4096 / 16) * 4) * (4)) * 4) // (MBx * 4) * (4) = 7680
#define DCAC_SZ           ((((1920 / 16) * 8) * 128) / 8) // (MBx * 8 * 128) bits = (MBx * 128) bytes = 15360
#endif
#define VER_MVEC_SA           (MP4_DCAC_SA + DCAC_SZ)
#define VER_MVEC_SZ           (((1920 / 16) * 4 * (1088 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define VER_BMB1_SA           (VER_MVEC_SA + VER_MVEC_SZ)
#define VER_BMB1_SZ           (((1920 / 16) * 4 * (1088 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define VER_BMB2_SA           (VER_BMB1_SA + VER_BMB1_SZ)
#define VER_BMB2_SZ           (((1920 / 16) * 4 * (1088 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define BCODE_SA          (VER_BMB2_SA + VER_BMB2_SZ)
#define BCODE_SZ          (((1920 / 16) * 2) * 4) // (MBx * 2) = 90
#endif
#else  //not define WMV_DEFINE
#ifdef WRITE_FULL_DCAC_DATA
#define DCAC_SZ           ((((720 / 16) * 4) * ((576 / 16) * 4)) * 4) // (MBx * 4) * (MBy * 4) = 25920
#else
// 6589NEW 1.1
//#define DCAC_SZ           ((((720 / 16) * 4) * (4)) * 4) // (MBx * 4) * (4) = 720
#define DCAC_SZ           ((((1920 / 16) * 8) * 128) / 8) // (MBx * 8 * 128) bits = (MBx * 128) bytes = 15360
#endif
#define VER_MVEC_SA           (MP4_DCAC_SA + DCAC_SZ)
#define VER_MVEC_SZ           (((720 / 16) * 4 * (576 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define VER_BMB1_SA           (VER_MVEC_SA + VER_MVEC_SZ)
#define VER_BMB1_SZ           (((720 / 16) * 4 * (576 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define VER_BMB2_SA           (VER_BMB1_SA + VER_BMB1_SZ)
#define VER_BMB2_SZ           (((720 / 16) * 4 * (576 / 16)) * 4) // (MBx * 4) * (MBy) = 6480
#define BCODE_SA          (VER_BMB2_SA + VER_BMB2_SZ)
#define BCODE_SZ          (((720 / 16) * 2) * 4) // (MBx * 2) = 90
#endif

//6589NEW 2.2 2.6
#define DATA_PARTITION_SZ   ((2 * (1920 / 16) * (1088 / 16) * 128) / 8) // (2 * MBx * MBy * 128) bits = 261120 bytes
// 6589
#define NOT_CODED_SZ        (((((1920 / 16) * (1088 / 16) + 127) >> 7) << 7) / 8) // ((((1 * MBx * MBy) + 127) >> 7) << 7) bits = 1024 bytes
// 6582
//#define NOT_CODED_SZ        (((((1920 / 16) * (1088 / 16) + 128) >> 7) << 7) / 8) // ((((1 * MBx * MBy) + 127) >> 7) << 7) bits = 1024 bytes


//6589NEW 4.1
#define MV_DIRECT_SZ        (((1920 / 16) * (1088 / 16) * 128) / 8) // (MBx * MBy) * 128 bit = 130560 bytes 

#define KMALLOC_SZ 0x400000


extern void vMemoryAllocate(UINT32 u4InstID);
extern void vMemoryFree(UINT32 u4InstID);
extern void* vMemoryAllocateLoop(UINT32 u4Size);
extern void* vMemoryFreeLoop(void *pvAddr, UINT32 u4Size);
extern void x_free_aligned_verify_mem(void *pvAddr, BOOL fgChannelA);

extern BOOL fgVDecAllocWorkBuffer(UINT32 u4InstID);
extern void vVDecFreeWorkBuffer(UINT32 u4InstID);

extern int m4u_v2p(unsigned int va);

void vVDec_FlushDCacheRange(UINT32 u4Start, UINT32 u4Len);
void vVDec_CleanDCacheRange(UINT32 u4Start, UINT32 u4Len);
void vVDec_InvDCacheRange(UINT32 u4Start, UINT32 u4Len);

#endif
