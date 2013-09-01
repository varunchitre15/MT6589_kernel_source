#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/aee.h>

#include <mach/irqs.h>
#include <mach/mt_boot.h>
#include <mach/mt_cirq.h>
#include <mach/mt_spm.h>
#include <mach/mt_spm_sleep.h>
#include <mach/mt_clkmgr.h>
#include <mach/mt_dcm.h>
#include <mach/mt_dormant.h>
#include <mach/mt_mci.h>
#include <mach/eint.h>
#include <mach/mtk_ccci_helper.h>

/**************************************
 * only for internal debug
 **************************************/
#ifdef CONFIG_MTK_LDVT
#define SPM_PWAKE_EN            0
#define SPM_BYPASS_SYSPWREQ     1
#else
#define SPM_PWAKE_EN            1
#define SPM_BYPASS_SYSPWREQ     0
#endif


/**********************************************************
 * PCM code for suspend (v70rc7 @ 2013-01-16)
 **********************************************************/
#if defined(MTK_ENABLE_MD2) && defined(MODEM2_3G)   /* for AST */
static const u32 spm_pcm_suspend[] = {
    0x19c0001f, 0x003c0bd7, 0x1800001f, 0xf7cf1f3d, 0x1b80001f, 0x20000000,
    0x1800001f, 0xf7cf1f14, 0x19c0001f, 0x003c0bc7, 0x1a00001f, 0x2000000a,
    0xd8000426, 0x17c07c1f, 0x18c0001f, 0x10006234, 0xe0e00e16, 0x1380201f,
    0xe0e00e1e, 0x1380201f, 0xe0e00e0e, 0xe0e00e0f, 0xe0e00e0d, 0xe0e00e0d,
    0xe0e00c0d, 0xe0e0080d, 0xe0e0000d, 0x18c0001f, 0x10006240, 0xe0e00f16,
    0xe0e00f1e, 0xe0e00f0e, 0xe0e00f0f, 0xe8208000, 0x10006354, 0xfffffffd,
    0xe8208000, 0x1000627c, 0x00000005, 0x1b80001f, 0x20000020, 0xe8208000,
    0x1000627c, 0x00000009, 0x19c0001f, 0x003c0b87, 0x1b80001f, 0x20000030,
    0x1800001f, 0xc7cf1f14, 0x1b80001f, 0x200016a8, 0x1800001f, 0x87c21f14,
    0x1b80001f, 0x20000424, 0x1800001f, 0x04821f14, 0x1b80001f, 0x20000424,
    0x1800001f, 0x00821f14, 0x1b80001f, 0x20000424, 0x1800001f, 0x00820214,
    0x1800001f, 0x00820210, 0x1b80001f, 0x2000000a, 0x18c0001f, 0x10006240,
    0xe0e00f0d, 0x19c0001f, 0x00200b87, 0x19c0001f, 0x00200b85, 0xe8208000,
    0x10006354, 0xffff00fd, 0xe8208000, 0x10006314, 0x0b160060, 0x19c0001f,
    0x00210a85, 0x1890001f, 0x10006600, 0x1900001f, 0x10006910, 0x80c08801,
    0x82410801, 0xd8200be3, 0x17c07c1f, 0x1900001f, 0x10006284, 0x1212041f,
    0xe120001e, 0x1380201f, 0xe120000c, 0x1380201f, 0xe120000d, 0x1380201f,
    0xe120001f, 0xe1200012, 0xd8200dc9, 0x17c07c1f, 0x1900001f, 0x10006280,
    0xd0000be0, 0x12407c1f, 0x80c00801, 0x1910001f, 0x10001050, 0x89000004,
    0xfffffeff, 0xa0d40c04, 0x1900001f, 0x10001050, 0xe1000003, 0x1b00001f,
    0x3ffff7ff, 0xf0000000, 0x17c07c1f, 0x1b00001f, 0x3fffe7ff, 0x1b80001f,
    0x20000004, 0x8080b401, 0xb0823441, 0xd8001b2c, 0xa080080c, 0x1b00001f,
    0x3ffff7ff, 0xd8001b22, 0x17c07c1f, 0xe8208000, 0x10006314, 0x0b160068,
    0x18d0001f, 0x10001050, 0x88c00003, 0xfffffeff, 0x1900001f, 0x10001050,
    0xe1000003, 0x18d0001f, 0x10001050, 0xe8208000, 0x10006354, 0xffffffff,
    0x19c0001f, 0x00240b05, 0x1b80001f, 0x2000000a, 0x19c0001f, 0x00240b07,
    0x1b80001f, 0x20000100, 0x8880000d, 0x00700000, 0x68a00002, 0x00700000,
    0xd80015c2, 0x17c07c1f, 0x1880001f, 0x10006320, 0xe080000f, 0x1880001f,
    0x10006814, 0xe0800001, 0x1b00001f, 0x3fffe7ff, 0xd0001b20, 0x17c07c1f,
    0x19c0001f, 0x003c0bc7, 0x1b80001f, 0x20000030, 0xe8208000, 0x1000627c,
    0x00000005, 0xe8208000, 0x1000627c, 0x00000004, 0xd80017e6, 0x17c07c1f,
    0x18c0001f, 0x10006240, 0xe0e00f0f, 0xe0e00f1e, 0xe0e00f12, 0x1800001f,
    0x00820234, 0x1800001f, 0x00821f34, 0x1800001f, 0x87c21f34, 0x1800001f,
    0xc7cf1f34, 0xd8001a26, 0x17c07c1f, 0x18c0001f, 0x10006234, 0xe0e00e0f,
    0xe0e00e1e, 0xe0e00e12, 0xe8208000, 0x10006234, 0x000f0e12, 0x19c0001f,
    0x003c2bd7, 0x1800001f, 0xf7ff1f3d, 0x19c0001f, 0x001823d7, 0x1b00001f,
    0x3fffefff, 0xf0000000, 0x17c07c1f, 0xe2e00e16, 0x1380201f, 0xe2e00e1e,
    0x1380201f, 0xe2e00e0e, 0x1380201f, 0xe2e00e0f, 0xe2e00e0d, 0xe2e00e0d,
    0xe2e00c0d, 0xe2e0080d, 0xe2e0000d, 0xf0000000, 0x17c07c1f, 0xd8001daa,
    0x17c07c1f, 0xe2e0006d, 0xe2e0002d, 0xd8201e4a, 0x17c07c1f, 0xe2e0002f,
    0xe2e0003e, 0xe2e00032, 0xf0000000, 0x17c07c1f, 0x1a00001f, 0xffffffff,
    0x1210a01f, 0xe2c00008, 0xd8001eca, 0x02a0040a, 0xf0000000, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001,
    0xa1d48407, 0x1990001f, 0x10006400, 0x1b00001f, 0x3fffe7ff, 0x1b80001f,
    0xd00f0000, 0x8880000c, 0x3effe7ff, 0xd8003fc2, 0x1140041f, 0x809c040c,
    0xd80020a2, 0x8098040d, 0xc0c03d60, 0x81471801, 0xd80026a5, 0x17c07c1f,
    0x18c0001f, 0x10006200, 0xc0c01d20, 0x12807c1f, 0x1900001f, 0x1000625c,
    0xe1307fff, 0xc0c01d20, 0x1280041f, 0x18c0001f, 0x10006208, 0xc0c01d20,
    0x12807c1f, 0x1900001f, 0x10006248, 0x1a80001f, 0x00000020, 0xc1001e80,
    0x17c07c1f, 0x1900001f, 0x10006250, 0x1a80001f, 0x00000008, 0xc1001e80,
    0x17c07c1f, 0x1900001f, 0x10006258, 0x1a80001f, 0x00000004, 0xc1001e80,
    0x17c07c1f, 0xc0c01d20, 0x1280041f, 0x81879801, 0x1a10001f, 0x10006608,
    0x8249a001, 0x828a2001, 0x1b00001f, 0x3fffefff, 0x1b80001f, 0xd0010000,
    0x8098840d, 0x80c0b40a, 0xb0c23469, 0xa0800c02, 0xd8202942, 0x17c07c1f,
    0x8880000c, 0x3effe7ff, 0xd8003b42, 0x17c07c1f, 0xd0002740, 0x17c07c1f,
    0xe8208000, 0x10006310, 0x0b160038, 0xe8208000, 0x10006314, 0x0b160068,
    0x18d0001f, 0x10001050, 0x88c00003, 0xfffffeff, 0x1900001f, 0x10001050,
    0xe1000003, 0x18d0001f, 0x10001050, 0x19c0001f, 0x00200a01, 0x1880001f,
    0x10006320, 0xe8208000, 0x10006354, 0xffffffff, 0xc0c03e00, 0xe080000f,
    0xd8002743, 0x17c07c1f, 0xe080001f, 0x19c0001f, 0x00380bc7, 0x1b80001f,
    0x20000030, 0xe8208000, 0x1000627c, 0x00000005, 0xe8208000, 0x1000627c,
    0x00000004, 0xd8002ea6, 0x17c07c1f, 0x18c0001f, 0x10006240, 0xc0c03c60,
    0x17c07c1f, 0x1800001f, 0x00000034, 0x1800001f, 0x00001f34, 0x1800001f,
    0x87c01f34, 0x1800001f, 0xc7cf1f34, 0xd80030c6, 0x17c07c1f, 0x18c0001f,
    0x10006234, 0xc0c03c60, 0x17c07c1f, 0xe8208000, 0x10006234, 0x000f0e12,
    0x1b00001f, 0x3fffefff, 0x19c0001f, 0x00382bd7, 0x1800001f, 0xf7ff1f3d,
    0x19c0001f, 0x001823d7, 0x1b80001f, 0x90100000, 0x80881c01, 0xd80039a2,
    0x17c07c1f, 0x19c0001f, 0x003c0bd7, 0x1800001f, 0xf7cf1f3d, 0x1b80001f,
    0x20000000, 0x1800001f, 0xf7cf1f14, 0x19c0001f, 0x003c0bc7, 0x18c0001f,
    0x10006240, 0x1900001f, 0x10006234, 0xd8003526, 0x17c07c1f, 0xc1001b60,
    0x1211841f, 0xe0e00f16, 0xe0e00f1e, 0xe0e00f0e, 0xe0e00f0f, 0xe8208000,
    0x1000627c, 0x00000005, 0x1b80001f, 0x20000020, 0xe8208000, 0x1000627c,
    0x00000009, 0x19c0001f, 0x003c0b87, 0x1800001f, 0xc7cf1f14, 0x1b80001f,
    0x200016a8, 0x1800001f, 0x87c01f14, 0x1b80001f, 0x20000424, 0x1800001f,
    0x04001f14, 0x1b80001f, 0x20000424, 0x1800001f, 0x00001f14, 0x1b80001f,
    0x20000424, 0x1800001f, 0x00000014, 0x10007c1f, 0x1b80001f, 0x2000000a,
    0xe0e00f0d, 0x19c0001f, 0x00240b07, 0xd8203b42, 0x17c07c1f, 0x1800001f,
    0x00800210, 0x1b80001f, 0x20000424, 0x1800001f, 0x00000210, 0x1b80001f,
    0x20000424, 0x1800001f, 0x00000010, 0x10007c1f, 0x19c0001f, 0x00250b05,
    0xe8208000, 0x10006354, 0xffff00fd, 0x19c0001f, 0x00210b05, 0x19c0001f,
    0x00210a05, 0xd0003fc0, 0x17c07c1f, 0xe2e0000d, 0xe2e0020d, 0xe2e0060d,
    0xe2e00e0f, 0xe2e00e1e, 0xe2e00e12, 0xf0000000, 0x17c07c1f, 0xa1d10407,
    0x1b80001f, 0x20000014, 0xf0000000, 0x17c07c1f, 0xa1d08407, 0x1b80001f,
    0x20000280, 0x82eab401, 0x1a00001f, 0x10006814, 0xe200000b, 0xf0000000,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0xe8208000, 0x10006354, 0xffff00fd, 0x1890001f,
    0x10006600, 0x80c08801, 0xd82041c3, 0x17c07c1f, 0x1900001f, 0x10006284,
    0xc1004ee0, 0x1211841f, 0xc10050a0, 0x17c07c1f, 0x80c10801, 0xd82042e3,
    0x17c07c1f, 0x1900001f, 0x10006280, 0xc1004ee0, 0x1211841f, 0xc10050a0,
    0x17c07c1f, 0x1890001f, 0x10006600, 0x80800801, 0x18d0001f, 0x10001050,
    0x8a000003, 0xfffffeff, 0x1900001f, 0x10001050, 0xe1000008, 0x18d0001f,
    0x10001050, 0xa0d40803, 0xe1000003, 0xe8208000, 0x10006314, 0x0b160060,
    0xd8004a85, 0x17c07c1f, 0x18c0001f, 0x10006208, 0x1212841f, 0xc0c04b00,
    0x12807c1f, 0x1900001f, 0x10006248, 0x1a80001f, 0x00000020, 0xc1004e00,
    0x17c07c1f, 0x1900001f, 0x10006250, 0x1a80001f, 0x00000008, 0xc1004e00,
    0x17c07c1f, 0x1900001f, 0x10006258, 0x1a80001f, 0x00000004, 0xc1004e00,
    0x17c07c1f, 0x1b80001f, 0x20000080, 0xc0c04b00, 0x1280041f, 0x18c0001f,
    0x10006200, 0x1212841f, 0xc0c04b00, 0x12807c1f, 0x1900001f, 0x1000625c,
    0x1a80001f, 0x00000010, 0xc1004d00, 0x17c07c1f, 0x1b80001f, 0x20000080,
    0xc0c04b00, 0x1280041f, 0x19c0001f, 0x00215800, 0x10007c1f, 0xf0000000,
    0xd8004c0a, 0x17c07c1f, 0xe2e00036, 0x1380201f, 0xe2e0003e, 0x1380201f,
    0xe2e0002e, 0x1380201f, 0xd8204cca, 0x17c07c1f, 0xe2e0006e, 0xe2e0004e,
    0xe2e0004c, 0xe2e0004d, 0xf0000000, 0x17c07c1f, 0x1a00001f, 0xffffffff,
    0x1210a01f, 0xe2c00008, 0xd8004d4a, 0x02a0040a, 0xf0000000, 0x17c07c1f,
    0x12007c1f, 0xa210a001, 0xe2c00008, 0xd8004e2a, 0x02a0040a, 0xf0000000,
    0x17c07c1f, 0xe2e00e16, 0x1380201f, 0xe2e00e1e, 0x1380201f, 0xe2e00e0e,
    0x1380201f, 0xe2e00e0c, 0xe2e00e0d, 0xe2e00e0d, 0xe2e00c0d, 0xe2e0080d,
    0xe2e0000d, 0xf0000000, 0x17c07c1f, 0xe2e0000d, 0xe2e0020d, 0xe2e0060d,
    0xe2e00e0f, 0xe2e00e1e, 0xe2e00e12, 0xf0000000, 0x17c07c1f
};
#define PCM_SUSPEND_BASE        __pa(spm_pcm_suspend)
#define PCM_SUSPEND_LEN         (653 - 1)
#define PCM_SUSPEND_VEC0        EVENT_VEC(11, 1, 0, 0)      /* MD-wake event */
#define PCM_SUSPEND_VEC1        EVENT_VEC(12, 1, 0, 123)    /* MD-sleep event */
#else
static const u32 spm_pcm_suspend[] = {
    0x19c0001f, 0x003c0bd7, 0x1800001f, 0xf7cf1f3d, 0x1b80001f, 0x20000000,
    0x1800001f, 0xf7cf1f14, 0x19c0001f, 0x003c0bc7, 0x1a00001f, 0x2000000a,
    0xd8000426, 0x17c07c1f, 0x18c0001f, 0x10006234, 0xe0e00e16, 0x1380201f,
    0xe0e00e1e, 0x1380201f, 0xe0e00e0e, 0xe0e00e0f, 0xe0e00e0d, 0xe0e00e0d,
    0xe0e00c0d, 0xe0e0080d, 0xe0e0000d, 0x18c0001f, 0x10006240, 0xe0e00f16,
    0xe0e00f1e, 0xe0e00f0e, 0xe0e00f0f, 0xe8208000, 0x10006354, 0xfffffffd,
    0xe8208000, 0x1000627c, 0x00000005, 0x1b80001f, 0x20000020, 0xe8208000,
    0x1000627c, 0x00000009, 0x19c0001f, 0x003c0b87, 0x1b80001f, 0x20000030,
    0x1800001f, 0xc7cf1f14, 0x1b80001f, 0x200016a8, 0x1800001f, 0xc7ce1f14,
    0x1b80001f, 0x20000424, 0x1800001f, 0xc78e1f14, 0x1b80001f, 0x20000424,
    0x1800001f, 0xc38e1f14, 0x1b80001f, 0x20000424, 0x1800001f, 0xc38e1e14,
    0x1800001f, 0xc38e1e10, 0x1b80001f, 0x2000000a, 0x18c0001f, 0x10006240,
    0xe0e00f0d, 0x19c0001f, 0x002c0b87, 0x19c0001f, 0x002c0b85, 0xe8208000,
    0x10006354, 0xffff00fd, 0xe8208000, 0x10006314, 0x0b160060, 0x19c0001f,
    0x002d0b85, 0x1890001f, 0x10006600, 0x1900001f, 0x10006910, 0x80c08801,
    0x82410801, 0xd8200be3, 0x17c07c1f, 0x1900001f, 0x10006284, 0x1212041f,
    0xe120001e, 0x1380201f, 0xe120000c, 0x1380201f, 0xe120000d, 0x1380201f,
    0xe120001f, 0xe1200012, 0xd8200dc9, 0x17c07c1f, 0x1900001f, 0x10006280,
    0xd0000be0, 0x12407c1f, 0x80c00801, 0x1910001f, 0x10001050, 0x89000004,
    0xfffffeff, 0xa0d40c04, 0x1900001f, 0x10001050, 0xe1000003, 0x1b00001f,
    0x3ffff7ff, 0xf0000000, 0x17c07c1f, 0x1b00001f, 0x3fffe7ff, 0x1b80001f,
    0x20000004, 0x8080b401, 0xb0823441, 0xd8001b2c, 0xa080080c, 0x1b00001f,
    0x3ffff7ff, 0xd8001b22, 0x17c07c1f, 0xe8208000, 0x10006314, 0x0b160068,
    0x18d0001f, 0x10001050, 0x88c00003, 0xfffffeff, 0x1900001f, 0x10001050,
    0xe1000003, 0x18d0001f, 0x10001050, 0xe8208000, 0x10006354, 0xffffffff,
    0x19c0001f, 0x00240b05, 0x1b80001f, 0x2000000a, 0x19c0001f, 0x00240b07,
    0x1b80001f, 0x20000100, 0x8880000d, 0x00700000, 0x68a00002, 0x00700000,
    0xd80015c2, 0x17c07c1f, 0x1880001f, 0x10006320, 0xe080000f, 0x1880001f,
    0x10006814, 0xe0800001, 0x1b00001f, 0x3fffe7ff, 0xd0001b20, 0x17c07c1f,
    0x19c0001f, 0x003c0bc7, 0x1b80001f, 0x20000030, 0xe8208000, 0x1000627c,
    0x00000005, 0xe8208000, 0x1000627c, 0x00000004, 0xd80017e6, 0x17c07c1f,
    0x18c0001f, 0x10006240, 0xe0e00f0f, 0xe0e00f1e, 0xe0e00f12, 0x1800001f,
    0xc38e1e34, 0x1800001f, 0xc38e1f34, 0x1800001f, 0xc7ce1f34, 0x1800001f,
    0xc7cf1f34, 0xd8001a26, 0x17c07c1f, 0x18c0001f, 0x10006234, 0xe0e00e0f,
    0xe0e00e1e, 0xe0e00e12, 0xe8208000, 0x10006234, 0x000f0e12, 0x19c0001f,
    0x003c2bd7, 0x1800001f, 0xf7ff1f3d, 0x19c0001f, 0x001823d7, 0x1b00001f,
    0x3fffefff, 0xf0000000, 0x17c07c1f, 0xe2e00e16, 0x1380201f, 0xe2e00e1e,
    0x1380201f, 0xe2e00e0e, 0x1380201f, 0xe2e00e0f, 0xe2e00e0d, 0xe2e00e0d,
    0xe2e00c0d, 0xe2e0080d, 0xe2e0000d, 0xf0000000, 0x17c07c1f, 0xd8001daa,
    0x17c07c1f, 0xe2e0006d, 0xe2e0002d, 0xd8201e4a, 0x17c07c1f, 0xe2e0002f,
    0xe2e0003e, 0xe2e00032, 0xf0000000, 0x17c07c1f, 0x1a00001f, 0xffffffff,
    0x1210a01f, 0xe2c00008, 0xd8001eca, 0x02a0040a, 0xf0000000, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001,
    0xa1d48407, 0x1990001f, 0x10006400, 0x1b00001f, 0x3fffe7ff, 0x1b80001f,
    0xd00f0000, 0x8880000c, 0x3effe7ff, 0xd8003fc2, 0x1140041f, 0x809c040c,
    0xd80020a2, 0x8098040d, 0xc0c03de0, 0x81471801, 0xd80026a5, 0x17c07c1f,
    0x18c0001f, 0x10006200, 0xc0c01d20, 0x12807c1f, 0x1900001f, 0x1000625c,
    0xe1307fff, 0xc0c01d20, 0x1280041f, 0x18c0001f, 0x10006208, 0xc0c01d20,
    0x12807c1f, 0x1900001f, 0x10006248, 0x1a80001f, 0x00000020, 0xc1001e80,
    0x17c07c1f, 0x1900001f, 0x10006250, 0x1a80001f, 0x00000008, 0xc1001e80,
    0x17c07c1f, 0x1900001f, 0x10006258, 0x1a80001f, 0x00000004, 0xc1001e80,
    0x17c07c1f, 0xc0c01d20, 0x1280041f, 0xa1d40407, 0x1b80001f, 0x20000008,
    0xa1d90407, 0x81879801, 0x1a10001f, 0x10006608, 0x8249a001, 0x828a2001,
    0x1b00001f, 0x3fffefff, 0x1b80001f, 0xd0010000, 0x8098840d, 0x80c0b40a,
    0xb0c23469, 0xa0800c02, 0xd82029c2, 0x17c07c1f, 0x8880000c, 0x3effe7ff,
    0xd8003bc2, 0x17c07c1f, 0xd00027c0, 0x17c07c1f, 0xe8208000, 0x10006310,
    0x0b160038, 0xe8208000, 0x10006314, 0x0b160068, 0x18d0001f, 0x10001050,
    0x88c00003, 0xfffffeff, 0x1900001f, 0x10001050, 0xe1000003, 0x18d0001f,
    0x10001050, 0x19c0001f, 0x00240b01, 0x1880001f, 0x10006320, 0xe8208000,
    0x10006354, 0xffffffff, 0xc0c03e80, 0xe080000f, 0xd80027c3, 0x17c07c1f,
    0xe080001f, 0x19c0001f, 0x003c0bc7, 0x1b80001f, 0x20000030, 0xe8208000,
    0x1000627c, 0x00000005, 0xe8208000, 0x1000627c, 0x00000004, 0xd8002f26,
    0x17c07c1f, 0x18c0001f, 0x10006240, 0xc0c03ce0, 0x17c07c1f, 0x1800001f,
    0x00000034, 0x1800001f, 0x00001f34, 0x1800001f, 0x87c01f34, 0x1800001f,
    0xc7cf1f34, 0xd8003146, 0x17c07c1f, 0x18c0001f, 0x10006234, 0xc0c03ce0,
    0x17c07c1f, 0xe8208000, 0x10006234, 0x000f0e12, 0x1b00001f, 0x3fffefff,
    0x19c0001f, 0x003c2bd7, 0x1800001f, 0xf7ff1f3d, 0x19c0001f, 0x001823d7,
    0x1b80001f, 0x90100000, 0x80881c01, 0xd8003a22, 0x17c07c1f, 0x19c0001f,
    0x003c0bd7, 0x1800001f, 0xf7cf1f3d, 0x1b80001f, 0x20000000, 0x1800001f,
    0xf7cf1f14, 0x19c0001f, 0x003c0bc7, 0x18c0001f, 0x10006240, 0x1900001f,
    0x10006234, 0xd80035a6, 0x17c07c1f, 0xc1001b60, 0x1211841f, 0xe0e00f16,
    0xe0e00f1e, 0xe0e00f0e, 0xe0e00f0f, 0xe8208000, 0x1000627c, 0x00000005,
    0x1b80001f, 0x20000020, 0xe8208000, 0x1000627c, 0x00000009, 0x19c0001f,
    0x003c0b87, 0x1800001f, 0xc7cf1f14, 0x1b80001f, 0x200016a8, 0x1800001f,
    0x87c01f14, 0x1b80001f, 0x20000424, 0x1800001f, 0x04001f14, 0x1b80001f,
    0x20000424, 0x1800001f, 0x00001f14, 0x1b80001f, 0x20000424, 0x1800001f,
    0x00000014, 0x10007c1f, 0x1b80001f, 0x2000000a, 0xe0e00f0d, 0x19c0001f,
    0x00240b07, 0xd8203bc2, 0x17c07c1f, 0x1800001f, 0x83801e10, 0x1b80001f,
    0x20000424, 0x1800001f, 0x00001e10, 0x1b80001f, 0x20000424, 0x1800001f,
    0x00000010, 0x10007c1f, 0x19c0001f, 0x00250b05, 0xe8208000, 0x10006354,
    0xffff00fd, 0x19c0001f, 0x00210b05, 0x19c0001f, 0x00210a05, 0xd0003fc0,
    0x17c07c1f, 0xe2e0000d, 0xe2e0020d, 0xe2e0060d, 0xe2e00e0f, 0xe2e00e1e,
    0xe2e00e12, 0xf0000000, 0x17c07c1f, 0xa1d10407, 0x1b80001f, 0x20000014,
    0xf0000000, 0x17c07c1f, 0xa1d08407, 0x1b80001f, 0x20000280, 0x82eab401,
    0x1a00001f, 0x10006814, 0xe200000b, 0xf0000000, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0xe8208000, 0x10006354, 0xffff00fd, 0x1890001f,
    0x10006600, 0x80c08801, 0xd82041c3, 0x17c07c1f, 0x1900001f, 0x10006284,
    0xc1004ee0, 0x1211841f, 0xc10050a0, 0x17c07c1f, 0x80c10801, 0xd82042e3,
    0x17c07c1f, 0x1900001f, 0x10006280, 0xc1004ee0, 0x1211841f, 0xc10050a0,
    0x17c07c1f, 0x1890001f, 0x10006600, 0x80800801, 0x18d0001f, 0x10001050,
    0x8a000003, 0xfffffeff, 0x1900001f, 0x10001050, 0xe1000008, 0x18d0001f,
    0x10001050, 0xa0d40803, 0xe1000003, 0xe8208000, 0x10006314, 0x0b160060,
    0xd8004a85, 0x17c07c1f, 0x18c0001f, 0x10006208, 0x1212841f, 0xc0c04b00,
    0x12807c1f, 0x1900001f, 0x10006248, 0x1a80001f, 0x00000020, 0xc1004e00,
    0x17c07c1f, 0x1900001f, 0x10006250, 0x1a80001f, 0x00000008, 0xc1004e00,
    0x17c07c1f, 0x1900001f, 0x10006258, 0x1a80001f, 0x00000004, 0xc1004e00,
    0x17c07c1f, 0x1b80001f, 0x20000080, 0xc0c04b00, 0x1280041f, 0x18c0001f,
    0x10006200, 0x1212841f, 0xc0c04b00, 0x12807c1f, 0x1900001f, 0x1000625c,
    0x1a80001f, 0x00000010, 0xc1004d00, 0x17c07c1f, 0x1b80001f, 0x20000080,
    0xc0c04b00, 0x1280041f, 0x19c0001f, 0x00215800, 0x10007c1f, 0xf0000000,
    0xd8004c0a, 0x17c07c1f, 0xe2e00036, 0x1380201f, 0xe2e0003e, 0x1380201f,
    0xe2e0002e, 0x1380201f, 0xd8204cca, 0x17c07c1f, 0xe2e0006e, 0xe2e0004e,
    0xe2e0004c, 0xe2e0004d, 0xf0000000, 0x17c07c1f, 0x1a00001f, 0xffffffff,
    0x1210a01f, 0xe2c00008, 0xd8004d4a, 0x02a0040a, 0xf0000000, 0x17c07c1f,
    0x12007c1f, 0xa210a001, 0xe2c00008, 0xd8004e2a, 0x02a0040a, 0xf0000000,
    0x17c07c1f, 0xe2e00e16, 0x1380201f, 0xe2e00e1e, 0x1380201f, 0xe2e00e0e,
    0x1380201f, 0xe2e00e0c, 0xe2e00e0d, 0xe2e00e0d, 0xe2e00c0d, 0xe2e0080d,
    0xe2e0000d, 0xf0000000, 0x17c07c1f, 0xe2e0000d, 0xe2e0020d, 0xe2e0060d,
    0xe2e00e0f, 0xe2e00e1e, 0xe2e00e12, 0xf0000000, 0x17c07c1f
};
#define PCM_SUSPEND_BASE        __pa(spm_pcm_suspend)
#define PCM_SUSPEND_LEN         (653 - 1)
#define PCM_SUSPEND_VEC0        EVENT_VEC(11, 1, 0, 0)      /* MD-wake event */
#define PCM_SUSPEND_VEC1        EVENT_VEC(12, 1, 0, 123)    /* MD-sleep event */
#endif


/**********************************************************
 * PCM code for deep idle (v23rc3 @ 2013-01-16)
 **********************************************************/
#if defined(MTK_ENABLE_MD2) && defined(MODEM2_3G)   /* for AST */
static const u32 spm_pcm_dpidle[] = {
    0x1800001f, 0xc7cf1f34, 0xe8208000, 0x10006354, 0xffff00fd, 0x19c0001f,
    0x00244b07, 0x18c0001f, 0x00000005, 0x1a00001f, 0x10006604, 0xe2000003,
    0x1b80001f, 0x20000080, 0x1800001f, 0x87c21f14, 0x1b80001f, 0x20000424,
    0x1800001f, 0x04821f14, 0x1b80001f, 0x20000424, 0x1800001f, 0x00821f14,
    0x1b80001f, 0x20000424, 0x1800001f, 0x00820214, 0x1800001f, 0x00820210,
    0x19c0001f, 0x00204b07, 0x19c0001f, 0x00204b05, 0xe8208000, 0x10006314,
    0x0b160060, 0x19c0001f, 0x00214a05, 0x1b00001f, 0x3ffff7ff, 0xf0000000,
    0x17c07c1f, 0x1b00001f, 0x3fffe7ff, 0x1b80001f, 0x20000004, 0x8090840d,
    0xb092044d, 0xd8000c2c, 0x17c07c1f, 0x1b00001f, 0x3ffff7ff, 0xd8000c22,
    0x17c07c1f, 0xe8208000, 0x10006314, 0x0b160068, 0xe8208000, 0x10006354,
    0xfffffffd, 0x19c0001f, 0x00244b27, 0x1b80001f, 0x20000100, 0x808ab401,
    0xd80009c2, 0x17c07c1f, 0x1880001f, 0x10006320, 0xe080000f, 0x1880001f,
    0x10006814, 0xe0800001, 0x1b00001f, 0x3fffe7ff, 0xd0000c20, 0x17c07c1f,
    0x1800001f, 0x00820234, 0x1800001f, 0x00821f34, 0x1800001f, 0x87c21f34,
    0x1800001f, 0xc7cf1f34, 0x18c0001f, 0x00000007, 0x1a00001f, 0x10006604,
    0xe2000003, 0x1b80001f, 0x20000080, 0x1800001f, 0xc7cf1f1c, 0x1b00001f,
    0x3fffefff, 0xf0000000, 0x17c07c1f, 0xe2e00f16, 0x1380201f, 0xe2e00f1e,
    0x1380201f, 0xe2e00f0e, 0x1380201f, 0xe2e00f0c, 0xe2e00f0d, 0xe2e00e0d,
    0xe2e00c0d, 0xe2e0080d, 0xe2e0000d, 0xf0000000, 0x17c07c1f, 0xe2e0010d,
    0xe2e0030d, 0xe2e0070d, 0xe2e00f0d, 0xe2e00f1e, 0xe2e00f12, 0xf0000000,
    0x17c07c1f, 0xd8000fca, 0x17c07c1f, 0xe2e00036, 0xe2e0003e, 0xe2e0002e,
    0xd820108a, 0x17c07c1f, 0xe2e0006e, 0xe2e0004e, 0xe2e0004c, 0xe2e0004d,
    0xf0000000, 0x17c07c1f, 0xd800114a, 0x17c07c1f, 0xe2e0006d, 0xe2e0002d,
    0xd82011ea, 0x17c07c1f, 0xe2e0002f, 0xe2e0003e, 0xe2e00032, 0xf0000000,
    0x17c07c1f, 0x12007c1f, 0xa210a001, 0xe2c00008, 0xd800124a, 0x02a0040a,
    0xf0000000, 0x17c07c1f, 0x1a00001f, 0xffffffff, 0x1210a01f, 0xe2c00008,
    0xd800134a, 0x02a0040a, 0xf0000000, 0x17c07c1f, 0xa1d10407, 0x1b80001f,
    0x20000014, 0x12c07c1f, 0xf0000000, 0x17c07c1f, 0xa1d08407, 0x1b80001f,
    0x20000280, 0x82eab401, 0x1a00001f, 0x10006814, 0xe200000b, 0xf0000000,
    0x17c07c1f, 0x12802c1f, 0x82ed3401, 0xd82016ca, 0x17c07c1f, 0x1a00001f,
    0x10006814, 0xe200000b, 0xf0000000, 0x17c07c1f, 0x1a00001f, 0x10006604,
    0xd800182b, 0x17c07c1f, 0xe2200007, 0x1b80001f, 0x20000020, 0xd820188b,
    0x17c07c1f, 0xe2200005, 0x1b80001f, 0x20000020, 0xf0000000, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001,
    0xa1d48407, 0x1b00001f, 0x3fffefff, 0x1b80001f, 0xd0010000, 0x8098840d,
    0x80d0840d, 0xb0d2046d, 0xa0800c02, 0xd8202322, 0x17c07c1f, 0x8880000c,
    0x3effe7ff, 0xd8003c22, 0x17c07c1f, 0x809c040c, 0xd8002062, 0x8098040d,
    0xd82022e2, 0x17c07c1f, 0x1014841f, 0xd0002060, 0x17c07c1f, 0xe8208000,
    0x10006310, 0x0b160038, 0xe8208000, 0x10006314, 0x0b160068, 0x1990001f,
    0x10006600, 0x1880001f, 0x10006320, 0x80c01801, 0x81009801, 0xa1d40407,
    0x1b80001f, 0x20000014, 0xa1d90407, 0xc0c01400, 0xe080000f, 0xd8002063,
    0x17c07c1f, 0xc0c014c0, 0xe080000f, 0xd8002063, 0x17c07c1f, 0xe080001f,
    0x19c0001f, 0x00244b27, 0x1950001f, 0x10006400, 0x80d70405, 0xd8002ba3,
    0x17c07c1f, 0x18c0001f, 0x10006200, 0xc0c010c0, 0x12807c1f, 0x1900001f,
    0x1000625c, 0x1a80001f, 0x00000010, 0xc1001220, 0x17c07c1f, 0xc0c010c0,
    0x1280041f, 0x18c0001f, 0x10006208, 0xc0c010c0, 0x12807c1f, 0x1900001f,
    0x10006248, 0x1a80001f, 0x00000020, 0xc1001300, 0x17c07c1f, 0x1900001f,
    0x10006250, 0x1a80001f, 0x00000008, 0xc1001300, 0x17c07c1f, 0x1900001f,
    0x10006258, 0x1a80001f, 0x00000004, 0xc1001300, 0x17c07c1f, 0xc0c010c0,
    0x1280041f, 0x18d0001f, 0x10000164, 0x88c00003, 0xfffffff8, 0x1900001f,
    0x10000164, 0xe1000003, 0x18d0001f, 0x10000164, 0x1800001f, 0x00000034,
    0x1800001f, 0x00001f34, 0x1800001f, 0x87c01f34, 0x1800001f, 0xc7cf1f34,
    0xc0c01700, 0x10c07c1f, 0x80c01801, 0xd8202ea3, 0x17c07c1f, 0x1800001f,
    0xc7cf1f3c, 0x1b00001f, 0x3fffefff, 0x1b80001f, 0x90100000, 0x80881c01,
    0xd80033a2, 0x17c07c1f, 0x1800001f, 0xc7cf1f34, 0x19c0001f, 0x00244b07,
    0xc0c01700, 0x10c0041f, 0x1800001f, 0x87c01f14, 0x1b80001f, 0x20000424,
    0x1800001f, 0x04001f14, 0x1b80001f, 0x20000424, 0x1800001f, 0x00001f14,
    0x1b80001f, 0x20000424, 0x1800001f, 0x00000014, 0x10007c1f, 0x18d0001f,
    0x10000164, 0xb8c08003, 0xfffffff8, 0x00000005, 0x1900001f, 0x10000164,
    0xe1000003, 0x18d0001f, 0x10000164, 0xd8203502, 0x17c07c1f, 0x1800001f,
    0x00800210, 0x1b80001f, 0x20000424, 0x1800001f, 0x00000210, 0x1b80001f,
    0x20000424, 0x1800001f, 0x00000010, 0x10007c1f, 0x19c0001f, 0x00254b04,
    0xe8208000, 0x10006354, 0xffff00fd, 0x19c0001f, 0x00214b04, 0x19c0001f,
    0x00214a04, 0xe8208000, 0x10006314, 0x0b160060, 0x1950001f, 0x10006400,
    0x80d70405, 0xd8003c23, 0x17c07c1f, 0x18c0001f, 0x10006208, 0xc0c00f20,
    0x12807c1f, 0x1900001f, 0x10006248, 0x1a80001f, 0x00000020, 0xc1001220,
    0x17c07c1f, 0x1900001f, 0x10006250, 0x1a80001f, 0x00000008, 0xc1001220,
    0x17c07c1f, 0x1900001f, 0x10006258, 0x1a80001f, 0x00000004, 0xc1001220,
    0x17c07c1f, 0x1b80001f, 0x20000080, 0xc0c00f20, 0x1280041f, 0x18c0001f,
    0x10006200, 0xc0c00f20, 0x12807c1f, 0x1900001f, 0x1000625c, 0x1a80001f,
    0x00000010, 0xc1001300, 0x17c07c1f, 0x1b80001f, 0x20000080, 0xc0c00f20,
    0x1280041f, 0x19c0001f, 0x00215800, 0x10007c1f, 0xf0000000
};
#define PCM_DPIDLE_BASE         __pa(spm_pcm_dpidle)
#define PCM_DPIDLE_LEN          (485 - 1)
#define PCM_DPIDLE_VEC0         EVENT_VEC(11, 1, 0, 0)      /* MD-wake event */
#define PCM_DPIDLE_VEC1         EVENT_VEC(12, 1, 0, 43)     /* MD-sleep event */
#else
static const u32 spm_pcm_dpidle[] = {
    0x1800001f, 0xc7cf1f34, 0xe8208000, 0x10006354, 0xffff00fd, 0x19c0001f,
    0x00244b07, 0x18c0001f, 0x00000005, 0x1a00001f, 0x10006604, 0xe2000003,
    0x1b80001f, 0x20000080, 0x1800001f, 0xc7ce1f14, 0x1b80001f, 0x20000424,
    0x1800001f, 0xc78e1f14, 0x1b80001f, 0x20000424, 0x1800001f, 0xc38e1f14,
    0x1b80001f, 0x20000424, 0x1800001f, 0xc38e1e14, 0x1800001f, 0xc38e1e10,
    0x19c0001f, 0x00244b07, 0x19c0001f, 0x00244b05, 0xe8208000, 0x10006314,
    0x0b160060, 0x19c0001f, 0x00254b05, 0x1b00001f, 0x3ffff7ff, 0xf0000000,
    0x17c07c1f, 0x1b00001f, 0x3fffe7ff, 0x1b80001f, 0x20000004, 0x8090840d,
    0xb092044d, 0xd8000c2c, 0x17c07c1f, 0x1b00001f, 0x3ffff7ff, 0xd8000c22,
    0x17c07c1f, 0xe8208000, 0x10006314, 0x0b160068, 0xe8208000, 0x10006354,
    0xfffffffd, 0x19c0001f, 0x00244b27, 0x1b80001f, 0x20000100, 0x808ab401,
    0xd80009c2, 0x17c07c1f, 0x1880001f, 0x10006320, 0xe080000f, 0x1880001f,
    0x10006814, 0xe0800001, 0x1b00001f, 0x3fffe7ff, 0xd0000c20, 0x17c07c1f,
    0x1800001f, 0xc38e1e34, 0x1800001f, 0xc38e1f34, 0x1800001f, 0xc7ce1f34,
    0x1800001f, 0xc7cf1f34, 0x18c0001f, 0x00000007, 0x1a00001f, 0x10006604,
    0xe2000003, 0x1b80001f, 0x20000080, 0x1800001f, 0xc7cf1f1c, 0x1b00001f,
    0x3fffefff, 0xf0000000, 0x17c07c1f, 0xe2e00f16, 0x1380201f, 0xe2e00f1e,
    0x1380201f, 0xe2e00f0e, 0x1380201f, 0xe2e00f0c, 0xe2e00f0d, 0xe2e00e0d,
    0xe2e00c0d, 0xe2e0080d, 0xe2e0000d, 0xf0000000, 0x17c07c1f, 0xe2e0010d,
    0xe2e0030d, 0xe2e0070d, 0xe2e00f0d, 0xe2e00f1e, 0xe2e00f12, 0xf0000000,
    0x17c07c1f, 0xd8000fca, 0x17c07c1f, 0xe2e00036, 0xe2e0003e, 0xe2e0002e,
    0xd820108a, 0x17c07c1f, 0xe2e0006e, 0xe2e0004e, 0xe2e0004c, 0xe2e0004d,
    0xf0000000, 0x17c07c1f, 0xd800114a, 0x17c07c1f, 0xe2e0006d, 0xe2e0002d,
    0xd82011ea, 0x17c07c1f, 0xe2e0002f, 0xe2e0003e, 0xe2e00032, 0xf0000000,
    0x17c07c1f, 0x12007c1f, 0xa210a001, 0xe2c00008, 0xd800124a, 0x02a0040a,
    0xf0000000, 0x17c07c1f, 0x1a00001f, 0xffffffff, 0x1210a01f, 0xe2c00008,
    0xd800134a, 0x02a0040a, 0xf0000000, 0x17c07c1f, 0xa1d10407, 0x1b80001f,
    0x20000014, 0x12c07c1f, 0xf0000000, 0x17c07c1f, 0xa1d08407, 0x1b80001f,
    0x20000280, 0x82eab401, 0x1a00001f, 0x10006814, 0xe200000b, 0xf0000000,
    0x17c07c1f, 0x12802c1f, 0x82ed3401, 0xd82016ca, 0x17c07c1f, 0x1a00001f,
    0x10006814, 0xe200000b, 0xf0000000, 0x17c07c1f, 0x1a00001f, 0x10006604,
    0xd800182b, 0x17c07c1f, 0xe2200007, 0x1b80001f, 0x20000020, 0xd820188b,
    0x17c07c1f, 0xe2200005, 0x1b80001f, 0x20000020, 0xf0000000, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f,
    0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x17c07c1f, 0x1840001f, 0x00000001,
    0xa1d48407, 0x1b00001f, 0x3fffefff, 0x1b80001f, 0xd0010000, 0x8098840d,
    0x80d0840d, 0xb0d2046d, 0xa0800c02, 0xd8202322, 0x17c07c1f, 0x8880000c,
    0x3effe7ff, 0xd8003c22, 0x17c07c1f, 0x809c040c, 0xd8002062, 0x8098040d,
    0xd82022e2, 0x17c07c1f, 0x1014841f, 0xd0002060, 0x17c07c1f, 0xe8208000,
    0x10006310, 0x0b160038, 0xe8208000, 0x10006314, 0x0b160068, 0x1990001f,
    0x10006600, 0x1880001f, 0x10006320, 0x80c01801, 0x81009801, 0xa1d40407,
    0x1b80001f, 0x20000014, 0xa1d90407, 0xc0c01400, 0xe080000f, 0xd8002063,
    0x17c07c1f, 0xc0c014c0, 0xe080000f, 0xd8002063, 0x17c07c1f, 0xe080001f,
    0x19c0001f, 0x00244b27, 0x1950001f, 0x10006400, 0x80d70405, 0xd8002ba3,
    0x17c07c1f, 0x18c0001f, 0x10006200, 0xc0c010c0, 0x12807c1f, 0x1900001f,
    0x1000625c, 0x1a80001f, 0x00000010, 0xc1001220, 0x17c07c1f, 0xc0c010c0,
    0x1280041f, 0x18c0001f, 0x10006208, 0xc0c010c0, 0x12807c1f, 0x1900001f,
    0x10006248, 0x1a80001f, 0x00000020, 0xc1001300, 0x17c07c1f, 0x1900001f,
    0x10006250, 0x1a80001f, 0x00000008, 0xc1001300, 0x17c07c1f, 0x1900001f,
    0x10006258, 0x1a80001f, 0x00000004, 0xc1001300, 0x17c07c1f, 0xc0c010c0,
    0x1280041f, 0x18d0001f, 0x10000164, 0x88c00003, 0xfffffff8, 0x1900001f,
    0x10000164, 0xe1000003, 0x18d0001f, 0x10000164, 0x1800001f, 0x00000034,
    0x1800001f, 0x00001f34, 0x1800001f, 0x87c01f34, 0x1800001f, 0xc7cf1f34,
    0xc0c01700, 0x10c07c1f, 0x80c01801, 0xd8202ea3, 0x17c07c1f, 0x1800001f,
    0xc7cf1f3c, 0x1b00001f, 0x3fffefff, 0x1b80001f, 0x90100000, 0x80881c01,
    0xd80033a2, 0x17c07c1f, 0x1800001f, 0xc7cf1f34, 0x19c0001f, 0x00244b07,
    0xc0c01700, 0x10c0041f, 0x1800001f, 0x87c01f14, 0x1b80001f, 0x20000424,
    0x1800001f, 0x04001f14, 0x1b80001f, 0x20000424, 0x1800001f, 0x00001f14,
    0x1b80001f, 0x20000424, 0x1800001f, 0x00000014, 0x10007c1f, 0x18d0001f,
    0x10000164, 0xb8c08003, 0xfffffff8, 0x00000005, 0x1900001f, 0x10000164,
    0xe1000003, 0x18d0001f, 0x10000164, 0xd8203502, 0x17c07c1f, 0x1800001f,
    0x83801e10, 0x1b80001f, 0x20000424, 0x1800001f, 0x00001e10, 0x1b80001f,
    0x20000424, 0x1800001f, 0x00000010, 0x10007c1f, 0x19c0001f, 0x00254b04,
    0xe8208000, 0x10006354, 0xffff00fd, 0x19c0001f, 0x00214b04, 0x19c0001f,
    0x00214a04, 0xe8208000, 0x10006314, 0x0b160060, 0x1950001f, 0x10006400,
    0x80d70405, 0xd8003c23, 0x17c07c1f, 0x18c0001f, 0x10006208, 0xc0c00f20,
    0x12807c1f, 0x1900001f, 0x10006248, 0x1a80001f, 0x00000020, 0xc1001220,
    0x17c07c1f, 0x1900001f, 0x10006250, 0x1a80001f, 0x00000008, 0xc1001220,
    0x17c07c1f, 0x1900001f, 0x10006258, 0x1a80001f, 0x00000004, 0xc1001220,
    0x17c07c1f, 0x1b80001f, 0x20000080, 0xc0c00f20, 0x1280041f, 0x18c0001f,
    0x10006200, 0xc0c00f20, 0x12807c1f, 0x1900001f, 0x1000625c, 0x1a80001f,
    0x00000010, 0xc1001300, 0x17c07c1f, 0x1b80001f, 0x20000080, 0xc0c00f20,
    0x1280041f, 0x19c0001f, 0x00215800, 0x10007c1f, 0xf0000000
};
#define PCM_DPIDLE_BASE         __pa(spm_pcm_dpidle)
#define PCM_DPIDLE_LEN          (485 - 1)
#define PCM_DPIDLE_VEC0         EVENT_VEC(11, 1, 0, 0)      /* MD-wake event */
#define PCM_DPIDLE_VEC1         EVENT_VEC(12, 1, 0, 43)     /* MD-sleep event */
#endif


/**************************************
 * SW code for suspend and deep idle
 **************************************/
#define SPM_SYSCLK_SETTLE       99      /* 3ms */

#define WAIT_UART_ACK_TIMES     10      /* 10 * 10us */

#define SPM_WAKE_PERIOD         600     /* sec */

#define WAKE_SRC_FOR_SUSPEND                                                \
    (WAKE_SRC_KP | WAKE_SRC_EINT | WAKE_SRC_CCIF_MD2 | WAKE_SRC_CCIF_MD1 |  \
     WAKE_SRC_USB0_CD | WAKE_SRC_USB1_CD | WAKE_SRC_PWRAP |                 \
     WAKE_SRC_SYSPWREQ | WAKE_SRC_MD_WDT)

#define WAKE_SRC_FOR_DPIDLE                                                 \
    (WAKE_SRC_KP | WAKE_SRC_GPT | WAKE_SRC_EINT | WAKE_SRC_CCIF_MD2 |       \
     WAKE_SRC_CCIF_MD1 | WAKE_SRC_USB0_CD | WAKE_SRC_USB1_CD |              \
     WAKE_SRC_USB1_PDN | WAKE_SRC_USB0_PDN | WAKE_SRC_AFE |                 \
     WAKE_SRC_PWRAP | WAKE_SRC_SYSPWREQ | WAKE_SRC_MD_WDT)

#define wfi_with_sync()                         \
do {                                            \
    isb();                                      \
    dsb();                                      \
    __asm__ __volatile__("wfi" : : : "memory"); \
} while (0)

#define spm_crit2(fmt, args...)     \
do {                                \
    aee_sram_printk(fmt, ##args);   \
    spm_crit(fmt, ##args);          \
} while (0)

#define spm_error2(fmt, args...)    \
do {                                \
    aee_sram_printk(fmt, ##args);   \
    spm_error(fmt, ##args);         \
} while (0)

typedef struct {
    u32 debug_reg;      /* PCM_REG_DATA_INI */
    u32 r12;            /* PCM_REG12_DATA */
    u32 raw_sta;        /* SLEEP_ISR_RAW_STA */
    u32 cpu_wake;       /* SLEEP_CPU_WAKEUP_EVENT */
    u32 timer_out;      /* PCM_TIMER_OUT */
    u32 event_reg;      /* PCM_EVENT_REG_STA */
    u32 isr;            /* SLEEP_ISR_STATUS */
    u32 r13;            /* PCM_REG13_DATA */
} wake_status_t;

extern int get_dynamic_period(int first_use, int first_wakeup_time, int battery_capacity_level);

extern void mtk_wdt_suspend(void);
extern void mtk_wdt_resume(void);

extern int mt_irq_mask_all(struct mtk_irq_mask *mask);
extern int mt_irq_mask_restore(struct mtk_irq_mask *mask);
extern void mt_irq_unmask_for_sleep(unsigned int irq);

extern void mtk_uart_restore(void);
extern void dump_uart_reg(void);

extern spinlock_t spm_lock;

static u32 spm_sleep_wakesrc = WAKE_SRC_FOR_SUSPEND;

static void spm_set_sysclk_settle(void)
{
    u32 md_settle, settle;

#if !defined(MTK_ENABLE_MD1) && defined(MTK_ENABLE_MD2)
    /* MD SYSCLK settle is from MD2 */
    spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) & ~CC_SYSSETTLE_SEL);
#else
    /* MD SYSCLK settle is from MD1 */
    spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) | CC_SYSSETTLE_SEL);
#endif

    /* get MD SYSCLK settle */
    spm_write(SPM_CLK_SETTLE, 0);
    md_settle = spm_read(SPM_CLK_SETTLE);

    /* SYSCLK settle = MD SYSCLK settle but set it again for MD PDN */
    spm_write(SPM_CLK_SETTLE, SPM_SYSCLK_SETTLE - md_settle);
    settle = spm_read(SPM_CLK_SETTLE);

    spm_crit2("md_settle = %u, settle = %u\n", md_settle, settle);
}

static void spm_reset_and_init_pcm(void)
{
    /* reset PCM */
    spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_PCM_SW_RESET);
    spm_write(SPM_PCM_CON0, CON0_CFG_KEY);

    /* init PCM control register (disable event vector and PCM timer) */
    spm_write(SPM_PCM_CON0, CON0_CFG_KEY | CON0_IM_SLEEP_DVS);
    spm_write(SPM_PCM_CON1, CON1_CFG_KEY | CON1_IM_NONRP_EN /*| CON1_MIF_APBEN*/);
}

/*
 * code_base: SPM_PCM_XXX_BASE
 * code_len : SPM_PCM_XXX_LEN
 */
static void spm_kick_im_to_fetch(u32 code_base, u16 code_len)
{
    u32 con0;

    /* tell IM where is PCM code */
    BUG_ON(code_base & 0x00000003);     /* check 4-byte alignment */
    spm_write(SPM_PCM_IM_PTR, code_base);
    spm_write(SPM_PCM_IM_LEN, code_len);

    /* kick IM to fetch (only toggle IM_KICK) */
    con0 = spm_read(SPM_PCM_CON0) & ~(CON0_IM_KICK | CON0_PCM_KICK);
    spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY | CON0_IM_KICK);
    spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY);
}

static int spm_request_uart_to_sleep(void)
{
    u32 val1;
    int i = 0;

    /* request UART to sleep */
    val1 = spm_read(SPM_POWER_ON_VAL1);
    spm_write(SPM_POWER_ON_VAL1, val1 | R7_UART_CLK_OFF_REQ);

    /* wait for UART to ACK */
    while (!(spm_read(SPM_PCM_REG13_DATA) & R13_UART_CLK_OFF_ACK)) {
        if (i++ >= WAIT_UART_ACK_TIMES) {
            spm_write(SPM_POWER_ON_VAL1, val1);
            spm_error2("CANNOT GET UART SLEEP ACK (0x%x)\n", spm_read(PERI_PDN0_STA));
            dump_uart_reg();
            return -EBUSY;
        }
        udelay(10);
    }

    return 0;
}

static void spm_init_pcm_register(void)
{
    /* init r0 with POWER_ON_VAL0 */
    spm_write(SPM_PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL0));
    spm_write(SPM_PCM_PWR_IO_EN, PCM_RF_SYNC_R0);
    spm_write(SPM_PCM_PWR_IO_EN, 0);

    /* init r7 with POWER_ON_VAL1 */
    spm_write(SPM_PCM_REG_DATA_INI, spm_read(SPM_POWER_ON_VAL1));
    spm_write(SPM_PCM_PWR_IO_EN, PCM_RF_SYNC_R7);
    spm_write(SPM_PCM_PWR_IO_EN, 0);

    /* clear REG_DATA_INI for PCM after init rX */
    spm_write(SPM_PCM_REG_DATA_INI, 0);
}

/*
 * vec0_cfg: SPM_PCM_XXX_VEC0 or 0
 * vec1_cfg: SPM_PCM_XXX_VEC1 or 0
 * vec2_cfg: SPM_PCM_XXX_VEC2 or 0
 * vec3_cfg: SPM_PCM_XXX_VEC3 or 0
 */
static void spm_init_event_vector(u32 vec0_cfg, u32 vec1_cfg, u32 vec2_cfg, u32 vec3_cfg)
{
    /* init event vector register */
    spm_write(SPM_PCM_EVENT_VECTOR0, vec0_cfg);
    spm_write(SPM_PCM_EVENT_VECTOR1, vec1_cfg);
    spm_write(SPM_PCM_EVENT_VECTOR2, vec2_cfg);
    spm_write(SPM_PCM_EVENT_VECTOR3, vec3_cfg);

    /* event vector will be enabled by PCM itself */
}

static void spm_set_pwrctl_for_sleep(void)
{
    u32 pwrctl = 0;

    if (spm_read(INFRA_DCMCTL) & (1U << 8))
        pwrctl |= (1U << 0);    /* input INFRA DCM info for INFRA PDN */
    if (mt_get_chip_sw_ver() == CHIP_SW_VER_01) {
        if (!(spm_read(SPM_MD1_PWR_CON) & (0x3 << 2)))
            pwrctl |= (1U << 1);    /* input MD1 PDN info for WHQA_00013637 */
        if (!(spm_read(SPM_MD2_PWR_CON) & (0x3 << 2)))
            pwrctl |= (1U << 2);    /* input MD2 PDN info for WHQA_00013637 */
    }
    spm_write(SPM_APMCU_PWRCTL, pwrctl);

    spm_write(SPM_AP_STANBY_CON, (0x3 << 19) |  /* unmask MD1 and MD2 */
                                 (0x3 << 16) |  /* unmask DISP and MFG */
                                 (0 << 6) |     /* check SCU idle */
                                 (0 << 5) |     /* check L2C idle */
                                 (1U << 4));    /* Reduce AND */
    spm_write(SPM_CORE0_CON, 0x1);
    spm_write(SPM_CORE1_CON, 0x1);
    spm_write(SPM_CORE2_CON, 0x1);
    spm_write(SPM_CORE3_CON, 0x1);
}

static void spm_set_pwrctl_for_dpidle(u8 pwrlevel)
{
    if (pwrlevel > 1)
        pwrlevel = 1;
    spm_write(SPM_APMCU_PWRCTL, 1U << pwrlevel);

    spm_write(SPM_AP_STANBY_CON, (0x3 << 19) |  /* unmask MD1 and MD2 */
                                 (0x3 << 16) |  /* unmask DISP and MFG */
                                 (0 << 6) |     /* check SCU idle */
                                 (0 << 5) |     /* check L2C idle */
                                 (1U << 4));    /* Reduce AND */
    spm_write(SPM_CORE0_CON, 0x1);
    spm_write(SPM_CORE1_CON, 0x1);
    spm_write(SPM_CORE2_CON, 0x1);
    spm_write(SPM_CORE3_CON, 0x1);
}

/*
 * timer_val: PCM timer value (0 = disable)
 * wake_src : WAKE_SRC_XXX
 */
static void spm_set_wakeup_event(u32 timer_val, u32 wake_src)
{
    /* set PCM timer (set to max when disable) */
    spm_write(SPM_PCM_TIMER_VAL, timer_val ? : 0xffffffff);
    spm_write(SPM_PCM_CON1, spm_read(SPM_PCM_CON1) | CON1_CFG_KEY | CON1_PCM_TIMER_EN);

    /* unmask wakeup source */
#if SPM_BYPASS_SYSPWREQ
    wake_src &= ~WAKE_SRC_SYSPWREQ;     /* make 26M off when attach ICE */
#endif
    spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, ~wake_src);

    /* unmask SPM ISR */
    spm_write(SPM_SLEEP_ISR_MASK, 0);
}

static void spm_kick_pcm_to_run(bool cpu_pdn, bool infra_pdn)
{
    u32 clk, con0;

    /* keep CPU or INFRA/DDRPHY power if needed and lock INFRA DCM */
    clk = spm_read(SPM_CLK_CON) & ~(CC_DISABLE_DORM_PWR | CC_DISABLE_INFRA_PWR);
    if (!cpu_pdn)
        clk |= CC_DISABLE_DORM_PWR;
    if (!infra_pdn)
        clk |= CC_DISABLE_INFRA_PWR;
    spm_write(SPM_CLK_CON, clk | CC_LOCK_INFRA_DCM);

    /* init pause request mask for PCM */
    spm_write(SPM_PCM_MAS_PAUSE_MASK, 0xffff00fd /*0xffffffff*/);

    /* enable r0 and r7 to control power */
    spm_write(SPM_PCM_PWR_IO_EN, PCM_PWRIO_EN_R0 | PCM_PWRIO_EN_R7 |
                                 PCM_PWRIO_EN_R1 | PCM_PWRIO_EN_R2);

    if (mt_get_chip_sw_ver() == CHIP_SW_VER_01) {
        /* SRCLKENA_PERI: SRCLKEN0|SRCLKEN1|SRCLKEN2|MD1_SRCLKEN|r7 (PWR_IO_EN[7]=1) */
        /* SRCLKENA_MD: SRCLKEN1|r7 (PWR_IO_EN[2]=1) */
        spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) |
                               CC_SYSCLK0_EN_1 | CC_SYSCLK0_EN_0 | CC_SYSCLK1_EN_1);
    } else {
        /* SRCLKENA_PERI: SRCLKEN0|SRCLKEN1|SRCLKEN2|r7 (PWR_IO_EN[7]=1) */
        /* SRCLKENA_MD: SRCLKEN1|r7 (PWR_IO_EN[2]=1) */
        spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) |
                               CC_SYSCLK0_EN_0 | CC_SYSCLK1_EN_1);
    }

    /* kick PCM to run (only toggle PCM_KICK) */
    con0 = spm_read(SPM_PCM_CON0) & ~(CON0_IM_KICK | CON0_PCM_KICK);
    spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY | CON0_PCM_KICK);
    spm_write(SPM_PCM_CON0, con0 | CON0_CFG_KEY);
}

static void spm_trigger_wfi_for_sleep(bool cpu_pdn, bool infra_pdn)
{
    disable_peri_dcm();     /* workaround for WHQA_00013158 */

    //if (infra_pdn)
    //    disable_infra_dcm();    /* PCM will do this */

    if (cpu_pdn) {
        if (!cpu_power_down(SHUTDOWN_MODE)) {
            switch_to_amp();
            wfi_with_sync();
        }
        switch_to_smp();
        cpu_check_dormant_abort();
    } else {
        mci_snoop_sleep();
        wfi_with_sync();
        mci_snoop_restore();
    }

    if (infra_pdn) {
        //restore_infra_dcm();    /* PCM has done this */
        mtk_uart_restore();
    }

    restore_peri_dcm();
}

static void spm_trigger_wfi_for_dpidle(bool cpu_pdn)
{
    disable_peri_dcm();     /* workaround for WHQA_00013158 */

    //pmicspi_mempll2clksq();     /* PCM will do this */

    if (cpu_pdn) {
        if (!cpu_power_down(DORMANT_MODE)) {
            switch_to_amp();
            wfi_with_sync();
        }
        switch_to_smp();
        cpu_check_dormant_abort();
    } else {
        mci_snoop_sleep();
        wfi_with_sync();
        mci_snoop_restore();
    }

    //pmicspi_clksq2mempll();     /* PCM has done this */

    restore_peri_dcm();
}

static void spm_get_wakeup_status(wake_status_t *wakesta)
{
    /* get PC value if PCM assert (pause abort) */
    wakesta->debug_reg = spm_read(SPM_PCM_REG_DATA_INI);

    /* get wakeup event */
    wakesta->r12 = spm_read(SPM_PCM_REG12_DATA);
    wakesta->raw_sta = spm_read(SPM_SLEEP_ISR_RAW_STA);
    wakesta->cpu_wake = spm_read(SPM_SLEEP_CPU_WAKEUP_EVENT);

    /* get sleep time */
    wakesta->timer_out = spm_read(SPM_PCM_TIMER_OUT);

    /* get special pattern if sleep abort */
    wakesta->event_reg = spm_read(SPM_PCM_EVENT_REG_STA);

    /* get ISR status */
    wakesta->isr = spm_read(SPM_SLEEP_ISR_STATUS);

    /* get MD related status */
    wakesta->r13 = spm_read(SPM_PCM_REG13_DATA);
}

static void spm_clean_after_wakeup(void)
{
    /* PCM has cleared uart_clk_off_req and now clear it in POWER_ON_VAL1 */
    spm_write(SPM_POWER_ON_VAL1, spm_read(SPM_POWER_ON_VAL1) & ~R7_UART_CLK_OFF_REQ);

    /* SRCLKENA_PERI: POWER_ON_VAL1|r7 (PWR_IO_EN[7]=1) */
    /* SRCLKENA_MD: POWER_ON_VAL1|r7 (PWR_IO_EN[2]=1) */
    spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) &
                           ~(CC_SYSCLK0_EN_1 | CC_SYSCLK0_EN_0 | CC_SYSCLK1_EN_1));

    /* re-enable POWER_ON_VAL0/1 to control power */
    spm_write(SPM_PCM_PWR_IO_EN, 0);

    /* unlock INFRA DCM */
    spm_write(SPM_CLK_CON, spm_read(SPM_CLK_CON) & ~CC_LOCK_INFRA_DCM);

    /* clean PCM timer event */
    spm_write(SPM_PCM_CON1, CON1_CFG_KEY | (spm_read(SPM_PCM_CON1) & ~CON1_PCM_TIMER_EN));

    /* clean CPU wakeup event (pause abort) */
    spm_write(SPM_SLEEP_CPU_WAKEUP_EVENT, 0);

    /* clean wakeup event raw status */
    spm_write(SPM_SLEEP_WAKEUP_EVENT_MASK, 0xffffffff);

    /* clean ISR status */
    spm_write(SPM_SLEEP_ISR_MASK, 0x0008);
    spm_write(SPM_SLEEP_ISR_STATUS, 0x0018);
}

static wake_reason_t spm_output_wake_reason(wake_status_t *wakesta, bool dpidle)
{
    char str[200] = { 0 };
    wake_reason_t wr = WR_NONE;

    if (wakesta->debug_reg != 0) {
        spm_error2("PCM ASSERT AND PC = %u (0x%x)(0x%x)\n",
                   wakesta->debug_reg, wakesta->r13, wakesta->event_reg);
        return WR_PCM_ASSERT;
    }

    if (dpidle)     /* bypass wakeup event check */
        return WR_WAKE_SRC;

    if (wakesta->r12 & (1U << 0)) {
        if (!wakesta->cpu_wake) {
            strcat(str, "PCM_TIMER ");
            wr = WR_PCM_TIMER;
        } else {
            strcat(str, "CPU ");
            wr = WR_WAKE_SRC;
        }
    }
    if (wakesta->r12 & WAKE_SRC_TS) {
        strcat(str, "TS ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_KP) {
        strcat(str, "KP ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_GPT) {
        strcat(str, "GPT ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_EINT) {
        strcat(str, "EINT ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_CCIF_MD2) {
        strcat(str, "CCIF_MD2 ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_CCIF_MD1) {
        strcat(str, "CCIF_MD1 ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_LOW_BAT) {
        strcat(str, "LOW_BAT ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_USB0_CD) {
        strcat(str, "USB0_CD ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_USB1_CD) {
        strcat(str, "USB1_CD ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_USB1_PDN) {
        strcat(str, "USB1_PDN ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_USB0_PDN) {
        strcat(str, "USB0_PDN ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_DBGSYS) {
        strcat(str, "DBGSYS ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_AFE) {
        strcat(str, "AFE ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_THERM) {
        strcat(str, "THERM ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_CIRQ) {
        strcat(str, "CIRQ ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_PWRAP) {
        strcat(str, "PWRAP ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_SYSPWREQ) {
        strcat(str, "SYSPWREQ ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_MD_WDT) {
        strcat(str, "MD_WDT ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_CPU0_IRQ) {
        strcat(str, "CPU0_IRQ ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_CPU1_IRQ) {
        strcat(str, "CPU1_IRQ ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_CPU2_IRQ) {
        strcat(str, "CPU2_IRQ ");
        wr = WR_WAKE_SRC;
    }
    if (wakesta->r12 & WAKE_SRC_CPU3_IRQ) {
        strcat(str, "CPU3_IRQ ");
        wr = WR_WAKE_SRC;
    }

    spm_crit2("wake up by %s(0x%x)(0x%x)(%u)\n",
              str, wakesta->r12, wakesta->raw_sta, wakesta->timer_out);
    spm_crit2("event_reg = 0x%x, isr = 0x%x, r13 = 0x%x\n",
              wakesta->event_reg, wakesta->isr, wakesta->r13);
    BUG_ON(wr == WR_NONE);

    if (wakesta->r12 & WAKE_SRC_EINT)
        mt_eint_print_status();
    if (wakesta->r12 & WAKE_SRC_CCIF_MD2)
        exec_ccci_kern_func_by_md_id(1, ID_GET_MD_WAKEUP_SRC, NULL, 0);
    if (wakesta->r12 & WAKE_SRC_CCIF_MD1)
        exec_ccci_kern_func_by_md_id(0, ID_GET_MD_WAKEUP_SRC, NULL, 0);

    return wr;
}

#if SPM_PWAKE_EN
static u32 spm_get_wake_period(wake_reason_t last_wr)
{
    int period = SPM_WAKE_PERIOD;

#if 1
    /* use FG to get the period of 1% battery decrease */
    period = get_dynamic_period(last_wr != WR_PCM_TIMER ? 1 : 0, SPM_WAKE_PERIOD, 1);
    if (period <= 0) {
        spm_warning("CANNOT GET PERIOD FROM FUEL GAUGE\n");
        period = SPM_WAKE_PERIOD;
    } else if (period > 36 * 3600) {    /* max period is 36.4 hours */
        period = 36 * 3600;
    }
#endif

    return period;
}
#endif

/*
 * wakesrc: WAKE_SRC_XXX
 * enable : enable or disable @wakesrc
 * replace: if true, will replace the default setting
 */
int spm_set_sleep_wakesrc(u32 wakesrc, bool enable, bool replace)
{
    unsigned long flags;

    if (spm_is_wakesrc_invalid(wakesrc))
        return -EINVAL;

    spin_lock_irqsave(&spm_lock, flags);
    if (enable) {
        if (replace)
            spm_sleep_wakesrc = wakesrc;
        else
            spm_sleep_wakesrc |= wakesrc;
    } else {
        if (replace)
            spm_sleep_wakesrc = 0;
        else
            spm_sleep_wakesrc &= ~wakesrc;
    }
    spin_unlock_irqrestore(&spm_lock, flags);

    return 0;
}

/*
 * cpu_pdn:
 *    true  = CPU shutdown
 *    false = CPU standby
 * infra_pdn:
 *    true  = INFRA/DDRPHY power down
 *    false = keep INFRA/DDRPHY power
 */
wake_reason_t spm_go_to_sleep(bool cpu_pdn, bool infra_pdn)
{
    u32 sec = 0;
    wake_status_t wakesta;
    unsigned long flags;
    struct mtk_irq_mask mask;
    static wake_reason_t last_wr = WR_NONE;

#if SPM_PWAKE_EN
    sec = spm_get_wake_period(last_wr);
#endif

    mtk_wdt_suspend();

    spin_lock_irqsave(&spm_lock, flags);
    mt_irq_mask_all(&mask);
    mt_irq_unmask_for_sleep(MT_SPM_IRQ_ID);
    mt_cirq_clone_gic();
    mt_cirq_enable();

    spm_set_sysclk_settle();

    spm_crit2("sec = %u, wakesrc = 0x%x (%u)(%u)\n",
              sec, spm_sleep_wakesrc, cpu_pdn, infra_pdn);

    spm_reset_and_init_pcm();

    spm_kick_im_to_fetch(PCM_SUSPEND_BASE, PCM_SUSPEND_LEN);

    if (spm_request_uart_to_sleep()) {
        last_wr = WR_UART_BUSY;
        goto RESTORE_IRQ;
    }

    spm_init_pcm_register();

    spm_init_event_vector(PCM_SUSPEND_VEC0, PCM_SUSPEND_VEC1, 0, 0);

    spm_set_pwrctl_for_sleep();

    spm_set_wakeup_event(sec * 32768, spm_sleep_wakesrc);

    spm_kick_pcm_to_run(cpu_pdn, infra_pdn);

    spm_trigger_wfi_for_sleep(cpu_pdn, infra_pdn);

    spm_get_wakeup_status(&wakesta);

    spm_clean_after_wakeup();

    last_wr = spm_output_wake_reason(&wakesta, false);

RESTORE_IRQ:
    mt_cirq_flush();
    mt_cirq_disable();
    mt_irq_mask_restore(&mask);
    spin_unlock_irqrestore(&spm_lock, flags);

    mtk_wdt_resume();

    return last_wr;
}

/*
 * cpu_pdn:
 *    true  = CPU dormant
 *    false = CPU standby
 * pwrlevel:
 *    0 = AXI is off
 *    1 = AXI is 26M
 */
wake_reason_t spm_go_to_sleep_dpidle(bool cpu_pdn, u8 pwrlevel)
{
    u32 sec = 0;
    wake_status_t wakesta;
    unsigned long flags;
    struct mtk_irq_mask mask;
    static wake_reason_t last_wr = WR_NONE;

#if SPM_PWAKE_EN
    sec = spm_get_wake_period(last_wr);
#endif

    mtk_wdt_suspend();

    spin_lock_irqsave(&spm_lock, flags);
    mt_irq_mask_all(&mask);
    mt_irq_unmask_for_sleep(MT_SPM_IRQ_ID);
    mt_cirq_clone_gic();
    mt_cirq_enable();

    spm_crit2("sec = %u, wakesrc = 0x%x [%u][%u]\n",
              sec, spm_sleep_wakesrc, cpu_pdn, pwrlevel);

    spm_reset_and_init_pcm();

    spm_kick_im_to_fetch(PCM_DPIDLE_BASE, PCM_DPIDLE_LEN);

    if (spm_request_uart_to_sleep()) {
        last_wr = WR_UART_BUSY;
        goto RESTORE_IRQ;
    }

    spm_init_pcm_register();

    spm_init_event_vector(PCM_DPIDLE_VEC0, PCM_DPIDLE_VEC1, 0, 0);

    spm_set_pwrctl_for_dpidle(pwrlevel);

    spm_set_wakeup_event(sec * 32768, spm_sleep_wakesrc);

    spm_kick_pcm_to_run(cpu_pdn, false);    /* keep INFRA/DDRPHY power */

    spm_trigger_wfi_for_dpidle(cpu_pdn);

    spm_get_wakeup_status(&wakesta);

    spm_clean_after_wakeup();

    last_wr = spm_output_wake_reason(&wakesta, false);

RESTORE_IRQ:
    mt_cirq_flush();
    mt_cirq_disable();
    mt_irq_mask_restore(&mask);
    spin_unlock_irqrestore(&spm_lock, flags);

    mtk_wdt_resume();

    return last_wr;
}

bool __attribute__((weak)) spm_dpidle_can_enter(void)
{
    return true;
}

void __attribute__((weak)) spm_dpidle_before_wfi(void)
{
}

void __attribute__((weak)) spm_dpidle_after_wfi(void)
{
}

/*
 * cpu_pdn:
 *    true  = CPU dormant
 *    false = CPU standby
 * pwrlevel:
 *    0 = AXI is off
 *    1 = AXI is 26M
 */
wake_reason_t spm_go_to_dpidle(bool cpu_pdn, u8 pwrlevel)
{
    wake_status_t wakesta;
    unsigned long flags;
    struct mtk_irq_mask mask;
    wake_reason_t wr = WR_NONE;

    spin_lock_irqsave(&spm_lock, flags);
    mt_irq_mask_all(&mask);
    mt_irq_unmask_for_sleep(MT_SPM_IRQ_ID);
    mt_cirq_clone_gic();
    mt_cirq_enable();

    spm_reset_and_init_pcm();

    spm_kick_im_to_fetch(PCM_DPIDLE_BASE, PCM_DPIDLE_LEN);

    if (spm_request_uart_to_sleep()) {
        wr = WR_UART_BUSY;
        goto RESTORE_IRQ;
    }

    spm_init_pcm_register();

    spm_init_event_vector(PCM_DPIDLE_VEC0, PCM_DPIDLE_VEC1, 0, 0);

    spm_set_pwrctl_for_dpidle(pwrlevel);

    spm_set_wakeup_event(0, WAKE_SRC_FOR_DPIDLE);

    if (spm_dpidle_can_enter()) {
        spm_kick_pcm_to_run(cpu_pdn, false);    /* keep INFRA/DDRPHY power */

        spm_dpidle_before_wfi();

        spm_trigger_wfi_for_dpidle(cpu_pdn);

        spm_dpidle_after_wfi();

        spm_get_wakeup_status(&wakesta);

        spm_clean_after_wakeup();

        wr = spm_output_wake_reason(&wakesta, true);
    } else {
        spm_clean_after_wakeup();

        wr = WR_SW_ABORT;
    }

RESTORE_IRQ:
    mt_cirq_flush();
    mt_cirq_disable();
    mt_irq_mask_restore(&mask);
    spin_unlock_irqrestore(&spm_lock, flags);

    return wr;
}

bool spm_is_md1_sleep(void)
{
    return !(spm_read(SPM_PCM_REG13_DATA) & R13_MD1_SRCCLKENI);
}

bool spm_is_md2_sleep(void)
{
    return !(spm_read(SPM_PCM_REG13_DATA) & R13_MD2_SRCCLKENI);
}

void spm_output_sleep_option(void)
{
#if defined(MTK_ENABLE_MD2) && defined(MODEM2_3G)   /* for AST */
    spm_notice("PWAKE_EN=%d, BYPASS_SYSPWREQ=%d, SW_VER=%d\n",
#else
    spm_notice("PWAKE_EN:%d, BYPASS_SYSPWREQ:%d, SW_VER:%d\n",
#endif
               SPM_PWAKE_EN, SPM_BYPASS_SYSPWREQ, mt_get_chip_sw_ver());
}

MODULE_AUTHOR("Terry Chang <terry.chang@mediatek.com>");
MODULE_DESCRIPTION("MT6589 SPM-Sleep Driver v2.2");
