#include <lcm_drv.h>
extern LCM_DRIVER hx8369_lcm_drv;
extern LCM_DRIVER hx8369_6575_lcm_drv;
extern LCM_DRIVER hx8363_6575_dsi_lcm_drv;
extern LCM_DRIVER hx8363_6575_dsi_hvga_lcm_drv;
extern LCM_DRIVER bm8578_lcm_drv;
extern LCM_DRIVER nt35582_mcu_lcm_drv;
extern LCM_DRIVER nt35582_mcu_6575_lcm_drv;
extern LCM_DRIVER nt35582_rgb_6575_lcm_drv;
extern LCM_DRIVER hx8357b_lcm_drv;
extern LCM_DRIVER hx8369_dsi_lcm_drv;
extern LCM_DRIVER hx8369_dsi_6575_lcm_drv;
extern LCM_DRIVER hx8369_dsi_6575_hvga_lcm_drv;
extern LCM_DRIVER hx8369_dsi_6575_qvga_lcm_drv;
extern LCM_DRIVER hx8369_dsi_vdo_lcm_drv;
extern LCM_DRIVER hx8369b_dsi_vdo_lcm_drv;
extern LCM_DRIVER hx8369_hvga_lcm_drv;
extern LCM_DRIVER ili9481_lcm_drv;
extern LCM_DRIVER nt35582_lcm_drv;
extern LCM_DRIVER s6d0170_lcm_drv;
extern LCM_DRIVER spfd5461a_lcm_drv;
extern LCM_DRIVER ta7601_lcm_drv;
extern LCM_DRIVER tft1p3037_lcm_drv;
extern LCM_DRIVER ha5266_lcm_drv;
extern LCM_DRIVER hsd070idw1_lcm_drv;
extern LCM_DRIVER lg4571_lcm_drv;
extern LCM_DRIVER lvds_wsvga_lcm_drv;
extern LCM_DRIVER lvds_wsvga_ti_lcm_drv;
extern LCM_DRIVER lvds_wsvga_ti_n_lcm_drv;
extern LCM_DRIVER nt35565_3d_lcm_drv;
extern LCM_DRIVER tm070ddh03_lcm_drv;
extern LCM_DRIVER r61408_lcm_drv;
extern LCM_DRIVER nt35510_lcm_drv;
extern LCM_DRIVER nt35510_dpi_lcm_drv;
extern LCM_DRIVER nt35510_hvga_lcm_drv;
extern LCM_DRIVER nt35510_qvga_lcm_drv;
extern LCM_DRIVER nt35510_6517_lcm_drv;
extern LCM_DRIVER r63303_idisplay_lcm_drv;
extern LCM_DRIVER hj080ia_lcm_drv;
extern LCM_DRIVER hj101na02a_lcm_drv;
extern LCM_DRIVER hsd070pfw3_lcm_drv;
extern LCM_DRIVER scf0700m48ggu02_lcm_drv;
extern LCM_DRIVER nt35510_fwvga_lcm_drv;
#if defined(GN_SSD2825_SMD_S6E8AA)
extern LCM_DRIVER gn_ssd2825_smd_s6e8aa;
#endif
extern LCM_DRIVER hx8369_dsi_bld_lcm_drv;
extern LCM_DRIVER hx8369_dsi_tm_lcm_drv;
extern LCM_DRIVER otm8018b_dsi_vdo_lcm_drv;	 
extern LCM_DRIVER nt35512_dsi_vdo_lcm_drv;
extern LCM_DRIVER hx8369_rgb_6585_fpga_lcm_drv;
extern LCM_DRIVER hx8392a_dsi_cmd_lcm_drv;
extern LCM_DRIVER nt35590_hd720_dsi_vdo_truly_lcm_drv;
extern LCM_DRIVER ssd2075_hd720_dsi_vdo_truly_lcm_drv;
extern LCM_DRIVER nt35590_hd720_dsi_cmd_auo_lcm_drv;
extern LCM_DRIVER nt35590_hd720_dsi_cmd_auo_fwvga_lcm_drv;
extern LCM_DRIVER nt35590_hd720_dsi_cmd_auo_qhd_lcm_drv;
extern LCM_DRIVER nt35590_hd720_dsi_cmd_cmi_lcm_drv;
extern LCM_DRIVER nt35516_qhd_dsi_cmd_ipsboe_lcm_drv;
extern LCM_DRIVER nt35516_qhd_dsi_cmd_ipsboe_wvga_lcm_drv;
extern LCM_DRIVER bp070ws1_lcm_drv;
extern LCM_DRIVER bp070ws1_n_lcm_drv;
extern LCM_DRIVER bp101wx1_lcm_drv;
extern LCM_DRIVER bp101wx1_n_lcm_drv;
extern LCM_DRIVER nt35516_qhd_rav4_lcm_drv;
extern LCM_DRIVER r63311_fhd_dsi_vdo_sharp_lcm_drv;
extern LCM_DRIVER nt35596_fhd_dsi_vdo_truly_lcm_drv;
extern LCM_DRIVER lgld070wx3_dsi_vdo_lcm_drv;
extern LCM_DRIVER he080ia_lcm_drv;
//<2013/02/26-22208-stevenchenm Add Himax HX8389-B LCM driver.
extern LCM_DRIVER hx8389b_qhd_dsi_vdo_byd_lcm_drv;
//>2013/02/26-22208-stevenchen

LCM_DRIVER* lcm_driver_list[] = 
{ 
#if defined(HX8369)
	&hx8369_lcm_drv,
#endif

#if defined(HX8369_6575)
	&hx8369_6575_lcm_drv,
#endif

#if defined(BM8578)
	&bm8578_lcm_drv,
#endif

#if defined(NT35582_MCU)
	&nt35582_mcu_lcm_drv,
#endif

#if defined(NT35582_MCU_6575)
	&nt35582_mcu_6575_lcm_drv,
#endif

#if defined(NT35590_HD720_DSI_VDO_TRULY)
	&nt35590_hd720_dsi_vdo_truly_lcm_drv, 
#endif

#if defined(SSD2075_HD720_DSI_VDO_TRULY)
	&ssd2075_hd720_dsi_vdo_truly_lcm_drv, 
#endif


#if defined(NT35590_HD720_DSI_CMD_AUO)
	&nt35590_hd720_dsi_cmd_auo_lcm_drv,
#endif

#if defined(NT35590_HD720_DSI_CMD_AUO_QHD)
	&nt35590_hd720_dsi_cmd_auo_qhd_lcm_drv,
#endif

#if defined(NT35590_HD720_DSI_CMD_AUO_FWVGA)
	&nt35590_hd720_dsi_cmd_auo_fwvga_lcm_drv,
#endif

#if defined(NT35590_HD720_DSI_CMD_CMI)
	&nt35590_hd720_dsi_cmd_cmi_lcm_drv,
#endif

#if defined(NT35582_RGB_6575)
	&nt35582_rgb_6575_lcm_drv,
#endif

#if defined(HX8369_RGB_6585_FPGA)
	&hx8369_rgb_6585_fpga_lcm_drv,
#endif

#if defined(HX8357B)
	&hx8357b_lcm_drv,
#endif

#if defined(R61408)
	&r61408_lcm_drv,
#endif

#if defined(HX8369_DSI_VDO)
	&hx8369_dsi_vdo_lcm_drv,
#endif

#if defined(HX8369_DSI)
	&hx8369_dsi_lcm_drv,
#endif

#if defined(HX8369_6575_DSI)
	&hx8369_dsi_6575_lcm_drv,
#endif

#if defined(HX8369_6575_DSI_NFC_ZTE)
	&hx8369_dsi_6575_lcm_drv,
#endif

#if defined(HX8369_6575_DSI_HVGA)
	&hx8369_dsi_6575_hvga_lcm_drv,
#endif

#if defined(HX8369_6575_DSI_QVGA)
	&hx8369_dsi_6575_qvga_lcm_drv,
#endif

#if defined(HX8369_HVGA)
	&hx8369_hvga_lcm_drv,
#endif

#if defined(NT35510)
	&nt35510_lcm_drv,
#endif

#if defined(NT35510_RGB_6575) 
	&nt35510_dpi_lcm_drv,
#endif	
	

#if defined(NT35510_HVGA)
	&nt35510_hvga_lcm_drv,
#endif

#if defined(NT35510_QVGA)
	&nt35510_qvga_lcm_drv,
#endif

#if defined(NT35510_6517)
	&nt35510_6517_lcm_drv,
#endif

#if defined(ILI9481)
	&ili9481_lcm_drv,
#endif

#if defined(NT35582)
	&nt35582_lcm_drv,
#endif

#if defined(S6D0170)
	&s6d0170_lcm_drv,
#endif

#if defined(SPFD5461A)
	&spfd5461a_lcm_drv,
#endif

#if defined(TA7601)
	&ta7601_lcm_drv,
#endif

#if defined(TFT1P3037)
	&tft1p3037_lcm_drv,
#endif

#if defined(HA5266)
	&ha5266_lcm_drv,
#endif

#if defined(HSD070IDW1)
	&hsd070idw1_lcm_drv,
#endif

#if defined(HX8363_6575_DSI)
	&hx8363_6575_dsi_lcm_drv,
#endif

#if defined(HX8363_6575_DSI_HVGA)
	&hx8363_6575_dsi_hvga_lcm_drv,
#endif

#if defined(LG4571)
	&lg4571_lcm_drv,
#endif

#if defined(LVDS_WSVGA)
	&lvds_wsvga_lcm_drv,
#endif

#if defined(LVDS_WSVGA_TI)
	&lvds_wsvga_ti_lcm_drv,
#endif

#if defined(LVDS_WSVGA_TI_N)
	&lvds_wsvga_ti_n_lcm_drv,
#endif

#if defined(NT35565_3D)
	&nt35565_3d_lcm_drv,
#endif

#if defined(TM070DDH03)
	&tm070ddh03_lcm_drv,
#endif
#if defined(R63303_IDISPLAY)
	&r63303_idisplay_lcm_drv,
#endif

#if defined(HX8369B_DSI_VDO)
	&hx8369b_dsi_vdo_lcm_drv,
#endif

#if defined(GN_SSD2825_SMD_S6E8AA)
	&gn_ssd2825_smd_s6e8aa,
#endif
#if defined(HX8369_TM_DSI)
	&hx8369_dsi_tm_lcm_drv,
#endif

#if defined(HX8369_BLD_DSI)
	&hx8369_dsi_bld_lcm_drv,
#endif

#if defined(HJ080IA)
	&hj080ia_lcm_drv,
#endif

#if defined(HJ101NA02A)
	&hj101na02a_lcm_drv,
#endif

#if defined(HSD070PFW3)
	&hsd070pfw3_lcm_drv,
#endif

#if defined(SCF0700M48GGU02)
	&scf0700m48ggu02_lcm_drv,
#endif

#if defined(OTM8018B_DSI_VDO)	
	&otm8018b_dsi_vdo_lcm_drv, 
#endif

#if defined(NT35512_DSI_VDO)
	&nt35512_dsi_vdo_lcm_drv, 
#endif

#if defined(HX8392A_DSI_CMD)
  &hx8392a_dsi_cmd_lcm_drv,
#endif 

#if defined(NT35516_QHD_DSI_CMD_IPSBOE)
  &nt35516_qhd_dsi_cmd_ipsboe_lcm_drv,
#endif

#if defined(NT35516_QHD_DSI_CMD_IPSBOE_WVGA)
  &nt35516_qhd_dsi_cmd_ipsboe_wvga_lcm_drv,
#endif

#if defined(NT35516_QHD_DSI_VEDIO)
  &nt35516_qhd_rav4_lcm_drv,
#endif

#if defined(BP070WS1)
  &bp070ws1_lcm_drv,
#endif

#if defined(BP070WS1_N)
  &bp070ws1_n_lcm_drv,
#endif

#if defined(BP101WX1)
  &bp101wx1_lcm_drv,
#endif

#if defined(BP101WX1_N)
  &bp101wx1_n_lcm_drv,
#endif

#if defined(NT35510_FWVGA)
  &nt35510_fwvga_lcm_drv,
#endif

#if defined(R63311_FHD_DSI_VDO_SHARP)
	&r63311_fhd_dsi_vdo_sharp_lcm_drv,
#endif

#if defined(NT35596_FHD_DSI_VDO_TRULY)
	&nt35596_fhd_dsi_vdo_truly_lcm_drv,
#endif

#if defined(LGLD070WX3_DSI_VDO)
    &lgld070wx3_dsi_vdo_lcm_drv,
#endif

#if defined(HE080IA)
	&he080ia_lcm_drv,
#endif

//<2013/02/26-22208-stevenchen, Add Himax HX8389-B LCM driver.
#if defined(HX8389B_QHD_DSI_VDO_BYD)
	&hx8389b_qhd_dsi_vdo_byd_lcm_drv,
#endif
//>2013/02/26-22208-stevenchen

};

#define LCM_COMPILE_ASSERT(condition) LCM_COMPILE_ASSERT_X(condition, __LINE__)
#define LCM_COMPILE_ASSERT_X(condition, line) LCM_COMPILE_ASSERT_XX(condition, line)
#define LCM_COMPILE_ASSERT_XX(condition, line) char assertion_failed_at_line_##line[(condition)?1:-1]

unsigned int lcm_count = sizeof(lcm_driver_list)/sizeof(LCM_DRIVER*);
//LCM_COMPILE_ASSERT(0 != sizeof(lcm_driver_list)/sizeof(LCM_DRIVER*));
