
#ifndef BUILD_LK
#include <linux/string.h>
#endif
#include "lcm_drv.h"

#ifdef BUILD_LK
	#include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
	#include <asm/arch/mt_gpio.h>
#else
	#include <mach/mt_gpio.h>
#endif
// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  (540)
#define FRAME_HEIGHT (960)

#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER

#define LCM_ID_HX8389B (0x89)


#ifndef TRUE
    #define TRUE 1
#endif

#ifndef FALSE
    #define FALSE 0
#endif

static unsigned int lcm_esd_test = FALSE;      ///only for ESD test


// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))

//<2013/06/03-25639-stevenchen, [Pelican][drv] Add second source TCL LCM.
int lcm_id = 0;
//>2013/06/03-25639-stevenchen

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	        lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)											lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   				lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)   


#define   LCM_DSI_CMD_MODE							0

static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[128]; //64
};

static void init_lcm_registers(void)
{

	unsigned int data_array[40];
 
    data_array[0] = 0x00043902; // SET password                          
    data_array[1] = 0x8983FFB9;                
    dsi_set_cmdq(&data_array, 2, 1);
    MDELAY(5);
 
    data_array[0] = 0x00043902;
    data_array[1] = 0x105805DE;                
    dsi_set_cmdq(&data_array, 2, 1);
    MDELAY(5);

    data_array[0] = 0x00033902;                        
    data_array[1] = 0x009201BA; //SET MIPI 41:2 lane 00:1 lane
    dsi_set_cmdq(&data_array, 2, 1);
    MDELAY(10);
 
    data_array[0] = 0x00143902;                        
    data_array[1] = 0x040000B1;
    data_array[2] = 0x111096E3;
    data_array[3] = 0x3028EF6F;                        
    data_array[4] = 0x00432323;
    data_array[5] = 0x0020F258;
    dsi_set_cmdq(&data_array, 6, 1);
    MDELAY(5);
 
    data_array[0] = 0x00083902;                         
    data_array[1] = 0x780000B2;
    data_array[2] = 0x80000308;
    dsi_set_cmdq(&data_array, 3, 1);
    MDELAY(5);
        
    data_array[0] = 0x00183902;                        
    data_array[1] = 0x000880B4;  // SET CYC
    data_array[2] = 0x00001032;
    data_array[3] = 0x00000000;                        
    data_array[4] = 0x400A3700;
    data_array[5] = 0x400A3704;
    data_array[6] = 0x0A555014;
    dsi_set_cmdq(&data_array, 7, 1);
    MDELAY(5);
 
    data_array[0] = 0x00313902;                        
    data_array[1] = 0x4C0000D5;  // SET GIP
    data_array[2] = 0x00000100;
    data_array[3] = 0x99006000;                        
    data_array[4] = 0x88889988;
    data_array[5] = 0x88108832;
    data_array[6] = 0x10548876;
    data_array[7] = 0x88888832;
    data_array[8] = 0x99888888;
    data_array[9] = 0x45889988;
    data_array[10] = 0x01886788;
    data_array[11] = 0x01232388;
    data_array[12] = 0x88888888;
    data_array[13] = 0x00000088;
    dsi_set_cmdq(&data_array, 14, 1);
    MDELAY(5);

    data_array[0] = 0x00233902;                        
    data_array[1] = 0x181405E0;
    data_array[2] = 0x203F342D;
    data_array[3] = 0x0E0E083C;                        
    data_array[4] = 0x12101311;
    data_array[5] = 0x14051C1A;
    data_array[6] = 0x3F342D18;
    data_array[7] = 0x0E083C20;
    data_array[8] = 0x1013110E;
    data_array[9] = 0x001C1A12;
    dsi_set_cmdq(&data_array, 10, 1);
    MDELAY(5);

    data_array[0] = 0x00803902;                        
    data_array[1] = 0x080201C1;
    data_array[2] = 0x2D272017;
    data_array[3] = 0x48403832;
    data_array[4] = 0x675F574F;
    data_array[5] = 0x89817970;
    data_array[6] = 0xA8A19991;
    data_array[7] = 0xC9C2B9B0;
    data_array[8] = 0xE9E2D9D0;
    data_array[9] = 0xD0FFFAF3;
    data_array[10] = 0xA9A9DFAF;
    data_array[11] = 0xC092793D;
    data_array[12] = 0x20170802;
    data_array[13] = 0x38322D27;
    data_array[14] = 0x574F4840;
    data_array[15] = 0x7970675F;
    data_array[16] = 0x99918981;
    data_array[17] = 0xB9B0A8A1;
    data_array[18] = 0xD9D0C9C2;
    data_array[19] = 0xFAF3E9E2;
    data_array[20] = 0xDFAFD0FF;
    data_array[21] = 0x793DA9A9;
    data_array[22] = 0x0802C092;
    data_array[23] = 0x2D272017;
    data_array[24] = 0x48403832;
    data_array[25] = 0x675F574F;
    data_array[26] = 0x89817970;
    data_array[27] = 0xA8A19991;
    data_array[28] = 0xC9C2B9B0;
    data_array[29] = 0xE9E2D9D0;
    data_array[30] = 0xD0FFFAF3;
    data_array[31] = 0xA9A9DFAF;
    data_array[32] = 0xC092793D;
    dsi_set_cmdq(&data_array, 33, 1);
    MDELAY(10);

    data_array[0] = 0x00023902;                    
    data_array[1] = 0x00000ECC;                
    dsi_set_cmdq(&data_array, 2, 1);
    MDELAY(5);

    data_array[0] = 0x00053902;                    
    data_array[1] = 0x00A000B6;
    data_array[2] = 0x000000A0;
    dsi_set_cmdq(&data_array, 3, 1);
    MDELAY(5);

    data_array[0] = 0x00033902;                    
    data_array[1] = 0x000707CB;                
    dsi_set_cmdq(&data_array, 2, 1);
    MDELAY(5);
 
 
    data_array[0] = 0x00053902;                     
    data_array[1] = 0xFF0000BB;
    data_array[2] = 0x00000080;
    dsi_set_cmdq(&data_array, 3, 1);
    MDELAY(5);
 
 
    data_array[0] = 0x00110500;      //Sleep Out         
    dsi_set_cmdq(&data_array, 1, 1);
    MDELAY(120);			//EricHsieh,2013/6/18,Bootup time tuning
 
  
    data_array[0] = 0x00290500;     //Display On          
    dsi_set_cmdq(&data_array, 1, 1);
    MDELAY(10);			//EricHsieh,2013/6/18,Bootup time tuning


}

//<2013/05/03-24543-stevenchen, [ATS00158948][Settings] It should not take more than 0.7s to wake up DUT from sleep mode.
//<2013/04/30-24461-stevenchen, [Pelican][drv] Improve LCM performance.
static struct LCM_setting_table lcm_initialization_setting[] = {
	
	/*
	Note :

	Data ID will depends on the following rule.
	
		count of parameters > 1	=> Data ID = 0x39
		count of parameters = 1	=> Data ID = 0x15
		count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag
	
	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/


	//must use 0x39 for init setting for all register.

	{0xB9, 3, {0xFF,0x83,0x89}},
	//{REGFLAG_DELAY, 1, {}},

	{0xDE, 3, {0x05,0x58,0x10}},
	//{REGFLAG_DELAY, 1, {}},

//<2013/04/10-23689-stevenchen, [Pelican][drv] Update LCM initial code.
//<2013/03/21-23086-stevenchen, Lower the standby current of LCD.
	{0xBA, 7, {0x41,0x93, 0x00, 0x16, 0xA4,0x10,0x18}},	//{0xBA, 2, {0x01,0x92}},
	//{REGFLAG_DELAY, 1, {}},
//>2013/03/21-23086-stevenchen
//>2013/04/10-23689-stevenchen

	{0xB1, 19, {0x00,0x00,0x04,0xE3,	
				0x96,0x10,0x11,0x6F,
				0xEF,0x28,0x30,0x23,
				0x23,0x43,0x00,0x58,
		  		0xF2,0x20,0x00}},
	//{REGFLAG_DELAY, 1, {}},

	{0xB2, 7,   {0x00,0x00,0x78,0x08,0x03,0x00,0x80}},
	//{REGFLAG_DELAY, 1, {}},
	
	{0xB4, 23, {0x80,0x08,0x00,0x32,
				0x10,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,
				0x37,0x0A,0x40,0x04,
				0x37,0x0A,0x40,0x14,
				0x50,0x55,0x0A}}, 
	//{REGFLAG_DELAY, 1, {}},
	
	{0xD5, 48, {0x00,0x00,0x4C,0x00,
				0x01,0x00,0x00,0x00,
				0x60,0x00,0x99,0x88,
				0x99,0x88,0x88,0x32,
				0x88,0x10,0x88,0x76,
				0x88,0x54,0x10,0x32,
				0x88,0x88,0x88,0x88,
				0x88,0x88,0x99,0x88,
				0x99,0x88,0x45,0x88,
				0x67,0x88,0x01,0x88,
				0x23,0x23,0x01,0x88,
				0x88,0x88,0x88,0x88}}, 
	//{REGFLAG_DELAY, 1, {}},
	
//<2013/04/25-24292-stevenchen, [Pelican][drv]Update LCM gamma parameters.
	{0xE0, 34, {0x05,0x14,0x18,0x2D,
				0x34,0x3F,0x1A,0x35,
				0x05,0x09,0x0E,0x11,
				0x13,0x13,0x12,0x18,
				0x1C,0x05,0x14,0x18,
				0x2D,0x34,0x3F,0x1A,
				0x35,0x05,0x09,0x0E,
				0x11,0x13,0x13,0x12,
				0x18,0x1C}},
	//{REGFLAG_DELAY, 1, {}},
//>2013/04/25-24292-stevenchen

	{0xC1, 127, {0x01,0x02,0x08,0x17,
				0x20,0x27,0x2D,0x32,
				0x38,0x40,0x48,0x4F,
				0x57,0x5F,0x67,0x70,
				0x79,0x81,0x89,0x91,
				0x99,0xA1,0xA8,0xB0,
				0xB9,0xC2,0xC9,0xD0,
				0xD9,0xE2,0xE9,0xF3,
				0xFA,0xFF,0xD0,0xAF,
				0xDF,0xA9,0xA9,0x3D,
				0x79,0x92,0xC0,0x02,
				0x08,0x17,0x20,0x27,
				0x2D,0x32,0x38,0x40,
				0x48,0x4F,0x57,0x5F,
				0x67,0x70,0x79,0x81,
				0x89,0x91,0x99,0xA1,
				0xA8,0xB0,0xB9,0xC2,
				0xC9,0xD0,0xD9,0xE2,
				0xE9,0xF3,0xFA,0xFF,
				0xD0,0xAF,0xDF,0xA9,
				0xA9,0x3D,0x79,0x92,
				0xC0,0x02,0x08,0x17,
				0x20,0x27,0x2D,0x32,
				0x38,0x40,0x48,0x4F,
				0x57,0x5F,0x67,0x70,
				0x79,0x81,0x89,0x91,
				0x99,0xA1,0xA8,0xB0,
				0xB9,0xC2,0xC9,0xD0,
				0xD9,0xE2,0xE9,0xF3,
				0xFA,0xFF,0xD0,0xAF,
				0xDF,0xA9,0xA9,0x3D,
				0x79,0x92,0xC0}}, 
	//{REGFLAG_DELAY, 1, {}},

	{0xCC, 1,   {0x0E}},
	//{REGFLAG_DELAY, 1, {}},

	{0xB6, 4,   {0x00,0xA0,0x00,0xA0}},
	//{REGFLAG_DELAY, 1, {}},

	{0xCB, 2,   {0x07,0x07}},
	//{REGFLAG_DELAY, 1, {}},

//<2013/03/21-23086-stevenchen, Lower the standby current of LCD.
	{0xC6, 1,   {0x08}},
	//{REGFLAG_DELAY, 1, {}},
//>2013/03/21-23086-stevenchen

	{0xBB, 4,   {0x00,0x00,0xFF,0x80}},
	//{REGFLAG_DELAY, 1, {}},

	{0x35, 1,   {0x00}},//TE on
	//{REGFLAG_DELAY, 1, {}},

    	// Sleep Out
	{0x11, 0, {0x00}},
    	{REGFLAG_DELAY, 120, {}},

    	// Display ON
	//<<EricHsieh,2013/6/18,Bootup time tuning
	{0x29, 0, {0x00}},
	{REGFLAG_DELAY, 10, {}},
	//>>EricHsieh,2013/6/18,Bootup time tuning

	// Note
	// Strongly recommend not to set Sleep out / Display On here. That will cause messed frame to be shown as later the backlight is on.

	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
//>2013/04/30-24461-stevenchen
//>2013/05/03-24543-stevenchen

//<2013/06/03-25639-stevenchen, [Pelican][drv] Add second source TCL LCM.
static struct LCM_setting_table lcm_tcl_initialization_setting[] = 
{
	//must use 0x39 for init setting for all register.

	{0xB9, 3, {0xFF,0x83,0x89}},

	{0xBA, 7, {0x41,0x93, 0x00, 0x16, 0xA4,0x10,0x18}},

	{0xB1, 19, {0x00,0x00,0x07,0xE8,	
				0x97,0x10,0x11,0xCF,
				0xEF,0x25,0x2D,0x22,
				0x22,0x42,0x01,0x3A,
		  		0xF7,0x20,0x80}},

	{0xC6, 1,   {0x08}},

	{0xB2, 7,   {0x00,0x00,0x78,0x08,0x03,0x3F,0x20}},
	
	{0xB4, 23, {0x80,0x08,0x00,0x32,
				0x10,0x00,0x00,0x00,
				0x00,0x00,0x00,0x00,
				0x37,0x0A,0x40,0x04,
				0x37,0x0A,0x48,0x14,
				0x50,0x58,0x0A}}, 
	
	{0xD5, 48, {0x00,0x00,0x4C,0x00,
				0x01,0x00,0x00,0x00,
				0x60,0x00,0x99,0x88,
				0x99,0x88,0x88,0x32,
				0x88,0x10,0x88,0x76,
				0x88,0x54,0x10,0x32,
				0x88,0x88,0x88,0x88,
				0x88,0x88,0x99,0x88,
				0x99,0x88,0x45,0x88,
				0x67,0x88,0x01,0x88,
				0x23,0x23,0x01,0x88,
				0x88,0x88,0x88,0x88}}, 
//<2013/07/04-26584-stevenchen, [5860][drv] Update second source TCL LCM gamma.	
	{0xE0, 34, {0x04,0x0C,0x0F,0x2D,
				0x34,0x3F,0x17,0x36,
				0x07,0x0C,0x0E,0x10,
				0x13,0x11,0x12,0x18,
				0x1C,0x04,0x0C,0x0F,
				0x2D,0x34,0x3F,0x17,
				0x36,0x07,0x0C,0x0E,
				0x10,0x13,0x11,0x12,
				0x18,0x1C}},
//>2013/07/04-26584-stevenchen
	{0xCC, 1,   {0x0E}},

	{0xB6, 4,   {0x00,0xA0,0x00,0xA0}},

	{0x3A, 1,   {0x77}},

	{0x35, 1,   {0x00}},//TE on

    	// Sleep Out
	{0x11, 0, {0x00}},
    	{REGFLAG_DELAY, 150, {}},

    	// Display ON
	{0x29, 0, {0x00}},
	{REGFLAG_DELAY, 10, {}},

	// Setting ending by predefined flag
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
//>2013/06/03-25639-stevenchen

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

    for(i = 0; i < count; i++) {
		
        unsigned cmd;
        cmd = table[i].cmd;
		
        switch (cmd) {
			
            case REGFLAG_DELAY :
                MDELAY(table[i].count);
                break;
				
            case REGFLAG_END_OF_TABLE :
                break;
				
            default:
				dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
       	}
    }
	
}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}


static void lcm_get_params(LCM_PARAMS *params)
{

		memset(params, 0, sizeof(LCM_PARAMS));
	
		params->type   = LCM_TYPE_DSI;

		params->width  = FRAME_WIDTH;
		params->height = FRAME_HEIGHT;

		// enable tearing-free
		params->dbi.te_mode 			= LCM_DBI_TE_MODE_VSYNC_ONLY;
		params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;

        #if (LCM_DSI_CMD_MODE)
		params->dsi.mode   = CMD_MODE;
        #else
		//params->dsi.mode   = SYNC_PULSE_VDO_MODE;
		//params->dsi.mode   = BURST_VDO_MODE;
		params->dsi.mode   = SYNC_EVENT_VDO_MODE; 
        #endif
	
		// DSI
		/* Command mode setting */
		params->dsi.LANE_NUM				= LCM_TWO_LANE;
		//The following defined the fomat for data coming from LCD engine.
		params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
		params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
		params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
		params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

		// Highly depends on LCD driver capability.
		// Not support in MT6573
		params->dsi.packet_size=256;

		// Video mode setting		
		params->dsi.intermediat_buffer_num = 0;//because DSI/DPI HW design change, this parameters should be 0 when video mode in MT658X; or memory leakage

		params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
		params->dsi.word_count=540*3;	

		
		params->dsi.vertical_sync_active				= 3;  //---3
		params->dsi.vertical_backporch					= 5;  //---5
		params->dsi.vertical_frontporch					= 5;  //---5
		params->dsi.vertical_active_line				= FRAME_HEIGHT; 

		params->dsi.horizontal_sync_active				= 24; //---96
		params->dsi.horizontal_backporch				= 40; //---96
		params->dsi.horizontal_frontporch				= 24; //---48
		params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

		// Bit rate calculation
		//1 Every lane speed
		params->dsi.pll_div1=0;		// div1=0,1,2,3;div1_real=1,2,4,4 ----
		params->dsi.pll_div2=1;		// div2=0,1,2,3;div1_real=1,2,4,4	
//<2013/04/13-23820-stevenchen, [Pelican][drv] Fix RF de-sense issue caused by LCM MIPI.
//<2013/03/14-22797-stevenchen, Fine tune LCM performance.(Data rate up to 546Mbps) 
		params->dsi.fbk_div =19;    // fref=26MHz, fvco=fref*(fbk_div+1)*2/(div1_real*div2_real)	
//>2013/03/14-22797-stevenchen
//>2013/04/13-23820-stevenchen

		params->dsi.compatibility_for_nvk = 0;
}

static unsigned int lcm_compare_id(void);
static void lcm_init(void)
{
	unsigned int data_array[16];

    	lcm_compare_id();

//<2013/06/03-25639-stevenchen, [Pelican][drv] Add second source TCL LCM.
	lcm_id = mt_get_gpio_in(GPIO_LCM_ID_PIN);

	#ifdef BUILD_LK
       	printf("LCM_ID = %d \n", lcm_id);
	#endif
//>2013/06/03-25639-stevenchen	

//<2013/04/12-23801-stevenchen, [Pelican][drv] Implement esd recovery function.
//<<EricHsieh,2013/6/18,Bootup time tuning
	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);//1
	
	SET_RESET_PIN(1);
	MDELAY(10);    //20
//>>EricHsieh,2013/6/18,Bootup time tuning
//>2013/04/12-23801-stevenchen

#if 0
	init_lcm_registers();
#else
    //<2013/06/03-25639-stevenchen, [Pelican][drv] Add second source TCL LCM.
    if( lcm_id == 1 )
    {
	push_table(lcm_tcl_initialization_setting, sizeof(lcm_tcl_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    }
    else
    {
	push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    }
    //>2013/06/03-25639-stevenchen
#endif
}

//<2013/04/12-23797-stevenchen, [Pelican][drv] Fix the adb command of turning on/off LCM.
//<2013/04/15-23830-stevenchen, [Pelican][drv] Fix the adb command of turning on/off LCM.
static void lcm_poweron(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00110500; // Sleep Out
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(120);
	
	data_array[0] = 0x00290500; // Display On
	dsi_set_cmdq(&data_array, 1, 1); 
	MDELAY(100);
}
//>2013/04/15-23830-stevenchen

static void lcm_poweroff(void)
{
	unsigned int data_array[16];

	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(data_array, 1, 1);
	MDELAY(120);
}
//>2013/04/12-23797-stevenchen

static void lcm_suspend(void)
{
	unsigned int data_array[16];
//<2013/06/03-25639-stevenchen, [Pelican][drv] Add second source TCL LCM.
	static GetLcmId=0;
	
	if(GetLcmId == 0)
	{
		lcm_id = mt_get_gpio_in(GPIO_LCM_ID_PIN);
		GetLcmId=1;
	}
//>2013/06/03-25639-stevenchen

//<2013/03/21-23086-stevenchen, Lower the standby current of LCD.
	//data_array[0]=0x00280500; // Display Off
	//dsi_set_cmdq(&data_array, 1, 1);
	//MDELAY(10);
//>2013/03/21-23086-stevenchen	

	data_array[0] = 0x00100500; // Sleep In
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(120);

}


//<2013/04/12-23801-stevenchen, [Pelican][drv] Implement esd recovery function.
static void lcm_resume(void)
{
	unsigned int data_array[16];
#if 1
//<2013/06/03-25639-stevenchen, [Pelican][drv] Add second source TCL LCM.
    	if(lcm_id==1)
    	{
	    push_table(lcm_tcl_initialization_setting, sizeof(lcm_tcl_initialization_setting) / sizeof(struct LCM_setting_table), 1);
    	}
	else
	{
	    push_table(lcm_initialization_setting, sizeof(lcm_initialization_setting) / sizeof(struct LCM_setting_table), 1);
	}
//>2013/06/03-25639-stevenchen
#else
	data_array[0] = 0x00110500; // Sleep Out
	dsi_set_cmdq(&data_array, 1, 1);
	MDELAY(120);	//200
	
	data_array[0] = 0x00290500; // Display On
	dsi_set_cmdq(&data_array, 1, 1); 
	MDELAY(100);	//40
#endif
}
//>2013/04/12-23801-stevenchen         

static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	dsi_set_cmdq(&data_array, 3, 1);
	
	data_array[0]= 0x00053902;
	data_array[1]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[2]= (y1_LSB);
	dsi_set_cmdq(&data_array, 3, 1);

	data_array[0]= 0x00290508; //HW bug, so need send one HS packet
	dsi_set_cmdq(&data_array, 1, 1);
	
	data_array[0]= 0x002c3909;
	dsi_set_cmdq(&data_array, 1, 0);

}


static unsigned int lcm_compare_id(void)
{
   
	unsigned int id=0;
   	unsigned char buffer[2];
   	unsigned int array[16];	
   
    	SET_RESET_PIN(1);
	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
	MDELAY(10);//Must over 6 ms

	array[0]=0x00043902;
	array[1]=0x8983FFB9;// page enable
	dsi_set_cmdq(&array, 2, 1);
	MDELAY(10);
   
	array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	MDELAY(10);
   
	read_reg_v2(0xF4, buffer, 2);	//0xF4
	id = buffer[0]; //0
	   
    	#ifdef BUILD_LK
		printf("%s, LK hx8389b debug: id = 0x%08x\n", __func__, id);
	#else
		printk("%s, kernel hx8389b debug: id = 0x%08x\n", __func__, id);
    	#endif
   
	return (LCM_ID_HX8389B == id)?1:0;
}

//<2013/04/12-23801-stevenchen, [Pelican][drv] Implement esd recovery function.
static int count_n=0;
static unsigned int lcm_esd_check(void)
{
  #ifndef BUILD_LK
	char  buffer[3];
	int   array[4];

	count_n++;
	if(count_n==2)
		count_n=0;
	else
		return FALSE;

	if(lcm_esd_test)
	{
		lcm_esd_test = FALSE;
		return TRUE;
	}

	array[0]=0x00043902;
	array[1]=0x8983FFB9;// page enable
	dsi_set_cmdq(&array, 2, 1);
	//MDELAY(10);
   
	array[0] = 0x00023700;// return byte number
	dsi_set_cmdq(&array, 1, 1);
	//MDELAY(10);
   
	read_reg_v2(0x0A, buffer, 2); //Get power mode
	if(buffer[1]==0x1C)
	{
		return FALSE;
	}
	else
	{			 
		return TRUE;
	}
   #endif

}

static unsigned int lcm_esd_recover(void)
{
	lcm_init();
	//lcm_resume();

	return TRUE;
}
//>2013/04/12-23801-stevenchen


LCM_DRIVER hx8389b_qhd_dsi_vdo_byd_lcm_drv = 
{
    .name			= "hx8389b_qhd_dsi_vdo_byd",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
//<2013/04/12-23801-stevenchen, [Pelican][drv] Implement esd recovery function.
	.esd_check      = lcm_esd_check,
	.esd_recover    = lcm_esd_recover,
//>2013/04/12-23801-stevenchen

    	.compare_id    = lcm_compare_id,
#if (LCM_DSI_CMD_MODE)
    	.update         = lcm_update,
#endif
//<2013/04/12-23797-stevenchen, [Pelican][drv] Fix the adb command of turning on/off LCM.
	.poweron	= lcm_poweron,
	.poweroff	= lcm_poweroff
//>2013/04/12-23797-stevenchen
};
