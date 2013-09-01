#ifndef __ION_DRV_H__
#define __ION_DRV_H__
#include <linux/ion.h>
#include "ion_debugger.h"
// Structure definitions
typedef enum
{
    ION_HEAP_IDX_SYSTEM_CONTIG = 0,
    ION_HEAP_IDX_MULTIMEDIA,
    ION_HEAP_IDX_MAX
} ION_HEAP_IDX;

typedef enum 
{
    ION_CMD_SYSTEM,
    ION_CMD_MULTIMEDIA
} ION_CMDS;

typedef enum
{
    ION_MM_CONFIG_BUFFER,
} ION_MM_CMDS;

typedef enum
{
    ION_SYS_CACHE_SYNC,
    ION_SYS_GET_PHYS,
    ION_SYS_GET_CLIENT,
    ION_SYS_RECORD //define for ion
} ION_SYS_CMDS;

typedef enum
{
    ION_CACHE_CLEAN_BY_RANGE,
    ION_CACHE_INVALID_BY_RANGE,
    ION_CACHE_FLUSH_BY_RANGE,
    ION_CACHE_CLEAN_ALL,
    ION_CACHE_INVALID_ALL,
    ION_CACHE_FLUSH_ALL
} ION_CACHE_SYNC_TYPE;

typedef enum
{
    ION_ERROR_CONFIG_LOCKED = 0x10000
} ION_ERROR_E;

typedef struct ion_sys_cache_sync_param
{
    struct ion_handle* handle;
    ION_CACHE_SYNC_TYPE sync_type;
} ion_sys_cache_sync_param_t;

typedef struct ion_sys_get_phys_param
{
    struct ion_handle* handle;
    unsigned int phy_addr;
    unsigned int len;
} ion_sys_get_phys_param_t;

typedef struct ion_sys_get_client_param
{
    unsigned int client;
} ion_sys_get_client_param_t;

typedef struct ion_sys_data
{
    ION_SYS_CMDS sys_cmd;
    union
    {
        ion_sys_cache_sync_param_t cache_sync_param;
        ion_sys_get_phys_param_t   get_phys_param;
        ion_sys_get_client_param_t get_client_param;
	ion_sys_record_t record_param;
    };
} ion_sys_data_t;

typedef struct ion_mm_config_buffer_param
{
    struct ion_handle* handle;
    int eModuleID;
    unsigned int security;
    unsigned int coherent;
} ion_mm_config_buffer_param_t;

typedef struct ion_mm_data
{
    ION_MM_CMDS mm_cmd;
    union
    {
        ion_mm_config_buffer_param_t config_buffer_param;
    };
} ion_mm_data_t;

#ifdef __KERNEL__
// Exported global variables
extern struct ion_device *g_ion_device;

// Exported functions
long ion_kernel_ioctl(struct ion_client *client, unsigned int cmd, unsigned long arg);
#endif

#endif
