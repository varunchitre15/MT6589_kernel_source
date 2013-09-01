#include "e3135.h"
#include "tpd.h"
#include <mach/mt_pm_ldo.h> //power doname
#include <mach/mt_typedefs.h>
#include <linux/i2c.h> //i2c cliend
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/miscdevice.h>
#include <cust_eint.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include "cust_gpio_usage.h"
#include <linux/dma-mapping.h>

#define TPD_POWER_SOURCE_CUSTOM MT65XX_POWER_LDO_VGP5
static DECLARE_WAIT_QUEUE_HEAD( waiter );
static struct task_struct * thread = NULL;
struct i2c_client *i2c_client = NULL;

static u8 *I2CDMABuf_va = NULL;
static u32 I2CDMABuf_pa = NULL;
static struct semaphore pSem;
static const struct i2c_device_id e3135_tpd_id[] = {{e3135_NAME,e3135_I2C_Channel},{}};
static struct i2c_board_info __initdata e3135_i2c_tpd={ I2C_BOARD_INFO(e3135_NAME, (e3135_I2C_SLAVE_ADDR>>1))};
static struct task_struct * elan_tp_fw_update_task = NULL;
static struct task_struct * elan_tp_2wireice_task = NULL;
static int    tpd_flag  = 0;
/* FW Info */
uint8_t FW_ID = { CMD_R_PKT, 0xF0, 0x00, 0x01 };
uint8_t FW_VER = { CMD_R_PKT, 0x00, 0x00, 0x01 };
uint8_t FW_X_Res = { CMD_R_PKT, 0x60, 0x00, 0x00 };
uint8_t FW_Y_Res = { CMD_R_PKT, 0x63, 0x00, 0x00 };
uint8_t FW_PW = { CMD_R_PKT, 0x50, 0x00, 0x01 };
uint8_t FW_PW_ON = { CMD_W_PKT, 0x54, 0x00, 0x01 };
uint8_t FW_PW_OFF = { CMD_W_PKT, 0x50, 0x00, 0x01 };

static int hello_packet_handler(struct i2c_client *client);
static int fw_packet_handler(struct i2c_client *client);
static int elan_ktf3k_ts_rough_calibrate(struct i2c_client *client);
//<2013/02/05-21580-kevincheng,Add TP attr control
static int e3135_set_power_state(struct i2c_client *client, int state);
static int e3135_get_power_state(struct i2c_client *client);
//>2013/02/05-21580-kevincheng
static int get_fw_version(struct i2c_client *client);
static int get_fw_x_res(struct i2c_client *client);
static int get_fw_y_res(struct i2c_client *client);
static int get_fw_id(struct i2c_client *client);

extern struct tpd_device  * tpd;
extern int    tpd_load_status;
static int e3135_recv_data(struct i2c_client *client, uint8_t *buf);
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern unsigned int mt65xx_eint_set_sens(unsigned int eint_num, unsigned int sens);
extern void mt65xx_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
extern void mt65xx_eint_registration(unsigned int eint_num, unsigned int is_deb_en, unsigned int pol, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);

  static uint8_t  file_fw_data[] =
    {
      #include "fw_data.i"
    };

static uint8_t file_bin_data[] = 
    {
      #include "2wireice.i"    
    };

static uint8_t bin_data_45[] =
    {
      #include "45_2wireice.i"
    };

static uint8_t bin_data_50[] =
    {
      #include "50_2wireice.i"
    };


static int tpd_i2c_write(struct i2c_client *client, const uint8_t *buf, int len)
{
	int i = 0;
        
	for(i = 0 ; i < len; i++)
	{
		I2CDMABuf_va[i] = buf[i];
	}

	if(len < 8)
	{
                client->addr = client->addr & I2C_MASK_FLAG | I2C_ENEXT_FLAG;
		return i2c_master_send(client, buf, len);
	}
	else
	{
                client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG | I2C_ENEXT_FLAG;
		return i2c_master_send(client, I2CDMABuf_pa, len);
	}    
}

static int tpd_i2c_read(struct i2c_client *client, uint8_t *buf, int len)
{
    int i = 0, ret = 0;
    
    if(len < 8)
    {
        client->addr = client->addr & I2C_MASK_FLAG | I2C_ENEXT_FLAG;
        return i2c_master_recv(client, buf, len);
    }
    else
    {
        client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG | I2C_ENEXT_FLAG;
        ret = i2c_master_recv(client, I2CDMABuf_pa, len);
    
        if(ret < 0)
        {
            return ret;
        }
    
        for(i = 0; i < len; i++)
        {
            buf[i] = I2CDMABuf_va[i];
        }
    }
    return ret;
}

/////  2WIREICE////////
// Start 2WireICE
int write_ice_status=0;
int shift_out_16(struct i2c_client *client){
	int res;
        uint8_t buff[] = {0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbb,0xbf,0xff};
	res = tpd_i2c_write(client, buff,  sizeof(buff));
	return res;
}
int tms_reset(struct i2c_client *client){
	int res;
	uint8_t buff[] = {0xee,0xee,0xea,0xe0};
	res = tpd_i2c_write(client, buff,  sizeof(buff));
	return res;
}

int mode_gen(struct i2c_client *client){
	int res;
	//uint8_t buff[] = {0xee,0xee,0xee,0x2a,0x6a,0x66,0xaa,0x66,0xa6,0xaa,0x66,0xae,0x2a,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xae};
	uint8_t buff[] = {0xee,0xee,0xee,0x20,0xa6,0xa6,0x6a,0xa6,0x6a,0x6a,0xa6,0x6a,0xe2,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xe0};
	uint8_t buff_1[] = {0x2a,0x6a,0xa6,0xa6,0x6e};
	char mode_buff[2]={0};
	res = tpd_i2c_write(client, buff,  sizeof(buff));
        res = tpd_i2c_read(client, mode_buff, sizeof(mode_buff));
	printk("[elan] mode_gen read: %x %x\n", mode_buff[0], mode_buff[1]);
	
	res = tpd_i2c_write(client, buff_1,  sizeof(buff_1));
	return res;
}

int word_scan_out(struct i2c_client *client){
	//printk("[elan] fun = %s\n", __func__);
	int res;
	uint8_t buff[] = {0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x26,0x66};
	res = tpd_i2c_write(client, buff,  sizeof(buff));
	return res;
}

int long_word_scan_out(struct i2c_client *client){
	//printk("[elan] fun = %s\n", __func__);
	int res;
	uint8_t buff[] = {0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x26,0x66};
	res = tpd_i2c_write(client, buff,  sizeof(buff));
	return res;
}


int bit_manipulation(int TDI, int TMS, int TCK, int TDO,int TDI_1, int TMS_1, int TCK_1, int TDO_1){
        int res; 
	res= ((TDI<<3 |TMS<<2 |TCK |TDO)<<4) |(TDI_1<<3 |TMS_1<<2 |TCK_1 |TDO_1);
	//printk("[elan] func=%s, res=%x\n", __func__, res);
	return res;
}

int ins_write(struct i2c_client *client, uint8_t buf){
	int res=0;
	int length=13;
	//int write_buf[7]={0};
	uint8_t write_buf[7]={0};
	int TDI_bit[13]={0};
	int TMS_bit[13]={0};
	int i=0;
	uint8_t buf_rev=0;
        int TDI=0, TMS=0, TCK=0,TDO=0;
	int bit_tdi, bit_tms;
	int len;
		
	for(i=0;i<8;i++) 
	{
	     buf_rev = buf_rev | (((buf >> i) & 0x01) << (7-i));
	}
		
	//printk( "[elan ]bit = %x, buf_rev = %x \n", buf, buf_rev); 
	
        TDI = (0x7<<10) | buf_rev <<2 |0x00;
        TMS = 0x1007;
	TCK=0x2;
	TDO=0;

	

	//printk( "[elan ]TDI = %p\n", TDI); //6F -> 111F600 (1FD8)
	//printk( "[elan ]TMS = %p\n", TMS); 
	
        for ( len=0; len<=length-1; len++){
		bit_tdi = TDI & 0x1;
		bit_tms = TMS & 0x1;
		//printk( "[elan ]bit_tdi = %d, bit_tms = %d\n", bit_tdi, bit_tms );
		TDI_bit[length-1-len] =bit_tdi;
		TMS_bit[length-1-len] = bit_tms;
                TDI = TDI >>1;
		TMS = TMS >>1;
	}


       /*for (len=0;len<=12;len++){
	   	printk("[elan] TDI[%d]=%d,  TMS[%d]= %d TCK=%d, TDO=%d, write[buf]=%x\n", len, TDI_bit[len], len, TMS_bit[len], TCK, TDO, (TDI_bit[len]<<3 |TMS_bit[len]<<2 |TCK |TDO));
		printk("[elan] %d, %d, %d, %d\n", TDI_bit[len] << 3 ,TMS_bit[len]<<2, TCK, TDO);
	}
        */
       /*
        for (len=0; len<=12;len=len+2){
		if (len !=12){
	            write_buf[len/2] =((TDI_bit[len]<<3 |TMS_bit[len]<<2|TCK |TDO)<<4) |((TDI_bit[len+1]<<3 |TMS_bit[len+1]<<2 |TCK |TDO));
 		    printk("[elan] write_buf[%d]=%x\n", len/2, write_buf[len/2]) ;
		} else {
		    write_buf[len/2] = ((TDI_bit[len]<<3 |TMS_bit[len]<<2 |TCK |TDO)<<4) |0x0000;	
	 	    printk("[elan] write_buf[%d]=%x\n", len/2, write_buf[len/2]) ;
		}
	}
        */
        for (len=0;len<=length-1;len=len+2){
	     if (len == length-1 && len%2 ==0)
		 res = bit_manipulation(TDI_bit[len], TMS_bit[len], TCK, TDO, 0, 0, 0, 0); 	
	     else
		 res = bit_manipulation(TDI_bit[len], TMS_bit[len], TCK, TDO, TDI_bit[len+1], TMS_bit[len+1], TCK, TDO); 	
	     write_buf[len/2] = res;
        }

/* for debug msg
       for(len=0;len<=(length-1)/2;len++){
		printk("[elan] write_buf[%d]=%x\n", len, write_buf[len]);
	}
*/
        res = tpd_i2c_write(client, write_buf,  sizeof(write_buf));
	return res;
}


int word_scan_in(struct i2c_client *client, uint16_t buf){
	int res=0;
	uint8_t write_buf[10]={0};
	int TDI_bit[20]={0};
	int TMS_bit[20]={0};
	
	
        int TDI =  buf <<2 |0x00;
        int  TMS = 0x7;
	int  TCK=0x2;
	int TDO=0;
	
	int bit_tdi, bit_tms;
	int len;
	//printk( "[elan] fun =%s,   %x\n", __func__,buf); 
	
	//printk("[elan] work_scan_in, buf=%x\n", buf);
	
	//printk( "[elan]TDI = %p\n", TDI); //0302 ->  (c08)
	//printk( "[elan]TMS = %p\n", TMS); //7
	
        for ( len=0; len<=19; len++){    //length =20
		bit_tdi = TDI & 0x1;
		bit_tms = TMS & 0x1;
		//printk( "[elan ]bit_tdi = %d, bit_tms = %d\n", bit_tdi, bit_tms );
		
		TDI_bit[19-len] =bit_tdi;
		TMS_bit[19-len] = bit_tms;
                TDI = TDI >>1;
		TMS = TMS >>1;
	}

/* for debug msg
       for (len=0;len<=19;len++){
	   	printk("[elan] TDI[%d]=%d,  TMS[%d]= %d TCK=%d, TDO=%d, write[buf]=%x\n", len, TDI_bit[len], len, TMS_bit[len], TCK, TDO, (TDI_bit[len]<<3 |TMS_bit[len]<<2 |TCK |TDO));
		printk("[elan] %d, %d, %d, %d\n", TDI_bit[len] << 3 ,TMS_bit[len]<<2, TCK, TDO);
	}
        
    
        for (len=0; len<=19;len=len+2){
	            write_buf[len/2] =((TDI_bit[len]<<3 |TMS_bit[len]<<2|TCK |TDO)<<4) |((TDI_bit[len+1]<<3 |TMS_bit[len+1]<<2 |TCK |TDO));
 		    printk("[elan] write_buf[%d]=%x\n", len/2, write_buf[len/2]) ;
	}
*/
        for (len=0;len<=19;len=len+2){
	     if (len == 19 && len%2 ==0)
	 	res = bit_manipulation(TDI_bit[len], TMS_bit[len], TCK, TDO, 0,0,0,0); 
	     else
                res = bit_manipulation(TDI_bit[len], TMS_bit[len], TCK, TDO, TDI_bit[len+1], TMS_bit[len+1], TCK, TDO); 
	        write_buf[len/2] = res;
        }

 /*/for debug msg
        for(len=0;len<=9;len++){
		printk("[elan] write_buf[%d]=%x\n", len, write_buf[len]);
	}
*/
        
	res = tpd_i2c_write(client, write_buf,  sizeof(write_buf));
	return res;
}

int long_word_scan_in(struct i2c_client *client, int buf_1, int buf_2){
       	uint8_t write_buf[18]={0};
	int TDI_bit[36]={0};
	int TMS_bit[36]={0};
	//printk( "[elan] fun =%s, %x,   %x\n", __func__,buf_1, buf_2); 
	
        int TDI =  buf_1 <<18|buf_2<<2 |0x00;
        int  TMS = 0x7;
	int  TCK=0x2;
	int TDO=0;
	
	int bit_tdi, bit_tms;
	int len;
	int res=0;
	
	//printk( "[elan]TDI = %p\n", TDI); //007e,0020 ->  (1f80080)
	//printk( "[elan]TMS = %p\n", TMS); //7

        for ( len=0; len<=35; len++){    //length =36
		bit_tdi = TDI & 0x1;
		bit_tms = TMS & 0x1;
		
		TDI_bit[35-len] =bit_tdi;
		TMS_bit[35-len] = bit_tms;
                TDI = TDI >>1;
		TMS = TMS >>1;
	}


        for (len=0;len<=35;len=len+2){
	     if (len == 35 && len%2 ==0)
	 	res = bit_manipulation(TDI_bit[len], TMS_bit[len], TCK, TDO, 0,0,0,0); 
	     else
                res = bit_manipulation(TDI_bit[len], TMS_bit[len], TCK, TDO, TDI_bit[len+1], TMS_bit[len+1], TCK, TDO); 
	     write_buf[len/2] = res;
        }
/* for debug msg
        for(len=0;len<=17;len++){
		printk("[elan] write_buf[%d]=%x\n", len, write_buf[len]);
	}
*/		
        res = tpd_i2c_write(client, write_buf,  sizeof(write_buf));
	return res;
}

uint16_t trimtable[8]={0};

/**************************************************
* Description: Read System Flash Control Register 
* 
*
***************************************************/

int Read_SFR(struct i2c_client *client, int open){
        uint8_t voltage_recv[2]={0};
	
	int count, ret;
	//uint16_t address_1[8]={0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007};
	        
	ins_write(client, 0x6f); // IO write: Change to IO space
	long_word_scan_in(client, 0x007e, 0x0020);	// Write data 0x0020 to address 0x007e
	long_word_scan_in(client, 0x007f, 0x4000);	//
	long_word_scan_in(client, 0x007e, 0x0023);
	long_word_scan_in(client, 0x007f, 0x8000);

        //  0
	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x9002); 	// Enable SFR register can be readed.
	ins_write(client, 0x68); 										// Change to memory space
	word_scan_in(client, 0x0000);								// Scan in the address
	shift_out_16(client); 								// Prepare the data for i2c_master_recv
		
	mdelay(10);
	count = 0;
	ret = tpd_i2c_read(client, voltage_recv, sizeof(voltage_recv)); 
	trimtable[count]=voltage_recv[0]<<8 | voltage_recv[1];
	//printk("[elan] Open_High_Voltage recv -1 0 word =%x %x, trimtable[%d]=%x \n", voltage_recv[0],voltage_recv[1], count, trimtable[count]); 

        //  1
	ins_write(client, 0x6f); // IO write
	long_word_scan_in(client, 0x007e, 0x0020);
	long_word_scan_in(client, 0x007f, 0x4000);
	long_word_scan_in(client, 0x007e, 0x0023);
	long_word_scan_in(client, 0x007f, 0x8000);

	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x9002);
	ins_write(client, 0x68);
	word_scan_in(client, 0x0001);
        shift_out_16(client); 

	mdelay(10);
	count=1;
        ret = tpd_i2c_read(client, voltage_recv, sizeof(voltage_recv)); 
        trimtable[count]=voltage_recv[0]<<8 | voltage_recv[1];
	//printk("[elan] Open_High_Voltage recv -1 1word =%x %x, trimtable[%d]=%x \n", voltage_recv[0],voltage_recv[1], count, trimtable[count]); 
	

        //  2
	ins_write(client, 0x6f); // IO write
	long_word_scan_in(client, 0x007e, 0x0020);
	long_word_scan_in(client, 0x007f, 0x4000);
	long_word_scan_in(client, 0x007e, 0x0023);
	long_word_scan_in(client, 0x007f, 0x8000);

	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x9002);
	ins_write(client, 0x68);
	word_scan_in(client, 0x0002);
        shift_out_16(client); 

	mdelay(10);
	count=2;
        ret = tpd_i2c_read(client, voltage_recv, sizeof(voltage_recv)); 
        trimtable[count]=voltage_recv[0]<<8 | voltage_recv[1];
	//printk("[elan] Open_High_Voltage recv -1 1word =%x %x, trimtable[%d]=%x \n", voltage_recv[0],voltage_recv[1], count, trimtable[count]); 
	

        //  3
	ins_write(client, 0x6f); // IO write
	long_word_scan_in(client, 0x007e, 0x0020);
	long_word_scan_in(client, 0x007f, 0x4000);
	long_word_scan_in(client, 0x007e, 0x0023);
	long_word_scan_in(client, 0x007f, 0x8000);

	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x9002);
	ins_write(client, 0x68);
	word_scan_in(client, 0x0003);
        shift_out_16(client); 

	mdelay(10);
	count=3;
        ret = tpd_i2c_read(client, voltage_recv, sizeof(voltage_recv)); 
        trimtable[count]=voltage_recv[0]<<8 | voltage_recv[1];
	//printk("[elan] Open_High_Voltage recv -1 1word =%x %x, trimtable[%d]=%x \n", voltage_recv[0],voltage_recv[1], count, trimtable[count]); 

        //  4
	ins_write(client, 0x6f); // IO write
	long_word_scan_in(client, 0x007e, 0x0020);
	long_word_scan_in(client, 0x007f, 0x4000);
	long_word_scan_in(client, 0x007e, 0x0023);
	long_word_scan_in(client, 0x007f, 0x8000);

	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x9002);
	ins_write(client, 0x68);
	word_scan_in(client, 0x0004);
        shift_out_16(client); 

	mdelay(10);
	count=4;
        ret = tpd_i2c_read(client, voltage_recv, sizeof(voltage_recv)); 
        trimtable[count]=voltage_recv[0]<<8 | voltage_recv[1];
	//printk("[elan] Open_High_Voltage recv -1 1word =%x %x, trimtable[%d]=%x \n", voltage_recv[0],voltage_recv[1], count, trimtable[count]); 
	


        //  5
	ins_write(client, 0x6f); // IO write
	long_word_scan_in(client, 0x007e, 0x0020);
	long_word_scan_in(client, 0x007f, 0x4000);
	long_word_scan_in(client, 0x007e, 0x0023);
	long_word_scan_in(client, 0x007f, 0x8000);

	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x9002);
	ins_write(client, 0x68);
	word_scan_in(client, 0x0005);
        shift_out_16(client); 

	mdelay(10);
	count=5;
        ret = tpd_i2c_read(client, voltage_recv, sizeof(voltage_recv)); 
        trimtable[count]=voltage_recv[0]<<8 | voltage_recv[1];
	//printk("[elan] Open_High_Voltage recv -1 1word =%x %x, trimtable[%d]=%x \n", voltage_recv[0],voltage_recv[1], count, trimtable[count]); 
	
	
        //  6
	ins_write(client, 0x6f); // IO write
	long_word_scan_in(client, 0x007e, 0x0020);
	long_word_scan_in(client, 0x007f, 0x4000);
	long_word_scan_in(client, 0x007e, 0x0023);
	long_word_scan_in(client, 0x007f, 0x8000);

	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x9002);
	ins_write(client, 0x68);
	word_scan_in(client, 0x0006);
        shift_out_16(client); 

	mdelay(10);
	count=6;
        ret = tpd_i2c_read(client, voltage_recv, sizeof(voltage_recv)); 
        trimtable[count]=voltage_recv[0]<<8 | voltage_recv[1];
	//printk("[elan] Open_High_Voltage recv -1 1word =%x %x, trimtable[%d]=%x \n", voltage_recv[0],voltage_recv[1], count, trimtable[count]); 
	

	//  7
	ins_write(client, 0x6f); // IO write
	long_word_scan_in(client, 0x007e, 0x0020);
	long_word_scan_in(client, 0x007f, 0x4000);
	long_word_scan_in(client, 0x007e, 0x0023);
	long_word_scan_in(client, 0x007f, 0x8000);

	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x9002);
	ins_write(client, 0x68);
	word_scan_in(client, 0x0007);
        shift_out_16(client); 

	mdelay(10);
	count=7;
        ret = tpd_i2c_read(client, voltage_recv, sizeof(voltage_recv)); 
	printk("open= %d\n", open);
	if (open == 1)
            trimtable[count]=voltage_recv[0]<<8 |  (voltage_recv[1] & 0xbf);
	else
            trimtable[count]=voltage_recv[0]<<8 | (voltage_recv[1] | 0x40);
	printk("[elan] Open_High_Voltage recv -1 1word =%x %x, trimtable[%d]=%x \n", voltage_recv[0],voltage_recv[1], count, trimtable[count]); 


        ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x8000);


	 
/*	// can not used for loop to read the SFR data.
	for (count =0; count <8; count++){

	ins_write(client, 0x6f); // IO write
	long_word_scan_in(client, 0x007e, 0x0020);
	long_word_scan_in(client, 0x007f, 0x4000);
	long_word_scan_in(client, 0x007e, 0x0023);
	long_word_scan_in(client, 0x007f, 0x8000);

	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x9002);
	ins_write(client, 0x68);
	word_scan_in(client, address_1[count]);
        shift_out_16(client); 

	mdelay(10);
	//count=6;
        ret = i2c_master_recv(client, voltage_recv, sizeof(voltage_recv)); 
        trimtable[count]=voltage_recv[0]<<8 | voltage_recv[1];
	printk("[elan] Open_High_Voltage recv -1 1word =%x %x, trimtable[%d]=%x \n", voltage_recv[0],voltage_recv[1], count, trimtable[count]); 

	}
	
	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007f, 0x8000);

*/


	
	return 0;
}

int Write_SFR(struct i2c_client *client){


       //write_sfr
       ins_write(client, 0x6f);
       long_word_scan_in(client, 0x007f, 0x9001);


       ins_write(client, 0x66);
       long_word_scan_in(client, 0x0000, trimtable[0]);
       ins_write(client, 0xfd);
       word_scan_in(client,0x7f);
	
       ins_write(client, 0x66);
       long_word_scan_in(client, 0x0001, trimtable[1]);
       ins_write(client, 0xfd);
       word_scan_in(client,0x7f);

       ins_write(client, 0x66);
       long_word_scan_in(client, 0x0002, trimtable[2]);
       ins_write(client, 0xfd);
       word_scan_in(client,0x7f);

       ins_write(client, 0x66);
       long_word_scan_in(client, 0x0003, trimtable[3]);
       ins_write(client, 0xfd);
       word_scan_in(client,0x7f);

       ins_write(client, 0x66);
       long_word_scan_in(client, 0x0004, trimtable[4]);
       ins_write(client, 0xfd);
       word_scan_in(client,0x7f);

       ins_write(client, 0x66);
       long_word_scan_in(client, 0x0005, trimtable[5]);
       ins_write(client, 0xfd);
       word_scan_in(client,0x7f);
	   
       ins_write(client, 0x66);
       long_word_scan_in(client, 0x0006, trimtable[6]);	
       ins_write(client, 0xfd);
       word_scan_in(client,0x7f);

       ins_write(client, 0x66);
       long_word_scan_in(client, 0x0007, trimtable[7]);
       ins_write(client, 0xfd);
       word_scan_in(client,0x7f);


       ins_write(client, 0x6f);
       long_word_scan_in(client, 0x7f, 0x8000);	   
       /*
       for (count=0;count<8;count++){
              ins_write(client, 0x66);
	      long_word_scan_in(client, 0x0000+count, trimtable[count]);
		
       }
	*/

	return 0;
}

int Enter_Mode(struct i2c_client *client){
	mode_gen(client);
	tms_reset(client);
        msleep(1500);
	ins_write(client,0xfc); //system reset
	tms_reset(client);
	return 0;
}
int Open_High_Voltage(struct i2c_client *client, int open){
	Read_SFR(client, open);
	Write_SFR(client);
        Read_SFR(client, open);
	return 0;
}

int Mass_Erase(struct i2c_client *client){
	char mass_buff[4]={0};
	char mass_buff_1[2]={0};
	int ret, finish=0, i=0;
	printk("[Elan] Mass_Erase!!!!\n");
        
	ins_write(client,0x01); //id code read
	mdelay(2);
	long_word_scan_out(client);

	ret = tpd_i2c_read(client, mass_buff, sizeof(mass_buff));
	printk("[elan] Mass_Erase mass_buff=%x %x %x %x(c0 08 01 00)\n", mass_buff[0],mass_buff[1],mass_buff[2],mass_buff[3]);  //id: c0 08 01 00
	
/* / add for test
	ins_write(client, 0xf3);
        word_scan_out(client);
        ret = i2c_master_recv(client, mass_buff_1, sizeof(mass_buff_1));
	printk("[elan] Mass_Erase mass_buff_1=%x %x(a0 00)\n", mass_buff_1[0],mass_buff_1[1]);  // a0 00 : stop
//add for test

	//read low->high 5th bit
	ins_write(client, 0x6f);
	long_word_scan_in(client, 0x007e, 0x0020);
	long_word_scan_in(client, 0x007f, 0x4000);

// add for test
	ins_write(client, 0xf3);
        word_scan_out(client);
        ret = i2c_master_recv(client, mass_buff_1, sizeof(mass_buff_1));
	printk("[elan] Mass_Erase (II) mass_buff_1=%x %x(40 00)\n", mass_buff_1[0],mass_buff_1[1]);  // 40 00
//add for test
	mdelay(10); //for malata
	*/
	
	ins_write(client,0x6f);
	
	/*add 0121 start*/
	long_word_scan_in(client,0x007e,0x0020);
	long_word_scan_in(client,0x007f,0x4000);
	/*add 0121 end*/
	
	long_word_scan_in(client,0x007e,0x0023);
	long_word_scan_in(client,0x007f,0x8000);
	long_word_scan_in(client,0x007f,0x9040);
	ins_write(client,0x66);
	long_word_scan_in(client, 0x0000,0x8765);
	ins_write(client,0x6f);
	long_word_scan_in(client, 0x007f,0x8000);	

        ins_write(client,0xf3);
        
	while (finish==0){
	    word_scan_out(client);
	    ret=tpd_i2c_read(client, mass_buff_1, sizeof(mass_buff_1));
	    finish = (mass_buff_1[1] >> 4 ) & 0x01;

	    printk("[elan] mass_buff_1[0]=%x, mass_buff_1[1]=%x (80 10)!!!!!!!!!! finish=%d \n", mass_buff_1[0], mass_buff_1[1], finish); //80 10: OK, 80 00: fail
	    if (mass_buff_1[1]!= 0x10 && finish!=1 && i<100) {  
			mdelay(100);
			//printk("[elan] mass_buff_1[1] >>4  !=1\n");
			i++;
			if (i == 50) {
                                printk("[elan] Mass_Erase fail ! \n");
				//return -1;  //for test
			}
	    }
	    
	}

	return 0;
}

int Reset_ICE(struct i2c_client *client){
        //struct elan_ktf3k_ts_data *ts = i2c_get_clientdata(client);
        int res;
        printk(">> [Elan] Reset ICE!!!!\n");
	ins_write(client, 0x94);
	ins_write(client, 0xd4);
	ins_write(client, 0x20);
	//elan_ktf3k_ts_hw_reset(private_ts->client);  //???

	mt_set_gpio_out(GPIO_CTP_RST_PIN,0);
	mdelay(20);
	mt_set_gpio_out(GPIO_CTP_RST_PIN,1);
	mdelay(1500);
        printk(">> Reset ICE , client->addr : 0x%x , change to 0x10\r\n",client->addr);
        client->addr = 0x10;
	res = hello_packet_handler(client);
        mdelay(300);
        res = fw_packet_handler(client);
	
	return 0;
}

int normal_write_func(struct i2c_client *client, int j, uint8_t *szBuff){
	//char buff_check=0;
	uint16_t szbuff=0, szbuff_1=0;
	uint16_t sendbuff=0;
        int write_byte, iw;
	
	ins_write(client,0xfd);
        word_scan_in(client, j*64); 
        
	ins_write(client,0x65);

        write_byte =64;

        for(iw=0;iw<write_byte;iw++){ 
		szbuff = *szBuff;
		szbuff_1 = *(szBuff+1);
		sendbuff = szbuff_1 <<8 |szbuff;
		printk("[elan]  Write Page sendbuff=0x%04x @@@\n", sendbuff);
		//mdelay(1);
		word_scan_in(client, sendbuff); //data????   buff_read_data
		szBuff+=2;
		
	}
        return 0;
}

int fastmode_write_func(struct i2c_client *client, int j, uint8_t *szBuff){
	 uint8_t szfwbuff=0, szfwbuff_1=0;
	 uint8_t sendfwbuff[130]={0};
	 uint16_t tmpbuff;
	 int i=0, len=0;
	 i2c_client->addr = 0x76;

	 sendfwbuff[0] = (j*64)>>8;
	 tmpbuff = ((j*64)<< 8) >> 8;
	 sendfwbuff[1] = tmpbuff;
	 //printk("fastmode_write_func, sendfwbuff[0]=0x%x, sendfwbuff[1]=0x%x\n", sendfwbuff[0], sendfwbuff[1]);
	 
	 for (i=2;i < 129; i=i+2) {      //  1 Page = 64 word, 1 word=2Byte
	 
	     szfwbuff = *szBuff;
	     szfwbuff_1 = *(szBuff+1);
	     sendfwbuff[i] = szfwbuff_1;
	     sendfwbuff[i+1] = szfwbuff;
	     szBuff+=2;
	     //printk("[elan] sendfwbuff[%d]=0x%x, sendfwbuff[%d]=0x%x\n", i, sendfwbuff[i], i+1, sendfwbuff[i+1]);
	 }

	 
	 len = tpd_i2c_write(i2c_client, sendfwbuff,  sizeof(sendfwbuff));
	 //printk("fastmode_write_func, send len=%d (130), Page %d --\n", len, j);

	  i2c_client->addr = 0x77;
	  
	return 0;
}


int ektSize;
int lastpage_byte;
int lastpage_flag=0;
int Write_Page(struct i2c_client *client, int j, uint8_t *szBuff){
	int len, finish=0;
	char buff_read_data[2];
	int i=0;
	
	ins_write(client,0x6f);
	/*add 0121 start*/
	//long_word_scan_in(client,0x007e,0x0023);
	long_word_scan_in(client,0x007f,0x8000);
	/*add 0121 end*/
	
	long_word_scan_in(client,0x007f,0x9400);

	ins_write(client,0x66);
	//long_word_scan_in(client,0x0000,0x5a5a);
	//printk("[elan] j*64=0x%x @@ \n", j*64);
	
	/*add 0121 start*/
	//long_word_scan_in(client, j*64,0x5a5a);
	long_word_scan_in(client, j*64,0x0000);
	/*add 0121 end*/
		
        //normal_write_func(client, j, szBuff); ////////////choose one : normal / fast mode
        fastmode_write_func(client, j, szBuff); //////////
        
	ins_write(client,0x6f);
	long_word_scan_in(client,0x007f,0x9000);

	//ins_write(client,0x6f);
	long_word_scan_in(client,0x007f,0x8000);

	ins_write(client, 0xf3);
	
	while (finish==0){
	    word_scan_out(client);
	    len=tpd_i2c_read(client, buff_read_data, sizeof(buff_read_data));
	    finish = (buff_read_data[1] >> 4 ) & 0x01;
	    //printk("[elan] Write_Page:buff_read_data[0]=%x, buff_read_data[1]=%x !!!!!!!!!! finish=%d \n", buff_read_data[0], buff_read_data[1], finish); //80 10: ok
	    if (finish!=1) {  
			mdelay(10);
			printk("[elan] Write_Page finish !=1\n");
                        i++;
			if (i==50){ 
                                write_ice_status=1;
				return -1;
			}
	    }

	}
	return 0;
}

int fastmode_read_func(struct i2c_client *client, int j, uint8_t *szBuff){
	 uint8_t szfrbuff=0, szfrbuff_1=0;
	 uint8_t sendfrbuff[2]={0};
	 uint8_t recvfrbuff[130]={0};
	 uint16_t tmpbuff;
	 int i=0, len=0, retry=0;
	 /*add 0121 start*/
	 //ins_write(client,0x67);
	 /*add 0121 end*/
	 
	 i2c_client->addr = 0x76;

	 sendfrbuff[0] = (j*64)>>8;
	 tmpbuff = ((j*64)<< 8) >> 8;
	 sendfrbuff[1] = tmpbuff;
	 //printk("fastmode_write_func, sendfrbuff[0]=0x%x, sendfrbuff[1]=0x%x\n", sendfrbuff[0], sendfrbuff[1]);
	 len = tpd_i2c_write(i2c_client, sendfrbuff,  sizeof(sendfrbuff));

	 len = tpd_i2c_read(i2c_client, recvfrbuff,  sizeof(recvfrbuff));
	 //printk("fastmode_read_func, recv len=%d (128)\n", len);
		 
         for (i=2;i < 129;i=i+2){ 
		szfrbuff=*szBuff;
	        szfrbuff_1=*(szBuff+1);
	        szBuff+=2;
		if (recvfrbuff[i] != szfrbuff_1 || recvfrbuff[i+1] != szfrbuff)  
	        {
		  	 printk("[elan] @@@@Read Page Compare Fail. recvfrbuff[%d]=%x, recvfrbuff[i+1]=%x, szfrbuff_1=%x, szfrbuff=%x, ,j =%d@@@@@@@@@@@@@@@@\n\n", i,recvfrbuff[i], recvfrbuff[i+1], szfrbuff_1, szfrbuff, j);
		  	 write_ice_status=1;
		  	 retry=1;
		}
         }

	  i2c_client->addr = 0x77;
	  
	  if(retry==1){
	     return -1;
	  }
	  
	return 0;
}


int normal_read_func(struct i2c_client *client, int j,  uint8_t *szBuff){
        char read_buff[2];
        int m, len, read_byte;
	uint16_t szbuff=0, szbuff_1=0;
	
	ins_write(client,0xfd);
	
	//printk("[elan] Read_Page, j*64=0x%x\n", j*64);
	word_scan_in(client, j*64);
	ins_write(client,0x67);

	word_scan_out(client);

        read_byte=64;
	//for(m=0;m<64;m++){
	for(m=0;m<read_byte;m++){
            // compare......
                word_scan_out(client);
	        len=tpd_i2c_read(client, read_buff, sizeof(read_buff));

		szbuff=*szBuff;
	        szbuff_1=*(szBuff+1);
	        szBuff+=2;
	        printk("[elan] Read Page: byte=%x%x, szbuff=%x%x \n", read_buff[0], read_buff[1],szbuff, szbuff_1);
	        if (read_buff[0] != szbuff_1 || read_buff[1] != szbuff) 
	        {
		  	 printk("[elan] @@@@@@@@@@Read Page Compare Fail. j =%d. m=%d.@@@@@@@@@@@@@@@@\n\n", j, m);
		  	 write_ice_status=1;
		}
     }
     return 0;
}


int Read_Page(struct i2c_client *client, int j,  uint8_t *szBuff){

        int res = 0;	
	ins_write(client,0x6f);
	/*add 0121 start*/
	//long_word_scan_in(client,0x007e,0x0023);
	/*add 0121 end*/
	
	long_word_scan_in(client,0x007f,0x9000);
	
	/*add 0121 start*/
  ins_write(client,0x68);
  /*add 0121 end*/
  
        //mdelay(10); //for malata
	//normal_read_func(client, j,  szBuff); ////////////////choose one: normal / fastmode
	
	/*add 0121 start*/
	//fastmode_read_func(client, j,  szBuff);
	res=fastmode_read_func(client, j,  szBuff);
  /*add 0121 end*/

	//Clear Flashce
	ins_write(client,0x6f);
	
	/*add 0121 start*/
	//long_word_scan_in(client,0x007f,0x8000);
	long_word_scan_in(client,0x007f,0x0000);
	
	
	if (res==-1){
	    return -1;
	}
	/*add 0121 end*/
	
	return 0;
        
}

int TWO_WIRE_ICE(struct i2c_client *client){
     int i;
     
     //test	 
     uint8_t *szBuff = NULL;
     int curIndex = 0;
     int PageSize=128;
     int res,retry;

     //int ektSize;
     //test
//<2013/05/02-24538-kevincheng,Solve TP FW Update Bug
     panel_size = 50;
//>2013/05/02-24538-kevincheng
     write_ice_status=0;
     if(panel_size == 50)	 
         ektSize = sizeof(bin_data_50) /PageSize;
     else
	  ektSize = sizeof(bin_data_45) /PageSize;
     client = i2c_client;
     
     printk(">> [Elan] ektSize=%d \n ", ektSize);
     //test	
     i = Enter_Mode(client);
     i = Open_High_Voltage(client, 1);     
     if (i == -1) return -1; //test
    //return 0;
	
     i =Mass_Erase(client);  //mark temp
     if (i == -1) return -1; //test

     //return 0;  // for test

     //for fastmode
     ins_write(client,0x6f);
     long_word_scan_in(client, 0x7e, 0x36);
     long_word_scan_in(client, 0x7f, 0x8000);	
     
     /*add 0121 start*/
     //long_word_scan_in(client, 0x7e, 0x37);	 
     //long_word_scan_in(client, 0x7f, 0x76);
     long_word_scan_in(client, 0x007e, 0x0023);
     /*add 0121 end*/
	 
     //for fastmode
     for (i =0 ; i<ektSize; i++){
	 if(panel_size == 50)
	     szBuff = bin_data_50 + curIndex;
	 else
	     szBuff = bin_data_45 + curIndex;
        curIndex =  curIndex + PageSize; 
        /*add 0121 start*/
        retry=0;	
        /*add 0121 end*/
	//printk("[Elan] Write_Page %d........................wait\n ", i);	

        res=Write_Page(client, i, szBuff);
	//if (res == -1) break;
	//printk("[Elan] Read_Page %d........................wait\n ", i);
	mdelay(3);
        res=Read_Page(client,i, szBuff);
	//printk("[Elan] Finish  %d  Page!!!!!!!.........wait\n ", i);	
     }
     if(write_ice_status==0)
     {
     	printk("[elan] Update_FW_Boot Finish!!! \n");
     }
     else
     {
     	printk("[elan] Update_FW_Boot fail!!! \n");
     }

     i = Open_High_Voltage(client, 0);     
     if (i == -1) return -1; //test
     
     Reset_ICE(client);

     return 0;	
}

// End 2WireICE

static int EnterISPMode(struct i2c_client *client, uint8_t *isp_cmd, int cmd_len)
{
  int   len = 0, vRet = 0;
  char    buf[PACKET_SIZE] = { 0x00 };
  
  printk(">> %s <<\n",__func__);
  len = tpd_i2c_write( i2c_client, isp_cmd, cmd_len );
  if( len != cmd_len )
  {
    printk( ">> [ELAN]%s fail! len=%d", __func__, len );
    vRet = -1;
  }
  else 
    e3135_recv_data( i2c_client, buf );
    printk( ">> [ELAN]IAPMode write data successfully!" );

  return  vRet;
} 

static int WritePage(uint8_t * szPage, int byte)
{
int   len = 0, vRet = 0;

  len = tpd_i2c_write( i2c_client, szPage, byte );
  if( len != byte )
  {
    printk( ">> [ELAN]%s: Write page error, write error. err=%d", __func__, len );
  }

  return  vRet;
} 

static int GetAckData(struct i2c_client *client)
{
int   len = 0, vRet = 0;
char  buff[2] = { 0x00 };

  len = tpd_i2c_read( i2c_client, buff, sizeof( buff ));
  if( len != sizeof( buff ))
  {
    printk( ">>[ELAN]%s: Read data error, write 50 times error. len=%d\r", __func__, len );
  }

  if( buff[0] == 0xAA )
  {
    vRet = ACK_OK;
  }
  else if( buff[0] == 0x55 )
  {
    printk( ">>[ELAN]%s rewrite: 0x%02X,0x%02X", __func__, buff[0], buff[1] );
    vRet = ACK_REWRITE;
  }
  else
  {
    printk( ">>[ELAN]%s fail: 0x%02X,0x%02X", __func__, buff[0], buff[1] );
    vRet = ACK_FAIL;
  }

  return  vRet;
} /** End.. GetAckData() **/

static void print_progress(int page, int ic_num, int j)
{
int   i = 0, percent = 0, page_tatol = 0, percent_tatol = 0;
char  str[256] = { 0 };

  str[0] = '\0';
  for( i = 0; i < (( page ) / 10 ); i++ )
  {
    str[i] = '#';
    str[i + 1] = '\0';
  }

  page_tatol    = page + 249 * ( ic_num - j );
  percent       = (( 100 * page ) / ( 249 ));
  percent_tatol = (( 100 * page_tatol ) / ( 249 * ic_num ));

  if(( page ) == ( 249 ))
    percent = 100;

  if(( page_tatol ) == ( 249 * ic_num ))
    percent_tatol = 100;

  printk( "\rprogress %s| %d%%", str, percent );
  if( page == ( 249 ))
    printk( "\n" );
} /** End.. print_progress() **/

static int Update_FW(void)
{
int     res = 0;
int     iPage = 0, ackcnt = 0;
int     i = 0;
int     byte_count = 0;
int     curIndex = 0;
uint8_t isp_cmd[] = { 0x45, 0x49, 0x41, 0x50 };
uint8_t data = 0;
uint8_t * szBuff = NULL;
  
  EnterISPMode( i2c_client, isp_cmd, 4 );

write_fw:
/* IAP step 2: send dummy byte */
  res = tpd_i2c_write( i2c_client, &data, sizeof( data ));
  if( res != sizeof( data ))
  {
    printk( ">>[ELAN] Dummy byte %d\n",res );
  }
  else
  {
    printk( ">>[ELAN] Send dummy byte fail %x, res = %d\n", data, res );
  }
  printk( ">>[ELAN]: Start 248 page update procedure\n" );

PAGE_REWRITE:
  for( iPage = 1; iPage <= Page_Num; iPage++ )
  {
    for( byte_count = 1; byte_count <= 17; byte_count++ )
    {
      if( byte_count != 17 )
      {
        szBuff = file_fw_data + curIndex;
        curIndex = curIndex + 8;
        res = WritePage( szBuff, 8 );
      }
      else
      {
        szBuff = file_fw_data + curIndex;
        curIndex = curIndex + 4;

        res = WritePage( szBuff, 4 );
      }
      mdelay( 1 );
    }
    //mdelay( 50 );

      res = GetAckData( i2c_client );
 
    if( ACK_OK != res )
    {
      printk( ">>[ELAN] ERROR: GetAckData fail! res=%d , page : %d\n", res, iPage);
      ackcnt = ackcnt + 1;
      if( ackcnt == PAGERETRY )
      {
        printk( ">>[ELAN] ID 0x%02X %dth page ReWrite %d times fails!\n", data, iPage, PAGERETRY );
        return E_FD;
      }
      else
      {
        printk( ">>[ELAN] ---%d--- page ReWrite %d times!\n", iPage, ackcnt );
        goto PAGE_REWRITE;
      }
    }
    else
    	{
    	  printk( ">>[ELAN]  update ipage %d sccussful !\n", iPage);
         ackcnt = 0;
    	 }
  } 

  printk( ">> [ELAN] update Firmware successfully!!!\n" );

  return  0;
} /* End.. Update_FW() */

static int elan_fw_update_kthread(void)
{
struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };

  sched_setscheduler( current, SCHED_RR, &param );
  
  // disable_irq(CUST_EINT_TOUCH_PANEL_NUM);
    IAP_PW_Lock = On;
    mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO );
    msleep(1);
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
    msleep(1);
    printk(">> kthread update \n");
  
    Update_FW();

    msleep(2000);

    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    
    printk(">> FW Re-check\r\n");
    fw_packet_handler( i2c_client );
    
    IAP_PW_Lock = Off;	
	
  if( kthread_should_stop())
  {
    printk(">> elan tp fw update kthread stop\n"   );
    //break;
  }
  
  return 0;
} 

static int elan_2wireice_kthread(void)
{
  struct sched_param param = { .sched_priority = RTPM_PRIO_SCRN_UPDATE };

  printk(">> 2wireice kthread update \n");
  sched_setscheduler( current, SCHED_RR, &param );

  IAP_PW_Lock = On;
  mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
  i2c_client->addr = 0x77;
  TWO_WIRE_ICE(i2c_client);
  mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
  IAP_PW_Lock = Off;	
	
  if( kthread_should_stop())
  {
    printk(">> elan 2wireice kthread stop\n"   );
    //break;
  }
  
  return 0;
} 

/**************   IAP  ****************/
int elan_iap_open(struct inode *inode, struct file *file)
{ 
   printk(">>  Enter IAP Open <<\n");

   return nonseekable_open(inode, file);
}

int elan_iap_release(struct inode *inode, struct file *filp)
{    
   printk(">>  Enter IAP Open <<\n");
   return 0;
}

ssize_t elan_iap_write(struct file *filp, const char *buff,    size_t count, loff_t *offp)
{  
    int ret;
    char *tmp;
    printk(">> Entry IAP Write <<\n");

    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);
    
    if (tmp == NULL)
        return -ENOMEM;

    if (copy_from_user(tmp, buff, count)) {
        return -EFAULT;
    }

    ret = tpd_i2c_write(i2c_client, tmp, count);
    if (ret != count) printk(">> ELAN i2c_master_send fail, ret=%d \n", ret);
    kfree(tmp);
    return ret;

}

ssize_t elan_iap_read(struct file *filp, char *buff,    size_t count, loff_t *offp)
{    
    char *tmp;
    int ret;  
    long rc;
    printk(">> Entry IAP Read\n");
   
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);

    if (tmp == NULL)
        return -ENOMEM;

    ret = tpd_i2c_read(i2c_client, tmp, count);

    if (ret >= 0)
        rc = copy_to_user(buff, tmp, count);
    
    kfree(tmp);
	
    return ret;
}

static void tpd_FW_Update(void)
{
    elan_tp_fw_update_task = kthread_create( elan_fw_update_kthread , NULL, "elan_fw_update_kthread" );
    wake_up_process( elan_tp_fw_update_task );
}

static void tpd_2wireice(void)
{
    elan_tp_2wireice_task = kthread_create( elan_2wireice_kthread , NULL, "elan_fw_update_kthread" );
    wake_up_process( elan_tp_2wireice_task );
}

static int e3135_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int rc;
	char strbuf[E3135_BUFSIZE];
       void __user *data;
	printk("==== [ELAN]into elan_iap_ioctl : %d\n",cmd);

		switch (cmd) {        
		case IOCTL_I2C_SLAVE:
			 printk(">>  Elan-IOCTL , I2C_Slave\n");
                         i2c_client->addr = (int __user)arg;
			 break;   
		case IOCTL_MAJOR_FW_VER:
			 printk(">>  Elan-IOCTL , Major_FW_Ver\n");
			 break;        
		case IOCTL_MINOR_FW_VER:            
			 printk(">>  Elan-IOCTL , Minor_FW_Ver\n");
			 break;        
		case IOCTL_RESET:
			 printk(">>  Elan-IOCTL , Reset\n");
                         mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
                         mdelay(500);
                         mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
                         //mdelay(500);
			 break;
		case IOCTL_IAP_MODE_LOCK:
			 printk(">>  Elan-IOCTL , Lock\n");
			 mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
			 break;
		case IOCTL_IAP_MODE_UNLOCK:
                         printk(">>  Elan-IOCTL , Unlock\n");
			 mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
			 break;
		case IOCTL_CHECK_RECOVERY_MODE:
			 printk(">>  Elan-IOCTL , Recovery Mode\n");
			 break;
		case IOCTL_FW_VER:
			 printk(">>  Elan-IOCTL , FW_Verison\n");
                         get_fw_version(i2c_client);
			 return CTP_VER;
			 break;
		case IOCTL_X_RESOLUTION:
			 printk(">>  Elan-IOCTL , X_Resolution\n");
                         get_fw_x_res(i2c_client);
			 return CTP_X_Res;
			 break;
		case IOCTL_Y_RESOLUTION:
			 printk(">>  Elan-IOCTL , Y_Resolution\n");
                         get_fw_y_res(i2c_client);
			 return CTP_Y_Res;
			 break;
		case IOCTL_FW_ID:
			 printk(">>  Elan-IOCTL , FW_ID\n");
                         get_fw_id(i2c_client);
			 return CTP_ID;
			 break;
		case IOCTL_I2C_INT:
			 printk(">>  Elan-IOCTL , I2C_INT\n"); 
			 //put_user(gpio_get_value(GPIO_CTP_EINT_PIN), ip);
			 break;	
		case IOCTL_RESUME:
			 //tpd_resume(i2c_client);
			 break;
		case IOCTL_ROUGH_CALIBRATE:
			 printk(">> Elan-IOCTL , Rough Calibration\n");
			 elan_ktf3k_ts_rough_calibrate(i2c_client); 
			 break;
                case IOCTL_POWER_LOCK:
			 IAP_PW_Lock = On;
			 break;
		case IOCTL_POWER_UNLOCK:
			 IAP_PW_Lock = Off;
			 break;	
                case IOCTL_FW_UPDATE:
                         printk(">> ioctl FW_Update\n");
                         tpd_FW_Update();
                         return 0;
                         break;
                case IOCTL_2WIREICE:
                         printk(">> ioctl 2wireice, panel_size : %s\n",panel_size == 50 ? "5inch" : "4.5inch");
                         IAP_PW_Lock = On;
                         mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
			 i2c_client->addr = 0x77;
                         TWO_WIRE_ICE(i2c_client);
			 mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
                         IAP_PW_Lock = Off;
                         return 0;
                         break;
		case IOCTL_FW_Info:
                      printk(">> ioctl FW_Info\n");
                      data = (void __user *) arg;
			 if(data == NULL)
			 {
				printk(">> Warry!!!! , ioctl data is NULL\n");
				break;	  
			 }
			 sprintf(strbuf, "FW_Version : 0x%x , FW_Id : 0x%04x , X_Resolution : %d , Y_Resolution : %d", CTP_VER,CTP_ID,CTP_X_Res,CTP_Y_Res);
			 printk(">> Ioctl FW Info check then return , %s\n",strbuf);
			 if(copy_to_user(data, strbuf, strlen(strbuf)+1))
			 {
				printk(">> copy_to_user fail\n");
				break;	  
			 }				 
                      break;
		default:            
			break;   
	}       
	return 0;
}

struct file_operations elan_touch_fops = 
{    
       .open =         elan_iap_open,    
       .write =         elan_iap_write,    
       .read =          elan_iap_read,    
       .release =      elan_iap_release,    
       //.ioctl =          elan_iap_ioctl,  
       .unlocked_ioctl = e3135_unlocked_ioctl,
 };

static struct miscdevice  IAP_device =
{
      .minor  = MISC_DYNAMIC_MINOR,
      .name   = "elan-iap",
      .fops   = &elan_touch_fops,
      .mode   = S_IRWXUGO,
};
/**************   IAP  ****************/

//<2013/02/05-21580-kevincheng,Add TP attr control
static ssize_t e3135_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
   int     rc = 0, retry = 5;

   printk(">> %s\n",__func__);

   do{
      rc = e3135_set_power_state( i2c_client, PWR_STATE_NORMAL );
      msleep( 10 );
      rc = e3135_get_power_state( i2c_client );
      if( rc != PWR_STATE_NORMAL )
            printk(">> [ELAN]%s: wake up tp failed! err = %d\n", __func__, rc );

            retry -= 1;
         } while(( retry >= 0 ) && ( rc != PWR_STATE_NORMAL ));

         mt65xx_eint_unmask( CUST_EINT_TOUCH_PANEL_NUM );

   return 0;
}

static ssize_t e3135_off_show(struct device *dev, struct device_attribute *attr, char *buf)
{
   int rc = 0;

   printk(">> %s\n",__func__);

   if(!IAP_PW_Lock){
       mt65xx_eint_mask( CUST_EINT_TOUCH_PANEL_NUM );

       rc = e3135_set_power_state( i2c_client, PWR_STATE_DEEP_SLEEP );
       rc = e3135_get_power_state( i2c_client );
      }

   return 0;
}

static DEVICE_ATTR(e3135_on, S_IWUSR|S_IRUGO,e3135_on_show,NULL);
static DEVICE_ATTR(e3135_off, S_IWUSR|S_IRUGO,e3135_off_show,NULL);

static struct attribute *e3135_attributes[] =
{
    &dev_attr_e3135_on.attr,
    &dev_attr_e3135_off.attr,
    NULL,
};

static struct attribute_group e3135_attr_group =
{
    .attrs = e3135_attributes,
};
//>2013/02/05-21580-kevincheng

static int e3135_detect (struct i2c_client *client, struct i2c_board_info *info) 
 {
         printk(">> Elan %s\n",__FUNCTION__);
	 strcpy(info->type, e3135_NAME);	
	  return 0;
 }

static void e3135_parse_xy(uint8_t *data, int16_t *x, int16_t *y)
{
int16_t  tempH = 0, tempL = 0;
int16_t  tpdX = 0, tpdY = 0;

  *x = 0;
  *y = 0;
  tempH = (int16_t)( *data );

  tempL = (int16_t)( *( data + 1 ));
  tpdX  = (( tempH << 4 ) & 0x0F00 ) | ( tempL & 0x00FF );
  tempL = (int16_t)( *( data + 2 ));
  tpdY  = (( tempH << 8 ) & 0x0F00 ) | ( tempL & 0x00FF );
  
  *x = tpdX * ( TPD_RES_X - 1 ) / CTP_X_Res;
  *y = tpdY * ( TPD_RES_Y - 1 ) / CTP_Y_Res;

  //printk( ">>>>[ELAN]%s, x = %d, y = %d\n", __func__, *x, *y );
} 

static int e3135_ts_poll(struct i2c_client *client)
{
  int   status  = 0;
  int   retry   = 10;

  do
  {
    status = mt_get_gpio_in( GPIO_CTP_EINT_PIN ); 
    retry -= 1;
    mdelay( 20 );
  } while(( status == 1 ) && ( retry > 0 ));

  //printk( "[ELAN] %s: poll interrupt status %s\n", __func__, status == 1 ? "high" : "low" );
  return ( status == 0 ? 0 : -ETIMEDOUT );
} 

static int e3135_get_data(struct i2c_client *client, uint8_t *cmd, uint8_t *buf, size_t size)
{
int   vRet = 0;

  printk( "[ELAN]%s: enter\n", __func__ );

  if( NULL == buf )
    return  -EINVAL;

  if(( tpd_i2c_write( client, cmd, 4 )) != 4 ) 
  {
    printk( "[ELAN]%s: i2c_master_send failed\n", __func__ );
    return  -EINVAL;
  }

  vRet = e3135_ts_poll( client );
  if( vRet < 0 )
    return  -EINVAL;
  else
  { 
    if(( tpd_i2c_read( client, buf, size ) != size ) || ( buf[0] != CMD_S_PKT ))
      return  -EINVAL;
  }

  return  0;
} 

static void e3135_report_data(struct i2c_client *client, uint8_t *buf)
{
uint16_t  fbits = 0x0000;
int16_t   x = 0, y = 0;
uint8_t   i = 0, num = 0x00;
uint8_t   reporte_abs = 0;
int       rc, bytes_to_recv = PACKET_SIZE;

  printk( ">> [ELAN]%s: enter\n", __func__ );

  num  = *( buf + IDX_NUM );
  num &= 0x07; 
  if( num <= FINGER_NUM )
  {
    switch( buf[0] )
    {
      case NORMAL_PKT:
      case FIVE_FINGERS_PKT:
      {
        fbits  = ((uint16_t)(*( buf + IDX_NUM )) >> 3 );
        fbits &= 0x001F;
        if( num == 0 )
        {
          if( buf[KEY_ID_INDEX] & EKTF3135_KEY_BACK )
          {
            button = EKTF3135_KEY_BACK;
            input_report_key( tpd->dev, KEY_BACK, 1 );
          }
          else if( buf[KEY_ID_INDEX] & EKTF3135_KEY_HOME )
          {
            button = EKTF3135_KEY_HOME;
            input_report_key( tpd->dev, KEY_HOME, 1 );
          }
          else if( buf[KEY_ID_INDEX] & EKTF3135_KEY_MENU )
          {
            button = EKTF3135_KEY_MENU;
            input_report_key( tpd->dev, KEY_MENU, 1 );
          }
        #if defined( ELAN_KEY_SEARCH_ENABLE )
          else if( buf[KEY_ID_INDEX] & EKTF3135_KEY_SEARCH )
          {
            button = EKTF3135_KEY_SEARCH;
            input_report_key( tpd->dev, KEY_SEARCH, 1 );
          }
        #endif /* End.. (ELAN_KEY_SEARCH_ENABLE) */
          else if( button == EKTF3135_KEY_BACK )
          {
            button = EKTF3135_KEY_RELEASE;
            input_report_key( tpd->dev, KEY_BACK, 0 );
          }
          else if( button == EKTF3135_KEY_HOME )
          {
            button = EKTF3135_KEY_RELEASE;
            input_report_key( tpd->dev, KEY_HOME, 0 );
          }
          else if( button == EKTF3135_KEY_MENU )
          {
            button = EKTF3135_KEY_RELEASE;
            input_report_key( tpd->dev, KEY_MENU, 0 );
          }
        #if defined( ELAN_KEY_SEARCH_ENABLE )
          else if( button == EKTF3135_KEY_SEARCH )
          {
            button = EKTF3135_KEY_RELEASE;
            input_report_key( tpd->dev, KEY_SEARCH, 0 );
          }
        #endif

          input_mt_sync( tpd->dev );
        }
        else
        {
        uint8_t   idx = IDX_FINGER;

          for( i = 0; i < FINGER_NUM; i++ )
          {
            if( FINGER_ID == ( fbits & FINGER_ID ))
            {
              e3135_parse_xy((uint8_t*)( buf + idx ), (int16_t*)&x, (int16_t*)&y );
              if( !(( x <= 0 ) || ( y <= 0 ) || ( x > TPD_RES_X ) || ( y > TPD_RES_Y )))
              {
                reporte_abs += 1;
//<2013/04/25-24294-kevincheng,update TP FW to 0x5513
                input_report_abs( tpd->dev, ABS_MT_TRACKING_ID, i /*reporte_abs*/);
                input_report_abs( tpd->dev, ABS_MT_TOUCH_MAJOR, 8 );
                input_report_abs( tpd->dev, ABS_MT_POSITION_X,  x );
                input_report_abs( tpd->dev, ABS_MT_POSITION_Y,  y );
                input_mt_sync( tpd->dev );

                TPD_EM_PRINT( x, y, x, y, i/*reporte_abs*/, 1 );
//>2013/04/25-24294-kevincheng
              } 
            } 
            fbits = fbits >> 1;
            idx += 3;
          }

          input_report_key( tpd->dev, BTN_TOUCH, reporte_abs );
          if( !reporte_abs )
          {
            input_mt_sync( tpd->dev );
            TPD_EM_PRINT( x, y, x, y, 0, 0 );
          }
        } 
        input_sync( tpd->dev );
      } break;

      default:
      {
        printk(">> [ELAN]%s: unknown packet type: %02X\n", __func__, buf[0] );
      } break;
    } 
  } 
  else
  {
  #if 0
    checksum_err += 1;
    pr_info( "[ELAN] Checksum Error %d\n", checksum_err );
  #endif
  } 

  return;
} 

static int e3135_recv_data(struct i2c_client *client, uint8_t *buf)
{
int   vRet = 0, bytes_to_recv = PACKET_SIZE;

  if( NULL == buf )
    return  -EINVAL;

  memset( buf, 0x00, bytes_to_recv );

  vRet = tpd_i2c_read( client, buf, bytes_to_recv );

#if 0
  if(Transfor_SIZE == 8){
       printk(">> 6D prototcol\n");
  if( vRet != 8 ){
       printk(">> [ELAN]The first package error.\n");
       printk(">> [ELAN RECV1] %02X %02X %02X %02X %02X %02X %02X %02X\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
       msleep(30);
       return  -EINVAL;
       }
       msleep(1);

  if( buf[0] == FIVE_FINGERS_PKT )
  { 
    vRet = tpd_i2c_read( client, buf + 8, 8 );
    if( vRet != 8 ){
        printk(">> [ELAN]The second package error.\n");
        printk(">> [ELAN RECV2] %02X %02X %02X %02X %02X %02X %02X %02X\n",buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
        msleep(30);
        return  -EINVAL;
        }
        msleep(1);

    vRet = tpd_i2c_read( client, buf+ 16, 2 );
    if( vRet != 2 ){
        printk(">> [ELAN]The third package error.\n");
        printk(">> [ELAN RECV3] %02X %02X\n", buf[16], buf[17]);
        msleep(30);
        return  -EINVAL;
        }
  }}
#endif
#if 1 
  printk( ">>>>> ELAN recv data: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", \
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], buf[8], \
            buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15], buf[16], buf[17] );
#endif

  if( vRet != bytes_to_recv )
  {
    printk(">>  %s: i2c_master_recv error?! \n", __func__ );
    vRet = tpd_i2c_read( client, buf, bytes_to_recv );
    return  -EINVAL;
  }

  return  vRet;
}

static int tpd_event_handler(void *unused)
{
struct  sched_param   param = { .sched_priority = RTPM_PRIO_TPD };
char    buf[PACKET_SIZE] = { 0x00 };

    sched_setscheduler( current, SCHED_RR, &param );
    do
    {
      set_current_state( TASK_INTERRUPTIBLE );
      wait_event_interruptible( waiter, ( tpd_flag != 0 ));

      tpd_flag = 0;
    //<2012/05/22 Yuting Shih, Add for Android 4.x
      TPD_DEBUG_SET_TIME;
    //>2012/05/22 Yuting Shih.
      set_current_state( TASK_RUNNING );

      e3135_recv_data( i2c_client, buf );
      e3135_report_data( i2c_client, buf );
    } while( !kthread_should_stop());

    return  0;
} 

static void tpd_eint_interrupt_handler(void)
{
  TPD_DEBUG_PRINT_INT;
  tpd_flag = 1;
  wake_up_interruptible( &waiter );
} 

static int e3135_set_power_state(struct i2c_client *client, int state)
{
  uint8_t cmd[] = { CMD_W_PKT, 0x50, 0x00, 0x01 };

  printk(">> [ELAN] %s: enter , status : %d\n", __func__ ,state << 3);

  cmd[1] |= (( state << 3 ) & 0x8 );

  printk(">> [ELAN]Dump cmd: %02x, %02x, %02x, %02x\n",
        cmd[0], cmd[1], cmd[2], cmd[3] );

  if(( tpd_i2c_write( client, &cmd[0], sizeof( cmd ))) != sizeof( cmd ))
  {
    printk(">>[ELAN]%s: i2c_master_send failed\n", __func__ );
    return  -EINVAL;
  }

  return  0;
} 

static int e3135_get_power_state(struct i2c_client *client)
{
int   vRet = 0;
uint8_t cmd[] = { CMD_R_PKT, 0x50, 0x00, 0x01 };
uint8_t buf[4] = { 0x00 }, power_state = 0;

  vRet = e3135_get_data( client, (uint8_t*)&cmd[0], (uint8_t*)&buf[0], 4 );
  if( vRet < 0 )
  {
    printk( ">> Get power state fail\n" );
    return  vRet;
  }

  power_state = buf[1];
  printk(">> [ELAN]Dump repsponse: %0X\n", power_state );

  power_state = ( power_state & PWR_STATE_MASK ) >> 3;
  printk(">> [ELAN]Power state = %s\n",
           power_state == PWR_STATE_DEEP_SLEEP ? "Deep Sleep" : "Normal/Idle" );

  return  (int)power_state;
}
  
static int get_fw_version(struct i2c_client *client)
{
  uint8_t   cmd[] = { CMD_R_PKT, 0x00, 0x00, 0x01 };
  uint8_t   buf_recv[4] = { 0x00 };
  int   vRet = 0;
  int   major = 0, minor = 0;

  vRet = e3135_get_data( client, (uint8_t*)&cmd[0], (uint8_t*)&buf_recv[0], 4 );
  if( vRet < 0 )
  {
    printk( ">> %s: failed!?\n", __func__ );
  }
  else
  {
    major = (int)((( buf_recv[1] << 4 ) & 0xF0 ) | (( buf_recv[2] >> 4 ) & 0x0F ));
    minor = (int)((( buf_recv[2] << 4 ) & 0xF0 ) | (( buf_recv[3] >> 4 ) & 0x0F ));
    CTP_VER = (( major << 8 ) & 0xFF00 ) | ( minor & 0x00FF );

    printk( ">> %s: firmware version 0x%4.4X\n", __func__, CTP_VER );
  }
  return  vRet;
} 

static int get_fw_id(struct i2c_client *client)
{
  uint8_t   cmd[] = { CMD_R_PKT, 0xF0, 0x00, 0x01 }; 
  uint8_t   buf_recv[4] = { 0x00 };
  int   vRet = 0;
  int   major = 0, minor = 0;

  vRet = e3135_get_data( client, (uint8_t*)&cmd[0], (uint8_t*)&buf_recv[0], 4 );
  if( vRet < 0 )
  {
    printk( ">> %s: failed!?\n", __func__ );
  }
  else
  {
    major = (int)((( buf_recv[1] << 4 ) & 0xF0 ) | (( buf_recv[2] >> 4 ) & 0x0F ));
    minor = (int)((( buf_recv[2] << 4 ) & 0xF0 ) | (( buf_recv[3] >> 4 ) & 0x0F ));
    CTP_ID = (( major << 8 ) & 0xFF00 ) | ( minor & 0x00FF );
//<2013/05/02-24538-kevincheng,Solve TP FW Update Bug
    if((CTP_ID != 0x55AB)&&(CTP_ID != 0x0037))panel_size = 45;
//>2013/05/02-24538-kevincheng
    printk( ">> %s: firmware ID 0x%4.4X\n", __func__, CTP_ID );
  }
  return  vRet;
} 

static int get_fw_x_res(struct i2c_client *client)
{
  uint8_t   cmd[] = { CMD_R_PKT, 0x60, 0x00, 0x00 };   
  uint8_t   buf_recv[4] = { 0x00 };
  int   vRet = 0;
  int   major = 0, minor = 0;

  vRet = e3135_get_data( client, (uint8_t*)&cmd[0], (uint8_t*)&buf_recv[0], 4 );
  if( vRet < 0 )
  {
    printk( ">> %s: failed!?\n", __func__ );
  }
  else
  {
    major = (int)buf_recv[3];
    minor = (int)buf_recv[2];
    CTP_X_Res = (( major << 4 ) & 0x0F00 ) | ( minor & 0x00FF );
 
    printk( ">> %s: X resolution %d\n", __func__, CTP_X_Res );
  }
  return  vRet;
} 

static int get_fw_y_res(struct i2c_client *client)
{
  uint8_t   cmd[] = { CMD_R_PKT, 0x63, 0x00, 0x00 };  
  uint8_t   buf_recv[4] = { 0x00 };
  int   vRet = 0;
  int   major = 0, minor = 0;

  vRet = e3135_get_data( client, (uint8_t*)&cmd[0], (uint8_t*)&buf_recv[0], 4 );
  if( vRet < 0 )
  {
    printk( ">> %s: failed!?\n", __func__ );
  }
  else
  {
    major = (int)buf_recv[3];
    minor = (int)buf_recv[2];
    CTP_Y_Res = (( major << 4 ) & 0x0F00 ) | ( minor & 0x00FF );
  
    printk( ">> %s: Y resolution: %d\n", __func__, CTP_Y_Res );
  }
  return  vRet;
} 

static int fw_packet_handler(struct i2c_client *client)
{
  int   vRet = 0;

  pr_info( ">> [ELAN]%s: enter\n", __func__ );

  vRet = get_fw_version( client );
  if( vRet < 0 )
    return  vRet;

  vRet = get_fw_x_res( client );
  if( vRet < 0 )
    return  vRet;

  vRet = get_fw_y_res( client );
  if( vRet < 0 )
    return  vRet;

  vRet = get_fw_id( client );
  if( vRet < 0 )
    return  vRet;

  return  0;
} 

static int read_back(struct i2c_client *client , uint8_t* cmd)
{  
  uint8_t   buf_recv[4] = { 0x00 };
  int   vRet = 0;
  int   major = 0, minor = 0;

  vRet = e3135_get_data( client, cmd, buf_recv, 4 );
  if( vRet < 0 )
  {
    pr_info( ">> %s: failed!?\n", __func__ );
  }
  else
  {
    major = (int)buf_recv[3];
    minor = (int)buf_recv[2];
    vRet = (( major << 4 ) & 0x0F00 ) | ( minor & 0x00FF );

    pr_info( ">> %s: 0x%x , %dY\n", __func__, vRet, vRet);
  }
  return  vRet;
} 

static int CTP_GET_DATA(struct i2c_client *client , unsigned int cmd)
{
  int   vRet = 0;
  pr_info( ">> %s , 0x%x \n", __func__,cmd );

  switch( cmd )
  {
    case CMD_ID:
    {
       CTP_ID = read_back(client,FW_ID); 
    } break;

    case CMD_VER:
    {
       CTP_VER = read_back(client,FW_VER); 
    } break;

    case CMD_X_Res:
    {
       CTP_X_Res= read_back(client,FW_X_Res); 
    } break;

    case CMD_Y_Res:
    {
       CTP_Y_Res= read_back(client,FW_Y_Res); 
    } break;

    case CMD_PW:
    {
       CTP_PW_Status= read_back(client,FW_PW); 
    } break;

    case CMD_PW_ON:
    {
       e3135_set_power_state(client,On);
    } break;

    case CMD_PW_OFF:
    {
       e3135_set_power_state(client,Off); 
    } break;
	
    case CMD_All:
    {
       CTP_ID = read_back(client,FW_ID); 
	CTP_VER = read_back(client,FW_VER);
	CTP_X_Res= read_back(client,FW_X_Res); 
       CTP_Y_Res= read_back(client,FW_Y_Res); 
    } break;

    default:
      break;
  }
  return  0;
} 

static int elan_ktf3k_ts_rough_calibrate(struct i2c_client *client){
      uint8_t cmd[] = {CMD_W_PKT, 0x29, 0x01, 0x01};
      int length;

	printk(">> [elan] %s: enter\n", __func__);

	mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	//down(&pSem);
	length = tpd_i2c_write(client, cmd, sizeof(cmd));
	//up(&pSem);
	if (length != sizeof(cmd)) {
		printk(">> [elan] %s: i2c_master_send failed\n", __func__);
		mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		return -EINVAL;
	}
	else{
		//msleep(2000);
		mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		}

	return 0;
}

static int hello_packet_handler(struct i2c_client *client)
{
  int       i,vRet = 0,fail_try_no = 10;
  uint8_t   buf_recv[4] = { 0x00 };

  printk( ">> %s: enter\n", __func__ );

  vRet = e3135_ts_poll( client );
  if( vRet < 0 )
  {
    printk( ">> %s: failed!?\n", __func__ );
    return  -EINVAL;
  } 

      vRet = tpd_i2c_read( client, &buf_recv[0], 4 );

  printk( ">> Get_packet %02X %02X %02X %02X, ret: %d\n", buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3], vRet );

  if(( buf_recv[0] == 0x55 ) && ( buf_recv[1] == 0x55 ) && ( buf_recv[2] == 0x55 ) && ( buf_recv[3] == 0x55 ))
  {	
    return  tpd_normal;
  }
  else if(( buf_recv[0] == 0x55 ) && ( buf_recv[1] == 0x55 ) && ( buf_recv[2] == 0x80 ) && ( buf_recv[3] == 0x80 ))
  {	
    return  tpd_recovery;
  }
  
  return  -EINVAL;
} 

static int EKTF_3135_setup(struct i2c_client *client)
{
  int   i,vRet = 0;
  uint8_t buf[PACKET_SIZE] = { 0x00 };

  printk(">> %s\r\n",__FUNCTION__);
  
  vRet = hello_packet_handler( client );
  if( vRet < 0 )
    goto  hand_shake_failed;
  printk( ">> %s , hello packet got.\n", __func__ );

  if( 0x80 == vRet )  /** Recovery **/
  {
    return  vRet;
  }

  mdelay(300);
  e3135_recv_data( i2c_client, buf );
  printk(">> after hello 300ms , %x %x %x %x\n",buf[0],buf[1],buf[2],buf[3]);

  vRet = fw_packet_handler( client );
  if( vRet < 0 )
    goto  hand_shake_failed;

//  printk(">> delay 1.5S for calibration ready\n");
  //elan_ktf3k_ts_rough_calibrate(client);
//  msleep(1500);

  printk( ">> %s: firmware checking done.\n", __func__ );

hand_shake_failed:
  return  vRet;
}

static int tpd_hwPower_ctrl(int ctrl_OnOff)
{
  switch( ctrl_OnOff )
  {
    case On:
    {
        printk(">> TP PWR on 3.3v\r\n");
        hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_3300, E3135_HWPWM_NAME ); 
    } break;

    case Off:
    {
        hwPowerDown(TPD_POWER_SOURCE_CUSTOM, E3135_HWPWM_NAME );
    } break;

    default:
      break;
  }
  return  0;
}

static int  e3135_probe(struct i2c_client *pClient, const struct i2c_device_id *id)
 {
    char    buf[PACKET_SIZE] = { 0x00 };	 
    int   i,vRet  = 0;
    int   vErr  = 0;
    kal_uint32 j; 
//<2013/02/05-21580-kevincheng,Add TP attr control
    struct platform_device *sensor_dev;
//>2013/02/05-21580-kevincheng

    printk( ">> Yo Yo EKTF 3135 %s: enter..\n", __func__ );

    pClient->timing  = 400; 
    pClient->addr   |= I2C_ENEXT_FLAG;
    i2c_client       = pClient;

#if defined(Elan_IAP)
    if (misc_register(&IAP_device) < 0)
         printk(">> [ELAN]misc_register failed!!");
    else  
         printk(">> [ELAN]misc_register finished!!");
#endif

    tpd_hwPower_ctrl( On );
    msleep(10);
   
    printk(">> EKTF 3135 ,  GPIO_CTP_EN_PIN : %d , GPIO_CTP_RST_PIN : %d\n",GPIO_CTP_EINT_PIN,GPIO_CTP_RST_PIN);
    mt_set_gpio_mode( GPIO_CTP_EINT_PIN, GPIO_MODE_00 );
    mt_set_gpio_dir( GPIO_CTP_EINT_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out( GPIO_CTP_EINT_PIN, GPIO_OUT_ONE );

    mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
    mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );

    mt_set_gpio_mode( GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT );
    msleep( 1500 );

    I2CDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &I2CDMABuf_pa, GFP_KERNEL);
    if(!I2CDMABuf_va)
    {
	printk(">> Allocate Touch DMA I2C Buffer failed!\n");
	return -1;
    }
	
    if( !i2c_check_functionality( i2c_client->adapter, I2C_FUNC_SMBUS_BYTE_DATA )) 
    {
      printk( ">> [ELAN] No supported i2c func what we need?!!\n" );
    }

    vErr = EKTF_3135_setup( i2c_client );
    if( vErr < 0 )
    {
      printk( ">> No Elan chip inside\n" );
      return  -ENODEV;
    }
    else if( 0x80 == vErr ) 
    {
      printk( ">> TP Recovery Mode\n" );
    }
    printk( ">>>>>> [ELAN]EKTF %s success!!\n", __func__ );
    tpd_load_status = 1;  /** The panel checked success. **/

    thread = kthread_run( tpd_event_handler, 0, ELAN_TPD_DEVICE );
    if( IS_ERR( thread ))
    {
      vRet = PTR_ERR( thread );
      printk( TPD_DEVICE ">>  failed to create ektf 3135 kernel thread: %d\n", vRet );
    }

    set_bit( KEY_BACK,  tpd->dev->keybit );
    set_bit( KEY_MENU,  tpd->dev->keybit );
    set_bit( KEY_HOME,  tpd->dev->keybit );

    mt65xx_eint_set_sens( CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE );
    mt65xx_eint_set_hw_debounce( CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN );
    mt65xx_eint_registration( CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_TOUCH_PANEL_POLARITY, tpd_eint_interrupt_handler, 1);
    mt65xx_eint_unmask( CUST_EINT_TOUCH_PANEL_NUM );

    printk( ">> Touch Panel Device Probe %s\n", ( vRet < 0 ) ? "FAIL" : "PASS" );

//<2013/02/05-21580-kevincheng,Add TP attr control
    sensor_dev = platform_device_register_simple("e3135", -1, NULL, 0); 
    if (IS_ERR(sensor_dev)) 
    { 
        printk (">> TP attr driver_init: error\n"); 
    } 

    vErr = sysfs_create_group(&sensor_dev->dev.kobj, &e3135_attr_group);
    if (vErr !=0)
    {
        printk(">> %s:create sysfs group error", __func__);
    }
//>2013/02/05-21580-kevincheng
//<2013/03/21-23111-kevincheng , Fix TP abnormal after factory reset
//<2013/05/02-24538-kevincheng,Solve TP FW Update Bug 
    #if 1
   // if(CTP_ID == 0x55aa || CTP_ID == 0x0722)
   //   { 
   //    if(CTP_VER != 0x552D)
   //      {
   //       printk(">> ID : 0x%x , FW_VER : 0x%x , need to update\n",CTP_ID,CTP_VER);
   //  tpd_2wireice();
   //      }
   //   }
//<2013/04/11-23759-kevincheng , Auto Update TP FW
//<2013/04/18-24000-kevincheng , Update TP FW to 0x5510
//<2013/04/25-24294-kevincheng , Update TP FW to 0x5513
//<2013/05/10-24791-kevincheng , Update TP FW to 0x551B
//<2013/05/23-25221-kevincheng , Update TP FW to 0x5523
//<2013/05/31-25576-kevincheng , Update TP FW to 0x5526
//<2013/06/14-25970-kevincheng , Update TP FW to 0x5528
   //if(CTP_ID == 0x55ab || CTP_ID == 0x0037)
   //   { 
       if((CTP_VER < 0x5528) || ((CTP_ID != 0x55ab) && (CTP_ID != 0x0037)))
         {
          printk(">> ID : 0x%x , FW_VER : 0x%x , need to update\n",CTP_ID,CTP_VER);
	  tpd_2wireice();
         }
   //   }
//>2013/06/14-25970-kevincheng
//>2013/05/31-25576-kevincheng
//>2013/05/23-25221-kevincheng
//>2013/05/10-24791-kevincheng
//>2013//05/2-24538-kevincheng
//>2013/04/25-24294-kevincheng
//>2013/04/18-24000-kevincheng
//>2013/04/11-23759-kevincheng
    #endif
//>2013/03/21-23111-kevincheng

    return  0;
 }

 static int  e3135_remove(struct i2c_client *client)
 
 {
   printk(">> %s\n",__FUNCTION__);
   if(I2CDMABuf_va)
    {
	dma_free_coherent(NULL, 4096, I2CDMABuf_va, I2CDMABuf_pa);
	I2CDMABuf_va = NULL;
	I2CDMABuf_pa = 0;
    }
    
   return 0;
 }

static struct i2c_driver tpd_e3135_driver = {
  .driver = {
         .name = e3135_NAME,
//       .owner = THIS_MODULE,
  },
  .probe = e3135_probe,
  .remove = e3135_remove,
  .id_table = e3135_tpd_id,
  .detect = e3135_detect,
//  .address_data = &addr_data,
};

static int tpd_local_init(void)
 {
   printk(">> %s ,tpd_load_status : %d \n",__FUNCTION__,tpd_load_status);
   if(i2c_add_driver(&tpd_e3135_driver)!=0)
   	{
  		printk(">>EKTF 3135 unable to add i2c driver.\n");
      	return -1;
    }
    tpd_load_status = 1;
#if 0
    if(tpd_load_status == 0) 
    {
    	printk(">> EKTF 3135 add error touch panel driver.\n");
    	i2c_del_driver(&tpd_i2c_driver);
    	return -1;
    }
#endif 
	
#ifdef TPD_HAVE_BUTTON     
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif   
  
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))    
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT*4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT*4);
#endif 

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);	
#endif  
		TPD_DMESG("end %s, %d\n", __FUNCTION__, __LINE__);  
		tpd_type_cap = 1;
    return 0; 
 }

//<2013/05/02-24538-kevincheng,Solve FW Update bug
//<2013/06/14-25970-kevincheng,Update TP FW to 0x5528
#if 0
int tpd_resume(struct early_suspend *h)
{
  printk( ">> [ELAN]%s: enter.. \n", __func__ );
#if 0
  mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
  mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
  mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO );
  msleep(5);
  mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
#else
   tpd_hwPower_ctrl( On );
    msleep(10);
   
    printk(">> EKTF 3135 ,  GPIO_CTP_EN_PIN : %d , GPIO_CTP_RST_PIN : %d\n",GPIO_CTP_EINT_PIN,GPIO_CTP_RST_PIN);
    mt_set_gpio_mode( GPIO_CTP_EINT_PIN, GPIO_MODE_00 );
    mt_set_gpio_dir( GPIO_CTP_EINT_PIN, GPIO_DIR_OUT);
    mt_set_gpio_out( GPIO_CTP_EINT_PIN, GPIO_OUT_ONE );

    mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
    mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );

    mt_set_gpio_mode( GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT );
#endif
  mt65xx_eint_unmask( CUST_EINT_TOUCH_PANEL_NUM );
} 

void tpd_suspend(struct early_suspend *h)
{
  int     rc = 0;
  uint8_t buf[PACKET_SIZE] = { 0x00 };

  printk( ">> [ELAN]%s: enter.. , PW_Lock : %d\n", __func__,IAP_PW_Lock );
  
if(!IAP_PW_Lock){
  mt65xx_eint_mask( CUST_EINT_TOUCH_PANEL_NUM );

 // rc = e3135_set_power_state( i2c_client, PWR_STATE_DEEP_SLEEP );
 //rc = e3135_get_power_state( i2c_client );
   tpd_hwPower_ctrl( Off );

   mt_set_gpio_mode( GPIO_CTP_EINT_PIN, GPIO_MODE_00 );
   mt_set_gpio_dir( GPIO_CTP_EINT_PIN, GPIO_DIR_OUT );
   mt_set_gpio_out( GPIO_CTP_EINT_PIN, GPIO_OUT_ZERO );
   
   mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
   mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
   mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO );
 }
} 
#else
int tpd_resume(struct early_suspend *h)
{
  printk( ">> [ELAN]%s: enter.. \n", __func__ );

  mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
  mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
  mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO );
  msleep(5);
  mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
 
  mt65xx_eint_unmask( CUST_EINT_TOUCH_PANEL_NUM );
} 

void tpd_suspend(struct early_suspend *h)
{
  int     rc = 0;
  uint8_t buf[PACKET_SIZE] = { 0x00 };

  printk( ">> [ELAN]%s: enter.. , PW_Lock : %d\n", __func__,IAP_PW_Lock );
  
if(!IAP_PW_Lock){
  mt65xx_eint_mask( CUST_EINT_TOUCH_PANEL_NUM );

  rc = e3135_set_power_state( i2c_client, PWR_STATE_DEEP_SLEEP );
  rc = e3135_get_power_state( i2c_client );}
} 
#endif
//>2013/06/14-25970-kevincheng
//>2013/05/02-24538-kevincheng

static struct tpd_driver_t tpd_device_driver = {
		.tpd_device_name = e3135_NAME,
		.tpd_local_init = tpd_local_init,
		.suspend = tpd_suspend,
		.resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
		.tpd_have_button = 1,
#else
		.tpd_have_button = 0,
#endif		
};
		 
 static int __init tpd_driver_init(void) {
	 printk(">> EKTF 3135 touch panel driver init\n");
	   i2c_register_board_info(e3135_I2C_Channel, &e3135_i2c_tpd, 1);
		 if(tpd_driver_add(&tpd_device_driver) < 0)
			 TPD_DMESG("add EKTF driver failed\n");
	 return 0;
 }
 
 /* should never be called */
 static void __exit tpd_driver_exit(void) {
	 TPD_DMESG(">> EKTF 3135 touch panel driver exit\n");
	 tpd_driver_remove(&tpd_device_driver);
 }
 
 module_init(tpd_driver_init);
 module_exit(tpd_driver_exit);
