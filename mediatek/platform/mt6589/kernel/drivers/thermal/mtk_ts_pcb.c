#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#include "mach/mtk_thermal_monitor.h"

static int i_enable_dbg=0;
#define PK_DBG_FUNC(fmt, arg...)    if(i_enable_dbg==1) { printk("%s: " fmt, __FUNCTION__ ,##arg); }
#define PK_ERR(fmt, arg...)         if(i_enable_dbg==1) { printk("%s: " fmt, __FUNCTION__ ,##arg); }

#define TMP_I2C_BUSNUM 6

#define I2C_DRIVER_NAME_A "TMP103_HW_device_A"
#define I2C_DRIVER_NAME_B "TMP103_HW_device_B"
#define DEVICE_DRIVER_NAME "TMP103_HW_i2c_device"

static struct i2c_client * g_pstI2CclientA = NULL;
static struct i2c_client * g_pstI2CclientB = NULL;

static const struct i2c_device_id TMP103_HW_i2c_id_table[] = {
	{ I2C_DRIVER_NAME_A, 0 },
	{ I2C_DRIVER_NAME_B, 0 },
	{ }
};

static struct i2c_board_info __initdata TMP103_HW_i2c_info[] = {
	[0] = {
		.type       = I2C_DRIVER_NAME_A,
		.addr       = 0x70,
	},
	[1] = {
		.type       = I2C_DRIVER_NAME_B,
		.addr       = 0x71,
	},
};

static int TMP103A_value = -127;
static int TMP103B_value = -127;
static DEFINE_MUTEX(TMP103_i2c_access);

int TMP103_I2C_write_bytes(u8 id, u8 *SendData, u8 sizeofSendData)
{
	int ret = 0;
  PK_DBG_FUNC("id=0x%x \n",id);
  mutex_lock(&TMP103_i2c_access);
	//g_pstI2CclientA->addr = id;

	if (id == 0x70)
	{
			ret = i2c_master_send(g_pstI2CclientA, SendData, sizeofSendData);	
	}
	else if (id == 0x71)
	{
			ret = i2c_master_send(g_pstI2CclientB, SendData, sizeofSendData);	
	}
	
	if ( ret != sizeofSendData ) 
	{
		PK_ERR("i2c_master_send failed!!\n");
	}
	PK_DBG_FUNC("id=0x%x  ret=%d \n",id,ret);
	mutex_unlock(&TMP103_i2c_access);
	return ret;
}

int TMP103_I2C_read_bytes(u8 id, u8 *puBuff, u8 sizeofRecvData)
{
	int ret = 0;

	mutex_lock(&TMP103_i2c_access);
	//g_pstI2CclientA->addr = id;
  if (id == 0x70)
	{  
			ret = i2c_master_recv(g_pstI2CclientA, (char *)puBuff, sizeofRecvData);
	}
	else if (id == 0x71)
	{
			ret = i2c_master_recv(g_pstI2CclientB, (char *)puBuff, sizeofRecvData);
	}
	
	if ( ret != sizeofRecvData ) 
	{
		PK_ERR("i2c_master_recv failed!!\n");
	}

	mutex_unlock(&TMP103_i2c_access);
	return ret;
}

int mtk_TMP103_get_value(int iid)
{
		if (iid==1)
		{
			return TMP103A_value;
		}
		else if (iid==2)
		{
			return TMP103B_value;
		}
		else
			return 0;
}

static int TMP103_status_report(unsigned long data)
{	
		u8 out_buff[1];
		u8 in_buff[1]; 

		if (data==1)
		{		
		  	//TMP103A_value
		  	out_buff[0] = 0x00;  //Pointer Register byte
		  	in_buff[0] = 0xFF;
				if ((TMP103_I2C_write_bytes(0x70, (u8*)out_buff, sizeof(out_buff)))!= sizeof(out_buff)) 
				{
					PK_ERR("TMP103_I2C_write_bytes 0x70 failed!!\n");
				}
				
				if ((TMP103_I2C_read_bytes(0x70, (u8*)in_buff, sizeof(in_buff)))!= sizeof(in_buff)) 
				{
					PK_ERR("TMP103_I2C_read_bytes 0x70 failed!!\n");
				}
				else
				{
					PK_DBG_FUNC("TMP103A_value 0x%x \n",in_buff[0]);
					if (in_buff[0] & 0x80) //negative
					{
						TMP103A_value = (in_buff[0] - 256);
					}
					else  //positive
					{
						TMP103A_value = in_buff[0];
					}
				}
				PK_DBG_FUNC("TMP103A=%d \n",TMP103A_value);
		}
		else if (data==2)
		{
				//TMP103B_value
				out_buff[0] = 0x00;  //Pointer Register byte
		  	in_buff[0] = 0xFF;
				if ((TMP103_I2C_write_bytes(0x71, (u8*)out_buff, sizeof(out_buff)))!= sizeof(out_buff)) 
				{
					PK_ERR("TMP103_I2C_write_bytes 0x71 failed!!\n");
				}
				
				if ((TMP103_I2C_read_bytes(0x71, (u8*)in_buff, sizeof(in_buff)))!= sizeof(in_buff)) 
				{
					PK_ERR("TMP103_I2C_read_bytes 0x71 failed!!\n");
				}
				else
				{
					PK_DBG_FUNC("TMP103B_value 0x%x \n",in_buff[0]);
					if (in_buff[0] & 0x80) //negative
					{
						TMP103B_value = (in_buff[0] - 256);
					}
					else  //positive
					{
						TMP103B_value = in_buff[0];
					}
				}
				PK_DBG_FUNC("TMP103B=%d \n",TMP103B_value);
		}

		return 0;
}

/****************************************************************************************
* I2C skeleton 
****************************************************************************************/
static int i_enable_A=0;
static int i_enable_B=0;
static int TMP103_HW_i2c_probe(struct i2c_client *client,	const struct i2c_device_id *id)
{
	int ret = 0;
	u8 out_buff[2];

	PK_DBG_FUNC("TMP103_I2C_Probe\n");
	
	out_buff[0] = 0x01;  //Pointer Register byte
  out_buff[1] = 0x22;  //Data byte; continuous mode and 1Hz

	if (client->addr==0x70)
	{	
		printk("TMP103_HW_device_A 0x70\n");
		g_pstI2CclientA = client;
		g_pstI2CclientA->timing = 400;//400k
		g_pstI2CclientA->ext_flag = I2C_DIRECTION_FLAG | I2C_POLLING_FLAG;
		
		if ((TMP103_I2C_write_bytes(0x70, (u8*)out_buff, sizeof(out_buff)))!= sizeof(out_buff)) //TMP103A, continuous mode and 1Hz
		{
			PK_ERR("TMP103_I2C_write_bytes 0x70 failed!!\n");
			return 0;
		}
		i_enable_A = 1;
	}
	else if (client->addr==0x71)
	{
		printk("TMP103_HW_device_B 0x71\n");
		g_pstI2CclientB = client;
		g_pstI2CclientB->timing = 400;//400k
		g_pstI2CclientB->ext_flag = I2C_DIRECTION_FLAG | I2C_POLLING_FLAG;
		
		if ((TMP103_I2C_write_bytes(0x71, (u8*)out_buff, sizeof(out_buff)))!= sizeof(out_buff)) //TMP103B, continuous mode and 1Hz
		{
			PK_ERR("TMP103_I2C_write_bytes 0x71 failed!!\n");
			return 0;
		}
		i_enable_B = 1;
	}
	
	return ret; 
}

static struct i2c_driver TMP103_HW_i2c_driver = {
	.driver = {
		.name   = DEVICE_DRIVER_NAME,
		.owner  = THIS_MODULE,
	}, 
	.probe  = TMP103_HW_i2c_probe,
	.id_table = TMP103_HW_i2c_id_table,
};
/****************************************************************************************
* I2C skeleton 
****************************************************************************************/
static char cmd_buf[10];
static ssize_t TMP103_HW_Write_Proc(struct file *file, const char *buf, unsigned long len, void *data)
{
		int ret;
	  int i_par=0;
	  
	  ret = copy_from_user(cmd_buf, buf, len);
	  if (ret < 0)return -1;
	  	
	  cmd_buf[len] = '\0';
	  printk("[****TMP103_HW_Write_Proc****] %s\n", cmd_buf);

		sscanf(cmd_buf, "%d ", &i_par);
		i_enable_dbg = i_par;
		
    return -1;
}

static int TMP103_HW_Read_Proc(char *buf, char **start, off_t off, int count, int *eof, void *data)
{
		char *p = buf;
    int len = 0;
    int i_A, i_B;

		if (off != 0)
			return 0;
	
		printk("TMP103_HW_Read_Proc \n");
		
		i_A = mtk_TMP103_get_value(1);
		i_B = mtk_TMP103_get_value(2);
		printk("TMP103A_value=%d \n",i_A);
		printk("TMP103B_value=%d \n",i_B);

		p += sprintf(p, "PCB1 = %d\n", i_A); 
		p += sprintf(p, "PCB2 = %d\n", i_B); 
		
		*start = buf + off;

    len = p - buf;
    if (len > off)
        len -= off;
    else
        len = 0;

    return len < count ? len : count;
}

/*********************************************************************/
//Thermal Zone
/*********************************************************************/
static struct thermal_zone_device *thz_dev_pcb1;
static struct thermal_zone_device *thz_dev_pcb2;
static int kernelmode = 0;
/* bind callback functions to thermalzone */
static int mtktspcb1_bind(struct thermal_zone_device *thermal, struct thermal_cooling_device *cdev)
{ 
	return 0;
}
static int mtktspcb1_unbind(struct thermal_zone_device *thermal, struct thermal_cooling_device *cdev)
{ 
	return 0;
}
static int mtktspcb1_get_temp(struct thermal_zone_device *thermal, unsigned long *t)
{
		int temp_A_value;
		
	  if (i_enable_A == 0) //need to wait for I2C init 
	  {
	  	*t = (mtk_TMP103_get_value(1)*1000);
	  	PK_DBG_FUNC("mtktspcb1_get_temp %ld \n",*t);
	  	return 0;
	  }	
	  
		TMP103_status_report(1); // get A
		
		temp_A_value = (mtk_TMP103_get_value(1)*1000);
		if ((temp_A_value >100000) || (temp_A_value <-30000))
			printk("mtktspcb1_get_temp %d \n",temp_A_value);
			
		*t = temp_A_value;
    PK_DBG_FUNC("mtktspcb1_get_temp %ld \n",*t);
		return 0;
}
static int mtktspcb1_get_mode(struct thermal_zone_device *thermal, enum thermal_device_mode *mode)
{
    *mode = (kernelmode) ? THERMAL_DEVICE_ENABLED : THERMAL_DEVICE_DISABLED;
		return 0;
}
static int mtktspcb1_set_mode(struct thermal_zone_device *thermal, enum thermal_device_mode mode)
{
    kernelmode = mode;
		return 0;
}
static int mtktspcb1_get_trip_type(struct thermal_zone_device *thermal, int trip, enum thermal_trip_type *type)
{
		*type = 0;
		return 0;
}
static int mtktspcb1_get_trip_temp(struct thermal_zone_device *thermal, int trip, unsigned long *temp)
{
		*temp = 889000; 
		return 0;
}
static int mtktspcb1_get_crit_temp(struct thermal_zone_device *thermal, unsigned long *temperature)
{
		*temperature = 889000;
		return 0;
}
static struct thermal_zone_device_ops mtktspcb1_dev_ops = {
	.bind = mtktspcb1_bind,
	.unbind = mtktspcb1_unbind,
	.get_temp = mtktspcb1_get_temp,
	.get_mode = mtktspcb1_get_mode,
	.set_mode = mtktspcb1_set_mode,
	.get_trip_type = mtktspcb1_get_trip_type,
	.get_trip_temp = mtktspcb1_get_trip_temp,
	.get_crit_temp = mtktspcb1_get_crit_temp,
};

static int mtktspcb2_bind(struct thermal_zone_device *thermal, struct thermal_cooling_device *cdev)
{
	return 0;
}
static int mtktspcb2_unbind(struct thermal_zone_device *thermal, struct thermal_cooling_device *cdev)
{
	return 0;
}
static int mtktspcb2_get_temp(struct thermal_zone_device *thermal, unsigned long *t)
{
		int temp_B_value;
		
		if (i_enable_B == 0) //need to wait for I2C init 
	  {
	  	*t = (mtk_TMP103_get_value(2)*1000);
	  	PK_DBG_FUNC("mtktspcb2_get_temp %ld \n",*t);
	  	return 0;
	  }	
	  	
		TMP103_status_report(2); // get B
		
		temp_B_value = (mtk_TMP103_get_value(2)*1000);
    if ((temp_B_value >100000) || (temp_B_value <-30000))
			printk("mtktspcb2_get_temp %d \n",temp_B_value);
		
		*t = temp_B_value;
    PK_DBG_FUNC("mtktspcb2_get_temp %ld \n",*t);
		return 0;
}
static int mtktspcb2_get_mode(struct thermal_zone_device *thermal, enum thermal_device_mode *mode)
{
    *mode = (kernelmode) ? THERMAL_DEVICE_ENABLED : THERMAL_DEVICE_DISABLED;
		return 0;
}
static int mtktspcb2_set_mode(struct thermal_zone_device *thermal, enum thermal_device_mode mode)
{
    kernelmode = mode;
		return 0;
}
static int mtktspcb2_get_trip_type(struct thermal_zone_device *thermal, int trip, enum thermal_trip_type *type)
{
		*type = 0;
		return 0;
}
static int mtktspcb2_get_trip_temp(struct thermal_zone_device *thermal, int trip, unsigned long *temp)
{
		*temp = 889000; 
		return 0;
}
static int mtktspcb2_get_crit_temp(struct thermal_zone_device *thermal, unsigned long *temperature)
{
		*temperature = 889000;
		return 0;
}
static struct thermal_zone_device_ops mtktspcb2_dev_ops = {
	.bind = mtktspcb2_bind,
	.unbind = mtktspcb2_unbind,
	.get_temp = mtktspcb2_get_temp,
	.get_mode = mtktspcb2_get_mode,
	.set_mode = mtktspcb2_set_mode,
	.get_trip_type = mtktspcb2_get_trip_type,
	.get_trip_temp = mtktspcb2_get_trip_temp,
	.get_crit_temp = mtktspcb2_get_crit_temp,
};

int mtktspcb1_register_thermal(void)
{
    PK_DBG_FUNC("[mtktspcb1_register_thermal] \n");

    /* trips : trip 0~1 */
	  thz_dev_pcb1 = mtk_thermal_zone_device_register("mtktspcb1", 0, NULL,
                &mtktspcb1_dev_ops, 0, 0, 0, 1000);

    return 0;
}
int mtktspcb2_register_thermal(void)
{
    PK_DBG_FUNC("[mtktspcb2_register_thermal] \n");

    /* trips : trip 0~1 */
	  thz_dev_pcb2 = mtk_thermal_zone_device_register("mtktspcb2", 0, NULL,
                &mtktspcb2_dev_ops, 0, 0, 0, 1000);

    return 0;
}

void mtktspcb1_unregister_thermal(void)
{
    PK_DBG_FUNC("[mtktspcb1_unregister_thermal] \n");

    if (thz_dev_pcb1) {
        mtk_thermal_zone_device_unregister(thz_dev_pcb1);
        thz_dev_pcb1 = NULL;
    }
}
void mtktspcb2_unregister_thermal(void)
{
    PK_DBG_FUNC("[mtktspcb2_unregister_thermal] \n");

    if (thz_dev_pcb2) {
        mtk_thermal_zone_device_unregister(thz_dev_pcb2);
        thz_dev_pcb2 = NULL;
    }
}
/*********************************************************************/

static int __init TMP103_HW_i2c_init( void )
{
	struct proc_dir_entry *prEntry;

	i2c_register_board_info(TMP_I2C_BUSNUM, TMP103_HW_i2c_info, ARRAY_SIZE(TMP103_HW_i2c_info)); 
	
	prEntry = create_proc_entry("TMP103", 0660, 0); 

  if (prEntry) 
  {
  	prEntry->read_proc = TMP103_HW_Read_Proc; 
    prEntry->write_proc = TMP103_HW_Write_Proc; 
    PK_DBG_FUNC("add /proc/TMP103 entry success \n"); 
  }
  else 
  {
    PK_DBG_FUNC("add /proc/TMP103 entry fail \n");  
  }
  
	mtktspcb1_register_thermal();
	mtktspcb2_register_thermal();
	
	return i2c_add_driver( &TMP103_HW_i2c_driver );
}

static void __exit TMP103_HW_i2c_exit(void)
{
	i2c_del_driver(&TMP103_HW_i2c_driver);
	mtktspcb1_unregister_thermal();
	mtktspcb2_unregister_thermal();
}


EXPORT_SYMBOL(mtk_TMP103_get_value); 

module_init( TMP103_HW_i2c_init );
module_exit( TMP103_HW_i2c_exit);

MODULE_DESCRIPTION("TMP103 Sensor driver");
MODULE_AUTHOR("Ahsin Chen <ahsin.chen@mediatek.com>");
MODULE_LICENSE("GPL");
