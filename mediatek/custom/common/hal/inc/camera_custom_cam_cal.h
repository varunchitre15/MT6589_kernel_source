#ifndef _CAMERA_CUSTOM_CAM_CAL_H_
#define _CAMERA_CUSTOM_CAM_CAL_H_
//common folder
typedef unsigned int       MUINT32;
typedef unsigned short   MUINT16;
typedef unsigned char    MUINT8;
typedef signed int          MINT32;
typedef signed short      MINT16;
typedef signed char       MINT8;


//121016 for 658x #define MAX_DEFECT_PRV_SIZE     (256)  //(INT32)
//121016 for 658x #define MAX_DEFECT_CAP_SIZE     (2048)  //5M sensor BPC ratio 0.1% = 5000 pixel (INT32)
//121016 for 658x #define MAX_SHADING_Capture_SIZE        (32*32*16)//seanlin 120919 6589 (4096) //INT32 
//121016 for 658x #define MAX_SHADING_Preview_SIZE        (16*16*16)//seanlin 120919  (1600) //INT32 
//121016 for 658x #define MAX_SHADING_Video_SIZE            (16*16*16)//seanlin 120919  (1600) //INT32 

/*****************************************************************************
//for cam_cal_drv.h 	
virtual int GetCamCalCalData(unsigned long a_eSensorType,
                          unsigned long u4SensorID,
                          CAMERA_CAM_CAL_TYPE_ENUM a_eCamCalDataType,
	                      void *a_pCamCalData) = 0;
*****************************************************************************/
typedef enum
{
    CAMERA_CAM_CAL_DATA_MODULE_VERSION=0,            //seanlin 121016 it's for user to get info. of single module or N3D module    
    CAMERA_CAM_CAL_DATA_PART_NUMBER,                      //seanlin 121016 return 5x4 byes gulPartNumberRegCamCal[5]
    CAMERA_CAM_CAL_DATA_SHADING_TABLE,                  //seanlin 121016 return SingleLsc or N3DLsc
    CAMERA_CAM_CAL_DATA_3A_GAIN,                              //seanlin 121016 return Single2A or N3D3A
    CAMERA_CAM_CAL_DATA_3D_GEO,                               //seanlin 121016 return none or N3D3D 
    CAMERA_CAM_CAL_DATA_LIST
}CAMERA_CAM_CAL_TYPE_ENUM; //CAMERA_CAM_CAL_TYPE_ENUM;


//121016 for 658x     #define CAMERA_CAM_CAL_CMD_SHADING_TABLE	0x00000001
//121016 for 658x     #define CAMERA_CAM_CAL_CMD_DEFECT_TABLE		0x00000010
//121016 for 658x     #define CAMERA_CAM_CAL_CMD_PREGAIN			0x00000100

//121016 for 658x     #define CAM_CAL_ERR_NO_DEFECT 0x00000001
#if 0
#define CAM_CAL_ERR_NO_ERR          0x00000000
#define CAM_CAL_ERR_NO_SHADING 0x00000001
#define CAM_CAL_ERR_NO_3A_GAIN 0x00000010
#define CAM_CAL_ERR_NO_3D_GEO   0x00000100
#define CAM_CAL_ERR_NO_VERSION  0x00001000
#define CAM_CAL_ERR_NO_PARTNO   0x00010000
#else
#define CAM_CAL_ERR_NO_ERR          0x00000000
#define CAM_CAL_ERR_NO_VERSION  0x00000001
#define CAM_CAL_ERR_NO_PARTNO   0x00000010
#define CAM_CAL_ERR_NO_SHADING 0x00000100
#define CAM_CAL_ERR_NO_3A_GAIN 0x00001000
#define CAM_CAL_ERR_NO_3D_GEO   0x00010000
#endif
#define CAM_CAL_ERR_NO_DEVICE   0x8FFFFFFF
#define CAM_CAL_ERR_NO_CMD      0x1FFFFFFF
#define CAM_CAL_ERR_NO_DATA (CAM_CAL_ERR_NO_SHADING|CAM_CAL_ERR_NO_3A_GAIN|CAM_CAL_ERR_NO_3D_GEO|CAM_CAL_ERR_NO_VERSION|CAM_CAL_ERR_NO_PARTNO)
//                                                      CAM_CAL_ERR_NO_3A_GAIN|
//                                                      CAM_CAL_ERR_NO_3D_GEO|
//                                                      CAM_CAL_ERR_NO_VERSION|
//                                                      CAM_CAL_ERR_NO_PARTNO)
                                                      

                                                      
//121016 for 658x     #define CAM_CAL_ERR_SENSOR_SHADING 0x00001000
//121016 for 658x     #define CAM_CAL_ERR_NOT_WRITENVRAM 0x00010000

#define CAM_CAL_LSC_DATA_SIZE_SLIM_LSC1_8x9		144	//(8x9x2)XMUINT32(4)
#define CAM_CAL_LSC_DATA_SIZE_SLIM_LSC1_16x16     512	 //(16x16x2)XMUINT32(4)

//121016 for 658x     #define CAM_CAL_INFO_IN_COMM_LOAD 			34
//121016 for 658x     #define CAM_CAL_INFO_IN_COMM_SHADING_TYPE	33
#define CAM_CAL_INFO_IN_COMM_PARTNO_1		35
#define CAM_CAL_INFO_IN_COMM_PARTNO_2		36
#define CAM_CAL_INFO_IN_COMM_PARTNO_3		37
#define CAM_CAL_INFO_IN_COMM_PARTNO_4		38
#define CAM_CAL_INFO_IN_COMM_PARTNO_5		39


//121016 for 658x     #define CAM_CAL_INFO_IN_PV_WIDTH				40
//121016 for 658x     #define CAM_CAL_INFO_IN_PV_HEIGHT		              41
//121016 for 658x     #define CAM_CAL_INFO_IN_CAP_WIDTH			42
//121016 for 658x     #define CAM_CAL_INFO_IN_CAP_HEIGHT			43


#define CAM_CAL_PART_NUMBERS_COUNT 6
#define LSC_DATA_BYTES 4
#define CAM_CAL_PART_NUMBERS_COUNT_BYTE (CAM_CAL_PART_NUMBERS_COUNT*LSC_DATA_BYTES)
//121016 for 658x     #define CAM_CAL_SENSOR_RESOLUTION_COUNT 5
#define CAM_CAL_SINGLE_AWB_COUNT_BYTE (0x08)
//SeanLin@20110629>
#define CUSTOM_CAM_CAL_ROTATION_0_DEGREE 0
#define CUSTOM_CAM_CAL_ROTATION_180_DEGREE 1

#define CUSTOM_CAM_CAL_COLOR_SHIFT_00 0 
#define CUSTOM_CAM_CAL_COLOR_SHIFT_01 1 
#define CUSTOM_CAM_CAL_COLOR_SHIFT_10 2 
#define CUSTOM_CAM_CAL_COLOR_SHIFT_11 3 
//SeanLin@20110629<  
/*
const MUINT32 gulPartNumberRegCamCal[CAM_CAL_PART_NUMBERS_COUNT] = {
                                                       CAM_CAL_INFO_IN_COMM_PARTNO_1,
                                                       CAM_CAL_INFO_IN_COMM_PARTNO_2, 
                                                       CAM_CAL_INFO_IN_COMM_PARTNO_3, 
                                                       CAM_CAL_INFO_IN_COMM_PARTNO_4, 
                                                       CAM_CAL_INFO_IN_COMM_PARTNO_5};
*/
const MUINT32 CamCalReturnErr[CAMERA_CAM_CAL_DATA_LIST]= {
                                                    CAM_CAL_ERR_NO_VERSION,
                                                    CAM_CAL_ERR_NO_PARTNO,
                                                    CAM_CAL_ERR_NO_SHADING,
                                                    CAM_CAL_ERR_NO_3A_GAIN,
                                                    CAM_CAL_ERR_NO_3D_GEO};

const char CamCalErrString[CAMERA_CAM_CAL_DATA_LIST][24]={
                                                  {"ERR_NO_VERSION"},
                                                  {"ERR_NO_PARTNO"},
                                                  {"ERR_NO_SHADING"},
                                                  {"ERR_NO_3A_GAIN"},
                                                  {"ERR_NO_3D_GEO"}
                                                  };

#if 0//121016 for 658x   
const MUINT32 gulSensorResolCamCal[CAM_CAL_SENSOR_RESOLUTION_COUNT] = {CAM_CAL_INFO_IN_PV_WIDTH,
                                                       CAM_CAL_INFO_IN_PV_HEIGHT, 
                                                       CAM_CAL_INFO_IN_CAP_WIDTH, 
                                                       CAM_CAL_INFO_IN_CAP_HEIGHT};
   
typedef struct
{
    MUINT32 CommReg[64];// 0~30 reserve for compatiblilty with WCP1 setting; 
                                      // [31] for table defect control 0x0154h
                                      // [32] for defect table address 0x0158h
} CAM_CAL_NVRAM_COMMON_STRUCT, *PCAM_CAL_NVRAM_COMMON_STRUCT;
#endif//121016 for 658x  
typedef struct
{
    MUINT32 shading_ctrl1;
    MUINT32 shading_ctrl2;
    MUINT32 shading_read_addr;
    MUINT32 shading_last_blk;
    MUINT32 shading_ratio_cfg;
}CAM_CAL_SHADING_REG_STRUCT, *PCAM_CAL_SHADING_REG_STRUCT;

#if 0//121016 for 658x 
typedef struct
{
    MUINT16 PreviewSize;
    MUINT16 CaptureSize;
    MUINT32 PreviewTable[MAX_DEFECT_PRV_SIZE];
    MUINT32 CaptureTable1[MAX_DEFECT_CAP_SIZE];    
    CAM_CAL_NVRAM_COMMON_STRUCT     ISPComm;        
} CAM_CAL_DEFECT_STRUCT, *PCAM_CAL_DEFECT_STRUCT;

typedef struct
{ 
    CAM_CAL_SHADING_REG_STRUCT PreRegister;
    CAM_CAL_SHADING_REG_STRUCT CapRegister;    
    CAM_CAL_NVRAM_COMMON_STRUCT     ISPComm;        
    MUINT32 ShadingID;   //To let drv know the table convert 
    MUINT32 TableRotation;   //1:180, 0:0 To let drv know the table convert 
    MUINT32 ColorOrder;       //SeanLin@20110629:To Cover Sensor color order changing
    MUINT16 PreviewSize;
    MUINT16 CaptureSize;
    MUINT32 PreviewTable[3][ MAX_SHADING_Preview_SIZE];
    MUINT32 CaptureTable[3][ MAX_SHADING_Capture_SIZE];    
    MUINT8 SensorCalTable[MAX_SENSOR_CAL_SIZE];  //658x 2048 from 1024 of 657x
}CAM_CAL_SHADING_STRUCT, *PCAM_CAL_SHADING_STRUCT;
#endif//121016 for 658x  

/****************************************************************/
//for SINGLE Module
/****************************************************************/
#define CAM_CAL_MAX_LSC_SIZE 0x840 //(2112) //6589 Ver

#define MAX_SENSOR_CAL_SIZE             (2048) //Byte
/****************************************************************/
//for CAM_CAL_SINGLE_LSC_STRUCT SingleLsc;
/****************************************************************/
#define CAM_CAL_SINGLE_LSC_SIZE 0x284 //(664)
#define MAX_MTK_SHADING_SLIM_TABLE_SIZE (2048)//(16*16*2*4)
#define MAX_SENSOR_SHADING_TALE_SIZE MAX_SENSOR_CAL_SIZE//(2048)//(16*16*2*4)

    typedef struct {
        MUINT8      MtkLscType; //LSC Table type	"[0]sensor[1]MTK"	1
        MUINT8      PixId; //0,1,2,3: B,Gb,Gr,R
        MUINT16    TableSize; //table size (2108 Bytes )		2 
        MUINT32    SlimLscType; //00A0	FF 00 02 01 (4 bytes)		4
        MUINT32    PreviewWH; //00A4	Preview Width (2 bytes)	Preview Height (2 bytes)	2
        MUINT32    PreviewOffSet; //00A6	Preview Offset X (2 bytes)	Preview Offset Y (2 bytes)	2
        MUINT32    CaptureWH; //00A8	Capture Width (2 bytes)	Capture Height (2 bytes)	2
        MUINT32    CaptureOffSet; //00AA	Capture Offset X (2 bytes)	Capture Offfset Y (2 bytes)	2
        MUINT32    PreviewTblSize; //00AC	Preview Shading Table Size (4 bytes)		4
        MUINT32    CaptureTblSize; //00B0	Capture Shading Table Size (4 bytes)		4
        MUINT32    PvIspReg[5]; //00B4	Preview Shading Register Setting (5x4 bytes)		20
        MUINT32    CapIspReg[5]; //00C8	Capture Shading Register Setting (5x4 bytes)		20       
        MUINT8      CapTable[MAX_MTK_SHADING_SLIM_TABLE_SIZE]; //00DC	Capture Shading Table (16 X 16 X 2 X 4 bytes)		2048        
    }CAM_CAL_LSC_MTK_TYPE;
    typedef struct {
        MUINT8      MtkLscType; //LSC Table type	"[0]sensor[1]MTK"	1
        MUINT8      PixId; //0,1,2,3: B,Gb,Gr,R
        MUINT16    TableSize; //TABLE SIZE		2
        MUINT8      SensorTable[MAX_SENSOR_SHADING_TALE_SIZE]; //LSC Data (Max 2048)		2048
        MUINT8      Reserve[CAM_CAL_MAX_LSC_SIZE-sizeof(MUINT8)-sizeof(MUINT8)-sizeof(MUINT16)-(sizeof(MUINT8)*MAX_SENSOR_SHADING_TALE_SIZE)]; //
    }CAM_CAL_LSC_SENSOR_TYPE;

typedef union {
        MUINT8 Data[CAM_CAL_MAX_LSC_SIZE];
        CAM_CAL_LSC_MTK_TYPE       MtkLcsData;
        CAM_CAL_LSC_SENSOR_TYPE SensorLcsData;
} CAM_CAL_LSC_DATA;

typedef struct
{    
//    MUINT32 ShadingID;                  //To let drv know the table convert 
    MUINT32 TableRotation;            //1 :180, 0:0 To let drv know the table convert , if mirror/Flip it's 2/4
//    MUINT32 ColorOrder;                 //SeanLin@20110629:To Cover Sensor color order changing
    CAM_CAL_LSC_DATA LscTable;    
}CAM_CAL_SINGLE_LSC_STRUCT, *PCAM_CAL_SINGLE_LSC_STRUCT;

/****************************************************************/
//for CAM_CAL_SINGLE_2A_STRUCT Single2A;
/****************************************************************/
typedef struct
{
    MUINT32 rGoldGainu4R;
    MUINT32 rGoldGainu4G;
    MUINT32 rGoldGainu4B;
    MUINT32 rUnitGainu4R;
    MUINT32 rUnitGainu4G;
    MUINT32 rUnitGainu4B; 
}CAM_CAL_PREGAIN_STRUCT, *PCAM_CAL_PREGAIN_STRUCT;

#define CAM_CAL_AWB_BITEN (0x01<<0)
#define CAM_CAL_AF_BITEN (0x01<<1)
#define CAM_CAL_NONE_BITEN (0x00)
typedef struct
{
    MUINT8 S2aVer; //verson : 01
    MUINT8 S2aBitEn; //bit enable: 03 Bit0: AF Bit1: WB    
    MUINT8 S2aAfBitflagEn; //Bit: step 0(inf.), 1(marco), 2, 3, 4,5,6,7
    MUINT16 S2aAf[8];      //0x012c
    CAM_CAL_PREGAIN_STRUCT S2aAwb; //0x012c
}CAM_CAL_SINGLE_2A_STRUCT, *PCAM_CAL_SINGLE_2A_STRUCT;



/****************************************************************/
//for N3D Module
/****************************************************************/

#define CAM_CAL_N3D_NUM 2 //6589 Ver
/****************************************************************/
//for CAM_CAL_N3D_3A_STRUCT N3D3A;
/****************************************************************/
#define CAM_CAL_N3D_3A_SIZE 128 //6589 Ver
typedef struct
{
    MUINT8 Data[CAM_CAL_N3D_NUM][CAM_CAL_N3D_3A_SIZE];
}CAM_CAL_N3D_3A_STRUCT, *PCAM_CAL_N3D_3A_STRUCT;


/****************************************************************/
//for CAM_CAL_N3D_3D_STRUCT N3D3D;    
/****************************************************************/
#define CAM_CAL_N3D_3D_SIZE 2200 //6589 Ver
typedef struct
{
    MUINT8 Data[CAM_CAL_N3D_NUM][CAM_CAL_N3D_3D_SIZE];
}CAM_CAL_N3D_3D_STRUCT, *PCAM_CAL_N3D_3D_STRUCT;

/****************************************************************/
//for CAM_CAL_N3D_LSC_STRUCT N3DLsc;
/****************************************************************/
#define CAM_CAL_N3D_LSC_SIZE CAM_CAL_MAX_LSC_SIZE
typedef struct
{
    //MUINT8 Data[CAM_CAL_N3D_NUM][CAM_CAL_MAX_LSC_SIZE];
    CAM_CAL_SINGLE_LSC_STRUCT Data[CAM_CAL_N3D_NUM];
}CAM_CAL_N3D_LSC_STRUCT, *PCAM_CAL_N3D_LSC_STRUCT;

#if 1  
typedef enum
{
    CAM_CAL_NONE_LSC=(MUINT8)0x0,
    CAM_CAL_SENOR_LSC=(MUINT8)0x1,
    CAM_CAL_MTK_LSC=(MUINT8)0x2
} CAM_CAL_LSC_VER_ENUM;
#endif



typedef enum
{
    CAM_CAL_SINGLE_EEPROM_DATA,//=(MUINT8)0x0,
    CAM_CAL_SINGLE_OTP_DATA,
    CAM_CAL_N3D_DATA,
    CAM_CAL_TYPE_NUM
} CAM_CAL_DATA_VER_ENUM;

typedef struct
{
#if 0
    CAM_CAL_SHADING_STRUCT Shading;
    CAM_CAL_DEFECT_STRUCT  Defect;
    CAM_CAL_PREGAIN_STRUCT Pregain;
#else
    CAMERA_CAM_CAL_TYPE_ENUM Command;   //seanlin121016 it tells cam_cal driver to return what users want.
    CAM_CAL_DATA_VER_ENUM DataVer;    
    MUINT8 PartNumber[24];     
    CAM_CAL_SINGLE_LSC_STRUCT SingleLsc;
    CAM_CAL_SINGLE_2A_STRUCT Single2A;    
    CAM_CAL_N3D_LSC_STRUCT N3DLsc;   
    CAM_CAL_N3D_3A_STRUCT N3D3A;
    CAM_CAL_N3D_3D_STRUCT N3D3D;     
#endif
}CAM_CAL_DATA_STRUCT, *PCAM_CAL_DATA_STRUCT;

typedef enum
{
    CAM_CAL_NONE = 0,
    CAM_CAL_USED      
} CAM_CAL_TYPE_ENUM;

CAM_CAL_TYPE_ENUM CAM_CALInit(void);
unsigned int CAM_CALDeviceName(char* DevName);


#endif //_CAMERA_CUSTOM_CAM_CAL_H_






/*- 6589 MTK LSC
009C	LSC Table type	"[0]sensor[1]MTK"	1
009D	Camera 1 : First pixel	"[bit 0]B[bit 1]Gb[bit 2]Gr[bit 3]R"	1
009E	table size (2108 Bytes )		2
00A0	FF 00 02 01 (4 bytes)		4
00A4	Preview Width (2 bytes)	Preview Height (2 bytes)	2
00A6	Preview Offset X (2 bytes)	Preview Offset Y (2 bytes)	2
00A8	Capture Width (2 bytes)	Capture Height (2 bytes)	2
00AA	Capture Offset X (2 bytes)	Capture Offfset Y (2 bytes)	2
00AC	Preview Shading Table Size (4 bytes)		4
00B0	Capture Shading Table Size (4 bytes)		4
00B4	Preview Shading Register Setting (5x4 bytes)		20
00C8	Capture Shading Register Setting (5x4 bytes)		20
00DC	Capture Shading Table (16 X 16 X 2 X 4 bytes)		2048
*/
/* -6589 SENSOR LSC
LSC Table type	"[0]sensor[1]MTK"	1
Camera 2 : First pixel	"[0]B[1]Gb[2]Gr[3]R"	1
TABLE SIZE		2
LSC Data (Max 2048)		2048
*/
