#include "vdec_verify_mpv_prov.h"
#include "../hal/vdec_hal_if_common.h"
#include "../hal/vdec_hal_if_vp8.h"
#include "vdec_verify_keydef.h"
#include "vdec_verify_vparser_vp8.h"
#include "../include/vdec_info_vp8.h"
#include "../include/vdec_drv_vp8_info.h"
#include "vdec_verify_file_common.h"
#include "vdec_verify_filesetting.h"
#include "../hal/vdec_hw_vp8.h"
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>

extern int rand(void);
extern void vVerifySetVSyncPrmBufPtr(UINT32 u4InstID, UINT32 u4BufIdx);

INT32 i4VP8_Parse_Frame_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr);
INT32 i4VP8_Parse_Intra_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr);
INT32 i4VP8_Parse_Inter_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr);
BOOL fgIsVP8VDecComplete(UINT32 u4InstID);
void vVerVP8UpdateBufStatus(UINT32 u4InstID);
extern void vDrmaBusySet(UINT32  u4InstID);

static const UINT8 default_coef_probs [VP8_BLOCK_TYPES] [VP8_COEF_BANDS] [VP8_PREV_COEF_CONTEXTS] [VP8_COEF_TOKENS - 1] =
{
{
{
{ 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
{ 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
{ 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128}
},
{
{ 253, 136, 254, 255, 228, 219, 128, 128, 128, 128, 128},
{ 189, 129, 242, 255, 227, 213, 255, 219, 128, 128, 128},
{ 106, 126, 227, 252, 214, 209, 255, 255, 128, 128, 128}
},
{
{ 1, 98, 248, 255, 236, 226, 255, 255, 128, 128, 128},
{ 181, 133, 238, 254, 221, 234, 255, 154, 128, 128, 128},
{ 78, 134, 202, 247, 198, 180, 255, 219, 128, 128, 128}
},
{
{ 1, 185, 249, 255, 243, 255, 128, 128, 128, 128, 128},
{ 184, 150, 247, 255, 236, 224, 128, 128, 128, 128, 128},
{ 77, 110, 216, 255, 236, 230, 128, 128, 128, 128, 128}
},
{
{ 1, 101, 251, 255, 241, 255, 128, 128, 128, 128, 128},
{ 170, 139, 241, 252, 236, 209, 255, 255, 128, 128, 128},
{ 37, 116, 196, 243, 228, 255, 255, 255, 128, 128, 128}
},
{
{ 1, 204, 254, 255, 245, 255, 128, 128, 128, 128, 128},
{ 207, 160, 250, 255, 238, 128, 128, 128, 128, 128, 128},
{ 102, 103, 231, 255, 211, 171, 128, 128, 128, 128, 128}
},
{
{ 1, 152, 252, 255, 240, 255, 128, 128, 128, 128, 128},
{ 177, 135, 243, 255, 234, 225, 128, 128, 128, 128, 128},
{ 80, 129, 211, 255, 194, 224, 128, 128, 128, 128, 128}
},
{
{ 1, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
{ 246, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
{ 255, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128}
}
},
{
{
{ 198, 35, 237, 223, 193, 187, 162, 160, 145, 155, 62},
{ 131, 45, 198, 221, 172, 176, 220, 157, 252, 221, 1},
{ 68, 47, 146, 208, 149, 167, 221, 162, 255, 223, 128}
},
{
{ 1, 149, 241, 255, 221, 224, 255, 255, 128, 128, 128},
{ 184, 141, 234, 253, 222, 220, 255, 199, 128, 128, 128},
{ 81, 99, 181, 242, 176, 190, 249, 202, 255, 255, 128}
},
{
{ 1, 129, 232, 253, 214, 197, 242, 196, 255, 255, 128},
{ 99, 121, 210, 250, 201, 198, 255, 202, 128, 128, 128},
{ 23, 91, 163, 242, 170, 187, 247, 210, 255, 255, 128}
},
{
{ 1, 200, 246, 255, 234, 255, 128, 128, 128, 128, 128},
{ 109, 178, 241, 255, 231, 245, 255, 255, 128, 128, 128},
{ 44, 130, 201, 253, 205, 192, 255, 255, 128, 128, 128}
},
{
{ 1, 132, 239, 251, 219, 209, 255, 165, 128, 128, 128},
{ 94, 136, 225, 251, 218, 190, 255, 255, 128, 128, 128},
{ 22, 100, 174, 245, 186, 161, 255, 199, 128, 128, 128}
},
{
{ 1, 182, 249, 255, 232, 235, 128, 128, 128, 128, 128},
{ 124, 143, 241, 255, 227, 234, 128, 128, 128, 128, 128},
{ 35, 77, 181, 251, 193, 211, 255, 205, 128, 128, 128}
},
{
{ 1, 157, 247, 255, 236, 231, 255, 255, 128, 128, 128},
{ 121, 141, 235, 255, 225, 227, 255, 255, 128, 128, 128},
{ 45, 99, 188, 251, 195, 217, 255, 224, 128, 128, 128}
},
{
{ 1, 1, 251, 255, 213, 255, 128, 128, 128, 128, 128},
{ 203, 1, 248, 255, 255, 128, 128, 128, 128, 128, 128},
{ 137, 1, 177, 255, 224, 255, 128, 128, 128, 128, 128}
}
},
{
{
{ 253, 9, 248, 251, 207, 208, 255, 192, 128, 128, 128},
{ 175, 13, 224, 243, 193, 185, 249, 198, 255, 255, 128},
{ 73, 17, 171, 221, 161, 179, 236, 167, 255, 234, 128}
},
{
{ 1, 95, 247, 253, 212, 183, 255, 255, 128, 128, 128},
{ 239, 90, 244, 250, 211, 209, 255, 255, 128, 128, 128},
{ 155, 77, 195, 248, 188, 195, 255, 255, 128, 128, 128}
},
{
{ 1, 24, 239, 251, 218, 219, 255, 205, 128, 128, 128},
{ 201, 51, 219, 255, 196, 186, 128, 128, 128, 128, 128},
{ 69, 46, 190, 239, 201, 218, 255, 228, 128, 128, 128}
},
{
{ 1, 191, 251, 255, 255, 128, 128, 128, 128, 128, 128},
{ 223, 165, 249, 255, 213, 255, 128, 128, 128, 128, 128},
{ 141, 124, 248, 255, 255, 128, 128, 128, 128, 128, 128}
},
{
{ 1, 16, 248, 255, 255, 128, 128, 128, 128, 128, 128},
{ 190, 36, 230, 255, 236, 255, 128, 128, 128, 128, 128},
{ 149, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128}
},
{
{ 1, 226, 255, 128, 128, 128, 128, 128, 128, 128, 128},
{ 247, 192, 255, 128, 128, 128, 128, 128, 128, 128, 128},
{ 240, 128, 255, 128, 128, 128, 128, 128, 128, 128, 128}
},
{
{ 1, 134, 252, 255, 255, 128, 128, 128, 128, 128, 128},
{ 213, 62, 250, 255, 255, 128, 128, 128, 128, 128, 128},
{ 55, 93, 255, 128, 128, 128, 128, 128, 128, 128, 128}
},
{
{ 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
{ 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128},
{ 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128}
}
},
{
{
{ 202, 24, 213, 235, 186, 191, 220, 160, 240, 175, 255},
{ 126, 38, 182, 232, 169, 184, 228, 174, 255, 187, 128},
{ 61, 46, 138, 219, 151, 178, 240, 170, 255, 216, 128}
},
{
{ 1, 112, 230, 250, 199, 191, 247, 159, 255, 255, 128},
{ 166, 109, 228, 252, 211, 215, 255, 174, 128, 128, 128},
{ 39, 77, 162, 232, 172, 180, 245, 178, 255, 255, 128}
},
{
{ 1, 52, 220, 246, 198, 199, 249, 220, 255, 255, 128},
{ 124, 74, 191, 243, 183, 193, 250, 221, 255, 255, 128},
{ 24, 71, 130, 219, 154, 170, 243, 182, 255, 255, 128}
},
{
{ 1, 182, 225, 249, 219, 240, 255, 224, 128, 128, 128},
{ 149, 150, 226, 252, 216, 205, 255, 171, 128, 128, 128},
{ 28, 108, 170, 242, 183, 194, 254, 223, 255, 255, 128}
},
{
{ 1, 81, 230, 252, 204, 203, 255, 192, 128, 128, 128},
{ 123, 102, 209, 247, 188, 196, 255, 233, 128, 128, 128},
{ 20, 95, 153, 243, 164, 173, 255, 203, 128, 128, 128}
},
{
{ 1, 222, 248, 255, 216, 213, 128, 128, 128, 128, 128},
{ 168, 175, 246, 252, 235, 205, 255, 255, 128, 128, 128},
{ 47, 116, 215, 255, 211, 212, 255, 255, 128, 128, 128}
},
{
{ 1, 121, 236, 253, 212, 214, 255, 255, 128, 128, 128},
{ 141, 84, 213, 252, 201, 202, 255, 219, 128, 128, 128},
{ 42, 80, 160, 240, 162, 185, 255, 205, 128, 128, 128}
},
{
{ 1, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
{ 244, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128},
{ 238, 1, 255, 128, 128, 128, 128, 128, 128, 128, 128}
}
}
};

static const INT16 _i2rVp8DcQLookup[VP8_QINDEX_RANGE] =
{
    4,    5,    6,    7,    8,    9,   10,   10,   11,   12,   13,   14,   15,   16,   17,   17,
    18,   19,   20,   20,   21,   21,   22,   22,   23,   23,   24,   25,   25,   26,   27,   28,
    29,   30,   31,   32,   33,   34,   35,   36,   37,   37,   38,   39,   40,   41,   42,   43,
    44,   45,   46,   46,   47,   48,   49,   50,   51,   52,   53,   54,   55,   56,   57,   58,
    59,   60,   61,   62,   63,   64,   65,   66,   67,   68,   69,   70,   71,   72,   73,   74,
    75,   76,   76,   77,   78,   79,   80,   81,   82,   83,   84,   85,   86,   87,   88,   89,
    91,   93,   95,   96,   98,  100,  101,  102,  104,  106,  108,  110,  112,  114,  116,  118,
    122,  124,  126,  128,  130,  132,  134,  136,  138,  140,  143,  145,  148,  151,  154,  157,
};

static const INT16 _i2rVp8AcQLookup[VP8_QINDEX_RANGE] =
{
    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,   15,   16,   17,   18,   19,
    20,   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,
    36,   37,   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51,
    52,   53,   54,   55,   56,   57,   58,   60,   62,   64,   66,   68,   70,   72,   74,   76,
    78,   80,   82,   84,   86,   88,   90,   92,   94,   96,   98,  100,  102,  104,  106,  108,
    110,  112,  114,  116,  119,  122,  125,  128,  131,  134,  137,  140,  143,  146,  149,  152,
    155,  158,  161,  164,  167,  170,  173,  177,  181,  185,  189,  193,  197,  201,  205,  209,
    213,  217,  221,  225,  229,  234,  239,  245,  249,  254,  259,  264,  269,  274,  279,  284,
};

static const VDEC_VP8_MV_CONTEXT_T vp8_default_mv_context[2] =
{
    {{
        // row
        162,                                        // is short
        128,                                        // sign
        225, 146, 172, 147, 214,  39, 156,          // short tree
        128, 129, 132,  75, 145, 178, 206, 239, 254, 254 // long bits
    }},

    {{
        // same for column
        164,                                        // is short
        128,
        204, 170, 119, 235, 140, 230, 228,
        128, 130, 130,  74, 148, 180, 203, 236, 254, 254 // long bits

    }}
};

#ifndef VP8_HEADERPARSE_HWACCELATOR
static const UINT8 _uVp8CoefUpdateProbs[VP8_BLOCK_TYPES] [VP8_COEF_BANDS] [VP8_PREV_COEF_CONTEXTS] [VP8_COEF_TOKENS-1] =
{
        {
            {
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {176, 246, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {223, 241, 252, 255, 255, 255, 255, 255, 255, 255, 255, },
                {249, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 244, 252, 255, 255, 255, 255, 255, 255, 255, 255, },
                {234, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 246, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {239, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {251, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {251, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {254, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 254, 253, 255, 254, 255, 255, 255, 255, 255, 255, },
                {250, 255, 254, 255, 254, 255, 255, 255, 255, 255, 255, },
                {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
        },
        {
            {
                {217, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {225, 252, 241, 253, 255, 255, 254, 255, 255, 255, 255, },
                {234, 250, 241, 250, 253, 255, 253, 254, 255, 255, 255, },
            },
            {
                {255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {223, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {238, 253, 254, 254, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 248, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {249, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {247, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {252, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255, },
                {250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
        },
        {
            {
                {186, 251, 250, 255, 255, 255, 255, 255, 255, 255, 255, },
                {234, 251, 244, 254, 255, 255, 255, 255, 255, 255, 255, },
                {251, 251, 243, 253, 254, 255, 254, 255, 255, 255, 255, },
            },
            {
                {255, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {236, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {251, 253, 253, 254, 254, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {254, 254, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {254, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
        },
        {
            {
                {248, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {250, 254, 252, 254, 255, 255, 255, 255, 255, 255, 255, },
                {248, 254, 249, 253, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255, },
                {246, 253, 253, 255, 255, 255, 255, 255, 255, 255, 255, },
                {252, 254, 251, 254, 254, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 254, 252, 255, 255, 255, 255, 255, 255, 255, 255, },
                {248, 254, 253, 255, 255, 255, 255, 255, 255, 255, 255, },
                {253, 255, 254, 254, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {245, 251, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {253, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 251, 253, 255, 255, 255, 255, 255, 255, 255, 255, },
                {252, 253, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 254, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 252, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {249, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 254, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 255, 253, 255, 255, 255, 255, 255, 255, 255, 255, },
                {250, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
            {
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {254, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
                {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, },
            },
       },
};
const VDEC_VP8_MV_CONTEXT_T vp8_mv_update_probs[2] =
{
    {{
        237,
        246,
        253, 253, 254, 254, 254, 254, 254,
        254, 254, 254, 254, 254, 250, 250, 252, 254, 254
    }},
    {{
        231,
        243,
        245, 253, 254, 254, 254, 254, 254,
        254, 254, 254, 254, 254, 251, 251, 254, 254, 254
    }}
};
#endif

#ifndef VP8_USE_SMALLQT
static INT8 _Vp8DefaultZigZag1d[16]=
{
  0,1,4,8,
  5,2,3,6,
  9,12,13,10,
  7,11,14,15
};
static INT16 _rVp8Dequant[VP8_QTALBE_MAX][VP8_QINDEX_RANGE][4][4];
#else
static INT16 _rVp8Dequant[VP8_QTALBE_MAX][VP8_QINDEX_RANGE][1][2];
#endif
static INT32 _Vp8MbFeatureDataBits[VP8_MBLVL_MAX]={7,6};


void vVDEC_Vp8DumpReg(void)
{
    UINT32 i;
    /* VLD Registers */
    printk("******* VLD Registers *******\n");
    msleep(2);
    printk("VLD1 ReadPtr: 0x%08x\n", u4VDecReadVLD(0, RW_VLD_RPTR));
    msleep(2);
    printk("VLD2 ReadPtr: 0x%08x\n", u4VDecReadVP8VLD2(0, RW_VLD_RPTR));
    msleep(2);
    
    /* VP8 VLD */
    printk("******* VP8 VLD Registers *******\n");
    msleep(2);
    for (i = 41; i < 68; i++)
    {
        printk("VP8 VLD %d (0x%04x): 0x%08x\n", i, i * 4, u4VDecReadVP8VLD(0,i * 4));
        msleep(2);
    }
    
    for (i = 72; i < 97; i++)
    {
       printk("VP8 VLD %d (0x%04x): 0x%08x\n", i, i * 4, u4VDecReadVP8VLD(0, i * 4));
       msleep(2);
    }

    /* TOP VLD */
    printk("******* TOP VLD Registers *******\n");
    msleep(2);
    for (i = 0; i < 100; i++)
    {
        printk("TOP VLD %d (0x%04x): 0x%08x\n", i, i * 4, u4VDecReadVLDTOP(0, i * 4));
        msleep(2);
    }
    
    /* MC */
    printk("******* MC Registers *******\n");
    msleep(2);
    for (i = 0; i < 700; i++)
    {
        printk("MC %d (0x%04x): 0x%08x\n", i, i * 4, u4VDecReadMC(0, i * 4));
        msleep(2);
    }
    
    /* MV */
    printk("******* MV Registers *******\n");
    msleep(2);
    printk("MV 181 (0x0x02D4): 0x%08x\n", u4VDecReadVP8MV(0, i * 4));
    msleep(2);
    
    /* PP */
    printk("******* PP Registers *******\n");
    msleep(2);
    for (i = 0; i < 30; i++)
    {
        printk("PP %d (0x%04x): 0x%08x\n", i, i * 4, u4VDecReadVP8PP(0, i * 4));
        msleep(2);
    }
}


// *********************************************************************
// Function : BOOL u4VerVParserVP8(UINT32 u4InstID)
// Description :
// Parameter :
// Return    :
// *********************************************************************
UINT32 u4VerVParserVP8(UINT32 u4InstID, BOOL fgInquiry)
{
//    UINT32 u4Retry = 0;
    INT32  i4RetVal;
    UINT32 u4BsId = 0;
//    VDEC_INFO_VP8_VFIFO_PRM_T rVp8VFifoInitPrm;
    VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];
   // VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdrTmp;
   // prVDecVp8FrmHdrTmp = prVDecVp8FrmHdr;
     fgVDEC_Vp8DecReset(u4InstID, prVDecVp8FrmHdr, FALSE);
   // prVDecVp8FrmHdrTmp->u4FifoStart = PHYSICAL(prVDecVp8FrmHdr->u4FifoStart);
   // prVDecVp8FrmHdrTmp->u4FifoEnd = PHYSICAL(prVDecVp8FrmHdr->u4FifoEnd);
   // prVDecVp8FrmHdrTmp->u4VldStartPos = PHYSICAL(prVDecVp8FrmHdr->u4VldStartPos);
   // prVDecVp8FrmHdrTmp->u4WritePos = PHYSICAL(prVDecVp8FrmHdr->u4WritePos);
    i4RetVal = i4VP8_Parse_Frame_Header(u4BsId, u4InstID, prVDecVp8FrmHdr);
        
    if(i4RetVal == 0)
    {
        return(i4RetVal);
    }   
    _rVDecVp8FrmHdr->u4Height=prVDecVp8FrmHdr->u4Height;
    _rVDecVp8FrmHdr->u4Width=prVDecVp8FrmHdr->u4Width;

    return(i4RetVal);
}

static VOID _VDEC_Vp8VersonCfg(UINT32 u4Verson,VDEC_PARAM_VP8DEC_T *pVP8Param)
{
    switch(u4Verson)
    {
       case 0:
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_NO_LPF);
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_SIMPLER_LPF);
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_BILINER_MCFILTER);
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_FULL_PIXEL);
         break;
       case 1:
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_NO_LPF);
         VDEC_SETFLG(pVP8Param->u4FlagParam,VP8PARAM_SIMPLER_LPF);
         VDEC_SETFLG(pVP8Param->u4FlagParam,VP8PARAM_BILINER_MCFILTER);
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_FULL_PIXEL);
         break;
       case 2:
         VDEC_SETFLG(pVP8Param->u4FlagParam,VP8PARAM_NO_LPF);
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_SIMPLER_LPF);
         VDEC_SETFLG(pVP8Param->u4FlagParam,VP8PARAM_BILINER_MCFILTER);
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_FULL_PIXEL);        
         break;
       case 3:
         VDEC_SETFLG(pVP8Param->u4FlagParam,VP8PARAM_NO_LPF);
         VDEC_SETFLG(pVP8Param->u4FlagParam,VP8PARAM_SIMPLER_LPF);
         VDEC_SETFLG(pVP8Param->u4FlagParam,VP8PARAM_BILINER_MCFILTER);
         VDEC_SETFLG(pVP8Param->u4FlagParam,VP8PARAM_FULL_PIXEL);        
         break;
       default:
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_NO_LPF);
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_SIMPLER_LPF);
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_BILINER_MCFILTER);
         VDEC_CLRFLG(pVP8Param->u4FlagParam,VP8PARAM_FULL_PIXEL);
         break;
    }
}

static VOID _VDEC_Vp8InitDeQTable(VDEC_PARAM_VP8DEC_T *pVp8DecParam,BOOL *fgChange)
{
  UINT32 u4Index=0,u4QType=0;
  INT16 i2Q=0,i2Qvalue=0,i2QNewIndex;

  for(u4QType=VP8_QTYPE_Y1AC;u4QType<VP8_QTYPE_MAX;u4QType++)
  {
     if(!fgChange[u4QType])
     {
        continue;
     }

     i2QNewIndex=pVp8DecParam->QIndexInfo[u4QType];
     if(i2QNewIndex==VP8_QTYPE_Y1AC)
     {
        i2QNewIndex=0;
     }
     
     if(u4QType==VP8_QTYPE_Y1DC|| u4QType==VP8_QTYPE_Y2DC || u4QType==VP8_QTYPE_UVDC)
     {
         for(u4Index=0;u4Index<VP8_QINDEX_RANGE;u4Index++)
         {
            i2Q=u4Index+i2QNewIndex;
            i2Q=VDEC_VP8_DQINDEXCLAMP(i2Q);
            i2Qvalue=_i2rVp8DcQLookup[i2Q];
            if(u4QType==VP8_QTYPE_Y2DC)
            {
               i2Qvalue<<=1;
            }
            else if(u4QType==VP8_QTYPE_UVDC)
            {
               i2Qvalue= i2Qvalue>132 ? 132 : i2Qvalue;
            }
            
            _rVp8Dequant[u4QType>>1][u4Index][0][0]=i2Qvalue;
         }
     }
     else
     {
#ifndef VP8_USE_SMALLQT
         UINT32 u4AcIndex=0,u4Row,u4Col;
         for(u4Index=0;u4Index<VP8_QINDEX_RANGE;u4Index++)
         {
           for(u4AcIndex=1;u4AcIndex<16;u4AcIndex++)
           {
              u4Row=_Vp8DefaultZigZag1d[u4AcIndex]>>2;
              u4Col=_Vp8DefaultZigZag1d[u4AcIndex]&3;
              i2Q=u4Index+i2QNewIndex;
              i2Q=VDEC_VP8_DQINDEXCLAMP(i2Q);
              i2Qvalue=_i2rVp8AcQLookup[i2Q];
              if(u4QType==VP8_QTYPE_Y2AC)
              {
                i2Qvalue=(i2Qvalue*155)/100;
                if(i2Qvalue<8)
                {
                  i2Qvalue=8;
                }
              }
              _rVp8Dequant[u4QType>>1][u4Index][u4Row][u4Col]=i2Qvalue;
           }
         }
#else
         for(u4Index=0;u4Index<VP8_QINDEX_RANGE;u4Index++)
         {
            i2Q=u4Index+i2QNewIndex;
            i2Q=VDEC_VP8_DQINDEXCLAMP(i2Q);
            i2Qvalue=_i2rVp8AcQLookup[i2Q];
            if(u4QType==VP8_QTYPE_Y2AC)
            {
               i2Qvalue=(i2Qvalue*155)/100;
               if(i2Qvalue<8)
               {
                 i2Qvalue=8;
               }
            }
            _rVp8Dequant[u4QType>>1][u4Index][0][1]=i2Qvalue;
         }
#endif
     }
  }

}


static VOID _VDEC_Vp8MbInitDequantizer(UINT32 u4InstID, VDEC_PARAM_VP8DEC_T *prVp8DecParam)
{
   UINT32 u4SegIndex=0,u4Value;
   INT32 i4QIndex;
   if(1)
   {
      for(u4SegIndex=0;u4SegIndex<MAX_MB_SEGMENTS;u4SegIndex++)
      {
         if(VDEC_FLGSET(prVp8DecParam->u4FlagParam,VP8PARAM_SEGMENT_ENABLE))
         {
            if(VDEC_FLGSET(prVp8DecParam->u4FlagParam,VP8PARAM_SEGMENT_ABSDATA))
            {
               i4QIndex=prVp8DecParam->SegmentFeatureData[VP8_MBLVL_ALT_Q][u4SegIndex];
            }
            else
            {
               i4QIndex=prVp8DecParam->QIndexInfo[VP8_QTYPE_Y1AC]+prVp8DecParam->SegmentFeatureData[VP8_MBLVL_ALT_Q][u4SegIndex];
               i4QIndex = VDEC_VP8_DQINDEXCLAMP(i4QIndex);
            }
         }
         else
         {
            i4QIndex=prVp8DecParam->QIndexInfo[VP8_QTYPE_Y1AC];
         }

         
         u4Value=(_rVp8Dequant[VP8_QTABLE_Y2][i4QIndex][0][0]>>8)&0xff;
         u4Value=(u4Value<<8)|(_rVp8Dequant[VP8_QTABLE_Y2][i4QIndex][0][0]&0xff);
         u4Value=(u4Value<<8)|((_rVp8Dequant[VP8_QTABLE_Y2][i4QIndex][0][1]>>8)&0xff);
         u4Value=(u4Value<<8)|(_rVp8Dequant[VP8_QTABLE_Y2][i4QIndex][0][1]&0xff);         
         vVDecWriteVLD(u4InstID, RW_VLD_SCL_ADDR, (u4SegIndex<<4));
         vVDecWriteVLD(u4InstID, RW_VLD_SCL_DATA, u4Value);

         u4Value=(_rVp8Dequant[VP8_QTABLE_Y1][i4QIndex][0][0]>>8)&0xff;
         u4Value=(u4Value<<8)|(_rVp8Dequant[VP8_QTABLE_Y1][i4QIndex][0][0]&0xff);
         u4Value=(u4Value<<8)|((_rVp8Dequant[VP8_QTABLE_Y1][i4QIndex][0][1]>>8)&0xff);
         u4Value=(u4Value<<8)|(_rVp8Dequant[VP8_QTABLE_Y1][i4QIndex][0][1]&0xff);
         vVDecWriteVLD(u4InstID, RW_VLD_SCL_ADDR,(u4SegIndex<<4)+4);
         vVDecWriteVLD(u4InstID, RW_VLD_SCL_DATA,u4Value);

         u4Value=(_rVp8Dequant[VP8_QTABLE_UV][i4QIndex][0][0]>>8)&0xff;
         u4Value=(u4Value<<8)|(_rVp8Dequant[VP8_QTABLE_UV][i4QIndex][0][0]&0xff);
         u4Value=(u4Value<<8)|((_rVp8Dequant[VP8_QTABLE_UV][i4QIndex][0][1]>>8)&0xff);
         u4Value=(u4Value<<8)|(_rVp8Dequant[VP8_QTABLE_UV][i4QIndex][0][1]&0xff);
         vVDecWriteVLD(u4InstID, RW_VLD_SCL_ADDR,(u4SegIndex<<4)+8);
         vVDecWriteVLD(u4InstID, RW_VLD_SCL_DATA,u4Value);
      }
   }
}

static VOID _VDEC_Vp8LoadCoefProbs(VDEC_VP8FRAME_CONTEXT_T *prCtxt)
{
   UINT32 u4BusSramAddr=0,u4CoefProb=0;
   UINT32 u4BlockType=0,u4Band=0,u4Context=0;
   for(u4BlockType=0;u4BlockType<VP8_BLOCK_TYPES;u4BlockType++)
   {
      for(u4Band=0;u4Band<VP8_COEF_BANDS;u4Band++)
      {
         for(u4Context=0;u4Context<VP8_PREV_COEF_CONTEXTS;u4Context++)
         {  
            vVDecWriteVP8VLD(0,RW_VP8_BSASET,u4BusSramAddr,0);
            u4CoefProb=u4VDecReadVP8VLD(0,RW_VP8_BSDSET);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][0]=(UINT8)(u4CoefProb&0xff);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][1]=(UINT8)((u4CoefProb>>8)&0xff);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][2]=(UINT8)((u4CoefProb>>16)&0xff);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][3]=(UINT8)((u4CoefProb>>24)&0xff);
            u4CoefProb=u4VDecReadVP8VLD(0,RW_VP8_BSDSET);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][4]=(UINT8)(u4CoefProb&0xff);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][5]=(UINT8)((u4CoefProb>>8)&0xff);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][6]=(UINT8)((u4CoefProb>>16)&0xff);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][7]=(UINT8)((u4CoefProb>>24)&0xff);
            u4CoefProb=u4VDecReadVP8VLD(0,RW_VP8_BSDSET);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][8]=(UINT8)(u4CoefProb&0xff);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][9]=(UINT8)((u4CoefProb>>8)&0xff);
            prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][10]=(UINT8)((u4CoefProb>>16)&0xff);
            u4BusSramAddr+=4;
         }
      }
   }  
}

static VOID _VDEC_Vp8LoadCtxProbs(VDEC_VP8FRAME_CONTEXT_T *prCtxt)
{
  UINT32 u4HalValue=0;
  UINT8 *prMvc=(UINT8 *)prCtxt->MVC;
  
  vVDecWriteVP8VLD(0,RW_VP8_CSASET,4,0);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prCtxt->YModeProb[0]=(UINT8)(u4HalValue&0xff);
  prCtxt->YModeProb[1]=(UINT8)((u4HalValue>>8)&0xff);
  prCtxt->YModeProb[2]=(UINT8)((u4HalValue>>16)&0xff);
  prCtxt->YModeProb[3]=(UINT8)((u4HalValue>>24)&0xff);

  
  vVDecWriteVP8VLD(0,RW_VP8_CSASET,8,0);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prCtxt->UVModeProb[0]=(UINT8)(u4HalValue&0xff);
  prCtxt->UVModeProb[1]=(UINT8)((u4HalValue>>8)&0xff);
  prCtxt->UVModeProb[2]=(UINT8)((u4HalValue>>16)&0xff);


  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_MVPROB);
  prMvc[19]=(UINT8)(u4HalValue&0xff);
  prMvc[0]=(UINT8)((u4HalValue>>8)&0xff);
  prMvc[20]=(UINT8)((u4HalValue>>16)&0xff);
  prMvc[1]=(UINT8)((u4HalValue>>24)&0xff);

  
    vVDecWriteVP8VLD(0,RW_VP8_CSASET,0xC,0);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[2]=(UINT8)(u4HalValue&0xff);
  prMvc[3]=(UINT8)((u4HalValue>>8)&0xff);
  prMvc[4]=(UINT8)((u4HalValue>>16)&0xff);
  prMvc[5]=(UINT8)((u4HalValue>>24)&0xff);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[6]=(UINT8)(u4HalValue&0xff);
  prMvc[7]=(UINT8)((u4HalValue>>8)&0xff);
  prMvc[8]=(UINT8)((u4HalValue>>16)&0xff); 

  
   vVDecWriteVP8VLD(0,RW_VP8_CSASET,0x10,0);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[9]=(UINT8)(u4HalValue&0xff);
  prMvc[10]=(UINT8)((u4HalValue>>8)&0xff);
  prMvc[11]=(UINT8)((u4HalValue>>16)&0xff);
  prMvc[12]=(UINT8)((u4HalValue>>24)&0xff);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[13]=(UINT8)(u4HalValue&0xff);
  prMvc[14]=(UINT8)((u4HalValue>>8)&0xff);
  prMvc[15]=(UINT8)((u4HalValue>>16)&0xff);
  prMvc[16]=(UINT8)((u4HalValue>>24)&0xff);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[17]=(UINT8)(u4HalValue&0xff);
  prMvc[18]=(UINT8)((u4HalValue>>8)&0xff);

  
   vVDecWriteVP8VLD(0,RW_VP8_CSASET,0X14,0);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[21]=(UINT8)(u4HalValue&0xff);
  prMvc[22]=(UINT8)((u4HalValue>>8)&0xff);
  prMvc[23]=(UINT8)((u4HalValue>>16)&0xff);
  prMvc[24]=(UINT8)((u4HalValue>>24)&0xff);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[25]=(UINT8)(u4HalValue&0xff);
  prMvc[26]=(UINT8)((u4HalValue>>8)&0xff);
  prMvc[27]=(UINT8)((u4HalValue>>16)&0xff);

  
   vVDecWriteVP8VLD(0,RW_VP8_CSASET,0X18,0);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[28]=(UINT8)(u4HalValue&0xff);
  prMvc[29]=(UINT8)((u4HalValue>>8)&0xff);
  prMvc[30]=(UINT8)((u4HalValue>>16)&0xff);
  prMvc[31]=(UINT8)((u4HalValue>>24)&0xff);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[32]=(UINT8)(u4HalValue&0xff);
  prMvc[33]=(UINT8)((u4HalValue>>8)&0xff);
  prMvc[34]=(UINT8)((u4HalValue>>16)&0xff);
  prMvc[35]=(UINT8)((u4HalValue>>24)&0xff);
  u4HalValue=u4VDecReadVP8VLD(0,RW_VP8_CSDSET);
  prMvc[36]=(UINT8)(u4HalValue&0xff);
  prMvc[37]=(UINT8)((u4HalValue>>8)&0xff);
}
static VOID vDEC_Vp8FlushCoefProbs(UINT32 u4InstID, VDEC_VP8FRAME_CONTEXT_T *prCtxt)
{
   UINT32 u4BusSramAddr=0,u4CoefProb=0,u4Mod=0;
   UINT32 u4BlockType=0,u4Band=0,u4Context=0,u4Tolen=0;
   for(u4BlockType=0;u4BlockType<VP8_BLOCK_TYPES;u4BlockType++)
   {
      for(u4Band=0;u4Band<VP8_COEF_BANDS;u4Band++)
      {
         for(u4Context=0;u4Context<VP8_PREV_COEF_CONTEXTS;u4Context++)
         {  
         
            vVDecWriteVP8VLD(u4InstID, RW_VP8_BSASET,u4BusSramAddr,0);
            for(u4Tolen=0;u4Tolen<VP8_COEF_TOKENS-1;u4Tolen++)
            {
               u4Mod=u4Tolen%4;
               if(!u4Mod)
               {
                 u4CoefProb=0;
               }
               
               u4CoefProb+=prCtxt->CoefProbs[u4BlockType][u4Band][u4Context][u4Tolen]<<(u4Mod<<3);
               if(u4Mod==3)
               {
               
                 vVDecWriteVP8VLD(u4InstID, RW_VP8_BSDSET,u4CoefProb,0);
               }
            }
            vVDecWriteVP8VLD(u4InstID, RW_VP8_BSDSET,u4CoefProb,0);
            u4BusSramAddr+=4;
         }
      }
   }
}

static VOID _VDEC_Vp8FlushCtxProbs(UINT32 u4InstID, VDEC_VP8FRAME_CONTEXT_T *prCtxt,UINT32 u4FrameType)
{
   UINT32 u4HalValue=0;
   UINT8 *prMvc=(UINT8 *)prCtxt->MVC;
   
   if(u4FrameType==VP8_I_FRM)
   {
       // vp8_default_mv_context
       vVDecWriteVP8VLD(u4InstID, RW_VP8_MVPROB,0x8080a2a4,0); // mv_default_prob_update 128 128 162 164
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,0xc,0);   //CTX STRAM address, MV_SHORT_PROB_Y_Offset
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0x93ac92e1,0); // MV_SHORT_PROB_Y 147 172 146 225 
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0x9c27d6,0);   //MV_SHORT_PROB_Y 156 39 214
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,0x10,0);       // MV_LONG_PROB_Y_OFFSET
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0x4b848180,0); //MV_LONG_PROB_Y 75 132 129 128
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0xefceb291,0); // 239 206 178 145
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0xfefe,0);     // 254 254
       
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,0x14,0);       // mv_short_prob_x_offset
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0xeb77aacc,0); 
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0xe4e68c,0);
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,0x18,0);       // mv_long_prob_x_offset
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0x4a828280,0); 
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0xeccbb494,0);
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,0xfefe,0);

   }
   else
   {
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,4,0);
       u4HalValue=(prCtxt->YModeProb[3]<<24)+(prCtxt->YModeProb[2]<<16)
                  +(prCtxt->YModeProb[1]<<8)+prCtxt->YModeProb[0];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,8,0);
       u4HalValue=(prCtxt->UVModeProb[2]<<16)+(prCtxt->UVModeProb[1]<<8)
                  +prCtxt->UVModeProb[0];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       
       u4HalValue=(prMvc[1]<<24)+(prMvc[20]<<16)+(prMvc[0]<<8)+prMvc[19];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_MVPROB,u4HalValue,0);
       
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,0XC,0);
       u4HalValue=(prMvc[5]<<24)+(prMvc[4]<<16)+(prMvc[3]<<8)+prMvc[2];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       u4HalValue=(prMvc[8]<<16)+(prMvc[7]<<8)+prMvc[6];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,0X10,0);
       u4HalValue=(prMvc[12]<<24)+(prMvc[11]<<16)+(prMvc[10]<<8)+prMvc[9];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       u4HalValue=(prMvc[16]<<24)+(prMvc[15]<<16)+(prMvc[14]<<8)+prMvc[13];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       u4HalValue=(prMvc[18]<<8)+prMvc[17];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,0X14,0);
       u4HalValue=(prMvc[24]<<24)+(prMvc[23]<<16)+(prMvc[22]<<8)+prMvc[21];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       u4HalValue=(prMvc[27]<<16)+(prMvc[26]<<8)+prMvc[25];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,0X18,0);
       u4HalValue=(prMvc[31]<<24)+(prMvc[30]<<16)+(prMvc[29]<<8)+prMvc[28];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       u4HalValue=(prMvc[35]<<24)+(prMvc[34]<<16)+(prMvc[33]<<8)+prMvc[32];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
       u4HalValue=(prMvc[37]<<8)+prMvc[36];
       vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
   }

}

static UINT32 u4VDEC_Vp8DecInit(VDEC_INFO_VP8_FRM_HDR_T *pVp8DecInfo)
{
  UINT32 u4FlagParam=0;
  VDEC_VP8FRAME_CONTEXT_T *prCtx=&pVp8DecInfo->rVp8DecParam.rCurFc;
  BOOL fgQChange[VP8_QTYPE_MAX]={TRUE,TRUE,TRUE,TRUE,TRUE,TRUE};
  memset(pVp8DecInfo,0,sizeof(VDEC_INFO_VP8_FRM_HDR_T));
  VDEC_SETFLG(u4FlagParam,VP8PARAM_NOCOEF_SKIP);
  VDEC_SETFLG(u4FlagParam,VP8PARAM_REFRESH_PROBS);
  _VDEC_Vp8InitDeQTable(&pVp8DecInfo->rVp8DecParam,fgQChange);
  //memcpy(pVp8DecInfo->rVp8DecParam.rCurFc.PRE_MVC,vp8_default_mv_context,sizeof(vp8_default_mv_context));
  memcpy(prCtx->MVC,vp8_default_mv_context,sizeof(vp8_default_mv_context));
  memcpy(prCtx->CoefProbs,default_coef_probs,sizeof(default_coef_probs));
  prCtx->YModeProb[0]=112;// {112,86,140,37};
  prCtx->YModeProb[1]=86;
  prCtx->YModeProb[2]=140;
  prCtx->YModeProb[3]=37;
  prCtx->UVModeProb[0]=62;//{62,101,204};
  prCtx->UVModeProb[1]=101;
  prCtx->UVModeProb[2]=204;
  memcpy(pVp8DecInfo->rVp8DecParam.rCurFc.CoefProbs,default_coef_probs,sizeof(default_coef_probs));
  memcpy(&pVp8DecInfo->rVp8DecParam.rLastFC,&pVp8DecInfo->rVp8DecParam.rCurFc,sizeof(VDEC_VP8FRAME_CONTEXT_T));
  memset(pVp8DecInfo->rVp8DecParam.SegmentTreeProbs,255,MB_FEATURE_TREE_PROBS);
  pVp8DecInfo->rVp8DecParam.u4FlagParam=u4FlagParam;
  return 0;
}

void vVerInitVP8(UINT32 u4InstID)
{             
    //Open size file

    //Init VP8 Frame Header
    VDEC_INFO_VP8_VFIFO_PRM_T     rVp8VDecInitPrm;
    VDEC_INFO_VP8_FRM_HDR_T  *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];
    _u4TotalDecFrms[u4InstID] = 0;

    rVp8VDecInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rVp8VDecInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ-1;
    u4VDEC_Vp8DecInit(prVDecVp8FrmHdr);
    i4VDEC_HAL_VP8_InitVDecHW(u4InstID, &rVp8VDecInitPrm);
    fgVDEC_Vp8DecReset(u4InstID, prVDecVp8FrmHdr, TRUE);
//    i4VDEC_HAL_VP8_InitVDecHW(u4InstID, &rVp8VDecInitPrm);
    prVDecVp8FrmHdr->u4FifoStart = (UINT32)_pucVFifo[u4InstID];
    prVDecVp8FrmHdr->u4FifoEnd = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ-1;
    prVDecVp8FrmHdr->u4FrameSize = (UINT32)(*(_pucVFifo[u4InstID]+VP8_IVF_FILE_HEADER_SZ))+((UINT32)(*(_pucVFifo[u4InstID]+VP8_IVF_FILE_HEADER_SZ+1))<<8)+((UINT32)(*(_pucVFifo[u4InstID]+VP8_IVF_FILE_HEADER_SZ+2))<<16)+((UINT32)(*(_pucVFifo[u4InstID]+VP8_IVF_FILE_HEADER_SZ+3))<<24); // ivf file header 
    _u4VP8FrmSZ[u4InstID] = prVDecVp8FrmHdr->u4FrameSize;
    prVDecVp8FrmHdr->u4VldStartPos= (UINT32)_pucVFifo[u4InstID]+VP8_IVF_FILE_HEADER_SZ+VP8_IVF_FRAME_HEADER_SZ; // ivf file header + ivf frame header
  #ifndef  RING_VFIFO_SUPPORT
    prVDecVp8FrmHdr->u4WritePos = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ-1;
  #else
//    prVDecVp8FrmHdr->u4WritePos = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
    prVDecVp8FrmHdr->u4WritePos = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
  #endif

    i4VDEC_HAL_VP8_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, prVDecVp8FrmHdr);

   _u4VP8ByteCount[u4InstID] = VP8_IVF_FILE_HEADER_SZ+VP8_IVF_FRAME_HEADER_SZ;
   _u4VP8CurrBufIdx[u4InstID] = 0;    //Current frame buf Index
   _u4VP8LastBufIdx[u4InstID] = 1;    //Last frame buf Index
   _u4VP8GoldfBufIdx[u4InstID] = 2;    //Golden frame buf Index
   _u4VP8AltBufIdx[u4InstID] = 3;    //Alternative frame buf Index

}


INT32 i4VP8_Parse_Frame_Header(UINT32 u4BsId, UINT32 u4InstID, VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr)
{
  VDEC_PARAM_VP8DEC_T *prVp8DecParam=&prVDecVp8FrmHdr->rVp8DecParam;
  UINT32 u4HalValue=0,u4ReadOffset;
  UINT32 u4FlagParam=prVp8DecParam->u4FlagParam;
  
  VDEC_INFO_VP8_FRM_HDR_T prVDecVp8FrmHdrPhysical;
  memcpy(&prVDecVp8FrmHdrPhysical,prVDecVp8FrmHdr,sizeof(VDEC_INFO_VP8_FRM_HDR_T));

   prVDecVp8FrmHdrPhysical.u4FifoStart = PHYSICAL(prVDecVp8FrmHdr->u4FifoStart);
   prVDecVp8FrmHdrPhysical.u4FifoEnd = PHYSICAL(prVDecVp8FrmHdr->u4FifoEnd);
   prVDecVp8FrmHdrPhysical.u4VldStartPos = PHYSICAL(prVDecVp8FrmHdr->u4VldStartPos);
   prVDecVp8FrmHdrPhysical.u4WritePos = PHYSICAL(prVDecVp8FrmHdr->u4WritePos);
  u4HalValue=u4VDecVP8VLDGetBits(VP8_VLD1, u4InstID, 24);
  VDEC_INTREVERSE(u4HalValue,3);
  prVDecVp8FrmHdr->uFrameType=u4HalValue&1;
  prVDecVp8FrmHdr->uVersion=(u4HalValue>>1)&7;
  prVDecVp8FrmHdr->uShowFrame=(u4HalValue>>4)&1;
  prVDecVp8FrmHdr->u4FirstPartLen=u4HalValue>>5;
  prVDecVp8FrmHdr->u4FrameSize-=3;
  u4ReadOffset=3;
  prVDecVp8FrmHdr->u4FrameSize-=prVDecVp8FrmHdr->u4FirstPartLen;
  //VDEC_RPOS_INC(prVDecVp8FrmHdr->u4VldStartPos,3,prVDecVp8FrmHdr->u4FifoStart,prVDecVp8FrmHdr->u4FifoEnd);
  //VDEC_RPOS_INC(prVDecVp8FrmHdr->u4VldStartPos,prVDecVp8FrmHdr->u4FirstPartLen,prVDecVp8FrmHdr->u4FifoStart,prVDecVp8FrmHdr->u4FifoEnd);
  _VDEC_Vp8VersonCfg(prVDecVp8FrmHdr->uVersion,prVp8DecParam);
  u4FlagParam=prVp8DecParam->u4FlagParam;
  if(prVDecVp8FrmHdr->uFrameType==VP8_I_FRM)
  {
     u4HalValue=u4VDecVP8VLDGetBits(VP8_VLD1, u4InstID,24);
     if((u4HalValue)!=0x9d012a)
     {
         printk("VP8 Header Identify error (0x%x)\n",u4HalValue);
         return FALSE;
     }
     
     u4HalValue=u4VDecVP8VLDGetBits(VP8_VLD1, u4InstID,16);
     VDEC_INTREVERSE(u4HalValue,2);
     prVDecVp8FrmHdr->u4Width=u4HalValue&0x3fff;
     prVDecVp8FrmHdr->uHScale=u4HalValue>>14;
     u4HalValue=u4VDecVP8VLDGetBits(VP8_VLD1, u4InstID,16);
     VDEC_INTREVERSE(u4HalValue,2);
     prVDecVp8FrmHdr->u4Height=u4HalValue&0x3fff;
     prVDecVp8FrmHdr->uVScale=u4HalValue>>14;     
     prVDecVp8FrmHdr->u4FrameSize-=7;
     if(prVDecVp8FrmHdr->u4Width==0 || prVDecVp8FrmHdr->u4Height==0)
     {
        printk("VP8 Header W/H error \n");
        return FALSE;
     }
     u4ReadOffset+=7;
     //VDEC_RPOS_INC(prVDecVp8FrmHdr->u4VldStartPos,7,prVDecVp8FrmHdr->u4FifoStart,prVDecVp8FrmHdr->u4FifoEnd);
  }

  u4HalValue=0;
  VDEC_REG_SET_VALUE(u4HalValue, ((prVDecVp8FrmHdr->u4Width+15)>>4)-1, RW_PIC_WIDTH_IN_MBS_S, RW_PIC_WIDTH_IN_MBS_E);
  VDEC_REG_SET_VALUE(u4HalValue, ((prVDecVp8FrmHdr->u4Height+15)>>4)-1, RW_PIC_HEIGHT_IN_MBS_S, RW_PIC_HEIGHT_IN_MBS_E);
  vVDecWriteVLDTOP(u4InstID, RW_VLD_TOP_PIC_MB_SIZE_M1, u4HalValue);
  
  u4HalValue=0;
  VDEC_REG_SET_VALUE(u4HalValue, (prVDecVp8FrmHdr->u4FirstPartLen+u4ReadOffset), RW_VP8_FSTPB_S, RW_VP8_FSTPB_E);
  VDEC_REG_SET_VALUE(u4HalValue, prVDecVp8FrmHdr->uVersion, RW_VP8_VERSION_S, RW_VP8_VERSION_E);
  VDEC_REG_SET_FLAG(u4HalValue, prVDecVp8FrmHdr->uFrameType, RW_VP8_PICTYPE_FLAG);
  vVDecWriteVP8VLD(u4InstID, RW_VP8_HEADER2, u4HalValue,0);
  
  if(prVDecVp8FrmHdr->uFrameType==VP8_I_FRM)
  {
    _VDEC_Vp8FlushCtxProbs(u4InstID, NULL,VP8_I_FRM);
  }
  
  u4VDEC_HAL_VP8_InitBoolCoder(VP8_VLD1, u4InstID);
  
  VDEC_RPOS_INC(prVDecVp8FrmHdr->u4VldStartPos,u4ReadOffset, prVDecVp8FrmHdr->u4FifoStart,prVDecVp8FrmHdr->u4FifoEnd);
//  VDEC_RPOS_INC(prVDecVp8FrmHdrPhysical.u4VldStartPos,u4ReadOffset, prVDecVp8FrmHdrPhysical.u4FifoStart,prVDecVp8FrmHdrPhysical.u4FifoEnd);
 // prVDecVp8FrmHdr->u4VldStartPos = VIRTUAL(prVDecVp8FrmHdrPhysical.u4VldStartPos);
 // prVDecVp8FrmHdr->u4FifoStart = VIRTUAL(prVDecVp8FrmHdrPhysical.u4FifoStart);
//  prVDecVp8FrmHdr->u4FifoEnd = VIRTUAL(prVDecVp8FrmHdrPhysical.u4FifoEnd);

  
  if(prVDecVp8FrmHdr->uFrameType==VP8_I_FRM)
  {
    VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_COLOR, u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
    VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_CLAMP_TYPE, u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
    memcpy(prVp8DecParam->rCurFc.MVC,vp8_default_mv_context,sizeof(vp8_default_mv_context));
    memcpy(prVDecVp8FrmHdr->rVp8DecParam.rCurFc.CoefProbs,default_coef_probs,sizeof(default_coef_probs));
    prVp8DecParam->rCurFc.YModeProb[0]=112;// {112,86,140,37};
    prVp8DecParam->rCurFc.YModeProb[1]=86;
    prVp8DecParam->rCurFc.YModeProb[2]=140;
    prVp8DecParam->rCurFc.YModeProb[3]=37;
    prVp8DecParam->rCurFc.UVModeProb[0]=162;// {162,101,204};
    prVp8DecParam->rCurFc.UVModeProb[1]=101;
    prVp8DecParam->rCurFc.UVModeProb[2]=204;
    memset(prVp8DecParam->SegmentFeatureData,0,sizeof(prVp8DecParam->SegmentFeatureData));
    VDEC_CLRFLG(u4FlagParam,VP8PARAM_SEGMENT_ABSDATA);// use delta data
    memset(prVp8DecParam->RefLFDeltas,0,sizeof(prVp8DecParam->RefLFDeltas));
    memset(prVp8DecParam->ModeLFDeltas,0,sizeof(prVp8DecParam->ModeLFDeltas));
    VDEC_SETFLG(u4FlagParam,VP8PARAM_REFRESH_GOLDEN);
    VDEC_SETFLG(u4FlagParam,VP8PARAM_REFRESH_ALTRF);
    prVp8DecParam->uCopyBuf2Gf=0;
    prVp8DecParam->uCopyBuf2Arf=0;
    if(VDEC_FLGSET(u4FlagParam,VP8PARAM_COLOR))
    {
      printk("VP8 Header Color Space unsupport\n");
      return FALSE;
    }
  }

  VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_SEGMENT_ENABLE, u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));

  if(VDEC_FLGSET(u4FlagParam,VP8PARAM_SEGMENT_ENABLE))
  {
     VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_SEGMENT_UPDATE_MAP, u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
     VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_SEGMENT_UPDATE_DATA, u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
     
     if(VDEC_FLGSET(u4FlagParam,VP8PARAM_SEGMENT_UPDATE_DATA))
     {
       UINT32 u4LvlType,u4SegIndex;
       VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_SEGMENT_ABSDATA, u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
       for(u4LvlType=0;u4LvlType<VP8_MBLVL_MAX;u4LvlType++)
       {
         for(u4SegIndex=0;u4SegIndex<MAX_MB_SEGMENTS;u4SegIndex++)
         {
            prVp8DecParam->SegmentFeatureData[u4LvlType][u4SegIndex]=0;
            if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
            {
               prVp8DecParam->SegmentFeatureData[u4LvlType][u4SegIndex]=u4VDEC_VP8_VLDReadLiteral(u4InstID, _Vp8MbFeatureDataBits[u4LvlType]);
               if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
               {
                  prVp8DecParam->SegmentFeatureData[u4LvlType][u4SegIndex]=-prVp8DecParam->SegmentFeatureData[u4LvlType][u4SegIndex];
               }
            }
         }
       }
     }
     else
     {
        VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_SEGMENT_ABSDATA,VDEC_FLGSET(prVp8DecParam->u4LastParam,VP8PARAM_SEGMENT_ABSDATA));
     }

     if(VDEC_FLGSET(u4FlagParam,VP8PARAM_SEGMENT_UPDATE_MAP))
     {
       UINT32 u4ProbIndex=0;
       for(u4ProbIndex = 0; u4ProbIndex<MB_FEATURE_TREE_PROBS;u4ProbIndex++)
       {
         prVp8DecParam->SegmentTreeProbs[u4ProbIndex]=255;
         if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
         {
            prVp8DecParam->SegmentTreeProbs[u4ProbIndex]=(UINT8)u4VDEC_VP8_VLDReadLiteral(u4InstID, 8);
         }
       }
     }
     
     u4HalValue=(prVp8DecParam->SegmentTreeProbs[2]<<16)+(prVp8DecParam->SegmentTreeProbs[1]<<8)
                +prVp8DecParam->SegmentTreeProbs[0];
     //ctx_stram address: SET_ID_PROB_OFFSET
     vVDecWriteVP8VLD(u4InstID, RW_VP8_CSASET,0,0);
     vVDecWriteVP8VLD(u4InstID, RW_VP8_CSDSET,u4HalValue,0);
  }
  
  prVp8DecParam->eLoopFilterType=(VDEC_VP8_FILTER_TYPE_T)u4VDEC_VP8_VLDReadLiteral(u4InstID, 1);
  prVp8DecParam->iLoopFilterLvl=u4VDEC_VP8_VLDReadLiteral(u4InstID, 6);
  prVp8DecParam->iSharpLvl=u4VDEC_VP8_VLDReadLiteral(u4InstID, 3);

  VDEC_CLRFLG(u4FlagParam,VP8PARAM_MODEREF_IFDELTA_UPDATE);
  VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_MODEREF_LFDELTA_ENABLE,u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));

  if(VDEC_FLGSET(u4FlagParam,VP8PARAM_MODEREF_LFDELTA_ENABLE))
  {
     VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_MODEREF_IFDELTA_UPDATE,u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
     if(VDEC_FLGSET(u4FlagParam,VP8PARAM_MODEREF_IFDELTA_UPDATE))
     {
        UINT32 u4DeltaIndex=0;
        for(u4DeltaIndex=0;u4DeltaIndex<MAX_REF_LF_DELTAS;u4DeltaIndex++)
        {
           if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
           {
             prVp8DecParam->RefLFDeltas[u4DeltaIndex]=(INT8)u4VDEC_VP8_VLDReadLiteral(u4InstID, 6);
             if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
             {
                prVp8DecParam->RefLFDeltas[u4DeltaIndex]=prVp8DecParam->RefLFDeltas[u4DeltaIndex]*(-1);
             }
           }
        }

        for(u4DeltaIndex=0;u4DeltaIndex<MAX_MODE_LF_DELTAS;u4DeltaIndex++)
        {
            if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
            {
              prVp8DecParam->ModeLFDeltas[u4DeltaIndex]=(INT8)u4VDEC_VP8_VLDReadLiteral(u4InstID, 6);
              if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
              {
                 prVp8DecParam->ModeLFDeltas[u4DeltaIndex]=prVp8DecParam->ModeLFDeltas[u4DeltaIndex]*(-1);
              }
            }
        }
     }
  }

  VDEC_RPOS_INC(prVDecVp8FrmHdr->u4VldStartPos,prVDecVp8FrmHdr->u4FirstPartLen,prVDecVp8FrmHdr->u4FifoStart,prVDecVp8FrmHdr->u4FifoEnd);
//  VDEC_RPOS_INC(prVDecVp8FrmHdrPhysical.u4VldStartPos,prVDecVp8FrmHdr->u4FirstPartLen,prVDecVp8FrmHdrPhysical.u4FifoStart,prVDecVp8FrmHdrPhysical.u4FifoEnd);
//  prVDecVp8FrmHdr->u4VldStartPos = VIRTUAL(prVDecVp8FrmHdrPhysical.u4VldStartPos);
//  prVDecVp8FrmHdr->u4FifoStart = VIRTUAL(prVDecVp8FrmHdrPhysical.u4FifoStart);
//  prVDecVp8FrmHdr->u4FifoEnd = VIRTUAL(prVDecVp8FrmHdrPhysical.u4FifoEnd);
  while(!i4VDEC_HAL_VP8_InitBarrelShifter(VP8_VLD2, u4InstID, prVDecVp8FrmHdr))
  {
      printk("VP8 Init BarrelShifter fail\n");
      vVDEC_VP8_VldReset(VP8_VLD2);
  }

  // address or offset ?
  prVp8DecParam->uDataPartitionToken=u4VDEC_VP8_VLDReadLiteral(u4InstID, 2);
  prVp8DecParam->uDataPartitionNum=1<<prVp8DecParam->uDataPartitionToken;
  u4ReadOffset=3*(prVp8DecParam->uDataPartitionNum-1);
  VDEC_RPOS_INC(prVDecVp8FrmHdr->u4VldStartPos,u4ReadOffset,prVDecVp8FrmHdr->u4FifoStart,prVDecVp8FrmHdr->u4FifoEnd);
//  VDEC_RPOS_INC(prVDecVp8FrmHdrPhysical.u4VldStartPos,u4ReadOffset,prVDecVp8FrmHdrPhysical.u4FifoStart,prVDecVp8FrmHdrPhysical.u4FifoEnd);
//  prVDecVp8FrmHdr->u4VldStartPos = VIRTUAL(prVDecVp8FrmHdrPhysical.u4VldStartPos);
//  prVDecVp8FrmHdr->u4FifoStart = VIRTUAL(prVDecVp8FrmHdrPhysical.u4FifoStart);
 // prVDecVp8FrmHdr->u4FifoEnd = VIRTUAL(prVDecVp8FrmHdrPhysical.u4FifoEnd);

//  vVDecWriteVP8VLD(u4InstID, RW_VP8_COEFR0,prVDecVp8FrmHdrPhysical.u4VldStartPos,0);
  vVDecWriteVP8VLD(u4InstID, RW_VP8_COEFR0,PHYSICAL(prVDecVp8FrmHdr->u4VldStartPos),0);
  
  prVDecVp8FrmHdr->u4FrameSize-=u4ReadOffset;
  ASSERT(prVp8DecParam->uDataPartitionNum<=MAX_VP8_DATAPARTITION);
  {
    UINT32 u4PartionOffset=0,u4Index=0;
    for(u4Index=0;u4Index<prVp8DecParam->uDataPartitionNum;u4Index++)
    {
      if(u4Index<prVp8DecParam->uDataPartitionNum-1)
      {
          u4HalValue=u4VDecVP8VLDGetBits(VP8_VLD2, u4InstID, 24);
          VDEC_INTREVERSE(u4HalValue,3);
          vVDecWriteVP8VLD(u4InstID, RW_VP8_COEFPB0+u4Index*4,u4HalValue,0); // partion size
          u4PartionOffset=u4VDecReadVP8VLD(u4InstID, RW_VP8_COEFR0+u4Index*4);
          VDEC_RPOS_INC(u4PartionOffset,u4HalValue,prVDecVp8FrmHdr->u4FifoStart, prVDecVp8FrmHdr->u4FifoEnd);
//          VDEC_RPOS_INC(u4PartionOffset,u4HalValue,prVDecVp8FrmHdrPhysical.u4FifoStart, prVDecVp8FrmHdrPhysical.u4FifoEnd);
          vVDecWriteVP8VLD(u4InstID, RW_VP8_COEFR1+u4Index*4,u4PartionOffset,0); //offset
          ASSERT(prVDecVp8FrmHdr->u4FrameSize>=u4HalValue);
          prVDecVp8FrmHdr->u4FrameSize-=u4HalValue;
      }
      else
      {
          vVDecWriteVP8VLD(u4InstID, RW_VP8_COEFPB0+u4Index*4,prVDecVp8FrmHdr->u4FrameSize,0);
      }
    }
  }
 // prVDecVp8FrmHdr->u4FifoStart = VIRTUAL(prVDecVp8FrmHdrPhysical.u4FifoStart);
//  prVDecVp8FrmHdr->u4FifoEnd = VIRTUAL(prVDecVp8FrmHdrPhysical.u4FifoEnd);

  u4HalValue=VDEC_GET_FLAGVAL(1,RW_VP8_BCRT2_FLAG);
  vVDecWriteVP8VLD(u4InstID, RW_VP8_BCRT,u4HalValue,0);
  vVDecWriteVP8VLD(u4InstID, RW_VP8_BCRT,0,0);
  u4VDEC_HAL_VP8_InitBoolCoder(VP8_VLD2, u4InstID);

  {
     UINT32 u4Index=0;
     BOOL fgQupdate=FALSE;
     BOOL fgQChange[VP8_QTYPE_MAX]={FALSE,FALSE,FALSE,FALSE,FALSE,FALSE};
     INT16 i2QValue=0;
     prVp8DecParam->QIndexInfo[VP8_QTYPE_Y1AC]=(INT16)u4VDEC_VP8_VLDReadLiteral(u4InstID, 7);
     for(u4Index=VP8_QTYPE_Y1DC;u4Index<VP8_QTYPE_MAX;u4Index++)
     {
        i2QValue=prVp8DecParam->QIndexInfo[u4Index];
        if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
        {
           i2QValue=(INT16)u4VDEC_VP8_VLDReadLiteral(u4InstID, 4);
           if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
           {
             i2QValue=-i2QValue; 
           }
        }
        
        if(i2QValue!=prVp8DecParam->QIndexInfo[u4Index])
        {
           fgQupdate=TRUE;
           fgQChange[u4Index]=TRUE;
           prVp8DecParam->QIndexInfo[u4Index]=i2QValue;
        }
     }
     
     prVp8DecParam->u4FlagParam = u4FlagParam;
     VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_QINDEX_UPDATE,fgQupdate);
     if(fgQupdate)
     {
        _VDEC_Vp8InitDeQTable(prVp8DecParam,fgQChange);
     }
     _VDEC_Vp8MbInitDequantizer(u4InstID, prVp8DecParam);
     u4FlagParam=prVp8DecParam->u4FlagParam;
  }

  if(prVDecVp8FrmHdr->uFrameType!=VP8_I_FRM)
  {
      VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_REFRESH_GOLDEN,u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
      VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_REFRESH_ALTRF,u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
      prVp8DecParam->uCopyBuf2Gf=0;
      
      if(!VDEC_FLGSET(u4FlagParam,VP8PARAM_REFRESH_GOLDEN))
      {
         prVp8DecParam->uCopyBuf2Gf=u4VDEC_VP8_VLDReadLiteral(u4InstID, 2);
      }
      prVp8DecParam->uCopyBuf2Arf=0;
      if(!VDEC_FLGSET(u4FlagParam,VP8PARAM_REFRESH_ALTRF))
      {
         prVp8DecParam->uCopyBuf2Arf=u4VDEC_VP8_VLDReadLiteral(u4InstID, 2);
      }
   
      prVp8DecParam->RefFrameSignBias[VP8_MVREF_GOLDEN_FRAME]=u4VDEC_VP8_VLDReadLiteral(u4InstID, 1);
      prVp8DecParam->RefFrameSignBias[VP8_MVREF_ALTREF_FRAME]=u4VDEC_VP8_VLDReadLiteral(u4InstID, 1);
      u4HalValue=(prVp8DecParam->RefFrameSignBias[VP8_MVREF_ALTREF_FRAME]<<1)
                 +prVp8DecParam->RefFrameSignBias[VP8_MVREF_GOLDEN_FRAME];
      vVDecWriteVP8MV(u4InstID, RW_VP8_MV_RFSB, u4HalValue);
  }

  VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_REFRESH_PROBS,u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
  
  if(!VDEC_FLGSET(u4FlagParam,VP8PARAM_REFRESH_PROBS))
  {
     memcpy(&prVp8DecParam->rLastFC,&prVp8DecParam->rCurFc,sizeof(VDEC_VP8FRAME_CONTEXT_T));
  }
  
  if(prVDecVp8FrmHdr->uFrameType==VP8_I_FRM)
  {
     VDEC_SETFLG(u4FlagParam,VP8PARAM_REFRESH_LASTFRAME);
  }
  else
  {
     VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_REFRESH_LASTFRAME,u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));
  }

  vDEC_Vp8FlushCoefProbs(u4InstID, &prVp8DecParam->rCurFc);
  
#ifndef VP8_HEADERPARSE_HWACCELATOR
  {
    UINT32 u4BlockType=0,u4Band=0,u4Context=0,u4Tolen=0;
    UINT8 *puType=NULL,*puBand=NULL,*puContext=NULL,*puWriteProb;
    for(u4BlockType=0;u4BlockType<VP8_BLOCK_TYPES;u4BlockType++)
    {
       puType=(UINT8 *)VP8_PROB_TYPE(_uVp8CoefUpdateProbs,u4BlockType);
       for(u4Band=0;u4Band<VP8_COEF_BANDS;u4Band++)
       {
          puBand=VP8_PROB_BAND(puType,u4Band);
          for(u4Context=0;u4Context<VP8_PREV_COEF_CONTEXTS;u4Context++)
          {
             puContext=VP8_PROB_CTXT(puBand,u4Context);
             puWriteProb=prVp8DecParam->rCurFc.CoefProbs[u4BlockType][u4Band][u4Context];
             for(u4Tolen=0;u4Tolen<VP8_COEF_TOKENS-1;u4Tolen++)
             {
                if(u4VDec_VP8_VLDReadBit(u4InstID, puContext[u4Tolen]))
                {
                   puWriteProb[u4Tolen]=u4VDEC_VP8_VLDReadLiteral(u4InstID, 8);
                }
             }
          }
       }
    }
  }
  vDEC_Vp8FlushCoefProbs(u4InstID, &prVp8DecParam->rCurFc);
#else
  vVDec_VP8_HwAccCoefProbUpdate(u4InstID);
#endif
  
  VDEC_SETFLG_COND(u4FlagParam,VP8PARAM_NOCOEF_SKIP,u4VDEC_VP8_VLDReadLiteral(u4InstID, 1));

  u4HalValue=0;
  VDEC_REG_SET_FLAG(u4HalValue,VDEC_FLGSET(u4FlagParam,VP8PARAM_CLAMP_TYPE),RW_VP8_CLTYPE_FLAG);
  VDEC_REG_SET_FLAG(u4HalValue,VDEC_FLGSET(u4FlagParam,VP8PARAM_SEGMENT_ENABLE),RW_VP8_SEGEN_FLAG);
  VDEC_REG_SET_FLAG(u4HalValue,VDEC_FLGSET(u4FlagParam,VP8PARAM_SEGMENT_UPDATE_MAP),RW_VP8_UMBSM_FLAG);
  VDEC_REG_SET_FLAG(u4HalValue,VDEC_FLGSET(u4FlagParam,VP8PARAM_NOCOEF_SKIP),RW_VP8_MBSKIP_FLAG);
  VDEC_REG_SET_VALUE(u4HalValue,prVp8DecParam->uDataPartitionToken,RW_VP8_MTP_S,RW_VP8_MTP_E);
  vVDecWriteVP8VLD(u4InstID, RW_VP8_HEADER3,u4HalValue,0);
  
  if(prVDecVp8FrmHdr->uFrameType==VP8_I_FRM)
  {
     if(VDEC_FLGSET(u4FlagParam,VP8PARAM_NOCOEF_SKIP))
     {
       prVp8DecParam->uSkipFalseProb=u4VDEC_VP8_VLDReadLiteral(u4InstID, 8);
       vVDecWriteVP8VLD(u4InstID, RW_VP8_PICPROB,(UINT32)prVp8DecParam->uSkipFalseProb, 0);

     }
  }
  else
  {
      prVp8DecParam->uSkipFalseProb=0;
      if(VDEC_FLGSET(u4FlagParam,VP8PARAM_NOCOEF_SKIP))
      {
        prVp8DecParam->uSkipFalseProb=u4VDEC_VP8_VLDReadLiteral(u4InstID, 8);
      }
      
      prVp8DecParam->uIntraProb=u4VDEC_VP8_VLDReadLiteral(u4InstID, 8);
      prVp8DecParam->uLastProb=u4VDEC_VP8_VLDReadLiteral(u4InstID, 8);
      prVp8DecParam->uGoldenProb=u4VDEC_VP8_VLDReadLiteral(u4InstID, 8);
      u4HalValue=(prVp8DecParam->uGoldenProb<<24)+(prVp8DecParam->uLastProb<<16)
                 +(prVp8DecParam->uIntraProb<<8)+prVp8DecParam->uSkipFalseProb;

      vVDecWriteVP8VLD(u4InstID, RW_VP8_PICPROB,u4HalValue,0);
      
      if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
      {
         INT32 i4YMode=0;
         do
         {
           prVp8DecParam->rCurFc.YModeProb[i4YMode]=(UINT8)u4VDEC_VP8_VLDReadLiteral(u4InstID, 8);
         }while(++i4YMode<(VP8_YMODES-1));
      }
      
      if(u4VDEC_VP8_VLDReadLiteral(u4InstID, 1))
      {
         INT32 i4UVMode=0;
         do
         {
            prVp8DecParam->rCurFc.UVModeProb[i4UVMode]=(UINT8)u4VDEC_VP8_VLDReadLiteral(u4InstID, 8);
         }while(++i4UVMode<(VP8_UVMODES-1));
      }
#ifndef VP8_HEADERPARSE_HWACCELATOR
        {
          UINT32 u4Index=0;
          do
          {
            const UINT8 *pMvUpdateProb=vp8_mv_update_probs[u4Index].Prob;
            UINT8 *pCurFrameProb=prVp8DecParam->rCurFc.MVC[u4Index].Prob;
            UINT8 *const pStop=pCurFrameProb+VP8_MVDEF_MVPCOUNT;
            do
            {
               if(u4VDec_VP8_VLDReadBit(u4InstID, *pMvUpdateProb++))
               {
                  const UINT8 x=(UINT8)u4VDEC_VP8_VLDReadLiteral(u4InstID, 7);
                  *pCurFrameProb = x ? (x<<1) : 1;
               }
            }while(++pCurFrameProb<pStop);
          }while(++u4Index<2);
        }
        _VDEC_Vp8FlushCtxProbs(u4InstID, &prVp8DecParam->rCurFc,VP8_P_FRM);
#else   
        _VDEC_Vp8FlushCtxProbs(u4InstID, &prVp8DecParam->rCurFc,VP8_P_FRM);
        vVDEC_VP8_HwAccMVProbUpdate(u4InstID);
#endif
  }
  
  prVp8DecParam->u4FlagParam=u4FlagParam;
  return TRUE;
}
void vVerVP8SetBufStatus(UINT32 u4InstID, VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr)
{
//   VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdrAddr = &_rVDecVp8FrmHdrAddr[u4InstID];   

//*_pucWorkYBuf[u4InstID] = prVDecVp8FrmHdr->u4CurYAddr;
//*_pucDumpLstYBuf[u4InstID] = prVDecVp8FrmHdr->u4LstYAddr;
//*_pucDumpGldYBuf[u4InstID] = prVDecVp8FrmHdr->u4GldYAddr;
//*_pucDumpArfYBuf[u4InstID] = prVDecVp8FrmHdr->u4AlfYAddr;
/*    memcpy(_pucWorkYBuf[u4InstID],&prVDecVp8FrmHdr->u4CurYAddr,sizeof(UINT32));
    memcpy(_pucDumpLstYBuf[u4InstID],&prVDecVp8FrmHdr->u4LstYAddr,sizeof(UINT32));
    memcpy(_pucDumpGldYBuf[u4InstID],&prVDecVp8FrmHdr->u4GldYAddr,sizeof(UINT32));
    memcpy(_pucDumpArfYBuf[u4InstID],&prVDecVp8FrmHdr->u4AlfYAddr,sizeof(UINT32));
*/
#if VDEC_VP8_WRAPPER_OFF
       	prVDecVp8FrmHdr->u4VLDWrapper= (UINT32)_pucVLDWrapper[u4InstID];
       	prVDecVp8FrmHdr->u4SegIdWrapper= (UINT32)_pucSegIdWrapper[u4InstID];
       	prVDecVp8FrmHdr->u4PPWrapperY= (UINT32)_pucPPWrapperY[u4InstID];
       	prVDecVp8FrmHdr->u4PPWrapperC= (UINT32)_pucPPWrapperC[u4InstID];
#endif
       	prVDecVp8FrmHdr->u4VLDWrapperWrok= (UINT32)_pucVLDWrapperWrok[u4InstID];
       	prVDecVp8FrmHdr->u4PPWrapperWrok= (UINT32)_pucPPWrapperWork[u4InstID];

   switch(_u4VP8AltBufIdx[u4InstID])
   {
       case VP8_FRAME_CURRENT:
       	{
       	prVDecVp8FrmHdr->u4AlfYAddr = (UINT32)_pucWorkYBuf[u4InstID];
       	switch(_u4VP8GoldfBufIdx[u4InstID])
       	{
       	   case VP8_FRAME_CURRENT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                      		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                    		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_LAST:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_GOLD:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_ALT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }
       	   case VP8_FRAME_NO_UPD:
       	   	{
 //                    prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }       	   
       	   }
       	break;
       	}
       case VP8_FRAME_LAST:
       	{
       	prVDecVp8FrmHdr->u4AlfYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
       	switch(_u4VP8GoldfBufIdx[u4InstID])
       	{
       	   case VP8_FRAME_CURRENT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_LAST:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_GOLD:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_CURRENT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucWorkYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucWorkCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_ALT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_CURRENT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucWorkYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucWorkCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }
       	   case VP8_FRAME_NO_UPD:
       	   	{
 //                    prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }       	   
       	   }
       	break;
       	}
       case VP8_FRAME_GOLD:
       	{
       	prVDecVp8FrmHdr->u4AlfYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
       	switch(_u4VP8GoldfBufIdx[u4InstID])
       	{
       	   case VP8_FRAME_CURRENT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_LAST:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_CURRENT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucWorkYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucWorkCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_GOLD:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_ALT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_CURRENT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucWorkYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucWorkCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }
       	   case VP8_FRAME_NO_UPD:
       	   	{
 //                    prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_ALT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpArfCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }
       	   }
       	break;
       	}
       case VP8_FRAME_ALT:
       	{
       	prVDecVp8FrmHdr->u4AlfYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
       	switch(_u4VP8GoldfBufIdx[u4InstID])
       	{
       	   case VP8_FRAME_CURRENT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_LAST:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_CURRENT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucWorkYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucWorkCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_GOLD:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_CURRENT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucWorkYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucWorkCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_ALT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }
       	   case VP8_FRAME_NO_UPD:
       	   	{
 //                    prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }
       	   }
       	break;
       	}
       case VP8_FRAME_NO_UPD:
       	{
//       	prVDecVp8FrmHdr->u4AlfYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
       	switch(_u4VP8GoldfBufIdx[u4InstID])
       	{
       	   case VP8_FRAME_CURRENT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_LAST:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_CURRENT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucWorkYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucWorkCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_GOLD:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_CURRENT;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucWorkYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucWorkCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
       	   	}
       	   case VP8_FRAME_ALT:
       	   	{
                     prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }
       	   case VP8_FRAME_NO_UPD:
       	   	{
 //                    prVDecVp8FrmHdr->u4GldYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     switch(_u4VP8LastBufIdx[u4InstID])
                     	{
                     	case VP8_FRAME_CURRENT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucWorkYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_LAST:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpLstYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_GOLD;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpGldCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_GOLD:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpGldYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_ALT:
                     		prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	case VP8_FRAME_NO_UPD:
                     	//	prVDecVp8FrmHdr->u4LstYAddr = (UINT32)_pucDumpArfYBuf[u4InstID];
                     		_u4VP8CurrBufIdx[u4InstID] = VP8_FRAME_LAST;
                     		prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
                     		//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucDumpLstCBuf[u4InstID];
                     		break;
                     	}
                     break;
                     }
       	   }
       	break;
       	}        
       default:
       	prVDecVp8FrmHdr->u4CurYAddr= (UINT32)_pucWorkYBuf[u4InstID];
       	//prVDecVp8FrmHdr->u4CurCAddr= (UINT32)_pucWorkCBuf[u4InstID];
       	prVDecVp8FrmHdr->u4LstYAddr= (UINT32)_pucDumpLstYBuf[u4InstID];
       	prVDecVp8FrmHdr->u4GldYAddr= (UINT32)_pucDumpGldYBuf[u4InstID];
       	prVDecVp8FrmHdr->u4AlfYAddr= (UINT32)_pucDumpArfYBuf[u4InstID];
       	break;
   }   

}
void vVerifyVDecSetVP8Info(UINT32 u4InstID)
{    

  //  UINT32 u4BsId = 0;  
//    VDEC_INFO_DEC_PRM_T *prDecPrm = &_tVerMpvDecPrm[u4InstID];
    VDEC_INFO_VP8_DEC_PRM_T *prVDecVP8DecPrm =  &(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecVP8DecPrm);
    VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];    


    vVerVP8SetBufStatus(u4InstID, prVDecVp8FrmHdr);
    memcpy(&_rVDecVp8FrmHdrAddr[u4InstID],prVDecVp8FrmHdr,sizeof(VDEC_INFO_VP8_FRM_HDR_T));
    prVDecVP8DecPrm->prFrmHdr = &_rVDecVp8FrmHdrAddr[u4InstID];
    
    prVDecVP8DecPrm->prFrmHdr->u4CurCAddr = prVDecVP8DecPrm->prFrmHdr->u4CurYAddr +PIC_Y_SZ;
    
    i4VDEC_HAL_VP8_DecStart(u4InstID, prVDecVP8DecPrm->prFrmHdr);
    memcpy(prVDecVp8FrmHdr,prVDecVP8DecPrm->prFrmHdr,sizeof(VDEC_INFO_VP8_FRM_HDR_T));
    
}


UINT32 u4VP8InverseAddrSwap(UINT32 u4AddrSwapMode, UINT32 u4SwappedAddr, BOOL fgIsYComponent)
{
  unsigned int u4NonSwappedAddr, u4TempAddr;
  switch(u4AddrSwapMode)
  {
  case 0x1: //MT8520_SWAP_MODE_1
    u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFC0) | ((u4SwappedAddr&0x20)>>5) | ((u4SwappedAddr&0x10)>>2) | ((u4SwappedAddr&0x8)>>2) | ((u4SwappedAddr&0x7)<<3));
    break;
  case 0x2: //MT8520_SWAP_MODE_2
    u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFE0) | ((u4SwappedAddr&0x10)>>4) | ((u4SwappedAddr&0xF)<<1));
    break;
  case 0x4: // MT5351_SWAP_MODE_0
    if(fgIsYComponent)
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFF80) | ((u4SwappedAddr&0x40)>>4) | ((u4SwappedAddr&0x3C)<<1) | (u4SwappedAddr&0x3));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFF80) | ((u4TempAddr&0x7C)>>2) | ((u4TempAddr&0x3)<<5));
    }
    else
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFFC0) | ((u4SwappedAddr&0x20)>>3) | ((u4SwappedAddr&0x1C)<<1) | (u4SwappedAddr&0x3));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFFC0) | ((u4TempAddr&0x3C)>>2) | ((u4TempAddr&0x3)<<4));
    }
    break;
  case 0x5: // MT5351_SWAP_MODE_1
    if(fgIsYComponent)
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFF00) | ((~u4SwappedAddr)&0x80) | (u4SwappedAddr&0x7F));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFF80) | ((u4TempAddr&0x7C)>>2) | ((u4TempAddr&0x3)<<5));
    }
    else
    {
      u4TempAddr = ((u4SwappedAddr&0xFFFFFF80) | ((~u4SwappedAddr)&0x40) | (u4SwappedAddr&0x3F));
      u4NonSwappedAddr = ((u4TempAddr&0xFFFFFFC0) | ((u4TempAddr&0x3C)>>2) | ((u4TempAddr&0x3)<<4));
    }
    break;
  case 0x6: // MT5351_SWAP_MODE_2
    if(fgIsYComponent)
    {
      u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFF80) | ((u4SwappedAddr&0x7C)>>2) | ((u4SwappedAddr&0x3)<<5));
    }
    else
    {
      u4NonSwappedAddr = ((u4SwappedAddr&0xFFFFFFC0) | ((u4SwappedAddr&0x3C)>>2) | ((u4SwappedAddr&0x3)<<4));
    }
    break;
  default:
    u4NonSwappedAddr = u4SwappedAddr;
    break;
  }
  return u4NonSwappedAddr;
}

void vVP8_InvAddressSwap(UINT32 u4InstID, 
                                                         BYTE* pbSrcBufY, BYTE* pbSrcBufC, 
                                                         BYTE* pbOutBufY, BYTE* pbOutBufC,
                                                         UINT32 u4AlignWidth, UINT32 u4AlignHeight, UINT32 u4AlignSize,
                                                         UINT32 u4HwSwapMode)
{
  UINT32 i;
  UINT32 u4DataLength;
  UINT32 u4AlignW_Luma;
  UINT32 u4AlignH_Luma;
  UINT32 u4AlignW_Chroma;
  UINT32 u4AlignH_Chroma;
  //UINT32 u4AlignSize = 0x32000;
  UINT32 u4NonSwappedAddr;  
  UINT32 u4SwappedAddr;
  BYTE * pbTempBufAddr;
  UINT32 u4AddrressSwapSize = 16;
  UINT32 u4AddrSwapMode;

   UINT8 auAddrSwapMapTable[8] =
 {
    4, 5, 6, 7, 0, 1, 2, 3
  };

  #ifdef RM_DDR3MODE_ENABLE
  u4AddrressSwapSize = 16;
  #else //RM_DDR3MODE_ENABLE
  u4AddrressSwapSize = 16;
  #endif //RM_DDR3MODE_ENABLE

  //prParsingPic = (VDEC_INFO_RM_PICINFO_T*) &_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecRMDecPrm.rRMParsPicInfo;

  u4AlignW_Luma = u4AlignWidth;//prParsingPic->u4Width;
  u4AlignH_Luma = u4AlignHeight;//prParsingPic->u4Height;
  
  //pbSrcBufY = (BYTE*) prParsingPic->u4OutBufY;
  //pbSrcBufC = (BYTE*) prParsingPic->u4OutBufC;
  //pbOutBufY = (BYTE*) _pucDumpYBuf[u4InstID];
  //pbOutBufC = (BYTE*) _pucDumpCBuf[u4InstID];

  u4AddrSwapMode = auAddrSwapMapTable[u4HwSwapMode];
  
  //Luma 
  u4DataLength = u4AlignW_Luma * u4AlignH_Luma;
  //u4DataLength = (u4DataLength + u4AlignSize-1)/u4AlignSize;
  //u4DataLength = u4DataLength * u4AlignSize;
  u4SwappedAddr = 0;
  
  for (i=0; i<u4DataLength; i+=u4AddrressSwapSize)
  {
    u4NonSwappedAddr = u4VP8InverseAddrSwap(u4AddrSwapMode, u4SwappedAddr, TRUE);
    pbTempBufAddr = (BYTE*) (pbSrcBufY+i);
    memcpy(&pbOutBufY[u4NonSwappedAddr<<4], &pbTempBufAddr[0],u4AddrressSwapSize);
    u4SwappedAddr++;
  }
  

  //Chroma
  u4AlignW_Chroma = u4AlignW_Luma;
  u4AlignH_Chroma = u4AlignH_Luma / 2;
  
  u4DataLength = u4AlignW_Chroma * u4AlignH_Chroma;
  //u4DataLength = (u4DataLength + u4AlignSize-1)/u4AlignSize;
  //u4DataLength = u4DataLength * u4AlignSize;
  u4SwappedAddr = 0;

  for (i=0; i<u4DataLength; i+=u4AddrressSwapSize)
  {
    u4NonSwappedAddr = u4VP8InverseAddrSwap(u4AddrSwapMode, u4SwappedAddr, FALSE);
    pbTempBufAddr = (BYTE*) (pbSrcBufC+i);
    memcpy(&pbOutBufC[u4NonSwappedAddr<<4], &pbTempBufAddr[0],u4AddrressSwapSize);
    u4SwappedAddr++;
  }
}


void vVP8_CheckCRCResult(UINT32 u4InstID)//, UINT32 u4DecFrameCnt, UINT32 u4CRCResBuf)
{
//read
//REG_2: y_crc_checksum[31:0]
//REG_3: y_crc_checksum[63:32]
//REG_4: y_crc_checksum[95:64]
//REG_5: y_crc_checksum[127:96]

//REG_6: c_crc_checksum[31:0]
//REG_7: c_crc_checksum[63:32]
//REG_8: c_crc_checksum[95:64]
//REG_9: c_crc_checksum[127:96]


  //UINT32* pu4CRCResultCurrAddr;
  //UINT32 u4CRCResult = 0;
  //UINT32 i=0;
  //UINT32 u4SWResult;
    UINT32 u4HWCRC_Y0, u4HWCRC_Y1, u4HWCRC_Y2, u4HWCRC_Y3;
    UINT32 u4HWCRC_C0, u4HWCRC_C1, u4HWCRC_C2, u4HWCRC_C3;

    CHAR _bFileNameCRC[20] = {"_CRC.out\0"};
    CHAR _bGoldFileName[256] = {"\\CRC\\post_\0"};
    CHAR _bPatternPath [256];
    CHAR _bPathAddStr[256] = {"\Pattern\\gp\\"};
    CHAR _bFileName [256];
    UINT32 u4Temp;
//Searh path name
    UINT32 path_byte_addr = 0;
    UINT32 filename_byte_addr = 0;
    UINT32 u4CRCValueY0, u4CRCValueY1, u4CRCValueY2, u4CRCValueY3;
    UINT32 u4CRCValueC0, u4CRCValueC1, u4CRCValueC2, u4CRCValueC3;
    UINT32 u4CRCTmp3, u4CRCTmp2, u4CRCTmp1, u4CRCTmp0;
    BOOL fgDecErr = FALSE;
    BOOL fgOpen;
    INT32 i, j;
    UCHAR strMessage [ 256];
    
    for (i=0; ; i++)
    {
          if (_bFileStr1[u4InstID][1][i] == '\0')
          	break;

          if (_bFileStr1[u4InstID][1][i] == 'b' || _bFileStr1[u4InstID][1][i] == 'B')
          {
              if( (_bFileStr1[u4InstID][1][i+1] == 'i' || _bFileStr1[u4InstID][1][i+1] == 'I')
              && (_bFileStr1[u4InstID][1][i+2] == 't' || _bFileStr1[u4InstID][1][i+2] == 'T')
              && (_bFileStr1[u4InstID][1][i+3] == 's' || _bFileStr1[u4InstID][1][i+2] == 'S')
              && (_bFileStr1[u4InstID][1][i+4] == 't' || _bFileStr1[u4InstID][1][i+4] == 'T')
              && (_bFileStr1[u4InstID][1][i+5] == 'r' || _bFileStr1[u4InstID][1][i+5] == 'R')
              && (_bFileStr1[u4InstID][1][i+6] == 'e' || _bFileStr1[u4InstID][1][i+6] == 'E')
              && (_bFileStr1[u4InstID][1][i+7] == 'a' || _bFileStr1[u4InstID][1][i+7] == 'A')
              && (_bFileStr1[u4InstID][1][i+8] == 'm' || _bFileStr1[u4InstID][1][i+8] == 'M') )
              {
                  path_byte_addr = i;
              }
          }
          else
          if (_bFileStr1[u4InstID][1][i] == '.')
          {
              if( (_bFileStr1[u4InstID][1][i+1] == 'i' || _bFileStr1[u4InstID][1][i+1] == 'I')
              && (_bFileStr1[u4InstID][1][i+2] == 'v' || _bFileStr1[u4InstID][1][i+2] == 'V')
              && (_bFileStr1[u4InstID][1][i+3] == 'f' || _bFileStr1[u4InstID][1][i+3] == 'F'))
              {
                  filename_byte_addr = i;
              }
          }
    }

    j = 0;
    for (i=path_byte_addr+10; i < filename_byte_addr; i++)
    {
        _bFileName[j] = _bFileStr1[u4InstID][1][i];
        j++;
    }
    _bFileName[j] = '\0';

    for (j=0; j < path_byte_addr; j++)
    {
        _bPatternPath[j] = _bFileStr1[u4InstID][1][j];
    }

    u4Temp = sprintf(_bPatternPath+path_byte_addr, "%s", _bPathAddStr);
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bFileName);
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bGoldFileName);    
    _bPatternPath[path_byte_addr+u4Temp] = '\0';

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
    _tFBufFileInfo[u4InstID].pucTargetAddr = _pucCRCBuf[u4InstID];
    _tFBufFileInfo[u4InstID].u4TargetSz = 32;  
    _tFBufFileInfo[u4InstID].u4FileLength = 0;  
    vConcateStr((char*)_bFileStr1[u4InstID][3], (char*)_bPatternPath, (char*)_bFileNameCRC, (UINT32)_u4FileCnt[u4InstID]);

    #if VDEC_DRAM_BUSY_TEST
    i4VDEC_HAL_Dram_Busy_Off(u4InstID);
    fgOpenFile(u4InstID, (char*)_bFileStr1[u4InstID][3],(char*)"r+b", &_tFBufFileInfo[u4InstID]);
    vDrmaBusySet(u4InstID);
    #else
    fgOpenFile(u4InstID, (char*)_bFileStr1[u4InstID][3],(char*)"r+b", &_tFBufFileInfo[u4InstID]);
    #endif

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][3] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][2] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][1] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][0] );
    u4CRCValueY0 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][7] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][6] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][5] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][4] );
    u4CRCValueY1 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][11] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][10] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][9] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][8] );
    u4CRCValueY2 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][15] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][14] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][13] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][12] );
    u4CRCValueY3 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][19] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][18] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][17] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][16] );
    u4CRCValueC0 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][23] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][22] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][21] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][20] );
    u4CRCValueC1 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][27] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][26] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][25] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][24] );
    u4CRCValueC2 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4CRCTmp3 = (_pucCRCBuf[u4InstID][31] );
    u4CRCTmp2 = (_pucCRCBuf[u4InstID][30] );
    u4CRCTmp1 = (_pucCRCBuf[u4InstID][29] );
    u4CRCTmp0 = (_pucCRCBuf[u4InstID][28] );
    u4CRCValueC3 = (u4CRCTmp3 << 24) | (u4CRCTmp2 << 16) |  (u4CRCTmp1 << 8) |  (u4CRCTmp0 << 0) ;

    u4HWCRC_Y0 = u4VDecReadCRC(u4InstID, VDEC_CRC_Y_CHKSUM0);
    u4HWCRC_Y1 = u4VDecReadCRC(u4InstID, VDEC_CRC_Y_CHKSUM1);
    u4HWCRC_Y2 = u4VDecReadCRC(u4InstID, VDEC_CRC_Y_CHKSUM2);
    u4HWCRC_Y3 = u4VDecReadCRC(u4InstID, VDEC_CRC_Y_CHKSUM3);

    u4HWCRC_C0 = u4VDecReadCRC(u4InstID, VDEC_CRC_C_CHKSUM0);
    u4HWCRC_C1 = u4VDecReadCRC(u4InstID, VDEC_CRC_C_CHKSUM1);
    u4HWCRC_C2 = u4VDecReadCRC(u4InstID, VDEC_CRC_C_CHKSUM2);
    u4HWCRC_C3 = u4VDecReadCRC(u4InstID, VDEC_CRC_C_CHKSUM3);

    printk("u4HWCRC_Y0 = 0x%x, u4CRCValueY0 = 0x%x\n", u4HWCRC_Y0, u4CRCValueY0);
    printk("u4HWCRC_Y1 = 0x%x, u4CRCValueY1 = 0x%x\n", u4HWCRC_Y1, u4CRCValueY1);
    printk("u4HWCRC_Y2 = 0x%x, u4CRCValueY2 = 0x%x\n", u4HWCRC_Y2, u4CRCValueY2);
    printk("u4HWCRC_Y3 = 0x%x, u4CRCValueY3 = 0x%x\n", u4HWCRC_Y3, u4CRCValueY3);

    printk("u4HWCRC_C0 = 0x%x, u4CRCValueC0 = 0x%x\n", u4HWCRC_C0, u4CRCValueC0);
    printk("u4HWCRC_C1 = 0x%x, u4CRCValueC1 = 0x%x\n", u4HWCRC_C1, u4CRCValueC1);
    printk("u4HWCRC_C2 = 0x%x, u4CRCValueC2 = 0x%x\n", u4HWCRC_C2, u4CRCValueC2);
    printk("u4HWCRC_C3 = 0x%x, u4CRCValueC3 = 0x%x\n", u4HWCRC_C3, u4CRCValueC3);

    //while(1);

    if( (u4HWCRC_Y0 != u4CRCValueY0) 
    	|| (u4HWCRC_Y1 != u4CRCValueY1)
    	|| (u4HWCRC_Y2 != u4CRCValueY2)
    	|| (u4HWCRC_Y3 != u4CRCValueY3)
    	|| (u4HWCRC_C0 != u4CRCValueC0)
    	|| (u4HWCRC_C1 != u4CRCValueC1)
    	|| (u4HWCRC_C2 != u4CRCValueC2)
    	|| (u4HWCRC_C3 != u4CRCValueC3)
      )
    {
       fgDecErr = TRUE;
       sprintf(strMessage," Error ==> Pic count to [%d] \n", _u4FileCnt[u4InstID]);     
       fgWrMsg2PC((void*)strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    }
    
    if((_u4FileCnt[u4InstID]%10) == 0 || _fgVP8DumpReg)
        vVDecOutputDebugString("Pic count to [%d]\n", _u4FileCnt[u4InstID]);  
        sprintf(strMessage,"[%d], \n", _u4FileCnt[u4InstID]);  
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);

     _u4FileCnt[u4InstID]++;

    _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
    _tFBufFileInfo[u4InstID].pucTargetAddr = _pucCRCBuf[u4InstID];
    _tFBufFileInfo[u4InstID].u4TargetSz = 32;  
    _tFBufFileInfo[u4InstID].u4FileLength = 0;  
    vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileNameCRC, _u4FileCnt[u4InstID]);
    
    #ifdef IDE_READ_SUPPORT
    fgOpen = fgPureOpenIdeFile( _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);
    #else
    fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);
    #endif

    if((fgOpen == FALSE) ||(fgDecErr == TRUE))
    {
        sprintf(strMessage, "%s", "\n");
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
        //fprintf(_tFileListRecInfo.fpFile, "\n");  
        // break decode
        if(fgOpen == FALSE)
        {
#if defined(CAPTURE_ESA_LOG) && defined(CAPTURE_ALL_IN_ONE) && (!WRITE_ESA_PER_FRAME)
            fgWrData2PC(_pucESATotalBuf[u4InstID],_u4ESATotalLen[u4InstID],7,_ESAFileName[u4InstID]); 
#endif            
            sprintf(strMessage," Compare Finish==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);   
            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            //fprintf(_tFileListRecInfo.fpFile, " Compare Finish==> Pic count to [%d] \n", _u4FileCnt[_u4VDecID] - 1);   
            if(_u4FileCnt[u4InstID] == 1)
            {
                if(fgOpen == FALSE)
                {
                    vVDecOutputDebugString("real NULL\n");
                }
            }
        }
        else
        {
            sprintf(strMessage," Error ==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);     
            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            //fprintf(_tFileListRecInfo.fpFile, " Error ==> Pic count to [%d] \n", _u4FileCnt[_u4VDecID] - 1);         
        }
        _u4VerBitCount[u4InstID] = 0xffffffff;
    }
    
    if(_u4FileCnt[u4InstID] >= _u4EndCompPicNum[u4InstID])
    {
#if 0        
#if defined(CAPTURE_ESA_LOG) && defined(CAPTURE_ALL_IN_ONE) && (!WRITE_ESA_PER_FRAME)
        fgWrData2PC(_pucESATotalBuf[u4InstID],_u4ESATotalLen[u4InstID],7,_ESAFileName[u4InstID]); 
#endif
#endif
        _u4VerBitCount[u4InstID] = 0xffffffff;
    }    
}

// *********************************************************************
// Function    : void vVP8WrData2PC(UINT32 u4InstID, BYTE *ptAddr, UINT32 u4Size, BOOL *fgNextFrameExist)
// Description : Write the decoded data to PC for compare
// Parameter   : None
// Return      : None
// *********************************************************************
void vVP8WrData2PC(UINT32 u4InstID, UCHAR *ptAddr, UINT32 u4Size, BOOL *fgNextFrameExist)
{
    #if ((!defined(COMP_HW_CHKSUM)) || defined(DOWN_SCALE_SUPPORT))
    UINT32 u4Cnt;
    #ifdef GOLDEN_128BIT_COMP  
    UINT32 u4XPix,u4YPix;
    #endif

    UINT32 u4Width,u4Height;
    UINT32 u4YBase,u4CBase;
    //UINT32 u4BufferWidth;
    UCHAR *pbDecBuf,*pbGoldenBuf;

    #ifndef GOLDEN_128BIT_COMP  
    UINT32 u4Pix;
    #else
    UINT32 u4Ty0, u4Tx0, u4Ty1, u4Tx1;
    UINT32 u4X, u4Y;
    UINT32 mbw, mbh, i, j;
    UINT32 u4Start;  
    UINT32 u4StartGld;  
    UINT32 u4Offset;
    #endif  

    #endif  
    VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];   
    
    BOOL fgDecErr,fgOpen;
    char strMessage[256];
    
#if (!VDEC_VP8_ERR_TEST)
    BOOL fgCompare = TRUE;
#else
    BOOL fgCompare = FALSE;
#endif

//#ifndef DOWN_SCALE_SUPPORT               
    UINT32 u4NonSwapYBase = 0;
    UINT32 u4NonSwapCBase = 0;
//#endif   
  //  CHAR _VP8GoldenList[100]= {"\\\\pc-02098\\VP8_bitstream\\"}; 
    CHAR _bFileAddStrY[20] = {".y.dat\0"};
    CHAR _bFileAddStrC[20] = {".c.dat\0"};

  #ifdef  SATA_HDD_READ_SUPPORT
    CHAR _bGoldFileName[256] = {"/post_\0"};
    CHAR _bPathAddStr[256] = {"\Pattern\\gp\\"};
  #else
    CHAR _bGoldFileName[256] = {"\\post_\0"};
    CHAR _bPathAddStr[256] = {"gp\\"};
  #endif
    CHAR _bPatternPath [256];
//    CHAR _bPathAddStr[256] = {"golden_pattern\\"};
    CHAR _bFileName [256];
    UINT32 u4Temp;
//Searh path name
    UINT32 path_byte_addr = 0;
    UINT32 filename_byte_addr = 0;

    // BSP_FlushDCacheRange((UINT32)_pucDumpYBuf[u4InstID],GOLD_Y_SZ);
     //BSP_FlushDCacheRange((UINT32)_pucDumpCBuf[u4InstID],GOLD_C_SZ);

    
    for (i=0; ; i++)
    {
          if (_bFileStr1[u4InstID][1][i] == '\0')
          	break;

          if (_bFileStr1[u4InstID][1][i] == 'v' || _bFileStr1[u4InstID][1][i] == 'V')
          {
              if( (_bFileStr1[u4InstID][1][i+1] == 'p' || _bFileStr1[u4InstID][1][i+1] == 'P')
              && (_bFileStr1[u4InstID][1][i+2] == '8' || _bFileStr1[u4InstID][1][i+2] == '8')
              && (_bFileStr1[u4InstID][1][i+3] == '\\'))
//              && (_bFileStr1[u4InstID][1][i+4] == 'b' || _bFileStr1[u4InstID][1][i+4] == 'B')
//              && (_bFileStr1[u4InstID][1][i+5] == 'i' || _bFileStr1[u4InstID][1][i+5] == 'I')
//              && (_bFileStr1[u4InstID][1][i+6] == 't' || _bFileStr1[u4InstID][1][i+6] == 'T')
//              && (_bFileStr1[u4InstID][1][i+7] == 's' || _bFileStr1[u4InstID][1][i+7] == 'S')
//              && (_bFileStr1[u4InstID][1][i+8] == 't' || _bFileStr1[u4InstID][1][i+8] == 'T') )
              {
                  path_byte_addr = i;
              }
          }
          else
          if (_bFileStr1[u4InstID][1][i] == '.')
          {
              if( (_bFileStr1[u4InstID][1][i+1] == 'i' || _bFileStr1[u4InstID][1][i+1] == 'I')
              && (_bFileStr1[u4InstID][1][i+2] == 'v' || _bFileStr1[u4InstID][1][i+2] == 'V')
              && (_bFileStr1[u4InstID][1][i+3] == 'f' || _bFileStr1[u4InstID][1][i+3] == 'F'))
              {
                  filename_byte_addr = i;
/*		   for (j=filename_byte_addr; j>0 ; j--)
		   {
		   if (_bFileStr1[u4InstID][1][j] == '\\')
		     {
		   	filename_byte_addr = j; 
		   	break;
		     }
		   }
		   */
              }
          }
    }

    j = 0;
    for (i=path_byte_addr+14; i < filename_byte_addr; i++)
    {
        _bFileName[j] = _bFileStr1[u4InstID][1][i];
        j++;
    }
    _bFileName[j] = '\0';

    for (j=0; j < path_byte_addr+4; j++)
    {
        _bPatternPath[j] = _bFileStr1[u4InstID][1][j];
    }

    u4Temp = sprintf(_bPatternPath+path_byte_addr+4, "%s", _bPathAddStr)+4;  
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bFileName);
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bGoldFileName);    
    _bPatternPath[path_byte_addr+u4Temp] = '\0';
    vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);

#ifdef VDEC_BREAK_EN
    fgNextFrameExist = TRUE;
    _u4FileCnt[u4InstID] ++;
    _tFBufFileInfo[u4InstID].fgGetFileInfo = FALSE;  
    _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
    _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
    _tFBufFileInfo[u4InstID].u4FileLength = 0;      
    vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);
    fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);
    if (fgOpen == FALSE)
    	{
    	  _u4VerBitCount[u4InstID] = 0xffffffff;
    	}
    return;
#endif

    strcpy(_tFileListRecInfo[u4InstID].bFileName, _FileList_Rec[u4InstID]);

    #ifdef REDEC  
    if(_u4FileCnt[u4InstID] == _u4ReDecPicNum[u4InstID] )
    {
        if(_u4ReDecNum[u4InstID] != 0)
        {
            _u4ReDecPicNum[u4InstID] = 0xFFFFFFFF;
            _u4ReDecCnt[u4InstID] = _u4ReDecNum[u4InstID];
            vVDecOutputDebugString("/n!!!!!!!!!!!!!! Re-Decode and Wait for debug !!!!!!!!!!!!!!!!\n");
        }
    }
    if(_u4ReDecCnt[u4InstID] > 0)
    {
        _u4ReDecCnt[u4InstID]--;
    }
    #endif

    fgDecErr = FALSE;

    #ifdef GEN_HW_CHKSUM
    #ifndef INTERGRATION_WITH_DEMUX
    vWrite2PC(u4InstID, 9, NULL);
    #endif
    #endif

    #ifndef INTERGRATION_WITH_DEMUX
    if(fgCompare)
    {
        #if (defined(COMP_HW_CHKSUM) && (!defined(DOWN_SCALE_SUPPORT)))
        //if(!fgCompVP8ChkSumGolden(u4InstID))
        //{
        //    fgDecErr = TRUE;
        //    vVDecOutputDebugString("Check sum comparison mismatch\n");
        //}
        #else // compare pixel by pixel
        // Y compare
        _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
        _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
        _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
        _tFBufFileInfo[u4InstID].u4FileLength = 0;  
        // Y decoded data Compare   
         vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);

        #ifdef EXT_COMPARE 
        _tFBufFileInfo[u4InstID].u4FileLength = (((prVDecVp8FrmHdr->u2WidthDec + 15)>>4)<<4) *(((prVDecVp8FrmHdr->u2HeightDec + 31)>>5)<<5);
        #else
        #ifdef DIRECT_DEC
        if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
        #endif    
        {
            #ifdef DOWN_SCALE_SUPPORT
            if(_u4CodecVer[u4InstID] == VDEC_VP8)
            {
                if(!(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecVP8DecPrm.fgDec2ndFld))
                {
                    memset(_pucDumpYBuf[u4InstID], 0x0, GOLD_Y_SZ);
                    memset(_pucDumpCBuf[u4InstID], 0x0, GOLD_C_SZ);
                }
            }

            u4NonSwapYBase = (UINT32)_pucDecWorkBuf[u4InstID];
            u4NonSwapCBase = (UINT32)_pucDecCWorkBuf[u4InstID];
            if (_tVerMpvDecPrm[u4InstID].ucAddrSwapMode != 4)
            {
                UINT32 u4AlignWidth, u4AlignHeight;
                UINT32 u4AlignSize = 0;

                // swap off down scaler data
                u4AlignWidth = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4DispW;
                u4AlignWidth = (((u4AlignWidth +63) >>6) <<6); //Align to 4MB width

                if((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD) ||(_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD))
                {
                    u4AlignHeight = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight*2);
                }
                else
                {
                    u4AlignHeight = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight;
                }

                u4AlignHeight =  (((u4AlignHeight +31) >>5) <<5);

                vVP8_InvAddressSwap(u4InstID, 
                                   (BYTE*)_pucVDSCLBuf[u4InstID],  (BYTE*)(_pucVDSCLBuf[u4InstID] + 0x1FE000), 
                                   (BYTE*)_pucDumpYBuf[u4InstID], (BYTE*) _pucDumpCBuf[u4InstID],
                                  u4AlignWidth,  u4AlignHeight, u4AlignSize,
                                  _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);

                u4NonSwapYBase = (UINT32)_pucVDSCLBuf[u4InstID];
                u4NonSwapCBase = (UINT32)(_pucVDSCLBuf[u4InstID] + 0x1FE000);
                memcpy((UCHAR*)u4NonSwapYBase, _pucDumpYBuf[u4InstID],(u4AlignWidth*u4AlignHeight) +u4AlignSize);
                memcpy((UCHAR*)u4NonSwapCBase, _pucDumpCBuf[u4InstID],(u4AlignWidth*u4AlignHeight/2) + u4AlignSize);                 

                // swap off mc data
                u4AlignWidth = _tVerMpvDecPrm[u4InstID].u4PicW;
                u4AlignWidth = (((u4AlignWidth +63) >>6) <<6); //Align to 4MB width                    
                u4AlignHeight = _tVerMpvDecPrm[u4InstID].u4PicH;
                u4AlignHeight =  (((u4AlignHeight +31) >>5) <<5);              

                vVP8_InvAddressSwap(u4InstID, 
                                   (BYTE*)_pucDecWorkBuf[u4InstID],  (BYTE*)_pucDecCWorkBuf[u4InstID], 
                                   (BYTE*)_pucDumpYBuf[u4InstID], (BYTE*) _pucDumpCBuf[u4InstID],
                                  u4AlignWidth,  u4AlignHeight, u4AlignSize,
                                  _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);

                u4NonSwapYBase = (UINT32)_pucPpYSa[u4InstID];
                u4NonSwapCBase = (UINT32)_pucPpCSa[u4InstID];
                memcpy((UCHAR*)u4NonSwapYBase, _pucDumpYBuf[u4InstID],(u4AlignWidth*u4AlignHeight) +u4AlignSize);
                memcpy((UCHAR*)u4NonSwapCBase, _pucDumpCBuf[u4InstID],(u4AlignWidth*u4AlignHeight/2) + u4AlignSize);                 

                Printf("MPV emu VP8 inverse swap mode %d\n", _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);
            }

            if(_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecVP8DecPrm.rVP8PpInfo.fgPpEnable)
            {
                vGenerateDownScalerGolden(u4InstID, (UINT32)_pucPpYSa[u4InstID],(UINT32)(_pucPpCSa[u4InstID]),u4Size);
            }
            else
            {
                vGenerateDownScalerGolden(u4InstID, u4NonSwapYBase, u4NonSwapCBase, u4Size);
            }
            #else
         //   u4NonSwapYBase = (UINT32)_pucWorkYBuf[u4InstID];
         #if VMMU_SUPPORT
            u4NonSwapYBase = prVDecVp8FrmHdr->u4CurYAddr+0x1000;
            u4NonSwapCBase = u4NonSwapYBase+ PIC_Y_SZ;
         #else
            u4NonSwapYBase = prVDecVp8FrmHdr->u4CurYAddr;
            u4NonSwapCBase = u4NonSwapYBase+ PIC_Y_SZ;
         #endif
            if (_tVerMpvDecPrm[u4InstID].ucAddrSwapMode != 4)
            {
                UINT32 u4AlignWidth, u4AlignHeight;
                UINT32 u4AlignSize = 0;

                u4AlignWidth = prVDecVp8FrmHdr->u4Width;
                u4AlignWidth = (((u4AlignWidth +63) >>6) <<6); //Align to 4MB width                    
                u4AlignHeight = prVDecVp8FrmHdr->u4Height;
                u4AlignHeight =  (((u4AlignHeight +31) >>5) <<5);              

                vVP8_InvAddressSwap(u4InstID, 
                                   (BYTE*)u4NonSwapYBase,  (BYTE*)u4NonSwapCBase, 
                                   (BYTE*)_pucDumpYBuf[u4InstID], (BYTE*) _pucDumpCBuf[u4InstID],
                                  u4AlignWidth,  u4AlignHeight, u4AlignSize,
                                  _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);

                u4NonSwapYBase = (UINT32)_pucAddressSwapBuf[u4InstID];
                u4NonSwapCBase = (UINT32)_pucAddressSwapBuf[u4InstID] + PIC_Y_SZ;
                memcpy((UCHAR*)u4NonSwapYBase, _pucDumpYBuf[u4InstID],(u4AlignWidth*u4AlignHeight) +u4AlignSize);
                memcpy((UCHAR*)u4NonSwapCBase, _pucDumpCBuf[u4InstID],(u4AlignWidth*u4AlignHeight/2) + u4AlignSize);                 
//		HalFlushInvalidateDCache();

 //               Printf("MPV emu VP8 inverse swap mode %d\n", _tVerMpvDecPrm[u4InstID].ucAddrSwapMode);
            }
  //      _pu1VP8VDecYGoldPhy = (UINT8 *)BSP_dma_map_single((UINT32)_pucDumpYBuf[u4InstID], GOLD_Y_SZ, FROM_DEVICE);
            #if VDEC_DRAM_BUSY_TEST
            i4VDEC_HAL_Dram_Busy_Off(u4InstID);
            fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);               
            vDrmaBusySet(u4InstID);
            #else
            fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);               
            #endif
            _u4GoldenYSize[u4InstID] = _tFBufFileInfo[u4InstID].u4RealGetBytes;
            #endif
  //      BSP_dma_unmap_single((UINT32)_pu1VP8VDecYGoldPhy, GOLD_Y_SZ, FROM_DEVICE);
        }
        #endif

       //  msleep(1000);
        u4Cnt = 0; 
        #ifdef EXT_COMPARE 
        if(_ptCurrFBufInfo[u4InstID]->ucFBufStatus == FRAME)
        {
            vWrite2PC(u4InstID, 5, _pucDecWorkBuf[u4InstID]);
        }
        #else
        #if defined(DOWN_SCALE_SUPPORT)
        u4YBase = (UINT32)_pucVDSCLBuf[u4InstID];
        //u4BufferWidth = ((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4DispW + 15) >> 4) << 4;
        u4Width = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4DispW;
        if((_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == TOP_FIELD) ||(_tVerMpvDecPrm[u4InstID].rDownScalerPrm.ucPicStruct == BOTTOM_FIELD))
        {
            u4Height = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + (_tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight*2);
        }
        else
        {
            u4Height = _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgOffV + _tVerMpvDecPrm[u4InstID].rDownScalerPrm.u4TrgHeight;
        }

        Printf("MPV emu down scaler target width = %d\n", u4Width);
        Printf("MPV emu down scaler target height = %d\n", u4Height);
        
        #else
        u4YBase = u4NonSwapYBase;//(UINT32)_pucDecWorkBuf[u4InstID];
        
        u4Width = ((prVDecVp8FrmHdr->u4Width+15)>>4)<<4;
        u4Height = ((prVDecVp8FrmHdr->u4Height+15)>>4)<<4;
        #endif  

        //fred add for 32byte align in height
       // u4Height = ( (u4Height+31)>>5 ) <<5;
        u4Height = ( (u4Height)>>5 ) <<5;
        #ifdef DIRECT_DEC
        if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
        #endif    
        {
            #ifdef GOLDEN_128BIT_COMP
            u4Tx0 = (u4Width >> 4);   // w/ 16
            u4Ty0 = (u4Height >> 5);  // h /32
            u4X = (u4Width & 0xF);    // w % 16
            u4Y = (u4Height & 0x1F);  // h%32
            u4Tx1 = (u4X==0)? u4Tx0 : (u4Tx0+1);
            u4Ty1 = (u4Y==0)? u4Ty0 : (u4Ty0+1);

            for (mbh=0; mbh < u4Ty1; mbh++)
            {
                for (mbw=0; mbw < u4Tx1; mbw++)
                {
                if (_tVerMpvDecPrm[u4InstID].ucAddrSwapMode != 4) // do not compare Align to 4MB width unused data
                  {
                  u4Offset = mbh*(((((u4Width +63) >>6) <<6)-u4Width)*32);
                  }
                else
                  {
                  u4Offset = 0;
                  }
                    u4Start = (mbh*u4Tx1 + mbw) * (16*32) + u4Offset;
                    u4StartGld = (mbh*u4Tx1 + mbw) * (16*32);
                    pbGoldenBuf = (UCHAR*) (((UINT32) (_pucDumpYBuf[u4InstID])) + u4StartGld);
                    pbDecBuf = (UCHAR*) (u4YBase + u4Start);

                    for(j=0; j < 32; j++)
                    {             
                        for(i=0; i < 16; i++)
                        {                   
                            if(1)
                            {
                                if(  (mbw == u4Tx0 && i >= u4X) || (mbh == u4Ty0 && j >= u4Y))
                                {
                                    //Do not compare
                                }
                                else
                                {
                                    if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                                    {
                                        u4Cnt ++;
                                        u4XPix = mbw * 16 + i;
                                        u4YPix = mbh * 32 + j;
								        vVDecOutputDebugString("Pic count to [%d]\n", _u4FileCnt[u4InstID]);  
                                        vVDecOutputDebugString("Y Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                        sprintf(strMessage,"Y Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                                        //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                                        fgDecErr = TRUE;
                                        //vDumpReg();  // mark by ginny
                                        //vVDEC_HAL_VP8_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny                                          
                                        break;    
                                    }
                                }
                            }//end of if


                            pbGoldenBuf++;
                            pbDecBuf++;
                        }//End of i

                        if(fgDecErr == TRUE)
                        {
                        break;
                        }
                    }//End of j

                    if(fgDecErr == TRUE)
                    {
                        break;
                    }
                }

                if(fgDecErr == TRUE)
                {
                    break;
                }
            }
            #else              
            for (u4Pix = 0; u4Pix < u4Width*u4Height; u4Pix++)
            {
                if(1)
                {
                    //pbDecBuf = (UCHAR*)u4CalculatePixelAddress_Y(u4YBase, u4XPix, u4YPix, u4BufferWidth, 1, 4);
                    pbDecBuf = (UCHAR*)(u4YBase+u4Pix);
                    //pbGoldenBuf = (UCHAR*)u4CalculatePixelAddress_Y((UINT32)_pucDumpYBuf[u4InstID], u4XPix, u4YPix, u4BufferWidth, 1, 4);
                    pbGoldenBuf = (UCHAR*)((UINT32)_pucDumpYBuf[u4InstID]+u4Pix);
                    if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                    {
                        u4Cnt ++;
                        //vVDecOutputDebugString("Y Data Mismatch at [x= 0x%.8x, y=0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                        vVDecOutputDebugString("Y Data Mismatch at [%d] = 0x%.2x, Golden = 0x%.2x !!! \n", u4Pix, (*pbDecBuf), (*pbGoldenBuf));
                        sprintf(strMessage,"Y Data Mismatch at [%d] = 0x%.2x, Golden = 0x%.2x !!! \n", u4Pix, (*pbDecBuf), (*pbGoldenBuf));
                        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                        //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpYBuf[_u4VDecID][i]);
                        fgDecErr = TRUE;
                        //vVDEC_HAL_VP8_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny
                        break;    
                    }            
                }
                if(fgDecErr == TRUE)
                {
                    break;
                }
            }
            #endif		
            //vVDecOutputDebugString("\nY Data Compare Over!!! Total bytes [0x%.8x] & error [%d]\n", _u4GoldenYSize[_u4VDecID], u4Cnt);
        }
        #endif

        // CbCr compare
        //if(!fgIsMonoPic(_u4VDecID))
        {
            // CbCr decoded data Compare
            _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
            _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpCBuf[u4InstID];
            _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_C_SZ;  
            _tFBufFileInfo[u4InstID].u4FileLength = 0;
            vConcateStr(_bFileStr1[u4InstID][4], _bPatternPath, _bFileAddStrC, _u4FileCnt[u4InstID]);
            #ifdef EXT_COMPARE 
            _tFBufFileInfo[u4InstID].u4FileLength = (((prVDecVp8FrmHdr->u2WidthDec + 15)>>4)<<4) *(((prVDecVp8FrmHdr->u2HeightDec + 31)>>5)<<5) >>1;
            #else
            #ifdef DIRECT_DEC
            if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
            #endif    
            {  
                #ifndef DOWN_SCALE_SUPPORT
   //     _pu1VP8VDecCGoldPhy = (UINT8 *)BSP_dma_map_single((UINT32)_pucDumpCBuf[u4InstID], GOLD_C_SZ, FROM_DEVICE);
                #if VDEC_DRAM_BUSY_TEST
                i4VDEC_HAL_Dram_Busy_Off(u4InstID);
                fgOpenFile(u4InstID, _bFileStr1[u4InstID][4],"r+b", &_tFBufFileInfo[u4InstID]);               
                vDrmaBusySet(u4InstID);
                #else
                fgOpenFile(u4InstID, _bFileStr1[u4InstID][4],"r+b", &_tFBufFileInfo[u4InstID]);               
                #endif
  //      BSP_dma_unmap_single((UINT32)_pu1VP8VDecCGoldPhy, GOLD_C_SZ, FROM_DEVICE);
                _u4GoldenCSize[u4InstID] = _tFBufFileInfo[u4InstID].u4RealGetBytes;
                #endif
            }
            #endif       
         //   msleep(1000);
            u4Cnt = 0; 
            #ifdef EXT_COMPARE  
            if(_ptCurrFBufInfo[u4InstID]->ucFBufStatus == FRAME)
            {
                vWrite2PC(u4InstID, 6, _pucDecCWorkBuf[u4InstID]);
            }
            #else    
            #if defined(DOWN_SCALE_SUPPORT)
            UINT32 u4DramPicSize = 0x1FE000;
            u4CBase = (UINT32)_pucVDSCLBuf[u4InstID] + u4DramPicSize;
            #else
            u4CBase = u4NonSwapCBase;//(UINT32)_pucDecCWorkBuf[u4InstID];
            #endif  

            #ifdef DIRECT_DEC
            if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
            #endif      
            {
                #ifdef GOLDEN_128BIT_COMP
                UINT32 u4WidthC = u4Width / 2;
                UINT32 u4HeightC = u4Height / 2;
                u4Tx0 = ( (u4WidthC+7) >> 3);   // w/ 8
                u4Ty0 = ( (u4HeightC+15) >> 4);  // h /16
                u4X = (u4WidthC & 0x7);    // w % 8
                u4Y = (u4HeightC & 0xF);  // h % 16
                u4Tx1 = (u4X==0)? u4Tx0 : (u4Tx0+1);
                u4Ty1 = (u4Y==0)? u4Ty0 : (u4Ty0+1);

                for (mbh=0; mbh < u4Ty1; mbh++)
                {
                    for (mbw=0; mbw < u4Tx1; mbw++)
                    {
                    if (_tVerMpvDecPrm[u4InstID].ucAddrSwapMode != 4)
                    	{
                    	  u4Offset = mbh*(((((u4Width +63) >>6) <<6)-u4Width)*16);
                    	}
                    else
                    	{
                    	  u4Offset = 0;
                    	}
                       u4Start = (mbh*u4Tx1 + mbw) * (16*16) + u4Offset;
                       u4StartGld = (mbh*u4Tx1 + mbw) * (16*16);
                        pbGoldenBuf = (UCHAR*) (((UINT32) (_pucDumpCBuf[u4InstID])) + u4StartGld);
                        pbDecBuf = (UCHAR*) (u4CBase + u4Start);

                        for(j=0; j < 16; j++)
                        {
                            for(i=0; i < 8; i++)
                            {                       
                                if(1)
                                {
                                    if(  (mbw == u4Tx0 && i >= u4X) || (mbh == u4Ty0 && j >= u4Y))
                                    {
                                        //Do not compare
                                        pbGoldenBuf+=2;
                                        pbDecBuf+=2;
                                    }
                                    else
                                    {
                                        //Compare Cb
                                        if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                                        {
                                            u4XPix = mbw * 8 + i;
                                            u4YPix = mbh * 16 + j;
                                            u4Cnt ++;
                                            vVDecOutputDebugString("Cb Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            sprintf(strMessage,"Cb Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                                            //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                                            fgDecErr = TRUE;
                                            //vVDEC_HAL_VP8_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny
                                            break;    
                                        }

                                        pbGoldenBuf++;
                                        pbDecBuf++;
                                        //Compare Cr
                                        if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                                        {
                                            u4XPix = mbw * 8 + i;
                                            u4YPix = mbh * 16 + j;
                                            u4Cnt ++;
                                            vVDecOutputDebugString("Cr Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            sprintf(strMessage,"Cr Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                                            //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                                            fgDecErr = TRUE;
                                            //vVDEC_HAL_VP8_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny                                            
                                            break;    
                                        }
                                        pbGoldenBuf++;
                                        pbDecBuf++;
                                    }
                                }
                                else
                                {
                                    pbGoldenBuf+=2;
                                    pbDecBuf+=2;
                                }

                            }
                            if(fgDecErr == TRUE)
                            {
                                break;
                            }
                        }

                        if(fgDecErr == TRUE)
                        {
                            break;
                        }
                    }

                    if(fgDecErr == TRUE)
                    {
                        break;
                    }
                }
                #else                          
                for (u4Pix = 0; u4Pix < u4Width*(u4Height>>1); u4Pix++)
                {
                    if((_tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecVP8DecPrm.fgDec2ndFld) || (_tVerMpvDecPrm[u4InstID].ucPicStruct == FRM_PIC))
                    {
                        pbDecBuf = (UCHAR*)(u4CBase+u4Pix);
                        pbGoldenBuf = (UCHAR*)((UINT32)_pucDumpCBuf[u4InstID]+u4Pix);
                        if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                        {
                        #if defined(DOWN_SCALE_SUPPORT)
                            if(VDEC_PP_ENABLE)
                            {
                                if ((*(pbDecBuf)) > (*(pbGoldenBuf)) && (*(pbDecBuf))<= (*(pbGoldenBuf) + 1)  || 
                                    (*(pbDecBuf)) < (*(pbGoldenBuf)) && (*(pbDecBuf))>= (*(pbGoldenBuf) - 1) )
                                    {
                                        //Pass
                                        //How the C Code round off floating number method is not the same as HW in full agreement
                                        //Therefor, difference between +-1 is tolerated
                                    }
                            }
                            else
                        #endif
                            {
                            u4Cnt ++;
                            vVDecOutputDebugString("C Data Mismatch at [%d] = 0x%.2x, Golden = 0x%.2x !!! \n", u4Pix, (*pbDecBuf), (*pbGoldenBuf));
                            sprintf(strMessage,"C Data Mismatch at [%d] = 0x%.2x, Golden = 0x%.2x !!! \n", u4Pix, (*pbDecBuf), (*pbGoldenBuf));
                            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                            //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                            fgDecErr = TRUE;
                            //vVDEC_HAL_VP8_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny                            
                            break;    
                        }
                    }
                    }
                    if(fgDecErr == TRUE)
                    {
                        break;
                    }
                }
                #endif          
                //vVDecOutputDebugString("CbCr Data Compare Over!!! Total bytes [0x%.8x] & error [%d]\n", _u4GoldenCSize[_u4VDecID], u4Cnt);
            }
            #endif    
        }
        #endif
    }

    #ifndef IDE_WRITE_SUPPORT
//    if((_u4FileCnt[u4InstID]%10) == 0 || _fgVP8DumpReg)
    #endif
    {
        #ifndef IDE_WRITE_SUPPORT
        vVDecOutputDebugString("Pic count to [%d]\n", _u4FileCnt[u4InstID]);  
        #endif
        sprintf(strMessage,"[%d], \n", _u4FileCnt[u4InstID]);  
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    }
    #endif

    #ifdef REDEC    
       if(_u4ReDecCnt[u4InstID] == 0)
    #endif    
          _u4FileCnt[u4InstID] ++;

    #ifndef INTERGRATION_WITH_DEMUX
    // Check if still pic needed compare
 if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
    if (fgCompare)
    {
    #if (defined(COMP_HW_CHKSUM) && (!defined(DOWN_SCALE_SUPPORT)))
    _tTempFileInfo[u4InstID].fgGetFileInfo = TRUE;  
    _tTempFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
    _tTempFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
    _tTempFileInfo[u4InstID].u4FileLength = 0; 
    vConcateStr(_bFileStr1[u4InstID][3], _bFileStr1[u4InstID][0], "_chksum.bin\0", _u4FileCnt[u4InstID]);
    #ifdef IDE_READ_SUPPORT
    fgOpen = fgPureOpenIdeFile( _bTempStr1[u4InstID],"r+b", &_tTempFileInfo[u4InstID]);
    #else
    fgOpen = fgOpenFile(u4InstID, _bTempStr1[u4InstID],"r+b", &_tTempFileInfo[u4InstID]);
    #endif
    #else
    _tFBufFileInfo[u4InstID].fgGetFileInfo = FALSE;  
    _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
    _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
    _tFBufFileInfo[u4InstID].u4FileLength = 0;      
    vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);
    
    #ifdef IDE_READ_SUPPORT
    fgOpen = fgPureOpenIdeFile( _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);
    #else
    fgOpen = fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);
    #endif
    #endif

    if ((fgOpen == FALSE) ||(fgDecErr == TRUE) || (_fgVDecErr[u4InstID] == TRUE))
    {

        *fgNextFrameExist = FALSE;
        
        sprintf(strMessage, "%s", "\n");
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
        //fprintf(_tFileListRecInfo.fpFile, "\n");  
        // break decode
        if((fgOpen == FALSE) && (fgDecErr == FALSE))
        {
#if defined(CAPTURE_ESA_LOG) && defined(CAPTURE_ALL_IN_ONE) && (!WRITE_ESA_PER_FRAME)
            fgWrData2PC(_pucESATotalBuf[u4InstID],_u4ESATotalLen[u4InstID],7,_ESAFileName[u4InstID]); 
#endif  
            sprintf(strMessage," Compare Finish==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);   
            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            //fprintf(_tFileListRecInfo.fpFile, " Compare Finish==> Pic count to [%d] \n", _u4FileCnt[_u4VDecID] - 1);   
            if(_u4FileCnt[u4InstID] == 1)
            {
                if(fgOpen == FALSE)
                {
                    vVDecOutputDebugString("real NULL\n");
                }
            }
        }
        else
        {
            sprintf(strMessage," Error ==> Pic count to [%d] \n", _u4FileCnt[u4InstID] - 1);     
            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            //fprintf(_tFileListRecInfo.fpFile, " Error ==> Pic count to [%d] \n", _u4FileCnt[_u4VDecID] - 1);         
        }
        _u4VerBitCount[u4InstID] = 0xffffffff;
    }
  }//~fgCompare
    #endif
    if(_u4FileCnt[u4InstID] >= _u4EndCompPicNum[u4InstID])
    {
#if 0
#if defined(CAPTURE_ESA_LOG) && defined(CAPTURE_ALL_IN_ONE) && (!WRITE_ESA_PER_FRAME)
        fgWrData2PC(_pucESATotalBuf[u4InstID],_u4ESATotalLen[u4InstID],7,_ESAFileName[u4InstID]); 
#endif
#endif
        _u4VerBitCount[u4InstID] = 0xffffffff;
    }

}



void vVP8WEBPOpenFile(UINT32 u4InstID)
{
    UINT32 i, j;

    CHAR _bFileAddStrY[20] = {".y.dat\0"};
    CHAR _bFileAddStrC[20] = {".c.dat\0"};
    CHAR _bGoldFileName[256] = {"/post_\0"};
    CHAR _bPatternPath [256];
    CHAR _bPathAddStr[256] = {"\Pattern\\gp\\"};
    CHAR _bFileName [256];
    UINT32 u4Temp;
//Searh path name
    UINT32 path_byte_addr = 0;
    UINT32 filename_byte_addr = 0;
    BOOL fgDecErr;


    for (i=0; ; i++)
    {
          if (_bFileStr1[u4InstID][1][i] == '\0')
          	break;

          if (_bFileStr1[u4InstID][1][i] == 'v' || _bFileStr1[u4InstID][1][i] == 'V')
          {
              if( (_bFileStr1[u4InstID][1][i+1] == 'p' || _bFileStr1[u4InstID][1][i+1] == 'P')
              && (_bFileStr1[u4InstID][1][i+2] == '8' || _bFileStr1[u4InstID][1][i+2] == '8')
              && (_bFileStr1[u4InstID][1][i+3] == '\\')
//              && (_bFileStr1[u4InstID][1][i+4] == 'b' || _bFileStr1[u4InstID][1][i+4] == 'B')
//              && (_bFileStr1[u4InstID][1][i+5] == 'i' || _bFileStr1[u4InstID][1][i+5] == 'I')
//              && (_bFileStr1[u4InstID][1][i+6] == 't' || _bFileStr1[u4InstID][1][i+6] == 'T')
//              && (_bFileStr1[u4InstID][1][i+7] == 's' || _bFileStr1[u4InstID][1][i+7] == 'S')
//              && (_bFileStr1[u4InstID][1][i+8] == 't' || _bFileStr1[u4InstID][1][i+8] == 'T') 
              )
              {
                  path_byte_addr = i;
              }
          }
          else
          if (_bFileStr1[u4InstID][1][i] == '.')
          {
              if( (_bFileStr1[u4InstID][1][i+1] == 'i' || _bFileStr1[u4InstID][1][i+1] == 'I')
              && (_bFileStr1[u4InstID][1][i+2] == 'v' || _bFileStr1[u4InstID][1][i+2] == 'V')
              && (_bFileStr1[u4InstID][1][i+3] == 'f' || _bFileStr1[u4InstID][1][i+3] == 'F'))
              {
                  filename_byte_addr = i;
              }
          }
    }

    j = 0;
    for (i=path_byte_addr+14; i < filename_byte_addr; i++)
    {
        _bFileName[j] = _bFileStr1[u4InstID][1][i];
        j++;
    }
    _bFileName[j] = '\0';

    for (j=0; j < path_byte_addr+14; j++)
    {
        _bPatternPath[j] = _bFileStr1[u4InstID][1][j];
    }

    u4Temp = sprintf(_bPatternPath+path_byte_addr+4, "%s", _bPathAddStr)+4;  
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bFileName);
    u4Temp += sprintf(_bPatternPath+path_byte_addr+u4Temp, "%s", _bGoldFileName);    
    _bPatternPath[path_byte_addr+u4Temp] = '\0';
    vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);


    strcpy(_tFileListRecInfo[u4InstID].bFileName, _FileList_Rec[u4InstID]);

    fgDecErr = FALSE;


        _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
        _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpYBuf[u4InstID];
        _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_Y_SZ;  
        _tFBufFileInfo[u4InstID].u4FileLength = 0;  
        // Y decoded data Compare   
         vConcateStr(_bFileStr1[u4InstID][3], _bPatternPath, _bFileAddStrY, _u4FileCnt[u4InstID]);

        #ifdef DIRECT_DEC
        if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
        #endif    
        {
            fgOpenFile(u4InstID, _bFileStr1[u4InstID][3],"r+b", &_tFBufFileInfo[u4InstID]);               
            _u4GoldenYSize[u4InstID] = _tFBufFileInfo[u4InstID].u4RealGetBytes;
        }

        // CbCr decoded data Compare
        _tFBufFileInfo[u4InstID].fgGetFileInfo = TRUE;  
        _tFBufFileInfo[u4InstID].pucTargetAddr = _pucDumpCBuf[u4InstID];
        _tFBufFileInfo[u4InstID].u4TargetSz = GOLD_C_SZ;  
        _tFBufFileInfo[u4InstID].u4FileLength = 0;
        vConcateStr(_bFileStr1[u4InstID][4], _bPatternPath, _bFileAddStrC, _u4FileCnt[u4InstID]);
        #ifdef DIRECT_DEC
        if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
        #endif    
        {  
            fgOpenFile(u4InstID, _bFileStr1[u4InstID][4],"r+b", &_tFBufFileInfo[u4InstID]);               
            _u4GoldenCSize[u4InstID] = _tFBufFileInfo[u4InstID].u4RealGetBytes;
        }
        	
}

void vVP8WEBPPixComp(UINT32 u4InstID, UINT32 u4MbY)
{
    #if ((!defined(COMP_HW_CHKSUM)) || defined(DOWN_SCALE_SUPPORT))
    UINT32 u4Cnt;
    #ifdef GOLDEN_128BIT_COMP  
    UINT32 u4XPix,u4YPix;
    #endif
    UINT32 u4NonSwapYBase = 0;
    UINT32 u4NonSwapCBase = 0;

    UINT32 u4Width,u4Height;
    UINT32 u4YBase,u4CBase;
    //UINT32 u4BufferWidth;
    UCHAR *pbDecBuf,*pbGoldenBuf;

    UINT32 u4Ty0, u4Tx0, u4Ty1, u4Tx1;
    UINT32 u4X, u4Y;
    UINT32 mbw, i, j;
    UINT32 u4Start;  
    UINT32 u4StartGld;  
    UINT32 u4Offset;

    #endif  
    VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];   
    
    BOOL fgDecErr = FALSE;
    char strMessage[256];
    
    BOOL fgCompare = TRUE;

    #ifndef INTERGRATION_WITH_DEMUX
    if (!_fgOpenfile)
    	{
      vVP8WEBPOpenFile(u4InstID);
      _fgOpenfile = TRUE;
    	}

    if(fgCompare)
    {
        u4Cnt = 0; 
        u4NonSwapYBase = prVDecVp8FrmHdr->u4CurYAddr;
        u4NonSwapCBase = u4NonSwapYBase+ PIC_Y_SZ;

        u4YBase = u4NonSwapYBase;
        
        u4Width = ((prVDecVp8FrmHdr->u4Width+15)>>4)<<4;
        u4Height = ((prVDecVp8FrmHdr->u4Height+15)>>4)<<4;

        //fred add for 32byte align in height
        u4Height = ( (u4Height)>>5 ) <<5;
        #ifdef DIRECT_DEC
        if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
        #endif    
        {
            u4Tx0 = (u4Width >> 4);   // w/ 16
            u4Ty0 = (u4Height >> 4);  // h /16
            u4X = (u4Width & 0xF);    // w % 16
            u4Y = (u4Height & 0xF);  // h%16
            u4Tx1 = (u4X==0)? u4Tx0 : (u4Tx0+1);
            u4Ty1 = (u4Y==0)? u4Ty0 : (u4Ty0+1);

                for (mbw=0; mbw < u4Tx1; mbw++)
                {
                  u4Offset = 0;
                    u4Start = ((_u4MbH%2) *(16*16))  + mbw* (16*32) + u4Offset;
                    if (_u4MbH%2)
                    	{
                    u4StartGld = ((_u4MbH%2)*(16*16)) + ((_u4MbH>>1)*u4Tx1 + mbw) * (16*32);
                    	}
                    else
                    	{
                    u4StartGld = (_u4MbH == 0) ? ((_u4MbH*u4Tx1 + mbw) * (16*32)) : (((_u4MbH>>1)*u4Tx1 + mbw) * (16*32));
                    	}
                    pbGoldenBuf = (UCHAR*) (((UINT32) (_pucDumpYBuf[u4InstID])) + u4StartGld);
                    pbDecBuf = (UCHAR*) (u4YBase + u4Start);

                    for(j=0; j < 16; j++)
                    {             
                        for(i=0; i < 16; i++)
                        {                   
                                if(  (mbw == u4Tx0 && i >= u4X) || (_u4MbH == u4Ty0 && j >= u4Y))
                                {
                                    //Do not compare
                                }
                                else
                                {
                                    if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                                    {
                                        u4Cnt ++;
                                        u4XPix = mbw * 16 + i;
                                        u4YPix = _u4MbH * 16 + j;
					     vVDecOutputDebugString("Pic count to [%d]\n", _u4FileCnt[u4InstID]);  
                                        vVDecOutputDebugString("Y Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                        sprintf(strMessage,"Y Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                                        //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                                        fgDecErr = TRUE;
                                        //vDumpReg();  // mark by ginny
                                        //vVDEC_HAL_VP8_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny                                          
                                        break;    
                                    }
                                }


                            pbGoldenBuf++;
                            pbDecBuf++;
                        }//End of i

                        if(fgDecErr == TRUE)
                        {
                        break;
                        }
                    }//End of j

                    if(fgDecErr == TRUE)
                    {
                        break;
                    }
                }
        }

        // CbCr compare
            u4Cnt = 0; 
            u4CBase = u4NonSwapCBase;//(UINT32)_pucDecCWorkBuf[u4InstID];

            #ifdef DIRECT_DEC
            if((_u4FileCnt[u4InstID] >= _u4StartCompPicNum[u4InstID]) && ( _u4FileCnt[u4InstID] <= _u4EndCompPicNum[u4InstID]))
            #endif      
            {
                UINT32 u4WidthC = u4Width / 2;
                UINT32 u4HeightC = u4Height / 2;
                u4Tx0 = ( (u4WidthC+7) >> 3);   // w/ 8
                u4Ty0 = ( (u4HeightC+7) >> 3);  // h /8
                u4X = (u4WidthC & 0x7);    // w % 8
                u4Y = (u4HeightC & 0x7);  // h % 16
                u4Tx1 = (u4X==0)? u4Tx0 : (u4Tx0+1);
                u4Ty1 = (u4Y==0)? u4Ty0 : (u4Ty0+1);

                    for (mbw=0; mbw < u4Tx1; mbw++)
                    {
                    	  u4Offset = 0;
                       u4Start = ((_u4MbH%2)*(16*8)) + mbw* (16*16) + u4Offset;
                    if (_u4MbH%2)
                    	{
                    u4StartGld = ((_u4MbH%2)*(16*8)) + ((_u4MbH>>1)*u4Tx1 + mbw) * (16*16);
                    	}
                    else
                    	{
                    u4StartGld = (_u4MbH == 0) ? ((_u4MbH*u4Tx1 + mbw) * (16*16)) : (((_u4MbH>>1)*u4Tx1 + mbw) * (16*16));
                    	}
                        pbGoldenBuf = (UCHAR*) (((UINT32) (_pucDumpCBuf[u4InstID])) + u4StartGld);
                        pbDecBuf = (UCHAR*) (u4CBase + u4Start);

                        for(j=0; j < 8; j++)
                        {
                            for(i=0; i < 8; i++)
                            {                       
                                    if(  (mbw == u4Tx0 && i >= u4X) || (_u4MbH == u4Ty0 && j >= u4Y))
                                    {
                                        //Do not compare
                                        pbGoldenBuf+=2;
                                        pbDecBuf+=2;
                                    }
                                    else
                                    {
                                        //Compare Cb
                                        if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                                        {
                                            u4XPix = mbw * 8 + i;
                                            u4YPix = _u4MbH * 8 + j;
                                            u4Cnt ++;
                                            vVDecOutputDebugString("Cb Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            sprintf(strMessage,"Cb Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                                            //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                                            fgDecErr = TRUE;
                                            //vVDEC_HAL_VP8_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny
                                            break;    
                                        }

                                        pbGoldenBuf++;
                                        pbDecBuf++;
                                        //Compare Cr
                                        if ((*(pbDecBuf)) != (*(pbGoldenBuf)))
                                        {
                                            u4XPix = mbw * 8 + i;
                                            u4YPix = _u4MbH * 8 + j;
                                            u4Cnt ++;
                                            vVDecOutputDebugString("Cr Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            sprintf(strMessage,"Cr Data Mismatch at [x= 0x%d, y=0x%d] = 0x%x, Golden = 0x%x !!! \n", u4XPix, u4YPix, (*pbDecBuf), (*pbGoldenBuf));
                                            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
                                            //fprintf(_tFileListRecInfo.fpFile, "Y Data Mismatch at [0x%.8x] = 0x%.2x, Golden = 0x%.2x !!! \n", i, pbDecBuf[i], _pucDumpBuf[i]);
                                            fgDecErr = TRUE;
                                            //vVDEC_HAL_VP8_VDec_DumpReg(u4InstID, FALSE);  // mark by ginny                                            
                                            break;    
                                        }
                                        pbGoldenBuf++;
                                        pbDecBuf++;
                                    }
                            }
                            if(fgDecErr == TRUE)
                            {
                                break;
                            }
                        }

                        if(fgDecErr == TRUE)
                        {
                            break;
                        }
                    }
                //vVDecOutputDebugString("CbCr Data Compare Over!!! Total bytes [0x%.8x] & error [%d]\n", _u4GoldenCSize[_u4VDecID], u4Cnt);
            }
                _u4MbH ++ ;
    }

    #ifndef IDE_WRITE_SUPPORT
    if((_u4MbH%10) == 0 || _fgVP8DumpReg)
    #endif
    {
        #ifndef IDE_WRITE_SUPPORT
        vVDecOutputDebugString("[%d]\n", _u4MbH);  
        #endif
        sprintf(strMessage,"MB Row compare to [%d], \n", _u4MbH);  
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    }
    #endif

    _u4FileCnt[u4InstID] ++;

    #ifndef INTERGRATION_WITH_DEMUX
    // Check if still pic needed compare
    if (fgDecErr)
    {
    _u4ErrCnt++;
    sprintf(strMessage," Compare Error==> MB Row Count to [%d] \n", _u4FileCnt[u4InstID] - 1);   
    fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
    }
    #endif
    if(_u4FileCnt[u4InstID] >= _u4EndCompPicNum[u4InstID])
    {
        _u4VerBitCount[u4InstID] = 0xffffffff;
    }

}

void vVerVP8VDecEnd(UINT32 u4InstID)
{
  VDEC_INFO_VP8_VFIFO_PRM_T rVp8VFifoInitPrm;
//  VDEC_INFO_VP8_FRM_HDR_T rVp8BSInitPrm = _rVDecVp8FrmHdr[u4InstID];
  UINT32 u4VldByte,u4VldBit;
  UINT32 u4Size0, u4Size1, u4Size2, u4Size3;
  UINT32 u4TargFrm = 10000;
#if (!VDEC_VER_COMPARE_CRC)
//  VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];
#endif
#if (VDEC_VP8_ERR_TEST)
  CHAR *pBsData;
  UINT32 u4BsSize;
  UINT32 u4RndCnt;
  UINT32 u4RndPos;
  INT32   i;
#endif
  BOOL   fgNextFrameExist = TRUE;

#if (CONFIG_DRV_VERIFY_SUPPORT)
    //For Debug Only
    
    //u4VDEC_HAL_VP8_Read_QMatrix(u4BSID, u4VDecID);
    if (_u4FileCnt[u4InstID] == u4TargFrm)
    {
       //u4VDEC_HAL_VP8_Write_SRAMData1(u4BSID, u4VDecID);
       u4VDEC_HAL_VP8_Read_SRAMData1(0, u4InstID);
    }
#endif
  
  u4VldByte = u4VDEC_HAL_VP8_ReadRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], &u4VldBit) - 4;

#if VDEC_VER_COMPARE_CRC
   vVP8_CheckCRCResult(u4InstID);
#else
   vVP8WrData2PC(u4InstID, _pucDumpYBuf[u4InstID], ((((_rVDecVp8FrmHdr[u4InstID].u4Width+ 15) >> 4) * ((_rVDecVp8FrmHdr[u4InstID].u4Height + 31) >> 5)) << 9),  &fgNextFrameExist);
#endif

  // reset HW
#ifdef REDEC   
  if(_u4ReDecCnt[u4InstID] > 0)
  {
    rVp8VFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rVp8VFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    i4VDEC_HAL_VP8_InitVDecHW(u4InstID,&rVp8VFifoInitPrm);
    _rVDecVp8FrmHdr[u4InstID].u4FifoStart =  (UINT32)_pucVFifo[u4InstID];
    _rVDecVp8FrmHdr[u4InstID].u4FifoEnd= (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
    _rVDecVp8FrmHdr[u4InstID].u4VldStartPos =  (UINT32)_pucVFifo[u4InstID] + _u4VP8ByteCount[u4InstID];
  #ifndef  RING_VFIFO_SUPPORT
    _rVDecVp8FrmHdr[u4InstID].u4WritePos= (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ;
  #else
//    _rVDecVp8FrmHdr[u4InstID].u4WritePos = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
    _rVDecVp8FrmHdr[u4InstID].u4WritePos = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
  #endif
    i4VDEC_HAL_VP8_InitBarrelShifter(0, u4InstID, &_rVDecVp8FrmHdr[u4InstID]);  
    _tVerDec[u4InstID].ucState = DEC_NORM_WAIT_TO_DEC;
    return;
  }
#endif

  if (fgNextFrameExist)
  {
   _u4VP8ByteCount[u4InstID] += _u4VP8FrmSZ[u4InstID];
//   u4Size0 = (UINT32*)((UINT32)_pucVFifo[u4InstID] +_u4VP8ByteCount[u4InstID]);
//   u4Size1 = (UINT32*)((UINT32)_pucVFifo[u4InstID] +_u4VP8ByteCount[u4InstID] + 1);
//   u4Size2 = (UINT32*)((UINT32)_pucVFifo[u4InstID] +_u4VP8ByteCount[u4InstID] + 2);
//   u4Size3 = (UINT32*)((UINT32)_pucVFifo[u4InstID] +_u4VP8ByteCount[u4InstID] + 3);
   u4Size0 = *(_pucVFifo[u4InstID] +_u4VP8ByteCount[u4InstID]);
   u4Size1 = *(_pucVFifo[u4InstID] +_u4VP8ByteCount[u4InstID] + 1);
   u4Size2 = *(_pucVFifo[u4InstID] +_u4VP8ByteCount[u4InstID] + 2);
   u4Size3 = *(_pucVFifo[u4InstID] +_u4VP8ByteCount[u4InstID] + 3);

   _u4VP8FrmSZ[u4InstID] = ( ((u4Size0 & 0xFF)) | ((u4Size1 & 0xFF) << 8) | ((u4Size2 & 0xFF) << 16) | (u4Size3 & 0xFF)<<24);
   _u4VP8ByteCount[u4InstID] +=VP8_IVF_FRAME_HEADER_SZ;
    rVp8VFifoInitPrm.u4VFifoSa = (UINT32)_pucVFifo[u4InstID];
    rVp8VFifoInitPrm.u4VFifoEa = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ-1;
    i4VDEC_HAL_VP8_InitVDecHW(u4InstID,&rVp8VFifoInitPrm);
    _rVDecVp8FrmHdr[u4InstID].u4FifoStart =  (UINT32)_pucVFifo[u4InstID];
    _rVDecVp8FrmHdr[u4InstID].u4FifoEnd = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ-1;
    _rVDecVp8FrmHdr[u4InstID].u4VldStartPos =  (UINT32)_pucVFifo[u4InstID] + _u4VP8ByteCount[u4InstID];
  #ifndef  RING_VFIFO_SUPPORT
    _rVDecVp8FrmHdr[u4InstID].u4WritePos = (UINT32)_pucVFifo[u4InstID] + V_FIFO_SZ-1;
  #else
//    _rVDecVp8FrmHdr[u4InstID].u4WritePos = (UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ*(0.5 + 0.5 *(_u4LoadBitstreamCnt[u4InstID]%2)));
    _rVDecVp8FrmHdr[u4InstID].u4WritePos = (UINT32)_pucVFifo[u4InstID] + ((_u4LoadBitstreamCnt[u4InstID]%2)?(V_FIFO_SZ):(V_FIFO_SZ>>1));
  #endif
//    i4VDEC_HAL_VP8_InitBarrelShifter(_u4BSID[u4InstID], u4InstID, &_rVDecVp8FrmHdr[u4InstID]);
    _rVDecVp8FrmHdr[u4InstID].u4FrameSize= _u4VP8FrmSZ[u4InstID] ;
  //  _rVDecVp8FrmHdr[u4InstID] = _rVDecVp8FrmHdr[u4InstID];

#if (VDEC_VP8_ERR_TEST)
   u4Size0 = *((UINT32*)_pucSizeFileBuf[u4InstID] + ( ((_u4FileCnt[u4InstID]+1) *4) ) );
   u4Size1 = *((UINT32*)_pucSizeFileBuf[u4InstID] + ( ((_u4FileCnt[u4InstID]+1) *4) +1));
   u4Size2 = *((UINT32*)_pucSizeFileBuf[u4InstID] + ( ((_u4FileCnt[u4InstID]+1) *4) +2));
   u4Size3 = *((UINT32*)_pucSizeFileBuf[u4InstID] + ( ((_u4FileCnt[u4InstID]+1) *4) +3));
   u4BsSize = ( ((u4Size3 & 0xFF) << 24) | ((u4Size2 & 0xFF) << 16) | ((u4Size1 & 0xFF) << 8) | (u4Size0 & 0xFF) );   
   pBsData = (CHAR*)  (_pucVFifo[u4InstID] + _u4VP8ByteCount[u4InstID]);
   
   //
   if (u4BsSize > 20)
   {
      u4RndCnt = (UINT32) (rand()% (u4BsSize - 20));
      
      for (i=0; i <u4RndCnt; i++)
      {
          u4RndPos = 20 + (UINT32) (rand()% (u4BsSize - 20));

          if (u4RndPos < u4BsSize)
              pBsData[u4RndPos] = (UINT32) (rand()%255);
      }
   }  
#endif


#ifndef INTERGRATION_WITH_DEMUX
#ifdef  RING_VFIFO_SUPPORT
  if ( (_u4LoadBitstreamCnt[u4InstID]&0x1) 
  	 && (_rVDecVp8FrmHdr[u4InstID].u4ReadPointer  > ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))) )
  {
    _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID];
    _tInFileInfo[u4InstID].u4FileOffset = (V_FIFO_SZ * ((_u4LoadBitstreamCnt[u4InstID]+ 1)/2));
    _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
    _tInFileInfo[u4InstID].u4FileLength = 0; 
  #ifdef  SATA_HDD_READ_SUPPORT
    if(!fgOpenHDDFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
    {
      fgOpenPCFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
    }
  #else
    fgOpenPCFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #endif  
    _u4LoadBitstreamCnt[u4InstID]++;
  }
  else
  if( (!(_u4LoadBitstreamCnt[u4InstID]&0x1)) 
    && (_rVDecVp8FrmHdr[u4InstID].u4ReadPointer  < ((UINT32)_pucVFifo[u4InstID] + (V_FIFO_SZ/2))) )
  {
    _tInFileInfo[u4InstID].fgGetFileInfo = TRUE;
    _tInFileInfo[u4InstID].pucTargetAddr = _pucVFifo[u4InstID] + (V_FIFO_SZ/2);
    _tInFileInfo[u4InstID].u4FileOffset =  ((V_FIFO_SZ * (_u4LoadBitstreamCnt[u4InstID]+ 1)) /2);
    _tInFileInfo[u4InstID].u4TargetSz = (V_FIFO_SZ/2);    
    _tInFileInfo[u4InstID].u4FileLength = 0; 
  #ifdef  SATA_HDD_READ_SUPPORT
    if(!fgOpenHDDFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]))
    {
      fgOpenPCFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
    }
  #else
    fgOpenPCFile(u4InstID, (char*)_bFileStr1[u4InstID][1],"r+b", &_tInFileInfo[u4InstID]);
  #endif  
    _u4LoadBitstreamCnt[u4InstID]++;
  }
#endif
#endif

  }

  _tVerDec[u4InstID].ucState = DEC_NORM_VPARSER;

}


// *********************************************************************
// Function    : void vVerVP8UpdateBufStatus(UINT32 u4InstID)
// Description : Update VP8 Frame Buffer Status
// Parameter   : None
// Return      : None
// *********************************************************************
void vVerVP8UpdateBufStatus(UINT32 u4InstID)
{
   VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];   
   UINT32 u4FlagParam=prVDecVp8FrmHdr->rVp8DecParam.u4FlagParam;

//_pucWorkYBuf[u4InstID]=(UCHAR*)prVDecVp8FrmHdr->u4CurYAddr;
//_pucDumpLstYBuf[u4InstID]=(UCHAR*)prVDecVp8FrmHdr->u4LstYAddr;
//_pucDumpGldYBuf[u4InstID]=(UCHAR*)prVDecVp8FrmHdr->u4GldYAddr;
//_pucDumpArfYBuf[u4InstID]=(UCHAR*)prVDecVp8FrmHdr->u4AlfYAddr;

   if(prVDecVp8FrmHdr->rVp8DecParam.uCopyBuf2Arf==1) //last to arf
    {
    if (_fgVP8DumpReg)
    {
      printk("Last --> Alt\n");
    }
      _u4VP8AltBufIdx[u4InstID] = _u4VP8LastBufIdx[u4InstID]; 

//      prVDecVp8FrmHdr->u4AlfYAddr  = (UINT32)_pucDumpLstYBuf[u4InstID];
    }
   else 
   if(prVDecVp8FrmHdr->rVp8DecParam.uCopyBuf2Arf==2)//gld to arf
    {
    if (_fgVP8DumpReg)
    {
      printk("Gold --> Alt\n");
    }
      _u4VP8AltBufIdx[u4InstID] = _u4VP8GoldfBufIdx[u4InstID] ; 
//      prVDecVp8FrmHdr->u4AlfYAddr  = (UINT32)_pucDumpGldYBuf[u4InstID];
    }
   else
    {
    if (_fgVP8DumpReg)
    {
      printk("No Copy to Alt\n");
    }
//      _u4VP8AltBufIdx[u4InstID] = VP8_FRAME_NO_UPD; 
//      prVDecVp8FrmHdr->u4AlfYAddr  = (UINT32)_pucDumpGldYBuf[u4InstID];
    }

    if(prVDecVp8FrmHdr->rVp8DecParam.uCopyBuf2Gf==1) // last to gf
    {
    if (_fgVP8DumpReg)
    {
      printk("Last --> Gold\n");
    }
      _u4VP8GoldfBufIdx[u4InstID] = _u4VP8LastBufIdx[u4InstID]; 
//      prVDecVp8FrmHdr->u4GldYAddr  = (UINT32)_pucDumpLstYBuf[u4InstID];
    }
    else 
    if(prVDecVp8FrmHdr->rVp8DecParam.uCopyBuf2Gf==2) // alt to gf
    {   
    if (_fgVP8DumpReg)
    {
      printk("Alt --> Gold\n");
    }
      _u4VP8GoldfBufIdx[u4InstID] = _u4VP8AltBufIdx[u4InstID]; 
//      prVDecVp8FrmHdr->u4GldYAddr  = (UINT32)_pucDumpArfYBuf[u4InstID];
   }
    else
    {   
    if (_fgVP8DumpReg)
    {
      printk("No Copy to Gold\n");
    }
//      _u4VP8GoldfBufIdx[u4InstID] = VP8_FRAME_NO_UPD; 
//      prVDecVp8FrmHdr->u4GldYAddr  = (UINT32)_pucDumpArfYBuf[u4InstID];
   }

   if(VDEC_FLGSET(u4FlagParam,VP8PARAM_REFRESH_GOLDEN))
   {
    if (_fgVP8DumpReg)
    {
      printk("Gold Refresh\n");
    }
      _u4VP8GoldfBufIdx[u4InstID] = _u4VP8CurrBufIdx[u4InstID]; 
//      prVDecVp8FrmHdr->u4GldYAddr  = (UINT32)_pucWorkYBuf[u4InstID];
   }
   else
  {
    if (_fgVP8DumpReg)
    {
      printk("Gold NO Refresh\n");
    }
//      _u4VP8GoldfBufIdx[u4InstID] = VP8_FRAME_NO_UPD; 
//      prVDecVp8FrmHdr->u4GldYAddr  = (UINT32)_pucWorkYBuf[u4InstID];
   }   	

   if(VDEC_FLGSET(u4FlagParam,VP8PARAM_REFRESH_ALTRF))
   {
    if (_fgVP8DumpReg)
    {
      printk("Alt Refresh\n");
    }
      _u4VP8AltBufIdx[u4InstID] = _u4VP8CurrBufIdx[u4InstID]; 
//      prVDecVp8FrmHdr->u4AlfYAddr  =(UINT32) _pucWorkYBuf[u4InstID];
//       _pucDumpArfYBuf[u4InstID] = pucWorkYTmp;
//      _pucWorkYBuf[u4InstID] = pucArfYTmp;
   }
   else
  {
    if (_fgVP8DumpReg)
    {
      printk("Alt NO Refresh\n");
    }
//      _u4VP8AltBufIdx[u4InstID] = VP8_FRAME_NO_UPD; 
//      prVDecVp8FrmHdr->u4GldYAddr  = (UINT32)_pucWorkYBuf[u4InstID];
   }   	
   
   if(VDEC_FLGSET(u4FlagParam,VP8PARAM_REFRESH_LASTFRAME))
   {
    if (_fgVP8DumpReg)
    {
      printk("Last Refresh\n");
    }
      _u4VP8LastBufIdx[u4InstID] = _u4VP8CurrBufIdx[u4InstID]; 
//      prVDecVp8FrmHdr->u4LstYAddr  = (UINT32)_pucWorkYBuf[u4InstID];
//       _pucDumpLstYBuf[u4InstID] = pucWorkYTmp;
//      _pucWorkYBuf[u4InstID] = pucLstYTmp;
   }
   else
  {
    if (_fgVP8DumpReg)
    {
      printk("Last NO Refresh\n");
    }
//      _u4VP8LastBufIdx[u4InstID] = VP8_FRAME_NO_UPD; 
//      prVDecVp8FrmHdr->u4GldYAddr  = (UINT32)_pucWorkYBuf[u4InstID];
   }   	
 //   _pucWorkCBuf[u4InstID] = _pucWorkYBuf[u4InstID] +PIC_Y_SZ;


 
}

// *********************************************************************
// Function    : BOOL fgIsVP8VDecComplete(UINT32 u4InstID)
// Description : Check if VDec complete with interrupt
// Parameter   : None
// Return      : None
// *********************************************************************
BOOL fgIsVP8VDecComplete(UINT32 u4InstID)
{
  UINT32 u4MbX;
  UINT32 u4MbY;  
  VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];
  
  if(_fgVDecComplete[u4InstID])
  {
      vVDEC_HAL_VP8_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION
      printk("u4MbX = 0x%x, ((prVDecVp8FrmHdr->u4Width>>4)&0x1FF) = 0x%x\n",  u4MbX, ((prVDecVp8FrmHdr->u4Width>>4)&0x1FF));
      printk("u4MbY = 0x%x, ((prVDecVp8FrmHdr->u4Height>>4)&0x1FF) = 0x%x\n", u4MbY, ((prVDecVp8FrmHdr->u4Height>>4)&0x1FF));
      printk("prVDecVp8FrmHdr->u4Width = 0x%x, prVDecVp8FrmHdr->u4Height = 0x%x\n", prVDecVp8FrmHdr->u4Width, prVDecVp8FrmHdr->u4Height);      

      if(((u4MbX+1)>=((prVDecVp8FrmHdr->u4Width>>4)&0x1FF))&&((u4MbY+1)>=((prVDecVp8FrmHdr->u4Height>>4)&0x1FF)))
      {
        return TRUE;
      }
      else
      {
        vVDecWriteVLDTOP(u4InstID, RW_VLD_TOP_MB_DECSTART, 0x1);
        return FALSE;
      }    
#endif
      #if VP8_MB_ROW_MODE_SUPPORT
      if ((u4VDecReadVP8VLD(0,RO_VP8_VOKR)>>8))// for mb row mode check decode done a mb row.
      	{
     // 	printk("u4MbX = 0x%x, u4MbY = 0x%x\n", u4MbX, u4MbY);
      	if (_testCnt)
      	{
      	  vVP8WEBPPixComp(u4InstID, u4MbY);
     //    HalFlushInvalidateDCache();
      	}
      	vVDecWriteVLDTOP(u4InstID, RW_VLD_TOP_MB_DECSTART, 1);
      	_fgVDecComplete[u4InstID] = FALSE;
      	_testCnt++;
      	return FALSE;
  	}
      if (!_fgLastMBRow)
      	{
      	if (_testCnt)
      	{
      	  vVP8WEBPPixComp(u4InstID, u4MbY);
  //       HalFlushInvalidateDCache();
      	}
      	vVDecWriteVLDTOP(u4InstID, RW_VLD_TOP_MB_DECSTART, 1);
      	_testCnt++;
      	_fgLastMBRow = TRUE;
      	  vVP8WEBPPixComp(u4InstID, u4MbY);
      	}
      #endif
  
// because get MbX and MbY only have 9 bits, so we only compare 9bits
      if(((u4MbX+1)>=((prVDecVp8FrmHdr->u4Width>>4)&0x1FF))&&((u4MbY+1)>=((prVDecVp8FrmHdr->u4Height>>4)&0x1FF)))
      {
        return TRUE;
      }
      else
      {
        return FALSE;
      }    
  }

  return FALSE;
}

static VOID vVDEC_Vp8DecFinish(VDEC_INFO_VP8_FRM_HDR_T *pVp8DecInfo)
{
  VDEC_PARAM_VP8DEC_T *prVp8DecParam=&pVp8DecInfo->rVp8DecParam;
   if(!VDEC_FLGSET(prVp8DecParam->u4FlagParam,VP8PARAM_REFRESH_PROBS))
   {
      memcpy(&prVp8DecParam->rCurFc,&prVp8DecParam->rLastFC,sizeof(VDEC_VP8FRAME_CONTEXT_T));
   }
   else if(VDEC_FLGSET(prVp8DecParam->u4FlagParam,VP8PARAM_REFRESH_PROBS))
   {
      _VDEC_Vp8LoadCoefProbs(&prVp8DecParam->rCurFc);
      if(pVp8DecInfo->uFrameType==VP8_P_FRM)
      {
         _VDEC_Vp8LoadCtxProbs(&prVp8DecParam->rCurFc);
      }
   }
}

#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION
UINT32 vVerVP8DecEndProc_MB_ROW_START(UINT32 u4InstID)
{
    UINT32 u4Cnt = 0;
    UINT32 u4CntTimeChk = 0;
    UINT32 u4MbX;
    UINT32 u4MbY;  
    UINT32 u4MbX_last;
    UINT32 u4MbY_last;
    UINT32 u4RetValue = vVerResult_TIMEOUT;
   
    while(u4CntTimeChk < DEC_RETRY_NUM)
    {    
        u4Cnt ++;    
        if((u4Cnt & 0x3f)== 0x3f)
        {
            if (((u4VDecReadVLDTOP(u4InstID, 0xB4))&0x1) == 0x1)
            {
                _fgVDecComplete[u4InstID] = TRUE;
                u4RetValue = vVerResult_MB_ROW_DONE;
            }
            else
            {
                if(u4VDEC_HAL_VP8_VDec_ReadFinishFlag(u4InstID))
                {
                    _fgVDecComplete[u4InstID] = TRUE;
                    u4RetValue = vVerResult_FRAME_DONE;
                }
                else
                {
                    _fgVDecComplete[u4InstID] = FALSE;
                }
            }
      
            if(fgIsVP8VDecComplete(u4InstID))
            {
                u4CntTimeChk = 0;
                #ifdef CAPTURE_ESA_LOG
                vWrite2PC(u4InstID, 17, (UCHAR*)_pucESALog[u4InstID]);
                #endif
                break;
            }
            else
            {
                u4MbX_last = u4MbX;
                u4MbY_last = u4MbY;
                vVDEC_HAL_VP8_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
                if((u4MbX == u4MbX_last) && (u4MbY == u4MbY_last))
                {
                    u4CntTimeChk ++;
                }
                else
                {
                    u4CntTimeChk =0;
                }
            }
            u4Cnt = 0;
        }

        if (u4RetValue != vVerResult_TIMEOUT)
        {
            break;
        }
    }
    
    return u4RetValue;
}

void vVerVP8DecEndProc_MB_ROW_End(UINT32 u4InstID)
{
    UINT32 u4Cnt;
    UINT32 u4CntTimeChk;
    UINT32 u4MbX;
    UINT32 u4MbY;  
    UCHAR strMessage[256];
    UINT32 u4MbX_last;
    UINT32 u4MbY_last;
    UINT32 u4VP8ErrType = 0;
    VDEC_INFO_VP8_ERR_INFO_T rVp8ErrInfo;
    VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];
   
    u4Cnt=0;
    u4CntTimeChk = 0;
    _fgVDecErr[u4InstID] = FALSE;

    vVDEC_Vp8DecFinish(prVDecVp8FrmHdr);
#if VDEC_VP8_WEBP_SUPPORT
    if (_u4ErrCnt !=0)
    {printk("Webp Compare NG, ErrCnt = 0x%x\n", (UINT32)_u4ErrCnt);}
    else
    {printk("Webp Compare OK\n");}
    _u4MbH =0;
    _testCnt =0;
    _fgOpenfile = FALSE;
    _fgLastMBRow =FALSE;
    _u4ErrCnt =0;
#endif
    if (_fgVP8DumpReg)
    {    vVDEC_Vp8DumpReg();}
  
    vVDEC_HAL_VP8_GetErrInfo(u4InstID, &rVp8ErrInfo);
   
#ifndef VDEC_BREAK_EN
    if((u4CntTimeChk == DEC_RETRY_NUM) || (u4VP8ErrType!= 0))// || (rVp8ErrInfo.u4Vp8ErrCnt != 0))
    {
#ifndef INTERGRATION_WITH_DEMUX
        _fgVDecErr[u4InstID] = TRUE;
        if(u4CntTimeChk == DEC_RETRY_NUM)
        {
            vVDecOutputDebugString("\n!!!!!!!!! Decoding Timeout !!!!!!!\n");
            printk("\n!!!!!!!!! DDecoding Timeout %d!!!!!!!", DEC_RETRY_NUM);
            sprintf(strMessage, "%s", "\n!!!!!!!!! Decoding Timeout !!!!!!!");
            fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
            printk("u4MbX = 0x%x, u4MbY = 0x%x\n", u4MbX, u4MbY);
        }
        vVDEC_HAL_VP8_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
        fgWrMsg2PC((void*)strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
        sprintf(strMessage,"the length is %d (0x%.8x)\n", _tInFileInfo[u4InstID].u4FileLength, _tInFileInfo[u4InstID].u4FileLength);
        fgWrMsg2PC((void*)strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
        vWrite2PC(u4InstID, 1, (UCHAR*)_pucVFifo[u4InstID]);
        vWrite2PC(u4InstID, 12, (UCHAR *)(&_u4DumpChksum[u4InstID][0]));
#endif
    }
#endif
    vVerifySetVSyncPrmBufPtr(u4InstID, _u4DecBufIdx[u4InstID]);

    if((_u4FileCnt[u4InstID]%10) == 0 || _fgVP8DumpReg)
    {
        sprintf(strMessage,"======");  
        printk("%s\n", strMessage);
    }
  
    vVerVP8VDecEnd(u4InstID);
    vVerVP8UpdateBufStatus(u4InstID);
}
#endif

void vVerVP8DecEndProc(UINT32 u4InstID)
{
  UINT32 u4Cnt;
  UINT32 u4CntTimeChk;
  UINT32 u4MbX;
  UINT32 u4MbY;  
  UCHAR strMessage[256];
  UINT32 u4MbX_last;
  UINT32 u4MbY_last;
  UINT32 u4VP8ErrType = 0;
  VDEC_INFO_VP8_ERR_INFO_T rVp8ErrInfo;
   VDEC_INFO_VP8_FRM_HDR_T *prVDecVp8FrmHdr = &_rVDecVp8FrmHdr[u4InstID];

#if 0//ChunChia_LOG
  UINT32 u4Mc770, u4Mc774, u4Mc778, u4Mc8B8;
#endif
   
  u4Cnt=0;
  u4CntTimeChk = 0;
  _fgVDecErr[u4InstID] = FALSE;
 // if (0)
  {
    while(u4CntTimeChk < DEC_RETRY_NUM)
    {    
      u4Cnt ++;    
      if((u4Cnt & 0x3f)== 0x3f)
      {
  #ifndef IRQ_DISABLE    
  #else
#if VP8_MB_ROW_MODE_SUPPORT_ME2_INTEGRATION        
        if (((u4VDecReadVLDTOP(u4InstID, 0xB4))&0x1) == 0x1)
        {
            _fgVDecComplete[u4InstID] = TRUE;
        }
        else
        {
            if(u4VDEC_HAL_VP8_VDec_ReadFinishFlag(u4InstID))
            {
                _fgVDecComplete[u4InstID] = TRUE;
            }
            else
            {
                _fgVDecComplete[u4InstID] = FALSE;
            }
        }
#else
        if(u4VDEC_HAL_VP8_VDec_ReadFinishFlag(u4InstID))
        {
          _fgVDecComplete[u4InstID] = TRUE;
        }
#endif
  #endif      
        if(fgIsVP8VDecComplete(u4InstID))
        {
          u4CntTimeChk = 0;
          #ifdef CAPTURE_ESA_LOG
          vWrite2PC(u4InstID, 17, (UCHAR*)_pucESALog[u4InstID]);
          #endif
          break;
        }
        else
        {
          u4MbX_last = u4MbX;
          u4MbY_last = u4MbY;
          vVDEC_HAL_VP8_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
          if((u4MbX == u4MbX_last) && (u4MbY == u4MbY_last))
          {
            u4CntTimeChk ++;
          }
          else
          {
            u4CntTimeChk =0;
          }
        }
        u4Cnt = 0;
      }
    }

#if(CONFIG_DRV_VERIFY_SUPPORT && !MEM_ALLOCATE_IOREMAP)    
        dma_unmap_single(NULL, (UINT32)_pu1VP8VDecYAddrPhy, GOLD_Y_SZ+GOLD_C_SZ, DMA_FROM_DEVICE);
        dma_unmap_single(NULL, (UINT32)_pu1VP8VDecYAltPhy, GOLD_Y_SZ+GOLD_C_SZ, DMA_TO_DEVICE);
        dma_unmap_single(NULL, (UINT32)_pu1VP8VDecYCurPhy, GOLD_Y_SZ+GOLD_C_SZ, DMA_TO_DEVICE);
        dma_unmap_single(NULL, (UINT32)_pu1VP8VDecYGldPhy, GOLD_Y_SZ+GOLD_C_SZ, DMA_TO_DEVICE);
  #endif    

   vVDEC_Vp8DecFinish(prVDecVp8FrmHdr);
//      printk("u4MbX = 0x%x, u4MbY = 0x%x\n", u4MbX, u4MbY);
//      printk("_testCnt = 0x%x\n", (UINT32)_testCnt);
      #if VDEC_VP8_WEBP_SUPPORT
      if (_u4ErrCnt !=0)
      {printk("Webp Compare NG, ErrCnt = 0x%x\n", (UINT32)_u4ErrCnt);}
      	else
      {printk("Webp Compare OK\n");}
      _u4MbH =0;
      _testCnt =0;
      _fgOpenfile = FALSE;
     _fgLastMBRow =FALSE;
     _u4ErrCnt =0;
      #endif
if (_fgVP8DumpReg)
{    vVDEC_Vp8DumpReg();}
  
    vVDEC_HAL_VP8_GetErrInfo(u4InstID, &rVp8ErrInfo);
   // u4VP8ErrType = u4VDEC_HAL_VP8_GetErrType(u4InstID);
  #ifndef VDEC_BREAK_EN
    if((u4CntTimeChk == DEC_RETRY_NUM) || (u4VP8ErrType!= 0))// || (rVp8ErrInfo.u4Vp8ErrCnt != 0))
    {
    #ifndef INTERGRATION_WITH_DEMUX
    //#ifdef EXT_COMPARE     
      _fgVDecErr[u4InstID] = TRUE;
    //#endif
      if(u4CntTimeChk == DEC_RETRY_NUM)
      {
        vVDecOutputDebugString("\n!!!!!!!!! Decoding Timeout !!!!!!!\n");
        printk("\n!!!!!!!!! DDecoding Timeout %d!!!!!!!", DEC_RETRY_NUM);
        sprintf(strMessage, "%s", "\n!!!!!!!!! Decoding Timeout !!!!!!!");
        fgWrMsg2PC(strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
        //vDumpReg();
      printk("u4MbX = 0x%x, u4MbY = 0x%x\n", u4MbX, u4MbY);
      }
      vVDEC_HAL_VP8_GetMbxMby(u4InstID, &u4MbX, &u4MbY);
     // vVDecOutputDebugString("\n!!!!!!!!! Decoding Error 0x%.8x!!!!!!!\n", rVp8ErrInfo.u4Vp8ErrType);
     // sprintf(strMessage,"\n!!!!!!!!! Decoding Error 0x%.8x 0x%.8x 0x%.8xat MC (x,y)=(%d/%d, %d/%d)  !!!!!!!\n", u4VP8ErrType, 
     //           rVp8ErrInfo.u4Vp8ErrType,rVp8ErrInfo.u4Vp8ErrRow,u4MbX,0,u4MbY,0);// prVDecVp8FrmHdr->u2HFragments -1, u4MbY, prVDecVp8FrmHdr->u2VFragments -1);
      fgWrMsg2PC((void*)strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      sprintf(strMessage,"the length is %d (0x%.8x)\n", _tInFileInfo[u4InstID].u4FileLength, _tInFileInfo[u4InstID].u4FileLength);
      fgWrMsg2PC((void*)strMessage,strlen(strMessage),8,&_tFileListRecInfo[u4InstID]);
      //vReadVP8ChkSumGolden(u4InstID);
      vWrite2PC(u4InstID, 1, (UCHAR*)_pucVFifo[u4InstID]);
      vWrite2PC(u4InstID, 12, (UCHAR *)(&_u4DumpChksum[u4InstID][0]));
      //vDumpReg();
    #endif
    }
 #endif
    //vVDEC_HAL_VP8_AlignRdPtr(0, u4InstID, (UINT32)_pucVFifo[u4InstID], BYTE_ALIGN);
    vVerifySetVSyncPrmBufPtr(u4InstID, _u4DecBufIdx[u4InstID]);
    //vReadVP8ChkSumGolden(u4InstID);
  }

//Print LOG
    if((_u4FileCnt[u4InstID]%10) == 0 || _fgVP8DumpReg)
    {
    sprintf(strMessage,"======");  
    printk("%s\n", strMessage);
    }
#if 0//ChunChia_LOG  
  u4Mc770 = u4VDecReadMC(u4InstID, 0x770);
  u4Mc774 = u4VDecReadMC(u4InstID, 0x774);
  u4Mc778 = u4VDecReadMC(u4InstID, 0x778);
  u4Mc8B8 = u4VDecReadMC(u4InstID, 0x8B8);

  sprintf(strMessage,"======\n");  
  Printf("%s", strMessage);
  
  sprintf(strMessage,"(dram_dle_cnt: 0x%x, mc_dle_cnt: 0x%x, cycle_cnt: 0x%x, dram_dle_by_preq: 0x%x)\n", u4Mc770, u4Mc774, u4Mc778, u4Mc8B8);
  Printf("%s", strMessage);

#endif
  
  vVerVP8VDecEnd(u4InstID);
  vVerVP8UpdateBufStatus(u4InstID);
 // HalFlushInvalidateDCache();

}


