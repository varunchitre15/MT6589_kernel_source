#ifndef _DDP_RDMA_API_H_
#define _DDP_RDMA_API_H_


#define RDMA_INSTANCES  2
#define RDMA_MAX_WIDTH  2047
#define RDMA_MAX_HEIGHT 2047

enum RDMA_INPUT_FORMAT {
    RDMA_INPUT_FORMAT_YUYV   = 0,
    RDMA_INPUT_FORMAT_UYVY   = 1,
    RDMA_INPUT_FORMAT_YVYU   = 2,
    RDMA_INPUT_FORMAT_VYUY   = 3,
    RDMA_INPUT_FORMAT_RGB565 = 4,
    RDMA_INPUT_FORMAT_RGB888 = 8,
    RDMA_INPUT_FORMAT_ARGB   = 16,
};

enum RDMA_OUTPUT_FORMAT {
    RDMA_OUTPUT_FORMAT_ARGB   = 0,
    RDMA_OUTPUT_FORMAT_YUV444 = 1,
};

enum RDMA_MODE {
    RDMA_MODE_DIRECT_LINK = 0,
    RDMA_MODE_MEMORY      = 1,
};

typedef struct _RDMA_CONFIG_STRUCT
{
    unsigned idx;            // instance index
    enum RDMA_MODE mode;          // data mode
    enum RDMA_INPUT_FORMAT inputFormat;
    unsigned address;
    unsigned pitch;
    bool isByteSwap;
    enum RDMA_OUTPUT_FORMAT outputFormat;
    unsigned width; 
    unsigned height; 
    bool isRGBSwap;
}RDMA_CONFIG_STRUCT;


// start module
int RDMAStart(unsigned idx);

// stop module
int RDMAStop(unsigned idx);

// reset module
int RDMAReset(unsigned idx);

// configu module
int RDMAConfig(unsigned idx,
                    enum RDMA_MODE mode,
                    enum RDMA_INPUT_FORMAT inputFormat, 
                    unsigned address, 
                    enum RDMA_OUTPUT_FORMAT outputFormat, 
                    unsigned pitch,
                    unsigned width, 
                    unsigned height, 
                    bool isByteSwap, // input setting
                    bool isRGBSwap); // ourput setting

void RDMAWait(unsigned idx);

void RDMASetTargetLine(unsigned int idx, unsigned int line);

//---------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------
#define INT_ENABLE_FLD_FIFO_UNDERFLOW_INT_EN                   REG_FLD(1, 4)
#define INT_ENABLE_FLD_EOF_ABNORMAL_INT_EN                     REG_FLD(1, 3)
#define INT_ENABLE_FLD_FRAME_END_INT_EN                        REG_FLD(1, 2)
#define INT_ENABLE_FLD_FRAME_START_INT_EN                      REG_FLD(1, 1)
#define INT_ENABLE_FLD_REG_UPDATE_INT_EN                       REG_FLD(1, 0)

#define INT_STATUS_FLD_FIFO_UNDERFLOW_INT_FLAG                 REG_FLD(1, 4)
#define INT_STATUS_FLD_EOF_ABNORMAL_INT_FLAG                   REG_FLD(1, 3)
#define INT_STATUS_FLD_FRAME_END_INT_FLAG                      REG_FLD(1, 2)
#define INT_STATUS_FLD_FRAME_START_INT_FLAG                    REG_FLD(1, 1)
#define INT_STATUS_FLD_REG_UPDATE_INT_FLAG                     REG_FLD(1, 0)

#define GLOBAL_CON_FLD_SOFT_RESET                              REG_FLD(1, 4)
#define GLOBAL_CON_FLD_MODE_SEL                                REG_FLD(1, 1)
#define GLOBAL_CON_FLD_ENGINE_EN                               REG_FLD(1, 0)

#define SIZE_CON_0_FLD_OUTPUT_RGB_SWAP                           REG_FLD(1, 31)
#define SIZE_CON_0_FLD_INPUT_BYTE_SWAP                           REG_FLD(1, 30)
#define SIZE_CON_0_FLD_OUTPUT_FORMAT                             REG_FLD(1, 29)
#define SIZE_CON_0_FLD_OUTPUT_FRAME_WIDTH                        REG_FLD(12, 0)

#define SIZE_CON_1_FLD_OUTPUT_FRAME_HEIGHT                        REG_FLD(20, 0)

//#define MEM_START_TRIG_FLD_MEM_MODE_START_TRIG                 REG_FLD(1, 0)

#define MEM_CON_FLD_MEM_MODE_HORI_BLOCK_NUM                    REG_FLD(8, 24)
#define MEM_CON_FLD_MEM_MODE_INPUT_FORMAT                      REG_FLD(5, 4)
#define MEM_CON_FLD_MEM_MODE_TILE_EN                           REG_FLD(1, 0)

#define MEM_START_ADDR_FLD_MEM_MODE_START_ADDR                 REG_FLD(32, 0)

#define MEM_SRC_PITCH_FLD_MEM_MODE_SRC_PITCH                   REG_FLD(16, 0)

#define MEM_GMC_SETTING_0_FLD_ULTRA_THRESHOLD_LOW              REG_FLD(8, 0)
#define MEM_GMC_SETTING_0_FLD_PRE_ULTRA_THRESHOLD_LOW          REG_FLD(8, 8)
#define MEM_GMC_SETTING_0_FLD_ULTRA_THRESHOLD_HIGH_OFS         REG_FLD(8, 16)
#define MEM_GMC_SETTING_0_FLD_PRE_ULTRA_THRESHOLD_HIGH_OFS     REG_FLD(8, 24)

#define MEM_GMC_SETTING_1_FLD_ISSUE_REQ_THRESHOLD              REG_FLD(32, 0)

#define MEM_SLOW_CON_FLD_MEM_MODE_SLOW_COUNT                   REG_FLD(16, 16)
#define MEM_SLOW_CON_FLD_MEM_MODE_SLOW_EN                      REG_FLD(1, 0)

#define FIFO_CON_FLD_FIFO_UNDERFLOW_EN                         REG_FLD(1, 31)
#define FIFO_CON_FLD_FIFO_SIZE                                 REG_FLD(10, 16)
#define FIFO_CON_FLD_OUTPUT_VALID_FIFO_THRESHOLD               REG_FLD(10, 0)

#define CF_FLD                                                 REG_FLD(13, 0)
#define CF_ADD_FLD                                             REG_FLD(9, 0)


#endif
