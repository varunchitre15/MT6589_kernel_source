#ifndef __ARCH_ARM_MACH_MT6575_CUSTOM_BOARD_H
#define __ARCH_ARM_MACH_MT6575_CUSTOM_BOARD_H

#include <linux/autoconf.h>
#include "mach/mt_freqhopping.h"

/*=======================================================================*/
/* MT6589 MSDC                                                             */
/*=======================================================================*/
#ifdef MTK_EMMC_SUPPORT
#define CFG_DEV_MSDC0
#endif
#define CFG_DEV_MSDC1
//#define CFG_DEV_MSDC2
#define CFG_DEV_MSDC3
//#define CFG_DEV_MSDC4
#if defined(CONFIG_MTK_COMBO) || defined(CONFIG_MTK_COMBO_MODULE)
/*
SDIO slot index number used by connectivity combo chip:
0: invalid (used by memory card)
1: MSDC1
2: MSDC2
*/
#define CONFIG_MTK_WCN_CMB_SDIO_SLOT  (3) /* MSDC3 */
#else
#undef CONFIG_MTK_WCN_CMB_SDIO_SLOT
#endif

#if 0 /* FIXME. */
/*=======================================================================*/
/* MT6575 UART                                                           */
/*=======================================================================*/
#define CFG_DEV_UART1
#define CFG_DEV_UART2
#define CFG_DEV_UART3
#define CFG_DEV_UART4

#define CFG_UART_PORTS          (4)

/*=======================================================================*/
/* MT6575 I2C                                                            */
/*=======================================================================*/
#define CFG_DEV_I2C
//#define CFG_I2C_HIGH_SPEED_MODE
//#define CFG_I2C_DMA_MODE

/*=======================================================================*/
/* MT6575 ADB                                                            */
/*=======================================================================*/
#define ADB_SERIAL "E1K"

#endif

/*=======================================================================*/
/* MT6575 NAND FLASH                                                     */
/*=======================================================================*/
#if 0
#define RAMDOM_READ 1<<0
#define CACHE_READ  1<<1
/*******************************************************************************
 * NFI & ECC Configuration 
 *******************************************************************************/
typedef struct
{
    u16 id;			//deviceid+menuid
    u8  addr_cycle;
    u8  iowidth;
    u16 totalsize;	
    u16 blocksize;
    u16 pagesize;
    u32 timmingsetting;
    char devciename[14];
    u32 advancedmode;   //
}flashdev_info,*pflashdev_info;

static const flashdev_info g_FlashTable[]={
    //micro
    {0xAA2C,  5,  8,  256,	128,  2048,  0x01113,  "MT29F2G08ABD",	0},
    {0xB12C,  4,  16, 128,	128,  2048,  0x01113,  "MT29F1G16ABC",	0},
    {0xBA2C,  5,  16, 256,	128,  2048,  0x01113,  "MT29F2G16ABD",	0}, 
    {0xAC2C,  5,  8,  512,	128,  2048,  0x01113,  "MT29F4G08ABC",	0},
    {0xBC2C,  5,  16, 512,	128,  2048,  0x44333,  "MT29F4G16ABD",	0},
    //samsung 
    {0xBAEC,  5,  16, 256,	128,  2048,  0x01123,  "K522H1GACE",	0},
    {0xBCEC,  5,  16, 512,	128,  2048,  0x01123,  "K524G2GACB",	0},
    {0xDAEC,  5,  8,  256,	128,  2048,  0x33222,  "K9F2G08U0A",	RAMDOM_READ},
    {0xF1EC,  4,  8,  128,	128,  2048,  0x01123,  "K9F1G08U0A",	RAMDOM_READ},
    {0xAAEC,  5,  8,  256,	128,  2048,  0x01123,  "K9F2G08R0A",	0},
    //hynix
    {0xD3AD,  5,  8,  1024, 256,  2048,  0x44333,  "HY27UT088G2A",	0},
    {0xA1AD,  4,  8,  128,	128,  2048,  0x01123,  "H8BCSOPJOMCP",	0},
    {0xBCAD,  5,  16, 512,	128,  2048,  0x01123,  "H8BCSOUNOMCR",	0},
    {0xBAAD,  5,  16, 256,	128,  2048,  0x01123,  "H8BCSOSNOMCR",	0},
    //toshiba
    {0x9598,  5,  16, 816,	128,  2048,  0x00113,  "TY9C000000CMG", 0},
    {0x9498,  5,  16, 375,	128,  2048,  0x00113,  "TY9C000000CMG", 0},
    {0xC198,  4,  16, 128,	128,  2048,  0x44333,  "TC58NWGOS8C",	0},
    {0xBA98,  5,  16, 256,	128,  2048,  0x02113,  "TC58NYG1S8C",	0},
    //st-micro
    {0xBA20,  5,  16, 256,	128,  2048,  0x01123,  "ND02CGR4B2DI6", 0},

    // elpida
    {0xBC20,  5,  16, 512,  128,  2048,  0x01123,  "04GR4B2DDI6",   0},
    {0x0000,  0,  0,  0,	0,	  0,	 0, 	   "xxxxxxxxxxxxx", 0}
};
#endif
	
	
#define NFI_DEFAULT_ACCESS_TIMING        (0x44333)

//uboot only support 1 cs
#define NFI_CS_NUM                  (2)
#define NFI_DEFAULT_CS				(0)

#define USE_AHB_MODE                	(1)
#define PER_PROJECT_FH_SETTING \
static  fh_pll_t g_fh_pll[MT658X_FH_PLL_TOTAL_NUM] = { \
	{FH_FH_DISABLE,     FH_PLL_ENABLE   , 0, 0       , 0}, \
	{FH_FH_DISABLE,     FH_PLL_ENABLE   , 0, 1612000 , 0}, \
	{FH_FH_ENABLE_SSC,  FH_PLL_ENABLE   , 0, 266000  , 0}, \
	{FH_FH_DISABLE,     FH_PLL_ENABLE   , 0, 1599000 , 0}, \
	{FH_FH_DISABLE,     FH_PLL_ENABLE   , 0, 1188000 , 0}, \
	{FH_FH_ENABLE_SSC,     FH_PLL_ENABLE  , 0, 1664000 , 0}  \
}; \
static const struct freqhopping_ssc mt_ssc_armpll_setting[MT_SSC_NR_PREDEFINE_SETTING]= { \
	{0,0,0,0,0,0}, \
	{0,0xFF,0xFF,0xFF,0xFF,0xFF}, \
	{1209000 ,0 ,9 ,0, 0x770A, 0xBA000}, \
	{1001000 ,0 ,9 ,0, 0x628F, 0x9A000}, \
	{ 715000 ,0 ,9 ,0, 0x8CCC, 0xDC000}, \
	{ 419250 ,0 ,9 ,0, 0xA51E, 0x102000}, \
	{0,0,0,0,0,0} \
}; \
static const struct freqhopping_ssc mt_ssc_mainpll_setting[MT_SSC_NR_PREDEFINE_SETTING]= { \
	{0,0,0,0,0,0}, \
	{0,0xFF,0xFF,0xFF,0xFF,0xFF}, \
	{1612000 ,0 ,9 ,0x4f5c, 0x4f5c, 0xF8000}, \
	{0,0,0,0,0,0} \
}; \
static const struct freqhopping_ssc mt_ssc_mempll_setting[MT_SSC_NR_PREDEFINE_SETTING]= { \
	{0,0,0,0,0,0}, \
	{0,0xFF,0xFF,0xFF,0xFF,0xFF}, \
	{266000 ,0 ,7 ,0, 0x61CD, 0x00131A2E}, \
	{233500 ,0 ,7 ,0, 0x55C2, 0x0010C000}, \
	{208500 ,0 ,7 ,0, 0x4CA8, 0x000EF8F5}, \
	{200000 ,0 ,7 ,0, 0x4989, 0x000E5CCC}, \
	{0,0,0,0,0,0} \
}; \
static const struct freqhopping_ssc mt_ssc_msdcpll_setting[MT_SSC_NR_PREDEFINE_SETTING]= { \
	{0,0,0,0,0,0}, \
	{0,0xFF,0xFF,0xFF,0xFF,0xFF}, \
	{1599000, 0, 9, 0, 0x9d70, 0xF6000}, \
	{1352000, 0, 9, 0, 0x851e, 0xD0000}, \
	{0,0,0,0,0,0} \
}; \
static const struct freqhopping_ssc mt_ssc_tvdpll_setting[MT_SSC_NR_PREDEFINE_SETTING]= { \
	{0,0,0,0,0,0}, \
	{0,0xFF,0xFF,0xFF,0xFF,0xFF}, \
	{1188000 ,0 ,9 ,0, 0x74f8, 0xB6C4F}, \
	{1728000, 0, 9, 0, 0xaa24, 0x109D89},  \
	{0,0,0,0,0,0} \
}; \
static const struct freqhopping_ssc mt_ssc_lvdspll_setting[MT_SSC_NR_PREDEFINE_SETTING]= { \
	{0,0,0,0,0,0}, \
	{0,0xFF,0xFF,0xFF,0xFF,0xFF}, \
	{1664000, 0, 9, 0, 0x51EB, 0x80000}, \
	{1400000, 0, 9, 0, 0x8dc8, 0xDD89D}, \
	{0,0,0,0,0,0} \
}; \
static struct freqhopping_ssc mt_ssc_fhpll_userdefined[MT_FHPLL_MAX]= { \
	{0,1,1,2,2,0}, \
	{0,1,1,2,2,0}, \
	{0,1,1,2,2,0}, \
	{0,1,1,2,2,0}, \
	{0,1,1,2,2,0}, \
	{0,1,1,2,2,0} \
}; \


//for FH
#define MT_FH_DUMMY_READ	0
#define LOW_DRAMC_DDS		0x0010C000
#define LOW_DRAMC_INT		67 //233.5
#define LOW_DRAMC_FRACTION	0 //233.5
#define LOW_DRAMC		233 //233.5
#define LOW_DRAMC_FREQ		233500
#define LVDS_PLL_IS_ON		1
#define LVDS_SSC			8

#endif /* __ARCH_ARM_MACH_MT6575_CUSTOM_BOARD_H */

