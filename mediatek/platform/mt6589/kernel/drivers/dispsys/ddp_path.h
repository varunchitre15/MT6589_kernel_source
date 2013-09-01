#ifndef __DDP_PATH_H__
#define __DDP_PATH_H__

#include "ddp_ovl.h"
#include "ddp_rdma.h"
#include "ddp_wdma.h"
#include "ddp_bls.h"
#include "ddp_drv.h"
#include "ddp_scl.h"
#include "ddp_rot.h"

#define DDP_USE_CLOCK_API
#define DDP_OVL_LAYER_MUN 4
#define DDP_BACKUP_REG_NUM 0x1000
#define DDP_UNBACKED_REG_MEM 0xdeadbeef

struct disp_path_config_struct
{
    DISP_MODULE_ENUM srcModule;

	// if srcModule=RDMA0, set following value, else do not have to set following value
    unsigned int addr; 
    unsigned int inFormat; 
    unsigned int pitch;
    struct DISP_REGION srcROI;        // ROI

    OVL_CONFIG_STRUCT ovl_config;

    struct DISP_REGION bgROI;         // background ROI
    unsigned int bgColor;  // background color

    DISP_MODULE_ENUM dstModule;
    unsigned int outFormat; 
    unsigned int dstAddr;  // only take effect when dstModule=DISP_MODULE_WDMA0 or DISP_MODULE_WDMA1

    int srcWidth, srcHeight;
    int dstWidth, dstHeight;
    int dstPitch;
};

int disp_wait_timeout(bool flag, unsigned int timeout);
struct disp_path_config_mem_out_struct
{
    unsigned int enable;
    unsigned int dirty;
	unsigned int outFormat; 
    unsigned int dstAddr;
    struct DISP_REGION srcROI;        // ROI
};

int disp_path_config(struct disp_path_config_struct* pConfig);
int disp_path_config_layer(OVL_CONFIG_STRUCT* pOvlConfig);
int disp_path_config_layer_addr(unsigned int layer, unsigned int addr);
int disp_path_get_mutex(void);
int disp_path_release_mutex(void);
int disp_path_wait_reg_update(void);

int disp_path_get_mutex_(int mutexId);
int disp_path_release_mutex_(int mutexId);
int disp_path_config_(struct disp_path_config_struct* pConfig, int mutexId);

int disp_path_config_mem_out(struct disp_path_config_mem_out_struct* pConfig);
int disp_path_config_mem_out_without_lcd(struct disp_path_config_mem_out_struct* pConfig);
void disp_path_wait_mem_out_done(void);

#ifdef DDP_USE_CLOCK_API
int disp_reg_backup(void);
int disp_reg_restore(void);

int disp_path_clock_on(char* name);
int disp_path_clock_off(char* name);

// only called by assert layer
int disp_path_change_tdshp_status(unsigned int layer, unsigned int enable);

#endif
#endif
