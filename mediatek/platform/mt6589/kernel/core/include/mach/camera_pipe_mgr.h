
//-----------------------------------------------------------------------------
#ifndef CAMERA_PIPE_MGR_H
#define CAMERA_PIPE_MGR_H
//-----------------------------------------------------------------------------
#define CAM_PIPE_MGR_DEV_NAME       "camera-pipemgr"
#define CAM_PIPE_MGR_MAGIC_NO       'p'
//-----------------------------------------------------------------------------
#define CAM_PIPE_MGR_PIPE_MASK_CAM_IO       ((unsigned long)1 << 0)
#define CAM_PIPE_MGR_PIPE_MASK_POST_PROC    ((unsigned long)1 << 1)
#define CAM_PIPE_MGR_PIPE_MASK_CDP_CAM      ((unsigned long)1 << 2)
#define CAM_PIPE_MGR_PIPE_MASK_CDP_CONCUR   ((unsigned long)1 << 3)
#define CAM_PIPE_MGR_PIPE_MASK_CDP_LINK     ((unsigned long)1 << 4)
//-----------------------------------------------------------------------------
typedef enum
{
    CAM_PIPE_MGR_SCEN_SW_NONE,
    CAM_PIPE_MGR_SCEN_SW_CAM_IDLE,
    CAM_PIPE_MGR_SCEN_SW_CAM_PRV,
    CAM_PIPE_MGR_SCEN_SW_CAM_CAP,
    CAM_PIPE_MGR_SCEN_SW_VIDEO_PRV,
    CAM_PIPE_MGR_SCEN_SW_VIDEO_REC,
    CAM_PIPE_MGR_SCEN_SW_VIDEO_VSS,
    CAM_PIPE_MGR_SCEN_SW_ZSD,
    CAM_PIPE_MGR_SCEN_SW_N3D,
}CAM_PIPE_MGR_SCEN_SW_ENUM;
//
typedef enum
{
    CAM_PIPE_MGR_SCEN_HW_NONE,
    CAM_PIPE_MGR_SCEN_HW_IC,
    CAM_PIPE_MGR_SCEN_HW_VR,
    CAM_PIPE_MGR_SCEN_HW_ZSD,
    CAM_PIPE_MGR_SCEN_HW_IP,
    CAM_PIPE_MGR_SCEN_HW_N3D,
    CAM_PIPE_MGR_SCEN_HW_VSS
}CAM_PIPE_MGR_SCEN_HW_ENUM;
//
typedef enum
{
    CAM_PIPE_MGR_DEV_CAM,
    CAM_PIPE_MGR_DEV_ATV,
    CAM_PIPE_MGR_DEV_VT
}CAM_PIPE_MGR_DEV_ENUM;
//
typedef struct
{
    unsigned long   PipeMask;
    unsigned long   Timeout;
}CAM_PIPE_MGR_LOCK_STRUCT;
//
typedef struct
{
    unsigned long   PipeMask;
}CAM_PIPE_MGR_UNLOCK_STRUCT;
//
typedef struct
{
    CAM_PIPE_MGR_SCEN_SW_ENUM   ScenSw;
    CAM_PIPE_MGR_SCEN_HW_ENUM   ScenHw;
    CAM_PIPE_MGR_DEV_ENUM       Dev;
}CAM_PIPE_MGR_MODE_STRUCT;
//
typedef struct
{
    unsigned long   PipeMask;
}CAM_PIPE_MGR_ENABLE_STRUCT;
//
typedef struct
{
    unsigned long   PipeMask;
}CAM_PIPE_MGR_DISABLE_STRUCT;
//-----------------------------------------------------------------------------
typedef enum
{
    CAM_PIPE_MGR_CMD_LOCK,
    CAM_PIPE_MGR_CMD_UNLOCK,
    CAM_PIPE_MGR_CMD_DUMP,
    CAM_PIPE_MGR_CMD_SET_MODE,
    CAM_PIPE_MGR_CMD_GET_MODE,
    CAM_PIPE_MGR_CMD_ENABLE_PIPE,
    CAM_PIPE_MGR_CMD_DISABLE_PIPE
}CAM_PIPE_MGR_CMD_ENUM;
//-----------------------------------------------------------------------------
#define CAM_PIPE_MGR_LOCK           _IOW(   CAM_PIPE_MGR_MAGIC_NO,  CAM_PIPE_MGR_CMD_LOCK,          CAM_PIPE_MGR_LOCK_STRUCT)
#define CAM_PIPE_MGR_UNLOCK         _IOW(   CAM_PIPE_MGR_MAGIC_NO,  CAM_PIPE_MGR_CMD_UNLOCK,        CAM_PIPE_MGR_UNLOCK_STRUCT)
#define CAM_PIPE_MGR_DUMP           _IO(    CAM_PIPE_MGR_MAGIC_NO,  CAM_PIPE_MGR_CMD_DUMP)
#define CAM_PIPE_MGR_SET_MODE       _IOW(   CAM_PIPE_MGR_MAGIC_NO,  CAM_PIPE_MGR_CMD_SET_MODE,      CAM_PIPE_MGR_MODE_STRUCT)
#define CAM_PIPE_MGR_GET_MODE       _IOW(   CAM_PIPE_MGR_MAGIC_NO,  CAM_PIPE_MGR_CMD_GET_MODE,      CAM_PIPE_MGR_MODE_STRUCT)
#define CAM_PIPE_MGR_ENABLE_PIPE    _IOW(   CAM_PIPE_MGR_MAGIC_NO,  CAM_PIPE_MGR_CMD_ENABLE_PIPE,   CAM_PIPE_MGR_ENABLE_STRUCT)
#define CAM_PIPE_MGR_DISABLE_PIPE   _IOW(   CAM_PIPE_MGR_MAGIC_NO,  CAM_PIPE_MGR_CMD_DISABLE_PIPE,  CAM_PIPE_MGR_DISABLE_STRUCT)
//-----------------------------------------------------------------------------
#endif
//-----------------------------------------------------------------------------

