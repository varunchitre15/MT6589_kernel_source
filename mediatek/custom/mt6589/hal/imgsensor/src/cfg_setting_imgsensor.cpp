
//#ifndef _CFG_SETTING_IMGSENSOR_H_
//#define _CFG_SETTING_IMGSENSOR_H_
#include "camera_custom_imgsensor_cfg.h"
using namespace NSCamCustomSensor;

namespace NSCamCustomSensor {
/*******************************************************************************
* Image Sensor Orientation
*******************************************************************************/
SensorOrientation_T const&
getSensorOrientation()
{
    static SensorOrientation_T const inst = {
        u4Degree_0  : 90,   //  main sensor in degree (0, 90, 180, 270)
        u4Degree_1  : 0,    //  sub  sensor in degree (0, 90, 180, 270)
        u4Degree_2  : 90,   //  main2 sensor in degree (0, 90, 180, 270) 
    };
    return inst;

}


/*******************************************************************************
* Return fake orientation for front sensor or not
*       MTRUE: return 90 for front sensor in degree 0, 
*              return 270 for front sensor in degree 180.
*       MFALSE: not return fake orientation.
*******************************************************************************/
MBOOL isRetFakeSubOrientation()  
{
	return MFALSE;
}

/*******************************************************************************
* Return fake orientation for back sensor or not
*       MTRUE: return 90 for back sensor in degree 0, 
*              return 270 for back sensor in degree 180.
*       MFALSE: not return fake orientation.
*******************************************************************************/
MBOOL isRetFakeMainOrientation()  
{
	return MFALSE;
}

/*******************************************************************************
* Return fake orientation for back (3D) sensor or not
*       MTRUE: return 90 for back sensor in degree 0, 
*              return 270 for back sensor in degree 180.
*       MFALSE: not return fake orientation.
*******************************************************************************/
MBOOL isRetFakeMain2Orientation()  
{
	return MFALSE;
}


/*******************************************************************************
* Sensor Input Data Bit Order
*   Return:
*       0   : raw data input [9:2]
*       1   : raw data input [7:0]
*       -1  : error
*******************************************************************************/
MINT32
getSensorInputDataBitOrder(EDevId const eDevId)
{
    switch  (eDevId)
    {
    case eDevId_ImgSensor0:
        return  0;
    case eDevId_ImgSensor1:
        return  0;
    case eDevId_ImgSensor2:
        return  0;        
    default:
        break;
    }
    return  -1;
}


/*******************************************************************************
* Sensor Pixel Clock Inverse in PAD side.
*   Return:
*       0   : no inverse
*       1   : inverse
*       -1  : error
*******************************************************************************/
MINT32
getSensorPadPclkInv(EDevId const eDevId)
{
    switch  (eDevId)
    {
    case eDevId_ImgSensor0:
        return  0;
    case eDevId_ImgSensor1:
        return  0;
    case eDevId_ImgSensor2:
        return  0;        
    default:
        break;
    }
    return  -1;
}

/*******************************************************************************
* Sensor Placement Facing Direction
*   Return:
*       0   : Back side  
*       1   : Front side (LCD side)
*       -1  : error
*******************************************************************************/
MINT32  
getSensorFacingDirection(EDevId const eDevId)
{
    switch  (eDevId)
    {
    case eDevId_ImgSensor0:
        return  0;
    case eDevId_ImgSensor1:
        return  1;
    case eDevId_ImgSensor2:
        return  0;
    default:
        break;
    }
    return  -1;
}
//<2013/5/4-Stevenshi, for cts verifier case : Camera FOV Calibration.
/*******************************************************************************
* Image Sensor Module FOV
*******************************************************************************/
SensorViewAngle_T const&
getSensorViewAngle()
{
    static SensorViewAngle_T const inst = {
        MainSensorHorFOV  : 63,   
        MainSensorVerFOV  : 49,    
        SubSensorHorFOV  : 60,   
        SubSensorVerFOV  : 40,
        Main2SensorHorFOV  : 0,   //not support
        Main2SensorVerFOV  : 0,
    };
    return inst;

}
//>2013/5/4-Stevenshi, for cts verifier case : Camera FOV Calibration.
};

//#endif //  _CFG_SETTING_IMGSENSOR_H_

