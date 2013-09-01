#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cnt32_to_63.h>
#include <linux/proc_fs.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>

#include <mach/timer.h>
#include <mach/irqs.h>
#include <asm/uaccess.h>

#include <mach/pmic_mt6320_sw.h>
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>
#include <mach/mt_pm_ldo.h>

#define pmicname	"uvvp_pmic"

/*
	This is a Kernel driver used by user space. 
	This driver will using the interface of the PMIC production driver.  
	We implement some IOCTLs: 
*/
#define	UVVP_PMIC_GET_VERSION				_IOW('k', 0, int)
#define	UVVP_PMIC_GET_PCHR_CHRDET			_IOW('k', 1, int)

#define	UVVP_PMIC_VERIFY_DEFAULT_VALUE		_IOW('k', 2, int)
#define	UVVP_PMIC_TOP_WR_1                  _IOW('k', 3, int)
#define	UVVP_PMIC_TOP_WR_2                  _IOW('k', 4, int)

#define	UVVP_PMIC_AUXADC_611                _IOW('k', 5, int)
#define	UVVP_PMIC_AUXADC_612                _IOW('k', 6, int)
#define	UVVP_PMIC_AUXADC_621                _IOW('k', 7, int)
#define	UVVP_PMIC_AUXADC_622                _IOW('k', 8, int)

#define	UVVP_PMIC_INT_451                   _IOW('k', 9, int)
#define	UVVP_PMIC_INT_461                   _IOW('k', 10, int)
#define	UVVP_PMIC_INT_462                   _IOW('k', 11, int)
#define	UVVP_PMIC_INT_463                   _IOW('k', 12, int)
#define	UVVP_PMIC_INT_464                   _IOW('k', 13, int)
#define	UVVP_PMIC_INT_465                   _IOW('k', 14, int)
#define	UVVP_PMIC_INT_466                   _IOW('k', 15, int)
#define	UVVP_PMIC_INT_467                   _IOW('k', 16, int)
#define	UVVP_PMIC_INT_468                   _IOW('k', 17, int)

#define	UVVP_PMIC_LDO_ON_OFF_0              _IOW('k', 18, int)
#define	UVVP_PMIC_LDO_ON_OFF_1              _IOW('k', 19, int)
#define	UVVP_PMIC_LDO_ON_OFF_2              _IOW('k', 20, int)
#define	UVVP_PMIC_LDO_ON_OFF_3              _IOW('k', 21, int)
#define	UVVP_PMIC_LDO_ON_OFF_4              _IOW('k', 22, int)
#define	UVVP_PMIC_LDO_ON_OFF_5              _IOW('k', 23, int)
#define	UVVP_PMIC_LDO_ON_OFF_6              _IOW('k', 24, int)
#define	UVVP_PMIC_LDO_ON_OFF_7              _IOW('k', 25, int)
#define	UVVP_PMIC_LDO_ON_OFF_8              _IOW('k', 26, int)
#define	UVVP_PMIC_LDO_ON_OFF_9              _IOW('k', 27, int)
#define	UVVP_PMIC_LDO_ON_OFF_10             _IOW('k', 28, int)
#define	UVVP_PMIC_LDO_ON_OFF_11             _IOW('k', 29, int)
#define	UVVP_PMIC_LDO_ON_OFF_12             _IOW('k', 30, int)
#define	UVVP_PMIC_LDO_ON_OFF_13             _IOW('k', 31, int)
#define	UVVP_PMIC_LDO_ON_OFF_14             _IOW('k', 32, int)
#define	UVVP_PMIC_LDO_ON_OFF_15             _IOW('k', 33, int)
#define	UVVP_PMIC_LDO_ON_OFF_16             _IOW('k', 34, int)
#define	UVVP_PMIC_LDO_ON_OFF_17             _IOW('k', 35, int)
#define	UVVP_PMIC_LDO_ON_OFF_18             _IOW('k', 36, int)
#define	UVVP_PMIC_LDO_ON_OFF_19             _IOW('k', 37, int)
#define	UVVP_PMIC_LDO_ON_OFF_20             _IOW('k', 38, int)
#define	UVVP_PMIC_LDO_ON_OFF_21             _IOW('k', 39, int)
#define	UVVP_PMIC_LDO_ON_OFF_22             _IOW('k', 40, int)
#define	UVVP_PMIC_LDO_ON_OFF_23             _IOW('k', 41, int)

#define	UVVP_PMIC_LDO_VOSEL_0              _IOW('k', 42, int)
#define	UVVP_PMIC_LDO_VOSEL_1              _IOW('k', 43, int)
#define	UVVP_PMIC_LDO_VOSEL_2              _IOW('k', 44, int)
#define	UVVP_PMIC_LDO_VOSEL_3              _IOW('k', 45, int)
#define	UVVP_PMIC_LDO_VOSEL_4              _IOW('k', 46, int)
#define	UVVP_PMIC_LDO_VOSEL_5              _IOW('k', 47, int)
#define	UVVP_PMIC_LDO_VOSEL_6              _IOW('k', 48, int)
#define	UVVP_PMIC_LDO_VOSEL_7              _IOW('k', 49, int)
#define	UVVP_PMIC_LDO_VOSEL_8              _IOW('k', 50, int)
#define	UVVP_PMIC_LDO_VOSEL_9              _IOW('k', 51, int)
#define	UVVP_PMIC_LDO_VOSEL_10             _IOW('k', 52, int)
#define	UVVP_PMIC_LDO_VOSEL_11             _IOW('k', 53, int)
#define	UVVP_PMIC_LDO_VOSEL_12             _IOW('k', 54, int)
#define	UVVP_PMIC_LDO_VOSEL_13             _IOW('k', 55, int)
#define	UVVP_PMIC_LDO_VOSEL_14             _IOW('k', 56, int)
#define	UVVP_PMIC_LDO_VOSEL_15             _IOW('k', 57, int)
#define	UVVP_PMIC_LDO_VOSEL_16             _IOW('k', 58, int)
#define	UVVP_PMIC_LDO_VOSEL_17             _IOW('k', 59, int)
#define	UVVP_PMIC_LDO_VOSEL_18             _IOW('k', 60, int)
#define	UVVP_PMIC_LDO_VOSEL_19             _IOW('k', 61, int)
#define	UVVP_PMIC_LDO_VOSEL_20             _IOW('k', 62, int)
#define	UVVP_PMIC_LDO_VOSEL_21             _IOW('k', 63, int)
#define	UVVP_PMIC_LDO_VOSEL_22             _IOW('k', 64, int)
#define	UVVP_PMIC_LDO_VOSEL_23             _IOW('k', 65, int)

#define	UVVP_PMIC_LDO_CAL_0              _IOW('k', 66, int)
#define	UVVP_PMIC_LDO_CAL_1              _IOW('k', 67, int)
#define	UVVP_PMIC_LDO_CAL_2              _IOW('k', 68, int)
#define	UVVP_PMIC_LDO_CAL_3              _IOW('k', 69, int)
#define	UVVP_PMIC_LDO_CAL_4              _IOW('k', 70, int)
#define	UVVP_PMIC_LDO_CAL_5              _IOW('k', 71, int)
#define	UVVP_PMIC_LDO_CAL_6              _IOW('k', 72, int)
#define	UVVP_PMIC_LDO_CAL_7              _IOW('k', 73, int)
#define	UVVP_PMIC_LDO_CAL_8              _IOW('k', 74, int)
#define	UVVP_PMIC_LDO_CAL_9              _IOW('k', 75, int)
#define	UVVP_PMIC_LDO_CAL_10             _IOW('k', 76, int)
#define	UVVP_PMIC_LDO_CAL_11             _IOW('k', 77, int)
#define	UVVP_PMIC_LDO_CAL_12             _IOW('k', 78, int)
#define	UVVP_PMIC_LDO_CAL_13             _IOW('k', 79, int)
#define	UVVP_PMIC_LDO_CAL_14             _IOW('k', 80, int)
#define	UVVP_PMIC_LDO_CAL_15             _IOW('k', 81, int)
#define	UVVP_PMIC_LDO_CAL_16             _IOW('k', 82, int)
#define	UVVP_PMIC_LDO_CAL_17             _IOW('k', 83, int)
#define	UVVP_PMIC_LDO_CAL_18             _IOW('k', 84, int)
#define	UVVP_PMIC_LDO_CAL_19             _IOW('k', 85, int)
#define	UVVP_PMIC_LDO_CAL_20             _IOW('k', 86, int)
#define	UVVP_PMIC_LDO_CAL_21             _IOW('k', 87, int)
#define	UVVP_PMIC_LDO_CAL_22             _IOW('k', 88, int)
#define	UVVP_PMIC_LDO_CAL_23             _IOW('k', 89, int)

#define	UVVP_PMIC_BUCK_ON_OFF_0              _IOW('k', 90, int)
#define	UVVP_PMIC_BUCK_ON_OFF_1              _IOW('k', 91, int)
#define	UVVP_PMIC_BUCK_ON_OFF_2              _IOW('k', 92, int)
#define	UVVP_PMIC_BUCK_ON_OFF_3              _IOW('k', 93, int)
#define	UVVP_PMIC_BUCK_ON_OFF_4              _IOW('k', 94, int)
#define	UVVP_PMIC_BUCK_ON_OFF_5              _IOW('k', 95, int)
#define	UVVP_PMIC_BUCK_ON_OFF_6              _IOW('k', 96, int)
#define	UVVP_PMIC_BUCK_ON_OFF_7              _IOW('k', 97, int)

#define	UVVP_PMIC_BUCK_VOSEL_0              _IOW('k', 98, int)
#define	UVVP_PMIC_BUCK_VOSEL_1              _IOW('k', 99, int)
#define	UVVP_PMIC_BUCK_VOSEL_2              _IOW('k', 100, int)
#define	UVVP_PMIC_BUCK_VOSEL_3              _IOW('k', 101, int)
#define	UVVP_PMIC_BUCK_VOSEL_4              _IOW('k', 102, int)
#define	UVVP_PMIC_BUCK_VOSEL_5              _IOW('k', 103, int)
#define	UVVP_PMIC_BUCK_VOSEL_6              _IOW('k', 104, int)
#define	UVVP_PMIC_BUCK_VOSEL_7              _IOW('k', 105, int)

#define	UVVP_PMIC_BUCK_DLC_0              _IOW('k', 106, int)
#define	UVVP_PMIC_BUCK_DLC_1              _IOW('k', 107, int)
#define	UVVP_PMIC_BUCK_DLC_2              _IOW('k', 108, int)
#define	UVVP_PMIC_BUCK_DLC_3              _IOW('k', 109, int)
#define	UVVP_PMIC_BUCK_DLC_4              _IOW('k', 110, int)
#define	UVVP_PMIC_BUCK_DLC_5              _IOW('k', 111, int)
#define	UVVP_PMIC_BUCK_DLC_6              _IOW('k', 112, int)
#define	UVVP_PMIC_BUCK_DLC_7              _IOW('k', 113, int)

#define	UVVP_PMIC_BUCK_BURST_0              _IOW('k', 114, int)
#define	UVVP_PMIC_BUCK_BURST_1              _IOW('k', 115, int)
#define	UVVP_PMIC_BUCK_BURST_2              _IOW('k', 116, int)
#define	UVVP_PMIC_BUCK_BURST_3              _IOW('k', 117, int)
#define	UVVP_PMIC_BUCK_BURST_4              _IOW('k', 118, int)
#define	UVVP_PMIC_BUCK_BURST_5              _IOW('k', 119, int)
#define	UVVP_PMIC_BUCK_BURST_6              _IOW('k', 120, int)
#define	UVVP_PMIC_BUCK_BURST_7              _IOW('k', 121, int)


/*Define for test*/
#define PMIC6320_E1_CID_CODE    0x1020
#define PMIC6320_E2_CID_CODE    0x2020

/*Externs*/
extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);

/*Define for default value test */
static kal_uint32 aPMURegBeg_bank0[][2]= {  /* Register*//*reset val*/																
    {0x0,0x0001},
    {0x2,0x00F2},
    {0x4,0x0004},
    {0x6,0x0000},
    {0x8,0x000F},
    {0xA,0x0000},
    {0xC,0x0001},
    {0xE,0x0001},
    {0x10,0x0000},
    {0x12,0x0000},
    {0x14,0x0020},
    {0x16,0x0000},
    {0x18,0x0000},
    {0x1A,0x0010},
    {0x1C,0x0000},
    {0x1E,0x0000},
    {0x20,0x0001},
    {0x22,0x0000},
    {0x24,0x0000},
    {0x26,0x0000},
    {0x28,0x0022},
    {0x2A,0x0024},
    {0x2C,0x0044},
    {0x2E,0x0010},
    {0x30,0x0000},
    {0x32,0x0000},
    {0x34,0x0000},
    {0x36,0x0000},
    {0x38,0x0055},
    {0x3A,0x0000},
    {0x100,0x1020},
    {0x102,0xC6EF},
    {0x108,0x8002},
    {0x10E,0xFFFF},
    {0x114,0x0000},
    {0x11A,0x00FF},
    {0x120,0x007F},
    {0x126,0x0011},
    {0x128,0x001A},
    {0x12A,0x0401},
    {0x12C,0x0000},
    {0x12E,0x0000},
    {0x130,0x00F0},
    {0x132,0x0001},
    {0x134,0x0000},
    {0x136,0x0000},
    {0x138,0x0000},
    {0x13A,0x0F00},
    {0x13C,0x0000},
    {0x13E,0x0000},
    {0x140,0x0000},
    {0x142,0x0000},
    {0x144,0x0000},
    {0x146,0x0000},
    {0x148,0x0000},
    {0x14A,0x0000},
    {0x14C,0x0000},
    {0x14E,0x0000},
    {0x150,0x0000},
    {0x152,0x0000},
    {0x154,0x0000},
    {0x156,0x0000},
    {0x158,0x0000},
    {0x15A,0x0000},
    {0x15C,0x0000},
    {0x15E,0xCCCC},
    {0x160,0xCCCC},
    {0x162,0xCCCC},
    {0x164,0xCCCC},
    {0x166,0xCCCC},
    {0x168,0xCCCC},
    {0x16A,0xCCCC},
    {0x16C,0xCCCC},
    {0x16E,0xCCCC},
    {0x170,0xCCCC},
    {0x172,0xCCCC},
    {0x174,0xCCCC},
    {0x176,0x00CC},
    {0x178,0xF300},
    {0x17E,0x0001},
    {0x184,0x0000},
    {0x186,0x0000},
    {0x188,0x0000},
    {0x18A,0x0000},
    {0x18C,0x0000},
    {0x18E,0x0000},
    {0x190,0x0000},
    {0x192,0x0001},
    {0x194,0x0000},
    {0x196,0x0000},
    {0x198,0x0000},
    {0x19A,0x0000},
    {0x19C,0x0000},
    {0x19E,0x0000},
    {0x1A0,0x0000},
    {0x1A2,0x0000},
    {0x1A4,0x0000},
    {0x1A6,0x0000},
    {0x1A8,0x0000},
    {0x1AA,0x0000},
    {0x1AC,0x0000},
    {0x1AE,0x0000},
    {0x1B0,0x0000},
    {0x1B2,0x0000},
    {0x1B4,0x0000},
    {0x1B6,0x0000},
    {0x1B8,0x0000},
    {0x1BA,0x0000},
    {0x1BC,0x0000},
    {0x1BE,0x0000},
    {0x1C0,0x0000},
    {0x1C2,0x0000},
    {0x1C4,0x0000},
    {0x1C6,0x0000},
    {0x1C8,0x0000},
    {0x1CA,0x0000},
    {0x1CC,0x0000},
    {0x200,0x0000},
    {0x202,0x0000},
    {0x204,0x0000},
    {0x206,0x0000},
    {0x208,0x0383},
    {0x20A,0x0200},
    {0x20C,0x0013},
    {0x20E,0x00F0},
    {0x210,0x0000},
    {0x212,0x0000},
    {0x214,0x3001},
    {0x216,0x0808},
    {0x218,0x0048},
    {0x21A,0x0048},
    {0x21C,0x0048},
    {0x21E,0x0048},
    {0x220,0x1111},
    {0x222,0x3333},
    {0x224,0x3333},
    {0x226,0x0000},
    {0x228,0x0000},
    {0x22A,0x0001},
    {0x22C,0x0000},
    {0x22E,0x0000},
    {0x230,0x0200},
    {0x232,0x0010},
    {0x234,0x0000},
    {0x236,0x0000},
    {0x238,0x0000},
    {0x23A,0x3001},
    {0x23C,0x0808},
    {0x23E,0x0048},
    {0x240,0x0048},
    {0x242,0x0048},
    {0x244,0x0048},
    {0x246,0x0000},
    {0x248,0x0000},
    {0x24A,0x0000},
    {0x24C,0x0000},
    {0x24E,0x0000},
    {0x250,0x0001},
    {0x252,0x0000},
    {0x254,0x0000},
    {0x256,0x0000},
    {0x258,0x0000},
    {0x25A,0x0383},
    {0x25C,0x0200},
    {0x25E,0x0013},
    {0x260,0x00F0},
    {0x262,0x0000},
    {0x264,0x0000},
    {0x266,0x3001},
    {0x268,0x0808},
    {0x26A,0x0038},
    {0x26C,0x0038},
    {0x26E,0x0038},
    {0x270,0x0038},
    {0x272,0x1111},
    {0x274,0x3333},
    {0x276,0x3333},
    {0x278,0x0000},
    {0x27A,0x0000},
    {0x27C,0x0001},
    {0x27E,0x0000},
    {0x280,0x0383},
    {0x282,0x0200},
    {0x284,0x0003},
    {0x286,0x00F0},
    {0x288,0x0000},
    {0x28A,0x0000},
    {0x28C,0x3001},
    {0x28E,0x0808},
    {0x290,0x0050},
    {0x292,0x0050},
    {0x294,0x0050},
    {0x296,0x0050},
    {0x298,0x1111},
    {0x29A,0x3333},
    {0x29C,0x3333},
    {0x29E,0x0000},
    {0x2A0,0x0000},
    {0x2A2,0x0001},
    {0x300,0x0000},
    {0x302,0x0000},
    {0x304,0x0200},
    {0x306,0x0000},
    {0x308,0x0000},
    {0x30A,0x0000},
    {0x30C,0x0000},
    {0x30E,0x3001},
    {0x310,0x0808},
    {0x312,0x000F},
    {0x314,0x000F},
    {0x316,0x000F},
    {0x318,0x000F},
    {0x31A,0x0000},
    {0x31C,0x0000},
    {0x31E,0x0000},
    {0x320,0x0000},
    {0x322,0x0000},
    {0x324,0x0001},
    {0x326,0x0000},
    {0x328,0x0000},
    {0x32A,0x0200},
    {0x32C,0x0000},
    {0x32E,0x0000},
    {0x330,0x0000},
    {0x332,0x0000},
    {0x334,0x0000},
    {0x336,0x0101},
    {0x338,0x0000},
    {0x33A,0x0000},
    {0x33C,0x0000},
    {0x33E,0x0000},
    {0x340,0x0000},
    {0x342,0x0000},
    {0x344,0x0000},
    {0x346,0x0000},
    {0x348,0x0000},
    {0x34A,0x0071},
    {0x34C,0x0E00},
    {0x34E,0x2E14},
    {0x350,0x0000},
    {0x352,0x0000},
    {0x354,0x0200},
    {0x356,0x0000},
    {0x358,0x0000},
    {0x35A,0x0000},
    {0x35C,0x0000},
    {0x35E,0x0020},
    {0x360,0x0808},
    {0x362,0x001F},
    {0x364,0x001F},
    {0x366,0x001F},
    {0x368,0x001F},
    {0x36A,0x0000},
    {0x36C,0x0000},
    {0x36E,0x0000},
    {0x370,0x0000},
    {0x372,0x0000},
    {0x374,0x0001},
    {0x376,0x0000},
    {0x378,0x0000},
    {0x37A,0x0000},
    {0x37C,0x0000},
    {0x37E,0x0200},
    {0x380,0x0000},
    {0x382,0x0000},
    {0x384,0x0000},
    {0x386,0x0000},
    {0x388,0x0020},
    {0x38A,0x0808},
    {0x38C,0x001F},
    {0x38E,0x001F},
    {0x390,0x001F},
    {0x392,0x001F},
    {0x394,0x0000},
    {0x396,0x0000},
    {0x398,0x0000},
    {0x39A,0x0000},
    {0x39C,0x0000},
    {0x39E,0x0001},
    {0x3A0,0x0000},
    {0x3A2,0x0000},
    {0x400,0x0000},
    {0x402,0x3430},
    {0x404,0x4000},
    {0x406,0xC000},
    {0x408,0x0000},
    {0x40A,0x0000},
    {0x40C,0x0010},
    {0x40E,0x0010},
    {0x410,0x0004},
    {0x412,0x0058},
    {0x414,0x00D0},
    {0x416,0x0018},
    {0x418,0x0018},
    {0x41A,0x0020},
    {0x41C,0x2020},
    {0x41E,0x8000},
    {0x420,0x7000},
    {0x422,0x4000},
    {0x424,0x3000},
    {0x426,0x4000},
    {0x428,0x4000},
    {0x42A,0x3000},
    {0x42C,0x0000},
    {0x42E,0x0000},
    {0x430,0x0000},
    {0x432,0x0000},
    {0x434,0x0000},
    {0x436,0x0000},
    {0x438,0x0000},
    {0x43A,0x0100},
    {0x43C,0x0000},
    {0x43E,0x0000},
    {0x440,0x0000},
    {0x442,0x0000},
    {0x444,0x1004},
    {0x446,0x0010},
    {0x448,0x0010},
    {0x44A,0x0051},
    {0x44C,0x00C1},
    {0x44E,0x00C1},
    {0x450,0x00A1},
    {0x452,0x00A1},
    {0x454,0x00A1},
    {0x456,0x00A1},
    {0x458,0x00A1},
    {0x45A,0x00A1},
    {0x45C,0x00A1},
    {0x45E,0x00A1},
    {0x460,0x0010},
    {0x462,0xC000},
    {0x464,0x0061},
    {0x466,0x0000},
    {0x468,0x00A1},
    {0x46A,0x0000},
    {0x46C,0x0000},
    {0x46E,0x0000},
    {0x470,0x0000},
    {0x500,0x0000},
    {0x502,0x4001},
    {0x504,0x0000},
    {0x506,0x0000},
    {0x508,0x0000},
    {0x50A,0x0000},
    {0x50C,0x0000},
    {0x50E,0x0000},
    {0x510,0x0000},
    {0x512,0x0000},
    {0x514,0x0000},
    {0x516,0x0000},
    {0x518,0x0000},
    {0x51A,0x0000},
    {0x51C,0x0000},
    {0x51E,0x0000},
    {0x520,0x0000},
    {0x522,0x0000},
    {0x524,0x0000},
    {0x526,0x0000},
    {0x528,0x0000},
    {0x52A,0x0000},
    {0x52C,0x0000},
    {0x52E,0x0000},
    {0x530,0x0000},
    {0x532,0x0000},
    {0x534,0x0000},
    {0x536,0x0000},
    {0x538,0x0000},
    {0x53A,0x0000},
    {0x53C,0x0000},
    {0x53E,0x0000},
    {0x540,0x00A0},
    {0x542,0x0002},
    {0x544,0x0000},
    {0x546,0x0000},
    {0x548,0x0000},
    {0x54A,0x8000},
    {0x54C,0x8000},
    {0x54E,0x0000},
    {0x550,0x0000},
    {0x552,0x0000},
    {0x554,0x0506},
    {0x556,0x0000},
    {0x558,0x0000},
    {0x55A,0x0000},
    {0x55C,0x1000},
    {0x55E,0x0000},
    {0x560,0x0000},
    {0x562,0x0000},
    {0x564,0x0000},
    {0x566,0x0000},
    {0x568,0x0000},
    {0x56A,0x000B},
    {0x56C,0x0008},
    {0x56E,0x0008},
    {0x570,0x0000},
    {0x572,0x0000},
    {0x574,0x0000},
    {0x576,0x0000},
    {0x578,0x0000},
    {0x57A,0x0000},
    {0x57C,0x0000},
    {0x57E,0x0000},
    {0x580,0x0000},
    {0x582,0x0000},
    {0x584,0x0000},
    {0x586,0x0000},
    {0x588,0x0000},
    {0x58A,0x0000},
    {0x58C,0x0101},
    {0x58E,0x0010},
    {0x590,0x0010},
    {0x592,0x0010},
    {0x594,0x0010},
    {0x596,0x0333},
    {0x598,0x0000},
    {0x59A,0x0000},
    {0x59C,0x00FF},
    {0x59E,0x0004},
    {0x5A0,0x0000},
    {0x5A2,0x0000},
    {0x600,0x0000},
    {0x602,0x0000},
    {0x604,0x0014},
    {0x606,0x0000},
    {0x608,0x0000},
    {0x60A,0x0014},
    {0x60C,0x0000},
    {0x60E,0x4531},
    {0x610,0x0000},
    {0x612,0x2000},
    {0x614,0x0000},
    {0x616,0x0000},
    {0x618,0x0020},
    {0x61A,0x0000},
    {0x61C,0x0000},
    {0x61E,0x0000},
    {0x620,0x0000},
    {0x622,0x0000},
    {0x624,0x0000},
    {0x626,0x0000},
    {0x628,0x0000},
    {0x62A,0x0000},
    {0x62C,0x0000},
    {0x62E,0x0000},
    {0x630,0x00AB},
    {0x632,0x0000},
    {0x634,0x0000},
    {0x636,0x0000},
    {0x638,0x0000},
    {0x63A,0x0000},
    {0x63C,0x0000},
    {0x63E,0x0000},
    {0x640,0x0788},
    {0x642,0x0600},
    {0x700,0x0000},
    {0x702,0x0000},
    {0x704,0x0000},
    {0x706,0x0020},
    {0x708,0x0000},
    {0x70A,0x0000},
    {0x70C,0x1552},
    {0x70E,0x0030},
    {0x710,0x0000},
    {0x712,0x0000},
    {0x714,0x0000},
    {0x716,0x0000},
    {0x718,0x0004},
    {0x71A,0x8000},
    {0x71C,0x0000},
    {0x71E,0x0000},
    {0x720,0x1500},
    {0x722,0x0015},
    {0x724,0x0000},
    {0x726,0x0010},
    {0x728,0x0000},
    {0x72A,0x0000},
    {0x72C,0x0080},
    {0x72E,0x0204},
    {0x730,0x0000},
    {0x732,0x0000},
    {0x734,0x0000},
    {0x736,0x0000},
    {0x738,0x0000},
    {0x73A,0x000F},
    {0x73C,0x0F0F},
    {0x73E,0x000F},
    {0x740,0x0707},
    {0x742,0x0000},
    {0x744,0x0020},
    {0x746,0x0001}            																
};

static kal_uint32 aPMURegBeg_mask[490]= {  /* mask*/
	0x001D,
	0x00FF,
	0x000E,
	0x00DF,
	0x000F,
	0x0077,
	0x002F,
	0x00F7,
	0x03FF,
	0x0001,
	0x0030,
	0x003F,
	0x0077,
	0x011F,
	0x00FF,
	0x0003,
	0x1F0F,
	0x0037,
	0x000F,
	0x00FF,
	0x0077,
	0x0077,
	0x00FF,
	0x00D7,
	0x00F1,
	0x001F,
	0x000F,
	0x00FF,
	0x03FF,
	0x00FF,
	0x0000,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0x00FF,
	0x007F,
	0x03FF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0x00FF,
	0x00F3,
	0xFFFF,
	0xFFFF,
	0xFF9F,
	0x0FFF,
	0x3FFF,
	0x0000,
	0x0000,
	0x0000,
	0x0F00,
	0x0000,
	0x0000,
	0x0000,
	0xFFFF,
	0x0000,
	0x0000,
	0x01FF,
	0x01FF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0x0307,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0x00FF,
	0xFFFF,
	0xFF1F,
	0x0000,
	0x0000,
	0xC007,
	0xFFFF,
	0x0000,
	0x003F,
	0x0007,
	0x0001,
	0xFFFF,
	0x0001,
	0x0015,
	0x0000,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0001,
	0xFFFF,
	0x0000,
	0x1FFF,
	0x0077,
	0xC3F3,
	0x037F,
	0x0033,
	0x00FF,
	0x001F,
	0x7777,
	0x0001,
	0xFFFF,
	0x007F,
	0x007F,
	0x007F,
	0x0000,
	0x0333,
	0x0333,
	0x0333,
	0x0000,
	0x0000,
	0x0D73,
	0x0077,
	0xC3F3,
	0x0370,
	0x003F,
	0x00FF,
	0x003F,
	0x7777,
	0x0001,
	0xFFFF,
	0x007F,
	0x007F,
	0x007F,
	0x0000,
	0x0777,
	0x0333,
	0x0333,
	0x0000,
	0x0000,
	0x0D73,
	0x7F7F,
	0x7F7F,
	0x007F,
	0x0077,
	0xC3F3,
	0x037F,
	0x0033,
	0x00FF,
	0x000F,
	0x7777,
	0x0001,
	0xFFFF,
	0x007F,
	0x007F,
	0x007F,
	0x0000,
	0x0333,
	0x0333,
	0x0333,
	0x0000,
	0x0000,
	0x0D73,
	0x0077,
	0xC3F3,
	0x037F,
	0x0033,
	0x00FF,
	0x000F,
	0x7777,
	0x0001,
	0xFFFF,
	0x007F,
	0x007F,
	0x007F,
	0x0000,
	0x0333,
	0x0333,
	0x0333,
	0x0000,
	0x0000,
	0x0D73,
	0x0007,
	0xFFF3,
	0x3300,
	0x0003,
	0x00FF,
	0x000F,
	0x7777,
	0x0001,
	0xFFFF,
	0x001F,
	0x001F,
	0x001F,
	0x0000,
	0x0000,
	0x0333,
	0x0333,
	0x0333,
	0x0333,
	0x0D73,
	0x0077,
	0xFFF3,
	0xF300,
	0x0043,
	0xFFFF,
	0x000F,
	0x7777,
	0x0001,
	0xFFFF,
	0x003F,
	0x003F,
	0x003F,
	0x0000,
	0x0000,
	0x0777,
	0x0000,
	0x0333,
	0x0333,
	0x0D73,
	0x3F01,
	0x3F3F,
	0x0007,
	0xFFF7,
	0x0F08,
	0x0003,
	0x00FF,
	0x000F,
	0x7777,
	0x0031,
	0xFFFF,
	0x001F,
	0x001F,
	0x001F,
	0x0000,
	0x0000,
	0x0333,
	0x0333,
	0x0777,
	0x0777,
	0x0D73,
	0x0003,
	0x0002,
	0x0007,
	0xFFF7,
	0x0F08,
	0x0003,
	0x00FF,
	0x000F,
	0x7777,
	0x0031,
	0xFFFF,
	0x001F,
	0x001F,
	0x001F,
	0x0000,
	0x0000,
	0x0333,
	0x0333,
	0x0777,
	0x0777,
	0x0D73,
	0x1F7F,
	0x0000,
	0x5370,
	0x7F73,
	0x7033,
	0x7033,
	0xB000,
	0x00FD,
	0x0F50,
	0x0F50,
	0x0F54,
	0x0F58,
	0x0FF3,
	0x0F58,
	0x0F58,
	0x5370,
	0x7F73,
	0xAFFF,
	0x7033,
	0x7033,
	0x7333,
	0x7033,
	0x7033,
	0xB073,
	0xB073,
	0xB073,
	0xB073,
	0xB073,
	0xB073,
	0xB073,
	0xB077,
	0x0100,
	0x8C48,
	0xCCEF,
	0x0000,
	0x0000,
	0x76FF,
	0x0F50,
	0x0F50,
	0x0F55,
	0x0FD5,
	0x0FD5,
	0x0FF5,
	0x0FF5,
	0x0FF5,
	0x0FF5,
	0x0FF5,
	0x0FF5,
	0x0FF5,
	0x0FF5,
	0x0F50,
	0x7067,
	0x0FF5,
	0xB077,
	0x0FF5,
	0x8FFF,
	0x7777,
	0x7777,
	0xFF87,
	0x007B,
	0x71FF,
	0x001F,
	0x0003,
	0x0FF3,
	0x03FF,
	0x0030,
	0xFF37,
	0x0003,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0FFF,
	0x07E7,
	0xFFFF,
	0xFFFF,
	0x000F,
	0x33FF,
	0x33FF,
	0x0000,
	0x0000,
	0x03FF,
	0x0FFF,
	0x3F7F,
	0x0015,
	0x3F7F,
	0xFF05,
	0x7F03,
	0xFFFF,
	0x00B1,
	0xFF03,
	0xFFFF,
	0xB100,
	0xFF1F,
	0xFF1F,
	0xFF1F,
	0xC7C7,
	0x7F00,
	0x7F00,
	0x7F00,
	0x4FFF,
	0xFF0F,
	0xFF0F,
	0xFF0F,
	0xABF0,
	0xFEFB,
	0x0003,
	0x0077,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0xFFFF,
	0x8333,
	0x0100,
	0xF3FF,
	0x0000,
	0x0000,
	0xFFFF,
	0xFFFF,
	0x330D,
	0x7F00,
	0x07FF,
	0x330D,
	0x7F00,
	0x07FF,
	0x0F37,
	0x7FFF,
	0x7FFF,
	0xFFFF,
	0x0077,
	0xFFFF,
	0xFBFF,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0x0000,
	0xFFFF,
	0xFFFF,
	0x0000,
	0xFFFF,
	0x0000,
	0x3FFF,
	0x03FF,
	0x0107,
	0x0000,
	0x0000,
	0xFFFF,
	0x00FF,
	0xFFFF,
	0xC0FF,
	0x1FEB,
	0x13FF,
	0x001F,
	0xFFFF,
	0x1FFF,
	0x00FF,
	0x7FFF,
	0xFFFF,
	0x1FFF,
	0x007F,
	0x001F,
	0x003F,
	0x0FFF,
	0xFFFF,
	0x000F,
	0xF800,
	0x0FFF,
	0x7FFF,
	0x3FFF,
	0x00FF,
	0xFFFF,
	0x001F,
	0xFFFF,
	0x00FF,
	0x07FD,
	0x8F1F,
	0x03FF,
	0xFFFF,
	0x0001,
	0xFFFF,
	0x07FF,
	0x000F,
	0x0F0F,
	0x000F,
	0x0707,
	0x0000,
	0x31FF,
	0x0001
};

/*Define for default value test */
static kal_uint32 aPMURegBeg_bank0_write[][2]= {  /* Register*//*reset val*/																
    {0x10E,0xFFFF},
    {0x130,0x00F0},
    {0x132,0x0001},
    {0x134,0x0000},
    {0x136,0x0000},
    {0x13A,0x0F00},
    {0x14C,0x0000},
    {0x14E,0x0000},
    {0x150,0x0000},
    {0x152,0x0000},
    {0x154,0x0000},
    {0x156,0x0000},
    {0x158,0x0000},
    {0x164,0xCCCC},
    {0x166,0xCCCC},
    {0x168,0xCCCC},
    {0x16A,0xCCCC},
    {0x16C,0xCCCC},
    {0x16E,0xCCCC},
    {0x170,0xCCCC},
    {0x172,0xCCCC},
    {0x174,0xCCCC},
    {0x176,0x00CC},
    {0x700,0x0000},
    {0x702,0x0000},
    {0x704,0x0000},
    {0x706,0x0020},
    {0x708,0x0000},
    {0x70A,0x0000},
    {0x70C,0x1552},
    {0x70E,0x0030},
    {0x710,0x0000},
    {0x712,0x0000},
    {0x714,0x0000},
    {0x716,0x0000},
    {0x718,0x0004},
    {0x71A,0x8000},
    {0x71C,0x0000},
    {0x71E,0x0000},
    {0x720,0x1500},
    {0x722,0x0015},
    {0x724,0x0000},
    {0x726,0x0010},
    {0x728,0x0000},
    {0x72A,0x0000},
    {0x72C,0x0080},
    {0x72E,0x0204},
    {0x730,0x0000},
    {0x732,0x0000},
    {0x734,0x0000},
    {0x736,0x0000},
    {0x738,0x0000},
    {0x73A,0x000F},
    {0x73C,0x0F0F},
    {0x73E,0x000F},
    {0x740,0x0707},
    {0x742,0x0000},
    {0x744,0x0020},
    {0x746,0x0001}     
};

static kal_uint32 aPMURegBeg_mask_write[60]= {  /* mask*/   
   0xFFFF,
   0x00FF,
   0x00F3,
   0xFFFF,
   0xFFFF,
   0x0FFF,
   0xFFFF,
   0x0000,
   0x0000,
   0x01FF,
   0x01FF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0xFFFF,
   0x00FF,
   0x001F,
   0xFFFF,
   0x1FFF,
   0x00FF,
   0x7FFF,
   0xFFFF,
   0x1FFF,
   0x007F,
   0x001F,
   0x003F,
   0x0FFF,
   0xFFFF,
   0x000F,
   0xF800,
   0x0FFF,
   0x7FFF,
   0x3FFF,
   0x00FF,
   0xFFFF,
   0x001F,
   0xFFFF,
   0x00FF,
   0x07FD,
   0x8F1F,
   0x03FF,
   0xFFFF,
   0x0001,
   0xFFFF,
   0x07FF,
   0x000F,
   0x0F0F,
   0x000F,
   0x0707,
   0x0000,
   0x31FF,
   0x0001 
};


//---------------------------------------------------------------------------
// Common Test API
//---------------------------------------------------------------------------
extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
extern void upmu_set_reg_value(kal_uint32 reg, kal_uint32 reg_val);

//---------------------------------------------------------------------------
// Test Case Implementation
//---------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////////
//
//  TOP TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void pmic_get_chip_version_ldvt(void)
{
    //kal_uint32 eco_version = 0;
    kal_uint32 tmp32;

	tmp32 = upmu_get_cid();

	printk("[pmic_get_chip_version_ldvt] %x\n", tmp32);		
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_get_PCHR_CHRDET_ldvt(void)
{
	kal_uint32 tmp32;
	
	tmp32=upmu_get_rgs_chrdet();
		
	if(tmp32 == 0)
	{
		printk("[pmic_get_PCHR_CHRDET_ldvt] No charger\n");
	}
	else
	{
		printk("[pmic_get_PCHR_CHRDET_ldvt] Charger exist\n");
	}	
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_VERIFY_DEFAULT_VALUE_ldvt(void)
{
	//kal_uint32 tmp32=0;
	kal_uint32 u2PMUReg = 0;
    kal_uint32 u2Cnt = 0;
    kal_uint32 default_value_mask = 0;

	printk("RegNum,DefaultValue,GotValue,TestValue,Mask\n");

	for(u2Cnt = 0; u2Cnt < (sizeof(aPMURegBeg_bank0)/sizeof(*aPMURegBeg_bank0)); ++u2Cnt)
	{
	   u2PMUReg = upmu_get_reg_value(    (aPMURegBeg_bank0[u2Cnt][0])  );

	   //printk("[Before MASK] %x,%x,%x\r\n",(aPMURegBeg_bank0[u2Cnt][0]), u2PMUReg,(aPMURegBeg_bank0[u2Cnt][1]));	   
	   
       //only check value of mask
	   u2PMUReg &= aPMURegBeg_mask[u2Cnt];
	   
	   //printk("[After MASK]%x,%x,%x\r\n",(aPMURegBeg_bank0[u2Cnt][0]), u2PMUReg,(aPMURegBeg_bank0[u2Cnt][1]));

       default_value_mask = ((aPMURegBeg_bank0[u2Cnt][1]) & aPMURegBeg_mask[u2Cnt]);
       
	   //if(u2PMUReg != (aPMURegBeg_bank0[u2Cnt][1]))
	   if(u2PMUReg != default_value_mask)
	   {
	   	   printk("[error] %x,%x,%x,%x,%x\r\n",
            (aPMURegBeg_bank0[u2Cnt][0]), 
            (aPMURegBeg_bank0[u2Cnt][1]),
            upmu_get_reg_value(    (aPMURegBeg_bank0[u2Cnt][0])  ),              
            u2PMUReg, 
            aPMURegBeg_mask[u2Cnt]);
	   }
	}	
	
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_TOP_WR(int test_value)
{
	//kal_uint32 tmp32=0;
	kal_uint32 u2PMUReg = 0;
    kal_uint32 u2Cnt = 0;
    kal_uint32 default_value_mask = 0;

	printk("RegNum,write_value(default_value_mask),GotValue,TestValue,Mask\n");

	for(u2Cnt = 0; u2Cnt < (sizeof(aPMURegBeg_bank0_write)/sizeof(*aPMURegBeg_bank0_write)); ++u2Cnt)
	{
       //write test value
       upmu_set_reg_value( (aPMURegBeg_bank0_write[u2Cnt][0]), test_value );
	
	   //read back value 
	   u2PMUReg = upmu_get_reg_value(    (aPMURegBeg_bank0_write[u2Cnt][0])  );

	   //printk("[Before MASK] %x,%x,%x\r\n",(aPMURegBeg_bank0_write[u2Cnt][0]), u2PMUReg,(aPMURegBeg_bank0_write[u2Cnt][1]));	   
	   
       //only check value of mask
	   u2PMUReg &= aPMURegBeg_mask_write[u2Cnt];
	   
	   //printk("[After MASK]%x,%x,%x\r\n",(aPMURegBeg_bank0_write[u2Cnt][0]), u2PMUReg,(aPMURegBeg_bank0_write[u2Cnt][1]));
	   
       default_value_mask = (test_value & aPMURegBeg_mask_write[u2Cnt]);

	   if(u2PMUReg != default_value_mask)
	   {	   	   
           printk("[error] %x,%x(%x),%x,%x,%x\r\n",
            (aPMURegBeg_bank0_write[u2Cnt][0]), 
            //(aPMURegBeg_bank0[u2Cnt][1]),
            test_value,
            default_value_mask,
            upmu_get_reg_value(    (aPMURegBeg_bank0_write[u2Cnt][0])  ),              
            u2PMUReg, 
            aPMURegBeg_mask_write[u2Cnt]);
	   }
	}

    #if 0 //debug check
    for(u2Cnt = 0; u2Cnt < (sizeof(aPMURegBeg_bank0_write)/sizeof(*aPMURegBeg_bank0_write)); ++u2Cnt)
    {
        printk("Reg[%x] %x\n", 
            (aPMURegBeg_bank0_write[u2Cnt][0]), 
            upmu_get_reg_value(    (aPMURegBeg_bank0_write[u2Cnt][0])  )
            );
    }
    #endif
}

///////////////////////////////////////////////////////////////////////////////////
//
// AUXADC TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_AUXADC_611(void)
{
	kal_int32 i=0;
	kal_int32 ret_val=0;

	printk("\r\n[pmic_UVVP_PMIC_AUXADC_611]\r\n");

	for(i=0;i<=7;i++)
	{
		ret_val=PMIC_IMM_GetOneChannelValue(i,1,1);
		
        if(i==2)
            ret_val = ret_val / 100; // 2:v_charger
		
		printk("[pmic_UVVP_PMIC_AUXADC_611] ch_num=%d, val=%d\n", i, ret_val);
	}
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_AUXADC_612(void)
{
	kal_int32 i=0;
	kal_int32 ret_val=0;

	printk("\r\n[pmic_UVVP_PMIC_AUXADC_612]\r\n");

	for(i=0;i<=7;i++)
	{
		upmu_set_rg_avg_num(i);
		
		ret_val=PMIC_IMM_GetOneChannelValue(0,1,1); //0:vbat
		
		printk("[pmic_UVVP_PMIC_AUXADC_612] avg_num%d, val=%d, Reg[0x540]=%x\n", i, ret_val, upmu_get_reg_value(0x540));
	}
}
///////////////////////////////////////////////////////////////////////////////////
kal_uint32 LBAT_VOLT_MAX=0x037b; // wait SA provide, 4.2V
kal_uint32 LBAT_VOLT_MIN=0x02d1; // wait SA provide, 3.4V

kal_uint32 LBAT_DET_PRD_19_16=0x0; 
kal_uint32 LBAT_DET_PRD_15_0=0x03E8;

kal_uint32 LBAT_DEBT_MAX_8_0=1;
kal_uint32 LBAT_DEBT_MIN_8_0=1;

void pmic_UVVP_PMIC_AUXADC_621(void)
{
	kal_uint32 lbat_debounce_count_max=0;
	kal_uint32 lbat_debounce_count_min=0;
	kal_uint32 adc_out_lbat=0;

	printk("\r\n[pmic_UVVP_PMIC_AUXADC_621]\r\n");

	printk("LOW BATTERY (AUXADC) interrupt setting .. start\r\n");

	upmu_set_rg_int_en_bat_h(0);
	upmu_set_rg_int_en_bat_l(1);

//2.1.1 test low battery voltage interrupt 
//1. setup max voltage treshold as VBAT = 4.2
//    SetReg(RG_LBAT_VOLT_MAX_7_0, LBAT_VOLT_MAX[7:0]);
//    SetReg(RG_LBAT_VOLT_MAX_9_8, LBAT_VOLT_MAX[9:8]);
	upmu_set_rg_lbat_volt_max(LBAT_VOLT_MAX);
//2. setup min voltage treshold as VBAT = 3.4
//    SetReg(RG_LBAT_VOLT_MIN_7_0, LBAT_VOLT_MIN[7:0]);
//    SetReg(RG_LBAT_VOLT_MIN_9_8, LBAT_VOLT_MIN[9:8]);
	upmu_set_rg_lbat_volt_min(LBAT_VOLT_MIN);
//3. setup detection period
//    SetReg(RG_LBAT_DET_PRD_19_16, LBAT_DET_PRD[19:16]);
//    SetReg(RG_LBAT_DET_PRD_15_8, LBAT_DET_PRD[15:8]);
	upmu_set_rg_lbat_det_prd_19_16(LBAT_DET_PRD_19_16);
	upmu_set_rg_lbat_det_prd_15_0(LBAT_DET_PRD_15_0);
//4. setup max./min. debounce time.
//    SetReg(RG_LBAT_DEBT_MAX, LBAT_DEBT_MAX[8:0]);
//    SetReg(RG_LBAT_DEBT_MIN, LBAT_DEBT_MIN[8:0]);
	upmu_set_rg_lbat_debt_max(LBAT_DEBT_MAX_8_0);
	upmu_set_rg_lbat_debt_min(LBAT_DEBT_MIN_8_0);
//5. turn on IRQ
//    SetReg(RG_LBAT_IRQ_EN_MAX, 1'b0); // ?? =>1
//    SetReg(RG_LBAT_IRQ_EN_MIN, 1'b0); // ?? =>1
	upmu_set_rg_lbat_irq_en_max(1);
	upmu_set_rg_lbat_irq_en_min(1);
//6. turn on LowBattery Detection
//    SetReg(RG_LBAT_EN_MAX, 1'b1); 
//    SetReg(RG_LBAT_EN_MIN, 1'b1);
	upmu_set_rg_lbat_en_max(1);
	upmu_set_rg_lbat_en_min(1);
//7. Monitor Debounce counts
//    ReadReg(RG_LBAT_DEBOUNCE_COUNT_MAX);
//    ReadReg(BRG_LBAT_DEBOUNCE_COUNT_MIN);
	lbat_debounce_count_max = upmu_get_rg_lbat_debounce_count_max();
	lbat_debounce_count_min = upmu_get_rg_lbat_debounce_count_min();
//8. Read LowBattery Detect Value
//    while(! ReadReg(BRW_RG_ADC_RDY_LBAT));
//    ReadReg(BRW_RG_ADC_OUT_LBAT_7_0);
//    ReadReg(BRW_RG_ADC_OUT_LBAT_9_8);
	//while( upmu_get_rg_adc_rdy_lbat() != 1 )
	//{
	//	printk("1");
	//}
	adc_out_lbat = upmu_get_rg_adc_out_lbat();

    lbat_debounce_count_max = upmu_get_rg_lbat_debounce_count_max();
	lbat_debounce_count_min = upmu_get_rg_lbat_debounce_count_min();
	
//9. Test on VBAT = 3.5 -> 3.4 -> 3.3 and receive interrupt at pmic driver	
//	wake_up_pmic();

	printk("LOWBATTERY (AUXADC) interrupt setting .. done (adc_out_lbat=%d, lbat_debounce_count_max=%d, lbat_debounce_count_min=%d) \n", 
	adc_out_lbat, lbat_debounce_count_max, lbat_debounce_count_min);

}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_AUXADC_622(void)
{
	kal_uint32 lbat_debounce_count_max=0;
	kal_uint32 lbat_debounce_count_min=0;
	kal_uint32 adc_out_lbat=0;

	printk("\r\n[pmic_UVVP_PMIC_AUXADC_622]\r\n");

	printk("HIGH BATTERY (AUXADC) interrupt setting .. start\r\n");

	upmu_set_rg_int_en_bat_h(1);
	upmu_set_rg_int_en_bat_l(0);

//2.1.2 test low battery voltage interrupt 
//1. setup max voltage treshold as VBAT = 4.2
//    SetReg(RG_LBAT_VOLT_MAX_7_0, LBAT_VOLT_MAX[7:0]);
//    SetReg(RG_LBAT_VOLT_MAX_9_8, LBAT_VOLT_MAX[9:8]);
	upmu_set_rg_lbat_volt_max(LBAT_VOLT_MAX);
//2. setup min voltage treshold as VBAT = 3.4
//    SetReg(RG_LBAT_VOLT_MIN_7_0, LBAT_VOLT_MIN[7:0]);
//    SetReg(RG_LBAT_VOLT_MIN_9_8, LBAT_VOLT_MIN[9:8]);
	upmu_set_rg_lbat_volt_min(LBAT_VOLT_MIN);
//3. setup detection period
//    SetReg(RG_LBAT_DET_PRD_19_16, LBAT_DET_PRD[19:16]);
//    SetReg(RG_LBAT_DET_PRD_15_8, LBAT_DET_PRD[15:8]);
	upmu_set_rg_lbat_det_prd_19_16(LBAT_DET_PRD_19_16);
	upmu_set_rg_lbat_det_prd_15_0(LBAT_DET_PRD_15_0);
//4. setup max./min. debounce time.
//    SetReg(RG_LBAT_DEBT_MAX, LBAT_DEBT_MAX[8:0]);
//    SetReg(RG_LBAT_DEBT_MIN, LBAT_DEBT_MIN[8:0]);
	upmu_set_rg_lbat_debt_max(LBAT_DEBT_MAX_8_0);
	upmu_set_rg_lbat_debt_min(LBAT_DEBT_MIN_8_0);
//5. turn on IRQ
//    SetReg(RG_LBAT_IRQ_EN_MAX, 1'b0); // ?? =>1
//    SetReg(RG_LBAT_IRQ_EN_MIN, 1'b0); // ?? =>1
	upmu_set_rg_lbat_irq_en_max(1);
	upmu_set_rg_lbat_irq_en_min(1);
//6. turn on LowBattery Detection
//    SetReg(RG_LBAT_EN_MAX, 1'b1); 
//    SetReg(RG_LBAT_EN_MIN, 1'b1);
	upmu_set_rg_lbat_en_max(1);
	upmu_set_rg_lbat_en_min(1);
//7. Monitor Debounce counts
//    ReadReg(RG_LBAT_DEBOUNCE_COUNT_MAX);
//    ReadReg(BRG_LBAT_DEBOUNCE_COUNT_MIN);
	lbat_debounce_count_max = upmu_get_rg_lbat_debounce_count_max();
	lbat_debounce_count_min = upmu_get_rg_lbat_debounce_count_min();
//8. Read LowBattery Detect Value
//    while(! ReadReg(BRW_RG_ADC_RDY_LBAT));
//    ReadReg(BRW_RG_ADC_OUT_LBAT_7_0);
//    ReadReg(BRW_RG_ADC_OUT_LBAT_9_8);
	//while( upmu_get_rg_adc_rdy_lbat() != 1 )
	//{
	//	printk("2");
	//}
	adc_out_lbat = upmu_get_rg_adc_out_lbat();

    lbat_debounce_count_max = upmu_get_rg_lbat_debounce_count_max();
	lbat_debounce_count_min = upmu_get_rg_lbat_debounce_count_min();
	
//9. Test on VBAT = 4.0 -> 4.2 -> 4.3 and receive interrupt at pmic driver
//	wake_up_pmic();

	printk("LOWBATTERY (AUXADC) interrupt setting .. done (adc_out_lbat=%d, lbat_debounce_count_max=%d, lbat_debounce_count_min=%d) \n", 
	adc_out_lbat, lbat_debounce_count_max, lbat_debounce_count_min);

}

///////////////////////////////////////////////////////////////////////////////////
//
//  INTERRUPT TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_INT_451(void)
{
    upmu_set_rg_int_en_ldo(1);
    printk("[upmu_set_rg_int_en_ldo(1)]");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_INT_461(void)
{
    upmu_set_rg_int_en_vcore(1);
    pmic_config_interface(0x106, 0x1008, 0xFFFF, 0x0);//turn clock for buck oc
    printk("[upmu_set_rg_int_en_vcore(1)]");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_INT_462(void)
{
    upmu_set_rg_int_en_vio18(1);
    pmic_config_interface(0x106, 0x1008, 0xFFFF, 0x0);//turn clock for buck oc
    printk("[upmu_set_rg_int_en_vio18(1)]");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_INT_463(void)
{
    upmu_set_rg_int_en_vm(1);
    pmic_config_interface(0x106, 0x1008, 0xFFFF, 0x0);//turn clock for buck oc
    printk("[upmu_set_rg_int_en_vm(1)]");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_INT_464(void)
{
    upmu_set_rg_int_en_vpa(1);
    pmic_config_interface(0x106, 0x1008, 0xFFFF, 0x0);//turn clock for buck oc
    printk("[upmu_set_rg_int_en_vpa(1)]");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_INT_465(void)
{
    upmu_set_rg_int_en_vproc(1);
    pmic_config_interface(0x106, 0x1008, 0xFFFF, 0x0);//turn clock for buck oc
    printk("[upmu_set_rg_int_en_vproc(1)]");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_INT_466(void)
{
    upmu_set_rg_int_en_vrf18(1);
    pmic_config_interface(0x106, 0x1008, 0xFFFF, 0x0);//turn clock for buck oc
    printk("[upmu_set_rg_int_en_vrf18(1)]");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_INT_467(void)
{
    upmu_set_rg_int_en_vrf18_2(1);
    pmic_config_interface(0x106, 0x1008, 0xFFFF, 0x0);//turn clock for buck oc
    printk("[upmu_set_rg_int_en_vrf18_2(1)]");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_INT_468(void)
{
    upmu_set_rg_int_en_vsram(1);
    pmic_config_interface(0x106, 0x1008, 0xFFFF, 0x0);//turn clock for buck oc
    printk("[upmu_set_rg_int_en_vsram(1)]");
}

///////////////////////////////////////////////////////////////////////////////////
//
//  BUCK TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
int g_buck_num=100;
int g_log_buck_vosel=0;

extern int PMIC_IMM_GetOneChannelValue(int dwChannel, int deCount, int trimd);
void read_auxadc_value(void)
{
    int ret=0;
    
    if(g_buck_num == 0)
    {
        //vproc
        mdelay(1);
        if(g_log_buck_vosel==1)
        {
            printk("Reg[%x]=%d\n", 0x21e, upmu_get_reg_value(0x21e));
        }
        else
        {        
            printk("Reg[%x]=0x%x,Reg[%x]=0x%x,Reg[%x]=0x%x\n", 
                    0x128, upmu_get_reg_value(0x128),
                    0x214, upmu_get_reg_value(0x214),
                    0x21e, upmu_get_reg_value(0x21e)
                    );        
        }
    }
    else if(g_buck_num == 1)
    {
        //vsram
        mdelay(1);
        if(g_log_buck_vosel==1)
        {
            printk("Reg[%x]=%d\n", 0x244, upmu_get_reg_value(0x244));
        }
        else
        {
            printk("Reg[%x]=0x%x,Reg[%x]=0x%x,Reg[%x]=0x%x\n", 
                    0x128, upmu_get_reg_value(0x128),
                    0x23a, upmu_get_reg_value(0x23a),
                    0x244, upmu_get_reg_value(0x244)
                    );
        }
    }
    else if(g_buck_num == 2)
    {
        //vcore
        mdelay(1);
        if(g_log_buck_vosel==1)
        {
            printk("Reg[%x]=%d\n", 0x270, upmu_get_reg_value(0x270));
        }
        else
        {
            printk("Reg[%x]=0x%x,Reg[%x]=0x%x,Reg[%x]=0x%x\n", 
                    0x128, upmu_get_reg_value(0x128),
                    0x266, upmu_get_reg_value(0x266),
                    0x270, upmu_get_reg_value(0x270)
                    ); 
        }
    }
    else if(g_buck_num == 3)
    {
        //vm
        mdelay(1);
        if(g_log_buck_vosel==1)
        {
            printk("Reg[%x]=%d\n", 0x296, upmu_get_reg_value(0x296));
        }
        else
        {
            printk("Reg[%x]=0x%x,Reg[%x]=0x%x,Reg[%x]=0x%x\n", 
                    0x128, upmu_get_reg_value(0x128),
                    0x28c, upmu_get_reg_value(0x28c),
                    0x296, upmu_get_reg_value(0x296)
                    );
        }
    }
    else if(g_buck_num == 4)
    {
        //vio18
        mdelay(1);
        if(g_log_buck_vosel==1)
        {
            printk("Reg[%x]=%d\n", 0x318, upmu_get_reg_value(0x318));
        }
        else
        {
            printk("Reg[%x]=0x%x,Reg[%x]=0x%x,Reg[%x]=0x%x\n", 
                    0x128, upmu_get_reg_value(0x128),
                    0x30e, upmu_get_reg_value(0x30e),
                    0x318, upmu_get_reg_value(0x318)
                    );
        }
    }
    else if(g_buck_num == 5)
    {
        //vpa
        mdelay(1);
        if(g_log_buck_vosel==1)
        {
            printk("Reg[%x]=%d\n", 0x33e, upmu_get_reg_value(0x33e));
        }
        else
        {
            printk("Reg[%x]=0x%x,Reg[%x]=0x%x,Reg[%x]=0x%x\n", 
                    0x128, upmu_get_reg_value(0x128),
                    0x334, upmu_get_reg_value(0x334),
                    0x33e, upmu_get_reg_value(0x33e)
                    );
        }
    }
    else if(g_buck_num == 6)
    {
        //vrf18
        mdelay(1);
        if(g_log_buck_vosel==1)
        {
            printk("Reg[%x]=%d\n", 0x368, upmu_get_reg_value(0x368));
        }
        else
        {
            printk("Reg[%x]=0x%x,Reg[%x]=0x%x,Reg[%x]=0x%x\n", 
                    0x128, upmu_get_reg_value(0x128),
                    0x35e, upmu_get_reg_value(0x35e),
                    0x368, upmu_get_reg_value(0x368)
                    );
        }
    }
    else if(g_buck_num == 7)
    {
        //vrf18_2
        mdelay(1);
        if(g_log_buck_vosel==1)
        {
            printk("Reg[%x]=%d\n", 0x392, upmu_get_reg_value(0x392));
        }
        else
        {
            printk("Reg[%x]=0x%x,Reg[%x]=0x%x,Reg[%x]=0x%x\n", 
                    0x128, upmu_get_reg_value(0x128),
                    0x388, upmu_get_reg_value(0x388),
                    0x392, upmu_get_reg_value(0x392)
                    );
        }
    }
    else 
    {
    }

    mdelay(20);
    ret = PMIC_IMM_GetOneChannelValue(4,1,0);    
    ret = (ret*34)/10;
    
    printk("[read_auxadc_value] ret = %d\n\n", ret);
}
///////////////////////////////////////////////////////////////////////////////////
#if 1
void do_scrxxx_map(int j)
{
    switch(j){
        case 0:
            printk("[upmu_set_rg_srcvolt_en(1)] ");
            upmu_set_rg_srcvolt_en(1);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(0)] ");
            upmu_set_rg_srcvolt_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(1)] ");
            upmu_set_rg_srcvolt_en(1);
            read_auxadc_value();
            break;
            
        case 1:
            printk("[upmu_set_rg_srclkperi_en(1)] ");
            upmu_set_rg_srclkperi_en(1);
            read_auxadc_value();

            printk("[upmu_set_rg_srclkperi_en(0)] ");
            upmu_set_rg_srclkperi_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srclkperi_en(1)] ");
            upmu_set_rg_srclkperi_en(1);
            read_auxadc_value();
            break;
            
        case 2:
            printk("[upmu_set_rg_srclkmd2_en(1)] ");
            upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();

            printk("[upmu_set_rg_srclkmd2_en(0)] ");
            upmu_set_rg_srclkmd2_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srclkmd2_en(1)] ");
            upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();                            
            break;
            
        case 3:
            printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0);] ");
            upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1);] ");
            upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0);] ");
            upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1);] ");
            upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1);
            read_auxadc_value();
            break;
            
        case 4:
            printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);] ");
            upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);] ");
            upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);] ");
            upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);] ");
            upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();
            break;
            
        case 5:
            printk("[upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);] ");
            upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);] ");
            upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();

            printk("[upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);] ");
            upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);] ");
            upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();
            break;
            
        case 6:
            printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);] ");
            upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);] ");
            upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);] ");
            upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);] ");
            upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);] ");
            upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);] ");
            upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);] ");
            upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
            read_auxadc_value();

            printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);] ");
            upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
            read_auxadc_value();
            break;
            
        case 7:
            printk("[0] ");
            read_auxadc_value();
            break;
            
        default:
            printk("[do_scrxxx_map] Invalid channel value(%d)\n", j);
            break;    
    }
}

void do_vproc_en_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vproc_en_ctrl] %d\n", i);
        upmu_set_vproc_en_ctrl(i);        
        
        switch(i){
            case 0:
                printk("[upmu_set_vproc_en(0)] ");
                upmu_set_vproc_en(0);        
                read_auxadc_value();   
                
                printk("[upmu_set_vproc_en(1)] ");
                upmu_set_vproc_en(1);        
                read_auxadc_value();
                break;    

            case 1:    
                for(j=0;j<=7;j++)
                {
                    printk("[upmu_set_vproc_en_sel] %d\n", j);
                    upmu_set_vproc_en_sel(j);

                    g_buck_num=0;
                    do_scrxxx_map(j);
                    g_buck_num=100;
                }
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}

void do_vsram_en_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vsram_en_ctrl] %d\n", i);
        upmu_set_vsram_en_ctrl(i);        
        
        switch(i){
            case 0:
                printk("[upmu_set_vsram_en(0)] ");
                upmu_set_vsram_en(0);        
                read_auxadc_value();   
                
                printk("[upmu_set_vsram_en(1)] ");
                upmu_set_vsram_en(1);        
                read_auxadc_value();
                break;    

            case 1:    
                for(j=0;j<=7;j++)
                {
                    printk("[upmu_set_vsram_en_sel] %d\n", j);
                    upmu_set_vsram_en_sel(j);

                    g_buck_num=1;
                    do_scrxxx_map(j);
                    g_buck_num=100;
                }
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}

void do_vcore_en_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vcore_en_ctrl] %d\n", i);
        upmu_set_vcore_en_ctrl(i);        
        
        switch(i){
            case 0:
                printk("[upmu_set_vcore_en(0)] ");
                upmu_set_vcore_en(0);        
                read_auxadc_value();   
                
                printk("[upmu_set_vcore_en(1)] ");
                upmu_set_vcore_en(1);        
                read_auxadc_value();
                break;    

            case 1:    
                for(j=0;j<=7;j++)
                {
                    printk("[upmu_set_vcore_en_sel] %d\n", j);
                    upmu_set_vcore_en_sel(j);

                    g_buck_num=2;
                    do_scrxxx_map(j);
                    g_buck_num=100;                     
                }
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}

void do_vm_en_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vm_en_ctrl] %d\n", i);
        upmu_set_vm_en_ctrl(i);        
        
        switch(i){
            case 0:
                printk("[upmu_set_vm_en(0)] ");
                upmu_set_vm_en(0);        
                read_auxadc_value();   
                
                printk("[upmu_set_vm_en(1)] ");
                upmu_set_vm_en(1);        
                read_auxadc_value();
                break;    

            case 1:    
                for(j=0;j<=7;j++)
                {
                    printk("[upmu_set_vm_en_sel] %d\n", j);
                    upmu_set_vm_en_sel(j);

                    g_buck_num=3;    
                    do_scrxxx_map(j);
                    g_buck_num=100;
                }
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}

void do_vio18_en_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vio18_en_ctrl] %d\n", i);
        upmu_set_vio18_en_ctrl(i);        
        
        switch(i){
            case 0:
                printk("[upmu_set_vio18_en(0)] ");
                upmu_set_vio18_en(0);        
                read_auxadc_value();   
                
                printk("[upmu_set_vio18_en(1)] ");
                upmu_set_vio18_en(1);        
                read_auxadc_value();
                break;    

            case 1:    
                for(j=0;j<=7;j++)
                {
                    printk("[upmu_set_vio18_en_sel] %d\n", j);
                    upmu_set_vio18_en_sel(j);

                    g_buck_num=4;
                    do_scrxxx_map(j);
                    g_buck_num=200;               
                }
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }  
}

void do_vpa_en_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vpa_en_ctrl] %d\n", i);
        upmu_set_vpa_en_ctrl(i);        
        
        switch(i){
            case 0:
                printk("[upmu_set_vpa_en(0)] ");
                upmu_set_vpa_en(0);        
                read_auxadc_value();   
                
                printk("[upmu_set_vpa_en(1)] ");
                upmu_set_vpa_en(1);        
                read_auxadc_value();
                break;    

            case 1:    
                for(j=0;j<=7;j++)
                {
                    printk("[upmu_set_vpa_en_sel] %d\n", j);
                    upmu_set_vpa_en_sel(j);

                    g_buck_num=5;
                    do_scrxxx_map(j);
                    g_buck_num=100;                    
                }
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }  
}

void do_vrf18_en_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vrf18_en_ctrl] %d\n", i);
        upmu_set_vrf18_en_ctrl(i);        
        
        switch(i){
            case 0:
                //---------------------------------------------------------------------
                printk("[upmu_set_vrf18_md1_en_sel(0); upmu_set_vrf18_md2_en_sel(0);]\n");
                upmu_set_vrf18_md1_en_sel(0); upmu_set_vrf18_md2_en_sel(0);                               
                
                printk("[upmu_set_vrf18_en(0); upmu_set_vrf18_md2_en(0);] ");
                upmu_set_vrf18_en(0); upmu_set_vrf18_md2_en(0);
                g_buck_num=6; 
                read_auxadc_value();
                g_buck_num=100;
                
                printk("[upmu_set_vrf18_en(0); upmu_set_vrf18_md2_en(1);] ");
                upmu_set_vrf18_en(0); upmu_set_vrf18_md2_en(1);
                g_buck_num=6;    
                read_auxadc_value();
                g_buck_num=100;
                
                printk("[upmu_set_vrf18_en(1); upmu_set_vrf18_md2_en(0);] ");
                upmu_set_vrf18_en(1); upmu_set_vrf18_md2_en(0);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("[upmu_set_vrf18_en(1); upmu_set_vrf18_md2_en(1);] ");
                upmu_set_vrf18_en(1); upmu_set_vrf18_md2_en(1);                
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                //---------------------------------------------------------------------
                printk("[upmu_set_vrf18_md1_en_sel(0); upmu_set_vrf18_md2_en_sel(1);]\n");
                upmu_set_vrf18_md1_en_sel(0); upmu_set_vrf18_md2_en_sel(1);
                
                printk("upmu_set_vrf18_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_vrf18_en(0); upmu_set_rg_srclkmd2_en(0);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("upmu_set_vrf18_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_vrf18_en(0); upmu_set_rg_srclkmd2_en(1);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("upmu_set_vrf18_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_vrf18_en(1); upmu_set_rg_srclkmd2_en(0);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("upmu_set_vrf18_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_vrf18_en(1); upmu_set_rg_srclkmd2_en(1);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                //---------------------------------------------------------------------
                printk("[upmu_set_vrf18_md1_en_sel(1); upmu_set_vrf18_md2_en_sel(0);]\n");
                upmu_set_vrf18_md1_en_sel(1); upmu_set_vrf18_md2_en_sel(0);

                printk("upmu_set_rg_srcvolt_en(0); upmu_set_vrf18_md2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_vrf18_md2_en(0);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("upmu_set_rg_srcvolt_en(0); upmu_set_vrf18_md2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_vrf18_md2_en(1);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("upmu_set_rg_srcvolt_en(1); upmu_set_vrf18_md2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_vrf18_md2_en(0);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("upmu_set_rg_srcvolt_en(1); upmu_set_vrf18_md2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_vrf18_md2_en(1);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                //---------------------------------------------------------------------
                printk("[upmu_set_vrf18_md1_en_sel(1); upmu_set_vrf18_md2_en_sel(1);]\n");
                upmu_set_vrf18_md1_en_sel(1); upmu_set_vrf18_md2_en_sel(1);
                
                printk("upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                
                printk("upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);
                g_buck_num=6;
                read_auxadc_value();
                g_buck_num=100;
                //---------------------------------------------------------------------
                break;    

            case 1:    
                for(j=0;j<=7;j++)
                {
                    printk("[upmu_set_vrf18_en_sel] %d\n", j);
                    upmu_set_vrf18_en_sel(j);
                    g_buck_num=6;
                    do_scrxxx_map(j);
                    g_buck_num=100;
                }
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }  
}

void do_vrf18_2_en_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vrf18_2_en_ctrl] %d\n", i);
        upmu_set_vrf18_2_en_ctrl(i);        
        
        switch(i){
            case 0:
                printk("[upmu_set_vrf18_2_en(0)] ");
                upmu_set_vrf18_2_en(0);        
                read_auxadc_value();   
                
                printk("[upmu_set_vrf18_2_en(1)] ");
                upmu_set_vrf18_2_en(1);        
                read_auxadc_value();
                break;    

            case 1:    
                for(j=0;j<=7;j++)
                {
                    printk("[upmu_set_vrf18_2_en_sel] %d\n", j);
                    upmu_set_vrf18_2_en_sel(j);

                    g_buck_num=7;
                    do_scrxxx_map(j);
                    g_buck_num=100;                    
                }
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }  
}
#endif
//////////////////////////////////////////////////////////////////////
#if 1
void do_vproc_vosel_subtest(void)
{
    int i;

    for(i=0;i<=PMIC_VPROC_VOSEL_SLEEP_MASK;i++) 
    { 
        upmu_set_vproc_vosel_sleep(i); 
        printk("[upmu_set_vproc_vosel_sleep] i=%d, ",i);
        g_buck_num=0;
        read_auxadc_value();
        g_buck_num=100;
    }

    upmu_set_vproc_track_on_ctrl(0);
    
    for(i=0;i<=PMIC_VPROC_VOSEL_ON_MASK;i++) 
    { 
        upmu_set_vproc_vosel_on(i); 
        printk("[upmu_set_vproc_vosel_on] i=%d, ",i);
        g_buck_num=0;
        read_auxadc_value();
        g_buck_num=100;
    }    
    
}

void do_vsram_vosel_subtest(void)
{
    int i;

    upmu_set_vsram_track_sleep_ctrl(0);
    upmu_set_vsram_track_on_ctrl(0);

    for(i=0;i<=PMIC_VSRAM_VOSEL_SLEEP_MASK;i++) 
    { 
        upmu_set_vsram_vosel_sleep(i); 
        printk("[upmu_set_vsram_vosel_sleep] i=%d, ",i);
        g_buck_num=1;
        read_auxadc_value();
        g_buck_num=100;
    }
    for(i=0;i<=PMIC_VSRAM_VOSEL_ON_MASK;i++) 
    { 
        upmu_set_vsram_vosel_on(i); 
        printk("[upmu_set_vsram_vosel_on] i=%d, ",i);
        g_buck_num=1;
        read_auxadc_value();
        g_buck_num=100;
    }
    
}

void do_vcore_vosel_subtest(void)
{
    int i;
    
    for(i=0;i<=PMIC_VCORE_VOSEL_SLEEP_MASK;i++) 
    { 
        upmu_set_vcore_vosel_sleep(i); 
        printk("[upmu_set_vcore_vosel_sleep] i=%d, ",i);
        g_buck_num=2; 
        read_auxadc_value();
        g_buck_num=100;
    }
    for(i=0;i<=PMIC_VCORE_VOSEL_ON_MASK;i++) 
    { 
        upmu_set_vcore_vosel_on(i); 
        printk("[upmu_set_vcore_vosel_on] i=%d, ",i);
        g_buck_num=2;
        read_auxadc_value();
        g_buck_num=100;
    }
}

void do_vm_vosel_subtest(void)
{
    int i;
    
    for(i=0;i<=PMIC_VM_VOSEL_SLEEP_MASK;i++) 
    { 
        upmu_set_vm_vosel_sleep(i); 
        printk("[upmu_set_vm_vosel_sleep] i=%d, ",i);
        g_buck_num=3;
        read_auxadc_value();
        g_buck_num=100;
    }
    for(i=0;i<=PMIC_VM_VOSEL_ON_MASK;i++) 
    { 
        upmu_set_vm_vosel_on(i); 
        printk("[upmu_set_vm_vosel_on] i=%d, ",i);
        g_buck_num=3;
        read_auxadc_value();
        g_buck_num=100;
    }
}

void do_vio18_vosel_subtest(void)
{
    int i;
    
    for(i=0;i<=PMIC_VIO18_VOSEL_SLEEP_MASK;i++) 
    { 
        upmu_set_vio18_vosel_sleep(i); 
        printk("[upmu_set_vio18_vosel_sleep] i=%d, ",i);
        g_buck_num=4; 
        read_auxadc_value();
        g_buck_num=100;
    }
    for(i=0;i<=PMIC_VIO18_VOSEL_ON_MASK;i++) 
    { 
        upmu_set_vio18_vosel_on(i); 
        printk("[upmu_set_vio18_vosel_on] i=%d, ",i);
        g_buck_num=4;
        read_auxadc_value();
        g_buck_num=100;
    }
}

void do_vpa_vosel_subtest(void)
{
    int i;
    
    for(i=0;i<=PMIC_VPA_VOSEL_SLEEP_MASK;i++) 
    { 
        upmu_set_vpa_vosel_sleep(i); 
        printk("[upmu_set_vpa_vosel_sleep] i=%d, ",i);
        g_buck_num=5;
        read_auxadc_value();
        g_buck_num=100;
    }
    for(i=0;i<=PMIC_VPA_VOSEL_ON_MASK;i++) 
    { 
        upmu_set_vpa_vosel_on(i); 
        printk("[upmu_set_vpa_vosel_on] i=%d, ",i);
        g_buck_num=5;
        read_auxadc_value();
        g_buck_num=100;
    }
}

void do_vrf18_vosel_subtest(void)
{
    int i;
    
    for(i=0;i<=PMIC_VRF18_VOSEL_SLEEP_MASK;i++) 
    { 
        upmu_set_vrf18_vosel_sleep(i); 
        printk("[upmu_set_vrf18_vosel_sleep] i=%d, ",i);
        g_buck_num=6;
        read_auxadc_value();
        g_buck_num=100;
    }
    for(i=0;i<=PMIC_VRF18_VOSEL_ON_MASK;i++) 
    { 
        upmu_set_vrf18_vosel_on(i); 
        printk("[upmu_set_vrf18_vosel_on] i=%d, ",i);
        g_buck_num=6;
        read_auxadc_value();
        g_buck_num=100;
    }
}

void do_vrf18_2_vosel_subtest(void)
{
    int i;
    
    for(i=0;i<=PMIC_VRF18_2_VOSEL_SLEEP_MASK;i++) 
    { 
        upmu_set_vrf18_2_vosel_sleep(i); 
        printk("[upmu_set_vrf18_2_vosel_sleep] i=%d, ",i);
        g_buck_num=7;
        read_auxadc_value();
        g_buck_num=100;
    }
    for(i=0;i<=PMIC_VRF18_2_VOSEL_ON_MASK;i++) 
    { 
        upmu_set_vrf18_2_vosel_on(i); 
        printk("[upmu_set_vrf18_2_vosel_on] i=%d, ",i);
        g_buck_num=7;
        read_auxadc_value();
        g_buck_num=100;
    }
}

void do_vosel_subtest(int index_val)
{
    g_log_buck_vosel=1;

    switch(index_val){
        case 0:
            do_vproc_vosel_subtest();
            break;
        case 1:
            do_vsram_vosel_subtest();
            break;
        case 2:
            do_vcore_vosel_subtest();
            break;
        case 3:
            do_vm_vosel_subtest();
            break;
        case 4:
            do_vio18_vosel_subtest();
            break;
        case 5:
            do_vpa_vosel_subtest();
            break;
        case 6:
            do_vrf18_vosel_subtest();
            break;
        case 7:
            do_vrf18_2_vosel_subtest();
            break;
        default:
            printk("[do_vosel_subtest] Invalid channel value(%d)\n", index_val);
            break;     
    }

    g_log_buck_vosel=0;
}

void do_scrxxx_map_vosel(int index_val)
{
    int j;
    
    for(j=0;j<=7;j++)
    {
        switch(index_val){
            case 0:    
                printk("[upmu_set_vproc_vosel_sel] %d\n", j);
                upmu_set_vproc_vosel_sel(j);
                break;                
            case 1:    
                printk("[upmu_set_vsram_vosel_sel] %d\n", j);
                upmu_set_vsram_vosel_sel(j);
                break;
            case 2:    
                printk("[upmu_set_vcore_vosel_sel] %d\n", j);
                upmu_set_vcore_vosel_sel(j);
                break;
            case 3:    
                printk("[upmu_set_vm_vosel_sel] %d\n", j);
                upmu_set_vm_vosel_sel(j);
                break;
            case 4:    
                printk("[upmu_set_vio18_vosel_sel] %d\n", j);
                upmu_set_vio18_vosel_sel(j);
                break;                
            case 5:    
                printk("[upmu_set_vpa_vosel_sel] %d\n", j);
                upmu_set_vpa_vosel_sel(j);
                break;
            case 6:    
                printk("[upmu_set_vrf18_vosel_sel] %d\n", j);
                upmu_set_vrf18_vosel_sel(j);
                break;
            case 7:    
                printk("[upmu_set_vrf18_2_vosel_sel] %d\n", j);
                upmu_set_vrf18_2_vosel_sel(j);
                break;
            default:
                printk("[do_scrxxx_map] Invalid index_val value(%d)\n", index_val);
                break;    
        }

        switch(j){
            case 0:
                printk("[upmu_set_rg_srcvolt_en(0)]\n");
                upmu_set_rg_srcvolt_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1)]\n");
                upmu_set_rg_srcvolt_en(1);
                do_vosel_subtest(index_val);
                break;
                
            case 1:
                printk("[upmu_set_rg_srclkperi_en(0)]\n");
                upmu_set_rg_srclkperi_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(1)]\n");
                upmu_set_rg_srclkperi_en(1);
                do_vosel_subtest(index_val);
                break;
                
            case 2:
                printk("[upmu_set_rg_srclkmd2_en(0)]\n");
                upmu_set_rg_srclkmd2_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srclkmd2_en(1)]\n");
                upmu_set_rg_srclkmd2_en(1);
                do_vosel_subtest(index_val);                           
                break;
                
            case 3:
                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1);
                do_vosel_subtest(index_val);
                break;
                
            case 4:
                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);
                do_vosel_subtest(index_val);
                break;
                
            case 5:
                printk("[upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
                do_vosel_subtest(index_val);
                break;
                
            case 6:
                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
                do_vosel_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
                do_vosel_subtest(index_val);
                break;
                
            case 7:
                printk("[0]\n");
                do_vosel_subtest(index_val);
                break;
                
            default:
                printk("[do_scrxxx_map] Invalid channel value(%d)\n", j);
                break;    
        }                    
    }
}

void do_vproc_vosel_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vproc_vosel_ctrl] %d\n", i);
        upmu_set_vproc_vosel_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VPROC_VOSEL_MASK;j++)
                {
                    upmu_set_vproc_vosel(j);
                    printk("[upmu_set_vproc_vosel] j=%d, ",j);
                    read_auxadc_value();
                }
                break;    

            case 1:
                do_scrxxx_map_vosel(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }  
}

void do_vsram_vosel_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vsram_vosel_ctrl] %d\n", i);
        upmu_set_vsram_vosel_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VSRAM_VOSEL_MASK;j++)
                {
                    upmu_set_vsram_vosel(j);
                    printk("[upmu_set_vsram_vosel] j=%d, ",j);
                    read_auxadc_value();
                }
                break;    

            case 1:
                do_scrxxx_map_vosel(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }  
}

void do_vcore_vosel_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vcore_vosel_ctrl] %d\n", i);
        upmu_set_vcore_vosel_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VCORE_VOSEL_MASK;j++)
                {
                    upmu_set_vcore_vosel(j);
                    printk("[upmu_set_vcore_vosel] j=%d, ",j);
                    read_auxadc_value();
                }
                break;    

            case 1:
                do_scrxxx_map_vosel(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }   
}

void do_vm_vosel_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vm_vosel_ctrl] %d\n", i);
        upmu_set_vm_vosel_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VM_VOSEL_MASK;j++)
                {
                    upmu_set_vm_vosel(j);
                    printk("[upmu_set_vm_vosel] j=%d, ",j);
                    read_auxadc_value();
                }
                break;    

            case 1:
                do_scrxxx_map_vosel(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}

void do_vio18_vosel_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vio18_vosel_ctrl] %d\n", i);
        upmu_set_vio18_vosel_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VIO18_VOSEL_MASK;j++)
                {
                    upmu_set_vio18_vosel(j);
                    printk("[upmu_set_vio18_vosel] j=%d, ",j);
                    read_auxadc_value();
                }
                break;    

            case 1:
                do_scrxxx_map_vosel(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }   
}

void do_vpa_vosel_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vpa_vosel_ctrl] %d\n", i);
        upmu_set_vpa_vosel_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VPA_VOSEL_MASK;j++)
                {
                    upmu_set_vpa_vosel(j);
                    printk("[upmu_set_vpa_vosel] j=%d, ",j);
                    read_auxadc_value();
                }
                break;    

            case 1:
                do_scrxxx_map_vosel(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }    
}

void do_vrf18_vosel_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vrf18_vosel_ctrl] %d\n", i);
        upmu_set_vrf18_vosel_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VRF18_VOSEL_MASK;j++)
                {
                    upmu_set_vrf18_vosel(j);
                    printk("[upmu_set_vrf18_vosel] j=%d, ",j);
                    read_auxadc_value();
                }
                break;    

            case 1:
                do_scrxxx_map_vosel(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }     
}

void do_vrf18_2_vosel_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[upmu_set_vrf18_2_vosel_ctrl] %d\n", i);
        upmu_set_vrf18_2_vosel_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VRF18_2_VOSEL_MASK;j++)
                {
                    upmu_set_vrf18_2_vosel(j);
                    printk("[upmu_set_vrf18_2_vosel] j=%d, ",j);
                    read_auxadc_value();
                }
                break;    

            case 1:
                do_scrxxx_map_vosel(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }      
}
#endif
//////////////////////////////////////////////////////////////////////
#if 1
void do_vproc_dlc_subtest(void)
{
    int i; 

    printk("[do_vproc_dlc_subtest]\n");
    
    for(i=0;i<=PMIC_VPROC_DLC_SLEEP_MASK;i++)
    {
        upmu_set_vproc_dlc_sleep(i);
        upmu_set_vproc_dlc_n_sleep(i);

        printk("[do_vproc_dlc_subtest] upmu_set_vproc_dlc_sleep=%d, upmu_set_vproc_dlc_n_sleep=%d\n", i, i);

        printk("[do_vproc_dlc_subtest] upmu_get_qi_vproc_dlc=%d, upmu_get_qi_vproc_dlc_n=%d\n",
            upmu_get_qi_vproc_dlc(), upmu_get_qi_vproc_dlc_n());

    }

    printk("\n");

    for(i=0;i<=PMIC_VPROC_DLC_ON_MASK;i++)
    {
        upmu_set_vproc_dlc_on(i);
        upmu_set_vproc_dlc_n_on(i);

        printk("[do_vproc_dlc_subtest] upmu_set_vproc_dlc_on=%d, upmu_set_vproc_dlc_n_on=%d\n", i, i);

        printk("[do_vproc_dlc_subtest] upmu_get_qi_vproc_dlc=%d, upmu_get_qi_vproc_dlc_n=%d\n",
            upmu_get_qi_vproc_dlc(), upmu_get_qi_vproc_dlc_n());                   
    }

    printk("\n");

}
void do_vsram_dlc_subtest(void)
{
    int i; 
    
    printk("[do_vsram_dlc_subtest]\n");
    
    for(i=0;i<=PMIC_VSRAM_DLC_SLEEP_MASK;i++)
    {
        upmu_set_vsram_dlc_sleep(i);
        upmu_set_vsram_dlc_n_sleep(i);
    
        printk("[do_vsram_dlc_subtest] upmu_set_vsram_dlc_sleep=%d, upmu_set_vsram_dlc_n_sleep=%d\n", i, i);
    
        printk("[do_vsram_dlc_subtest] upmu_get_qi_vsram_dlc=%d, upmu_get_qi_vsram_dlc_n=%d\n",
            upmu_get_qi_vsram_dlc(), upmu_get_qi_vsram_dlc_n());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VSRAM_DLC_ON_MASK;i++)
    {
        upmu_set_vsram_dlc_on(i);
        upmu_set_vsram_dlc_n_on(i);
    
        printk("[do_vsram_dlc_subtest] upmu_set_vsram_dlc_on=%d, upmu_set_vsram_dlc_n_on=%d\n", i, i);
    
        printk("[do_vsram_dlc_subtest] upmu_get_qi_vsram_dlc=%d, upmu_get_qi_vsram_dlc_n=%d\n",
            upmu_get_qi_vsram_dlc(), upmu_get_qi_vsram_dlc_n());                   
    }

    printk("\n");

}
void do_vcore_dlc_subtest(void)
{
    int i; 
    
    printk("[do_vcore_dlc_subtest]\n");
    
    for(i=0;i<=PMIC_VCORE_DLC_SLEEP_MASK;i++)
    {
        upmu_set_vcore_dlc_sleep(i);
        upmu_set_vcore_dlc_n_sleep(i);
    
        printk("[do_vcore_dlc_subtest] upmu_set_vcore_dlc_sleep=%d, upmu_set_vcore_dlc_n_sleep=%d\n", i, i);
    
        printk("[do_vcore_dlc_subtest] upmu_get_qi_vcore_dlc=%d, upmu_get_qi_vcore_dlc_n=%d\n",
            upmu_get_qi_vcore_dlc(), upmu_get_qi_vcore_dlc_n());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VCORE_DLC_ON_MASK;i++)
    {
        upmu_set_vcore_dlc_on(i);
        upmu_set_vcore_dlc_n_on(i);
    
        printk("[do_vcore_dlc_subtest] upmu_set_vcore_dlc_on=%d, upmu_set_vcore_dlc_n_on=%d\n", i, i);
    
        printk("[do_vcore_dlc_subtest] upmu_get_qi_vcore_dlc=%d, upmu_get_qi_vcore_dlc_n=%d\n",
            upmu_get_qi_vcore_dlc(), upmu_get_qi_vcore_dlc_n());                   
    }

    printk("\n");

}
void do_vm_dlc_subtest(void)
{
    int i; 
    
    printk("[do_vm_dlc_subtest]\n");
    
    for(i=0;i<=PMIC_VM_DLC_SLEEP_MASK;i++)
    {
        upmu_set_vm_dlc_sleep(i);
        upmu_set_vm_dlc_n_sleep(i);
    
        printk("[do_vm_dlc_subtest] upmu_set_vm_dlc_sleep=%d, upmu_set_vm_dlc_n_sleep=%d\n", i, i);
    
        printk("[do_vm_dlc_subtest] upmu_get_qi_vm_dlc=%d, upmu_get_qi_vm_dlc_n=%d\n",
            upmu_get_qi_vm_dlc(), upmu_get_qi_vm_dlc_n());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VM_DLC_ON_MASK;i++)
    {
        upmu_set_vm_dlc_on(i);
        upmu_set_vm_dlc_n_on(i);
    
        printk("[do_vm_dlc_subtest] upmu_set_vm_dlc_on=%d, upmu_set_vm_dlc_n_on=%d\n", i, i);
    
        printk("[do_vm_dlc_subtest] upmu_get_qi_vm_dlc=%d, upmu_get_qi_vm_dlc_n=%d\n",
            upmu_get_qi_vm_dlc(), upmu_get_qi_vm_dlc_n());                   
    }

    printk("\n");

}
void do_vio18_dlc_subtest(void)
{
    int i; 
    
    printk("[do_vio18_dlc_subtest]\n");
    
    for(i=0;i<=PMIC_VIO18_DLC_SLEEP_MASK;i++)
    {
        upmu_set_vio18_dlc_sleep(i);
        upmu_set_vio18_dlc_n_sleep(i);
    
        printk("[do_vio18_dlc_subtest] upmu_set_vio18_dlc_sleep=%d, upmu_set_vio18_dlc_n_sleep=%d\n", i, i);
    
        printk("[do_vio18_dlc_subtest] upmu_get_qi_vio18_dlc=%d, upmu_get_qi_vio18_dlc_n=%d\n",
            upmu_get_qi_vio18_dlc(), upmu_get_qi_vio18_dlc_n());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VIO18_DLC_ON_MASK;i++)
    {
        upmu_set_vio18_dlc_on(i);
        upmu_set_vio18_dlc_n_on(i);
    
        printk("[do_vio18_dlc_subtest] upmu_set_vio18_dlc_on=%d, upmu_set_vio18_dlc_n_on=%d\n", i, i);
    
        printk("[do_vio18_dlc_subtest] upmu_get_qi_vio18_dlc=%d, upmu_get_qi_vio18_dlc_n=%d\n",
            upmu_get_qi_vio18_dlc(), upmu_get_qi_vio18_dlc_n());                   
    }

    printk("\n");

}
void do_vpa_dlc_subtest(void)
{
    int i; 
    
    printk("[do_vpa_dlc_subtest]\n");
    
    for(i=0;i<=PMIC_VPA_DLC_SLEEP_MASK;i++)
    {
        upmu_set_vpa_dlc_sleep(i);
    
        printk("[do_vpa_dlc_subtest] upmu_set_vpa_dlc_sleep=%d\n", i);
    
        printk("[do_vpa_dlc_subtest] upmu_get_qi_vpa_dlc=%d\n",
            upmu_get_qi_vpa_dlc());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VPA_DLC_ON_MASK;i++)
    {
        upmu_set_vpa_dlc_on(i);
    
        printk("[do_vpa_dlc_subtest] upmu_set_vpa_dlc_on=%d\n", i);
    
        printk("[do_vpa_dlc_subtest] upmu_get_qi_vpa_dlc=%d\n",
            upmu_get_qi_vpa_dlc());                   
    }

    printk("\n");

}
void do_vrf18_dlc_subtest(void)
{
    int i; 
    
    printk("[do_vrf18_dlc_subtest]\n");
    
    for(i=0;i<=PMIC_VRF18_DLC_SLEEP_MASK;i++)
    {
        upmu_set_vrf18_dlc_sleep(i);
        upmu_set_vrf18_dlc_n_sleep(i);
    
        printk("[do_vrf18_dlc_subtest] upmu_set_vrf18_dlc_sleep=%d, upmu_set_vrf18_dlc_n_sleep=%d\n", i, i);
    
        printk("[do_vrf18_dlc_subtest] upmu_get_qi_vrf18_dlc=%d, upmu_get_qi_vrf18_dlc_n=%d\n",
            upmu_get_qi_vrf18_dlc(), upmu_get_qi_vrf18_dlc_n());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VRF18_DLC_ON_MASK;i++)
    {
        upmu_set_vrf18_dlc_on(i);
        upmu_set_vrf18_dlc_n_on(i);
    
        printk("[do_vrf18_dlc_subtest] upmu_set_vrf18_dlc_on=%d, upmu_set_vrf18_dlc_n_on=%d\n", i, i);
    
        printk("[do_vrf18_dlc_subtest] upmu_get_qi_vrf18_dlc=%d, upmu_get_qi_vrf18_dlc_n=%d\n",
            upmu_get_qi_vrf18_dlc(), upmu_get_qi_vrf18_dlc_n());                   
    }

    printk("\n");

}
void do_vrf18_2_dlc_subtest(void)
{
    int i; 
    
    printk("[do_vrf18_2_dlc_subtest]\n");
    
    for(i=0;i<=PMIC_VRF18_2_DLC_SLEEP_MASK;i++)
    {
        upmu_set_vrf18_2_dlc_sleep(i);
        upmu_set_vrf18_2_dlc_n_sleep(i);
    
        printk("[do_vrf18_2_dlc_subtest] upmu_set_vrf18_2_dlc_sleep=%d, upmu_set_vrf18_2_dlc_n_sleep=%d\n", i, i);
    
        printk("[do_vrf18_2_dlc_subtest] upmu_get_qi_vrf18_2_dlc=%d, upmu_get_qi_vrf18_2_dlc_n=%d\n",
            upmu_get_qi_vrf18_2_dlc(), upmu_get_qi_vrf18_2_dlc_n());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VRF18_2_DLC_ON_MASK;i++)
    {
        upmu_set_vrf18_2_dlc_on(i);
        upmu_set_vrf18_2_dlc_n_on(i);
    
        printk("[do_vrf18_2_dlc_subtest] upmu_set_vrf18_2_dlc_on=%d, upmu_set_vrf18_2_dlc_n_on=%d\n", i, i);
    
        printk("[do_vrf18_2_dlc_subtest] upmu_get_qi_vrf18_2_dlc=%d, upmu_get_qi_vrf18_2_dlc_n=%d\n",
            upmu_get_qi_vrf18_2_dlc(), upmu_get_qi_vrf18_2_dlc_n());                   
    }

    printk("\n");

}

void do_dlc_subtest(int index_val)
{
    switch(index_val){
        case 0:
            do_vproc_dlc_subtest();
            break;
        case 1:
            do_vsram_dlc_subtest();
            break;
        case 2:
            do_vcore_dlc_subtest();
            break;
        case 3:
            do_vm_dlc_subtest();
            break;
        case 4:
            do_vio18_dlc_subtest();
            break;
        case 5:
            do_vpa_dlc_subtest();
            break;
        case 6:
            do_vrf18_dlc_subtest();
            break;
        case 7:
            do_vrf18_2_dlc_subtest();
            break;
        default:
            printk("[do_dlc_subtest] Invalid channel value(%d)\n", index_val);
            break;     
    }
}

void do_scrxxx_map_dlc(int index_val)
{
    int j;
    
    for(j=0;j<=7;j++)
    {
        switch(index_val){
            case 0:    
                printk("[upmu_set_vproc_dlc_sel] %d\n", j);
                upmu_set_vproc_dlc_sel(j);
                break;                
            case 1:    
                printk("[upmu_set_vsram_dlc_sel] %d\n", j);
                upmu_set_vsram_dlc_sel(j);
                break;
            case 2:    
                printk("[upmu_set_vcore_dlc_sel] %d\n", j);
                upmu_set_vcore_dlc_sel(j);
                break;
            case 3:    
                printk("[upmu_set_vm_dlc_sel] %d\n", j);
                upmu_set_vm_dlc_sel(j);
                break;
            case 4:    
                printk("[upmu_set_vio18_dlc_sel] %d\n", j);
                upmu_set_vio18_dlc_sel(j);
                break;                
            case 5:    
                printk("[upmu_set_vpa_dlc_sel] %d\n", j);
                upmu_set_vpa_dlc_sel(j);
                break;
            case 6:    
                printk("[upmu_set_vrf18_dlc_sel] %d\n", j);
                upmu_set_vrf18_dlc_sel(j);
                break;
            case 7:    
                printk("[upmu_set_vrf18_2_dlc_sel] %d\n", j);
                upmu_set_vrf18_2_dlc_sel(j);
                break;
            default:
                printk("[do_scrxxx_map] Invalid index_val value(%d)\n", index_val);
                break;    
        }

        switch(j){
            case 0:
                printk("[upmu_set_rg_srcvolt_en(0)]\n");
                upmu_set_rg_srcvolt_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1)]\n");
                upmu_set_rg_srcvolt_en(1);
                do_dlc_subtest(index_val);
                break;
                
            case 1:
                printk("[upmu_set_rg_srclkperi_en(0)]\n");
                upmu_set_rg_srclkperi_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(1)]\n");
                upmu_set_rg_srclkperi_en(1);
                do_dlc_subtest(index_val);
                break;
                
            case 2:
                printk("[upmu_set_rg_srclkmd2_en(0)]\n");
                upmu_set_rg_srclkmd2_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srclkmd2_en(1)]\n");
                upmu_set_rg_srclkmd2_en(1);
                do_dlc_subtest(index_val);                           
                break;
                
            case 3:
                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1);
                do_dlc_subtest(index_val);
                break;
                
            case 4:
                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);
                do_dlc_subtest(index_val);
                break;
                
            case 5:
                printk("[upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
                do_dlc_subtest(index_val);
                break;
                
            case 6:
                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
                do_dlc_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
                do_dlc_subtest(index_val);
                break;
                
            case 7:
                printk("[0]\n");
                do_dlc_subtest(index_val);
                break;
                
            default:
                printk("[do_scrxxx_map] Invalid channel value(%d)\n", j);
                break;    
        }                    
    }
}

void do_vproc_dlc_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[do_vproc_dlc_test] %d\n", i);
        upmu_set_vproc_dlc_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VPROC_DLC_MASK;j++)
                {
                    upmu_set_vproc_dlc(j);
                    upmu_set_vproc_dlc_n(j);

                    if( (upmu_get_qi_vproc_dlc()!=j) || (upmu_get_qi_vproc_dlc_n()!=j) )
                    {
                        printk("[do_vproc_dlc_test] fail at upmu_get_qi_vproc_dlc=%d, upmu_get_qi_vproc_dlc_n=%d\n",
                            upmu_get_qi_vproc_dlc(), upmu_get_qi_vproc_dlc_n());
                    }

                    printk("[do_vproc_dlc_test] upmu_set_vproc_dlc=%d, upmu_set_vproc_dlc_n=%d, upmu_get_qi_vproc_dlc=%d, upmu_get_qi_vproc_dlc_n=%d\n",
                            j, j, upmu_get_qi_vproc_dlc(), upmu_get_qi_vproc_dlc_n());
                }
                break;    

            case 1:
                do_scrxxx_map_dlc(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }    
}
void do_vsram_dlc_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vsram_dlc_test] %d\n", i);
        upmu_set_vsram_dlc_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VSRAM_DLC_MASK;j++)
                {
                    upmu_set_vsram_dlc(j);
                    upmu_set_vsram_dlc_n(j);

                    if( (upmu_get_qi_vsram_dlc()!=j) || (upmu_get_qi_vsram_dlc_n()!=j) )
                    {
                        printk("[do_vsram_dlc_test] fail at upmu_get_qi_vsram_dlc=%d, upmu_get_qi_vsram_dlc_n=%d\n",
                            upmu_get_qi_vsram_dlc(), upmu_get_qi_vsram_dlc_n());
                    }

                    printk("[do_vsram_dlc_test] upmu_set_vsram_dlc=%d, upmu_set_vsram_dlc_n=%d, upmu_get_qi_vsram_dlc=%d, upmu_get_qi_vsram_dlc_n=%d\n",
                            j, j, upmu_get_qi_vsram_dlc(), upmu_get_qi_vsram_dlc_n());
                }
                break;    

            case 1:
                do_scrxxx_map_dlc(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vcore_dlc_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vcore_dlc_test] %d\n", i);
        upmu_set_vcore_dlc_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VCORE_DLC_MASK;j++)
                {
                    upmu_set_vcore_dlc(j);
                    upmu_set_vcore_dlc_n(j);
    
                    if( (upmu_get_qi_vcore_dlc()!=j) || (upmu_get_qi_vcore_dlc_n()!=j) )
                    {
                        printk("[do_vcore_dlc_test] fail at upmu_get_qi_vcore_dlc=%d, upmu_get_qi_vcore_dlc_n=%d\n",
                            upmu_get_qi_vcore_dlc(), upmu_get_qi_vcore_dlc_n());
                    }

                    printk("[do_vcore_dlc_test] upmu_set_vcore_dlc=%d, upmu_set_vcore_dlc_n=%d, upmu_get_qi_vcore_dlc=%d, upmu_get_qi_vcore_dlc_n=%d\n",
                            j, j, upmu_get_qi_vcore_dlc(), upmu_get_qi_vcore_dlc_n());
                }
                break;    
    
            case 1:
                do_scrxxx_map_dlc(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vm_dlc_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vm_dlc_test] %d\n", i);
        upmu_set_vm_dlc_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VM_DLC_MASK;j++)
                {
                    upmu_set_vm_dlc(j);
                    upmu_set_vm_dlc_n(j);
    
                    if( (upmu_get_qi_vm_dlc()!=j) || (upmu_get_qi_vm_dlc_n()!=j) )
                    {
                        printk("[do_vm_dlc_test] fail at upmu_get_qi_vm_dlc=%d, upmu_get_qi_vm_dlc_n=%d\n",
                            upmu_get_qi_vm_dlc(), upmu_get_qi_vm_dlc_n());
                    }

                    printk("[do_vm_dlc_test] upmu_set_vm_dlc=%d, upmu_set_vm_dlc_n=%d, upmu_get_qi_vm_dlc=%d, upmu_get_qi_vm_dlc_n=%d\n",
                            j, j, upmu_get_qi_vm_dlc(), upmu_get_qi_vm_dlc_n());
                }
                break;    
    
            case 1:
                do_scrxxx_map_dlc(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vio18_dlc_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vio18_dlc_test] %d\n", i);
        upmu_set_vio18_dlc_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VIO18_DLC_MASK;j++)
                {
                    upmu_set_vio18_dlc(j);
                    upmu_set_vio18_dlc_n(j);
    
                    if( (upmu_get_qi_vio18_dlc()!=j) || (upmu_get_qi_vio18_dlc_n()!=j) )
                    {
                        printk("[do_vio18_dlc_test] fail at upmu_get_qi_vio18_dlc=%d, upmu_get_qi_vio18_dlc_n=%d\n",
                            upmu_get_qi_vio18_dlc(), upmu_get_qi_vio18_dlc_n());
                    }

                    printk("[do_vio18_dlc_test] upmu_set_vio18_dlc=%d, upmu_set_vio18_dlc_n=%d, upmu_get_qi_vio18_dlc=%d, upmu_get_qi_vio18_dlc_n=%d\n",
                            j, j, upmu_get_qi_vio18_dlc(), upmu_get_qi_vio18_dlc_n());
                }
                break;    
    
            case 1:
                do_scrxxx_map_dlc(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vpa_dlc_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vpa_dlc_test] %d\n", i);
        upmu_set_vpa_dlc_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VPA_DLC_MASK;j++)
                {
                    upmu_set_vpa_dlc(j);
    
                    if( (upmu_get_qi_vpa_dlc()!=j) )
                    {
                        printk("[do_vpa_dlc_test] fail at upmu_get_qi_vpa_dlc=%d\n",
                            upmu_get_qi_vpa_dlc());
                    }

                    printk("[do_vpa_dlc_test] upmu_set_vpa_dlc=%d, upmu_get_qi_vpa_dlc=%d\n",
                            j, upmu_get_qi_vpa_dlc());
                }
                break;    
    
            case 1:
                do_scrxxx_map_dlc(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 

    //------------------------------------------------------------------------
    printk("[do_vpa_dlc_test] special case : upmu_set_vpa_dlc_map_en(1);\n");    
    upmu_set_vpa_dlc_map_en(1);
    upmu_set_vpa_vosel_ctrl(0);
    for(i=0;i<=PMIC_VPA_VOSEL_MASK;i++)
    {
        upmu_set_vpa_vosel(i);
        printk("upmu_set_vpa_vosel=%d, upmu_get_qi_vpa_dlc=%d\n", i, upmu_get_qi_vpa_dlc());
    }
    //------------------------------------------------------------------------
    
}
void do_vrf18_dlc_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vrf18_dlc_test] %d\n", i);
        upmu_set_vrf18_dlc_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VRF18_DLC_MASK;j++)
                {
                    upmu_set_vrf18_dlc(j);
                    upmu_set_vrf18_dlc_n(j);
    
                    if( (upmu_get_qi_vrf18_dlc()!=j) || (upmu_get_qi_vrf18_dlc_n()!=j) )
                    {
                        printk("[do_vrf18_dlc_test] fail at upmu_get_qi_vrf18_dlc=%d, upmu_get_qi_vrf18_dlc_n=%d\n",
                            upmu_get_qi_vrf18_dlc(), upmu_get_qi_vrf18_dlc_n());
                    }

                    printk("[do_vrf18_dlc_test] upmu_set_vrf18_dlc=%d, upmu_set_vrf18_dlc_n=%d, upmu_get_qi_vrf18_dlc=%d, upmu_get_qi_vrf18_dlc_n=%d\n",
                            j, j, upmu_get_qi_vrf18_dlc(), upmu_get_qi_vrf18_dlc_n());
                }
                break;    
    
            case 1:
                do_scrxxx_map_dlc(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vrf18_2_dlc_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vrf18_2_dlc_test] %d\n", i);
        upmu_set_vrf18_2_dlc_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VRF18_2_DLC_MASK;j++)
                {
                    upmu_set_vrf18_2_dlc(j);
                    upmu_set_vrf18_2_dlc_n(j);
    
                    if( (upmu_get_qi_vrf18_2_dlc()!=j) || (upmu_get_qi_vrf18_2_dlc_n()!=j) )
                    {
                        printk("[do_vrf18_2_dlc_test] fail at upmu_get_qi_vrf18_2_dlc=%d, upmu_get_qi_vrf18_2_dlc_n=%d\n",
                            upmu_get_qi_vrf18_2_dlc(), upmu_get_qi_vrf18_2_dlc_n());
                    }

                    printk("[do_vrf18_2_dlc_test] upmu_set_vrf18_2_dlc=%d, upmu_set_vrf18_2_dlc_n=%d, upmu_get_qi_vrf18_2_dlc=%d, upmu_get_qi_vrf18_2_dlc_n=%d\n",
                            j, j, upmu_get_qi_vrf18_2_dlc(), upmu_get_qi_vrf18_2_dlc_n());
                }
                break;    
    
            case 1:
                do_scrxxx_map_dlc(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
#endif

#if 1
void do_vproc_burst_subtest(void)
{
    int i; 

    printk("[do_vproc_burst_subtest]\n");
    
    for(i=0;i<=PMIC_VPROC_BURST_SLEEP_MASK;i++)
    {
        upmu_set_vproc_burst_sleep(i);

        printk("[do_vproc_burst_subtest] upmu_set_vproc_bursth_sleep=%d\n", i);

        printk("[do_vproc_burst_subtest] upmu_get_qi_vproc_burst=%d\n",
            upmu_get_qi_vproc_burst());

    }

    printk("\n");

    for(i=0;i<=PMIC_VPROC_BURST_ON_MASK;i++)
    {
        upmu_set_vproc_burst_on(i);

        printk("[do_vproc_burst_subtest] upmu_set_vproc_bursth_on=%d\n", i);

        printk("[do_vproc_burst_subtest] upmu_get_qi_vproc_burst=%d\n",
            upmu_get_qi_vproc_burst());                   
    }

    printk("\n");

}
void do_vsram_burst_subtest(void)
{
    int i; 
    
    printk("[do_vsram_burst_subtest]\n");
    
    for(i=0;i<=PMIC_VSRAM_BURST_SLEEP_MASK;i++)
    {
        upmu_set_vsram_burst_sleep(i);
    
        printk("[do_vsram_burst_subtest] upmu_set_vsram_bursth_sleep=%d\n", i);
    
        printk("[do_vsram_burst_subtest] upmu_get_qi_vsram_burst=%d\n",
            upmu_get_qi_vsram_burst());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VSRAM_BURST_ON_MASK;i++)
    {
        upmu_set_vsram_burst_on(i);
    
        printk("[do_vsram_burst_subtest] upmu_set_vsram_bursth_on=%d\n", i);
    
        printk("[do_vsram_burst_subtest] upmu_get_qi_vsram_burst=%d\n",
            upmu_get_qi_vsram_burst());                   
    }

    printk("\n");

}
void do_vcore_burst_subtest(void)
{
    int i; 
    
    printk("[do_vcore_burst_subtest]\n");
    
    for(i=0;i<=PMIC_VCORE_BURST_SLEEP_MASK;i++)
    {
        upmu_set_vcore_burst_sleep(i);
    
        printk("[do_vcore_burst_subtest] upmu_set_vcore_bursth_sleep=%d\n", i);
    
        printk("[do_vcore_burst_subtest] upmu_get_qi_vcore_burst=%d\n",
            upmu_get_qi_vcore_burst());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VCORE_BURST_ON_MASK;i++)
    {
        upmu_set_vcore_burst_on(i);
    
        printk("[do_vcore_burst_subtest] upmu_set_vcore_bursth_on=%d\n", i);
    
        printk("[do_vcore_burst_subtest] upmu_get_qi_vcore_burst=%d\n",
            upmu_get_qi_vcore_burst());                   
    }

    printk("\n");

}
void do_vm_burst_subtest(void)
{
    int i; 
    
    printk("[do_vm_burst_subtest]\n");
    
    for(i=0;i<=PMIC_VM_BURST_SLEEP_MASK;i++)
    {
        upmu_set_vm_burst_sleep(i);
    
        printk("[do_vm_burst_subtest] upmu_set_vm_bursth_sleep=%d\n", i);
    
        printk("[do_vm_burst_subtest] upmu_get_qi_vm_burst=%d\n",
            upmu_get_qi_vm_burst());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VM_BURST_ON_MASK;i++)
    {
        upmu_set_vm_burst_on(i);
    
        printk("[do_vm_burst_subtest] upmu_set_vm_bursth_on=%d\n", i);
    
        printk("[do_vm_burst_subtest] upmu_get_qi_vm_burst=%d\n",
            upmu_get_qi_vm_burst());                   
    }

    printk("\n");

}
void do_vio18_burst_subtest(void)
{
    int i; 
    
    printk("[do_vio18_burst_subtest]\n");
    
    for(i=0;i<=PMIC_VIO18_BURSTH_SLEEP_MASK;i++)
    {
        upmu_set_vio18_bursth_sleep(i);
        upmu_set_vio18_burstl_sleep(i);
    
        printk("[do_vio18_burst_subtest] upmu_set_vio18_bursth_sleep=%d, upmu_set_vio18_burstl_sleep=%d\n", i, i);
    
        printk("[do_vio18_burst_subtest] upmu_get_qi_vio18_burst=%d, upmu_get_qi_vio18_burst_n=%d\n",
            upmu_get_qi_vio18_bursth(), upmu_get_qi_vio18_burstl());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VIO18_BURSTH_ON_MASK;i++)
    {
        upmu_set_vio18_bursth_on(i);
        upmu_set_vio18_burstl_on(i);
    
        printk("[do_vio18_burst_subtest] upmu_set_vio18_bursth_on=%d, upmu_set_vio18_burstl_on=%d\n", i, i);
    
        printk("[do_vio18_burst_subtest] upmu_get_qi_vio18_burst=%d, upmu_get_qi_vio18_burst_n=%d\n",
            upmu_get_qi_vio18_bursth(), upmu_get_qi_vio18_burstl());                   
    }

    printk("\n");

}
void do_vpa_burst_subtest(void)
{
    int i; 

    printk("[do_vpa_burst_subtest]\n");

    for(i=0;i<=PMIC_VPA_BURSTH_SLEEP_MASK;i++)
    {
        upmu_set_vpa_bursth_sleep(i);
        upmu_set_vpa_burstl_sleep(i);

        printk("[do_vpa_burst_subtest] upmu_set_vpa_bursth_sleep=%d, upmu_set_vpa_burstl_sleep=%d\n", i, i);

        printk("[do_vpa_burst_subtest] upmu_get_qi_vpa_burst=%d, upmu_get_qi_vpa_burst_n=%d\n",
            upmu_get_qi_vpa_bursth(), upmu_get_qi_vpa_burstl());

    }

    printk("\n");

    for(i=0;i<=PMIC_VPA_BURSTH_ON_MASK;i++)
    {
        upmu_set_vpa_bursth_on(i);
        upmu_set_vpa_burstl_on(i);

        printk("[do_vpa_burst_subtest] upmu_set_vpa_bursth_on=%d, upmu_set_vpa_burstl_on=%d\n", i, i);

        printk("[do_vpa_burst_subtest] upmu_get_qi_vpa_burst=%d, upmu_get_qi_vpa_burst_n=%d\n",
            upmu_get_qi_vpa_bursth(), upmu_get_qi_vpa_burstl());                   
    }  

    printk("\n");

}

void do_vrf18_burst_subtest(void)
{
    int i; 
    
    printk("[do_vrf18_burst_subtest]\n");
    
    for(i=0;i<=PMIC_VRF18_BURSTH_SLEEP_MASK;i++)
    {
        upmu_set_vrf18_bursth_sleep(i);
        upmu_set_vrf18_burstl_sleep(i);
    
        mdelay(1);
        printk("[do_vrf18_burst_subtest] upmu_set_vrf18_bursth_sleep=%d, upmu_set_vrf18_burstl_sleep=%d\n", i, i);
        mdelay(1);
        printk("[do_vrf18_burst_subtest] upmu_get_qi_vrf18_burst=%d, upmu_get_qi_vrf18_burst_n=%d\n",
            upmu_get_qi_vrf18_bursth(), upmu_get_qi_vrf18_burstl());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VRF18_BURSTH_ON_MASK;i++)
    {
        upmu_set_vrf18_bursth_on(i);
        upmu_set_vrf18_burstl_on(i);
    
        mdelay(1);
        printk("[do_vrf18_burst_subtest] upmu_set_vrf18_bursth_on=%d, upmu_set_vrf18_burstl_on=%d\n", i, i);
        mdelay(1);
        printk("[do_vrf18_burst_subtest] upmu_get_qi_vrf18_burst=%d, upmu_get_qi_vrf18_burst_n=%d\n",
            upmu_get_qi_vrf18_bursth(), upmu_get_qi_vrf18_burstl());                   
    }

    printk("\n");

}
void do_vrf18_2_burst_subtest(void)
{
    int i; 
    
    printk("[do_vrf18_2_burst_subtest]\n");
    
    for(i=0;i<=PMIC_VRF18_2_BURSTH_SLEEP_MASK;i++)
    {
        upmu_set_vrf18_2_bursth_sleep(i);
        upmu_set_vrf18_2_burstl_sleep(i);
    
        mdelay(1);
        printk("[do_vrf18_2_burst_subtest] upmu_set_vrf18_2_bursth_sleep=%d, upmu_set_vrf18_2_burstl_sleep=%d\n", i, i);
        mdelay(1);
        printk("[do_vrf18_2_burst_subtest] upmu_get_qi_vrf18_2_burst=%d, upmu_get_qi_vrf18_2_burst_n=%d\n",
            upmu_get_qi_vrf18_2_bursth(), upmu_get_qi_vrf18_2_burstl());
    
    }

    printk("\n");
    
    for(i=0;i<=PMIC_VRF18_2_BURSTH_ON_MASK;i++)
    {
        upmu_set_vrf18_2_bursth_on(i);
        upmu_set_vrf18_2_burstl_on(i);
    
        mdelay(1);
        printk("[do_vrf18_2_burst_subtest] upmu_set_vrf18_2_bursth_on=%d, upmu_set_vrf18_2_burstl_on=%d\n", i, i);
        mdelay(1);
        printk("[do_vrf18_2_burst_subtest] upmu_get_qi_vrf18_2_burst=%d, upmu_get_qi_vrf18_2_burst_n=%d\n",
            upmu_get_qi_vrf18_2_bursth(), upmu_get_qi_vrf18_2_burstl());                   
    }

    printk("\n");

}

void do_burst_subtest(int index_val)
{
    switch(index_val){
        case 0:
            do_vproc_burst_subtest();
            break;
        case 1:
            do_vsram_burst_subtest();
            break;
        case 2:
            do_vcore_burst_subtest();
            break;
        case 3:
            do_vm_burst_subtest();
            break;
        case 4:
            do_vio18_burst_subtest();
            break;
        case 5:
            do_vpa_burst_subtest();
            break;
        case 6:
            do_vrf18_burst_subtest();
            break;
        case 7:
            do_vrf18_2_burst_subtest();
            break;
        default:
            printk("[do_burst_subtest] Invalid channel value(%d)\n", index_val);
            break;     
    }
}

void do_scrxxx_map_bursth(int index_val)
{
    int j;
    
    for(j=0;j<=7;j++)
    {
        switch(index_val){
            case 0:    
                printk("[upmu_set_vproc_burst_sel] %d\n", j);
                upmu_set_vproc_burst_sel(j);
                break;                
            case 1:    
                printk("[upmu_set_vsram_burst_sel] %d\n", j);
                upmu_set_vsram_burst_sel(j);
                break;
            case 2:    
                printk("[upmu_set_vcore_burst_sel] %d\n", j);
                upmu_set_vcore_burst_sel(j);
                break;
            case 3:    
                printk("[upmu_set_vm_burst_sel] %d\n", j);
                upmu_set_vm_burst_sel(j);
                break;
            case 4:    
                printk("[upmu_set_vio18_burst_sel] %d\n", j);
                upmu_set_vio18_burst_sel(j);
                break;                
            case 5:    
                printk("[upmu_set_vpa_burst_sel] %d\n", j);
                upmu_set_vpa_burst_sel(j);
                break;
            case 6:    
                printk("[upmu_set_vrf18_burst_sel] %d\n", j);
                upmu_set_vrf18_burst_sel(j);
                break;
            case 7:    
                printk("[upmu_set_vrf18_2_burst_sel] %d\n", j);
                upmu_set_vrf18_2_burst_sel(j);
                break;
            default:
                printk("[do_scrxxx_map] Invalid index_val value(%d)\n", index_val);
                break;    
        }

        switch(j){
            case 0:
                printk("[upmu_set_rg_srcvolt_en(0)]\n");
                upmu_set_rg_srcvolt_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1)]\n");
                upmu_set_rg_srcvolt_en(1);
                do_burst_subtest(index_val);
                break;
                
            case 1:
                printk("[upmu_set_rg_srclkperi_en(0)]\n");
                upmu_set_rg_srclkperi_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(1)]\n");
                upmu_set_rg_srclkperi_en(1);
                do_burst_subtest(index_val);
                break;
                
            case 2:
                printk("[upmu_set_rg_srclkmd2_en(0)]\n");
                upmu_set_rg_srclkmd2_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srclkmd2_en(1)]\n");
                upmu_set_rg_srclkmd2_en(1);
                do_burst_subtest(index_val);                           
                break;
                
            case 3:
                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1);
                do_burst_subtest(index_val);
                break;
                
            case 4:
                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkmd2_en(1);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkmd2_en(1);
                do_burst_subtest(index_val);
                break;
                
            case 5:
                printk("[upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
                do_burst_subtest(index_val);
                break;
                
            case 6:
                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(0); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(0); upmu_set_rg_srclkmd2_en(1);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(0);
                do_burst_subtest(index_val);

                printk("[upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);]\n");
                upmu_set_rg_srcvolt_en(1); upmu_set_rg_srclkperi_en(1); upmu_set_rg_srclkmd2_en(1);
                do_burst_subtest(index_val);
                break;
                
            case 7:
                printk("[0]\n");
                do_burst_subtest(index_val);
                break;
                
            default:
                printk("[do_scrxxx_map] Invalid channel value(%d)\n", j);
                break;    
        }                    
    }
}

void do_vproc_burst_test(int index_val)
{
    int i,j;
    
    for(i=0;i<=1;i++)
    {
        printk("[do_vproc_burst_test] %d\n", i);
        upmu_set_vproc_burst_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VPROC_BURST_MASK;j++)
                {
                    upmu_set_vproc_burst(j);

                    if( (upmu_get_qi_vproc_burst()!=j) )
                    {
                        printk("[do_vproc_burst_test] fail at upmu_get_qi_vproc_burst=%d\n",
                            upmu_get_qi_vproc_burst());
                    }

                    printk("[do_vproc_burst_test] upmu_set_vproc_burst=%d, upmu_get_qi_vproc_burst=%d\n",
                            j, upmu_get_qi_vproc_burst());
                }
                break;    

            case 1:
                do_scrxxx_map_bursth(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    }    
}
void do_vsram_burst_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vsram_burst_test] %d\n", i);
        upmu_set_vsram_burst_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VSRAM_BURST_MASK;j++)
                {
                    upmu_set_vsram_burst(j);

                    if( (upmu_get_qi_vsram_burst()!=j) )
                    {
                        printk("[do_vsram_burst_test] fail at upmu_get_qi_vsram_burst=%d\n",
                            upmu_get_qi_vsram_burst());
                    }

                    printk("[do_vsram_burst_test] upmu_set_vsram_burst=%d, upmu_get_qi_vsram_burst=%d\n",
                            j, upmu_get_qi_vsram_burst());
                }
                break;    

            case 1:
                do_scrxxx_map_bursth(index_val);
                break;

            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vcore_burst_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vcore_burst_test] %d\n", i);
        upmu_set_vcore_burst_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VCORE_BURST_MASK;j++)
                {
                    upmu_set_vcore_burst(j);
    
                    if( (upmu_get_qi_vcore_burst()!=j) )
                    {
                        printk("[do_vcore_burst_test] fail at upmu_get_qi_vcore_burst=%d\n",
                            upmu_get_qi_vcore_burst());
                    }

                    printk("[do_vcore_burst_test] upmu_set_vcore_burst=%d, upmu_get_qi_vcore_burst=%d\n",
                            j, upmu_get_qi_vcore_burst());
                }
                break;    
    
            case 1:
                do_scrxxx_map_bursth(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vm_burst_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vm_burst_test] %d\n", i);
        upmu_set_vm_burst_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VM_BURST_MASK;j++)
                {
                    upmu_set_vm_burst(j);
    
                    if( (upmu_get_qi_vm_burst()!=j) )
                    {
                        printk("[do_vm_burst_test] fail at upmu_get_qi_vm_burst=%d\n",
                            upmu_get_qi_vm_burst());
                    }

                    printk("[do_vm_burst_test] upmu_set_vm_burst=%d, upmu_get_qi_vm_burst=%d\n",
                            j, upmu_get_qi_vm_burst());
                }
                break;    
    
            case 1:
                do_scrxxx_map_bursth(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vio18_burst_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vio18_burst_test] %d\n", i);
        upmu_set_vio18_burst_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VIO18_BURSTH_MASK;j++)
                {
                    upmu_set_vio18_bursth(j);
                    upmu_set_vio18_burstl(j);
    
                    if( (upmu_get_qi_vio18_bursth()!=j) || (upmu_get_qi_vio18_burstl()!=j) )
                    {
                        printk("[do_vio18_burst_test] fail at upmu_get_qi_vio18_burst=%d, upmu_get_qi_vio18_burst_n=%d\n",
                            upmu_get_qi_vio18_bursth(), upmu_get_qi_vio18_burstl());
                    }

                    printk("[do_vio18_burst_test] upmu_set_vio18_bursth=%d, upmu_set_vio18_burstl=%d, upmu_get_qi_vio18_bursth=%d, upmu_get_qi_vio18_burstl=%d\n",
                            j, j, upmu_get_qi_vio18_bursth(), upmu_get_qi_vio18_burstl());
                }
                break;    
    
            case 1:
                do_scrxxx_map_bursth(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vpa_burst_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vpa_burst_test] %d\n", i);
        upmu_set_vpa_burst_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VPA_BURSTH_MASK;j++)
                {
                    upmu_set_vpa_bursth(j);
                    upmu_set_vpa_burstl(j);
    
                    if( (upmu_get_qi_vpa_bursth()!=j) || (upmu_get_qi_vpa_burstl()!=j) )
                    {
                        printk("[do_vpa_burst_test] fail at upmu_get_qi_vpa_burst=%d, upmu_get_qi_vpa_burst_n=%d\n",
                            upmu_get_qi_vpa_bursth(), upmu_get_qi_vpa_burstl());
                    }

                    printk("[do_vpa_burst_test] upmu_set_vpa_bursth=%d, upmu_set_vpa_burstl=%d, upmu_get_qi_vpa_bursth=%d, upmu_get_qi_vpa_burstl=%d\n",
                            j, j, upmu_get_qi_vpa_bursth(), upmu_get_qi_vpa_burstl());
                }
                break;    
    
            case 1:
                do_scrxxx_map_bursth(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
    
}
void do_vrf18_burst_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vrf18_burst_test] %d\n", i);
        upmu_set_vrf18_burst_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VRF18_BURSTH_MASK;j++)
                {
                    upmu_set_vrf18_bursth(j);
                    upmu_set_vrf18_burstl(j);
    
                    if( (upmu_get_qi_vrf18_bursth()!=j) || (upmu_get_qi_vrf18_burstl()!=j) )
                    {
                        printk("[do_vrf18_burst_test] fail at upmu_get_qi_vrf18_burst=%d, upmu_get_qi_vrf18_burst_n=%d\n",
                            upmu_get_qi_vrf18_bursth(), upmu_get_qi_vrf18_burstl());
                    }

                    printk("[do_vrf18_burst_test] upmu_set_vrf18_bursth=%d, upmu_set_vrf18_burstl=%d, upmu_get_qi_vrf18_bursth=%d, upmu_get_qi_vrf18_burstl=%d\n",
                            j, j, upmu_get_qi_vrf18_bursth(), upmu_get_qi_vrf18_burstl());
                }
                break;    
    
            case 1:
                do_scrxxx_map_bursth(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
void do_vrf18_2_burst_test(int index_val)
{
    int i,j;
        
    for(i=0;i<=1;i++)
    {
        printk("[do_vrf18_2_burst_test] %d\n", i);
        upmu_set_vrf18_2_burst_ctrl(i);        
        
        switch(i){
            case 0:
                for(j=0;j<=PMIC_VRF18_2_BURSTH_MASK;j++)
                {
                    upmu_set_vrf18_2_bursth(j);
                    upmu_set_vrf18_2_burstl(j);
    
                    if( (upmu_get_qi_vrf18_2_bursth()!=j) || (upmu_get_qi_vrf18_2_burstl()!=j) )
                    {
                        printk("[do_vrf18_2_burst_test] fail at upmu_get_qi_vrf18_2_burst=%d, upmu_get_qi_vrf18_2_burst_n=%d\n",
                            upmu_get_qi_vrf18_2_bursth(), upmu_get_qi_vrf18_2_burstl());
                    }

                    printk("[do_vrf18_2_burst_test] upmu_set_vrf18_2_bursth=%d, upmu_set_vrf18_2_burstl=%d, upmu_get_qi_vrf18_2_bursth=%d, upmu_get_qi_vrf18_2_burstl=%d\n",
                            j, j, upmu_get_qi_vrf18_2_bursth(), upmu_get_qi_vrf18_2_burstl());
                }
                break;    
    
            case 1:
                do_scrxxx_map_bursth(index_val);
                break;
    
            default:
                printk("At %d, Invalid channel value(%d)\n", index_val, i);
                break;    
        }            
    } 
}
#endif

void pmic_UVVP_PMIC_BUCK_ON_OFF(int index_val)
{   
    printk("[pmic_UVVP_PMIC_BUCK_ON_OFF] start....\n");

    upmu_set_rg_srcvolt_hw_auto_en(0); // 0:SW control
    upmu_set_rg_srclkperi_hw_auto_en(0); // 0:SW control 
    upmu_set_rg_srclkmd2_hw_auto_en(0); // 0:SW control 

    switch(index_val){
      case 0:
        do_vproc_en_test(index_val);
        break;

      case 1:
        do_vsram_en_test(index_val);
        break;

      case 2:
        do_vcore_en_test(index_val);
        break;

      case 3:
        do_vm_en_test(index_val);
        break;

      case 4:
        do_vio18_en_test(index_val);
        break;

      case 5:
        do_vpa_en_test(index_val);
        break;

      case 6:
        do_vrf18_en_test(index_val);
        break;

      case 7:
        do_vrf18_2_en_test(index_val);                      
        break;
       
	  default:
        printk("[pmic_UVVP_PMIC_BUCK_ON_OFF] Invalid channel value(%d)\n", index_val);
        break;
        
    }
    
    printk("[pmic_UVVP_PMIC_BUCK_ON_OFF] end....\n");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_BUCK_VOSEL(int index_val)
{
    //int i=0; 
    
    printk("[pmic_UVVP_PMIC_BUCK_VOSEL] start....\n");

    upmu_set_rg_srcvolt_hw_auto_en(0); // 0:SW control
    upmu_set_rg_srclkperi_hw_auto_en(0); // 0:SW control 
    upmu_set_rg_srclkmd2_hw_auto_en(0); // 0:SW control 

    upmu_set_vproc_en_ctrl(0);
    upmu_set_vsram_en_ctrl(0);
    upmu_set_vcore_en_ctrl(0);
    upmu_set_vm_en_ctrl(0);
    upmu_set_vio18_en_ctrl(0);
    upmu_set_vpa_en_ctrl(0);
    upmu_set_vrf18_en_ctrl(0);
    upmu_set_vrf18_2_en_ctrl(0);

    switch(index_val){
      case 0:
        upmu_set_vproc_en(1);
        g_buck_num=0;
        g_log_buck_vosel=1;
        do_vproc_vosel_test(index_val);
        g_buck_num=100;
        g_log_buck_vosel=0;
        break;

      case 1:
        upmu_set_vsram_en(1);
        g_buck_num=1;
        g_log_buck_vosel=1;
        do_vsram_vosel_test(index_val);
        g_buck_num=100;
        g_log_buck_vosel=0;
        break;

      case 2:
        upmu_set_vcore_en(1);
        g_buck_num=2;
        g_log_buck_vosel=1;
        do_vcore_vosel_test(index_val);
        g_buck_num=100;
        g_log_buck_vosel=0;
        break;

      case 3:
        upmu_set_vm_en(1);
        g_buck_num=3;
        g_log_buck_vosel=1;
        do_vm_vosel_test(index_val);
        g_buck_num=100;
        g_log_buck_vosel=0;
        break;

      case 4:
        upmu_set_vio18_en(1);
        g_buck_num=4;
        g_log_buck_vosel=1;
        do_vio18_vosel_test(index_val);
        g_buck_num=100;
        g_log_buck_vosel=0;
        break;

      case 5:
        upmu_set_vpa_en(1);
        g_buck_num=5;
        g_log_buck_vosel=1;
        do_vpa_vosel_test(index_val);
        g_buck_num=100;
        g_log_buck_vosel=0;
        break;

      case 6:        
        upmu_set_vrf18_en(1);
        g_buck_num=6;
        g_log_buck_vosel=1;
        do_vrf18_vosel_test(index_val);        
        g_buck_num=100;
        g_log_buck_vosel=0;
        break;

      case 7:        
        upmu_set_vrf18_2_en(1);
        g_buck_num=7;
        g_log_buck_vosel=1;
        do_vrf18_2_vosel_test(index_val);        
        g_buck_num=100;
        g_log_buck_vosel=0;
        break;
       
	  default:
        printk("[pmic_UVVP_PMIC_BUCK_VOSEL] Invalid channel value(%d)\n", index_val);
        break;
        
    }
    
    printk("[pmic_UVVP_PMIC_BUCK_VOSEL] end....\n");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_BUCK_DLC(int index_val)
{
    //int i=0; 
    
    printk("[pmic_UVVP_PMIC_DLC_VOSEL] start....\n");

    upmu_set_rg_srcvolt_hw_auto_en(0); // 0:SW control
    upmu_set_rg_srclkperi_hw_auto_en(0); // 0:SW control 
    upmu_set_rg_srclkmd2_hw_auto_en(0); // 0:SW control 

    upmu_set_vproc_en_ctrl(0);
    upmu_set_vsram_en_ctrl(0);
    upmu_set_vcore_en_ctrl(0);
    upmu_set_vm_en_ctrl(0);
    upmu_set_vio18_en_ctrl(0);
    upmu_set_vpa_en_ctrl(0);
    upmu_set_vrf18_en_ctrl(0);
    upmu_set_vrf18_2_en_ctrl(0);

    switch(index_val){
      case 0:
        upmu_set_vproc_en(1);
        do_vproc_dlc_test(index_val);
        break;

      case 1:
        upmu_set_vsram_en(1);
        do_vsram_dlc_test(index_val);
        break;

      case 2:
        upmu_set_vcore_en(1);
        do_vcore_dlc_test(index_val);
        break;

      case 3:
        upmu_set_vm_en(1);
        do_vm_dlc_test(index_val);
        break;

      case 4:
        upmu_set_vio18_en(1);
        do_vio18_dlc_test(index_val);
        break;

      case 5:
        upmu_set_vpa_en(1);
        do_vpa_dlc_test(index_val);
        break;

      case 6:        
        upmu_set_vrf18_en(1);
        do_vrf18_dlc_test(index_val);        
        break;

      case 7:        
        upmu_set_vrf18_2_en(1);
        do_vrf18_2_dlc_test(index_val);        
        break;
       
	  default:
        printk("[pmic_UVVP_PMIC_BUCK_DLC] Invalid channel value(%d)\n", index_val);
        break;
        
    }
    
    printk("[pmic_UVVP_PMIC_BUCK_DLC] end....\n");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_BUCK_BURST(int index_val)
{
    //int i=0; 
    
    printk("[pmic_UVVP_PMIC_BUCK_BURST] start....\n");

    upmu_set_rg_srcvolt_hw_auto_en(0); // 0:SW control
    upmu_set_rg_srclkperi_hw_auto_en(0); // 0:SW control 
    upmu_set_rg_srclkmd2_hw_auto_en(0); // 0:SW control 

    upmu_set_vproc_en_ctrl(0);
    upmu_set_vsram_en_ctrl(0);
    upmu_set_vcore_en_ctrl(0);
    upmu_set_vm_en_ctrl(0);
    upmu_set_vio18_en_ctrl(0);
    upmu_set_vpa_en_ctrl(0);
    upmu_set_vrf18_en_ctrl(0);
    upmu_set_vrf18_2_en_ctrl(0);

    switch(index_val){
      case 0:
        upmu_set_vproc_en(1);
        do_vproc_burst_test(index_val);
        break;

      case 1:
        upmu_set_vsram_en(1);
        do_vsram_burst_test(index_val);
        break;

      case 2:
        upmu_set_vcore_en(1);
        do_vcore_burst_test(index_val);
        break;

      case 3:
        upmu_set_vm_en(1);
        do_vm_burst_test(index_val);
        break;

      case 4:
        upmu_set_vio18_en(1);
        do_vio18_burst_test(index_val);
        break;

      case 5:
        upmu_set_vpa_en(1);
        do_vpa_burst_test(index_val);
        break;

      case 6:        
        upmu_set_vrf18_en(1);
        do_vrf18_burst_test(index_val);        
        break;

      case 7:        
        upmu_set_vrf18_2_en(1);
        do_vrf18_2_burst_test(index_val);        
        break;
       
	  default:
        printk("[pmic_UVVP_PMIC_BUCK_BURST] Invalid channel value(%d)\n", index_val);
        break;
        
    }
    
    printk("[pmic_UVVP_PMIC_BUCK_BURST] end....\n");
}


///////////////////////////////////////////////////////////////////////////////////
//
//  LDO TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_LDO_ON_OFF(int index_val)
{
    printk("[pmic_UVVP_PMIC_LDO_ON_OFF] start....\n");

    switch(index_val){
      case 0:
        hwPowerOn(MT65XX_POWER_LDO_VIO28,    VOL_DEFAULT, "ldo_test");    
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIO28,     "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIO28,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();        
        hwPowerDown(MT65XX_POWER_LDO_VIO28,     "ldo_test");
        read_auxadc_value();
        break;

      case 1:
        hwPowerOn(MT65XX_POWER_LDO_VUSB,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VUSB,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VUSB,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VUSB,      "ldo_test");
        read_auxadc_value();
        break;

      case 2:
        hwPowerOn(MT65XX_POWER_LDO_VMC1,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMC1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VMC1,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMC1,      "ldo_test");
        read_auxadc_value();
        break;

      case 3:
        hwPowerOn(MT65XX_POWER_LDO_VMCH1,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMCH1,     "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VMCH1,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMCH1,     "ldo_test");
        read_auxadc_value();
        break;

      case 4:
        hwPowerOn(MT65XX_POWER_LDO_VEMC_3V3, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_3V3,  "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_3V3, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_3V3,  "ldo_test");
        read_auxadc_value();
        break;

      case 5:
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,  "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,  "ldo_test");
        read_auxadc_value();
        break;

      case 6:
        hwPowerOn(MT65XX_POWER_LDO_VGP1,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP1,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        break;

      case 7:  
        hwPowerOn(MT65XX_POWER_LDO_VGP2,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP2,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        break;

      case 8:  
        hwPowerOn(MT65XX_POWER_LDO_VGP3,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP3,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();        
        break;

      case 9:  
        hwPowerOn(MT65XX_POWER_LDO_VGP4,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP4,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        break;

      case 10:  
        hwPowerOn(MT65XX_POWER_LDO_VGP5,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP5,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        break;

      case 11:  
        hwPowerOn(MT65XX_POWER_LDO_VGP6,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP6,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        break;

      case 12:  
        hwPowerOn(MT65XX_POWER_LDO_VSIM1,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,     "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM1,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,     "ldo_test");
        read_auxadc_value();
        break;

      case 13:
        hwPowerOn(MT65XX_POWER_LDO_VSIM2,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,     "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM2,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,     "ldo_test");
        read_auxadc_value();
        break;

      case 14:  
        hwPowerOn(MT65XX_POWER_LDO_VIBR,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIBR,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        break;

      case 15:  
        hwPowerOn(MT65XX_POWER_LDO_VRTC,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRTC,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VRTC,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRTC,      "ldo_test");
        read_auxadc_value();
        break;

      case 16:  
        hwPowerOn(MT65XX_POWER_LDO_VAST,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VAST,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VAST,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VAST,      "ldo_test");
        read_auxadc_value();
        break;

      case 17:  
        hwPowerOn(MT65XX_POWER_LDO_VRF28,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28,     "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VRF28,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28,     "ldo_test");
        read_auxadc_value();        
        break;

      case 18:  
        hwPowerOn(MT65XX_POWER_LDO_VRF28_2,  VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28_2,   "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VRF28_2,  VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28_2,   "ldo_test");
        read_auxadc_value();
        break;

      case 19:  
        hwPowerOn(MT65XX_POWER_LDO_VTCXO,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VTCXO,     "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VTCXO,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VTCXO,     "ldo_test");
        read_auxadc_value();
        break;

      case 20:  
        hwPowerOn(MT65XX_POWER_LDO_VTCXO_2,  VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VTCXO_2,   "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VTCXO_2,  VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VTCXO_2,   "ldo_test");
        read_auxadc_value();        
        break;

      case 21:  
        hwPowerOn(MT65XX_POWER_LDO_VA,       VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VA,        "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VA,       VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VA,        "ldo_test");
        read_auxadc_value();
        break;

      case 22:  
        hwPowerOn(MT65XX_POWER_LDO_VA28,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VA28,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VA28,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VA28,      "ldo_test");
        read_auxadc_value();
        break;

      case 23:  
        hwPowerOn(MT65XX_POWER_LDO_VCAMA,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VCAMA,     "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VCAMA,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VCAMA,     "ldo_test");
        read_auxadc_value();
        break;
        
	  default:
        printk("[pmic_UVVP_PMIC_LDO_ON_OFF] Invalid channel value(%d)\n", index_val);
        break;
        
    }

    printk("[pmic_UVVP_PMIC_LDO_ON_OFF] end....\n");
    
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_LDO_VOSEL(int index_val)
{
    printk("[pmic_UVVP_PMIC_LDO_VOSEL] start....\n");

    switch(index_val){
      case 0:
        hwPowerOn(MT65XX_POWER_LDO_VIO28,    VOL_DEFAULT, "ldo_test");    
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIO28,      "ldo_test");
        read_auxadc_value();
        break;

      case 1:
        hwPowerOn(MT65XX_POWER_LDO_VUSB,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VUSB,      "ldo_test");
        read_auxadc_value();
        break;

      case 2:
        hwPowerOn(MT65XX_POWER_LDO_VMC1,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMC1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VMC1,     VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMC1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VMC1,     VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMC1,      "ldo_test");
        read_auxadc_value();
        break;

      case 3:
        hwPowerOn(MT65XX_POWER_LDO_VMCH1,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMCH1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VMCH1,    VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMCH1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VMCH1,    VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VMCH1,      "ldo_test");
        read_auxadc_value();
        break;

      case 4:
        hwPowerOn(MT65XX_POWER_LDO_VEMC_3V3, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_3V3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_3V3, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_3V3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_3V3, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_3V3,      "ldo_test");
        read_auxadc_value();
        break;

      case 5:
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,      "ldo_test");
        read_auxadc_value();
        break;

      case 6:       
        hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP1, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        break;

      case 7:  
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        break;

      case 8:  
        hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP3, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();
        break;

      case 9:  
        hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP4, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        break;

      case 10:  
        hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        break;

      case 11:  
        hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VGP6, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        break;

      case 12:  
        hwPowerOn(MT65XX_POWER_LDO_VSIM1, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM1, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM1, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM1, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM1, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM1, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM1, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM1, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM1, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,      "ldo_test");
        read_auxadc_value();
        break;

      case 13:
        hwPowerOn(MT65XX_POWER_LDO_VSIM2, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM2, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM2, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM2, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM2, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM2, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM2, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM2, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VSIM2, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,      "ldo_test");
        read_auxadc_value();
        break;

      case 14:  
        hwPowerOn(MT65XX_POWER_LDO_VIBR, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIBR, VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIBR, VOL_1300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIBR, VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIBR, VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIBR, VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIBR, VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIBR, VOL_3000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VIBR, VOL_3300, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        break;

      case 15:  
        hwPowerOn(MT65XX_POWER_LDO_VRTC,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRTC,      "ldo_test");
        read_auxadc_value();
        break;

      case 16:  
        hwPowerOn(MT65XX_POWER_LDO_VAST,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VAST,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VAST,     VOL_1200, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VAST,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VAST,     VOL_1100, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VAST,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VAST,     VOL_1000, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VAST,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VAST,     VOL_0900, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VAST,      "ldo_test");
        read_auxadc_value();
        break;

      case 17:  
        hwPowerOn(MT65XX_POWER_LDO_VRF28,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VRF28,    VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VRF28,    VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28,      "ldo_test");
        read_auxadc_value();
        break;

      case 18:  
        hwPowerOn(MT65XX_POWER_LDO_VRF28_2,  VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28_2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VRF28_2,  VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28_2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VRF28_2,  VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VRF28_2,      "ldo_test");
        read_auxadc_value();
        break;

      case 19:  
        hwPowerOn(MT65XX_POWER_LDO_VTCXO,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VTCXO,      "ldo_test");
        read_auxadc_value();
        break;

      case 20:  
        hwPowerOn(MT65XX_POWER_LDO_VTCXO_2,  VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VTCXO_2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VTCXO_2,  VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VTCXO_2,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VTCXO_2,  VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VTCXO_2,      "ldo_test");
        read_auxadc_value();
        break;

      case 21:  
        hwPowerOn(MT65XX_POWER_LDO_VA,       VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VA,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VA,  VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VA,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VA,  VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VA,      "ldo_test");
        read_auxadc_value();
        break;

      case 22:  
        hwPowerOn(MT65XX_POWER_LDO_VA28,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VA28,      "ldo_test");
        read_auxadc_value();
        break;

      case 23:  
        hwPowerOn(MT65XX_POWER_LDO_VCAMA,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VCAMA,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VCAMA,    VOL_1500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VCAMA,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VCAMA,    VOL_1800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VCAMA,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VCAMA,    VOL_2500, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VCAMA,      "ldo_test");
        read_auxadc_value();
        hwPowerOn(MT65XX_POWER_LDO_VCAMA,    VOL_2800, "ldo_test");
        read_auxadc_value();
        hwPowerDown(MT65XX_POWER_LDO_VCAMA,      "ldo_test");
        read_auxadc_value();
        break;
        
	  default:
        printk("[pmic_UVVP_PMIC_LDO_VOSEL] Invalid channel value(%d)\n", index_val);
        break;
        
    }
    
    printk("[pmic_UVVP_PMIC_LDO_VOSEL] end....\n");
}
///////////////////////////////////////////////////////////////////////////////////
void pmic_UVVP_PMIC_LDO_CAL(int index_val)
{
    int i=0;
    
    printk("[pmic_UVVP_PMIC_LDO_VOSEL] start....\n");

    switch(index_val){
      case 0:
        hwPowerOn(MT65XX_POWER_LDO_VIO28,    VOL_DEFAULT, "ldo_test");    
        read_auxadc_value();
        
        for(i=0;i<=PMIC_RG_VIO28_CAL_MASK;i++)
        {
            upmu_set_rg_vio28_cal(i);
            printk("[MT65XX_POWER_LDO_VIO28] cal=%d, ",i);
            read_auxadc_value();
        }

        hwPowerDown(MT65XX_POWER_LDO_VIO28,      "ldo_test");
        read_auxadc_value();
        break;

      case 1:
        hwPowerOn(MT65XX_POWER_LDO_VUSB,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        
        for(i=0;i<=PMIC_RG_VUSB_CAL_MASK;i++)
        {
            upmu_set_rg_vusb_cal(i);
            printk("[MT65XX_POWER_LDO_VUSB] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VUSB,      "ldo_test");
        read_auxadc_value();        
        break;

      case 2:
        hwPowerOn(MT65XX_POWER_LDO_VMC1,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VMC1_CAL_MASK;i++)
        {
            upmu_set_rg_vmc1_cal(i);
            printk("[MT65XX_POWER_LDO_VMC1] cal=%d, ",i);
            read_auxadc_value();
        }    
        
        hwPowerDown(MT65XX_POWER_LDO_VMC1,      "ldo_test");
        read_auxadc_value();        
        break;

      case 3:
        hwPowerOn(MT65XX_POWER_LDO_VMCH1,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VMCH1_CAL_MASK;i++)
        {
            upmu_set_rg_vmch1_cal(i);
            printk("[MT65XX_POWER_LDO_VMCH1] cal=%d, ",i);
            read_auxadc_value();
        }    
        
        hwPowerDown(MT65XX_POWER_LDO_VMCH1,     "ldo_test");
        read_auxadc_value();
        break;

      case 4:
        hwPowerOn(MT65XX_POWER_LDO_VEMC_3V3, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VEMC_3V3_CAL_MASK;i++)
        {
            upmu_set_rg_vemc_3v3_cal(i);
            printk("[MT65XX_POWER_LDO_VEMC_3V3] cal=%d, ",i);
            read_auxadc_value();
        }    
        
        hwPowerDown(MT65XX_POWER_LDO_VEMC_3V3,  "ldo_test");
        read_auxadc_value();       
        break;

      case 5:
        hwPowerOn(MT65XX_POWER_LDO_VEMC_1V8, VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VEMC_1V8_CAL_MASK;i++)
        {
            upmu_set_rg_vemc_1v8_cal(i);
            printk("[MT65XX_POWER_LDO_VEMC_1V8] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VEMC_1V8,  "ldo_test");
        read_auxadc_value();
        break;

      case 6:
        hwPowerOn(MT65XX_POWER_LDO_VGP1,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VGP1_CAL_MASK;i++)
        {
            upmu_set_rg_vgp1_cal(i);
            printk("[MT65XX_POWER_LDO_VGP1] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VGP1,      "ldo_test");
        read_auxadc_value();
        break;

      case 7:  
        hwPowerOn(MT65XX_POWER_LDO_VGP2,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VGP2_CAL_MASK;i++)
        {
            upmu_set_rg_vgp2_cal(i);
            printk("[MT65XX_POWER_LDO_VGP2] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VGP2,      "ldo_test");
        read_auxadc_value();
        break;

      case 8:  
        hwPowerOn(MT65XX_POWER_LDO_VGP3,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VGP3_CAL_MASK;i++)
        {
            upmu_set_rg_vgp3_cal(i);
            printk("[MT65XX_POWER_LDO_VGP3] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VGP3,      "ldo_test");
        read_auxadc_value();       
        break;

      case 9:  
        hwPowerOn(MT65XX_POWER_LDO_VGP4,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VGP4_CAL_MASK;i++)
        {
            upmu_set_rg_vgp4_cal(i);
            printk("[MT65XX_POWER_LDO_VGP4] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VGP4,      "ldo_test");
        read_auxadc_value();
        break;

      case 10:  
        hwPowerOn(MT65XX_POWER_LDO_VGP5,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VGP5_CAL_MASK;i++)
        {
            upmu_set_rg_vgp5_cal(i);
            printk("[MT65XX_POWER_LDO_VGP5] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VGP5,      "ldo_test");
        read_auxadc_value();
        break;

      case 11:  
        hwPowerOn(MT65XX_POWER_LDO_VGP6,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VGP6_CAL_MASK;i++)
        {
            upmu_set_rg_vgp6_cal(i);
            printk("[MT65XX_POWER_LDO_VGP6] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VGP6,      "ldo_test");
        read_auxadc_value();
        break;

      case 12:  
        hwPowerOn(MT65XX_POWER_LDO_VSIM1,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VSIM1_CAL_MASK;i++)
        {
            upmu_set_rg_vsim1_cal(i);
            printk("[MT65XX_POWER_LDO_VSIM1] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VSIM1,     "ldo_test");
        read_auxadc_value();
        break;

      case 13:
        hwPowerOn(MT65XX_POWER_LDO_VSIM2,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VSIM2_CAL_MASK;i++)
        {
            upmu_set_rg_vsim2_cal(i);
            printk("[MT65XX_POWER_LDO_VSIM2] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VSIM2,     "ldo_test");
        read_auxadc_value();
        break;

      case 14:  
        hwPowerOn(MT65XX_POWER_LDO_VIBR,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VIBR_CAL_MASK;i++)
        {
            upmu_set_rg_vibr_cal(i);
            printk("[MT65XX_POWER_LDO_VIBR] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VIBR,      "ldo_test");
        read_auxadc_value();
        break;

      case 15:  
        hwPowerOn(MT65XX_POWER_LDO_VRTC,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();
        #if 0
        for(i=0;i<=PMIC_RG_VRTC_CAL_MASK;i++)
        {
            upmu_set_rg_vrtc_cal(i);
            printk("[MT65XX_POWER_LDO_VRTC] cal=%d, ",i);
            read_auxadc_value();
        }
        #endif        
        hwPowerDown(MT65XX_POWER_LDO_VRTC,      "ldo_test");
        read_auxadc_value();
        break;

      case 16:  
        hwPowerOn(MT65XX_POWER_LDO_VAST,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VAST_CAL_MASK;i++)
        {
            upmu_set_rg_vast_cal(i);
            printk("[MT65XX_POWER_LDO_VAST] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VAST,      "ldo_test");
        read_auxadc_value();
        break;

      case 17:  
        hwPowerOn(MT65XX_POWER_LDO_VRF28,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VRF28_CAL_MASK;i++)
        {
            upmu_set_rg_vrf28_cal(i);
            printk("[MT65XX_POWER_LDO_VRF28] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VRF28,     "ldo_test");
        read_auxadc_value();       
        break;

      case 18:  
        hwPowerOn(MT65XX_POWER_LDO_VRF28_2,  VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VRF28_2_CAL_MASK;i++)
        {
            upmu_set_rg_vrf28_2_cal(i);
            printk("[MT65XX_POWER_LDO_VRF28_2] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VRF28_2,   "ldo_test");
        read_auxadc_value();
        break;

      case 19:  
        hwPowerOn(MT65XX_POWER_LDO_VTCXO,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VTCXO_CAL_MASK;i++)
        {
            upmu_set_rg_vtcxo_cal(i);
            printk("[MT65XX_POWER_LDO_VTCXO] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VTCXO,     "ldo_test");
        read_auxadc_value();
        break;

      case 20:  
        hwPowerOn(MT65XX_POWER_LDO_VTCXO_2,  VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VTCXO_2_CAL_MASK;i++)
        {
            upmu_set_rg_vtcxo_2_cal(i);
            printk("[MT65XX_POWER_LDO_VTCXO_2] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VTCXO_2,   "ldo_test");
        read_auxadc_value();       
        break;

      case 21:  
        hwPowerOn(MT65XX_POWER_LDO_VA,       VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VA_CAL_MASK;i++)
        {
            upmu_set_rg_va_cal(i);
            printk("[MT65XX_POWER_LDO_VA] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VA,        "ldo_test");
        read_auxadc_value();
        break;

      case 22:  
        hwPowerOn(MT65XX_POWER_LDO_VA28,     VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VA28_CAL_MASK;i++)
        {
            upmu_set_rg_va28_cal(i);
            printk("[MT65XX_POWER_LDO_VA28] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VA28,      "ldo_test");
        read_auxadc_value();
        break;

      case 23:  
        hwPowerOn(MT65XX_POWER_LDO_VCAMA,    VOL_DEFAULT, "ldo_test");
        read_auxadc_value();

        for(i=0;i<=PMIC_RG_VCAMA_CAL_MASK;i++)
        {
            upmu_set_rg_vcama_cal(i);
            printk("[MT65XX_POWER_LDO_VCAMA] cal=%d, ",i);
            read_auxadc_value();
        }
        
        hwPowerDown(MT65XX_POWER_LDO_VCAMA,     "ldo_test");
        read_auxadc_value();
        break;
        
	  default:
        printk("[pmic_UVVP_PMIC_LDO_VOSEL] Invalid channel value(%d)\n", index_val);
        break;
        
    }
    
    printk("[pmic_UVVP_PMIC_LDO_VOSEL] end....\n");
}


#if 0
///////////////////////////////////////////////////////////////////////////////////
//
//  INTERRUPT TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
#if 1
kal_uint32 LBAT_VOLT_MAX_7_0=0x7b; // wait SA provide, 4.2V
kal_uint32 LBAT_VOLT_MAX_9_8=0x03; // wait SA provide, 4.2V
kal_uint32 LBAT_VOLT_MIN_7_0=0xd1; // wait SA provide, 3.4V
kal_uint32 LBAT_VOLT_MIN_9_8=0x02; // wait SA provide, 3.4V
#endif
#if 0
kal_uint32 LBAT_VOLT_MAX_7_0=0xbb; // > 4.2
kal_uint32 LBAT_VOLT_MAX_9_8=0x03; // > 4.2
kal_uint32 LBAT_VOLT_MIN_7_0=0x7b; // wait SA provide, 4.2V
kal_uint32 LBAT_VOLT_MIN_9_8=0x03; // wait SA provide, 4.2V
#endif
#if 0
kal_uint32 LBAT_VOLT_MAX_7_0=0xd1; // wait SA provide, 3.4V
kal_uint32 LBAT_VOLT_MAX_9_8=0x02; // wait SA provide, 3.4V
kal_uint32 LBAT_VOLT_MIN_7_0=0x31; // < 3.4V
kal_uint32 LBAT_VOLT_MIN_9_8=0x02; // < 3.4V
#endif
kal_uint32 LBAT_DET_PRD_19_16=0xF; 
kal_uint32 LBAT_DET_PRD_15_8=0xFF;	
kal_uint32 LBAT_DEBT_MAX_8_0=1;

extern void wake_up_pmic(void);
extern void Set_gDVT_INTERRUPT_FLAG_1(int val);
extern void Set_gDVT_INTERRUPT_FLAG_2(int val);
extern void Set_gDVT_INTERRUPT_FLAG_3(int val);
extern void Set_gDVT_INTERRUPT_FLAG_4(int val);
extern void Set_gDVT_INTERRUPT_FLAG_5(int val);

void UVVP_PMIC_INTERRUPT_TC_1_01_ldvt(void)
{	
	printk("\r\n[UVVP_PMIC_INTERRUPT_TC_1_01_ldvt]\r\n");

	printk("LOW BATTERY (AUXADC) interrupt setting .. start\r\n");

	Set_gDVT_INTERRUPT_FLAG_1(1);
	Set_gDVT_INTERRUPT_FLAG_2(0);
	Set_gDVT_INTERRUPT_FLAG_3(0);
	Set_gDVT_INTERRUPT_FLAG_4(0);
	Set_gDVT_INTERRUPT_FLAG_5(0);

	upmu_chr_adcin_vchr_en(1);
	upmu_chr_adcin_vsen_en(1);
	upmu_chr_adcin_vbat_en(1);

	upmu_interrupt_high_bat_int_en(0);
	upmu_interrupt_low_bat_int_en(1);

//1. test low battery voltage interrupt 	
	//1.1 setup max voltage treshold as VBAT = 4.2
	//SetReg(BRW_RG_LBAT_VOLT_MAX_7_0, LBAT_VOLT_MAX[7:0]);
    //SetReg(BRW_RG_LBAT_VOLT_MAX_9_8, LBAT_VOLT_MAX[9:8]);
    upmu_auxadc_lowbat_set_max_voltage_7_0(LBAT_VOLT_MAX_7_0);
	upmu_auxadc_lowbat_set_max_voltage_9_8(LBAT_VOLT_MAX_9_8);
	//1.2 setup min voltage treshold as VBAT = 3.4
    //SetReg(BRW_RG_LBAT_VOLT_MIN_7_0, LBAT_VOLT_MIN[7:0]);
    //SetReg(BRW_RG_LBAT_VOLT_MIN_9_8, LBAT_VOLT_MIN[9:8]);
    upmu_auxadc_lowbat_set_min_voltage_7_0(LBAT_VOLT_MIN_7_0);
	upmu_auxadc_lowbat_set_min_voltage_9_8(LBAT_VOLT_MIN_9_8);
	//1.3 setup detection period (detection period must > 256)
    //SetReg(BRW_RG_LBAT_DET_PRD_19_16, LBAT_DET_PRD[19:16]);
    //SetReg(BRW_RG_LBAT_DET_PRD_15_8, LBAT_DET_PRD[15:8]);
    upmu_auxadc_lowbat_period_19_16(LBAT_DET_PRD_19_16);
	upmu_auxadc_lowbat_period_15_8(LBAT_DET_PRD_15_8);
	//1.4 setup max. debounce time.
    //SetReg(BRW_RG_LBAT_DEBT_MAX, LBAT_DEBT_MAX[8:0]);
    upmu_auxadc_lowbat_debtime_max(LBAT_DEBT_MAX_8_0);
	//1.5 turn on IRQ
    //SetReg(BRW_RG_LBAT_IRQ_EN_MAX, 1'b1);
    //SetReg(BRW_RG_LBAT_IRQ_EN_MIN, 1'b1);
    upmu_auxadc_lowbat_irq_en_max_volt(1);
    upmu_auxadc_lowbat_irq_en_min_volt(1);
	//1.6 turn on LowBattery Detection
    //SetReg(BRW_RG_LBAT_EN_MAX, 1'b1); 
    //SetReg(BRW_RG_LBAT_EN_MIN, 1'b1);
	upmu_auxadc_lowbat_en_max_volt(1);
	upmu_auxadc_lowbat_en_min_volt(1);

	//Note. Read LowBattery Detect Value
    //while(! ReadReg(BRW_RG_ADC_RDY_LBAT));
    //ReadReg(BRW_RG_ADC_OUT_LBAT_7_0);
    //ReadReg(BRW_RG_ADC_OUT_LBAT_9_8);
	
//2. Test on VBAT = 3.5 -> 3.4 -> 3.3 and receive interrupt

	//test
	//wake_up_pmic();

	printk("LOWBATTERY (AUXADC) interrupt setting .. done\r\n");
	
}
void UVVP_PMIC_INTERRUPT_TC_1_02_ldvt(void)
{
	printk("\r\n[UVVP_PMIC_INTERRUPT_TC_1_02_ldvt]\r\n");

	printk("HIGH BATTERY (AUXADC) interrupt setting .. start\r\n");
	
	Set_gDVT_INTERRUPT_FLAG_1(0);
	Set_gDVT_INTERRUPT_FLAG_2(1);
	Set_gDVT_INTERRUPT_FLAG_3(0);
	Set_gDVT_INTERRUPT_FLAG_4(0);
	Set_gDVT_INTERRUPT_FLAG_5(0);

	upmu_chr_adcin_vchr_en(1);
	upmu_chr_adcin_vsen_en(1);
	upmu_chr_adcin_vbat_en(1);

	upmu_interrupt_high_bat_int_en(1);
	upmu_interrupt_low_bat_int_en(0);
	
//1. test low battery voltage interrupt 	
	//1.1 setup max voltage treshold as VBAT = 4.2
	//SetReg(BRW_RG_LBAT_VOLT_MAX_7_0, LBAT_VOLT_MAX[7:0]);
	//SetReg(BRW_RG_LBAT_VOLT_MAX_9_8, LBAT_VOLT_MAX[9:8]);
	upmu_auxadc_lowbat_set_max_voltage_7_0(LBAT_VOLT_MAX_7_0);
	upmu_auxadc_lowbat_set_max_voltage_9_8(LBAT_VOLT_MAX_9_8);
	//1.2 setup min voltage treshold as VBAT = 3.4
	//SetReg(BRW_RG_LBAT_VOLT_MIN_7_0, LBAT_VOLT_MIN[7:0]);
	//SetReg(BRW_RG_LBAT_VOLT_MIN_9_8, LBAT_VOLT_MIN[9:8]);
	upmu_auxadc_lowbat_set_min_voltage_7_0(LBAT_VOLT_MIN_7_0);
	upmu_auxadc_lowbat_set_min_voltage_9_8(LBAT_VOLT_MIN_9_8);
	//1.3 setup detection period (detection period must > 256)
	//SetReg(BRW_RG_LBAT_DET_PRD_19_16, LBAT_DET_PRD[19:16]);
	//SetReg(BRW_RG_LBAT_DET_PRD_15_8, LBAT_DET_PRD[15:8]);
	upmu_auxadc_lowbat_period_19_16(LBAT_DET_PRD_19_16);
	upmu_auxadc_lowbat_period_15_8(LBAT_DET_PRD_15_8);
	//1.4 setup max. debounce time.
	//SetReg(BRW_RG_LBAT_DEBT_MAX, LBAT_DEBT_MAX[8:0]);
	upmu_auxadc_lowbat_debtime_max(LBAT_DEBT_MAX_8_0);
	//1.5 turn on IRQ
	//SetReg(BRW_RG_LBAT_IRQ_EN_MAX, 1'b1);
	//SetReg(BRW_RG_LBAT_IRQ_EN_MIN, 1'b1);
	upmu_auxadc_lowbat_irq_en_max_volt(1);
	upmu_auxadc_lowbat_irq_en_min_volt(1);
	//1.6 turn on LowBattery Detection
	//SetReg(BRW_RG_LBAT_EN_MAX, 1'b1); 
	//SetReg(BRW_RG_LBAT_EN_MIN, 1'b1);
	upmu_auxadc_lowbat_en_max_volt(1);
	upmu_auxadc_lowbat_en_min_volt(1);

	//Note. Read LowBattery Detect Value
	//while(! ReadReg(BRW_RG_ADC_RDY_LBAT));
	//ReadReg(BRW_RG_ADC_OUT_LBAT_7_0);
	//ReadReg(BRW_RG_ADC_OUT_LBAT_9_8);
		
//2. Test on VBAT = 4.1 -> 4.2 -> 4.3 and receive interrupt

	//test
	//wake_up_pmic();
	
	printk("HIGH BATTERY (AUXADC) interrupt setting .. done\r\n");
	
}      
void UVVP_PMIC_INTERRUPT_TC_1_03_ldvt(void)
{
	printk("\r\n[UVVP_PMIC_INTERRUPT_TC_1_03_ldvt]\r\n");

	printk("LDO OC interrupt setting .. start\r\n");
	
	Set_gDVT_INTERRUPT_FLAG_1(0);
	Set_gDVT_INTERRUPT_FLAG_2(0);
	Set_gDVT_INTERRUPT_FLAG_3(1);
	Set_gDVT_INTERRUPT_FLAG_4(0);
	Set_gDVT_INTERRUPT_FLAG_5(0);

	printk("upmu_interrupt_ldo_oc_int_en(1)\r\n");
	printk("upmu_interrupt_vpa_oc_int_en(0);\r\n");
	printk("upmu_interrupt_vrf18_oc_int_en(0);\r\n");

	upmu_interrupt_ldo_oc_int_en(1);
	upmu_interrupt_vpa_oc_int_en(0);
	upmu_interrupt_vrf18_oc_int_en(0);

	//test
	//wake_up_pmic();

	printk("LDO OC interrupt setting .. done\r\n");
	
}
void UVVP_PMIC_INTERRUPT_TC_1_04_ldvt(void)
{
	printk("\r\n[UVVP_PMIC_INTERRUPT_TC_1_04_ldvt]\r\n");

	printk("VPA OC interrupt setting .. start\r\n");
	
	Set_gDVT_INTERRUPT_FLAG_1(0);
	Set_gDVT_INTERRUPT_FLAG_2(0);
	Set_gDVT_INTERRUPT_FLAG_3(0);
	Set_gDVT_INTERRUPT_FLAG_4(1);
	Set_gDVT_INTERRUPT_FLAG_5(0);

	printk("upmu_interrupt_ldo_oc_int_en(0)\r\n");
	printk("upmu_interrupt_vpa_oc_int_en(1);\r\n");
	printk("upmu_interrupt_vrf18_oc_int_en(0);\r\n");

	upmu_interrupt_ldo_oc_int_en(0);
	upmu_interrupt_vpa_oc_int_en(1);
	upmu_interrupt_vrf18_oc_int_en(0);

	//test
	//wake_up_pmic();

	printk("VPA OC interrupt setting .. done\r\n");
	
}
void UVVP_PMIC_INTERRUPT_TC_1_05_ldvt(void)
{
	printk("\r\n[UVVP_PMIC_INTERRUPT_TC_1_05_ldvt]\r\n");

	printk("VRF18 OC interrupt setting .. start\r\n");

	Set_gDVT_INTERRUPT_FLAG_1(0);
	Set_gDVT_INTERRUPT_FLAG_2(0);
	Set_gDVT_INTERRUPT_FLAG_3(0);
	Set_gDVT_INTERRUPT_FLAG_4(0);
	Set_gDVT_INTERRUPT_FLAG_5(1);

	printk("upmu_interrupt_ldo_oc_int_en(0)\r\n");
	printk("upmu_interrupt_vpa_oc_int_en(0);\r\n");
	printk("upmu_interrupt_vrf18_oc_int_en(1);\r\n");

	upmu_interrupt_ldo_oc_int_en(0);
	upmu_interrupt_vpa_oc_int_en(0);
	upmu_interrupt_vrf18_oc_int_en(1);

	//test
	//wake_up_pmic();

	printk("VRF18 OC interrupt setting .. done\r\n");
}

///////////////////////////////////////////////////////////////////////////////////
//
//  DRIVER TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
int dvt_number_count_ret_read=0;

void UVVP_PMIC_DRIVER_TC_2_01_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	char read_data[17] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
		                  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	
	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_01_ldvt]\r\n");

	printk("TC,Reg,WriteValue,GetValue\r\n");

// write 0x5A to FLASH_CON_0~4
	for(index=0 ; index<5 ; index++)
	{
		ret=pmic_bank1_config_interface(BANK1_FLASH_CON0+index, 0x5A, 0xFF, 0x0);
	}	
	// read FLASH_CON_0~4
	for(index=0 ; index<5 ; index++)
	{
		ret=pmic_bank1_read_interface(BANK1_FLASH_CON0+index, &read_data[index], 0xFF, 0x0);
	}	
	// check those value	
	for(index=0 ; index<5 ; index++)
	{
		printk("[TC_2_01],%x,0x5A,%x\r\n", BANK1_FLASH_CON0+index, read_data[index]);
	}

// write 0x5A to KPLED_CON_0~4
	for(index=0 ; index<5 ; index++)
	{
		ret=pmic_bank1_config_interface(BANK1_KPLED_CON0+index, 0x5A, 0xFF, 0x0);
	}	
	// read KPLED_CON_0~4
	for(index=0 ; index<5 ; index++)
	{
		ret=pmic_bank1_read_interface(BANK1_KPLED_CON0+index, &read_data[index], 0xFF, 0x0);
	}	
	// check those value	
	for(index=0 ; index<5 ; index++)
	{
		printk("[TC_2_01],%x,0x5A,%x\r\n", BANK1_KPLED_CON0+index, read_data[index]);
	}
	
// write 0x5A to ISINK_CON0~16
	for(index=0 ; index<17 ; index++)
	{
		ret=pmic_bank1_config_interface(BANK1_ISINKS_CON0+index, 0x5A, 0xFF, 0x0);
	}	
	// read ISINK_CON0~16
	for(index=0 ; index<17 ; index++)
	{
		ret=pmic_bank1_read_interface(BANK1_ISINKS_CON0+index, &read_data[index], 0xFF, 0x0);
	}	
	// check those value	
	for(index=0 ; index<17 ; index++)
	{
		printk("[TC_2_01],%x,0x5A,%x\r\n", BANK1_ISINKS_CON0+index, read_data[index]);
	}
	
}
void UVVP_PMIC_DRIVER_TC_2_02_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	int index_max=0x1F;
	int count=0;
	int count_data=200;
	char read_data;
	kal_uint32 ret_read=0;
	kal_uint32 ret_read_ori=0;

	//power on clock
	ret=pmic_bank1_config_interface(0x15, 0x0, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(0x15, &read_data, 0xFF, 0x0);
	printk("After set Reg[0x15] = 0x0, (%d)\r\n",read_data);

	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_02_ldvt]\r\n");

	printk("TC,FLASH_DIM_DUTY,RO_MON[0]\r\n");

	upmu_testmode_mon_grp_sel(5);
	upmu_testmode_mon_flag_sel(6);

	for( index=0; index<index_max ; index++ )
	{
		upmu_flash_dim_duty(index);
		upmu_flash_mode_select(0);
		upmu_flash_ther_shdn_en(0);
		upmu_flash_en(1);
		dvt_number_count_ret_read=0;
		for(count=0 ; count<count_data ; count++)				
		{			
			ret_read = upmu_testmode_mon_flag_status();
			ret_read_ori = ret_read;
			ret_read = ret_read & 0x01;
			ret_read = ret_read >> 0;
			dvt_number_count_ret_read+=ret_read;
			//printk("[TC_2_02],%d,%x\r\n", index, upmu_testmode_mon_flag_status());
			printk("[TC_2_02],%d,%d,%x,%d\r\n", index, ret_read, ret_read_ori, dvt_number_count_ret_read);
		}
	}

	//reset
	upmu_testmode_mon_grp_sel(0xF);
	upmu_testmode_mon_flag_sel(0);
	upmu_flash_dim_duty(0);
	upmu_flash_en(0);
	
}
void UVVP_PMIC_DRIVER_TC_2_03_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	int index_max=0xFF;
	int count=0;
	int count_data=200;
	char read_data;
	kal_uint32 ret_read=0;
	kal_uint32 ret_read_ori=0;	

	//power on clock
	ret=pmic_bank1_config_interface(0x15, 0x0, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(0x15, &read_data, 0xFF, 0x0);
	printk("After set Reg[0x15] = 0x0, (%d)\r\n",read_data);
	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_03_ldvt]\r\n");

	printk("TC,FLASH_DIM_DIV,RO_MON[0]\r\n");

	upmu_testmode_mon_grp_sel(5);
	upmu_testmode_mon_flag_sel(6);

	for( index=0; index<index_max ; index++ )
	{
		upmu_flash_dim_div(index);
		upmu_flash_mode_select(0);
		upmu_flash_ther_shdn_en(0);
		upmu_flash_en(1);		
		dvt_number_count_ret_read=0;
		for(count=0 ; count<count_data ; count++)				
		{
			ret_read = upmu_testmode_mon_flag_status();
			ret_read_ori = ret_read;
			ret_read = ret_read & 0x01;
			ret_read = ret_read >> 0;
			dvt_number_count_ret_read+=ret_read;
			//printk("[TC_2_03],%d,%x\r\n", index, upmu_testmode_mon_flag_status());
			printk("[TC_2_03],%d,%d,%x,%d\r\n", index, ret_read, ret_read_ori, dvt_number_count_ret_read);
		}
	}

	//reset
	upmu_testmode_mon_grp_sel(0xF);
	upmu_testmode_mon_flag_sel(0);
	upmu_flash_dim_div(0);
	upmu_flash_en(0);
	
}
void UVVP_PMIC_DRIVER_TC_2_04_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	int index_max=0x1F;
	int count=0;
	int count_data=200;
	char read_data;
	kal_uint32 ret_read=0;
	kal_uint32 ret_read_ori=0;	

	//power on clock
	ret=pmic_bank1_config_interface(0x15, 0x0, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(0x15, &read_data, 0xFF, 0x0);
	printk("After set Reg[0x15] = 0x0, (%d)\r\n",read_data);
	
	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_04_ldvt]\r\n");

	printk("TC,KPLED_DIM_DUTY,RO_MON[0]\r\n");

	upmu_testmode_mon_grp_sel(5);
	upmu_testmode_mon_flag_sel(0xB);

	for( index=0; index<index_max ; index++ )
	{
		upmu_kpled_dim_duty(index);
		upmu_kpled_mode_select(0);
		upmu_kpled_ther_shdn_en(0);
		upmu_kpled_en(1);
		dvt_number_count_ret_read=0;
		for(count=0 ; count<count_data ; count++)				
		{
			ret_read = upmu_testmode_mon_flag_status();
			ret_read_ori = ret_read;
			ret_read = ret_read & 0x01;
			ret_read = ret_read >> 0;
			dvt_number_count_ret_read+=ret_read;
			//printk("[TC_2_04],%d,%x\r\n", index, upmu_testmode_mon_flag_status());
			printk("[TC_2_04],%d,%d,%x,%d\r\n", index, ret_read, ret_read_ori, dvt_number_count_ret_read);
		}
	}

	//reset
	upmu_testmode_mon_grp_sel(0xF);
	upmu_testmode_mon_flag_sel(0);
	upmu_kpled_dim_duty(0);
	upmu_kpled_en(0);
	
}
void UVVP_PMIC_DRIVER_TC_2_05_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	int index_max=0xFF;
	int count=0;
	int count_data=200;
	char read_data;
	kal_uint32 ret_read=0;
	kal_uint32 ret_read_ori=0;	

	//power on clock
	ret=pmic_bank1_config_interface(0x15, 0x0, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(0x15, &read_data, 0xFF, 0x0);
	printk("After set Reg[0x15] = 0x0, (%d)\r\n",read_data);
	
	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_05_ldvt]\r\n");

	printk("TC,KPLED_DIM_DIV,RO_MON[0]\r\n");

	upmu_testmode_mon_grp_sel(5);
	upmu_testmode_mon_flag_sel(0xB);

	for( index=0; index<index_max ; index++ )
	{
		upmu_kpled_dim_div(index);
		upmu_kpled_mode_select(0);
		upmu_kpled_ther_shdn_en(0);
		upmu_kpled_en(1);
		dvt_number_count_ret_read=0;
		for(count=0 ; count<count_data ; count++)				
		{
			ret_read = upmu_testmode_mon_flag_status();
			ret_read_ori = ret_read;
			ret_read = ret_read & 0x01;
			ret_read = ret_read >> 0;
			dvt_number_count_ret_read+=ret_read;
			//printk("[TC_2_05],%d,%x\r\n", index, upmu_testmode_mon_flag_status());
			printk("[TC_2_05],%d,%d,%x,%d\r\n", index, ret_read, ret_read_ori, dvt_number_count_ret_read);
		}
	}

	//reset
	upmu_testmode_mon_grp_sel(0xF);
	upmu_testmode_mon_flag_sel(0);
	upmu_kpled_dim_div(0);
	upmu_kpled_en(0);
	
}
void UVVP_PMIC_DRIVER_TC_2_06_ldvt(void)
{
	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_06_ldvt] : change to SA\r\n");
}
void UVVP_PMIC_DRIVER_TC_2_07_ldvt(void)
{		
	kal_uint32 ret=0;
	int mode=0;
	int mode_max=2;
	int index=0;
	int index_max=0x1F;
	int count=0;
	int count_data=200;
	char read_data;
	kal_uint32 ret_read=0;
	kal_uint32 ret_read_ori=0;	

	//power on clock
	ret=pmic_bank1_config_interface(0x15, 0x0, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(0x15, &read_data, 0xFF, 0x0);
	printk("After set Reg[0x15] = 0x0, (%d)\r\n",read_data);
	
	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_07_ldvt]\r\n");

	printk("TC,ISINKS_CH5_MODE,ISINKS_DIMY_DUTY,RO_MON[0]\r\n");

	upmu_testmode_mon_grp_sel(5);
	upmu_testmode_mon_flag_sel(0x23);
	upmu_boost_isink_hw_sel(0);
	upmu_isinks_ch5_cabc_en(0);
	
	upmu_isinks_dim1_fsel(0x00);
	upmu_isinks_dim2_fsel(0x00);

	for( mode=0; mode<=mode_max ; mode++ )
	{
		upmu_isinks_ch5_mode(mode);

		if( mode==0 )
		{
			for( index=0; index<=0xF ; index++ )
			{
				upmu_isinks_dim0_duty(index);
				upmu_isinks_ch5_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_07],%d,%d,%x\r\n", mode, index, upmu_testmode_mon_flag_status());
					printk("[TC_2_07],%d,%d,%d,%x,%d\r\n", mode, index, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}
			}
			//reset
			upmu_isinks_dim0_duty(0);
			upmu_isinks_ch5_en(0);
		}
		else if( mode==1 )
		{
			for( index=0; index<=index_max ; index++ )
			{
				upmu_isinks_dim1_duty(index);
				upmu_isinks_ch5_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_07],%d,%d,%x\r\n", mode, index, upmu_testmode_mon_flag_status());
					printk("[TC_2_07],%d,%d,%d,%x,%d\r\n", mode, index, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}
			}
			//reset
			upmu_isinks_dim1_duty(0);
			upmu_isinks_ch5_en(0);
		}
		else if( mode==2 )
		{
			for( index=0; index<=index_max ; index++ )
			{
				upmu_isinks_dim2_duty(index);
				upmu_isinks_ch5_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_07],%d,%d,%x\r\n", mode, index, upmu_testmode_mon_flag_status());
					printk("[TC_2_07],%d,%d,%d,%x,%d\r\n", mode, index, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}
			}
			//reset
			upmu_isinks_dim2_duty(0);
			upmu_isinks_ch5_en(0);			
		}
		else
		{
		}
		
	}

	//reset
	upmu_testmode_mon_grp_sel(0xF);
	upmu_testmode_mon_flag_sel(0);

	
}
void UVVP_PMIC_DRIVER_TC_2_08_ldvt(void)
{
	kal_uint32 ret=0;
	int mode=0;
	int mode_max=2;
	int index=0;
	int index_max=0x1F;
	int count=0;
	int count_data=200;
	char read_data;
	kal_uint32 ret_read=0;
	kal_uint32 ret_read_ori=0;	
	
	//power on clock
	ret=pmic_bank1_config_interface(0x15, 0x0, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(0x15, &read_data, 0xFF, 0x0);
	printk("After set Reg[0x15] = 0x0, (%d)\r\n",read_data);

	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_08_ldvt]\r\n");

	printk("TC,ISINKS_CH5_MODE,ISINKS_DIMY_FSEL,RO_MON[0]\r\n");

	upmu_testmode_mon_grp_sel(5);
	upmu_testmode_mon_flag_sel(0x23);
	upmu_boost_isink_hw_sel(0);
	upmu_isinks_ch5_cabc_en(0);

	for( mode=0; mode<=mode_max ; mode++ )
	{
		upmu_isinks_ch5_mode(mode);

		if( mode==0 )
		{
			for( index=0; index<=index_max ; index++ )
			{
				upmu_isinks_dim0_fsel(index);
				upmu_isinks_ch5_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_08],%d,%d,%x\r\n", mode, index, upmu_testmode_mon_flag_status());
					printk("[TC_2_08],%d,%d,%d,%x,%d\r\n", mode, index, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}
			}
			//reset
			upmu_isinks_dim0_fsel(0);
			upmu_isinks_ch5_en(0);
		}
		else if( mode==1 )
		{
			for( index=0; index<=index_max ; index++ )
			{
				upmu_isinks_dim1_fsel(index);
				upmu_isinks_ch5_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_08],%d,%d,%x\r\n", mode, index, upmu_testmode_mon_flag_status());
					printk("[TC_2_08],%d,%d,%d,%x,%d\r\n", mode, index, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}
			}
			//reset
			upmu_isinks_dim1_fsel(0);
			upmu_isinks_ch5_en(0);
		}
		else if( mode==2 )
		{
			for( index=0; index<=index_max ; index++ )
			{
				upmu_isinks_dim2_fsel(index);
				upmu_isinks_ch5_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_08],%d,%d,%x\r\n", mode, index, upmu_testmode_mon_flag_status());
					printk("[TC_2_08],%d,%d,%d,%x,%d\r\n", mode, index, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}
			}
			//reset
			upmu_isinks_dim2_fsel(0);
			upmu_isinks_ch5_en(0);			
		}
		else
		{
		}
		
	}

	//reset
	upmu_testmode_mon_grp_sel(0xF);
	upmu_testmode_mon_flag_sel(0);
	
}

void UVVP_PMIC_DRIVER_TC_2_09_ldvt(void)
{
	kal_uint32 ret=0;
	int channel=0;
	int channel_max=5;
	int mode=0;
	int mode_max=2;
	int index=0;
	int index_max=0xF;
	int count=0;
	int count_data=200;
	char read_data;
	int temp_value=0;
	kal_uint32 ret_read=0;
	kal_uint32 ret_read_ori=0;	

	//power on clock
	ret=pmic_bank1_config_interface(0x15, 0x0, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(0x15, &read_data, 0xFF, 0x0);
	printk("After set Reg[0x15] = 0x0, (%d)\r\n",read_data);

	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_09_ldvt]\r\n");

	printk("TC,ISINKS_CH,ISINKS_CH_MODE,RO_MON[0]\r\n");

	upmu_testmode_mon_grp_sel(5);
	upmu_boost_isink_hw_sel(0);
	upmu_isinks_ch1_cabc_en(0);
	upmu_isinks_ch2_cabc_en(0);
	upmu_isinks_ch3_cabc_en(0);
	upmu_isinks_ch4_cabc_en(0);
	upmu_isinks_ch5_cabc_en(0);

	for( channel=0; channel<=channel_max ; channel++ )
	{
		temp_value=(0x1E)+channel; 
		upmu_testmode_mon_flag_sel(temp_value);

		if( channel==0 )
		{				
			for( mode=0; mode<=mode_max ; mode++ )
			{
				upmu_isinks_ch0_mode(mode);				
				if( mode==0 )
				{
					upmu_isinks_dim0_duty(0x5A);
					upmu_isinks_dim0_fsel(0xA5);										
				}
				else if( mode==1 )
				{
					upmu_isinks_dim1_duty(0x5A);
					upmu_isinks_dim1_fsel(0xA5);
				}
				else if( mode==2 )
				{
					upmu_isinks_dim2_duty(0x5A);
					upmu_isinks_dim2_fsel(0xA5);
				}
				else
				{
				}
				upmu_isinks_ch0_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_09],%d,%d,%x\r\n", channel, mode, upmu_testmode_mon_flag_status());
					printk("[TC_2_09],%d,%d,%d,%x,%d\r\n", channel, mode, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}

				//reset
				upmu_isinks_ch0_en(0);				
			}
			
		}
		else if( channel==1 )
		{
			for( mode=0; mode<=mode_max ; mode++ )
			{
				upmu_isinks_ch1_mode(mode);				
				if( mode==0 )
				{
					upmu_isinks_dim0_duty(0x5A);
					upmu_isinks_dim0_fsel(0xA5);										
				}
				else if( mode==1 )
				{
					upmu_isinks_dim1_duty(0x5A);
					upmu_isinks_dim1_fsel(0xA5);
				}
				else if( mode==2 )
				{
					upmu_isinks_dim2_duty(0x5A);
					upmu_isinks_dim2_fsel(0xA5);
				}
				else
				{
				}
				upmu_isinks_ch1_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_09],%d,%d,%x\r\n", channel, mode, upmu_testmode_mon_flag_status());
					printk("[TC_2_09],%d,%d,%d,%x,%d\r\n", channel, mode, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}

				//reset
				upmu_isinks_ch1_en(0);				
			}
			
		}
		else if( channel==2 )
		{
			for( mode=0; mode<=mode_max ; mode++ )
			{
				upmu_isinks_ch2_mode(mode);				
				if( mode==0 )
				{
					upmu_isinks_dim0_duty(0x5A);
					upmu_isinks_dim0_fsel(0xA5);										
				}
				else if( mode==1 )
				{
					upmu_isinks_dim1_duty(0x5A);
					upmu_isinks_dim1_fsel(0xA5);
				}
				else if( mode==2 )
				{
					upmu_isinks_dim2_duty(0x5A);
					upmu_isinks_dim2_fsel(0xA5);
				}
				else
				{
				}
				upmu_isinks_ch2_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_09],%d,%d,%x\r\n", channel, mode, upmu_testmode_mon_flag_status());
					printk("[TC_2_09],%d,%d,%d,%x,%d\r\n", channel, mode, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}

				//reset
				upmu_isinks_ch2_en(0);				
			}
			
		}
		else if( channel==3 )
		{
			for( mode=0; mode<=mode_max ; mode++ )
			{
				upmu_isinks_ch3_mode(mode);				
				if( mode==0 )
				{
					upmu_isinks_dim0_duty(0x5A);
					upmu_isinks_dim0_fsel(0xA5);										
				}
				else if( mode==1 )
				{
					upmu_isinks_dim1_duty(0x5A);
					upmu_isinks_dim1_fsel(0xA5);
				}
				else if( mode==2 )
				{
					upmu_isinks_dim2_duty(0x5A);
					upmu_isinks_dim2_fsel(0xA5);
				}
				else
				{
				}
				upmu_isinks_ch3_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_09],%d,%d,%x\r\n", channel, mode, upmu_testmode_mon_flag_status());
					printk("[TC_2_09],%d,%d,%d,%x,%d\r\n", channel, mode, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}

				//reset
				upmu_isinks_ch3_en(0);				
			}
			
		}
		else if( channel==4 )
		{
			for( mode=0; mode<=mode_max ; mode++ )
			{
				upmu_isinks_ch4_mode(mode);				
				if( mode==0 )
				{
					upmu_isinks_dim0_duty(0x5A);
					upmu_isinks_dim0_fsel(0xA5);										
				}
				else if( mode==1 )
				{
					upmu_isinks_dim1_duty(0x5A);
					upmu_isinks_dim1_fsel(0xA5);
				}
				else if( mode==2 )
				{
					upmu_isinks_dim2_duty(0x5A);
					upmu_isinks_dim2_fsel(0xA5);
				}
				else
				{
				}
				upmu_isinks_ch4_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_09],%d,%d,%x\r\n", channel, mode, upmu_testmode_mon_flag_status());
					printk("[TC_2_09],%d,%d,%d,%x,%d\r\n", channel, mode, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}

				//reset
				upmu_isinks_ch4_en(0);				
			}
			
		}
		else if( channel==5 )
		{
			for( mode=0; mode<=mode_max ; mode++ )
			{
				upmu_isinks_ch5_mode(mode);				
				if( mode==0 )
				{
					upmu_isinks_dim0_duty(0x5A);
					upmu_isinks_dim0_fsel(0xA5);										
				}
				else if( mode==1 )
				{
					upmu_isinks_dim1_duty(0x5A);
					upmu_isinks_dim1_fsel(0xA5);
				}
				else if( mode==2 )
				{
					upmu_isinks_dim2_duty(0x5A);
					upmu_isinks_dim2_fsel(0xA5);
				}
				else
				{
				}
				upmu_isinks_ch5_en(1);

				dvt_number_count_ret_read=0;
				for(count=0 ; count<count_data ; count++)				
				{	
					ret_read = upmu_testmode_mon_flag_status();
					ret_read_ori = ret_read;
					ret_read = ret_read & 0x01;
					ret_read = ret_read >> 0;
					dvt_number_count_ret_read+=ret_read;
					//printk("[TC_2_09],%d,%d,%x\r\n", channel, mode, upmu_testmode_mon_flag_status());
					printk("[TC_2_09],%d,%d,%d,%x,%d\r\n", channel, mode, ret_read, ret_read_ori, dvt_number_count_ret_read);
				}

				//reset
				upmu_isinks_ch5_en(0);				
			}
			
		}
		else
		{
		}
	}

	//reset
	upmu_testmode_mon_grp_sel(0xF);
	upmu_testmode_mon_flag_sel(0);
	upmu_isinks_dim0_duty(0);
	upmu_isinks_dim0_fsel(0);
	upmu_isinks_dim1_duty(0);
	upmu_isinks_dim1_fsel(0);
	upmu_isinks_dim2_duty(0);
	upmu_isinks_dim2_fsel(0);
	
}

void UVVP_PMIC_DRIVER_TC_2_10_ldvt(void)
{
	kal_uint32 ret=0;
	int channel=0;
	int channel_max=5;
	int mode=0;
	int mode_max=2;
	int index=0;
	int index_max=0xF;
	int count=0;
	int count_data=200;
	char read_data;
	int temp_value=0;
	kal_uint32 ret_read=0;
	kal_uint32 ret_read_ori=0;	

	//power on clock
	ret=pmic_bank1_config_interface(0x15, 0x0, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(0x15, &read_data, 0xFF, 0x0);
	printk("After set Reg[0x15] = 0x0, (%d)\r\n",read_data);

	printk("\r\n[UVVP_PMIC_DRIVER_TC_2_10_ldvt]\r\n");

	printk("TC,ISINKS_CH,ISINKS_CH_MODE,RO_MON[0]\r\n");

	upmu_testmode_mon_grp_sel(5);
	upmu_boost_isink_hw_sel(1);
	upmu_isinks_ch1_cabc_en(0);
	upmu_isinks_ch2_cabc_en(0);
	upmu_isinks_ch3_cabc_en(0);
	upmu_isinks_ch4_cabc_en(0);
	upmu_isinks_ch5_cabc_en(0);	

	for( channel=0; channel<=channel_max ; channel++ )
	{
		temp_value=(0x1E)+channel; 
		upmu_testmode_mon_flag_sel(temp_value);

		if( channel==0 )
		{
			upmu_isinks_ch0_mode(0x3);
			upmu_isinks_ch0_en(1);
			dvt_number_count_ret_read=0;
			for(count=0 ; count<count_data ; count++)				
			{
				ret_read = upmu_testmode_mon_flag_status();
				ret_read_ori = ret_read;
				ret_read = ret_read & 0x01;
				ret_read = ret_read >> 0;
				dvt_number_count_ret_read+=ret_read;
				//printk("[TC_2_10],%d,%d,%x\r\n", channel, 0x3, upmu_testmode_mon_flag_status());
				printk("[TC_2_10],%d,%d,%d,%x,%d\r\n", channel, 0x3, ret_read, ret_read_ori, dvt_number_count_ret_read);
			}
			upmu_isinks_ch0_en(0);
		}
		else if( channel==1 )
		{
			upmu_isinks_ch1_mode(0x3);
			upmu_isinks_ch1_en(1);			
			dvt_number_count_ret_read=0;
			for(count=0 ; count<count_data ; count++)				
			{
				ret_read = upmu_testmode_mon_flag_status();
				ret_read_ori = ret_read;
				ret_read = ret_read & 0x01;
				ret_read = ret_read >> 0;
				dvt_number_count_ret_read+=ret_read;
				//printk("[TC_2_10],%d,%d,%x\r\n", channel, 0x3, upmu_testmode_mon_flag_status());
				printk("[TC_2_10],%d,%d,%d,%x,%d\r\n", channel, 0x3, ret_read, ret_read_ori, dvt_number_count_ret_read);
			}
			upmu_isinks_ch1_en(0);		
		}
		else if( channel==2 )
		{
			upmu_isinks_ch2_mode(0x3);
			upmu_isinks_ch2_en(1);
			dvt_number_count_ret_read=0;
			for(count=0 ; count<count_data ; count++)				
			{
				ret_read = upmu_testmode_mon_flag_status();
				ret_read_ori = ret_read;
				ret_read = ret_read & 0x01;
				ret_read = ret_read >> 0;
				dvt_number_count_ret_read+=ret_read;
				//printk("[TC_2_10],%d,%d,%x\r\n", channel, 0x3, upmu_testmode_mon_flag_status());
				printk("[TC_2_10],%d,%d,%d,%x,%d\r\n", channel, 0x3, ret_read, ret_read_ori, dvt_number_count_ret_read);
			}
			upmu_isinks_ch2_en(0);		
		}
		else if( channel==3 )
		{
			upmu_isinks_ch3_mode(0x3);
			upmu_isinks_ch3_en(1);
			dvt_number_count_ret_read=0;
			for(count=0 ; count<count_data ; count++)				
			{
				ret_read = upmu_testmode_mon_flag_status();
				ret_read_ori = ret_read;
				ret_read = ret_read & 0x01;
				ret_read = ret_read >> 0;
				dvt_number_count_ret_read+=ret_read;
				//printk("[TC_2_10],%d,%d,%x\r\n", channel, 0x3, upmu_testmode_mon_flag_status());
				printk("[TC_2_10],%d,%d,%d,%x,%d\r\n", channel, 0x3, ret_read, ret_read_ori, dvt_number_count_ret_read);
			}
			upmu_isinks_ch3_en(0);
		}
		else if( channel==4 )
		{
			upmu_isinks_ch4_mode(0x3);
			upmu_isinks_ch4_en(1);
			dvt_number_count_ret_read=0;
			for(count=0 ; count<count_data ; count++)				
			{
				ret_read = upmu_testmode_mon_flag_status();
				ret_read_ori = ret_read;
				ret_read = ret_read & 0x01;
				ret_read = ret_read >> 0;
				dvt_number_count_ret_read+=ret_read;
				//printk("[TC_2_10],%d,%d,%x\r\n", channel, 0x3, upmu_testmode_mon_flag_status());
				printk("[TC_2_10],%d,%d,%d,%x,%d\r\n", channel, 0x3, ret_read, ret_read_ori, dvt_number_count_ret_read);
			}
			upmu_isinks_ch4_en(0);		
		}
		else if( channel==5 )
		{
			upmu_isinks_ch5_mode(0x3);
			upmu_isinks_ch5_en(1);
			dvt_number_count_ret_read=0;
			for(count=0 ; count<count_data ; count++)				
			{
				ret_read = upmu_testmode_mon_flag_status();
				ret_read_ori = ret_read;
				ret_read = ret_read & 0x01;
				ret_read = ret_read >> 0;
				dvt_number_count_ret_read+=ret_read;
				//printk("[TC_2_10],%d,%d,%x\r\n", channel, 0x3, upmu_testmode_mon_flag_status());
				printk("[TC_2_10],%d,%d,%d,%x,%d\r\n", channel, 0x3, ret_read, ret_read_ori, dvt_number_count_ret_read);
			}
			upmu_isinks_ch5_en(0);		
		}
		else 
		{
		}

	}

	//reset
	upmu_testmode_mon_grp_sel(0xF);	
	upmu_testmode_mon_flag_sel(0);
	upmu_boost_isink_hw_sel(0);
	upmu_isinks_ch0_mode(0);
	upmu_isinks_ch1_mode(0);
	upmu_isinks_ch2_mode(0);
	
}

///////////////////////////////////////////////////////////////////////////////////
//
//  BOOST TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void UVVP_PMIC_BOOST_TC_3_01_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	char read_data[4] = {0xFF,0xFF,0xFF,0xFF,0xFF};
	
	printk("\r\n[UVVP_PMIC_BOOST_TC_3_01_ldvt]\r\n");

	// write 0x5A to BOOST_CON_0~4
	for(index=0 ; index<5 ; index++)
	{
		ret=pmic_bank1_config_interface(BANK1_BOOST_CON0+index, 0x5A, 0xFF, 0x0);
	}	
	// read BOOST_CON_0~4
	for(index=0 ; index<5 ; index++)
	{
		ret=pmic_bank1_read_interface(BANK1_BOOST_CON0+index, &read_data[index], 0xFF, 0x0);
	}	
	// check those value
	printk("Reg,WriteValue,GetValue\r\n");
	for(index=0 ; index<5 ; index++)
	{
		printk("%x,0x5A,%x\r\n", BANK1_BOOST_CON0+index, read_data[index]);
	}
	
}
void UVVP_PMIC_BOOST_TC_3_02_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	int count=0;
	int count_data=200;
	char read_data;
	kal_uint32 ret_read=0;
	kal_uint32 ret_read_ori=0;

	//power on clock
	ret=pmic_bank1_config_interface(0x15, 0x0, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(0x15, &read_data, 0xFF, 0x0);
	printk("After set Reg[0x15] = 0x0, (%d)\r\n",read_data);
	
	printk("\r\n[UVVP_PMIC_BOOST_TC_3_02_ldvt]\r\n");

	upmu_testmode_mon_grp_sel(4);
	upmu_testmode_mon_flag_sel(1);
	upmu_boost_isink_hw_sel(1);
	upmu_boost_cabc_en(0);

	printk("boost_mode,dimY_duty,dimY_fsel,RO_MON[1]\r\n");

	for(index=0 ; index<3 ; index++)
	{
		upmu_boost_mode(index);

		if( index==0 )
		{	
			upmu_isinks_dim0_duty(0x5A);
			//upmu_isinks_dim0_fsel(0xA5);
			upmu_isinks_dim0_fsel(0x12);
			upmu_boost_en(1);
			for(count=0 ; count<count_data ; count++)				
			{
				ret_read = upmu_testmode_mon_flag_status();
				ret_read_ori = ret_read;
				ret_read = ret_read & 0x02;
				ret_read = ret_read >> 1;
				printk("%d,0x5A,0xA5,%d,%x\r\n", index, ret_read, ret_read_ori);
			}
		}
		else if( index==1 )
		{
			upmu_isinks_dim1_duty(0x5A);
			upmu_isinks_dim1_fsel(0xA5);
			upmu_boost_en(1);
			for(count=0 ; count<count_data ; count++)				
			{				
				ret_read = upmu_testmode_mon_flag_status();
				ret_read_ori = ret_read;
				ret_read = ret_read & 0x02;
				ret_read = ret_read >> 1;
				printk("%d,0x5A,0xA5,%d,%x\r\n", index, ret_read, ret_read_ori);
			}
		}
		else if( index==2 )
		{
			upmu_isinks_dim2_duty(0x5A);
			upmu_isinks_dim2_fsel(0xA5);
			upmu_boost_en(1);
			for(count=0 ; count<count_data ; count++)				
			{				
				ret_read = upmu_testmode_mon_flag_status();
				ret_read_ori = ret_read;
				ret_read = ret_read & 0x02;
				ret_read = ret_read >> 1;
				printk("%d,0x5A,0xA5,%d,%x\r\n", index, ret_read, ret_read_ori);				
			}			
		}
		else 
		{
		}

		//reset
		upmu_boost_en(0);
	}

	//reset
	upmu_testmode_mon_grp_sel(0xF);
	upmu_testmode_mon_flag_sel(0);
	upmu_boost_en(0);
	upmu_boost_isink_hw_sel(0);

}

///////////////////////////////////////////////////////////////////////////////////
//
//  FGADC TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void UVVP_PMIC_FGADC_TC_4_01_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	char read_data[33] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
						  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
						  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
						  0xFF,0xFF,0xFF};

	printk("\r\n[UVVP_PMIC_FGADC_TC_4_01_ldvt]\r\n");

	//write 0x5A to FGADC_CON_0~32
	for(index=0 ; index<33 ; index++)
	{
		ret=pmic_bank1_config_interface(BANK1_FGADC_CON0+index, 0x5A, 0xFF, 0x0);
	}	
	//read FGADC_CON_0~32
	for(index=0 ; index<33 ; index++)
	{
		ret=pmic_bank1_read_interface(BANK1_FGADC_CON0+index, &read_data[index], 0xFF, 0x0);
	}
	//check
	printk("Reg,WriteValue,GetValue\r\n");
	for(index=0 ; index<33 ; index++)
	{
		printk("%x,0x5A,%x\r\n", BANK1_FGADC_CON0+index, read_data[index]);
	}
	
}

///////////////////////////////////////////////////////////////////////////////////
//
//  AUXADC TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void UVVP_PMIC_AUXADC_TC_5_01_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	int count=0;
	int count_time_out=5;
	kal_uint32 ret_data=0;
	kal_uint32 temp_data_7_0=0;
	kal_uint32 temp_data_9_8=0;
	
	printk("\r\n[UVVP_PMIC_AUXADC_TC_5_01_ldvt]\r\n");

	printk("Set ADCIN enable for VBAT, VSENSE, CHR\r\n");
	upmu_chr_adcin_vchr_en(1);
	upmu_chr_adcin_vsen_en(1);
	upmu_chr_adcin_vbat_en(1);
	upmu_chr_baton_tdet_en(1);

	printk("Please input the voltage for channel 0,1,2,3\r\n");

	for(index=0 ; index<=3 ; index++)
	{
		count=0;
		ret_data=0;
		temp_data_7_0=0;
		temp_data_9_8=0;
		
		upmu_auxadc_ch_sel(index);
		upmu_auxadc_start(0);
		upmu_auxadc_start(1);

		if( index==0 )
		{
			while( upmu_auxadc_get_ch0_ready() != 1 )
			{
				printk("Wait upmu_auxadc_get_ch0_ready\r\n");
				
				if( (count++) > count_time_out)
				{
					printk("Time out!\r\n");
					break;
				}
			}	
			temp_data_7_0=upmu_auxadc_get_ch0_data_7_0();
			ret_data |= temp_data_7_0;
			temp_data_9_8=upmu_auxadc_get_ch0_data_9_8();
			ret_data |= (temp_data_9_8 << 8);
			printk("[CH0] ret_data=%d (9_8=%x,7_0=%x)\r\n", ret_data, temp_data_9_8, temp_data_7_0);
			ret_data = ((ret_data*1200)/1024)*4;
			printk("[CH0] Voltage=%d \r\n", ret_data);
		}
		else if( index==1 )
		{			
			while( upmu_auxadc_get_ch1_ready() != 1 )
			{
				printk("Wait upmu_auxadc_get_ch1_ready\r\n");
				
				if( (count++) > count_time_out)
				{
					printk("Time out!\r\n");
					break;
				}
			}
			temp_data_7_0=upmu_auxadc_get_ch1_data_7_0();
			ret_data |= temp_data_7_0;
			temp_data_9_8=upmu_auxadc_get_ch1_data_9_8();
			ret_data |= (temp_data_9_8 << 8);
			printk("[CH1] ret_data=%d (9_8=%x,7_0=%x)\r\n", ret_data, temp_data_9_8, temp_data_7_0);
			ret_data = ((ret_data*1200)/1024)*4;
			printk("[CH1] Voltage=%d \r\n", ret_data);
		}
		else if( index==2 )
		{			
			while( upmu_auxadc_get_ch2_ready() != 1 )
			{
				printk("Wait upmu_auxadc_get_ch2_ready\r\n");
				
				if( (count++) > count_time_out)
				{
					printk("Time out!\r\n");
					break;
				}
			}			
			temp_data_7_0=upmu_auxadc_get_ch2_data_7_0();
			ret_data |= temp_data_7_0;
			temp_data_9_8=upmu_auxadc_get_ch2_data_9_8();
			ret_data |= (temp_data_9_8 << 8);
			printk("[CH2] ret_data=%d (9_8=%x,7_0=%x)\r\n", ret_data, temp_data_9_8, temp_data_7_0);
			ret_data = (((ret_data*1200)/1024)*369)/39;
			printk("[CH2] Voltage=%d \r\n", ret_data);
		}
		else if( index==3 )
		{
			while( upmu_auxadc_get_ch3_ready() != 1 )
			{
				printk("Wait upmu_auxadc_get_ch3_ready\r\n");
				
				if( (count++) > count_time_out)					
				{
					printk("Time out!\r\n");
					break;
				}
			}			
			temp_data_7_0=upmu_auxadc_get_ch3_data_7_0();
			ret_data |= temp_data_7_0;
			temp_data_9_8=upmu_auxadc_get_ch3_data_9_8();
			ret_data |= (temp_data_9_8 << 8);
			printk("[CH3] ret_data=%d (9_8=%x,7_0=%x)\r\n", ret_data, temp_data_9_8, temp_data_7_0);
			ret_data = ((ret_data*1200)/1024);
			printk("[CH3] Voltage=%d \r\n", ret_data);
		}
		else 
		{
		}

		upmu_auxadc_start(0);
	}

	
}
void UVVP_PMIC_AUXADC_TC_5_02_ldvt(void)
{
	kal_uint32 ret=0;
	int index=0;
	int count=0;
	int count_time_out=5;
	kal_uint32 not_trim_data=0;
	kal_uint32 trim_data=0;
	kal_uint32 temp_data_7_0=0;
	kal_uint32 temp_data_9_8=0;
	kal_uint32 gain_value=0;
	kal_uint32 offset_value=0;
	
	kal_uint32 offset_trim=0;
	kal_uint32 gain_trim=0;
	kal_uint32 offset_trim_data=0;
	kal_uint32 gain_trim_data=0;
	kal_uint32 data_63_56=0;
	kal_uint32 data_55_48=0;
	
	printk("\r\n[UVVP_PMIC_AUXADC_TC_5_02_ldvt]\r\n");

	printk("Set ADCIN enable for VBAT, VSENSE, CHR\r\n");
	upmu_chr_adcin_vchr_en(1);
	upmu_chr_adcin_vsen_en(1);
	upmu_chr_adcin_vbat_en(1);
	
#if 0
// 1. Check trimed value and not trimed value
	// 1.1 Set Vin as 3.4 / 3.8 / 4.2
	printk("Please input the voltage for channel 0\r\n");
	// Get VBAT
	upmu_auxadc_ch_sel(0);
	upmu_auxadc_start(0);
	upmu_auxadc_start(1);
	while( upmu_auxadc_get_ch0_ready() != 1 )
	{
		printk("Wait upmu_auxadc_get_ch0_ready\r\n");
		
		if( (count++) > count_time_out)
		{
			printk("Time out!\r\n");
			break;
		}
	}
	// not trimed value
	temp_data_7_0=upmu_auxadc_get_ch0_data_7_0();
	not_trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_ch0_data_9_8();
	not_trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:not trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", not_trim_data, temp_data_9_8, temp_data_7_0);
	// trimed value
	temp_data_7_0=upmu_auxadc_get_trimming_data_7_0();
	trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_trimming_data_9_8();
	trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", trim_data, temp_data_9_8, temp_data_7_0);
	upmu_auxadc_start(0);

// 2. Check trimed value = fun( not_trimed_value, gain, offset )	
	// 2.1 Get gain and offset
	gain_value=upmu_otpc_otp_pdo_63_56();
	offset_value=upmu_otpc_otp_pdo_55_48();
	// 2.2 check trimed value ?= fun( not_trimed_value, gain, offset )
	printk("[CH0] TrimData=%d, NotTrimData=%d, Gain=%d, Offset=%d\r\n", 
		trim_data, not_trim_data, gain_value, offset_value);
#endif

// 1. Set Vin as 3.4 / 3.8 / 4.2
	printk("Please input the voltage for channel 0\r\n");

	// 2. set OTP value
	gain_trim=0x4D;
	offset_trim=0x22;
	gain_trim_data=0x4D;
	offset_trim_data=0x11;
	upmu_otpc_otp_out_sel(0x1);
	upmu_otpc_otp_skip_out(0x1);
	upmu_otpc_otp_val_63_56(offset_trim_data);
	upmu_otpc_otp_val_55_48(gain_trim_data);
	// 3. Get value
	// Get VBAT
	upmu_auxadc_ch_sel(0);
	upmu_auxadc_start(0);
	upmu_auxadc_start(1);
	while( upmu_auxadc_get_ch0_ready() != 1 )
	{
		printk("Wait upmu_auxadc_get_ch0_ready\r\n");
		
		if( (count++) > count_time_out)
		{
			printk("Time out!\r\n");
			break;
		}
	}
	not_trim_data=0;
	trim_data=0;
	// not trimed value
	temp_data_7_0=upmu_auxadc_get_ch0_data_7_0();
	not_trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_ch0_data_9_8();
	not_trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:not trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", not_trim_data, temp_data_9_8, temp_data_7_0);
	// trimed value
	temp_data_7_0=upmu_auxadc_get_trimming_data_7_0();
	trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_trimming_data_9_8();
	trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", trim_data, temp_data_9_8, temp_data_7_0);
	upmu_auxadc_start(0);
	//Get gain and offset
	data_63_56=upmu_otpc_otp_pdo_63_56();
	data_55_48=upmu_otpc_otp_pdo_55_48();
	//Check trimed value ?= fun( not_trimed_value, gain, offset )
	printk("[CH0] TrimData=%d, NotTrimData=%d, data_63_56=0x%x, data_55_48=0x%x\r\n", trim_data, not_trim_data, data_63_56, data_55_48);
	//---------------------------------------------------------------------------------------------------------------
	// 2. set OTP value
	gain_trim=0x4D;
	offset_trim=0x1F;
	gain_trim_data=0xCD;
	offset_trim_data=0x0F;
	upmu_otpc_otp_out_sel(0x1);
	upmu_otpc_otp_skip_out(0x1);
	upmu_otpc_otp_val_63_56(offset_trim_data);
	upmu_otpc_otp_val_55_48(gain_trim_data);
	// 3. Get value
	// Get VBAT
	upmu_auxadc_ch_sel(0);
	upmu_auxadc_start(0);
	upmu_auxadc_start(1);
	while( upmu_auxadc_get_ch0_ready() != 1 )
	{
		printk("Wait upmu_auxadc_get_ch0_ready\r\n");
		
		if( (count++) > count_time_out)
		{
			printk("Time out!\r\n");
			break;
		}
	}
	not_trim_data=0;
	trim_data=0;
	// not trimed value
	temp_data_7_0=upmu_auxadc_get_ch0_data_7_0();
	not_trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_ch0_data_9_8();
	not_trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:not trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", not_trim_data, temp_data_9_8, temp_data_7_0);
	// trimed value
	temp_data_7_0=upmu_auxadc_get_trimming_data_7_0();
	trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_trimming_data_9_8();
	trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", trim_data, temp_data_9_8, temp_data_7_0);
	upmu_auxadc_start(0);
	//Get gain and offset
	data_63_56=upmu_otpc_otp_pdo_63_56();
	data_55_48=upmu_otpc_otp_pdo_55_48();
	//Check trimed value ?= fun( not_trimed_value, gain, offset )
	printk("[CH0] TrimData=%d, NotTrimData=%d, data_63_56=0x%x, data_55_48=0x%x\r\n", trim_data, not_trim_data, data_63_56, data_55_48);
	//---------------------------------------------------------------------------------------------------------------
	// 2. set OTP value
	gain_trim=0x33;
	offset_trim=0x22;
	gain_trim_data=0x33;
	offset_trim_data=0x11;
	upmu_otpc_otp_out_sel(0x1);
	upmu_otpc_otp_skip_out(0x1);
	upmu_otpc_otp_val_63_56(offset_trim_data);
	upmu_otpc_otp_val_55_48(gain_trim_data);
	// 3. Get value
	// Get VBAT
	upmu_auxadc_ch_sel(0);
	upmu_auxadc_start(0);
	upmu_auxadc_start(1);
	while( upmu_auxadc_get_ch0_ready() != 1 )
	{
		printk("Wait upmu_auxadc_get_ch0_ready\r\n");
		
		if( (count++) > count_time_out)
		{
			printk("Time out!\r\n");
			break;
		}
	}
	not_trim_data=0;
	trim_data=0;
	// not trimed value
	temp_data_7_0=upmu_auxadc_get_ch0_data_7_0();
	not_trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_ch0_data_9_8();
	not_trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:not trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", not_trim_data, temp_data_9_8, temp_data_7_0);
	// trimed value
	temp_data_7_0=upmu_auxadc_get_trimming_data_7_0();
	trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_trimming_data_9_8();
	trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", trim_data, temp_data_9_8, temp_data_7_0);
	upmu_auxadc_start(0);
	//Get gain and offset
	data_63_56=upmu_otpc_otp_pdo_63_56();
	data_55_48=upmu_otpc_otp_pdo_55_48();
	//Check trimed value ?= fun( not_trimed_value, gain, offset )
	printk("[CH0] TrimData=%d, NotTrimData=%d, data_63_56=0x%x, data_55_48=0x%x\r\n", trim_data, not_trim_data, data_63_56, data_55_48);
	//---------------------------------------------------------------------------------------------------------------
	// 2. set OTP value
	gain_trim=0x33;
	offset_trim=0x1F;
	gain_trim_data=0xB3;
	offset_trim_data=0x0F;
	upmu_otpc_otp_out_sel(0x1);
	upmu_otpc_otp_skip_out(0x1);
	upmu_otpc_otp_val_63_56(offset_trim_data);
	upmu_otpc_otp_val_55_48(gain_trim_data);
	// 3. Get value
	// Get VBAT
	upmu_auxadc_ch_sel(0);
	upmu_auxadc_start(0);
	upmu_auxadc_start(1);
	while( upmu_auxadc_get_ch0_ready() != 1 )
	{
		printk("Wait upmu_auxadc_get_ch0_ready\r\n");
		
		if( (count++) > count_time_out)
		{
			printk("Time out!\r\n");
			break;
		}
	}
	not_trim_data=0;
	trim_data=0;	
	// not trimed value
	temp_data_7_0=upmu_auxadc_get_ch0_data_7_0();
	not_trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_ch0_data_9_8();
	not_trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:not trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", not_trim_data, temp_data_9_8, temp_data_7_0);
	// trimed value
	temp_data_7_0=upmu_auxadc_get_trimming_data_7_0();
	trim_data |= temp_data_7_0;
	temp_data_9_8=upmu_auxadc_get_trimming_data_9_8();
	trim_data |= (temp_data_9_8 << 8);
	printk("[CH0:trimed value] Voltage=%d (9_8=%x,7_0=%x)\r\n", trim_data, temp_data_9_8, temp_data_7_0);
	upmu_auxadc_start(0);
	//Get gain and offset
	data_63_56=upmu_otpc_otp_pdo_63_56();
	data_55_48=upmu_otpc_otp_pdo_55_48();
	//Check trimed value ?= fun( not_trimed_value, gain, offset )
	printk("[CH0] TrimData=%d, NotTrimData=%d, data_63_56=0x%x, data_55_48=0x%x\r\n", trim_data, not_trim_data, data_63_56, data_55_48);
	//---------------------------------------------------------------------------------------------------------------
}

///////////////////////////////////////////////////////////////////////////////////
//
//  BUCK TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void UVVP_PMIC_BUCK_TC_6_01_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VPA_CTRL=0;
	kal_uint32 VPA_CTRL_max=1;
	kal_uint32 VPA_VOSEL=0;
	kal_uint32 VPA_VOSEL_max=0x1F;
	kal_uint32 PASEL_value=0;
	kal_uint32 PASEL_value_max=7;
	kal_uint32 PASEL_SETZ=0;
	kal_uint32 PASEL_SETZ_max=0x1F;
	kal_uint32 Ext_AUXADC_value=0;
	upmu_buck_profile_entry *p_upmu_entry = &upmu_buck_profile[(kal_uint32)BUCK_VPA];
	upmu_buck_vol_enum vol_temp=0;
	
	printk("\r\n[UVVP_PMIC_BUCK_TC_6_01_ldvt]\r\n");

	printk("RG_VPA_CTRL,RG_VPA_VOSEL,PAD_Value,RG_PASEL_SETZ,VPA_VOLTAGE\r\n");

	upmu_buck_en(BUCK_VPA,0x1);

	//Change MUX to VPA
	pmu_power_type = BUCK_TYPE;
	pdt_ext_max396_switch(BUCK_VPA);

	for(VPA_CTRL=0 ; VPA_CTRL<=VPA_CTRL_max ; VPA_CTRL++)
	{
		upmu_buck_ctrl(BUCK_VPA,VPA_CTRL);
		
		if( VPA_CTRL==0 )
		{
			for(VPA_VOSEL=0 ; VPA_VOSEL<=VPA_VOSEL_max ; VPA_VOSEL++)			
			{
				vol_temp=p_upmu_entry->vol_list[VPA_VOSEL];
				//vol_temp=UPMU_VOLT_1_0_0_0_V;				
				upmu_buck_vosel(BUCK_VPA, vol_temp);				
				Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPA);				
				printk("%d,%d,%d,%d,%d\r\n", VPA_CTRL, VPA_VOSEL, PASEL_value, PASEL_SETZ, Ext_AUXADC_value);
			}
		}
		else if( VPA_CTRL==1 )
		{
			for(PASEL_value=0 ; PASEL_value<=PASEL_value_max ; PASEL_value++)
			{
				//set PASEL from pad
				if( PASEL_value==0 )
				{	
					Set_PA_SEL(0);
				}
				else if( PASEL_value==7 )
				{
					Set_PA_SEL(7);
				}
				else
				{
					continue;
				}

				if(PASEL_value==0)
				{
					for(PASEL_SETZ=0 ; PASEL_SETZ<=PASEL_SETZ_max ; PASEL_SETZ++)
					//PASEL_SETZ=0x0;
					{						
						upmu_buck_pasel_set_0(BUCK_VPA, PASEL_SETZ);							
						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPA);						
						printk("%d,%d,%d,%d,%d\r\n", VPA_CTRL, VPA_VOSEL, PASEL_value, PASEL_SETZ, Ext_AUXADC_value);
					}
				}
				else if(PASEL_value==1)
				{
					for(PASEL_SETZ=0 ; PASEL_SETZ<=PASEL_SETZ_max ; PASEL_SETZ++)
					//PASEL_SETZ=0x1;
					{						
						upmu_buck_pasel_set_1(BUCK_VPA, PASEL_SETZ);							
						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPA);						
						printk("%d,%d,%d,%d,%d\r\n", VPA_CTRL, VPA_VOSEL, PASEL_value, PASEL_SETZ, Ext_AUXADC_value);
					}
				}
				else if(PASEL_value==2)
				{
					for(PASEL_SETZ=0 ; PASEL_SETZ<=PASEL_SETZ_max ; PASEL_SETZ++)
					//PASEL_SETZ=0x2;
					{						
						upmu_buck_pasel_set_2(BUCK_VPA, PASEL_SETZ);							
						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPA);						
						printk("%d,%d,%d,%d,%d\r\n", VPA_CTRL, VPA_VOSEL, PASEL_value, PASEL_SETZ, Ext_AUXADC_value);
					}
				}
				else if(PASEL_value==3)
				{
					for(PASEL_SETZ=0 ; PASEL_SETZ<=PASEL_SETZ_max ; PASEL_SETZ++)
					//PASEL_SETZ=0x3;
					{						
						upmu_buck_pasel_set_3(BUCK_VPA, PASEL_SETZ);							
						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPA);						
						printk("%d,%d,%d,%d,%d\r\n", VPA_CTRL, VPA_VOSEL, PASEL_value, PASEL_SETZ, Ext_AUXADC_value);
					}
				}
				else if(PASEL_value==4)
				{
					for(PASEL_SETZ=0 ; PASEL_SETZ<=PASEL_SETZ_max ; PASEL_SETZ++)
					//PASEL_SETZ=0x4;
					{						
						upmu_buck_pasel_set_4(BUCK_VPA, PASEL_SETZ);							
						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPA);						
						printk("%d,%d,%d,%d,%d\r\n", VPA_CTRL, VPA_VOSEL, PASEL_value, PASEL_SETZ, Ext_AUXADC_value);
					}
				}
				else if(PASEL_value==5)
				{
					for(PASEL_SETZ=0 ; PASEL_SETZ<=PASEL_SETZ_max ; PASEL_SETZ++)
					//PASEL_SETZ=0x5;
					{						
						upmu_buck_pasel_set_5(BUCK_VPA, PASEL_SETZ);							
						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPA);						
						printk("%d,%d,%d,%d,%d\r\n", VPA_CTRL, VPA_VOSEL, PASEL_value, PASEL_SETZ, Ext_AUXADC_value);
					}
				}
				else if(PASEL_value==6)
				{
					for(PASEL_SETZ=0 ; PASEL_SETZ<=PASEL_SETZ_max ; PASEL_SETZ++)
					//PASEL_SETZ=0x6;
					{						
						upmu_buck_pasel_set_6(BUCK_VPA, PASEL_SETZ);							
						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPA);						
						printk("%d,%d,%d,%d,%d\r\n", VPA_CTRL, VPA_VOSEL, PASEL_value, PASEL_SETZ, Ext_AUXADC_value);
					}
				}
				else if(PASEL_value==7)
				{
					for(PASEL_SETZ=0 ; PASEL_SETZ<=PASEL_SETZ_max ; PASEL_SETZ++)
					//PASEL_SETZ=0x7;
					{						
						upmu_buck_pasel_set_7(BUCK_VPA, PASEL_SETZ);							
						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPA);						
						printk("%d,%d,%d,%d,%d\r\n", VPA_CTRL, VPA_VOSEL, PASEL_value, PASEL_SETZ, Ext_AUXADC_value);
					}
				}
				else
				{
				}
			}

			//reset
			Set_PA_SEL(0);
			
		}
		else
		{
		}
	}
	
}
void UVVP_PMIC_BUCK_TC_6_02_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VRF18_ON_CTRL=0;
	kal_uint32 VRF18_ON_CTRL_max=1;
	kal_uint32 VRF18_EN=0;
	kal_uint32 VRF18_EN_max=1;
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;
	kal_uint32 Ext_AUXADC_value=0;

	printk("\r\n[UVVP_PMIC_BUCK_TC_6_02_ldvt]\r\n");

	printk("RG_VRF18_ON_CTRL,RG_VRF18_EN,SRCLK_EN,VRF18_VOLTAGE\r\n");

	//Change MUX to VRF18
	pmu_power_type = BUCK_TYPE;
	pdt_ext_max396_switch(BUCK_VRF18);

	for(VRF18_ON_CTRL=0 ; VRF18_ON_CTRL<=VRF18_ON_CTRL_max ; VRF18_ON_CTRL++)
	{
		upmu_buck_on_ctrl(BUCK_VRF18,VRF18_ON_CTRL);

		if(VRF18_ON_CTRL==0)
		{
			for(VRF18_EN=0 ; VRF18_EN<=VRF18_EN_max ; VRF18_EN++)
			{
				upmu_buck_en(BUCK_VRF18,VRF18_EN);

				Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VRF18);
				printk("%d,%d,%d,%d\r\n", VRF18_ON_CTRL, VRF18_EN, SRCLK_EN, Ext_AUXADC_value);
			}

			//reset
			upmu_buck_en(BUCK_VRF18, 0);
		}
		else if(VRF18_ON_CTRL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);
				
				Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VRF18);
				printk("%d,%d,%d,%d\r\n", VRF18_ON_CTRL, VRF18_EN, SRCLK_EN, Ext_AUXADC_value);
			}

			//reset
			Set_SRCLK_EN(0);			
		}
		else
		{
		}
	}
	
}
void UVVP_PMIC_BUCK_TC_6_03_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VPROC_CTRL=0;
	kal_uint32 VPROC_CTRL_max=3;
	kal_uint32 VPROC_VOSEL=0;
	kal_uint32 VPROC_VOSEL_max=0x1F;
	kal_uint32 VPROC_VOSEL_SRCLKEN0=0;
	kal_uint32 VPROC_VOSEL_SRCLKEN0_max=0x1F;	
	kal_uint32 VPROC_VOSEL_SRCLKEN1=0;
	kal_uint32 VPROC_VOSEL_SRCLKEN1_max=0x1F;
	kal_uint32 DVS_SELECT=0;
	kal_uint32 DVS_SELECT_max=3;		
	kal_uint32 VPROC_VOSEL_DVS00=0;
	kal_uint32 VPROC_VOSEL_DVS00_max=0x1F;	
	kal_uint32 VPROC_VOSEL_DVS01=0;
	kal_uint32 VPROC_VOSEL_DVS01_max=0x1F;	
	kal_uint32 VPROC_VOSEL_DVS10=0;
	kal_uint32 VPROC_VOSEL_DVS10_max=0x1F;	
	kal_uint32 VPROC_VOSEL_DVS11=0;
	kal_uint32 VPROC_VOSEL_DVS11_max=0x1F;		
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	upmu_buck_profile_entry *p_upmu_entry = &upmu_buck_profile[(kal_uint32)BUCK_VPROC];
	upmu_buck_vol_enum vol_temp=0;
	char read_data;

	printk("\r\n[UVVP_PMIC_BUCK_TC_6_03_ldvt]\r\n");

	printk("VPROC_CTRL,VPROC_VOSEL,VPROC_VOSEL_SRCLKEN0,VPROC_VOSEL_SRCLKEN1,DVS_SELECT,VPROC_VOSEL_DVS00,VPROC_VOSEL_DVS01,VPROC_VOSEL_DVS10,VPROC_VOSEL_DVS11,SRCLK_EN,VPROC_VOLTAGE,Reg[0x4e]\r\n");

	//Change MUX to VPROC
	pmu_power_type = BUCK_TYPE;
	pdt_ext_max396_switch(BUCK_VPROC);

	for(VPROC_CTRL=0 ; VPROC_CTRL<=VPROC_CTRL_max ; VPROC_CTRL++)
	{
		upmu_buck_ctrl(BUCK_VPROC,VPROC_CTRL);

		if(VPROC_CTRL==0)
		{
			for(VPROC_VOSEL=0 ; VPROC_VOSEL<=VPROC_VOSEL_max ; VPROC_VOSEL++)
			{
				vol_temp=p_upmu_entry->vol_list[VPROC_VOSEL];
				//vol_temp=UPMU_VOLT_1_0_0_0_V;				
				upmu_buck_vosel(BUCK_VPROC, vol_temp);

				ret=pmic_read_interface(0x4e,&read_data,0xFF,0x0);

				Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPROC);
				printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
				VPROC_CTRL,VPROC_VOSEL,VPROC_VOSEL_SRCLKEN0,VPROC_VOSEL_SRCLKEN1,DVS_SELECT,
				VPROC_VOSEL_DVS00,VPROC_VOSEL_DVS01,VPROC_VOSEL_DVS10,VPROC_VOSEL_DVS11,SRCLK_EN,Ext_AUXADC_value,read_data);
			}
		}
		else if(VPROC_CTRL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);
				
				if(SRCLK_EN==0)
				{
					for(VPROC_VOSEL_SRCLKEN0=0 ; VPROC_VOSEL_SRCLKEN0<=VPROC_VOSEL_SRCLKEN0_max ; VPROC_VOSEL_SRCLKEN0++)
					{			
						upmu_buck_vosel_srclken_0(BUCK_VPROC, VPROC_VOSEL_SRCLKEN0);

						ret=pmic_read_interface(0x4e,&read_data,0xFF,0x0);

						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPROC);
						printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
						VPROC_CTRL,VPROC_VOSEL,VPROC_VOSEL_SRCLKEN0,VPROC_VOSEL_SRCLKEN1,DVS_SELECT,
						VPROC_VOSEL_DVS00,VPROC_VOSEL_DVS01,VPROC_VOSEL_DVS10,VPROC_VOSEL_DVS11,SRCLK_EN,Ext_AUXADC_value,read_data);
					}
				}
				else if(SRCLK_EN==1)
				{
					
					for(VPROC_VOSEL_SRCLKEN1=0 ; VPROC_VOSEL_SRCLKEN1<=VPROC_VOSEL_SRCLKEN1_max ; VPROC_VOSEL_SRCLKEN1++)
					{			
						upmu_buck_vosel_srclken_1(BUCK_VPROC, VPROC_VOSEL_SRCLKEN1);

						ret=pmic_read_interface(0x4e,&read_data,0xFF,0x0);

						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPROC);
						printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
						VPROC_CTRL,VPROC_VOSEL,VPROC_VOSEL_SRCLKEN0,VPROC_VOSEL_SRCLKEN1,DVS_SELECT,
						VPROC_VOSEL_DVS00,VPROC_VOSEL_DVS01,VPROC_VOSEL_DVS10,VPROC_VOSEL_DVS11,SRCLK_EN,Ext_AUXADC_value,read_data);
					}
				}
				else
				{
				}			
			}

			//reset
			Set_SRCLK_EN(0);

		}
		/*VPROC_CTRL==2 || VPROC_CTRL==3*/
		else
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);
				
				if(SRCLK_EN==0)
				{
					for(VPROC_VOSEL_SRCLKEN0=0 ; VPROC_VOSEL_SRCLKEN0<=VPROC_VOSEL_SRCLKEN0_max ; VPROC_VOSEL_SRCLKEN0++)
					{
						upmu_buck_vosel_srclken_0(BUCK_VPROC, VPROC_VOSEL_SRCLKEN0);

						ret=pmic_read_interface(0x4e,&read_data,0xFF,0x0);

						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPROC);
						printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
						VPROC_CTRL,VPROC_VOSEL,VPROC_VOSEL_SRCLKEN0,VPROC_VOSEL_SRCLKEN1,DVS_SELECT,
						VPROC_VOSEL_DVS00,VPROC_VOSEL_DVS01,VPROC_VOSEL_DVS10,VPROC_VOSEL_DVS11,SRCLK_EN,Ext_AUXADC_value,read_data);
					}
				}
				else if(SRCLK_EN==1)
				{
	
					for(DVS_SELECT=0 ; DVS_SELECT<=DVS_SELECT_max ; DVS_SELECT++)
					{
						//set DVT0/1

						if(DVS_SELECT==0)
						{
							Set_DVS_SEL(0);
						}
						else if(DVS_SELECT==3)
						{
							Set_DVS_SEL(3);
						}
						else
						{
						}
						
						if(DVS_SELECT==0)
						{
							for(VPROC_VOSEL_DVS00=0 ; VPROC_VOSEL_DVS00<=VPROC_VOSEL_DVS00_max ; VPROC_VOSEL_DVS00++)
							{			
								upmu_buck_vosel_dvs_00(BUCK_VPROC,VPROC_VOSEL_DVS00);

								ret=pmic_read_interface(0x4e,&read_data,0xFF,0x0);

								Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPROC);
								printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
								VPROC_CTRL,VPROC_VOSEL,VPROC_VOSEL_SRCLKEN0,VPROC_VOSEL_SRCLKEN1,DVS_SELECT,
								VPROC_VOSEL_DVS00,VPROC_VOSEL_DVS01,VPROC_VOSEL_DVS10,VPROC_VOSEL_DVS11,SRCLK_EN,Ext_AUXADC_value,read_data);
							}
						}
						else if(DVS_SELECT==1)
						{
							for(VPROC_VOSEL_DVS01=0 ; VPROC_VOSEL_DVS01<=VPROC_VOSEL_DVS01_max ; VPROC_VOSEL_DVS01++)
							{			
								upmu_buck_vosel_dvs_01(BUCK_VPROC,VPROC_VOSEL_DVS01);

								ret=pmic_read_interface(0x4e,&read_data,0xFF,0x0);

								Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPROC);
								printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
								VPROC_CTRL,VPROC_VOSEL,VPROC_VOSEL_SRCLKEN0,VPROC_VOSEL_SRCLKEN1,DVS_SELECT,
								VPROC_VOSEL_DVS00,VPROC_VOSEL_DVS01,VPROC_VOSEL_DVS10,VPROC_VOSEL_DVS11,SRCLK_EN,Ext_AUXADC_value,read_data);
							}
						}
						else if(DVS_SELECT==2)
						{
							for(VPROC_VOSEL_DVS10=0 ; VPROC_VOSEL_DVS10<=VPROC_VOSEL_DVS10_max ; VPROC_VOSEL_DVS10++)
							{			
								upmu_buck_vosel_dvs_10(BUCK_VPROC,VPROC_VOSEL_DVS10);

								ret=pmic_read_interface(0x4e,&read_data,0xFF,0x0);

								Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPROC);
								printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
								VPROC_CTRL,VPROC_VOSEL,VPROC_VOSEL_SRCLKEN0,VPROC_VOSEL_SRCLKEN1,DVS_SELECT,
								VPROC_VOSEL_DVS00,VPROC_VOSEL_DVS01,VPROC_VOSEL_DVS10,VPROC_VOSEL_DVS11,SRCLK_EN,Ext_AUXADC_value,read_data);
							}
						}
						else if(DVS_SELECT==3)
						{
							for(VPROC_VOSEL_DVS11=0 ; VPROC_VOSEL_DVS11<=VPROC_VOSEL_DVS11_max ; VPROC_VOSEL_DVS11++)
							{			
								upmu_buck_vosel_dvs_11(BUCK_VPROC,VPROC_VOSEL_DVS11);

								ret=pmic_read_interface(0x4e,&read_data,0xFF,0x0);

								Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VPROC);
								printk("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
								VPROC_CTRL,VPROC_VOSEL,VPROC_VOSEL_SRCLKEN0,VPROC_VOSEL_SRCLKEN1,DVS_SELECT,
								VPROC_VOSEL_DVS00,VPROC_VOSEL_DVS01,VPROC_VOSEL_DVS10,VPROC_VOSEL_DVS11,SRCLK_EN,Ext_AUXADC_value,read_data);
							}
						}
						else 
						{
						}
					}

					//reset
					Set_DVS_SEL(0);
					
				}
				else
				{
				}			
			}

			//reset
			Set_SRCLK_EN(0);
		}
	}
	
}
void UVVP_PMIC_BUCK_TC_6_04_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VCORE_CTRL=0;
	kal_uint32 VCORE_CTRL_max=1;
	kal_uint32 VCORE_VOSEL=0;
	kal_uint32 VCORE_VOSEL_max=0x1F;
	kal_uint32 VCORE_VOSEL_CON1=0;
	kal_uint32 VCORE_VOSEL_CON1_max=0x1F;	
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	upmu_buck_profile_entry *p_upmu_entry = &upmu_buck_profile[(kal_uint32)BUCK_VCORE];
	upmu_buck_vol_enum vol_temp=0;

	printk("\r\n[UVVP_PMIC_BUCK_TC_6_04_ldvt]\r\n");

	printk("VCORE_CTRL,VCORE_VOSEL,VCORE_VOSEL_CON1,SRCLK_EN,VCORE_VOLTAGE\r\n");

	//Change MUX to VCORE
	pmu_power_type = BUCK_TYPE;
	pdt_ext_max396_switch(BUCK_VCORE);

	for(VCORE_CTRL=0 ; VCORE_CTRL<=VCORE_CTRL_max ; VCORE_CTRL++)
	{
		upmu_buck_ctrl(BUCK_VCORE,VCORE_CTRL);
	
		if(VCORE_CTRL==0)
		{
			for(VCORE_VOSEL=0 ; VCORE_VOSEL<=VCORE_VOSEL_max ; VCORE_VOSEL++)
			{
				vol_temp=p_upmu_entry->vol_list[VCORE_VOSEL];
				//vol_temp=UPMU_VOLT_1_0_0_0_V;				
				upmu_buck_vosel(BUCK_VCORE, vol_temp);

				Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VCORE);
				printk("%d,%d,%d,%d,%d\r\n", VCORE_CTRL, VCORE_VOSEL, VCORE_VOSEL_CON1, SRCLK_EN, Ext_AUXADC_value);
			}
		}
		else if(VCORE_CTRL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);
				
				if(SRCLK_EN==0)
				{
					for(VCORE_VOSEL=0 ; VCORE_VOSEL<=VCORE_VOSEL_max ; VCORE_VOSEL++)
					{
						vol_temp=p_upmu_entry->vol_list[VCORE_VOSEL];
						//vol_temp=UPMU_VOLT_1_0_0_0_V;				
						upmu_buck_vosel(BUCK_VCORE, vol_temp);

						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VCORE);
						printk("%d,%d,%d,%d,%d\r\n", VCORE_CTRL, VCORE_VOSEL, VCORE_VOSEL_CON1, SRCLK_EN, Ext_AUXADC_value);
					}
				}
				else if(SRCLK_EN==1)
				{
	
					for(VCORE_VOSEL_CON1=0 ; VCORE_VOSEL_CON1<=VCORE_VOSEL_CON1_max ; VCORE_VOSEL_CON1++)
					{			
						upmu_buck_vosel_con_1(BUCK_VCORE, VCORE_VOSEL_CON1);

						Ext_AUXADC_value=Get_BUCK_AUXADC_Value(BUCK_VCORE);
						printk("%d,%d,%d,%d,%d\r\n", VCORE_CTRL, VCORE_VOSEL, VCORE_VOSEL_CON1, SRCLK_EN, Ext_AUXADC_value);
					}
				}
				else
				{
				}								
			}

			//reset
			Set_SRCLK_EN(0);
		}
		else
		{
		}
	}
	
}

///////////////////////////////////////////////////////////////////////////////////
//
//  DIGLDO TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void UVVP_PMIC_DIGLDO_TC_7_01_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 PMU_RESETB=0;
	kal_uint32 PMU_RESETB_max=1;
	kal_uint32 STRUP_VM12_1_EN=0;
	kal_uint32 STRUP_VM12_1_EN_max=1;
	kal_uint32 VM12_1_EN=0;
	kal_uint32 VM12_1_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;

	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_01_ldvt]\r\n");

	printk("PMU_RESETB,STRUP_VM12_1_EN,VM12_1_EN,VM12_1_VOLTAGE\r\n");

	//Change MUX to VM12_1
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VM12_1);

	for(PMU_RESETB=0 ; PMU_RESETB<=PMU_RESETB_max ; PMU_RESETB++)
	{
		//SW can not set PMU_RESETB

		if(PMU_RESETB==0)
		{
			for(STRUP_VM12_1_EN=0 ; STRUP_VM12_1_EN<=STRUP_VM12_1_EN_max ; STRUP_VM12_1_EN++)
			{
				//SW can not set STRUP_VM12_1_EN

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_1);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VM12_1_EN, VM12_1_EN, Ext_AUXADC_value);
			}
		}
		else if(PMU_RESETB==1)
		{
			for(VM12_1_EN=0 ; VM12_1_EN<=VM12_1_EN_max ; VM12_1_EN++)
			{
				upmu_ldo_en(LDO_VM12_1, VM12_1_EN);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_1);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VM12_1_EN, VM12_1_EN, Ext_AUXADC_value);
			}
		}
		else
		{
		}
	}
	
}
void UVVP_PMIC_DIGLDO_TC_7_02_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VM12_1_LP_SEL=0;
	kal_uint32 VM12_1_LP_SEL_max=1;
	kal_uint32 VM12_1_LP_SET=0;
	kal_uint32 VM12_1_LP_SET_max=1;
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	char read_data;

	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_02_ldvt]\r\n");

	printk("VM12_1_LP_SEL,VM12_1_LP_SET,SRCLK_EN,VM12_1_LP_VOLTAGE,Reg[0x85]\r\n");

	//Change MUX to VM12_1_LP
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VM12_1);

	for(VM12_1_LP_SEL=0 ; VM12_1_LP_SEL<=VM12_1_LP_SEL_max ; VM12_1_LP_SEL++)
	{
		upmu_ldo_lp_sel(LDO_VM12_1, VM12_1_LP_SEL);
	
		if(VM12_1_LP_SEL==0)
		{
			for(VM12_1_LP_SET=0 ; VM12_1_LP_SET<=VM12_1_LP_SET_max ; VM12_1_LP_SET++)
			{
				upmu_ldo_lp_set(LDO_VM12_1, VM12_1_LP_SET);

				ret=pmic_read_interface(0x85,&read_data,0xFF,0x0);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_1);				
				printk("%d,%d,%d,%d,%x\r\n", VM12_1_LP_SEL, VM12_1_LP_SET, SRCLK_EN, Ext_AUXADC_value,read_data);
			}
		}
		else if(VM12_1_LP_SEL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);

				ret=pmic_read_interface(0x85,&read_data,0xFF,0x0);
				
				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_1);				
				printk("%d,%d,%d,%d,%x\r\n", VM12_1_LP_SEL, VM12_1_LP_SET, SRCLK_EN, Ext_AUXADC_value, read_data);
			}

			//reset
			Set_SRCLK_EN(0);
		}
		else
		{
		}
	}
}
void UVVP_PMIC_DIGLDO_TC_7_03_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 PMU_RESETB=0;
	kal_uint32 PMU_RESETB_max=1;
	kal_uint32 STRUP_VM12_2_EN=0;
	kal_uint32 STRUP_VM12_2_EN_max=1;
	kal_uint32 VM12_2_EN=0;
	kal_uint32 VM12_2_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;

	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_03_ldvt]\r\n");

	printk("PMU_RESETB,STRUP_VM12_2_EN,VM12_2_EN,VM12_2_VOLTAGE\r\n");

	//Change MUX to VM12_2
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VM12_2);	

	for(PMU_RESETB=0 ; PMU_RESETB<=PMU_RESETB_max ; PMU_RESETB++)
	{
		//SW can not set PMU_RESETB

		if(PMU_RESETB==0)
		{
			for(STRUP_VM12_2_EN=0 ; STRUP_VM12_2_EN<=STRUP_VM12_2_EN_max ; STRUP_VM12_2_EN++)
			{
				//SW can not set STRUP_VM12_2_EN

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_2);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VM12_2_EN, VM12_2_EN, Ext_AUXADC_value);
			}
		}
		else if(PMU_RESETB==1)
		{
			for(VM12_2_EN=0 ; VM12_2_EN<=VM12_2_EN_max ; VM12_2_EN++)
			{
				upmu_ldo_en(LDO_VM12_2, VM12_2_EN);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_2);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VM12_2_EN, VM12_2_EN, Ext_AUXADC_value);
			}
		}
		else
		{
		}
	}
}
void UVVP_PMIC_DIGLDO_TC_7_04_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VM12_2_LP_SEL=0;
	kal_uint32 VM12_2_LP_SEL_max=1;
	kal_uint32 VM12_2_LP_SET=0;
	kal_uint32 VM12_2_LP_SET_max=1;
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	char read_data;

	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_04_ldvt]\r\n");

	printk("VM12_2_LP_SEL,VM12_2_LP_SET,SRCLK_EN,VM12_2_LP_VOLTAGE,Reg[0x89]\r\n");

	//Change MUX to VM12_2_LP
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VM12_2);		

	for(VM12_2_LP_SEL=0 ; VM12_2_LP_SEL<=VM12_2_LP_SEL_max ; VM12_2_LP_SEL++)
	{
		upmu_ldo_lp_sel(LDO_VM12_2, VM12_2_LP_SEL);
	
		if(VM12_2_LP_SEL==0)
		{
			for(VM12_2_LP_SET=0 ; VM12_2_LP_SET<=VM12_2_LP_SET_max ; VM12_2_LP_SET++)
			{
				upmu_ldo_lp_set(LDO_VM12_2, VM12_2_LP_SET);

				ret=pmic_read_interface(0x89,&read_data,0xFF,0x0);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_2);				
				printk("%d,%d,%d,%d,%x\r\n", VM12_2_LP_SEL, VM12_2_LP_SET, SRCLK_EN, Ext_AUXADC_value,read_data);
			}
		}
		else if(VM12_2_LP_SEL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);

				ret=pmic_read_interface(0x89,&read_data,0xFF,0x0);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_2);				
				printk("%d,%d,%d,%d,%x\r\n", VM12_2_LP_SEL, VM12_2_LP_SET, SRCLK_EN, Ext_AUXADC_value, read_data);
			}

			//reset
			Set_SRCLK_EN(0);
		}
		else
		{
		}
	}
	
}
void UVVP_PMIC_DIGLDO_TC_7_05_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 PMU_RESETB=0;
	kal_uint32 PMU_RESETB_max=1;
	kal_uint32 STRUP_VM12_INT_EN=0;
	kal_uint32 STRUP_VM12_INT_EN_max=1;
	kal_uint32 VM12_INT_EN=0;
	kal_uint32 VM12_INT_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;

	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_05_ldvt]\r\n");

	printk("PMU_RESETB,STRUP_VM12_INT_EN,VM12_INT_EN,VM12_INT_VOLTAGE\r\n");

	//Change MUX to VM12_INT
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VM12_INT);		

	for(PMU_RESETB=0 ; PMU_RESETB<=PMU_RESETB_max ; PMU_RESETB++)
	{
		//SW can not set PMU_RESETB

		if(PMU_RESETB==0)
		{
			for(STRUP_VM12_INT_EN=0 ; STRUP_VM12_INT_EN<=STRUP_VM12_INT_EN_max ; STRUP_VM12_INT_EN++)
			{
				//SW can not set STRUP_VM12_INT_EN

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_INT);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VM12_INT_EN, VM12_INT_EN, Ext_AUXADC_value);
			}
		}
		else if(PMU_RESETB==1)
		{
			for(VM12_INT_EN=0 ; VM12_INT_EN<=VM12_INT_EN_max ; VM12_INT_EN++)
			{
				upmu_ldo_en(LDO_VM12_INT, VM12_INT_EN);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_INT);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VM12_INT_EN, VM12_INT_EN, Ext_AUXADC_value);
			}
		}
		else
		{
		}
	}
}
void UVVP_PMIC_DIGLDO_TC_7_06_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VM12_INT_CTRL_SEL=0;
	kal_uint32 VM12_INT_CTRL_SEL_max=1;
	kal_uint32 VM12_INT_CAL=0;
	kal_uint32 VM12_INT_CAL_max=0x1F;	
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;
	kal_uint32 VM12_INT_SLEEP=0;
	kal_uint32 VM12_INT_SLEEP_max=0x1F;	
	kal_uint32 VPROC_VOSEL=0;
	kal_uint32 VPROC_VOSEL_max=0x1F;
	kal_uint32 VM12_INT_LOW_BOUND=0;
	kal_uint32 VM12_INT_LOW_BOUND_max=0x1F;	
	kal_uint32 Ext_AUXADC_value=0;
	upmu_buck_profile_entry *p_upmu_entry = &upmu_buck_profile[(kal_uint32)BUCK_VPROC];
	upmu_buck_vol_enum vol_temp=0;
	char read_data;

	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_06_ldvt]\r\n");

	printk("VM12_INT_CTRL_SEL,VM12_INT_CAL,SRCLK_EN,VM12_INT_SLEEP,VPROC_VOSEL,VM12_INT_LOW_BOUND,VM12_INT_VOLTAGE,Reg[0xb9]\r\n");

	//Change MUX to VM12_INT
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VM12_INT);	

	for(VM12_INT_CTRL_SEL=0 ; VM12_INT_CTRL_SEL<=VM12_INT_CTRL_SEL_max ; VM12_INT_CTRL_SEL++)
	{
		ret=pmic_config_interface(BANK0_DIGLDO_COND,VM12_INT_CTRL_SEL,
			BANK_0_VM12_INT_CTRL_SEL_MASK,BANK_0_VM12_INT_CTRL_SEL_SHIFT);

		if(VM12_INT_CTRL_SEL==0)
		{
			for(VM12_INT_CAL=0 ; VM12_INT_CAL<=VM12_INT_CAL_max ; VM12_INT_CAL++)
			{
				upmu_ldo_cal(LDO_VM12_INT, VM12_INT_CAL);

				ret=pmic_read_interface( 0xb9,&read_data,0xFF,0x0);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_INT);				
				printk("%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
					VM12_INT_CTRL_SEL,VM12_INT_CAL,SRCLK_EN,VM12_INT_SLEEP,VPROC_VOSEL,VM12_INT_LOW_BOUND,Ext_AUXADC_value,read_data);
			}
		}
		else if(VM12_INT_CTRL_SEL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);
				
				if(SRCLK_EN==0)
				{
					for(VM12_INT_SLEEP=0 ; VM12_INT_SLEEP<=VM12_INT_SLEEP_max ; VM12_INT_SLEEP++)
					{
						ret=pmic_config_interface(BANK0_DIGLDO_CON9,VM12_INT_SLEEP,
							BANK_0_VM12_INT_SLEEP_MASK,BANK_0_VM12_INT_SLEEP_SHIFT);

						ret=pmic_read_interface( 0xb9,&read_data,0xFF,0x0);
						
						Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_INT);				
						printk("%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
							VM12_INT_CTRL_SEL,VM12_INT_CAL,SRCLK_EN,VM12_INT_SLEEP,VPROC_VOSEL,VM12_INT_LOW_BOUND,Ext_AUXADC_value,read_data);
					}
				}
				else if(SRCLK_EN==1)
				{
					for(VPROC_VOSEL=0 ; VPROC_VOSEL<=VPROC_VOSEL_max ; VPROC_VOSEL++)
					{
						vol_temp=p_upmu_entry->vol_list[VPROC_VOSEL];
						//vol_temp=UPMU_VOLT_1_0_0_0_V;				
						upmu_buck_vosel(BUCK_VPROC, vol_temp);
						
						for(VM12_INT_LOW_BOUND=0 ; VM12_INT_LOW_BOUND<=VM12_INT_LOW_BOUND_max ; VM12_INT_LOW_BOUND++)
						{
							ret=pmic_config_interface(BANK0_DIGLDO_CONA,VM12_INT_LOW_BOUND,
								BANK_0_VM12_INT_LOW_BOUND_MASK,BANK_0_VM12_INT_LOW_BOUND_SHIFT);

							ret=pmic_read_interface( 0xb9,&read_data,0xFF,0x0);
						
							Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VM12_INT);				
							printk("%d,%d,%d,%d,%d,%d,%d,%x\r\n", 
								VM12_INT_CTRL_SEL,VM12_INT_CAL,SRCLK_EN,VM12_INT_SLEEP,VPROC_VOSEL,VM12_INT_LOW_BOUND,Ext_AUXADC_value,read_data);
						}
					}
				}
				else
				{
				}				
				
			}

			//reset
			Set_SRCLK_EN(0);
		}
		else
		{
		}
	}
	
}
void UVVP_PMIC_DIGLDO_TC_7_07_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 PMU_RESETB=0;
	kal_uint32 PMU_RESETB_max=1;
	kal_uint32 STRUP_VIO28_EN=0;
	kal_uint32 STRUP_VIO28_EN_max=1;
	kal_uint32 VIO28_EN=0;
	kal_uint32 VIO28_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;

	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_07_ldvt]\r\n");

	printk("PMU_RESETB,STRUP_VIO28_EN,VIO28_EN,VIO28_VOLTAGE\r\n");

	//Change MUX to VIO28
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VIO28);	

	for(PMU_RESETB=0 ; PMU_RESETB<=PMU_RESETB_max ; PMU_RESETB++)
	{
		//SW can not set PMU_RESETB

		if(PMU_RESETB==0)
		{
			for(STRUP_VIO28_EN=0 ; STRUP_VIO28_EN<=STRUP_VIO28_EN_max ; STRUP_VIO28_EN++)
			{
				//SW can not set STRUP_VIO28_EN

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VIO28);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VIO28_EN, VIO28_EN, Ext_AUXADC_value);
			}
		}
		else if(PMU_RESETB==1)
		{
			for(VIO28_EN=0 ; VIO28_EN<=VIO28_EN_max ; VIO28_EN++)
			{
				upmu_ldo_en(LDO_VIO28, VIO28_EN);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VIO28);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VIO28_EN, VIO28_EN, Ext_AUXADC_value);
			}
		}
		else
		{
		}
	}
}
void UVVP_PMIC_DIGLDO_TC_7_08_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 PMU_RESETB=0;
	kal_uint32 PMU_RESETB_max=1;
	kal_uint32 STRUP_VMC_EN=0;
	kal_uint32 STRUP_VMC_EN_max=1;
	kal_uint32 VMC_EN=0;
	kal_uint32 VMC_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	
	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_08_ldvt]\r\n"); 

	printk("PMU_RESETB,STRUP_VMC_EN,VMC_EN,VMC_VOLTAGE\r\n");

	//Change MUX to VMC
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VMC);	

	for(PMU_RESETB=0 ; PMU_RESETB<=PMU_RESETB_max ; PMU_RESETB++)
	{
		//SW can not set PMU_RESETB

		if(PMU_RESETB==0)
		{
			for(STRUP_VMC_EN=0 ; STRUP_VMC_EN<=STRUP_VMC_EN_max ; STRUP_VMC_EN++)
			{
				//SW can not set STRUP_VMC_EN

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VMC);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VMC_EN, VMC_EN, Ext_AUXADC_value);
			}
		}
		else if(PMU_RESETB==1)
		{
			for(VMC_EN=0 ; VMC_EN<=VMC_EN_max ; VMC_EN++)
			{
				upmu_ldo_en(LDO_VMC, VMC_EN);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VMC);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VMC_EN, VMC_EN, Ext_AUXADC_value);
			}
		}
		else
		{
		}
	}	
}
void UVVP_PMIC_DIGLDO_TC_7_09_ldvt(void)
{
	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_09_ldvt]\r\n");

	printk("Waveform with HQA/DE\r\n");
}
void UVVP_PMIC_DIGLDO_TC_7_10_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VMC_LP_SEL=0;
	kal_uint32 VMC_LP_SEL_max=1;
	kal_uint32 VMC_LP_MODE_SET=0;
	kal_uint32 VMC_LP_MODE_SET_max=1;
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	char read_data;

	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_10_ldvt]\r\n");

	printk("VMC_LP_SEL,VMC_LP_MODE_SET,SRCLK_EN,VMC_LP_VOLTAGE,Reg[0xa9]\r\n");

	//Change MUX to VMC
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VMC);	

	for(VMC_LP_SEL=0 ; VMC_LP_SEL<=VMC_LP_SEL_max ; VMC_LP_SEL++)
	{
		upmu_ldo_lp_sel(LDO_VMC, VMC_LP_SEL);
	
		if(VMC_LP_SEL==0)
		{
			for(VMC_LP_MODE_SET=0 ; VMC_LP_MODE_SET<=VMC_LP_MODE_SET_max ; VMC_LP_MODE_SET++)
			{
				upmu_ldo_lp_set(LDO_VMC, VMC_LP_MODE_SET);

				ret=pmic_read_interface(0xa9,&read_data,0xFF,0x0);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VMC);				
				printk("%d,%d,%d,%d,%x\r\n", VMC_LP_SEL, VMC_LP_MODE_SET, SRCLK_EN, Ext_AUXADC_value,read_data);
			}
		}
		else if(VMC_LP_SEL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);

				ret=pmic_read_interface(0xa9,&read_data,0xFF,0x0);
				
				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VMC);				
				printk("%d,%d,%d,%d,%x\r\n", VMC_LP_SEL, VMC_LP_MODE_SET, SRCLK_EN, Ext_AUXADC_value,read_data);
			}

			//reset
			Set_SRCLK_EN(0);
		}
		else
		{
		}
	}	
}
void UVVP_PMIC_DIGLDO_TC_7_11_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 PMU_RESETB=0;
	kal_uint32 PMU_RESETB_max=1;
	kal_uint32 STRUP_VMCH_EN=0;
	kal_uint32 STRUP_VMCH_EN_max=1;
	kal_uint32 VMCH_EN=0;
	kal_uint32 VMCH_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;

	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_11_ldvt]\r\n");

	printk("PMU_RESETB,STRUP_VMCH_EN,VMCH_EN,VMCH_VOLTAGE\r\n");

	//Change MUX to VMCH
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VMCH);	

	for(PMU_RESETB=0 ; PMU_RESETB<=PMU_RESETB_max ; PMU_RESETB++)
	{
		//SW can not set PMU_RESETB

		if(PMU_RESETB==0)
		{
			for(STRUP_VMCH_EN=0 ; STRUP_VMCH_EN<=STRUP_VMCH_EN_max ; STRUP_VMCH_EN++)
			{
				//SW can not set STRUP_VMC_EN

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VMCH);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VMCH_EN, VMCH_EN, Ext_AUXADC_value);
			}
		}
		else if(PMU_RESETB==1)
		{
			for(VMCH_EN=0 ; VMCH_EN<=VMCH_EN_max ; VMCH_EN++)
			{
				upmu_ldo_en(LDO_VMCH, VMCH_EN);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VMCH);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VMCH_EN, VMCH_EN, Ext_AUXADC_value);
			}
		}
		else
		{
		}
	}	
}
void UVVP_PMIC_DIGLDO_TC_7_12_ldvt(void)
{
	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_12_ldvt]\r\n");
	
	printk("Waveform with HQA/DE\r\n");	
}
void UVVP_PMIC_DIGLDO_TC_7_13_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VMCH_LP_SEL=0;
	kal_uint32 VMCH_LP_SEL_max=1;
	kal_uint32 VMCH_LP_MODE_SET=0;
	kal_uint32 VMCH_LP_MODE_SET_max=1;
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	char read_data;
	
	printk("\r\n[UVVP_PMIC_DIGLDO_TC_7_13_ldvt]\r\n");

	printk("VMCH_LP_SEL,VMCH_LP_MODE_SET,SRCLK_EN,VMCH_LP_VOLTAGE,Reg[0xad]\r\n");

	//Change MUX to VMCH
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VMCH);		
	
	for(VMCH_LP_SEL=0 ; VMCH_LP_SEL<=VMCH_LP_SEL_max ; VMCH_LP_SEL++)
	{
		upmu_ldo_lp_sel(LDO_VMCH, VMCH_LP_SEL);
	
		if(VMCH_LP_SEL==0)
		{
			for(VMCH_LP_MODE_SET=0 ; VMCH_LP_MODE_SET<=VMCH_LP_MODE_SET_max ; VMCH_LP_MODE_SET++)
			{
				upmu_ldo_lp_set(LDO_VMCH, VMCH_LP_MODE_SET);

				ret=pmic_read_interface(0xad,&read_data,0xFF,0x0);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VMCH);				
				printk("%d,%d,%d,%d,%x\r\n", VMCH_LP_SEL, VMCH_LP_MODE_SET, SRCLK_EN, Ext_AUXADC_value,read_data);
			}
		}
		else if(VMCH_LP_SEL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);

				ret=pmic_read_interface(0xad,&read_data,0xFF,0x0);
				
				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VMCH);				
				printk("%d,%d,%d,%d,%x\r\n", VMCH_LP_SEL, VMCH_LP_MODE_SET, SRCLK_EN, Ext_AUXADC_value,read_data);
			}

			//reset
			Set_SRCLK_EN(0);
		}
		else
		{
		}
	}		
}

///////////////////////////////////////////////////////////////////////////////////
//
//  ANALDO TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void UVVP_PMIC_ANALDO_TC_8_01_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VRF_ON_CTRL=0;
	kal_uint32 VRF_ON_CTRL_max=1;
	kal_uint32 VRF_EN=0;
	kal_uint32 VRF_EN_max=1;
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	char read_data;

	printk("\r\n[UVVP_PMIC_ANALDO_TC_8_01_ldvt]\r\n");

	printk("VRF_ON_CTRL,VRF_EN,SRCLK_EN,VRF_VOLTAGE,Reg[0xbe]\r\n");

	//Change MUX to VRF
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VRF);	

	for(VRF_ON_CTRL=0 ; VRF_ON_CTRL<=VRF_ON_CTRL_max ; VRF_ON_CTRL++)
	{
		upmu_ldo_on_ctrl(LDO_VRF, VRF_ON_CTRL);
	
		if(VRF_ON_CTRL==0)
		{
			for(VRF_EN=0 ; VRF_EN<=VRF_EN_max ; VRF_EN++)
			{
				upmu_ldo_en(LDO_VRF, VRF_EN);

				ret=pmic_read_interface(0xbe,&read_data,0xFF,0x0);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VRF);				
				printk("%d,%d,%d,%d,%d\r\n", VRF_ON_CTRL, VRF_EN, SRCLK_EN, Ext_AUXADC_value, read_data);
			}
		}
		else if(VRF_ON_CTRL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);

				ret=pmic_read_interface(0xbe,&read_data,0xFF,0x0);
				
				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VRF);				
				printk("%d,%d,%d,%d,%d\r\n", VRF_ON_CTRL, VRF_EN, SRCLK_EN, Ext_AUXADC_value, read_data);
			}

			//reset
			Set_SRCLK_EN(0);
		}
		else
		{
		}
	}		
}
void UVVP_PMIC_ANALDO_TC_8_02_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 PMU_RESETB=0;
	kal_uint32 PMU_RESETB_max=1;
	kal_uint32 STRUP_VTCXO_EN=0;
	kal_uint32 STRUP_VTCXO_EN_max=1;
	kal_uint32 VTCXO_ON_CTRL=0;
	kal_uint32 VTCXO_ON_CTRL_max=1;
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;	
	kal_uint32 VTCXO_EN=0;
	kal_uint32 VTCXO_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;

	printk("\r\n[UVVP_PMIC_ANALDO_TC_8_02_ldvt]\r\n");

	printk("PMU_RESETB,STRUP_VTCXO_EN,VTCXO_ON_CTRL,SRCLK_EN,VTCXO_EN,VTCXO_VOLTAGE\r\n");

	//Change MUX to VTCXO
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VTCXO);		

	for(PMU_RESETB=0 ; PMU_RESETB<=PMU_RESETB_max ; PMU_RESETB++)
	{
		//SW can not set PMU_RESETB

		if(PMU_RESETB==0)
		{
			for(STRUP_VTCXO_EN=0 ; STRUP_VTCXO_EN<=STRUP_VTCXO_EN_max ; STRUP_VTCXO_EN++)
			{
				//SW can not set STRUP_VTCXO_EN

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VTCXO);				
				printk("%d,%d,%d,%d,%d,%d\r\n", PMU_RESETB,STRUP_VTCXO_EN,VTCXO_ON_CTRL,SRCLK_EN,VTCXO_EN,Ext_AUXADC_value);
			}
		}
		else if(PMU_RESETB==1)
		{
			for(VTCXO_ON_CTRL=0 ; VTCXO_ON_CTRL<=VTCXO_ON_CTRL_max ; VTCXO_ON_CTRL++)
			{
				upmu_ldo_on_ctrl(LDO_VTCXO, VTCXO_ON_CTRL);
			
				if(VTCXO_ON_CTRL==0)
				{
					for(VTCXO_EN=0 ; VTCXO_EN<=VTCXO_EN_max ; VTCXO_EN++)
					{
						upmu_ldo_en(LDO_VTCXO, VTCXO_EN);
			
						Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VTCXO);				
						printk("%d,%d,%d,%d,%d,%d\r\n", PMU_RESETB,STRUP_VTCXO_EN,VTCXO_ON_CTRL,SRCLK_EN,VTCXO_EN,Ext_AUXADC_value);
					}
				}
				else if(VTCXO_ON_CTRL==1)
				{
					for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
					{
						Set_SRCLK_EN(SRCLK_EN);
						
						Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VTCXO);				
						printk("%d,%d,%d,%d,%d,%d\r\n", PMU_RESETB,STRUP_VTCXO_EN,VTCXO_ON_CTRL,SRCLK_EN,VTCXO_EN,Ext_AUXADC_value);
					}
			
					//reset
					Set_SRCLK_EN(0);
				}
				else
				{
				}
			}	

		}
		else
		{
		}
	}		
}
void UVVP_PMIC_ANALDO_TC_8_03_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 PMU_RESETB=0;
	kal_uint32 PMU_RESETB_max=1;
	kal_uint32 STRUP_VA1_EN=0;
	kal_uint32 STRUP_VA1_EN_max=1;
	kal_uint32 VA1_EN=0;
	kal_uint32 VA1_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;

	printk("\r\n[UVVP_PMIC_ANALDO_TC_8_03_ldvt]\r\n");

	printk("PMU_RESETB,STRUP_VA1_EN,VA1_EN,VA1_VOLTAGE\r\n");

	//Change MUX to VA1
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VA1);	

	for(PMU_RESETB=0 ; PMU_RESETB<=PMU_RESETB_max ; PMU_RESETB++)
	{
		//SW can not set PMU_RESETB

		if(PMU_RESETB==0)
		{
			for(STRUP_VA1_EN=0 ; STRUP_VA1_EN<=STRUP_VA1_EN_max ; STRUP_VA1_EN++)
			{
				//SW can not set STRUP_VA1_EN

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VA1);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VA1_EN, VA1_EN, Ext_AUXADC_value);
			}
		}
		else if(PMU_RESETB==1)
		{
			for(VA1_EN=0 ; VA1_EN<=VA1_EN_max ; VA1_EN++)
			{
				upmu_ldo_en(LDO_VA1, VA1_EN);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VA1);				
				printk("%d,%d,%d,%d\r\n", PMU_RESETB, STRUP_VA1_EN, VA1_EN, Ext_AUXADC_value);
			}
		}
		else
		{
		}
	}	
}
void UVVP_PMIC_ANALDO_TC_8_04_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VA1_LP_SEL=0;
	kal_uint32 VA1_LP_SEL_max=1;
	kal_uint32 VA1_LP_SET=0;
	kal_uint32 VA1_LP_SET_max=1;
	kal_uint32 SRCLK_EN=0;
	kal_uint32 SRCLK_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	char read_data;

	printk("\r\n[UVVP_PMIC_ANALDO_TC_8_04_ldvt]\r\n");

	printk("VA1_LP_SEL,VA1_LP_SET,SRCLK_EN,VA1_LP_VOLTAGE,Reg[0xc6]\r\n");

	//Change MUX to VA1_LP
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VA1);		
	
	for(VA1_LP_SEL=0 ; VA1_LP_SEL<=VA1_LP_SEL_max ; VA1_LP_SEL++)
	{
		upmu_ldo_lp_sel(LDO_VA1, VA1_LP_SEL);
	
		if(VA1_LP_SEL==0)
		{
			for(VA1_LP_SET=0 ; VA1_LP_SET<=VA1_LP_SET_max ; VA1_LP_SET++)
			{
				upmu_ldo_lp_set(LDO_VA1, VA1_LP_SET);

				ret=pmic_read_interface(0xc6,&read_data,0xFF,0x0);

				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VA1);				
				printk("%d,%d,%d,%d,%x\r\n", VA1_LP_SEL, VA1_LP_SET, SRCLK_EN, Ext_AUXADC_value, read_data);
			}
		}
		else if(VA1_LP_SEL==1)
		{
			for(SRCLK_EN=0 ; SRCLK_EN<=SRCLK_EN_max ; SRCLK_EN++)
			{
				Set_SRCLK_EN(SRCLK_EN);

				ret=pmic_read_interface(0xc6,&read_data,0xFF,0x0);
				
				Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VA1);				
				printk("%d,%d,%d,%d,%x\r\n", VA1_LP_SEL, VA1_LP_SET, SRCLK_EN, Ext_AUXADC_value, read_data);
			}

			//reset
			Set_SRCLK_EN(0);
		}
		else
		{
		}
	}	
}
void UVVP_PMIC_ANALDO_TC_8_05_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 VRTC_EN=0;
	kal_uint32 VRTC_EN_max=1;	
	kal_uint32 Ext_AUXADC_value=0;
	char read_data;

	printk("\r\n[UVVP_PMIC_ANALDO_TC_8_05_ldvt]\r\n");

	printk("VRTC_EN,VRTC_VOLTAGE,Reg[0xcf]\r\n");

	//Change MUX to VRTC
	pmu_power_type = LDO_TYPE;
	pdt_ext_max396_switch(LDO_VRTC);	

	for(VRTC_EN=0 ; VRTC_EN<=VRTC_EN_max ; VRTC_EN++)
	{
		upmu_ldo_en(LDO_VRTC, VRTC_EN);

		ret=pmic_read_interface(0xcf,&read_data,0xFF,0x0);

		Ext_AUXADC_value=Get_LDO_AUXADC_Value(LDO_VRTC);				
		printk("%d,%d,%d\r\n", VRTC_EN, Ext_AUXADC_value, read_data);
	}
}

///////////////////////////////////////////////////////////////////////////////////
//
//  I2C TEST CASE
//
///////////////////////////////////////////////////////////////////////////////////
void UVVP_PMIC_I2C_TC_9_01_ldvt(void)
{
	kal_uint32 ret=0;
	kal_uint32 temp_val=0;

	printk("\r\n[UVVP_PMIC_I2C_TC_9_01_ldvt]\r\n");

	//For BANK 0
	//write 0x5A to INT_CON0
	ret=pmic_config_interface(BANK0_INT_CON0, 0x5A, 0xFF, 0x0);
	//read INT_CON0 
	ret=pmic_read_interface(BANK0_INT_CON0, &temp_val, 0xFF, 0x0);	
	printk("[Bank0] Reg=%x, WriteData=%x, ReadBackDate=%x\r\n", BANK0_INT_CON0, 0x5A, temp_val);
	
	//For BANK 1
	//write 0xA5 to FLASH_CON0
	ret=pmic_bank1_config_interface(BANK1_FLASH_CON0, 0xA5, 0xFF, 0x0);
	//read INT_CON0 
	ret=pmic_bank1_read_interface(BANK1_FLASH_CON0, &temp_val, 0xFF, 0x0);	
	printk("[Bank1] Reg=%x, WriteData=%x, ReadBackDate=%x\r\n", BANK1_FLASH_CON0, 0xA5, temp_val);	
	
}
void UVVP_PMIC_I2C_TC_9_02_ldvt(void)
{
	kal_uint32 ret=0;	
	
	int len=3;
	char cmd[3] = {BANK0_INT_CON0, BANK0_INT_CON1, BANK0_INT_CON2};
	char write_data[3] = {0x05, 0x0A, 0x30};
	kal_uint32 temp_val_1=0;
	kal_uint32 temp_val_2=0;
	kal_uint32 temp_val_3=0;
	kal_uint32 temp_val_4=0;	
	
	int len_bank1=4;
	char cmd_bank1[4] = {BANK1_FLASH_CON0, BANK1_FLASH_CON1, BANK1_FLASH_CON2, BANK1_FLASH_CON3};
	char write_data_bank1[4] = {0x50, 0x03, 0x05, 0x0A};
	
	printk("\r\n[UVVP_PMIC_I2C_TC_9_02_ldvt]\r\n");

	//For BANK 0
	//write 0x05, 0x0A, 0x50, 0xA0 to INT_CON0~3, but no INT_CON3
	mt6329_multi_write_byte(cmd, write_data, len);
	//read one by one
	ret=pmic_read_interface(BANK0_INT_CON0, &temp_val_1, 0xFF, 0x0);
	ret=pmic_read_interface(BANK0_INT_CON1, &temp_val_2, 0xFF, 0x0);
	ret=pmic_read_interface(BANK0_INT_CON2, &temp_val_3, 0xFF, 0x0);
	printk("[Bank0] Reg=%x|%x|%x, WriteData=%x|%x|%x, ReadBackDate=%x|%x|%x\r\n", 
		BANK0_INT_CON0, BANK0_INT_CON1, BANK0_INT_CON2, 
		0x05, 0x0A, 0x30,
		temp_val_1, temp_val_2, temp_val_3);
	
	//For BANK 1
	//write 0x50, 0xA0, 0x05, 0x0A to FLASH_CON0~3
	mt6329_bank1_multi_write_byte(cmd_bank1, write_data_bank1, len_bank1);
	//read one by one
	ret=pmic_bank1_read_interface(BANK1_FLASH_CON0, &temp_val_1, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(BANK1_FLASH_CON1, &temp_val_2, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(BANK1_FLASH_CON2, &temp_val_3, 0xFF, 0x0);
	ret=pmic_bank1_read_interface(BANK1_FLASH_CON3, &temp_val_4, 0xFF, 0x0);
	printk("[Bank1] Reg=%x|%x|%x|%x, WriteData=%x|%x|%x|%x, ReadBackDate=%x|%x|%x|%x\r\n", 
		BANK1_FLASH_CON0, BANK1_FLASH_CON1, BANK1_FLASH_CON2, BANK1_FLASH_CON3, 
		0x50, 0x03, 0x05, 0x0A,
		temp_val_1, temp_val_2, temp_val_3, temp_val_4);
	
}
void UVVP_PMIC_I2C_TC_9_03_ldvt(void)
{
	kal_uint32 ret=0;
	
	int len=3;
	char write_data[3] = {0x05, 0x0A, 0x30};
	char read_data[3] = {0xFF, 0xFF, 0xFF};

	int len_bank1=4;
	char write_data_bank1[4] = {0x50, 0x03, 0x05, 0x0A};	
	char read_data_bank1[4] = {0xFF, 0xFF, 0xFF, 0xFF};
	
	printk("\r\n[UVVP_PMIC_I2C_TC_9_03_ldvt]\r\n");

	//For BANK 0
	//write one by one
	ret=pmic_config_interface(BANK0_INT_CON0, write_data[0], 0xFF, 0x0);
	ret=pmic_config_interface(BANK0_INT_CON1, write_data[1], 0xFF, 0x0);
	ret=pmic_config_interface(BANK0_INT_CON2, write_data[2], 0xFF, 0x0);
	//read all back
	ret=mt6329_multi_read_byte(BANK0_INT_CON0, read_data, len);
	printk("[Bank0] Reg=%x|%x|%x, WriteData=%x|%x|%x, ReadBackDate=%x|%x|%x\r\n", 
		BANK0_INT_CON0, BANK0_INT_CON1, BANK0_INT_CON2, 
		0x05, 0x0A, 0x30,
		read_data[0], read_data[1], read_data[2]);
	
	//For BANK 1
	//write one by one
	ret=pmic_bank1_config_interface(BANK1_FLASH_CON0, write_data_bank1[0], 0xFF, 0x0);
	ret=pmic_bank1_config_interface(BANK1_FLASH_CON1, write_data_bank1[1], 0xFF, 0x0);
	ret=pmic_bank1_config_interface(BANK1_FLASH_CON2, write_data_bank1[2], 0xFF, 0x0);
	ret=pmic_bank1_config_interface(BANK1_FLASH_CON3, write_data_bank1[3], 0xFF, 0x0);	
	//read all back
	ret=mt6329_bank1_multi_read_byte(BANK1_FLASH_CON0, read_data_bank1, len_bank1);
	printk("[Bank1] Reg=%x|%x|%x|%x, WriteData=%x|%x|%x|%x, ReadBackDate=%x|%x|%x|%x\r\n", 
		BANK1_FLASH_CON0, BANK1_FLASH_CON1, BANK1_FLASH_CON2, BANK1_FLASH_CON3, 
		0x50, 0x03, 0x05, 0x0A,
		read_data_bank1[0], read_data_bank1[1], read_data_bank1[2], read_data_bank1[3]);
	
}

void UVVP_PMIC_EXT_BOARD_SWITCH_GETADC_ldvt(void)
{
	int i=0;
	int m=0;
	int buckNumber=0;
	int ldoNumber=0;
	int delay=0;
	kal_uint16 adc_voltage=0;

	printk("[UVVP_PMIC_EXT_BOARD_SWITCH_GETADC_ldvt]\n");
	
	pmu_power_type = BUCK_TYPE;

	for(i=buckNumber; i< BUCK_MAX_NUMBER; i++,buckNumber++)
	{
		pdt_ext_max396_switch(buckNumber);	//switch max396 to make capacitor charge

		//pdt_ext_resistor_switch_set(0);		//set to No loading before swtich voltage before OC occur
		//resistor = pdt_resistor_setup(buckNumber,buck_voltage,k);
		//pdt_ext_resistor_switch_set(resistor);
		//pdt_ext_MOS_switch_set(buckNumber,KAL_TRUE);

		if(pmu_dvt_buck_profile[buckNumber].channel < 16)
		{
			for(m=0;m<8;m++)
			{
				for(delay=0;delay<20000000;delay++);		//delay to make the ADC stable
				adc_voltage = pdt_spi_AdcRead(0);
			}			
		}
		else
		{
			for(m=0;m<8;m++)
			{
				for(delay=0;delay<20000000;delay++);		//delay to make the ADC stable
				adc_voltage = pdt_spi_AdcRead(1);
			}
		}

		printk("pmu_power_type=%d, buckNumber=%d, channel=%d, pin=%d, adc_voltage=%d\n", 
			pmu_power_type, buckNumber, pmu_dvt_buck_profile[buckNumber].channel, 
			(pmu_dvt_buck_profile[buckNumber].channel)+1, adc_voltage);
	}

	pmu_power_type = LDO_TYPE;

	for(i=ldoNumber; i< LDO_MAX_NUMBER; i++,ldoNumber++)
	{
		pdt_ext_max396_switch(ldoNumber);	//switch max396 to make capacitor charge

		//pdt_ext_resistor_switch_set(0);		//set to No loading before swtich voltage before OC occur
		//resistor = pdt_resistor_setup(buckNumber,buck_voltage,k);
		//pdt_ext_resistor_switch_set(resistor);
		//pdt_ext_MOS_switch_set(ldoNumber,KAL_TRUE);

		if(pmu_dvt_ldo_profile[ldoNumber].channel < 16)
		{
			for(m=0;m<8;m++)
			{
				for(delay=0;delay<20000000;delay++);		//delay to make the ADC stable
				adc_voltage = pdt_spi_AdcRead(0);
			}			
		}
		else
		{
			for(m=0;m<8;m++)
			{
				for(delay=0;delay<20000000;delay++);		//delay to make the ADC stable
				adc_voltage = pdt_spi_AdcRead(1);
			}
		}

		printk("pmu_power_type=%d, ldoNumber=%d, channel=%d, pin=%d, adc_voltage=%d\n", 
			pmu_power_type, ldoNumber, pmu_dvt_ldo_profile[ldoNumber].channel,
			(pmu_dvt_ldo_profile[ldoNumber].channel)+1, adc_voltage);
	}
}

void print_raw_data(int buckldoNumber)
{
	int m=0;
	kal_uint32 delay=0;
	kal_uint16 adc_voltage=0;

	if(pmu_power_type == BUCK_TYPE)	
	{	
		if(pmu_dvt_buck_profile[buckldoNumber].channel < 16)
		{
			for(m=0;m<8;m++)
			{
				for(delay=0;delay<2000000000;delay++);		//delay to make the ADC stable
				adc_voltage = pdt_spi_AdcRead(0);
			}			
		}
		else
		{
			for(m=0;m<8;m++)
			{
				for(delay=0;delay<2000000000;delay++);		//delay to make the ADC stable
				adc_voltage = pdt_spi_AdcRead(1);
			}
		}
		printk("%d,%s,%d,%d,%d\n", 
			pmu_power_type, buck_name[buckldoNumber], pmu_dvt_buck_profile[buckldoNumber].channel, 
			(pmu_dvt_buck_profile[buckldoNumber].channel)+1, adc_voltage);
	}
	else if(pmu_power_type == LDO_TYPE)
	{
		if(pmu_dvt_ldo_profile[buckldoNumber].channel < 16)
		{
			for(m=0;m<8;m++)
			{
				for(delay=0;delay<2000000000;delay++);		//delay to make the ADC stable
				adc_voltage = pdt_spi_AdcRead(0);
			}			
		}
		else
		{
			for(m=0;m<8;m++)
			{
				for(delay=0;delay<2000000000;delay++);		//delay to make the ADC stable
				adc_voltage = pdt_spi_AdcRead(1);
			}
		}
		printk("%d,%s,%d,%d,%d\n", 
			pmu_power_type, ldo_name[buckldoNumber], pmu_dvt_ldo_profile[buckldoNumber].channel,
			(pmu_dvt_ldo_profile[buckldoNumber].channel)+1, adc_voltage);
	}
	else
	{
	}
}

void UVVP_PMIC_BUCK_VOLTAGE_ldvt(void)
{
	int i=0;	
	int buckNumber=0;
	int delay=0;	
	int j=0;
	int vol=0;

	printk("[UVVP_PMIC_BUCK_VOLTAGE_ldvt]\n");

	printk("pmu_power_type,name,channel,pin,adc_voltage\n");
	
	pmu_power_type = BUCK_TYPE;

	for(i=buckNumber; i< BUCK_MAX_NUMBER; i++,buckNumber++)
	{
		pdt_ext_max396_switch(buckNumber);	//switch max396 to make capacitor charge

		upmu_buck_en(buckNumber, 1);

		if( (buckNumber==BUCK_VPROC) || (buckNumber==BUCK_VCORE) )
		{	
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_7_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_7_2_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_7_5_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_7_7_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_8_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_8_2_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_8_5_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_8_7_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_9_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_9_2_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_9_5_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_9_7_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_0_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_0_2_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_0_5_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_0_7_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_1_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_1_2_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_1_5_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_1_7_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_2_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_2_2_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_2_5_0_V);
			print_raw_data(buckNumber);			
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_2_7_5_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(buckNumber);	
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(buckNumber);	
		}
		else if( (buckNumber==BUCK_VRF18) || (buckNumber==BUCK_VIO18) )
		{
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_5_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_5_2_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_5_4_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_5_6_0_V);
			print_raw_data(buckNumber);			
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_5_8_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_6_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_6_2_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_6_4_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_6_6_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_6_8_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_7_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_7_2_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_7_4_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_7_6_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_7_8_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_8_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_8_2_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_8_4_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_8_6_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_8_8_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_9_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_9_2_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_9_4_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_9_6_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_9_8_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_0_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_0_2_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_0_4_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_0_6_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_0_8_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_1_0_0_V);
			print_raw_data(buckNumber);	
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_1_2_0_V);
			print_raw_data(buckNumber);	
		}
		else if( buckNumber==BUCK_VPA )
		{
			upmu_buck_vosel(buckNumber,UPMU_VOLT_0_9_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_0_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_1_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_2_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_4_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_5_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_6_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_7_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_8_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_1_9_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_0_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_1_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_2_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_3_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_4_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_5_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_6_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_7_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_8_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_2_9_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_0_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_1_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_2_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_3_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_4_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_4_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_4_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_4_0_0_V);
			print_raw_data(buckNumber);
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_4_0_0_V);
			print_raw_data(buckNumber);	
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_4_0_0_V);
			print_raw_data(buckNumber);	
			upmu_buck_vosel(buckNumber,UPMU_VOLT_3_4_0_0_V);
			print_raw_data(buckNumber);	
		}
		else
		{
			printk("[UVVP_PMIC_BUCK_VOLTAGE_ldvt] No BUCK !!!!!!!!!!!!!!!!!!!\n");
		}			
	}

}

void UVVP_PMIC_BUCK_ON_OFF_ldvt(void)
{
	int i=0;	
	int buckNumber=0;
	int delay=0;	
	int j=0;
	int vol=0;

	printk("[UVVP_PMIC_BUCK_ON_OFF_ldvt]\n");
	
	printk("pmu_power_type,name,channel,pin,adc_voltage\n");
	
	pmu_power_type = BUCK_TYPE;

	for(i=buckNumber; i< BUCK_MAX_NUMBER; i++,buckNumber++)
	{
		pdt_ext_max396_switch(buckNumber);	//switch max396 to make capacitor charge

		printk("[UVVP_PMIC_BUCK_ON_OFF_ldvt] ON...........................\n");	
		upmu_buck_en(buckNumber, 1);
		print_raw_data(buckNumber);

		printk("[UVVP_PMIC_BUCK_ON_OFF_ldvt] OFF...........................\n");	
		upmu_buck_en(buckNumber, 0);
		print_raw_data(buckNumber);

		printk("[UVVP_PMIC_BUCK_ON_OFF_ldvt] ON...........................\n");	
		upmu_buck_en(buckNumber, 1);
		print_raw_data(buckNumber);
	}

}

void UVVP_PMIC_LDO_VOLTAGE_ldvt(void)
{
	int i=0;	
	int ldoNumber=0;
	int delay=0;	
	int j=0;
	int vol=0;

	printk("[UVVP_PMIC_LDO_VOLTAGE_ldvt]\n");

	printk("pmu_power_type,name,channel,pin,adc_voltage\n");

#if 0	
	pmu_power_type = LDO_TYPE;

	for(i=ldoNumber; i< LDO_MAX_NUMBER; i++,ldoNumber++)
	{
		pdt_ext_max396_switch(ldoNumber);	//switch max396 to make capacitor charge

		if( ldoNumber==LDO_VSIM )
		{	
			upmu_ldo_en(ldoNumber, 1);
			
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_1_8_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_3_0_0_0_V);
			print_raw_data(ldoNumber);	
		}
		else if( 	(ldoNumber==LDO_VSIM2) 		|| 
					(ldoNumber==LDO_VCAMD) 		|| 		
					(ldoNumber==LDO_VCAM_IO) 	||
					(ldoNumber==LDO_VCAM_AF)	||
					(ldoNumber==LDO_VMC)		||
					(ldoNumber==LDO_VMCH)		||
					(ldoNumber==LDO_VGP)		||
					(ldoNumber==LDO_VGP2)		||
					(ldoNumber==LDO_VIBR)		
				)
		{
			upmu_ldo_en(ldoNumber, 1);
			
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_1_3_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_1_5_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_1_8_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_2_5_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_2_8_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_3_0_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_3_3_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_3_3_0_0_V);
			print_raw_data(ldoNumber);				
		}
		else if( ldoNumber==LDO_VA1 )
		{
			upmu_ldo_en(ldoNumber, 1);
			
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_2_5_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_2_1_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_2_0_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_1_8_0_0_V);
			print_raw_data(ldoNumber);	
		}
		else if( ldoNumber==LDO_VA2 )
		{
			upmu_ldo_en(ldoNumber, 1);
			
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_2_5_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_2_8_0_0_V);
			print_raw_data(ldoNumber);	
		}	
		else if( ldoNumber==LDO_VCAMA )
		{
			upmu_ldo_en(ldoNumber, 1);
			
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_1_5_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_1_8_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_2_5_0_0_V);
			print_raw_data(ldoNumber);	
			upmu_ldo_vosel(ldoNumber,UPMU_VOLT_2_8_0_0_V);
			print_raw_data(ldoNumber);			
		}		
		else
		{
			printk("[UVVP_PMIC_LDO_VOLTAGE_ldvt] No VOSEL : %s !!!!!!!!!!!!!!!!!!!\n", ldo_name[ldoNumber]);
		}			
	}
#endif

	hwPowerOn(MT65XX_POWER_LDO_VSIM,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VSIM,VOL_3000,"PMIC DVT");
	
	hwPowerOn(MT65XX_POWER_LDO_VSIM2,VOL_1300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VSIM2,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VSIM2,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VSIM2,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VSIM2,VOL_2800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VSIM2,VOL_3000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VSIM2,VOL_3300,"PMIC DVT");

	hwPowerOn(MT65XX_POWER_LDO_VCAMD,VOL_1300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMD,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMD,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMD,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMD,VOL_2800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMD,VOL_3000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMD,VOL_3300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMD,VOL_1200,"PMIC DVT");

	hwPowerOn(MT65XX_POWER_LDO_VCAM_IO,VOL_1300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_IO,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_IO,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_IO,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_IO,VOL_2800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_IO,VOL_3000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_IO,VOL_3300,"PMIC DVT");

	hwPowerOn(MT65XX_POWER_LDO_VCAM_AF,VOL_1300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_AF,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_AF,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_AF,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_AF,VOL_2800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_AF,VOL_3000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAM_AF,VOL_3300,"PMIC DVT");	

	hwPowerOn(MT65XX_POWER_LDO_VMC,VOL_1300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMC,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMC,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMC,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMC,VOL_2800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMC,VOL_3000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMC,VOL_3300,"PMIC DVT");	

	hwPowerOn(MT65XX_POWER_LDO_VMCH,VOL_1300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMCH,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMCH,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMCH,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMCH,VOL_2800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMCH,VOL_3000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VMCH,VOL_3300,"PMIC DVT");

	hwPowerOn(MT65XX_POWER_LDO_VGP,VOL_1300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP,VOL_2800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP,VOL_3000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP,VOL_3300,"PMIC DVT");

	hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_1300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_2800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_3000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VGP2,VOL_3300,"PMIC DVT");	

	hwPowerOn(MT65XX_POWER_LDO_VIBR,VOL_1300,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VIBR,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VIBR,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VIBR,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VIBR,VOL_2800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VIBR,VOL_3000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VIBR,VOL_3300,"PMIC DVT");	

	hwPowerOn(MT65XX_POWER_LDO_VA1,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VA1,VOL_2100,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VA1,VOL_2000,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VA1,VOL_1800,"PMIC DVT");

	hwPowerOn(MT65XX_POWER_LDO_VA2,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VA2,VOL_2800,"PMIC DVT");	

	hwPowerOn(MT65XX_POWER_LDO_VCAMA,VOL_1500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMA,VOL_1800,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMA,VOL_2500,"PMIC DVT");
	hwPowerOn(MT65XX_POWER_LDO_VCAMA,VOL_2800,"PMIC DVT");	

}

void UVVP_PMIC_LDO_ON_OFF_ldvt(void)
{
	int i=0;	
	int ldoNumber=0;
	int delay=0;	
	int j=0;
	int vol=0;

	printk("[UVVP_PMIC_LDO_ON_OFF_ldvt]\n");
#if 0
	printk("pmu_power_type,name,channel,pin,adc_voltage\n");
	
	pmu_power_type = LDO_TYPE;

	for(i=ldoNumber; i< LDO_MAX_NUMBER; i++,ldoNumber++)
	{
		pdt_ext_max396_switch(ldoNumber);	//switch max396 to make capacitor charge

		printk("[UVVP_PMIC_LDO_ON_OFF_ldvt] ON ................................\n");
		upmu_ldo_en(ldoNumber, 1);		
		print_raw_data(ldoNumber);			

		printk("[UVVP_PMIC_LDO_ON_OFF_ldvt] OFF ................................\n");
		upmu_ldo_en(ldoNumber, 0);		
		print_raw_data(ldoNumber);

		printk("[UVVP_PMIC_LDO_ON_OFF_ldvt] ON ................................\n");
		upmu_ldo_en(ldoNumber, 1);		
		print_raw_data(ldoNumber);		
	}
#endif

#if 1
	hwPowerOn(	MT65XX_POWER_LDO_VM12_1,	VOL_DEFAULT,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VM12_1,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VM12_1,	VOL_DEFAULT,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VM12_2,	VOL_DEFAULT,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VM12_2,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VM12_2,	VOL_DEFAULT,	"PMIC DVT");
	
	hwPowerOn(	MT65XX_POWER_LDO_VM12_INT,	VOL_DEFAULT,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VM12_INT,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VM12_INT,	VOL_DEFAULT,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VIO28,		VOL_2800,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VIO28,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VIO28,		VOL_2800,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VSIM,		VOL_3000,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VSIM,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VSIM,		VOL_3000,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VSIM2,		VOL_3000,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VSIM2,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VSIM2,		VOL_3000,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VUSB,		VOL_DEFAULT,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VUSB,						"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VUSB,		VOL_DEFAULT,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VCAMD,		VOL_3300,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VCAMD,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VCAMD,		VOL_3300,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VCAM_IO,		VOL_2800,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VCAM_IO,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VCAM_IO,		VOL_2800,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VCAM_AF,		VOL_1800,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VCAM_AF,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VCAM_AF,		VOL_1800,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VMC,		VOL_1500,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VMC,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VMC,		VOL_1500,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VMCH,		VOL_1300,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VMCH,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VMCH,		VOL_1300,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VGP,		VOL_1500,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VGP,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VGP,		VOL_1500,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VGP2,		VOL_2500,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VGP2,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VGP2,		VOL_2500,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VIBR,		VOL_2800,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VIBR,					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VIBR,		VOL_2800,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VRF,		VOL_DEFAULT,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VRF,						"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VRF,		VOL_DEFAULT,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VTCXO,		VOL_DEFAULT,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VTCXO,						"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VTCXO,		VOL_DEFAULT,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VA1, 	VOL_2100,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VA1, 				"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VA1, 	VOL_2100,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VA2, 	VOL_2500,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VA2, 				"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VA2, 	VOL_2500,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VCAMA, 	VOL_1500,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VCAMA, 				"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VCAMA, 	VOL_1500,	"PMIC DVT");

	hwPowerOn(	MT65XX_POWER_LDO_VRTC, 	VOL_DEFAULT,	"PMIC DVT");
	hwPowerDown(MT65XX_POWER_LDO_VRTC, 					"PMIC DVT");
	hwPowerOn(	MT65XX_POWER_LDO_VRTC, 	VOL_DEFAULT,	"PMIC DVT");
#endif

}
#endif

//---------------------------------------------------------------------------
// IOCTL
//---------------------------------------------------------------------------
//static int uvvp_pmic_ioctl(struct file *file,
//								unsigned int cmd, unsigned long arg)
static long uvvp_pmic_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	printk("\r\n******** uvvp_pmic_ioctl cmd[%d]********\r\n",cmd);
	
	switch (cmd) {
		default:
			return -1;

		//  General TEST CASE
		case UVVP_PMIC_GET_VERSION:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_GET_VERSION ********\r\n");	
			pmic_get_chip_version_ldvt();
			return 0;					
		case UVVP_PMIC_GET_PCHR_CHRDET:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_GET_PCHR_CHRDET ********\r\n");
			pmic_get_PCHR_CHRDET_ldvt();			
			return 0;							

		// Top TEST CASE        
		case UVVP_PMIC_VERIFY_DEFAULT_VALUE:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_VERIFY_DEFAULT_VALUE ********\r\n");
			pmic_VERIFY_DEFAULT_VALUE_ldvt();			
			return 0;
		case UVVP_PMIC_TOP_WR_1:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_TOP_WR_1 ********\r\n");
			pmic_UVVP_PMIC_TOP_WR(0x5a5a);			
			return 0;
		case UVVP_PMIC_TOP_WR_2:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_TOP_WR_2 ********\r\n");
			pmic_UVVP_PMIC_TOP_WR(0xa5a5);			
			return 0;			

		//  INTERRUPT TEST CASE
		case UVVP_PMIC_INT_451:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_INT_451 ********\r\n");
			pmic_UVVP_PMIC_INT_451();			
			return 0;
        case UVVP_PMIC_INT_461:
            printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_INT_461 ********\r\n");
            pmic_UVVP_PMIC_INT_461();           
            return 0;
        case UVVP_PMIC_INT_462:
            printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_INT_462 ********\r\n");
            pmic_UVVP_PMIC_INT_462();           
            return 0;
        case UVVP_PMIC_INT_463:
            printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_INT_463 ********\r\n");
            pmic_UVVP_PMIC_INT_463();           
            return 0;
        case UVVP_PMIC_INT_464:
            printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_INT_464 ********\r\n");
            pmic_UVVP_PMIC_INT_464();           
            return 0;
        case UVVP_PMIC_INT_465:
            printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_INT_465 ********\r\n");
            pmic_UVVP_PMIC_INT_465();           
            return 0;
        case UVVP_PMIC_INT_466:
            printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_INT_466 ********\r\n");
            pmic_UVVP_PMIC_INT_466();           
            return 0;
        case UVVP_PMIC_INT_467:
            printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_INT_467 ********\r\n");
            pmic_UVVP_PMIC_INT_467();           
            return 0;
        case UVVP_PMIC_INT_468:
            printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_INT_468 ********\r\n");
            pmic_UVVP_PMIC_INT_468();           
            return 0;
        
		//  AUXADC TEST CASE					
		case UVVP_PMIC_AUXADC_611:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_AUXADC_611 ********\r\n");
			pmic_UVVP_PMIC_AUXADC_611();			
			return 0;
		case UVVP_PMIC_AUXADC_612:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_AUXADC_612 ********\r\n");
			pmic_UVVP_PMIC_AUXADC_612();			
			return 0;
		case UVVP_PMIC_AUXADC_621:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_AUXADC_621 ********\r\n");
			pmic_UVVP_PMIC_AUXADC_621();			
			return 0;
		case UVVP_PMIC_AUXADC_622:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_AUXADC_622 ********\r\n");
			pmic_UVVP_PMIC_AUXADC_622();			
			return 0;
			
		//  LDO TEST CASE			
        case UVVP_PMIC_LDO_ON_OFF_0:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_0 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(0);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_1:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_1 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(1);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_2:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_2 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(2);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_3:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_3 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(3);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_4:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_4 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(4);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_5:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_5 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(5);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_6:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_6 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(6);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_7:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_7 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(7);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_8:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_8 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(8);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_9:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_9 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(9);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_10:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_10 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(10);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_11:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_11 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(11);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_12:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_12 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(12);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_13:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_13 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(13);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_14:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_14 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(14);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_15:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_15 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(15);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_16:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_16 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(16);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_17:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_17 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(17);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_18:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_18 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(18);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_19:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_19 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(19);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_20:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_20 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(20);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_21:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_21 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(21);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_22:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_22 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(22);			
			return 0;
        case UVVP_PMIC_LDO_ON_OFF_23:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_ON_OFF_23 ********\r\n");
			pmic_UVVP_PMIC_LDO_ON_OFF(23);			
			return 0;
        //----------------------------------------------------------------------------    
        case UVVP_PMIC_LDO_VOSEL_0:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_0 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(0);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_1:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_1 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(1);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_2:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_2 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(2);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_3:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_3 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(3);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_4:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_4 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(4);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_5:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_5 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(5);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_6:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_6 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(6);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_7:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_7 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(7);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_8:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_8 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(8);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_9:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_9 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(9);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_10:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_10 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(10);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_11:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_11 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(11);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_12:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_12 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(12);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_13:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_13 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(13);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_14:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_14 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(14);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_15:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_15 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(15);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_16:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_16 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(16);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_17:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_17 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(17);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_18:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_18 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(18);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_19:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_19 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(19);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_20:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_20 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(20);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_21:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_21 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(21);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_22:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_22 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(22);			
			return 0;
        case UVVP_PMIC_LDO_VOSEL_23:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_VOSEL_23 ********\r\n");
			pmic_UVVP_PMIC_LDO_VOSEL(23);			
			return 0;
        //---------------------------------------------------------------------------- 
        case UVVP_PMIC_LDO_CAL_0:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_0 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(0);			
			return 0;
        case UVVP_PMIC_LDO_CAL_1:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_1 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(1);			
			return 0;
        case UVVP_PMIC_LDO_CAL_2:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_2 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(2);			
			return 0;
        case UVVP_PMIC_LDO_CAL_3:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_3 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(3);			
			return 0;
        case UVVP_PMIC_LDO_CAL_4:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_4 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(4);			
			return 0;
        case UVVP_PMIC_LDO_CAL_5:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_5 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(5);			
			return 0;
        case UVVP_PMIC_LDO_CAL_6:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_6 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(6);			
			return 0;
        case UVVP_PMIC_LDO_CAL_7:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_7 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(7);			
			return 0;
        case UVVP_PMIC_LDO_CAL_8:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_8 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(8);			
			return 0;
        case UVVP_PMIC_LDO_CAL_9:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_9 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(9);			
			return 0;
        case UVVP_PMIC_LDO_CAL_10:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_10 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(10);			
			return 0;
        case UVVP_PMIC_LDO_CAL_11:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_11 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(11);			
			return 0;
        case UVVP_PMIC_LDO_CAL_12:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_12 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(12);			
			return 0;
        case UVVP_PMIC_LDO_CAL_13:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_13 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(13);			
			return 0;
        case UVVP_PMIC_LDO_CAL_14:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_14 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(14);			
			return 0;
        case UVVP_PMIC_LDO_CAL_15:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_15 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(15);			
			return 0;
        case UVVP_PMIC_LDO_CAL_16:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_16 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(16);			
			return 0;
        case UVVP_PMIC_LDO_CAL_17:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_17 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(17);			
			return 0;
        case UVVP_PMIC_LDO_CAL_18:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_18 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(18);			
			return 0;
        case UVVP_PMIC_LDO_CAL_19:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_19 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(19);			
			return 0;
        case UVVP_PMIC_LDO_CAL_20:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_20 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(20);			
			return 0;
        case UVVP_PMIC_LDO_CAL_21:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_21 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(21);			
			return 0;
        case UVVP_PMIC_LDO_CAL_22:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_22 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(22);			
			return 0;
        case UVVP_PMIC_LDO_CAL_23:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_LDO_CAL_23 ********\r\n");
			pmic_UVVP_PMIC_LDO_CAL(23);			
			return 0;

   		//  BUCK TEST CASE		
        case UVVP_PMIC_BUCK_ON_OFF_0:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_ON_OFF_0 ********\r\n");
			pmic_UVVP_PMIC_BUCK_ON_OFF(0);			
			return 0;
        case UVVP_PMIC_BUCK_ON_OFF_1:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_ON_OFF_1 ********\r\n");
			pmic_UVVP_PMIC_BUCK_ON_OFF(1);			
			return 0;
        case UVVP_PMIC_BUCK_ON_OFF_2:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_ON_OFF_2 ********\r\n");
			pmic_UVVP_PMIC_BUCK_ON_OFF(2);			
			return 0;
        case UVVP_PMIC_BUCK_ON_OFF_3:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_ON_OFF_3 ********\r\n");
			pmic_UVVP_PMIC_BUCK_ON_OFF(3);			
			return 0;
        case UVVP_PMIC_BUCK_ON_OFF_4:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_ON_OFF_4 ********\r\n");
			pmic_UVVP_PMIC_BUCK_ON_OFF(4);			
			return 0;
        case UVVP_PMIC_BUCK_ON_OFF_5:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_ON_OFF_5 ********\r\n");
			pmic_UVVP_PMIC_BUCK_ON_OFF(5);			
			return 0;
        case UVVP_PMIC_BUCK_ON_OFF_6:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_ON_OFF_6 ********\r\n");
			pmic_UVVP_PMIC_BUCK_ON_OFF(6);			
			return 0;
        case UVVP_PMIC_BUCK_ON_OFF_7:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_ON_OFF_7 ********\r\n");
			pmic_UVVP_PMIC_BUCK_ON_OFF(7);			
			return 0;
        //----------------------------------------------------------------------------
        case UVVP_PMIC_BUCK_VOSEL_0:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_VOSEL_0 ********\r\n");
			pmic_UVVP_PMIC_BUCK_VOSEL(0);			
			return 0;
        case UVVP_PMIC_BUCK_VOSEL_1:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_VOSEL_1 ********\r\n");
			pmic_UVVP_PMIC_BUCK_VOSEL(1);			
			return 0;
        case UVVP_PMIC_BUCK_VOSEL_2:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_VOSEL_2 ********\r\n");
			pmic_UVVP_PMIC_BUCK_VOSEL(2);			
			return 0;
        case UVVP_PMIC_BUCK_VOSEL_3:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_VOSEL_3 ********\r\n");
			pmic_UVVP_PMIC_BUCK_VOSEL(3);			
			return 0;
        case UVVP_PMIC_BUCK_VOSEL_4:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_VOSEL_4 ********\r\n");
			pmic_UVVP_PMIC_BUCK_VOSEL(4);			
			return 0;
        case UVVP_PMIC_BUCK_VOSEL_5:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_VOSEL_5 ********\r\n");
			pmic_UVVP_PMIC_BUCK_VOSEL(5);			
			return 0;
        case UVVP_PMIC_BUCK_VOSEL_6:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_VOSEL_6 ********\r\n");
			pmic_UVVP_PMIC_BUCK_VOSEL(6);			
			return 0;
        case UVVP_PMIC_BUCK_VOSEL_7:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_VOSEL_7 ********\r\n");
			pmic_UVVP_PMIC_BUCK_VOSEL(7);			
			return 0;
        //----------------------------------------------------------------------------
        case UVVP_PMIC_BUCK_DLC_0:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_DLC_0 ********\r\n");
			pmic_UVVP_PMIC_BUCK_DLC(0);			
			return 0;
        case UVVP_PMIC_BUCK_DLC_1:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_DLC_1 ********\r\n");
			pmic_UVVP_PMIC_BUCK_DLC(1);			
			return 0;
        case UVVP_PMIC_BUCK_DLC_2:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_DLC_2 ********\r\n");
			pmic_UVVP_PMIC_BUCK_DLC(2);			
			return 0;
        case UVVP_PMIC_BUCK_DLC_3:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_DLC_3 ********\r\n");
			pmic_UVVP_PMIC_BUCK_DLC(3);			
			return 0;
        case UVVP_PMIC_BUCK_DLC_4:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_DLC_4 ********\r\n");
			pmic_UVVP_PMIC_BUCK_DLC(4);			
			return 0;
        case UVVP_PMIC_BUCK_DLC_5:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_DLC_5 ********\r\n");
			pmic_UVVP_PMIC_BUCK_DLC(5);			
			return 0;
        case UVVP_PMIC_BUCK_DLC_6:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_DLC_6 ********\r\n");
			pmic_UVVP_PMIC_BUCK_DLC(6);			
			return 0;
        case UVVP_PMIC_BUCK_DLC_7:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_DLC_7 ********\r\n");
			pmic_UVVP_PMIC_BUCK_DLC(7);			
			return 0;
        //----------------------------------------------------------------------------
        case UVVP_PMIC_BUCK_BURST_0:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_BURST_0 ********\r\n");
			pmic_UVVP_PMIC_BUCK_BURST(0);			
			return 0;
        case UVVP_PMIC_BUCK_BURST_1:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_BURST_1 ********\r\n");
			pmic_UVVP_PMIC_BUCK_BURST(1);			
			return 0;
        case UVVP_PMIC_BUCK_BURST_2:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_BURST_2 ********\r\n");
			pmic_UVVP_PMIC_BUCK_BURST(2);			
			return 0;
        case UVVP_PMIC_BUCK_BURST_3:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_BURST_3 ********\r\n");
			pmic_UVVP_PMIC_BUCK_BURST(3);			
			return 0;
        case UVVP_PMIC_BUCK_BURST_4:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_BURST_4 ********\r\n");
			pmic_UVVP_PMIC_BUCK_BURST(4);			
			return 0;
        case UVVP_PMIC_BUCK_BURST_5:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_BURST_5 ********\r\n");
			pmic_UVVP_PMIC_BUCK_BURST(5);			
			return 0;
        case UVVP_PMIC_BUCK_BURST_6:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_BURST_6 ********\r\n");
			pmic_UVVP_PMIC_BUCK_BURST(6);			
			return 0;
        case UVVP_PMIC_BUCK_BURST_7:
			printk("\r\n******** uvvp_pmic_ioctl UVVP_PMIC_BUCK_BURST_7 ********\r\n");
			pmic_UVVP_PMIC_BUCK_BURST(7);			
			return 0;

        //  DRIVER TEST CASE (coverd by SA)
        //  P-chr TEST CASE (coverd by SA)
		//  FGADC TEST CASE (coverd by UVVP_PMIC_VERIFY_DEFAULT_VALUE and SA)	
            
	}

	return 0;	
}

static int uvvp_pmic_open(struct inode *inode, struct file *file)
{
	return 0;
}

static struct file_operations uvvp_pmic_fops = {
	.owner           = THIS_MODULE,
		
	.open            = uvvp_pmic_open,
	.unlocked_ioctl  = uvvp_pmic_ioctl,
	//.compat_ioctl    = uvvp_pmic_ioctl,
};

static struct miscdevice uvvp_pmic_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = pmicname,
	.fops = &uvvp_pmic_fops,
};

static int __init uvvp_pmic_init(void)
{
    int ret;
	printk("\r\n******** uvvp_pmic_init  ********\r\n");
	ret = misc_register(&uvvp_pmic_dev);
    if(ret){
		printk("register driver failed\n");
	}
	return 0;
}

static void __exit uvvp_pmic_exit(void)
{

}

module_init(uvvp_pmic_init);
module_exit(uvvp_pmic_exit);


