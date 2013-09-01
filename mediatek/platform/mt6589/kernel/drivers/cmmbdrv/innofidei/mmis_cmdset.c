
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>

//#include "inno_regs.h"
#include "mmis_cmdlist.h"
#include "if208.h"

extern struct inno_device g_inno_dev;
extern struct mutex inno_spi_mutex;

extern int INNO_SPI_Exec_Cmd(int cmd, unsigned char *rsp);
extern int INNO_SPI_Read_Byte_Type2(unsigned long addr, unsigned char *buffer, int len);
extern int INNO_SPI_Write_Byte_Type2(unsigned long addr, unsigned char *data, int len);
extern int INNO_SPI_Read_Bytes_Type4(int cmd, unsigned char *buffer, int len);
extern int inno_spi_drive_cs(int);
extern INNO_RET INNO_SPI_Write_One_Byte(unsigned char data);
extern INNO_RET INNO_SPI_Read_Bytes(unsigned char *buffer, int len);
/*======================================
 *
 *  SPI MMIS COMMAND SET
 *
 *=====================================*/
/*
static void mmis_send_recv_data_cmd(u8 channelID)
{
	u8 cmd;
	switch(channelID)
	{
		case LG0:
			//cmd = FETCH_LG0_DATA;
			cmd = FETCH_LG1_DATA;
			break;
		case LG1:
			//cmd = FETCH_LG1_DATA;
			cmd = FETCH_LG2_DATA;
			break;
		case UAM:
			//cmd = FETCH_UAM_DATA;
			cmd = FETCH_LG3_DATA;
			break;
		default:
			return;
	}
	INNO_SPI_Write_One_Byte(cmd);			// Send SPI command 
}
*/
static u32 mmis_get_lgx_length(u8 channelID)
{
        INNO_RET ret= INNO_NO_ERROR;
	unsigned char outBuf[8] = {0},status;
	unsigned int channel_length, j;
	mutex_lock(&inno_spi_mutex);
	if ( channelID == UAM ) // dhlee 20100714 for UAM problem.
	{
		inno_dbg("mmis_get_lgx_length");
		for(j = 0; j< 500; j++)   	
		{
			ret=INNO_SPI_Read_Byte_Type2(UAM_STATUS_REG, &status, 1);	
			if(ret != INNO_NO_ERROR){
				inno_err("INNO_SPI_Read_Byte_Type2 fail");
				mutex_unlock(&inno_spi_mutex);
				return -1;
			}
			inno_dbg(" UAM_STATUS_REG = 0x%x", status);
			if(status != 0x00)
			{
				if(status  == 0x80)
				{
					ret=INNO_SPI_Read_Byte_Type2(UAM_BASE_ADDR, outBuf, 8);  //0x55 0xaa 
					if(ret != INNO_NO_ERROR){
						inno_err("INNO_SPI_Read_Byte_Type2 fail");
						mutex_unlock(&inno_spi_mutex);
						return -1;
					}
					mutex_unlock(&inno_spi_mutex);
					channel_length= ((unsigned int)outBuf[2] << 8) + (unsigned int)outBuf[3];
					inno_dbg("=zz=  channel_length =%d", channel_length);
					return channel_length;
				}
				else
				{		
	      				inno_err("uam rsp error------0x%x", status);
					status = 0;
					ret=INNO_SPI_Write_Byte_Type2(UAM_STATUS_REG, &status, 1);
					if(ret != INNO_NO_ERROR){
						inno_err("INNO_SPI_Write_Byte_Type2 fail");
						mutex_unlock(&inno_spi_mutex);
						return -1;
					}
					mutex_unlock(&inno_spi_mutex);
					return -1;                                          //error return -1
				}
			}
		}
		mutex_unlock(&inno_spi_mutex);
		inno_msg("Warning:INNO_SPI_Read_Byte_Type2 time out");
		return -1;                                                       //error return -1
	}
	else
	{
		switch(channelID){
			case LG0:
				ret = INNO_SPI_Read_Bytes_Type4(READ_LG1_LEN, outBuf, 3);
				break;
			case LG1:
				ret = INNO_SPI_Read_Bytes_Type4(READ_LG2_LEN, outBuf, 3);
				break;
			case LG2:                                               //xingyu 0922
				ret = INNO_SPI_Read_Bytes_Type4(READ_LG3_LEN, outBuf, 3);
				break;

			default:
				inno_err("channelID =%d",channelID);
				ret = INNO_PARAM_ERR;
				break;
		}	
	}
	mutex_unlock(&inno_spi_mutex);
	if(ret == INNO_NO_ERROR)
	{	
		channel_length = ((unsigned int)outBuf[2] << 16) | ((unsigned int)outBuf[1] << 8) | (unsigned int)outBuf[0];
		return channel_length;
	}
	else{
		inno_err("INNO_SPI_Read_Bytes_Type4 ret =%d",ret);
		return -1;
	}
#if 0
	int ret = INNO_SUCCESS;	
	//unsigned char l_byte = 0, m_byte = 0, h_byte = 0;
	unsigned char outBuf[3] = {0};
	unsigned int channel_length;
	switch(channelID){
		case LG0:
			ret = INNO_SPI_Read_Bytes_Type4(READ_LG0_LEN, outBuf, 3);
			break;
		case LG1:
			ret = INNO_SPI_Read_Bytes_Type4(READ_LG1_LEN, outBuf, 3);
			break;
		case LG2:
			ret = INNO_SPI_Read_Bytes_Type4(READ_LG2_LEN, outBuf, 3);
			break;
		case UAM:	
			ret = INNO_SPI_Read_Bytes_Type4(READ_LG3_LEN, outBuf, 3);
			break;
		default:
			ret = INNO_PARAM_ERR;
			break;
	}	
	if(ret == INNO_SUCCESS)
	{	
		channel_length = ((unsigned int)outBuf[2] << 16) | ((unsigned int)outBuf[1] << 8) | (unsigned int)outBuf[0];
		return channel_length;
	}
	return 0;
#endif
}

static u32 mmis_get_intr_status(void)
{
	unsigned char int_status[3]={0};
	u32 intr =0;
	INNO_RET ret = INNO_NO_ERROR;

	//FIHTDC, ALXiao, 20101227, protect SPI used, can not remove {
	mutex_lock(&inno_spi_mutex);
	ret=INNO_SPI_Exec_Cmd(READ_INT0_STATUS, &int_status[0]);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Exec_Cmd fail");
		mutex_unlock(&inno_spi_mutex);
		return 0;
	}
	ret=INNO_SPI_Exec_Cmd(READ_INT1_STATUS, &int_status[1]);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Exec_Cmd fail");
		mutex_unlock(&inno_spi_mutex);
		return 0;
	}
	ret=INNO_SPI_Exec_Cmd(READ_INT2_STATUS, &int_status[2]); // b[23] is for UAM response detection
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Exec_Cmd fail");
		mutex_unlock(&inno_spi_mutex);
		return 0;
	}
	mutex_unlock(&inno_spi_mutex);
	//FIHTDC, ALXiao, 20101227, protect SPI used, can not remove }
	//intr = (( int_status[0] & 0x02) >> 1) + (( int_status[0] & 0x04) >> 1) + (( int_status[2] & 0x80) >> 5) +(( int_status[2] & 0x40) >> 3); 
        int_status[0] = int_status[0] & 0xfe;
	intr = (int_status[2] << 16) + (((int_status[1] << 8) + int_status[0])>>1);
        //xingyu, debug int status, when int_status is 0
       // if(intr == 0)
            inno_msg("Warning:intr=0,int_status[0]=%d,int_status[1]=%d,int_status[2]=%d",int_status[0],int_status[1],int_status[2]);
	return intr;
}

static u32 mmis_fetch_lgx_data(u8 channelID, u8* pbuff, u32 len)
{ 
	INNO_RET ret = INNO_NO_ERROR;
	unsigned char fetch_data_cmd = 0;
	unsigned char outbuf[8] = {0};
	unsigned char status;

	if( channelID == UAM )
	{
		mutex_lock(&inno_spi_mutex);
		ret=INNO_SPI_Read_Byte_Type2(UAM_DATA_REG, pbuff, len);
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Read_Byte_Type2 fail");
			mutex_unlock(&inno_spi_mutex);
			return -1;
		}
		ret=INNO_SPI_Read_Byte_Type2(UAM_DATA_REG, outbuf, 8);
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Read_Byte_Type2 fail");
			mutex_unlock(&inno_spi_mutex);
			return -1;
		}

		status = 0;
		ret=INNO_SPI_Write_Byte_Type2(UAM_STATUS_REG, &status, 1);	
		if(ret != INNO_NO_ERROR){
			inno_err("INNO_SPI_Write_Byte_Type2 fail");
			mutex_unlock(&inno_spi_mutex);
			return -1;
		}
		mutex_unlock(&inno_spi_mutex);	
		inno_dbg("=zz= fetch_lgx_data end_0x%x,0x%x,0x%x, 0x%x", outbuf[0], outbuf[1], outbuf[2], outbuf[3]);
		return 0;
	}

#if 0
	mutex_lock(&inno_spi_mutex);
	g_inno_dev.cfg.trans_size = TRANS_4BYTES;
	//        g_inno_dev.cfg.trans_size = TRANS_BURST;
	switch(g_inno_dev.cfg.trans_size)
	{
		case TRANS_BURST:
			mmis_send_recv_data_cmd(channelID);
			inno_spi_drive_cs(0);
			//			g_inno_dev.spi_driver->read(pbuff, len);
			INNO_SPI_Read_Bytes(pbuff, len);		// Read all datas
			inno_spi_drive_cs(1);
			printk("mmis_fetch_lgx_data len = %d \n", len);
			//write_data_to_file(pbuff, len); //for test
			break;

		case TRANS_4BYTES:
			{
				u32 times = (len+3)>>2;
				u8* p = pbuff;
				int i;
				for(i=0; i<times; i++){
					mmis_send_recv_data_cmd(channelID);
					inno_spi_drive_cs(0);
					//g_inno_dev.spi_driver->read(p, 4);
					INNO_SPI_Read_Bytes(p,4);		// Read all datas
					inno_spi_drive_cs(1);
					p += 4;
				}
				break;            
			}


		case TRANS_2BYTES:
			{
				u32 times = (len+1)>>1;
				u8* p = pbuff;
				int i;
				for(i=0; i<times; i++){
					mmis_send_recv_data_cmd(channelID);
					inno_spi_drive_cs(0);
					g_inno_dev.spi_driver->read(p, 2);
					inno_spi_drive_cs(1);
					p += 2;
				}
				break;
			}

		case TRANS_1BYTE:
			{
				u8 *p = pbuff;
				int i;
				for(i=0; i<len; i++){
					mmis_send_recv_data_cmd(channelID);
					inno_spi_drive_cs(0);
					g_inno_dev.spi_driver->read(p, 1);
					inno_spi_drive_cs(1);
					p++;
				}
				break;
			}

		default:
			break;
	}
	mutex_unlock(&inno_spi_mutex);
	static int onlyone =0;
	if(onlyone==0){
		onlyone++;
		inno_msg("trans_size=%d",g_inno_dev.cfg.trans_size);
	}

#else
	mutex_lock(&inno_spi_mutex);
	switch(channelID)
	{
		case LG0:
			//cmd = FETCH_LG0_DATA;
			fetch_data_cmd = FETCH_LG1_DATA;
			break;
		case LG1:
			//cmd = FETCH_LG1_DATA;
			fetch_data_cmd = FETCH_LG2_DATA;
			break;
		case LG2:                       //xingyu 0922
			//cmd = FETCH_LG2_DATA;
			fetch_data_cmd = FETCH_LG3_DATA;
			break;			
//		case UAM:
			//ASUS_BSP+++ JimmyLin "[CMMB] apply inno's IF228 driver"					
			//cmd = FETCH_UAM_DATA;
	//		fetch_data_cmd = FETCH_LG3_DATA;
			//ASUS_BSP---			
		//	break;
		default:
		       inno_err("channelID =%d",channelID);
	   		mutex_unlock(&inno_spi_mutex);
			return -1;
	}
	ret = INNO_SPI_Write_One_Byte(fetch_data_cmd);			// Send SPI command 
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Write_One_Byte fail");
		mutex_unlock(&inno_spi_mutex);
		return -1;
	}
	inno_dbg("--s len =%d",len);
	ret = INNO_SPI_Read_Bytes(pbuff, len);		// Read all datas
	inno_dbg("--e");
	mutex_unlock(&inno_spi_mutex);
	if(ret != INNO_NO_ERROR){
		inno_err("INNO_SPI_Read_Bytes");
		return -1;
	}
#endif
	return 0;
}

struct inno_cmdset inno_cmdset_ops = {
	.get_intr_status			       =	mmis_get_intr_status,
	.get_lgx_length				= 	mmis_get_lgx_length,
	.fetch_lgx_data				=	mmis_fetch_lgx_data
};

