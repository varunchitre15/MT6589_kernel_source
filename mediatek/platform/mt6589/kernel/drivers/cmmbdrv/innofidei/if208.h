/************************************************
 * FileName:	if208.h
 *
 * Description:
 *              some cmd and struct	
 *
 ***********************************************/
#ifndef _IF101_H_
//if208.h
#define _IF101_H_
#include <linux/wait.h>		// wait_queue_head_t
#include <linux/semaphore.h>
#include <linux/completion.h>
#include <linux/types.h>
#include <linux/list.h>

#define CONFIG_INNODEV_V2_DEBUG
#if 0
#define inno_msg(fmt, arg...)	printk(KERN_ERR "[inno]"fmt"\n", ##arg)
#define inno_dbg(fmt, arg...)	printk(KERN_ERR "[inno]"fmt"\n", ##arg)
#define inno_err(fmt, arg...)	printk(KERN_ERR "[inno:ERROR!!!]"fmt"\n", ##arg) 
#else
#define inno_msg(fmt, arg...)	printk(KERN_ERR "[cmmb-drv]%s: " fmt "\n", __func__, ##arg)
#define inno_dbg(fmt, arg...)	//printk(KERN_ERR "[cmmb-drv]%s: "fmt "\n", __func__, ##arg)
#define inno_err(fmt, arg...)	printk(KERN_ERR "[cmmb-drv]ERROR!!! %s: "fmt "\n", __func__, ##arg) 
#endif
////// **************** /////////////////////////////
//#define  _check_block_time      // Switch use thread to check no eint issue,when get data from chip

#define UAM_COMPLETE
///// ***************** ///////////////////////////
#define IF228_CHIPID          0x00 
#define IF238_CHIPID          0x05 
#define IF258_CHIPID          0x08 
typedef enum{
	//CHANNEL_0 = 0,
	CHANNEL_1 = 1,
	CHANNEL_2 = 2,
	CHANNEL_3 = 3,
	CHANNEL_4 = 4,
	CHANNEL_5 = 5,
	CHANNEL_6 = 6,
	CHANNEL_7 = 7,
	CHANNEL_8 = 8,
	CHANNEL_9 = 9,
	CHANNEL_10 = 10
	//CHANNEL_11 = 11,
	//CHANNEL_ts0 = 12,  
}INNO_CHANNEL_ID;
// character device major and name
#define INNODEV_MAJOR		99
#define INNODEV_CHRDEV_NAME		"innodev"

// if101 i2c slave address
#define INNO_I2C_SLAVE_ADDR		0x10

#define INNO_BUFFER_SIZE	(0x10E00)

#define READ_AHBM2             0x71     //type2
#define WRITE_AHBM2		0x73    //type2
#define READ_AHBM1  		0x70     //type3 --can't find in datasheet, helen
#define WRITE_AHBM1 		0x72     //type3 --can't find in datasheet, helen

#if defined(FLAG_CHIP_IF258_26M)  
#define UAM_BASE_ADDR       0x0000BC00 
#else
#define UAM_BASE_ADDR		0x0000FC00
#endif
#define UAM_STATUS_REG		(UAM_BASE_ADDR + 0x004)
#define UAM_INT_REG		(UAM_BASE_ADDR + 0x005)   //temp
#define UAM_DATA_REG          	(UAM_BASE_ADDR + 0x008)

//SEND CMD:
#define FETCH_PER_COMM0             0x50002140     //CMD code
#define FETCH_PER_COMM1             0x50002141     //AP communication register 1
#define FETCH_PER_COMM2             0x50002142     //AP communication register 2
#define FETCH_PER_COMM3             0x50002143     //AP communication register 3
#define FETCH_PER_COMM4             0x50002144     //AP communication register 4
#define FETCH_PER_COMM5             0x50002145     //AP communication register 5
#define FETCH_PER_COMM6             0x50002146     //AP communication register 6
#define FETCH_PER_COMM7             0x50002147     //AP communication register 7

 

//ACK:
#define FETCH_PER_COMM8             0x50002148     //AP communication register 8
#define FETCH_PER_COMM9             0x50002149     //AP communication register 9
#define FETCH_PER_COMM10            0x5000214a     //AP communication register 10
#define FETCH_PER_COMM11            0x5000214b     //AP communication register 11
#define FETCH_PER_COMM12            0x5000214c     //AP communication register 12
#define FETCH_PER_COMM13            0x5000214d     //AP communication register 13
#define FETCH_PER_COMM14            0x5000214e     //AP communication register 14
#define FETCH_PER_COMM15            0x5000214f     //AP communication register 15

//Status:

#define FETCH_PER_COMM31            0x5000215f    //bit7:1 busy 0 over bit6-0:0 sucess 1 error


//current freq
#define FETCH_PER_COMM17            0x50002151    //AP communication register 17
//M0_REG address
#define M0_REG_CLK_CTR           0x4000401c
#define M0_REG_CPU_CTR		0x40004010
#define FW_BASE_ADDR		0x20000000
#define M0_REG_PLL1_CTR         	0x4000402c         //xingyu add 

#define M0_REG_PLL_STATUS  0x40004048 

#if defined(FLAG_CHIP_IF258_26M)
#define LG_CW_STATE_BASE   0x0000BBF0  
#else
#define LG_CW_STATE_BASE   0xFBF0
#endif

#define LG0_CW_STATE_REG   LG_CW_STATE_BASE
#define LG1_CW_STATE_REG   LG_CW_STATE_BASE+1
#define LG2_CW_STATE_REG   LG_CW_STATE_BASE+2
#define LG3_CW_STATE_REG   LG_CW_STATE_BASE+3
#define LG4_CW_STATE_REG   LG_CW_STATE_BASE+4
#define LG5_CW_STATE_REG   LG_CW_STATE_BASE+5
#define LG6_CW_STATE_REG   LG_CW_STATE_BASE+6
#define LG7_CW_STATE_REG   LG_CW_STATE_BASE+7
#define LG8_CW_STATE_REG   LG_CW_STATE_BASE+8
#define LG9_CW_STATE_REG   LG_CW_STATE_BASE+9
#define LG10_CW_STATE_REG  LG_CW_STATE_BASE+10
#define LG11_CW_STATE_REG  LG_CW_STATE_BASE+11
//==================================
//IF228+
//==================================
//Status:
#define FETCH_PER_COMM31            0x5000215f    //bit7:1 busy 0 over bit6-0:0 sucess 1 error
//Signal power
#define FETCH_PER_COMM16            0x50002150     //AP communication register 16
//current freq
#define FETCH_PER_COMM17            0x50002151    //AP communication register 17
//CN(SNR)
#define FETCH_PER_COMM18            0x50002152    //AP communication register 18
#define FETCH_PER_COMM19            0x50002153    //AP communication register 19
#define FETCH_PER_COMM21     	      0x50002155    //AP communication register 21
//BER
#define FETCH_PER_COMM22            0x50002156     //AP communication register 22
#define FETCH_PER_COMM23            0x50002157    //AP communication register 23
//
//error status
#define FETCH_PER_COMM29            0x5000215d    //AP communication register 29
//Signal quality:
#define FETCH_PER_COMM30            0x5000215e

#define OFDM_SYNC_STATE        0x50000028
#define FETCH_LDPC_TOTAL       0x50002044  
#define FETCH_LDPC_ERR		0x5000204C
#define FETCH_RS_TOTAL       	0x50002050
#define FETCH_RS_ERR			0x50002054
#define FETCH_LG8_11_MODE  	0x50002068
#define FETCH_INT_STATUS0  	0x50002104   
#define FETCH_INTEN0_ADDR  	0x50002100   
//
#define READ_LG0_LEN				0x62
#define READ_LG1_LEN				0x63
#define READ_LG2_LEN				0x64
#define READ_LG3_LEN				0x65
#define READ_LG4_LEN				0x66
#define READ_LG5_LEN				0x67
#define READ_LG6_LEN				0x68
#define READ_LG7_LEN				0x69
#define READ_LG8_LEN				0x6a
#define READ_LG9_LEN				0x6b
//
#define FETCH_TS0_DATA 				0x9d
#define FETCH_LG0_DATA				0x99
#define FETCH_LG1_DATA				0x9b
#define FETCH_LG2_DATA				0x80
#define FETCH_LG3_DATA				0x81
#define FETCH_LG4_DATA				0x82
#define FETCH_LG5_DATA				0x83
#define FETCH_LG6_DATA				0x84
#define FETCH_LG7_DATA				0x85
#define FETCH_LG8_DATA				0x86
#define FETCH_LG9_DATA				0x87
#define FETCH_LG10_DATA				0x88
#define FETCH_LG11_DATA				0x89
//==================================

typedef int (*i2c_init_func_t)(void);
typedef int (*i2c_read_func_t)(u16 reg, u8* data);
typedef int (*i2c_write_func_t)(u16 reg, u8 data);
typedef int (*i2c_exit_func_t)(void);
typedef struct inno_i2c_driver{
	i2c_init_func_t		init;
	i2c_read_func_t		read;
	i2c_write_func_t	write;
	i2c_exit_func_t		exit;
}inno_i2c_driver_t;

#define INNO_SPI_MASTER		0
#define INNO_SPI_SLAVE		1
typedef int (*spi_init_func_t)(u8 type);
typedef int (*spi_read_func_t)(u8* pdata, u32 len);
typedef int (*spi_write_func_t)(u8* pdata, u32 len);
typedef int (*spi_uninit_func_t)(void);
typedef int (*spi_sync_cs_func_t)(void);	//slave mode to download fireware
typedef int (*spi_drive_cs_func_t)(int drive);
typedef struct inno_spi_driver{
	spi_init_func_t		init;
	spi_read_func_t		read;
	spi_write_func_t	write;
	spi_uninit_func_t	uninit;
	spi_sync_cs_func_t	sync_cs;
	spi_drive_cs_func_t	drive_cs;
	
	spi_read_func_t		read2;
	spi_write_func_t	write2;
}inno_spi_driver_t;

typedef struct inno_cmdset {
    u32 (*get_lgx_length)(u8 channelID);
    u32  (*get_intr_status)(void);
    u32 (*fetch_lgx_data)(u8 chanelID, u8* buf, u32 len);
    void (*fetch_lgx_data2)(u8 chanelID, u8* buf, u32 len);    
}inno_cmdset_t;

typedef enum TRANSFER_DATA_SIZE {
	TRANS_1BYTE = 0,			/* transfer 8bits data every time */
	TRANS_2BYTES,				/* transfer 16bits data every time */
	TRANS_4BYTES,				/* transfer 32bits data every time */
	TRANS_BURST				/* continous transfer until nCS high */
}trans_size_t;

typedef enum INTR_TYPE {
	FALLING_EDGE = 0,
	RISING_EDGE
}intr_type_t;

typedef enum SPI_CLK_MODE {
	SPI_CLK_MODE_0 = 0,		//idle low, transmit falling edge, receive rising edge.
	SPI_CLK_MODE_1,			//idle high, transmit rising edge, receive falling edge.
	SPI_CLK_MODE_2,			//idle low, transmit rising edge, receive falling edge.
	SPI_CLK_MODE_3			//idle high, transmit falling edge, receive rising edge.
}spi_clk_mode_t;

typedef enum SPI_INTERFACE {
	NORMAL_SPI = 0,		// CLK, CS, TXD, RXD (4pins)
	ONEWIRE_SPI,		// CLK, CS, TRXD (3pins)
	FOURWIRES_SPI,		// CLK, CS, TRXD0, TRXD1, TRXD2, TRXD3 (6pins)
}spi_interface_t;

typedef struct inno_configuration{
	spi_interface_t	spi_interface;
	spi_clk_mode_t	spi_clk_mode;	
	intr_type_t		intr_type;
	trans_size_t	trans_size;
	int				i2c_cmd_mode;	/* if set 1, use i2c send command, otherwise use spi send command */
}inno_cfg_t;

typedef enum Logic_Channel_ID {
	LG0 = 0,
	LG1,
	LG2,                                    //xingyu 0922
	UAM,
	LG_END
}lgx_t;

struct inno_lgx_ids{
	char* 	name;
	int 	id;
};

typedef struct inno_buffer{
	void* start;
	int 	own_id;
	int		valid;
	u32		valid_size;
	struct semaphore sem;
	void		*vaddr;		/* vmap() return virtual address */
	struct page	**pages;	/* physical pages */
	int		page_num;	/* physical pages count */
	int		bufsize;	/* buffer size */
}inno_buffer_t;

typedef struct _Rx_Data
{
	unsigned int rx_len;		      //mfs file length   about 50k
	int channel_id; 				// the data come from channel
	unsigned char * rx_buf;             // data buffer
} Rx_Data_t;

#define MAX_INNOLG_NUM	LG_END	// maxium logic channel number
typedef struct inno_lgx{
	struct inno_lgx_ids*	ids;
	int use_count;
	struct semaphore sem;
	
	wait_queue_head_t	read_wait;
	wait_queue_head_t	uam_wait;
        
	struct completion uam_complete;         // add xingyu
	u32		bytes_available;	// byte size available by if101
	struct inno_buffer	inno_buffer;
}inno_lgx_t;

typedef struct inno_device{
	struct inno_configuration cfg;
	struct inno_lgx*	lgxs[MAX_INNOLG_NUM];

	inno_i2c_driver_t*		i2c_driver;
	inno_spi_driver_t*		spi_driver;
	inno_cmdset_t*		cmdset;	

	int 	use_count;

	struct semaphore	sem;	// inno device semaphore
}inno_device_t;

typedef enum INNO_RETURN_VALUE {
	INNO_SUCCESS = 0,
	INNO_ERROR,
	INNO_TIMEOUT,	// timeout error
	INNO_PARAM_ERR,	//parameter error
}inno_return_value_t;
//Foxconn add start, helen.shi, 2010-07-13
typedef enum{		
    INNO_NO_ERROR = 0,		
    INNO_GENERAL_ERROR = 1,		
    INNO_TIMEOUT_ERROR = 2,		
    INNO_FW_OPERATION_ERROR = 3,		
    INNO_FW_DOWNLOAD_ERROR = 4,		
    INNO_PARAMETER_ERROR = 5,
}INNO_RET;
//Foxconn add end, helen.shi, 2010-07-13

//=========== UAM ==============//
#define UAM_RESET_CMD 0xc1
#define UAM_PPS_CMD 0xc4
#define SET_UAM_CLOCK_CMD 0xc5
#define SEND_UAM_CMD 0xc2
#define GET_UAM_CMD 0xc3

#define CMD_UAM_CMD_TEST			0xC0
#define CMD_UAM_RESET				0xC1
#define CMD_UAM_SEND_DATA 			0xC2
#define CMD_UAM_GET_DATA			0xC3
#define CMD_UAM_PPS					0xC4
#define CMD_UAM_SET_CLOCK 			0xC5

#define SCARD_PROTOCOL_UNDEFINED    0x00000000  // There is no active protocol.
#define SCARD_PROTOCOL_T0           0x00000001  // T=0 is the active protocol.
#define SCARD_PROTOCOL_T1           0x00000002  // T=1 is the active protocol.

// Block waiting integer default value as definded by ISO
#define T1_BWI_DEFAULT		4
// Character waiting integer default value as definded by ISO
#define T1_CWI_DEFAULT		13

typedef enum _OP_MODE
{
	OP_SPECIFIC,
	OP_NEGOTIABLE,
}OP_MODE_T;

typedef struct _CLOCK_RATE_CONVERSION {

	const unsigned long F;
	const unsigned long fs; 

} CLOCK_RATE_CONVERSION, *PCLOCK_RATE_CONVERSION;

typedef struct _BIT_RATE_ADJUSTMENT {

	const unsigned long DNumerator;
	const unsigned long DDivisor;

} BIT_RATE_ADJUSTMENT, *PBIT_RATE_ADJUSTMENT;

typedef struct _PTS_DATA {
#define PTS_TYPE_DEFAULT 0x00
#define PTS_TYPE_OPTIMAL 0x01
#define PTS_TYPE_USER    0x02

    unsigned char Type;  

    unsigned char Fl;    // Fl value for PTS

    unsigned char Dl;     	    // Dl value for PTS

    unsigned long  CLKFrequency;    // New clock frequency

    unsigned long DataRate;    // New baud rate to be used after pts

    unsigned char StopBits;    // new stop bits to be used after pts
} PTS_DATA, *PPTS_DATA;

typedef struct _SCARD_CARD_CAPABILITIES
{
	// Flag that indicates that the current card uses invers convention
	int InversConvention;

	// Calculated etu 
	unsigned long	etu;
      
    // Answer To Reset string returned by card.
	struct {
		unsigned char Buffer[64];
		unsigned char Length;
	} ATR;

	struct {
		unsigned char Buffer[16];
		unsigned char Length;
	} HistoricalChars;

	PCLOCK_RATE_CONVERSION 	ClockRateConversion;
	PBIT_RATE_ADJUSTMENT 	BitRateAdjustment;

	unsigned char FI;	// Clock rate conversion 
	unsigned char DI;	// Bit rate adjustment
	unsigned char II;	// Maximum programming current
	unsigned char P;	// Programming voltage in .1 Volts
	unsigned char N;	// Extra guard time in etu 
	unsigned long GT;// Calculated guard time in micro seconds

	struct {
		unsigned long Supported;		// This is a bit mask of the supported protocols
		unsigned long Selected;		// The currently selected protocol
	} Protocol;

	// T=0 specific data
	struct {
		unsigned char WI;		// Waiting integer
		unsigned long WT;		// Waiting time in micro seconds
	} T0;

	// T=1 specific data
	struct {
		unsigned char IFSC;		// Information field size of card

		// Character waiting integer and block waiting integer
		unsigned char CWI;
		unsigned char BWI;

		unsigned char EDC;		// Error detection code

		// Character and block waiting time in micro seconds
		unsigned long CWT;
		unsigned long BWT;

		unsigned long BGT;		// Block guarding time in micro seconds
	} T1;

    PTS_DATA PtsData;

    unsigned char Reserved[100 - sizeof(PTS_DATA)];

} SCARD_CARD_CAPABILITIES, *PSCARD_CARD_CAPABILITIES;


//=======IOCTL COMMANDS==============

#define INNO_IOC_MAGIC		'i'
#define INNO_GET_INTR_TYPE			_IOR(INNO_IOC_MAGIC, 1, int)
#define INNO_MMIS_READ				_IOWR(INNO_IOC_MAGIC, 2, int)
#define INNO_MMIS_WRITE				_IOWR(INNO_IOC_MAGIC, 3, int)
#define INNO_MMIS_CMD				_IOWR(INNO_IOC_MAGIC, 4, int)
#define INNO_READ_REG				_IOWR(INNO_IOC_MAGIC, 5, int)
#define INNO_WRITE_REG				_IOWR(INNO_IOC_MAGIC, 6, int)
#define INNO_SCAN_FREQUENCY			_IOWR(INNO_IOC_MAGIC, 7, int)
#define INNO_SCAN_FREQUENCY_DOT		_IOWR(INNO_IOC_MAGIC, 8, int)
#define INNO_SET_FREQUENCY			_IOWR(INNO_IOC_MAGIC, 9, int)
#define INNO_SET_FREQUENCY_DOT		_IOWR(INNO_IOC_MAGIC, 10, int)
#define INNO_SET_CHANNEL_CONFIG		_IOWR(INNO_IOC_MAGIC, 11, int)
#define INNO_GET_CHANNEL_CONFIG		_IOWR(INNO_IOC_MAGIC, 12, int)
#define INNO_GET_SYS_STATE			_IOWR(INNO_IOC_MAGIC, 13, int)
#define INNO_SET_PM					_IOWR(INNO_IOC_MAGIC, 14, int)
#define INNO_GET_FW_VER				_IOWR(INNO_IOC_MAGIC, 15, int)
#define INNO_FW_DOWNLOAD			_IOWR(INNO_IOC_MAGIC, 16, int)
#define INNO_SET_CP_TYPE			_IOWR(INNO_IOC_MAGIC, 17, int)
#define INNO_UPDATE_FW				_IOWR(INNO_IOC_MAGIC, 22, int)
#define INNO_GET_BUFFERLEN			_IOR(INNO_IOC_MAGIC, 23, int)
#define INNO_ENABLE_IRQ				_IOWR(INNO_IOC_MAGIC, 24, int)
#define INNO_DISABLE_IRQ			_IOWR(INNO_IOC_MAGIC, 25, int)
#define INNO_SHUTDOWN_IRQ			_IOWR(INNO_IOC_MAGIC, 26, int)

#define INNO_UAM_SET_CMD			_IOW(INNO_IOC_MAGIC, 31, int)
#define INNO_UAM_SET_CMDL			_IOW(INNO_IOC_MAGIC, 32, int)
#define INNO_UAM_READ_RES			_IOR(INNO_IOC_MAGIC, 33, int)
#define INNO_UAM_READ_STATUS			_IOR(INNO_IOC_MAGIC, 34, int)
#define INNO_UAM_READ_STATUS_LEN                _IOR(INNO_IOC_MAGIC, 35, int)
#define INNO_UAM_TRANSFER                       _IOR(INNO_IOC_MAGIC, 36, int)                       //xingyu add
#define INNO_UAM_INIT                           _IOR(INNO_IOC_MAGIC, 37, int)                       //xingyu add
#define INNO_SEND_UAM_CMD                       _IOR(INNO_IOC_MAGIC, 38, int)                       //xingyu add
#define INNO_SEND_CMD                           _IOR(INNO_IOC_MAGIC, 39, int)                       //xingyu add
#define INNO_GET_FW_BYTES                       _IOR(INNO_IOC_MAGIC, 40, int)                       //xingyu add
#define INNO_STOP_POLL                          _IOR(INNO_IOC_MAGIC, 41, int)                       //xingyu add
#define INNO_MEMSET_MFS                         _IOR(INNO_IOC_MAGIC, 42, int)                       //xingyu add


struct INNOCHAR_FW_DATA {                      //xingyu
	char *fw_buf;
	int fw_size;
};

/*
 * IOCTL COMMAND PARAMETER
 */
//INNO_READ_REG & INNO_WRITE_REG command  parameter
typedef struct inno_reg_data{	
	u16 reg;
	u16 data;
}inno_reg_t;

// INNO_SCAN_FREQUENCY & INNO_SCAN_FREQUENCY_DOT command parameter
struct inno_freq_scan_area {
	u16		start;
	u16		end;
};

// INNO_SET_CP_TYPE	command parameter
#define CP_TYPE_MASK 	0x1
enum inno_cp_type {
	SHORT_CP = 0,
	LONG_CP,
	CP_40TS = SHORT_CP,
	CP_36TS = LONG_CP
};

// INNO_SET_PM command parameter
struct inno_pm_control {
	u8	sys_pause:1;	// System
	u8	stun_off:1;		// S-Tuner
	u8	utun_off:1;		// U-Tuner
	u8	tspwr_off:1;	// TS Power
};

// INNO_SET_CHANNEL_CONFIG	command parameter
#define CFG_LGCH0				0x00
#define CFG_LGCH1				0x01
#define CFG_LGCH2                          0x02                    //xingyu 0922
#define CFG_CLOSE_LGCH			0x01
#define CFG_TS_INVALID			0x3F
#define MODULATE_BPSK			0x00
#define MODULATE_QPSK			0x01
#define MODULATE_16QAM			0x02
#define MODULATE_RESERVED		0x03
#define LDPC_1DB2				0x00		// 1/2 LDPC
#define LDPC_3DB4				0x01		// 3/4 LDPC
#define ITLV_RESERVED			0x00
#define ITLV_MODE1				0x01
#define ITLV_MODE2				0x02
#define ITLV_MODE3				0x03
#define RS_240_240				0x00	//RS(240, 240)
#define RS_240_224				0x01	//RS(240, 224)
#define RS_240_192				0x02	//RS(240, 192)
#define RS_240_176				0x03	//RS(240, 176)
struct inno_channel_config{
	u8	ch_id;		//0--logic channel 0, 1--logic channel 1
	u8	ch_close:1;		//1--close this logic channel
	u8	ts_start:6;
	u8	ts_count;	
	u8	ldpc:2;
	u8 	itlv:2;
	u8	rs:2;
	u8	modulate:2;
       u8	subframe_ID;
};


// INNO_GET_SYS_STATE command parameter
struct inno_sys_state{
	u8	stattype;
	union{
		u8	sync_state;
		u32	signal_strength;
		u32	ldpc_total_count;
		u32 ldpc_err_count;
		u32	rs_total_count;
		u32	rs_err_count;
		u32  fw_err_status;                  //xingyu 0317 uam 
		u32 signal_quality;
		u32  BER;
		u32  SNR;
	}statdata;
};

/**
 * err_status                               //xingyu 0317 uam
 */
typedef enum{
	CAS_OK = 0x00,
	NO_MATCHING_CAS = 0x15,
	CARD_OP_ERROR = 0x17,

	MAC_ERR = 0x80,
	GSM_ERR = 0x81,
	KEY_ERR	= 0x82,
	KS_NOT_FIND	= 0x83,
	KEY_NOT_FIND	= 0x84,
	CMD_ERR	= 0x85,
}ERR_STATUS;
// INNO_GET_FW_VER command parameter
struct inno_fw_ver {
	u8	major;
	u8	minor;
};

struct uam_cmd_par{
	unsigned char* cmd;
	int len;
};

// INNO_FW_DOWNLOAD command parameter
struct inno_fw_bin {
	u8	data[1024*24];
};

// INNO_UAM_TRANSFER command parameter
struct inno_uam_parameter {
	unsigned char pBufIn[260];
	unsigned int bufInLen;
	unsigned char pBufOut[256];
	unsigned int pBufOutLen;
	unsigned short sw;
};

#endif
