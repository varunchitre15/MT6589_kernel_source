#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_pmic_wrap.h>
#include <mach/upmu_hw.h>
#include <mach/mt_spm_idle.h>
#include <mach/mt_clkmgr.h>
#include <mach/pmic_mt6320_sw.h>
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>

u32 MT6589_Suspend_Golden[] = {
// mipi register ==================
0xF0012044, 0xffffffff, 0x88492480,
0xF0012000, 0xffffffff, 0x00000000,
0xF0012004, 0xffffffff, 0x00000820,
0xF0012008, 0xffffffff, 0x00000400,
0xF001200C, 0xffffffff, 0x00000100,
0xF0012010, 0xffffffff, 0x00000100,
0xF0012014, 0xffffffff, 0x00000100,
0xF0012040, 0xffffffff, 0x00000080,
0xF0012068, 0xffffffff, 0x00000000,
0xF0012824, 0xffffffff, 0x24248800,
0xF0012820, 0xffffffff, 0x00000000,
0xF0012800, 0xffffffff, 0x00008000,
0xF0012804, 0xffffffff, 0x00008000,
0xF0012808, 0xffffffff, 0x00008000,
0xF001280C, 0xffffffff, 0x00008000,
0xF0012810, 0xffffffff, 0x00008000,
0xF0012814, 0xffffffff, 0x00008000,
0xF0012818, 0xffffffff, 0x00008000,
0xF001281C, 0xffffffff, 0x00008000,
0xF0012838, 0xffffffff, 0x00000000,
0xF001283C, 0xffffffff, 0x00000000,
0xF0012840, 0xffffffff, 0x00000000,
0xF0012060, 0xffffffff, 0x00000200,
0xF0012050, 0xffffffff, 0xBE510026,
};

u32 MT6320_E1_Suspend_Golden[] = {	
0x714, 0xffff, 0x992   ,
0x71A, 0x9000, 0x9000  ,
//0x422, 0xc000, 0x0     ,
0x424, 0x9000, 0x2000  ,
0x420, 0xffff, 0xf001  ,
//0x444, 0x9000, 0x4     , // vast, always on
0x462, 0xffff, 0x0     ,
0x428, 0xffff, 0x0     ,
0x402, 0xffff, 0xbc30  ,
0x426, 0xc000, 0x0     ,
0x406, 0xffff, 0xc001  ,
0x404, 0xffff, 0xc001  ,
0x214, 0xffff, 0x3001  ,
0x27C, 0xffff, 0x101   ,
0x2A2, 0xffff, 0x101   ,
0x324, 0xffff, 0x101   ,
0x250, 0xffff, 0x113   ,
0x436, 0xffff, 0x0     ,
0x438, 0xffff, 0x0     ,
0x12A, 0xffff, 0x8001  ,
0x502, 0xf00f, 0x400c  ,
0x102, 0xffff, 0xc61f  ,
0x108, 0xffff, 0x0     ,
0x128, 0x003f, 0xf0a   ,
0x718, 0xffff, 0x4     ,
//0x014, 0x0020, 0x0     ,
0x746, 0xffff, 0x1     ,
0x712, 0x0003, 0x0     ,
0x710, 0x0001, 0x0     ,
0x70c, 0x1000, 0x1552  ,
0x702, 0x0007, 0x0     ,
0x700, 0x000f, 0x0     ,
};

u32 MT6320_E2_Suspend_Golden[] = {	
0x714, 0xffff, 0x192   ,
0x71A, 0x9000, 0x8000  ,
//0x422, 0xc000, 0x0     ,
0x424, 0x9000, 0x2000  ,
0x420, 0xffff, 0xf001  ,
//0x444, 0x9000, 0x4     , // vast, always on
0x462, 0xffff, 0x0     ,
0x428, 0xffff, 0x0     ,
0x402, 0xffff, 0xbc30  ,
0x426, 0xc000, 0x0     ,
0x406, 0xffff, 0xc001  ,
0x404, 0xffff, 0xc001  ,
0x214, 0xffff, 0x3001  ,
0x27C, 0xffff, 0x101   ,
0x2A2, 0xffff, 0x101   ,
0x324, 0xffff, 0x101   ,
0x250, 0xffff, 0x113   ,
0x436, 0xffff, 0x0     ,
0x438, 0xffff, 0x0     ,
0x12A, 0xffff, 0x8001  ,
0x502, 0xf00f, 0x400c  ,
0x102, 0xffff, 0xc61f  ,
0x108, 0xffff, 0x0     ,
0x128, 0x003f, 0xf0a   ,
0x718, 0xffff, 0x6     ,
//0x014, 0x0020, 0x0     ,
0x746, 0xffff, 0x1     ,
0x712, 0x0003, 0x0     ,
0x710, 0x0001, 0x0     ,
0x70c, 0x1000, 0x1552  ,
0x702, 0x0007, 0x0     ,
0x700, 0x000f, 0x0     ,
};

char pmic_6320_reg_0x13e[][10] = { 
 "Vproc", "Vsram", "Vcore", "Vm", "Vio18", "Vpa", "Vrf18", "Vrf18_2",
"Vusb", "Vtcxo", "Vtcxo_2", "Vsim2", "Vsim1", "Vrf28", "Vrf28_2", "Vrtc"
};

char pmic_6320_reg_0x140[][10] = { 
"Vmc", "Vmc1", "Vio28", "Vibr", "Vgp6", "Vgp5", "Vgp4", "Vgp3",
"Vgp2", "Vgp1", "Vemc_3v3", "Vemc_1v8", "Vcama", "Va", "Vast", "Va28"
};


#define gs_read(addr)		(*(volatile u32 *)(addr))

static U16 gs_pmic_read(U16 addr)
{
	U32 rdata=0;
	pwrap_read((U32)addr, &rdata);
	return (U16)rdata;
}

static void Golden_Setting_Compare_PLL(void)
{
    if (pll_is_on(MSDCPLL)) {
        clc_notice("MSDCPLL: %s\n", pll_is_on(MSDCPLL) ?  "on" : "off");
    }
    if (pll_is_on(TVDPLL)) {
        clc_notice("TVDPLL: %s\n", pll_is_on(TVDPLL) ? "on" : "off");
    }
    if (pll_is_on(LVDSPLL)) {
        clc_notice("LVDSPLL: %s\n", pll_is_on(LVDSPLL) ? "on" : "off");
    }
    if (pll_is_on(ISPPLL)) {
        clc_notice("ISPPLL: %s\n", pll_is_on(ISPPLL) ? "on" : "off");
    }
    if (subsys_is_on(SYS_MD1)) {
        clc_notice("SYS_MD1: %s\n", subsys_is_on(SYS_MD1) ? "on" : "off");
    }
    if (subsys_is_on(SYS_MD2)) {
        clc_notice("SYS_MD2: %s\n", subsys_is_on(SYS_MD2) ? "on" : "off");
    }
    if (subsys_is_on(SYS_DIS)) {
        clc_notice("SYS_DIS: %s\n", subsys_is_on(SYS_DIS) ? "on" : "off");
    }
    if (subsys_is_on(SYS_MFG)) {
        clc_notice("SYS_MFG: %s\n", subsys_is_on(SYS_MFG) ? "on" : "off");
    }
    if (subsys_is_on(SYS_ISP)) {
        clc_notice("SYS_ISP: %s\n", subsys_is_on(SYS_ISP) ? "on" : "off");
    }
    if (subsys_is_on(SYS_VEN)) {
        clc_notice("SYS_VEN: %s\n", subsys_is_on(SYS_VEN) ? "on" : "off");
    }
    if (subsys_is_on(SYS_VDE)) {
        clc_notice("SYS_VDE: %s\n", subsys_is_on(SYS_VDE) ? "on" : "off");
    }
}

static void Golden_Setting_Compare_PMIC_LDO(void)                                         
{                 
    u16 temp_value, temp_i;

    // PMIC 0x13E ==========================================
    temp_value = gs_pmic_read(0x13E);

    for( temp_i=0 ; temp_i<16 ; temp_i++ )
    {
        if( (temp_value & (0x1<<temp_i)) == (0x1<<temp_i) )
        {
            clc_notice("PMIC %s : On.\n", pmic_6320_reg_0x13e[temp_i]);      
        }
    }
    
    // PMIC 0x140 ==========================================
    temp_value = gs_pmic_read(0x140);

    for( temp_i=0 ; temp_i<16 ; temp_i++ )
    {
        if( (temp_value & (0x1<<temp_i)) == (0x1<<temp_i) )
        {
            clc_notice("PMIC %s : On.\n", pmic_6320_reg_0x140[temp_i]);      
        }
    }
}

void Golden_Setting_Compare_for_Suspend(void)
{
    u32 i, counter_6589, counter_6320;
    u32 MT_6589_Len, MT_6320_Len;
    u32 chip_version = 0;
    u32 *MT6320_Suspend_Golden_ptr;
    
    // check MT6320_E1 or MT6320_E2
    chip_version = upmu_get_cid();    
    if(chip_version == PMIC6320_E1_CID_CODE)
    {
        MT_6320_Len = sizeof(MT6320_E1_Suspend_Golden) / sizeof(u32);
        MT6320_Suspend_Golden_ptr = (u32 *)MT6320_E1_Suspend_Golden;
    }
    else if(chip_version == PMIC6320_E2_CID_CODE)
    {
        MT_6320_Len = sizeof(MT6320_E2_Suspend_Golden) / sizeof(u32);
        MT6320_Suspend_Golden_ptr = (u32 *)MT6320_E2_Suspend_Golden;
    }
    else
    {
        MT_6320_Len = sizeof(MT6320_E2_Suspend_Golden) / sizeof(u32);
        MT6320_Suspend_Golden_ptr = (u32 *)MT6320_E2_Suspend_Golden;
    }

    MT_6589_Len = sizeof(MT6589_Suspend_Golden) / sizeof(u32);
    counter_6589 = 0;
    counter_6320 = 0;

    // MT6589 ======================================================================================================
    for( i=0 ; i<MT_6589_Len ; i+=3 )
    {
        if( (gs_read(MT6589_Suspend_Golden[i]) & MT6589_Suspend_Golden[i+1]) != (MT6589_Suspend_Golden[i+2] & MT6589_Suspend_Golden[i+1]))
        {
            counter_6589++;
            clc_notice("MT6589 Suspend register[0x%x] = 0x%x (mask : 0x%x, value : 0x%x)\n", MT6589_Suspend_Golden[i], gs_read(MT6589_Suspend_Golden[i]), MT6589_Suspend_Golden[i+1], MT6589_Suspend_Golden[i+2] );
        }
    }

    if(counter_6589 == 0)
    {
        clc_notice("MT6589 Suspend golden setting : pass.\n");
    }

    Golden_Setting_Compare_PLL();

    // MT6320 ======================================================================================================
    for( i=0 ; i<MT_6320_Len ; i+=3 )
    {
        if( (gs_pmic_read(MT6320_Suspend_Golden_ptr[i]) & MT6320_Suspend_Golden_ptr[i+1]) != (MT6320_Suspend_Golden_ptr[i+2] & MT6320_Suspend_Golden_ptr[i+1]))
        {
            counter_6320++;
            clc_notice("MT6320 Suspend register[0x%x] = 0x%x (mask : 0x%x, value : 0x%x)\n", MT6320_Suspend_Golden_ptr[i], gs_pmic_read(MT6320_Suspend_Golden_ptr[i]), MT6320_Suspend_Golden_ptr[i+1], MT6320_Suspend_Golden_ptr[i+2] );
        }
    }

    if(counter_6320 == 0)
    {
        clc_notice("MT6320 Suspend golden setting : pass.\n");
    }

    Golden_Setting_Compare_PMIC_LDO();
    
    // ============================================================================================================
}

static struct platform_driver mtk_golden_setting_driver = {
    .remove     = NULL,
    .shutdown   = NULL,
    .probe      = NULL,
    .suspend	= NULL,
    .resume		= NULL,
    .driver     = {
        .name = "mtk-golden-setting",
    },
};

/***************************
* golden_setting_debug_read
****************************/
static int golden_setting_debug_read(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    char *p = buf;

    p += sprintf(p, "golden setting\n");

    len = p - buf;
    return len;
}

/************************************
* golden_setting_debug_write
*************************************/
static ssize_t golden_setting_debug_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
    int enabled = 0;

    if (sscanf(buffer, "%d", &enabled) == 1)
    {
        if (enabled == 1)
        {
            Golden_Setting_Compare_for_Suspend();
        }
        else
        {
            clc_notice("bad argument_0!! argument should be \"1\"\n");
        }
    }
    else
    {
        clc_notice("bad argument_0!! argument should be \"1\"\n");
    }

    return count;
}


static int __init golden_setting_init(void)
{
    struct proc_dir_entry *mt_entry = NULL;
    struct proc_dir_entry *mt_golden_setting_dir = NULL;
    int golden_setting_err = 0;

    mt_golden_setting_dir = proc_mkdir("golden_setting", NULL);
    if (!mt_golden_setting_dir)
    {
        clc_notice("[%s]: mkdir /proc/golden_setting failed\n", __FUNCTION__);
    }
    else
    {
        mt_entry = create_proc_entry("golden_setting_debug", S_IRUGO | S_IWUSR | S_IWGRP, mt_golden_setting_dir);
        if (mt_entry)
        {
            mt_entry->read_proc = golden_setting_debug_read;
            mt_entry->write_proc = golden_setting_debug_write;
        }
    }

    golden_setting_err = platform_driver_register(&mtk_golden_setting_driver);
    
    if (golden_setting_err)
    {
        clc_notice("golden setting driver callback register failed..\n");
        return golden_setting_err;
    }
    
    return 0;
}


MODULE_DESCRIPTION("MT6589 golden setting compare v0.1");

late_initcall(golden_setting_init);

