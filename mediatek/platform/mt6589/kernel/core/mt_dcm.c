#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#include <mach/mt_typedefs.h>
#include <mach/sync_write.h>
#include <mach/mt_dcm.h>
#include <mach/mt_clkmgr.h>


#define USING_XLOG

#ifdef USING_XLOG 

#include <linux/xlog.h>
#define TAG     "Power/dcm"

#define dcm_err(fmt, args...)       \
    xlog_printk(ANDROID_LOG_ERROR, TAG, fmt, ##args)
#define dcm_warn(fmt, args...)      \
    xlog_printk(ANDROID_LOG_WARN, TAG, fmt, ##args)
#define dcm_info(fmt, args...)      \
    xlog_printk(ANDROID_LOG_INFO, TAG, fmt, ##args)
#define dcm_dbg(fmt, args...)       \
    xlog_printk(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define dcm_ver(fmt, args...)       \
    xlog_printk(ANDROID_LOG_VERBOSE, TAG, fmt, ##args)

#else /* !USING_XLOG */

#define TAG     "[Power/dcm] "

#define dcm_err(fmt, args...)       \
    printk(KERN_ERR TAG);           \
    printk(KERN_CONT fmt, ##args) 
#define dcm_warn(fmt, args...)      \
    printk(KERN_WARNING TAG);       \
    printk(KERN_CONT fmt, ##args)
#define dcm_info(fmt, args...)      \
    printk(KERN_NOTICE TAG);        \
    printk(KERN_CONT fmt, ##args)
#define dcm_dbg(fmt, args...)       \
    printk(KERN_INFO TAG);          \
    printk(KERN_CONT fmt, ##args)
#define dcm_ver(fmt, args...)       \
    printk(KERN_DEBUG TAG);         \
    printk(KERN_CONT fmt, ##args)

#endif


#define dcm_readl(addr) \
    DRV_Reg32(addr)

#define dcm_writel(addr, val)   \
    mt65xx_reg_sync_writel(val, addr)

#define dcm_setl(addr, val) \
    mt65xx_reg_sync_writel(dcm_readl(addr) | (val), addr)

#define dcm_clrl(addr, val) \
    mt65xx_reg_sync_writel(dcm_readl(addr) & ~(val), addr)


static DEFINE_MUTEX(dcm_lock);

static unsigned int dcm_sta = 0;

void dcm_dump_regs(unsigned int type)
{
    volatile unsigned int dcm_cfg;

    mutex_lock(&dcm_lock);

    dcm_cfg = dcm_readl(DCM_CFG);
    dcm_info("[BUS_DCM]DCM_CFG(0x%08x)\n", dcm_cfg);

    if (type & CPU_DCM) {
        volatile unsigned int fsel, dbc, ctl;
        fsel = dcm_readl(TOP_CA7DCMFSEL);
        dbc = dcm_readl(TOP_DCMDBC);
        ctl = dcm_readl(TOP_DCMCTL);
        dcm_info("[CPU_DCM]FSEL(0x%08x), DBC(0x%08x), CTL(0x%08x)\n",
                fsel, dbc, ctl);
    }

    if (type & IFR_DCM) {
        volatile unsigned int fsel, dbc, ctl;
        volatile unsigned int dramc;
        fsel = dcm_readl(INFRA_DCMFSEL);
        dbc = dcm_readl(INFRA_DCMDBC);
        ctl = dcm_readl(INFRA_DCMCTL);
        dramc = dcm_readl(DRAMC_PD_CTRL);
        dcm_info("[IFR_DCM]FSEL(0x%08x), DBC(0x%08x), CTL(0x%08x)\n", 
                fsel, dbc, ctl);
        dcm_info("[IFR_DCM]DRAMC_PD_CTRL(0x%08x)\n", dramc);
    }

    if (type & PER_DCM) {
        volatile unsigned int fsel, dbc, ctl;
        fsel = dcm_readl(PERI_GCON_DCMFSEL);
        dbc = dcm_readl(PERI_GCON_DCMDBC);
        ctl = dcm_readl(PERI_GCON_DCMCTL);
        dcm_info("[PER_DCM]FSEL(0x%08x), DBC(0x%08x), CTL(0x%08x)\n", 
                fsel, dbc, ctl);
    }

    if (type & SMI_DCM) {
        volatile unsigned int smi_com_dcm, smi_sec_dcm, m4u_dcm;
        smi_com_dcm = dcm_readl(SMI_COMMON_DCM);
        smi_sec_dcm = dcm_readl(SMI_SECURE_DCMCON);
        m4u_dcm = dcm_readl(M4U_DCM);
        dcm_info("[SMI_DCM]SMI_COMMON_DCM(0x%08x), SMI_SECURE_DCM(0x%08x)\n" , 
                smi_com_dcm, smi_sec_dcm);
        dcm_info("[SMI_DCM]M4U_DCM(0x%08x)\n", m4u_dcm);
    }

    if (type & MFG_DCM) {
        if (subsys_is_on(SYS_MFG)) {
            volatile unsigned int mfg0, mfg1;
            mfg0 = dcm_readl(MFG_DCM_CON0);
            mfg1 = dcm_readl(MFG_DCM_CON0);
            dcm_info("[MFG_DCM]MFG_DCM_CON0(0x%08x), MFG_DCM_CON1(0x%08x)\n", 
                    mfg0, mfg1);
        } else {
            dcm_info("[MFG_DCM]subsy MFG is off\n");
        }
    }

    if (type & DIS_DCM) {
        if (subsys_is_on(SYS_DIS)) {
            volatile unsigned int dis0, dis1, larb2;
            dis0 = dcm_readl(DISP_HW_DCM_DIS0);
            dis1 = dcm_readl(DISP_HW_DCM_DIS1);
            larb2 = dcm_readl(SMILARB2_DCM_CON);
            dcm_info("[DIS_DCM]DISP_HW_DCM_DIS0(0x%08x), DISP_HW_DCM_DIS1(0x%08x)\n", 
                    dis0, dis1);
            dcm_info("[DIS_DCM]SMILARB2_DCM_CON(0x%08x)\n", larb2);
        } else {
            dcm_info("[DIS_DCM]subsys DIS is off\n");
        }
    }

    if (type & ISP_DCM) {
        if (subsys_is_on(SYS_ISP)) {
            volatile unsigned int raw, rgb, yuv, cdp;
            volatile unsigned int jpgdec, jpgenc, isp_com, larb3, larb4; 
            raw = dcm_readl(CAM_CTL_RAW_DCM);
            rgb = dcm_readl(CAM_CTL_RGB_DCM);
            yuv = dcm_readl(CAM_CTL_YUV_DCM);
            cdp = dcm_readl(CAM_CTL_CDP_DCM);
            jpgdec = dcm_readl(JPGDEC_DCM_CTRL);
            jpgenc = dcm_readl(JPGENC_DCM_CTRL);
            isp_com = dcm_readl(SMI_ISP_COMMON_DCMCON);
            larb3 = dcm_readl(SMILARB3_DCM_CON);
            larb4 = dcm_readl(SMILARB4_DCM_CON);
            dcm_info("[ISP_DCM]CAM_CTL_RAW_DCM(0x%08x), CAM_CTL_RGB_DCM(0x%08x)\n", 
                    raw, rgb);
            dcm_info("[ISP_DCM]CAM_CTL_YUV_DCM(0x%08x), CAM_CTL_CDP_DCM(0x%08x)\n", 
                    yuv, cdp);
            dcm_info("[ISP_DCM]JPGDEC_DCM_CTRL(0x%08x), JPGENC_DCM_CTRL(0x%08x)\n",
                    jpgdec, jpgenc);
            dcm_info("[ISP_DCM]SMI_ISP_COMMON_DCMCON(0x%08x)\n", isp_com);
            dcm_info("[ISP_DCM]SMILARB3_DCM_CON(0x%08x), SMILARB4_DCM_CON(0x%08x)\n",
                    larb3, larb4);
        } else {
            dcm_info("[ISP_DCM]subsys ISP is off\n");
        }
    }

    if (type & VDE_DCM) {
        if (subsys_is_on(SYS_VDE)) {
            volatile unsigned int vdec, larb1;
            vdec = dcm_readl(VDEC_DCM_CON);
            larb1 = dcm_readl(SMILARB1_DCM_CON);
            dcm_info("[VDE_DCM]VDEC_DCM_CON(0x%08x), SMILARB1_DCM_CON(0x%08x)\n",
                    vdec, larb1);
        } else {
            dcm_info("[VDE_DCM]subsys VDE is off\n");
        }
    }

    if (type & VEN_DCM) {
        if (subsys_is_on(SYS_VEN)) {
            volatile unsigned int dcm_ctl, cg_ctl, mp4_dcm, larb0;
            dcm_writel(VENC_CE, 0x1);
            dcm_ctl = dcm_readl(VENC_CLK_DCM_CTRL);
            cg_ctl = dcm_readl(VENC_CLK_CG_CTRL);
            mp4_dcm = dcm_readl(VENC_MP4_DCM_CTRL);
            larb0 = dcm_readl(SMILARB0_DCM_CON);
            dcm_info("[VEN_DCM]VENC_CE=0x%08x", dcm_readl(VENC_CE));
            dcm_info("[VEN_DCM]VENC_CLK_DCM_CTRL(0x%08x), VENC_CLK_CG_CTRL(0x%08x)\n",
                    dcm_ctl, cg_ctl);
            dcm_info("[VEN_DCM]VENC_MP4_DCM_CTRL(0x%08x), SMILARB0_DCM_CON(0x%08x)\n",
                    mp4_dcm, larb0);
        } else {
            dcm_info("[VEN_DCM]subsys VEN is off\n");
        }
    }

    mutex_unlock(&dcm_lock);
}

void dcm_enable(unsigned int type)
{
    dcm_info("[%s]type:0x%08x\n", __func__, type);

    mutex_lock(&dcm_lock);
    //dcm_sta |= type & ALL_DCM;

    if (type & CPU_DCM) {
        dcm_writel(TOP_CA7DCMFSEL, 0x7000000);
        dcm_writel(TOP_DCMDBC, 0x1); // force to 26M
        dcm_setl(TOP_DCMCTL, 0x3 << 1);
        dcm_sta |= CPU_DCM;
    }

#if 0
    if (type & BUS_DCM) {
        //1. AXI bus dcm
        dcm_setl(DCM_CFG, 0x1 << 7);
        dcm_sta |= BUS_DCM;
    }
#endif

    if (type & IFR_DCM) {
        //dcm_writel(INFRA_DCMFSEL, 0x0);// divided most
        dcm_writel(INFRA_DCMFSEL, 0x001F0000);// divided by 1
        dcm_writel(INFRA_DCMDBC, 0x0000037F);
        dcm_setl(INFRA_DCMCTL, 0x103);
        dcm_setl(DRAMC_PD_CTRL, 0x3 << 24);
        dcm_sta |= IFR_DCM;
    }

    if (type & PER_DCM) {
        dcm_writel(PERI_GCON_DCMFSEL, 0x0);
        dcm_writel(PERI_GCON_DCMCTL, 0xF3);
        dcm_sta |= PER_DCM;
    }

    if (type & SMI_DCM) {
        //smi_common
        dcm_writel(SMI_COMMON_DCM, 0x1 << 0);
        dcm_writel(SMI_SECURE_DCMSET, 0x1 << 2);
        //m4u_dcm
        dcm_writel(M4U_DCM, 0x1);
        dcm_sta |= SMI_DCM;
    }

    if (type & MFG_DCM) {
        if (subsys_is_on(SYS_MFG)) {            
            //dcm_setl(MFG_DCM_CON0, 0x80008000); 
            //dcm_setl(MFG_DCM_CON1, 0x8000);
            dcm_writel(MFG_DCM_CON0, 0xC03FC03F); 
            dcm_writel(MFG_DCM_CON1, 0xC03F);
            dcm_sta |= MFG_DCM;
        }
    }

    if (type & DIS_DCM) {
        if (subsys_is_on(SYS_DIS)) {
            dcm_writel(DISP_HW_DCM_DIS_CLR0, 0xFFFFFF);  
            dcm_writel(DISP_HW_DCM_DIS_CLR1, 0x7);  
            dcm_writel(SMILARB2_DCM_SET, 0x3 << 15);
            dcm_sta |= DIS_DCM;
        }
    }

    if (type & ISP_DCM) {
        if (subsys_is_on(SYS_ISP)) {
            dcm_writel(CAM_CTL_RAW_DCM, 0x0);
            dcm_writel(CAM_CTL_RGB_DCM, 0x0);
            dcm_writel(CAM_CTL_YUV_DCM, 0x0);
            dcm_writel(CAM_CTL_CDP_DCM, 0x0);

            dcm_writel(JPGDEC_DCM_CTRL, 0x0);
            dcm_writel(JPGENC_DCM_CTRL, 0x0);

            dcm_writel(SMI_ISP_COMMON_DCMSET, 0x1 << 1);

            dcm_writel(SMILARB3_DCM_SET, 0x3 << 15);
            dcm_writel(SMILARB4_DCM_SET, 0x3 << 15);

            dcm_sta |= ISP_DCM;
        }
    }

    if (type & VDE_DCM) {
        if (subsys_is_on(SYS_VDE)) {
            dcm_writel(VDEC_DCM_CON, 0x0);
            dcm_writel(SMILARB1_DCM_SET, 0x3 << 15);

            dcm_sta |= VDE_DCM;
        }
    }

    if (type & VEN_DCM) {
        if (subsys_is_on(SYS_VEN)) {
            dcm_writel(VENC_CE, 0x1);
            dcm_writel(VENC_CLK_DCM_CTRL, 0x1);
            dcm_writel(VENC_CLK_CG_CTRL, 0xFFFFFFFF);
            dcm_writel(VENC_MP4_DCM_CTRL, 0x0);
            dcm_writel(SMILARB0_DCM_SET, 0x3 << 15);

            dcm_sta |= VEN_DCM;
        }
    }

    mutex_unlock(&dcm_lock);
}

void dcm_disable(unsigned int type)
{
    dcm_info("[%s]type:0x%08x\n", __func__, type);

    mutex_lock(&dcm_lock);
    //dcm_sta &= ~(type & ALL_DCM);

    if (type & CPU_DCM) {
        dcm_clrl(TOP_DCMCTL, 0x3 << 1);
        dcm_sta &= ~CPU_DCM;
    }

#if 0
    if (type & BUS_DCM) {
        //1. AXI bus dcm
        dcm_clrl(DCM_CFG, 0x1 << 7);
    }
#endif

    if (type & IFR_DCM) {
        dcm_clrl(INFRA_DCMCTL, 0x103);
        dcm_clrl(DRAMC_PD_CTRL, 0x1 << 25);
        dcm_sta &= ~IFR_DCM;
    }

    if (type & PER_DCM) {
        dcm_writel(PERI_GCON_DCMCTL, 0xF2);
        dcm_sta &= ~PER_DCM;
    }

    if (type & SMI_DCM) {
        //smi_common
        dcm_writel(SMI_COMMON_DCM, 0x0);
        dcm_writel(SMI_SECURE_DCMCLR, 0x1 << 2);
        //m4u_dcm
        dcm_writel(M4U_DCM, 0x0);
        dcm_sta &= ~SMI_DCM;
    }

    if (type & MFG_DCM) {
        if (subsys_is_on(SYS_MFG)) {            
            dcm_clrl(MFG_DCM_CON0, 0x80008000); 
            dcm_clrl(MFG_DCM_CON1, 0x8000);
            dcm_sta &= ~MFG_DCM;
        }
    }

    if (type & DIS_DCM) {
        if (subsys_is_on(SYS_DIS)) {            
            dcm_writel(DISP_HW_DCM_DIS_SET0, 0xFFFFFF);  
            dcm_writel(DISP_HW_DCM_DIS_SET1, 0x7);  
            dcm_writel(SMILARB2_DCM_CLR, 0x3 << 15);
            dcm_sta &= ~DIS_DCM;
        }
    }

    if (type & ISP_DCM) {
        if (subsys_is_on(SYS_ISP)) {            
            dcm_writel(CAM_CTL_RAW_DCM, 0x7FFF);
            dcm_writel(CAM_CTL_RGB_DCM, 0xFF);
            dcm_writel(CAM_CTL_YUV_DCM, 0xF);
            dcm_writel(CAM_CTL_CDP_DCM, 0x1FF);

            dcm_writel(JPGDEC_DCM_CTRL, 0x1);
            dcm_writel(JPGENC_DCM_CTRL, 0x1);

            dcm_writel(SMI_ISP_COMMON_DCMCLR, 0x1 << 1);

            dcm_writel(SMILARB3_DCM_CLR, 0x3 << 15);
            dcm_writel(SMILARB4_DCM_CLR, 0x3 << 15);
            dcm_sta &= ~ISP_DCM;
        }
    }

    if (type & VDE_DCM) {
        if (subsys_is_on(SYS_VDE)) {            
            dcm_writel(VDEC_DCM_CON, 0x1);
            dcm_writel(SMILARB1_DCM_CLR, 0x3 << 15);
            dcm_sta &= ~VDE_DCM;
        }
    }

    if (type & VEN_DCM) {
        if (subsys_is_on(SYS_VEN)) {            
            dcm_writel(VENC_CE, 0x1);
            dcm_writel(VENC_CLK_DCM_CTRL, 0x0);
            dcm_writel(VENC_CLK_CG_CTRL, 0x0);
            dcm_writel(VENC_MP4_DCM_CTRL, 0x1);
            dcm_writel(SMILARB0_DCM_CLR, 0x3 << 15);
            dcm_sta &= ~VEN_DCM;
        }
    }

    mutex_unlock(&dcm_lock);
}

void bus_dcm_enable(void)
{
    dcm_writel(DCM_CFG, 0x1 << 7 | 0xF);
}

void bus_dcm_disable(void)
{
    dcm_clrl(DCM_CFG, 0x1 << 7);
}

static unsigned int infra_dcm = 0;
void disable_infra_dcm(void)
{
    infra_dcm = dcm_readl(INFRA_DCMCTL); 
    dcm_clrl(INFRA_DCMCTL, 0x100);
}

void restore_infra_dcm(void)
{
    dcm_writel(INFRA_DCMCTL, infra_dcm);
}

static unsigned int peri_dcm = 0;
void disable_peri_dcm(void)
{
    peri_dcm = dcm_readl(PERI_GCON_DCMCTL);
    dcm_clrl(PERI_GCON_DCMCTL, 0x1);
}

void restore_peri_dcm(void)
{
    dcm_writel(PERI_GCON_DCMCTL, peri_dcm);
}

#define dcm_attr(_name)                         \
static struct kobj_attribute _name##_attr = {   \
    .attr = {                                   \
        .name = __stringify(_name),             \
        .mode = 0644,                           \
    },                                          \
    .show = _name##_show,                       \
    .store = _name##_store,                     \
}

static const char *dcm_name[NR_DCMS] = {
    "CPU_DCM",
    "IFR_DCM",
    "PER_DCM",
    "SMI_DCM",
    "MFG_DCM",
    "DIS_DCM",
    "ISP_DCM",
    "VDE_DCM",
    "VEN_DCM",
};

static ssize_t dcm_state_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int len = 0;
    char *p = buf;

    int i;
    unsigned int sta;
    
    p += sprintf(p, "********** dcm_state dump **********\n");
    mutex_lock(&dcm_lock);
    
    for (i = 0; i < NR_DCMS; i++) {
        sta = dcm_sta & (0x1 << i);
        p += sprintf(p, "[%d][%s]%s\n", i, dcm_name[i], sta ? "on" : "off");
    }

    mutex_unlock(&dcm_lock);

    p += sprintf(p, "\n********** dcm_state help *********\n");
    p += sprintf(p, "enable dcm:    echo enable mask(dec) > /sys/power/dcm_state\n");
    p += sprintf(p, "disable dcm:   echo disable mask(dec) > /sys/power/dcm_state\n");
    p += sprintf(p, "dump reg:      echo dump mask(dec) > /sys/power/dcm_state\n");


    len = p - buf;
    return len;
}

static ssize_t dcm_state_store(struct kobject *kobj, struct kobj_attribute *attr,const char *buf, size_t n)
{
    char cmd[10];
    unsigned int mask;

    if (sscanf(buf, "%s %x", cmd, &mask) == 2) {
        mask &= ALL_DCM;
        if (!strcmp(cmd, "enable")) {
            dcm_dump_regs(mask);
            dcm_enable(mask);
            dcm_dump_regs(mask);
        } else if (!strcmp(cmd, "disable")) {
            dcm_dump_regs(mask);
            dcm_disable(mask);
            dcm_dump_regs(mask);
        } else if (!strcmp(cmd, "dump")) {
            dcm_dump_regs(mask);
        }
        return n;
    }
    
    return -EINVAL;
}
dcm_attr(dcm_state);


void mt_dcm_init(void)
{
    int err = 0;
    
    dcm_info("[%s]entry!!\n", __func__);
    dcm_enable(ALL_DCM);
    //dcm_enable(ALL_DCM & (~IFR_DCM));
    
    err = sysfs_create_file(power_kobj, &dcm_state_attr.attr);

    if (err) {
        dcm_err("[%s]: fail to create sysfs\n", __func__);
    }
}
