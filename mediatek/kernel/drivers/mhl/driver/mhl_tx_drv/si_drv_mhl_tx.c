#ifndef __KERNEL__
#include <string.h>
#include "hal_local.h"
#include "si_common.h"
#else
#ifdef DEBUG
#define TRACE_INT_TIME
#endif
#ifdef TRACE_INT_TIME
#include <linux/jiffies.h>
#endif
#include <linux/string.h>
#endif
#include <linux/delay.h>
#include "si_cra.h"
#include "si_cra_cfg.h"
#include "si_mhl_defs.h"
#include "si_mhl_tx_api.h"
#include "si_mhl_tx_base_drv_api.h"  
#include "si_8338_regs.h"
#include "si_drv_mhl_tx.h"
#include "si_platform.h"
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <cust_eint.h>

#define SILICON_IMAGE_ADOPTER_ID 322
#define	POWER_STATE_D3				3
#define	POWER_STATE_D0_NO_MHL		2
#define	POWER_STATE_D0_MHL			0
#define	POWER_STATE_FIRST_INIT		0xFF
#define TX_HW_RESET_PERIOD		10	
#define TX_HW_RESET_DELAY			100
static uint8_t fwPowerState = POWER_STATE_FIRST_INIT;
static	bool_t		mscCmdInProgress;	
bool_t		mscAbortFlag = false;	
static	uint8_t	dsHpdStatus = 0;
#define WriteByteCBUS(offset,value)  SiiRegWrite(TX_PAGE_CBUS | (uint16_t)offset,value)
#define ReadByteCBUS(offset)         SiiRegRead( TX_PAGE_CBUS | (uint16_t)offset)
#define	SET_BIT(offset,bitnumber)		SiiRegModify(offset,(1<<bitnumber),(1<<bitnumber))
#define	CLR_BIT(offset,bitnumber)		SiiRegModify(offset,(1<<bitnumber),0x00)
#define	DISABLE_DISCOVERY				SiiRegModify(REG_DISC_CTRL1,BIT0,0)
#define	ENABLE_DISCOVERY				SiiRegModify(REG_DISC_CTRL1,BIT0,BIT0)
#define STROBE_POWER_ON					SiiRegModify(REG_DISC_CTRL1,BIT1,0)
#define INTR_1_DESIRED_MASK   (BIT7|BIT6)
#define	UNMASK_INTR_1_INTERRUPTS		SiiRegWrite(REG_INTR1_MASK, INTR_1_DESIRED_MASK)
#define	MASK_INTR_1_INTERRUPTS			SiiRegWrite(REG_INTR1_MASK, 0x00)
#define INTR_2_DESIRED_MASK   (BIT1)
#define	UNMASK_INTR_2_INTERRUPTS		SiiRegWrite(REG_INTR2_MASK, INTR_2_DESIRED_MASK)
#define	INTR_4_DESIRED_MASK				(BIT0 | BIT2 | BIT3 | BIT4 | BIT5 | BIT6) 
#define	UNMASK_INTR_4_INTERRUPTS		SiiRegWrite(REG_INTR4_MASK, INTR_4_DESIRED_MASK)
#define	MASK_INTR_4_INTERRUPTS			SiiRegWrite(REG_INTR4_MASK, 0x00)
#define	INTR_5_DESIRED_MASK				(BIT2 | BIT3 | BIT4) 
#define	UNMASK_INTR_5_INTERRUPTS		SiiRegWrite(REG_INTR5_MASK, INTR_5_DESIRED_MASK)
#define	MASK_INTR_5_INTERRUPTS			SiiRegWrite(REG_INTR5_MASK, 0x00)
#define	INTR_CBUS1_DESIRED_MASK			(BIT2 | BIT3 | BIT4 | BIT5 | BIT6)
#define	UNMASK_CBUS1_INTERRUPTS			SiiRegWrite(REG_CBUS_INTR_ENABLE, INTR_CBUS1_DESIRED_MASK)
#define	MASK_CBUS1_INTERRUPTS			SiiRegWrite(REG_CBUS_INTR_ENABLE, 0x00)
#define	INTR_CBUS2_DESIRED_MASK			(BIT2 | BIT3)
#define	UNMASK_CBUS2_INTERRUPTS			SiiRegWrite(REG_CBUS_MSC_INT2_ENABLE, INTR_CBUS2_DESIRED_MASK)
#define	MASK_CBUS2_INTERRUPTS			SiiRegWrite(REG_CBUS_MSC_INT2_ENABLE, 0x00)
#define I2C_INACCESSIBLE -1
#define I2C_ACCESSIBLE 1
#define SIZE_AVI_INFOFRAME				17
static void Int1Isr (void);
static	int	Int4Isr( void );
static void Int5Isr (void);
static	void	MhlCbusIsr( void );
static	void	CbusReset(void);
static	void	SwitchToD0( void );
	void	SwitchToD3( void );
static	void	WriteInitialRegisterValues ( void );
static	void	InitCBusRegs( void );
static	void	ForceUsbIdSwitchOpen ( void );
static	void	ReleaseUsbIdSwitchOpen ( void );
static	void	MhlTxDrvProcessConnection ( void );
static	void	MhlTxDrvProcessDisconnection ( void );
static bool_t tmdsPowRdy;
video_data_t video_data;
static AVMode_t AVmode = {VM_INVALID, AUD_INVALID};

static void AudioVideoIsr(bool_t force_update);
static uint8_t CalculateInfoFrameChecksum (uint8_t *infoFrameData);
static void SendAviInfoframe (void);
static void ProcessScdtStatusChange (void);
static void SetAudioMode(inAudioTypes_t audiomode);
static void SetACRNValue (void);
static void SendAudioInfoFrame (void);
void SiiMhlTxHwReset(uint16_t hwResetPeriod,uint16_t hwResetDelay);

#ifdef __KERNEL__
static SiiOsTimer_t		MscAbortTimer = NULL;
static void SiiMhlMscAbortTimerCB(void * pArg)
{
    SiiOsTimerDelete(MscAbortTimer);
    MscAbortTimer = NULL;
    mscAbortFlag = false;
    SiiMhlTriggerSoftInt();
}
uint8_t SiiCheckDevice(uint8_t dev)
{
#ifdef _RGB_BOARD
    if(HalI2cReadByte(0x60,0x70)==0xff)
        return false;
#endif
    if(dev == 0)
    {
        if(POWER_STATE_D3 != fwPowerState&&HalI2cReadByte(0x9a,0x21)==0xff)
            return false;
    }
    else if(dev ==1) 
    {
        if(POWER_STATE_D3 == fwPowerState)
            return false;
    }
    return true;
}
void SiiMhlTriggerSoftInt(void)
{
    SiiRegBitsSet(REG_INT_CTRL,BIT3,true);
	HalTimerWait(5);
    SiiRegBitsSet(REG_INT_CTRL,BIT3,false);
}
#endif 
static void Int1Isr(void)
{
    uint8_t regIntr1;
    regIntr1 = SiiRegRead(REG_INTR1);
    if (regIntr1)
    {
        SiiRegWrite(REG_INTR1,regIntr1);
        if (BIT6 & regIntr1)
        {
            uint8_t cbusStatus;
        	cbusStatus = SiiRegRead(REG_MSC_REQ_ABORT_REASON);
        	TX_DEBUG_PRINT(("Drv: dsHpdStatus =%02X\n", (int) dsHpdStatus));
        	if(BIT6 & (dsHpdStatus ^ cbusStatus))
        	{
                uint8_t status = cbusStatus & BIT6;
        		TX_DEBUG_PRINT(("Drv: Downstream HPD changed to: %02X\n", (int) cbusStatus));
        		SiiMhlTxNotifyDsHpdChange( status );
                if(status)
                {
                   AudioVideoIsr(true);
                }
        		dsHpdStatus = cbusStatus;
        	}
        }
        if(BIT7 & regIntr1)
        {
        	TX_DEBUG_PRINT(("MHL soft interrupt triggered \n"));
        }
    }
}
static	int	Int2Isr( void )
{
    if(SiiRegRead(REG_INTR2) & INTR_2_DESIRED_MASK)
    {
        SiiRegWrite(REG_INTR2, INTR_2_DESIRED_MASK);
        if(SiiRegRead(REG_SYS_STAT) & BIT1)
        {
            TX_DEBUG_PRINT(("PCLK is STABLE\n"));
            if (tmdsPowRdy)
            {
                SendAudioInfoFrame();
                SendAviInfoframe();
            }
        }
    }
    return 0;
}
bool_t SiiMhlTxChipInitialize ( void )
{
    unsigned int initState = 0;
    tmdsPowRdy = false;
    mscCmdInProgress = false;	
    dsHpdStatus = 0;
    fwPowerState = POWER_STATE_D0_NO_MHL;
    SI_OS_DISABLE_DEBUG_CHANNEL(SII_OSAL_DEBUG_SCHEDULER);
    //memset(&video_data, 0x00, sizeof(video_data));	
    video_data.inputColorSpace = COLOR_SPACE_RGB;
    video_data.outputColorSpace = COLOR_SPACE_RGB;
    video_data.outputVideoCode = 2;
    video_data.inputcolorimetryAspectRatio = 0x18;
    video_data.outputcolorimetryAspectRatio = 0x18;
    video_data.output_AR = 0;
    //SiiMhlTxHwReset(TX_HW_RESET_PERIOD,TX_HW_RESET_DELAY);  
    //SiiCraInitialize();

    initState = (SiiRegRead(TX_PAGE_L0 | 0x03) << 8) | SiiRegRead(TX_PAGE_L0 | 0x02);
	//TX_DEBUG_PRINT(("Drv: SiiMhlTxChipInitialize: %02X%02x\n",
//						SiiRegRead(TX_PAGE_L0 | 0x03),
//						SiiRegRead(TX_PAGE_L0 | 0x02)));
	
    TX_DEBUG_PRINT(("Drv: SiiMhlTxChipInitialize: 0x%04X\n", initState));
	WriteInitialRegisterValues();
#ifndef __KERNEL__
    SiiOsMhlTxInterruptEnable();
#endif
	SwitchToD3();
    if (0xFFFF == initState)//0x8356
    {
	    return false;
    }
	return true;
}

typedef enum{
	HDMI_STATE_NO_DEVICE,
	HDMI_STATE_ACTIVE,
	HDMI_STATE_DPI_ENABLE
}HDMI_STATE;
extern  void hdmi_state_callback(HDMI_STATE state);


void 	SiiMhlTxDeviceIsr( void )
{
    uint8_t intMStatus, i; 
#ifdef TRACE_INT_TIME
    unsigned long K1;
    unsigned long K2;
    printk("-------------------SiiMhlTxDeviceIsr start -----------------\n");
	K1 = get_jiffies_64();
#endif
	i=0;
    do
    {
        if( POWER_STATE_D0_MHL != fwPowerState )
        {
            if(I2C_INACCESSIBLE == Int4Isr())
            {
				TX_DEBUG_PRINT(("Drv: I2C_INACCESSIBLE in Int4Isr in not D0 mode\n"));
                return; 
            }
        }
        else if( POWER_STATE_D0_MHL == fwPowerState )
        {
            if(I2C_INACCESSIBLE == Int4Isr())
            {
           		TX_DEBUG_PRINT(("Drv: I2C_INACCESSIBLE in Int4Isr in D0 mode\n"));
    			return; 
            }
            MhlCbusIsr();
            Int5Isr();
            Int1Isr();
            Int2Isr();
        }
        if( POWER_STATE_D3 != fwPowerState )
        {
    		MhlTxProcessEvents();
        }
		intMStatus = SiiRegRead(REG_INTR_STATE);	
		if(0xFF == intMStatus)
		{
			intMStatus = 0;
		    TX_DEBUG_PRINT(("Drv: EXITING ISR DUE TO intMStatus - 0xFF loop = [%02X] intMStatus = [%02X] \n\n", (int) i, (int)intMStatus));
		}
		i++;
		intMStatus &= 0x01; 
        if(i>60)
        {
            TX_DEBUG_PRINT(("force exit SiiMhlTxDeviceIsr \n"));
            break;
        }
        else if(i> 50)
        {
            TX_DEBUG_PRINT(("something error in SiiMhlTxDeviceIsr \n"));
        }
	} while (intMStatus);
#ifdef TRACE_INT_TIME
    K2 = get_jiffies_64();
    printk("-------------------SiiMhlTxDeviceIsr last %d ms----------------\n",(int)(K2 - K1));
#endif
}
void SiiExtDeviceIsr(void)
{
    if( fwPowerState <= POWER_STATE_D0_NO_MHL )
    {
#ifdef TRACE_INT_TIME
    unsigned long K1;
    unsigned long K2;
	K1 = get_jiffies_64();
#endif
        AudioVideoIsr(false);
#ifdef TRACE_INT_TIME
    K2 = get_jiffies_64();
#endif
    }
    else
    {
        TX_DEBUG_PRINT(("in D3 mode , SiiExtDeviceIsr not handled\n"));
    }
}
void SiiMhlTxDrvTmdsControl (bool_t enable)
{
	if (enable)
	{
        tmdsPowRdy = true;
	    SiiRegModify(REG_AUDP_TXCTRL, BIT0, SET_BITS);
        if (1)//(SiiVideoInputIsValid())
        {
    		SiiRegModify(REG_TMDS_CCTRL, TMDS_OE, SET_BITS);
            SendAudioInfoFrame();
            SendAviInfoframe();
            TX_DEBUG_PRINT(("TMDS Output Enabled\n"));
        }
        else
        {
		    TX_DEBUG_PRINT(("TMDS Output not Enabled due to invalid input\n"));
        }
	}
	else
	{
		SiiRegModify(REG_TMDS_CCTRL, TMDS_OE, CLEAR_BITS);
		SiiRegModify(REG_AUDP_TXCTRL, BIT0, CLEAR_BITS);
        tmdsPowRdy = false;
	    TX_DEBUG_PRINT(("TMDS Ouput Disabled\n"));
	}
}
void	SiiMhlTxDrvNotifyEdidChange ( void )
{
    TX_DEBUG_PRINT(("SiiMhlTxDrvNotifyEdidChange\n"));
	SiiDrvMhlTxReadEdid();
}
bool_t SiiMhlTxDrvSendCbusCommand ( cbus_req_t *pReq  )
{
    bool_t  success = true;
    uint8_t i, startbit;
	if( (POWER_STATE_D0_MHL != fwPowerState ) || (mscCmdInProgress))
	{
	    TX_DEBUG_PRINT(("Error: fwPowerState: %02X, or CBUS(0x0A):%02X mscCmdInProgress = %d\n",
			(int) fwPowerState,
			(int) SiiRegRead(REG_CBUS_BUS_STATUS),
			(int) mscCmdInProgress));
   		return false;
	}
	mscCmdInProgress	= true;
    TX_DEBUG_PRINT(("Sending MSC command %02X, %02X, %02X\n", 
			(int)pReq->command, 
            (int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[0]:pReq->offsetData),
            (int)((MHL_MSC_MSG == pReq->command)?pReq->payload_u.msgData[1]:pReq->payload_u.msgData[0])));
	SiiRegWrite(REG_MSC_CMD_OR_OFFSET, pReq->offsetData); 	
	SiiRegWrite(REG_CBUS_PRI_WR_DATA_1ST, pReq->payload_u.msgData[0]);
    startbit = 0x00;
    switch ( pReq->command )
    {
		case MHL_SET_INT:	
			startbit = MSC_START_BIT_WRITE_REG;
			break;
        case MHL_WRITE_STAT:	
            startbit = MSC_START_BIT_WRITE_REG;
            break;
        case MHL_READ_DEVCAP:	
            startbit = MSC_START_BIT_READ_REG;
            break;
 		case MHL_GET_STATE:			
		case MHL_GET_VENDOR_ID:		
		case MHL_SET_HPD:			
		case MHL_CLR_HPD:			
		case MHL_GET_SC1_ERRORCODE:		
		case MHL_GET_DDC_ERRORCODE:		
		case MHL_GET_MSC_ERRORCODE:		
		case MHL_GET_SC3_ERRORCODE:		
			SiiRegWrite(REG_MSC_CMD_OR_OFFSET, pReq->command );
            startbit = MSC_START_BIT_MSC_CMD;
            break;
        case MHL_MSC_MSG:
			SiiRegWrite(REG_CBUS_PRI_WR_DATA_2ND, pReq->payload_u.msgData[1] );
			SiiRegWrite(REG_MSC_CMD_OR_OFFSET, pReq->command );
            startbit = MSC_START_BIT_VS_CMD;
            break;
        case MHL_WRITE_BURST:
            SiiRegWrite(REG_MSC_WRITE_BURST_LEN, pReq->length -1 );
            if (NULL == pReq->payload_u.pdatabytes)
            {
                TX_DEBUG_PRINT(("\nPut pointer to WRITE_BURST data in req.pdatabytes!!!\n\n"));
                success = false;
}
            else
            {
	            uint8_t *pData = pReq->payload_u.pdatabytes;
                TX_DEBUG_PRINT(("\nWriting data into scratchpad\n\n"));
	            for ( i = 0; i < pReq->length; i++ )
				{
					SiiRegWrite(REG_CBUS_SCRATCHPAD_0 + i, *pData++ );
				}
	            startbit = MSC_START_BIT_WRITE_BURST;
			}
            break;
        default:
            success = false;
            break;
    }
    if ( success )
    {
        SiiRegWrite(REG_MSC_COMMAND_START, startbit );
    }
    else
    {
        TX_DEBUG_PRINT(("\nSiiMhlTxDrvSendCbusCommand failed\n\n"));
    }
    return( success );
}
bool_t SiiMhlTxDrvCBusBusy(void)
{
    return mscCmdInProgress ? true :false;
}
static void WriteInitialRegisterValues ( void )
{
	TX_DEBUG_PRINT(("WriteInitialRegisterValues\n"));
	SiiRegWrite(REG_POWER_EN, 0x3F);			
	SiiRegWrite(REG_MHLTX_CTL6, 0xBC);              
	SiiRegWrite(REG_MHLTX_CTL2, 0x3C);              
	SiiRegWrite(REG_MHLTX_CTL4, 0xC8);              
	SiiRegWrite(REG_MHLTX_CTL7, 0x03);              
	SiiRegWrite(REG_MHLTX_CTL8, 0x0A);              
    SiiRegWrite(REG_TMDS_CCTRL, 0x08);              
	SiiRegWrite(REG_USB_CHARGE_PUMP_MHL, 0x03);     
	SiiRegWrite(REG_USB_CHARGE_PUMP, 0x8C);         
	SiiRegWrite(REG_SYS_CTRL1, 0x35);               
	SiiRegWrite(REG_DISC_CTRL5, 0x57);				
	SiiRegWrite(REG_DISC_CTRL9, 0x24);				
	SiiRegWrite(REG_DISC_CTRL1, 0x27);				
	SiiRegWrite(REG_DISC_CTRL3, 0x86);				
	CbusReset();
	InitCBusRegs();
    SiiRegModify(REG_LM_DDC, VID_MUTE, SET_BITS);       
    SiiRegModify(REG_AUDP_TXCTRL, BIT2, SET_BITS);      
}
static void InitCBusRegs( void )
{
	SiiRegWrite(REG_CBUS_CFG, 0xF2); 			
	SiiRegWrite(REG_CBUS_CTRL1, 0x01);          
	SiiRegWrite(REG_CBUS_CTRL2, 0x2D);          
	SiiRegWrite(REG_CBUS_CTRL7, 0x0a); 		    
	SiiRegWrite(REG_CBUS_DRV_STR0, 0x03); 		
    SiiRegWrite(REG_MSC_COMP_CTRL , 0x11);      
	SiiRegWrite(REG_MSC_TIMEOUT_LIMIT, 0x0F);           
#define DEVCAP_REG(x) (REG_CBUS_DEVICE_CAP_0 | DEVCAP_OFFSET_##x)
	SiiRegWrite(DEVCAP_REG(DEV_STATE      ) ,DEVCAP_VAL_DEV_STATE       );
	SiiRegWrite(DEVCAP_REG(MHL_VERSION    ) ,DEVCAP_VAL_MHL_VERSION     );
	SiiRegWrite(DEVCAP_REG(DEV_CAT        ) ,DEVCAP_VAL_DEV_CAT         );
	SiiRegWrite(DEVCAP_REG(ADOPTER_ID_H   ) ,DEVCAP_VAL_ADOPTER_ID_H    );
	SiiRegWrite(DEVCAP_REG(ADOPTER_ID_L   ) ,DEVCAP_VAL_ADOPTER_ID_L    );
	SiiRegWrite(DEVCAP_REG(VID_LINK_MODE  ) ,DEVCAP_VAL_VID_LINK_MODE   );
	SiiRegWrite(DEVCAP_REG(AUD_LINK_MODE  ) ,DEVCAP_VAL_AUD_LINK_MODE   );
	SiiRegWrite(DEVCAP_REG(VIDEO_TYPE     ) ,DEVCAP_VAL_VIDEO_TYPE      );
	SiiRegWrite(DEVCAP_REG(LOG_DEV_MAP    ) ,DEVCAP_VAL_LOG_DEV_MAP     );
	SiiRegWrite(DEVCAP_REG(BANDWIDTH      ) ,DEVCAP_VAL_BANDWIDTH       );
	SiiRegWrite(DEVCAP_REG(FEATURE_FLAG   ) ,DEVCAP_VAL_FEATURE_FLAG    );
	SiiRegWrite(DEVCAP_REG(DEVICE_ID_H    ) ,DEVCAP_VAL_DEVICE_ID_H     );
	SiiRegWrite(DEVCAP_REG(DEVICE_ID_L    ) ,DEVCAP_VAL_DEVICE_ID_L     );
	SiiRegWrite(DEVCAP_REG(SCRATCHPAD_SIZE) ,DEVCAP_VAL_SCRATCHPAD_SIZE );
	SiiRegWrite(DEVCAP_REG(INT_STAT_SIZE  ) ,DEVCAP_VAL_INT_STAT_SIZE   );
	SiiRegWrite(DEVCAP_REG(RESERVED       ) ,DEVCAP_VAL_RESERVED        );
}
static void ForceUsbIdSwitchOpen ( void )
{
	DISABLE_DISCOVERY;
	SiiRegModify(REG_DISC_CTRL6, BIT6, BIT6);				
	SiiRegWrite(REG_DISC_CTRL3, 0x86);
}
static void ReleaseUsbIdSwitchOpen ( void )
{
	HalTimerWait(50); 
	SiiRegModify(REG_DISC_CTRL6, BIT6, 0x00);
	ENABLE_DISCOVERY;
}
void SiiMhlTxDrvProcessRgndMhl( void )
{
	SiiRegModify(REG_DISC_CTRL9, BIT0, BIT0);
}
void ProcessRgnd (void)
{
	uint8_t rgndImpedance;
	rgndImpedance = SiiRegRead(REG_DISC_STAT2) & 0x03;
	TX_DEBUG_PRINT(("RGND = %02X : \n", (int)rgndImpedance));
	if (0x02 == rgndImpedance)
	{
		TX_DEBUG_PRINT(("(MHL Device)\n"));
		SiiMhlTxNotifyRgndMhl(); 
	}
	else
	{
		SiiRegModify(REG_DISC_CTRL9, BIT3, BIT3);	
		TX_DEBUG_PRINT(("(Non-MHL Device)\n"));
	}
}
void	SwitchToD0( void )
{
	TX_DEBUG_PRINT(("Switch to D0\n"));
	WriteInitialRegisterValues();
	STROBE_POWER_ON; 
	fwPowerState = POWER_STATE_D0_NO_MHL;
    AudioVideoIsr(true);
}


extern void CBusQueueReset(void);
void	SwitchToD3( void )
{
	if(POWER_STATE_D3 != fwPowerState)
	{
        TX_DEBUG_PRINT(("Switch To D3\n"));
#ifdef __KERNEL__
        //HalGpioSetPin(GPIO_M2U_VBUS_CTRL,1);
#else
        pinM2uVbusCtrlM = 1;
#endif
#if (SYSTEM_BOARD == SB_EPV5_MARK_II)
        pinMhlConn = 1;
        pinUsbConn = 0;
#elif (SYSTEM_BOARD == SB_STARTER_KIT_X01)
#ifdef __KERNEL__
        //HalGpioSetPin(GPIO_MHL_USB,1);
#else
        pinMhlUsb = 1;
#endif
#endif
		// change TMDS termination to high impedance
		//bits 1:0 set to 03
		SiiRegWrite(TX_PAGE_2|0x0001, 0x03);
		SiiRegWrite(REG_MHLTX_CTL1, 0xD0);

		// clear all interrupt here before go into D3 mode by oscar
		SiiRegWrite(REG_INTR1,0xFF);
		SiiRegWrite(REG_INTR2,0xFF);
		SiiRegWrite(REG_INTR4,0xFF); 
		SiiRegWrite(REG_INTR5,0xFF);
		SiiRegWrite(REG_CBUS_INTR_STATUS,0xFF); 
		SiiRegWrite(REG_CBUS_MSC_INT2_STATUS,0xFF); 


#ifndef __KERNEL__
		//if(HalGpioGetPin(pinAllowD3))
		{
#endif
		ForceUsbIdSwitchOpen();
		//HalTimerWait(50);
		ReleaseUsbIdSwitchOpen();

		//HalTimerWait(50);
		CLR_BIT(REG_POWER_EN, 0);
		CBusQueueReset();
		fwPowerState = POWER_STATE_D3;
#ifndef __KERNEL__
		}/*else
		{
            //fwPowerState = POWER_STATE_D0_NO_MHL;
		}
		*/
#endif
	}
}

void	ForceSwitchToD3( void )
{
		//HalTimerWait(50);
		CLR_BIT(REG_POWER_EN, 0);
		CBusQueueReset();
		fwPowerState = POWER_STATE_D3;
}

static	int	Int4Isr( void )
{
	uint8_t int4Status;
	int4Status = SiiRegRead(REG_INTR4);	
    if(!int4Status)
    {
    }
	else if(0xFF == int4Status)
	{
        return I2C_INACCESSIBLE;
	}else
	{
        TX_DEBUG_PRINT(("INT4 Status = %02X\n", (int) int4Status));
    	if(int4Status & BIT0) 
    	{
    		ProcessScdtStatusChange();
    	}
    	if(int4Status & BIT2) 
    	{
    		MhlTxDrvProcessConnection();
    	}
    	else if(int4Status & BIT3)
    	{
    		TX_DEBUG_PRINT(("uUSB-A type device detected.\n"));
    		SiiRegWrite(REG_DISC_STAT2, 0x80);	
    		SwitchToD3();
			return I2C_INACCESSIBLE;
        }
    	if (int4Status & BIT5)
    	{
    		MhlTxDrvProcessDisconnection();
            return I2C_INACCESSIBLE;
    	}
        if((POWER_STATE_D0_MHL != fwPowerState) && (int4Status & BIT6))
    	{
    		SwitchToD0();
    		ProcessRgnd();
    	}
    	if(fwPowerState != POWER_STATE_D3)
        {
            if (int4Status & BIT4)
            {
                TX_DEBUG_PRINT(("CBus Lockout\n"));
                ForceUsbIdSwitchOpen();
                ReleaseUsbIdSwitchOpen();
            }
        }
    }
	SiiRegWrite(REG_INTR4, int4Status);	
	return I2C_ACCESSIBLE;
}
static void Int5Isr (void)
{
	uint8_t int5Status;
	int5Status = SiiRegRead(REG_INTR5);	
	if (int5Status)
	{
#if (SYSTEM_BOARD == SB_STARTER_KIT_X01)
    	if((int5Status & BIT3) || (int5Status & BIT2))
    	{
    		TX_DEBUG_PRINT (("** Apply MHL FIFO Reset\n"));
    		SiiRegModify(REG_SRST, BIT4, SET_BITS);
    		SiiRegModify(REG_SRST, BIT4, CLEAR_BITS);
    	}
#endif
        if (int5Status & BIT4)
        {
    		TX_DEBUG_PRINT (("** PXL Format changed\n"));
#ifndef __KERNEL__
            SiiOsBumpMhlTxEvent();
#else
            //SiiTriggerExtInt();
#endif
        }
    	SiiRegWrite(REG_INTR5, int5Status);	
	}
}
static void MhlTxDrvProcessConnection ( void )
{
	TX_DEBUG_PRINT (("MHL Cable Connected. CBUS:0x0A = %02X\n", (int) SiiRegRead(REG_CBUS_BUS_STATUS)));
	if( POWER_STATE_D0_MHL == fwPowerState )
	{
		return;
	}
#ifdef __KERNEL__
    //HalGpioSetPin(GPIO_M2U_VBUS_CTRL,0);
#else
    pinM2uVbusCtrlM = 0;
#endif
#if (SYSTEM_BOARD == SB_EPV5_MARK_II)
    pinMhlConn = 0;
    pinUsbConn = 1;
#elif (SYSTEM_BOARD == SB_STARTER_KIT_X01)
#ifdef __KERNEL__
    //HalGpioSetPin(GPIO_MHL_USB,0);
#else
    pinMhlUsb = 0;
#endif
#endif
	SiiRegWrite(REG_MHLTX_CTL1, 0x10);
	fwPowerState = POWER_STATE_D0_MHL;
	
// change TMDS termination to 50 ohm termination(default)
//bits 1:0 set to 00
	SiiRegWrite(TX_PAGE_2|0x0001, 0x00);
	
	ENABLE_DISCOVERY;
	SiiMhlTxNotifyConnection(true);
}
static void MhlTxDrvProcessDisconnection ( void )
{
	TX_DEBUG_PRINT(("MhlTxDrvProcessDisconnection\n"));
	SiiRegWrite(REG_INTR4, SiiRegRead(REG_INTR4));
	dsHpdStatus &= ~BIT6;  
	SiiRegWrite(REG_MSC_REQ_ABORT_REASON, dsHpdStatus);
	SiiMhlTxNotifyDsHpdChange(0);
	if( POWER_STATE_D0_MHL == fwPowerState )
	{
		SiiMhlTxNotifyConnection(false);
	}
	SwitchToD3();
}
void CbusReset (void)
{
    uint8_t idx;
	TX_DEBUG_PRINT( ("CBUS reset!!!\n"));
	SET_BIT(REG_SRST, 3);
	HalTimerWait(2);
	CLR_BIT(REG_SRST, 3);
	mscCmdInProgress = false;
    UNMASK_INTR_4_INTERRUPTS;
#if (SYSTEM_BOARD == SB_STARTER_KIT_X01)
    UNMASK_INTR_1_INTERRUPTS;
    UNMASK_INTR_2_INTERRUPTS;
    UNMASK_INTR_5_INTERRUPTS;
#else
    MASK_INTR_1_INTERRUPTS;
    MASK_INTR_5_INTERRUPTS;
#endif
    UNMASK_CBUS1_INTERRUPTS;
    UNMASK_CBUS2_INTERRUPTS;
	for(idx=0; idx < 4; idx++)
	{
		WriteByteCBUS((0xE0 + idx), 0xFF);
		WriteByteCBUS((0xF0 + idx), 0xFF);
	}
}
static uint8_t CBusProcessErrors( uint8_t intStatus )
{
    uint8_t result          = 0;
    uint8_t abortReason  = 0;
    intStatus &= (BIT_MSC_ABORT | BIT_MSC_XFR_ABORT | BIT_DDC_ABORT);
    if ( intStatus )
    {
		if( intStatus & BIT_DDC_ABORT )
		{
			abortReason |= SiiRegRead(REG_DDC_ABORT_REASON);
			TX_DEBUG_PRINT( ("CBUS:: DDC ABORT happened. Clearing 0x0C\n"));
            SiiRegWrite(REG_DDC_ABORT_REASON, 0xFF);
		}
        if ( intStatus & BIT_MSC_XFR_ABORT )
        {
            abortReason |= SiiRegRead(REG_MSC_REQ_ABORT_REASON );
            TX_DEBUG_PRINT( ("CBUS:: MSC Requester ABORTED. Clearing 0x0D\n"));
            SiiRegWrite(REG_MSC_REQ_ABORT_REASON , 0xFF);
        }
        if ( intStatus & BIT_MSC_ABORT )
        {
            abortReason |= SiiRegRead(REG_MSC_RES_ABORT_REASON );
            TX_DEBUG_PRINT( ("CBUS:: MSC Responder ABORT. Clearing 0x0E\n"));
            SiiRegWrite(REG_MSC_RES_ABORT_REASON , 0xFF);
        }
        if ( abortReason & (BIT0|BIT1|BIT2|BIT3|BIT4|BIT7) )
        {
            TX_DEBUG_PRINT( ("CBUS:: Reason for ABORT is ....0x%02X \n", (int)abortReason ));
            if ( abortReason & CBUSABORT_BIT_REQ_MAXFAIL)
            {
                TX_DEBUG_PRINT( ("Retry threshold exceeded\n"));
            }
            if ( abortReason & CBUSABORT_BIT_PROTOCOL_ERROR)
            {
                TX_DEBUG_PRINT( ("Protocol Error\n"));
            }
            if ( abortReason & CBUSABORT_BIT_REQ_TIMEOUT)
            {
                TX_DEBUG_PRINT( ("Translation layer timeout\n"));
            }
            if ( abortReason & CBUSABORT_BIT_UNDEFINED_OPCODE)
            {
                TX_DEBUG_PRINT( ("Undefined opcode\n"));
            }
            if ( abortReason & CBUSABORT_BIT_UNDEFINED_OFFSET)
            {
                TX_DEBUG_PRINT( ("Undefined offset\n"));
            }
            if ( abortReason & CBUSABORT_BIT_PEER_BUSY)
            {
                TX_DEBUG_PRINT( ("Opposite device is busy\n"));
            }
            if ( abortReason & CBUSABORT_BIT_PEER_ABORTED)
            {
#ifndef __KERNEL__
                HalTimerSet(TIMER_ABORT, T_ABORT_NEXT);
                mscAbortFlag = true;
#else
                if(MscAbortTimer)
                {
                    SiiOsTimerDelete(MscAbortTimer);
                    MscAbortTimer = NULL;
                }
                mscAbortFlag = true;
                SiiOsTimerCreate("Abort Time Out", SiiMhlMscAbortTimerCB, NULL, true,
                2000, false, &MscAbortTimer);
#endif
                TX_DEBUG_PRINT( ("Peer sent an abort, start 2s timer Tabort_next\n"));
            }
        }
    }
    return( result );
}
void SiiMhlTxDrvGetScratchPad(uint8_t startReg,uint8_t *pData,uint8_t length)
{
int i;
    for (i = 0; i < length;++i,++startReg)
    {
        *pData++ = SiiRegRead(REG_CBUS_SCRATCHPAD_0 + startReg);
    }
}
static void MhlCbusIsr( void )
{
	uint8_t		cbusInt;
	uint8_t     gotData[4];	
	uint8_t		i;
	cbusInt = SiiRegRead(REG_CBUS_MSC_INT2_STATUS);
	if(cbusInt == 0xFF)
	{
		return;
	}
	if( cbusInt )
	{
        if ( BIT0 & cbusInt)
        {
            SiiMhlTxMscWriteBurstDone( cbusInt );
        }
    	if(cbusInt & BIT2)
    	{
            uint8_t intr[4]={0};
    	    TX_DEBUG_PRINT(("MHL INTR Received\n"));
            SiiRegReadBlock(REG_CBUS_SET_INT_0, intr, 4);
    		SiiMhlTxGotMhlIntr( intr[0], intr[1] );
            SiiRegWriteBlock(REG_CBUS_SET_INT_0, intr, 4);
    	}
    	if(cbusInt & BIT3)
    	{
            uint8_t status[4]={0};
    		TX_DEBUG_PRINT(("MHL STATUS Received\n"));
            for (i = 0; i < 4;++i)
            {
                status[i] = SiiRegRead(REG_CBUS_WRITE_STAT_0 + i);
    			SiiRegWrite((REG_CBUS_WRITE_STAT_0 + i), 0xFF  );
            }
    		SiiMhlTxGotMhlStatus( status[0], status[1] );
    	}
    	SiiRegWrite(REG_CBUS_MSC_INT2_STATUS, cbusInt);
        TX_DEBUG_PRINT(("Drv: Clear CBUS INTR_2: %02X\n", (int) cbusInt));
	}
	cbusInt = SiiRegRead(REG_CBUS_INTR_STATUS);
	if (cbusInt)
	{
		SiiRegWrite(REG_CBUS_INTR_STATUS, cbusInt);
	    TX_DEBUG_PRINT(("Drv: Clear CBUS INTR_1: %02X\n", (int) cbusInt));
	}
	if((cbusInt & BIT3))
	{
        uint8_t mscMsg[2];
	    TX_DEBUG_PRINT(("MSC_MSG Received\n"));
        mscMsg[0] = SiiRegRead(REG_CBUS_PRI_VS_CMD);
        mscMsg[1] = SiiRegRead(REG_CBUS_PRI_VS_DATA);
	    TX_DEBUG_PRINT(("MSC MSG: %02X %02X\n", (int)mscMsg[0], (int)mscMsg[1] ));
		SiiMhlTxGotMhlMscMsg( mscMsg[0], mscMsg[1] );
	}
	if (cbusInt & (BIT_MSC_ABORT | BIT_MSC_XFR_ABORT | BIT_DDC_ABORT))
	{
		gotData[0] = CBusProcessErrors(cbusInt);
		mscCmdInProgress = false;
	}
	if(cbusInt & BIT4)
	{
	    TX_DEBUG_PRINT(("MSC_REQ_DONE\n"));
		mscCmdInProgress = false;
		SiiMhlTxMscCommandDone( SiiRegRead(REG_CBUS_PRI_RD_DATA_1ST) );
	}
    if (cbusInt & BIT7)
    {
        TX_DEBUG_PRINT(("Parity error count reaches 15\n"));
        SiiRegWrite(REG_CBUS_STAT2, 0x00);
    }
}
static void ProcessScdtStatusChange (void)
{
}
void SiiMhlTxDrvPowBitChange (bool_t enable)
{
	if (enable)
	{
		SiiRegModify(REG_DISC_CTRL8, BIT2, SET_BITS);
	    TX_DEBUG_PRINT(("POW bit 0->1, set DISC_CTRL8[2] = 1\n"));
	}
}
void SiMhlTxDrvSetClkMode(uint8_t clkMode)
{
	TX_DEBUG_PRINT(("SiMhlTxDrvSetClkMode:0x%02x\n",(int)clkMode));
}
static void SendAviInfoframe (void)
{
    uint8_t ifData[SIZE_AVI_INFOFRAME];
	extern uint8_t VIDEO_CAPABILITY_D_BLOCK_found;
    ifData[0] = 0x82;       
    ifData[1] = 0x02;       
    ifData[2] = 0x0D; 
	ifData[3] = 0x00;  
  	ifData[4] = video_data.outputColorSpace << 5; 
	ifData[5] = video_data.outputcolorimetryAspectRatio; 

	if(VIDEO_CAPABILITY_D_BLOCK_found){
		ifData[6] = 0x04; 
		TX_DEBUG_PRINT(("VIDEO_CAPABILITY_D_BLOCK_found = true, limited range\n"));
		}
	else{
    	ifData[6] = 0x00;  
		TX_DEBUG_PRINT(("VIDEO_CAPABILITY_D_BLOCK_found= false. defult range\n"));
		}
    //ifData[4] = video_data.outputColorSpace << 5; 
    ifData[7] = video_data.inputVideoCode; 
	TX_DEBUG_PRINT(("video_data.inputVideoCode:0x%02x, video_data.outputVideoCode=0x%x\n",(int)video_data.inputVideoCode,video_data.outputVideoCode));

    //ifData[7] = video_data.outputVideoCode;
    ifData[8] = 0x00;
    ifData[9] = 0x00;
    ifData[10] = 0x00; 
    ifData[11] = 0x00;
    ifData[12] = 0x00; 
    ifData[13] = 0x00;
    ifData[14] = 0x00;
    ifData[15] = 0x00;
    ifData[16] = 0x00;
    ifData[3] = CalculateInfoFrameChecksum(ifData);
    SiiRegModify(TX_PAGE_L1 | 0x3E, BIT0|BIT1, CLEAR_BITS);
    SiiRegWriteBlock(TX_PAGE_L1 | 0x0040, ifData, SIZE_AVI_INFOFRAME);
    SiiRegModify(TX_PAGE_L1 | 0x3E, BIT0|BIT1, SET_BITS);
}
static uint8_t CalculateInfoFrameChecksum (uint8_t *infoFrameData)
{
	uint8_t i, checksum;
	checksum = 0x00;
    for (i = 0; i < SIZE_AVI_INFOFRAME; i++)
    {
        checksum += infoFrameData[i];
    }
    checksum = 0x100 - checksum;
	return checksum;
}
PLACE_IN_CODE_SEG const audioConfig_t audioData[AUD_TYP_NUM] =
{
    {0x11, 0x40, 0x0E, 0x03, 0x00, 0x05},
    {0x11, 0x40, 0x0A, 0x01, 0x00, 0x05},
    {0x11, 0x40, 0x02, 0x00, 0x00, 0x05},
    {0x11, 0x40, 0x0C, 0x03, 0x00, 0x05},
    {0x11, 0x40, 0x08, 0x01, 0x00, 0x05},
    {0x11, 0x40, 0x00, 0x00, 0x00, 0x05},
    {0x11, 0x40, 0x03, 0x00, 0x00, 0x05},
    {0x11, 0x40, 0x0E, 0x03, 0x03, 0x05},
    {0x11, 0x40, 0x0A, 0x01, 0x03, 0x05},
    {0x11, 0x40, 0x02, 0x00, 0x03, 0x05},
    {0x11, 0x40, 0x0C, 0x03, 0x03, 0x05},
    {0x11, 0x40, 0x08, 0x01, 0x03, 0x05},
    {0x11, 0x40, 0x00, 0x00, 0x03, 0x05},
    {0x11, 0x40, 0x03, 0x00, 0x03, 0x05},
    {0xF1, 0x40, 0x0E, 0x00, 0x03, 0x07},
    {0x03, 0x00, 0x00, 0x00, 0x00, 0x05}
};
static void SetAudioMode(inAudioTypes_t audiomode)
{
    if (audiomode >= AUD_TYP_NUM)
        audiomode = I2S_48;
    SiiRegWrite(REG_AUDP_TXCTRL, audioData[audiomode].regAUD_path);
    SiiRegWrite(TX_PAGE_L1 | 0x14, audioData[audiomode].regAUD_mode);
    SiiRegWrite(TX_PAGE_L1 | 0x1D, audioData[audiomode].regAUD_ctrl);
    SiiRegWrite(TX_PAGE_L1 | 0x21, audioData[audiomode].regAUD_freq);
    SiiRegWrite(TX_PAGE_L1 | 0x23, audioData[audiomode].regAUD_src);
    SiiRegWrite(TX_PAGE_L1 | 0x28, audioData[audiomode].regAUD_tdm_ctrl);
//  SiiRegWrite(TX_PAGE_L1 | 0x22, 0x0B); 
//0x02 for word length=16bits
		    SiiRegWrite(TX_PAGE_L1 | 0x22, 0x02); 
		SiiRegWrite(TX_PAGE_L1 | 0x24,0x02); 

    SiiRegWrite(TX_PAGE_L1 | 0x15, 0x00); 
	//0x7A:0x24 = 0x0B for word lenth is defult 24bit
	
	TX_DEBUG_PRINT(("SiiRegRead(TX_PAGE_L1 | 0x21)=0x%x\n",SiiRegRead(TX_PAGE_L1 | 0x21)));
	TX_DEBUG_PRINT(("SiiRegRead(TX_PAGE_L1 | 0x1D)=0x%x\n",SiiRegRead(TX_PAGE_L1 | 0x1D)));
}
static void SetACRNValue (void)
{
    uint8_t audioFs;
    if ((SiiRegRead(TX_PAGE_L1 | 0x14) & BIT1) && !(SiiRegRead(TX_PAGE_L1 | 0x15) & BIT1))
        audioFs = SiiRegRead(TX_PAGE_L1 | 0x18) & 0x0F;
    else
        audioFs = SiiRegRead(TX_PAGE_L1 | 0x21) & 0x0F;
    switch (audioFs)
    {
        case 0x03:
            SiiRegWrite(TX_PAGE_L1 | 0x05, (uint8_t)(ACR_N_value_32k >> 16));
            SiiRegWrite(TX_PAGE_L1 | 0x04, (uint8_t)(ACR_N_value_32k >> 8));
            SiiRegWrite(TX_PAGE_L1 | 0x03, (uint8_t)(ACR_N_value_32k));
            break;
        case 0x00:
            SiiRegWrite(TX_PAGE_L1 | 0x05, (uint8_t)(ACR_N_value_44k >> 16));
            SiiRegWrite(TX_PAGE_L1 | 0x04, (uint8_t)(ACR_N_value_44k >> 8));
            SiiRegWrite(TX_PAGE_L1 | 0x03, (uint8_t)(ACR_N_value_44k));
            break;
        case 0x02:
            SiiRegWrite(TX_PAGE_L1 | 0x05, (uint8_t)(ACR_N_value_48k >> 16));
            SiiRegWrite(TX_PAGE_L1 | 0x04, (uint8_t)(ACR_N_value_48k >> 8));
            SiiRegWrite(TX_PAGE_L1 | 0x03, (uint8_t)(ACR_N_value_48k));
            break;
        case 0x08:
            SiiRegWrite(TX_PAGE_L1 | 0x05, (uint8_t)(ACR_N_value_88k >> 16));
            SiiRegWrite(TX_PAGE_L1 | 0x04, (uint8_t)(ACR_N_value_88k >> 8));
            SiiRegWrite(TX_PAGE_L1 | 0x03, (uint8_t)(ACR_N_value_88k));
            break;
        case 0x0A:
            SiiRegWrite(TX_PAGE_L1 | 0x05, (uint8_t)(ACR_N_value_96k >> 16));
            SiiRegWrite(TX_PAGE_L1 | 0x04, (uint8_t)(ACR_N_value_96k >> 8));
            SiiRegWrite(TX_PAGE_L1 | 0x03, (uint8_t)(ACR_N_value_96k));
            break;
        case 0x0C:
            SiiRegWrite(TX_PAGE_L1 | 0x05, (uint8_t)(ACR_N_value_176k >> 16));
            SiiRegWrite(TX_PAGE_L1 | 0x04, (uint8_t)(ACR_N_value_176k >> 8));
            SiiRegWrite(TX_PAGE_L1 | 0x03, (uint8_t)(ACR_N_value_176k));
            break;
        case 0x0E:
            SiiRegWrite(TX_PAGE_L1 | 0x05, (uint8_t)(ACR_N_value_192k >> 16));
            SiiRegWrite(TX_PAGE_L1 | 0x04, (uint8_t)(ACR_N_value_192k >> 8));
            SiiRegWrite(TX_PAGE_L1 | 0x03, (uint8_t)(ACR_N_value_192k));
            break;
        default:
            SiiRegWrite(TX_PAGE_L1 | 0x05, (uint8_t)(ACR_N_value_default >> 16));
            SiiRegWrite(TX_PAGE_L1 | 0x04, (uint8_t)(ACR_N_value_default >> 8));
            SiiRegWrite(TX_PAGE_L1 | 0x03, (uint8_t)(ACR_N_value_default));
            break;
    }
    SiiRegModify(REG_AUDP_TXCTRL, BIT2, CLEAR_BITS); 
}
static void SendAudioInfoFrame (void)
{
    SiiRegModify(TX_PAGE_L1 | 0x3E, BIT4|BIT5, CLEAR_BITS); 
    SiiRegWrite(TX_PAGE_L1 | 0x80, 0x84); 
    SiiRegWrite(TX_PAGE_L1 | 0x81, 0x01); 
    SiiRegWrite(TX_PAGE_L1 | 0x82, 0x0A); 
    SiiRegWrite(TX_PAGE_L1 | 0x83, 0x70); 
    SiiRegWrite(TX_PAGE_L1 | 0x84, 0x01); 
    SiiRegWrite(TX_PAGE_L1 | 0x8D, 0x00); 
    SiiRegModify(TX_PAGE_L1 | 0x3E, BIT4|BIT5, SET_BITS); 
}
static void AudioVideoIsr(bool_t force_update)
{
    AVModeChange_t mode_change = {false, false};
    //static AVMode_t mode = {VM_INVALID, AUD_INVALID};
    if (force_update)
    {
        if (1)//(SiiVideoInputIsValid())
        {TX_DEBUG_PRINT(("SiiVideoInputIsValid,audio_changed,video_changed\n"));
            mode_change.audio_change = true;
            mode_change.video_change = true;
        }
    }
    else
    {	TX_DEBUG_PRINT(("force_update=false...............\n"));
        //AVModeDetect(&mode_change, &AVmode);
    }
    if (mode_change.audio_change)
    {
    	TX_DEBUG_PRINT(("SetAudioMode & SetACRNValue\n"));
        //SetAudioMode(mode.audio_mode);
		SetAudioMode(AVmode.audio_mode);
        SetACRNValue();
    }
    if(mode_change.video_change)// && SiiVideoInputIsValid())
    {
    	TX_DEBUG_PRINT(("mode_change.video_changed =true\n "));
        SiiRegModify(REG_LM_DDC, VID_MUTE, SET_BITS);   
        SiiRegModify(REG_SRST, BIT0, SET_BITS);    
        //video_data.outputColorSpace = video_data.inputColorSpace;
        video_data.outputVideoCode = video_data.inputVideoCode;
        video_data.outputcolorimetryAspectRatio = video_data.inputcolorimetryAspectRatio;
        SiiRegModify(REG_SRST, BIT0, CLEAR_BITS);
        SiiRegModify(REG_LM_DDC, VID_MUTE, CLEAR_BITS);   
    }
    if ((mode_change.video_change || mode_change.audio_change) && tmdsPowRdy)
    {
        if (1)//(SiiVideoInputIsValid())
        {
            SiiRegModify(REG_TMDS_CCTRL, TMDS_OE, SET_BITS);
            SendAudioInfoFrame();
        	TX_DEBUG_PRINT(("((mode_change.video_change || mode_change.audio_change) && tmdsPowRdy) \n"));
            SendAviInfoframe();
        }
        else
        {
            SiiRegModify(REG_TMDS_CCTRL, TMDS_OE, CLEAR_BITS);
            TX_DEBUG_PRINT (("TMDS Ouput Disabled due to invalid input\n"));
        }
    }
}

#if !defined GPIO_MHL_RST_B_PIN
#error GPIO_MHL_RST_B_PIN no defined
#endif

void SiiMhlTxHwResetKeepLow(void)
{	
	printk("%s,%d\n", __func__, __LINE__);
	mt_set_gpio_out(GPIO_MHL_RST_B_PIN, GPIO_OUT_ZERO);
}

void SiiMhlTxHwReset(uint16_t hwResetPeriod,uint16_t hwResetDelay)
{	
	printk("%s,%d\n", __func__, __LINE__);
	mt_set_gpio_out(GPIO_MHL_RST_B_PIN, GPIO_OUT_ONE);
	msleep(hwResetPeriod);
	mt_set_gpio_out(GPIO_MHL_RST_B_PIN, GPIO_OUT_ZERO);
	msleep(hwResetPeriod);
	mt_set_gpio_out(GPIO_MHL_RST_B_PIN, GPIO_OUT_ONE);
	msleep(hwResetDelay);
}

#if 0
//mt6577
void SiiMhlTxHwGpioSuspend(void)
{
	int i;
	u32 gpio[]={
		GPIO19, GPIO20, GPIO21, GPIO22, GPIO23, 
		GPIO24, GPIO25, GPIO26, GPIO27, GPIO28, 
		GPIO29, GPIO30, GPIO31, GPIO32, GPIO33, 
		GPIO34, GPIO35, GPIO36, GPIO37, GPIO38, 
		GPIO39, GPIO40, GPIO41, GPIO42, GPIO43, 
		GPIO44, GPIO45, GPIO46, GPIO53, GPIO54, 
		GPIO55, 
	};

	printk("%s,%d\n", __func__, __LINE__);
	
	for(i=0; i<ARRAY_SIZE(gpio); i++){
		mt_set_gpio_mode(gpio[i], GPIO_MODE_00);
		mt_set_gpio_dir(gpio[i], GPIO_DIR_IN);
		mt_set_gpio_pull_select(gpio[i], GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(gpio[i], GPIO_PULL_ENABLE);
	}	
}
#else
void SiiMhlTxHwGpioSuspend(void)
{
    int i;
    u32 gpio[]={
        GPIO143, GPIO144, GPIO145, GPIO146, GPIO147,
        GPIO148, GPIO149, GPIO150, GPIO151, GPIO152,
        GPIO153, GPIO154, GPIO155, GPIO156, GPIO157,
        GPIO158, GPIO159, GPIO160, GPIO161, GPIO162,
        GPIO163, GPIO164, GPIO165, GPIO166, GPIO167,
        GPIO168, GPIO169, GPIO170, GPIO120, GPIO121,
        GPIO122,
    };
    printk("%s,%d\n", __func__, __LINE__);
    for(i=0; i<ARRAY_SIZE(gpio); i++){
        mt_set_gpio_mode(gpio[i], GPIO_MODE_00);
        mt_set_gpio_dir(gpio[i], GPIO_DIR_IN);
        mt_set_gpio_pull_select(gpio[i], GPIO_PULL_DOWN);
        mt_set_gpio_pull_enable(gpio[i], GPIO_PULL_ENABLE);
    }
}
#endif

#if 0
//mt6577
void SiiMhlTxHwGpioResume(void)
{
	int i;
	u32 gpio_rgb[]={
		GPIO19, GPIO20, GPIO21, GPIO22, GPIO23, 
		GPIO24, GPIO25, GPIO26, GPIO27, GPIO28, 
		GPIO29, GPIO30, GPIO31, GPIO32, GPIO33, 
		GPIO34, GPIO35, GPIO36, GPIO37, GPIO38, 
		GPIO39, GPIO40, GPIO41, GPIO42, GPIO43, 
		GPIO44, GPIO45, GPIO46
	};

	u32 gpio_i2s[]={GPIO53, GPIO54, GPIO55};

	printk("%s,%d\n", __func__, __LINE__);
	
	for(i=0; i<ARRAY_SIZE(gpio_rgb); i++){
		mt_set_gpio_mode(gpio_rgb[i], GPIO_MODE_01);		
		mt_set_gpio_pull_enable(gpio_rgb[i], GPIO_PULL_DISABLE);
	}	

	for(i=0; i<ARRAY_SIZE(gpio_i2s); i++){
		mt_set_gpio_mode(gpio_i2s[i], GPIO_MODE_04);
		mt_set_gpio_pull_select(gpio_i2s[i], GPIO_PULL_DOWN);
		mt_set_gpio_pull_enable(gpio_i2s[i], GPIO_PULL_ENABLE);
	}	
}
#else
void SiiMhlTxHwGpioResume(void)
{
    int i;
    u32 gpio_rgb[]={
        GPIO143, GPIO144, GPIO145, GPIO146, GPIO147,
        GPIO148, GPIO149, GPIO150, GPIO151, GPIO152,
        GPIO153, GPIO154, GPIO155, GPIO156, GPIO157,
        GPIO158, GPIO159, GPIO160, GPIO161, GPIO162,
        GPIO163, GPIO164, GPIO165, GPIO166, GPIO167,
        GPIO168, GPIO169, GPIO170
    };
    u32 gpio_i2s[]={GPIO120, GPIO121, GPIO122};
    printk("%s,%d\n", __func__, __LINE__);
    for(i=0; i<ARRAY_SIZE(gpio_rgb); i++){
        mt_set_gpio_mode(gpio_rgb[i], GPIO_MODE_01);
        mt_set_gpio_pull_enable(gpio_rgb[i], GPIO_PULL_DISABLE);
    }

    for(i=0; i<ARRAY_SIZE(gpio_i2s); i++){
        mt_set_gpio_mode(gpio_i2s[i], GPIO_MODE_01);
        mt_set_gpio_pull_select(gpio_i2s[i], GPIO_PULL_DOWN);
        mt_set_gpio_pull_enable(gpio_i2s[i], GPIO_PULL_ENABLE);
    }
}
#endif


#if defined(USE_PROC)&&defined(__KERNEL__)
void drv_mhl_seq_show(struct seq_file *s)
{
    int gpio_value;
    switch(fwPowerState)
    {
    case POWER_STATE_D3:
             seq_printf(s, "MHL POWER STATE          [D3]\n");
             break;
        case POWER_STATE_D0_NO_MHL:
             seq_printf(s, "MHL POWER STATE          [D0_NO_MHL]\n");
             break;
        case POWER_STATE_D0_MHL:
             seq_printf(s, "MHL POWER STATE          [D0_MHL]\n");
             break;
        case POWER_STATE_FIRST_INIT:
             seq_printf(s, "MHL POWER STATE          [FIRST_INIT]\n");
             break;
        default:break;
    }   
    if(tmdsPowRdy)  
        seq_printf(s, "TMDS                     [ON]\n");
    else
        seq_printf(s, "TMDS                     [OFF]\n");
    HalGpioGetPin(GPIO_SRC_VBUS_ON,&gpio_value);
    if(gpio_value)
        seq_printf(s, "SRC BUS                  [ON]\n");
    else
        seq_printf(s, "SRC BUS                  [OFF]\n");
    HalGpioGetPin(GPIO_SINK_VBUS_ON,&gpio_value);
    if(gpio_value)
        seq_printf(s, "SINK BUS                  [ON]\n");
    else
        seq_printf(s, "SINK BUS                  [OFF]\n");
}
#endif

//------------------------------------------------------------------------------
// Function Name: siHdmiTx_VideoSel()
// Function Description: Select output video mode
//
// Accepts: Video mode
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void siHdmiTx_VideoSel (int vmode)
{
	int AspectRatio = 0;
	video_data.inputColorSpace	= COLOR_SPACE_RGB;
	video_data.outputColorSpace = COLOR_SPACE_RGB;
	video_data.inputVideoCode	= vmode;
	//siHdmiTx.ColorDepth			= VMD_COLOR_DEPTH_8BIT;
	//siHdmiTx.SyncMode 			= EXTERNAL_HSVSDE;
		
	switch (vmode)
	{
		case HDMI_480I60_4X3:
		case HDMI_576I50_4X3:
			AspectRatio = VMD_ASPECT_RATIO_4x3;
			break;
			
		case HDMI_480I60_16X9:
		case HDMI_576I50_16X9:
			AspectRatio  	= VMD_ASPECT_RATIO_16x9;
			break;
			
		case HDMI_480P60_4X3:
		case HDMI_576P50_4X3:
		case HDMI_640X480P:
			AspectRatio  	= VMD_ASPECT_RATIO_4x3;
			break;

		case HDMI_480P60_16X9:
		case HDMI_576P50_16X9:
			AspectRatio  	= VMD_ASPECT_RATIO_16x9;
			break;
			
		case HDMI_720P60:
		case HDMI_720P50:
		case HDMI_1080I60:
		case HDMI_1080I50:
		case HDMI_1080P24:
		case HDMI_1080P25:
		case HDMI_1080P30:				
			AspectRatio  	= VMD_ASPECT_RATIO_16x9;
			break;
				
		default:
			break;
		}
	if (AspectRatio == VMD_ASPECT_RATIO_4x3)
        video_data.inputcolorimetryAspectRatio = 0x18;
    else video_data.inputcolorimetryAspectRatio = 0x28;
   		video_data.input_AR = AspectRatio;

}

//------------------------------------------------------------------------------
// Function Name: siHdmiTx_AudioSel()
// Function Description: Select output audio mode
//
// Accepts: Audio Fs
// Returns: none
// Globals: none
//------------------------------------------------------------------------------
void siHdmiTx_AudioSel (int AduioMode)
{
	AVmode.audio_mode			= AduioMode;
	/*
	siHdmiTx.AudioChannels		= ACHANNEL_2CH;
	siHdmiTx.AudioFs				= Afs;
	siHdmiTx.AudioWordLength		= ALENGTH_24BITS;
	siHdmiTx.AudioI2SFormat		= (MCLK256FS << 4) |SCK_SAMPLE_RISING_EDGE |0x00; //last num 0x00-->0x02
	*/
}

//------------------------------------------------------------------------------
// Function Name: siMhlTx_AudioSet()
// Function Description: Set the 9022/4 audio interface to basic audio.
//
// Accepts: none
// Returns: Success message if audio changed successfully.
//                  Error Code if resolution change failed
// Globals: mhlTxAv
//------------------------------------------------------------------------------
uint8_t siMhlTx_AudioSet (void)
{
	TX_DEBUG_PRINT(("[MHL]: >>siMhlTx_AudioSet()\n"));
		  
	//SetAudioMute(AUDIO_MUTE_MUTED);	// mute output
	SetAudioMode(AVmode.audio_mode);
	SetACRNValue();
	return 0;
}
//------------------------------------------------------------------------------
// Function Name: siMhlTx_VideoAudioSet()
// Function Description: Set the 9022/4 video resolution
//
// Accepts: none
// Returns: Success message if video resolution changed successfully.
//                  Error Code if resolution change failed
// Globals: mhlTxAv
//------------------------------------------------------------------------------
//============================================================
#define T_RES_CHANGE_DELAY      128         // delay between turning TMDS bus off and changing output resolution

uint8_t siMhlTx_VideoAudioSet (void)   
{
  	TX_DEBUG_PRINT(("[MHL]: >>siMhlTx_VideoAudioSet()\n"));
	
	SiiRegModify(TX_PAGE_L1 | 0xDF, BIT0, SET_BITS);

	SiiMhlTxDrvTmdsControl( false );
	HalTimerWait(T_RES_CHANGE_DELAY);	// allow control InfoFrames to pass through to the sink device.
	//siMhlTx_AudioSet();
	AudioVideoIsr(true);
	//siMhlTx_Init();
	SiiRegModify(TX_PAGE_L1 | 0xDF, BIT0, CLEAR_BITS);
	SiiMhlTxTmdsEnable();
    return 0;
}


