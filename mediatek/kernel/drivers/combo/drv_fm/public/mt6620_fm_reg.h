#ifndef __MT6620_FM_REG_H__
#define __MT6620_FM_REG_H__

//private chip define
#define MT6620_FM_PGSEL         (0x9F)
#define MT6620_FM_RSSI_IND      (0xE8)
#define MT6620_FM_PAMD_IND      (0xE9)
#define MT6620_FM_MR_IND        (0xF2)
#define MT6620_FM_STEROMONO_IND (0xF8)
#define MT6620_FM_STEROMONO_CTR (0xE0)

//common define 
#define FM_PGSEL        MT6620_FM_PGSEL
#define FM_RSSI_IND     MT6620_FM_RSSI_IND
#define FM_PAMD_IND     MT6620_FM_PAMD_IND
#define FM_MR_IND       MT6620_FM_MR_IND
#define FM_STEROMONO_IND MT6620_FM_STEROMONO_IND
#define FM_STEROMONO_CTR MT6620_FM_STEROMONO_CTR

//FM reg page
enum FM_PAGE{
    FM_PG0 = 0,
    FM_PG1,
    FM_PG2,
    FM_PG3,
    FM_PGMAX
};

#endif //__MT6620_FM_REG_H__

