
#ifndef _CAMERA_CUSTOM_AEPTABLE_H_
#define _CAMERA_CUSTOM_AEPTABLE_H_

#define MAX_PLINE_TABLE 40

// AE mode definition ==> need to change the AE Pline table
typedef enum                           
{
    LIB3A_AE_MODE_UNSUPPORTED           = -1,
    LIB3A_AE_MODE_OFF                            =  0,            // disable AE
    LIB3A_AE_MODE_AUTO                          =  1,           // auto mode   full auto ,EV ISO LCE .. is inactive
    LIB3A_AE_MODE_NIGHT                         =  2,           // preview Night Scene mode
    LIB3A_AE_MODE_ACTION                       =  3,           // AE Action mode
    LIB3A_AE_MODE_BEACH                         =  4,           // AE beach mode
    LIB3A_AE_MODE_CANDLELIGHT             =  5,           // AE Candlelight mode
    LIB3A_AE_MODE_FIREWORKS                 = 6,           // AE firework mode
    LIB3A_AE_MODE_LANDSCAPE                 = 7,           // AE landscape mode
    LIB3A_AE_MODE_PORTRAIT		            = 8,			 // AE portrait mode
    LIB3A_AE_MODE_NIGHT_PORTRAIT       = 9,           // AE night portrait mode
    LIB3A_AE_MODE_PARTY                          = 10,           // AE party mode
    LIB3A_AE_MODE_SNOW                           = 11,           // AE snow mode
    LIB3A_AE_MODE_SPORTS                        = 12,           // AE sport mode
    LIB3A_AE_MODE_STEADYPHOTO             = 13,           // AE steadyphoto mode
    LIB3A_AE_MODE_SUNSET                        = 14,           // AE sunset mode
    LIB3A_AE_MODE_THEATRE                      = 15,           // AE theatre mode
    LIB3A_AE_MODE_ISO_ANTI_SHAKE        = 16,           // AE ISO anti shake mode
    LIB3A_AE_MODE_MAX
} LIB3A_AE_MODE_T;

// AE ISO speed
typedef enum
{
    LIB3A_AE_ISO_SPEED_UNSUPPORTED =     -1,
    LIB3A_AE_ISO_SPEED_AUTO                =      0,
    LIB3A_AE_ISO_SPEED_100                   =    100,
    LIB3A_AE_ISO_SPEED_150                   =    150,    
    LIB3A_AE_ISO_SPEED_200                   =    200,
    LIB3A_AE_ISO_SPEED_300                   =    300,    
    LIB3A_AE_ISO_SPEED_400                   =    400,
    LIB3A_AE_ISO_SPEED_600                   =    600,    
    LIB3A_AE_ISO_SPEED_800                   =    800,
    LIB3A_AE_ISO_SPEED_1200                 =   1200,     
    LIB3A_AE_ISO_SPEED_1600                 =   1600, 
    LIB3A_AE_ISO_SPEED_2400                 =   2400, 
    LIB3A_AE_ISO_SPEED_3200                 =   3200,
    LIB3A_AE_ISO_SPEED_MAX = LIB3A_AE_ISO_SPEED_3200
}LIB3A_AE_ISO_SPEED_T;

//AE Parameter Structure
typedef enum
{
    AETABLE_RPEVIEW_AUTO = 0,     // default 60Hz
    AETABLE_VIDEO,
    AETABLE_VIDEO_DYNAMIC,    // video mode but the frame rate don't fix
    AETABLE_VIDEO_NIGHT,
    AETABLE_VIDEO_NIGHT_DYNAMIC,    // video mode but the frame rate don't fix
    AETABLE_CAPTURE_AUTO,
    AETABLE_CAPTURE_AUTO_ZSD,
    AETABLE_CAPTURE_ISO100,
    AETABLE_CAPTURE_ISO100_ZSD,
    AETABLE_CAPTURE_ISO200,
    AETABLE_CAPTURE_ISO200_ZSD,
    AETABLE_CAPTURE_ISO400,
    AETABLE_CAPTURE_ISO400_ZSD,
    AETABLE_CAPTURE_ISO800,
    AETABLE_CAPTURE_ISO800_ZSD,
    AETABLE_CAPTURE_ISO1600,
    AETABLE_CAPTURE_ISO1600_ZSD,
    AETABLE_CAPTURE_ISO3200,
    AETABLE_CAPTURE_ISO3200_ZSD,
    AETABLE_MODE_INDEX1,                           // for mode used of capture
    AETABLE_MODE_INDEX2,
    AETABLE_MODE_INDEX3,
    AETABLE_MODE_INDEX4,
    AETABLE_MODE_INDEX5,
    AETABLE_MODE_INDEX6,
    AETABLE_MODE_INDEX7,
    AETABLE_MODE_INDEX8,
    AETABLE_MODE_INDEX9,
    AETABLE_MODE_INDEX10,
    AETABLE_MODE_INDEX11,
    AETABLE_MODE_INDEX12,
    AETABLE_MODE_INDEX13,
    AETABLE_MODE_INDEX14,
    AETABLE_MODE_INDEX15,
    AETABLE_MODE_INDEX16,
    AETABLE_MODE_INDEX17,
}eAETableID;

typedef struct	strEvSetting
{
    MUINT32 u4Eposuretime;   //!<: Exposure time in ms
    MUINT32 u4AfeGain;           //!<: raw gain
    MUINT32 u4IspGain;           //!<: sensor gain
    MUINT8  uIris;                    //!<: Iris
    MUINT8  uSensorMode;      //!<: sensor mode
    MUINT8  uFlag;                   //!<: flag to indicate hysteresis ...
//    MUINT8  uLV;                        //!<: LV avlue , in ISO 100 condition  LV=TV+AV
}strEvSetting;

typedef struct
{
   eAETableID   eID;
   MUINT32       u4TotalIndex;      //preview table Tatal index
   MINT32        i4StrobeTrigerBV;  // Strobe triger point in strobe auto mode
   MINT32        i4MaxBV;
   MINT32        i4MinBV;
   LIB3A_AE_ISO_SPEED_T   ISOSpeed;
   strEvSetting *pTable60Hz;   //point to 50Hz table
   strEvSetting *pTable50Hz;   //point to 60Hz table
   strEvSetting *pCurrentTable;   //point to current table
}strAETable;

typedef struct
{
    LIB3A_AE_MODE_T eAEMode;
    eAETableID ePLineID;
    eAETableID eZSDPLineID;
} strAEPLineMapping;

typedef struct
{
    strAETable    *PlineTable[MAX_PLINE_TABLE];    //AE Pline Table
}strAEPLineTable;

typedef struct
{
    strAEPLineMapping    *pAEModePLineMapping;   // Get PLine ID for different AE mode
    strAEPLineTable AEPlineTable;
} AE_PLINETABLE_T, *PAE_PLINETABLE_STRUCT; 

#endif // _CAMERA_CUSTOM_AEPTABLE_H_

