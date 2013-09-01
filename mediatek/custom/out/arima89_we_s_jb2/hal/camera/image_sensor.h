#ifndef _IMAGE_SENSOR_H
#define _IMAGE_SENSOR_H

//#if defined(MT6516)
#include "kd_imgsensor.h"
//#endif //defined(MT6516)

#if 0
#if defined(MT6573)
/* SENSOR CHIP VERSION */
#define MC501CB_SENSOR_ID                       0x0062
#define MC501CC_SENSOR_ID                       0x0074
#define MC501CA_SENSOR_ID                       0x0011

#define MT9D011_SENSOR_ID                       0x1511
#define MT9D111_SENSOR_ID                       0x1511
#define MT9D112_SENSOR_ID                       0x1580
#define MT9M011_SENSOR_ID                       0x1433
#define MT9M111_SENSOR_ID                       0x143A
#define MT9M112_SENSOR_ID                       0x148C
#define MT9M113_SENSOR_ID                       0x2480
#define MT9P012_SENSOR_ID                       0x2801
#define MT9T012_SENSOR_ID                       0x1600
#define MT9T013_SENSOR_ID                       0x2600
#define MT9V112_SENSOR_ID                       0x1229
#define MT9DX11_SENSOR_ID                       0x1519
#define MT9D113_SENSOR_ID                       0x2580

#define NOON200PC11_SENSOR_ID                   0x0013
#define NOON200PC20_SENSOR_ID                   0x0063
#define NOON200PC40_SENSOR_ID                   0x0063
#define NOON200PC51_SENSOR_ID                   0x006C
#define NOON130PC51_SENSOR_ID                   0x0076

#define OV2630_SENSOR_ID                        0x2633
#define OV2640_SENSOR_ID                        0x2642
#define OV2650_SENSOR_ID                        0x2652
#define OV3640_SENSOR_ID                        0x364C
#define OV6680_SENSOR_ID                        0x6681
#define OV7660_SENSOR_ID                        0x7660
#define OV7670_SENSOR_ID                        0x7673
#define OV7680_SENSOR_ID                        0x7680
#define OV9650_SENSOR_ID                        0x9652
#define OV9655_SENSOR_ID                        0x9657
#define OV9660_SENSOR_ID                        0x9663
#define OV3647_SENSOR_ID                        0x364A
#define OV2655_SENSOR_ID                        0x2656
#define OV9665_SENSOR_ID                        0x9663
#define OV5630_SENSOR_ID                        0x5634
#define OV7675_SENSOR_ID                        0x7673

#define PO6030K_SENSOR_ID                       0x0060
#define PO4010K_SENSOR_ID                       0x0040

#define SID020A_SENSOR_ID                       0x12B4
#define SIV100B_SENSOR_ID                       0x0C11
#define SIV100A_SENSOR_ID                       0x0C10
#define SIV120A_SENSOR_ID                       0x1210
#define SIV120B_SENSOR_ID                       0x0012
#define SIM101B_SENSOR_ID                       0x09A0
#define SIM120C_SENSOR_ID                       0x0012
#define SID130B_SENSOR_ID                       0x001b
#define SIC110A_SENSOR_ID                       0x000D

#define S5KA3DFX_SENSOR_ID                      0x00AB
#define S5K4B2FX_SENSOR_ID                      0x5080
#define S5K3AAEA_SENSOR_ID                      0x07AC
#define S5K3BAFB_SENSOR_ID                      0x7070
#define S5K53BEX_SENSOR_ID                      0x45A8
#define S5K53BEB_SENSOR_ID                      0x87A8
#define S5K83AFX_SENSOR_ID                      0x01C4
#define S5K5BAFX_SENSOR_ID                      0x05BA
#define S5K3E2FX_SENSOR_ID                      0x3E2F

#define PAS105_SENSOR_ID                        0x0065
#define PAS302_SENSOR_ID                        0x0064
#define PAS5101_SENSOR_ID                       0x0067

#define ET8EE6_SENSOR_ID                        0x0034
#define ET8EF2_SENSOR_ID                        0x1048

#define OM6802_SENSOR_ID                        0x1705

#define HV7131_SENSOR_ID                        0x0042

#define RJ53S1BA0C_SENSOR_ID                    0x0129

#define HI251_SENSOR_ID                         0x0084
#define HIVICF_SENSOR_ID                        0x0081

#define IMX058_SENSOR_ID                        0x0058
#define MT9V113_SENSOR_ID                         0x2280
#define MT9P015_SENSOR_ID                       0x2803

#define MT9V114_SENSOR_ID                         0x2283
/* SENSOR DEVICE DRIVER NAME */
#define SENSOR_DRVNAME_MT9P012_RAW  "mt9p012"
#define SENSOR_DRVNAME_MT9P015_RAW  "mt9p015"
#define SENSOR_DRVNAME_OV2650_RAW   "ov265x"
#define SENSOR_DRVNAME_OV2655_YUV   "ov2655yuv"
#define SENSOR_DRVNAME_OV3640_RAW   "ov3640"
#define SENSOR_DRVNAME_OV3640_YUV    "ov3640yuv"
#define SENSOR_DRVNAME_MT9V113_YUV    "mt9v113yuv"
#endif //#if defined(MT6573)

#define SENSOR_DRVNAME_MT9V114_YUV    "mt9v114"
#endif

#endif /* _IMAGE_SENSOR_H */

