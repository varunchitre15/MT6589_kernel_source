#include "camera_custom_fd.h"

void get_fd_CustomizeData(FD_Customize_PARA  *FDDataOut)
{    
    FDDataOut->FDThreadNum = 2;
    FDDataOut->FDThreshold = 32;
    FDDataOut->MajorFaceDecision = 1;
    FDDataOut->OTRatio = 1088;
    FDDataOut->SmoothLevel = 1;
    FDDataOut->FDSkipStep = 4;
    FDDataOut->FDRectify = 100000;
    FDDataOut->FDRefresh = 60;
    FDDataOut->SDThreshold = 69;
// Begin_Arima_HCSW7_20130412_JohnnyZheng - [ThunderSoft] face detection and smile shutter deliver - [ALPS00486286]
    FDDataOut->SDMainFaceMust = 0;
    FDDataOut->SDMaxSmileNum = 15;
 // FDDataOut->SDMainFaceMust = 1;
 // FDDataOut->SDMaxSmileNum = 3; //Arima_HCSW7_20130305_ChrisTsai - Face & Smile detection - ALPS00486286
// End_Arima_HCSW7_20130412_JohnnyZheng - [ThunderSoft]
    FDDataOut->GSensor = 1;
}


