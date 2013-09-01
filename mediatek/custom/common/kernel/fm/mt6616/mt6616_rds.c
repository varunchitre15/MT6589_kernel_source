#include "mt6616_fm.h"
                  
#if FM_RDS_ENABLE
/******************************************************************************
 * GLOBAL DATA
 *****************************************************************************/
#define MT6616_RDS_BLER_TH1 90
#define MT6616_RDS_BLER_TH2 60
#define MT6616_RDS_BLER_C1  12
#define MT6616_RDS_BLER_C2  6
#define MT6616_RDS_BLER_T1  5000
#define MT6616_RDS_BLER_T2  5000

//FM_RDS_DATA_CRC_FFOST(0xB2)
#define FM_RDS_GDBK_IND_A	 (0x08)	
#define FM_RDS_GDBK_IND_B	 (0x04)	
#define FM_RDS_GDBK_IND_C	 (0x02)	
#define FM_RDS_GDBK_IND_D	 (0x01)	
#define FM_RDS_DCO_FIFO_OFST (0x01E0)
#define	FM_RDS_READ_DELAY	 (0x80)

extern uint16_t g_dbg_level;
/******************************************************************************
 * GLOBAL VARIABLE
 *****************************************************************************/
static bool bRDS_FirstIn = false;
static uint16_t RDS_Sync_Cnt = 0, RDS_Block_Reset_Cnt = 0;
static bool PreTextABFlag;
extern int16_t _current_frequency;
uint32_t gBLER_CHK_INTERVAL = 5000;
static int16_t preAF_Num = 0;
static int16_t preAFON_Num = 0;
uint16_t GOOD_BLK_CNT = 0, BAD_BLK_CNT = 0;
uint8_t BAD_BLK_RATIO = 0;


/******************************************************************************
 * Local function extern
 *****************************************************************************/
static bool MT6616_RDS_support(struct i2c_client *client);
static void MT6616_RDS_enable(struct i2c_client *client);
static void MT6616_RDS_disable(struct i2c_client *client);
static uint16_t MT6616_RDS_Get_GoodBlock_Counter(struct i2c_client *client);
static uint16_t MT6616_RDS_Get_BadBlock_Counter(struct i2c_client *client);
static void MT6616_RDS_Reset_Block_Counter(struct i2c_client *client);
static void MT6616_RDS_Reset(struct i2c_client *client);
static void MT6616_RDS_Reset_Block(struct i2c_client *client);
static void MT6616_RDS_GetData(struct i2c_client *client, uint16_t *data, uint16_t datalen);
static void MT6616_RDS_Init_Data(RDSData_Struct *pstRDSData);
static void MT6616_RDS_RetrieveGroup0(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData);
static void MT6616_RDS_RetrieveGroup1(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData);
static void MT6616_RDS_RetrieveGroup2(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData);
static void MT6616_RDS_RetrieveGroup4(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData);
static void MT6616_RDS_RetrieveGroup14(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData);



//implement by LCH
static bool MT6616_RDS_support(struct i2c_client *client)
{
    uint16_t tmp_reg;
    
    MT6616_write(client, FM_MAIN_PGSEL, 0x0001);
	MT6616_read(client, 0x79, &tmp_reg);
	MT6616_write(client, FM_MAIN_PGSEL, 0x0);
	if(tmp_reg&0x4000) //RDS not Support
	{
		return false;
	}
	else //RDS Support
	{
		return true;
	}
}

static void MT6616_RDS_enable(struct i2c_client *client)
{
	MT6616_write(client, FM_RDS_FFOST_TRIG_TH, 0xC296);  //0xB3
	MT6616_set_bits(client, FM_MAIN_EXTINTRMASK, FM_EXT_RDS_MASK, 0xFFDF);
	MT6616_set_bits(client, FM_MAIN_CTRL, 0x0010, 0xFFEF);
	MT6616_write(client, FM_RDS_BER_THD_SET_REG, 0x2d0); //0xA7	
}

static void MT6616_RDS_disable(struct i2c_client *client)
{
    MT6616_set_bits(client, FM_MAIN_CTRL, 0x0, 0xFFEF);
    MT6616_set_bits(client, FM_MAIN_EXTINTRMASK, 0x0, 0xFFDF);
}

static uint16_t MT6616_RDS_Get_GoodBlock_Counter(struct i2c_client *client)
{
    uint16_t tmp_reg;
    MT6616_read(client, FM_RDS_GOODBK_CNT, &tmp_reg);
    return tmp_reg;  
}

static uint16_t MT6616_RDS_Get_BadBlock_Counter(struct i2c_client *client)
{
    uint16_t tmp_reg;
    MT6616_read(client, FM_RDS_BADBK_CNT, &tmp_reg);
    return tmp_reg;  
}


static void MT6616_RDS_Reset_Block_Counter(struct i2c_client *client)
{
    MT6616_write(client, 0xAC, 0x0003);  //0xB3   
}

static void MT6616_RDS_Reset(struct i2c_client *client)
{
    MT6616_write(client, FM_RDS_RESET, 0x0001);  //0xB3 
}

static void MT6616_RDS_Reset_Block(struct i2c_client *client)
{
    MT6616_write(client, 0x84, 0x8702);
	Delayus(85);
	MT6616_write(client, 0x84, 0x0702);
}

void MT6616_RDS_BlerCheck(struct fm *fm)
{
    struct i2c_client *client = fm->i2c_client;
	RDSData_Struct *pstRDSData = fm->pstRDSData;
    uint16_t TOTAL_CNT;
	WCN_DBG(L7|D_BLKC, "pstRDSData->AF_Data.Addr_Cnt=0x%x\n", pstRDSData->AF_Data.Addr_Cnt);
    if(pstRDSData->AF_Data.Addr_Cnt == 0xFF) //AF List Finished
	{
	    pstRDSData->event_status |= RDS_EVENT_AF;  //Need notfiy application
	    //loop pstRDSData->event_status then act 
        if(pstRDSData->event_status != 0)
        {
            fm->RDS_Data_ready = true;
            wake_up_interruptible(&fm->read_wait);
            WCN_DBG(L6|D_BLKC, "RDS_EVENT_AF, trigger read\n");
        }
	}
	
	gBLER_CHK_INTERVAL = MT6616_RDS_BLER_T1;
	GOOD_BLK_CNT = MT6616_RDS_Get_GoodBlock_Counter(client);
	BAD_BLK_CNT = MT6616_RDS_Get_BadBlock_Counter(client);
	TOTAL_CNT = GOOD_BLK_CNT + BAD_BLK_CNT;
	MT6616_RDS_Reset_Block_Counter(client);
	
	if((GOOD_BLK_CNT==0)&&(BAD_BLK_CNT==0))
    {
        BAD_BLK_RATIO = 0;
    }
	else {
        BAD_BLK_RATIO = (BAD_BLK_CNT*100)/TOTAL_CNT;
    }

	//MT6616_RDS_BLER_TH1 90
	//MT6616_RDS_BLER_TH2 60
	//MT6616_RDS_BLER_C1  12
	//MT6616_RDS_BLER_C2  6
	//MT6616_RDS_BLER_T2  5000
	if((BAD_BLK_RATIO < MT6616_RDS_BLER_TH2)&&(RDS_Sync_Cnt > MT6616_RDS_BLER_C1))
	{
		gBLER_CHK_INTERVAL = MT6616_RDS_BLER_T2;
		if(RDS_Block_Reset_Cnt > 1)
			RDS_Block_Reset_Cnt--;
	}
	else
	{
		if(BAD_BLK_RATIO > MT6616_RDS_BLER_TH1)  //>90%
		{
			MT6616_RDS_Reset_Block_Counter(client);
			RDS_Sync_Cnt = 0;   //need clear or not, Question, LCH.
			RDS_Block_Reset_Cnt++;
			if((RDS_Block_Reset_Cnt > MT6616_RDS_BLER_C2)||bRDS_FirstIn)
			{
                if(bRDS_FirstIn)
				    bRDS_FirstIn = false;
				MT6616_RDS_Reset(client);
				RDS_Block_Reset_Cnt = 0;
			}
			else if(TOTAL_CNT > 12)  //LCH question 2, why 12???
			{
				MT6616_RDS_Reset_Block(client);
			}
		}
		else
		{    
			RDS_Sync_Cnt++; //(60%-90%)
			if(RDS_Block_Reset_Cnt > 1)
				RDS_Block_Reset_Cnt--;
			if(RDS_Sync_Cnt > MT6616_RDS_BLER_C1)
			{
				gBLER_CHK_INTERVAL = MT6616_RDS_BLER_T2;
			}
		}
	}
	WCN_DBG(L6|D_BLKC, "BLER: TOTAL_CNT:%d BAD_BLK_CNT:%d, GOOD_BLK_CNT:%d, BAD_BLK_RATIO:%d, RDS_Block_Reset_Cnt:%d, RDS_Sync_Cnt:%d\n", TOTAL_CNT, BAD_BLK_CNT, GOOD_BLK_CNT, BAD_BLK_RATIO, RDS_Block_Reset_Cnt, RDS_Sync_Cnt);
}

static void MT6616_RDS_GetData(struct i2c_client *client, uint16_t *data, uint16_t datalen)
{
    if(datalen < 5)
    {
        WCN_DBG(L4|D_ALL, "RDS_GetData buffer not enough, data len:%d!!\n", datalen);
        return;
    }

    MT6616_read(client, FM_RDS_DATA_A_E_REG, &data[0]);
	MT6616_read(client, FM_RDS_DATA_B_E_REG, &data[1]);
	MT6616_read(client, FM_RDS_DATA_C_E_REG, &data[2]);
	MT6616_read(client, FM_RDS_DATA_D_E_REG, &data[3]);
	MT6616_read(client, FM_RDS_DATA_CRC_FFOST, &data[4]);
	WCN_DBG(L7|D_RAW, "+++++++%04x %04x %04x %04x %04x", data[0], data[1], data[2], data[3], data[4]);
}

static void MT6616_RDS_Init_Data(RDSData_Struct *pstRDSData)
{
	uint8_t indx;

    memset(pstRDSData, 0 ,sizeof(RDSData_Struct)); 
    bRDS_FirstIn = true;
	
	for(indx = 0; indx < 64; indx++)
	{
		pstRDSData->RT_Data.TextData[0][indx]=0x20;
		pstRDSData->RT_Data.TextData[1][indx]=0x20;		
	}
	for(indx = 0; indx < 8; indx++)
	{
		pstRDSData->PS_Data.PS[0][indx] = '\0';
		pstRDSData->PS_Data.PS[1][indx] = '\0';	
		pstRDSData->PS_Data.PS[2][indx] = '\0';
		pstRDSData->PS_ON[indx] = 0x20;
	}	
}

static void MT6616_RDS_RetrieveGroup0(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData)
{
    uint8_t indx, indx2, DI_Code, DI_Flag, PS_Num, AF_H, AF_L, num;
//    uint16_t tmp_reg;

	//SubType = (*(block_data+1)&0x0800)>>11;
	if(!(block_data[4]&FM_RDS_GDBK_IND_D))
	{
	    WCN_DBG(L7|D_RAW, "RDS_RetrieveGroup0 crc err!!\n");
    	return;
    }
    
	if(pstRDSData->RDSFlag.TA != ((block_data[1]&0x0010)>>4)) //TA=Traffic Announcement code
	{
		pstRDSData->RDSFlag.TA= (block_data[1]&0x0010)>>4;		   	
		pstRDSData->event_status |= RDS_EVENT_FLAGS;
		pstRDSData->RDSFlag.flag_status |= RDS_FLAG_IS_TA;	
	}
	if(pstRDSData->RDSFlag.Music!= ((block_data[1]&0x0008)>>3)) //M/S=music speech switch code
	{
		pstRDSData->RDSFlag.Music= (block_data[1]&0x0008)>>3;
		pstRDSData->event_status |= RDS_EVENT_FLAGS;
		pstRDSData->RDSFlag.flag_status |= RDS_FLAG_IS_MUSIC;			
	}
	if((pstRDSData->Switch_TP)&&(pstRDSData->RDSFlag.TP)&&!(pstRDSData->RDSFlag.TA))
		pstRDSData->event_status |= RDS_EVENT_TAON_OFF;
		
	if(!SubType) //Type A
	{
		if(block_data[4]&FM_RDS_GDBK_IND_C)
		{
		    AF_H = (block_data[2]&0xFF00)>>8;
		    AF_L= block_data[2]&0x00FF;

			if((AF_H > 224)&&(AF_H < 250)) //Followed AF Number, see RDS spec Table 11, valid(224-249)
			{
			    WCN_DBG(L7|D_G0, "RetrieveGroup0 AF number AF_H:%d, AF_L:%d\n", AF_H, AF_L);
				pstRDSData->AF_Data.isAFNum_Get = 0;
				preAF_Num = AF_H - 224; //AF Number
				if(preAF_Num != pstRDSData->AF_Data.AF_Num)
		        {
					pstRDSData->AF_Data.AF_Num = preAF_Num;
		        }
				else  //Get the same AFNum two times
				{
					pstRDSData->AF_Data.isAFNum_Get = 1;
				}
					
				if((AF_L < 205) && (AF_L > 0)) //See RDS Spec table 10, valid VHF
		        {
					pstRDSData->AF_Data.AF[0][0] = AF_L+875; //convert to 100KHz
					WCN_DBG(L7|D_G0, "RetrieveGroup0 AF[0][0]:%d\n", pstRDSData->AF_Data.AF[0][0]);
					if((pstRDSData->AF_Data.AF[0][0]) != (pstRDSData->AF_Data.AF[1][0]))
				    {
						pstRDSData->AF_Data.AF[1][0] = pstRDSData->AF_Data.AF[0][0];
				    }
				    else				        
					{
						if(pstRDSData->AF_Data.AF[1][0] !=  _current_frequency)
							pstRDSData->AF_Data.isMethod_A = 1;
						else
							pstRDSData->AF_Data.isMethod_A = 0;
					}

					WCN_DBG(L7|D_G0, "RetrieveGroup0 isAFNum_Get:%d, isMethod_A:%d\n", pstRDSData->AF_Data.isAFNum_Get, pstRDSData->AF_Data.isMethod_A);

					//only one AF handle
					if((pstRDSData->AF_Data.isAFNum_Get)&& (pstRDSData->AF_Data.AF_Num == 1))
				    {
				        pstRDSData->AF_Data.Addr_Cnt = 0xFF;
				        pstRDSData->event_status |= RDS_EVENT_AF_LIST;
						WCN_DBG(L7|D_G0, "RetrieveGroup0 RDS_EVENT_AF_LIST update\n");
				    }					
				}				
			}				
			else if((pstRDSData->AF_Data.isAFNum_Get)&&(pstRDSData->AF_Data.Addr_Cnt != 0xFF)) //AF Num correct
			{
				num = pstRDSData->AF_Data.AF_Num;
				num = num>>1;				
                WCN_DBG(L7|D_G0, "RetrieveGroup0 +num:%d\n", num);
				
				//Put AF freq into buffer and check if AF freq is repeat again
			    for(indx = 1; indx < (num+1); indx++)
		       	{
                    if((AF_H == (pstRDSData->AF_Data.AF[0][2*num-1]))&&(AF_L == (pstRDSData->AF_Data.AF[0][2*indx])))
					{
					    WCN_DBG(L7|D_G0, "RetrieveGroup0 AF same as indx:%d\n", indx);
						break;
				    }
					else if(!(pstRDSData->AF_Data.AF[0][2*indx-1])) //null buffer
		            {
						pstRDSData->AF_Data.AF[0][2*indx-1] = AF_H+875; //convert to 100KHz
						pstRDSData->AF_Data.AF[0][2*indx] = AF_L+875;
						WCN_DBG(L7|D_G0, "RetrieveGroup0 AF[0][%d]:%d, AF[0][%d]:%d\n", 
                                   2*indx-1, pstRDSData->AF_Data.AF[0][2*indx-1], 2*indx, pstRDSData->AF_Data.AF[0][2*indx]);
						break;
					}
		       	}
				num = pstRDSData->AF_Data.AF_Num;
				WCN_DBG(L7|D_G0, "RetrieveGroup0 ++num:%d\n", num);
			    if(num > 0)
			    {
			        if((pstRDSData->AF_Data.AF[0][num-1]) != 0)
				    {
						num = num>>1;
						WCN_DBG(L7|D_G0, "RetrieveGroup0 +++num:%d\n", num);
						//arrange frequency from low to high:start
						for(indx = 1; indx < num; indx++)
						{
							for(indx2 = indx+1; indx2 < (num+1); indx2++)
							{
								AF_H = pstRDSData->AF_Data.AF[0][2*indx-1];
								AF_L = pstRDSData->AF_Data.AF[0][2*indx];
								if(AF_H > (pstRDSData->AF_Data.AF[0][2*indx2-1]))
								{
									pstRDSData->AF_Data.AF[0][2*indx-1] = pstRDSData->AF_Data.AF[0][2*indx2-1];
									pstRDSData->AF_Data.AF[0][2*indx] = pstRDSData->AF_Data.AF[0][2*indx2];
									pstRDSData->AF_Data.AF[0][2*indx2-1] = AF_H;
									pstRDSData->AF_Data.AF[0][2*indx2] = AF_L;
								}
								else if(AF_H == (pstRDSData->AF_Data.AF[0][2*indx2-1]))
								{
									if(AF_L > (pstRDSData->AF_Data.AF[0][2*indx2]))
									{
										pstRDSData->AF_Data.AF[0][2*indx-1] = pstRDSData->AF_Data.AF[0][2*indx2-1];
										pstRDSData->AF_Data.AF[0][2*indx] = pstRDSData->AF_Data.AF[0][2*indx2];
										pstRDSData->AF_Data.AF[0][2*indx2-1] = AF_H;
										pstRDSData->AF_Data.AF[0][2*indx2] = AF_L;
									}
								}
							}
						}
						//arrange frequency from low to high:end						
						//compare AF buff0 and buff1 data:start
						num = pstRDSData->AF_Data.AF_Num;
						indx2 = 0;
						
						for(indx = 0; indx < num; indx++)
		                {
                            if((pstRDSData->AF_Data.AF[1][indx]) == (pstRDSData->AF_Data.AF[0][indx]))
			                {
							    if(pstRDSData->AF_Data.AF[1][indx] != 0)
									indx2++;
						    }
						    else						    
							    pstRDSData->AF_Data.AF[1][indx] = pstRDSData->AF_Data.AF[0][indx];
					    }
						WCN_DBG(L7|D_G0, "RetrieveGroup0 indx2:%d, num:%d\n", indx2, num);
						//compare AF buff0 and buff1 data:end						
						if(indx2 == num)
					    {
						    pstRDSData->AF_Data.Addr_Cnt = 0xFF;
						    pstRDSData->event_status |= RDS_EVENT_AF_LIST;
							WCN_DBG(L7|D_G0, "AF RetrieveGroup0 RDS_EVENT_AF_LIST AF_Num:%d\n", pstRDSData->AF_Data.AF_Num);
						    for(indx = 0; indx < num; indx++)
						    {
								if((pstRDSData->AF_Data.AF[1][indx]) == 0)
							    {
								    pstRDSData->AF_Data.Addr_Cnt = 0x0F;
								    pstRDSData->event_status &= (~RDS_EVENT_AF_LIST);
							    }
						   }
					    }
					    else
					 	    pstRDSData->AF_Data.Addr_Cnt = 0x0F;
				    }
			    }
		    }
	    }
		int pnum;
			for (pnum=0;pnum<2;pnum++) {
				WCN_DBG(L7|D_G0, "AF [%d]  0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n", pnum, pstRDSData->AF_Data.AF[pnum][0], pstRDSData->AF_Data.AF[pnum][1], pstRDSData->AF_Data.AF[pnum][2], pstRDSData->AF_Data.AF[pnum][3], pstRDSData->AF_Data.AF[pnum][4], pstRDSData->AF_Data.AF[pnum][5], pstRDSData->AF_Data.AF[pnum][6], pstRDSData->AF_Data.AF[pnum][7], pstRDSData->AF_Data.AF[pnum][8], pstRDSData->AF_Data.AF[pnum][9], pstRDSData->AF_Data.AF[pnum][10], pstRDSData->AF_Data.AF[pnum][11], pstRDSData->AF_Data.AF[pnum][12], pstRDSData->AF_Data.AF[pnum][13], pstRDSData->AF_Data.AF[pnum][14], pstRDSData->AF_Data.AF[pnum][15], pstRDSData->AF_Data.AF[pnum][16], pstRDSData->AF_Data.AF[pnum][17], pstRDSData->AF_Data.AF[pnum][18], pstRDSData->AF_Data.AF[pnum][19], pstRDSData->AF_Data.AF[pnum][20], pstRDSData->AF_Data.AF[pnum][21], pstRDSData->AF_Data.AF[pnum][22], pstRDSData->AF_Data.AF[pnum][23], pstRDSData->AF_Data.AF[pnum][24]);
			}
	}
			   	
	/*DI_Code[1:0]:   "00" = d3 *
	  *               "01" = d2 *
	  *               "10" = d1 *
	  *               "11" = d0 */
			   	
	DI_Code = block_data[1]&0x0003;  //DI=decoder identification code.
	DI_Flag = (block_data[1]&0x0004)>>2;
				  
	switch(DI_Code)
	{
	    case 3:
  		    if(pstRDSData->RDSFlag.Stereo != DI_Flag)
  		    {
	   	    	pstRDSData->RDSFlag.Stereo = DI_Flag;
		    	pstRDSData->event_status |= RDS_EVENT_FLAGS;
		    	pstRDSData->RDSFlag.flag_status |= RDS_FLAG_IS_STEREO;
	        }
	   	    break;
        case 2:
  		    if(pstRDSData->RDSFlag.Artificial_Head != DI_Flag)
	        {
	   		    pstRDSData->RDSFlag.Artificial_Head = DI_Flag;
			    pstRDSData->event_status |= RDS_EVENT_FLAGS;
			    pstRDSData->RDSFlag.flag_status |= RDS_FLAG_IS_ARTIFICIAL_HEAD;
  		    }
	   	    break;
        case 1:
  		    if(pstRDSData->RDSFlag.Compressed != DI_Flag)
  		    {
	   		    pstRDSData->RDSFlag.Compressed = DI_Flag;
			    pstRDSData->event_status |= RDS_EVENT_FLAGS;
			    pstRDSData->RDSFlag.flag_status |= RDS_FLAG_IS_COMPRESSED;	
	        }
	      	break;
        case 0:
  		    if(pstRDSData->RDSFlag.Dynamic_PTY != DI_Flag)
	        {
	   		    pstRDSData->RDSFlag.Dynamic_PTY = DI_Flag;
			    pstRDSData->event_status |= RDS_EVENT_FLAGS;
			    pstRDSData->RDSFlag.flag_status |= RDS_FLAG_IS_DYNAMIC_PTY;	
  		    }
		    break;		
        default:
	      	break;			 
	}

	WCN_DBG(L7|D_G0, "PS +++++ buffer[0] 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", pstRDSData->PS_Data.PS[0][0], pstRDSData->PS_Data.PS[0][1], pstRDSData->PS_Data.PS[0][2], pstRDSData->PS_Data.PS[0][3], pstRDSData->PS_Data.PS[0][4], pstRDSData->PS_Data.PS[0][5], pstRDSData->PS_Data.PS[0][6], pstRDSData->PS_Data.PS[0][7]);
	WCN_DBG(L7|D_G0, "PS +++++ buffer[1] 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x", pstRDSData->PS_Data.PS[0][0], pstRDSData->PS_Data.PS[1][1], pstRDSData->PS_Data.PS[1][2], pstRDSData->PS_Data.PS[1][3], pstRDSData->PS_Data.PS[1][4], pstRDSData->PS_Data.PS[1][5], pstRDSData->PS_Data.PS[1][6], pstRDSData->PS_Data.PS[1][7]);

	PS_Num = block_data[1]&0x0003;  //Figure 12 Type 0 group.
    AF_H = pstRDSData->PS_Data.PS[0][2*PS_Num];
    AF_L = pstRDSData->PS_Data.PS[0][2*PS_Num+1];
    if((AF_H == (block_data[3])>>8)&&(AF_L == (block_data[3]&0xFF)))
	{
	    if((!((pstRDSData->event_status)&RDS_EVENT_PROGRAMNAME))&&((PS_Num == 0)||(pstRDSData->PS_Data.Addr_Cnt)))
		{
            pstRDSData->PS_Data.PS[1][2*PS_Num]=(block_data[3])>>8;
			pstRDSData->PS_Data.PS[1][2*PS_Num+1] = (block_data[3])&0xFF;			
			WCN_DBG(L7|D_G0, "RetrieveGroup0 PS second time, NUM:%x H:%x L:%x\n", 
				      PS_Num, pstRDSData->PS_Data.PS[1][2*PS_Num], pstRDSData->PS_Data.PS[1][2*PS_Num+1]);
			
			//Need clear buff0, LCH question 1, should clear not not?
			if((PS_Num == 0)&&(pstRDSData->PS_Data.Addr_Cnt == 0))
			{
				for(indx = 2; indx < 8; indx++)
				{
					pstRDSData->PS_Data.PS[0][indx] = '\0'; //clear buff0
				}
			}
			pstRDSData->PS_Data.Addr_Cnt |= 1<<PS_Num;
			WCN_DBG(L7|D_G0, "RetrieveGroup0, Addr_Cnt:%x\n", pstRDSData->PS_Data.Addr_Cnt);
			if(pstRDSData->PS_Data.Addr_Cnt == 0x0F)
			{
			    //Avoid PS transient:Start 
				num = 0;	
				for(indx = 0; indx < 8; indx++)
				{
					if(pstRDSData->PS_Data.PS[0][indx] == pstRDSData->PS_Data.PS[1][indx])
						num++;
				}
				pstRDSData->PS_Data.Addr_Cnt = 0;
			    //Avoid PS transient:END 
			    
				if(num == 8) // get same data 2 times
			    {
					WCN_DBG(L6|D_G0, "PS Set event RDS_EVENT_PROGRAMNAME\n");
				    num = 0;
				    for(indx = 0; indx < 8; indx++)
				    {
					    if(pstRDSData->PS_Data.PS[1][indx] == pstRDSData->PS_Data.PS[2][indx])
					        num++;
				    }
				    //if(num != 8) //get same data 2 times, and not same as the last show. 
					    pstRDSData->event_status |= RDS_EVENT_PROGRAMNAME;
					    
					for(indx = 0; indx < 8; indx++)
					{
						pstRDSData->PS_Data.PS[2][indx] = pstRDSData->PS_Data.PS[1][indx];
						pstRDSData->PS_Data.PS[1][indx] = '\0';
						pstRDSData->PS_Data.PS[0][indx] = '\0';
					}
				}
				else
				{
					pstRDSData->PS_Data.Addr_Cnt |= 1<<PS_Num;
				}
			}
		}
	}
	else
    {
        pstRDSData->PS_Data.PS[0][2*PS_Num]=(block_data[3])>>8;
		pstRDSData->PS_Data.PS[0][2*PS_Num+1] = (block_data[3])&0xFF;
		WCN_DBG(L7|D_G0, "RetrieveGroup0 PS, NUM:%x H:%x L:%x\n", 
			      PS_Num, pstRDSData->PS_Data.PS[0][2*PS_Num], pstRDSData->PS_Data.PS[0][2*PS_Num+1]);
    }
    
	if((pstRDSData->event_status)&RDS_EVENT_PROGRAMNAME)
	{
		PS_Num = 0;
		for(num = 0; num < 8;num++)
		{
			if(pstRDSData->PS_Data.PS[2][num] == '\0')
			    PS_Num |= 1<<num;
		}
		if(PS_Num == 0xFF)
		{
			WCN_DBG(L7|D_G0, "RDS PS Canncel event 0x08");
			pstRDSData->event_status &= (~RDS_EVENT_PROGRAMNAME);
	    }
	}   
}

static void MT6616_RDS_RetrieveGroup1(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData)
{
	uint8_t variant_code = (block_data[2]&0x7000)>>12;
	
	if(variant_code == 0)
	{	    
		pstRDSData->Extend_Country_Code = (uint8_t)block_data[2]&0xFF;
		WCN_DBG(L7|D_G1, "Extend_Country_Code:%d\n", pstRDSData->Extend_Country_Code);
	}
	else if(variant_code == 3)
    {
		pstRDSData->Language_Code = block_data[2]&0xFFF;
		WCN_DBG(L7|D_G1, "Language_Code:%d\n", pstRDSData->Language_Code);
	}
				
	pstRDSData->Radio_Page_Code = block_data[1]&0x001F;
	pstRDSData->Program_Item_Number_Code = block_data[3];
}

static void MT6616_RDS_RetrieveGroup2(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData)
{
    uint8_t TextAddr, indx, indx2, space, byte0, byte1;
	uint16_t addrcnt;
	TextAddr = (uint8_t)block_data[1]&0x0F;
	
	if(pstRDSData->RDSFlag.Text_AB != ((block_data[1]&0x0010)>>4))
	{
		pstRDSData->RDSFlag.Text_AB = (block_data[1]&0x0010)>>4;
		pstRDSData->event_status |= RDS_EVENT_FLAGS;
		pstRDSData->RDSFlag.flag_status |= RDS_FLAG_TEXT_AB;
		WCN_DBG(L6|D_G2, "RT RetrieveGroup2 TextABFlag: %x --> %x\n", PreTextABFlag, pstRDSData->RDSFlag.Text_AB);
	}

	if(PreTextABFlag != pstRDSData->RDSFlag.Text_AB)
	{
		/*DDB:Some station don't send 0x0D, it just switch TextAB if it want to send next message.*/
		addrcnt = 0xFFFF>>(0x0F-indx);
		if (pstRDSData->RT_Data.isRTDisplay == 0)// && (((pstRDSData->RT_Data.Addr_Cnt)&addrcnt) == addrcnt)) 
		{
			WCN_DBG(L6|D_G2, "RT TextAB changed and RT has not been show.\n");
			pstRDSData->event_status |= RDS_EVENT_LAST_RADIOTEXT; 
			space = 0;
			for(indx = 0; indx < 64; indx++)
			{
				if(pstRDSData->RT_Data.TextData[1][indx] == 0x20)
					space++;
			}	
			if(space == 64)
				pstRDSData->event_status &= (~RDS_EVENT_LAST_RADIOTEXT);

			if (pstRDSData->event_status & RDS_EVENT_LAST_RADIOTEXT) {
				/*DDB:Why TextData[1][0] NOT TextData[2][0],  Because some station just send a message one time, and then change TextAB, send another message, SUCH As Beijing 90.0*/
				memcpy(&(pstRDSData->RT_Data.TextData[3][0]), &(pstRDSData->RT_Data.TextData[1][0]), sizeof(pstRDSData->RT_Data.TextData[3]));

				uint8_t tmp[66] = {0};
				tmp[64] = 'E';
				memcpy(tmp, pstRDSData->RT_Data.TextData[1], 64);
			
				WCN_DBG(L6|D_G2, "RT Radio text---%s\n", tmp);
				WCN_DBG(L6|D_G2, "RT set RDS_EVENT_LAST_RADIOTEXT flag, no 0x0D case.\n");
				pstRDSData->RT_Data.isRTDisplay = 1;
			}
		}
		/*DDB, end*/
	    memset(&(pstRDSData->RT_Data.TextData[0][0]), 0x20, sizeof(pstRDSData->RT_Data.TextData[0]));
		memset(&(pstRDSData->RT_Data.TextData[1][0]), 0x20, sizeof(pstRDSData->RT_Data.TextData[1]));
		memset(&(pstRDSData->RT_Data.TextData[2][0]), 0x20, sizeof(pstRDSData->RT_Data.TextData[2]));
		PreTextABFlag = pstRDSData->RDSFlag.Text_AB;
  		pstRDSData->RT_Data.TextLength =  0;
		pstRDSData->RT_Data.GetLength = 0;
		pstRDSData->RT_Data.Addr_Cnt = 0;
		//pstRDSData->RT_Data.isRTDisplay = 0;
	}
	
	if(!SubType) //Type A
	{
		pstRDSData->RT_Data.isTypeA = 1;
	  	if(block_data[4]&(FM_RDS_GDBK_IND_C|FM_RDS_GDBK_IND_D))
        {
  			pstRDSData->RT_Data.TextData[0][4*TextAddr] = block_data[2]>>8;
			pstRDSData->RT_Data.TextData[0][4*TextAddr+1] = block_data[2]&0xFF;
  			pstRDSData->RT_Data.TextData[0][4*TextAddr+2] = block_data[3]>>8;
			pstRDSData->RT_Data.TextData[0][4*TextAddr+3] = block_data[3]&0xFF;
			space = 0;

            for(indx = 0; indx < 4;indx++)
	  	    {
	  		    byte0 = pstRDSData->RT_Data.TextData[0][4*TextAddr+indx];
			    byte1 = pstRDSData->RT_Data.TextData[1][4*TextAddr+indx];
				if (TextAddr == 0 && indx == 0) {	//if the first block lost?
					WCN_DBG(L6|D_G2, "RT Received the first block.\n");
					pstRDSData->RT_Data.isRTDisplay = 0;
				}

				if((!(pstRDSData->event_status&RDS_EVENT_LAST_RADIOTEXT))&&(byte0 == byte1)) //get the same byte 2 times
				{
					WCN_DBG(L7|D_G2, "RT put to TextData[2] %d 0x%x(%c)", 4*TextAddr+indx, byte0, byte0);
					space++;
					pstRDSData->RT_Data.TextData[2][4*TextAddr+indx] = byte0;
				}
				else
				{
					WCN_DBG(L7|D_G2, "RT put to TextData[1] %d 0x%x(%c)", 4*TextAddr+indx, byte0, byte0);
					pstRDSData->RT_Data.TextData[1][4*TextAddr+indx] = byte0;
				}
			}

			if(space == 4)
			{
                addrcnt = pstRDSData->RT_Data.Addr_Cnt;
				pstRDSData->RT_Data.Addr_Cnt |= (1<<TextAddr);
				//WCN_DBG(L7|D_G2, "RT RetrieveGroup2 RT addrcnt: 0x%x, RT_Data.Addr_Cnt: 0x%x\n", addrcnt, pstRDSData->RT_Data.Addr_Cnt);
				
				if(addrcnt == pstRDSData->RT_Data.Addr_Cnt)
				{
				    pstRDSData->RT_Data.BufCnt++;
				}
				else if(pstRDSData->RT_Data.BufCnt > 0)
				{
					pstRDSData->RT_Data.BufCnt--;
	  	        }
			}
	  	} else {
			WCN_DBG(L7|D_G2, "RT %04x %04x %04x %04x %04x CRC error.", block_data[0], block_data[1], block_data[2], block_data[3], block_data[4]);
		}
	  	for(indx = 0; indx < 4; indx++)
	  	{
	  		if(pstRDSData->RT_Data.TextData[2][4*TextAddr+indx] == 0x0D)
	        {
				WCN_DBG(L6|D_G2, "RT ---buffer[2] received 0x0D.\n");
	            pstRDSData->RT_Data.TextLength = 4*TextAddr+indx+1; //Add terminate charater
				pstRDSData->RT_Data.TextData[2][4*TextAddr+indx] = '\0';
				pstRDSData->RT_Data.GetLength = 1;
	        }
			else if((4*TextAddr+indx) == 63 && pstRDSData->RT_Data.Addr_Cnt == 0xffff) //type A full data. /*add by dongbo, make sure it's TextData[2], Not TextData[1]*/
			{
				WCN_DBG(L6|D_G2, "RT ---The 63 byte received.\n");
			    pstRDSData->RT_Data.TextLength = 4*TextAddr+indx+1;  //no terminal character
				pstRDSData->RT_Data.GetLength = 1;
			}
        }
	}
	else
	{
	    //FM_DEBUG("RetrieveGroup2 Type B RT NUM: 0x%x Text: 0x%x", TextAddr, block_data[3]);
		WCN_DBG(L7|D_G2, "RT %04x %04x %04x %04x %04x", block_data[0], block_data[1], block_data[2], block_data[3], block_data[4]);
		pstRDSData->RT_Data.isTypeA = 0;
		if(block_data[4]&FM_RDS_GDBK_IND_D)
		{
            pstRDSData->RT_Data.TextData[0][2*TextAddr] = block_data[3]>>8;
		    pstRDSData->RT_Data.TextData[0][2*TextAddr+1] = block_data[3]&0xFF;
			space = 0;
			
	  	    for(indx = 0; indx < 2; indx++)
	  	    {
	  	        byte0 = pstRDSData->RT_Data.TextData[0][2*TextAddr+indx];
			    byte1 = pstRDSData->RT_Data.TextData[1][2*TextAddr+indx];
				
				if((!((pstRDSData->event_status)&RDS_EVENT_LAST_RADIOTEXT))&&(byte0 == byte1))
				{
					space++;
					pstRDSData->RT_Data.TextData[2][2*TextAddr+indx] = byte0;
				}
				else
				{
					pstRDSData->RT_Data.TextData[1][2*TextAddr+indx] = byte0;
				}
	  		}
			if(space == 2)
			{
			    addrcnt = pstRDSData->RT_Data.Addr_Cnt;
				pstRDSData->RT_Data.Addr_Cnt |= (1<<TextAddr);
				WCN_DBG(L7|D_G2, "RT RetrieveGroup2 RT B addrcnt: 0x%x, RT_Data.Addr_Cnt: 0x%x\n", addrcnt, pstRDSData->RT_Data.Addr_Cnt);
				
                if(addrcnt == pstRDSData->RT_Data.Addr_Cnt)
				{
				    pstRDSData->RT_Data.BufCnt++;
				}
				else if(pstRDSData->RT_Data.BufCnt > 0)
				{
					pstRDSData->RT_Data.BufCnt--;
				}
            }
		} else {
			WCN_DBG(L7|D_G2, "RT %04x %04x %04x %04x %04x CRC error.", block_data[0], block_data[1], block_data[2], block_data[3], block_data[4]);
		}
		
	 	for(indx = 0; indx < 2; indx++)
	  	{
	  		if((pstRDSData->RT_Data.TextData[2][2*TextAddr+indx]) == 0x0D) //0x0D=end code
	  		{
	  		    pstRDSData->RT_Data.TextLength = 2*TextAddr+indx+1;  //Add terminate charater
				pstRDSData->RT_Data.TextData[2][2*TextAddr+indx] = '\0';
				pstRDSData->RT_Data.GetLength = 1;
	  		}
			else if((2*TextAddr+indx) == 31) //full data
			{
			    pstRDSData->RT_Data.TextLength = 2*TextAddr+indx+1;  //Add terminate charater
                pstRDSData->RT_Data.TextData[2][2*TextAddr+indx] = '\0';
				pstRDSData->RT_Data.GetLength = 1;
			}
	  	}		
	}

    //Check if text is fully received
	indx = TextAddr;
	if(pstRDSData->RT_Data.GetLength == 1)
	{
		addrcnt = 0xFFFF>>(0x0F-indx);
	}
	else if(pstRDSData->RT_Data.BufCnt > 100)
	{
	    pstRDSData->RT_Data.BufCnt = 0;
	    for(indx = 15; indx >= 0; indx--)
	    {
	        addrcnt = (pstRDSData->RT_Data.Addr_Cnt)&(1<<indx);
	        if(addrcnt)
	            break;
		}
		
		//get valid radio text length
		if (pstRDSData->RT_Data.isTypeA)
		{
		    for(indx2 = 0; indx2 < 4; indx2++)
		    {
		        if(pstRDSData->RT_Data.TextData[2][4*indx+indx2] == 0x0D)
	            {
	  			    pstRDSData->RT_Data.TextLength = 4*indx+indx2+1;
					pstRDSData->RT_Data.TextData[2][4*indx+indx2] = '\0';
	            }
		    }
	    }
	    else
	    {
	        for(indx2 = 0; indx2 < 2; indx2++)
		    {
		        if(pstRDSData->RT_Data.TextData[2][2*indx+indx2] == 0x0D)
	            {
	  			    pstRDSData->RT_Data.TextLength = 2*indx+indx2+1;
					pstRDSData->RT_Data.TextData[2][2*indx+indx2] = '\0';
	            }
		    }
	        
	    }
		addrcnt = 0xFFFF>>(0x0F-indx);
	}
	else
	{
		pstRDSData->RT_Data.TextLength = 0x0F;
		addrcnt = 0xFFFF;
	}
	
    WCN_DBG(L7|D_G2, "RetrieveGroup2 RDS RT: Addr_Cnt: 0x%x Length: 0x%x addrcnt: 0x%x\n", pstRDSData->RT_Data.Addr_Cnt, pstRDSData->RT_Data.TextLength, addrcnt);

	if(((((pstRDSData->RT_Data.Addr_Cnt)&addrcnt) == addrcnt)||((TextAddr == 0x0f) && (pstRDSData->RT_Data.Addr_Cnt == 0xffff))))//&&(pstRDSData->RT_Data.isRTDisplay == 0))
	{		
		pstRDSData->RT_Data.Addr_Cnt = 0;
		//pstRDSData->RT_Data.isRTDisplay = 1;

		pstRDSData->event_status |= RDS_EVENT_LAST_RADIOTEXT; 
		space = 0;
		for(indx = 0; indx < 64; indx++)
		{
			if(pstRDSData->RT_Data.TextData[2][indx] == 0x20)
				space++;
	    }	
	    if(space == 64)
            pstRDSData->event_status &= (~RDS_EVENT_LAST_RADIOTEXT);

		memset(&(pstRDSData->RT_Data.TextData[1][0]), 0x20, sizeof(pstRDSData->RT_Data.TextData[1]));
		memset(&(pstRDSData->RT_Data.TextData[0][0]), 0x20, sizeof(pstRDSData->RT_Data.TextData[0]));

		if (pstRDSData->event_status & RDS_EVENT_LAST_RADIOTEXT) {
			memcpy(&(pstRDSData->RT_Data.TextData[3][0]), &(pstRDSData->RT_Data.TextData[2][0]), sizeof(pstRDSData->RT_Data.TextData[3]));

			uint8_t tmp[66] = {0};
			tmp[64] = 'E';
			memcpy(tmp, pstRDSData->RT_Data.TextData[3], 64);
		
			WCN_DBG(L6|D_G2, "RT Radio text---%s\n", tmp);
			WCN_DBG(L6|D_G2, "RT set RDS_EVENT_LAST_RADIOTEXT flag, +++0x0D case.\n");
			pstRDSData->RT_Data.isRTDisplay = 1;
		}
    }
}

static void MT6616_RDS_RetrieveGroup4(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData)
{
    uint16_t year, month, k=0, D2, minute;
    uint32_t MJD, D1;
	WCN_DBG(L7|D_G4, "RetrieveGroup4 %d\n", SubType);
    if(!SubType) //Type A
    {
        if((block_data[4]&FM_RDS_GDBK_IND_C)&&(block_data[4]&FM_RDS_GDBK_IND_D))
        {            
            MJD = (uint32_t) (((block_data[1]&0x0003)<<15) + ((block_data[2]&0xFFFE)>>1));
            year = (MJD*100 - 1507820)/36525;
			month = (MJD*10000-149561000-3652500*year)/306001;			
		    if((month == 14)||(month == 15))
	            k = 1;	        
	        D1 = (uint32_t)((36525*year)/100);
	        D2 = (uint16_t)((306001*month)/10000);
	        pstRDSData->CT.Year = 1900 + year + k;
	        pstRDSData->CT.Month = month - 1 - k*12;
	        pstRDSData->CT.Day = (uint16_t)(MJD - 14956 - D1 - D2);
	        pstRDSData->CT.Hour = ((block_data[2]&0x0001)<<4)+((block_data[3]&0xF000)>>12);
	        minute = (block_data[3]&0x0FC0)>>6;

	        if(block_data[3]&0x0020)
	        {
  	            pstRDSData->CT.Local_Time_offset_signbit = 1; //0=+, 1=-
	        }
	        pstRDSData->CT.Local_Time_offset_half_hour = block_data[3]&0x001F;
	        if(pstRDSData->CT.Minute != minute)
	        {
	            pstRDSData->CT.Minute = (block_data[3]&0x0FC0)>>6;
	            pstRDSData->event_status |= RDS_EVENT_UTCDATETIME;
            }
        }
    }
}

static void MT6616_RDS_RetrieveGroup14(struct i2c_client *client, uint16_t *block_data, uint8_t SubType, RDSData_Struct *pstRDSData)
{
    uint8_t TP_ON, TA_ON, PI_ON, PS_Num, AF_H, AF_L, indx, indx2, num;

    WCN_DBG(L7|D_G14, "TA RetrieveGroup14 %d\n", SubType);
	WCN_DBG(L7|D_G14, "TA %04x %04x %04x %04x %04x\n", block_data[0], block_data[1], block_data[2], block_data[3], block_data[4]);
	//SubType = (*(block_data+1)&0x0800)>>11;
    PI_ON = block_data[3];
	TP_ON = block_data[1]&0x0010;						   	
	if((!SubType) && (block_data[4]&FM_RDS_GDBK_IND_C)) //Type A
	{
		PS_Num= block_data[1]&0x000F;
		if(PS_Num <4)
		{
			for(indx = 0; indx < 2; indx++)
			{
				pstRDSData->PS_ON[2*PS_Num] = block_data[2]>>8;
				pstRDSData->PS_ON[2*PS_Num+1] = block_data[2]&0xFF;
			}						
		}
		else if(PS_Num == 4)
		{
			AF_H = (block_data[2]&0xFF00)>>8;
			AF_L = block_data[2]&0x00FF;
			if((AF_H > 223)&&(AF_H < 250)) //Followed AF Number
			{
			    pstRDSData->AFON_Data.isAFNum_Get = 0;
				preAFON_Num = AF_H - 224;
				if(pstRDSData->AFON_Data.AF_Num != preAFON_Num)
				{
					pstRDSData->AFON_Data.AF_Num = preAFON_Num;
				}
				else
					pstRDSData->AFON_Data.isAFNum_Get= 1;
					
				if(AF_L < 205)
				{
					pstRDSData->AFON_Data.AF[0][0] = AF_L+875;
					if((pstRDSData->AFON_Data.AF[0][0]) != (pstRDSData->AFON_Data.AF[1][0]))
					{
						pstRDSData->AFON_Data.AF[1][0] = pstRDSData->AFON_Data.AF[0][0];
					}
					else
					{
						pstRDSData->AFON_Data.isMethod_A = 1;
					}
				}
			}
			else if((pstRDSData->AFON_Data.isAFNum_Get)&&((pstRDSData->AFON_Data.Addr_Cnt) != 0xFF)) //AF Num correct
			{
				num = pstRDSData->AFON_Data.AF_Num;
				num = num>>1;
				//Put AF freq into buffer and check if AF freq is repeat again
				for(indx = 1; indx < (num+1); indx++)
		       	{
		       		if((AF_H == (pstRDSData->AFON_Data.AF[0][2*indx-1]))&&(AF_L == (pstRDSData->AFON_Data.AF[0][2*indx])))    
					{
					    WCN_DBG(L7|D_G14, "RetrieveGroup14 AFON same as indx:%d\n", indx);
						break;
					}
					else if(!(pstRDSData->AFON_Data.AF[0][2*indx-1])) //null buffer
					{
						pstRDSData->AFON_Data.AF[0][2*indx-1] = AF_H+875;
						pstRDSData->AFON_Data.AF[0][2*indx] = AF_L+875;
						break;
					}
		       	}
				
				num = pstRDSData->AFON_Data.AF_Num;
				if(num > 0)
				{
					if((pstRDSData->AFON_Data.AF[0][num-1]) != 0)
					{
						num = num>> 1;
						//arrange frequency from low to high:start
						for(indx = 1; indx < num; indx++)
						{
							for(indx2 = indx+1; indx2 < (num+1); indx2++)
							{
								AF_H = pstRDSData->AFON_Data.AF[0][2*indx-1];
								AF_L = pstRDSData->AFON_Data.AF[0][2*indx];
								if(AF_H > (pstRDSData->AFON_Data.AF[0][2*indx2-1]))
								{
									pstRDSData->AFON_Data.AF[0][2*indx-1] = pstRDSData->AFON_Data.AF[0][2*indx2-1];
									pstRDSData->AFON_Data.AF[0][2*indx] = pstRDSData->AFON_Data.AF[0][2*indx2];
									pstRDSData->AFON_Data.AF[0][2*indx2-1] = AF_H;
									pstRDSData->AFON_Data.AF[0][2*indx2] = AF_L;
								}
								else if(AF_H == (pstRDSData->AFON_Data.AF[0][2*indx2-1]))
								{
									if(AF_L > (pstRDSData->AFON_Data.AF[0][2*indx2]))
                                    {			
										pstRDSData->AFON_Data.AF[0][2*indx-1] = pstRDSData->AFON_Data.AF[0][2*indx2-1];
										pstRDSData->AFON_Data.AF[0][2*indx] = pstRDSData->AFON_Data.AF[0][2*indx2];
										pstRDSData->AFON_Data.AF[0][2*indx2-1] = AF_H;
										pstRDSData->AFON_Data.AF[0][2*indx2] = AF_L;
									}
			                    }
							}
						}
						//arrange frequency from low to high:end
						//compare AF buff0 and buff1 data:start
						num = pstRDSData->AFON_Data.AF_Num;
						indx2 = 0;
						for(indx = 0; indx < num; indx++)
						{
							if((pstRDSData->AFON_Data.AF[1][indx]) == (pstRDSData->AFON_Data.AF[0][indx]))
                            {
								if(pstRDSData->AFON_Data.AF[1][indx] != 0)
									indx2++;
							}
							else
								pstRDSData->AFON_Data.AF[1][indx] = pstRDSData->AFON_Data.AF[0][indx];
                        }
						//compare AF buff0 and buff1 data:end						
						if(indx2 == num)
						{
							pstRDSData->AFON_Data.Addr_Cnt = 0xFF;
                            pstRDSData->event_status |= RDS_EVENT_AFON_LIST;				
							for(indx = 0; indx < num; indx++)
							{
								if((pstRDSData->AFON_Data.AF[1][indx]) == 0)
								{
									pstRDSData->AFON_Data.Addr_Cnt = 0x0F;
									pstRDSData->event_status &= (~RDS_EVENT_AFON_LIST);
								}
							}
						}
						else
							pstRDSData->AFON_Data.Addr_Cnt = 0x0F;
					}
				}
			}
			int pnum;
			for (pnum=0;pnum<2;pnum++) {
				WCN_DBG(L7|D_G14, "TA [%d]  0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n", pnum, pstRDSData->AFON_Data.AF[pnum][0], pstRDSData->AFON_Data.AF[pnum][1], pstRDSData->AFON_Data.AF[pnum][2], pstRDSData->AFON_Data.AF[pnum][3], pstRDSData->AFON_Data.AF[pnum][4], pstRDSData->AFON_Data.AF[pnum][5], pstRDSData->AFON_Data.AF[pnum][6], pstRDSData->AFON_Data.AF[pnum][7], pstRDSData->AFON_Data.AF[pnum][8], pstRDSData->AFON_Data.AF[pnum][9], pstRDSData->AFON_Data.AF[pnum][10], pstRDSData->AFON_Data.AF[pnum][11], pstRDSData->AFON_Data.AF[pnum][12], pstRDSData->AFON_Data.AF[pnum][13], pstRDSData->AFON_Data.AF[pnum][14], pstRDSData->AFON_Data.AF[pnum][15], pstRDSData->AFON_Data.AF[pnum][16], pstRDSData->AFON_Data.AF[pnum][17], pstRDSData->AFON_Data.AF[pnum][18], pstRDSData->AFON_Data.AF[pnum][19], pstRDSData->AFON_Data.AF[pnum][20], pstRDSData->AFON_Data.AF[pnum][21], pstRDSData->AFON_Data.AF[pnum][22], pstRDSData->AFON_Data.AF[pnum][23], pstRDSData->AFON_Data.AF[pnum][24]);
			}
		}		
	}
	else//Type B
	{
	    TA_ON = block_data[1]&0x0008;
		WCN_DBG(L7|D_G14, "TA group14 typeB pstRDSData->RDSFlag.TP=%d pstRDSData->RDSFlag.TA=%d TP_ON=%d TA_ON=%d\n", pstRDSData->RDSFlag.TP, pstRDSData->RDSFlag.TA, TP_ON, TA_ON);
        if((!pstRDSData->RDSFlag.TP)&&(pstRDSData->RDSFlag.TA)&&TP_ON&&TA_ON)
        {
			int TA_num=0;
			for (num=0;num<25;num++) {
				if (pstRDSData->AFON_Data.AF[1][num] != 0) {
					TA_num++;
				} else {
					break;
				}
			}
			WCN_DBG(L6|D_G14, "TA set RDS_EVENT_TAON");
			if (TA_num == pstRDSData->AFON_Data.AF_Num) {
				pstRDSData->event_status |= RDS_EVENT_TAON;
			}
		}
	}
}

bool MT6616_RDS_OnOff(struct fm *fm, bool bFlag)
{
    struct i2c_client *client = fm->i2c_client;
    RDSData_Struct *pstRDSData = fm->pstRDSData;
    
    if(MT6616_RDS_support(client) == false)
    {
        WCN_DBG(L4|D_IOCTL, "MT6616_RDS_OnOff failed, RDS not support\n");
        return false;
    }
    
    if(bFlag)
    {
        MT6616_RDS_Init_Data(pstRDSData);
        MT6616_RDS_enable(client);
    }
    else {
        MT6616_RDS_disable(client);
    }
    
    return true;   
}
 
void MT6616_RDS_Eint_Handler(struct fm *fm)
{
    uint16_t block_data[5], fifo_offset;
	uint8_t GroupType, SubType = 0;
	struct i2c_client *client = fm->i2c_client;
	RDSData_Struct *pstRDSData = fm->pstRDSData;	
	
	//pstRDSData->EINT_Flag = 1;
	do
	{
		MT6616_RDS_GetData(client, &block_data[0], sizeof(block_data));

        //FM_DEBUG("RDS_Eint_Hanlder CRC: %04x\n", block_data[4]);
		//if((block_data[4]&FM_RDS_GDBK_IND_A)||(block_data[4]&FM_RDS_GDBK_IND_B)||(block_data[4]&FM_RDS_GDBK_IND_C)||(block_data[4]&FM_RDS_GDBK_IND_D))
		//WCN_DBG(L7|D_RDS, "RDS_Eint_Hanlder BLOCK: %04x %04x %04x %04x\n", block_data[0], block_data[1], block_data[2], block_data[3]);

        fifo_offset = (block_data[4]&FM_RDS_DCO_FIFO_OFST) >> 5; //FM_RDS_DATA_CRC_FFOST
		//FM_DEBUG("RDS fifo_offset:%d\n", fifo_offset);
		
		if(block_data[4]&FM_RDS_GDBK_IND_B) //Block1 CRC OK
		{
		    GroupType = (block_data[1]&0xF000)>>12;
		    SubType = (block_data[1]&0x0800)>>11;
		}
		else
		{
		    WCN_DBG(L7|D_RDS, "RDS_Eint_Hanlder Block1 CRC error!!!\n");
			continue;
	    }
	    
	    /*		
		if(pstRDSData->Group_Cnt[2*GroupType+SubType] < 0xFFFF)
		    pstRDSData->Group_Cnt[2*GroupType+SubType] += 1;
		    
		//Backup Block data
		pstRDSData->Block_Backup[GroupType*2+SubType][0] = block_data[0];
		pstRDSData->Block_Backup[GroupType*2+SubType][1] = block_data[1];
		pstRDSData->Block_Backup[GroupType*2+SubType][2] = block_data[2];
		pstRDSData->Block_Backup[GroupType*2+SubType][3] = block_data[3];
             */
		if(block_data[4]&FM_RDS_GDBK_IND_A) //Block0 CRC OK
		{		
			if(pstRDSData->PI != block_data[0])  //PI=program Identication
			{
				pstRDSData->PI = block_data[0];
				pstRDSData->event_status |= RDS_EVENT_PI_CODE;
			}			
		}
		
		if(pstRDSData->PTY != ((block_data[1]&0x03E0)>>5))  //PTY=Program Type Code
		{
			pstRDSData->PTY = (block_data[1]&0x03E0)>>5;
			pstRDSData->event_status |= RDS_EVENT_PTY_CODE;			
		}
		
		if((pstRDSData->RDSFlag.TP != ((block_data[1]&0x0400)>>10)) && (((block_data[1]&0xf000)>>12)  != 0x0e)) //Tranfic Program Identification
		{
			pstRDSData->RDSFlag.TP = (block_data[1]&0x0400)>>10;
			pstRDSData->event_status |= RDS_EVENT_FLAGS;			
			pstRDSData->RDSFlag.flag_status |= RDS_FLAG_IS_TP;						
		}
		
		switch(GroupType)
		{
		    case 0:
		   	    MT6616_RDS_RetrieveGroup0(client, &block_data[0], SubType, pstRDSData);
		        break;
            case 1:
		   	    MT6616_RDS_RetrieveGroup1(client, &block_data[0], SubType, pstRDSData);
		   	    break;
            case 2:
				MT6616_RDS_RetrieveGroup2(client, &block_data[0], SubType, pstRDSData);
	            break;
            case 4:
		   	    MT6616_RDS_RetrieveGroup4(client, &block_data[0], SubType, pstRDSData);		
			    break;		
		    case 14:
		   	    MT6616_RDS_RetrieveGroup14(client, &block_data[0], SubType, pstRDSData);										   	
		   	    break;	
            default:
	          	break;			 
		}
	}while(fifo_offset > 1);
}

#else
bool MT6616_RDS_OnOff(struct i2c_client *client, bool bFlag, RDSData_Struct *pstRDSData)
{
    return false; 
}

void MT6616_RDS_Eint_Handler(struct i2c_client *client, RDSData_Struct *pstRDSData)
{
    return;  
}

#endif
