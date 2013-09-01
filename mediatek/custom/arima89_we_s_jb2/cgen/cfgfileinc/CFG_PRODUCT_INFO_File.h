


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_PRODUCT_INFO_File.h
 *
 * Project:
 * --------
 *   YuSu
 *
 * Description:
 * ------------
 *    header file of main function
 *
 * Author:
 * -------
 *   Yuchi Xu(MTK81073)
 *
 *------------------------------------------------------------------------------
 *
 *******************************************************************************/



#ifndef _CFG_PRODUCT_INFO_FILE_H
#define _CFG_PRODUCT_INFO_FILE_H


// the record structure define of PRODUCT_INFO nvram file
#define _PRODUCT_INFO_SIZE_ 1024
typedef struct
{
	/*
	PCBA Info
	*/
	char ref_pcba[12];
	char short_code[4];
	char ics[2];
	char site_fac_pcba;
	char line_fac_pcba;
	char date_prod_pcba[3];
	char sn_pcba[4];

	/*
	Handset Info
	*/
	char indus_ref_handset[12];
	char info_ptm[2];
	char site_fac_handset;
	char line_fac_handset;
	char date_prod_handset[3];
	char sn_handset[4];

	/*
	Mini Info
	*/
	char info_pts_mini[3];
	char info_name_mini[20];
	char info_tech_mini[20];

	/*
	Golden Sample
	*/
	char info_golden_flag;
	char info_golden_date[3];

	/*
	HDTB(Reworked PCBA download)
	*/
	char info_id_baie_hdtb[3];
	char info_date_pass_hdtb[3];

	/*
	PT1 Test
	*/
	char info_prod_baie_para_sys[3];
	char info_status_para_sys;
	char info_nbre_pass_para_sys;
	char info_date_pass_para_sys[3];

	/*
	PT2 Test
	*/
	char info_prod_baie_para_sys_2[3];
	char info_status_para_sys_2;
	char info_nbre_pass_para_sys_2;
	char info_date_pass_para_sys_2[3];

	/*
	Bluetooth Test
	*/
	char info_prod_baie_para_sys_3[3];
	char info_status_para_sys_3;
	char info_nbre_pass_para_sys_3;
	char info_date_pass_para_sys_3[3];

	/*
	Wifi Test
	*/
	char info_prod_baie_bw[3];
	char info_status_bw;
	char info_nbre_pass_bw;
	char info_date_baie_bw[3];

	/*
	GPS Test
	*/
	char info_prod_baie_gps[3];
	char info_status_gps;
	char info_nbre_pass_gps;
	char info_date_baie_gps[3];

	/*
	MMI Test
	*/
	char info_status_mmi_test;

	/*
	Final Test(Antenna Test)
	*/
	char info_prod_baie_final[3];
	char info_status_final;
	char info_nbre_pass_final;
	char info_date_baie_final[3];

	/*
	Final Test2(Antenna Test)
	*/
	char info_prod_baie_final_2[3];
	char info_status_final_2;
	char info_nbre_pass_final_2;
	char info_date_baie_final_2[3];

	/*
	HDT (CU perso download)
	*/
	char info_id_baie_hdt[3];
	char info_date_pass_hdt[3];

	/*
	CU SW Info
	*/
	char info_comm_ref[20];
	char info_pts_appli[3];
	char info_name_appli[20];
	char info_name_perso1[20];
	char info_name_perso2[20];
	char info_name_perso3[20];
	char info_name_perso4[20];
	char info_spare_region[20];
	/*
	test bit
	*/
	//int test;
}ap_nvram_trace_config_struct;

typedef struct
{
	unsigned char bt_addr[6];
	unsigned char wifi_addr[6];

} BT_WIFI_ADDR;

typedef struct
{
    unsigned char imei[8];
    unsigned char svn;
    unsigned char pad;
} nvram_ef_imei_imeisv_struct;


typedef struct
{
	unsigned char space[_PRODUCT_INFO_SIZE_\
		                -sizeof(ap_nvram_trace_config_struct)\
		                -sizeof(BT_WIFI_ADDR) \
				-4*sizeof(nvram_ef_imei_imeisv_struct)];
} RESERVED_S;

typedef struct
{
	ap_nvram_trace_config_struct trace_nvram_data;
	unsigned char bt_addr[6];
	unsigned char wifi_addr[6];
	nvram_ef_imei_imeisv_struct imei_svn[4];
	RESERVED_S reserved;
	
}PRODUCT_INFO;    //JRD TRACE STRUCT

//the record size and number of PRODUCT_INFO nvram file
#define CFG_FILE_PRODUCT_INFO_SIZE    sizeof(PRODUCT_INFO)
#define CFG_FILE_PRODUCT_INFO_TOTAL   1

#endif /* _CFG_PRODUCT_INFO_FILE_H */
