#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/xlog.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_gpio.h>

#include "ddp_drv.h"
#include "ddp_reg.h"
#include "ddp_path.h"
#include "ddp_tdshp.h"


unsigned int sharpGain = 0;

static DISPLAY_TDSHP_T g_TDSHP_Index; 

DISPLAY_TDSHP_T *get_TDSHP_index(void)
{    
    return &g_TDSHP_Index;
}


void DpEngine_SHARPonInit(void)
{
    //XLOGD("init SHARP \n");



    DISP_REG_SET((DISPSYS_TDSHP_BASE + 0xf00) ,0x00000001);

    //wrapper color matrix index
    DISP_REG_SET(DISPSYS_TDSHP_BASE + TDS_HSYNC_WIDTH            , 0x00000040  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + TDS_ACTIVE_WIDTH_IN_VBLANK , 0x00000040  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_C00            , 0x00000132  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_C01            , 0x00000259  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_C02            , 0x00000075  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_C10            , 0x00001F53  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_C11            , 0x00001EAD  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_C12            , 0x00000200  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_C20            , 0x00000200  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_C21            , 0x00001E53  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_C22            , 0x00001FAD  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_IN_OFFSET0     , 0x00000000  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_IN_OFFSET1     , 0x00000000  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_IN_OFFSET2     , 0x00000000  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_OUT_OFFSET0    , 0x00000000  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_OUT_OFFSET1    , 0x00000080  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_IN_MTX_OUT_OFFSET2    , 0x00000080  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_C00           , 0x00000400  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_C01           , 0x00001FFF  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_C02           , 0x0000059C  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_C10           , 0x00000400  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_C11           , 0x00001E9F  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_C12           , 0x00001D25  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_C20           , 0x00000400  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_C21           , 0x00000716  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_C22           , 0x00000001  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_IN_OFFSET0    , 0x00000000  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_IN_OFFSET1    , 0x00000180  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_IN_OFFSET2    , 0x00000180  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_OUT_OFFSET0   , 0x00000000  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_OUT_OFFSET1   , 0x00000000  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_OUT_MTX_OUT_OFFSET2   , 0x00000000  );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x350 , 0); //bypass off

}


void DpEngine_SHARPonConfig(unsigned int srcWidth,unsigned int srcHeight)
{
   // XLOGD("config SHARP %d %d \n",  srcWidth,  srcHeight);
    DISP_REG_SET((DISPSYS_TDSHP_BASE + 0xf40), srcWidth);
    DISP_REG_SET((DISPSYS_TDSHP_BASE + 0xf44), srcHeight);

    // enable R2Y/Y2R in Color Wrapper
    DISP_REG_SET(DISPSYS_TDSHP_BASE + DTDS_CONFIG ,0x00000006);

	if(sharpGain >= SHARP_TUNING_INDEX)
	{
		//XLOGD("SHARP Tuning index range error !\n");
		return;	    
	} 

        DISP_REG_SET(DISPSYS_TDSHP_BASE+0x000,0); // turn off TDSHP
/*
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x000 , TDSHP_Param[ sharpGain][0]<< 30
                                       | TDSHP_Param[ sharpGain][1]<< 29
                                       | TDSHP_Param[ sharpGain][2]<< 21
                                       | TDSHP_Param[ sharpGain][3]<< 20
                                       | 1 << 31
                                       | TDSHP_Param[ sharpGain][5]<< 3
                                       | TDSHP_Param[ sharpGain][6]<< 1
                                       | TDSHP_Param[ sharpGain][7]<< 8
                                       | TDSHP_Param[ sharpGain][8]<< 7
                                       | TDSHP_Param[ sharpGain][9]<< 19
                                       | TDSHP_Param[ sharpGain][10]<< 18
                                       | TDSHP_Param[ sharpGain][11]<< 11
                                       | TDSHP_Param[ sharpGain][12]<< 9
                                       | TDSHP_Param[ sharpGain][13]<< 4 );
*/
	DISP_REG_SET(DISPSYS_TDSHP_BASE+0x014 , TDSHP_Param[ sharpGain][14]<< 31
                                       | TDSHP_Param[ sharpGain][15]<< 16
                                       | TDSHP_Param[ sharpGain][16]<< 26 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x020 , TDSHP_Param[ sharpGain][17]<< 24
                                       | TDSHP_Param[ sharpGain][18]<< 16);
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x02C , TDSHP_Param[ sharpGain][19]<< 28
                                       | TDSHP_Param[ sharpGain][20]<< 24);
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x040 , TDSHP_Param[ sharpGain][23]<< 24
                                       | TDSHP_Param[ sharpGain][24]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x04C , TDSHP_Param[ sharpGain][25]<< 24
                                       | TDSHP_Param[ sharpGain][26]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x058 , TDSHP_Param[ sharpGain][27]<< 24
                                       | TDSHP_Param[ sharpGain][28]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x064 , TDSHP_Param[ sharpGain][29]<< 28
                                       | TDSHP_Param[ sharpGain][30]<< 24 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x068 , TDSHP_Param[ sharpGain][31]<< 10
                                       | TDSHP_Param[ sharpGain][32]<< 9 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x06C , TDSHP_Param[ sharpGain][33]<< 24
                                       | TDSHP_Param[ sharpGain][34]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x078 , TDSHP_Param[ sharpGain][35]<< 24
                                       | TDSHP_Param[ sharpGain][36]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x028 , TDSHP_Param[ sharpGain][37]<< 16
                                       | TDSHP_Param[ sharpGain][38]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x030 , TDSHP_Param[ sharpGain][39]<< 24
                                       | TDSHP_Param[ sharpGain][40]<< 20 );
	DISP_REG_SET(DISPSYS_TDSHP_BASE+0x034 , TDSHP_Param[ sharpGain][21]<< 16
                                       | TDSHP_Param[ sharpGain][22]<< 24 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x03C , TDSHP_Param[ sharpGain][41]<< 16
                                       | TDSHP_Param[ sharpGain][42]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x048 , TDSHP_Param[ sharpGain][43]<< 16
                                       | TDSHP_Param[ sharpGain][44]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x054 , TDSHP_Param[ sharpGain][45]<< 16
                                       | TDSHP_Param[ sharpGain][46]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x060 , TDSHP_Param[ sharpGain][47]<< 16
                                       | TDSHP_Param[ sharpGain][48]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x068 , TDSHP_Param[ sharpGain][49]<< 1
                                       | TDSHP_Param[ sharpGain][50]<< 24
                                       | TDSHP_Param[ sharpGain][51]<< 0
                                       | TDSHP_Param[ sharpGain][52]<< 20 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x074 , TDSHP_Param[ sharpGain][53]<< 16
                                       | TDSHP_Param[ sharpGain][54]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x080 , TDSHP_Param[ sharpGain][55]<< 16
                                       | TDSHP_Param[ sharpGain][56]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x084 , TDSHP_Param[ sharpGain][57]<< 31
                                       | TDSHP_Param[ sharpGain][58]<< 8
                                       | TDSHP_Param[ sharpGain][59]<< 16
                                       | TDSHP_Param[ sharpGain][60]<< 24
                                       | TDSHP_Param[ sharpGain][61]<< 30 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x088 , TDSHP_Param[ sharpGain][62]<< 31
                                       | TDSHP_Param[ sharpGain][63]<< 8
                                       | TDSHP_Param[ sharpGain][64]<< 16
                                       | TDSHP_Param[ sharpGain][65]<< 24
                                       | TDSHP_Param[ sharpGain][66]<< 30 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x094 , TDSHP_Param[ sharpGain][67]<< 31
                                       | TDSHP_Param[ sharpGain][68]<< 8
                                       | TDSHP_Param[ sharpGain][69]<< 16
                                       | TDSHP_Param[ sharpGain][70]<< 24
                                       | TDSHP_Param[ sharpGain][71]<< 30 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x098 , TDSHP_Param[ sharpGain][72]<< 31
                                       | TDSHP_Param[ sharpGain][73]<< 8
                                       | TDSHP_Param[ sharpGain][74]<< 16
                                       | TDSHP_Param[ sharpGain][75]<< 24
                                       | TDSHP_Param[ sharpGain][76]<< 30 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x09C , TDSHP_Param[ sharpGain][77]<< 24
                                       | TDSHP_Param[ sharpGain][78]<< 16
                                       | TDSHP_Param[ sharpGain][79]<< 4
                                       | TDSHP_Param[ sharpGain][80]<< 12
                                       | TDSHP_Param[ sharpGain][81]<< 00 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x0A0 , TDSHP_Param[ sharpGain][82]<< 0
                                       | TDSHP_Param[ sharpGain][83]<< 16
                                       | TDSHP_Param[ sharpGain][84]<< 8
                                       | TDSHP_Param[ sharpGain][85]<< 24);
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x0A4 , TDSHP_Param[ sharpGain][86]<< 31 
		                               | TDSHP_Param[ sharpGain][87]<< 8
                                       | TDSHP_Param[ sharpGain][88]<< 16
                                       | TDSHP_Param[ sharpGain][89]<< 24
                                       | TDSHP_Param[ sharpGain][90]<< 30
                                       | TDSHP_Param[ sharpGain][91]<< 0 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x01C , TDSHP_Param[ sharpGain][92]<< 0 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x0A8 , TDSHP_Param[ sharpGain][93]<< 24
                                       | TDSHP_Param[ sharpGain][94]<< 8
                                       | TDSHP_Param[ sharpGain][95]<< 0
                                       | TDSHP_Param[ sharpGain][96]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x0AC , TDSHP_Param[ sharpGain][97]<< 24
                                       | TDSHP_Param[ sharpGain][98]<< 16
                                       | TDSHP_Param[ sharpGain][99]<< 8
                                       | TDSHP_Param[ sharpGain][100]<< 00 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x0B0 , TDSHP_Param[ sharpGain][101]<< 24
                                       | TDSHP_Param[ sharpGain][102]<< 16
                                       | TDSHP_Param[ sharpGain][103]<< 8
                                       | TDSHP_Param[ sharpGain][104]<< 00 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x0B4 , TDSHP_Param[ sharpGain][105]<< 24
                                       | TDSHP_Param[ sharpGain][106]<< 16
                                       | TDSHP_Param[ sharpGain][107]<< 8
                                       | TDSHP_Param[ sharpGain][108]<< 0 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x0B8 , TDSHP_Param[ sharpGain][109]<< 16
                                       | TDSHP_Param[ sharpGain][110]<< 14
                                       | TDSHP_Param[ sharpGain][111]<< 24
                                       | TDSHP_Param[ sharpGain][112]<< 15 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x278 , TDSHP_Param[ sharpGain][113]<< 3 );

    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x208 , TDSHP_Param[ sharpGain][129]<< 20
		                               | TDSHP_Param[ sharpGain][117]<< 19
                                       | TDSHP_Param[ sharpGain][118]<< 0
                                       | TDSHP_Param[ sharpGain][119]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x20C , TDSHP_Param[ sharpGain][120]<< 24
                                       | TDSHP_Param[ sharpGain][121]<< 15
                                       | TDSHP_Param[ sharpGain][122]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x200 , TDSHP_Param[ sharpGain][114]<< 31 
		                               | TDSHP_Param[ sharpGain][123]<< 16
                                       | TDSHP_Param[ sharpGain][124]<< 0
                                       | TDSHP_Param[ sharpGain][125]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x204 , TDSHP_Param[ sharpGain][126]<< 0
                                       | TDSHP_Param[ sharpGain][127]<< 12
                                       | TDSHP_Param[ sharpGain][128]<< 22 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x218 , TDSHP_Param[ sharpGain][130]<< 24
                                       | TDSHP_Param[ sharpGain][131]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x210 , TDSHP_Param[ sharpGain][132]<< 24
                                       | TDSHP_Param[ sharpGain][133]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x220 , TDSHP_Param[ sharpGain][134]<< 24
                                       | TDSHP_Param[ sharpGain][135]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x230 , TDSHP_Param[ sharpGain][148]<< 20 
		                               | TDSHP_Param[ sharpGain][136]<< 19
                                       | TDSHP_Param[ sharpGain][137]<< 0
                                       | TDSHP_Param[ sharpGain][138]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x234 , TDSHP_Param[ sharpGain][139]<< 24
                                       | TDSHP_Param[ sharpGain][140]<< 15
                                       | TDSHP_Param[ sharpGain][141]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x228 , TDSHP_Param[ sharpGain][115]<< 31
		                               | TDSHP_Param[ sharpGain][142]<< 16
                                       | TDSHP_Param[ sharpGain][143]<< 0
                                       | TDSHP_Param[ sharpGain][144]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x22C , TDSHP_Param[ sharpGain][145]<< 0
                                       | TDSHP_Param[ sharpGain][146]<< 12
                                       | TDSHP_Param[ sharpGain][147]<< 22 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x240 , TDSHP_Param[ sharpGain][149]<< 24
                                       | TDSHP_Param[ sharpGain][150]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x238 , TDSHP_Param[ sharpGain][151]<< 24
                                       | TDSHP_Param[ sharpGain][152]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x248 , TDSHP_Param[ sharpGain][153]<< 24
                                       | TDSHP_Param[ sharpGain][154]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x258 , TDSHP_Param[ sharpGain][167]<< 20 
		                               | TDSHP_Param[ sharpGain][155]<< 19
                                       | TDSHP_Param[ sharpGain][156]<< 0
                                       | TDSHP_Param[ sharpGain][157]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x25C , TDSHP_Param[ sharpGain][158]<< 24
                                       | TDSHP_Param[ sharpGain][159]<< 15
                                       | TDSHP_Param[ sharpGain][160]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x250 , TDSHP_Param[ sharpGain][116]<< 31
		                               | TDSHP_Param[ sharpGain][161]<< 16
                                       | TDSHP_Param[ sharpGain][162]<< 0
                                       | TDSHP_Param[ sharpGain][163]<< 8 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x254 , TDSHP_Param[ sharpGain][164]<< 0
                                       | TDSHP_Param[ sharpGain][165]<< 12
                                       | TDSHP_Param[ sharpGain][166]<< 22 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x268 , TDSHP_Param[ sharpGain][168]<< 24
                                       | TDSHP_Param[ sharpGain][169]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x260 , TDSHP_Param[ sharpGain][170]<< 24
                                       | TDSHP_Param[ sharpGain][171]<< 16 );
    DISP_REG_SET(DISPSYS_TDSHP_BASE+0x270 , TDSHP_Param[ sharpGain][172]<< 24
                                       | TDSHP_Param[ sharpGain][173]<< 16 );

}





