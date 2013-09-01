
#ifndef _CAMERA_CUSTOM_EEPROM_H_
#define _CAMERA_CUSTOM_EEPROM_H_

typedef unsigned int        MUINT32;
typedef unsigned short      MUINT16;

typedef signed int          MINT32;
typedef signed short        MINT16;
typedef unsigned char   MUINT8;


#define MAX_DEFECT_PRV_SIZE             (256)  //(INT32)
#define MAX_DEFECT_CAP_SIZE             (2048)  //5M sensor BPC ratio 0.1% = 5000 pixel (INT32)
#define MAX_SHADING_Capture_SIZE        (32*32*16)//seanlin 120919 6589 (4096) //INT32
#define MAX_SHADING_Preview_SIZE        (16*16*16)//seanlin 120919  (1600) //INT32
#define MAX_SHADING_Video_SIZE          (16*16*16)//seanlin 120919  (1600) //INT32
#define MAX_EEPROM_SENSOR_CAL_SIZE             (1024) //Byte

typedef enum
{
    CAMERA_EEPROM_DATA_SHADING_TABLE=0,
    CAMERA_EEPROM_DATA_DEFECT_TABLE,
    CAMERA_EEPROM_DATA_PREGAIN,
    CAMERA_EEPROM_TYPE_NUM
} CAMERA_EEPROM_TYPE_ENUM;


#define CAMERA_EEPROM_CMD_SHADING_TABLE	0x00000001
#define CAMERA_EEPROM_CMD_DEFECT_TABLE		0x00000010
#define CAMERA_EEPROM_CMD_PREGAIN			0x00000100

#define EEPROM_ERR_NO_DEFECT 0x00000001
#define EEPROM_ERR_NO_PREGAIN 0x00000010
#define EEPROM_ERR_NO_SHADING 0x00000100
#define EEPROM_ERR_NO_DATA (EEPROM_ERR_NO_DEFECT|EEPROM_ERR_NO_PREGAIN|EEPROM_ERR_NO_SHADING)
#define EEPROM_ERR_SENSOR_SHADING 0x00001000
#define EEPROM_ERR_NOT_WRITENVRAM 0x00010000



#define EEPROM_LSC_DATA_SIZE_SLIM_LSC1		144


#define EEPROM_INFO_IN_COMM_LOAD 			34
#define EEPROM_INFO_IN_COMM_SHADING_TYPE	33
#define EEPROM_INFO_IN_COMM_PARTNO_1		35
#define EEPROM_INFO_IN_COMM_PARTNO_2		36
#define EEPROM_INFO_IN_COMM_PARTNO_3		37
#define EEPROM_INFO_IN_COMM_PARTNO_4		38
#define EEPROM_INFO_IN_COMM_PARTNO_5		39


#define EEPROM_INFO_IN_PV_WIDTH				40
#define EEPROM_INFO_IN_PV_HEIGHT			41
#define EEPROM_INFO_IN_CAP_WIDTH			42
#define EEPROM_INFO_IN_CAP_HEIGHT			43


#define EEPROM_PART_NUMBERS_COUNT 5
#define EEPROM_SENSOR_RESOLUTION_COUNT 5

//SeanLin@20110629>
#define CUSTOM_EEPROM_ROTATION_0_DEGREE 0
#define CUSTOM_EEPROM_ROTATION_180_DEGREE 1

#define CUSTOM_EEPROM_COLOR_SHIFT_00 0
#define CUSTOM_EEPROM_COLOR_SHIFT_01 1
#define CUSTOM_EEPROM_COLOR_SHIFT_10 2
#define CUSTOM_EEPROM_COLOR_SHIFT_11 3
//SeanLin@20110629<

const MUINT32 gulPartNumberReg[EEPROM_PART_NUMBERS_COUNT] = {EEPROM_INFO_IN_COMM_PARTNO_1,
                                                       EEPROM_INFO_IN_COMM_PARTNO_2,
                                                       EEPROM_INFO_IN_COMM_PARTNO_3,
                                                       EEPROM_INFO_IN_COMM_PARTNO_4,
                                                       EEPROM_INFO_IN_COMM_PARTNO_5};

const MUINT32 gulSensorResol[EEPROM_SENSOR_RESOLUTION_COUNT] = {EEPROM_INFO_IN_PV_WIDTH,
                                                       EEPROM_INFO_IN_PV_HEIGHT,
                                                       EEPROM_INFO_IN_CAP_WIDTH,
                                                       EEPROM_INFO_IN_CAP_HEIGHT};
typedef struct
{
    MUINT32 CommReg[64];// 0~30 reserve for compatiblilty with WCP1 setting;
                                      // [31] for table defect control 0x0154h
                                      // [32] for defect table address 0x0158h
} EEPROM_NVRAM_COMMON_STRUCT, *PEEPROM_NVRAM_COMMON_STRUCT;




typedef struct
{
    MUINT32 shading_ctrl1;
    MUINT32 shading_ctrl2;
    MUINT32 shading_ctrl3;
    MUINT32 shading_lblock;
    MUINT32 shading_ratio;
    MUINT32 shading_gain_th;
}EEPROM_SHADING_REG_STRUCT, *PEEPROM_SHADING_REG_STRUCT;

typedef struct
{


    MUINT16 PreviewSize;
    MUINT16 CaptureSize;
    MUINT32 PreviewTable[MAX_DEFECT_PRV_SIZE];
    MUINT32 CaptureTable1[MAX_DEFECT_CAP_SIZE];
    EEPROM_NVRAM_COMMON_STRUCT     ISPComm;
} EEPROM_DEFECT_STRUCT, *PEEPROM_DEFECT_STRUCT;
#if 0
typedef union {
    enum { COUNT = 5 };
    MUINT32 set[COUNT];
    struct {
        ISP_NVRAM_RAW_ACC_CFG_T         raw_acc_cfg;
        ISP_NVRAM_RAW_ACC_WIN_T         raw_acc_win;
        ISP_NVRAM_RAW_ACC_RESULT_T      raw_acc_result_b;
        ISP_NVRAM_RAW_ACC_RESULT_T      raw_acc_result_gb;
        ISP_NVRAM_RAW_ACC_RESULT_T      raw_acc_result_gr;
        ISP_NVRAM_RAW_ACC_RESULT_T      raw_acc_result_r;
        ISP_NVRAM_SHADING_CTRL1_T       shading_ctrl1;
        ISP_NVRAM_SHADING_CTRL2_T       shading_ctrl2;
        ISP_NVRAM_SHADING_READ_ADDR_T   shading_read_addr;
        ISP_NVRAM_SHADING_LAST_BLK_T    shading_last_blk;
        ISP_NVRAM_SHADING_RATIO_CFG_T   shading_ratio_cfg;
    };
} ISP_NVRAM_SHADING_T;
#endif



typedef struct
{
    EEPROM_SHADING_REG_STRUCT PreRegister;
    EEPROM_SHADING_REG_STRUCT CapRegister;
    EEPROM_NVRAM_COMMON_STRUCT     ISPComm;
    MUINT32 ShadingID;   //To let drv know the table convert
    MUINT32 TableRotation;   //1:180, 0:0 To let drv know the table convert
    MUINT32 ColorOrder;       //SeanLin@20110629:To Cover Sensor color order changing
    MUINT16 PreviewSize;
    MUINT16 CaptureSize;
    MUINT32 PreviewTable[3][ MAX_SHADING_Preview_SIZE];
    MUINT32 CaptureTable[3][ MAX_SHADING_Capture_SIZE];
    MUINT8 SensorCalTable[MAX_EEPROM_SENSOR_CAL_SIZE];
}EEPROM_SHADING_STRUCT, *PEEPROM_SHADING_STRUCT;


typedef struct
{
    MUINT32 rFacGainu4R;
    MUINT32 rFacGainu4G;
    MUINT32 rFacGainu4B;
    MUINT32 rCalGainu4R;
    MUINT32 rCalGainu4G;
    MUINT32 rCalGainu4B;
}EEPROM_PREGAIN_STRUCT, *PEEPROM_PREGAIN_STRUCT;


typedef struct
{
    EEPROM_SHADING_STRUCT Shading;
    EEPROM_DEFECT_STRUCT  Defect;
    EEPROM_PREGAIN_STRUCT Pregain;
}EEPROM_DATA_STRUCT, *PEEPROM_DATA_STRUCT;

typedef enum
{
    EEPROM_NONE = 0,
    EEPROM_USED
} EEPROM_TYPE_ENUM;

EEPROM_TYPE_ENUM EEPROMInit(void);
unsigned int EEPROMDeviceName(char* DevName);


#endif //_CAMERA_CUSTOM_EEPROM_H_
