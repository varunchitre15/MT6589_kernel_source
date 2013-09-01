#ifndef _DDP_SCL_API_H_
#define _DDP_SCL_API_H_

#include "ddp_drv.h"

#define RDMA_INSTANCES  2
#define RDMA_MAX_WIDTH  2047
#define RDMA_MAX_HEIGHT 2047

// start module
int SCLStart(void);

// stop module
int SCLStop(void);

// reset module
int SCLReset(void);

// configu module
int SCLConfig(DISP_INTERLACE_FORMAT interlace,
              int rotateDegree,
              int srcWidth, int srcHeight,
              int dstWidth, int dstHeight,
              int flip);

void disp_scl_set_scale_param (unsigned src_width,
                               unsigned src_height,
                               unsigned dst_width,
                               unsigned dst_height,
                               unsigned char interlaced,  //0=progressive source, 1=interlaced source
                               unsigned char fieldID,     //0=source is top field, 1=source is bottom field
                               unsigned char rotation,    //source rotated; 0=no, 1=90, 2=180, 3=270 (clockwise)
                               unsigned char flipping);   //source horizontal flipped; 0=no flip, 1=flipped
void disp_scl_reg_enable(BOOL enable);
void disp_scl_enable_relay_mode(BOOL enable, unsigned src_width, unsigned src_height);


//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
#define DSCL_CTRL_FLD_SCL_RESET                                REG_FLD(1, 1)
#define DSCL_CTRL_FLD_SCL_EN                                   REG_FLD(1, 0)

#define DSCL_INTEN_FLD_OF_END_INT_EN                           REG_FLD(1, 1)
#define DSCL_INTEN_FLD_IF_END_INT_EN                           REG_FLD(1, 0)

#define DSCL_INTSTA_FLD_OF_END_INT                             REG_FLD(1, 1)
#define DSCL_INTSTA_FLD_IF_END_INT                             REG_FLD(1, 0)

#define DSCL_STATUS_FLD_OF_UNFINISH                            REG_FLD(1, 1)
#define DSCL_STATUS_FLD_IF_UNFINISH                            REG_FLD(1, 0)

#define DSCL_CFG_FLD_C444_SEL                                  REG_FLD(1, 11)
#define DSCL_CFG_FLD_C422_SEL                                  REG_FLD(1, 10)
#define DSCL_CFG_FLD_VRZ_POSIT                                 REG_FLD(1, 9)
#define DSCL_CFG_FLD_HRZ_POSIT                                 REG_FLD(1, 8)
#define DSCL_CFG_FLD_VPK_EN                                    REG_FLD(1, 7)
#define DSCL_CFG_FLD_HPK_EN                                    REG_FLD(1, 6)
#define DSCL_CFG_FLD_YUV422                                    REG_FLD(1, 4)
#define DSCL_CFG_FLD_FIFO_EN                                   REG_FLD(1, 3)
#define DSCL_CFG_FLD_VRZ_EN                                    REG_FLD(1, 2)
#define DSCL_CFG_FLD_HRZ_EN                                    REG_FLD(1, 1)
#define DSCL_CFG_FLD_RELAY_MODE                                REG_FLD(1, 0)

#define DSCL_INP_CHKSUM_FLD_INP_CHKSUM_EN                      REG_FLD(1, 31)
#define DSCL_INP_CHKSUM_FLD_INP_CHKSUM                         REG_FLD(24, 0)

#define DSCL_OUTP_CHKSUM_FLD_OUTP_CHKSUM_EN                    REG_FLD(1, 31)
#define DSCL_OUTP_CHKSUM_FLD_OUTP_CHKSUM                       REG_FLD(24, 0)

#define DSCL_HRZ_CFG_FLD_HRZ_COEF_SEL                          REG_FLD(5, 8)
#define DSCL_HRZ_CFG_FLD_HRZ_DERING_EN                         REG_FLD(1, 5)
#define DSCL_HRZ_CFG_FLD_HRZ_COEF_TYPE                         REG_FLD(1, 4)
#define DSCL_HRZ_CFG_FLD_HRZ_TAPNUM_SEL                        REG_FLD(2, 2)
#define DSCL_HRZ_CFG_FLD_HRZ_METHOD                            REG_FLD(1, 1)
#define DSCL_HRZ_CFG_FLD_HRZ_MODE                              REG_FLD(1, 0)

#define DSCL_HRZ_SIZE_FLD_HRZ_TARSZ                            REG_FLD(12, 16)
#define DSCL_HRZ_SIZE_FLD_HRZ_SRCSZ                            REG_FLD(12, 0)

#define DSCL_HRZ_FACTOR_FLD_HRZ_FACTOR                         REG_FLD(24, 0)

#ifndef REG_BASE_C_MODULE
#define DSCL_HRZ_OFFSET_FLD_HRZ_OFFSET                         REG_FLD(24, 0)
#else
#define DSCL_HRZ_OFFSET_FLD_HRZ_OFFSET                         REG_FLD(32, 0)
#endif

#define DSCL_VRZ_CFG_FLD_VRZ_COEF_SEL                          REG_FLD(5, 8)
#define DSCL_VRZ_CFG_FLD_VRZ_DERING_EN                         REG_FLD(1, 5)
#define DSCL_VRZ_CFG_FLD_VRZ_COEF_TYPE                         REG_FLD(1, 4)
#define DSCL_VRZ_CFG_FLD_VRZ_TAPNUM_SEL                        REG_FLD(2, 2)
#define DSCL_VRZ_CFG_FLD_VRZ_METHOD                            REG_FLD(1, 1)
#define DSCL_VRZ_CFG_FLD_VRZ_MODE                              REG_FLD(1, 0)

#define DSCL_VRZ_SIZE_FLD_VRZ_TARSZ                            REG_FLD(12, 16)
#define DSCL_VRZ_SIZE_FLD_VRZ_SRCSZ                            REG_FLD(12, 0)

#define DSCL_VRZ_FACTOR_FLD_VRZ_FACTOR                         REG_FLD(24, 0)

#ifndef REG_BASE_C_MODULE
#define DSCL_VRZ_OFFSET_FLD_VRZ_OFFSET                         REG_FLD(24, 0)
#else
#define DSCL_VRZ_OFFSET_FLD_VRZ_OFFSET                         REG_FLD(32, 0)
#endif

#define DSCL_EXT_COEF_FLD_EXTC_SEL                             REG_FLD(1, 29)
#define DSCL_EXT_COEF_FLD_EXTC_WEN                             REG_FLD(1, 28)
#define DSCL_EXT_COEF_FLD_EXTC_BANK                            REG_FLD(6, 20)
#define DSCL_EXT_COEF_FLD_EXTC_TAP                             REG_FLD(3, 16)
#define DSCL_EXT_COEF_FLD_EXTC_COEFF                           REG_FLD(11, 0)

#define DSCL_PEAK_CFG_FLD_HPK_BP_GAIN                          REG_FLD(4, 8)
#define DSCL_PEAK_CFG_FLD_HPK_HP_GAIN                          REG_FLD(4, 4)
#define DSCL_PEAK_CFG_FLD_VPK_HP_GAIN                          REG_FLD(4, 0)



#endif
