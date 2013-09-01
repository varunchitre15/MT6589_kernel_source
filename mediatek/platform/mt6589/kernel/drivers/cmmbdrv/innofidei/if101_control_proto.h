#ifndef _IF101_CONTROL_PROTO_H_
#define _IF101_CONTROL_PROTO_H_
#include <linux/types.h>

/*
 * IIC Communication Register
 */
#define I2C_CMD_DATA_REG		0x298
#define I2C_CMD_STATUS_REG		0x299
#define I2C_RESPONSE_DATA_REG	0x29A
#define I2C_RESPONSE_STATUS_REG	0x29B

/*
 * IIC Communication Register Bits
 */
#define CMD_STATUS_HDEN			0x80	//Command Data Enable.  AP write set, IF101 read clear
#define CMD_STATUS_CMDSEL		0x04	//Command Select. AP set if current I2C_CMD_DATA_REG value is cmd code.
#define RESPONSE_STATUS_CDEN	0x80	//Client Data Enable. Set by IF101 then AP can read current I2C_RESPONSE_STATUS_REG value, 
										// after read,  AP should clearthis bit.

/*
 * COMMAND CODE
 */
#define CMD_GET_CONFIG			0x01
#define CMD_SET_PM				0x02
#define CMD_GET_FW_VER			0x04
#define CMD_SET_LG_CONFIG		0x05
#define CMD_SET_FREQUENCY		0x06
#define CMD_SCAN_FREQUENCY		0x09
#define CMD_GET_ERROR_INFO		0x0A
#define CMD_GET_FW_CHECK_SUM		0x0B
#define CMD_UPDATE_FW			0x81
/*
 * COMMAND FRAME FIELD TYPE
 */
#define CMD_FIELD_CODE		(CMD_STATUS_HDEN | CMD_STATUS_CMDSEL)
#define CMD_FIELD_DATA		(CMD_STATUS_HDEN)
#define CMD_FIELD_CHECKSUM	(CMD_STATUS_HDEN)

/**********************************************************************************************************
 *			CMD and RSP status bit
**********************************************************************************************************/
#define CMD_CODE				0x84
#define CMD_PAYLOAD				0x80
#define RSP_DATA_VALID				0x80
#define CMD_BUSY				0x80

struct inno_cmd_frame {
	u8	code;
	u8	data[6];
	u8	checksum;
};

struct inno_response_frame {
	u8	code;
	u8	data[6];
	u8	checksum;
};


/*
 * CMD_SET_PM
 */
#define PM_SYS_SUSPEND_BIT		0x80
#define PM_SYS_RESUME_BIT		0x40
#define PM_STUN_OFF_BIT			0x20
#define PM_STUN_ON_BIT			0x10
#define PM_UTUN_OFF_BIT			0x08
#define PM_UTUN_ON_BIT			0x04
#define PM_TSPWR_OFF_BIT		0x02
#define PM_TSPWR_ON_BIT			0x01
/*
 * CMD_GET_SYS_STATE
 */
#define STATTYPE_SYNC_STATE	        0x00
#define STATTYPE_SIGNAL_STRENGTH	0x01
#define STATTYPE_LDPC_TOTAL_COUNT	0x02
#define STATTYPE_LDPC_ERR_COUNT		0x03
#define STATTYPE_RS_TOTAL_COUNT		0x04
#define STATTYPE_RS_ERR_COUNT		0x05
#define STATTYPE_SIGNAL_QUALITY		0x06
#define FW_ERR_STATUS		        0x07                                    //xingyu add
#define STATTYPE_BER_COUNT              0x08                                    //xingyu add
#define STATTYPE_SNR_COUNT              0x09                                   //xingyu add
/*
 * CMD_GET_FW_VER
 */

/*
 * CMD_SET_LG_CONFIG
 */


#define cmd_frame_checksum(cmd_frame)				inno_gen_checksum((u8*)cmd_frame, 7)
#define response_frame_checksum(response_frame)		inno_gen_checksum((u8*)response_frame, 7)
/*
 * Generate checksum of command frame or response frame
 */
static __inline u8 inno_gen_checksum(u8* data, u8 size)
{
	u8 checksum = 0xA5;
	int i=0;

	for(i=0; i<size; i++){
		checksum ^= data[i];
	}
	return checksum;
}

static const u16 freq_mhz_table[] = {
// [0]  [1]  [2]  [3]  [4]  [5]  [6]  [7]  [8]  [9]  
	0,   0,   0,   0,   0,   0,   0,   0,   0,   0,			//0-9
	0,   0,   0,   474, 482, 490, 498, 506, 514, 522, 		//10-19
	530, 538, 546, 554, 562, 610, 618, 626, 634, 642,	 	//20-29
	650, 658, 666, 674, 682, 690, 698, 706, 714, 722, 		//30-39
	730, 738, 746, 754, 762, 770, 778, 786, 794, 802, 		//40-49
	810, 818, 826, 834, 842, 850, 858, 866, 874, 0,			//50-59
	882, 890, 898, 906, 914, 922, 930, 938, 954, 0		//60-69	
};

/*
 * Map frequency in MHZ to frequency dot
 */
static __inline u8 inno_freq2dot(u16 freq_mhz)
{
#if 0
	u8 i = 0;
	int table_size = sizeof(freq_mhz_table)/sizeof(u16);

	for(i=13; i<table_size; i++){
		if(freq_mhz_table[i] && freq_mhz_table[i]==freq_mhz)
			return i;
	}
#else
	if(freq_mhz>=470 && freq_mhz<566)		//13 ~ 24
		return (13 + ((freq_mhz-470)>>3));
	else if(freq_mhz>=606 && freq_mhz<878)	//25 ~ 58
		return (25 + ((freq_mhz-606)>>3));	
	else if(freq_mhz>=878 && freq_mhz<942)	//60 ~ 67
		return (60 + ((freq_mhz-878)>>3));
	else if(freq_mhz>=950 && freq_mhz<958)	// 68
		return 68;
#endif
	return 0;
}


/*
 * Functions
 */
//int inno_scan_frequency_dot(u8 start_fdot, u8 end_fdot);
//int inno_scan_frequency(u16 start_freq, u16 end_freq);
int inno_set_frequency_dot(u8 freq_dot);
int inno_set_frequency(u16 freq_mhz);
int inno_set_channel_config(struct inno_channel_config* cfg);
int INNO_GetChannelConfig(struct inno_channel_config* cfg);
int inno_get_system_state(struct inno_sys_state* state);
int inno_set_pm(struct inno_pm_control* pmctl);
int inno_get_fireware_version(struct inno_fw_ver* version);
int inno_update_fireware(void);
int inno_download_fireware(unsigned char *fw_bin, int nsize);
INNO_RET INNO_CheckFWDownload(unsigned char *buffer, int len, int *pass);
int inno_set_cp_type(u8 cp_type);
int inno_uam_transfer(struct inno_uam_parameter *uam);

//FIHTDC, ALXiao, provide API to check frequency valid or not {
//FIHTDC, ALXiao, provide API to check frequency valid or not }
#endif	//_IF101_CONTROL_PROTO_H_
