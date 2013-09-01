
/* Basic */
#define   e3135_NAME                  "ektf -3135"
#define   e3135_I2C_SLAVE_ADDR        0x20
#define   e3135_I2C_Channel           0x02
#define   ELAN_TPD_DEVICE             TPD_DEVICE
#define   PACKET_SIZE                 18
#define   E3135_BUFSIZE             256
#define   FINGER_NUM                  5
#define   IDX_NUM                     0x01
#define   IDX_FINGER                  0x02
#define   FINGER_ID                   0x0001 
#define   KEY_ID_INDEX                17
 
/* Bootup Mode */
#define   tpd_normal                  0x00
int   tpd_recovery = 0x80;

/* Data Info */
int CTP_ID = 0;
int CTP_VER = 0;
int CTP_X_Res = 0;
int CTP_Y_Res = 0;
int CTP_PW_Status = 0;

    typedef enum  elan2136_iap_op
    {
      Page_Size   = 132,
      Page_Num    = 249,
      ACK_FAIL    = 0x00,
      ACK_OK      = 0xAA,
      ACK_REWRITE = 0x55,
    };
 enum
    {
      E_FD = -1,
    };
 #define   PAGERETRY                   10

/* CMD */
typedef enum 
{
  CMD_ID,
  CMD_VER,
  CMD_X_Res,
  CMD_Y_Res,
  CMD_PW,
  CMD_PW_ON,
  CMD_PW_OFF,
  CMD_All
}Elan_CMD;

/* Power */
typedef enum 
{
  Off,
  On
}Elan_PW;

#define   E3135_HWPWM_NAME          "EKTF-3135"
#define   PWR_STATE_DEEP_SLEEP      0
#define   PWR_STATE_NORMAL          1
#define   PWR_STATE_MASK            0x8//  BIT(3)

/* Packet Type */
#define   CMD_S_PKT                 0x52
#define   CMD_R_PKT                 0x53
#define   CMD_W_PKT                 0x54
#define   HELLO_PKT                 0x55

#define   NORMAL_PKT                0x5D    /** 2 Fingers: 5A 5 Fingers 5D, 10 Fingers: 62 **/
#define   FIVE_FINGERS_PKT          0x6D

#define   RPT_LOCK_PKT              0x56
#define   RPT_UNLOCK_PKT            0xA6

#define   RESET_PKT                 0x77
#define   CALIB_PKT                 0xA8

/* Button Type */
#define   EKTF3135_KEY_PRESS        0x01
#define   EKTF3135_KEY_RELEASE      0x00
#define   EKTF3135_KEY_BACK         0x20
#define   EKTF3135_KEY_HOME         0x40
#define   EKTF3135_KEY_MENU         0x80
#define   EKTF3135_KEY_SEARCH       0x10

int button = EKTF3135_KEY_RELEASE;

/* LCM Resolution */
int panel_size = 50;

/* Elan IAP */
#define Elan_IAP
int IAP_PW_Lock = 0;

#define ELAN_IOCTLID	0xD0
#define IOCTL_I2C_SLAVE		 _IOW(ELAN_IOCTLID,  1, int)
#define IOCTL_MAJOR_FW_VER  _IOR(ELAN_IOCTLID, 2, int)
#define IOCTL_MINOR_FW_VER  _IOR(ELAN_IOCTLID, 3, int)
#define IOCTL_RESET  _IOR(ELAN_IOCTLID, 4, int)
#define IOCTL_IAP_MODE_LOCK  _IOR(ELAN_IOCTLID, 5, int)
#define IOCTL_CHECK_RECOVERY_MODE  _IOR(ELAN_IOCTLID, 6, int)
#define IOCTL_FW_VER  _IOR(ELAN_IOCTLID, 7, int)
#define IOCTL_X_RESOLUTION  _IOR(ELAN_IOCTLID, 8, int)
#define IOCTL_Y_RESOLUTION  _IOR(ELAN_IOCTLID, 9, int)
#define IOCTL_FW_ID  _IOR(ELAN_IOCTLID, 10, int)
#define IOCTL_ROUGH_CALIBRATE  _IOR(ELAN_IOCTLID, 11, int)
#define IOCTL_IAP_MODE_UNLOCK  _IOR(ELAN_IOCTLID, 12, int)
#define IOCTL_I2C_INT  _IOR(ELAN_IOCTLID, 13, int)
#define IOCTL_RESUME  _IOR(ELAN_IOCTLID, 14, int)
#define IOCTL_POWER_LOCK  _IOR(ELAN_IOCTLID, 15, int)
#define IOCTL_POWER_UNLOCK  _IOR(ELAN_IOCTLID, 16, int)
#define IOCTL_FW_UPDATE  _IOR(ELAN_IOCTLID, 17, int)
#define IOCTL_2WIREICE  _IOR(ELAN_IOCTLID, 19, int)
#define IOCTL_FW_Info  _IOR(ELAN_IOCTLID, 101, int)

