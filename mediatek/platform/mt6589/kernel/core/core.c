#include <linux/pm.h>
#include <asm/mach/arch.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/hardware/cache-l2x0.h>
#include <asm/smp_scu.h>
#include <mach/mt_reg_base.h>
#include <mach/irqs.h>
#include <asm/page.h>
#include <mach/mt_mci.h>
#include <linux/bug.h>

extern void arm_machine_restart(char mode, const char *cmd);
extern struct sys_timer mt6589_timer;
extern void mt_fixup(struct tag *tags, char **cmdline, struct meminfo *mi);
extern void mt_power_off(void);


void __init mt_init(void)
{
    pm_power_off = mt_power_off;
    panic_on_oops = 1;
}

static struct map_desc mt_io_desc[] __initdata = 
{
     {
        .virtual = VDEC_GCON_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(VDEC_GCON_BASE)),
        .length = SZ_128K,
        .type = MT_DEVICE
    },
    {
        .virtual = VDEC_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(VDEC_BASE)),
        .length = SZ_64K,
        .type = MT_DEVICE
    },
    /*  
    {
        .virtual = MCUSYS_CFGREG_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(MCUSYS_CFGREG_BASE)),
        .length = SZ_64K,
        .type = MT_DEVICE
    },
    {
        .virtual = CORTEXA7MP_BASE,
        .pfn = __phys_to_pfn(CORTEXA7MP_BASE),
        .length = SZ_1M,
        .type = MT_DEVICE
    },*/
    {
        .virtual = TOPRGU_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(TOPRGU_BASE)),
        .length = SZ_4M,
        .type = MT_DEVICE
    },
    /* {
        .virtual = DEBUGTOP_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(DEBUGTOP_BASE)),
        .length = SZ_1M,
        .type = MT_DEVICE
    },*/
     {
        .virtual = AP_DMA_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(AP_DMA_BASE)),
        .length = SZ_2M + SZ_1M,
        .type = MT_DEVICE
    },
    {
        .virtual = SYSRAM_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(SYSRAM_BASE)),
        .length = SZ_256K,
        .type = MT_MEMORY_NONCACHED
    },    
    {
        .virtual = DISPSYS_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(DISPSYS_BASE)),
        .length = SZ_16M,
        .type = MT_DEVICE
    },
    {
        .virtual = IMGSYS_CONFG_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(IMGSYS_CONFG_BASE)),
        .length = SZ_16M,
        .type = MT_DEVICE
    },
    {
        .virtual = AUDIO_TOP_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(AUDIO_TOP_BASE)),
        .length = SZ_64K,
        .type = MT_DEVICE
    },
    {
        .virtual = VENC_TOP_BASE,
        .pfn = __phys_to_pfn(IO_VIRT_TO_PHYS(VENC_TOP_BASE)),
        .length = SZ_64K,
        .type = MT_DEVICE
    },
    {
        .virtual = DEVINFO_BASE,
        .pfn = __phys_to_pfn(0x08000000),
        .length = SZ_64K,
        .type = MT_DEVICE
    },
    {
        .virtual = INTER_SRAM,
        .pfn = __phys_to_pfn(0x00100000),
        .length = SZ_64K,
        .type = MT_MEMORY_NONCACHED
    },


};

void __init mt_map_io(void)
{
    iotable_init(mt_io_desc, ARRAY_SIZE(mt_io_desc));
}

#ifdef MTK_TABLET_PLATFORM
MACHINE_START(MT6589, MTK_TABLET_PLATFORM)
#else
MACHINE_START(MT6589, "MT6589")
#endif
    .atag_offset    = 0x00000100,
    .map_io         = mt_map_io,
    .init_irq       = mt_init_irq,
    .timer          = &mt6589_timer,
    .init_machine   = mt_init,
    .fixup          = mt_fixup,
    .restart        = arm_machine_restart
MACHINE_END
