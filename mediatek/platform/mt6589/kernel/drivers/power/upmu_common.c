#include <linux/kernel.h>
#include <linux/xlog.h>
#include <linux/module.h>

#include <mach/pmic_mt6320_sw.h>
#include <mach/upmu_common.h>
#include <mach/upmu_hw.h>

U32 upmu_get_rgs_vcdt_hv_det(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON0),
                           (&val),
                           (U32)(PMIC_RGS_VCDT_HV_DET_MASK),
                           (U32)(PMIC_RGS_VCDT_HV_DET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rgs_vcdt_lv_det(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON0),
                           (&val),
                           (U32)(PMIC_RGS_VCDT_LV_DET_MASK),
                           (U32)(PMIC_RGS_VCDT_LV_DET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rgs_chrdet(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON0),
                           (&val),
                           (U32)(PMIC_RGS_CHRDET_MASK),
                           (U32)(PMIC_RGS_CHRDET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_chr_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_CHR_EN_MASK),
                             (U32)(PMIC_RG_CHR_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_csdac_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_CSDAC_EN_MASK),
                             (U32)(PMIC_RG_CSDAC_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_automode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_AUTOMODE_MASK),
                             (U32)(PMIC_RG_PCHR_AUTOMODE_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rgs_chr_ldo_det(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON0),
                           (&val),
                           (U32)(PMIC_RGS_CHR_LDO_DET_MASK),
                           (U32)(PMIC_RGS_CHR_LDO_DET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_vcdt_hv_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VCDT_HV_EN_MASK),
                             (U32)(PMIC_RG_VCDT_HV_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcdt_hv_vth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VCDT_HV_VTH_MASK),
                             (U32)(PMIC_RG_VCDT_HV_VTH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcdt_lv_vth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VCDT_LV_VTH_MASK),
                             (U32)(PMIC_RG_VCDT_LV_VTH_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rgs_vbat_cc_det(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON2),
                           (&val),
                           (U32)(PMIC_RGS_VBAT_CC_DET_MASK),
                           (U32)(PMIC_RGS_VBAT_CC_DET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rgs_vbat_cv_det(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON2),
                           (&val),
                           (U32)(PMIC_RGS_VBAT_CV_DET_MASK),
                           (U32)(PMIC_RGS_VBAT_CV_DET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rgs_cs_det(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON2),
                           (&val),
                           (U32)(PMIC_RGS_CS_DET_MASK),
                           (U32)(PMIC_RGS_CS_DET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_cs_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_CS_EN_MASK),
                             (U32)(PMIC_RG_CS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbat_cc_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VBAT_CC_EN_MASK),
                             (U32)(PMIC_RG_VBAT_CC_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbat_cv_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VBAT_CV_EN_MASK),
                             (U32)(PMIC_RG_VBAT_CV_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbat_cc_vth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VBAT_CC_VTH_MASK),
                             (U32)(PMIC_RG_VBAT_CC_VTH_SHIFT)
	                         );
  pmic_unlock();
}

extern int g_pmic_cid;

void upmu_set_rg_vbat_cv_vth(U32 val)
{
  U32 ret=0;

  pmic_lock();

  if(g_pmic_cid == 0x1020)
  {
    if( (val==0x01) || (val==0x02) || (val==0x03) || (val==0x04) ||
        (val==0x05) || (val==0x06) || (val==0x07) || (val==0x08) ||
        (val==0x09) || (val==0x0A) || (val==0x0B) || (val==0x0C) 
        )
    {
        printk("[upmu_set_rg_vbat_cv_vth] cannot set %d in 0x1020, need reset to 0x0 \n", val);
        val=0x0; // reset to 4.2V        
    }
  }
  
  ret=pmic_config_interface( (U32)(CHR_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VBAT_CV_VTH_MASK),
                             (U32)(PMIC_RG_VBAT_CV_VTH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_cs_vth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_CS_VTH_MASK),
                             (U32)(PMIC_RG_CS_VTH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_toltc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_TOLTC_MASK),
                             (U32)(PMIC_RG_PCHR_TOLTC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_tohtc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_TOHTC_MASK),
                             (U32)(PMIC_RG_PCHR_TOHTC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rgs_vbat_ov_det(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON6),
                           (&val),
                           (U32)(PMIC_RGS_VBAT_OV_DET_MASK),
                           (U32)(PMIC_RGS_VBAT_OV_DET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_vbat_ov_deg(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_VBAT_OV_DEG_MASK),
                             (U32)(PMIC_RG_VBAT_OV_DEG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbat_ov_vth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_VBAT_OV_VTH_MASK),
                             (U32)(PMIC_RG_VBAT_OV_VTH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbat_ov_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_VBAT_OV_EN_MASK),
                             (U32)(PMIC_RG_VBAT_OV_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rgs_baton_undet(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON7),
                           (&val),
                           (U32)(PMIC_RGS_BATON_UNDET_MASK),
                           (U32)(PMIC_RGS_BATON_UNDET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_baton_ht_trim_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_BATON_HT_TRIM_SET_MASK),
                             (U32)(PMIC_RG_BATON_HT_TRIM_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_baton_ht_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_BATON_HT_TRIM_MASK),
                             (U32)(PMIC_RG_BATON_HT_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_baton_tdet_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON7),
                             (U32)(val),
                             (U32)(PMIC_BATON_TDET_EN_MASK),
                             (U32)(PMIC_BATON_TDET_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_baton_ht_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_BATON_HT_EN_MASK),
                             (U32)(PMIC_RG_BATON_HT_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_baton_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_BATON_EN_MASK),
                             (U32)(PMIC_RG_BATON_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_csdac_data(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_CSDAC_DATA_MASK),
                             (U32)(PMIC_RG_CSDAC_DATA_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_frc_csvth_usbdl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_FRC_CSVTH_USBDL_MASK),
                             (U32)(PMIC_RG_FRC_CSVTH_USBDL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rgs_otg_bvalid_det(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON10),
                           (&val),
                           (U32)(PMIC_RGS_OTG_BVALID_DET_MASK),
                           (U32)(PMIC_RGS_OTG_BVALID_DET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_otg_bvalid_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_OTG_BVALID_EN_MASK),
                             (U32)(PMIC_RG_OTG_BVALID_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_flag_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_FLAG_EN_MASK),
                             (U32)(PMIC_RG_PCHR_FLAG_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rgs_pchr_flag_out(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON10),
                           (&val),
                           (U32)(PMIC_RGS_PCHR_FLAG_OUT_MASK),
                           (U32)(PMIC_RGS_PCHR_FLAG_OUT_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_pchr_flag_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_FLAG_SEL_MASK),
                             (U32)(PMIC_RG_PCHR_FLAG_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_ft_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_FT_CTRL_MASK),
                             (U32)(PMIC_RG_PCHR_FT_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_RST_MASK),
                             (U32)(PMIC_RG_PCHR_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_csdac_testmode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_CSDAC_TESTMODE_MASK),
                             (U32)(PMIC_RG_CSDAC_TESTMODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_testmode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_TESTMODE_MASK),
                             (U32)(PMIC_RG_PCHR_TESTMODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_chrwdt_wr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON13),
                             (U32)(val),
                             (U32)(PMIC_RG_CHRWDT_WR_MASK),
                             (U32)(PMIC_RG_CHRWDT_WR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_chrwdt_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON13),
                             (U32)(val),
                             (U32)(PMIC_RG_CHRWDT_EN_MASK),
                             (U32)(PMIC_RG_CHRWDT_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_chrwdt_td(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON13),
                             (U32)(val),
                             (U32)(PMIC_RG_CHRWDT_TD_MASK),
                             (U32)(PMIC_RG_CHRWDT_TD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_rv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON14),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_RV_MASK),
                             (U32)(PMIC_RG_PCHR_RV_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rgs_chrwdt_out(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON15),
                           (&val),
                           (U32)(PMIC_RGS_CHRWDT_OUT_MASK),
                           (U32)(PMIC_RGS_CHRWDT_OUT_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_chrwdt_flag_wr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON15),
                             (U32)(val),
                             (U32)(PMIC_RG_CHRWDT_FLAG_WR_MASK),
                             (U32)(PMIC_RG_CHRWDT_FLAG_WR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_chrwdt_int_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON15),
                             (U32)(val),
                             (U32)(PMIC_RG_CHRWDT_INT_EN_MASK),
                             (U32)(PMIC_RG_CHRWDT_INT_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_adcin_vchr_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON16),
                             (U32)(val),
                             (U32)(PMIC_ADCIN_VCHR_EN_MASK),
                             (U32)(PMIC_ADCIN_VCHR_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_adcin_vsen_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON16),
                             (U32)(val),
                             (U32)(PMIC_ADCIN_VSEN_EN_MASK),
                             (U32)(PMIC_ADCIN_VSEN_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_adcin_vbat_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON16),
                             (U32)(val),
                             (U32)(PMIC_ADCIN_VBAT_EN_MASK),
                             (U32)(PMIC_ADCIN_VBAT_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_adcin_vsen_ext_baton_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON16),
                             (U32)(val),
                             (U32)(PMIC_RG_ADCIN_VSEN_EXT_BATON_EN_MASK),
                             (U32)(PMIC_RG_ADCIN_VSEN_EXT_BATON_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_adcin_vsen_mux_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON16),
                             (U32)(val),
                             (U32)(PMIC_ADCIN_VSEN_MUX_EN_MASK),
                             (U32)(PMIC_ADCIN_VSEN_MUX_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_usbdl_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON16),
                             (U32)(val),
                             (U32)(PMIC_RG_USBDL_SET_MASK),
                             (U32)(PMIC_RG_USBDL_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_usbdl_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON16),
                             (U32)(val),
                             (U32)(PMIC_RG_USBDL_RST_MASK),
                             (U32)(PMIC_RG_USBDL_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_uvlo_vthl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON16),
                             (U32)(val),
                             (U32)(PMIC_RG_UVLO_VTHL_MASK),
                             (U32)(PMIC_RG_UVLO_VTHL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bgr_unchop(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_BGR_UNCHOP_MASK),
                             (U32)(PMIC_RG_BGR_UNCHOP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bgr_unchop_ph(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_BGR_UNCHOP_PH_MASK),
                             (U32)(PMIC_RG_BGR_UNCHOP_PH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bgr_rsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_BGR_RSEL_MASK),
                             (U32)(PMIC_RG_BGR_RSEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rgs_bc11_cmp_out(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHR_CON18),
                           (&val),
                           (U32)(PMIC_RGS_BC11_CMP_OUT_MASK),
                           (U32)(PMIC_RGS_BC11_CMP_OUT_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_bc11_vsrc_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON18),
                             (U32)(val),
                             (U32)(PMIC_RG_BC11_VSRC_EN_MASK),
                             (U32)(PMIC_RG_BC11_VSRC_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bc11_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON18),
                             (U32)(val),
                             (U32)(PMIC_RG_BC11_RST_MASK),
                             (U32)(PMIC_RG_BC11_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bc11_bb_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON18),
                             (U32)(val),
                             (U32)(PMIC_RG_BC11_BB_CTRL_MASK),
                             (U32)(PMIC_RG_BC11_BB_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bc11_bias_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON19),
                             (U32)(val),
                             (U32)(PMIC_RG_BC11_BIAS_EN_MASK),
                             (U32)(PMIC_RG_BC11_BIAS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bc11_ipu_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON19),
                             (U32)(val),
                             (U32)(PMIC_RG_BC11_IPU_EN_MASK),
                             (U32)(PMIC_RG_BC11_IPU_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bc11_ipd_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON19),
                             (U32)(val),
                             (U32)(PMIC_RG_BC11_IPD_EN_MASK),
                             (U32)(PMIC_RG_BC11_IPD_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bc11_cmp_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON19),
                             (U32)(val),
                             (U32)(PMIC_RG_BC11_CMP_EN_MASK),
                             (U32)(PMIC_RG_BC11_CMP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bc11_vref_vth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON19),
                             (U32)(val),
                             (U32)(PMIC_RG_BC11_VREF_VTH_MASK),
                             (U32)(PMIC_RG_BC11_VREF_VTH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_csdac_stp_dec(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON20),
                             (U32)(val),
                             (U32)(PMIC_RG_CSDAC_STP_DEC_MASK),
                             (U32)(PMIC_RG_CSDAC_STP_DEC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_csdac_stp_inc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON20),
                             (U32)(val),
                             (U32)(PMIC_RG_CSDAC_STP_INC_MASK),
                             (U32)(PMIC_RG_CSDAC_STP_INC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_csdac_stp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON21),
                             (U32)(val),
                             (U32)(PMIC_RG_CSDAC_STP_MASK),
                             (U32)(PMIC_RG_CSDAC_STP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_csdac_dly(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON21),
                             (U32)(val),
                             (U32)(PMIC_RG_CSDAC_DLY_MASK),
                             (U32)(PMIC_RG_CSDAC_DLY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_chrind_dimming(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON22),
                             (U32)(val),
                             (U32)(PMIC_RG_CHRIND_DIMMING_MASK),
                             (U32)(PMIC_RG_CHRIND_DIMMING_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_chrind_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON22),
                             (U32)(val),
                             (U32)(PMIC_RG_CHRIND_ON_MASK),
                             (U32)(PMIC_RG_CHRIND_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_low_ich_db(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON22),
                             (U32)(val),
                             (U32)(PMIC_RG_LOW_ICH_DB_MASK),
                             (U32)(PMIC_RG_LOW_ICH_DB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_ulc_det_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON23),
                             (U32)(val),
                             (U32)(PMIC_RG_ULC_DET_EN_MASK),
                             (U32)(PMIC_RG_ULC_DET_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hwcv_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON23),
                             (U32)(val),
                             (U32)(PMIC_RG_HWCV_EN_MASK),
                             (U32)(PMIC_RG_HWCV_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_tracking_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON23),
                             (U32)(val),
                             (U32)(PMIC_RG_TRACKING_EN_MASK),
                             (U32)(PMIC_RG_TRACKING_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_csdac_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON23),
                             (U32)(val),
                             (U32)(PMIC_RG_CSDAC_MODE_MASK),
                             (U32)(PMIC_RG_CSDAC_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcdt_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON23),
                             (U32)(val),
                             (U32)(PMIC_RG_VCDT_MODE_MASK),
                             (U32)(PMIC_RG_VCDT_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_cv_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON23),
                             (U32)(val),
                             (U32)(PMIC_RG_CV_MODE_MASK),
                             (U32)(PMIC_RG_CV_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_ichrg_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON24),
                             (U32)(val),
                             (U32)(PMIC_RG_ICHRG_TRIM_MASK),
                             (U32)(PMIC_RG_ICHRG_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bgr_trim_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON24),
                             (U32)(val),
                             (U32)(PMIC_RG_BGR_TRIM_EN_MASK),
                             (U32)(PMIC_RG_BGR_TRIM_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bgr_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON25),
                             (U32)(val),
                             (U32)(PMIC_RG_BGR_TRIM_MASK),
                             (U32)(PMIC_RG_BGR_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_ovp_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON26),
                             (U32)(val),
                             (U32)(PMIC_RG_OVP_TRIM_MASK),
                             (U32)(PMIC_RG_OVP_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bgr_test_rstb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON27),
                             (U32)(val),
                             (U32)(PMIC_RG_BGR_TEST_RSTB_MASK),
                             (U32)(PMIC_RG_BGR_TEST_RSTB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bgr_test_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON27),
                             (U32)(val),
                             (U32)(PMIC_RG_BGR_TEST_EN_MASK),
                             (U32)(PMIC_RG_BGR_TEST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_qi_bgr_ext_buf_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON27),
                             (U32)(val),
                             (U32)(PMIC_QI_BGR_EXT_BUF_EN_MASK),
                             (U32)(PMIC_QI_BGR_EXT_BUF_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_chr_osc_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON27),
                             (U32)(val),
                             (U32)(PMIC_CHR_OSC_TRIM_MASK),
                             (U32)(PMIC_CHR_OSC_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_dac_usbdl_max(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON28),
                             (U32)(val),
                             (U32)(PMIC_RG_DAC_USBDL_MAX_MASK),
                             (U32)(PMIC_RG_DAC_USBDL_MAX_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHR_CON29),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_RSV_MASK),
                             (U32)(PMIC_RG_PCHR_RSV_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_cid(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CID),
                           (&val),
                           (U32)(PMIC_CID_MASK),
                           (U32)(PMIC_CID_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_strup_6m_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_STRUP_6M_PDN_MASK),
                             (U32)(PMIC_RG_STRUP_6M_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_accdet_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_ACCDET_CK_PDN_MASK),
                             (U32)(PMIC_RG_ACCDET_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_CK_PDN_MASK),
                             (U32)(PMIC_RG_AUXADC_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smps_ck_div_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_SMPS_CK_DIV_PDN_MASK),
                             (U32)(PMIC_RG_SMPS_CK_DIV_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smps_ck_div2_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_SMPS_CK_DIV2_PDN_MASK),
                             (U32)(PMIC_RG_SMPS_CK_DIV2_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_div_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_DIV_PDN_MASK),
                             (U32)(PMIC_RG_SPK_DIV_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_pwm_div_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_PWM_DIV_PDN_MASK),
                             (U32)(PMIC_RG_SPK_PWM_DIV_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rtc_mclk_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_RTC_MCLK_PDN_MASK),
                             (U32)(PMIC_RG_RTC_MCLK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_bst_drv_1m_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_BST_DRV_1M_CK_PDN_MASK),
                             (U32)(PMIC_RG_BST_DRV_1M_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_fgadc_ana_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_FGADC_ANA_CK_PDN_MASK),
                             (U32)(PMIC_RG_FGADC_ANA_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_fgadc_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_FGADC_CK_PDN_MASK),
                             (U32)(PMIC_RG_FGADC_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_CK_PDN_MASK),
                             (U32)(PMIC_RG_EFUSE_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pwmoc_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_PWMOC_CK_PDN_MASK),
                             (U32)(PMIC_RG_PWMOC_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_CK_PDN_MASK),
                             (U32)(PMIC_RG_SPK_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_aud_13m_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_AUD_13M_PDN_MASK),
                             (U32)(PMIC_RG_AUD_13M_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_aud_26m_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_AUD_26M_PDN_MASK),
                             (U32)(PMIC_RG_AUD_26M_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_top_ckpdn2_rsv_15(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_TOP_CKPDN2_RSV_15_MASK),
                             (U32)(PMIC_RG_TOP_CKPDN2_RSV_15_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rtc_75k_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_RTC_75K_CK_PDN_MASK),
                             (U32)(PMIC_RG_RTC_75K_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_strup_32k_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_STRUP_32K_CK_PDN_MASK),
                             (U32)(PMIC_RG_STRUP_32K_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_buck_1m_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_BUCK_1M_CK_PDN_MASK),
                             (U32)(PMIC_RG_BUCK_1M_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_buck32k_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_BUCK32K_PDN_MASK),
                             (U32)(PMIC_RG_BUCK32K_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_buck_ana_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_BUCK_ANA_CK_PDN_MASK),
                             (U32)(PMIC_RG_BUCK_ANA_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_buck_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_BUCK_CK_PDN_MASK),
                             (U32)(PMIC_RG_BUCK_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_chr1m_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_CHR1M_CK_PDN_MASK),
                             (U32)(PMIC_RG_CHR1M_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_drv_32k_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_DRV_32K_CK_PDN_MASK),
                             (U32)(PMIC_RG_DRV_32K_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_intrp_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_INTRP_CK_PDN_MASK),
                             (U32)(PMIC_RG_INTRP_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_ldostb_1m_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_LDOSTB_1M_CK_PDN_MASK),
                             (U32)(PMIC_RG_LDOSTB_1M_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_32k_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_32K_CK_PDN_MASK),
                             (U32)(PMIC_RG_PCHR_32K_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rtc_32k_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_RTC_32K_CK_PDN_MASK),
                             (U32)(PMIC_RG_RTC_32K_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_strup_75k_ck_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_STRUP_75K_CK_PDN_MASK),
                             (U32)(PMIC_RG_STRUP_75K_CK_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_fqmtr_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_FQMTR_PDN_MASK),
                             (U32)(PMIC_RG_FQMTR_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rtc32k_1v8_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKPDN2),
                             (U32)(val),
                             (U32)(PMIC_RG_RTC32K_1V8_PDN_MASK),
                             (U32)(PMIC_RG_RTC32K_1V8_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_top_gpio_ckpdn_rsv_15_14(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_GPIO_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_TOP_GPIO_CKPDN_RSV_15_14_MASK),
                             (U32)(PMIC_RG_TOP_GPIO_CKPDN_RSV_15_14_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_gpio32k_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_GPIO_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_GPIO32K_PDN_MASK),
                             (U32)(PMIC_RG_GPIO32K_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_gpio26m_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_GPIO_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_GPIO26M_PDN_MASK),
                             (U32)(PMIC_RG_GPIO26M_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_top_rst_con_rsv_15_9(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_TOP_RST_CON_RSV_15_9_MASK),
                             (U32)(PMIC_RG_TOP_RST_CON_RSV_15_9_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_fqmtr_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_FQMTR_RST_MASK),
                             (U32)(PMIC_RG_FQMTR_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rtc_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_RTC_RST_MASK),
                             (U32)(PMIC_RG_RTC_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_driver_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_DRIVER_RST_MASK),
                             (U32)(PMIC_RG_DRIVER_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_RST_MASK),
                             (U32)(PMIC_RG_SPK_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_accdet_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_ACCDET_RST_MASK),
                             (U32)(PMIC_RG_ACCDET_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_fgadc_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_FGADC_RST_MASK),
                             (U32)(PMIC_RG_FGADC_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audio_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDIO_RST_MASK),
                             (U32)(PMIC_RG_AUDIO_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_RST_MASK),
                             (U32)(PMIC_RG_AUXADC_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_man_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_MAN_RST_MASK),
                             (U32)(PMIC_RG_EFUSE_MAN_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_wrp_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_WRP_PDN_MASK),
                             (U32)(PMIC_RG_WRP_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_wrp_32k_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_WRP_32K_PDN_MASK),
                             (U32)(PMIC_RG_WRP_32K_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_wrp_eint_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_WRP_EINT_PDN_MASK),
                             (U32)(PMIC_RG_WRP_EINT_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_wrp_kp_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_WRP_KP_PDN_MASK),
                             (U32)(PMIC_RG_WRP_KP_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_wrp_pwm_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_WRP_PWM_PDN_MASK),
                             (U32)(PMIC_RG_WRP_PWM_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_wrp_i2c2_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_WRP_I2C2_PDN_MASK),
                             (U32)(PMIC_RG_WRP_I2C2_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_wrp_i2c1_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_WRP_I2C1_PDN_MASK),
                             (U32)(PMIC_RG_WRP_I2C1_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_wrp_i2c0_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_CKPDN),
                             (U32)(val),
                             (U32)(PMIC_RG_WRP_I2C0_PDN_MASK),
                             (U32)(PMIC_RG_WRP_I2C0_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_wrp_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_WRP_RST_MASK),
                             (U32)(PMIC_RG_WRP_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_eint_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_EINT_RST_MASK),
                             (U32)(PMIC_RG_EINT_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_kp_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_KP_RST_MASK),
                             (U32)(PMIC_RG_KP_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pwm_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_PWM_RST_MASK),
                             (U32)(PMIC_RG_PWM_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_i2c2_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_I2C2_RST_MASK),
                             (U32)(PMIC_RG_I2C2_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_i2c1_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_I2C1_RST_MASK),
                             (U32)(PMIC_RG_I2C1_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_i2c0_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(WRP_RST_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_I2C0_RST_MASK),
                             (U32)(PMIC_RG_I2C0_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pwrkey_rst_td(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_MISC),
                             (U32)(val),
                             (U32)(PMIC_RG_PWRKEY_RST_TD_MASK),
                             (U32)(PMIC_RG_PWRKEY_RST_TD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pwrrst_tmr_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_MISC),
                             (U32)(val),
                             (U32)(PMIC_RG_PWRRST_TMR_DIS_MASK),
                             (U32)(PMIC_RG_PWRRST_TMR_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pwrkey_rst_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_MISC),
                             (U32)(val),
                             (U32)(PMIC_RG_PWRKEY_RST_EN_MASK),
                             (U32)(PMIC_RG_PWRKEY_RST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_homekey_rst_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_MISC),
                             (U32)(val),
                             (U32)(PMIC_RG_HOMEKEY_RST_EN_MASK),
                             (U32)(PMIC_RG_HOMEKEY_RST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rst_part_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_MISC),
                             (U32)(val),
                             (U32)(PMIC_RG_RST_PART_SEL_MASK),
                             (U32)(PMIC_RG_RST_PART_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_top_rst_misc_rsv_3(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_MISC),
                             (U32)(val),
                             (U32)(PMIC_RG_TOP_RST_MISC_RSV_3_MASK),
                             (U32)(PMIC_RG_TOP_RST_MISC_RSV_3_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_strup_man_rst_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_MISC),
                             (U32)(val),
                             (U32)(PMIC_RG_STRUP_MAN_RST_EN_MASK),
                             (U32)(PMIC_RG_STRUP_MAN_RST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_sysrstb_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_MISC),
                             (U32)(val),
                             (U32)(PMIC_RG_SYSRSTB_EN_MASK),
                             (U32)(PMIC_RG_SYSRSTB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_ap_rst_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_RST_MISC),
                             (U32)(val),
                             (U32)(PMIC_RG_AP_RST_DIS_MASK),
                             (U32)(PMIC_RG_AP_RST_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_osc_sel_align_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_OSC_SEL_ALIGN_DIS_MASK),
                             (U32)(PMIC_RG_OSC_SEL_ALIGN_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spitxck_inv_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_rg_spitxck_inv_sel_MASK),
                             (U32)(PMIC_rg_spitxck_inv_sel_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_osc_hw_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_OSC_HW_SEL_MASK),
                             (U32)(PMIC_RG_OSC_HW_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_clksq_hw_auto_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_CLKSQ_HW_AUTO_EN_MASK),
                             (U32)(PMIC_RG_CLKSQ_HW_AUTO_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_srclkperi_hw_auto_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SRCLKPERI_HW_AUTO_EN_MASK),
                             (U32)(PMIC_RG_SRCLKPERI_HW_AUTO_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_srclkmd2_hw_auto_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SRCLKMD2_HW_AUTO_EN_MASK),
                             (U32)(PMIC_RG_SRCLKMD2_HW_AUTO_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_srcvolt_hw_auto_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SRCVOLT_HW_AUTO_EN_MASK),
                             (U32)(PMIC_RG_SRCVOLT_HW_AUTO_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_osc_sel_auto(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_OSC_SEL_AUTO_MASK),
                             (U32)(PMIC_RG_OSC_SEL_AUTO_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_top_ckcon1_rsv_07(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TOP_CKCON1_RSV_07_MASK),
                             (U32)(PMIC_RG_TOP_CKCON1_RSV_07_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smps_div2_src_autoff_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMPS_DIV2_SRC_AUTOFF_DIS_MASK),
                             (U32)(PMIC_RG_SMPS_DIV2_SRC_AUTOFF_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smps_autoff_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMPS_AUTOFF_DIS_MASK),
                             (U32)(PMIC_RG_SMPS_AUTOFF_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_clksq_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_CLKSQ_EN_MASK),
                             (U32)(PMIC_RG_CLKSQ_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_srclkperi_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SRCLKPERI_EN_MASK),
                             (U32)(PMIC_RG_SRCLKPERI_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_srclkmd2_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SRCLKMD2_EN_MASK),
                             (U32)(PMIC_RG_SRCLKMD2_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_srcvolt_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SRCVOLT_EN_MASK),
                             (U32)(PMIC_RG_SRCVOLT_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_osc_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON1),
                             (U32)(val),
                             (U32)(PMIC_RG_OSC_SEL_MASK),
                             (U32)(PMIC_RG_OSC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_fqmtr_cksel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_RG_FQMTR_CKSEL_MASK),
                             (U32)(PMIC_RG_FQMTR_CKSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_accdet_cksel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_RG_ACCDET_CKSEL_MASK),
                             (U32)(PMIC_RG_ACCDET_CKSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_fg_ana_cksel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_RG_FG_ANA_CKSEL_MASK),
                             (U32)(PMIC_RG_FG_ANA_CKSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_regck_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_RG_REGCK_SEL_MASK),
                             (U32)(PMIC_RG_REGCK_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_buck_2m_sel_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_RG_BUCK_2M_SEL_EN_MASK),
                             (U32)(PMIC_RG_BUCK_2M_SEL_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_6m_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_VPROC_6M_SEL_MASK),
                             (U32)(PMIC_VPROC_6M_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_6m_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_VCORE_6M_SEL_MASK),
                             (U32)(PMIC_VCORE_6M_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_div_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_DIV_SEL_MASK),
                             (U32)(PMIC_RG_SPK_DIV_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_pwm_div_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_PWM_DIV_SEL_MASK),
                             (U32)(PMIC_RG_SPK_PWM_DIV_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_div_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKCON2),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_DIV_SEL_MASK),
                             (U32)(PMIC_RG_AUXADC_DIV_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_auxadc_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_AUXADC_TSTSEL_MASK),
                             (U32)(PMIC_AUXADC_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_pmu75k_tst_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_PMU75K_TST_DIS_MASK),
                             (U32)(PMIC_PMU75K_TST_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_smps_tst_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_SMPS_TST_DIS_MASK),
                             (U32)(PMIC_SMPS_TST_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_chr1m_tst_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_CHR1M_TST_DIS_MASK),
                             (U32)(PMIC_CHR1M_TST_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_aud26m_tst_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_AUD26M_TST_DIS_MASK),
                             (U32)(PMIC_AUD26M_TST_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rtc32k_tst_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_RTC32K_TST_DIS_MASK),
                             (U32)(PMIC_RTC32K_TST_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_tst_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_FG_TST_DIS_MASK),
                             (U32)(PMIC_FG_TST_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_tst_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_SPK_TST_DIS_MASK),
                             (U32)(PMIC_SPK_TST_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_chr1m_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_CHR1M_TSTSEL_MASK),
                             (U32)(PMIC_CHR1M_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_smps_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_SMPS_TSTSEL_MASK),
                             (U32)(PMIC_SMPS_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_pmu75k_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_PMU75K_TSTSEL_MASK),
                             (U32)(PMIC_PMU75K_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_aud26m_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_AUD26M_TSTSEL_MASK),
                             (U32)(PMIC_AUD26M_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rtc32k_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_RTC32K_TSTSEL_MASK),
                             (U32)(PMIC_RTC32K_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_FG_TSTSEL_MASK),
                             (U32)(PMIC_FG_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST1),
                             (U32)(val),
                             (U32)(PMIC_SPK_TSTSEL_MASK),
                             (U32)(PMIC_SPK_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_top_cktst2_rsv_15_8(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST2),
                             (U32)(val),
                             (U32)(PMIC_RG_TOP_CKTST2_RSV_15_8_MASK),
                             (U32)(PMIC_RG_TOP_CKTST2_RSV_15_8_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_osc32_cksel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST2),
                             (U32)(val),
                             (U32)(PMIC_OSC32_CKSEL_MASK),
                             (U32)(PMIC_OSC32_CKSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_xosc32_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST2),
                             (U32)(val),
                             (U32)(PMIC_XOSC32_TSTSEL_MASK),
                             (U32)(PMIC_XOSC32_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_xosc32_tst_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST2),
                             (U32)(val),
                             (U32)(PMIC_XOSC32_TST_DIS_MASK),
                             (U32)(PMIC_XOSC32_TST_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pchr_test_ck_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST2),
                             (U32)(val),
                             (U32)(PMIC_RG_PCHR_TEST_CK_SEL_MASK),
                             (U32)(PMIC_RG_PCHR_TEST_CK_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_strup_75k_26m_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST2),
                             (U32)(val),
                             (U32)(PMIC_RG_STRUP_75K_26M_SEL_MASK),
                             (U32)(PMIC_RG_STRUP_75K_26M_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST2),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_TSTSEL_MASK),
                             (U32)(PMIC_ACCDET_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_ck1m2m_tstsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST2),
                             (U32)(val),
                             (U32)(PMIC_CK1M2M_TSTSEL_MASK),
                             (U32)(PMIC_CK1M2M_TSTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_bgr_test_ck_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TOP_CKTST2),
                             (U32)(val),
                             (U32)(PMIC_BGR_TEST_CK_EN_MASK),
                             (U32)(PMIC_BGR_TEST_CK_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_deg_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_DEG_EN),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_DEG_EN_MASK),
                             (U32)(PMIC_VRF18_2_DEG_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_deg_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_DEG_EN),
                             (U32)(val),
                             (U32)(PMIC_VRF18_DEG_EN_MASK),
                             (U32)(PMIC_VRF18_DEG_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_deg_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_DEG_EN),
                             (U32)(val),
                             (U32)(PMIC_VPA_DEG_EN_MASK),
                             (U32)(PMIC_VPA_DEG_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_deg_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_DEG_EN),
                             (U32)(val),
                             (U32)(PMIC_VIO18_DEG_EN_MASK),
                             (U32)(PMIC_VIO18_DEG_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_deg_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_DEG_EN),
                             (U32)(val),
                             (U32)(PMIC_VM_DEG_EN_MASK),
                             (U32)(PMIC_VM_DEG_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_deg_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_DEG_EN),
                             (U32)(val),
                             (U32)(PMIC_VCORE_DEG_EN_MASK),
                             (U32)(PMIC_VCORE_DEG_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_deg_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_DEG_EN),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_DEG_EN_MASK),
                             (U32)(PMIC_VSRAM_DEG_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_deg_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_DEG_EN),
                             (U32)(val),
                             (U32)(PMIC_VPROC_DEG_EN_MASK),
                             (U32)(PMIC_VPROC_DEG_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_oc_gear_bvalid_det(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL0),
                             (U32)(val),
                             (U32)(PMIC_OC_GEAR_BVALID_DET_MASK),
                             (U32)(PMIC_OC_GEAR_BVALID_DET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_oc_gear_vbaton_undet(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL0),
                             (U32)(val),
                             (U32)(PMIC_OC_GEAR_VBATON_UNDET_MASK),
                             (U32)(PMIC_OC_GEAR_VBATON_UNDET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_oc_gear_ldo(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL0),
                             (U32)(val),
                             (U32)(PMIC_OC_GEAR_LDO_MASK),
                             (U32)(PMIC_OC_GEAR_LDO_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_oc_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL1),
                             (U32)(val),
                             (U32)(PMIC_VM_OC_WND_MASK),
                             (U32)(PMIC_VM_OC_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_oc_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL1),
                             (U32)(val),
                             (U32)(PMIC_VM_OC_THD_MASK),
                             (U32)(PMIC_VM_OC_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_oc_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL1),
                             (U32)(val),
                             (U32)(PMIC_VCORE_OC_WND_MASK),
                             (U32)(PMIC_VCORE_OC_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_oc_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL1),
                             (U32)(val),
                             (U32)(PMIC_VCORE_OC_THD_MASK),
                             (U32)(PMIC_VCORE_OC_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_oc_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL1),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_OC_WND_MASK),
                             (U32)(PMIC_VSRAM_OC_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_oc_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL1),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_OC_THD_MASK),
                             (U32)(PMIC_VSRAM_OC_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_oc_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL1),
                             (U32)(val),
                             (U32)(PMIC_VPROC_OC_WND_MASK),
                             (U32)(PMIC_VPROC_OC_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_oc_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL1),
                             (U32)(val),
                             (U32)(PMIC_VPROC_OC_THD_MASK),
                             (U32)(PMIC_VPROC_OC_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_oc_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL2),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_OC_WND_MASK),
                             (U32)(PMIC_VRF18_2_OC_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_oc_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL2),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_OC_THD_MASK),
                             (U32)(PMIC_VRF18_2_OC_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_oc_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL2),
                             (U32)(val),
                             (U32)(PMIC_VRF18_OC_WND_MASK),
                             (U32)(PMIC_VRF18_OC_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_oc_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL2),
                             (U32)(val),
                             (U32)(PMIC_VRF18_OC_THD_MASK),
                             (U32)(PMIC_VRF18_OC_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_oc_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL2),
                             (U32)(val),
                             (U32)(PMIC_VPA_OC_WND_MASK),
                             (U32)(PMIC_VPA_OC_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_oc_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL2),
                             (U32)(val),
                             (U32)(PMIC_VPA_OC_THD_MASK),
                             (U32)(PMIC_VPA_OC_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_oc_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL2),
                             (U32)(val),
                             (U32)(PMIC_VIO18_OC_WND_MASK),
                             (U32)(PMIC_VIO18_OC_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_oc_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(OC_CTL2),
                             (U32)(val),
                             (U32)(PMIC_VIO18_OC_THD_MASK),
                             (U32)(PMIC_VIO18_OC_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_int_rsv_15_8(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_RSV),
                             (U32)(val),
                             (U32)(PMIC_INT_RSV_15_8_MASK),
                             (U32)(PMIC_INT_RSV_15_8_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_ivgen_ext_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_RSV),
                             (U32)(val),
                             (U32)(PMIC_IVGEN_EXT_EN_MASK),
                             (U32)(PMIC_IVGEN_EXT_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pwrkey_int_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_RSV),
                             (U32)(val),
                             (U32)(PMIC_RG_PWRKEY_INT_SEL_MASK),
                             (U32)(PMIC_RG_PWRKEY_INT_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_homekey_int_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_RSV),
                             (U32)(val),
                             (U32)(PMIC_RG_HOMEKEY_INT_SEL_MASK),
                             (U32)(PMIC_RG_HOMEKEY_INT_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_polarity_bvalid_det(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_RSV),
                             (U32)(val),
                             (U32)(PMIC_POLARITY_BVALID_DET_MASK),
                             (U32)(PMIC_POLARITY_BVALID_DET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_polarity_vbaton_undet(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_RSV),
                             (U32)(val),
                             (U32)(PMIC_POLARITY_VBATON_UNDET_MASK),
                             (U32)(PMIC_POLARITY_VBATON_UNDET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_polarity(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_RSV),
                             (U32)(val),
                             (U32)(PMIC_POLARITY_MASK),
                             (U32)(PMIC_POLARITY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_mon_grp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_MON_GRP_SEL_MASK),
                             (U32)(PMIC_RG_MON_GRP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_mon_flag_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_MON_FLAG_SEL_MASK),
                             (U32)(PMIC_RG_MON_FLAG_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_spk_pwm(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_SPK_PWM_MASK),
                             (U32)(PMIC_RG_TEST_SPK_PWM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_spk(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_SPK_MASK),
                             (U32)(PMIC_RG_TEST_SPK_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_strup(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_STRUP_MASK),
                             (U32)(PMIC_RG_TEST_STRUP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_MODE_MASK),
                             (U32)(PMIC_RG_EFUSE_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_nandtree_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_NANDTREE_MODE_MASK),
                             (U32)(PMIC_RG_NANDTREE_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_auxadc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_AUXADC_MASK),
                             (U32)(PMIC_RG_TEST_AUXADC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_fgpll(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_FGPLL_MASK),
                             (U32)(PMIC_RG_TEST_FGPLL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_fg(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_FG_MASK),
                             (U32)(PMIC_RG_TEST_FG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_aud(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_AUD_MASK),
                             (U32)(PMIC_RG_TEST_AUD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_wrap(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_WRAP_MASK),
                             (U32)(PMIC_RG_TEST_WRAP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_io_fg_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_IO_FG_SEL_MASK),
                             (U32)(PMIC_RG_TEST_IO_FG_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_classd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_CLASSD_MASK),
                             (U32)(PMIC_RG_TEST_CLASSD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_driver(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_DRIVER_MASK),
                             (U32)(PMIC_RG_TEST_DRIVER_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_test_boost(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TEST_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_TEST_BOOST_MASK),
                             (U32)(PMIC_RG_TEST_BOOST_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_vrtc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_VRTC_STATUS_MASK),
                           (U32)(PMIC_VRTC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vrf28_2_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VRF28_2_EN_MASK),
                           (U32)(PMIC_STATUS_VRF28_2_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vrf28_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VRF28_EN_MASK),
                           (U32)(PMIC_STATUS_VRF28_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vsim1_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VSIM1_EN_MASK),
                           (U32)(PMIC_STATUS_VSIM1_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vsim2_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VSIM2_EN_MASK),
                           (U32)(PMIC_STATUS_VSIM2_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vtcxo_2_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VTCXO_2_EN_MASK),
                           (U32)(PMIC_STATUS_VTCXO_2_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vtcxo_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VTCXO_EN_MASK),
                           (U32)(PMIC_STATUS_VTCXO_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vusb_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VUSB_EN_MASK),
                           (U32)(PMIC_STATUS_VUSB_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vrf18_2_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VRF18_2_EN_MASK),
                           (U32)(PMIC_STATUS_VRF18_2_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vrf18_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VRF18_EN_MASK),
                           (U32)(PMIC_STATUS_VRF18_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vpa_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VPA_EN_MASK),
                           (U32)(PMIC_STATUS_VPA_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vio18_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VIO18_EN_MASK),
                           (U32)(PMIC_STATUS_VIO18_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vm_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VM_EN_MASK),
                           (U32)(PMIC_STATUS_VM_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vcore_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VCORE_EN_MASK),
                           (U32)(PMIC_STATUS_VCORE_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vsram_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VSRAM_EN_MASK),
                           (U32)(PMIC_STATUS_VSRAM_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vproc_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS0),
                           (&val),
                           (U32)(PMIC_STATUS_VPROC_EN_MASK),
                           (U32)(PMIC_STATUS_VPROC_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_va28_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VA28_EN_MASK),
                           (U32)(PMIC_STATUS_VA28_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vast_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VAST_EN_MASK),
                           (U32)(PMIC_STATUS_VAST_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_va_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VA_EN_MASK),
                           (U32)(PMIC_STATUS_VA_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vcama_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VCAMA_EN_MASK),
                           (U32)(PMIC_STATUS_VCAMA_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vemc_1v8_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VEMC_1V8_EN_MASK),
                           (U32)(PMIC_STATUS_VEMC_1V8_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vemc_3v3_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VEMC_3V3_EN_MASK),
                           (U32)(PMIC_STATUS_VEMC_3V3_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vgp1_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VGP1_EN_MASK),
                           (U32)(PMIC_STATUS_VGP1_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vgp2_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VGP2_EN_MASK),
                           (U32)(PMIC_STATUS_VGP2_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vgp3_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VGP3_EN_MASK),
                           (U32)(PMIC_STATUS_VGP3_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vgp4_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VGP4_EN_MASK),
                           (U32)(PMIC_STATUS_VGP4_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vgp5_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VGP5_EN_MASK),
                           (U32)(PMIC_STATUS_VGP5_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vgp6_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VGP6_EN_MASK),
                           (U32)(PMIC_STATUS_VGP6_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vibr_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VIBR_EN_MASK),
                           (U32)(PMIC_STATUS_VIBR_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vio28_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VIO28_EN_MASK),
                           (U32)(PMIC_STATUS_VIO28_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vmc1_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VMC1_EN_MASK),
                           (U32)(PMIC_STATUS_VMC1_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_status_vmch1_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STATUS1),
                           (&val),
                           (U32)(PMIC_STATUS_VMCH1_EN_MASK),
                           (U32)(PMIC_STATUS_VMCH1_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_vio28_pg_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(PGSTATUS),
                           (&val),
                           (U32)(PMIC_VIO28_PG_DEB_MASK),
                           (U32)(PMIC_VIO28_PG_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_va28_pg_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(PGSTATUS),
                           (&val),
                           (U32)(PMIC_VA28_PG_DEB_MASK),
                           (U32)(PMIC_VA28_PG_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_va_pg_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(PGSTATUS),
                           (&val),
                           (U32)(PMIC_VA_PG_DEB_MASK),
                           (U32)(PMIC_VA_PG_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_vtcxo_pg_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(PGSTATUS),
                           (&val),
                           (U32)(PMIC_VTCXO_PG_DEB_MASK),
                           (U32)(PMIC_VTCXO_PG_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_vio18_pg_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(PGSTATUS),
                           (&val),
                           (U32)(PMIC_VIO18_PG_DEB_MASK),
                           (U32)(PMIC_VIO18_PG_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_vm_pg_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(PGSTATUS),
                           (&val),
                           (U32)(PMIC_VM_PG_DEB_MASK),
                           (U32)(PMIC_VM_PG_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_vcore_pg_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(PGSTATUS),
                           (&val),
                           (U32)(PMIC_VCORE_PG_DEB_MASK),
                           (U32)(PMIC_VCORE_PG_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_vsram_pg_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(PGSTATUS),
                           (&val),
                           (U32)(PMIC_VSRAM_PG_DEB_MASK),
                           (U32)(PMIC_VSRAM_PG_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_vproc_pg_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(PGSTATUS),
                           (&val),
                           (U32)(PMIC_VPROC_PG_DEB_MASK),
                           (U32)(PMIC_VPROC_PG_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rtc_xtal_det_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(CHRSTATUS),
                             (U32)(val),
                             (U32)(PMIC_rtc_xtal_det_rsv_MASK),
                             (U32)(PMIC_rtc_xtal_det_rsv_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rtc_xtal_det_done(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHRSTATUS),
                           (&val),
                           (U32)(PMIC_rtc_xtal_det_done_MASK),
                           (U32)(PMIC_rtc_xtal_det_done_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ro_baton_undet(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHRSTATUS),
                           (&val),
                           (U32)(PMIC_RO_BATON_UNDET_MASK),
                           (U32)(PMIC_RO_BATON_UNDET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_pchr_chrdet(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHRSTATUS),
                           (&val),
                           (U32)(PMIC_PCHR_CHRDET_MASK),
                           (U32)(PMIC_PCHR_CHRDET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_vbat_ov(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHRSTATUS),
                           (&val),
                           (U32)(PMIC_VBAT_OV_MASK),
                           (U32)(PMIC_VBAT_OV_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_pwrkey_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHRSTATUS),
                           (&val),
                           (U32)(PMIC_PWRKEY_DEB_MASK),
                           (U32)(PMIC_PWRKEY_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_usbdl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHRSTATUS),
                           (&val),
                           (U32)(PMIC_USBDL_MASK),
                           (U32)(PMIC_USBDL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_pmu_test_mode_scan(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(CHRSTATUS),
                           (&val),
                           (U32)(PMIC_PMU_TEST_MODE_SCAN_MASK),
                           (U32)(PMIC_PMU_TEST_MODE_SCAN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vrf28_2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VRF28_2_MASK),
                           (U32)(PMIC_OC_STATUS_VRF28_2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vrf28(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VRF28_MASK),
                           (U32)(PMIC_OC_STATUS_VRF28_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vsim1(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VSIM1_MASK),
                           (U32)(PMIC_OC_STATUS_VSIM1_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vsim2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VSIM2_MASK),
                           (U32)(PMIC_OC_STATUS_VSIM2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vtcxo_2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VTCXO_2_MASK),
                           (U32)(PMIC_OC_STATUS_VTCXO_2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vtcxo(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VTCXO_MASK),
                           (U32)(PMIC_OC_STATUS_VTCXO_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vusb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VUSB_MASK),
                           (U32)(PMIC_OC_STATUS_VUSB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vrf18_2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VRF18_2_MASK),
                           (U32)(PMIC_OC_STATUS_VRF18_2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vrf18(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VRF18_MASK),
                           (U32)(PMIC_OC_STATUS_VRF18_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vpa(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VPA_MASK),
                           (U32)(PMIC_OC_STATUS_VPA_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vio18(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VIO18_MASK),
                           (U32)(PMIC_OC_STATUS_VIO18_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vm(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VM_MASK),
                           (U32)(PMIC_OC_STATUS_VM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vcore(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VCORE_MASK),
                           (U32)(PMIC_OC_STATUS_VCORE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vsram(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VSRAM_MASK),
                           (U32)(PMIC_OC_STATUS_VSRAM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vproc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS0),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VPROC_MASK),
                           (U32)(PMIC_OC_STATUS_VPROC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_va28(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VA28_MASK),
                           (U32)(PMIC_OC_STATUS_VA28_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vast(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VAST_MASK),
                           (U32)(PMIC_OC_STATUS_VAST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_va(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VA_MASK),
                           (U32)(PMIC_OC_STATUS_VA_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vcama(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VCAMA_MASK),
                           (U32)(PMIC_OC_STATUS_VCAMA_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vemc_1v8(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VEMC_1V8_MASK),
                           (U32)(PMIC_OC_STATUS_VEMC_1V8_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vemc_3v3(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VEMC_3V3_MASK),
                           (U32)(PMIC_OC_STATUS_VEMC_3V3_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vgp1(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VGP1_MASK),
                           (U32)(PMIC_OC_STATUS_VGP1_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vgp2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VGP2_MASK),
                           (U32)(PMIC_OC_STATUS_VGP2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vgp3(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VGP3_MASK),
                           (U32)(PMIC_OC_STATUS_VGP3_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vgp4(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VGP4_MASK),
                           (U32)(PMIC_OC_STATUS_VGP4_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vgp5(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VGP5_MASK),
                           (U32)(PMIC_OC_STATUS_VGP5_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vgp6(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VGP6_MASK),
                           (U32)(PMIC_OC_STATUS_VGP6_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vibr(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VIBR_MASK),
                           (U32)(PMIC_OC_STATUS_VIBR_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vio28(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VIO28_MASK),
                           (U32)(PMIC_OC_STATUS_VIO28_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vmc1(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VMC1_MASK),
                           (U32)(PMIC_OC_STATUS_VMC1_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_oc_status_vmch1(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS1),
                           (&val),
                           (U32)(PMIC_OC_STATUS_VMCH1_MASK),
                           (U32)(PMIC_OC_STATUS_VMCH1_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_homekey_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS2),
                           (&val),
                           (U32)(PMIC_HOMEKEY_DEB_MASK),
                           (U32)(PMIC_HOMEKEY_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_spk_oc_det_d_r(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS2),
                           (&val),
                           (U32)(PMIC_NI_SPK_OC_DET_D_R_MASK),
                           (U32)(PMIC_NI_SPK_OC_DET_D_R_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_spk_oc_det_d_l(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS2),
                           (&val),
                           (U32)(PMIC_NI_SPK_OC_DET_D_L_MASK),
                           (U32)(PMIC_NI_SPK_OC_DET_D_L_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_spk_oc_det_ab_r(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS2),
                           (&val),
                           (U32)(PMIC_NI_SPK_OC_DET_AB_R_MASK),
                           (U32)(PMIC_NI_SPK_OC_DET_AB_R_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_spk_oc_det_ab_l(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(OCSTATUS2),
                           (&val),
                           (U32)(PMIC_NI_SPK_OC_DET_AB_L_MASK),
                           (U32)(PMIC_NI_SPK_OC_DET_AB_L_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_simls2_srst_conf(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SIMLS_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SIMLS2_SRST_CONF_MASK),
                             (U32)(PMIC_RG_SIMLS2_SRST_CONF_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_simls2_sclk_conf(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SIMLS_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SIMLS2_SCLK_CONF_MASK),
                             (U32)(PMIC_RG_SIMLS2_SCLK_CONF_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_simls1_srst_conf(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SIMLS_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SIMLS1_SRST_CONF_MASK),
                             (U32)(PMIC_RG_SIMLS1_SRST_CONF_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_simls1_sclk_conf(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SIMLS_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SIMLS1_SCLK_CONF_MASK),
                             (U32)(PMIC_RG_SIMLS1_SCLK_CONF_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_test_out_l(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(TEST_OUT_L),
                           (&val),
                           (U32)(PMIC_TEST_OUT_L_MASK),
                           (U32)(PMIC_TEST_OUT_L_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_test_out_h(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(TEST_OUT_H),
                           (&val),
                           (U32)(PMIC_TEST_OUT_H_MASK),
                           (U32)(PMIC_TEST_OUT_H_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_simls_tdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SIMLS_TDSEL_MASK),
                             (U32)(PMIC_RG_SIMLS_TDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pmu_tdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_PMU_TDSEL_MASK),
                             (U32)(PMIC_RG_PMU_TDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spi_tdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SPI_TDSEL_MASK),
                             (U32)(PMIC_RG_SPI_TDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_i2s_tdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_I2S_TDSEL_MASK),
                             (U32)(PMIC_RG_I2S_TDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_kp_tdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_KP_TDSEL_MASK),
                             (U32)(PMIC_RG_KP_TDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pwm_tdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_PWM_TDSEL_MASK),
                             (U32)(PMIC_RG_PWM_TDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_i2c_tdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_I2C_TDSEL_MASK),
                             (U32)(PMIC_RG_I2C_TDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_simap_tdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(TDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SIMAP_TDSEL_MASK),
                             (U32)(PMIC_RG_SIMAP_TDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_simls_rdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SIMLS_RDSEL_MASK),
                             (U32)(PMIC_RG_SIMLS_RDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pmu_rdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_PMU_RDSEL_MASK),
                             (U32)(PMIC_RG_PMU_RDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spi_rdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SPI_RDSEL_MASK),
                             (U32)(PMIC_RG_SPI_RDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_i2s_rdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_I2S_RDSEL_MASK),
                             (U32)(PMIC_RG_I2S_RDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_kp_rdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_KP_RDSEL_MASK),
                             (U32)(PMIC_RG_KP_RDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pwm_rdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_PWM_RDSEL_MASK),
                             (U32)(PMIC_RG_PWM_RDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_i2c_rdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_I2C_RDSEL_MASK),
                             (U32)(PMIC_RG_I2C_RDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_simap_rdsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RDSEL_CON),
                             (U32)(val),
                             (U32)(PMIC_RG_SIMAP_RDSEL_MASK),
                             (U32)(PMIC_RG_SIMAP_RDSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt15(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT15_MASK),
                             (U32)(PMIC_RG_SMT15_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt14(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT14_MASK),
                             (U32)(PMIC_RG_SMT14_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt13(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT13_MASK),
                             (U32)(PMIC_RG_SMT13_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT12_MASK),
                             (U32)(PMIC_RG_SMT12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt11(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT11_MASK),
                             (U32)(PMIC_RG_SMT11_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt10(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT10_MASK),
                             (U32)(PMIC_RG_SMT10_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt9(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT9_MASK),
                             (U32)(PMIC_RG_SMT9_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt8(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT8_MASK),
                             (U32)(PMIC_RG_SMT8_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt7(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT7_MASK),
                             (U32)(PMIC_RG_SMT7_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt6(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT6_MASK),
                             (U32)(PMIC_RG_SMT6_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt5(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT5_MASK),
                             (U32)(PMIC_RG_SMT5_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt4(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT4_MASK),
                             (U32)(PMIC_RG_SMT4_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt3(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT3_MASK),
                             (U32)(PMIC_RG_SMT3_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT2_MASK),
                             (U32)(PMIC_RG_SMT2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT1_MASK),
                             (U32)(PMIC_RG_SMT1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT0_MASK),
                             (U32)(PMIC_RG_SMT0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt31(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT31_MASK),
                             (U32)(PMIC_RG_SMT31_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt30(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT30_MASK),
                             (U32)(PMIC_RG_SMT30_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt29(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT29_MASK),
                             (U32)(PMIC_RG_SMT29_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT28_MASK),
                             (U32)(PMIC_RG_SMT28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt27(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT27_MASK),
                             (U32)(PMIC_RG_SMT27_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt26(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT26_MASK),
                             (U32)(PMIC_RG_SMT26_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt25(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT25_MASK),
                             (U32)(PMIC_RG_SMT25_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt24(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT24_MASK),
                             (U32)(PMIC_RG_SMT24_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt23(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT23_MASK),
                             (U32)(PMIC_RG_SMT23_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt22(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT22_MASK),
                             (U32)(PMIC_RG_SMT22_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt21(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT21_MASK),
                             (U32)(PMIC_RG_SMT21_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt20(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT20_MASK),
                             (U32)(PMIC_RG_SMT20_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt19(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT19_MASK),
                             (U32)(PMIC_RG_SMT19_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt18(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT18_MASK),
                             (U32)(PMIC_RG_SMT18_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt17(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT17_MASK),
                             (U32)(PMIC_RG_SMT17_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt16(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT16_MASK),
                             (U32)(PMIC_RG_SMT16_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt47(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT47_MASK),
                             (U32)(PMIC_RG_SMT47_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt46(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT46_MASK),
                             (U32)(PMIC_RG_SMT46_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt45(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT45_MASK),
                             (U32)(PMIC_RG_SMT45_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt44(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT44_MASK),
                             (U32)(PMIC_RG_SMT44_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt43(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT43_MASK),
                             (U32)(PMIC_RG_SMT43_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt42(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT42_MASK),
                             (U32)(PMIC_RG_SMT42_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt41(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT41_MASK),
                             (U32)(PMIC_RG_SMT41_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt40(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT40_MASK),
                             (U32)(PMIC_RG_SMT40_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt39(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT39_MASK),
                             (U32)(PMIC_RG_SMT39_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt38(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT38_MASK),
                             (U32)(PMIC_RG_SMT38_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt37(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT37_MASK),
                             (U32)(PMIC_RG_SMT37_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt36(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT36_MASK),
                             (U32)(PMIC_RG_SMT36_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt35(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT35_MASK),
                             (U32)(PMIC_RG_SMT35_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt34(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT34_MASK),
                             (U32)(PMIC_RG_SMT34_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT33_MASK),
                             (U32)(PMIC_RG_SMT33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt32(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT32_MASK),
                             (U32)(PMIC_RG_SMT32_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_homekey_pden(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_HOMEKEY_PDEN_MASK),
                             (U32)(PMIC_RG_HOMEKEY_PDEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_homekey_puen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_HOMEKEY_PUEN_MASK),
                             (U32)(PMIC_RG_HOMEKEY_PUEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt50(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT50_MASK),
                             (U32)(PMIC_RG_SMT50_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt49(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT49_MASK),
                             (U32)(PMIC_RG_SMT49_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smt48(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(GPIO_SMT_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_SMT48_MASK),
                             (U32)(PMIC_RG_SMT48_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_srclken_peri(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SRCLKEN_PERI_MASK),
                             (U32)(PMIC_RG_OCTL_SRCLKEN_PERI_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_srcvolten(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SRCVOLTEN_MASK),
                             (U32)(PMIC_RG_OCTL_SRCVOLTEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_int(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_INT_MASK),
                             (U32)(PMIC_RG_OCTL_INT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_homekey(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_HOMEKEY_MASK),
                             (U32)(PMIC_RG_OCTL_HOMEKEY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_spi_clk(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SPI_CLK_MASK),
                             (U32)(PMIC_RG_OCTL_SPI_CLK_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_wrap_event(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_WRAP_EVENT_MASK),
                             (U32)(PMIC_RG_OCTL_WRAP_EVENT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_rtc_32k1v8(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_RTC_32K1V8_MASK),
                             (U32)(PMIC_RG_OCTL_RTC_32K1V8_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_srclken_md2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SRCLKEN_MD2_MASK),
                             (U32)(PMIC_RG_OCTL_SRCLKEN_MD2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_adc_ck(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ADC_CK_MASK),
                             (U32)(PMIC_RG_OCTL_ADC_CK_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_spi_miso(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SPI_MISO_MASK),
                             (U32)(PMIC_RG_OCTL_SPI_MISO_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_spi_mosi(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SPI_MOSI_MASK),
                             (U32)(PMIC_RG_OCTL_SPI_MOSI_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_spi_csn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SPI_CSN_MASK),
                             (U32)(PMIC_RG_OCTL_SPI_CSN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_dac_ws(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_DAC_WS_MASK),
                             (U32)(PMIC_RG_OCTL_DAC_WS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_dac_ck(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_DAC_CK_MASK),
                             (U32)(PMIC_RG_OCTL_DAC_CK_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_adc_dat(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ADC_DAT_MASK),
                             (U32)(PMIC_RG_OCTL_ADC_DAT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_adc_ws(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ADC_WS_MASK),
                             (U32)(PMIC_RG_OCTL_ADC_WS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_col2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_COL2_MASK),
                             (U32)(PMIC_RG_OCTL_COL2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_col1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_COL1_MASK),
                             (U32)(PMIC_RG_OCTL_COL1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_col0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_COL0_MASK),
                             (U32)(PMIC_RG_OCTL_COL0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_dac_dat(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_DAC_DAT_MASK),
                             (U32)(PMIC_RG_OCTL_DAC_DAT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_col6(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_COL6_MASK),
                             (U32)(PMIC_RG_OCTL_COL6_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_col5(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_COL5_MASK),
                             (U32)(PMIC_RG_OCTL_COL5_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_col4(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_COL4_MASK),
                             (U32)(PMIC_RG_OCTL_COL4_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_col3(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_COL3_MASK),
                             (U32)(PMIC_RG_OCTL_COL3_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_row2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ROW2_MASK),
                             (U32)(PMIC_RG_OCTL_ROW2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_row1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ROW1_MASK),
                             (U32)(PMIC_RG_OCTL_ROW1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_row0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ROW0_MASK),
                             (U32)(PMIC_RG_OCTL_ROW0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_col7(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_COL7_MASK),
                             (U32)(PMIC_RG_OCTL_COL7_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_row6(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ROW6_MASK),
                             (U32)(PMIC_RG_OCTL_ROW6_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_row5(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ROW5_MASK),
                             (U32)(PMIC_RG_OCTL_ROW5_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_row4(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ROW4_MASK),
                             (U32)(PMIC_RG_OCTL_ROW4_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_row3(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ROW3_MASK),
                             (U32)(PMIC_RG_OCTL_ROW3_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_pwm(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_PWM_MASK),
                             (U32)(PMIC_RG_OCTL_PWM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_vmsel2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_VMSEL2_MASK),
                             (U32)(PMIC_RG_OCTL_VMSEL2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_vmsel1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_VMSEL1_MASK),
                             (U32)(PMIC_RG_OCTL_VMSEL1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_row7(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_ROW7_MASK),
                             (U32)(PMIC_RG_OCTL_ROW7_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_sda1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SDA1_MASK),
                             (U32)(PMIC_RG_OCTL_SDA1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_scl1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SCL1_MASK),
                             (U32)(PMIC_RG_OCTL_SCL1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_sda0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SDA0_MASK),
                             (U32)(PMIC_RG_OCTL_SDA0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_scl0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SCL0_MASK),
                             (U32)(PMIC_RG_OCTL_SCL0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_sim1_ap_srst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SIM1_AP_SRST_MASK),
                             (U32)(PMIC_RG_OCTL_SIM1_AP_SRST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_sim1_ap_sclk(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SIM1_AP_SCLK_MASK),
                             (U32)(PMIC_RG_OCTL_SIM1_AP_SCLK_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_sda2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SDA2_MASK),
                             (U32)(PMIC_RG_OCTL_SDA2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_scl2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SCL2_MASK),
                             (U32)(PMIC_RG_OCTL_SCL2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_simls1_srst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SIMLS1_SRST_MASK),
                             (U32)(PMIC_RG_OCTL_SIMLS1_SRST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_simls1_sclk(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SIMLS1_SCLK_MASK),
                             (U32)(PMIC_RG_OCTL_SIMLS1_SCLK_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_sim2_ap_srst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SIM2_AP_SRST_MASK),
                             (U32)(PMIC_RG_OCTL_SIM2_AP_SRST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_sim2_ap_sclk(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SIM2_AP_SCLK_MASK),
                             (U32)(PMIC_RG_OCTL_SIM2_AP_SCLK_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_simls2_srst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SIMLS2_SRST_MASK),
                             (U32)(PMIC_RG_OCTL_SIMLS2_SRST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_octl_simls2_sclk(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DRV_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_OCTL_SIMLS2_SCLK_MASK),
                             (U32)(PMIC_RG_OCTL_SIMLS2_SCLK_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_ov(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_OV_MASK),
                             (U32)(PMIC_RG_INT_EN_OV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_chrdet(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_CHRDET_MASK),
                             (U32)(PMIC_RG_INT_EN_CHRDET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_bvalid_det(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_BVALID_DET_MASK),
                             (U32)(PMIC_RG_INT_EN_BVALID_DET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_vbaton_undet(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_VBATON_UNDET_MASK),
                             (U32)(PMIC_RG_INT_EN_VBATON_UNDET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_thr_h(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_THR_H_MASK),
                             (U32)(PMIC_RG_INT_EN_THR_H_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_thr_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_THR_L_MASK),
                             (U32)(PMIC_RG_INT_EN_THR_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_pwrkey(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_PWRKEY_MASK),
                             (U32)(PMIC_RG_INT_EN_PWRKEY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_watchdog(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_WATCHDOG_MASK),
                             (U32)(PMIC_RG_INT_EN_WATCHDOG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_fg_bat_h(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_FG_BAT_H_MASK),
                             (U32)(PMIC_RG_INT_EN_FG_BAT_H_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_fg_bat_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_FG_BAT_L_MASK),
                             (U32)(PMIC_RG_INT_EN_FG_BAT_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_bat_h(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_BAT_H_MASK),
                             (U32)(PMIC_RG_INT_EN_BAT_H_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_bat_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_BAT_L_MASK),
                             (U32)(PMIC_RG_INT_EN_BAT_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_spkr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_SPKR_MASK),
                             (U32)(PMIC_RG_INT_EN_SPKR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_spkl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_SPKL_MASK),
                             (U32)(PMIC_RG_INT_EN_SPKL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_spkr_ab(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_SPKR_AB_MASK),
                             (U32)(PMIC_RG_INT_EN_SPKR_AB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_spkl_ab(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_SPKL_AB_MASK),
                             (U32)(PMIC_RG_INT_EN_SPKL_AB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_vrf18_2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_VRF18_2_MASK),
                             (U32)(PMIC_RG_INT_EN_VRF18_2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_vrf18(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_VRF18_MASK),
                             (U32)(PMIC_RG_INT_EN_VRF18_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_vpa(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_VPA_MASK),
                             (U32)(PMIC_RG_INT_EN_VPA_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_vio18(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_VIO18_MASK),
                             (U32)(PMIC_RG_INT_EN_VIO18_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_vm(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_VM_MASK),
                             (U32)(PMIC_RG_INT_EN_VM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_vcore(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_VCORE_MASK),
                             (U32)(PMIC_RG_INT_EN_VCORE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_vsram(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_VSRAM_MASK),
                             (U32)(PMIC_RG_INT_EN_VSRAM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_vproc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_VPROC_MASK),
                             (U32)(PMIC_RG_INT_EN_VPROC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_rtc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_RTC_MASK),
                             (U32)(PMIC_RG_INT_EN_RTC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_audio(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_AUDIO_MASK),
                             (U32)(PMIC_RG_INT_EN_AUDIO_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_accdet(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_ACCDET_MASK),
                             (U32)(PMIC_RG_INT_EN_ACCDET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_homekey(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_HOMEKEY_MASK),
                             (U32)(PMIC_RG_INT_EN_HOMEKEY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_int_en_ldo(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(INT_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_INT_EN_LDO_MASK),
                             (U32)(PMIC_RG_INT_EN_LDO_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rg_int_status_ov(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_OV_MASK),
                           (U32)(PMIC_RG_INT_STATUS_OV_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_chrdet(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_CHRDET_MASK),
                           (U32)(PMIC_RG_INT_STATUS_CHRDET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_bvalid_det(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_BVALID_DET_MASK),
                           (U32)(PMIC_RG_INT_STATUS_BVALID_DET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_vbaton_undet(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_VBATON_UNDET_MASK),
                           (U32)(PMIC_RG_INT_STATUS_VBATON_UNDET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_thr_h(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_THR_H_MASK),
                           (U32)(PMIC_RG_INT_STATUS_THR_H_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_thr_l(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_THR_L_MASK),
                           (U32)(PMIC_RG_INT_STATUS_THR_L_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_pwrkey(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_PWRKEY_MASK),
                           (U32)(PMIC_RG_INT_STATUS_PWRKEY_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_watchdog(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_WATCHDOG_MASK),
                           (U32)(PMIC_RG_INT_STATUS_WATCHDOG_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_fg_bat_h(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_FG_BAT_H_MASK),
                           (U32)(PMIC_RG_INT_STATUS_FG_BAT_H_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_fg_bat_l(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_FG_BAT_L_MASK),
                           (U32)(PMIC_RG_INT_STATUS_FG_BAT_L_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_bat_h(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_BAT_H_MASK),
                           (U32)(PMIC_RG_INT_STATUS_BAT_H_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_bat_l(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_BAT_L_MASK),
                           (U32)(PMIC_RG_INT_STATUS_BAT_L_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_spkr(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_SPKR_MASK),
                           (U32)(PMIC_RG_INT_STATUS_SPKR_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_spkl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_SPKL_MASK),
                           (U32)(PMIC_RG_INT_STATUS_SPKL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_spkr_ab(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_SPKR_AB_MASK),
                           (U32)(PMIC_RG_INT_STATUS_SPKR_AB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_spkl_ab(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS0),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_SPKL_AB_MASK),
                           (U32)(PMIC_RG_INT_STATUS_SPKL_AB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_vrf18_2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_VRF18_2_MASK),
                           (U32)(PMIC_RG_INT_STATUS_VRF18_2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_vrf18(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_VRF18_MASK),
                           (U32)(PMIC_RG_INT_STATUS_VRF18_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_vpa(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_VPA_MASK),
                           (U32)(PMIC_RG_INT_STATUS_VPA_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_vio18(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_VIO18_MASK),
                           (U32)(PMIC_RG_INT_STATUS_VIO18_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_vm(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_VM_MASK),
                           (U32)(PMIC_RG_INT_STATUS_VM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_vcore(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_VCORE_MASK),
                           (U32)(PMIC_RG_INT_STATUS_VCORE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_vsram(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_VSRAM_MASK),
                           (U32)(PMIC_RG_INT_STATUS_VSRAM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_vproc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_VPROC_MASK),
                           (U32)(PMIC_RG_INT_STATUS_VPROC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_rtc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_RTC_MASK),
                           (U32)(PMIC_RG_INT_STATUS_RTC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_audio(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_AUDIO_MASK),
                           (U32)(PMIC_RG_INT_STATUS_AUDIO_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_accdet(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_ACCDET_MASK),
                           (U32)(PMIC_RG_INT_STATUS_ACCDET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_homekey(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_HOMEKEY_MASK),
                           (U32)(PMIC_RG_INT_STATUS_HOMEKEY_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_int_status_ldo(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(INT_STATUS1),
                           (&val),
                           (U32)(PMIC_RG_INT_STATUS_LDO_MASK),
                           (U32)(PMIC_RG_INT_STATUS_LDO_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_fqmtr_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FQMTR_CON0),
                             (U32)(val),
                             (U32)(PMIC_FQMTR_EN_MASK),
                             (U32)(PMIC_FQMTR_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fqmtr_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FQMTR_CON0),
                             (U32)(val),
                             (U32)(PMIC_FQMTR_RST_MASK),
                             (U32)(PMIC_FQMTR_RST_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_fqmtr_busy(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FQMTR_CON0),
                           (&val),
                           (U32)(PMIC_FQMTR_BUSY_MASK),
                           (U32)(PMIC_FQMTR_BUSY_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_fqmtr_tcksel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FQMTR_CON0),
                             (U32)(val),
                             (U32)(PMIC_FQMTR_TCKSEL_MASK),
                             (U32)(PMIC_FQMTR_TCKSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fqmtr_winset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FQMTR_CON1),
                             (U32)(val),
                             (U32)(PMIC_FQMTR_WINSET_MASK),
                             (U32)(PMIC_FQMTR_WINSET_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_fqmtr_data(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FQMTR_CON2),
                           (&val),
                           (U32)(PMIC_FQMTR_DATA_MASK),
                           (U32)(PMIC_FQMTR_DATA_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_efuse_addr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_ADDR_MASK),
                             (U32)(PMIC_RG_EFUSE_ADDR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_prog(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_PROG_MASK),
                             (U32)(PMIC_RG_EFUSE_PROG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_EN_MASK),
                             (U32)(PMIC_RG_EFUSE_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_pkey(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_PKEY_MASK),
                             (U32)(PMIC_RG_EFUSE_PKEY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_rd_trig(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_RD_TRIG_MASK),
                             (U32)(PMIC_RG_EFUSE_RD_TRIG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rd_rdy_bypass(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_RD_RDY_BYPASS_MASK),
                             (U32)(PMIC_RG_RD_RDY_BYPASS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_skip_efuse_out(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SKIP_EFUSE_OUT_MASK),
                             (U32)(PMIC_RG_SKIP_EFUSE_OUT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_prog_src(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_PROG_SRC_MASK),
                             (U32)(PMIC_RG_EFUSE_PROG_SRC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rg_efuse_busy(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_CON6),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_BUSY_MASK),
                           (U32)(PMIC_RG_EFUSE_BUSY_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_rd_ack(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_CON6),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_RD_ACK_MASK),
                           (U32)(PMIC_RG_EFUSE_RD_ACK_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_efuse_val_0_15(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_0_15),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_0_15_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_0_15_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_16_31(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_16_31),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_16_31_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_16_31_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_32_47(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_32_47),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_32_47_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_32_47_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_48_63(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_48_63),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_48_63_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_48_63_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_64_79(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_64_79),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_64_79_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_64_79_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_80_95(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_80_95),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_80_95_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_80_95_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_96_111(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_96_111),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_96_111_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_96_111_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_112_127(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_112_127),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_112_127_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_112_127_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_128_143(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_128_143),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_128_143_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_128_143_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_144_159(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_144_159),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_144_159_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_144_159_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_160_175(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_160_175),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_160_175_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_160_175_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_efuse_val_176_191(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(EFUSE_VAL_176_191),
                             (U32)(val),
                             (U32)(PMIC_RG_EFUSE_VAL_176_191_MASK),
                             (U32)(PMIC_RG_EFUSE_VAL_176_191_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rg_efuse_dout_0_15(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_0_15),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_0_15_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_0_15_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_16_31(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_16_31),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_16_31_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_16_31_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_32_47(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_32_47),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_32_47_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_32_47_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_48_63(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_48_63),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_48_63_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_48_63_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_64_79(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_64_79),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_64_79_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_64_79_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_80_95(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_80_95),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_80_95_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_80_95_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_96_111(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_96_111),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_96_111_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_96_111_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_112_127(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_112_127),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_112_127_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_112_127_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_128_143(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_128_143),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_128_143_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_128_143_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_144_159(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_144_159),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_144_159_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_144_159_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_160_175(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_160_175),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_160_175_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_160_175_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_efuse_dout_176_191(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(EFUSE_DOUT_176_191),
                           (&val),
                           (U32)(PMIC_RG_EFUSE_DOUT_176_191_MASK),
                           (U32)(PMIC_RG_EFUSE_DOUT_176_191_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_spi_con(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPI_CON),
                             (U32)(val),
                             (U32)(PMIC_SPI_CON_MASK),
                             (U32)(PMIC_SPI_CON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smps_testmode_b(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMPS_TESTMODE_B_MASK),
                             (U32)(PMIC_RG_SMPS_TESTMODE_B_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_smps_testmode_a(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SMPS_TESTMODE_A_MASK),
                             (U32)(PMIC_RG_SMPS_TESTMODE_A_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vm_dig_mon(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(BUCK_CON1),
                           (&val),
                           (U32)(PMIC_QI_VM_DIG_MON_MASK),
                           (U32)(PMIC_QI_VM_DIG_MON_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vcore_dig_mon(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(BUCK_CON1),
                           (&val),
                           (U32)(PMIC_QI_VCORE_DIG_MON_MASK),
                           (U32)(PMIC_QI_VCORE_DIG_MON_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vsram_dig_mon(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(BUCK_CON1),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_DIG_MON_MASK),
                           (U32)(PMIC_QI_VSRAM_DIG_MON_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vproc_dig_mon(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(BUCK_CON1),
                           (&val),
                           (U32)(PMIC_QI_VPROC_DIG_MON_MASK),
                           (U32)(PMIC_QI_VPROC_DIG_MON_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_buck_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_CON2),
                             (U32)(val),
                             (U32)(PMIC_BUCK_RSV_MASK),
                             (U32)(PMIC_BUCK_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_pg_h2l_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_CON2),
                             (U32)(val),
                             (U32)(PMIC_VCORE_PG_H2L_EN_MASK),
                             (U32)(PMIC_VCORE_PG_H2L_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_pg_h2l_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_CON2),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_PG_H2L_EN_MASK),
                             (U32)(PMIC_VSRAM_PG_H2L_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_pg_h2l_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_CON2),
                             (U32)(val),
                             (U32)(PMIC_VPROC_PG_H2L_EN_MASK),
                             (U32)(PMIC_VPROC_PG_H2L_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_trimh(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_TRIMH_MASK),
                             (U32)(PMIC_RG_VPROC_TRIMH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_triml(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_TRIML_MASK),
                             (U32)(PMIC_RG_VPROC_TRIML_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_zx_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_ZX_OS_MASK),
                             (U32)(PMIC_RG_VPROC_ZX_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_csl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_CSL_MASK),
                             (U32)(PMIC_RG_VPROC_CSL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_csr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_CSR_MASK),
                             (U32)(PMIC_RG_VPROC_CSR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_cc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_CC_MASK),
                             (U32)(PMIC_RG_VPROC_CC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_rzsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_RZSEL_MASK),
                             (U32)(PMIC_RG_VPROC_RZSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VPROC_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_modeset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_MODESET_MASK),
                             (U32)(PMIC_RG_VPROC_MODESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_csm(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_CSM_MASK),
                             (U32)(PMIC_RG_VPROC_CSM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_avp_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_AVP_EN_MASK),
                             (U32)(PMIC_RG_VPROC_AVP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_avp_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_AVP_OS_MASK),
                             (U32)(PMIC_RG_VPROC_AVP_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_qi_vproc_vsleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON3),
                             (U32)(val),
                             (U32)(PMIC_QI_VPROC_VSLEEP_MASK),
                             (U32)(PMIC_QI_VPROC_VSLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_slp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_SLP_MASK),
                             (U32)(PMIC_RG_VPROC_SLP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vproc_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VPROC_RSV_MASK),
                             (U32)(PMIC_RG_VPROC_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_track_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON5),
                             (U32)(val),
                             (U32)(PMIC_VPROC_TRACK_ON_CTRL_MASK),
                             (U32)(PMIC_VPROC_TRACK_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_burst_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON5),
                             (U32)(val),
                             (U32)(PMIC_VPROC_BURST_CTRL_MASK),
                             (U32)(PMIC_VPROC_BURST_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_dlc_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON5),
                             (U32)(val),
                             (U32)(PMIC_VPROC_DLC_CTRL_MASK),
                             (U32)(PMIC_VPROC_DLC_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_vosel_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON5),
                             (U32)(val),
                             (U32)(PMIC_VPROC_VOSEL_CTRL_MASK),
                             (U32)(PMIC_VPROC_VOSEL_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_en_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON5),
                             (U32)(val),
                             (U32)(PMIC_VPROC_EN_CTRL_MASK),
                             (U32)(PMIC_VPROC_EN_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_burst_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON6),
                             (U32)(val),
                             (U32)(PMIC_VPROC_BURST_SEL_MASK),
                             (U32)(PMIC_VPROC_BURST_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_dlc_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON6),
                             (U32)(val),
                             (U32)(PMIC_VPROC_DLC_SEL_MASK),
                             (U32)(PMIC_VPROC_DLC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_vosel_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON6),
                             (U32)(val),
                             (U32)(PMIC_VPROC_VOSEL_SEL_MASK),
                             (U32)(PMIC_VPROC_VOSEL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON6),
                             (U32)(val),
                             (U32)(PMIC_VPROC_EN_SEL_MASK),
                             (U32)(PMIC_VPROC_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vproc_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON7),
                           (&val),
                           (U32)(PMIC_QI_VPROC_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VPROC_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vproc_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON7),
                           (&val),
                           (U32)(PMIC_QI_VPROC_MODE_MASK),
                           (U32)(PMIC_QI_VPROC_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vproc_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON7),
                           (&val),
                           (U32)(PMIC_QI_VPROC_EN_MASK),
                           (U32)(PMIC_QI_VPROC_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vproc_stb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON7),
                           (&val),
                           (U32)(PMIC_QI_VPROC_STB_MASK),
                           (U32)(PMIC_QI_VPROC_STB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vproc_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON7),
                             (U32)(val),
                             (U32)(PMIC_VPROC_EN_MASK),
                             (U32)(PMIC_VPROC_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_sfchg_ren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON8),
                             (U32)(val),
                             (U32)(PMIC_VPROC_SFCHG_REN_MASK),
                             (U32)(PMIC_VPROC_SFCHG_REN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_sfchg_rrate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON8),
                             (U32)(val),
                             (U32)(PMIC_VPROC_SFCHG_RRATE_MASK),
                             (U32)(PMIC_VPROC_SFCHG_RRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_sfchg_fen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON8),
                             (U32)(val),
                             (U32)(PMIC_VPROC_SFCHG_FEN_MASK),
                             (U32)(PMIC_VPROC_SFCHG_FEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_sfchg_frate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON8),
                             (U32)(val),
                             (U32)(PMIC_VPROC_SFCHG_FRATE_MASK),
                             (U32)(PMIC_VPROC_SFCHG_FRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON9),
                             (U32)(val),
                             (U32)(PMIC_VPROC_VOSEL_MASK),
                             (U32)(PMIC_VPROC_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_vosel_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON10),
                             (U32)(val),
                             (U32)(PMIC_VPROC_VOSEL_ON_MASK),
                             (U32)(PMIC_VPROC_VOSEL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_vosel_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON11),
                             (U32)(val),
                             (U32)(PMIC_VPROC_VOSEL_SLEEP_MASK),
                             (U32)(PMIC_VPROC_VOSEL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vproc_vosel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON12),
                           (&val),
                           (U32)(PMIC_NI_VPROC_VOSEL_MASK),
                           (U32)(PMIC_NI_VPROC_VOSEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vproc_burst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON13),
                           (&val),
                           (U32)(PMIC_QI_VPROC_BURST_MASK),
                           (U32)(PMIC_QI_VPROC_BURST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vproc_burst_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON13),
                             (U32)(val),
                             (U32)(PMIC_VPROC_BURST_SLEEP_MASK),
                             (U32)(PMIC_VPROC_BURST_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_burst_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON13),
                             (U32)(val),
                             (U32)(PMIC_VPROC_BURST_ON_MASK),
                             (U32)(PMIC_VPROC_BURST_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_burst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON13),
                             (U32)(val),
                             (U32)(PMIC_VPROC_BURST_MASK),
                             (U32)(PMIC_VPROC_BURST_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vproc_dlc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON14),
                           (&val),
                           (U32)(PMIC_QI_VPROC_DLC_MASK),
                           (U32)(PMIC_QI_VPROC_DLC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vproc_dlc_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON14),
                             (U32)(val),
                             (U32)(PMIC_VPROC_DLC_SLEEP_MASK),
                             (U32)(PMIC_VPROC_DLC_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_dlc_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON14),
                             (U32)(val),
                             (U32)(PMIC_VPROC_DLC_ON_MASK),
                             (U32)(PMIC_VPROC_DLC_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_dlc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON14),
                             (U32)(val),
                             (U32)(PMIC_VPROC_DLC_MASK),
                             (U32)(PMIC_VPROC_DLC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vproc_dlc_n(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON15),
                           (&val),
                           (U32)(PMIC_QI_VPROC_DLC_N_MASK),
                           (U32)(PMIC_QI_VPROC_DLC_N_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vproc_dlc_n_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON15),
                             (U32)(val),
                             (U32)(PMIC_VPROC_DLC_N_SLEEP_MASK),
                             (U32)(PMIC_VPROC_DLC_N_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_dlc_n_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON15),
                             (U32)(val),
                             (U32)(PMIC_VPROC_DLC_N_ON_MASK),
                             (U32)(PMIC_VPROC_DLC_N_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_dlc_n(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON15),
                             (U32)(val),
                             (U32)(PMIC_VPROC_DLC_N_MASK),
                             (U32)(PMIC_VPROC_DLC_N_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vproc_bursth(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON16),
                           (&val),
                           (U32)(PMIC_QI_VPROC_BURSTH_MASK),
                           (U32)(PMIC_QI_VPROC_BURSTH_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vproc_burstl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON17),
                           (&val),
                           (U32)(PMIC_QI_VPROC_BURSTL_MASK),
                           (U32)(PMIC_QI_VPROC_BURSTL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vproc_vsleep_sel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON18),
                           (&val),
                           (U32)(PMIC_NI_VPROC_VSLEEP_SEL_MASK),
                           (U32)(PMIC_NI_VPROC_VSLEEP_SEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vproc_r2r_pdn(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON18),
                           (&val),
                           (U32)(PMIC_NI_VPROC_R2R_PDN_MASK),
                           (U32)(PMIC_NI_VPROC_R2R_PDN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vproc_vsleep_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPROC_VSLEEP_SEL_MASK),
                             (U32)(PMIC_VPROC_VSLEEP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_r2r_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPROC_R2R_PDN_MASK),
                             (U32)(PMIC_VPROC_R2R_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_vsleep_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPROC_VSLEEP_EN_MASK),
                             (U32)(PMIC_VPROC_VSLEEP_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vproc_vosel_trans(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPROC_CON18),
                           (&val),
                           (U32)(PMIC_NI_VPROC_VOSEL_TRANS_MASK),
                           (U32)(PMIC_NI_VPROC_VOSEL_TRANS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vproc_vosel_trans_once(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPROC_VOSEL_TRANS_ONCE_MASK),
                             (U32)(PMIC_VPROC_VOSEL_TRANS_ONCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_vosel_trans_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPROC_VOSEL_TRANS_EN_MASK),
                             (U32)(PMIC_VPROC_VOSEL_TRANS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_transtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPROC_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPROC_TRANSTD_MASK),
                             (U32)(PMIC_VPROC_TRANSTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_trimh(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_TRIMH_MASK),
                             (U32)(PMIC_RG_VSRAM_TRIMH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_triml(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_TRIML_MASK),
                             (U32)(PMIC_RG_VSRAM_TRIML_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_zx_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_ZX_OS_MASK),
                             (U32)(PMIC_RG_VSRAM_ZX_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_csl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_CSL_MASK),
                             (U32)(PMIC_RG_VSRAM_CSL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_csr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_CSR_MASK),
                             (U32)(PMIC_RG_VSRAM_CSR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_cc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_CC_MASK),
                             (U32)(PMIC_RG_VSRAM_CC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_rzsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_RZSEL_MASK),
                             (U32)(PMIC_RG_VSRAM_RZSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VSRAM_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_modeset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_MODESET_MASK),
                             (U32)(PMIC_RG_VSRAM_MODESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_csm(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_CSM_MASK),
                             (U32)(PMIC_RG_VSRAM_CSM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_qi_vsram_vsleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON3),
                             (U32)(val),
                             (U32)(PMIC_QI_VSRAM_VSLEEP_MASK),
                             (U32)(PMIC_QI_VSRAM_VSLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_slp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_SLP_MASK),
                             (U32)(PMIC_RG_VSRAM_SLP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsram_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VSRAM_RSV_MASK),
                             (U32)(PMIC_RG_VSRAM_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_track_sleep_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_TRACK_SLEEP_CTRL_MASK),
                             (U32)(PMIC_VSRAM_TRACK_SLEEP_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_track_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_TRACK_ON_CTRL_MASK),
                             (U32)(PMIC_VSRAM_TRACK_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_burst_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_BURST_CTRL_MASK),
                             (U32)(PMIC_VSRAM_BURST_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_dlc_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_DLC_CTRL_MASK),
                             (U32)(PMIC_VSRAM_DLC_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_CTRL_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_en_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_EN_CTRL_MASK),
                             (U32)(PMIC_VSRAM_EN_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_burst_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON6),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_BURST_SEL_MASK),
                             (U32)(PMIC_VSRAM_BURST_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_dlc_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON6),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_DLC_SEL_MASK),
                             (U32)(PMIC_VSRAM_DLC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON6),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_SEL_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON6),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_EN_SEL_MASK),
                             (U32)(PMIC_VSRAM_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vsram_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON7),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VSRAM_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vsram_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON7),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_MODE_MASK),
                           (U32)(PMIC_QI_VSRAM_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vsram_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON7),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_EN_MASK),
                           (U32)(PMIC_QI_VSRAM_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vsram_stb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON7),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_STB_MASK),
                           (U32)(PMIC_QI_VSRAM_STB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vsram_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON7),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_EN_MASK),
                             (U32)(PMIC_VSRAM_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_sfchg_ren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON8),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_SFCHG_REN_MASK),
                             (U32)(PMIC_VSRAM_SFCHG_REN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_sfchg_rrate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON8),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_SFCHG_RRATE_MASK),
                             (U32)(PMIC_VSRAM_SFCHG_RRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_sfchg_fen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON8),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_SFCHG_FEN_MASK),
                             (U32)(PMIC_VSRAM_SFCHG_FEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_sfchg_frate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON8),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_SFCHG_FRATE_MASK),
                             (U32)(PMIC_VSRAM_SFCHG_FRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON9),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON10),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_ON_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON11),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_SLEEP_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vsram_vosel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON12),
                           (&val),
                           (U32)(PMIC_NI_VSRAM_VOSEL_MASK),
                           (U32)(PMIC_NI_VSRAM_VOSEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vsram_burst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON13),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_BURST_MASK),
                           (U32)(PMIC_QI_VSRAM_BURST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vsram_burst_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON13),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_BURST_SLEEP_MASK),
                             (U32)(PMIC_VSRAM_BURST_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_burst_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON13),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_BURST_ON_MASK),
                             (U32)(PMIC_VSRAM_BURST_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_burst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON13),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_BURST_MASK),
                             (U32)(PMIC_VSRAM_BURST_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vsram_dlc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON14),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_DLC_MASK),
                           (U32)(PMIC_QI_VSRAM_DLC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vsram_dlc_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON14),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_DLC_SLEEP_MASK),
                             (U32)(PMIC_VSRAM_DLC_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_dlc_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON14),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_DLC_ON_MASK),
                             (U32)(PMIC_VSRAM_DLC_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_dlc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON14),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_DLC_MASK),
                             (U32)(PMIC_VSRAM_DLC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vsram_dlc_n(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON15),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_DLC_N_MASK),
                           (U32)(PMIC_QI_VSRAM_DLC_N_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vsram_dlc_n_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON15),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_DLC_N_SLEEP_MASK),
                             (U32)(PMIC_VSRAM_DLC_N_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_dlc_n_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON15),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_DLC_N_ON_MASK),
                             (U32)(PMIC_VSRAM_DLC_N_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_dlc_n(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON15),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_DLC_N_MASK),
                             (U32)(PMIC_VSRAM_DLC_N_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vsram_bursth(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON16),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_BURSTH_MASK),
                           (U32)(PMIC_QI_VSRAM_BURSTH_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vsram_burstl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON17),
                           (&val),
                           (U32)(PMIC_QI_VSRAM_BURSTL_MASK),
                           (U32)(PMIC_QI_VSRAM_BURSTL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vsram_vsleep_sel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON18),
                           (&val),
                           (U32)(PMIC_NI_VSRAM_VSLEEP_SEL_MASK),
                           (U32)(PMIC_NI_VSRAM_VSLEEP_SEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vsram_r2r_pdn(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON18),
                           (&val),
                           (U32)(PMIC_NI_VSRAM_R2R_PDN_MASK),
                           (U32)(PMIC_NI_VSRAM_R2R_PDN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vsram_vsleep_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VSLEEP_SEL_MASK),
                             (U32)(PMIC_VSRAM_VSLEEP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_r2r_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_R2R_PDN_MASK),
                             (U32)(PMIC_VSRAM_R2R_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vsleep_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VSLEEP_EN_MASK),
                             (U32)(PMIC_VSRAM_VSLEEP_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vsram_vosel_trans(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VSRAM_CON18),
                           (&val),
                           (U32)(PMIC_NI_VSRAM_VOSEL_TRANS_MASK),
                           (U32)(PMIC_NI_VSRAM_VOSEL_TRANS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vsram_vosel_trans_once(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_TRANS_ONCE_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_TRANS_ONCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_trans_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_TRANS_EN_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_TRANS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_transtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_TRANSTD_MASK),
                             (U32)(PMIC_VSRAM_TRANSTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_offset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON19),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_OFFSET_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_OFFSET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_delta(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON19),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_DELTA_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_DELTA_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_on_hb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON20),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_ON_HB_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_ON_HB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_on_lb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON20),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_ON_LB_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_ON_LB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_vosel_sleep_lb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VSRAM_CON21),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_VOSEL_SLEEP_LB_MASK),
                             (U32)(PMIC_VSRAM_VOSEL_SLEEP_LB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_trimh(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_TRIMH_MASK),
                             (U32)(PMIC_RG_VCORE_TRIMH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_triml(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_TRIML_MASK),
                             (U32)(PMIC_RG_VCORE_TRIML_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_zx_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_ZX_OS_MASK),
                             (U32)(PMIC_RG_VCORE_ZX_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_csl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_CSL_MASK),
                             (U32)(PMIC_RG_VCORE_CSL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_csr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_CSR_MASK),
                             (U32)(PMIC_RG_VCORE_CSR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_cc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_CC_MASK),
                             (U32)(PMIC_RG_VCORE_CC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_rzsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_RZSEL_MASK),
                             (U32)(PMIC_RG_VCORE_RZSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VCORE_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_modeset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_MODESET_MASK),
                             (U32)(PMIC_RG_VCORE_MODESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_csm(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_CSM_MASK),
                             (U32)(PMIC_RG_VCORE_CSM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_avp_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_AVP_EN_MASK),
                             (U32)(PMIC_RG_VCORE_AVP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_avp_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_AVP_OS_MASK),
                             (U32)(PMIC_RG_VCORE_AVP_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_qi_vcore_vsleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON3),
                             (U32)(val),
                             (U32)(PMIC_QI_VCORE_VSLEEP_MASK),
                             (U32)(PMIC_QI_VCORE_VSLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_slp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_SLP_MASK),
                             (U32)(PMIC_RG_VCORE_SLP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcore_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VCORE_RSV_MASK),
                             (U32)(PMIC_RG_VCORE_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_burst_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON5),
                             (U32)(val),
                             (U32)(PMIC_VCORE_BURST_CTRL_MASK),
                             (U32)(PMIC_VCORE_BURST_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_dlc_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON5),
                             (U32)(val),
                             (U32)(PMIC_VCORE_DLC_CTRL_MASK),
                             (U32)(PMIC_VCORE_DLC_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_vosel_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON5),
                             (U32)(val),
                             (U32)(PMIC_VCORE_VOSEL_CTRL_MASK),
                             (U32)(PMIC_VCORE_VOSEL_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_en_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON5),
                             (U32)(val),
                             (U32)(PMIC_VCORE_EN_CTRL_MASK),
                             (U32)(PMIC_VCORE_EN_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_burst_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON6),
                             (U32)(val),
                             (U32)(PMIC_VCORE_BURST_SEL_MASK),
                             (U32)(PMIC_VCORE_BURST_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_dlc_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON6),
                             (U32)(val),
                             (U32)(PMIC_VCORE_DLC_SEL_MASK),
                             (U32)(PMIC_VCORE_DLC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_vosel_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON6),
                             (U32)(val),
                             (U32)(PMIC_VCORE_VOSEL_SEL_MASK),
                             (U32)(PMIC_VCORE_VOSEL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON6),
                             (U32)(val),
                             (U32)(PMIC_VCORE_EN_SEL_MASK),
                             (U32)(PMIC_VCORE_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vcore_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON7),
                           (&val),
                           (U32)(PMIC_QI_VCORE_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VCORE_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vcore_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON7),
                           (&val),
                           (U32)(PMIC_QI_VCORE_MODE_MASK),
                           (U32)(PMIC_QI_VCORE_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vcore_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON7),
                           (&val),
                           (U32)(PMIC_QI_VCORE_EN_MASK),
                           (U32)(PMIC_QI_VCORE_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vcore_stb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON7),
                           (&val),
                           (U32)(PMIC_QI_VCORE_STB_MASK),
                           (U32)(PMIC_QI_VCORE_STB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vcore_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON7),
                             (U32)(val),
                             (U32)(PMIC_VCORE_EN_MASK),
                             (U32)(PMIC_VCORE_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_sfchg_ren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON8),
                             (U32)(val),
                             (U32)(PMIC_VCORE_SFCHG_REN_MASK),
                             (U32)(PMIC_VCORE_SFCHG_REN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_sfchg_rrate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON8),
                             (U32)(val),
                             (U32)(PMIC_VCORE_SFCHG_RRATE_MASK),
                             (U32)(PMIC_VCORE_SFCHG_RRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_sfchg_fen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON8),
                             (U32)(val),
                             (U32)(PMIC_VCORE_SFCHG_FEN_MASK),
                             (U32)(PMIC_VCORE_SFCHG_FEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_sfchg_frate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON8),
                             (U32)(val),
                             (U32)(PMIC_VCORE_SFCHG_FRATE_MASK),
                             (U32)(PMIC_VCORE_SFCHG_FRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON9),
                             (U32)(val),
                             (U32)(PMIC_VCORE_VOSEL_MASK),
                             (U32)(PMIC_VCORE_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_vosel_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON10),
                             (U32)(val),
                             (U32)(PMIC_VCORE_VOSEL_ON_MASK),
                             (U32)(PMIC_VCORE_VOSEL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_vosel_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON11),
                             (U32)(val),
                             (U32)(PMIC_VCORE_VOSEL_SLEEP_MASK),
                             (U32)(PMIC_VCORE_VOSEL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vcore_vosel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON12),
                           (&val),
                           (U32)(PMIC_NI_VCORE_VOSEL_MASK),
                           (U32)(PMIC_NI_VCORE_VOSEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vcore_burst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON13),
                           (&val),
                           (U32)(PMIC_QI_VCORE_BURST_MASK),
                           (U32)(PMIC_QI_VCORE_BURST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vcore_burst_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON13),
                             (U32)(val),
                             (U32)(PMIC_VCORE_BURST_SLEEP_MASK),
                             (U32)(PMIC_VCORE_BURST_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_burst_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON13),
                             (U32)(val),
                             (U32)(PMIC_VCORE_BURST_ON_MASK),
                             (U32)(PMIC_VCORE_BURST_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_burst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON13),
                             (U32)(val),
                             (U32)(PMIC_VCORE_BURST_MASK),
                             (U32)(PMIC_VCORE_BURST_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vcore_dlc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON14),
                           (&val),
                           (U32)(PMIC_QI_VCORE_DLC_MASK),
                           (U32)(PMIC_QI_VCORE_DLC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vcore_dlc_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON14),
                             (U32)(val),
                             (U32)(PMIC_VCORE_DLC_SLEEP_MASK),
                             (U32)(PMIC_VCORE_DLC_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_dlc_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON14),
                             (U32)(val),
                             (U32)(PMIC_VCORE_DLC_ON_MASK),
                             (U32)(PMIC_VCORE_DLC_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_dlc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON14),
                             (U32)(val),
                             (U32)(PMIC_VCORE_DLC_MASK),
                             (U32)(PMIC_VCORE_DLC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vcore_dlc_n(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON15),
                           (&val),
                           (U32)(PMIC_QI_VCORE_DLC_N_MASK),
                           (U32)(PMIC_QI_VCORE_DLC_N_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vcore_dlc_n_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON15),
                             (U32)(val),
                             (U32)(PMIC_VCORE_DLC_N_SLEEP_MASK),
                             (U32)(PMIC_VCORE_DLC_N_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_dlc_n_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON15),
                             (U32)(val),
                             (U32)(PMIC_VCORE_DLC_N_ON_MASK),
                             (U32)(PMIC_VCORE_DLC_N_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_dlc_n(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON15),
                             (U32)(val),
                             (U32)(PMIC_VCORE_DLC_N_MASK),
                             (U32)(PMIC_VCORE_DLC_N_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vcore_bursth(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON16),
                           (&val),
                           (U32)(PMIC_QI_VCORE_BURSTH_MASK),
                           (U32)(PMIC_QI_VCORE_BURSTH_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vcore_burstl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON17),
                           (&val),
                           (U32)(PMIC_QI_VCORE_BURSTL_MASK),
                           (U32)(PMIC_QI_VCORE_BURSTL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vcore_vsleep_sel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON18),
                           (&val),
                           (U32)(PMIC_NI_VCORE_VSLEEP_SEL_MASK),
                           (U32)(PMIC_NI_VCORE_VSLEEP_SEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vcore_r2r_pdn(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON18),
                           (&val),
                           (U32)(PMIC_NI_VCORE_R2R_PDN_MASK),
                           (U32)(PMIC_NI_VCORE_R2R_PDN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vcore_vsleep_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON18),
                             (U32)(val),
                             (U32)(PMIC_VCORE_VSLEEP_SEL_MASK),
                             (U32)(PMIC_VCORE_VSLEEP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_r2r_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON18),
                             (U32)(val),
                             (U32)(PMIC_VCORE_R2R_PDN_MASK),
                             (U32)(PMIC_VCORE_R2R_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_vsleep_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON18),
                             (U32)(val),
                             (U32)(PMIC_VCORE_VSLEEP_EN_MASK),
                             (U32)(PMIC_VCORE_VSLEEP_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vcore_vosel_trans(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VCORE_CON18),
                           (&val),
                           (U32)(PMIC_NI_VCORE_VOSEL_TRANS_MASK),
                           (U32)(PMIC_NI_VCORE_VOSEL_TRANS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vcore_vosel_trans_once(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON18),
                             (U32)(val),
                             (U32)(PMIC_VCORE_VOSEL_TRANS_ONCE_MASK),
                             (U32)(PMIC_VCORE_VOSEL_TRANS_ONCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_vosel_trans_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON18),
                             (U32)(val),
                             (U32)(PMIC_VCORE_VOSEL_TRANS_EN_MASK),
                             (U32)(PMIC_VCORE_VOSEL_TRANS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_transtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VCORE_CON18),
                             (U32)(val),
                             (U32)(PMIC_VCORE_TRANSTD_MASK),
                             (U32)(PMIC_VCORE_TRANSTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_trimh(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_TRIMH_MASK),
                             (U32)(PMIC_RG_VM_TRIMH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_triml(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_TRIML_MASK),
                             (U32)(PMIC_RG_VM_TRIML_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_zx_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_ZX_OS_MASK),
                             (U32)(PMIC_RG_VM_ZX_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_csl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_CSL_MASK),
                             (U32)(PMIC_RG_VM_CSL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_csr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_CSR_MASK),
                             (U32)(PMIC_RG_VM_CSR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_cc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_CC_MASK),
                             (U32)(PMIC_RG_VM_CC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_rzsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_RZSEL_MASK),
                             (U32)(PMIC_RG_VM_RZSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VM_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_modeset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_MODESET_MASK),
                             (U32)(PMIC_RG_VM_MODESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_csm(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_CSM_MASK),
                             (U32)(PMIC_RG_VM_CSM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_avp_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_AVP_EN_MASK),
                             (U32)(PMIC_RG_VM_AVP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_avp_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_AVP_OS_MASK),
                             (U32)(PMIC_RG_VM_AVP_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_qi_vm_vsleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON3),
                             (U32)(val),
                             (U32)(PMIC_QI_VM_VSLEEP_MASK),
                             (U32)(PMIC_QI_VM_VSLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_slp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_SLP_MASK),
                             (U32)(PMIC_RG_VM_SLP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vm_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VM_RSV_MASK),
                             (U32)(PMIC_RG_VM_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_burst_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VM_BURST_CTRL_MASK),
                             (U32)(PMIC_VM_BURST_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_dlc_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VM_DLC_CTRL_MASK),
                             (U32)(PMIC_VM_DLC_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_vosel_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VM_VOSEL_CTRL_MASK),
                             (U32)(PMIC_VM_VOSEL_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_en_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON5),
                             (U32)(val),
                             (U32)(PMIC_VM_EN_CTRL_MASK),
                             (U32)(PMIC_VM_EN_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_burst_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON6),
                             (U32)(val),
                             (U32)(PMIC_VM_BURST_SEL_MASK),
                             (U32)(PMIC_VM_BURST_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_dlc_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON6),
                             (U32)(val),
                             (U32)(PMIC_VM_DLC_SEL_MASK),
                             (U32)(PMIC_VM_DLC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_vosel_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON6),
                             (U32)(val),
                             (U32)(PMIC_VM_VOSEL_SEL_MASK),
                             (U32)(PMIC_VM_VOSEL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON6),
                             (U32)(val),
                             (U32)(PMIC_VM_EN_SEL_MASK),
                             (U32)(PMIC_VM_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vm_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON7),
                           (&val),
                           (U32)(PMIC_QI_VM_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VM_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vm_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON7),
                           (&val),
                           (U32)(PMIC_QI_VM_MODE_MASK),
                           (U32)(PMIC_QI_VM_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vm_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON7),
                           (&val),
                           (U32)(PMIC_QI_VM_EN_MASK),
                           (U32)(PMIC_QI_VM_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vm_stb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON7),
                           (&val),
                           (U32)(PMIC_QI_VM_STB_MASK),
                           (U32)(PMIC_QI_VM_STB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vm_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON7),
                             (U32)(val),
                             (U32)(PMIC_VM_EN_MASK),
                             (U32)(PMIC_VM_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_sfchg_ren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON8),
                             (U32)(val),
                             (U32)(PMIC_VM_SFCHG_REN_MASK),
                             (U32)(PMIC_VM_SFCHG_REN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_sfchg_rrate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON8),
                             (U32)(val),
                             (U32)(PMIC_VM_SFCHG_RRATE_MASK),
                             (U32)(PMIC_VM_SFCHG_RRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_sfchg_fen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON8),
                             (U32)(val),
                             (U32)(PMIC_VM_SFCHG_FEN_MASK),
                             (U32)(PMIC_VM_SFCHG_FEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_sfchg_frate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON8),
                             (U32)(val),
                             (U32)(PMIC_VM_SFCHG_FRATE_MASK),
                             (U32)(PMIC_VM_SFCHG_FRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON9),
                             (U32)(val),
                             (U32)(PMIC_VM_VOSEL_MASK),
                             (U32)(PMIC_VM_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_vosel_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON10),
                             (U32)(val),
                             (U32)(PMIC_VM_VOSEL_ON_MASK),
                             (U32)(PMIC_VM_VOSEL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_vosel_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON11),
                             (U32)(val),
                             (U32)(PMIC_VM_VOSEL_SLEEP_MASK),
                             (U32)(PMIC_VM_VOSEL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vm_vosel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON12),
                           (&val),
                           (U32)(PMIC_NI_VM_VOSEL_MASK),
                           (U32)(PMIC_NI_VM_VOSEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vm_burst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON13),
                           (&val),
                           (U32)(PMIC_QI_VM_BURST_MASK),
                           (U32)(PMIC_QI_VM_BURST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vm_burst_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON13),
                             (U32)(val),
                             (U32)(PMIC_VM_BURST_SLEEP_MASK),
                             (U32)(PMIC_VM_BURST_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_burst_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON13),
                             (U32)(val),
                             (U32)(PMIC_VM_BURST_ON_MASK),
                             (U32)(PMIC_VM_BURST_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_burst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON13),
                             (U32)(val),
                             (U32)(PMIC_VM_BURST_MASK),
                             (U32)(PMIC_VM_BURST_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vm_dlc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON14),
                           (&val),
                           (U32)(PMIC_QI_VM_DLC_MASK),
                           (U32)(PMIC_QI_VM_DLC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vm_dlc_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON14),
                             (U32)(val),
                             (U32)(PMIC_VM_DLC_SLEEP_MASK),
                             (U32)(PMIC_VM_DLC_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_dlc_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON14),
                             (U32)(val),
                             (U32)(PMIC_VM_DLC_ON_MASK),
                             (U32)(PMIC_VM_DLC_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_dlc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON14),
                             (U32)(val),
                             (U32)(PMIC_VM_DLC_MASK),
                             (U32)(PMIC_VM_DLC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vm_dlc_n(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON15),
                           (&val),
                           (U32)(PMIC_QI_VM_DLC_N_MASK),
                           (U32)(PMIC_QI_VM_DLC_N_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vm_dlc_n_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON15),
                             (U32)(val),
                             (U32)(PMIC_VM_DLC_N_SLEEP_MASK),
                             (U32)(PMIC_VM_DLC_N_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_dlc_n_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON15),
                             (U32)(val),
                             (U32)(PMIC_VM_DLC_N_ON_MASK),
                             (U32)(PMIC_VM_DLC_N_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_dlc_n(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON15),
                             (U32)(val),
                             (U32)(PMIC_VM_DLC_N_MASK),
                             (U32)(PMIC_VM_DLC_N_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vm_bursth(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON16),
                           (&val),
                           (U32)(PMIC_QI_VM_BURSTH_MASK),
                           (U32)(PMIC_QI_VM_BURSTH_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vm_burstl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON17),
                           (&val),
                           (U32)(PMIC_QI_VM_BURSTL_MASK),
                           (U32)(PMIC_QI_VM_BURSTL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vm_vsleep_sel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON18),
                           (&val),
                           (U32)(PMIC_NI_VM_VSLEEP_SEL_MASK),
                           (U32)(PMIC_NI_VM_VSLEEP_SEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vm_r2r_pdn(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON18),
                           (&val),
                           (U32)(PMIC_NI_VM_R2R_PDN_MASK),
                           (U32)(PMIC_NI_VM_R2R_PDN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vm_vsleep_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VM_VSLEEP_SEL_MASK),
                             (U32)(PMIC_VM_VSLEEP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_r2r_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VM_R2R_PDN_MASK),
                             (U32)(PMIC_VM_R2R_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_vsleep_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VM_VSLEEP_EN_MASK),
                             (U32)(PMIC_VM_VSLEEP_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vm_vosel_trans(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VM_CON18),
                           (&val),
                           (U32)(PMIC_NI_VM_VOSEL_TRANS_MASK),
                           (U32)(PMIC_NI_VM_VOSEL_TRANS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vm_vosel_trans_once(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VM_VOSEL_TRANS_ONCE_MASK),
                             (U32)(PMIC_VM_VOSEL_TRANS_ONCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_vosel_trans_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VM_VOSEL_TRANS_EN_MASK),
                             (U32)(PMIC_VM_VOSEL_TRANS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_transtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VM_CON18),
                             (U32)(val),
                             (U32)(PMIC_VM_TRANSTD_MASK),
                             (U32)(PMIC_VM_TRANSTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_TRIM_MASK),
                             (U32)(PMIC_RG_VIO18_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_zx_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_ZX_OS_MASK),
                             (U32)(PMIC_RG_VIO18_ZX_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_slew(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_SLEW_MASK),
                             (U32)(PMIC_RG_VIO18_SLEW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_slew_nmos(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_SLEW_NMOS_MASK),
                             (U32)(PMIC_RG_VIO18_SLEW_NMOS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_csl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_CSL_MASK),
                             (U32)(PMIC_RG_VIO18_CSL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_csr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_CSR_MASK),
                             (U32)(PMIC_RG_VIO18_CSR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_cc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_CC_MASK),
                             (U32)(PMIC_RG_VIO18_CC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_rzsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_RZSEL_MASK),
                             (U32)(PMIC_RG_VIO18_RZSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_csmir(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_CSMIR_MASK),
                             (U32)(PMIC_RG_VIO18_CSMIR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VIO18_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_modeset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_MODESET_MASK),
                             (U32)(PMIC_RG_VIO18_MODESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_slp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_SLP_MASK),
                             (U32)(PMIC_RG_VIO18_SLP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio18_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO18_RSV_MASK),
                             (U32)(PMIC_RG_VIO18_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_burst_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON5),
                             (U32)(val),
                             (U32)(PMIC_VIO18_BURST_CTRL_MASK),
                             (U32)(PMIC_VIO18_BURST_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_dlc_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON5),
                             (U32)(val),
                             (U32)(PMIC_VIO18_DLC_CTRL_MASK),
                             (U32)(PMIC_VIO18_DLC_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_vosel_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON5),
                             (U32)(val),
                             (U32)(PMIC_VIO18_VOSEL_CTRL_MASK),
                             (U32)(PMIC_VIO18_VOSEL_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_en_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON5),
                             (U32)(val),
                             (U32)(PMIC_VIO18_EN_CTRL_MASK),
                             (U32)(PMIC_VIO18_EN_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_burst_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON6),
                             (U32)(val),
                             (U32)(PMIC_VIO18_BURST_SEL_MASK),
                             (U32)(PMIC_VIO18_BURST_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_dlc_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON6),
                             (U32)(val),
                             (U32)(PMIC_VIO18_DLC_SEL_MASK),
                             (U32)(PMIC_VIO18_DLC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_vosel_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON6),
                             (U32)(val),
                             (U32)(PMIC_VIO18_VOSEL_SEL_MASK),
                             (U32)(PMIC_VIO18_VOSEL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON6),
                             (U32)(val),
                             (U32)(PMIC_VIO18_EN_SEL_MASK),
                             (U32)(PMIC_VIO18_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vio18_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON7),
                           (&val),
                           (U32)(PMIC_QI_VIO18_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VIO18_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vio18_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON7),
                           (&val),
                           (U32)(PMIC_QI_VIO18_MODE_MASK),
                           (U32)(PMIC_QI_VIO18_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vio18_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON7),
                           (&val),
                           (U32)(PMIC_QI_VIO18_EN_MASK),
                           (U32)(PMIC_QI_VIO18_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vio18_stb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON7),
                           (&val),
                           (U32)(PMIC_QI_VIO18_STB_MASK),
                           (U32)(PMIC_QI_VIO18_STB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vio18_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON7),
                             (U32)(val),
                             (U32)(PMIC_VIO18_EN_MASK),
                             (U32)(PMIC_VIO18_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_sfchg_ren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON8),
                             (U32)(val),
                             (U32)(PMIC_VIO18_SFCHG_REN_MASK),
                             (U32)(PMIC_VIO18_SFCHG_REN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_sfchg_rrate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON8),
                             (U32)(val),
                             (U32)(PMIC_VIO18_SFCHG_RRATE_MASK),
                             (U32)(PMIC_VIO18_SFCHG_RRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_sfchg_fen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON8),
                             (U32)(val),
                             (U32)(PMIC_VIO18_SFCHG_FEN_MASK),
                             (U32)(PMIC_VIO18_SFCHG_FEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_sfchg_frate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON8),
                             (U32)(val),
                             (U32)(PMIC_VIO18_SFCHG_FRATE_MASK),
                             (U32)(PMIC_VIO18_SFCHG_FRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON9),
                             (U32)(val),
                             (U32)(PMIC_VIO18_VOSEL_MASK),
                             (U32)(PMIC_VIO18_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_vosel_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON10),
                             (U32)(val),
                             (U32)(PMIC_VIO18_VOSEL_ON_MASK),
                             (U32)(PMIC_VIO18_VOSEL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_vosel_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON11),
                             (U32)(val),
                             (U32)(PMIC_VIO18_VOSEL_SLEEP_MASK),
                             (U32)(PMIC_VIO18_VOSEL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vio18_vosel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON12),
                           (&val),
                           (U32)(PMIC_NI_VIO18_VOSEL_MASK),
                           (U32)(PMIC_NI_VIO18_VOSEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vio18_burst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON13),
                           (&val),
                           (U32)(PMIC_QI_VIO18_BURST_MASK),
                           (U32)(PMIC_QI_VIO18_BURST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vio18_dlc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON14),
                           (&val),
                           (U32)(PMIC_QI_VIO18_DLC_MASK),
                           (U32)(PMIC_QI_VIO18_DLC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vio18_dlc_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON14),
                             (U32)(val),
                             (U32)(PMIC_VIO18_DLC_SLEEP_MASK),
                             (U32)(PMIC_VIO18_DLC_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_dlc_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON14),
                             (U32)(val),
                             (U32)(PMIC_VIO18_DLC_ON_MASK),
                             (U32)(PMIC_VIO18_DLC_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_dlc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON14),
                             (U32)(val),
                             (U32)(PMIC_VIO18_DLC_MASK),
                             (U32)(PMIC_VIO18_DLC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vio18_dlc_n(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON15),
                           (&val),
                           (U32)(PMIC_QI_VIO18_DLC_N_MASK),
                           (U32)(PMIC_QI_VIO18_DLC_N_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vio18_dlc_n_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON15),
                             (U32)(val),
                             (U32)(PMIC_VIO18_DLC_N_SLEEP_MASK),
                             (U32)(PMIC_VIO18_DLC_N_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_dlc_n_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON15),
                             (U32)(val),
                             (U32)(PMIC_VIO18_DLC_N_ON_MASK),
                             (U32)(PMIC_VIO18_DLC_N_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_dlc_n(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON15),
                             (U32)(val),
                             (U32)(PMIC_VIO18_DLC_N_MASK),
                             (U32)(PMIC_VIO18_DLC_N_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vio18_bursth(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON16),
                           (&val),
                           (U32)(PMIC_QI_VIO18_BURSTH_MASK),
                           (U32)(PMIC_QI_VIO18_BURSTH_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vio18_bursth_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON16),
                             (U32)(val),
                             (U32)(PMIC_VIO18_BURSTH_SLEEP_MASK),
                             (U32)(PMIC_VIO18_BURSTH_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_bursth_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON16),
                             (U32)(val),
                             (U32)(PMIC_VIO18_BURSTH_ON_MASK),
                             (U32)(PMIC_VIO18_BURSTH_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_bursth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON16),
                             (U32)(val),
                             (U32)(PMIC_VIO18_BURSTH_MASK),
                             (U32)(PMIC_VIO18_BURSTH_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vio18_burstl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON17),
                           (&val),
                           (U32)(PMIC_QI_VIO18_BURSTL_MASK),
                           (U32)(PMIC_QI_VIO18_BURSTL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vio18_burstl_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON17),
                             (U32)(val),
                             (U32)(PMIC_VIO18_BURSTL_SLEEP_MASK),
                             (U32)(PMIC_VIO18_BURSTL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_burstl_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON17),
                             (U32)(val),
                             (U32)(PMIC_VIO18_BURSTL_ON_MASK),
                             (U32)(PMIC_VIO18_BURSTL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_burstl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON17),
                             (U32)(val),
                             (U32)(PMIC_VIO18_BURSTL_MASK),
                             (U32)(PMIC_VIO18_BURSTL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vio18_vsleep_sel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON18),
                           (&val),
                           (U32)(PMIC_NI_VIO18_VSLEEP_SEL_MASK),
                           (U32)(PMIC_NI_VIO18_VSLEEP_SEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vio18_sleep_pdn(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON18),
                           (&val),
                           (U32)(PMIC_QI_VIO18_SLEEP_PDN_MASK),
                           (U32)(PMIC_QI_VIO18_SLEEP_PDN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vio18_vsleep_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VIO18_VSLEEP_SEL_MASK),
                             (U32)(PMIC_VIO18_VSLEEP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_sleep_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VIO18_SLEEP_PDN_MASK),
                             (U32)(PMIC_VIO18_SLEEP_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_vsleep_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VIO18_VSLEEP_EN_MASK),
                             (U32)(PMIC_VIO18_VSLEEP_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vio18_vosel_trans(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VIO18_CON18),
                           (&val),
                           (U32)(PMIC_NI_VIO18_VOSEL_TRANS_MASK),
                           (U32)(PMIC_NI_VIO18_VOSEL_TRANS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vio18_vosel_trans_once(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VIO18_VOSEL_TRANS_ONCE_MASK),
                             (U32)(PMIC_VIO18_VOSEL_TRANS_ONCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_vosel_trans_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VIO18_VOSEL_TRANS_EN_MASK),
                             (U32)(PMIC_VIO18_VOSEL_TRANS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_transtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VIO18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VIO18_TRANSTD_MASK),
                             (U32)(PMIC_VIO18_TRANSTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_trimh(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_TRIMH_MASK),
                             (U32)(PMIC_RG_VPA_TRIMH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_triml(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_TRIML_MASK),
                             (U32)(PMIC_RG_VPA_TRIML_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_zx_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_ZX_OS_MASK),
                             (U32)(PMIC_RG_VPA_ZX_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_slew(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_SLEW_MASK),
                             (U32)(PMIC_RG_VPA_SLEW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_slew_nmos(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_SLEW_NMOS_MASK),
                             (U32)(PMIC_RG_VPA_SLEW_NMOS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_csl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_CSL_MASK),
                             (U32)(PMIC_RG_VPA_CSL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_csr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_CSR_MASK),
                             (U32)(PMIC_RG_VPA_CSR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_cc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_CC_MASK),
                             (U32)(PMIC_RG_VPA_CC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_rzsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_RZSEL_MASK),
                             (U32)(PMIC_RG_VPA_RZSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_vbat_del(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_VBAT_DEL_MASK),
                             (U32)(PMIC_RG_VPA_VBAT_DEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_csmir(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_CSMIR_MASK),
                             (U32)(PMIC_RG_VPA_CSMIR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VPA_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_modeset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_MODESET_MASK),
                             (U32)(PMIC_RG_VPA_MODESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_gpu_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_GPU_EN_MASK),
                             (U32)(PMIC_RG_VPA_GPU_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_slp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_SLP_MASK),
                             (U32)(PMIC_RG_VPA_SLP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vpa_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VPA_RSV_MASK),
                             (U32)(PMIC_RG_VPA_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_burst_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON5),
                             (U32)(val),
                             (U32)(PMIC_VPA_BURST_CTRL_MASK),
                             (U32)(PMIC_VPA_BURST_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_dlc_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON5),
                             (U32)(val),
                             (U32)(PMIC_VPA_DLC_CTRL_MASK),
                             (U32)(PMIC_VPA_DLC_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vosel_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON5),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_CTRL_MASK),
                             (U32)(PMIC_VPA_VOSEL_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_en_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON5),
                             (U32)(val),
                             (U32)(PMIC_VPA_EN_CTRL_MASK),
                             (U32)(PMIC_VPA_EN_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_burst_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON6),
                             (U32)(val),
                             (U32)(PMIC_VPA_BURST_SEL_MASK),
                             (U32)(PMIC_VPA_BURST_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_dlc_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON6),
                             (U32)(val),
                             (U32)(PMIC_VPA_DLC_SEL_MASK),
                             (U32)(PMIC_VPA_DLC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vosel_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON6),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_SEL_MASK),
                             (U32)(PMIC_VPA_VOSEL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON6),
                             (U32)(val),
                             (U32)(PMIC_VPA_EN_SEL_MASK),
                             (U32)(PMIC_VPA_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vpa_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON7),
                           (&val),
                           (U32)(PMIC_QI_VPA_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VPA_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vpa_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON7),
                           (&val),
                           (U32)(PMIC_QI_VPA_MODE_MASK),
                           (U32)(PMIC_QI_VPA_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vpa_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON7),
                           (&val),
                           (U32)(PMIC_QI_VPA_EN_MASK),
                           (U32)(PMIC_QI_VPA_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vpa_stb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON7),
                           (&val),
                           (U32)(PMIC_QI_VPA_STB_MASK),
                           (U32)(PMIC_QI_VPA_STB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vpa_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON7),
                             (U32)(val),
                             (U32)(PMIC_VPA_EN_MASK),
                             (U32)(PMIC_VPA_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_sfchg_ren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON8),
                             (U32)(val),
                             (U32)(PMIC_VPA_SFCHG_REN_MASK),
                             (U32)(PMIC_VPA_SFCHG_REN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_sfchg_rrate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON8),
                             (U32)(val),
                             (U32)(PMIC_VPA_SFCHG_RRATE_MASK),
                             (U32)(PMIC_VPA_SFCHG_RRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_sfchg_fen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON8),
                             (U32)(val),
                             (U32)(PMIC_VPA_SFCHG_FEN_MASK),
                             (U32)(PMIC_VPA_SFCHG_FEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_sfchg_frate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON8),
                             (U32)(val),
                             (U32)(PMIC_VPA_SFCHG_FRATE_MASK),
                             (U32)(PMIC_VPA_SFCHG_FRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON9),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_MASK),
                             (U32)(PMIC_VPA_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vosel_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON10),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_ON_MASK),
                             (U32)(PMIC_VPA_VOSEL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vosel_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON11),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_SLEEP_MASK),
                             (U32)(PMIC_VPA_VOSEL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vpa_vosel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON12),
                           (&val),
                           (U32)(PMIC_NI_VPA_VOSEL_MASK),
                           (U32)(PMIC_NI_VPA_VOSEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vpa_burst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON13),
                           (&val),
                           (U32)(PMIC_QI_VPA_BURST_MASK),
                           (U32)(PMIC_QI_VPA_BURST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vpa_dlc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON14),
                           (&val),
                           (U32)(PMIC_QI_VPA_DLC_MASK),
                           (U32)(PMIC_QI_VPA_DLC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vpa_dlc_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON14),
                             (U32)(val),
                             (U32)(PMIC_VPA_DLC_SLEEP_MASK),
                             (U32)(PMIC_VPA_DLC_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_dlc_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON14),
                             (U32)(val),
                             (U32)(PMIC_VPA_DLC_ON_MASK),
                             (U32)(PMIC_VPA_DLC_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_dlc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON14),
                             (U32)(val),
                             (U32)(PMIC_VPA_DLC_MASK),
                             (U32)(PMIC_VPA_DLC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vpa_dlc_n(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON15),
                           (&val),
                           (U32)(PMIC_QI_VPA_DLC_N_MASK),
                           (U32)(PMIC_QI_VPA_DLC_N_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vpa_bursth(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON16),
                           (&val),
                           (U32)(PMIC_QI_VPA_BURSTH_MASK),
                           (U32)(PMIC_QI_VPA_BURSTH_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vpa_bursth_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON16),
                             (U32)(val),
                             (U32)(PMIC_VPA_BURSTH_SLEEP_MASK),
                             (U32)(PMIC_VPA_BURSTH_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_bursth_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON16),
                             (U32)(val),
                             (U32)(PMIC_VPA_BURSTH_ON_MASK),
                             (U32)(PMIC_VPA_BURSTH_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_bursth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON16),
                             (U32)(val),
                             (U32)(PMIC_VPA_BURSTH_MASK),
                             (U32)(PMIC_VPA_BURSTH_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vpa_burstl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON17),
                           (&val),
                           (U32)(PMIC_QI_VPA_BURSTL_MASK),
                           (U32)(PMIC_QI_VPA_BURSTL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vpa_burstl_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON17),
                             (U32)(val),
                             (U32)(PMIC_VPA_BURSTL_SLEEP_MASK),
                             (U32)(PMIC_VPA_BURSTL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_burstl_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON17),
                             (U32)(val),
                             (U32)(PMIC_VPA_BURSTL_ON_MASK),
                             (U32)(PMIC_VPA_BURSTL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_burstl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON17),
                             (U32)(val),
                             (U32)(PMIC_VPA_BURSTL_MASK),
                             (U32)(PMIC_VPA_BURSTL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vpa_vsleep_sel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON18),
                           (&val),
                           (U32)(PMIC_NI_VPA_VSLEEP_SEL_MASK),
                           (U32)(PMIC_NI_VPA_VSLEEP_SEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vpa_r2r_pdn(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON18),
                           (&val),
                           (U32)(PMIC_NI_VPA_R2R_PDN_MASK),
                           (U32)(PMIC_NI_VPA_R2R_PDN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vpa_vsleep_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPA_VSLEEP_SEL_MASK),
                             (U32)(PMIC_VPA_VSLEEP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_r2r_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPA_R2R_PDN_MASK),
                             (U32)(PMIC_VPA_R2R_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vsleep_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPA_VSLEEP_EN_MASK),
                             (U32)(PMIC_VPA_VSLEEP_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vpa_dvs_bw(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VPA_CON18),
                           (&val),
                           (U32)(PMIC_NI_VPA_DVS_BW_MASK),
                           (U32)(PMIC_NI_VPA_DVS_BW_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vpa_vosel_trans_once(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_TRANS_ONCE_MASK),
                             (U32)(PMIC_VPA_VOSEL_TRANS_ONCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vosel_trans_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_TRANS_EN_MASK),
                             (U32)(PMIC_VPA_VOSEL_TRANS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_transtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON18),
                             (U32)(val),
                             (U32)(PMIC_VPA_TRANSTD_MASK),
                             (U32)(PMIC_VPA_TRANSTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vosel_dlc001(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON19),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_DLC001_MASK),
                             (U32)(PMIC_VPA_VOSEL_DLC001_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_dlc_map_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON19),
                             (U32)(val),
                             (U32)(PMIC_VPA_DLC_MAP_EN_MASK),
                             (U32)(PMIC_VPA_DLC_MAP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vosel_dlc111(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON20),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_DLC111_MASK),
                             (U32)(PMIC_VPA_VOSEL_DLC111_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vpa_vosel_dlc011(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VPA_CON20),
                             (U32)(val),
                             (U32)(PMIC_VPA_VOSEL_DLC011_MASK),
                             (U32)(PMIC_VPA_VOSEL_DLC011_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_TRIM_MASK),
                             (U32)(PMIC_RG_VRF18_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_zx_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_ZX_OS_MASK),
                             (U32)(PMIC_RG_VRF18_ZX_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_slew(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_SLEW_MASK),
                             (U32)(PMIC_RG_VRF18_SLEW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_slew_nmos(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_SLEW_NMOS_MASK),
                             (U32)(PMIC_RG_VRF18_SLEW_NMOS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_csl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_CSL_MASK),
                             (U32)(PMIC_RG_VRF18_CSL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_csr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_CSR_MASK),
                             (U32)(PMIC_RG_VRF18_CSR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_cc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_CC_MASK),
                             (U32)(PMIC_RG_VRF18_CC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_rzsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_RZSEL_MASK),
                             (U32)(PMIC_RG_VRF18_RZSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_gmsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_GMSEL_MASK),
                             (U32)(PMIC_RG_VRF18_GMSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_ocfb_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_OCFB_EN_MASK),
                             (U32)(PMIC_RG_VRF18_OCFB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VRF18_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_modeset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_MODESET_MASK),
                             (U32)(PMIC_RG_VRF18_MODESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_avp_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_AVP_EN_MASK),
                             (U32)(PMIC_RG_VRF18_AVP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_slp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_SLP_MASK),
                             (U32)(PMIC_RG_VRF18_SLP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_rsv_7_2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_RSV_7_2_MASK),
                             (U32)(PMIC_RG_VRF18_RSV_7_2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_bk_ldo(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_BK_LDO_MASK),
                             (U32)(PMIC_RG_VRF18_BK_LDO_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_rsv_0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_RSV_0_MASK),
                             (U32)(PMIC_RG_VRF18_RSV_0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_burst_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON5),
                             (U32)(val),
                             (U32)(PMIC_VRF18_BURST_CTRL_MASK),
                             (U32)(PMIC_VRF18_BURST_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_dlc_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON5),
                             (U32)(val),
                             (U32)(PMIC_VRF18_DLC_CTRL_MASK),
                             (U32)(PMIC_VRF18_DLC_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_vosel_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON5),
                             (U32)(val),
                             (U32)(PMIC_VRF18_VOSEL_CTRL_MASK),
                             (U32)(PMIC_VRF18_VOSEL_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_en_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON5),
                             (U32)(val),
                             (U32)(PMIC_VRF18_EN_CTRL_MASK),
                             (U32)(PMIC_VRF18_EN_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_burst_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON6),
                             (U32)(val),
                             (U32)(PMIC_VRF18_BURST_SEL_MASK),
                             (U32)(PMIC_VRF18_BURST_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_dlc_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON6),
                             (U32)(val),
                             (U32)(PMIC_VRF18_DLC_SEL_MASK),
                             (U32)(PMIC_VRF18_DLC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_vosel_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON6),
                             (U32)(val),
                             (U32)(PMIC_VRF18_VOSEL_SEL_MASK),
                             (U32)(PMIC_VRF18_VOSEL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON6),
                             (U32)(val),
                             (U32)(PMIC_VRF18_EN_SEL_MASK),
                             (U32)(PMIC_VRF18_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf18_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON7),
                           (&val),
                           (U32)(PMIC_QI_VRF18_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VRF18_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON7),
                           (&val),
                           (U32)(PMIC_QI_VRF18_MODE_MASK),
                           (U32)(PMIC_QI_VRF18_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON7),
                           (&val),
                           (U32)(PMIC_QI_VRF18_EN_MASK),
                           (U32)(PMIC_QI_VRF18_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_stb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON7),
                           (&val),
                           (U32)(PMIC_QI_VRF18_STB_MASK),
                           (U32)(PMIC_QI_VRF18_STB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON7),
                             (U32)(val),
                             (U32)(PMIC_VRF18_STBTD_MASK),
                             (U32)(PMIC_VRF18_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON7),
                             (U32)(val),
                             (U32)(PMIC_VRF18_EN_MASK),
                             (U32)(PMIC_VRF18_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_sfchg_ren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON8),
                             (U32)(val),
                             (U32)(PMIC_VRF18_SFCHG_REN_MASK),
                             (U32)(PMIC_VRF18_SFCHG_REN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_sfchg_rrate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON8),
                             (U32)(val),
                             (U32)(PMIC_VRF18_SFCHG_RRATE_MASK),
                             (U32)(PMIC_VRF18_SFCHG_RRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_sfchg_fen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON8),
                             (U32)(val),
                             (U32)(PMIC_VRF18_SFCHG_FEN_MASK),
                             (U32)(PMIC_VRF18_SFCHG_FEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_sfchg_frate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON8),
                             (U32)(val),
                             (U32)(PMIC_VRF18_SFCHG_FRATE_MASK),
                             (U32)(PMIC_VRF18_SFCHG_FRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON9),
                             (U32)(val),
                             (U32)(PMIC_VRF18_VOSEL_MASK),
                             (U32)(PMIC_VRF18_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_vosel_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON10),
                             (U32)(val),
                             (U32)(PMIC_VRF18_VOSEL_ON_MASK),
                             (U32)(PMIC_VRF18_VOSEL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_vosel_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON11),
                             (U32)(val),
                             (U32)(PMIC_VRF18_VOSEL_SLEEP_MASK),
                             (U32)(PMIC_VRF18_VOSEL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vrf18_vosel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON12),
                           (&val),
                           (U32)(PMIC_NI_VRF18_VOSEL_MASK),
                           (U32)(PMIC_NI_VRF18_VOSEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_burst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON13),
                           (&val),
                           (U32)(PMIC_QI_VRF18_BURST_MASK),
                           (U32)(PMIC_QI_VRF18_BURST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_dlc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON14),
                           (&val),
                           (U32)(PMIC_QI_VRF18_DLC_MASK),
                           (U32)(PMIC_QI_VRF18_DLC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_dlc_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON14),
                             (U32)(val),
                             (U32)(PMIC_VRF18_DLC_SLEEP_MASK),
                             (U32)(PMIC_VRF18_DLC_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_dlc_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON14),
                             (U32)(val),
                             (U32)(PMIC_VRF18_DLC_ON_MASK),
                             (U32)(PMIC_VRF18_DLC_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_dlc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON14),
                             (U32)(val),
                             (U32)(PMIC_VRF18_DLC_MASK),
                             (U32)(PMIC_VRF18_DLC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf18_dlc_n(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON15),
                           (&val),
                           (U32)(PMIC_QI_VRF18_DLC_N_MASK),
                           (U32)(PMIC_QI_VRF18_DLC_N_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_dlc_n_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON15),
                             (U32)(val),
                             (U32)(PMIC_VRF18_DLC_N_SLEEP_MASK),
                             (U32)(PMIC_VRF18_DLC_N_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_dlc_n_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON15),
                             (U32)(val),
                             (U32)(PMIC_VRF18_DLC_N_ON_MASK),
                             (U32)(PMIC_VRF18_DLC_N_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_dlc_n(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON15),
                             (U32)(val),
                             (U32)(PMIC_VRF18_DLC_N_MASK),
                             (U32)(PMIC_VRF18_DLC_N_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf18_bursth(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON16),
                           (&val),
                           (U32)(PMIC_QI_VRF18_BURSTH_MASK),
                           (U32)(PMIC_QI_VRF18_BURSTH_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_bursth_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON16),
                             (U32)(val),
                             (U32)(PMIC_VRF18_BURSTH_SLEEP_MASK),
                             (U32)(PMIC_VRF18_BURSTH_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_bursth_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON16),
                             (U32)(val),
                             (U32)(PMIC_VRF18_BURSTH_ON_MASK),
                             (U32)(PMIC_VRF18_BURSTH_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_bursth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON16),
                             (U32)(val),
                             (U32)(PMIC_VRF18_BURSTH_MASK),
                             (U32)(PMIC_VRF18_BURSTH_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf18_burstl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON17),
                           (&val),
                           (U32)(PMIC_QI_VRF18_BURSTL_MASK),
                           (U32)(PMIC_QI_VRF18_BURSTL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_burstl_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON17),
                             (U32)(val),
                             (U32)(PMIC_VRF18_BURSTL_SLEEP_MASK),
                             (U32)(PMIC_VRF18_BURSTL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_burstl_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON17),
                             (U32)(val),
                             (U32)(PMIC_VRF18_BURSTL_ON_MASK),
                             (U32)(PMIC_VRF18_BURSTL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_burstl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON17),
                             (U32)(val),
                             (U32)(PMIC_VRF18_BURSTL_MASK),
                             (U32)(PMIC_VRF18_BURSTL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vrf18_vsleep_sel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON18),
                           (&val),
                           (U32)(PMIC_NI_VRF18_VSLEEP_SEL_MASK),
                           (U32)(PMIC_NI_VRF18_VSLEEP_SEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vrf18_r2r_pdn(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON18),
                           (&val),
                           (U32)(PMIC_NI_VRF18_R2R_PDN_MASK),
                           (U32)(PMIC_NI_VRF18_R2R_PDN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_vsleep_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_VSLEEP_SEL_MASK),
                             (U32)(PMIC_VRF18_VSLEEP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_r2r_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_R2R_PDN_MASK),
                             (U32)(PMIC_VRF18_R2R_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_vsleep_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_VSLEEP_EN_MASK),
                             (U32)(PMIC_VRF18_VSLEEP_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vrf18_vosel_trans(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_CON18),
                           (&val),
                           (U32)(PMIC_NI_VRF18_VOSEL_TRANS_MASK),
                           (U32)(PMIC_NI_VRF18_VOSEL_TRANS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_vosel_trans_once(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_VOSEL_TRANS_ONCE_MASK),
                             (U32)(PMIC_VRF18_VOSEL_TRANS_ONCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_vosel_trans_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_VOSEL_TRANS_EN_MASK),
                             (U32)(PMIC_VRF18_VOSEL_TRANS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_transtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_TRANSTD_MASK),
                             (U32)(PMIC_VRF18_TRANSTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_md2_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON19),
                             (U32)(val),
                             (U32)(PMIC_VRF18_MD2_EN_SEL_MASK),
                             (U32)(PMIC_VRF18_MD2_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_md2_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON19),
                             (U32)(val),
                             (U32)(PMIC_VRF18_MD2_EN_MASK),
                             (U32)(PMIC_VRF18_MD2_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_md1_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_CON20),
                             (U32)(val),
                             (U32)(PMIC_VRF18_MD1_EN_SEL_MASK),
                             (U32)(PMIC_VRF18_MD1_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_TRIM_MASK),
                             (U32)(PMIC_RG_VRF18_2_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_zx_os(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_ZX_OS_MASK),
                             (U32)(PMIC_RG_VRF18_2_ZX_OS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_slew(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_SLEW_MASK),
                             (U32)(PMIC_RG_VRF18_2_SLEW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_slew_nmos(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_SLEW_NMOS_MASK),
                             (U32)(PMIC_RG_VRF18_2_SLEW_NMOS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_csl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_CSL_MASK),
                             (U32)(PMIC_RG_VRF18_2_CSL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_csr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_CSR_MASK),
                             (U32)(PMIC_RG_VRF18_2_CSR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_cc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_CC_MASK),
                             (U32)(PMIC_RG_VRF18_2_CC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_rzsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_RZSEL_MASK),
                             (U32)(PMIC_RG_VRF18_2_RZSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_gmsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_GMSEL_MASK),
                             (U32)(PMIC_RG_VRF18_2_GMSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_ocfb_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_OCFB_EN_MASK),
                             (U32)(PMIC_RG_VRF18_2_OCFB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VRF18_2_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_modeset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_MODESET_MASK),
                             (U32)(PMIC_RG_VRF18_2_MODESET_SHIFT)
	                         );
  pmic_unlock();
}
EXPORT_SYMBOL(upmu_set_rg_vrf18_2_modeset);


void upmu_set_rg_vrf18_2_avp_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_AVP_EN_MASK),
                             (U32)(PMIC_RG_VRF18_2_AVP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_slp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_SLP_MASK),
                             (U32)(PMIC_RG_VRF18_2_SLP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_rsv_7_2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_RSV_7_2_MASK),
                             (U32)(PMIC_RG_VRF18_2_RSV_7_2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_bk_ldo(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_BK_LDO_MASK),
                             (U32)(PMIC_RG_VRF18_2_BK_LDO_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf18_2_rsv_0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF18_2_RSV_0_MASK),
                             (U32)(PMIC_RG_VRF18_2_RSV_0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_burst_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON5),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_BURST_CTRL_MASK),
                             (U32)(PMIC_VRF18_2_BURST_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_dlc_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON5),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_DLC_CTRL_MASK),
                             (U32)(PMIC_VRF18_2_DLC_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_vosel_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON5),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_VOSEL_CTRL_MASK),
                             (U32)(PMIC_VRF18_2_VOSEL_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_en_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON5),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_EN_CTRL_MASK),
                             (U32)(PMIC_VRF18_2_EN_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_burst_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON6),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_BURST_SEL_MASK),
                             (U32)(PMIC_VRF18_2_BURST_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_dlc_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON6),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_DLC_SEL_MASK),
                             (U32)(PMIC_VRF18_2_DLC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_vosel_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON6),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_VOSEL_SEL_MASK),
                             (U32)(PMIC_VRF18_2_VOSEL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON6),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_EN_SEL_MASK),
                             (U32)(PMIC_VRF18_2_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf18_2_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON7),
                           (&val),
                           (U32)(PMIC_QI_VRF18_2_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VRF18_2_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_2_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON7),
                           (&val),
                           (U32)(PMIC_QI_VRF18_2_MODE_MASK),
                           (U32)(PMIC_QI_VRF18_2_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_2_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON7),
                           (&val),
                           (U32)(PMIC_QI_VRF18_2_EN_MASK),
                           (U32)(PMIC_QI_VRF18_2_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_2_stb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON7),
                           (&val),
                           (U32)(PMIC_QI_VRF18_2_STB_MASK),
                           (U32)(PMIC_QI_VRF18_2_STB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_2_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON7),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_STBTD_MASK),
                             (U32)(PMIC_VRF18_2_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON7),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_EN_MASK),
                             (U32)(PMIC_VRF18_2_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_sfchg_ren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON8),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_SFCHG_REN_MASK),
                             (U32)(PMIC_VRF18_2_SFCHG_REN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_sfchg_rrate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON8),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_SFCHG_RRATE_MASK),
                             (U32)(PMIC_VRF18_2_SFCHG_RRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_sfchg_fen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON8),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_SFCHG_FEN_MASK),
                             (U32)(PMIC_VRF18_2_SFCHG_FEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_sfchg_frate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON8),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_SFCHG_FRATE_MASK),
                             (U32)(PMIC_VRF18_2_SFCHG_FRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON9),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_VOSEL_MASK),
                             (U32)(PMIC_VRF18_2_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_vosel_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON10),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_VOSEL_ON_MASK),
                             (U32)(PMIC_VRF18_2_VOSEL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_vosel_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON11),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_VOSEL_SLEEP_MASK),
                             (U32)(PMIC_VRF18_2_VOSEL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vrf18_2_vosel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON12),
                           (&val),
                           (U32)(PMIC_NI_VRF18_2_VOSEL_MASK),
                           (U32)(PMIC_NI_VRF18_2_VOSEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_2_burst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON13),
                           (&val),
                           (U32)(PMIC_QI_VRF18_2_BURST_MASK),
                           (U32)(PMIC_QI_VRF18_2_BURST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf18_2_dlc(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON14),
                           (&val),
                           (U32)(PMIC_QI_VRF18_2_DLC_MASK),
                           (U32)(PMIC_QI_VRF18_2_DLC_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_2_dlc_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON14),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_DLC_SLEEP_MASK),
                             (U32)(PMIC_VRF18_2_DLC_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_dlc_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON14),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_DLC_ON_MASK),
                             (U32)(PMIC_VRF18_2_DLC_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_dlc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON14),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_DLC_MASK),
                             (U32)(PMIC_VRF18_2_DLC_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf18_2_dlc_n(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON15),
                           (&val),
                           (U32)(PMIC_QI_VRF18_2_DLC_N_MASK),
                           (U32)(PMIC_QI_VRF18_2_DLC_N_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_2_dlc_n_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON15),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_DLC_N_SLEEP_MASK),
                             (U32)(PMIC_VRF18_2_DLC_N_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_dlc_n_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON15),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_DLC_N_ON_MASK),
                             (U32)(PMIC_VRF18_2_DLC_N_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_dlc_n(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON15),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_DLC_N_MASK),
                             (U32)(PMIC_VRF18_2_DLC_N_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf18_2_bursth(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON16),
                           (&val),
                           (U32)(PMIC_QI_VRF18_2_BURSTH_MASK),
                           (U32)(PMIC_QI_VRF18_2_BURSTH_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_2_bursth_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON16),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_BURSTH_SLEEP_MASK),
                             (U32)(PMIC_VRF18_2_BURSTH_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_bursth_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON16),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_BURSTH_ON_MASK),
                             (U32)(PMIC_VRF18_2_BURSTH_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_bursth(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON16),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_BURSTH_MASK),
                             (U32)(PMIC_VRF18_2_BURSTH_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf18_2_burstl(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON17),
                           (&val),
                           (U32)(PMIC_QI_VRF18_2_BURSTL_MASK),
                           (U32)(PMIC_QI_VRF18_2_BURSTL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_2_burstl_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON17),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_BURSTL_SLEEP_MASK),
                             (U32)(PMIC_VRF18_2_BURSTL_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_burstl_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON17),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_BURSTL_ON_MASK),
                             (U32)(PMIC_VRF18_2_BURSTL_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_burstl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON17),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_BURSTL_MASK),
                             (U32)(PMIC_VRF18_2_BURSTL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vrf18_2_vsleep_sel(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON18),
                           (&val),
                           (U32)(PMIC_NI_VRF18_2_VSLEEP_SEL_MASK),
                           (U32)(PMIC_NI_VRF18_2_VSLEEP_SEL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_ni_vrf18_2_r2r_pdn(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON18),
                           (&val),
                           (U32)(PMIC_NI_VRF18_2_R2R_PDN_MASK),
                           (U32)(PMIC_NI_VRF18_2_R2R_PDN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_2_vsleep_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_VSLEEP_SEL_MASK),
                             (U32)(PMIC_VRF18_2_VSLEEP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_r2r_pdn(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_R2R_PDN_MASK),
                             (U32)(PMIC_VRF18_2_R2R_PDN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_vsleep_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_VSLEEP_EN_MASK),
                             (U32)(PMIC_VRF18_2_VSLEEP_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_ni_vrf18_2_vosel_trans(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(VRF18_2_CON18),
                           (&val),
                           (U32)(PMIC_NI_VRF18_2_VOSEL_TRANS_MASK),
                           (U32)(PMIC_NI_VRF18_2_VOSEL_TRANS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf18_2_vosel_trans_once(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_VOSEL_TRANS_ONCE_MASK),
                             (U32)(PMIC_VRF18_2_VOSEL_TRANS_ONCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_vosel_trans_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_VOSEL_TRANS_EN_MASK),
                             (U32)(PMIC_VRF18_2_VOSEL_TRANS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf18_2_transtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(VRF18_2_CON18),
                             (U32)(val),
                             (U32)(PMIC_VRF18_2_TRANSTD_MASK),
                             (U32)(PMIC_VRF18_2_TRANSTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_k_control_smps(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_K_CON0),
                             (U32)(val),
                             (U32)(PMIC_K_CONTROL_SMPS_MASK),
                             (U32)(PMIC_K_CONTROL_SMPS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_k_auto_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_K_CON0),
                             (U32)(val),
                             (U32)(PMIC_K_AUTO_EN_MASK),
                             (U32)(PMIC_K_AUTO_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_k_src_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_K_CON0),
                             (U32)(val),
                             (U32)(PMIC_K_SRC_SEL_MASK),
                             (U32)(PMIC_K_SRC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_k_start_manual(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_K_CON0),
                             (U32)(val),
                             (U32)(PMIC_K_START_MANUAL_MASK),
                             (U32)(PMIC_K_START_MANUAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_k_once(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_K_CON0),
                             (U32)(val),
                             (U32)(PMIC_K_ONCE_MASK),
                             (U32)(PMIC_K_ONCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_k_once_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_K_CON0),
                             (U32)(val),
                             (U32)(PMIC_K_ONCE_EN_MASK),
                             (U32)(PMIC_K_ONCE_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_k_map_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_K_CON0),
                             (U32)(val),
                             (U32)(PMIC_K_MAP_SEL_MASK),
                             (U32)(PMIC_K_MAP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_k_rst_done(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(BUCK_K_CON0),
                             (U32)(val),
                             (U32)(PMIC_K_RST_DONE_MASK),
                             (U32)(PMIC_K_RST_DONE_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_smps_osc_cal(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(BUCK_K_CON1),
                           (&val),
                           (U32)(PMIC_QI_SMPS_OSC_CAL_MASK),
                           (U32)(PMIC_QI_SMPS_OSC_CAL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_k_control(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(BUCK_K_CON1),
                           (&val),
                           (U32)(PMIC_K_CONTROL_MASK),
                           (U32)(PMIC_K_CONTROL_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_k_done(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(BUCK_K_CON1),
                           (&val),
                           (U32)(PMIC_K_DONE_MASK),
                           (U32)(PMIC_K_DONE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_k_result(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(BUCK_K_CON1),
                           (&val),
                           (U32)(PMIC_K_RESULT_MASK),
                           (U32)(PMIC_K_RESULT_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vrf28_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON0),
                           (&val),
                           (U32)(PMIC_QI_VRF28_EN_MASK),
                           (U32)(PMIC_QI_VRF28_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf28_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON0),
                             (U32)(val),
                             (U32)(PMIC_VRF28_ON_CTRL_MASK),
                             (U32)(PMIC_VRF28_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_EN_MASK),
                             (U32)(PMIC_RG_VRF28_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_STBTD_MASK),
                             (U32)(PMIC_RG_VRF28_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf28_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON0),
                             (U32)(val),
                             (U32)(PMIC_VRF28_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VRF28_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vtcxo_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON1),
                           (&val),
                           (U32)(PMIC_QI_VTCXO_EN_MASK),
                           (U32)(PMIC_QI_VTCXO_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vtcxo_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON1),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VTCXO_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vtcxo_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON1),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_ON_CTRL_MASK),
                             (U32)(PMIC_VTCXO_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_EN_MASK),
                             (U32)(PMIC_RG_VTCXO_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_STBTD_MASK),
                             (U32)(PMIC_RG_VTCXO_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vtcxo_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON1),
                           (&val),
                           (U32)(PMIC_QI_VTCXO_MODE_MASK),
                           (U32)(PMIC_QI_VTCXO_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vtcxo_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON1),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VTCXO_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vtcxo_lp_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON1),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_LP_SET_MASK),
                             (U32)(PMIC_VTCXO_LP_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vtcxo_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON1),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_LP_SEL_MASK),
                             (U32)(PMIC_VTCXO_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_va_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON2),
                           (&val),
                           (U32)(PMIC_QI_VA_EN_MASK),
                           (U32)(PMIC_QI_VA_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_va_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VA_EN_MASK),
                             (U32)(PMIC_RG_VA_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VA_STBTD_MASK),
                             (U32)(PMIC_RG_VA_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_va_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON2),
                           (&val),
                           (U32)(PMIC_QI_VA_MODE_MASK),
                           (U32)(PMIC_QI_VA_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_va_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_VA_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VA_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_va_lp_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_VA_LP_SET_MASK),
                             (U32)(PMIC_VA_LP_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_va_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_VA_LP_SEL_MASK),
                             (U32)(PMIC_VA_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_va28_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON3),
                           (&val),
                           (U32)(PMIC_QI_VA28_EN_MASK),
                           (U32)(PMIC_QI_VA28_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_va28_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VA28_EN_MASK),
                             (U32)(PMIC_RG_VA28_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va28_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VA28_STBTD_MASK),
                             (U32)(PMIC_RG_VA28_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_va28_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON3),
                           (&val),
                           (U32)(PMIC_QI_VA28_MODE_MASK),
                           (U32)(PMIC_QI_VA28_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_va28_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_VA28_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VA28_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_va28_lp_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_VA28_LP_SET_MASK),
                             (U32)(PMIC_VA28_LP_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_va28_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_VA28_LP_SEL_MASK),
                             (U32)(PMIC_VA28_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcama_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VCAMA_EN_MASK),
                             (U32)(PMIC_RG_VCAMA_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcama_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_VCAMA_STBTD_MASK),
                             (U32)(PMIC_RG_VCAMA_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf28_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON5),
                           (&val),
                           (U32)(PMIC_QI_VRF28_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VRF28_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vtcxo_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON5),
                           (&val),
                           (U32)(PMIC_QI_VTCXO_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VTCXO_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_va_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON5),
                           (&val),
                           (U32)(PMIC_QI_VA_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VA_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_va28_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON5),
                           (&val),
                           (U32)(PMIC_QI_VA28_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VA28_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vcama_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON5),
                           (&val),
                           (U32)(PMIC_QI_VCAMA_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VCAMA_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_vrf28_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_BIST_EN_MASK),
                             (U32)(PMIC_RG_VRF28_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_2_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_2_BIST_EN_MASK),
                             (U32)(PMIC_RG_VRF28_2_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_BIST_EN_MASK),
                             (U32)(PMIC_RG_VTCXO_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_2_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_2_BIST_EN_MASK),
                             (U32)(PMIC_RG_VTCXO_2_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_VA_BIST_EN_MASK),
                             (U32)(PMIC_RG_VA_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va28_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_VA28_BIST_EN_MASK),
                             (U32)(PMIC_RG_VA28_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcama_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_VCAMA_BIST_EN_MASK),
                             (U32)(PMIC_RG_VCAMA_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_CAL_MASK),
                             (U32)(PMIC_RG_VRF28_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_OCFB_MASK),
                             (U32)(PMIC_RG_VRF28_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VRF28_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_CAL_MASK),
                             (U32)(PMIC_RG_VTCXO_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_OCFB_MASK),
                             (U32)(PMIC_RG_VTCXO_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VTCXO_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_VA_CAL_MASK),
                             (U32)(PMIC_RG_VA_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_VA_VOSEL_MASK),
                             (U32)(PMIC_RG_VA_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_VA_OCFB_MASK),
                             (U32)(PMIC_RG_VA_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_VA_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VA_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va28_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_VA28_CAL_MASK),
                             (U32)(PMIC_RG_VA28_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va28_ocfb_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_VA28_OCFB_EN_MASK),
                             (U32)(PMIC_RG_VA28_OCFB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va28_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_VA28_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VA28_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_VOSEL_MASK),
                             (U32)(PMIC_RG_VRF28_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcama_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_VCAMA_CAL_MASK),
                             (U32)(PMIC_RG_VCAMA_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcama_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_VCAMA_VOSEL_MASK),
                             (U32)(PMIC_RG_VCAMA_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcama_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_VCAMA_OCFB_MASK),
                             (U32)(PMIC_RG_VCAMA_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcama_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_VCAMA_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VCAMA_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vcama_fbsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_VCAMA_FBSEL_MASK),
                             (U32)(PMIC_RG_VCAMA_FBSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_2_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_2_CAL_MASK),
                             (U32)(PMIC_RG_VTCXO_2_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_2_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_2_OCFB_MASK),
                             (U32)(PMIC_RG_VTCXO_2_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_2_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_2_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VTCXO_2_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_2_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_2_VOSEL_MASK),
                             (U32)(PMIC_RG_VTCXO_2_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_2_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_2_CAL_MASK),
                             (U32)(PMIC_RG_VRF28_2_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_2_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_2_OCFB_MASK),
                             (U32)(PMIC_RG_VRF28_2_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_2_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_2_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VRF28_2_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_2_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_2_VOSEL_MASK),
                             (U32)(PMIC_RG_VRF28_2_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrf28_2_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON13),
                           (&val),
                           (U32)(PMIC_QI_VRF28_2_EN_MASK),
                           (U32)(PMIC_QI_VRF28_2_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrf28_on_2_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON13),
                             (U32)(val),
                             (U32)(PMIC_VRF28_ON_2_CTRL_MASK),
                             (U32)(PMIC_VRF28_ON_2_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_2_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON13),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_2_EN_MASK),
                             (U32)(PMIC_RG_VRF28_2_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrf28_2_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON13),
                             (U32)(val),
                             (U32)(PMIC_RG_VRF28_2_STBTD_MASK),
                             (U32)(PMIC_RG_VRF28_2_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vrf28_2_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON13),
                             (U32)(val),
                             (U32)(PMIC_VRF28_2_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VRF28_2_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vtcxo_2_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON14),
                           (&val),
                           (U32)(PMIC_QI_VTCXO_2_EN_MASK),
                           (U32)(PMIC_QI_VTCXO_2_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vtcxo_2_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_2_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VTCXO_2_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vtcxo_2_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_2_ON_CTRL_MASK),
                             (U32)(PMIC_VTCXO_2_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_2_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_2_EN_MASK),
                             (U32)(PMIC_RG_VTCXO_2_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vtcxo_2_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_RG_VTCXO_2_STBTD_MASK),
                             (U32)(PMIC_RG_VTCXO_2_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vtcxo_2_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ANALDO_CON14),
                           (&val),
                           (U32)(PMIC_QI_VTCXO_2_MODE_MASK),
                           (U32)(PMIC_QI_VTCXO_2_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vtcxo_2_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_2_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VTCXO_2_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vtcxo_2_lp_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_2_LP_SET_MASK),
                             (U32)(PMIC_VTCXO_2_LP_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vtcxo_2_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_2_LP_SEL_MASK),
                             (U32)(PMIC_VTCXO_2_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_aldo_reserve_1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON15),
                             (U32)(val),
                             (U32)(PMIC_RG_ALDO_RESERVE_1_MASK),
                             (U32)(PMIC_RG_ALDO_RESERVE_1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_aldo_reserve_2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON15),
                             (U32)(val),
                             (U32)(PMIC_RG_ALDO_RESERVE_2_MASK),
                             (U32)(PMIC_RG_ALDO_RESERVE_2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_analdo_rsv0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON15),
                             (U32)(val),
                             (U32)(PMIC_ANALDO_RSV0_MASK),
                             (U32)(PMIC_ANALDO_RSV0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_analdo_rsv1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ANALDO_CON15),
                             (U32)(val),
                             (U32)(PMIC_ANALDO_RSV1_MASK),
                             (U32)(PMIC_ANALDO_RSV1_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vio28_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON0),
                           (&val),
                           (U32)(PMIC_QI_VIO28_EN_MASK),
                           (U32)(PMIC_QI_VIO28_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vio28_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON0),
                             (U32)(val),
                             (U32)(PMIC_VIO28_EN_MASK),
                             (U32)(PMIC_VIO28_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio28_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO28_STBTD_MASK),
                             (U32)(PMIC_RG_VIO28_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vio28_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON0),
                           (&val),
                           (U32)(PMIC_QI_VIO28_MODE_MASK),
                           (U32)(PMIC_QI_VIO28_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vio28_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON0),
                             (U32)(val),
                             (U32)(PMIC_VIO28_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VIO28_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio28_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON0),
                             (U32)(val),
                             (U32)(PMIC_VIO28_LP_MODE_SET_MASK),
                             (U32)(PMIC_VIO28_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio28_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON0),
                             (U32)(val),
                             (U32)(PMIC_VIO28_LP_SEL_MASK),
                             (U32)(PMIC_VIO28_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vusb_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON2),
                           (&val),
                           (U32)(PMIC_QI_VUSB_EN_MASK),
                           (U32)(PMIC_QI_VUSB_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_vusb_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VUSB_EN_MASK),
                             (U32)(PMIC_RG_VUSB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vusb_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VUSB_STBTD_MASK),
                             (U32)(PMIC_RG_VUSB_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vusb_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON2),
                           (&val),
                           (U32)(PMIC_QI_VUSB_MODE_MASK),
                           (U32)(PMIC_QI_VUSB_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vusb_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_VUSB_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VUSB_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vusb_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_VUSB_LP_MODE_SET_MASK),
                             (U32)(PMIC_VUSB_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vusb_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON2),
                             (U32)(val),
                             (U32)(PMIC_VUSB_LP_SEL_MASK),
                             (U32)(PMIC_VUSB_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vmc1_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON3),
                           (&val),
                           (U32)(PMIC_QI_VMC1_EN_MASK),
                           (U32)(PMIC_QI_VMC1_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_vmc1_int_dis_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VMC1_INT_DIS_SEL_MASK),
                             (U32)(PMIC_RG_VMC1_INT_DIS_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmc1_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VMC1_EN_MASK),
                             (U32)(PMIC_RG_VMC1_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmc1_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_VMC1_STBTD_MASK),
                             (U32)(PMIC_RG_VMC1_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vmc1_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON3),
                           (&val),
                           (U32)(PMIC_QI_VMC1_MODE_MASK),
                           (U32)(PMIC_QI_VMC1_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vmc1_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_VMC1_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VMC1_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vmc1_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_VMC1_LP_MODE_SET_MASK),
                             (U32)(PMIC_VMC1_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vmc1_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON3),
                             (U32)(val),
                             (U32)(PMIC_VMC1_LP_SEL_MASK),
                             (U32)(PMIC_VMC1_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vmch1_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON5),
                           (&val),
                           (U32)(PMIC_QI_VMCH1_EN_MASK),
                           (U32)(PMIC_QI_VMCH1_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_vmch1_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_VMCH1_EN_MASK),
                             (U32)(PMIC_RG_VMCH1_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmch1_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_VMCH1_STBTD_MASK),
                             (U32)(PMIC_RG_VMCH1_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vmch1_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON5),
                           (&val),
                           (U32)(PMIC_QI_VMCH1_MODE_MASK),
                           (U32)(PMIC_QI_VMCH1_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vmch1_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_VMCH1_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VMCH1_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vmch1_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_VMCH1_LP_MODE_SET_MASK),
                             (U32)(PMIC_VMCH1_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vmch1_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON5),
                             (U32)(val),
                             (U32)(PMIC_VMCH1_LP_SEL_MASK),
                             (U32)(PMIC_VMCH1_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vemc_3v3_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON6),
                           (&val),
                           (U32)(PMIC_QI_VEMC_3V3_EN_MASK),
                           (U32)(PMIC_QI_VEMC_3V3_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_vemc_3v3_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_3V3_EN_MASK),
                             (U32)(PMIC_RG_VEMC_3V3_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_3v3_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_3V3_STBTD_MASK),
                             (U32)(PMIC_RG_VEMC_3V3_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vemc_3v3_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON6),
                           (&val),
                           (U32)(PMIC_QI_VEMC_3V3_MODE_MASK),
                           (U32)(PMIC_QI_VEMC_3V3_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vemc_3v3_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON6),
                             (U32)(val),
                             (U32)(PMIC_VEMC_3V3_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VEMC_3V3_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vemc_3v3_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON6),
                             (U32)(val),
                             (U32)(PMIC_VEMC_3V3_LP_MODE_SET_MASK),
                             (U32)(PMIC_VEMC_3V3_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vemc_3v3_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON6),
                             (U32)(val),
                             (U32)(PMIC_VEMC_3V3_LP_SEL_MASK),
                             (U32)(PMIC_VEMC_3V3_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp1_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP1_EN_MASK),
                             (U32)(PMIC_RG_VGP1_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp1_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP1_STBTD_MASK),
                             (U32)(PMIC_RG_VGP1_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vgp1_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON7),
                           (&val),
                           (U32)(PMIC_QI_VGP1_MODE_MASK),
                           (U32)(PMIC_QI_VGP1_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vgp1_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON7),
                             (U32)(val),
                             (U32)(PMIC_VGP1_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VGP1_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp1_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON7),
                             (U32)(val),
                             (U32)(PMIC_VGP1_LP_MODE_SET_MASK),
                             (U32)(PMIC_VGP1_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp1_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON7),
                             (U32)(val),
                             (U32)(PMIC_VGP1_LP_SEL_MASK),
                             (U32)(PMIC_VGP1_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp2_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP2_EN_MASK),
                             (U32)(PMIC_RG_VGP2_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp2_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP2_STBTD_MASK),
                             (U32)(PMIC_RG_VGP2_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vgp2_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON8),
                           (&val),
                           (U32)(PMIC_QI_VGP2_MODE_MASK),
                           (U32)(PMIC_QI_VGP2_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vgp2_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON8),
                             (U32)(val),
                             (U32)(PMIC_VGP2_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VGP2_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp2_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON8),
                             (U32)(val),
                             (U32)(PMIC_VGP2_LP_MODE_SET_MASK),
                             (U32)(PMIC_VGP2_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp2_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON8),
                             (U32)(val),
                             (U32)(PMIC_VGP2_LP_SEL_MASK),
                             (U32)(PMIC_VGP2_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp3_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP3_EN_MASK),
                             (U32)(PMIC_RG_VGP3_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp3_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP3_STBTD_MASK),
                             (U32)(PMIC_RG_VGP3_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vgp3_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON9),
                           (&val),
                           (U32)(PMIC_QI_VGP3_MODE_MASK),
                           (U32)(PMIC_QI_VGP3_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vgp3_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON9),
                             (U32)(val),
                             (U32)(PMIC_VGP3_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VGP3_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp3_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON9),
                             (U32)(val),
                             (U32)(PMIC_VGP3_LP_MODE_SET_MASK),
                             (U32)(PMIC_VGP3_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp3_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON9),
                             (U32)(val),
                             (U32)(PMIC_VGP3_LP_SEL_MASK),
                             (U32)(PMIC_VGP3_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp4_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP4_EN_MASK),
                             (U32)(PMIC_RG_VGP4_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp4_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP4_STBTD_MASK),
                             (U32)(PMIC_RG_VGP4_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vgp4_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON10),
                           (&val),
                           (U32)(PMIC_QI_VGP4_MODE_MASK),
                           (U32)(PMIC_QI_VGP4_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vgp4_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_VGP4_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VGP4_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp4_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_VGP4_LP_MODE_SET_MASK),
                             (U32)(PMIC_VGP4_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp4_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON10),
                             (U32)(val),
                             (U32)(PMIC_VGP4_LP_SEL_MASK),
                             (U32)(PMIC_VGP4_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp5_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP5_EN_MASK),
                             (U32)(PMIC_RG_VGP5_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp5_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP5_STBTD_MASK),
                             (U32)(PMIC_RG_VGP5_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vgp5_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON11),
                           (&val),
                           (U32)(PMIC_QI_VGP5_MODE_MASK),
                           (U32)(PMIC_QI_VGP5_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vgp5_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON11),
                             (U32)(val),
                             (U32)(PMIC_VGP5_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VGP5_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp5_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON11),
                             (U32)(val),
                             (U32)(PMIC_VGP5_LP_MODE_SET_MASK),
                             (U32)(PMIC_VGP5_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp5_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON11),
                             (U32)(val),
                             (U32)(PMIC_VGP5_LP_SEL_MASK),
                             (U32)(PMIC_VGP5_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp6_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP6_EN_MASK),
                             (U32)(PMIC_RG_VGP6_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp6_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP6_STBTD_MASK),
                             (U32)(PMIC_RG_VGP6_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vgp6_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON12),
                           (&val),
                           (U32)(PMIC_QI_VGP6_MODE_MASK),
                           (U32)(PMIC_QI_VGP6_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vgp6_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON12),
                             (U32)(val),
                             (U32)(PMIC_VGP6_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VGP6_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp6_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON12),
                             (U32)(val),
                             (U32)(PMIC_VGP6_LP_MODE_SET_MASK),
                             (U32)(PMIC_VGP6_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp6_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON12),
                             (U32)(val),
                             (U32)(PMIC_VGP6_LP_SEL_MASK),
                             (U32)(PMIC_VGP6_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim1_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON13),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM1_EN_MASK),
                             (U32)(PMIC_RG_VSIM1_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim1_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON13),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM1_STBTD_MASK),
                             (U32)(PMIC_RG_VSIM1_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vsim1_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON13),
                           (&val),
                           (U32)(PMIC_QI_VSIM1_MODE_MASK),
                           (U32)(PMIC_QI_VSIM1_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vsim1_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON13),
                             (U32)(val),
                             (U32)(PMIC_VSIM1_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VSIM1_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsim1_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON13),
                             (U32)(val),
                             (U32)(PMIC_VSIM1_LP_MODE_SET_MASK),
                             (U32)(PMIC_VSIM1_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsim1_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON13),
                             (U32)(val),
                             (U32)(PMIC_VSIM1_LP_SEL_MASK),
                             (U32)(PMIC_VSIM1_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim2_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM2_EN_MASK),
                             (U32)(PMIC_RG_VSIM2_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim2_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM2_STBTD_MASK),
                             (U32)(PMIC_RG_VSIM2_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vsim2_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON14),
                           (&val),
                           (U32)(PMIC_QI_VSIM2_MODE_MASK),
                           (U32)(PMIC_QI_VSIM2_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vsim2_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_VSIM2_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VSIM2_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsim2_ther_shdn_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_VSIM2_THER_SHDN_EN_MASK),
                             (U32)(PMIC_VSIM2_THER_SHDN_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsim2_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_VSIM2_LP_MODE_SET_MASK),
                             (U32)(PMIC_VSIM2_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsim2_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON14),
                             (U32)(val),
                             (U32)(PMIC_VSIM2_LP_SEL_MASK),
                             (U32)(PMIC_VSIM2_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vrtc_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON15),
                           (&val),
                           (U32)(PMIC_QI_VRTC_EN_MASK),
                           (U32)(PMIC_QI_VRTC_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vrtc_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON15),
                             (U32)(val),
                             (U32)(PMIC_VRTC_EN_MASK),
                             (U32)(PMIC_VRTC_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio28_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON16),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO28_BIST_EN_MASK),
                             (U32)(PMIC_RG_VIO28_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vusb_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON16),
                             (U32)(val),
                             (U32)(PMIC_RG_VUSB_BIST_EN_MASK),
                             (U32)(PMIC_RG_VUSB_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmc1_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON16),
                             (U32)(val),
                             (U32)(PMIC_RG_VMC1_BIST_EN_MASK),
                             (U32)(PMIC_RG_VMC1_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmch1_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON16),
                             (U32)(val),
                             (U32)(PMIC_RG_VMCH1_BIST_EN_MASK),
                             (U32)(PMIC_RG_VMCH1_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_3v3_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON16),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_3V3_BIST_EN_MASK),
                             (U32)(PMIC_RG_VEMC_3V3_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp1_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP1_BIST_EN_MASK),
                             (U32)(PMIC_RG_VGP1_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp2_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP2_BIST_EN_MASK),
                             (U32)(PMIC_RG_VGP2_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp3_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP3_BIST_EN_MASK),
                             (U32)(PMIC_RG_VGP3_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp4_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP4_BIST_EN_MASK),
                             (U32)(PMIC_RG_VGP4_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp5_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP5_BIST_EN_MASK),
                             (U32)(PMIC_RG_VGP5_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp6_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP6_BIST_EN_MASK),
                             (U32)(PMIC_RG_VGP6_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vibr_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VIBR_BIST_EN_MASK),
                             (U32)(PMIC_RG_VIBR_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim1_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM1_BIST_EN_MASK),
                             (U32)(PMIC_RG_VSIM1_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim2_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM2_BIST_EN_MASK),
                             (U32)(PMIC_RG_VSIM2_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_1v8_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_1V8_BIST_EN_MASK),
                             (U32)(PMIC_RG_VEMC_1V8_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vrtc_bist_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON17),
                             (U32)(val),
                             (U32)(PMIC_RG_VRTC_BIST_EN_MASK),
                             (U32)(PMIC_RG_VRTC_BIST_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vio28_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON18),
                           (&val),
                           (U32)(PMIC_QI_VIO28_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VIO28_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vusb_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON18),
                           (&val),
                           (U32)(PMIC_QI_VUSB_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VUSB_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vmc1_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON18),
                           (&val),
                           (U32)(PMIC_QI_VMC1_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VMC1_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vmch1_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON18),
                           (&val),
                           (U32)(PMIC_QI_VMCH1_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VMCH1_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vemc_3v3_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON18),
                           (&val),
                           (U32)(PMIC_QI_VEMC_3V3_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VEMC_3V3_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vgp1_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VGP1_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VGP1_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vgp2_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VGP2_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VGP2_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vgp3_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VGP3_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VGP3_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vgp4_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VGP4_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VGP4_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vgp5_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VGP5_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VGP5_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vgp6_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VGP6_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VGP6_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vibr_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VIBR_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VIBR_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vsim1_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VSIM1_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VSIM1_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vsim2_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VSIM2_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VSIM2_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vemc_1v8_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VEMC_1V8_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VEMC_1V8_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vast_oc_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON19),
                           (&val),
                           (U32)(PMIC_QI_VAST_OC_STATUS_MASK),
                           (U32)(PMIC_QI_VAST_OC_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_qi_vast_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON20),
                           (&val),
                           (U32)(PMIC_QI_VAST_EN_MASK),
                           (U32)(PMIC_QI_VAST_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_vast_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON20),
                             (U32)(val),
                             (U32)(PMIC_RG_VAST_VOSEL_MASK),
                             (U32)(PMIC_RG_VAST_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vast_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON20),
                             (U32)(val),
                             (U32)(PMIC_RG_VAST_EN_MASK),
                             (U32)(PMIC_RG_VAST_EN_SHIFT)
	                         );
  pmic_unlock();
}
EXPORT_SYMBOL(upmu_set_rg_vast_en);

void upmu_set_rg_vast_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON20),
                             (U32)(val),
                             (U32)(PMIC_RG_VAST_STBTD_MASK),
                             (U32)(PMIC_RG_VAST_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vast_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON20),
                           (&val),
                           (U32)(PMIC_QI_VAST_MODE_MASK),
                           (U32)(PMIC_QI_VAST_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vast_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON20),
                             (U32)(val),
                             (U32)(PMIC_VAST_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VAST_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vast_sleep(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON20),
                             (U32)(val),
                             (U32)(PMIC_RG_VAST_SLEEP_MASK),
                             (U32)(PMIC_RG_VAST_SLEEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vast_dis_srclken(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON20),
                             (U32)(val),
                             (U32)(PMIC_RG_VAST_DIS_SRCLKEN_MASK),
                             (U32)(PMIC_RG_VAST_DIS_SRCLKEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vast_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON20),
                             (U32)(val),
                             (U32)(PMIC_VAST_LP_MODE_SET_MASK),
                             (U32)(PMIC_VAST_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vast_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON20),
                             (U32)(val),
                             (U32)(PMIC_VAST_LP_SEL_MASK),
                             (U32)(PMIC_VAST_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio28_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON21),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO28_CAL_MASK),
                             (U32)(PMIC_RG_VIO28_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio28_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON21),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO28_OCFB_MASK),
                             (U32)(PMIC_RG_VIO28_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vio28_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON21),
                             (U32)(val),
                             (U32)(PMIC_RG_VIO28_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VIO28_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vusb_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON23),
                             (U32)(val),
                             (U32)(PMIC_RG_VUSB_CAL_MASK),
                             (U32)(PMIC_RG_VUSB_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vusb_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON23),
                             (U32)(val),
                             (U32)(PMIC_RG_VUSB_OCFB_MASK),
                             (U32)(PMIC_RG_VUSB_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vusb_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON23),
                             (U32)(val),
                             (U32)(PMIC_RG_VUSB_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VUSB_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmc1_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON24),
                             (U32)(val),
                             (U32)(PMIC_RG_VMC1_CAL_MASK),
                             (U32)(PMIC_RG_VMC1_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmc1_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON24),
                             (U32)(val),
                             (U32)(PMIC_RG_VMC1_STB_SEL_MASK),
                             (U32)(PMIC_RG_VMC1_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmc1_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON24),
                             (U32)(val),
                             (U32)(PMIC_RG_VMC1_VOSEL_MASK),
                             (U32)(PMIC_RG_VMC1_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmc1_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON24),
                             (U32)(val),
                             (U32)(PMIC_RG_VMC1_OCFB_MASK),
                             (U32)(PMIC_RG_VMC1_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmc1_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON24),
                             (U32)(val),
                             (U32)(PMIC_RG_VMC1_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VMC1_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmch1_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON26),
                             (U32)(val),
                             (U32)(PMIC_RG_VMCH1_CAL_MASK),
                             (U32)(PMIC_RG_VMCH1_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmch1_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON26),
                             (U32)(val),
                             (U32)(PMIC_RG_VMCH1_VOSEL_MASK),
                             (U32)(PMIC_RG_VMCH1_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmch1_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON26),
                             (U32)(val),
                             (U32)(PMIC_RG_VMCH1_STB_SEL_MASK),
                             (U32)(PMIC_RG_VMCH1_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmch1_db_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON26),
                             (U32)(val),
                             (U32)(PMIC_RG_VMCH1_DB_EN_MASK),
                             (U32)(PMIC_RG_VMCH1_DB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmch1_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON26),
                             (U32)(val),
                             (U32)(PMIC_RG_VMCH1_OCFB_MASK),
                             (U32)(PMIC_RG_VMCH1_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vmch1_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON26),
                             (U32)(val),
                             (U32)(PMIC_RG_VMCH1_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VMCH1_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_3v3_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON27),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_3V3_CAL_MASK),
                             (U32)(PMIC_RG_VEMC_3V3_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_3v3_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON27),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_3V3_VOSEL_MASK),
                             (U32)(PMIC_RG_VEMC_3V3_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_3v3_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON27),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_3V3_STB_SEL_MASK),
                             (U32)(PMIC_RG_VEMC_3V3_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_3v3_db_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON27),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_3V3_DB_EN_MASK),
                             (U32)(PMIC_RG_VEMC_3V3_DB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_3v3_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON27),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_3V3_OCFB_MASK),
                             (U32)(PMIC_RG_VEMC_3V3_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_3v3_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON27),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_3V3_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VEMC_3V3_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp1_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON28),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP1_CAL_MASK),
                             (U32)(PMIC_RG_VGP1_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp1_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON28),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP1_VOSEL_MASK),
                             (U32)(PMIC_RG_VGP1_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp1_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON28),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP1_STB_SEL_MASK),
                             (U32)(PMIC_RG_VGP1_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp1_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON28),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP1_OCFB_MASK),
                             (U32)(PMIC_RG_VGP1_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp1_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON28),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP1_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VGP1_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp2_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON29),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP2_CAL_MASK),
                             (U32)(PMIC_RG_VGP2_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp2_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON29),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP2_VOSEL_MASK),
                             (U32)(PMIC_RG_VGP2_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp2_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON29),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP2_STB_SEL_MASK),
                             (U32)(PMIC_RG_VGP2_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp2_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON29),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP2_OCFB_MASK),
                             (U32)(PMIC_RG_VGP2_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp2_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON29),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP2_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VGP2_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp3_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON30),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP3_CAL_MASK),
                             (U32)(PMIC_RG_VGP3_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp3_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON30),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP3_VOSEL_MASK),
                             (U32)(PMIC_RG_VGP3_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp3_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON30),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP3_STB_SEL_MASK),
                             (U32)(PMIC_RG_VGP3_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp3_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON30),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP3_OCFB_MASK),
                             (U32)(PMIC_RG_VGP3_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp3_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON30),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP3_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VGP3_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp4_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON31),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP4_CAL_MASK),
                             (U32)(PMIC_RG_VGP4_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp4_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON31),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP4_VOSEL_MASK),
                             (U32)(PMIC_RG_VGP4_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp4_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON31),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP4_STB_SEL_MASK),
                             (U32)(PMIC_RG_VGP4_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp4_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON31),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP4_OCFB_MASK),
                             (U32)(PMIC_RG_VGP4_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp4_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON31),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP4_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VGP4_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp5_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON32),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP5_CAL_MASK),
                             (U32)(PMIC_RG_VGP5_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp5_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON32),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP5_VOSEL_MASK),
                             (U32)(PMIC_RG_VGP5_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp5_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON32),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP5_STB_SEL_MASK),
                             (U32)(PMIC_RG_VGP5_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp5_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON32),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP5_OCFB_MASK),
                             (U32)(PMIC_RG_VGP5_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp5_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON32),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP5_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VGP5_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp6_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON33),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP6_CAL_MASK),
                             (U32)(PMIC_RG_VGP6_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp6_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON33),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP6_VOSEL_MASK),
                             (U32)(PMIC_RG_VGP6_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp6_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON33),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP6_STB_SEL_MASK),
                             (U32)(PMIC_RG_VGP6_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp6_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON33),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP6_OCFB_MASK),
                             (U32)(PMIC_RG_VGP6_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vgp6_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON33),
                             (U32)(val),
                             (U32)(PMIC_RG_VGP6_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VGP6_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim1_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON34),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM1_CAL_MASK),
                             (U32)(PMIC_RG_VSIM1_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim1_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON34),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM1_VOSEL_MASK),
                             (U32)(PMIC_RG_VSIM1_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim1_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON34),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM1_STB_SEL_MASK),
                             (U32)(PMIC_RG_VSIM1_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim1_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON34),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM1_OCFB_MASK),
                             (U32)(PMIC_RG_VSIM1_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim1_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON34),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM1_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VSIM1_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim2_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON35),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM2_CAL_MASK),
                             (U32)(PMIC_RG_VSIM2_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim2_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON35),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM2_VOSEL_MASK),
                             (U32)(PMIC_RG_VSIM2_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim2_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON35),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM2_STB_SEL_MASK),
                             (U32)(PMIC_RG_VSIM2_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim2_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON35),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM2_OCFB_MASK),
                             (U32)(PMIC_RG_VSIM2_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vsim2_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON35),
                             (U32)(val),
                             (U32)(PMIC_RG_VSIM2_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VSIM2_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vast_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON36),
                             (U32)(val),
                             (U32)(PMIC_RG_VAST_CAL_MASK),
                             (U32)(PMIC_RG_VAST_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vast_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON36),
                             (U32)(val),
                             (U32)(PMIC_RG_VAST_OCFB_MASK),
                             (U32)(PMIC_RG_VAST_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vast_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON36),
                             (U32)(val),
                             (U32)(PMIC_RG_VAST_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VAST_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vemc_1v8_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON37),
                           (&val),
                           (U32)(PMIC_QI_VEMC_1V8_EN_MASK),
                           (U32)(PMIC_QI_VEMC_1V8_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_vemc_1v8_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON37),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_1V8_EN_MASK),
                             (U32)(PMIC_RG_VEMC_1V8_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_1v8_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON37),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_1V8_STBTD_MASK),
                             (U32)(PMIC_RG_VEMC_1V8_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vemc_1v8_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON37),
                           (&val),
                           (U32)(PMIC_QI_VEMC_1V8_MODE_MASK),
                           (U32)(PMIC_QI_VEMC_1V8_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vemc_1v8_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON37),
                             (U32)(val),
                             (U32)(PMIC_VEMC_1V8_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VEMC_1V8_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vemc_1v8_ther_shdn_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON37),
                             (U32)(val),
                             (U32)(PMIC_VEMC_1V8_THER_SHDN_EN_MASK),
                             (U32)(PMIC_VEMC_1V8_THER_SHDN_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vemc_1v8_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON37),
                             (U32)(val),
                             (U32)(PMIC_VEMC_1V8_LP_MODE_SET_MASK),
                             (U32)(PMIC_VEMC_1V8_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vemc_1v8_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON37),
                             (U32)(val),
                             (U32)(PMIC_VEMC_1V8_LP_SEL_MASK),
                             (U32)(PMIC_VEMC_1V8_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_1v8_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON38),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_1V8_CAL_MASK),
                             (U32)(PMIC_RG_VEMC_1V8_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_1v8_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON38),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_1V8_VOSEL_MASK),
                             (U32)(PMIC_RG_VEMC_1V8_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_1v8_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON38),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_1V8_STB_SEL_MASK),
                             (U32)(PMIC_RG_VEMC_1V8_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_1v8_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON38),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_1V8_OCFB_MASK),
                             (U32)(PMIC_RG_VEMC_1V8_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vemc_1v8_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON38),
                             (U32)(val),
                             (U32)(PMIC_RG_VEMC_1V8_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VEMC_1V8_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vibr_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON39),
                             (U32)(val),
                             (U32)(PMIC_RG_VIBR_EN_MASK),
                             (U32)(PMIC_RG_VIBR_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vibr_stbtd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON39),
                             (U32)(val),
                             (U32)(PMIC_RG_VIBR_STBTD_MASK),
                             (U32)(PMIC_RG_VIBR_STBTD_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_vibr_mode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(DIGLDO_CON39),
                           (&val),
                           (U32)(PMIC_QI_VIBR_MODE_MASK),
                           (U32)(PMIC_QI_VIBR_MODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vibr_srclk_mode_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON39),
                             (U32)(val),
                             (U32)(PMIC_VIBR_SRCLK_MODE_SEL_MASK),
                             (U32)(PMIC_VIBR_SRCLK_MODE_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vibr_ther_shen_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON39),
                             (U32)(val),
                             (U32)(PMIC_VIBR_THER_SHEN_EN_MASK),
                             (U32)(PMIC_VIBR_THER_SHEN_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vibr_lp_mode_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON39),
                             (U32)(val),
                             (U32)(PMIC_VIBR_LP_MODE_SET_MASK),
                             (U32)(PMIC_VIBR_LP_MODE_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vibr_lp_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON39),
                             (U32)(val),
                             (U32)(PMIC_VIBR_LP_SEL_MASK),
                             (U32)(PMIC_VIBR_LP_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vibr_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON40),
                             (U32)(val),
                             (U32)(PMIC_RG_VIBR_CAL_MASK),
                             (U32)(PMIC_RG_VIBR_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vibr_vosel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON40),
                             (U32)(val),
                             (U32)(PMIC_RG_VIBR_VOSEL_MASK),
                             (U32)(PMIC_RG_VIBR_VOSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vibr_stb_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON40),
                             (U32)(val),
                             (U32)(PMIC_RG_VIBR_STB_SEL_MASK),
                             (U32)(PMIC_RG_VIBR_STB_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vibr_ocfb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON40),
                             (U32)(val),
                             (U32)(PMIC_RG_VIBR_OCFB_MASK),
                             (U32)(PMIC_RG_VIBR_OCFB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vibr_ndis_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON40),
                             (U32)(val),
                             (U32)(PMIC_RG_VIBR_NDIS_EN_MASK),
                             (U32)(PMIC_RG_VIBR_NDIS_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_ldo_ft(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON41),
                             (U32)(val),
                             (U32)(PMIC_RG_LDO_FT_MASK),
                             (U32)(PMIC_RG_LDO_FT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_digldo_rsv0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON41),
                             (U32)(val),
                             (U32)(PMIC_DIGLDO_RSV0_MASK),
                             (U32)(PMIC_DIGLDO_RSV0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_digldo_rsv1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON41),
                             (U32)(val),
                             (U32)(PMIC_DIGLDO_RSV1_MASK),
                             (U32)(PMIC_DIGLDO_RSV1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp1_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON42),
                             (U32)(val),
                             (U32)(PMIC_VGP1_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VGP1_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp2_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON42),
                             (U32)(val),
                             (U32)(PMIC_VGP2_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VGP2_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp3_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON42),
                             (U32)(val),
                             (U32)(PMIC_VGP3_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VGP3_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp4_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON42),
                             (U32)(val),
                             (U32)(PMIC_VGP4_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VGP4_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp6_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON43),
                             (U32)(val),
                             (U32)(PMIC_VGP6_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VGP6_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsim1_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON43),
                             (U32)(val),
                             (U32)(PMIC_VSIM1_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VSIM1_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsim2_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON43),
                             (U32)(val),
                             (U32)(PMIC_VSIM2_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VSIM2_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp5_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON43),
                             (U32)(val),
                             (U32)(PMIC_VGP5_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VGP5_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp1_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VGP1_ON_CTRL_MASK),
                             (U32)(PMIC_VGP1_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp2_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VGP2_ON_CTRL_MASK),
                             (U32)(PMIC_VGP2_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp3_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VGP3_ON_CTRL_MASK),
                             (U32)(PMIC_VGP3_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp4_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VGP4_ON_CTRL_MASK),
                             (U32)(PMIC_VGP4_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp5_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VGP5_ON_CTRL_MASK),
                             (U32)(PMIC_VGP5_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vgp6_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VGP6_ON_CTRL_MASK),
                             (U32)(PMIC_VGP6_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsim1_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VSIM1_ON_CTRL_MASK),
                             (U32)(PMIC_VSIM1_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsim2_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VSIM2_ON_CTRL_MASK),
                             (U32)(PMIC_VSIM2_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vibr_on_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VIBR_ON_CTRL_MASK),
                             (U32)(PMIC_VIBR_ON_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vibr_srclk_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(DIGLDO_CON44),
                             (U32)(val),
                             (U32)(PMIC_VIBR_SRCLK_EN_SEL_MASK),
                             (U32)(PMIC_VIBR_SRCLK_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_thrdet_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_THRDET_SEL_MASK),
                             (U32)(PMIC_RG_THRDET_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_thr_hwpdn_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON0),
                             (U32)(val),
                             (U32)(PMIC_THR_HWPDN_EN_MASK),
                             (U32)(PMIC_THR_HWPDN_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_strup_thr_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_STRUP_THR_SEL_MASK),
                             (U32)(PMIC_RG_STRUP_THR_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_thr_tmode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_THR_TMODE_MASK),
                             (U32)(PMIC_RG_THR_TMODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_thr_det_dis(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON0),
                             (U32)(val),
                             (U32)(PMIC_THR_DET_DIS_MASK),
                             (U32)(PMIC_THR_DET_DIS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vref_bg(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_VREF_BG_MASK),
                             (U32)(PMIC_RG_VREF_BG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_strup_iref_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_STRUP_IREF_TRIM_MASK),
                             (U32)(PMIC_RG_STRUP_IREF_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rst_drvsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_RST_DRVSEL_MASK),
                             (U32)(PMIC_RG_RST_DRVSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_en_drvsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_EN_DRVSEL_MASK),
                             (U32)(PMIC_RG_EN_DRVSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_usbdl_keydet_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_USBDL_KEYDET_EN_MASK),
                             (U32)(PMIC_RG_USBDL_KEYDET_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_usbdl_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_USBDL_EN_MASK),
                             (U32)(PMIC_RG_USBDL_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pmu_lev_ungate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_PMU_LEV_UNGATE_MASK),
                             (U32)(PMIC_RG_PMU_LEV_UNGATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pmu_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_PMU_RSV_MASK),
                             (U32)(PMIC_RG_PMU_RSV_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_pmu_thr_status(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STRUP_CON4),
                           (&val),
                           (U32)(PMIC_PMU_THR_STATUS_MASK),
                           (U32)(PMIC_PMU_THR_STATUS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_pmu_thr_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STRUP_CON4),
                           (&val),
                           (U32)(PMIC_PMU_THR_DEB_MASK),
                           (U32)(PMIC_PMU_THR_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_thr_test(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON4),
                             (U32)(val),
                             (U32)(PMIC_THR_TEST_MASK),
                             (U32)(PMIC_THR_TEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_bias_gen_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON5),
                             (U32)(val),
                             (U32)(PMIC_BIAS_GEN_EN_SEL_MASK),
                             (U32)(PMIC_BIAS_GEN_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_bias_gen_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON5),
                             (U32)(val),
                             (U32)(PMIC_BIAS_GEN_EN_MASK),
                             (U32)(PMIC_BIAS_GEN_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_pwron_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON5),
                             (U32)(val),
                             (U32)(PMIC_STRUP_PWRON_SEL_MASK),
                             (U32)(PMIC_STRUP_PWRON_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_pwron(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON5),
                             (U32)(val),
                             (U32)(PMIC_STRUP_PWRON_MASK),
                             (U32)(PMIC_STRUP_PWRON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_bias_gen_en_force(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON5),
                             (U32)(val),
                             (U32)(PMIC_BIAS_GEN_EN_FORCE_MASK),
                             (U32)(PMIC_BIAS_GEN_EN_FORCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_pwron_force(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON5),
                             (U32)(val),
                             (U32)(PMIC_STRUP_PWRON_FORCE_MASK),
                             (U32)(PMIC_STRUP_PWRON_FORCE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_ft_ctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON5),
                             (U32)(val),
                             (U32)(PMIC_STRUP_FT_CTRL_MASK),
                             (U32)(PMIC_STRUP_FT_CTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_pwrbb_deb_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON5),
                             (U32)(val),
                             (U32)(PMIC_PWRBB_DEB_EN_MASK),
                             (U32)(PMIC_PWRBB_DEB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_dduvlo_deb_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON5),
                             (U32)(val),
                             (U32)(PMIC_DDUVLO_DEB_EN_MASK),
                             (U32)(PMIC_DDUVLO_DEB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vtcxo_2_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_2_PG_ENB_MASK),
                             (U32)(PMIC_VTCXO_2_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio28_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VIO28_PG_ENB_MASK),
                             (U32)(PMIC_VIO28_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_va28_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VA28_PG_ENB_MASK),
                             (U32)(PMIC_VA28_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_va_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VA_PG_ENB_MASK),
                             (U32)(PMIC_VA_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vtcxo_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VTCXO_PG_ENB_MASK),
                             (U32)(PMIC_VTCXO_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vio18_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VIO18_PG_ENB_MASK),
                             (U32)(PMIC_VIO18_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vsram_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VSRAM_PG_ENB_MASK),
                             (U32)(PMIC_VSRAM_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vm_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VM_PG_ENB_MASK),
                             (U32)(PMIC_VM_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vcore_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VCORE_PG_ENB_MASK),
                             (U32)(PMIC_VCORE_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vproc_pg_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON6),
                             (U32)(val),
                             (U32)(PMIC_VPROC_PG_ENB_MASK),
                             (U32)(PMIC_VPROC_PG_ENB_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_qi_osc_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STRUP_CON7),
                           (&val),
                           (U32)(PMIC_QI_OSC_EN_MASK),
                           (U32)(PMIC_QI_OSC_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_just_pwrkey_rst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(STRUP_CON7),
                           (&val),
                           (U32)(PMIC_JUST_PWRKEY_RST_MASK),
                           (U32)(PMIC_JUST_PWRKEY_RST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_uvlo_l2h_deb_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON7),
                             (U32)(val),
                             (U32)(PMIC_UVLO_L2H_DEB_EN_MASK),
                             (U32)(PMIC_UVLO_L2H_DEB_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_clr_just_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON7),
                             (U32)(val),
                             (U32)(PMIC_CLR_JUST_RST_MASK),
                             (U32)(PMIC_CLR_JUST_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_con8_rsv0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON8),
                             (U32)(val),
                             (U32)(PMIC_STRUP_CON8_RSV0_MASK),
                             (U32)(PMIC_STRUP_CON8_RSV0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_ext_pmic_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON8),
                             (U32)(val),
                             (U32)(PMIC_STRUP_EXT_PMIC_SEL_MASK),
                             (U32)(PMIC_STRUP_EXT_PMIC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_ext_pmic_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON8),
                             (U32)(val),
                             (U32)(PMIC_STRUP_EXT_PMIC_EN_MASK),
                             (U32)(PMIC_STRUP_EXT_PMIC_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_auxadc_en_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON8),
                             (U32)(val),
                             (U32)(PMIC_STRUP_AUXADC_EN_SEL_MASK),
                             (U32)(PMIC_STRUP_AUXADC_EN_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_pwroff_preoff_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON9),
                             (U32)(val),
                             (U32)(PMIC_STRUP_PWROFF_PREOFF_EN_MASK),
                             (U32)(PMIC_STRUP_PWROFF_PREOFF_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_strup_pwroff_seq_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(STRUP_CON9),
                             (U32)(val),
                             (U32)(PMIC_STRUP_PWROFF_SEQ_EN_MASK),
                             (U32)(PMIC_STRUP_PWROFF_SEQ_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rg_adc_rdy_c0(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC0),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_C0_MASK),
                           (U32)(PMIC_RG_ADC_RDY_C0_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c0(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC0),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C0_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C0_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_c1(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC1),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_C1_MASK),
                           (U32)(PMIC_RG_ADC_RDY_C1_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c1(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC1),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C1_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C1_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_c2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC2),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_C2_MASK),
                           (U32)(PMIC_RG_ADC_RDY_C2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC2),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C2_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_c3(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC3),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_C3_MASK),
                           (U32)(PMIC_RG_ADC_RDY_C3_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c3(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC3),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C3_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C3_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_c4(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC4),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_C4_MASK),
                           (U32)(PMIC_RG_ADC_RDY_C4_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c4(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC4),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C4_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C4_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_c5(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC5),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_C5_MASK),
                           (U32)(PMIC_RG_ADC_RDY_C5_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c5(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC5),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C5_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C5_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_c6(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC6),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_C6_MASK),
                           (U32)(PMIC_RG_ADC_RDY_C6_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c6(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC6),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C6_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C6_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_c7(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC7),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_C7_MASK),
                           (U32)(PMIC_RG_ADC_RDY_C7_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c7(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC7),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C7_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C7_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_wakeup_pchr(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC8),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_WAKEUP_PCHR_MASK),
                           (U32)(PMIC_RG_ADC_RDY_WAKEUP_PCHR_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_wakeup_pchr(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC8),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_WAKEUP_PCHR_MASK),
                           (U32)(PMIC_RG_ADC_OUT_WAKEUP_PCHR_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_wakeup_swchr(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC9),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_WAKEUP_SWCHR_MASK),
                           (U32)(PMIC_RG_ADC_RDY_WAKEUP_SWCHR_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_wakeup_swchr(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC9),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_WAKEUP_SWCHR_MASK),
                           (U32)(PMIC_RG_ADC_OUT_WAKEUP_SWCHR_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_rdy_lbat(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC10),
                           (&val),
                           (U32)(PMIC_RG_ADC_RDY_LBAT_MASK),
                           (U32)(PMIC_RG_ADC_RDY_LBAT_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_lbat(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC10),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_LBAT_MASK),
                           (U32)(PMIC_RG_ADC_OUT_LBAT_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c0_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC11),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C0_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C0_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c1_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC12),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C1_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C1_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c2_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC13),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C2_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C2_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c3_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC14),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C3_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C3_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c4_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC15),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C4_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C4_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c5_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC16),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C5_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C5_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c6_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC17),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C6_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C6_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_c7_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC18),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_C7_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_C7_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_wakeup_pchr_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC19),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_WAKEUP_PCHR_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_WAKEUP_PCHR_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_wakeup_swchr_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC20),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_WAKEUP_SWCHR_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_WAKEUP_SWCHR_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_lbat_trim(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC21),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_LBAT_TRIM_MASK),
                           (U32)(PMIC_RG_ADC_OUT_LBAT_TRIM_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_adc_out_avg_deci(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_ADC22),
                           (&val),
                           (U32)(PMIC_RG_ADC_OUT_AVG_DECI_MASK),
                           (U32)(PMIC_RG_ADC_OUT_AVG_DECI_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_spl_num(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SPL_NUM_MASK),
                             (U32)(PMIC_RG_SPL_NUM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_avg_num(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AVG_NUM_MASK),
                             (U32)(PMIC_RG_AVG_NUM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_buf_pwd_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_BUF_PWD_ON_MASK),
                             (U32)(PMIC_RG_BUF_PWD_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_adc_pwd_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_ADC_PWD_ON_MASK),
                             (U32)(PMIC_RG_ADC_PWD_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_buf_pwd_b(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_BUF_PWD_B_MASK),
                             (U32)(PMIC_RG_BUF_PWD_B_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_adc_pwd_b(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_ADC_PWD_B_MASK),
                             (U32)(PMIC_RG_ADC_PWD_B_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_chsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_CHSEL_MASK),
                             (U32)(PMIC_RG_AUXADC_CHSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_auto_str_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_AUTO_STR_EN_MASK),
                             (U32)(PMIC_RG_AUXADC_AUTO_STR_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_auto_str(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_AUTO_STR_MASK),
                             (U32)(PMIC_RG_AUXADC_AUTO_STR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_adc_trim_comp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_ADC_TRIM_COMP_MASK),
                             (U32)(PMIC_RG_ADC_TRIM_COMP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_bist_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_BIST_ENB_MASK),
                             (U32)(PMIC_RG_AUXADC_BIST_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_start(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_START_MASK),
                             (U32)(PMIC_RG_AUXADC_START_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lbat_debt_min(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_DEBT_MIN_MASK),
                             (U32)(PMIC_RG_LBAT_DEBT_MIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lbat_debt_max(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_DEBT_MAX_MASK),
                             (U32)(PMIC_RG_LBAT_DEBT_MAX_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lbat_det_prd_15_0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_DET_PRD_15_0_MASK),
                             (U32)(PMIC_RG_LBAT_DET_PRD_15_0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lbat_det_prd_19_16(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_DET_PRD_19_16_MASK),
                             (U32)(PMIC_RG_LBAT_DET_PRD_19_16_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rg_lbat_max_irq_b(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_CON5),
                           (&val),
                           (U32)(PMIC_RG_LBAT_MAX_IRQ_B_MASK),
                           (U32)(PMIC_RG_LBAT_MAX_IRQ_B_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_lbat_en_max(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_EN_MAX_MASK),
                             (U32)(PMIC_RG_LBAT_EN_MAX_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lbat_irq_en_max(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_IRQ_EN_MAX_MASK),
                             (U32)(PMIC_RG_LBAT_IRQ_EN_MAX_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lbat_volt_max(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_VOLT_MAX_MASK),
                             (U32)(PMIC_RG_LBAT_VOLT_MAX_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rg_lbat_min_irq_b(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_CON6),
                           (&val),
                           (U32)(PMIC_RG_LBAT_MIN_IRQ_B_MASK),
                           (U32)(PMIC_RG_LBAT_MIN_IRQ_B_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_lbat_en_min(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_EN_MIN_MASK),
                             (U32)(PMIC_RG_LBAT_EN_MIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lbat_irq_en_min(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_IRQ_EN_MIN_MASK),
                             (U32)(PMIC_RG_LBAT_IRQ_EN_MIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lbat_volt_min(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_LBAT_VOLT_MIN_MASK),
                             (U32)(PMIC_RG_LBAT_VOLT_MIN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rg_lbat_debounce_count_max(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_CON7),
                           (&val),
                           (U32)(PMIC_RG_LBAT_DEBOUNCE_COUNT_MAX_MASK),
                           (U32)(PMIC_RG_LBAT_DEBOUNCE_COUNT_MAX_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_lbat_debounce_count_min(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_CON8),
                           (&val),
                           (U32)(PMIC_RG_LBAT_DEBOUNCE_COUNT_MIN_MASK),
                           (U32)(PMIC_RG_LBAT_DEBOUNCE_COUNT_MIN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_ni_comp(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(AUXADC_CON9),
                           (&val),
                           (U32)(PMIC_RG_NI_COMP_MASK),
                           (U32)(PMIC_RG_NI_COMP_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_da_dac(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_DA_DAC_MASK),
                             (U32)(PMIC_RG_DA_DAC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_cali(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_CALI_MASK),
                             (U32)(PMIC_RG_AUXADC_CALI_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_buf_cali(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_BUF_CALI_MASK),
                             (U32)(PMIC_RG_BUF_CALI_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auxadc_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_AUXADC_RSV_MASK),
                             (U32)(PMIC_RG_AUXADC_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_da_dac_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_DA_DAC_SEL_MASK),
                             (U32)(PMIC_RG_DA_DAC_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_aux_out_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_AUX_OUT_SEL_MASK),
                             (U32)(PMIC_RG_AUX_OUT_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_arb_prio_2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_ARB_PRIO_2_MASK),
                             (U32)(PMIC_RG_ARB_PRIO_2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_arb_prio_1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_ARB_PRIO_1_MASK),
                             (U32)(PMIC_RG_ARB_PRIO_1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_arb_prio_0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON10),
                             (U32)(val),
                             (U32)(PMIC_RG_ARB_PRIO_0_MASK),
                             (U32)(PMIC_RG_ARB_PRIO_0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_efuse_offset_ch0_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON11),
                             (U32)(val),
                             (U32)(PMIC_EFUSE_OFFSET_CH0_TRIM_MASK),
                             (U32)(PMIC_EFUSE_OFFSET_CH0_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_efuse_gain_ch0_trim(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON11),
                             (U32)(val),
                             (U32)(PMIC_EFUSE_GAIN_CH0_TRIM_MASK),
                             (U32)(PMIC_EFUSE_GAIN_CH0_TRIM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbuf_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_VBUF_EN_MASK),
                             (U32)(PMIC_RG_VBUF_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbuf_byp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_VBUF_BYP_MASK),
                             (U32)(PMIC_RG_VBUF_BYP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbuf_calen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON12),
                             (U32)(val),
                             (U32)(PMIC_RG_VBUF_CALEN_MASK),
                             (U32)(PMIC_RG_VBUF_CALEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_thermal_adc_oe(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON13),
                             (U32)(val),
                             (U32)(PMIC_RG_THERMAL_ADC_OE_MASK),
                             (U32)(PMIC_RG_THERMAL_ADC_OE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_thermal_adc_ge(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON13),
                             (U32)(val),
                             (U32)(PMIC_RG_THERMAL_ADC_GE_MASK),
                             (U32)(PMIC_RG_THERMAL_ADC_GE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_adc_trim_ch_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON14),
                             (U32)(val),
                             (U32)(PMIC_RG_ADC_TRIM_CH_SEL_MASK),
                             (U32)(PMIC_RG_ADC_TRIM_CH_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_source_ch0_norm_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON14),
                             (U32)(val),
                             (U32)(PMIC_RG_SOURCE_CH0_NORM_SEL_MASK),
                             (U32)(PMIC_RG_SOURCE_CH0_NORM_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_source_ch0_lbat_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUXADC_CON14),
                             (U32)(val),
                             (U32)(PMIC_RG_SOURCE_CH0_LBAT_SEL_MASK),
                             (U32)(PMIC_RG_SOURCE_CH0_LBAT_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_rsv0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON0),
                             (U32)(val),
                             (U32)(PMIC_FLASH_RSV0_MASK),
                             (U32)(PMIC_FLASH_RSV0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_dim_duty(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON0),
                             (U32)(val),
                             (U32)(PMIC_FLASH_DIM_DUTY_MASK),
                             (U32)(PMIC_FLASH_DIM_DUTY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_ther_shdn_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON0),
                             (U32)(val),
                             (U32)(PMIC_FLASH_THER_SHDN_EN_MASK),
                             (U32)(PMIC_FLASH_THER_SHDN_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON0),
                             (U32)(val),
                             (U32)(PMIC_FLASH_EN_MASK),
                             (U32)(PMIC_FLASH_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_dim_div(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON1),
                             (U32)(val),
                             (U32)(PMIC_FLASH_DIM_DIV_MASK),
                             (U32)(PMIC_FLASH_DIM_DIV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_rsv1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON1),
                             (U32)(val),
                             (U32)(PMIC_FLASH_RSV1_MASK),
                             (U32)(PMIC_FLASH_RSV1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON1),
                             (U32)(val),
                             (U32)(PMIC_FLASH_SEL_MASK),
                             (U32)(PMIC_FLASH_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_sfstren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON2),
                             (U32)(val),
                             (U32)(PMIC_FLASH_SFSTREN_MASK),
                             (U32)(PMIC_FLASH_SFSTREN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_sfstr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON2),
                             (U32)(val),
                             (U32)(PMIC_FLASH_SFSTR_MASK),
                             (U32)(PMIC_FLASH_SFSTR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_flash_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FLASH_CON2),
                             (U32)(val),
                             (U32)(PMIC_FLASH_MODE_MASK),
                             (U32)(PMIC_FLASH_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_rsv0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON0),
                             (U32)(val),
                             (U32)(PMIC_KPLED_RSV0_MASK),
                             (U32)(PMIC_KPLED_RSV0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_dim_duty(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON0),
                             (U32)(val),
                             (U32)(PMIC_KPLED_DIM_DUTY_MASK),
                             (U32)(PMIC_KPLED_DIM_DUTY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_ther_shdn_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON0),
                             (U32)(val),
                             (U32)(PMIC_KPLED_THER_SHDN_EN_MASK),
                             (U32)(PMIC_KPLED_THER_SHDN_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON0),
                             (U32)(val),
                             (U32)(PMIC_KPLED_EN_MASK),
                             (U32)(PMIC_KPLED_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_dim_div(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON1),
                             (U32)(val),
                             (U32)(PMIC_KPLED_DIM_DIV_MASK),
                             (U32)(PMIC_KPLED_DIM_DIV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_rsv1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON1),
                             (U32)(val),
                             (U32)(PMIC_KPLED_RSV1_MASK),
                             (U32)(PMIC_KPLED_RSV1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON1),
                             (U32)(val),
                             (U32)(PMIC_KPLED_SEL_MASK),
                             (U32)(PMIC_KPLED_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_sfstren(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON2),
                             (U32)(val),
                             (U32)(PMIC_KPLED_SFSTREN_MASK),
                             (U32)(PMIC_KPLED_SFSTREN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_sfstr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON2),
                             (U32)(val),
                             (U32)(PMIC_KPLED_SFSTR_MASK),
                             (U32)(PMIC_KPLED_SFSTR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_kpled_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(KPLED_CON2),
                             (U32)(val),
                             (U32)(PMIC_KPLED_MODE_MASK),
                             (U32)(PMIC_KPLED_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_rsv0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON0),
                             (U32)(val),
                             (U32)(PMIC_ISINK_RSV0_MASK),
                             (U32)(PMIC_ISINK_RSV0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_dim0_duty(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON0),
                             (U32)(val),
                             (U32)(PMIC_ISINK_DIM0_DUTY_MASK),
                             (U32)(PMIC_ISINK_DIM0_DUTY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_dim0_fsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON0),
                             (U32)(val),
                             (U32)(PMIC_ISINK_DIM0_FSEL_MASK),
                             (U32)(PMIC_ISINK_DIM0_FSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_rsv1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON1),
                             (U32)(val),
                             (U32)(PMIC_ISINK_RSV1_MASK),
                             (U32)(PMIC_ISINK_RSV1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_dim1_duty(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON1),
                             (U32)(val),
                             (U32)(PMIC_ISINK_DIM1_DUTY_MASK),
                             (U32)(PMIC_ISINK_DIM1_DUTY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_dim1_fsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON1),
                             (U32)(val),
                             (U32)(PMIC_ISINK_DIM1_FSEL_MASK),
                             (U32)(PMIC_ISINK_DIM1_FSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_rsv2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON2),
                             (U32)(val),
                             (U32)(PMIC_ISINK_RSV2_MASK),
                             (U32)(PMIC_ISINK_RSV2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_dim2_duty(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON2),
                             (U32)(val),
                             (U32)(PMIC_ISINK_DIM2_DUTY_MASK),
                             (U32)(PMIC_ISINK_DIM2_DUTY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_dim2_fsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON2),
                             (U32)(val),
                             (U32)(PMIC_ISINK_DIM2_FSEL_MASK),
                             (U32)(PMIC_ISINK_DIM2_FSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_rsv3(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON3),
                             (U32)(val),
                             (U32)(PMIC_ISINK_RSV3_MASK),
                             (U32)(PMIC_ISINK_RSV3_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_ch2_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON3),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_CH2_EN_MASK),
                             (U32)(PMIC_ISINKS_CH2_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_ch1_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON3),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_CH1_EN_MASK),
                             (U32)(PMIC_ISINKS_CH1_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_ch0_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON3),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_CH0_EN_MASK),
                             (U32)(PMIC_ISINKS_CH0_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink_rsv4(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON3),
                             (U32)(val),
                             (U32)(PMIC_ISINK_RSV4_MASK),
                             (U32)(PMIC_ISINK_RSV4_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks2_chop_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON3),
                             (U32)(val),
                             (U32)(PMIC_ISINKS2_CHOP_EN_MASK),
                             (U32)(PMIC_ISINKS2_CHOP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks1_chop_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON3),
                             (U32)(val),
                             (U32)(PMIC_ISINKS1_CHOP_EN_MASK),
                             (U32)(PMIC_ISINKS1_CHOP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks0_chop_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON3),
                             (U32)(val),
                             (U32)(PMIC_ISINKS0_CHOP_EN_MASK),
                             (U32)(PMIC_ISINKS0_CHOP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_ch0_step(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON4),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_CH0_STEP_MASK),
                             (U32)(PMIC_ISINKS_CH0_STEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink0_chop_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON4),
                             (U32)(val),
                             (U32)(PMIC_ISINK0_CHOP_MODE_MASK),
                             (U32)(PMIC_ISINK0_CHOP_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink0_test_reg(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON4),
                             (U32)(val),
                             (U32)(PMIC_ISINK0_TEST_REG_MASK),
                             (U32)(PMIC_ISINK0_TEST_REG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_ch0_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON4),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_CH0_MODE_MASK),
                             (U32)(PMIC_ISINKS_CH0_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_ch1_step(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON5),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_CH1_STEP_MASK),
                             (U32)(PMIC_ISINKS_CH1_STEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink1_chop_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON5),
                             (U32)(val),
                             (U32)(PMIC_ISINK1_CHOP_MODE_MASK),
                             (U32)(PMIC_ISINK1_CHOP_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink1_test_reg(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON5),
                             (U32)(val),
                             (U32)(PMIC_ISINK1_TEST_REG_MASK),
                             (U32)(PMIC_ISINK1_TEST_REG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_ch1_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON5),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_CH1_MODE_MASK),
                             (U32)(PMIC_ISINKS_CH1_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_ch2_step(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON6),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_CH2_STEP_MASK),
                             (U32)(PMIC_ISINKS_CH2_STEP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink2_chop_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON6),
                             (U32)(val),
                             (U32)(PMIC_ISINK2_CHOP_MODE_MASK),
                             (U32)(PMIC_ISINK2_CHOP_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink2_test_reg(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON6),
                             (U32)(val),
                             (U32)(PMIC_ISINK2_TEST_REG_MASK),
                             (U32)(PMIC_ISINK2_TEST_REG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_ch2_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON6),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_CH2_MODE_MASK),
                             (U32)(PMIC_ISINKS_CH2_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_trim_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_TRIM_EN_MASK),
                             (U32)(PMIC_RG_TRIM_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_trim_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_TRIM_SEL_MASK),
                             (U32)(PMIC_RG_TRIM_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_ldo_bist(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_LDO_BIST_MASK),
                             (U32)(PMIC_RG_LDO_BIST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_isinks_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON7),
                             (U32)(val),
                             (U32)(PMIC_RG_ISINKS_RSV_MASK),
                             (U32)(PMIC_RG_ISINKS_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_breath0_trf_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON8),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_BREATH0_TRF_SEL_MASK),
                             (U32)(PMIC_ISINKS_BREATH0_TRF_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_breath0_ton_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON8),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_BREATH0_TON_SEL_MASK),
                             (U32)(PMIC_ISINKS_BREATH0_TON_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_breath0_toff_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON8),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_BREATH0_TOFF_SEL_MASK),
                             (U32)(PMIC_ISINKS_BREATH0_TOFF_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_breath1_trf_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON9),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_BREATH1_TRF_SEL_MASK),
                             (U32)(PMIC_ISINKS_BREATH1_TRF_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_breath1_ton_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON9),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_BREATH1_TON_SEL_MASK),
                             (U32)(PMIC_ISINKS_BREATH1_TON_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_breath1_toff_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON9),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_BREATH1_TOFF_SEL_MASK),
                             (U32)(PMIC_ISINKS_BREATH1_TOFF_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_breath2_trf_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON10),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_BREATH2_TRF_SEL_MASK),
                             (U32)(PMIC_ISINKS_BREATH2_TRF_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_breath2_ton_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON10),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_BREATH2_TON_SEL_MASK),
                             (U32)(PMIC_ISINKS_BREATH2_TON_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isinks_breath2_toff_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON10),
                             (U32)(val),
                             (U32)(PMIC_ISINKS_BREATH2_TOFF_SEL_MASK),
                             (U32)(PMIC_ISINKS_BREATH2_TOFF_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink0_sfstr_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON11),
                             (U32)(val),
                             (U32)(PMIC_ISINK0_SFSTR_EN_MASK),
                             (U32)(PMIC_ISINK0_SFSTR_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink1_sfstr_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON11),
                             (U32)(val),
                             (U32)(PMIC_ISINK1_SFSTR_EN_MASK),
                             (U32)(PMIC_ISINK1_SFSTR_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink2_sfstr_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON11),
                             (U32)(val),
                             (U32)(PMIC_ISINK2_SFSTR_EN_MASK),
                             (U32)(PMIC_ISINK2_SFSTR_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink0_sfstr_tc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON11),
                             (U32)(val),
                             (U32)(PMIC_ISINK0_SFSTR_TC_MASK),
                             (U32)(PMIC_ISINK0_SFSTR_TC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink1_sfstr_tc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON11),
                             (U32)(val),
                             (U32)(PMIC_ISINK1_SFSTR_TC_MASK),
                             (U32)(PMIC_ISINK1_SFSTR_TC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_isink2_sfstr_tc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ISINKS_CON11),
                             (U32)(val),
                             (U32)(PMIC_ISINK2_SFSTR_TC_MASK),
                             (U32)(PMIC_ISINK2_SFSTR_TC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audaccdetrsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDACCDETRSV_MASK),
                             (U32)(PMIC_RG_AUDACCDETRSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_con0_rsv1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON0),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_CON0_RSV1_MASK),
                             (U32)(PMIC_ACCDET_CON0_RSV1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audaccdetvin1pulllow(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDACCDETVIN1PULLLOW_MASK),
                             (U32)(PMIC_RG_AUDACCDETVIN1PULLLOW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audaccdettvdet(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDACCDETTVDET_MASK),
                             (U32)(PMIC_RG_AUDACCDETTVDET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audaccdetanaswctrl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDACCDETANASWCTRL_MASK),
                             (U32)(PMIC_RG_AUDACCDETANASWCTRL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_audaccdetanaswctrl_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON0),
                             (U32)(val),
                             (U32)(PMIC_AUDACCDETANASWCTRL_SEL_MASK),
                             (U32)(PMIC_AUDACCDETANASWCTRL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_con0_rsv0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON0),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_CON0_RSV0_MASK),
                             (U32)(PMIC_ACCDET_CON0_RSV0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audaccdetvthcal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDACCDETVTHCAL_MASK),
                             (U32)(PMIC_RG_AUDACCDETVTHCAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_seq_init(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON1),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_SEQ_INIT_MASK),
                             (U32)(PMIC_ACCDET_SEQ_INIT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON1),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_EN_MASK),
                             (U32)(PMIC_ACCDET_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_mbias_pwm_idle(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON2),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_MBIAS_PWM_IDLE_MASK),
                             (U32)(PMIC_ACCDET_MBIAS_PWM_IDLE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_vth_pwm_idle(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON2),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_VTH_PWM_IDLE_MASK),
                             (U32)(PMIC_ACCDET_VTH_PWM_IDLE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_cmp_pwm_idle(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON2),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_CMP_PWM_IDLE_MASK),
                             (U32)(PMIC_ACCDET_CMP_PWM_IDLE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_mbias_pwm_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON2),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_MBIAS_PWM_EN_MASK),
                             (U32)(PMIC_ACCDET_MBIAS_PWM_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_vth_pwm_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON2),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_VTH_PWM_EN_MASK),
                             (U32)(PMIC_ACCDET_VTH_PWM_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_cmp_pwm_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON2),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_CMP_PWM_EN_MASK),
                             (U32)(PMIC_ACCDET_CMP_PWM_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_pwm_width(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON3),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_PWM_WIDTH_MASK),
                             (U32)(PMIC_ACCDET_PWM_WIDTH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_pwm_thresh(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON4),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_PWM_THRESH_MASK),
                             (U32)(PMIC_ACCDET_PWM_THRESH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_fall_delay(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON5),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_FALL_DELAY_MASK),
                             (U32)(PMIC_ACCDET_FALL_DELAY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_rise_delay(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON5),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_RISE_DELAY_MASK),
                             (U32)(PMIC_ACCDET_RISE_DELAY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_debounce0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON6),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_DEBOUNCE0_MASK),
                             (U32)(PMIC_ACCDET_DEBOUNCE0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_debounce1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON7),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_DEBOUNCE1_MASK),
                             (U32)(PMIC_ACCDET_DEBOUNCE1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_debounce2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON8),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_DEBOUNCE2_MASK),
                             (U32)(PMIC_ACCDET_DEBOUNCE2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_debounce3(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON9),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_DEBOUNCE3_MASK),
                             (U32)(PMIC_ACCDET_DEBOUNCE3_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_ival_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON10),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_IVAL_SEL_MASK),
                             (U32)(PMIC_ACCDET_IVAL_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_ival_mem_in(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON10),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_IVAL_MEM_IN_MASK),
                             (U32)(PMIC_ACCDET_IVAL_MEM_IN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_ival_sam_in(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON10),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_IVAL_SAM_IN_MASK),
                             (U32)(PMIC_ACCDET_IVAL_SAM_IN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_ival_cur_in(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON10),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_IVAL_CUR_IN_MASK),
                             (U32)(PMIC_ACCDET_IVAL_CUR_IN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_irq_clr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON11),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_IRQ_CLR_MASK),
                             (U32)(PMIC_ACCDET_IRQ_CLR_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_accdet_irq(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON11),
                           (&val),
                           (U32)(PMIC_ACCDET_IRQ_MASK),
                           (U32)(PMIC_ACCDET_IRQ_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_accdet_pwm_en_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_PWM_EN_SW_MASK),
                             (U32)(PMIC_ACCDET_PWM_EN_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_mbias_en_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_MBIAS_EN_SW_MASK),
                             (U32)(PMIC_ACCDET_MBIAS_EN_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_vth_en_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_VTH_EN_SW_MASK),
                             (U32)(PMIC_ACCDET_VTH_EN_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_cmp_en_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_CMP_EN_SW_MASK),
                             (U32)(PMIC_ACCDET_CMP_EN_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_in_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_IN_SW_MASK),
                             (U32)(PMIC_ACCDET_IN_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_pwm_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_PWM_SEL_MASK),
                             (U32)(PMIC_ACCDET_PWM_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_test_mode5(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_TEST_MODE5_MASK),
                             (U32)(PMIC_ACCDET_TEST_MODE5_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_test_mode4(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_TEST_MODE4_MASK),
                             (U32)(PMIC_ACCDET_TEST_MODE4_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_test_mode3(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_TEST_MODE3_MASK),
                             (U32)(PMIC_ACCDET_TEST_MODE3_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_test_mode2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_TEST_MODE2_MASK),
                             (U32)(PMIC_ACCDET_TEST_MODE2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_test_mode1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_TEST_MODE1_MASK),
                             (U32)(PMIC_ACCDET_TEST_MODE1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_test_mode0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON12),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_TEST_MODE0_MASK),
                             (U32)(PMIC_ACCDET_TEST_MODE0_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_accdet_cmp_clk(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON13),
                           (&val),
                           (U32)(PMIC_ACCDET_CMP_CLK_MASK),
                           (U32)(PMIC_ACCDET_CMP_CLK_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_accdet_vth_clk(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON13),
                           (&val),
                           (U32)(PMIC_ACCDET_VTH_CLK_MASK),
                           (U32)(PMIC_ACCDET_VTH_CLK_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_accdet_mbias_clk(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON13),
                           (&val),
                           (U32)(PMIC_ACCDET_MBIAS_CLK_MASK),
                           (U32)(PMIC_ACCDET_MBIAS_CLK_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_accdet_state(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON13),
                           (&val),
                           (U32)(PMIC_ACCDET_STATE_MASK),
                           (U32)(PMIC_ACCDET_STATE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_accdet_mem_in(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON13),
                           (&val),
                           (U32)(PMIC_ACCDET_MEM_IN_MASK),
                           (U32)(PMIC_ACCDET_MEM_IN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_accdet_sam_in(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON13),
                           (&val),
                           (U32)(PMIC_ACCDET_SAM_IN_MASK),
                           (U32)(PMIC_ACCDET_SAM_IN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_accdet_cur_in(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON13),
                           (&val),
                           (U32)(PMIC_ACCDET_CUR_IN_MASK),
                           (U32)(PMIC_ACCDET_CUR_IN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_accdet_in(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON13),
                           (&val),
                           (U32)(PMIC_ACCDET_IN_MASK),
                           (U32)(PMIC_ACCDET_IN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_accdet_cur_deb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ACCDET_CON14),
                           (&val),
                           (U32)(PMIC_ACCDET_CUR_DEB_MASK),
                           (U32)(PMIC_ACCDET_CUR_DEB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_accdet_rsv_con0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON15),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_RSV_CON0_MASK),
                             (U32)(PMIC_ACCDET_RSV_CON0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_accdet_rsv_con1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ACCDET_CON16),
                             (U32)(val),
                             (U32)(PMIC_ACCDET_RSV_CON1_MASK),
                             (U32)(PMIC_ACCDET_RSV_CON1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_gainl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_GAINL_MASK),
                             (U32)(PMIC_RG_SPK_GAINL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_ther_shdn_l_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON0),
                             (U32)(val),
                             (U32)(PMIC_SPK_THER_SHDN_L_EN_MASK),
                             (U32)(PMIC_SPK_THER_SHDN_L_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_oc_shdn_dl(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON0),
                             (U32)(val),
                             (U32)(PMIC_SPK_OC_SHDN_DL_MASK),
                             (U32)(PMIC_SPK_OC_SHDN_DL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_trim_en_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON0),
                             (U32)(val),
                             (U32)(PMIC_SPK_TRIM_EN_L_MASK),
                             (U32)(PMIC_SPK_TRIM_EN_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spkmode_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON0),
                             (U32)(val),
                             (U32)(PMIC_SPKMODE_L_MASK),
                             (U32)(PMIC_SPKMODE_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_en_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON0),
                             (U32)(val),
                             (U32)(PMIC_SPK_EN_L_MASK),
                             (U32)(PMIC_SPK_EN_L_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_spk_trim_done_l(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON1),
                           (&val),
                           (U32)(PMIC_SPK_TRIM_DONE_L_MASK),
                           (U32)(PMIC_SPK_TRIM_DONE_L_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_spk_offset_l_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON1),
                             (U32)(val),
                             (U32)(PMIC_SPK_OFFSET_L_MODE_MASK),
                             (U32)(PMIC_SPK_OFFSET_L_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_lead_l_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON1),
                             (U32)(val),
                             (U32)(PMIC_SPK_LEAD_L_SW_MASK),
                             (U32)(PMIC_SPK_LEAD_L_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_offset_l_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON1),
                             (U32)(val),
                             (U32)(PMIC_SPK_OFFSET_L_SW_MASK),
                             (U32)(PMIC_SPK_OFFSET_L_SW_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_spk_offset_l_ov(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON1),
                           (&val),
                           (U32)(PMIC_SPK_OFFSET_L_OV_MASK),
                           (U32)(PMIC_SPK_OFFSET_L_OV_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_spk_lead_l_flag(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON1),
                           (&val),
                           (U32)(PMIC_SPK_LEAD_L_FLAG_MASK),
                           (U32)(PMIC_SPK_LEAD_L_FLAG_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_spk_lead_l_flag_deg(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON1),
                           (&val),
                           (U32)(PMIC_SPK_LEAD_L_FLAG_DEG_MASK),
                           (U32)(PMIC_SPK_LEAD_L_FLAG_DEG_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_spk_offset_l(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON1),
                           (&val),
                           (U32)(PMIC_SPK_OFFSET_L_MASK),
                           (U32)(PMIC_SPK_OFFSET_L_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_spk_oc_en_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_OC_EN_L_MASK),
                             (U32)(PMIC_RG_SPK_OC_EN_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spkab_oc_en_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPKAB_OC_EN_L_MASK),
                             (U32)(PMIC_RG_SPKAB_OC_EN_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_test_en_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_TEST_EN_L_MASK),
                             (U32)(PMIC_RG_SPK_TEST_EN_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_drc_en_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_DRC_EN_L_MASK),
                             (U32)(PMIC_RG_SPK_DRC_EN_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spkrcv_en_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPKRCV_EN_L_MASK),
                             (U32)(PMIC_RG_SPKRCV_EN_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spkab_obias_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPKAB_OBIAS_L_MASK),
                             (U32)(PMIC_RG_SPKAB_OBIAS_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_slew_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_SLEW_L_MASK),
                             (U32)(PMIC_RG_SPK_SLEW_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_force_en_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_FORCE_EN_L_MASK),
                             (U32)(PMIC_RG_SPK_FORCE_EN_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_intg_rst_l(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_INTG_RST_L_MASK),
                             (U32)(PMIC_RG_SPK_INTG_RST_L_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_gainr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_GAINR_MASK),
                             (U32)(PMIC_RG_SPK_GAINR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_ther_shdn_r_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON3),
                             (U32)(val),
                             (U32)(PMIC_SPK_THER_SHDN_R_EN_MASK),
                             (U32)(PMIC_SPK_THER_SHDN_R_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_oc_shdn_dr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON3),
                             (U32)(val),
                             (U32)(PMIC_SPK_OC_SHDN_DR_MASK),
                             (U32)(PMIC_SPK_OC_SHDN_DR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_trim_en_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON3),
                             (U32)(val),
                             (U32)(PMIC_SPK_TRIM_EN_R_MASK),
                             (U32)(PMIC_SPK_TRIM_EN_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spkmode_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON3),
                             (U32)(val),
                             (U32)(PMIC_SPKMODE_R_MASK),
                             (U32)(PMIC_SPKMODE_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_en_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON3),
                             (U32)(val),
                             (U32)(PMIC_SPK_EN_R_MASK),
                             (U32)(PMIC_SPK_EN_R_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_spk_trim_done_r(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON4),
                           (&val),
                           (U32)(PMIC_SPK_TRIM_DONE_R_MASK),
                           (U32)(PMIC_SPK_TRIM_DONE_R_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_spk_offset_r_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON4),
                             (U32)(val),
                             (U32)(PMIC_SPK_OFFSET_R_MODE_MASK),
                             (U32)(PMIC_SPK_OFFSET_R_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_lead_r_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON4),
                             (U32)(val),
                             (U32)(PMIC_SPK_LEAD_R_SW_MASK),
                             (U32)(PMIC_SPK_LEAD_R_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_offset_r_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON4),
                             (U32)(val),
                             (U32)(PMIC_SPK_OFFSET_R_SW_MASK),
                             (U32)(PMIC_SPK_OFFSET_R_SW_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_spk_offset_r_ov(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON4),
                           (&val),
                           (U32)(PMIC_SPK_OFFSET_R_OV_MASK),
                           (U32)(PMIC_SPK_OFFSET_R_OV_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_spk_lead_r_flag(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON4),
                           (&val),
                           (U32)(PMIC_SPK_LEAD_R_FLAG_MASK),
                           (U32)(PMIC_SPK_LEAD_R_FLAG_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_spk_lead_r_flag_deg(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON4),
                           (&val),
                           (U32)(PMIC_SPK_LEAD_R_FLAG_DEG_MASK),
                           (U32)(PMIC_SPK_LEAD_R_FLAG_DEG_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_spk_offset_r(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON4),
                           (&val),
                           (U32)(PMIC_SPK_OFFSET_R_MASK),
                           (U32)(PMIC_SPK_OFFSET_R_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_spk_oc_en_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_OC_EN_R_MASK),
                             (U32)(PMIC_RG_SPK_OC_EN_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spkab_oc_en_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SPKAB_OC_EN_R_MASK),
                             (U32)(PMIC_RG_SPKAB_OC_EN_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_test_en_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_TEST_EN_R_MASK),
                             (U32)(PMIC_RG_SPK_TEST_EN_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_drc_en_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_DRC_EN_R_MASK),
                             (U32)(PMIC_RG_SPK_DRC_EN_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spkrcv_en_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SPKRCV_EN_R_MASK),
                             (U32)(PMIC_RG_SPKRCV_EN_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spkab_obias_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SPKAB_OBIAS_R_MASK),
                             (U32)(PMIC_RG_SPKAB_OBIAS_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_slew_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_SLEW_R_MASK),
                             (U32)(PMIC_RG_SPK_SLEW_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_force_en_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_FORCE_EN_R_MASK),
                             (U32)(PMIC_RG_SPK_FORCE_EN_R_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_intg_rst_r(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_INTG_RST_R_MASK),
                             (U32)(PMIC_RG_SPK_INTG_RST_R_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_spk_ab_oc_l_deg(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON6),
                           (&val),
                           (U32)(PMIC_SPK_AB_OC_L_DEG_MASK),
                           (U32)(PMIC_SPK_AB_OC_L_DEG_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_spk_d_oc_l_deg(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON6),
                           (&val),
                           (U32)(PMIC_SPK_D_OC_L_DEG_MASK),
                           (U32)(PMIC_SPK_D_OC_L_DEG_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_spk_ab_oc_r_deg(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON6),
                           (&val),
                           (U32)(PMIC_SPK_AB_OC_R_DEG_MASK),
                           (U32)(PMIC_SPK_AB_OC_R_DEG_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_spk_d_oc_r_deg(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(SPK_CON6),
                           (&val),
                           (U32)(PMIC_SPK_D_OC_R_DEG_MASK),
                           (U32)(PMIC_SPK_D_OC_R_DEG_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_spk_oc_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON6),
                             (U32)(val),
                             (U32)(PMIC_SPK_OC_THD_MASK),
                             (U32)(PMIC_SPK_OC_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_oc_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON6),
                             (U32)(val),
                             (U32)(PMIC_SPK_OC_WND_MASK),
                             (U32)(PMIC_SPK_OC_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_trim_thd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON6),
                             (U32)(val),
                             (U32)(PMIC_SPK_TRIM_THD_MASK),
                             (U32)(PMIC_SPK_TRIM_THD_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_trim_wnd(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON6),
                             (U32)(val),
                             (U32)(PMIC_SPK_TRIM_WND_MASK),
                             (U32)(PMIC_SPK_TRIM_WND_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_trim_div(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON7),
                             (U32)(val),
                             (U32)(PMIC_SPK_TRIM_DIV_MASK),
                             (U32)(PMIC_SPK_TRIM_DIV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_td3(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON7),
                             (U32)(val),
                             (U32)(PMIC_SPK_TD3_MASK),
                             (U32)(PMIC_SPK_TD3_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_td2(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON7),
                             (U32)(val),
                             (U32)(PMIC_SPK_TD2_MASK),
                             (U32)(PMIC_SPK_TD2_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_td1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON7),
                             (U32)(val),
                             (U32)(PMIC_SPK_TD1_MASK),
                             (U32)(PMIC_SPK_TD1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_octh_d(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_OCTH_D_MASK),
                             (U32)(PMIC_RG_SPK_OCTH_D_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spkab_ovdrv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_SPKAB_OVDRV_MASK),
                             (U32)(PMIC_RG_SPKAB_OVDRV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_fbrc_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_FBRC_EN_MASK),
                             (U32)(PMIC_RG_SPK_FBRC_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_vcm_ibsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_VCM_IBSEL_MASK),
                             (U32)(PMIC_RG_SPK_VCM_IBSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_vcm_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_VCM_SEL_MASK),
                             (U32)(PMIC_RG_SPK_VCM_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_en_view_clk(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_EN_VIEW_CLK_MASK),
                             (U32)(PMIC_RG_SPK_EN_VIEW_CLK_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_en_view_vcm(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_EN_VIEW_VCM_MASK),
                             (U32)(PMIC_RG_SPK_EN_VIEW_VCM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_ccode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_CCODE_MASK),
                             (U32)(PMIC_RG_SPK_CCODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_ibias_sel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_IBIAS_SEL_MASK),
                             (U32)(PMIC_RG_SPK_IBIAS_SEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_btl_set(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON8),
                             (U32)(val),
                             (U32)(PMIC_RG_BTL_SET_MASK),
                             (U32)(PMIC_RG_BTL_SET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_test_mode1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON9),
                             (U32)(val),
                             (U32)(PMIC_SPK_TEST_MODE1_MASK),
                             (U32)(PMIC_SPK_TEST_MODE1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_test_mode0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON9),
                             (U32)(val),
                             (U32)(PMIC_SPK_TEST_MODE0_MASK),
                             (U32)(PMIC_SPK_TEST_MODE0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_vcm_fast_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON9),
                             (U32)(val),
                             (U32)(PMIC_SPK_VCM_FAST_EN_MASK),
                             (U32)(PMIC_SPK_VCM_FAST_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_rsv0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON9),
                             (U32)(val),
                             (U32)(PMIC_SPK_RSV0_MASK),
                             (U32)(PMIC_SPK_RSV0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spkpga_gain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_SPKPGA_GAIN_MASK),
                             (U32)(PMIC_RG_SPKPGA_GAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spk_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON9),
                             (U32)(val),
                             (U32)(PMIC_RG_SPK_RSV_MASK),
                             (U32)(PMIC_RG_SPK_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_td_done(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON10),
                             (U32)(val),
                             (U32)(PMIC_SPK_TD_DONE_MASK),
                             (U32)(PMIC_SPK_TD_DONE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_td_wait(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON10),
                             (U32)(val),
                             (U32)(PMIC_SPK_TD_WAIT_MASK),
                             (U32)(PMIC_SPK_TD_WAIT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_trim_stop_l_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_TRIM_STOP_L_SW_MASK),
                             (U32)(PMIC_SPK_TRIM_STOP_L_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_trim_stop_r_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_TRIM_STOP_R_SW_MASK),
                             (U32)(PMIC_SPK_TRIM_STOP_R_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_trim_en_l_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_TRIM_EN_L_SW_MASK),
                             (U32)(PMIC_SPK_TRIM_EN_L_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_trim_en_r_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_TRIM_EN_R_SW_MASK),
                             (U32)(PMIC_SPK_TRIM_EN_R_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_outstg_en_l_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_OUTSTG_EN_L_SW_MASK),
                             (U32)(PMIC_SPK_OUTSTG_EN_L_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_outstg_en_r_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_OUTSTG_EN_R_SW_MASK),
                             (U32)(PMIC_SPK_OUTSTG_EN_R_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_en_l_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_EN_L_SW_MASK),
                             (U32)(PMIC_SPK_EN_L_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_en_r_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_EN_R_SW_MASK),
                             (U32)(PMIC_SPK_EN_R_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_depop_en_l_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_DEPOP_EN_L_SW_MASK),
                             (U32)(PMIC_SPK_DEPOP_EN_L_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_depop_en_r_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_DEPOP_EN_R_SW_MASK),
                             (U32)(PMIC_SPK_DEPOP_EN_R_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spkmode_l_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPKMODE_L_SW_MASK),
                             (U32)(PMIC_SPKMODE_L_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spkmode_r_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPKMODE_R_SW_MASK),
                             (U32)(PMIC_SPKMODE_R_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_rst_l_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_RST_L_SW_MASK),
                             (U32)(PMIC_SPK_RST_L_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_rst_r_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_RST_R_SW_MASK),
                             (U32)(PMIC_SPK_RST_R_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_vcm_fast_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_VCM_FAST_SW_MASK),
                             (U32)(PMIC_SPK_VCM_FAST_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_spk_en_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(SPK_CON11),
                             (U32)(val),
                             (U32)(PMIC_SPK_EN_MODE_MASK),
                             (U32)(PMIC_SPK_EN_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_sw_rstclr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_SW_RSTCLR_MASK),
                             (U32)(PMIC_FG_SW_RSTCLR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_charge_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_CHARGE_RST_MASK),
                             (U32)(PMIC_FG_CHARGE_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_time_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_TIME_RST_MASK),
                             (U32)(PMIC_FG_TIME_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_offset_rst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_OFFSET_RST_MASK),
                             (U32)(PMIC_FG_OFFSET_RST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_sw_clear(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_SW_CLEAR_MASK),
                             (U32)(PMIC_FG_SW_CLEAR_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_fg_latchdata_st(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON0),
                           (&val),
                           (U32)(PMIC_FG_LATCHDATA_ST_MASK),
                           (U32)(PMIC_FG_LATCHDATA_ST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_fg_sw_read_pre(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_SW_READ_PRE_MASK),
                             (U32)(PMIC_FG_SW_READ_PRE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_sw_cr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_SW_CR_MASK),
                             (U32)(PMIC_FG_SW_CR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_fgclksrc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_FGCLKSRC_MASK),
                             (U32)(PMIC_RG_FGCLKSRC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_autocalrate(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_AUTOCALRATE_MASK),
                             (U32)(PMIC_FG_AUTOCALRATE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_cal(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_CAL_MASK),
                             (U32)(PMIC_FG_CAL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_vmode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_VMODE_MASK),
                             (U32)(PMIC_FG_VMODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_FG_ON_MASK),
                             (U32)(PMIC_FG_ON_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_fg_car_35_32(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON1),
                           (&val),
                           (U32)(PMIC_FG_CAR_35_32_MASK),
                           (U32)(PMIC_FG_CAR_35_32_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fg_car_31_16(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON2),
                           (&val),
                           (U32)(PMIC_FG_CAR_31_16_MASK),
                           (U32)(PMIC_FG_CAR_31_16_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fg_car_15_00(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON3),
                           (&val),
                           (U32)(PMIC_FG_CAR_15_00_MASK),
                           (U32)(PMIC_FG_CAR_15_00_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fg_nter_29_16(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON4),
                           (&val),
                           (U32)(PMIC_FG_NTER_29_16_MASK),
                           (U32)(PMIC_FG_NTER_29_16_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fg_nter_15_00(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON5),
                           (&val),
                           (U32)(PMIC_FG_NTER_15_00_MASK),
                           (U32)(PMIC_FG_NTER_15_00_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_fg_bltr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON6),
                             (U32)(val),
                             (U32)(PMIC_FG_BLTR_MASK),
                             (U32)(PMIC_FG_BLTR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_bftr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON7),
                             (U32)(val),
                             (U32)(PMIC_FG_BFTR_MASK),
                             (U32)(PMIC_FG_BFTR_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_fg_current_out(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON8),
                           (&val),
                           (U32)(PMIC_FG_CURRENT_OUT_MASK),
                           (U32)(PMIC_FG_CURRENT_OUT_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_fg_adjust_offset_value(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON9),
                             (U32)(val),
                             (U32)(PMIC_FG_ADJUST_OFFSET_VALUE_MASK),
                             (U32)(PMIC_FG_ADJUST_OFFSET_VALUE_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_fg_offset(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON10),
                           (&val),
                           (U32)(PMIC_FG_OFFSET_MASK),
                           (U32)(PMIC_FG_OFFSET_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_inputclksel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_INPUTCLKSEL_MASK),
                             (U32)(PMIC_RG_INPUTCLKSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_fganalogtest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_FGANALOGTEST_MASK),
                             (U32)(PMIC_RG_FGANALOGTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_spare(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON11),
                             (U32)(val),
                             (U32)(PMIC_RG_SPARE_MASK),
                             (U32)(PMIC_RG_SPARE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_adc_autorst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON12),
                             (U32)(val),
                             (U32)(PMIC_FG_ADC_AUTORST_MASK),
                             (U32)(PMIC_FG_ADC_AUTORST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_adj_offset_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON12),
                             (U32)(val),
                             (U32)(PMIC_FG_ADJ_OFFSET_EN_MASK),
                             (U32)(PMIC_FG_ADJ_OFFSET_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vol_osr_h(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON12),
                             (U32)(val),
                             (U32)(PMIC_VOL_OSR_H_MASK),
                             (U32)(PMIC_VOL_OSR_H_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_vol_osr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON12),
                             (U32)(val),
                             (U32)(PMIC_VOL_OSR_MASK),
                             (U32)(PMIC_VOL_OSR_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_osr_h(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON12),
                             (U32)(val),
                             (U32)(PMIC_FG_OSR_H_MASK),
                             (U32)(PMIC_FG_OSR_H_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_osr(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON12),
                             (U32)(val),
                             (U32)(PMIC_FG_OSR_MASK),
                             (U32)(PMIC_FG_OSR_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rg_fgvmode(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON13),
                           (&val),
                           (U32)(PMIC_RG_FGVMODE_MASK),
                           (U32)(PMIC_RG_FGVMODE_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fg_rst(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON13),
                           (&val),
                           (U32)(PMIC_FG_RST_MASK),
                           (U32)(PMIC_FG_RST_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fgcal_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON13),
                           (&val),
                           (U32)(PMIC_FGCAL_EN_MASK),
                           (U32)(PMIC_FGCAL_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fgadc_en(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON13),
                           (&val),
                           (U32)(PMIC_FGADC_EN_MASK),
                           (U32)(PMIC_FGADC_EN_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_fg_slp_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON13),
                             (U32)(val),
                             (U32)(PMIC_FG_SLP_EN_MASK),
                             (U32)(PMIC_FG_SLP_EN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_fg_adc_rstdetect(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON13),
                           (&val),
                           (U32)(PMIC_FG_ADC_RSTDETECT_MASK),
                           (U32)(PMIC_FG_ADC_RSTDETECT_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fg_h_int_sts(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON13),
                           (&val),
                           (U32)(PMIC_FG_H_INT_STS_MASK),
                           (U32)(PMIC_FG_H_INT_STS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fg_l_int_sts(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON13),
                           (&val),
                           (U32)(PMIC_FG_L_INT_STS_MASK),
                           (U32)(PMIC_FG_L_INT_STS_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_vol_fir1bypass(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON13),
                             (U32)(val),
                             (U32)(PMIC_VOL_FIR1BYPASS_MASK),
                             (U32)(PMIC_VOL_FIR1BYPASS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_fir2bypass(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON13),
                             (U32)(val),
                             (U32)(PMIC_FG_FIR2BYPASS_MASK),
                             (U32)(PMIC_FG_FIR2BYPASS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_fir1bypass(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON13),
                             (U32)(val),
                             (U32)(PMIC_FG_FIR1BYPASS_MASK),
                             (U32)(PMIC_FG_FIR1BYPASS_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_vol_current_out(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON14),
                           (&val),
                           (U32)(PMIC_VOL_CURRENT_OUT_MASK),
                           (U32)(PMIC_VOL_CURRENT_OUT_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_fg_cic2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(FGADC_CON15),
                           (&val),
                           (U32)(PMIC_FG_CIC2_MASK),
                           (U32)(PMIC_FG_CIC2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_fg_slp_cur_th(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON16),
                             (U32)(val),
                             (U32)(PMIC_FG_SLP_CUR_TH_MASK),
                             (U32)(PMIC_FG_SLP_CUR_TH_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_slp_time(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON17),
                             (U32)(val),
                             (U32)(PMIC_FG_SLP_TIME_MASK),
                             (U32)(PMIC_FG_SLP_TIME_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_det_time(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON18),
                             (U32)(val),
                             (U32)(PMIC_FG_DET_TIME_MASK),
                             (U32)(PMIC_FG_DET_TIME_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_srcvolten_ftime(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON18),
                             (U32)(val),
                             (U32)(PMIC_FG_SRCVOLTEN_FTIME_MASK),
                             (U32)(PMIC_FG_SRCVOLTEN_FTIME_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_test_mode1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON19),
                             (U32)(val),
                             (U32)(PMIC_FG_TEST_MODE1_MASK),
                             (U32)(PMIC_FG_TEST_MODE1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_test_mode0(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON19),
                             (U32)(val),
                             (U32)(PMIC_FG_TEST_MODE0_MASK),
                             (U32)(PMIC_FG_TEST_MODE0_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_rsv1(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON19),
                             (U32)(val),
                             (U32)(PMIC_FG_RSV1_MASK),
                             (U32)(PMIC_FG_RSV1_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_vmode_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON19),
                             (U32)(val),
                             (U32)(PMIC_FG_VMODE_SW_MASK),
                             (U32)(PMIC_FG_VMODE_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_fgadc_en_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON19),
                             (U32)(val),
                             (U32)(PMIC_FG_FGADC_EN_SW_MASK),
                             (U32)(PMIC_FG_FGADC_EN_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_fgcal_en_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON19),
                             (U32)(val),
                             (U32)(PMIC_FG_FGCAL_EN_SW_MASK),
                             (U32)(PMIC_FG_FGCAL_EN_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_rst_sw(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON19),
                             (U32)(val),
                             (U32)(PMIC_FG_RST_SW_MASK),
                             (U32)(PMIC_FG_RST_SW_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_fg_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(FGADC_CON19),
                             (U32)(val),
                             (U32)(PMIC_FG_MODE_MASK),
                             (U32)(PMIC_FG_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_stmp_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON0),
                             (U32)(val),
                             (U32)(PMIC_STMP_MODE_MASK),
                             (U32)(PMIC_STMP_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_xosc32_stp_cali(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON0),
                             (U32)(val),
                             (U32)(PMIC_MIX_XOSC32_STP_CALI_MASK),
                             (U32)(PMIC_MIX_XOSC32_STP_CALI_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_xosc32_stp_lpdrst(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON0),
                             (U32)(val),
                             (U32)(PMIC_MIX_XOSC32_STP_LPDRST_MASK),
                             (U32)(PMIC_MIX_XOSC32_STP_LPDRST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_xosc32_stp_lpden(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON0),
                             (U32)(val),
                             (U32)(PMIC_MIX_XOSC32_STP_LPDEN_MASK),
                             (U32)(PMIC_MIX_XOSC32_STP_LPDEN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_mix_xosc32_stp_lpdtb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(RTC_MIX_CON0),
                           (&val),
                           (U32)(PMIC_MIX_XOSC32_STP_LPDTB_MASK),
                           (U32)(PMIC_MIX_XOSC32_STP_LPDTB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_mix_xosc32_stp_pwdb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON0),
                             (U32)(val),
                             (U32)(PMIC_MIX_XOSC32_STP_PWDB_MASK),
                             (U32)(PMIC_MIX_XOSC32_STP_PWDB_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_mix_xosc32_stp_cpdtb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(RTC_MIX_CON0),
                           (&val),
                           (U32)(PMIC_MIX_XOSC32_STP_CPDTB_MASK),
                           (U32)(PMIC_MIX_XOSC32_STP_CPDTB_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_mix_eosc32_opt(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON0),
                             (U32)(val),
                             (U32)(PMIC_MIX_EOSC32_OPT_MASK),
                             (U32)(PMIC_MIX_EOSC32_OPT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_efuse_xosc32_enb_opt(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_mix_efuse_xosc32_enb_opt_MASK),
                             (U32)(PMIC_mix_efuse_xosc32_enb_opt_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_mix_rtc_xosc32_enb(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(RTC_MIX_CON1),
                           (&val),
                           (U32)(PMIC_mix_rtc_xosc32_enb_MASK),
                           (U32)(PMIC_mix_rtc_xosc32_enb_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_mix_stp_rtc_ddlo(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(RTC_MIX_CON1),
                           (&val),
                           (U32)(PMIC_mix_stp_rtc_ddlo_MASK),
                           (U32)(PMIC_mix_stp_rtc_ddlo_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_mix_stp_bbwakeup(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_mix_stp_bbwakeup_MASK),
                             (U32)(PMIC_mix_stp_bbwakeup_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_eosc32_vct_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_MIX_EOSC32_VCT_EN_MASK),
                             (U32)(PMIC_MIX_EOSC32_VCT_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_eosc32_stp_rsv(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_MIX_EOSC32_STP_RSV_MASK),
                             (U32)(PMIC_MIX_EOSC32_STP_RSV_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_dcxo_stp_test_deglitch_mode(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_MIX_DCXO_STP_TEST_DEGLITCH_MODE_MASK),
                             (U32)(PMIC_MIX_DCXO_STP_TEST_DEGLITCH_MODE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_rtc_stp_xosc32_enb(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_MIX_RTC_STP_XOSC32_ENB_MASK),
                             (U32)(PMIC_MIX_RTC_STP_XOSC32_ENB_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_pmu_stp_ddlo_vrtc_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_MIX_PMU_STP_DDLO_VRTC_EN_MASK),
                             (U32)(PMIC_MIX_PMU_STP_DDLO_VRTC_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_pmu_stp_ddlo_vrtc(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_MIX_PMU_STP_DDLO_VRTC_MASK),
                             (U32)(PMIC_MIX_PMU_STP_DDLO_VRTC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_dcxo_stp_lvsh_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_MIX_DCXO_STP_LVSH_EN_MASK),
                             (U32)(PMIC_MIX_DCXO_STP_LVSH_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_mix_eosc32_stp_chop_en(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(RTC_MIX_CON1),
                             (U32)(val),
                             (U32)(PMIC_MIX_EOSC32_STP_CHOP_EN_MASK),
                             (U32)(PMIC_MIX_EOSC32_STP_CHOP_EN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_dc2ac_en_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDAC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_DC2AC_EN_VAUDP12_MASK),
                             (U32)(PMIC_RG_DC2AC_EN_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_aud_dac_pwl_up_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDAC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUD_DAC_PWL_UP_VA28_MASK),
                             (U32)(PMIC_RG_AUD_DAC_PWL_UP_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_aud_dac_pwr_up_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDAC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUD_DAC_PWR_UP_VA28_MASK),
                             (U32)(PMIC_RG_AUD_DAC_PWR_UP_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auddacrpwrup_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDAC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDDACRPWRUP_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDDACRPWRUP_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auddaclpwrup_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDAC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDDACLPWRUP_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDDACLPWRUP_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhprscdisable_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPRSCDISABLE_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPRSCDISABLE_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhplscdisable_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPLSCDISABLE_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPLSCDISABLE_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhsscdisable_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHSSCDISABLE_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHSSCDISABLE_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhprmuxinputsel_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPRMUXINPUTSEL_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPRMUXINPUTSEL_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhplmuxinputsel_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPLMUXINPUTSEL_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPLMUXINPUTSEL_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhsmuxinputsel_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHSMUXINPUTSEL_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHSMUXINPUTSEL_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhprpwrup_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPRPWRUP_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPRPWRUP_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhplpwrup_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPLPWRUP_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPLPWRUP_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhspwrup_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHSPWRUP_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHSPWRUP_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_linenoiseenh_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_LINENOISEENH_VAUDP12_MASK),
                             (U32)(PMIC_RG_LINENOISEENH_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hpout_shortvcm_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_HPOUT_SHORTVCM_VAUDP12_MASK),
                             (U32)(PMIC_RG_HPOUT_SHORTVCM_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hpoutputreset0_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_HPOUTPUTRESET0_VAUDP12_MASK),
                             (U32)(PMIC_RG_HPOUTPUTRESET0_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hpinputreset0_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_HPINPUTRESET0_VAUDP12_MASK),
                             (U32)(PMIC_RG_HPINPUTRESET0_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hpoutputstbenh_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_HPOUTPUTSTBENH_VAUDP12_MASK),
                             (U32)(PMIC_RG_HPOUTPUTSTBENH_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hpinputstbenh_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_HPINPUTSTBENH_VAUDP12_MASK),
                             (U32)(PMIC_RG_HPINPUTSTBENH_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_prechargebuf_en_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_PRECHARGEBUF_EN_VAUDP12_MASK),
                             (U32)(PMIC_RG_PRECHARGEBUF_EN_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audbgbon_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDBGBON_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDBGBON_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhsstartup_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHSSTARTUP_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHSSTARTUP_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhpstartup_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPSTARTUP_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPSTARTUP_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhsbsccurrent_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHSBSCCURRENT_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHSBSCCURRENT_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhprbsccurrent_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPRBSCCURRENT_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPRBSCCURRENT_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhplbsccurrent_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPLBSCCURRENT_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPLBSCCURRENT_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hsout_shortvcm_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG2),
                             (U32)(val),
                             (U32)(PMIC_RG_HSOUT_SHORTVCM_VAUDP12_MASK),
                             (U32)(PMIC_RG_HSOUT_SHORTVCM_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hpoutstb_rsel_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG2),
                             (U32)(val),
                             (U32)(PMIC_RG_HPOUTSTB_RSEL_VAUDP12_MASK),
                             (U32)(PMIC_RG_HPOUTSTB_RSEL_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hsoutputreset0_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG2),
                             (U32)(val),
                             (U32)(PMIC_RG_HSOUTPUTRESET0_VAUDP12_MASK),
                             (U32)(PMIC_RG_HSOUTPUTRESET0_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hsinputreset0_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG2),
                             (U32)(val),
                             (U32)(PMIC_RG_HSINPUTRESET0_VAUDP12_MASK),
                             (U32)(PMIC_RG_HSINPUTRESET0_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hsoutputstbenh_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG2),
                             (U32)(val),
                             (U32)(PMIC_RG_HSOUTPUTSTBENH_VAUDP12_MASK),
                             (U32)(PMIC_RG_HSOUTPUTSTBENH_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hsinputstbenh_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG2),
                             (U32)(val),
                             (U32)(PMIC_RG_HSINPUTSTBENH_VAUDP12_MASK),
                             (U32)(PMIC_RG_HSINPUTSTBENH_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_line_pull0v_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG3),
                             (U32)(val),
                             (U32)(PMIC_RG_LINE_PULL0V_VAUDP12_MASK),
                             (U32)(PMIC_RG_LINE_PULL0V_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhprfinetrim_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG3),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPRFINETRIM_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPRFINETRIM_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhplfinetrim_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG3),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPLFINETRIM_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPLFINETRIM_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhptrim_en_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG3),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPTRIM_EN_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPTRIM_EN_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhprtrim_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG3),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPRTRIM_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPRTRIM_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhpltrim_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG3),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPLTRIM_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDHPLTRIM_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_abidec_reserved_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG4),
                             (U32)(val),
                             (U32)(PMIC_RG_ABIDEC_RESERVED_VAUDP12_MASK),
                             (U32)(PMIC_RG_ABIDEC_RESERVED_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_abidec_reserved_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDBUF_CFG4),
                             (U32)(val),
                             (U32)(PMIC_RG_ABIDEC_RESERVED_VA28_MASK),
                             (U32)(PMIC_RG_ABIDEC_RESERVED_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audibiaspwrdn_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(IBIASDIST_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDIBIASPWRDN_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDIBIASPWRDN_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audbiasadj_1_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(IBIASDIST_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDBIASADJ_1_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDBIASADJ_1_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audbiasadj_0_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(IBIASDIST_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDBIASADJ_0_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDBIASADJ_0_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_chargeoption_depop_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDACCDEPOP_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_CHARGEOPTION_DEPOP_VA28_MASK),
                             (U32)(PMIC_RG_CHARGEOPTION_DEPOP_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_depop_isel_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDACCDEPOP_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_DEPOP_ISEL_VA28_MASK),
                             (U32)(PMIC_RG_DEPOP_ISEL_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_depop_vcmgen_en_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDACCDEPOP_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_DEPOP_VCMGEN_EN_VA28_MASK),
                             (U32)(PMIC_RG_DEPOP_VCMGEN_EN_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_depop_rsel_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDACCDEPOP_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_DEPOP_RSEL_VA28_MASK),
                             (U32)(PMIC_RG_DEPOP_RSEL_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_depop_ren_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDACCDEPOP_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_DEPOP_REN_VA28_MASK),
                             (U32)(PMIC_RG_DEPOP_REN_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audivlmuxsel_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUD_IV_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDIVLMUXSEL_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDIVLMUXSEL_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audivlstartup_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUD_IV_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDIVLSTARTUP_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDIVLSTARTUP_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audivlpwrup_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUD_IV_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDIVLPWRUP_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDIVLPWRUP_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audivlmute_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDCLKGEN_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDIVLMUTE_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDIVLMUTE_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_sel_delay_vcore(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDCLKGEN_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_SEL_DELAY_VCORE_MASK),
                             (U32)(PMIC_RG_SEL_DELAY_VCORE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_sel_encoder_96k_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDCLKGEN_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_SEL_ENCODER_96K_VA28_MASK),
                             (U32)(PMIC_RG_SEL_ENCODER_96K_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_sel_decoder_96k_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDCLKGEN_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_SEL_DECODER_96K_VA28_MASK),
                             (U32)(PMIC_RG_SEL_DECODER_96K_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rstb_encoder_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDCLKGEN_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_RSTB_ENCODER_VA28_MASK),
                             (U32)(PMIC_RG_RSTB_ENCODER_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_rstb_decoder_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDCLKGEN_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_RSTB_DECODER_VA28_MASK),
                             (U32)(PMIC_RG_RSTB_DECODER_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va28refgen_en_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_VA28REFGEN_EN_VA28_MASK),
                             (U32)(PMIC_RG_VA28REFGEN_EN_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_va33refgen_en_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_VA33REFGEN_EN_VA33_MASK),
                             (U32)(PMIC_RG_VA33REFGEN_EN_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbatrefgen_en_vbat(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_VBATREFGEN_EN_VBAT_MASK),
                             (U32)(PMIC_RG_VBATREFGEN_EN_VBAT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_vbatprereg_pddis_en_vbat(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_VBATPREREG_PDDIS_EN_VBAT_MASK),
                             (U32)(PMIC_RG_VBATPREREG_PDDIS_EN_VBAT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lcldo_enc_remote_sense_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_LCLDO_ENC_REMOTE_SENSE_VA28_MASK),
                             (U32)(PMIC_RG_LCLDO_ENC_REMOTE_SENSE_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lcldo_enc_pddis_en_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_LCLDO_ENC_PDDIS_EN_VA28_MASK),
                             (U32)(PMIC_RG_LCLDO_ENC_PDDIS_EN_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lcldo_vosel_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_LCLDO_VOSEL_VA33_MASK),
                             (U32)(PMIC_RG_LCLDO_VOSEL_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lcldo_remote_sense_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_LCLDO_REMOTE_SENSE_VA33_MASK),
                             (U32)(PMIC_RG_LCLDO_REMOTE_SENSE_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_lcldo_pddis_en_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_LCLDO_PDDIS_EN_VA33_MASK),
                             (U32)(PMIC_RG_LCLDO_PDDIS_EN_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hcldo_vosel_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_HCLDO_VOSEL_VA33_MASK),
                             (U32)(PMIC_RG_HCLDO_VOSEL_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hcldo_remote_sense_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_HCLDO_REMOTE_SENSE_VA33_MASK),
                             (U32)(PMIC_RG_HCLDO_REMOTE_SENSE_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_hcldo_pddis_en_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_HCLDO_PDDIS_EN_VA33_MASK),
                             (U32)(PMIC_RG_HCLDO_PDDIS_EN_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpmu_reserved_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPMU_RESERVED_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDPMU_RESERVED_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpmu_reserved_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPMU_RESERVED_VA28_MASK),
                             (U32)(PMIC_RG_AUDPMU_RESERVED_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpmu_reserved_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPMU_RESERVED_VA33_MASK),
                             (U32)(PMIC_RG_AUDPMU_RESERVED_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpmu_reserved_vbat(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLDO_CFG1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPMU_RESERVED_VBAT_MASK),
                             (U32)(PMIC_RG_AUDPMU_RESERVED_VBAT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_da_nvreg_en_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDNVREGGLB_CFG0),
                             (U32)(val),
                             (U32)(PMIC_DA_NVREG_EN_VAUDP12_MASK),
                             (U32)(PMIC_DA_NVREG_EN_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_acc_dcc_sel_audglb_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDNVREGGLB_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_ACC_DCC_SEL_AUDGLB_VA28_MASK),
                             (U32)(PMIC_RG_ACC_DCC_SEL_AUDGLB_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audglb_pwrdn_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDNVREGGLB_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDGLB_PWRDN_VA28_MASK),
                             (U32)(PMIC_RG_AUDGLB_PWRDN_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_nvreg_pull0v_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDNVREGGLB_CFG0),
                             (U32)(val),
                             (U32)(PMIC_RG_NVREG_PULL0V_VAUDP12_MASK),
                             (U32)(PMIC_RG_NVREG_PULL0V_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_ncp_remote_sense_va18(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUD_NCP0),
                             (U32)(val),
                             (U32)(PMIC_RG_NCP_REMOTE_SENSE_VA18_MASK),
                             (U32)(PMIC_RG_NCP_REMOTE_SENSE_VA18_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_da_hcldo_en_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUD_NCP0),
                             (U32)(val),
                             (U32)(PMIC_DA_HCLDO_EN_VA33_MASK),
                             (U32)(PMIC_DA_HCLDO_EN_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_da_lcldo_en_va33(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUD_NCP0),
                             (U32)(val),
                             (U32)(PMIC_DA_LCLDO_EN_VA33_MASK),
                             (U32)(PMIC_DA_LCLDO_EN_VA33_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_da_lcldo_enc_en_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUD_NCP0),
                             (U32)(val),
                             (U32)(PMIC_DA_LCLDO_ENC_EN_VA28_MASK),
                             (U32)(PMIC_DA_LCLDO_ENC_EN_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_da_vbatprereg_en_vbat(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUD_NCP0),
                             (U32)(val),
                             (U32)(PMIC_DA_VBATPREREG_EN_VBAT_MASK),
                             (U32)(PMIC_DA_VBATPREREG_EN_VBAT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreampiddtest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPIDDTEST_MASK),
                             (U32)(PMIC_RG_AUDPREAMPIDDTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreamprpgatest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPRPGATEST_MASK),
                             (U32)(PMIC_RG_AUDPREAMPRPGATEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreamplpgatest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPLPGATEST_MASK),
                             (U32)(PMIC_RG_AUDPREAMPLPGATEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreamprinputsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPRINPUTSEL_MASK),
                             (U32)(PMIC_RG_AUDPREAMPRINPUTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreamplinputsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPLINPUTSEL_MASK),
                             (U32)(PMIC_RG_AUDPREAMPLINPUTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreampron(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPRON_MASK),
                             (U32)(PMIC_RG_AUDPREAMPRON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreamplon(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMP_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPLON_MASK),
                             (U32)(PMIC_RG_AUDPREAMPLON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadc3rdstagereset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADC3RDSTAGERESET_MASK),
                             (U32)(PMIC_RG_AUDADC3RDSTAGERESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadc2ndstagereset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADC2NDSTAGERESET_MASK),
                             (U32)(PMIC_RG_AUDADC2NDSTAGERESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadc2ndstageiddtest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADC2NDSTAGEIDDTEST_MASK),
                             (U32)(PMIC_RG_AUDADC2NDSTAGEIDDTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadc1ststageiddtest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADC1STSTAGEIDDTEST_MASK),
                             (U32)(PMIC_RG_AUDADC1STSTAGEIDDTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcclksel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCCLKSEL_MASK),
                             (U32)(PMIC_RG_AUDADCCLKSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcrinputsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCRINPUTSEL_MASK),
                             (U32)(PMIC_RG_AUDADCRINPUTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadclinputsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCLINPUTSEL_MASK),
                             (U32)(PMIC_RG_AUDADCLINPUTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcrpwrup(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCRPWRUP_MASK),
                             (U32)(PMIC_RG_AUDADCRPWRUP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadclpwrup(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCLPWRUP_MASK),
                             (U32)(PMIC_RG_AUDADCLPWRUP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audrctunelsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDRCTUNELSEL_MASK),
                             (U32)(PMIC_RG_AUDRCTUNELSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audrctunel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDRCTUNEL_MASK),
                             (U32)(PMIC_RG_AUDRCTUNEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcffbypass(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCFFBYPASS_MASK),
                             (U32)(PMIC_RG_AUDADCFFBYPASS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcbypass(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCBYPASS_MASK),
                             (U32)(PMIC_RG_AUDADCBYPASS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcflashiddtest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCFLASHIDDTEST_MASK),
                             (U32)(PMIC_RG_AUDADCFLASHIDDTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcrefbufiddtest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCREFBUFIDDTEST_MASK),
                             (U32)(PMIC_RG_AUDADCREFBUFIDDTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcdaciddtest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCDACIDDTEST_MASK),
                             (U32)(PMIC_RG_AUDADCDACIDDTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcdacfbcurrent(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCDACFBCURRENT_MASK),
                             (U32)(PMIC_RG_AUDADCDACFBCURRENT_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcnodem(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCNODEM_MASK),
                             (U32)(PMIC_RG_AUDADCNODEM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audrctunersel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDRCTUNERSEL_MASK),
                             (U32)(PMIC_RG_AUDRCTUNERSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audrctuner(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDRCTUNER_MASK),
                             (U32)(PMIC_RG_AUDRCTUNER_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadctestdata(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCTESTDATA_MASK),
                             (U32)(PMIC_RG_AUDADCTESTDATA_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcdacnrz(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCDACNRZ_MASK),
                             (U32)(PMIC_RG_AUDADCDACNRZ_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcfsreset(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCFSRESET_MASK),
                             (U32)(PMIC_RG_AUDADCFSRESET_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcdactest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCDACTEST_MASK),
                             (U32)(PMIC_RG_AUDADCDACTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcnopatest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCNOPATEST_MASK),
                             (U32)(PMIC_RG_AUDADCNOPATEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audadcwidecm(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDADCWIDECM_MASK),
                             (U32)(PMIC_RG_AUDADCWIDECM_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audspareva18(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDSPAREVA18_MASK),
                             (U32)(PMIC_RG_AUDSPAREVA18_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audspareva28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON5),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDSPAREVA28_MASK),
                             (U32)(PMIC_RG_AUDSPAREVA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audsparevaudp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDSPAREVAUDP_MASK),
                             (U32)(PMIC_RG_AUDSPAREVAUDP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audsparevmic(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDADC_CON6),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDSPAREVMIC_MASK),
                             (U32)(PMIC_RG_AUDSPAREVMIC_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audmicbiasvref(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDIGMI_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDMICBIASVREF_MASK),
                             (U32)(PMIC_RG_AUDMICBIASVREF_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpwdbmicbias(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDIGMI_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPWDBMICBIAS_MASK),
                             (U32)(PMIC_RG_AUDPWDBMICBIAS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auddigmicbias(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDIGMI_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDDIGMICBIAS_MASK),
                             (U32)(PMIC_RG_AUDDIGMICBIAS_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auddigmicnduty(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDIGMI_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDDIGMICNDUTY_MASK),
                             (U32)(PMIC_RG_AUDDIGMICNDUTY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auddigmicpduty(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDIGMI_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDDIGMICPDUTY_MASK),
                             (U32)(PMIC_RG_AUDDIGMICPDUTY_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_auddigmicen(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDDIGMI_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDDIGMICEN_MASK),
                             (U32)(PMIC_RG_AUDDIGMICEN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbufrmute(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUFRMUTE_MASK),
                             (U32)(PMIC_RG_AUDLSBUFRMUTE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbufrgain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUFRGAIN_MASK),
                             (U32)(PMIC_RG_AUDLSBUFRGAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbuflmute(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUFLMUTE_MASK),
                             (U32)(PMIC_RG_AUDLSBUFLMUTE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbuflgain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUFLGAIN_MASK),
                             (U32)(PMIC_RG_AUDLSBUFLGAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbufrpwrup(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUFRPWRUP_MASK),
                             (U32)(PMIC_RG_AUDLSBUFRPWRUP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbuflpwrup(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUFLPWRUP_MASK),
                             (U32)(PMIC_RG_AUDLSBUFLPWRUP_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbuf2iddtest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUF2IDDTEST_MASK),
                             (U32)(PMIC_RG_AUDLSBUF2IDDTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbufiddtest(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUFIDDTEST_MASK),
                             (U32)(PMIC_RG_AUDLSBUFIDDTEST_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbufrinputsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUFRINPUTSEL_MASK),
                             (U32)(PMIC_RG_AUDLSBUFRINPUTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlsbuflinputsel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDLSBUF_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLSBUFLINPUTSEL_MASK),
                             (U32)(PMIC_RG_AUDLSBUFLINPUTSEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audencspareva18(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDENCSPARE_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDENCSPAREVA18_MASK),
                             (U32)(PMIC_RG_AUDENCSPAREVA18_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audencspareva28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDENCSPARE_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDENCSPAREVA28_MASK),
                             (U32)(PMIC_RG_AUDENCSPAREVA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_clksq_monen_va28(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDENCCLKSQ_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_CLKSQ_MONEN_VA28_MASK),
                             (U32)(PMIC_RG_CLKSQ_MONEN_VA28_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audenc_reserved(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMPGAIN_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDENC_reserved_MASK),
                             (U32)(PMIC_RG_AUDENC_reserved_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreampr_reserved(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMPGAIN_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPR_reserved_MASK),
                             (U32)(PMIC_RG_AUDPREAMPR_reserved_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreamprgain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMPGAIN_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPRGAIN_MASK),
                             (U32)(PMIC_RG_AUDPREAMPRGAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreampl_reserved(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMPGAIN_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPL_reserved_MASK),
                             (U32)(PMIC_RG_AUDPREAMPL_reserved_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audpreamplgain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(AUDPREAMPGAIN_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDPREAMPLGAIN_MASK),
                             (U32)(PMIC_RG_AUDPREAMPLGAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audzcdmuxsel_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDZCDMUXSEL_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDZCDMUXSEL_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audzcdclksel_vaudp12(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDZCDCLKSEL_VAUDP12_MASK),
                             (U32)(PMIC_RG_AUDZCDCLKSEL_VAUDP12_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audzcdtimeoutmodesel(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDZCDTIMEOUTMODESEL_MASK),
                             (U32)(PMIC_RG_AUDZCDTIMEOUTMODESEL_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audzcdgainstepsize(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDZCDGAINSTEPSIZE_MASK),
                             (U32)(PMIC_RG_AUDZCDGAINSTEPSIZE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audzcdgainsteptime(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDZCDGAINSTEPTIME_MASK),
                             (U32)(PMIC_RG_AUDZCDGAINSTEPTIME_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audzcdenable(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDZCDENABLE_MASK),
                             (U32)(PMIC_RG_AUDZCDENABLE_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audlinegain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDLINEGAIN_MASK),
                             (U32)(PMIC_RG_AUDLINEGAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhprgain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPRGAIN_MASK),
                             (U32)(PMIC_RG_AUDHPRGAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhplgain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON2),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHPLGAIN_MASK),
                             (U32)(PMIC_RG_AUDHPLGAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audhsgain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON3),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDHSGAIN_MASK),
                             (U32)(PMIC_RG_AUDHSGAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audivrgain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDIVRGAIN_MASK),
                             (U32)(PMIC_RG_AUDIVRGAIN_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_audivlgain(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(ZCD_CON4),
                             (U32)(val),
                             (U32)(PMIC_RG_AUDIVLGAIN_MASK),
                             (U32)(PMIC_RG_AUDIVLGAIN_SHIFT)
	                         );
  pmic_unlock();
}

U32 upmu_get_rg_audintgain2(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ZCD_CON5),
                           (&val),
                           (U32)(PMIC_RG_AUDINTGAIN2_MASK),
                           (U32)(PMIC_RG_AUDINTGAIN2_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

U32 upmu_get_rg_audintgain1(void)
{
  U32 ret=0;
  U32 val=0;

  pmic_lock();
  ret=pmic_read_interface( (U32)(ZCD_CON5),
                           (&val),
                           (U32)(PMIC_RG_AUDINTGAIN1_MASK),
                           (U32)(PMIC_RG_AUDINTGAIN1_SHIFT)
	                       );
  pmic_unlock();

  return val;
}

void upmu_set_rg_divcks_chg(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(NCP_CLKDIV_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_DIVCKS_CHG_MASK),
                             (U32)(PMIC_RG_DIVCKS_CHG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_divcks_on(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(NCP_CLKDIV_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_DIVCKS_ON_MASK),
                             (U32)(PMIC_RG_DIVCKS_ON_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_divcks_prg(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(NCP_CLKDIV_CON0),
                             (U32)(val),
                             (U32)(PMIC_RG_DIVCKS_PRG_MASK),
                             (U32)(PMIC_RG_DIVCKS_PRG_SHIFT)
	                         );
  pmic_unlock();
}

void upmu_set_rg_pwd_ncp(U32 val)
{
  U32 ret=0;

  pmic_lock();
  ret=pmic_config_interface( (U32)(NCP_CLKDIV_CON1),
                             (U32)(val),
                             (U32)(PMIC_RG_PWD_NCP_MASK),
                             (U32)(PMIC_RG_PWD_NCP_SHIFT)
	                         );
  pmic_unlock();
}


