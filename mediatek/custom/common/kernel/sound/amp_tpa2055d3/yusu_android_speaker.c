
/*****************************************************************************
*                E X T E R N A L      R E F E R E N C E S
******************************************************************************
*/
#include <asm/uaccess.h>
#include <linux/xlog.h>
#include <linux/i2c.h>
#include "yusu_android_speaker.h"


/*****************************************************************************
*                          DEBUG INFO
******************************************************************************
*/

static bool eamp_log_on = true;

#define EAMP_PRINTK(fmt, arg...) \
	do { \
		if (eamp_log_on) xlog_printk(ANDROID_LOG_INFO,"EAMP", "[EAMP]: %s() "fmt"\n", __func__,##arg); \
	}while (0)


/*****************************************************************************
*				For I2C defination
******************************************************************************
*/	

// device address
#define EAMP_SLAVE_ADDR_WRITE	0xE0
#define EAMP_SLAVE_ADDR_READ	0xE1
#define EAMP_I2C_CHANNEL     	(0)        //I2C Channel 0
#define EAMP_I2C_DEVNAME "TPA2055D3"

//define registers
#define EAMP_REG_SUBSYSTEMCONTROL  			0x00
#define EMPA_REG_INPUTCONTROL				0x01		
#define EMPA_REG_LIMITER_CONTROL			0x02  
#define EMPA_REG_SPEAKER_OUTPUT_CONTROL		0x03
#define EMPA_REG_HEADPHONE_OUTPUT_CONTROL	0x04
#define EMPA_REG_SPEAKER_VOLUME				0x05
#define EMPA_REG_HEADPHONE_LEFT_VOLUME		0x06
#define EMPA_REG_HEADPHONE_RIGHT_VOLUME		0x07

//control point	
#define AUDIO_CONTROL_POINT_NUM (5);

// I2C variable
static struct i2c_client *new_client = NULL;


#if 0 // old I2C register method
static const struct i2c_device_id eamp_i2c_id[] = {{EAMP_I2C_DEVNAME,0},{}};   
static unsigned short force[] = {EAMP_I2C_CHANNEL, EAMP_SLAVE_ADDR_WRITE, I2C_CLIENT_END, I2C_CLIENT_END};	 
static const unsigned short * const forces[] = { force, NULL }; 			 
static struct i2c_client_address_data addr_data = { .forces = forces,};
#endif

// new I2C register method
static const struct i2c_device_id eamp_i2c_id[] = {{EAMP_I2C_DEVNAME,0},{}}; 
static struct i2c_board_info __initdata  eamp_dev={I2C_BOARD_INFO(EAMP_I2C_DEVNAME,(EAMP_SLAVE_ADDR_WRITE>>1))};

//function declration
static int eamp_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int eamp_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int eamp_i2c_remove(struct i2c_client *client);
//i2c driver 
struct i2c_driver eamp_i2c_driver = { 
    .probe = eamp_i2c_probe,								   
    .remove = eamp_i2c_remove,							 
    //.detect = eamp_i2c_detect,							 
    .driver = {
    	.name =EAMP_I2C_DEVNAME,
    	},
    .id_table = eamp_i2c_id,							 
    //.address_data = &addr_data, 					   
};

// speaker, earpiece, headphone status and path;
static bool gsk_on = false;
static bool gep_on = false;
static bool ghp_on = false;
//volume and gain
static u8  gspvol = 0x1f;
static u8  ghplvol = 0x19;
static u8  ghprvol = 0x19;
static u8  gch1gain = 0x0;
static u8  gch2gain = 0x0;

//mode
static u32 gMode	 = 0;
static u32 gPreMode  = 0; 

//mask
typedef enum
{
    GAIN_MASK_HP	  = 0x1,
    GAIN_MASK_SPEAKER = 0x2,
    GAIN_MASK_INPUT2  = 0x4,
    GAIN_MASK_INPUT1  = 0x8,
    GAIN_MASK_ALL     = (GAIN_MASK_HP     |
                         GAIN_MASK_SPEAKER |
                         GAIN_MASK_INPUT2  |
                         GAIN_MASK_INPUT1)
}gain_mask;

//kernal to open speaker 
static bool gsk_forceon = false;
static bool gsk_preon   = false;
static bool gep_preon   = false;



// volume table

static  int gCtrPointNum = AUDIO_CONTROL_POINT_NUM;
static  s8  gCtrPoint[] = {2,2,5,5,5};  // repesent 2bis, 2bits,5bits,5bits,5bits
static  s8  gCtrPoint_in1Gain[]= {0,6,12,20};
static  s8  gCtrPoint_in2Gain[]= {0,6,12,20};
static  s8  gCtrPoint_SpeakerVol[] = {
	-60 ,-50, -45, -42,
	-39 ,-36, -33, -30, 
	-27 ,-24, -21, -20,
	-19 ,-18, -17, -16,
	-15 ,-14, -13, -12,
	-11 ,-10, -9,  -8 ,
	-7  ,-6 , -5 , -4 ,
	-3  ,-2 , -1,  0};

static  s8 gCtrPoint_HeadPhoneLVol[]= {
	-60 ,-50, -45, -42,
	-39 ,-36, -33, -30, 
	-27 ,-24, -21, -20,
	-19 ,-18, -17, -16,
	-15 ,-14, -13, -12,
	-11 ,-10, -9,  -8 ,
	-7  ,-6 , -5 , -4 ,
	-3  ,-2 , -1,  0};

static  s8 gCtrPoint_HeadPhoneRVol[]= {
	-60 ,-50, -45, -42,
	-39 ,-36, -33, -30, 
	-27 ,-24, -21, -20,
	-19 ,-18, -17, -16,
	-15 ,-14, -13, -12,
	-11 ,-10, -9,  -8 ,
	-7  ,-6 , -5 , -4 ,
	-3  ,-2 , -1,  0};
	
static  s8 *gCtrPoint_table[5]={
	gCtrPoint_in1Gain,
	gCtrPoint_in2Gain,
	gCtrPoint_SpeakerVol,
	gCtrPoint_HeadPhoneLVol,
	gCtrPoint_HeadPhoneRVol};

// function implementation

//read one register
ssize_t static eamp_read_byte(u8 addr, u8 *returnData)
{
	if(!new_client)
	{	
		EAMP_PRINTK("I2C client not initialized!!");
		return -1;
	}	
	char	 cmd_buf[1]={0x00};
	char	 readData = 0;
	int 	ret=0;
	cmd_buf[0] = addr;
	ret = i2c_master_send(new_client, &cmd_buf[0], 1);
	if (ret < 0) {
		EAMP_PRINTK("read sends command error!!");
		return -1;
	}
	ret = i2c_master_recv(new_client, &readData, 1);
	if (ret < 0) {
		EAMP_PRINTK("reads recv data error!!");
		return -1;
	} 
	*returnData = readData;
	EAMP_PRINTK("addr 0x%x data 0x%x",addr, readData);
	return 0;
}
	
//read one register
static u8	I2CRead(u8 addr)
{
	u8 regvalue;
	eamp_read_byte(addr,&regvalue);
	return regvalue;
}

// write register
static ssize_t	I2CWrite(u8 addr, u8 writeData)
{
	if(!new_client)
	{	
		EAMP_PRINTK("I2C client not initialized!!");
		return -1;
	}	
	char	write_data[2] = {0};
	int    ret=0;	
	write_data[0] = addr;		  // ex. 0x01
	write_data[1] = writeData;
	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		EAMP_PRINTK("write sends command error!!");
		return -1;
	}
	EAMP_PRINTK("addr 0x%x data 0x%x",addr,writeData);
	return 0;
}
	
//write register
static ssize_t	eamp_write_byte(u8 addr, u8 writeData)
{
	if(!new_client)
	{	
		EAMP_PRINTK("I2C client not initialized!!");
		return -1;
	}	
	char	write_data[2] = {0};
	int    ret=0;	
	write_data[0] = addr;		  // ex. 0x01
	write_data[1] = writeData;
	ret = i2c_master_send(new_client, write_data, 2);
	if (ret < 0) {
		EAMP_PRINTK("write sends command error!!");
		return -1;
	}
	EAMP_PRINTK("addr 0x%x data 0x%x",addr,writeData);
	return 0;
}

//*****************************subsystem control Register, functions to control bits*************************
//speaker bypass mode
static ssize_t eamp_set_bypass_mode(bool enable)
{
	EAMP_PRINTK("enable=%d",enable);
	u8 temp_control_reg = 0;
	eamp_read_byte(EAMP_REG_SUBSYSTEMCONTROL,&temp_control_reg);

	if (enable == true)
	{
		//enable bypass
		eamp_write_byte(EAMP_REG_SUBSYSTEMCONTROL, temp_control_reg & ~0x10);
	}
	else
	{
		//turn off bypass
		eamp_write_byte(EAMP_REG_SUBSYSTEMCONTROL, temp_control_reg | 0x10);
	}
	return 0;
}

//Software Shutdown mode
	
static ssize_t eamp_set_sws_mode(bool deactivate)
{
	EAMP_PRINTK("deactivate=%d",deactivate);
	u8 temp_control_reg = 0;
	eamp_read_byte(EAMP_REG_SUBSYSTEMCONTROL,&temp_control_reg);
	if (deactivate == true)
	{
		//enable bypass
		eamp_write_byte(EAMP_REG_SUBSYSTEMCONTROL, temp_control_reg | 0x08);
	}
	else
	{
		//turn off bypass
		eamp_write_byte(EAMP_REG_SUBSYSTEMCONTROL, temp_control_reg & ~0x08);
	}
	return 0;
}
	
//input mode and volume control Register
static ssize_t eamp_clear_input_gain()
{
	EAMP_PRINTK("");
	u8 temp_input_reg = 0;
	
	//eamp_read_byte(EMPA_REG_INPUTCONTROL,&temp_input_reg);
	//temp_input_reg = (temp_input_reg >>4)<<4;
	eamp_write_byte(EMPA_REG_INPUTCONTROL, temp_input_reg);
	
	return 0;
}
// set input gain on channel 1 and 2
static ssize_t eamp_set_input_gain( u8 inGain)
{
	EAMP_PRINTK("inGain(0x%x)",inGain);
	u8 temp_input_reg = 0;
	
	eamp_read_byte(EMPA_REG_INPUTCONTROL,&temp_input_reg);
	temp_input_reg = (temp_input_reg >>4)<<4;
	eamp_write_byte(EMPA_REG_INPUTCONTROL, temp_input_reg | (inGain & 0xf));
	
	return 0;
}

// set input gain on channel 1 
static ssize_t eamp_set_input1_gain( u8 inGain)
{
	EAMP_PRINTK("inGain(0x%x)",inGain);
	u8 temp_input_reg = 0;
	
	eamp_read_byte(EMPA_REG_INPUTCONTROL,&temp_input_reg);
	temp_input_reg = (temp_input_reg & 0xf3) | (inGain & 0x3)<<2 ;
	eamp_write_byte(EMPA_REG_INPUTCONTROL, temp_input_reg);
	
	return 0;
}

// set input gain on channel 2

static ssize_t eamp_set_input2_gain( u8 inGain)
{
	EAMP_PRINTK("inGain(0x%x)",inGain);
	u8 temp_input_reg = 0;
	
	eamp_read_byte(EMPA_REG_INPUTCONTROL,&temp_input_reg);
	temp_input_reg = ((temp_input_reg >>2)<<2) | (inGain & 0x3) ;
	eamp_write_byte(EMPA_REG_INPUTCONTROL, temp_input_reg);
	
	return 0;
}


//  set input mode on channel 1 and 2.  0 for single-end inputs, 1 for differential inputs.
static ssize_t eamp_set_input_mode( bool in1se, bool in2se)
{
	u8 temp_input_reg = 0;
	eamp_read_byte(EMPA_REG_INPUTCONTROL,&temp_input_reg);
	if(in1se)
	{
		
		temp_input_reg = temp_input_reg & ~0x20;
	}
	else
	{
		temp_input_reg =temp_input_reg | 0x20;
	}
	
	if(in2se)
	{
		
		temp_input_reg =temp_input_reg & ~0x10;
	}
	else
	{
		temp_input_reg =temp_input_reg | 0x10;
	}
	return eamp_write_byte(EMPA_REG_INPUTCONTROL, temp_input_reg);
}
	
//Release and attack time
static ssize_t eamp_Release_attackTime_speed(u8 ATK_time, u8 REL_time)
{
	
	u8	write_data = (ATK_time & 0x07) | (REL_time & 0x0f)<<3 ;
	return eamp_write_byte(EMPA_REG_LIMITER_CONTROL,write_data);
}
	
//speaker mux and limiter 
	
//set limiter level
static ssize_t eamp_set_speakerLimiter_level(u8 limitlev )
{
	u8 temp_speak_reg = 0;
	eamp_read_byte(EMPA_REG_SPEAKER_OUTPUT_CONTROL,&temp_speak_reg);
	
	temp_speak_reg = (temp_speak_reg >>4) <<4;
	temp_speak_reg  = temp_speak_reg | (limitlev & 0x0f);
	return eamp_write_byte(EMPA_REG_SPEAKER_OUTPUT_CONTROL,temp_speak_reg);
}
	
// speaker limiter enable. 1 enable, 0 disable.
static ssize_t eamp_speakerLimiter_enable(bool enable )
{
	u8 temp_speak_reg = 0;
	eamp_read_byte(EMPA_REG_SPEAKER_OUTPUT_CONTROL,&temp_speak_reg);
	
	if (enable == true)
	{
		//enable 
		eamp_write_byte(EMPA_REG_SPEAKER_OUTPUT_CONTROL, temp_speak_reg | 0x10);
	}
	else
	{
		//disable
		eamp_write_byte(EMPA_REG_SPEAKER_OUTPUT_CONTROL, temp_speak_reg & ~0x10);
	}
	return 0;
}
	
	
// control for speaker channel.
static ssize_t eamp_set_speakerOut(u8  speakerout )
{
	u8 temp_speak_reg = 0;
	eamp_read_byte(EMPA_REG_SPEAKER_OUTPUT_CONTROL,&temp_speak_reg);
	
	temp_speak_reg = (temp_speak_reg & 0x9f ) | ((speakerout & 0x03) << 5);
	eamp_write_byte(EMPA_REG_SPEAKER_OUTPUT_CONTROL, temp_speak_reg);
	return 0;
}
	
//Headphone mux and limiter 
// set headphone limiter level
static ssize_t eamp_set_headPhoneLimiter_level(u8 limitlev )
{
	u8 temp_headphone_reg = 0;
	eamp_read_byte(EMPA_REG_HEADPHONE_OUTPUT_CONTROL,&temp_headphone_reg);
	
	temp_headphone_reg = (temp_headphone_reg >>3) <<3;
	temp_headphone_reg	= temp_headphone_reg | (limitlev & 0x07);
	return eamp_write_byte(EMPA_REG_HEADPHONE_OUTPUT_CONTROL,temp_headphone_reg);
	
}

// enable /disable headphone limiter
static ssize_t eamp_headPhoneLimiter_enable(bool enable )
{
	u8 temp_headphone_reg = 0;
	eamp_read_byte(EMPA_REG_HEADPHONE_OUTPUT_CONTROL,&temp_headphone_reg);
	
	if (enable == true)
	{
		//enable 
		eamp_write_byte(EMPA_REG_HEADPHONE_OUTPUT_CONTROL, temp_headphone_reg | 0x10);
	}
	else
	{
		//disable
		eamp_write_byte(EMPA_REG_HEADPHONE_OUTPUT_CONTROL, temp_headphone_reg & ~0x10);
	}
	return 0;
}

// control for headphone channel
static ssize_t eamp_set_headPhoneOut(u8  headphoneout )
{
	u8 temp_headphone_reg = 0;
	eamp_read_byte(EMPA_REG_HEADPHONE_OUTPUT_CONTROL,&temp_headphone_reg);
	
	temp_headphone_reg = (temp_headphone_reg & 0x9f ) | ((headphoneout & 0x03) << 5);
	return eamp_write_byte(EMPA_REG_HEADPHONE_OUTPUT_CONTROL, temp_headphone_reg);
}
	
//speaker volume  

// openspeaker
static ssize_t eamp_set_speaker_Open(bool enable)
{
	EAMP_PRINTK("enable=%d",enable);
	u8 temp_spvol_reg = 0;
	eamp_read_byte(EMPA_REG_SPEAKER_VOLUME,&temp_spvol_reg);
	
	if (enable == true)
	{
		//enable 
		eamp_write_byte(EMPA_REG_SPEAKER_VOLUME, temp_spvol_reg | 0x20);
	}
	else
	{
		//disable
		eamp_write_byte(EMPA_REG_SPEAKER_VOLUME, temp_spvol_reg & ~0x20);
	}
	return 0;	
}
	
//set speaker volume
static ssize_t eamp_set_speaker_vol(u8	vol)
{
	EAMP_PRINTK("vol=0x%x",vol);
	u8 temp_spvol_reg = 0;
	eamp_read_byte(EMPA_REG_SPEAKER_VOLUME,&temp_spvol_reg);
	temp_spvol_reg = (temp_spvol_reg >> 5) << 5;
	temp_spvol_reg = temp_spvol_reg | (vol & 0x1f);
	eamp_write_byte(EMPA_REG_SPEAKER_VOLUME, temp_spvol_reg);
	return 0;
}
	
//Headphone left channel volume  
//enable headphone left  channel
static ssize_t eamp_set_headPhoneL_open( bool  enable )
{
	EAMP_PRINTK("enable=%d",enable);
	u8 temp_hpvol_reg = 0;
	eamp_read_byte(EMPA_REG_HEADPHONE_LEFT_VOLUME,&temp_hpvol_reg);
	
	if (enable == true)
	{
		//enable 
		eamp_write_byte(EMPA_REG_HEADPHONE_LEFT_VOLUME, temp_hpvol_reg | 0x20);
	}
	else
	{
		//disable
		eamp_write_byte(EMPA_REG_HEADPHONE_LEFT_VOLUME, temp_hpvol_reg & ~0x20);
	}
	return 0;
	
}
	
//set  headphone  volume
static ssize_t eamp_set_headPhone_vol(u8 HP_vol)
{
	EAMP_PRINTK("vol=0x%x",HP_vol);
	u8 temp_hpvol_reg = 0;
	eamp_read_byte(EMPA_REG_HEADPHONE_LEFT_VOLUME,&temp_hpvol_reg);
	temp_hpvol_reg = (temp_hpvol_reg>>5)<<5;
	temp_hpvol_reg = temp_hpvol_reg | 0x40;
	temp_hpvol_reg = temp_hpvol_reg | (HP_vol & 0x1f);
	return eamp_write_byte(EMPA_REG_HEADPHONE_LEFT_VOLUME,temp_hpvol_reg);
}
	
//set  headphone left  volume
static ssize_t eamp_set_headPhone_lvol(u8 HPL_Vol)
{
	EAMP_PRINTK("vol=0x%x",HPL_Vol);

	u8 temp_hpvol_reg = 0;
	eamp_read_byte(EMPA_REG_HEADPHONE_LEFT_VOLUME,&temp_hpvol_reg);
	temp_hpvol_reg = (temp_hpvol_reg>>5)<<5;
	temp_hpvol_reg = temp_hpvol_reg & 0xbf;
	temp_hpvol_reg = temp_hpvol_reg | (HPL_Vol & 0x1f);
	
	return eamp_write_byte(EMPA_REG_HEADPHONE_LEFT_VOLUME,temp_hpvol_reg);
}
	
//Headphone right channel volume  register
	
	
//enable headphone Right  channel
static ssize_t eamp_set_headPhoneR_open( bool  enable )
{
	EAMP_PRINTK("enable=%d",enable);
	u8 temp_hpvol_reg = 0;
	eamp_read_byte(EMPA_REG_HEADPHONE_RIGHT_VOLUME,&temp_hpvol_reg);
	
	if (enable == true)
	{
		//enable 
		eamp_write_byte(EMPA_REG_HEADPHONE_RIGHT_VOLUME, temp_hpvol_reg | 0x20);
	}
	else
	{
		//disable
		eamp_write_byte(EMPA_REG_HEADPHONE_RIGHT_VOLUME, temp_hpvol_reg & ~0x20);
	}
	return 0;
	
}
	
//set  headphone right volume
static ssize_t eamp_set_headPhone_rvol(u8 HPR_Vol)
{
	EAMP_PRINTK("vol=0x%x",HPR_Vol);

	u8 temp_hpvol_reg = 0;
	eamp_read_byte(EMPA_REG_HEADPHONE_RIGHT_VOLUME,&temp_hpvol_reg);
	temp_hpvol_reg = (temp_hpvol_reg>>5)<<5;
	temp_hpvol_reg = temp_hpvol_reg | (HPR_Vol & 0x1f);
	
	return eamp_write_byte(EMPA_REG_HEADPHONE_RIGHT_VOLUME,temp_hpvol_reg);
}
	
	
//**********************************functions to control devices***********************************

// set registers to default value
static ssize_t eamp_resetRegister()
{
	EAMP_PRINTK("");
	I2CWrite(0,0x18);
	I2CWrite(1,0x00);
	I2CWrite(2,0x2A);
	I2CWrite(3,0x0F);
	I2CWrite(4,0x00);
	I2CWrite(5,0x15);
	I2CWrite(6,0x5F);
	I2CWrite(7,0x1F);
	return 0;
}
	
	
static ssize_t eamp_openEarpiece()
{
	EAMP_PRINTK("");
	
	I2CWrite(0,0x10); //close bypass and not shut down device for headphone
	gep_on=true;
	return 0;
}
	
static ssize_t eamp_closeEarpiece()
{
	EAMP_PRINTK("");
	I2CWrite(0,0x00);//open bypass, open device for headphone
	gep_on=false;
	return 0;
}
	
static ssize_t eamp_openheadPhone()
{
	EAMP_PRINTK("");
	if(gep_on) // if earpiece is open ,not open bypass
	{
		I2CWrite(0,0x10);//turn open ,not  open bypass
	}
	else
	{
		I2CWrite(0,0x00);//turn open
	}
	if(gMode==2) //MODE_IN_CALL
	{
		I2CWrite(1,0x30 | gch1gain<<2 | gch2gain);//difference end inputs with gchgain input gain, 110000
	}
	else
	{
		I2CWrite(1,0x00 | gch1gain<<2 | gch2gain);//Single-ended inputs with gchgain input gain 		
	}
	I2CWrite(6,0x60);//set headphone -60 dB 
	I2CWrite(7,0x20);//Right headphone enable  
	if(gMode==2)
	{
		I2CWrite(4,0x40);//IN2Input, limiter off
	}
	else
	{
		I2CWrite(4,0x20);//IN1 Input, limiter off	
	}
	if(ghplvol == ghprvol)
	{
		I2CWrite(6,(0x60|ghplvol));//HPL gain = HPR gain = vol  
	}
	else
	{
		I2CWrite(6,(0x20|ghplvol));
		I2CWrite(7,(0x20|ghprvol));
	}
	ghp_on = true;
	return 0;
}

static ssize_t eamp_closeheadPhone()
{
	EAMP_PRINTK("");
	I2CWrite(6,0x20);  //Turn volume down to -60dB,this maybe 0x60,
	I2CWrite(4,0x00);	//mute HP
	I2CWrite(6,0x00);	//Disable HPL
	I2CWrite(7,0x00);	//Disable HPR
	//I2CWrite(6,0x20);
	//I2CWrite(7,0x20);
	if(!gsk_on)
	{
		I2CWrite(0,0x08); //shut down
	}
	ghp_on = false;
	return 0;
}
	
static ssize_t eamp_openspeaker()
{
	EAMP_PRINTK("");
	
	if(gMode == 2) //MODE_IN_CALL
	{
		I2CWrite(0,0x00);//turn on subsystem
		I2CWrite(1,0x30 | gch1gain<<2 | gch2gain);// difference-ended inputs with gchgain input gain 
		I2CWrite(5,0x35);// Set SPK Volume	:110101
		//I2CWrite(2,0x00);// turn on AGC fo all media play. 
		I2CWrite(3,0x40);// set in2 and limiter : 1011101
		I2CWrite(5,0x20 | gspvol);// Set SPK Volume,default :111111
	}
	else
	{
		I2CWrite(0,0x00);//turn on subsystem
		I2CWrite(1,0x00 | gch1gain<<2 | gch2gain);// Single-ended inputs with 0 dB input gain 
		I2CWrite(5,0x35);// Set SPK Volume	:110101
		I2CWrite(2,0x00);// turn on AGC fo all media play. 
		I2CWrite(3,0x3D);// set in1 and limiter : 111101
		I2CWrite(5,0x20 | gspvol);// Set SPK Volume,default :111111
	}
	gsk_on = true;
	return 0;
}

static ssize_t eamp_closespeaker()
{
	EAMP_PRINTK("");
	
	I2CWrite(5,0x35);//Set SPK	-60 dB	:110101
	I2CWrite(3,0x00);//Mute SPK 				:
	I2CWrite(5,0x00);//Disable SPK	
	if(!ghp_on)
	{
		I2CWrite(0,0x08); //shut down
	}
	gsk_on = false;
	return 0;
}
	
// gainvol is composed of 24 bit, the format is as bellows.
//XXXXXXXXXXXXXXXXXXXXXXXX
// bit[ 0-1]:in1 gain, bit[ 2-3]:in2 gain, bit[4-8]: speaker vol, 
// bit[9-13]: headphone L volume, 
//bit[14-18]: headphone R volume [resolved]
// bit[21-24] mask 

static ssize_t eamp_changeGainVolume(unsigned long int param)
{
	EAMP_PRINTK("param(0x%x)",param);
	u8 mask = param & 0xF;
	u32 gainvol = param & 0xFFFFFF;
	if(mask & GAIN_MASK_INPUT1)
	{
		u8 ch1 = (gainvol>>22) & 0x3;
		if(gch1gain != ch1)
		{
			gch1gain = ch1;
			eamp_set_input1_gain(gch1gain);
		}
	}
	if(mask & GAIN_MASK_INPUT2)
	{
		u8 ch2 = (gainvol>>20) & 0x3;
	    if(gch2gain != ch2)
		{
			gch2gain = ch2;
			eamp_set_input2_gain(gch2gain);
		}
	}
	if(mask & GAIN_MASK_SPEAKER)
	{
	    u8 spk  = (gainvol>>15) & 0x1f;
		if(gspvol != spk)
		{
			gspvol	= spk;
			eamp_set_speaker_vol(gspvol);

		}
	}
	if(mask & GAIN_MASK_HP)
	{
		u8 hpl = (gainvol>>10) & 0x1f;
		if(ghplvol != hpl)
	    {
	    	ghplvol = hpl;
	        ghprvol = ghplvol; // hpl equal hpr
			eamp_set_headPhone_vol(ghplvol);
		}
	}
	return 0;
}
	
static ssize_t eamp_getGainVolume(void)
{
	EAMP_PRINTK("");
	u8 gain     = I2CRead(1) & 0xF ;
	u8 speakvol = I2CRead(5) & 0x1F;
	u8 hplreg	= I2CRead(6);
	u8 hplvol	= hplreg & 0x1F;
	u8 hprvol	= I2CRead(7) & 0x1F;
	
	if(  hplreg & 0x20 )
		return ( gain << 20 | speakvol << 15 |  \
		hplvol << 10 | hplvol << 5 );
	
	return ( gain << 20 | speakvol << 15 |	\
		hplvol << 10 | hprvol << 5 );
	
}
	
static ssize_t eamp_suspend()  
{
	EAMP_PRINTK("");
	eamp_resetRegister();
	return 0;
}
	
static ssize_t eamp_resume()
{
	EAMP_PRINTK("");
	if(gsk_on)
	{
		eamp_openspeaker();
	}
	if(ghp_on)
	{
		eamp_openheadPhone();
	}
	if(gep_on)
	{
		eamp_openEarpiece();
	}
	return 0;
}

static ssize_t eamp_getRegister(unsigned int regName)
{
	EAMP_PRINTK("Regname=%u",regName);
	
	if(regName >7)
		return -1;
	return I2CRead(regName);
}
	
static ssize_t eamp_setRegister(unsigned long int param)
{
	EAMP_PRINTK("");
	AMP_Control * p = (AMP_Control*)param;
	
	if(p->param1 >7)
		return -1;
	
	return I2CWrite(p->param1,p->param2);
}
	
	
static ssize_t eamp_setMode(unsigned long int param)
{
	EAMP_PRINTK("mode(%u)",param);
	gMode = param;
	return 0;
}

static ssize_t eamp_getCtrlPointNum()
{
	EAMP_PRINTK("");
	return gCtrPointNum;
}

static ssize_t eamp_getCtrPointBits(unsigned long int param)
{
	EAMP_PRINTK("CtrPointBits(%u)",param);
	return gCtrPoint[param];
}
	
static ssize_t eamp_getCtrlPointTable(unsigned long int param)
{
	EAMP_PRINTK("CtrlPointTable(0x%x)",param);
	AMP_Control *ampCtl = (AMP_Control*)param;
	if(copy_to_user((void __user *)ampCtl->param2,(void *)gCtrPoint_table[ampCtl->param1], 1<<gCtrPoint[ampCtl->param1])){
		return -1;
	}
	return 0;
}
	
static int eamp_command( unsigned int  type, unsigned long args,unsigned int count)
{
	EAMP_PRINTK("type(%u)",type);
	switch(type)
	{
		case EAMP_SPEAKER_CLOSE:
		{
			eamp_closespeaker();
			break;
		}
		case EAMP_SPEAKER_OPEN:
		{
			eamp_openspeaker();
			break;
		}
		case EAMP_HEADPHONE_CLOSE:
		{
			eamp_closeheadPhone();
			break;
		}
		case EAMP_HEADPHONE_OPEN:
		{
			eamp_openheadPhone();
			break;
		}
		case EAMP_EARPIECE_OPEN:
		{
			eamp_openEarpiece();
			break;
		}
		case EAMP_EARPIECE_CLOSE:
		{
			eamp_closeEarpiece();
			break;
		}
		case EAMP_GETREGISTER_VALUE:
		{
			return eamp_getRegister(args);
			break;
		}
		case EAMP_GETAMP_GAIN:
		{
			return eamp_getGainVolume();
			break;
		}
		case EAMP_SETAMP_GAIN:
		{
			eamp_changeGainVolume(args);
			break;
		}
		case EAMP_SETREGISTER_VALUE:
		{
			eamp_setRegister(args);
			break;
		}
		case EAMP_GET_CTRP_NUM:
		{
			return eamp_getCtrlPointNum();
			break;
		}
		case EAMP_GET_CTRP_BITS:
		{
			return eamp_getCtrPointBits(args);
			break;
		}
		case EAMP_GET_CTRP_TABLE:
		{
			eamp_getCtrlPointTable(args);
			break;
		}
		case EAMP_SETMODE:
		{
			eamp_setMode(args);		
		}
		default:
		return 0;
	}	
	return 0;
}
	
int Audio_eamp_command(unsigned int type, unsigned long args, unsigned int count)
{
	return eamp_command(type,args,count);
}

#if 0	
static int eamp_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {		   
    strcpy(info->type, EAMP_I2C_DEVNAME);														  
	return 0;																						
}																								   
#endif

static int eamp_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id) {			   
	new_client = client;
	eamp_resetRegister();
	EAMP_PRINTK("client=%x !!",client);
	return 0;																						
} 
	
static int eamp_i2c_remove(struct i2c_client *client)
{
	EAMP_PRINTK("");
	new_client = NULL;
    i2c_unregister_device(client);
    i2c_del_driver(&eamp_i2c_driver);
	return 0;
} 
	
static void eamp_poweron(void)
{
	return;
}
	
static void eamp_powerdown(void)
{
	return;
}
	
static int eamp_init()
{
	eamp_poweron();
	return 0;
}
	
static int eamp_deinit()
{
	EAMP_PRINTK("");
	eamp_powerdown();
	return 0;
}

static int eamp_register()
{
	EAMP_PRINTK("");
	i2c_register_board_info(0,&eamp_dev,1);
	if (i2c_add_driver(&eamp_i2c_driver)){
		EAMP_PRINTK("fail to add device into i2c");
		return -1;
	}
	return 0;
}

/*****************************************************************************
*                  F U N C T I O N        D E F I N I T I O N
******************************************************************************
*/
extern void Yusu_Sound_AMP_Switch(BOOL enable);

bool Speaker_Init(void)
{
	EAMP_PRINTK("");
	eamp_init();
	return true;
}

bool Speaker_Register(void)
{
	EAMP_PRINTK("");
	eamp_register();
	return true;
}

int ExternalAmp()
{
	return 1;
}
void Sound_SpeakerL_SetVolLevel(int level)
{
	EAMP_PRINTK("level=%d",level);
}
	
void Sound_SpeakerR_SetVolLevel(int level)
{
	EAMP_PRINTK("level=%d",level);
}
	
void Sound_Speaker_Turnon(int channel)
{
	EAMP_PRINTK("channel = %d",channel);
	eamp_command(EAMP_SPEAKER_OPEN,channel,1);
}
	
void Sound_Speaker_Turnoff(int channel)
{
	EAMP_PRINTK("channel = %d",channel);
	eamp_command(EAMP_SPEAKER_CLOSE,channel,1);
}
	
void Sound_Speaker_SetVolLevel(int level)
{
	
}
	
void Sound_Headset_Turnon(void)
{
	EAMP_PRINTK("");
}
void Sound_Headset_Turnoff(void)
{
	EAMP_PRINTK("");
}
	
//kernal use
void AudioAMPDevice_Suspend(void)
{
	EAMP_PRINTK("");
	eamp_suspend();
}

void AudioAMPDevice_Resume(void)
{
	EAMP_PRINTK("");
	eamp_resume();	
}
	
// for AEE beep sound
void AudioAMPDevice_SpeakerLouderOpen(void)
{
	EAMP_PRINTK("");
	if(gsk_on && gMode != 2) //speaker on and not incall mode
		return;	
	gsk_forceon = true;
	gPreMode = gMode;
	gsk_preon = gsk_on;
	gep_preon = gep_on;
	if(gsk_on)
	{
		eamp_closespeaker();
	}
	gMode = 0;
	eamp_openspeaker();
	return ;
}
	
// for AEE beep sound
void AudioAMPDevice_SpeakerLouderClose(void)
{
	EAMP_PRINTK("");
	if(gsk_forceon)
	{
		eamp_closespeaker();
		gMode = gPreMode;
		if(gep_preon)
		{
			eamp_openEarpiece();
		}
		else if(gsk_preon)
		{
			eamp_openspeaker();
		}
	}
	gsk_forceon = false;
}
	
// mute device when INIT_DL1_STREAM
void AudioAMPDevice_mute(void)
{
	if(ghp_on)
		eamp_closeheadPhone();	
	if(gsk_on)
		eamp_closespeaker();
	// now not control earpiece.
}
	
bool Speaker_DeInit(void)
{
	eamp_deinit();
	return true;
}
	
	
static char *ExtFunArray[] =
{
	"InfoMATVAudioStart",
	"InfoMATVAudioStop",
	"End",
};
	
kal_int32 Sound_ExtFunction(const char* name, void* param, int param_size)
{
	int i = 0;
	int funNum = -1;
	
	//Search the supported function defined in ExtFunArray
	while(strcmp("End",ExtFunArray[i]) != 0 ) {		//while function not equal to "End"
		
		if (strcmp(name,ExtFunArray[i]) == 0 ) {		//When function name equal to table, break
			funNum = i;
			break;
		}
		i++;
	}
	
	switch (funNum) {
	case 0:			//InfoMATVAudioStart
		printk("InfoMATVAudioStart");
		break;
		
	case 1:			//InfoMATVAudioStop
		printk("InfoMATVAudioStop");
		break;
		
	default:
		break;
	}
	return 1;
}
	
