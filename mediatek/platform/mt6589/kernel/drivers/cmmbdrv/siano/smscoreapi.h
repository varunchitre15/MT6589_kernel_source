/****************************************************************

Siano Mobile Silicon, Inc.
MDTV receiver kernel modules.
Copyright (C) 2006-2010, Erez Cohen

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

 This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

****************************************************************/

#ifndef __SMS_CORE_API_H__
#define __SMS_CORE_API_H__

#include <linux/version.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/compat.h>
#include <linux/wait.h>
#include <linux/timer.h>

#include <asm/page.h>

#include "smsir.h"
#include "smsspicommon.h"

//some funtion define
#define AUTO_CREATE_DEV            //auto create dev
#define MTK_SPI
//#define mtk_fp_spi
#define _FW_SIZE               (100*1024)          // static buffer size for fw

#define SMS_HOSTLIB_SUBSYS                                           //xingyu add

#define kmutex_init(_p_) mutex_init(_p_)
#define kmutex_lock(_p_) mutex_lock(_p_)
#define kmutex_trylock(_p_) mutex_trylock(_p_)
#define kmutex_unlock(_p_) mutex_unlock(_p_)

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define SMS_PROTOCOL_MAX_RAOUNDTRIP_MS			(10000)
#define SMS_ALLOC_ALIGNMENT				128
#define SMS_DMA_ALIGNMENT				16
#define SMS_ALIGN_ADDRESS(addr) \
	((((uintptr_t)(addr)) + (SMS_DMA_ALIGNMENT-1)) & ~(SMS_DMA_ALIGNMENT-1))

#define RX_BUFFER_SIZE			(0x1000 + SPI_PACKET_SIZE + 0x100)       //1202
#define SMS_DEVICE_FAMILY2				1
#define SMS_ROM_NO_RESPONSE				2
#define SMS_DEVICE_NOT_READY				0x8000000

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 10)
//#define REQUEST_FIRMWARE_SUPPORTED                              //1202 KO VERSION
//#define LOOPBACK                                                // loopback test
#define DEFAULT_FW_FILE_PATH "/data"
#else
#define DEFAULT_FW_FILE_PATH "/lib/firmware"
#endif

enum sms_device_type_st {
	SMS_UNKNOWN_TYPE = -1,
	SMS_STELLAR = 0,
	SMS_NOVA_A0,
	SMS_NOVA_B0,
	SMS_VEGA,
	SMS_VENICE,
	SMS_MING,
	SMS_PELE,
	SMS_RIO,
	SMS_DENVER_1530,
	SMS_DENVER_2160,
	SMS_NUM_OF_DEVICE_TYPES
};

struct smscore_device_t;
struct smscore_client_t;
struct smscore_buffer_t;

typedef int (*hotplug_t)(struct smscore_device_t *coredev,
			 struct device *device, int arrival);

typedef int (*setmode_t)(void *context, int mode);
typedef void (*detectmode_t)(void *context, int *mode);
typedef int (*sendrequest_t)(void *context, void *buffer, size_t size);
typedef int (*loadfirmware_t)(void *context, void *buffer, size_t size);
typedef int (*preload_t)(void *context);
typedef int (*postload_t)(void *context);

typedef int (*onresponse_t)(void *context, struct smscore_buffer_t *cb);
typedef void (*onremove_t)(void *context);

struct smsmdtv_version_t {
	int major;
	int minor;
	int revision;
};

struct smscore_buffer_t {
	/* public members, once passed to clients can be changed freely */
	struct list_head entry;
	int size;
	int offset;

	/* private members, read-only for clients */
	void *p;
	dma_addr_t phys;
	unsigned long offset_in_common;
};

struct smsdevice_params_t {
	struct device *device;

	int buffer_size;
	int num_buffers;

	char devpath[32];
	unsigned long flags;

	setmode_t setmode_handler;
	detectmode_t detectmode_handler;
	sendrequest_t sendrequest_handler;
	preload_t preload_handler;
	postload_t postload_handler;

	void *context;
	enum sms_device_type_st device_type;
};

struct smsclient_params_t {
	int initial_id;
	int data_type;
	onresponse_t onresponse_handler;
	onremove_t onremove_handler;
	void *context;
};

struct smscore_device_t {
	struct list_head entry;

	struct list_head clients;
	struct list_head subclients;
	spinlock_t clientslock;

	struct list_head buffers;
	spinlock_t bufferslock;
	int num_buffers;

	void *common_buffer;
	int common_buffer_size;
	dma_addr_t common_buffer_phys;

	void *context;
	struct device *device;

	char devpath[32];
	unsigned long device_flags;

	setmode_t setmode_handler;
	detectmode_t detectmode_handler;
	sendrequest_t sendrequest_handler;
	preload_t preload_handler;
	postload_t postload_handler;

	int mode, modes_supported;

	/* host <--> device messages */
	struct completion version_ex_done, data_download_done, trigger_done,clock_done;                  //clock msg clock_down
	struct completion init_device_done, reload_start_done, resume_done, device_ready_done;
	struct completion gpio_configuration_done, gpio_set_level_done;
	struct completion gpio_get_level_done, ir_init_done;
#if 1 //JACKZ
	struct completion periodic_stat_done, scan_done, scan_complete;
	struct completion start_service_done, dab_count_done, loopback_res_done;
#endif
	/* Buffer management */
	wait_queue_head_t buffer_mng_waitq;

	/* GPIO */
	int gpio_get_res;

	/* Target hardware board */
	int board_id;

	/* Firmware */
	u8 *fw_buf;
	u32 fw_buf_size;

	/* Infrared (IR) */
	struct ir_t ir;
};

/* GPIO definitions for antenna frequency domain control (SMS8021) */
#define SMS_ANTENNA_GPIO_0				1
#define SMS_ANTENNA_GPIO_1				0

#define BW_8_MHZ					0
#define BW_7_MHZ					1
#define BW_6_MHZ					2
#define BW_5_MHZ					3
#define BW_ISDBT_1SEG					4
#define BW_ISDBT_3SEG					5
#define BW_ISDBT_13SEG                  6

#define MSG_HDR_FLAG_SPLIT_MSG				4

#define MAX_GPIO_PIN_NUMBER				31

#define HIF_TASK					11
#define SMS_HOST_LIB					150
#define DVBT_BDA_CONTROL_MSG_ID				201

#define SMS_MAX_PAYLOAD_SIZE				240
#define SMS_TUNE_TIMEOUT				500

#define MSG_SMS_GPIO_CONFIG_REQ				507
#define MSG_SMS_GPIO_CONFIG_RES				508
#define MSG_SMS_GPIO_SET_LEVEL_REQ			509
#define MSG_SMS_GPIO_SET_LEVEL_RES			510
#define MSG_SMS_GPIO_GET_LEVEL_REQ			511
#define MSG_SMS_GPIO_GET_LEVEL_RES			512
#define MSG_SMS_SET_MAX_TX_MSG_LEN_REQ			516
#define MSG_SMS_SET_MAX_TX_MSG_LEN_RES			517
#define MSG_SMS_RF_TUNE_REQ				561
#define MSG_SMS_RF_TUNE_RES				562
#define MSG_SMS_INIT_DEVICE_REQ				578
#define MSG_SMS_INIT_DEVICE_RES				579
#define MSG_SMS_ADD_PID_FILTER_REQ			601
#define MSG_SMS_ADD_PID_FILTER_RES			602
#define MSG_SMS_REMOVE_PID_FILTER_REQ			603
#define MSG_SMS_REMOVE_PID_FILTER_RES			604
#define MSG_SMS_DAB_CHANNEL				607
#define MSG_SMS_GET_PID_FILTER_LIST_REQ			608
#define MSG_SMS_GET_PID_FILTER_LIST_RES			609
#define MSG_SMS_SCAN_START_REQ			618
#define MSG_SMS_SCAN_START_RES			619
#define MSG_SMS_SCAN_COMPLETE_IND		623
#define MSG_SMS_HO_PER_SLICES_IND			630
#define MSG_SMS_SET_ANTENNA_CONFIG_REQ			651
#define MSG_SMS_SET_ANTENNA_CONFIG_RES			652
#define MSG_SMS_GET_STATISTICS_EX_REQ			653
#define MSG_SMS_GET_STATISTICS_EX_RES			654
#define MSG_SMS_SLEEP_RESUME_COMP_IND			655
#define MSG_SMS_DATA_DOWNLOAD_REQ			660
#define MSG_SMS_DATA_DOWNLOAD_RES			661
#define MSG_SMS_SWDOWNLOAD_TRIGGER_REQ			664
#define MSG_SMS_SWDOWNLOAD_TRIGGER_RES			665
#define MSG_SMS_SWDOWNLOAD_BACKDOOR_REQ			666
#define MSG_SMS_SWDOWNLOAD_BACKDOOR_RES			667
#define MSG_SMS_GET_VERSION_EX_REQ			668
#define MSG_SMS_GET_VERSION_EX_RES			669
#define MSG_SMS_SET_CLOCK_OUTPUT_REQ			670
#define MSG_SMS_I2C_SET_FREQ_REQ			685
#define MSG_SMS_GENERIC_I2C_REQ				687
#define MSG_SMS_GENERIC_I2C_RES				688
#define MSG_SMS_DVBT_BDA_DATA				693
#define MSG_SW_RELOAD_REQ				697
#define MSG_SMS_DATA_MSG				699
#define MSG_SW_RELOAD_START_REQ				702
#define MSG_SW_RELOAD_START_RES				703
#define MSG_SW_RELOAD_EXEC_REQ				704
#define MSG_SW_RELOAD_EXEC_RES				705
#define MSG_SMS_SPI_INT_LINE_SET_REQ		710
#define MSG_SMS_SPI_INT_LINE_SET_RES		711			
#define MSG_SMS_GPIO_CONFIG_EX_REQ			712
#define MSG_SMS_GPIO_CONFIG_EX_RES			713
#define MSG_SMS_LOOPBACK_REQ			718
#define MSG_SMS_LOOPBACK_RES			719
#define MSG_SMS_CMMB_START_SERVICE_REQ		762
#define MSG_SMS_CMMB_START_SERVICE_RES		763
#define MSG_SMS_ISDBT_TUNE_REQ				776
#define MSG_SMS_ISDBT_TUNE_RES				777
#define MSG_SMS_TRANSMISSION_IND			782
#define	MSG_SMS_NEW_CRYSTAL_REQ				794
#define	MSG_SMS_NEW_CRYSTAL_RES				795
#define MSG_SMS_START_IR_REQ				800
#define MSG_SMS_START_IR_RES				801
#define MSG_SMS_IR_SAMPLES_IND				802
#define MSG_SMS_INTERFACE_LOCK_IND			805
#define MSG_SMS_INTERFACE_UNLOCK_IND		806
#define MSG_SMS_SIGNAL_DETECTED_IND			827
#define MSG_SMS_NO_SIGNAL_IND				828
#define MSG_SMS_SET_PERIODIC_STATISTICS_REQ  836
#define MSG_SMS_SET_PERIODIC_STATISTICS_RES  837
#define MSG_SMS_DEVICE_READY_IND			872
#define MSG_SMS_CMMB_SET_CA_CW_RES		877
#define MSG_SMS_CMMB_SET_CA_SALT_RES	879

#define SMS_INIT_MSG_EX(ptr, type, src, dst, len) do { \
	(ptr)->msgType = type; (ptr)->msgSrcId = src; (ptr)->msgDstId = dst; \
	(ptr)->msgLength = len; (ptr)->msgFlags = 0; \
} while (0)
#define SMS_INIT_MSG(ptr, type, len) \
	SMS_INIT_MSG_EX(ptr, type, 0, HIF_TASK, len)

enum SMS_DVB3_EVENTS {
	DVB3_EVENT_INIT = 0,
	DVB3_EVENT_SLEEP,
	DVB3_EVENT_HOTPLUG,
	DVB3_EVENT_FE_LOCK,
	DVB3_EVENT_FE_UNLOCK,
	DVB3_EVENT_UNC_OK,
	DVB3_EVENT_UNC_ERR
};

enum SMS_DEVICE_MODE {
	DEVICE_MODE_NONE = -1,
	DEVICE_MODE_DVBT = 0,
	DEVICE_MODE_DVBH,
	DEVICE_MODE_DAB_TDMB,
	DEVICE_MODE_DAB_TDMB_DABIP,
	DEVICE_MODE_DVBT_BDA,
	DEVICE_MODE_ISDBT,
	DEVICE_MODE_ISDBT_BDA,
	DEVICE_MODE_CMMB,
	DEVICE_MODE_RAW_TUNER,
	DEVICE_MODE_FM_TUNER,
	DEVICE_MODE_FM_TUNER_BDA,
	DEVICE_MODE_ATSC,
	DEVICE_MODE_MAX,
};

//shouldn't these two structs be inside the gpio_config (sms-cards.h)?
struct sms_antenna_freq_domains_ST {
	int bottom_freq;
	int top_freq;
};

struct sms_antenna_config_ST {
	int pinNum[2];
	struct sms_antenna_freq_domains_ST freq_domains[4];
};

struct SmsMsgHdr_ST {
	u16 msgType;
	u8 msgSrcId;
	u8 msgDstId;
	u16 msgLength;		/* Length of entire message, including header */
	u16 msgFlags;
};

struct SmsMsgData_ST {
	struct SmsMsgHdr_ST xMsgHeader;
	//JACKZ
	u32 msgData[6];
	//u32 msgData[1];
};

struct SmsMsgData_ST2 {
	struct SmsMsgHdr_ST xMsgHeader;
	u32 msgData[2];
};

struct SmsDataDownload_ST {
	struct SmsMsgHdr_ST xMsgHeader;
	u32 MemAddr;
	u8 Payload[SMS_MAX_PAYLOAD_SIZE];
};

struct SmsMsgAntennaConfig_ST {
	struct SmsMsgHdr_ST xMsgHeader;
	struct sms_antenna_config_ST msgData;	
};

struct SmsVersionRes_ST {
	struct SmsMsgHdr_ST xMsgHeader;

	u16 ChipModel; /* e.g. 0x1102 for SMS-1102 "Nova" */
	u8 Step; /* 0 - Step A */
	u8 MetalFix; /* 0 - Metal 0 */

	/* FirmwareId 0xFF if ROM, otherwise the
	 * value indicated by SMSHOSTLIB_DEVICE_MODES_E */
	u8 FirmwareId;
	/* SupportedProtocols Bitwise OR combination of
	 * supported protocols */
	u8 SupportedProtocols;

	u8 VersionMajor;
	u8 VersionMinor;
	u8 VersionPatch;
	u8 VersionFieldPatch;

	u8 RomVersionMajor;
	u8 RomVersionMinor;
	u8 RomVersionPatch;
	u8 RomVersionFieldPatch;

	u8 TextLabel[34];
};

struct SmsFirmware_ST {
	u32 CheckSum;
	u32 Length;
	u32 StartAddress;
	u8 Payload[1];                               //xingyu add
};

//JACKZ
 struct SmsStatisticsRes_ST {
	struct SmsMsgHdr_ST xMsgHeader;

	u32 IsRfLocked;
	u32 IsDemodLocked;
	int  InBandPower;
	u32 SNR;
	u32 Frequency;
	u32 ReceptionQuality;
	u32 Ber;
	u32 RsTotalRows;
	u32 RsBadRows;
	u32 LdpcTotalWords;
	u32 LdpcBadWords;
};

struct SmsStatisticsExRes_ST {
	struct SmsMsgHdr_ST xMsgHeader;

  u32 ReturnCode;

	u32	Reserved1;				//!< Reserved field. 
	u32	Reserved2;				//!< Reserved field. 

	// Common parameters
	u32	IsRfLocked;				//!< 0 - not locked, 1 - locked
	u32	IsDemodLocked;			//!< 0 - not locked, 1 - locked
	u16	IsExternalAntennaOn;		//!< 0 - external LNA off, 1 - external LNA on
	u16	IsExternalLNAOn;			//!< 0 - external LNA off, 1 - external LNA on

	// Reception quality
	int	SNR;					//!< dB
	int	RSSI;					//!< dBm
	u32	ErrorsCounter;			//!< Firmware errors
	int	InBandPwr;				//!< In band power in dBM
	int	CarrierOffset;			//!< Carrier Offset in Hz
	u32	BER;                     //!< BER * 10^-6

	// Transmission parameters
	u32	Frequency;				//!< Frequency in Hz
	u32	Reserved3;				//!< Reserved field. 

	u8	ModemState;				//!< 0 - Acquisition, 1 - Locked
	u8	ReceptionQuality;		//!< //!< Signal quality on a scale 0-5
	u16	Reserved6;				//!< Reserved field. 
	
	// Per Channel Information
	u32	NumActiveChannels;		//!< Number of channels - including control
}; 


/// Statistics information returned as response for SmsHostApiGetStatistics_Req
struct SMSHOSTLIB_STATISTICS_S
{
	u32 Reserved;		//!< Reserved

	// Common parameters
	u32 IsRfLocked;		//!< 0 - not locked, 1 - locked
	u32 IsDemodLocked;	//!< 0 - not locked, 1 - locked
	u32 IsExternalLNAOn;	//!< 0 - external LNA off, 1 - external LNA on

	// Reception quality
	s32 SNR;		//!< dB
	u32 BER;		//!< Post Viterbi BER [1E-5]
	u32 FIB_CRC;		//!< CRC errors percentage, valid only for DAB
	u32 TS_PER;		//!< Transport stream PER, 0xFFFFFFFF indicate N/A, valid only for DVB-T/H
	u32 MFER;		//!< DVB-H frame error rate in percentage, 0xFFFFFFFF indicate N/A, valid only for DVB-H
	s32 RSSI;		//!< dBm
	s32 InBandPwr;		//!< In band power in dBM
	s32 CarrierOffset;	//!< Carrier Offset in bin/1024

	// Transmission parameters
	u32 Frequency;		//!< Frequency in Hz
	u32 Bandwidth;		//!< Bandwidth in MHz, valid only for DVB-T/H
	u32 TransmissionMode;	//!< Transmission Mode, for DAB modes 1-4, for DVB-T/H FFT mode carriers in Kilos
	u32 ModemState;		//!< from SMSHOSTLIB_DVB_MODEM_STATE_ET , valid only for DVB-T/H
	u32 GuardInterval;	//!< Guard Interval from SMSHOSTLIB_GUARD_INTERVALS_ET, valid only for DVB-T/H
	u32 CodeRate;		//!< Code Rate from SMSHOSTLIB_CODE_RATE_ET, valid only for DVB-T/H
	u32 LPCodeRate;		//!< Low Priority Code Rate from SMSHOSTLIB_CODE_RATE_ET, valid only for DVB-T/H
	u32 Hierarchy;		//!< Hierarchy from SMSHOSTLIB_HIERARCHY_ET, valid only for DVB-T/H
	u32 Constellation;	//!< Constellation from SMSHOSTLIB_CONSTELLATION_ET, valid only for DVB-T/H

	// Burst parameters, valid only for DVB-H
	u32 BurstSize;		//!< Current burst size in bytes, valid only for DVB-H
	u32 BurstDuration;	//!< Current burst duration in mSec, valid only for DVB-H
	u32 BurstCycleTime;	//!< Current burst cycle time in mSec, valid only for DVB-H
	u32 CalculatedBurstCycleTime;//!< Current burst cycle time in mSec, as calculated by demodulator, valid only for DVB-H
	u32 NumOfRows;		//!< Number of rows in MPE table, valid only for DVB-H
	u32 NumOfPaddCols;	//!< Number of padding columns in MPE table, valid only for DVB-H
	u32 NumOfPunctCols;	//!< Number of puncturing columns in MPE table, valid only for DVB-H
	u32 ErrorTSPackets;	//!< Number of erroneous transport-stream packets
	u32 TotalTSPackets;	//!< Total number of transport-stream packets
	u32 NumOfValidMpeTlbs;	//!< Number of MPE tables which do not include errors after MPE RS decoding
	u32 NumOfInvalidMpeTlbs;//!< Number of MPE tables which include errors after MPE RS decoding
	u32 NumOfCorrectedMpeTlbs;//!< Number of MPE tables which were corrected by MPE RS decoding
 	// Common params
	u32 BERErrorCount;	//!< Number of errornous SYNC bits.
	u32 BERBitCount;	//!< Total number of SYNC bits.

	// Interface information
	u32 SmsToHostTxErrors;	//!< Total number of transmission errors.

	// DAB/T-DMB
	u32 PreBER; 		//!< DAB/T-DMB only: Pre Viterbi BER [1E-5]

	// DVB-H TPS parameters
	u32 CellId;		//!< TPS Cell ID in bits 15..0, bits 31..16 zero; if set to 0xFFFFFFFF cell_id not yet recovered
	u32 DvbhSrvIndHP;	//!< DVB-H service indication info, bit 1 - Time Slicing indicator, bit 0 - MPE-FEC indicator
	u32 DvbhSrvIndLP;	//!< DVB-H service indication info, bit 1 - Time Slicing indicator, bit 0 - MPE-FEC indicator

	u32 NumMPEReceived;	//!< DVB-H, Num MPE section received

	u32 ReservedFields[10];	//!< Reserved
};

struct PID_STATISTICS_DATA_S
{
	struct PID_BURST_S
	{
		u32	size;
		u32	padding_cols;
		u32	punct_cols;
		u32	duration;
		u32	cycle;
		u32	calc_cycle;
	} burst;

	u32	tot_tbl_cnt;
	u32	invalid_tbl_cnt;
	u32  tot_cor_tbl;
};

struct PID_DATA_S
{
	u32 pid;
	u32 num_rows;
	struct PID_STATISTICS_DATA_S pid_statistics;
};

#define CORRECT_STAT_RSSI(_stat) (_stat).RSSI *= -1
#define CORRECT_STAT_BANDWIDTH(_stat) _stat.Bandwidth = 8 - _stat.Bandwidth
#define CORRECT_STAT_TRANSMISSON_MODE(_stat) \
	if(_stat.TransmissionMode == 0) _stat.TransmissionMode = 2; \
	else if(_stat.TransmissionMode == 1) _stat.TransmissionMode = 8; \
		else _stat.TransmissionMode = 4;

struct TRANSMISSION_STATISTICS_S
{
	u32 Frequency;		//!< Frequency in Hz
	u32 Bandwidth;		//!< Bandwidth in MHz
	u32 TransmissionMode;	//!< FFT mode carriers in Kilos
	u32 GuardInterval;	//!< Guard Interval from SMSHOSTLIB_GUARD_INTERVALS_ET
	u32 CodeRate;		//!< Code Rate from SMSHOSTLIB_CODE_RATE_ET
	u32 LPCodeRate;		//!< Low Priority Code Rate from SMSHOSTLIB_CODE_RATE_ET
	u32 Hierarchy;		//!< Hierarchy from SMSHOSTLIB_HIERARCHY_ET
	u32 Constellation;	//!< Constellation from SMSHOSTLIB_CONSTELLATION_ET

	// DVB-H TPS parameters
	u32 CellId;		//!< TPS Cell ID in bits 15..0, bits 31..16 zero; if set to 0xFFFFFFFF cell_id not yet recovered
	u32 DvbhSrvIndHP;	//!< DVB-H service indication info, bit 1 - Time Slicing indicator, bit 0 - MPE-FEC indicator
	u32 DvbhSrvIndLP;	//!< DVB-H service indication info, bit 1 - Time Slicing indicator, bit 0 - MPE-FEC indicator
	u32 IsDemodLocked;	//!< 0 - not locked, 1 - locked
};

struct RECEPTION_STATISTICS_S
{
	u32 IsRfLocked;		//!< 0 - not locked, 1 - locked
	u32 IsDemodLocked;	//!< 0 - not locked, 1 - locked
	u32 IsExternalLNAOn;	//!< 0 - external LNA off, 1 - external LNA on

	u32 ModemState;		//!< from SMSHOSTLIB_DVB_MODEM_STATE_ET
	s32 SNR;		//!< dB
	u32 BER;		//!< Post Viterbi BER [1E-5]
	u32 BERErrorCount;	//!< Number of erronous SYNC bits.
	u32 BERBitCount;	//!< Total number of SYNC bits.
	u32 TS_PER;		//!< Transport stream PER, 0xFFFFFFFF indicate N/A
	u32 MFER;		//!< DVB-H frame error rate in percentage, 0xFFFFFFFF indicate N/A, valid only for DVB-H
	s32 RSSI;		//!< dBm
	s32 InBandPwr;		//!< In band power in dBM
	s32 CarrierOffset;	//!< Carrier Offset in bin/1024
	u32 ErrorTSPackets;	//!< Number of erroneous transport-stream packets
	u32 TotalTSPackets;	//!< Total number of transport-stream packets

	s32 MRC_SNR;		//!< dB
	s32 MRC_RSSI;		//!< dBm
	s32 MRC_InBandPwr;	//!< In band power in dBM
};


// Statistics information returned as response for SmsHostApiGetStatisticsEx_Req for DVB applications, SMS1100 and up
struct SMSHOSTLIB_STATISTICS_DVB_S
{
	// Reception
	struct RECEPTION_STATISTICS_S ReceptionData;

	// Transmission parameters
	struct TRANSMISSION_STATISTICS_S TransmissionData;

	// Burst parameters, valid only for DVB-H
#define	SRVM_MAX_PID_FILTERS							8
	struct PID_DATA_S PidData[SRVM_MAX_PID_FILTERS];
};

struct SMSHOSTLIB_ISDBT_LAYER_STAT_S
{
	// Per-layer information
	u32 CodeRate;			//!< Code Rate from SMSHOSTLIB_CODE_RATE_ET, 255 means layer does not exist
	u32 Constellation;		//!< Constellation from SMSHOSTLIB_CONSTELLATION_ET, 255 means layer does not exist
	u32 BER;			//!< Post Viterbi BER [1E-5], 0xFFFFFFFF indicate N/A
	u32 BERErrorCount;		//!< Post Viterbi Error Bits Count
	u32 BERBitCount;		//!< Post Viterbi Total Bits Count
	u32 PreBER; 			//!< Pre Viterbi BER [1E-5], 0xFFFFFFFF indicate N/A
	u32 TS_PER;			//!< Transport stream PER [%], 0xFFFFFFFF indicate N/A
	u32 ErrorTSPackets;		//!< Number of erroneous transport-stream packets
	u32 TotalTSPackets;		//!< Total number of transport-stream packets
	u32 TILdepthI;			//!< Time interleaver depth I parameter, 255 means layer does not exist
	u32 NumberOfSegments;		//!< Number of segments in layer A, 255 means layer does not exist
	u32 TMCCErrors;			//!< TMCC errors
};

struct SMSHOSTLIB_STATISTICS_ISDBT_S
{
	u32 StatisticsType;			//!< Enumerator identifying the type of the structure.  Values are the same as SMSHOSTLIB_DEVICE_MODES_E
	//!< This field MUST always first in any statistics structure

	u32 FullSize;				//!< Total size of the structure returned by the modem.  If the size requested by
	//!< the host is smaller than FullSize, the struct will be truncated

	// Common parameters
	u32 IsRfLocked;				//!< 0 - not locked, 1 - locked
	u32 IsDemodLocked;			//!< 0 - not locked, 1 - locked
	u32 IsExternalLNAOn;			//!< 0 - external LNA off, 1 - external LNA on

	// Reception quality
	s32  SNR;				//!< dB
	s32  RSSI;				//!< dBm
	s32  InBandPwr;				//!< In band power in dBM
	s32  CarrierOffset;			//!< Carrier Offset in Hz

	// Transmission parameters
	u32 Frequency;				//!< Frequency in Hz
	u32 Bandwidth;				//!< Bandwidth in MHz
	u32 TransmissionMode;			//!< ISDB-T transmission mode
	u32 ModemState;				//!< 0 - Acquisition, 1 - Locked
	u32 GuardInterval;			//!< Guard Interval, 1 divided by value
	u32 SystemType;				//!< ISDB-T system type (ISDB-T / ISDB-Tsb)
	u32 PartialReception;			//!< TRUE - partial reception, FALSE otherwise
	u32 NumOfLayers;			//!< Number of ISDB-T layers in the network
	u32 SegmentNumber;			//!< Segment number for ISDB-Tsb
	u32 TuneBW;				//!< Tuned bandwidth - BW_ISDBT_1SEG / BW_ISDBT_3SEG

	// Per-layer information
	// Layers A, B and C
	struct SMSHOSTLIB_ISDBT_LAYER_STAT_S	LayerInfo[3];	//!< Per-layer statistics, see struct SMSHOSTLIB_ISDBT_LAYER_STAT_S

	// Interface information
	u32 SmsToHostTxErrors;		//!< Total number of transmission errors.

	// Proprietary information	
	u32 ExtAntenna;

};


struct SRVM_SIGNAL_STATUS_S {
	u32 result;
	u32 snr;
	u32 tsPackets;
	u32 etsPackets;
	u32 constellation;
	u32 hpCode;
	u32 tpsSrvIndLP;
	u32 tpsSrvIndHP;
	u32 cellId;
	u32 reason;

	s32 inBandPower;
	u32 requestId;
};

struct smscore_gpio_config {
#define SMS_GPIO_DIRECTION_INPUT  0
#define SMS_GPIO_DIRECTION_OUTPUT 1
	u8 Direction;

#define SMS_GPIO_PULLUPDOWN_NONE     0
#define SMS_GPIO_PULLUPDOWN_PULLDOWN 1
#define SMS_GPIO_PULLUPDOWN_PULLUP   2
#define SMS_GPIO_PULLUPDOWN_KEEPER   3
	u8 PullUpDown;

#define SMS_GPIO_INPUTCHARACTERISTICS_NORMAL  0
#define SMS_GPIO_INPUTCHARACTERISTICS_SCHMITT 1
	u8 InputCharacteristics;

#define SMS_GPIO_OUTPUTSLEWRATE_SLOW		0 /* 10xx */
#define SMS_GPIO_OUTPUTSLEWRATE_FAST		1 /* 10xx */

#define SMS_GPIO_OUTPUTSLEWRATE_0_45_V_NS	0 /* 11xx */
#define SMS_GPIO_OUTPUTSLEWRATE_0_9_V_NS	1 /* 11xx */
#define SMS_GPIO_OUTPUTSLEWRATE_1_7_V_NS	2 /* 11xx */
#define SMS_GPIO_OUTPUTSLEWRATE_3_3_V_NS	3 /* 11xx */
	u8 OutputSlewRate;

#define SMS_GPIO_OUTPUTDRIVING_S_4mA		0 /* 10xx */
#define SMS_GPIO_OUTPUTDRIVING_S_8mA		1 /* 10xx */
#define SMS_GPIO_OUTPUTDRIVING_S_12mA		2 /* 10xx */
#define SMS_GPIO_OUTPUTDRIVING_S_16mA		3 /* 10xx */

#define SMS_GPIO_OUTPUTDRIVING_1_5mA		0 /* 11xx */
#define SMS_GPIO_OUTPUTDRIVING_2_8mA		1 /* 11xx */
#define SMS_GPIO_OUTPUTDRIVING_4mA			2 /* 11xx */
#define SMS_GPIO_OUTPUTDRIVING_7mA			3 /* 11xx */
#define SMS_GPIO_OUTPUTDRIVING_10mA			4 /* 11xx */
#define SMS_GPIO_OUTPUTDRIVING_11mA			5 /* 11xx */
#define SMS_GPIO_OUTPUTDRIVING_14mA			6 /* 11xx */
#define SMS_GPIO_OUTPUTDRIVING_16mA			7 /* 11xx */
	u8 OutputDriving;
};

extern void smschipreset(void *context);
extern void smschipEintEnable(void);
extern void smschipEintDisable(void);

extern int smscore_reset_device_drvs(struct smscore_device_t *coredev);

extern void smscore_registry_setmode(char *devpath, int mode);
extern int smscore_registry_getmode(char *devpath);

extern int smscore_register_hotplug(hotplug_t hotplug);
extern void smscore_unregister_hotplug(hotplug_t hotplug);

extern int smscore_register_device(struct smsdevice_params_t *params,
		struct smscore_device_t **coredev);
extern void smscore_unregister_device(struct smscore_device_t *coredev);

extern int smscore_start_device(struct smscore_device_t *coredev);
extern int smscore_load_firmware(struct smscore_device_t *coredev,
		char *filename, loadfirmware_t loadfirmware_handler);

extern int smscore_set_device_mode(struct smscore_device_t *coredev, int mode);
extern int smscore_get_device_mode(struct smscore_device_t *coredev);

extern int smscore_configure_board(struct smscore_device_t *coredev);

extern int smscore_register_client(struct smscore_device_t *coredev,
		struct smsclient_params_t *params,
		struct smscore_client_t **client);
extern void smscore_unregister_client(struct smscore_client_t *client);

extern int smsclient_sendrequest(struct smscore_client_t *client, void *buffer,
		size_t size);
extern void smscore_onresponse(struct smscore_device_t *coredev,
		struct smscore_buffer_t *cb);


extern int smscore_get_common_buffer_size(struct smscore_device_t *coredev);
extern int smscore_map_common_buffer(struct smscore_device_t *coredev,
		struct vm_area_struct *vma);
extern char *smscore_get_fw_filename(struct smscore_device_t *coredev, int mode, int lookup);

extern int smscore_send_fw_file(struct smscore_device_t *coredev, u8 *ufwbuf,
		int size);

extern int smscore_register_client(struct smscore_device_t *coredev,
		struct smsclient_params_t *params,
		struct smscore_client_t **client);
extern void smscore_unregister_client(struct smscore_client_t *client);

extern int smsclient_sendrequest(struct smscore_client_t *client, void *buffer,
		size_t size);

extern int smscore_register_device(struct smsdevice_params_t *params,
		struct smscore_device_t **coredev);

extern void smscore_onresponse(struct smscore_device_t *coredev,
		struct smscore_buffer_t *cb);

extern int smscore_get_common_buffer_size(struct smscore_device_t *coredev);
extern int smscore_map_common_buffer(struct smscore_device_t *coredev,
		struct vm_area_struct *vma);

extern struct smscore_buffer_t *smscore_getbuffer(
		struct smscore_device_t *coredev);
extern void smscore_putbuffer(struct smscore_device_t *coredev,
		struct smscore_buffer_t *cb);

int smscore_gpio_configure(struct smscore_device_t *coredev, u8 PinNum,
		struct smscore_gpio_config *pGpioConfig);
int smscore_gpio_set_level(struct smscore_device_t *coredev, u8 PinNum,
		u8 NewLevel);
int smscore_gpio_get_level(struct smscore_device_t *coredev, u8 PinNum,
		u8 *level);

void smscore_set_board_id(struct smscore_device_t *core, int id);
int smscore_get_board_id(struct smscore_device_t *core);

#ifdef SMS_HOSTLIB_SUBSYS
extern int smschar_register(void);
extern void smschar_unregister(void);
#endif

#ifdef SMS_NET_SUBSYS
extern int smsnet_register(void);
extern void smsnet_unregister(void);
#endif

#ifdef SMS_DVB3_SUBSYS
extern int smsdvb_register(void);
extern void smsdvb_unregister(void);
#endif

#ifdef SMS_USB_DRV
extern int smsusb_register(void);
extern void smsusb_unregister(void);
#endif

#ifdef SMS_SDIO_DRV
extern int smssdio_register(void);
extern void smssdio_unregister(void);
#endif

#define  SMS_SPI_MTK                                    
#ifdef SMS_SPI_MTK
extern int smsspi_register(void);
extern void smsspi_unregister(void);
#endif

/* ------------------------------------------------------------------------ */

extern int sms_debug;

#define DBG_INFO 1
#define DBG_ADV  2

#include <linux/rtc.h>
#include <linux/time.h>

//#define logtime
#ifdef  logtime                           //xingyu 1215
	extern struct timespec ts;
	extern struct rtc_time tm;
#define sms_printk(kern, fmt, arg...) {\
	getnstimeofday(&ts); \
       rtc_time_to_tm(ts.tv_sec, &tm); \
	printk(kern "%lld : (%d-%02d-%02d %02d:%02d:%02d.%09lu UTC), %s: " fmt "\n",ktime_to_ns(ktime_get()), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec, __func__, ##arg);}
#else
#define sms_printk(kern, fmt, arg...) \
	printk(kern "[cmmbdrv] %s: " fmt "\n", __func__, ##arg)
#endif
#define dprintk(kern, lvl, fmt, arg...) do {\
	if (sms_debug & lvl) \
		sms_printk(kern, fmt, ##arg); } while (0)
#if 0                                                //xingyu add
#define sms_log(fmt, arg...) sms_printk(KERN_INFO, fmt, ##arg)
#define sms_err(fmt, arg...) \
	sms_printk(KERN_ERR, "line: %d: " fmt, __LINE__, ##arg)
#define sms_warn(fmt, arg...)  sms_printk(KERN_WARNING, fmt, ##arg)
#define sms_info(fmt, arg...) \
	dprintk(KERN_INFO, DBG_INFO, fmt, ##arg)
#define sms_debug(fmt, arg...) \
	dprintk(KERN_DEBUG, DBG_ADV, fmt, ##arg)
#else

#if 0
#define sms_log(fmt, arg...) {sms_printk(KERN_ERR, fmt, ##arg)}
#define sms_err(fmt, arg...)  {sms_printk(KERN_ERR, fmt, ##arg)}
#else
#define sms_log(fmt, arg...) sms_printk(KERN_ERR, fmt, ##arg)
#define sms_err(fmt, arg...) \
	sms_printk(KERN_ERR, "line: %d: " fmt, __LINE__, ##arg)
#endif

#define sms_warn(fmt, arg...)  sms_printk(KERN_ERR, fmt, ##arg)
#define sms_info(fmt, arg...) \
	sms_printk(KERN_ERR, fmt, ##arg)
#define sms_debug(fmt, arg...) \
	sms_printk(KERN_ERR, fmt, ##arg)
#endif

#endif /* __SMS_CORE_API_H__ */
