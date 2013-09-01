#ifndef _VDEC_VERIFY_KEYDEF_H_
#define _VDEC_VERIFY_KEYDEF_H_



#define DEC_NORM_INIT_PRM 0
#define DEC_NORM_VPARSER 1
#define DEC_NORM_WAIT_TO_DEC 2
#define DEC_NORM_WAIT_DECODE 3

#define DEC_RETRY_NUM 8000
#define MAX_CHKSUM_NUM 80


#define BARREL1 1
#define BARREL2 2



//#define START_CODE 0x000001

#define OUT_OF_FILE 0x000001
#define VER_FORBIDEN_ERR 0x000002
#define DEC_INIT_FAILED 0x000003

#define NON_IDR_SLICE 0x01
#define IDR_SLICE 0x05
#define VER_SEI 0x06
#define SPS 0x07
#define PPS 0x08
#define MVC_PREFIX_NAL 0x0E
#define SUB_SPS 0x0F
#define SLICE_EXT 0x14

// Slice_type
#define P_Slice 0
#define B_Slice 1
#define I_Slice 2
#define SP_Slice 3
#define SI_Slice 4
#define P_Slice_ALL 5
#define B_Slice_ALL 6
#define I_Slice_ALL 7
#define SP_Slice_ALL 8
#define SI_Slice_ALL 9

#define NO_PIC 0
#define TOP_FIELD 1
#define BOTTOM_FIELD 2
#define FRAME 3
#define TOP_BOTTOM_FIELD 4

#define NREF_PIC 0
#define SREF_PIC 1
#define LREF_PIC 2

#define FREXT_HP        100      //!< YUV 4:2:0/8 "High"
#define FREXT_Hi10P     110      //!< YUV 4:2:0/10 "High 10"
#define FREXT_Hi422     122      //!< YUV 4:2:2/10 "High 4:2:2"
#define FREXT_Hi444     144      //!< YUV 4:4:4/12 "High 4:4:4"

#define YUV400 0
#define YUV420 1
#define YUV422 2
#define YUV444 3

#define SubWidthC  [4]= { 1, 2, 2, 1};
#define SubHeightC [4]= { 1, 2, 1, 1};


// ginny for WMV

/***************/
/* definitions */
/***************/
// WMV start code(s)
#define WMV_SC_PREFIX 0x000001
#define WMV_SC_ENDOFSEQ     0x10A
#define WMV_SC_SLICE        0x10B
#define WMV_SC_FIELD        0x10C
#define WMV_SC_FRAME        0x10D
#define WMV_SC_ENTRY        0x10E
#define WMV_SC_SEQ          0x10F
#define WMV_SC_SLICE_DATA   0x11B
#define WMV_SC_FIELD_DATA   0x11C
#define WMV_SC_FRAME_DATA   0x11D
#define WMV_SC_ENTRY_DATA   0x11E
#define WMV_SC_SEQ_DATA     0x11F

#define PP_NO_SCALE             (0x0)
#define PP_SCALE_DOWN           (0x1)
#define PP_SCALE_UP             (0x1 << 1)


#define OFF 0
#define ON 1

#define MAX_USER_DATA_SIZE      200
#define MAX_RETRY_COUNT1 200*1024
#define MAX_VER_RETRY_COUNT 36

#define MAXHALFQP           8
#define MAXHIGHRATEQP       8
#define MAX3QP              8

#define MIN_BITRATE_MB_TABLE 50
#define MAX_BITRATE_DCPred_IMBInPFrame 128
#define NUMBITS_SLICE_SIZE 5 // maximum 32 MB's
#define NUMBITS_SLICE_SIZE_WMV2 3 // To indicate Processor's #




#define MAX_PIC_IDX_TAB_NUM 10000

#ifndef MAKEFOURCC_WMV
#define MAKEFOURCC_WMV(ch0, ch1, ch2, ch3) \
        ((UINT32)(UCHAR)(ch0) | ((UINT32)(UCHAR)(ch1) << 8) |   \
        ((UINT32)(UCHAR)(ch2) << 16) | ((UINT32)(UCHAR)(ch3) << 24 ))

#define mmioFOURCC_WMV(ch0, ch1, ch2, ch3)  MAKEFOURCC_WMV(ch0, ch1, ch2, ch3)
#endif

#define FOURCC_WVC1_WMV     mmioFOURCC_WMV('W','V','C','1')
#define FOURCC_WMVA_WMV     mmioFOURCC_WMV('W','M','V','A')
#define FOURCC_wmva_WMV     mmioFOURCC_WMV('w','m','v','a')
#define FOURCC_WMVP_WMV     mmioFOURCC_WMV('W','M','V','P')
#define FOURCC_wmvp_WMV     mmioFOURCC_WMV('w','m','v','p')
#define FOURCC_WMV3_WMV     mmioFOURCC_WMV('W','M','V','3')
#define FOURCC_wmv3_WMV     mmioFOURCC_WMV('w','m','v','3')
#define FOURCC_WMV2_WMV     mmioFOURCC_WMV('W','M','V','2')
#define FOURCC_wmv2_WMV     mmioFOURCC_WMV('w','m','v','2')
#define FOURCC_WMV1_WMV     mmioFOURCC_WMV('W','M','V','1')
#define FOURCC_wmv1_WMV     mmioFOURCC_WMV('w','m','v','1')
#define FOURCC_M4S2_WMV     mmioFOURCC_WMV('M','4','S','2')
#define FOURCC_m4s2_WMV     mmioFOURCC_WMV('m','4','s','2')
#define FOURCC_MP43_WMV     mmioFOURCC_WMV('M','P','4','3')
#define FOURCC_mp43_WMV     mmioFOURCC_WMV('m','p','4','3')
#define FOURCC_MP4S_WMV     mmioFOURCC_WMV('M','P','4','S')
#define FOURCC_mp4s_WMV     mmioFOURCC_WMV('m','p','4','s')
#define FOURCC_MP42_WMV     mmioFOURCC_WMV('M','P','4','2')
#define FOURCC_mp42_WMV     mmioFOURCC_WMV('m','p','4','2')
#endif
// ~ginny for WMV

// ginny for MPEG
// start code
#define PICTURE_START_CODE      0x100
#define SLICE_START_CODE_MIN    0x101
#define SLICE_START_CODE_MAX    0x1AF
#define USER_DATA_START_CODE    0x1B2
#define SEQUENCE_HEADER_CODE    0x1B3
#define SEQUENCE_ERROR_CODE     0x1B4
#define EXTENSION_START_CODE    0x1B5
#define SEQUENCE_END_CODE       0x1B7
#define GROUP_START_CODE        0x1B8
//#define SYSTEM_START_CODE_MIN   0x1B9
//#define SYSTEM_START_CODE_MAX   0x1FF

#define VIDEO_OBJECT_START_CODE_MIN       0x100
#define VIDEO_OBJECT_START_CODE_MAX       0x11F
#define VIDEO_OBJECT_LAYER_START_CODE_MIN 0x120
#define VIDEO_OBJECT_LAYER_START_CODE_MAX 0x12F
#define USER_DATA_START_CODE              0x1B2
#define GROUP_OF_VOP_START_CODE           0x1B3
#define VISUAL_OBJECT_START_CODE          0x1B5
#define VOP_START_CODE                    0x1B6
#define STUFFING_START_CODE               0x1C3
#define SHORT_VIDEO_START_MASK            0xfffffc00
#define SHORT_VIDEO_START_MARKER          0x00008000
#define H263_VIDEO_START_MASK             0xffff8000
#define H263_VIDEO_START_MARKER          0x00008000

// extension start code IDs
#define SEQUENCE_EXTENSION_ID                    1
#define SEQUENCE_DISPLAY_EXTENSION_ID            2
#define QUANT_MATRIX_EXTENSION_ID                3
#define COPYRIGHT_EXTENSION_ID                   4
#define SEQUENCE_SCALABLE_EXTENSION_ID           5
#define PICTURE_DISPLAY_EXTENSION_ID             7
#define PICTURE_CODING_EXTENSION_ID              8
#define PICTURE_SPATIAL_SCALABLE_EXTENSION_ID    9
#define PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID  10

#define LEVEL_ID_LOW        10
#define LEVEL_ID_MAIN       8
#define LEVEL_ID_HIGH_1440  6
#define LEVEL_ID_HIGH       4

#define DEFAULT_H_SIZE      720
#define DEFAULT_V_SIZE      480
// ~ginny for MPEG
