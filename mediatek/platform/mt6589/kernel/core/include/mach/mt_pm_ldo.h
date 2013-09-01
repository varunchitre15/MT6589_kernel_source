
#ifndef _MT6320_PMIC_LDO_H_
#define _MT6320_PMIC_LDO_H_

#include <mach/mt_typedefs.h>

#define MAX_DEVICE      20
#define MAX_MOD_NAME    32

#define NON_OP "NOP"

/* Debug message event */
#define DBG_PMAPI_NONE 0x00000000	
#define DBG_PMAPI_CG   0x00000001	
#define DBG_PMAPI_PLL  0x00000002	
#define DBG_PMAPI_SUB  0x00000004	
#define DBG_PMAPI_PMIC 0x00000008	
#define DBG_PMAPI_ALL  0xFFFFFFFF	
	
#define DBG_PMAPI_MASK (DBG_PMAPI_ALL)

typedef enum MT65XX_POWER_TAG {	
	//Digital LDO
	MT65XX_POWER_LDO_VIO28 = 0,
	MT65XX_POWER_LDO_VUSB,	
	MT65XX_POWER_LDO_VMC1,
	MT65XX_POWER_LDO_VMCH1,
	MT65XX_POWER_LDO_VEMC_3V3,
	MT65XX_POWER_LDO_VEMC_1V8,
	MT65XX_POWER_LDO_VGP1,
	MT65XX_POWER_LDO_VGP2,
	MT65XX_POWER_LDO_VGP3,
	MT65XX_POWER_LDO_VGP4,
	MT65XX_POWER_LDO_VGP5,
	MT65XX_POWER_LDO_VGP6,
	MT65XX_POWER_LDO_VSIM1,
	MT65XX_POWER_LDO_VSIM2,
	MT65XX_POWER_LDO_VIBR,
	MT65XX_POWER_LDO_VRTC,
	MT65XX_POWER_LDO_VAST,

	//Analog LDO
	MT65XX_POWER_LDO_VRF28,
	MT65XX_POWER_LDO_VRF28_2,
	MT65XX_POWER_LDO_VTCXO,
	MT65XX_POWER_LDO_VTCXO_2,
	MT65XX_POWER_LDO_VA,
	MT65XX_POWER_LDO_VA28,
	MT65XX_POWER_LDO_VCAMA,
	
	MT65XX_POWER_COUNT_END,
	MT65XX_POWER_NONE = -1
} MT65XX_POWER;

typedef enum MT65XX_POWER_VOL_TAG 
{
    VOL_DEFAULT, 
    VOL_0900 = 900,
    VOL_1000 = 1000,
    VOL_1100 = 1100,
    VOL_1200 = 1200,	
    VOL_1300 = 1300,    
    VOL_1500 = 1500,    
    VOL_1800 = 1800,    
    VOL_2000 = 2000,
    VOL_2100 = 2100,
    VOL_2500 = 2500,    
    VOL_2800 = 2800, 
    VOL_3000 = 3000,
    VOL_3300 = 3300        
} MT65XX_POWER_VOLTAGE;	

typedef struct { 
    DWORD dwPowerCount; 
    BOOL bDefault_on;
    char name[MAX_MOD_NAME];        
    char mod_name[MAX_DEVICE][MAX_MOD_NAME];    
} DEVICE_POWER;

typedef struct
{    
    DEVICE_POWER Power[MT65XX_POWER_COUNT_END];    
} ROOTBUS_HW;

//==============================================================================
// PMIC6320 Exported Function for power service
//==============================================================================
extern void pmic_ldo_enable(MT65XX_POWER powerId, kal_bool powerEnable);
extern void pmic_ldo_vol_sel(MT65XX_POWER powerId, MT65XX_POWER_VOLTAGE powerVolt);

extern bool hwPowerOn(MT65XX_POWER powerId, MT65XX_POWER_VOLTAGE powerVolt, char *mode_name);
extern bool hwPowerDown(MT65XX_POWER powerId, char *mode_name);

#endif // _MT6320_PMIC_LDO_H_

