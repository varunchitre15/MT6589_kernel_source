

#ifndef __TVROT_DRV_H__
#define __TVROT_DRV_H__


#define MTK_TVROT_LDVT

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------

#define TVR_CHECK_RET(expr)             \
    do {                                \
        TVR_STATUS ret = (expr);        \
        ASSERT(TVR_STATUS_OK == ret);   \
    } while (0)

// ---------------------------------------------------------------------------

typedef enum
{
   TVR_STATUS_OK = 0,

   TVR_STATUS_ERROR,
   TVR_STATUS_INSUFFICIENT_SRAM,
} TVR_STATUS;


typedef enum
{
    TVR_RGB565  = 2,
    TVR_YUYV422 = 4,
} TVR_FORMAT;


typedef enum
{
    TVR_ROT_0   = 0,
    TVR_ROT_90  = 1,
    TVR_ROT_180 = 2,
    TVR_ROT_270 = 3,
} TVR_ROT;

typedef struct
{
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
} TVR_SRC_ROI;


// ---------------------------------------------------------------------------


#define TVR_BUFFERS (16)

typedef struct
{
    unsigned int srcWidth;
    unsigned int srcHeight;
    TVR_SRC_ROI  srcRoi;

    unsigned int dstWidth;
    TVR_FORMAT outputFormat;
    unsigned int dstBufNum;
    unsigned int dstBufAddr[TVR_BUFFERS];

    TVR_ROT rotation;
    int    bAuto;
    int    flip;

} TVR_PARAM;


TVR_STATUS TVR_Init(void);
TVR_STATUS TVR_Deinit(void);

TVR_STATUS TVR_PowerOn(void);
TVR_STATUS TVR_PowerOff(void);

TVR_STATUS TVR_Config(const TVR_PARAM *param);
TVR_STATUS TVR_Start(void);
TVR_STATUS TVR_Stop(void);
TVR_STATUS TVR_Wait_Done(void);


// Debug
TVR_STATUS TVR_DumpRegisters(void);


// ---------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

#endif // __TVROT_DRV_H__
