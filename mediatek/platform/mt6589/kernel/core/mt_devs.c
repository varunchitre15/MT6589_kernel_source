#include <generated/autoconf.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/ioport.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/android_pmem.h>
//FIX-ME: marked for early porting
//#include <linux/android_vmem.h>
#include <asm/setup.h>
#include <asm/mach/arch.h>
#include <linux/sysfs.h>
#include <asm/io.h>
#include <linux/spi/spi.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/musbfsh.h>
#include "mach/memory.h"
#include "mach/irqs.h"
#include <mach/mt_reg_base.h>
#include <mach/devs.h>
#include <mach/mt_boot.h>
#include <linux/version.h>
#include "mach/mtk_ccci_helper.h"
#include <mach/mtk_memcfg.h>
//Begin_Arima_HCSW7_20130402_ChrisHe - define USB_UNIQUE_SERIAL
#ifndef ARIMA_PCBA_RELEASE
#define CONFIG_MTK_USB_UNIQUE_SERIAL
#endif
//End_Arima_HCSW7_20130402_ChrisHe - define USB_UNIQUE_SERIAL
#define SERIALNO_LEN 32
static char serial_number[SERIALNO_LEN];

extern u32 get_devinfo_with_index(u32 index);
extern BOOTMODE get_boot_mode(void);
extern u32 g_devinfo_data[];
extern u32 g_devinfo_data_size;
extern void adjust_kernel_cmd_line_setting_for_console(char*, char*);
unsigned int get_max_DRAM_size(void);

struct {
	u32 base;
	u32 size;
} bl_fb = {0, 0};

static int use_bl_fb = 0;

/*=======================================================================*/
/* MT6589 USB GADGET                                                     */
/*=======================================================================*/
static u64 usb_dmamask = DMA_BIT_MASK(32);

struct platform_device mt_device_usb = {
	.name		  = "mt_usb",
	.id		  = -1,
	.dev = {
		//.platform_data          = &usb_data_mt65xx,
		.dma_mask               = &usb_dmamask,
		.coherent_dma_mask      = DMA_BIT_MASK(32),
		//.release=musbfsh_hcd_release,
		},
};

/*=======================================================================*/
/* MT6589 USB11 Host                      */
/*=======================================================================*/
#if defined(CONFIG_MTK_USBFSH)
static u64 usb11_dmamask = DMA_BIT_MASK(32);
extern void musbfsh_hcd_release (struct device *dev);

static struct musbfsh_hdrc_config musbfsh_config_mt65xx = {
	.multipoint     = false,
	.dyn_fifo       = true,
	.soft_con       = true,
	.dma            = true,
	.num_eps        = 6,
	.dma_channels   = 4,
};
static struct musbfsh_hdrc_platform_data usb_data_mt65xx = {
	.mode           = 1,
	.config         = &musbfsh_config_mt65xx,
};
static struct platform_device mt_usb11_dev = {
	.name           = "musbfsh_hdrc",
	.id             = -1,
	.dev = {
		.platform_data          = &usb_data_mt65xx,
		.dma_mask               = &usb11_dmamask,
		.coherent_dma_mask      = DMA_BIT_MASK(32),
		.release		= musbfsh_hcd_release,
	},
};
#endif

/*=======================================================================*/
/* MT6589 UART Ports                                                     */
/*=======================================================================*/
#if defined(CFG_DEV_UART1)
static struct resource mtk_resource_uart1[] = {
	{
		.start		= IO_VIRT_TO_PHYS(UART1_BASE),
		.end		= IO_VIRT_TO_PHYS(UART1_BASE) + MTK_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT_UART1_IRQ_ID,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART2)
static struct resource mtk_resource_uart2[] = {
	{
		.start		= IO_VIRT_TO_PHYS(UART2_BASE),
		.end		= IO_VIRT_TO_PHYS(UART2_BASE) + MTK_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT_UART2_IRQ_ID,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART3)
static struct resource mtk_resource_uart3[] = {
	{
		.start		= IO_VIRT_TO_PHYS(UART3_BASE),
		.end		= IO_VIRT_TO_PHYS(UART3_BASE) + MTK_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT_UART3_IRQ_ID,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#if defined(CFG_DEV_UART4)
static struct resource mtk_resource_uart4[] = {
	{
		.start		= IO_VIRT_TO_PHYS(UART4_BASE),
		.end		= IO_VIRT_TO_PHYS(UART4_BASE) + MTK_UART_SIZE - 1,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT_UART4_IRQ_ID,
		.flags		= IORESOURCE_IRQ,
	},
};
#endif

#define MAX_NR_MODEM 2
unsigned long modem_start_addr_list[MAX_NR_MODEM] = { 0x0, 0x0, };

unsigned int get_modem_size(void)
{
    int i, nr_modem;
    unsigned int size = 0, *modem_size_list;
    modem_size_list = get_modem_size_list();
    nr_modem = get_nr_modem();
    if (modem_size_list) {
        for (i = 0; i < nr_modem; i++) {
            size += modem_size_list[i];
        }
        return size;
    } else {
        return 0;
    }
}

unsigned long *get_modem_start_addr_list(void)
{
    return modem_start_addr_list;
}
EXPORT_SYMBOL(get_modem_start_addr_list);

extern unsigned long max_pfn;
#define RESERVED_MEM_MODEM  (0x0) // do not reserve memory in advance, do it in mt_fixup
#ifndef CONFIG_RESERVED_MEM_SIZE_FOR_PMEM
#define CONFIG_RESERVED_MEM_SIZE_FOR_PMEM 	1
#endif


#if defined(CONFIG_MTK_FB)
char temp_command_line[1024] = {0};
extern unsigned int DISP_GetVRamSizeBoot(char* cmdline);
#define RESERVED_MEM_SIZE_FOR_FB (DISP_GetVRamSizeBoot((char*)&temp_command_line))
extern void   mtkfb_set_lcm_inited(bool isLcmInited);
#else
#define RESERVED_MEM_SIZE_FOR_FB (0x400000)
#endif

/*
 * The memory size reserved for PMEM
 *
 * The size could be varied in different solutions.
 * The size is set in mt65xx_fixup function.
 * - MT6589 in develop should be 0x3600000
 * - MT6589 SQC should be 0x0
 */
unsigned int RESERVED_MEM_SIZE_FOR_PMEM = 0x1700000;
unsigned long pmem_start = 0x12345678;  // pmem_start is inited in mt_fixup
unsigned long kernel_mem_sz = 0x0;       // kernel_mem_sz is inited in mt_fixup
unsigned int RESERVED_MEM_SIZE_FOR_TEST_3D = 0x0;
unsigned int FB_SIZE_EXTERN = 0x0;
unsigned int RESERVED_MEM_SIZE_FOR_FB_MAX = 0x1500000;

#define TOTAL_RESERVED_MEM_SIZE (RESERVED_MEM_SIZE_FOR_PMEM + \
                                 RESERVED_MEM_SIZE_FOR_FB)

#define MAX_PFN        ((max_pfn << PAGE_SHIFT) + PHYS_OFFSET)

#define PMEM_MM_START  (pmem_start)
#define PMEM_MM_SIZE   (RESERVED_MEM_SIZE_FOR_PMEM)

#define TEST_3D_START  (PMEM_MM_START + PMEM_MM_SIZE)
#define TEST_3D_SIZE   (RESERVED_MEM_SIZE_FOR_TEST_3D)

#define FB_START      (TEST_3D_START + RESERVED_MEM_SIZE_FOR_TEST_3D)
#define FB_SIZE       (RESERVED_MEM_SIZE_FOR_FB)

static struct platform_device mtk_device_uart[] = {

    #if defined(CFG_DEV_UART1)
    {
    	.name			= "mtk-uart",
    	.id				= 0,
    	.num_resources	= ARRAY_SIZE(mtk_resource_uart1),
    	.resource		= mtk_resource_uart1,
    },
    #endif
    #if defined(CFG_DEV_UART2)
    {
    	.name			= "mtk-uart",
    	.id				= 1,
    	.num_resources	= ARRAY_SIZE(mtk_resource_uart2),
    	.resource		= mtk_resource_uart2,
    },
    #endif
    #if defined(CFG_DEV_UART3)
    {
    	.name			= "mtk-uart",
    	.id				= 2,
    	.num_resources	= ARRAY_SIZE(mtk_resource_uart3),
    	.resource		= mtk_resource_uart3,
    },
    #endif

    #if defined(CFG_DEV_UART4)
    {
    	.name			= "mtk-uart",
    	.id				= 3,
    	.num_resources	= ARRAY_SIZE(mtk_resource_uart4),
    	.resource		= mtk_resource_uart4,
    },
    #endif
};
#if defined(CONFIG_FIQ_DEBUGGER)
extern void fiq_uart_fixup(int uart_port);
extern struct platform_device mt_fiq_debugger;
#endif

/* ========================================================================= */
/* implementation of serial number attribute                                 */
/* ========================================================================= */
#define to_sysinfo_attribute(x) container_of(x, struct sysinfo_attribute, attr)

struct sysinfo_attribute{
    struct attribute attr;
    ssize_t (*show)(char *buf);
    ssize_t (*store)(const char *buf, size_t count);
};

static struct kobject sn_kobj;

static ssize_t sn_show(char *buf){
    return snprintf(buf, 4096, "%s\n", serial_number);
}

struct sysinfo_attribute sn_attr = {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
    .attr = {"serial_number", THIS_MODULE, 0644},
#else
    .attr = {"serial_number", 0644},
#endif
    .show = sn_show,
    .store = NULL
};

static ssize_t sysinfo_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct sysinfo_attribute *sysinfo_attr = to_sysinfo_attribute(attr);
    ssize_t ret = -EIO;

    if(sysinfo_attr->show)
        ret = sysinfo_attr->show(buf);

    return ret;
}

static ssize_t sysinfo_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    struct sysinfo_attribute *sysinfo_attr = to_sysinfo_attribute(attr);
    ssize_t ret = -EIO;

    if(sysinfo_attr->store)
        ret = sysinfo_attr->store(buf, count);

    return ret;
}

static struct sysfs_ops sn_sysfs_ops = {
    .show = sysinfo_show,
    .store = sysinfo_store
};

static struct attribute *sn_attrs[] = {
    &sn_attr.attr,
    NULL
};

static struct kobj_type sn_ktype = {
    .sysfs_ops = &sn_sysfs_ops,
    .default_attrs = sn_attrs
};

#define HASH_ARRAY_SIZE 4

static char udc_chr[32] = {"ABCDEFGHIJKLMNOPQRSTUVWSYZ456789"};
int get_serial(uint64_t hwkey, uint32_t chipid, char ser[SERIALNO_LEN])
{
    uint16_t hashkey[HASH_ARRAY_SIZE];
    int idx, ser_idx;
    uint32_t digit, id;
    uint64_t tmp = hwkey;

    memset(ser, 0x00, SERIALNO_LEN);

    /* split to 4 key with 16-bit width each */
    tmp = hwkey;
    for (idx = 0; idx < HASH_ARRAY_SIZE; idx++) {
        hashkey[idx] = (uint16_t)(tmp & 0xffff);
        tmp >>= 16;
    }

    /* hash the key with chip id */
    id = chipid;
    for (idx = 0; idx < HASH_ARRAY_SIZE; idx++) {
        digit = (id % 10);
        hashkey[idx] = (hashkey[idx] >> digit) | (hashkey[idx] << (16-digit));
        id = (id / 10);
    }

    /* generate serail using hashkey */
    ser_idx = 0;
    for (idx = 0; idx < HASH_ARRAY_SIZE; idx++) {
        ser[ser_idx++] = (hashkey[idx] & 0x001f);
        ser[ser_idx++] = (hashkey[idx] & 0x00f8) >> 3;
        ser[ser_idx++] = (hashkey[idx] & 0x1f00) >> 8;
        ser[ser_idx++] = (hashkey[idx] & 0xf800) >> 11;
    }
    for (idx = 0; idx < ser_idx; idx++)
        ser[idx] = udc_chr[(int)ser[idx]];
    ser[ser_idx] = 0x00;
    return 0;
}

/*=======================================================================*/
/* MT6589 GPIO                                                           */
/*=======================================================================*/
struct platform_device gpio_dev =
{
    .name = "mt-gpio",
    .id   = -1,
};
struct platform_device fh_dev =
{
    .name = "mt-freqhopping",
    .id   = -1,
};
struct platform_device pmic_wrap_dev =
{
    .name = "pmic_wrap",
    .id   = -1,
};

#if defined(CONFIG_SERIAL_AMBA_PL011)
static struct amba_device uart1_device =
{
    .dev =
    {
        .coherent_dma_mask = ~0,
        .init_name = "dev:f1",
        .platform_data = NULL,
    },
    .res =
    {
        .start  = 0xE01F1000,
        .end = 0xE01F1000 + SZ_4K - 1,
        .flags  = IORESOURCE_MEM,
    },
    .dma_mask = ~0,
    .irq = MT_UART1_IRQ_ID,
};
#endif

/*=======================================================================*/
/* MT6589 MSDC Hosts                                                       */
/*=======================================================================*/
#if defined(CFG_DEV_MSDC0)
static struct resource mt_resource_msdc0[] = {
    {
        .start  = IO_VIRT_TO_PHYS(MSDC_0_BASE),
        .end    = IO_VIRT_TO_PHYS(MSDC_0_BASE) + 0x108,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_MSDC0_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};
#endif

#if defined(CFG_DEV_MSDC1)
static struct resource mt_resource_msdc1[] = {
    {
        .start  = IO_VIRT_TO_PHYS(MSDC_1_BASE),
        .end    = IO_VIRT_TO_PHYS(MSDC_1_BASE) + 0x108,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_MSDC1_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};
#endif

#if defined(CFG_DEV_MSDC2)
static struct resource mt_resource_msdc2[] = {
    {
        .start  = IO_VIRT_TO_PHYS(MSDC_2_BASE),
        .end    = IO_VIRT_TO_PHYS(MSDC_2_BASE) + 0x108,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_MSDC2_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};
#endif

#if defined(CFG_DEV_MSDC3)
static struct resource mt_resource_msdc3[] = {
    {
        .start  = IO_VIRT_TO_PHYS(MSDC_3_BASE),
        .end    = IO_VIRT_TO_PHYS(MSDC_3_BASE) + 0x108,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_MSDC3_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};
#endif
#if defined(CFG_DEV_MSDC4)
static struct resource mt_resource_msdc4[] = {
    {
        .start  = IO_VIRT_TO_PHYS(MSDC_4_BASE),
        .end    = IO_VIRT_TO_PHYS(MSDC_4_BASE) + 0x108,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_MSDC4_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};
#endif

#if defined(CONFIG_MTK_FB)
static u64 mtkfb_dmamask = ~(u32)0;

static struct resource resource_fb[] = {
	{
		.start		= 0, /* Will be redefined later */
		.end		= 0,
		.flags		= IORESOURCE_MEM
	}
};

static struct platform_device mt6575_device_fb = {
    .name = "mtkfb",
    .id   = 0,
    .num_resources = ARRAY_SIZE(resource_fb),
    .resource      = resource_fb,
    .dev = {
        .dma_mask = &mtkfb_dmamask,
        .coherent_dma_mask = 0xffffffff,
    },
};
#endif

#ifdef MTK_MULTIBRIDGE_SUPPORT
static struct platform_device mtk_multibridge_dev = {
    .name = "multibridge",
    .id   = 0,
};
#endif
#ifdef MTK_HDMI_SUPPORT
static struct platform_device mtk_hdmi_dev = {
    .name = "hdmitx",
    .id   = 0,
};
#endif


#ifdef MTK_MT8193_SUPPORT
static struct platform_device mtk_ckgen_dev = {
    .name = "mt8193-ckgen",
    .id   = 0,
};
#endif


#if defined (CONFIG_MTK_SPI)
static struct resource mt_spi_resources[] =
{
	[0]={
		.start = IO_VIRT_TO_PHYS(SPI1_BASE),
		.end = IO_VIRT_TO_PHYS(SPI1_BASE) + 0x0028,
		.flags = IORESOURCE_MEM,
	},
	[1]={
		.start = MT6589_SPI1_IRQ_ID,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device mt_spi_device = {
	.name = "mt-spi",
	.num_resources = ARRAY_SIZE(mt_spi_resources),
	.resource=mt_spi_resources
};

#endif


#if defined(CONFIG_USB_MTK_ACM_TEMP)
struct platform_device usbacm_temp_device = {
	.name	  ="USB_ACM_Temp_Driver",
	.id		  = -1,
};
#endif

#if defined(CONFIG_MTK_ACCDET)
struct platform_device accdet_device = {
	.name	  ="Accdet_Driver",
	.id		  = -1,
	//.dev    ={
	//.release = accdet_dumy_release,
	//}
};
#endif

#if defined(MTK_TVOUT_SUPPORT)

static struct resource mt6575_TVOUT_resource[] = {
    [0] = {//TVC
        .start = TVC_BASE,
        .end   = TVC_BASE + 0x78,
        .flags = IORESOURCE_MEM,
    },
    [1] = {//TVR
        .start = TV_ROT_BASE,
        .end   = TV_ROT_BASE + 0x378,
        .flags = IORESOURCE_MEM,
    },
    [2] = {//TVE
        .start = TVE_BASE,
        .end   = TVE_BASE + 0x84,
        .flags = IORESOURCE_MEM,
    },
};

static u64 mt6575_TVOUT_dmamask = ~(u32)0;

static struct platform_device mt6575_TVOUT_dev = {
	.name		  = "TV-out",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(mt6575_TVOUT_resource),
	.resource	  = mt6575_TVOUT_resource,
	.dev              = {
		.dma_mask = &mt6575_TVOUT_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
#endif
static struct platform_device mt_device_msdc[] =
{
#if defined(CFG_DEV_MSDC0)
    {
        .name           = "mtk-msdc",
        .id             = 0,
        .num_resources  = ARRAY_SIZE(mt_resource_msdc0),
        .resource       = mt_resource_msdc0,
        .dev = {
            .platform_data = &msdc0_hw,
        },
    },
#endif
#if defined(CFG_DEV_MSDC1)
    {
        .name           = "mtk-msdc",
        .id             = 1,
        .num_resources  = ARRAY_SIZE(mt_resource_msdc1),
        .resource       = mt_resource_msdc1,
        .dev = {
            .platform_data = &msdc1_hw,
        },
    },
#endif
#if defined(CFG_DEV_MSDC2)
    {
        .name           = "mtk-msdc",
        .id             = 2,
        .num_resources  = ARRAY_SIZE(mt_resource_msdc2),
        .resource       = mt_resource_msdc2,
        .dev = {
            .platform_data = &msdc2_hw,
        },
    },
#endif
#if defined(CFG_DEV_MSDC3)
    {
        .name           = "mtk-msdc",
        .id             = 3,
        .num_resources  = ARRAY_SIZE(mt_resource_msdc3),
        .resource       = mt_resource_msdc3,
        .dev = {
            .platform_data = &msdc3_hw,
        },
    },
#endif
#if defined(CFG_DEV_MSDC4)
    {
        .name           = "mtk-msdc",
        .id             = 4,
        .num_resources  = ARRAY_SIZE(mt_resource_msdc4),
        .resource       = mt_resource_msdc4,
        .dev = {
            .platform_data = &msdc4_hw,
        },
    },
#endif

};

/*=======================================================================*/
/* MT6575 Keypad                                                         */
/*=======================================================================*/
#ifdef CONFIG_MTK_KEYPAD
static struct platform_device kpd_pdev = {
	.name	= "mtk-kpd",
	.id	= -1,
};
#endif

#ifdef CONFIG_RFKILL
/*=======================================================================*/
/* MT6589 RFKill module (BT and WLAN)                                             */
/*=======================================================================*/
/* MT66xx RFKill BT */
struct platform_device mt_rfkill_device = {
    .name   = "mt-rfkill",
    .id     = -1,
};
#endif

/*=======================================================================*/
/* HID Keyboard  add by zhangsg                                                 */
/*=======================================================================*/

#if defined(CONFIG_KEYBOARD_HID)
static struct platform_device mt_hid_dev = {
    .name = "hid-keyboard",
    .id   = -1,
};
#endif 

/*=======================================================================*/
/* MT6575 Touch Panel                                                    */
/*=======================================================================*/
static struct platform_device mtk_tpd_dev = {
    .name = "mtk-tpd",
    .id   = -1,
};

/*=======================================================================*/
/* MT6575 ofn                                                           */
/*=======================================================================*/
#if defined(CUSTOM_KERNEL_OFN)
static struct platform_device ofn_driver =
{
    .name = "mtofn",
    .id   = -1,
};
#endif

/*=======================================================================*/
/* CPUFreq                                                               */
/*=======================================================================*/
#ifdef CONFIG_CPU_FREQ
static struct platform_device cpufreq_pdev = {
    .name = "mt-cpufreq",
    .id   = -1,
};
#endif

/*=======================================================================*/
/* MT6575 Thermal Controller module                                      */
/*=======================================================================*/
struct platform_device thermal_pdev = {
    .name = "mtk-thermal",
    .id   = -1,
};

#if 1
struct platform_device mtk_therm_mon_pdev = {
    .name = "mtk-therm-mon",
    .id   = -1,
};
#endif

/*=======================================================================*/
/* MT6589 PTP module                                      */
/*=======================================================================*/
struct platform_device ptp_pdev = {
    .name = "mtk-ptp",
    .id   = -1,
};

/*=======================================================================*/
/* MT6589 SPM-MCDI module                                      */
/*=======================================================================*/
struct platform_device spm_mcdi_pdev = {
    .name = "mtk-spm-mcdi",
    .id   = -1,
};

/*=======================================================================*/
/* MT6589 Golden Setting module                                      */
/*=======================================================================*/
struct platform_device golden_setting_pdev = {
    .name = "mtk-golden-setting",
    .id   = -1,
};

/*=======================================================================*/
/* MT6589 USIF-DUMCHAR                                                          */
/*=======================================================================*/

static struct platform_device dummychar_device =
{
       .name           = "dummy_char",
        .id             = 0,
};

/*=======================================================================*/
/* MT6589 NAND                                                           */
/*=======================================================================*/
#if defined(CONFIG_MTK_MTD_NAND)
#define NFI_base    NFI_BASE//0x80032000
#define NFIECC_base NFIECC_BASE//0x80038000
static struct resource mtk_resource_nand[] = {
	{
		.start		= IO_VIRT_TO_PHYS(NFI_base),
		.end		= IO_VIRT_TO_PHYS(NFI_base) + 0x1A0,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= IO_VIRT_TO_PHYS(NFIECC_base),
		.end		= IO_VIRT_TO_PHYS(NFIECC_base) + 0x150,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= MT_NFI_IRQ_ID,
		.flags		= IORESOURCE_IRQ,
	},
	{
		.start		= MT_NFIECC_IRQ_ID,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device mtk_nand_dev = {
    .name = "mtk-nand",
    .id   = 0,
   	.num_resources	= ARRAY_SIZE(mtk_resource_nand),
   	.resource		= mtk_resource_nand,
    .dev            = {
        .platform_data = &mtk_nand_hw,
    },
};
#endif

/*=======================================================================*/
/* Audio                                                 */
/*=======================================================================*/
static u64        AudDrv_dmamask      = 0xffffffffUL;
static struct platform_device AudDrv_device = {
	.name  = "AudDrv_driver_device",
	.id    = 0,
	.dev   = {
		        .dma_mask = &AudDrv_dmamask,
		        .coherent_dma_mask =  0xffffffffUL
	         }
};

/*=======================================================================*/
/* MTK I2C                                                            */
/*=======================================================================*/

static struct resource mt_resource_i2c0[] = {
    {
        .start  = IO_VIRT_TO_PHYS(I2C0_BASE),
        .end    = IO_VIRT_TO_PHYS(I2C0_BASE) + 0x70,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_I2C0_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct resource mt_resource_i2c1[] = {
    {
        .start  = IO_VIRT_TO_PHYS(I2C1_BASE),
        .end    = IO_VIRT_TO_PHYS(I2C1_BASE) + 0x70,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_I2C1_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct resource mt_resource_i2c2[] = {
    {
        .start  = IO_VIRT_TO_PHYS(I2C2_BASE),
        .end    = IO_VIRT_TO_PHYS(I2C2_BASE) + 0x70,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_I2C2_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};
static struct resource mt_resource_i2c3[] = {
    {
        .start  = IO_VIRT_TO_PHYS(I2C3_BASE),
        .end    = IO_VIRT_TO_PHYS(I2C3_BASE) + 0x70,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_I2C3_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};
static struct resource mt_resource_i2c4[] = {
    {
        .start  = IO_VIRT_TO_PHYS(I2C4_BASE),
        .end    = IO_VIRT_TO_PHYS(I2C4_BASE) + 0x70,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_I2C4_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct resource mt_resource_i2c5[] = {
    {
        .start  = IO_VIRT_TO_PHYS(I2C5_BASE),
        .end    = IO_VIRT_TO_PHYS(I2C5_BASE) + 0x70,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_I2C5_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};

static struct resource mt_resource_i2c6[] = {
    {
        .start  = IO_VIRT_TO_PHYS(I2C6_BASE),
        .end    = IO_VIRT_TO_PHYS(I2C6_BASE) + 0x70,
        .flags  = IORESOURCE_MEM,
    },
    {
        .start  = MT_I2C6_IRQ_ID,
        .flags  = IORESOURCE_IRQ,
    },
};
static struct platform_device mt_device_i2c[] = {
    {
        .name           = "mt-i2c",
        .id             = 0,
        .num_resources  = ARRAY_SIZE(mt_resource_i2c0),
        .resource       = mt_resource_i2c0,
    },
    {
        .name           = "mt-i2c",
        .id             = 1,
        .num_resources  = ARRAY_SIZE(mt_resource_i2c1),
        .resource       = mt_resource_i2c1,
    },
    {
        .name           = "mt-i2c",
        .id             = 2,
        .num_resources  = ARRAY_SIZE(mt_resource_i2c2),
        .resource       = mt_resource_i2c2,
    },
		{
        .name           = "mt-i2c",
        .id             = 3,
        .num_resources  = ARRAY_SIZE(mt_resource_i2c3),
        .resource       = mt_resource_i2c3,
    },
{
        .name           = "mt-i2c",
        .id             = 4,
        .num_resources  = ARRAY_SIZE(mt_resource_i2c4),
        .resource       = mt_resource_i2c4,
    },

{
        .name           = "mt-i2c",
        .id             = 5,
        .num_resources  = ARRAY_SIZE(mt_resource_i2c5),
        .resource       = mt_resource_i2c5,
    },

{
        .name           = "mt-i2c",
        .id             = 6,
        .num_resources  = ARRAY_SIZE(mt_resource_i2c6),
        .resource       = mt_resource_i2c6,
    },

};



static u64 mtk_smi_dmamask = ~(u32)0;

static struct platform_device mtk_smi_dev = {
	.name		  = "MTK_SMI",
	.id		  = 0,
	.dev              = {
		.dma_mask = &mtk_smi_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};


static u64 mtk_m4u_dmamask = ~(u32)0;

static struct platform_device mtk_m4u_dev = {
	.name		  = "M4U_device",
	.id		  = 0,
	.dev              = {
		.dma_mask = &mtk_m4u_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};


/*=======================================================================*/
/* MT6573 GPS module                                                    */
/*=======================================================================*/
/* MT3326 GPS */
#ifdef CONFIG_MTK_GPS
struct platform_device mt3326_device_gps = {
	.name	       = "mt3326-gps",
	.id            = -1,
	.dev = {
        .platform_data = &mt3326_gps_hw,
    },
};
#endif

/*=======================================================================*/
/* MT6573 PMEM                                                           */
/*=======================================================================*/
#if defined(CONFIG_ANDROID_PMEM)
static struct android_pmem_platform_data  pdata_multimedia = {
        .name = "pmem_multimedia",
        .no_allocator = 0,
        .cached = 1,
        .buffered = 1
};

static struct platform_device pmem_multimedia_device = {
        .name = "android_pmem",
        .id = 1,
        .dev = { .platform_data = &pdata_multimedia }
};
#endif

#if defined(CONFIG_ANDROID_VMEM)
static struct android_vmem_platform_data  pdata_vmultimedia = {
        .name = "vmem_multimedia",
        .no_allocator = 0,
        .cached = 1,
        .buffered = 1
};

static struct platform_device vmem_multimedia_device = {
        .name = "android_vmem",
        .id = -1,
        .dev = { .platform_data = &pdata_vmultimedia }
};
#endif

/*=======================================================================*/
/* MT6575 SYSRAM                                                         */
/*=======================================================================*/
static struct platform_device camera_sysram_dev = {
	.name	= "camera-sysram", /* FIXME. Sync to driver, init.rc, MHAL */
	.id     = 0,
};

/*=======================================================================*/
/*=======================================================================*/
/* Commandline filter                                                    */
/* This function is used to filter undesired command passed from LK      */
/*=======================================================================*/
static void cmdline_filter(struct tag *cmdline_tag, char *default_cmdline)
{
	const char *undesired_cmds[] = {
	                             "console=",
                                     "root=",
			             };

	int i;
	int ck_f = 0;
	char *cs,*ce;

	cs = cmdline_tag->u.cmdline.cmdline;
	ce = cs;
	while((__u32)ce < (__u32)tag_next(cmdline_tag)) {

	    while(*cs == ' ' || *cs == '\0') {
	    	cs++;
	    	ce = cs;
	    }

	    if (*ce == ' ' || *ce == '\0') {
	    	for (i = 0; i < sizeof(undesired_cmds)/sizeof(char *); i++){
	    	    if (memcmp(cs, undesired_cmds[i], strlen(undesired_cmds[i])) == 0) {
			ck_f = 1;
                        break;
                    }    		
	    	}

                if(ck_f == 0){
		    *ce = '\0';
                    //Append to the default command line
                    strcat(default_cmdline, " ");
                    strcat(default_cmdline, cs);
		}
		ck_f = 0;
	    	cs = ce + 1;
	    }
	    ce++;
	}
	if (strlen(default_cmdline) >= COMMAND_LINE_SIZE)
	{
		panic("Command line length is too long.\n\r");
	}
}
/*=======================================================================*/
/* Parse the framebuffer info						 */
/*=======================================================================*/
static int __init parse_tag_videofb_fixup(const struct tag *tags)
{
	bl_fb.base = tags->u.videolfb.lfb_base;
	bl_fb.size = tags->u.videolfb.lfb_size;
        use_bl_fb++;
	return 0;
}

static int __init parse_tag_devinfo_data_fixup(const struct tag *tags)
{
    int i=0;
    int size = tags->u.devinfo_data.devinfo_data_size;

    for (i=0;i<size;i++){
        g_devinfo_data[i] = tags->u.devinfo_data.devinfo_data[i];
    }
    /* print chip id for debugging purpose */
    printk("tag_devinfo_data_rid, indx[%d]:0x%x\n", 12,g_devinfo_data[12]);
    printk("tag_devinfo_data size:%d\n", size);
    g_devinfo_data_size = size;
	return 0;
}


void mt_fixup(struct tag *tags, char **cmdline, struct meminfo *mi)
{
/* FIXME: need porting */
#if 1
    struct tag *cmdline_tag = NULL;
    struct tag *reserved_mem_bank_tag = NULL;
    struct tag *none_tag = NULL;

    unsigned long max_limit_size = CONFIG_MAX_DRAM_SIZE_SUPPORT -
                             RESERVED_MEM_MODEM;
    unsigned long avail_dram = 0;
    unsigned long bl_mem_sz = 0;
    /* for modem fixup */
    unsigned int nr_modem = 0, i = 0;
    unsigned int max_avail_addr = 0;
    unsigned int modem_start_addr = 0;
    unsigned int hole_start_addr = 0;
    unsigned int hole_size = 0;
    unsigned int *modem_size_list = 0;

#if defined(CONFIG_MTK_FB)
	struct tag *temp_tags = tags;
	for (; temp_tags->hdr.size; temp_tags = tag_next(temp_tags))
	{
		if(temp_tags->hdr.tag == ATAG_CMDLINE)
			cmdline_filter(temp_tags, (char*)&temp_command_line);
	}
#endif

    for (; tags->hdr.size; tags = tag_next(tags)) {
        if (tags->hdr.tag == ATAG_MEM) {
	    bl_mem_sz += tags->u.mem.size;

	    /*
             * Modify the memory tag to limit available memory to
             * CONFIG_MAX_DRAM_SIZE_SUPPORT
             */
            if (max_limit_size > 0) {
                if (max_limit_size >= tags->u.mem.size) {
                    max_limit_size -= tags->u.mem.size;
                    avail_dram += tags->u.mem.size;
                } else {
                    tags->u.mem.size = max_limit_size;
                    avail_dram += max_limit_size;
                    max_limit_size = 0;
                }
		// By Keene:
		// remove this check to avoid calcuate pmem size before we know all dram size
		// Assuming the minimum size of memory bank is 256MB
                //if (tags->u.mem.size >= (TOTAL_RESERVED_MEM_SIZE)) {
                    reserved_mem_bank_tag = tags;
                //}
            } else {
                tags->u.mem.size = 0;
            }
	}
        else if (tags->hdr.tag == ATAG_CMDLINE) {
            cmdline_tag = tags;
        } else if (tags->hdr.tag == ATAG_BOOT) {
            g_boot_mode = tags->u.boot.bootmode;
        } else if (tags->hdr.tag == ATAG_VIDEOLFB) {
            parse_tag_videofb_fixup(tags);
        }else if (tags->hdr.tag == ATAG_DEVINFO_DATA){
            parse_tag_devinfo_data_fixup(tags);
        }
        else if(tags->hdr.tag == ATAG_META_COM)
        {
          g_meta_com_type = tags->u.meta_com.meta_com_type;
          g_meta_com_id = tags->u.meta_com.meta_com_id;
        }
    }

    kernel_mem_sz = avail_dram; // keep the DRAM size (limited by CONFIG_MAX_DRAM_SIZE_SUPPORT)
    /*
    * If the maximum memory size configured in kernel
    * is smaller than the actual size (passed from BL)
    * Still limit the maximum memory size but use the FB
    * initialized by BL
    */
    if (bl_mem_sz >= (CONFIG_MAX_DRAM_SIZE_SUPPORT - RESERVED_MEM_MODEM)) {
	use_bl_fb++;
    }

    /*
     * Setup PMEM size
     */
    /*
    if (avail_dram < 0x10000000)
        RESERVED_MEM_SIZE_FOR_PMEM = 0x1700000;
    else */
        RESERVED_MEM_SIZE_FOR_PMEM = 0x0;

    /* Reserve memory in the last bank */
    if (reserved_mem_bank_tag) {
        reserved_mem_bank_tag->u.mem.size -= ((__u32)TOTAL_RESERVED_MEM_SIZE);
	if(g_boot_mode == FACTORY_BOOT) {
            /* we need to reserved the maximum FB_SIZE to get a fixed TEST_3D pa. */
            unsigned int rest_fb_size = RESERVED_MEM_SIZE_FOR_FB_MAX - FB_SIZE;
            RESERVED_MEM_SIZE_FOR_TEST_3D = 0x9a00000 + rest_fb_size;
            reserved_mem_bank_tag->u.mem.size -= RESERVED_MEM_SIZE_FOR_TEST_3D;
        }
        FB_SIZE_EXTERN = FB_SIZE;
        pmem_start = reserved_mem_bank_tag->u.mem.start + reserved_mem_bank_tag->u.mem.size;
    } else // we should always have reserved memory
    	BUG();

    MTK_MEMCFG_LOG_AND_PRINTK(KERN_ALERT
            "[PHY layout]avaiable DRAM size (lk) = 0x%08lx\n"
            "[PHY layout]avaiable DRAM size = 0x%08lx\n"
            "[PHY layout]FB       :   0x%08lx - 0x%08lx  (0x%08x)\n",
            bl_mem_sz,
            kernel_mem_sz,
            FB_START, (FB_START + FB_SIZE - 1), FB_SIZE);
    if(g_boot_mode == FACTORY_BOOT)
        MTK_MEMCFG_LOG_AND_PRINTK(KERN_ALERT
                "[PHY layout]3D       :   0x%08lx - 0x%08lx  (0x%08x)\n",
                TEST_3D_START, TEST_3D_START + TEST_3D_SIZE, TEST_3D_SIZE);
    if (PMEM_MM_SIZE) {
        MTK_MEMCFG_LOG_AND_PRINTK(KERN_ALERT
                "[PHY layout]PMEM     :   0x%08lx - 0x%08lx  (0x%08x)\n",
                PMEM_MM_START, (PMEM_MM_START + PMEM_MM_SIZE - 1), PMEM_MM_SIZE);
    }
    /*
     * fixup memory tags for dual modem model
     * assumptions:
     * 1) modem start addresses should be 32MiB aligned
     */
    nr_modem = get_nr_modem();
    modem_size_list = get_modem_size_list();
    if(tags->hdr.tag == ATAG_NONE) {
        for (i = 0; i < nr_modem; i++) {
            /* sanity test */
            if (modem_size_list[i]) {
                printk(KERN_ALERT"fixup for modem [%d], size = 0x%08x\n", i,
                        modem_size_list[i]);
            } else {
                printk(KERN_ALERT"[Error]skip empty modem [%d]\n", i);
                continue;
            }
            printk(KERN_ALERT
                    "reserved_mem_bank_tag start = 0x%08x, "
                    "reserved_mem_bank_tag size = 0x%08x, "
                    "TOTAL_RESERVED_MEM_SIZE = 0x%08x\n",
                    reserved_mem_bank_tag->u.mem.start,
                    reserved_mem_bank_tag->u.mem.size,
                    TOTAL_RESERVED_MEM_SIZE);
            /* find out start address for modem */
            max_avail_addr = reserved_mem_bank_tag->u.mem.start +
                             reserved_mem_bank_tag->u.mem.size;
            modem_start_addr =
                round_down((max_avail_addr -
                        modem_size_list[i]), 0x2000000);
            /* sanity test */
            if (modem_size_list[i] > reserved_mem_bank_tag->u.mem.size) {
                printk(KERN_ALERT"[Error]skip modem [%d] fixup: "
                        "size too large: 0x%08x, "
                        "reserved_mem_bank_tag->u.mem.size: 0x%08x\n", i,
                        modem_size_list[i],
                        reserved_mem_bank_tag->u.mem.size);
                continue;
            }
            if (modem_start_addr < reserved_mem_bank_tag->u.mem.start) {
                printk(KERN_ALERT"[Error]skip modem [%d] fixup: "
                        "modem crosses memory bank boundary: 0x%08x, "
                        "reserved_mem_bank_tag->u.mem.start: 0x%08x\n", i,
                        modem_start_addr,
                        reserved_mem_bank_tag->u.mem.start);
                continue;
            }
            printk(KERN_ALERT"modem fixup sanity test pass\n");
            modem_start_addr_list[i] = modem_start_addr;
            hole_start_addr = modem_start_addr + modem_size_list[i];
            hole_size = max_avail_addr - hole_start_addr;
            printk(KERN_ALERT
                    "max_avail_addr = 0x%08x, "
                    "modem_start_addr_list[%d] = 0x%08x, "
                    "hole_start_addr = 0x%08x, hole_size = 0x%08x\n",
                    max_avail_addr, i, modem_start_addr,
                    hole_start_addr, hole_size);
            MTK_MEMCFG_LOG_AND_PRINTK(KERN_ALERT
                    "[PHY layout]MD       :   0x%08x - 0x%08x  (0x%08x)\n",
                    modem_start_addr,
                    (modem_start_addr + modem_size_list[i] - 1),
                    modem_size_list[i]);
            /* shrink reserved_mem_bank */
            reserved_mem_bank_tag->u.mem.size -=
                (max_avail_addr - modem_start_addr);
            printk(KERN_ALERT
                    "reserved_mem_bank: start = 0x%08x, size = 0x%08x\n",
                    reserved_mem_bank_tag->u.mem.start,
                    reserved_mem_bank_tag->u.mem.size);
            /* setup a new memory tag */
            tags->hdr.tag = ATAG_MEM;
            tags->hdr.size = tag_size(tag_mem32);
            tags->u.mem.start = hole_start_addr;
            tags->u.mem.size = hole_size;
            /* do next tag */
            tags = tag_next(tags);
        }
        tags->hdr.tag = ATAG_NONE; // mark the end of the tag list
        tags->hdr.size = 0;
    }

    if(tags->hdr.tag == ATAG_NONE)
	none_tag = tags;
    if (cmdline_tag != NULL) {
#ifdef CONFIG_FIQ_DEBUGGER
        char *console_ptr;
        int uart_port;
#endif
	char *br_ptr;
        // This function may modify ttyMT3 to ttyMT0 if needed
        adjust_kernel_cmd_line_setting_for_console(cmdline_tag->u.cmdline.cmdline, *cmdline);
#ifdef CONFIG_FIQ_DEBUGGER
        if ((console_ptr=strstr(*cmdline, "ttyMT")) != 0)
        {
            uart_port = console_ptr[5] - '0';
            if (uart_port > 3)
                uart_port = -1;

            fiq_uart_fixup(uart_port);
        }
#endif

	/*FIXME mark for porting*/
        cmdline_filter(cmdline_tag, *cmdline);
		if ((br_ptr = strstr(*cmdline, "boot_reason=")) != 0) {
			/* get boot reason */
			g_boot_reason = br_ptr[12] - '0';
		}
        /* Use the default cmdline */
        memcpy((void*)cmdline_tag,
               (void*)tag_next(cmdline_tag),
               /* ATAG_NONE actual size */
               (uint32_t)(none_tag) - (uint32_t)(tag_next(cmdline_tag)) + 8);
    }
#endif
}

struct platform_device auxadc_device = {
    .name   = "mt-auxadc",
    .id     = -1,
};

/*=======================================================================*/
/* MT6575 sensor module                                                  */
/*=======================================================================*/
struct platform_device sensor_gsensor = {
	.name	       = "gsensor",
	.id            = -1,
};

struct platform_device sensor_msensor = {
	.name	       = "msensor",
	.id            = -1,
};

struct platform_device sensor_orientation = {
	.name	       = "orientation",
	.id            = -1,
};

struct platform_device sensor_alsps = {
	.name	       = "als_ps",
	.id            = -1,
};

struct platform_device sensor_gyroscope = {
	.name	       = "gyroscope",
	.id            = -1,
};

struct platform_device sensor_barometer = {
	.name	       = "barometer",
	.id            = -1,
};
/* hwmon sensor */
struct platform_device hwmon_sensor = {
	.name	       = "hwmsensor",
	.id            = -1,
};
/*=======================================================================*/
/* Camera ISP                                                            */
/*=======================================================================*/
static struct resource mt_resource_isp[] = {
    { // ISP configuration
        .start = IO_VIRT_TO_PHYS(CAMINF_BASE),
        .end   = IO_VIRT_TO_PHYS(CAMINF_BASE) + 0xE000,
        .flags = IORESOURCE_MEM,
    },
    { // ISP IRQ
        .start = CAMERA_ISP_IRQ0_ID,
        .flags = IORESOURCE_IRQ,
    }
};
static u64 mt_isp_dmamask = ~(u32) 0;
//
static struct platform_device mt_isp_dev = {
	.name		   = "camera-isp",
	.id		       = 0,
	.num_resources = ARRAY_SIZE(mt_resource_isp),
	.resource	   = mt_resource_isp,
	.dev           = {
		.dma_mask  = &mt_isp_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

#if 0
/*=======================================================================*/
/* MT6575 EIS                                                            */
/*=======================================================================*/
static struct resource mt_resource_eis[] = {
    [0] = { // EIS configuration
        .start = EIS_BASE,
        .end   = EIS_BASE + 0x2C,
        .flags = IORESOURCE_MEM,
    }
};
static u64 mt_eis_dmamask = ~(u32) 0;
//
static struct platform_device mt_eis_dev = {
	.name		   = "camera-eis",
	.id		       = 0,
	.num_resources = ARRAY_SIZE(mt_resource_eis),
	.resource	   = mt_resource_eis,
	.dev           = {
		.dma_mask  = &mt_eis_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};

#endif
//
/*=======================================================================*/
/* Image sensor                                                        */
/*=======================================================================*/
static struct platform_device sensor_dev = {
	.name		  = "image_sensor",
	.id		  = -1,
};
static struct platform_device sensor_dev_bus2 = {
	.name		  = "image_sensor_bus2",
	.id		  = -1,
};

//
/*=======================================================================*/
/* Lens actuator                                                        */
/*=======================================================================*/
static struct platform_device actuator_dev = {
	.name		  = "lens_actuator",
	.id		  = -1,
};
/*=======================================================================*/
/* MT6575 jogball                                                        */
/*=======================================================================*/
#ifdef CONFIG_MOUSE_PANASONIC_EVQWJN
static struct platform_device jbd_pdev = {
	.name = "mt6575-jb",
	.id = -1,
};
#endif

/*=======================================================================*/
/* MT6589 Pipe Manager                                                         */
/*=======================================================================*/
static struct platform_device camera_pipemgr_dev = {
	.name	= "camera-pipemgr",
	.id     = -1,
};

static struct platform_device mt65xx_leds_device = {
	.name = "leds-mt65xx",
	.id = -1
};
/*=======================================================================*/
/* NFC                                                                          */
/*=======================================================================*/
static struct platform_device mtk_nfc_6605_dev = {
    .name   = "mt6605",
    .id     = -1,
};

/*=======================================================================*/
/* Sim switch driver                                                         */
/*=======================================================================*/
#if defined (CUSTOM_KERNEL_SSW)
static struct platform_device ssw_device = {	
	.name = "sim-switch",	
	.id = -1};
#endif

/*=======================================================================*/
/* MT6589 Board Device Initialization                                    */
/*=======================================================================*/
__init int mt6589_board_init(void)
{
    int i = 0, retval = 0;

#if defined(CONFIG_MTK_SERIAL)
    for (i = 0; i < ARRAY_SIZE(mtk_device_uart); i++){
        retval = platform_device_register(&mtk_device_uart[i]);
        printk("register uart device\n");
        if (retval != 0){
            return retval;
        }
    }
#endif

#ifdef CONFIG_FIQ_DEBUGGER
        retval = platform_device_register(&mt_fiq_debugger);
        if (retval != 0){
                return retval;
        }
#endif

	{
		uint64_t key;
#if defined(CONFIG_MTK_USB_UNIQUE_SERIAL)
		key = get_devinfo_with_index(13);
		key = (key << 32) | get_devinfo_with_index(12);
#else
		key = 0;
#endif
		if (key != 0)
			get_serial(key, get_chip_code(), serial_number);
		else
			memcpy(serial_number, "0123456789ABCDEF", 16);

		retval = kobject_init_and_add(&sn_kobj, &sn_ktype, NULL, "sys_info");

		if (retval < 0)
			printk("[%s] fail to add kobject\n", "sys_info");
	}

#if defined(CONFIG_MTK_MTD_NAND)
    retval = platform_device_register(&mtk_nand_dev);
    if (retval != 0) {
        printk(KERN_ERR "register nand device fail\n");
        return retval;
    }
#endif

	retval = platform_device_register(&gpio_dev);
	if (retval != 0){
		return retval;
	}
	retval = platform_device_register(&fh_dev);
	if (retval != 0){
		return retval;
	}
	retval = platform_device_register(&pmic_wrap_dev);
	if (retval != 0){
		return retval;
	}
#ifdef CONFIG_MTK_KEYPAD
	retval = platform_device_register(&kpd_pdev);
	if (retval != 0) {
		return retval;
	}
#endif

#ifdef CONFIG_MOUSE_PANASONIC_EVQWJN
	retval = platform_device_register(&jbd_pdev);
	if (retval != 0) {
		return retval;
	}
#endif

#if defined(CONFIG_KEYBOARD_HID)
	retval = platform_device_register(&mt_hid_dev);
	if (retval != 0){
		return retval;
	}
#endif

#if defined(CONFIG_MTK_I2C)
	//i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	//i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	//i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));
		for (i = 0; i < ARRAY_SIZE(mt_device_i2c); i++){
			retval = platform_device_register(&mt_device_i2c[i]);
			if (retval != 0){
				return retval;
			}
		}
#endif
#if defined(CONFIG_MTK_MMC)
    for (i = 0; i < ARRAY_SIZE(mt_device_msdc); i++){
        retval = platform_device_register(&mt_device_msdc[i]);
			if (retval != 0){
				return retval;
			}
		}
#endif

#if defined(CONFIG_MTK_SOUND)
    retval = platform_device_register(&AudDrv_device);
    printk("AudDrv_driver_device \n!");
    if (retval != 0){
       return retval;
    }
#endif

#ifdef MTK_MULTIBRIDGE_SUPPORT
    retval = platform_device_register(&mtk_multibridge_dev);
    printk("multibridge_driver_device \n!");
    if (retval != 0){
        return retval;
    }
#endif



//=====SMI/M4U devices===========
    printk("register MTK_SMI device\n");
    retval = platform_device_register(&mtk_smi_dev);
    if (retval != 0) {
        return retval;
    }

    printk("register M4U device: %d\n", retval);
    retval = platform_device_register(&mtk_m4u_dev);
    if (retval != 0) {
        return retval;
    }

//===========================

#ifdef MTK_MT8193_SUPPORT
    printk("register 8193_CKGEN device\n");
    retval = platform_device_register(&mtk_ckgen_dev);
    if (retval != 0){
        
        printk("register 8193_CKGEN device FAILS!\n");
        return retval;
    }
#endif


#if defined(CONFIG_MTK_FB)
    /*
     * Bypass matching the frame buffer info. between boot loader and kernel
     * if the limited memory size of the kernel is smaller than the
     * memory size from bootloader
     */
    if (((bl_fb.base == FB_START) && (bl_fb.size == FB_SIZE)) ||
         (use_bl_fb == 2)) {
        printk("FB is initialized by BL(%d)\n", use_bl_fb);
        mtkfb_set_lcm_inited(1);
    } else if ((bl_fb.base == 0) && (bl_fb.size == 0)) {
        printk("FB is not initialized(%d)\n", use_bl_fb);
        mtkfb_set_lcm_inited(0);
    } else {
        printk(
"******************************************************************************\n"
"   WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING\n"
"******************************************************************************\n"
"\n"
"  The default FB base & size values are not matched between BL and kernel\n"
"    - BOOTLD: start 0x%08x, size %d\n"
"    - KERNEL: start 0x%08lx, size %d\n"
"\n"
"  If you see this warning message, please update your uboot.\n"
"\n"
"******************************************************************************\n"
"   WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING\n"
"******************************************************************************\n"
"\n",   bl_fb.base, bl_fb.size, FB_START, FB_SIZE);
        /* workaround for TEST_3D_START */
        mtkfb_set_lcm_inited(1);
        {
            int delay_sec = 5;

            while (delay_sec >= 0) {
                printk("\rcontinue after %d seconds ...", delay_sec--);
                mdelay(1000);
            }
            printk("\n");
        }
#if 0
        panic("The default base & size values are not matched "
              "between BL and kernel\n");
#endif
    }

    if ((use_bl_fb == 2)) {
        /* use FB initialized by LK */
        printk("use LK FB %d\n", use_bl_fb);
        resource_fb[0].start = bl_fb.base;
        resource_fb[0].end  = bl_fb.base + bl_fb.size - 1;
    } else {
        resource_fb[0].start = FB_START;
        resource_fb[0].end   = FB_START + FB_SIZE - 1;
    }

    printk("FB start: 0x%x end: 0x%x\n", resource_fb[0].start,
                                         resource_fb[0].end);

    retval = platform_device_register(&mt6575_device_fb);
    if (retval != 0) {
         return retval;
    }
#endif

#if defined(CONFIG_MTK_LEDS)
	retval = platform_device_register(&mt65xx_leds_device);
	if (retval != 0)
		return retval;
	printk("bei:device LEDS register\n");
#endif

#ifdef MTK_HDMI_SUPPORT
	retval = platform_device_register(&mtk_hdmi_dev);
	if (retval != 0){
		return retval;
	}
#endif


#if defined(CONFIG_MTK_SPI)
//    spi_register_board_info(spi_board_devs, ARRAY_SIZE(spi_board_devs));
    platform_device_register(&mt_spi_device);
#endif




    

#if defined(MTK_TVOUT_SUPPORT)
    retval = platform_device_register(&mt6575_TVOUT_dev);
	printk("register TV-out device\n");
    if (retval != 0) {
         return retval;
    }
#endif

#if 1
  retval = platform_device_register(&auxadc_device);
  if(retval != 0)
  {
     printk("****[auxadc_driver] Unable to device register(%d)\n", retval);
	 return retval;
  }
#endif

#if defined(CONFIG_MTK_ACCDET)


    retval = platform_device_register(&accdet_device);
	printk("register accdet device\n");

	if (retval != 0)
	{
		printk("platform_device_accdet_register error:(%d)\n", retval);
		return retval;
	}
	else
	{
		printk("platform_device_accdet_register done!\n");
	}

#endif

#if defined(CONFIG_USB_MTK_ACM_TEMP)

    retval = platform_device_register(&usbacm_temp_device);
	printk("register usbacm temp device\n");

	if (retval != 0)
	{
		printk("platform_device_usbacm_register error:(%d)\n", retval);
		return retval;
	}
	else
	{
		printk("platform_device_usbacm_register done!\n");
	}

#endif


#if 0 //defined(CONFIG_MDP_MT6575)
    //printk("[MDP]platform_device_register\n\r");
    retval = platform_device_register(&mt6575_MDP_dev);
    if(retval != 0){
        return retval;
    }
#endif

#if defined(MTK_SENSOR_SUPPORT)

	retval = platform_device_register(&hwmon_sensor);
	printk("hwmon_sensor device!");
	if (retval != 0)
		return retval;

#if defined(CUSTOM_KERNEL_ACCELEROMETER)
	retval = platform_device_register(&sensor_gsensor);
		printk("sensor_gsensor device!");
	if (retval != 0)
		return retval;
#endif

#if defined(CUSTOM_KERNEL_MAGNETOMETER)
	retval = platform_device_register(&sensor_msensor);
		printk("sensor_msensor device!");
	if (retval != 0)
		return retval;

	retval = platform_device_register(&sensor_orientation);
		printk("sensor_osensor device!");
	if (retval != 0)
		return retval;

#endif

#if defined(CUSTOM_KERNEL_GYROSCOPE)
	retval = platform_device_register(&sensor_gyroscope);
		printk("sensor_gyroscope device!");
	if (retval != 0)
		return retval;
#endif

#if defined(CUSTOM_KERNEL_BAROMETER)
	retval = platform_device_register(&sensor_barometer);
		printk("sensor_barometer device!");
	if (retval != 0)
		return retval;
#endif

#if defined(CUSTOM_KERNEL_ALSPS)
	retval = platform_device_register(&sensor_alsps);
		printk("sensor_alsps device!");
	if (retval != 0)
		return retval;
#endif
#endif

#if defined(CONFIG_MTK_USBFSH)
	printk("register musbfsh device\n");
	retval = platform_device_register(&mt_usb11_dev);
	if (retval != 0){
		printk("register musbfsh device fail!\n");
		return retval;
	}
#endif

#if defined(CONFIG_USB_MTK_HDRC)
	printk("mt_device_usb register\n");
	retval = platform_device_register(&mt_device_usb);
	if (retval != 0){
	printk("mt_device_usb register fail\n");
        return retval;
	}
#endif

#if defined(CONFIG_MTK_TOUCHPANEL)
    retval = platform_device_register(&mtk_tpd_dev);
    if (retval != 0) {
        return retval;
    }
#endif
#if defined(CUSTOM_KERNEL_OFN)
    retval = platform_device_register(&ofn_driver);
    if (retval != 0){
        return retval;
    }
#endif

#if (defined(CONFIG_MTK_MTD_NAND) ||defined(CONFIG_MTK_MMC))
retval = platform_device_register(&dummychar_device);
	if (retval != 0){
		return retval;
	}
#endif


#if defined(CONFIG_ANDROID_PMEM)
    pdata_multimedia.start = PMEM_MM_START;;
    pdata_multimedia.size = PMEM_MM_SIZE;
    printk("PMEM start: 0x%lx size: 0x%lx\n", pdata_multimedia.start, pdata_multimedia.size);

    retval = platform_device_register(&pmem_multimedia_device);
    if (retval != 0){
       return retval;
    }
#endif

#if defined(CONFIG_ANDROID_VMEM)
    pdata_vmultimedia.start = PMEM_MM_START;;
    pdata_vmultimedia.size = PMEM_MM_SIZE;
    printk("VMEM start: 0x%lx size: 0x%lx\n", pdata_vmultimedia.start, pdata_vmultimedia.size);

    retval = platform_device_register(&vmem_multimedia_device);
    if (retval != 0){
	printk("vmem platform register failed\n");
       return retval;
    }
#endif

#ifdef CONFIG_CPU_FREQ
    retval = platform_device_register(&cpufreq_pdev);
    if (retval != 0) {
        return retval;
    }
#endif

#if 1
    retval = platform_device_register(&thermal_pdev);
    if (retval != 0) {
        return retval;
    }
#endif

#if 1
    retval = platform_device_register(&mtk_therm_mon_pdev);
    if (retval != 0) {
        return retval;
    }
#endif

    retval = platform_device_register(&ptp_pdev);
    if (retval != 0) {
        return retval;
    }

    retval = platform_device_register(&spm_mcdi_pdev);
    if (retval != 0) {
        return retval;
    }    

    retval = platform_device_register(&golden_setting_pdev);
    if (retval != 0) {
        return retval;
    }

//
//=======================================================================
// Image sensor
//=======================================================================
#if 1 ///defined(CONFIG_VIDEO_CAPTURE_DRIVERS)
    retval = platform_device_register(&sensor_dev);
    if (retval != 0){
    	return retval;
    }
#endif
#if 1 ///defined(CONFIG_VIDEO_CAPTURE_DRIVERS)
    retval = platform_device_register(&sensor_dev_bus2);
    if (retval != 0){
    	return retval;
    }
#endif

//
//=======================================================================
// Lens motor
//=======================================================================
#if 1  //defined(CONFIG_ACTUATOR)
    retval = platform_device_register(&actuator_dev);
    if (retval != 0){
        return retval;
    }
#endif
//
//=======================================================================
// Camera ISP
//=======================================================================

#if !defined(CONFIG_MT6589_FPGA) // If not FPGA.
	printk("ISP platform device registering...\n");
    retval = platform_device_register(&mt_isp_dev);
    if (retval != 0){
	    printk("ISP platform device register failed.\n");
        return retval;
    }
#endif

#if 0
    retval = platform_device_register(&mt_eis_dev);
    if (retval != 0){
        return retval;
    }
#endif

#ifdef CONFIG_RFKILL
    retval = platform_device_register(&mt_rfkill_device);
    if (retval != 0){
        return retval;
    }
#endif

#if 1
	retval = platform_device_register(&camera_sysram_dev);
	if (retval != 0){
		return retval;
	}
#endif

#if defined(CONFIG_MTK_GPS)
	retval = platform_device_register(&mt3326_device_gps);
	if (retval != 0){
		return retval;
	}
#endif

	retval = platform_device_register(&camera_pipemgr_dev);
	if (retval != 0){
		return retval;
	}


#if 1//defined(CONFIG_MTK_NFC) //NFC
	retval = platform_device_register(&mtk_nfc_6605_dev);
	printk("mtk_nfc_6605_dev register ret %d", retval);
	if (retval != 0){
		return retval;
	}
#endif

#if defined (CUSTOM_KERNEL_SSW)	
	retval = platform_device_register(&ssw_device);    
	if (retval != 0) {        
		return retval;    
	}
#endif

    return 0;
}

/*
 * is_pmem_range
 * Input
 *   base: buffer base physical address
 *   size: buffer len in byte
 * Return
 *   1: buffer is located in pmem address range
 *   0: buffer is out of pmem address range
 */
int is_pmem_range(unsigned long *base, unsigned long size)
{
        unsigned long start = (unsigned long)base;
        unsigned long end = start + size;

        //printk("[PMEM] start=0x%p,end=0x%p,size=%d\n", start, end, size);
        //printk("[PMEM] PMEM_MM_START=0x%p,PMEM_MM_SIZE=%d\n", PMEM_MM_START, PMEM_MM_SIZE);

        if (start < PMEM_MM_START)
                return 0;
        if (end >= PMEM_MM_START + PMEM_MM_SIZE)
                return 0;

        return 1;
}
EXPORT_SYMBOL(is_pmem_range);


// return the size of memory from start pfn to max pfn,
// _NOTE_
// the memory area may be discontinuous
unsigned int get_memory_size (void)
{
    return (MAX_PFN) - (PHYS_OFFSET);
}
EXPORT_SYMBOL(get_memory_size) ;


// return the actual physical DRAM size
unsigned int get_max_DRAM_size(void)
{
        return kernel_mem_sz + RESERVED_MEM_MODEM;
}
EXPORT_SYMBOL(get_max_DRAM_size);


unsigned int get_phys_offset(void)
{
	return PHYS_OFFSET;
}
EXPORT_SYMBOL(get_phys_offset);


#include <asm/sections.h>
void get_text_region (unsigned int *s, unsigned int *e)
{
    *s = (unsigned int)_text, *e=(unsigned int)_etext ;
}
EXPORT_SYMBOL(get_text_region) ;
