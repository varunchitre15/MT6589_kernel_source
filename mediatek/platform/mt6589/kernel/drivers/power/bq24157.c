
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <asm/atomic.h>

#include <cust_acc.h>
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#include <linux/hwmsen_helper.h>

#include <mach/mt_typedefs.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pm_ldo.h>

#include "bq24157.h"

/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define bq24157_SLAVE_ADDR_WRITE   0xD4
#define bq24157_SLAVE_ADDR_Read    0xD5

extern int Enable_BATDRV_LOG;

unsigned int g_bq24157_rdy_flag = 0;
static struct i2c_client *new_client = NULL;
static const struct i2c_device_id bq24157_i2c_id[] = {{"bq24157",0},{}};   

static int bq24157_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);

static struct i2c_driver bq24157_driver = {
	.driver = {
		.name	= "bq24157",
	},
	.probe		= bq24157_driver_probe,
	.id_table	= bq24157_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/
#define bq24157_REG_NUM 7  
kal_uint8 bq24157_reg[bq24157_REG_NUM] = {0};

static DEFINE_MUTEX(bq24157_i2c_access);
/**********************************************************
  *
  *   [I2C Function For Read/Write bq24157] 
  *
  *********************************************************/
int bq24157_read_byte(kal_uint8 cmd, kal_uint8 *returnData)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int     ret=0;

    mutex_lock(&bq24157_i2c_access);
	
    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG;// | I2C_DIRECTION_FLAG;

    cmd_buf[0] = cmd;
    ret = i2c_master_send(new_client, &cmd_buf[0], (1<<8 | 1));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;

        mutex_unlock(&bq24157_i2c_access);
        return 0;
    }
    
    readData = cmd_buf[0];
    *returnData = readData;

    // new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
    
    mutex_unlock(&bq24157_i2c_access);    
    return 1;
}

ssize_t bq24157_write_byte(u8 cmd, u8 writeData)
{
    char    write_data[2] = {0};
    int     ret=0;
    
    mutex_lock(&bq24157_i2c_access);
    
    write_data[0] = cmd;
    write_data[1] = writeData;
    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG );// | I2C_DIRECTION_FLAG;
    
    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0) 
    {
        new_client->ext_flag=0;
        mutex_unlock(&bq24157_i2c_access);
        return 0;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&bq24157_i2c_access);
    return 1;
}

/**********************************************************
  *
  *   [Read / Write Function] 
  *
  *********************************************************/
kal_uint32 bq24157_read_interface(kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
	kal_uint8 bq24157_reg = 0;
    int ret = 0;

	//printk("--------------------------------------------------\n");

    ret = bq24157_read_byte(RegNum, &bq24157_reg);
	if (Enable_BATDRV_LOG == 1) {
		printk("[bq24157_read_interface] Reg[%x]=0x%x\n", RegNum, bq24157_reg);
	}
    bq24157_reg &= (MASK << SHIFT);
    *val = (bq24157_reg >> SHIFT);
	if (Enable_BATDRV_LOG == 1) {
		printk("[bq24157_read_interface] val=0x%x\n", *val);
	}
    return ret;
}

kal_uint32 bq24157_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 bq24157_reg = 0;
    int ret = 0;

    printk("--------------------------------------------------\n");

    ret = bq24157_read_byte(RegNum, &bq24157_reg);
    printk("[bq24157_config_interface] Reg[%x]=0x%x\n", RegNum, bq24157_reg);
    
    bq24157_reg &= ~(MASK << SHIFT);
    bq24157_reg |= (val << SHIFT);

    ret = bq24157_write_byte(RegNum, bq24157_reg);
    printk("[bq24157_config_interface] write Reg[%x]=0x%x\n", RegNum, bq24157_reg);

    // Check
    //bq24157_read_byte(RegNum, &bq24157_reg);
    //printk("[bq24157_config_interface] Check Reg[%x]=0x%x\n", RegNum, bq24157_reg);

    return ret;
}

kal_uint32 bq24157_config_interface_liao(kal_uint8 RegNum, kal_uint8 val)
{   
    int ret = 0;
    
    ret = bq24157_write_byte(RegNum, val);

    return ret;
}

/**********************************************************
  *
  *   [bq24157 Function] 
  *
  *********************************************************/
//CON0
void bq24157_set_tmr_rst(kal_uint32 val)
{
	kal_uint32 ret=0;	

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON0), 
									(kal_uint8)(val),
									(kal_uint8)(CON0_TMR_RST_MASK),
									(kal_uint8)(CON0_TMR_RST_SHIFT)
									);
}

kal_uint32 bq24157_get_slrst_status(void)
{
	kal_uint32 ret=0;
	kal_uint8 val=0;

	ret=bq24157_read_interface( 	(kal_uint8)(bq24157_CON0), 
									(&val),
									(kal_uint8)(CON0_SLRST_MASK),
									(kal_uint8)(CON0_SLRST_SHIFT)
									);
	return val;
}

void bq24157_set_en_stat(kal_uint32 val)
{
	kal_uint32 ret=0;	

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON0), 
									(kal_uint8)(val),
									(kal_uint8)(CON0_EN_STAT_MASK),
									(kal_uint8)(CON0_EN_STAT_SHIFT)
									);
}

kal_uint32 bq24157_get_chip_status(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

	ret=bq24157_read_interface( 	(kal_uint8)(bq24157_CON0), 
                                    (&val),
                                    (kal_uint8)(CON0_STAT_MASK),
                                    (kal_uint8)(CON0_STAT_SHIFT)
                                    );
    return val;
}

kal_uint32 bq24157_get_fault_reason(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

	ret=bq24157_read_interface( 	(kal_uint8)(bq24157_CON0), 
									(&val),
									(kal_uint8)(CON0_FAULT_MASK),
									(kal_uint8)(CON0_FAULT_SHIFT)
									);
	return val;
}

//CON1
void bq24157_set_lin_limit(kal_uint32 val)
{
	kal_uint32 ret=0;	

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON1), 
									(kal_uint8)(val),
									(kal_uint8)(CON1_LIN_LIMIT_MASK),
									(kal_uint8)(CON1_LIN_LIMIT_SHIFT)
									);
}

void bq24157_set_lowv_2(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON1), 
                                    (kal_uint8)(val),
									(kal_uint8)(CON1_LOW_V_2_MASK),
									(kal_uint8)(CON1_LOW_V_2_SHIFT)
                                    );
}

void bq24157_set_lowv_1(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON1), 
                                    (kal_uint8)(val),
									(kal_uint8)(CON1_LOW_V_1_MASK),
									(kal_uint8)(CON1_LOW_V_1_SHIFT)
                                    );
}

void bq24157_set_te(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_TE_MASK),
                                    (kal_uint8)(CON1_TE_SHIFT)
                                    );
}

void bq24157_set_ce(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_CE_MASK),
                                    (kal_uint8)(CON1_CE_SHIFT)
                                    );
}

void bq24157_set_hz_mode(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_HZ_MODE_MASK),
                                    (kal_uint8)(CON1_HZ_MODE_SHIFT)
                                    );
}

void bq24157_set_opa_mode(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_OPA_MODE_MASK),
                                    (kal_uint8)(CON1_OPA_MODE_SHIFT)
                                    );
}

//CON2
void bq24157_set_cv_vth(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON2), 
                                    (kal_uint8)(val),
									(kal_uint8)(CON2_CV_VTH_MASK),
									(kal_uint8)(CON2_CV_VTH_SHIFT)
                                    );
}

void bq24157_set_otg_pl(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON2), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON2_OTG_PL_MASK),
                                    (kal_uint8)(CON2_OTG_PL_SHIFT)
                                    );
}

void bq24157_set_otg_en(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON2), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON2_OTG_EN_MASK),
                                    (kal_uint8)(CON2_OTG_EN_SHIFT)
                                    );
}

//CON3
kal_uint32 bq24157_get_vender_code(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

	ret=bq24157_read_interface( 	(kal_uint8)(bq24157_CON3), 
                                    (&val),
                                    (kal_uint8)(CON3_VENDER_CODE_MASK),
                                    (kal_uint8)(CON3_VENDER_CODE_SHIFT)
                                    );
    return val;
}

kal_uint32 bq24157_get_pin(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

	ret=bq24157_read_interface( 	(kal_uint8)(bq24157_CON3), 
                                    (&val),
                                    (kal_uint8)(CON3_PIN_MASK),
                                    (kal_uint8)(CON3_PIN_SHIFT)
                                    );
    return val;
}

kal_uint32 bq24157_get_revision(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

	ret=bq24157_read_interface( 	(kal_uint8)(bq24157_CON3), 
                                    (&val),
                                    (kal_uint8)(CON3_REVISION_MASK),
                                    (kal_uint8)(CON3_REVISION_SHIFT)
                                    );
    return val;
}

//CON4
void bq24157_set_reset(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON4), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON4_RESET_MASK),
                                    (kal_uint8)(CON4_RESET_SHIFT)
                                    );
}

void bq24157_set_ac_charging_current(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON4), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON4_I_CHR_MASK),
                                    (kal_uint8)(CON4_I_CHR_SHIFT)
                                    );
}

void bq24157_set_termination_current(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON4), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON4_I_TERM_MASK),
                                    (kal_uint8)(CON4_I_TERM_SHIFT)
                                    );
}

//CON5
void bq24157_set_low_chg(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON5), 
                                    (kal_uint8)(val),
									(kal_uint8)(CON5_LOW_CHG_MASK),
									(kal_uint8)(CON5_LOW_CHG_SHIFT)
                                    );
}

kal_uint32 bq24157_get_dpm_status(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

	ret=bq24157_read_interface( 	(kal_uint8)(bq24157_CON5), 
                                    (&val),
									(kal_uint8)(CON5_DPM_STATUS_MASK),
									(kal_uint8)(CON5_DPM_STATUS_SHIFT)
                                    );
    return val;
}

kal_uint32 bq24157_get_cd_status(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

	ret=bq24157_read_interface( 	(kal_uint8)(bq24157_CON5), 
                                    (&val),
									(kal_uint8)(CON5_CD_STATUS_MASK),
									(kal_uint8)(CON5_CD_STATUS_SHIFT)
                                    );
    return val;
}

void bq24157_set_vsreg(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON5), 
                                    (kal_uint8)(val),
									(kal_uint8)(CON5_VSREG_MASK),
									(kal_uint8)(CON5_VSREG_SHIFT)
                                    );
}

//CON6
void bq24157_set_mchrg(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON6), 
                                    (kal_uint8)(val),
									(kal_uint8)(CON6_MCHRG_MASK),
									(kal_uint8)(CON6_MCHRG_SHIFT)
                                    );
}

void bq24157_set_mreg(kal_uint32 val)
{
    kal_uint32 ret=0;    

	ret=bq24157_config_interface(	(kal_uint8)(bq24157_CON6), 
                                    (kal_uint8)(val),
									(kal_uint8)(CON6_MREG_MASK),
									(kal_uint8)(CON6_MREG_SHIFT)
                                    );
}

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
void bq24157_dump_register(void)
{
    int i=0;
    printk("[bq24157] ");
    for (i=0;i<bq24157_REG_NUM;i++)
    {
        bq24157_read_byte(i, &bq24157_reg[i]);
        printk("[0x%x]=0x%x ", i, bq24157_reg[i]);        
    }
    printk("\n");
}

extern int g_enable_high_vbat_spec;
extern int g_pmic_cid;

void bq24157_hw_init(void)
{	
    if(g_enable_high_vbat_spec == 1)
    {
        if(g_pmic_cid == 0x1020)
        {
            printk("[bq24157_hw_init] (0x06,0x70) because 0x1020\n");
            bq24157_config_interface_liao(0x06,0x70); // set ISAFE
        }
        else
        {
            printk("[bq24157_hw_init] (0x06,0x77)\n");
            bq24157_config_interface_liao(0x06,0x77); // set ISAFE and HW CV point (4.34)
        }
    }
    else
    {
        printk("[bq24157_hw_init] (0x06,0x70) \n");
        bq24157_config_interface_liao(0x06,0x70); // set ISAFE
    }
    
//<2013/1/21-20698-jessicatseng, [Pelican] To turn off charging IC is controlled by GPIO, not IC register
    bq24157_config_interface_liao(0x40, 0x80);
//>2013/1/21-20698-jessicatseng    
}

static int bq24157_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
    int err=0; 

    printk("[bq24157_driver_probe] \n");

    if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }    
    memset(new_client, 0, sizeof(struct i2c_client));

    new_client = client;    

    //---------------------
	bq24157_hw_init();
	bq24157_dump_register();
	g_bq24157_rdy_flag = 1;
	
    return 0;                                                                                       

exit:
    return err;

}

/**********************************************************
  *
  *   [platform_driver API] 
  *
  *********************************************************/
kal_uint8 g_reg_value_bq24157=0;
static ssize_t show_bq24157_access(struct device *dev,struct device_attribute *attr, char *buf)
{
	printk("[show_bq24157_access] 0x%x\n", g_reg_value_bq24157);
	return sprintf(buf, "%u\n", g_reg_value_bq24157);
}
static ssize_t store_bq24157_access(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int ret=0;
    char *pvalue = NULL;
    unsigned int reg_value = 0;
    unsigned int reg_address = 0;
    
	printk("[store_bq24157_access] \n");
    
    if(buf != NULL && size != 0)
    {
		printk("[store_bq24157_access] buf is %s and size is %d \n",buf,size);
        reg_address = simple_strtoul(buf,&pvalue,16);
        
        if(size > 3)
        {        
            reg_value = simple_strtoul((pvalue+1),NULL,16);        
			printk("[store_bq24157_access] write bq24157 reg 0x%x with value 0x%x !\n",reg_address,reg_value);
			ret=bq24157_config_interface(reg_address, reg_value, 0xFF, 0x0);
        }
        else
        {    
			ret=bq24157_read_interface(reg_address, &g_reg_value_bq24157, 0xFF, 0x0);
			printk("[store_bq24157_access] read bq24157 reg 0x%x with value 0x%x !\n",reg_address,g_reg_value_bq24157);
			printk("[store_bq24157_access] Please use \"cat bq24157_access\" to get value\r\n");
        }        
    }    
    return size;
}
static DEVICE_ATTR(bq24157_access, 0664, show_bq24157_access, store_bq24157_access); //664

static int bq24157_user_space_probe(struct platform_device *dev)	
{    
    int ret_device_file = 0;

	printk("******** bq24157_user_space_probe!! ********\n" );
    
	ret_device_file = device_create_file(&(dev->dev), &dev_attr_bq24157_access);
    
    return 0;
}

struct platform_device bq24157_user_space_device = {
    .name   = "bq24157-user",
    .id     = -1,
};

static struct platform_driver bq24157_user_space_driver = {
    .probe		= bq24157_user_space_probe,
    .driver     = {
        .name = "bq24157-user",
    },
};

#define BQ24157_BUSNUM 0
static struct i2c_board_info __initdata i2c_bq24157 = { I2C_BOARD_INFO("bq24157", (0xd4>>1))};

static int __init bq24157_init(void)
{    
    int ret=0;
    
	printk("[bq24157_init] init start\n");
    
	i2c_register_board_info(BQ24157_BUSNUM, &i2c_bq24157, 1);

	if(i2c_add_driver(&bq24157_driver)!=0)
    {
		printk("[bq24157_init] failed to register bq24157 i2c driver.\n");
    }
    else
    {
		printk("[bq24157_init] Success to register bq24157 i2c driver.\n");
    }

	// bq24157 user space access interface
	ret = platform_device_register(&bq24157_user_space_device);
    if (ret) {
		printk("****[bq24157_init] Unable to device register(%d)\n", ret);
        return ret;
    }    
    ret = platform_driver_register(&bq24157_user_space_driver);
    if (ret) {
		printk("****[bq24157_init] Unable to register driver (%d)\n", ret);
        return ret;
    }
    
    return 0;        
}

static void __exit bq24157_exit(void)
{
	i2c_del_driver(&bq24157_driver);
}

module_init(bq24157_init);
module_exit(bq24157_exit);
   
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C bq24157 Driver");
MODULE_AUTHOR("James Lo<james.lo@mediatek.com>");
