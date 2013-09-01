// ---------------------------------------------------------------------------

#ifndef     HDMITX_H
#define     HDMITX_H

#define HDMI_CHECK_RET(expr)                                                \
    do {                                                                    \
        HDMI_STATUS ret = (expr);                                           \
        if (HDMI_STATUS_OK != ret) {                                        \
            printk("[ERROR][mtkfb] HDMI API return error code: 0x%x\n"      \
                   "  file : %s, line : %d\n"                               \
                   "  expr : %s\n", ret, __FILE__, __LINE__, #expr);        \
        }                                                                   \
    } while (0)


typedef enum
{	
   HDMI_STATUS_OK = 0,

   HDMI_STATUS_NOT_IMPLEMENTED,
   HDMI_STATUS_ALREADY_SET,
   HDMI_STATUS_ERROR,
} HDMI_STATUS;
		
typedef enum
{	
   HDMI_POWER_STATE_OFF = 0,
   HDMI_POWER_STATE_ON,
   HDMI_POWER_STATE_STANDBY,
} HDMI_POWER_STATE;

typedef struct
{	
	
} HDMI_CAPABILITY;



typedef struct
{
	bool is_audio_enabled;
	bool is_video_enabled;
} hdmi_device_status;

struct hdmi_video_buffer_info
{
    void* src_vir_addr;
    unsigned int src_size;
};

#ifdef MTK_MT8193_HDMI_SUPPORT
typedef struct
{
	unsigned int u4Addr;
	unsigned int u4Data;
} hdmi_device_write;

typedef struct
{
	unsigned int u4Data1;
	unsigned int u4Data2;
} hdmi_para_setting;

typedef struct
{
	unsigned char u1Hdcpkey[287];
} hdmi_hdcp_key;

typedef struct
{
	unsigned char u1sendsltdata[15];
} send_slt_data;

typedef   struct  _HDMI_EDID_T
{
  unsigned int ui4_ntsc_resolution;//use EDID_VIDEO_RES_T, there are many resolution
  unsigned int ui4_pal_resolution;// use EDID_VIDEO_RES_T
  unsigned int ui4_sink_native_ntsc_resolution;//use EDID_VIDEO_RES_T, only one NTSC resolution, Zero means none native NTSC resolution is avaiable
  unsigned int ui4_sink_native_pal_resolution; //use EDID_VIDEO_RES_T, only one resolution, Zero means none native PAL resolution is avaiable
  unsigned int ui4_sink_cea_ntsc_resolution;//use EDID_VIDEO_RES_T
  unsigned int ui4_sink_cea_pal_resolution;//use EDID_VIDEO_RES_T
  unsigned int ui4_sink_dtd_ntsc_resolution;//use EDID_VIDEO_RES_T
  unsigned int ui4_sink_dtd_pal_resolution;//use EDID_VIDEO_RES_T
  unsigned int ui4_sink_1st_dtd_ntsc_resolution;//use EDID_VIDEO_RES_T
  unsigned int ui4_sink_1st_dtd_pal_resolution;//use EDID_VIDEO_RES_T
  unsigned short ui2_sink_colorimetry;//use EDID_VIDEO_COLORIMETRY_T
  unsigned char ui1_sink_rgb_color_bit;//color bit for RGB
  unsigned char ui1_sink_ycbcr_color_bit; // color bit for YCbCr
  unsigned short ui2_sink_aud_dec;// use EDID_AUDIO_DECODER_T
  unsigned char ui1_sink_is_plug_in;//1: Plug in 0:Plug Out
  unsigned int ui4_hdmi_pcm_ch_type;//use EDID_A_FMT_CH_TYPE
  unsigned int ui4_hdmi_pcm_ch3ch4ch5ch7_type;//use EDID_A_FMT_CH_TYPE1
  unsigned int ui4_dac_pcm_ch_type;//use EDID_A_FMT_CH_TYPE
  unsigned char ui1_sink_i_latency_present;
  unsigned char ui1_sink_p_audio_latency;
  unsigned char ui1_sink_p_video_latency;
  unsigned char ui1_sink_i_audio_latency;
  unsigned char ui1_sink_i_video_latency;
  unsigned char ui1ExtEdid_Revision;
  unsigned char ui1Edid_Version;
  unsigned char ui1Edid_Revision;
  unsigned char ui1_Display_Horizontal_Size;
  unsigned char ui1_Display_Vertical_Size;
  unsigned int ui4_ID_Serial_Number;
  unsigned int ui4_sink_cea_3D_resolution;
  unsigned char ui1_sink_support_ai;//0: not support AI, 1:support AI
  unsigned short ui2_sink_cec_address;
  unsigned short ui1_sink_max_tmds_clock;
  unsigned short ui2_sink_3D_structure;
  unsigned int ui4_sink_cea_FP_SUP_3D_resolution;
  unsigned int ui4_sink_cea_TOB_SUP_3D_resolution;
  unsigned int ui4_sink_cea_SBS_SUP_3D_resolution;
  unsigned short ui2_sink_ID_manufacturer_name;//(08H~09H)
  unsigned short ui2_sink_ID_product_code;           //(0aH~0bH)
  unsigned int ui4_sink_ID_serial_number;         //(0cH~0fH)
  unsigned char  ui1_sink_week_of_manufacture;   //(10H)
  unsigned char  ui1_sink_year_of_manufacture;   //(11H)  base on year 1990
}   HDMI_EDID_T;

typedef struct 
{
    unsigned char       ui1_la_num;
   	unsigned char     e_la[3];
    unsigned short     ui2_pa;
    unsigned short    h_cecm_svc;
} CEC_DRV_ADDR_CFG;

typedef struct
{
	unsigned char destination : 4;
	unsigned char initiator   : 4;
} CEC_HEADER_BLOCK_IO;

typedef struct
{
	CEC_HEADER_BLOCK_IO header;
	unsigned char opcode;
	unsigned char operand[15];
} CEC_FRAME_BLOCK_IO;

typedef struct
{
	unsigned char size;
	unsigned char sendidx;
	unsigned char reTXcnt;
	void* txtag;
	CEC_FRAME_BLOCK_IO blocks;
} CEC_FRAME_DESCRIPTION_IO;

typedef struct _CEC_FRAME_INFO
{
    unsigned char       ui1_init_addr;
    unsigned char       ui1_dest_addr;
    unsigned short      ui2_opcode;
    unsigned char       aui1_operand[14];
    unsigned int      z_operand_size;
}   CEC_FRAME_INFO;

typedef struct _CEC_SEND_MSG
{
	CEC_FRAME_INFO	    t_frame_info;
	unsigned char       b_enqueue_ok;
}CEC_SEND_MSG;

typedef struct 
{
	unsigned char		ui1_la;
	unsigned short	   ui2_pa;
}	CEC_ADDRESS_IO;

typedef struct 
{
	unsigned char	u1Size;
	unsigned char	au1Data[14];
}	CEC_GETSLT_DATA;

typedef struct 
{
	unsigned int	u1adress;
	unsigned int	pu1Data;
}	READ_REG_VALUE;

#endif
extern unsigned int mtkfb_get_fb_phys_addr(void);
extern unsigned int mtkfb_get_fb_size(void);
extern unsigned int mtkfb_get_fb_va(void);

#define HDMI_IOW(num, dtype)     _IOW('H', num, dtype)
#define HDMI_IOR(num, dtype)     _IOR('H', num, dtype)
#define HDMI_IOWR(num, dtype)    _IOWR('H', num, dtype)
#define HDMI_IO(num)             _IO('H', num)

#define MTK_HDMI_AUDIO_VIDEO_ENABLE			HDMI_IO(1)
#define MTK_HDMI_AUDIO_ENABLE							HDMI_IO(2)
#define MTK_HDMI_VIDEO_ENABLE							HDMI_IO(3)
#define MTK_HDMI_GET_CAPABILITY						HDMI_IOWR(4, HDMI_CAPABILITY)
#define MTK_HDMI_GET_DEVICE_STATUS				HDMI_IOWR(5, hdmi_device_status)
#define MTK_HDMI_VIDEO_CONFIG							HDMI_IOWR(6, int)
#define MTK_HDMI_AUDIO_CONFIG							HDMI_IOWR(7, int)
#define MTK_HDMI_FORCE_FULLSCREEN_ON		HDMI_IOWR(8, int)
#define MTK_HDMI_FORCE_FULLSCREEN_OFF	HDMI_IOWR(9, int)
#define MTK_HDMI_IPO_POWEROFF	        			HDMI_IOWR(10, int)
#define MTK_HDMI_IPO_POWERON	        				HDMI_IOWR(11, int)
#define MTK_HDMI_POWER_ENABLE           			HDMI_IOW(12, int)
#define MTK_HDMI_PORTRAIT_ENABLE        			HDMI_IOW(13, int)
#define MTK_HDMI_FORCE_OPEN							HDMI_IOWR(14, int)
#define MTK_HDMI_FORCE_CLOSE							HDMI_IOWR(15, int)
#define MTK_HDMI_IS_FORCE_AWAKE                 HDMI_IOWR(16, int)
#define MTK_HDMI_ENTER_VIDEO_MODE               HDMI_IO(17)
#define MTK_HDMI_LEAVE_VIDEO_MODE               HDMI_IO(18)
#define MTK_HDMI_REGISTER_VIDEO_BUFFER          HDMI_IOW(19, struct hdmi_video_buffer_info)
#define MTK_HDMI_POST_VIDEO_BUFFER              HDMI_IOW(20, struct hdmi_video_buffer_info)
#define MTK_HDMI_FACTORY_MODE_ENABLE            HDMI_IOW(21, int)
#ifdef MTK_MT8193_HDMI_SUPPORT
#define MTK_HDMI_WRITE_DEV           HDMI_IOWR(22, hdmi_device_write)
#define MTK_HDMI_READ_DEV            HDMI_IOWR(23, unsigned int)
#define MTK_HDMI_ENABLE_LOG          HDMI_IOWR(24, unsigned int)
#define MTK_HDMI_CHECK_EDID          HDMI_IOWR(25, unsigned int)
#define MTK_HDMI_INFOFRAME_SETTING   HDMI_IOWR(26, hdmi_para_setting)
#define MTK_HDMI_COLOR_DEEP          HDMI_IOWR(27, hdmi_para_setting)
#define MTK_HDMI_ENABLE_HDCP         HDMI_IOWR(28, unsigned int)
#define MTK_HDMI_STATUS           HDMI_IOWR(29, unsigned int)
#define MTK_HDMI_HDCP_KEY         HDMI_IOWR(30, hdmi_hdcp_key)
#define MTK_HDMI_GET_EDID         HDMI_IOWR(31, HDMI_EDID_T)
#define MTK_HDMI_SETLA            HDMI_IOWR(32, CEC_DRV_ADDR_CFG)
#define MTK_HDMI_GET_CECCMD       HDMI_IOWR(33, CEC_FRAME_DESCRIPTION_IO)
#define MTK_HDMI_SET_CECCMD       HDMI_IOWR(34, CEC_SEND_MSG)
#define MTK_HDMI_CEC_ENABLE       HDMI_IOWR(35, unsigned int)
#define MTK_HDMI_GET_CECADDR      HDMI_IOWR(36, CEC_ADDRESS_IO)
#define MTK_HDMI_CECRX_MODE       HDMI_IOWR(37, unsigned int)
#define MTK_HDMI_SENDSLTDATA      HDMI_IOWR(38, send_slt_data)
#define MTK_HDMI_GET_SLTDATA      HDMI_IOWR(39, CEC_GETSLT_DATA)
#endif


struct ext_memory_info{
	unsigned int buffer_num;
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
};

struct ext_buffer{
	unsigned int id;
	unsigned int ts_sec;
	unsigned int ts_nsec;
};

#define MTK_EXT_DISPLAY_ENTER									HDMI_IO(40)
#define MTK_EXT_DISPLAY_LEAVE									HDMI_IO(41)
#define MTK_EXT_DISPLAY_START									HDMI_IO(42)
#define MTK_EXT_DISPLAY_STOP									HDMI_IO(43)
#define MTK_EXT_DISPLAY_SET_MEMORY_INFO		HDMI_IOW(44, struct ext_memory_info)
#define MTK_EXT_DISPLAY_GET_MEMORY_INFO		HDMI_IOW(45, struct ext_memory_info)
#define MTK_EXT_DISPLAY_GET_BUFFER						HDMI_IOW(46, struct ext_buffer)
#define MTK_EXT_DISPLAY_FREE_BUFFER						HDMI_IOW(47, struct ext_buffer)



enum HDMI_report_state
{
    NO_DEVICE =0,
    HDMI_PLUGIN = 1,
};



bool is_hdmi_enable(void);
void hdmi_setorientation(int orientation);
void hdmi_suspend(void);
void hdmi_resume(void);
void hdmi_power_on(void);
void hdmi_power_off(void);
void hdmi_update_buffer_switch(void);
void hdmi_update(void);
void hdmi_dpi_config_clock(void);
void hdmi_dpi_power_switch(bool enable);
int hdmi_audio_config(int samplerate);
int hdmi_video_enable(bool enable);
int hdmi_audio_enable(bool enable);
int hdmi_audio_delay_mute(int latency);
void hdmi_set_mode(unsigned char ucMode);
void hdmi_reg_dump(void);
#ifdef MTK_MT8193_HDMI_SUPPORT
void hdmi_read_reg(unsigned char u8Reg, unsigned int *p4Data);
#else
void hdmi_read_reg(unsigned char u8Reg);
#endif
void hdmi_write_reg(unsigned char u8Reg, unsigned char u8Data);

#endif
