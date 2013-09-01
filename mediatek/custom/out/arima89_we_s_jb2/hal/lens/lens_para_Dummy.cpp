
#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

//#include "msdk_nvram_camera_exp.h"
#include "camera_custom_nvram.h"
#include "camera_custom_lens.h"
//#include "msdk_lens_exp.h"


const NVRAM_LENS_PARA_STRUCT DUMMY_LENS_PARA_DEFAULT_VALUE =
{
    //Version
    NVRAM_CAMERA_LENS_FILE_VERSION,

    // Focus Range NVRAM
    {0, 1023},

    // AF NVRAM
    {
        // -------- AF ------------
        {300, // i4Offset
          12, // i4NormalNum
          13, // i4MacroNum
           0, // i4InfIdxOffset
           0, //i4MacroIdxOffset
            {
              0,  25,  55,  90, 130, 175, 225, 280, 335, 390,
            455, 520, 585, 650, 715,   0,   0,   0,   0,   0,
              0,   0,   0,   0,   0,   0,   0,   0,   0,   0              
            },
          30, // i4THRES_MAIN;
          20, // i4THRES_SUB;            
          3,  // i4INIT_WAIT;
          {500, 500, 500, 500, 500}, // i4FRAME_WAIT
          0,  // i4DONE_WAIT;
            
          0,  // i4FAIL_POS;

          33,  // i4FRAME_TIME            
          10,  // i4FIRST_FV_WAIT;
            
          45,  // i4FV_CHANGE_THRES;
          10000,  // i4FV_CHANGE_OFFSET;        
          12,  // i4FV_CHANGE_CNT;
          40,  // i4GS_CHANGE_THRES;    
          10000,  // i4GS_CHANGE_OFFSET;    
          12,  // i4GS_CHANGE_CNT;            
          15,  // i4FV_STABLE_THRES;         // percentage -> 0 more stable  
          10000,  // i4FV_STABLE_OFFSET;        // value -> 0 more stable
          4,  // i4FV_STABLE_NUM;           // max = 9 (more stable), reset = 0
          4,  // i4FV_STABLE_CNT;           // max = 9                                      
          12,  // i4FV_1ST_STABLE_THRES;        
          10000,  // i4FV_1ST_STABLE_OFFSET;
          6,  // i4FV_1ST_STABLE_NUM;                        
          6  // i4FV_1ST_STABLE_CNT;      
         },
         
         // -------- ZSD AF ------------
         {  0, // i4Offset
           12, // i4NormalNum
           14, // i4MacroNum
            0, // i4InfIdxOffset
            0, //i4MacroIdxOffset
           {
              0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
              0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
              0,   0,   0,   0,   0,   0,   0,   0,   0,   0
           },
           30, // i4THRES_MAIN;
           20, // i4THRES_SUB;            
           1,  // i4INIT_WAIT;
           {500, 500, 500, 500, 500}, // i4FRAME_WAIT
           0,  // i4DONE_WAIT;
                     
           0,  // i4FAIL_POS;

           66,  // i4FRAME_TIME                     
           10,  // i4FIRST_FV_WAIT;
                     
           45,  // i4FV_CHANGE_THRES;
           10000,  // i4FV_CHANGE_OFFSET;        
           12,  // i4FV_CHANGE_CNT;
           40,  // i4GS_CHANGE_THRES;    
           10000,  // i4GS_CHANGE_OFFSET;    
           12,  // i4GS_CHANGE_CNT;            
           15,  // i4FV_STABLE_THRES;         // percentage -> 0 more stable  
           10000,  // i4FV_STABLE_OFFSET;        // value -> 0 more stable
           4,  // i4FV_STABLE_NUM;           // max = 9 (more stable), reset = 0
           4,  // i4FV_STABLE_CNT;           // max = 9                                      
           12,  // i4FV_1ST_STABLE_THRES;        
           10000,  // i4FV_1ST_STABLE_OFFSET;
           6,  // i4FV_1ST_STABLE_NUM;                        
           6  // i4FV_1ST_STABLE_CNT;      
           }, 
           
           // -------- VAFC ------------
         {  0, // i4Offset
           21, // i4NormalNum
           21, // i4MacroNum
            0, // i4InfIdxOffset
            0, //i4MacroIdxOffset           
             {
                  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
                  0,   0,   0,   0,   0,   0,   0,   0,   0,   0
             },
           30, // i4THRES_MAIN;
           20, // i4THRES_SUB;            
           1,  // i4INIT_WAIT;
           {500, 500, 500, 500, 500}, // i4FRAME_WAIT
           0,  // i4DONE_WAIT;
             
           0,  // i4FAIL_POS;

           33,  // i4FRAME_TIME             
           10,  // i4FIRST_FV_WAIT;
             
           45,  // i4FV_CHANGE_THRES;
           10000,  // i4FV_CHANGE_OFFSET;        
           12,  // i4FV_CHANGE_CNT;
           40,  // i4GS_CHANGE_THRES;    
           10000,  // i4GS_CHANGE_OFFSET;    
           12,  // i4GS_CHANGE_CNT;            
           15,  // i4FV_STABLE_THRES;         // percentage -> 0 more stable  
           10000,  // i4FV_STABLE_OFFSET;        // value -> 0 more stable
           4,  // i4FV_STABLE_NUM;           // max = 9 (more stable), reset = 0
           4,  // i4FV_STABLE_CNT;           // max = 9                                      
           12,  // i4FV_1ST_STABLE_THRES;        
           10000,  // i4FV_1ST_STABLE_OFFSET;
           6,  // i4FV_1ST_STABLE_NUM;                        
           6  // i4FV_1ST_STABLE_CNT;      
          },

        // --- sAF_TH ---
         {
          8,   // i4ISONum;
          {100,150,200,300,400,600,800,1600},       // i4ISO[ISO_MAX_NUM];
                  
          6,   // i4GMeanNum;
          {20,55,105,150,180,205},        // i4GMean[GMEAN_MAX_NUM];

          { 31, 31, 31, 31, 31, 31, 31, 31,
            63, 63, 63, 63, 63, 63, 63, 63,
           127,127,127,127,127,127,127,127},        // i4GMR[3][ISO_MAX_NUM];
                  
          {0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0},        // i4FV_DC[GMEAN_MAX_NUM][ISO_MAX_NUM];
           
          {50000,50000,50000,50000,50000,50000,50000,50000,
           50000,50000,50000,50000,50000,50000,50000,50000,
           50000,50000,50000,50000,50000,50000,50000,50000,
           50000,50000,50000,50000,50000,50000,50000,50000,
           50000,50000,50000,50000,50000,50000,50000,50000,
           50000,50000,50000,50000,50000,50000,50000,50000},         // i4MIN_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];        

          {0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0},       // i4HW_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];       

         {0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0},        // i4FV_DC2[GMEAN_MAX_NUM][ISO_MAX_NUM];
          
         {0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0},         // i4MIN_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];
         
         {0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0,
          0,0,0,0,0,0,0,0}          // i4HW_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];       
           
         },

         // --- sZSDAF_TH ---
          {
           8,   // i4ISONum;
           {100,150,200,300,400,600,800,1600},       // i4ISO[ISO_MAX_NUM];
                   
           6,   // i4GMeanNum;
           {20,55,105,150,180,205},        // i4GMean[GMEAN_MAX_NUM];

           { 31, 31, 31, 31, 31, 31, 31, 31,
             63, 63, 63, 63, 63, 63, 63, 63,
            127,127,127,127,127,127,127,127},        // i4GMR[3][ISO_MAX_NUM];
                   
           {0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0},        // i4FV_DC[GMEAN_MAX_NUM][ISO_MAX_NUM];
            
           {50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000,
            50000,50000,50000,50000,50000,50000,50000,50000},         // i4MIN_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];        
         
           {0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0},       // i4HW_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];       

           {0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0},        // i4FV_DC2[GMEAN_MAX_NUM][ISO_MAX_NUM];
            
           {0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0},         // i4MIN_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];
           
           {0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0,
            0,0,0,0,0,0,0,0}          // i4HW_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];       
           
          },

          2, // i4VAFC_FAIL_CNT;
          0, // i4CHANGE_CNT_DELTA;
          
          30, // i4LV_THRES;
          
          18, // i4WIN_PERCENT_W;
          24, // i4WIN_PERCENT_H;                
          0,  // i4InfPos;
          20, //i4AFC_STEP_SIZE;

          {
              {50, 100, 150, 200, 250}, // back to bestpos step
              { 0,   0,   0,   0,   0}  // hysteresis compensate step
          },

          {0, 50, 150, 250, 350}, // back jump
          400,  //i4BackJumpPos

          80, // i4FDWinPercent;
          40, // i4FDSizeDiff;

          3,   //i4StatGain          
          
          {0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0}// i4Coef[20];
    },

    {0}
};



UINT32 Dummy_getDefaultData(VOID *pDataBuf, UINT32 size)
{
	UINT32 dataSize = sizeof(NVRAM_LENS_PARA_STRUCT);

    if ((pDataBuf == NULL) || (size < dataSize))
    {
        return 1;
    }

	// copy from Buff to global struct
    memcpy(pDataBuf, &DUMMY_LENS_PARA_DEFAULT_VALUE, dataSize);

    return 0;
}

PFUNC_GETLENSDEFAULT pDummy_getDefaultData = Dummy_getDefaultData;


