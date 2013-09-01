#ifndef _MMIS_CMD_LIST_V2_H_
#define _MMIS_CMD_LIST_V2_H_


//============IF101 v2====================
#define READ_MODE_SEL				0x01
#define READ_MODE_CONFIG			0x02

#define READ_DEMOD_STATUS			0x03

#define READ_INT0_ENABLE			0x04
#define READ_INT1_ENABLE			0x05
#define READ_INT2_ENABLE			0x06

#define WRITE_INT0_ENABLE			0x07
#define WRITE_INT1_ENABLE			0x08
#define WRITE_INT2_ENABLE			0x09

#define READ_INT0_STATUS			0x0A
#define READ_INT1_STATUS			0x0B
#define READ_INT2_STATUS			0x0C

#define READ_INT_ENABLE			READ_INT0_ENABLE	
#define WRITE_INT_ENABLE			WRITE_INT0_ENABLE
#define READ_INT_STATUS			READ_INT0_STATUS

#define READ_LG0_FILTER_PID_L		0x0D
#define WRITE_LG0_FILTER_PID_L		0x0E
#define READ_LG0_FILTER_PID_H		0x0F
#define WRITE_LG0_FILTER_PID_H		0x10

#define READ_LG1_FILTER_PID_L		0x11
#define WRITE_LG1_FILTER_PID_L		0x12
#define READ_LG1_FILTER_PID_H		0x13
#define WRITE_LG1_FILTER_PID_H		0x14

#define READ_LG0_LEN_LOW			0x15
#define READ_LG0_LEN_MID			0x16
#define READ_LG0_LEN_HIGH			0x17

#define READ_LG1_LEN_LOW			0x18
#define READ_LG1_LEN_MID			0x19
#define READ_LG1_LEN_HIGH			0x1A

#define READ_UAM_LEN_LOW			0x1B
#define READ_UAM_LEN_MID			0x1C
#define READ_UAM_LEN_HIGH			0x1D

#define FETCH_UAM_DATA			0x9D

#define PREPARE_UAM_DATA			0x1E
#define PREPARE_TS1_DATA			0x1F

#define TX_ABORT					0x40	/* master break off transmitting */

// INT0_STATUS Bit Mask
//#define LG0_DATA_RDY        (0x01)
//#define LG1_DATA_RDY        (0x02)
//#define UAM_DATA_RDY        (0x04)
// INT_STATUS Bit Mask
#define LG0_DATA_RDY				(0x01)
#define LG1_DATA_RDY				(0x02)
#define LG2_DATA_RDY				(0x04)    //100
#define LG3_DATA_RDY				(0x08)    //1000
#define LG4_DATA_RDY				(0x10)    //10000
#define LG5_DATA_RDY				(0x20)    //100000
#define LG6_DATA_RDY				(0x40)    //1000000
#define LG7_DATA_RDY				(0x80)    //10000000
#define LG8_DATA_RDY				(0x100)   //100000000
#define LG9_DATA_RDY				(0x200)   //1000000000
#define LG10_DATA_RDY				(0x400)   //10000000000
#define LG11_DATA_RDY				(0x800)   //100000000000
//#define UAM_DATA_RDY 				(0x04)
#define UAM_DATA_RDY 				(0x80<<16)
//#define FW_ERR_DATA_RDY				(0x08)
#define FW_ERR_DATA_RDY				(0x40<<16)
#define LG0_ERR						(0x1u<<3)
#define LG1_ERR						(0x1u<<4)
#define UAM_ERR						(0x1u<<5)
 
/*******************************************************************************
*			SPI Only CMD
*******************************************************************************/
#define SPI_R_FREQ					0x04
#define SPI_W_FREQ					0x07
#define SPI_R_STATUS				0x05
#define SPI_W_STATUS				0x08
#define SPI_R_LDPC					0x06	
#define SPI_W_LDPC					0x09

#define SPI_R_CMD_DATA_ADDR		0x0D
#define SPI_W_CMD_DATA_ADDR		0x0E
#define SPI_R_CMD_STATUS_ADDR		0x0F
#define SPI_W_CMD_STATUS_ADDR	0x10
#define SPI_R_RSP_DATA_ADDR		0x11
#define SPI_W_RSP_DATA_ADDR		0x12
#define SPI_R_RSP_STATUS_ADDR		0x13
#define SPI_W_RSP_STATUS_ADDR		0x14

/* chip IF202 */
#define SPI_R_LDPC_Total_0			0x30
#define SPI_R_LDPC_Total_1			0x31
#define SPI_R_LDPC_Total_2			0x32
#define SPI_R_LDPC_Total_3			0x33
#define SPI_R_LDPC_Error_0			0x34
#define SPI_R_LDPC_Error_1			0x35
#define SPI_R_LDPC_Error_2			0x36
#define SPI_R_LDPC_Error_3			0x37

#define SPI_R_RS_Total_0				0x38
#define SPI_R_RS_Total_1				0x39
#define SPI_R_RS_Total_2				0x3A
#define SPI_R_RS_Total_3				0x3B
#define SPI_R_RS_Error_0			0x3C
#define SPI_R_RS_Error_1			0x3D
#define SPI_R_RS_Error_2			0x3E
#define SPI_R_RS_Error_3			0x3F

#define SPI_R_SYNC					0x50

/******************************************
*			IF202 Download
*******************************************/
#define WRITE_DATA_CMD 			0x23
#define WRITE_STATUS_CMD			0x21
#define READ_STATUS_CMD			0x20
#define SEND_CONTINUE				0x55
#define SEND_OVER					0xAA
#define DOWNLOAD_OVER				0xCC

#endif //_MMIS_CMD_LIST_V2_H_
