/* drivers/misc/afa750_cali.c
 *
 * Copyright 2009 Afa Micro.
 *	Frances Chu <franceschu@afamicro.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/delay.h>

/**************************************************/
/*              Constant Definition               */
/**************************************************/
#define AvgDepth							0x50
#define VI_AvgDiffRatio						10
#define ConsAP_InsAutoCalibrationActive      False

#define BSP_SWAP_USHORT(u16Val) 			u16Val = ((u16Val<<8) & 0xFF00) | ((u16Val>>8) & 0x00FF)
#define AFM314_EncodeVI(u16Vi)		(u16Vi & 0x0040) ? (u16Vi ^ 0x003F) : (u16Vi)	/*!< Encodes the VIL or VIH for hardware protection. */

#define AFA750_TXLEN 1
#define AFA750_RD_CNT 3
#define AFA750_RD_VILH 4
#define AFA750_TX_MAX_LEN 8
#define AFA750_RX_MAX_LEN 8
#define AFA750_RD_OP 0x40
#define AFA750_WR_OP 0x00

#define AFA750_RREG 0x10
#define AFA750_VILH_REG 0x48
#define AFA750_FS_CTL 0x02
#define AFA750_OTP_ADDR_REG 0x39
#define AFA750_OTP_CTL_REG 0x3C
#define AFA750_OTP_DOUT_REG 0x3B


#define OTP_CTRL_OTP_RD 0x01

#define AFA750_FLOAT_SCALING 100


/**************************************************/
/*               Data type                        */
/**************************************************/

enum AFM_FULL_SCALE {
    AFM_SCALE_2G = 0,
    AFM_SCALE_4G,
    AFM_SCALE_8G
};

struct TS_CONSAP_VilVihContent
{
	 u16 VilX;
	 u16 VihX;
	 u16 VilY;
	 u16 VihY;
	 u16 VilZ;
	 u16 VihZ;
};

struct TS_CONSAP_VilVihContent_Tmp
{
	 int VilX;
	 int VihX;
	 int VilY;
	 int VihY;
	 int VilZ;
	 int VihZ;
};
 
struct TS_CONSAP_CntAllContent
{
	 s16 CntX;
	 s16 CntY;
	 s16 CntZ;
};
 
struct TS_CONSAP_GainContent
{
	 u16 GainX;
	 u16 GainY;
	 u16 GainZ;
};

struct AFA750_CALI{
    int offset[3];
	int gain[3];
};

struct AFA750_I2C{

    struct i2c_client *client;
	u8	tx_buf[AFA750_TX_MAX_LEN];
	u8	tx_buf2[AFA750_TX_MAX_LEN];
	u8	rx_buf[AFA750_RX_MAX_LEN];
	u8	rx_buf2[AFA750_RX_MAX_LEN];

};
/**************************************************/
/*          Internal Function Declaration         */
/**************************************************/
/**
static void spi_read_op_len(struct AFA750_SPI *priv, u8 op,
                           u8 addr, int len)
{
        int ret;
        struct spi_transfer *xfer;
        struct spi_transfer *xfer2;
        struct spi_message *msg;
        struct spi_message *msg2;
        u8 i;


        for(i= 0; i < len * 2; i++){ 
            priv->tx_buf[0] = op | (addr+i);

            msg = &priv->spi_msg;
            xfer = &priv->spi_xfer;

            xfer->tx_buf = priv->tx_buf;
            xfer->rx_buf = NULL;
            xfer->len = 1;

            priv->tx_buf2[0] = 0;
            msg2 = &priv->spi_msg2;
            xfer2 = priv->spi_xfer2;

            xfer2->tx_buf = NULL; //priv->tx_buf2;
            xfer2->rx_buf = &(priv->rx_buf2[i]);
            xfer2->len = 1;
        
            xfer2++;

            xfer2->tx_buf = priv->tx_buf2;
            xfer2->rx_buf = NULL;
            xfer2->len = 1;

            ret = spi_sync(priv->spi, msg);
            ret = spi_sync(priv->spi, msg2);

            if (ret < 0){
                //printk(KERN_DEBUG DRV_NAME ": %s() failed: ret = %d\n",
                printk("Frances: %s() failed: ret = %d\n",
                        __func__, ret);
                while(1);
            }
        }
}
**/

static void i2c_read_op_len(struct AFA750_I2C *priv, u8 op,
                           u8 addr, int len)
{
        int ret;
        u8 i;
		int error;

	    error = i2c_check_functionality(priv->client->adapter, I2C_FUNC_SMBUS_BYTE_DATA);
	    if (!error) {
			for(i= 0; i < len; i++){ 
				priv->tx_buf[0] = (addr+i);

				ret = i2c_master_send(priv->client, priv->tx_buf, 1);
				if (ret < 0){
						printk("FrancesLog****: i2c read block send fialed %d\n", ret);
						while(1);
				}

				ret = i2c_master_recv(priv->client, &(priv->rx_buf2[i]), 1);
				if (ret < 0){
						printk("FrancesLog****: i2c read block recv fialed %d\n", ret);
						while(1);
				}

				if (ret != len){
						printk("FrancesLog****: i2c read block ret != count fialed \n");
						while(1);
				}

			}
		}else{
		    for(i= 0; i < len; i++){ 
				priv->tx_buf[0] = (addr+i);

				//ret = spi_write_then_read(priv->spi, priv->tx_buf, 1, &(priv->rx_buf2[i]), 1);
				ret = i2c_smbus_read_i2c_block_data(priv->client, priv->tx_buf[0], 1, &(priv->rx_buf2[i]));

				if (ret < 0){
					//printk(KERN_DEBUG DRV_NAME ": %s() failed: ret = %d\n",
					printk("Frances: %s() failed: ret = %d\n",
							__func__, ret);
					while(1);
				}
            }
		}
}
/*
 * basic SPI write operation
 */
static int i2c_write_op(struct AFA750_I2C *priv, u8 op,
			u8 addr, int len)
{
	int ret;
    u8 i;
	
    for(i= 0; i < len; i++){
		priv->tx_buf2[0] = (addr + i);
		priv->tx_buf2[1] = priv->tx_buf[i];
		//ret = spi_write(priv->spi, priv->tx_buf2, 2);
		ret = i2c_smbus_write_byte_data(priv->client, priv->tx_buf2[0], priv->tx_buf2[1]);
		if (ret < 0){
			 //printk(KERN_DEBUG DRV_NAME ": %s() failed: ret = %d\n",
			 printk("Frances: %s() failed: ret = %d\n",
					 __func__, ret);
			 while(1);
		 }
	 }
}

/** get convert **/
static void Get_VILH_Convert(struct TS_CONSAP_VilVihContent* vi)
{

	//Swap u16VILH for 8bit MCU
	vi->VilX = BSP_SWAP_USHORT(vi->VilX);
	vi->VihX = BSP_SWAP_USHORT(vi->VihX);
	vi->VilY = BSP_SWAP_USHORT(vi->VilY);
	vi->VihY = BSP_SWAP_USHORT(vi->VihY);
	vi->VilZ = BSP_SWAP_USHORT(vi->VilZ);
	vi->VihZ = BSP_SWAP_USHORT(vi->VihZ);

	
	vi->VilX = AFM314_EncodeVI(vi->VilX);
	vi->VihX = AFM314_EncodeVI(vi->VihX);
	vi->VilY = AFM314_EncodeVI(vi->VilY);
	vi->VihY = AFM314_EncodeVI(vi->VihY);
	vi->VilZ = AFM314_EncodeVI(vi->VilZ);
	vi->VihZ = AFM314_EncodeVI(vi->VihZ);
	
}

/** set convert **/
static void Set_VILH_Convert(struct TS_CONSAP_VilVihContent* vi)
{
	
	vi->VilX = AFM314_EncodeVI(vi->VilX);
	vi->VihX = AFM314_EncodeVI(vi->VihX);
	vi->VilY = AFM314_EncodeVI(vi->VilY);
	vi->VihY = AFM314_EncodeVI(vi->VihY);



	//Swap u16VILH for 8bit MCU
	vi->VilX = BSP_SWAP_USHORT(vi->VilX);
	vi->VihX = BSP_SWAP_USHORT(vi->VihX);
	vi->VilY = BSP_SWAP_USHORT(vi->VilY);
	vi->VihY = BSP_SWAP_USHORT(vi->VihY);


	
}

/** ready for tx buf **/
static void GetReady4VILH2tx(struct AFA750_I2C *priv, struct TS_CONSAP_VilVihContent* vi){
    priv->tx_buf[0] = (u8)(vi->VilX >> 8);
	priv->tx_buf[1] = (u8)vi->VilX;
	priv->tx_buf[2] = (u8)(vi->VihX >> 8);
	priv->tx_buf[3] = (u8)vi->VihX;
	priv->tx_buf[4] = (u8)(vi->VilY >> 8);
	priv->tx_buf[5] = (u8)vi->VilY;
	priv->tx_buf[6] = (u8)(vi->VihY >> 8);
	priv->tx_buf[7] = (u8)vi->VihY;
	
	printk("Frances VILH TX: tx0: %d, tx2: %d, tx4: %d, tx6: %d\n", priv->tx_buf[0], priv->tx_buf[2], priv->tx_buf[4], priv->tx_buf[6]);
	printk("Frances VILH TX: tx1: %d, tx3: %d, tx5: %d, tx7: %d\n", priv->tx_buf[1], priv->tx_buf[3], priv->tx_buf[5], priv->tx_buf[7]);
}

/** ready for rx buf for cnt **/
static void GetReady4rx2Cnt(struct AFA750_I2C *priv, struct TS_CONSAP_CntAllContent *cnt){
   
    cnt->CntX = (s16)(priv->rx_buf2[0]|(priv->rx_buf2[1] << 8));
	cnt->CntY = (s16)(priv->rx_buf2[2]|(priv->rx_buf2[3] << 8));
	cnt->CntZ = (s16)(priv->rx_buf2[4]|(priv->rx_buf2[5] << 8));
}

/** ready for rx buf for gain **/
static void GetReady4rx2Gain(struct AFA750_I2C *priv, struct TS_CONSAP_GainContent *gain){
    printk("Frances gain: rx0: %d, rx2: %d, rx4: %d\n", priv->rx_buf[0], priv->rx_buf[2], priv->rx_buf[4]);
	printk("Frances gain: rx1: %d, rx3: %d, rx5: %d\n", priv->rx_buf[1], priv->rx_buf[3], priv->rx_buf[5]);
	
    gain->GainX = (u16)(priv->rx_buf[0] << 8|priv->rx_buf[1]);
	gain->GainY = (u16)(priv->rx_buf[2] << 8|priv->rx_buf[3]);
	gain->GainZ = (u16)(priv->rx_buf[4] << 8|priv->rx_buf[5]);
}

/** TS_CONSAP_VilVihContent **/
static void GetReady4rx2VILH(struct AFA750_I2C *priv, struct TS_CONSAP_VilVihContent *vi){
        printk("Frances0: rx0: %d, rx2: %d, rx4: %d, rx6: %d\n", priv->rx_buf2[0], priv->rx_buf2[2], priv->rx_buf2[4], priv->rx_buf2[6]);
	printk("Frances0: rx1: %d, rx3: %d, rx5: %d, rx7: %d\n", priv->rx_buf2[1], priv->rx_buf2[3], priv->rx_buf2[5], priv->rx_buf2[7]);
        vi->VilX = (u16)(priv->rx_buf2[0] << 8|priv->rx_buf2[1]);
	vi->VihX = (u16)(priv->rx_buf2[2] << 8|priv->rx_buf2[3]);
	vi->VilY = (u16)(priv->rx_buf2[4] << 8|priv->rx_buf2[5]);
	vi->VihY = (u16)(priv->rx_buf2[6] << 8|priv->rx_buf2[7]);
}

static void ConsAP_GetAVGCntOne(struct AFA750_I2C *priv, struct TS_CONSAP_CntAllContent *cnt)
{
	u8 AvgCount = 0;
	struct TS_CONSAP_CntAllContent tmp_cnt;
	s32 Tmp_CNT_X, Tmp_CNT_Y, Tmp_CNT_Z;

	Tmp_CNT_X = 0;
	Tmp_CNT_Y = 0;
	Tmp_CNT_Z = 0;
	
	
	
	for(AvgCount = 0; AvgCount< AvgDepth; AvgCount++)
	{

	    i2c_read_op_len(priv, AFA750_RD_OP, AFA750_RREG, AFA750_RD_CNT*2);
		GetReady4rx2Cnt(priv, &tmp_cnt);
		
		Tmp_CNT_X += tmp_cnt.CntX;
		Tmp_CNT_Y += tmp_cnt.CntY;
		Tmp_CNT_Z += tmp_cnt.CntZ;
		
		mdelay(2);
	}
	
	cnt->CntX = (s16) (Tmp_CNT_X/AvgDepth);
	cnt->CntY = (s16) (Tmp_CNT_Y/AvgDepth);
	cnt->CntZ = (s16) (Tmp_CNT_Z/AvgDepth);	
}

static int ReadOTP(struct AFA750_I2C *priv, struct TS_CONSAP_GainContent *gain, u8 addr, u16 u16Length)
{
    u8 i, j;
    u8 cnt = 5;
	
    for (i = addr, j = 0; i < (addr + u16Length); i++)
			{
				// Load address
				priv->tx_buf[0] = i;
				i2c_write_op(priv, AFA750_WR_OP, AFA750_OTP_ADDR_REG, 1);
				
				// Start reading
				priv->tx_buf[0] = OTP_CTRL_OTP_RD;
				i2c_write_op(priv, AFA750_WR_OP, AFA750_OTP_CTL_REG, 1);

                mdelay(1);
				// Wait for the bit OTP_CTRL_READ is deactivated
				i2c_read_op_len(priv, AFA750_RD_OP, AFA750_OTP_CTL_REG, 1);
				while(priv->rx_buf2[0] != 0x00 && cnt--){ //times??
				    i2c_read_op_len(priv, AFA750_RD_OP, AFA750_OTP_CTL_REG, 1);
				    mdelay(1);
				}
				if(priv->rx_buf2[0] == 0x00){
					i2c_read_op_len(priv, AFA750_RD_OP, AFA750_OTP_DOUT_REG, 1);
					priv->rx_buf[j++] = priv->rx_buf2[0];
				}else{
					printk("Frances: %s() failed\n",
					 __func__);
			        //while(1);
					return -1;
                }				
			}
			
			GetReady4rx2Gain(priv, gain);
			return 0;
}
/**************************************************/
/*          Special Function Declaration         */
/**************************************************/
void ConsAP_OneTouchCalibration(struct i2c_client *client, void* Cali)
{
	u8 Full_Scale_tmp;
	struct TS_CONSAP_VilVihContent ViBackup;
	struct TS_CONSAP_VilVihContent newViBackup;
	struct TS_CONSAP_VilVihContent_Tmp newViBackupTmp;
	struct TS_CONSAP_CntAllContent ptsContent;
	struct TS_CONSAP_GainContent pGainGet;
	struct TS_CONSAP_GainContent nGainGet;
	int ret;
	int retflag = 0;
	
	u16 VI_STEP_X = 0;
	u16 VI_STEP_Y = 0;
	
	int ConsAP_GDRatio_X, ConsAP_GDRatio_Y, ConsAP_GDRatio_Z;
	int ConsAP_TempZeroOffset_X, ConsAP_TempZeroOffset_Y, ConsAP_TempZeroOffset_Z;
	
	struct AFA750_I2C i2c_content;
	struct AFA750_CALI *getCali;
	
	i2c_content.client = client;

	getCali = (struct AFA750_CALI*)Cali;
	
	memset(getCali, 0x00, sizeof(struct AFA750_CALI));

	/*
	BSP_AFM314_GetVILH_I2C(AFM_CH_SEL_X, &(pViBackup->VilX), &(pViBackup->VihX));
	BSP_AFM314_GetVILH_I2C(AFM_CH_SEL_Y, &(pViBackup->VilY), &(pViBackup->VihY));
	*/
	
	 i2c_read_op_len(&i2c_content, AFA750_RD_OP, AFA750_VILH_REG, AFA750_RD_VILH*2);
     GetReady4rx2VILH(&i2c_content, &ViBackup);
	 Get_VILH_Convert(&ViBackup);
	
	

    i2c_read_op_len(&i2c_content, AFA750_RD_OP, AFA750_FS_CTL, 1);
	Full_Scale_tmp = i2c_content.rx_buf2[0];
	if(Full_Scale_tmp == AFM_SCALE_2G)
	{
		//Get Z Axis 2G Gain
		ret = ReadOTP(&i2c_content, &pGainGet, 0x40, 6);
		if(ret < 0) retflag = 1;
		ret = ReadOTP(&i2c_content, &nGainGet, 0x52, 6);
		if(ret < 0) retflag = 1;
		/*
		BSP_AFM314_ReadOTP_I2C((U8*) pGainGet, (sizeof(TS_CONSAP_GainContent)), 0x40, 3);
		BSP_AFM314_ReadOTP_I2C((U8*) nGainGet, (sizeof(TS_CONSAP_GainContent)), 0x52, 3);
	    */
	}
	else if(Full_Scale_tmp == AFM_SCALE_4G)
	{
		//Get Z Axis 4G Gain
		ret = ReadOTP(&i2c_content, &pGainGet, 0x46, 6);
		if(ret < 0) retflag = 1;
		ret = ReadOTP(&i2c_content, &nGainGet, 0x58, 6);
		if(ret < 0) retflag = 1;
		/*
		BSP_AFM314_ReadOTP_I2C((U8*) pGainGet, (sizeof(TS_CONSAP_GainContent)), 0x46, 3);
		BSP_AFM314_ReadOTP_I2C((U8*) nGainGet, (sizeof(TS_CONSAP_GainContent)), 0x58, 3);
	    */
	}
	else
	{
		//Get Z Axis 8G Gain
		ret = ReadOTP(&i2c_content, &pGainGet, 0x4C, 6);
		if(ret < 0) retflag = 1;
		ret = ReadOTP(&i2c_content, &nGainGet, 0x5E, 6);
		if(ret < 0) retflag = 1;
		
		printk("Frances2: pGainX: %d, pGainY: %d, pGainZ: %d\n", pGainGet.GainX, pGainGet.GainY, pGainGet.GainZ);
		printk("Frances2: nGainX: %d, nGainY: %d, nGainZ: %d\n", nGainGet.GainX, nGainGet.GainY, nGainGet.GainZ);
		/*
		BSP_AFM314_ReadOTP_I2C((U8*) pGainGet, (sizeof(TS_CONSAP_GainContent)), 0x4C, 3);
		BSP_AFM314_ReadOTP_I2C((U8*) nGainGet, (sizeof(TS_CONSAP_GainContent)), 0x5E, 3);
	    */
	} 

	mdelay(60);
	ConsAP_GetAVGCntOne(&i2c_content, &ptsContent);
        printk("Frances3: CntX: %d, CntY: %d, CntZ: %d\n", ptsContent.CntX, ptsContent.CntY, ptsContent.CntZ);
	
	ConsAP_TempZeroOffset_X = ptsContent.CntX;
	ConsAP_TempZeroOffset_Y = ptsContent.CntY;
	if(ptsContent.CntZ > 0){
	    ConsAP_TempZeroOffset_Z = ptsContent.CntZ - (pGainGet.GainZ << 4);
	}else{
	    ConsAP_TempZeroOffset_Z = (nGainGet.GainZ << 4) + ptsContent.CntZ;
	}
	if(retflag == 1){
	     getCali->offset[0] = ConsAP_TempZeroOffset_X;
	     getCali->offset[1] = ConsAP_TempZeroOffset_Y;
	     getCali->offset[2] = ConsAP_TempZeroOffset_Z;
	 
	     getCali->gain[0] = 1 * AFA750_FLOAT_SCALING;
	     getCali->gain[1] = 1 * AFA750_FLOAT_SCALING;
	     getCali->gain[2] = 1 * AFA750_FLOAT_SCALING;
		 
		 return;
	}
	
	VI_STEP_X = ((ViBackup.VilX - ViBackup.VihX)+(VI_AvgDiffRatio/2))/VI_AvgDiffRatio;
	VI_STEP_Y = ((ViBackup.VilY - ViBackup.VihY)+(VI_AvgDiffRatio/2))/VI_AvgDiffRatio;
	
	printk("Frances STEP: X: %d, Y: %d\n", VI_STEP_X, VI_STEP_Y);
	
	ConsAP_GDRatio_X = 0.0;
	ConsAP_GDRatio_Y = 0.0;
	ConsAP_GDRatio_Z = 1.0;
	
	//VI_Avg + VI_AvgDiff
	newViBackupTmp.VilX = ViBackup.VilX+VI_STEP_X; // > 1024 => gain = 1
	newViBackupTmp.VihX = ViBackup.VihX+VI_STEP_X;
	newViBackupTmp.VilY = ViBackup.VilY+VI_STEP_Y;
	newViBackupTmp.VihY = ViBackup.VihY+VI_STEP_Y;
	
	if(newViBackupTmp.VilX > 1023 || newViBackupTmp.VihX > 1023 || newViBackupTmp.VilY > 1023 || newViBackupTmp.VihY > 1023){
	     getCali->offset[0] = ConsAP_TempZeroOffset_X;
	     getCali->offset[1] = ConsAP_TempZeroOffset_Y;
	     getCali->offset[2] = ConsAP_TempZeroOffset_Z;
	 
	     getCali->gain[0] = 1 * AFA750_FLOAT_SCALING;
	     getCali->gain[1] = 1 * AFA750_FLOAT_SCALING;
	     getCali->gain[2] = (pGainGet.GainZ*AFA750_FLOAT_SCALING) / nGainGet.GainZ;
		 
		 Set_VILH_Convert(&ViBackup);
	     GetReady4VILH2tx(&i2c_content, &ViBackup);
	     i2c_write_op(&i2c_content, AFA750_WR_OP, AFA750_VILH_REG, AFA750_RD_VILH*2);
		 
		 mdelay(60);
		 
		 return;
	}
	
    newViBackup.VilX = ViBackup.VilX+VI_STEP_X; // > 1024 => gain = 1
	newViBackup.VihX = ViBackup.VihX+VI_STEP_X;
	newViBackup.VilY = ViBackup.VilY+VI_STEP_Y;
	newViBackup.VihY = ViBackup.VihY+VI_STEP_Y;
	
	Set_VILH_Convert(&newViBackup);
	GetReady4VILH2tx(&i2c_content, &newViBackup);
	i2c_write_op(&i2c_content, AFA750_WR_OP, AFA750_VILH_REG, AFA750_RD_VILH*2);
	/*
	BSP_AFM314_SetVILH_I2C(AFM_CH_SEL_X, pViBackup->VilX+VI_STEP_X, pViBackup->VihX+VI_STEP_X);
	BSP_AFM314_SetVILH_I2C(AFM_CH_SEL_Y, pViBackup->VilY+VI_STEP_Y, pViBackup->VihY+VI_STEP_Y);
	*/
	mdelay(60);
	
	ConsAP_GetAVGCntOne(&i2c_content, &ptsContent);
        printk("Frances4: CntX: %d, CntY: %d, CntZ: %d\n", ptsContent.CntX, ptsContent.CntY, ptsContent.CntZ);
	
	ConsAP_GDRatio_X += (ptsContent.CntX - ConsAP_TempZeroOffset_X);
	ConsAP_GDRatio_Y += (ptsContent.CntY - ConsAP_TempZeroOffset_Y);
	
	//VI_Avg - VI_AvgDiff
	/*
	newViBackup.VilX = ViBackup.VilX-VI_STEP_X; // < 0 => gain =1
	newViBackup.VihX = ViBackup.VihX-VI_STEP_X;
	newViBackup.VilY = ViBackup.VilY-VI_STEP_Y;
	newViBackup.VihY = ViBackup.VihY-VI_STEP_Y;
	*/
	//if(newViBackup.VilX < 0 || newViBackup.VihX < 0 || newViBackup.VilY < 0 || newViBackup.VihY < 0){
	//if((newViBackup.VilX & 0x200) || (newViBackup.VihX & 0x200) || (newViBackup.VilY & 0x200) || (newViBackup.VihY & 0x200)){
	if((VI_STEP_X > ViBackup.VilX) || (VI_STEP_X > ViBackup.VihX) || (VI_STEP_Y > ViBackup.VilY) || (VI_STEP_Y > ViBackup.VihY)){
	     getCali->offset[0] = ConsAP_TempZeroOffset_X;
	     getCali->offset[1] = ConsAP_TempZeroOffset_Y;
	     getCali->offset[2] = ConsAP_TempZeroOffset_Z;
	 
	     getCali->gain[0] = 1 * AFA750_FLOAT_SCALING;
	     getCali->gain[1] = 1 * AFA750_FLOAT_SCALING;
	     getCali->gain[2] = (pGainGet.GainZ*AFA750_FLOAT_SCALING) / nGainGet.GainZ;
		 
		 Set_VILH_Convert(&ViBackup);
	     GetReady4VILH2tx(&i2c_content, &ViBackup);
	     i2c_write_op(&i2c_content, AFA750_WR_OP, AFA750_VILH_REG, AFA750_RD_VILH*2);
		 
		 mdelay(60);
		 
		 return;
	}
	
	newViBackup.VilX = ViBackup.VilX-VI_STEP_X; // < 0 => gain =1
	newViBackup.VihX = ViBackup.VihX-VI_STEP_X;
	newViBackup.VilY = ViBackup.VilY-VI_STEP_Y;
	newViBackup.VihY = ViBackup.VihY-VI_STEP_Y;
	
	Set_VILH_Convert(&newViBackup);
	GetReady4VILH2tx(&i2c_content, &newViBackup);
	i2c_write_op(&i2c_content, AFA750_WR_OP, AFA750_VILH_REG, AFA750_RD_VILH*2);
	/*
	BSP_AFM314_SetVILH_I2C(AFM_CH_SEL_X, pViBackup->VilX-VI_STEP_X, pViBackup->VihX-VI_STEP_X);
	BSP_AFM314_SetVILH_I2C(AFM_CH_SEL_Y, pViBackup->VilY-VI_STEP_Y, pViBackup->VihY-VI_STEP_Y);
	*/
	mdelay(60);
	
	ConsAP_GetAVGCntOne(&i2c_content, &ptsContent);
	printk("Frances5: CntX: %d, CntY: %d, CntZ: %d\n", ptsContent.CntX, ptsContent.CntY, ptsContent.CntZ);

	ConsAP_GDRatio_X = (ConsAP_GDRatio_X*AFA750_FLOAT_SCALING) / (ConsAP_TempZeroOffset_X - ptsContent.CntX);
	ConsAP_GDRatio_Y = (ConsAP_GDRatio_Y*AFA750_FLOAT_SCALING) / (ConsAP_TempZeroOffset_Y - ptsContent.CntY);
	ConsAP_GDRatio_Z = (pGainGet.GainZ*AFA750_FLOAT_SCALING) / nGainGet.GainZ;
	
	//Set Default VI_Avg
    Set_VILH_Convert(&ViBackup);
	GetReady4VILH2tx(&i2c_content, &ViBackup);
	i2c_write_op(&i2c_content, AFA750_WR_OP, AFA750_VILH_REG, AFA750_RD_VILH*2);
	/*
	BSP_AFM314_SetVILH_I2C(AFM_CH_SEL_X, pViBackup->VilX, pViBackup->VihX);
	BSP_AFM314_SetVILH_I2C(AFM_CH_SEL_Y, pViBackup->VilY, pViBackup->VihY);
		*/
	mdelay(60);
		/*** return!!!***/
     getCali->offset[0] = ConsAP_TempZeroOffset_X;
	 getCali->offset[1] = ConsAP_TempZeroOffset_Y;
	 getCali->offset[2] = ConsAP_TempZeroOffset_Z;
	 
	 getCali->gain[0] = ConsAP_GDRatio_X;
	 getCali->gain[1] = ConsAP_GDRatio_Y;
	 getCali->gain[2] = ConsAP_GDRatio_Z;
	 
	 printk("Frances final: OffsetX: %d, OffsetY: %d, OffsetZ: %d\n", ConsAP_TempZeroOffset_X, ConsAP_TempZeroOffset_Y, ConsAP_TempZeroOffset_Z);
	 printk("Frances final: GainX: %d, GainY: %d, GainZ: %d\n", ConsAP_GDRatio_X, ConsAP_GDRatio_Y, ConsAP_GDRatio_Z);
}
/***
void _ConsAP_GetCntAll(TS_JobMgr_JobDscp* ptsJob) reentrant
{
	BSP_ICU_EnableAllInt(FALSE);
	
	ptsContent = (TS_CONSAP_CntAllContent*) DS_PTCL_GetBuf(JobMgr_GetRspDtg(ptsJob));
				
	while (BSP_EXT0_GetFlag() == FALSE);	// Poll interrupt flag until the flag is set
	BSP_EXT0_SetFlag(FALSE);				// Clean interrupt flag
	ptsContent->CntX = BSP_AFM314_GetCnt_I2C(AFM_CH_SEL_X);
	ptsContent->CntY = BSP_AFM314_GetCnt_I2C(AFM_CH_SEL_Y);
	ptsContent->CntZ = BSP_AFM314_GetCnt_I2C(AFM_CH_SEL_Z);
			
	//One toutch Calibration
	if(ConsAP_InsAutoCalibrationActive)
	{
		ptsContent->CntX -= ConsAP_TempZeroOffset_X;
		ptsContent->CntY -= ConsAP_TempZeroOffset_Y;
		ptsContent->CntZ -= ConsAP_TempZeroOffset_Z;
					
		if (ptsContent->CntX < 0)
		{
			ptsContent->CntX = (S16)(((float)ptsContent->CntX)*ConsAP_GDRatio_X);
			if(ptsContent->CntX < -2048)
			{
				ptsContent->CntX = -2048;
			}
		}
					
		if (ptsContent->CntY < 0)
		{
			ptsContent->CntY = (S16)(((float)ptsContent->CntY)*ConsAP_GDRatio_Y);
			if(ptsContent->CntY < -2048)
			{
				ptsContent->CntY = -2048;
			}
		}
				
		if (ptsContent->CntZ < 0)
		{
			ptsContent->CntZ = (S16)(((float)ptsContent->CntZ)*ConsAP_GDRatio_Z);
			if(ptsContent->CntZ < -2048)
			{
				ptsContent->CntZ = -2048;
			}
		}
	}
}
***/
