
#include <utils/Log.h>
#include <fcntl.h>
#include <math.h>

//#include "msdk_nvram_camera_exp.h"
//#include "msdk_lens_exp.h"
#include "camera_custom_nvram.h"
#include "camera_custom_lens.h"

const NVRAM_LENS_PARA_STRUCT BU6424AF_LENS_PARA_DEFAULT_VALUE =
{
    //Version
    NVRAM_CAMERA_LENS_FILE_VERSION,

    // Focus Range NVRAM
    {0, 1023},

    // AF NVRAM
    {
        // -------- AF ------------
        {100, // i4Offset
          10, // i4NormalNum
          15, // i4MacroNum
           0, // i4InfIdxOffset
           0, //i4MacroIdxOffset          
    	{
                 0,  25,  55,  90, 130, 175, 225, 280, 335, 390,
               455, 520, 585, 650, 715,   0,   0,   0,   0,   0,
             0,   0,   0,   0,   0,   0,   0,   0,   0,   0
            },
          15, // i4THRES_MAIN;
          10, // i4THRES_SUB;            
          4,  // i4INIT_WAIT;
          {500, 500, 500, 500, 500}, // i4FRAME_WAIT
          0,  // i4DONE_WAIT;
              
          0,  // i4FAIL_POS;

          33,  // i4FRAME_TIME                        
          5,  // i4FIRST_FV_WAIT;
            
          30,  // i4FV_CHANGE_THRES;
          10000,  // i4FV_CHANGE_OFFSET;        
          3,  // i4FV_CHANGE_CNT;
          0,  // i4GS_CHANGE_THRES;    
          15,  // i4GS_CHANGE_OFFSET;    
          5,  // i4GS_CHANGE_CNT;            
          10,  // i4FV_STABLE_THRES;         // percentage -> 0 more stable  
          10000,  // i4FV_STABLE_OFFSET;        // value -> 0 more stable
          8,  // i4FV_STABLE_NUM;           // max = 9 (more stable), reset = 0
          8,  // i4FV_STABLE_CNT;           // max = 9                                      
          12,  // i4FV_1ST_STABLE_THRES;        
          10000,  // i4FV_1ST_STABLE_OFFSET;
          6,  // i4FV_1ST_STABLE_NUM;                        
          6  // i4FV_1ST_STABLE_CNT;      
         },
         
         // -------- ZSD AF ------------
         {100, // i4Offset
           10, // i4NormalNum
           15, // i4MacroNum
            0, // i4InfIdxOffset
            0, //i4MacroIdxOffset           
           {
               0,  25,  55,  90, 130, 175, 225, 280, 335, 390,
             455, 520, 585, 650, 715,   0,   0,   0,   0,   0,
                   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
           },
           15, // i4THRES_MAIN;
           10, // i4THRES_SUB;            
           4,  // i4INIT_WAIT;
           {500, 500, 500, 500, 500}, // i4FRAME_WAIT
           0,  // i4DONE_WAIT;
                     
           0,  // i4FAIL_POS;

           66,  // i4FRAME_TIME                                  
           5,  // i4FIRST_FV_WAIT;
                     
           30,  // i4FV_CHANGE_THRES;
           10000,  // i4FV_CHANGE_OFFSET;        
           3,  // i4FV_CHANGE_CNT;
           0,  // i4GS_CHANGE_THRES;    
           15,  // i4GS_CHANGE_OFFSET;    
           5,  // i4GS_CHANGE_CNT;            
           10,  // i4FV_STABLE_THRES;         // percentage -> 0 more stable  
           10000,  // i4FV_STABLE_OFFSET;        // value -> 0 more stable
           5,  // i4FV_STABLE_NUM;           // max = 9 (more stable), reset = 0
           5,  // i4FV_STABLE_CNT;           // max = 9                                      
           12,  // i4FV_1ST_STABLE_THRES;        
           10000,  // i4FV_1ST_STABLE_OFFSET;
           6,  // i4FV_1ST_STABLE_NUM;                        
           6  // i4FV_1ST_STABLE_CNT;      
           }, 
           
           // -------- VAFC ------------
         {100, // i4Offset
           20, // i4NormalNum
           20, // i4MacroNum
            0, // i4InfIdxOffset
            0, //i4MacroIdxOffset           
             {
                  0,  20,  40,  60,  80, 100, 120, 140, 160, 180,
                200, 220, 240, 260, 280, 300, 320, 340, 360, 390,
                  0,   0,   0,   0,   0,   0,   0,   0,   0,   0
             },
           15, // i4THRES_MAIN;
           10, // i4THRES_SUB;            
           1,  // i4INIT_WAIT;
           {500, 500, 500, 500, 500}, // i4FRAME_WAIT
           0,  // i4DONE_WAIT;
             
           0,  // i4FAIL_POS;

           33,  // i4FRAME_TIME                          
           5,  // i4FIRST_FV_WAIT;
             
           30,  // i4FV_CHANGE_THRES;
           10000,  // i4FV_CHANGE_OFFSET;        
           6,  // i4FV_CHANGE_CNT;
           0,  // i4GS_CHANGE_THRES;    
           15,  // i4GS_CHANGE_OFFSET;    
           5,  // i4GS_CHANGE_CNT;            
           10,  // i4FV_STABLE_THRES;         // percentage -> 0 more stable  
           10000,  // i4FV_STABLE_OFFSET;        // value -> 0 more stable
           8,  // i4FV_STABLE_NUM;           // max = 9 (more stable), reset = 0
           8,  // i4FV_STABLE_CNT;           // max = 9                                      
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

           { 88, 89, 88, 83, 80, 87, 86, 80,
            126,126,126,122,119,125,125,120,
            180,180,180,177,176,179,179,176},        // i4GMR[3][ISO_MAX_NUM];
          
// ------------------------------------------------------------------------                  
           {1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930},        // i4FV_DC[GMEAN_MAX_NUM][ISO_MAX_NUM];
           
          {150000,150000,150000,120000,80000,50000,30000,30000,
           150000,150000,150000,120000,80000,50000,30000,30000,
           150000,150000,150000,120000,80000,50000,30000,30000,
           150000,150000,150000,120000,80000,50000,30000,30000,
           150000,150000,150000,120000,80000,50000,30000,30000,
           150000,150000,150000,120000,80000,50000,30000,30000},         // i4MIN_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];        

           {   4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11}, // i4HW_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];       

// ------------------------------------------------------------------------
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
          
           {   8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17} // i4HW_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];       

          
         },
// ------------------------------------------------------------------------

         // --- sZSDAF_TH ---
          {
           8,   // i4ISONum;
           {100,150,200,300,400,600,800,1600},       // i4ISO[ISO_MAX_NUM];
                   
           6,   // i4GMeanNum;
           {20,55,105,150,180,205},        // i4GMean[GMEAN_MAX_NUM];

           { 88, 89, 88, 83, 80, 87, 86, 80,
            126,126,126,122,119,125,125,120,
            180,180,180,177,176,179,179,176},        // i4GMR[3][ISO_MAX_NUM];
           
// ------------------------------------------------------------------------                   
           {1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930,
            1049,1049,1049,1638,2359,3211,4194,7930},        // i4FV_DC[GMEAN_MAX_NUM][ISO_MAX_NUM];
            
           {150000,150000,150000,120000,80000,50000,30000,30000,
            150000,150000,150000,120000,80000,50000,30000,30000,
            150000,150000,150000,120000,80000,50000,30000,30000,
            150000,150000,150000,120000,80000,50000,30000,30000,
            150000,150000,150000,120000,80000,50000,30000,30000,
            150000,150000,150000,120000,80000,50000,30000,30000},         // i4MIN_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];        
           
           {   4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11,
               4,4,4,5,6,7,8,11}, // i4HW_TH[GMEAN_MAX_NUM][ISO_MAX_NUM];       

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
           
           {   8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17,
               8,8,8,9,10,11,14,17} // i4HW_TH2[GMEAN_MAX_NUM][ISO_MAX_NUM];       
           
          },

          1, // i4VAFC_FAIL_CNT;
          0, // i4CHANGE_CNT_DELTA;

          0, // i4LV_THRES;

          18, // i4WIN_PERCENT_W;
          24, // i4WIN_PERCENT_H;                
          100,  // i4InfPos;
          20, //i4AFC_STEP_SIZE;

          {
              {50, 100, 150, 200, 250}, // back to bestpos step
              { 0,   0,   0,   0,   0}  // hysteresis compensate step
          },

          {0, 50, 150, 250, 350}, // back jump
          200,  //i4BackJumpPos

          80, // i4FDWinPercent;
          40, // i4FDSizeDiff;

          3,   //i4StatGain          

          {0,0,0,0,0,0,0,0,0,0,
           0,0,0,0,0,0,0,0,0,0}// i4Coef[20];
    },

    {0}
};


UINT32 BU6424AF_getDefaultData(VOID *pDataBuf, UINT32 size)
{
    UINT32 dataSize = sizeof(NVRAM_LENS_PARA_STRUCT);

    if ((pDataBuf == NULL) || (size < dataSize))
    {
        return 1;
    }

    // copy from Buff to global struct
    memcpy(pDataBuf, &BU6424AF_LENS_PARA_DEFAULT_VALUE, dataSize);

    return 0;
}

PFUNC_GETLENSDEFAULT pBU6424AF_getDefaultData = BU6424AF_getDefaultData;


