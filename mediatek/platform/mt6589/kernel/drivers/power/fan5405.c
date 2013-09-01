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

#include "fan5405.h"

/**********************************************************
  *
  *   [I2C Slave Setting] 
  *
  *********************************************************/
#define fan5405_SLAVE_ADDR_WRITE   0xD4
#define fan5405_SLAVE_ADDR_Read    0xD5

static struct i2c_client *new_client = NULL;
static const struct i2c_device_id fan5405_i2c_id[] = {{"fan5405",0},{}};   
unsigned int g_fan5405_rdy_flag = 0;
static int fan5405_driver_probe(struct i2c_client *client, const struct i2c_device_id *id);
extern int Enable_BATDRV_LOG;

static struct i2c_driver fan5405_driver = {
    .driver = {
        .name    = "fan5405",
    },
    .probe       = fan5405_driver_probe,
    .id_table    = fan5405_i2c_id,
};

/**********************************************************
  *
  *   [Global Variable] 
  *
  *********************************************************/
#define fan5405_REG_NUM 7  
kal_uint8 fan5405_reg[fan5405_REG_NUM] = {0};

static DEFINE_MUTEX(fan5405_i2c_access);
/**********************************************************
  *
  *   [I2C Function For Read/Write fan5405] 
  *
  *********************************************************/
int fan5405_read_byte(kal_uint8 cmd, kal_uint8 *returnData)
{
    char     cmd_buf[1]={0x00};
    char     readData = 0;
    int      ret=0;

    mutex_lock(&fan5405_i2c_access);
    
    //new_client->addr = ((new_client->addr) & I2C_MASK_FLAG) | I2C_WR_FLAG;    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_WR_FLAG | I2C_DIRECTION_FLAG;

    cmd_buf[0] = cmd;
    ret = i2c_master_send(new_client, &cmd_buf[0], (1<<8 | 1));
    if (ret < 0) 
    {    
        //new_client->addr = new_client->addr & I2C_MASK_FLAG;
        new_client->ext_flag=0;

        mutex_unlock(&fan5405_i2c_access);
        return 0;
    }
    
    readData = cmd_buf[0];
    *returnData = readData;

    // new_client->addr = new_client->addr & I2C_MASK_FLAG;
    new_client->ext_flag=0;
    
    mutex_unlock(&fan5405_i2c_access);    
    return 1;
}

int fan5405_write_byte(kal_uint8 cmd, kal_uint8 writeData)
{
    char    write_data[2] = {0};
    int     ret=0;
    
    mutex_lock(&fan5405_i2c_access);
    
    write_data[0] = cmd;
    write_data[1] = writeData;
    
    new_client->ext_flag=((new_client->ext_flag ) & I2C_MASK_FLAG ) | I2C_DIRECTION_FLAG;
    
    ret = i2c_master_send(new_client, write_data, 2);
    if (ret < 0) 
    {
       
        new_client->ext_flag=0;
        mutex_unlock(&fan5405_i2c_access);
        return 0;
    }
    
    new_client->ext_flag=0;
    mutex_unlock(&fan5405_i2c_access);
    return 1;
}

/**********************************************************
  *
  *   [Read / Write Function] 
  *
  *********************************************************/
kal_uint32 fan5405_read_interface (kal_uint8 RegNum, kal_uint8 *val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 fan5405_reg = 0;
    int ret = 0;

    printk("--------------------------------------------------\n");

    ret = fan5405_read_byte(RegNum, &fan5405_reg);
	if (Enable_BATDRV_LOG == 1) {
    printk("[fan5405_read_interface] Reg[%x]=0x%x\n", RegNum, fan5405_reg);
	}
    fan5405_reg &= (MASK << SHIFT);
    *val = (fan5405_reg >> SHIFT);    
	if (Enable_BATDRV_LOG == 1) {
    printk("[fan5405_read_interface] val=0x%x\n", *val);
	}
    return ret;
}

kal_uint32 fan5405_config_interface (kal_uint8 RegNum, kal_uint8 val, kal_uint8 MASK, kal_uint8 SHIFT)
{
    kal_uint8 fan5405_reg = 0;
    int ret = 0;

    printk("--------------------------------------------------\n");

    ret = fan5405_read_byte(RegNum, &fan5405_reg);
    printk("[fan5405_config_interface] Reg[%x]=0x%x\n", RegNum, fan5405_reg);
    
    fan5405_reg &= ~(MASK << SHIFT);
    fan5405_reg |= (val << SHIFT);

    ret = fan5405_write_byte(RegNum, fan5405_reg);
    printk("[fan5405_config_interface] write Reg[%x]=0x%x\n", RegNum, fan5405_reg);

    // Check
    //fan5405_read_byte(RegNum, &fan5405_reg);
    //printk("[fan5405_config_interface] Check Reg[%x]=0x%x\n", RegNum, fan5405_reg);

    return ret;
}

//write one register directly
kal_uint32 fan5405_config_interface_liao (kal_uint8 RegNum, kal_uint8 val)
{   
    int ret = 0;
    
    ret = fan5405_write_byte(RegNum, val);

    return ret;
}

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
//CON0----------------------------------------------------

void fan5405_set_tmr_rst(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON0), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON0_TMR_RST_MASK),
                                    (kal_uint8)(CON0_TMR_RST_SHIFT)
                                    );
}

kal_uint32 fan5405_get_otg_status(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=fan5405_read_interface(     (kal_uint8)(fan5405_CON0), 
                                    (&val),
                                    (kal_uint8)(CON0_OTG_MASK),
                                    (kal_uint8)(CON0_OTG_SHIFT)
                                    );
    return val;
}

void fan5405_set_en_stat(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON0), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON0_EN_STAT_MASK),
                                    (kal_uint8)(CON0_EN_STAT_SHIFT)
                                    );
}

kal_uint32 fan5405_get_chip_status(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=fan5405_read_interface(     (kal_uint8)(fan5405_CON0), 
                                    (&val),
                                    (kal_uint8)(CON0_STAT_MASK),
                                    (kal_uint8)(CON0_STAT_SHIFT)
                                    );
    return val;
}

kal_uint32 fan5405_get_boost_status(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=fan5405_read_interface(     (kal_uint8)(fan5405_CON0), 
                                    (&val),
                                    (kal_uint8)(CON0_BOOST_MASK),
                                    (kal_uint8)(CON0_BOOST_SHIFT)
                                    );
    return val;
}

kal_uint32 fan5405_get_fault_status(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=fan5405_read_interface(     (kal_uint8)(fan5405_CON0), 
                                    (&val),
                                    (kal_uint8)(CON0_FAULT_MASK),
                                    (kal_uint8)(CON0_FAULT_SHIFT)
                                    );
    return val;
}

//CON1----------------------------------------------------

void fan5405_set_input_charging_current(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_LIN_LIMIT_MASK),
                                    (kal_uint8)(CON1_LIN_LIMIT_SHIFT)
                                    );
}

void fan5405_set_v_low(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_LOW_V_MASK),
                                    (kal_uint8)(CON1_LOW_V_SHIFT)
                                    );
}

void fan5405_set_te(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_TE_MASK),
                                    (kal_uint8)(CON1_TE_SHIFT)
                                    );
}

void fan5405_set_ce(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_CE_MASK),
                                    (kal_uint8)(CON1_CE_SHIFT)
                                    );
}

void fan5405_set_hz_mode(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_HZ_MODE_MASK),
                                    (kal_uint8)(CON1_HZ_MODE_SHIFT)
                                    );
}

void fan5405_set_opa_mode(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON1), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON1_OPA_MODE_MASK),
                                    (kal_uint8)(CON1_OPA_MODE_SHIFT)
                                    );
}

//CON2----------------------------------------------------

void fan5405_set_oreg(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON2), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON2_OREG_MASK),
                                    (kal_uint8)(CON2_OREG_SHIFT)
                                    );
}

void fan5405_set_otg_pl(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON2), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON2_OTG_PL_MASK),
                                    (kal_uint8)(CON2_OTG_PL_SHIFT)
                                    );
}

void fan5405_set_otg_en(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON2), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON2_OTG_EN_MASK),
                                    (kal_uint8)(CON2_OTG_EN_SHIFT)
                                    );
}

//CON3----------------------------------------------------

kal_uint32 fan5405_get_vender_code(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=fan5405_read_interface(     (kal_uint8)(fan5405_CON3), 
                                    (&val),
                                    (kal_uint8)(CON3_VENDER_CODE_MASK),
                                    (kal_uint8)(CON3_VENDER_CODE_SHIFT)
                                    );
    return val;
}

kal_uint32 fan5405_get_pn(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=fan5405_read_interface(     (kal_uint8)(fan5405_CON3), 
                                    (&val),
                                    (kal_uint8)(CON3_PIN_MASK),
                                    (kal_uint8)(CON3_PIN_SHIFT)
                                    );
    return val;
}

kal_uint32 fan5405_get_revision(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=fan5405_read_interface(     (kal_uint8)(fan5405_CON3), 
                                    (&val),
                                    (kal_uint8)(CON3_REVISION_MASK),
                                    (kal_uint8)(CON3_REVISION_SHIFT)
                                    );
    return val;
}

//CON4----------------------------------------------------

void fan5405_set_reset(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON4), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON4_RESET_MASK),
                                    (kal_uint8)(CON4_RESET_SHIFT)
                                    );
}

void fan5405_set_iocharge(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON4), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON4_I_CHR_MASK),
                                    (kal_uint8)(CON4_I_CHR_SHIFT)
                                    );
}

void fan5405_set_iterm(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON4), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON4_I_TERM_MASK),
                                    (kal_uint8)(CON4_I_TERM_SHIFT)
                                    );
}

//CON5----------------------------------------------------

void fan5405_set_dis_vreg(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON5), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON5_DIS_VREG_MASK),
                                    (kal_uint8)(CON5_DIS_VREG_SHIFT)
                                    );
}

void fan5405_set_io_level(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON5), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON5_IO_LEVEL_MASK),
                                    (kal_uint8)(CON5_IO_LEVEL_SHIFT)
                                    );
}

kal_uint32 fan5405_get_sp_status(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=fan5405_read_interface(     (kal_uint8)(fan5405_CON5), 
                                    (&val),
                                    (kal_uint8)(CON5_SP_STATUS_MASK),
                                    (kal_uint8)(CON5_SP_STATUS_SHIFT)
                                    );
    return val;
}

kal_uint32 fan5405_get_en_level(void)
{
    kal_uint32 ret=0;
    kal_uint8 val=0;

    ret=fan5405_read_interface(     (kal_uint8)(fan5405_CON5), 
                                    (&val),
                                    (kal_uint8)(CON5_EN_LEVEL_MASK),
                                    (kal_uint8)(CON5_EN_LEVEL_SHIFT)
                                    );
    return val;
}

void fan5405_set_vsp(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON5), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON5_VSP_MASK),
                                    (kal_uint8)(CON5_VSP_SHIFT)
                                    );
}

//CON6----------------------------------------------------

void fan5405_set_i_safe(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON6), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON6_ISAFE_MASK),
                                    (kal_uint8)(CON6_ISAFE_SHIFT)
                                    );
}

void fan5405_set_v_safe(kal_uint32 val)
{
    kal_uint32 ret=0;    

    ret=fan5405_config_interface(   (kal_uint8)(fan5405_CON6), 
                                    (kal_uint8)(val),
                                    (kal_uint8)(CON6_VSAFE_MASK),
                                    (kal_uint8)(CON6_VSAFE_SHIFT)
                                    );
}

/**********************************************************
  *
  *   [Internal Function] 
  *
  *********************************************************/
void fan5405_dump_register(void)
{
    int i=0;
    printk("[fan5405] ");
    for (i=0;i<fan5405_REG_NUM;i++)
    {
        fan5405_read_byte(i, &fan5405_reg[i]);
        printk("[0x%x]=0x%x ", i, fan5405_reg[i]);        
    }
    printk("\n");
}

extern int g_enable_high_vbat_spec;
extern int g_pmic_cid;

void fan5405_hw_init(void)
{    
    if(g_enable_high_vbat_spec == 1)
    {
        if(g_pmic_cid == 0x1020)
        {
            printk("[fan5405_hw_init] (0x06,0x70) because 0x1020\n");
            fan5405_config_interface_liao(0x06,0x70); // set ISAFE
        }
        else
        {
            printk("[fan5405_hw_init] (0x06,0x77)\n");
            fan5405_config_interface_liao(0x06,0x77); // set ISAFE and HW CV point (4.34)
        }
    }
    else
    {
        printk("[fan5405_hw_init] (0x06,0x70) \n");
        fan5405_config_interface_liao(0x06,0x70); // set ISAFE
    }
}

static int fan5405_driver_probe(struct i2c_client *client, const struct i2c_device_id *id) 
{             
    int err=0; 

    printk("[fan5405_driver_probe] \n");

    if (!(new_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL))) {
        err = -ENOMEM;
        goto exit;
    }    
    memset(new_client, 0, sizeof(struct i2c_client));

    new_client = client;    

    //---------------------
    fan5405_hw_init();
    fan5405_dump_register();
	g_fan5405_rdy_flag = 1;

    return 0;                                                                                       

exit:
    return err;

}

/**********************************************************
  *
  *   [platform_driver API] 
  *
  *********************************************************/
kal_uint8 g_reg_value_fan5405=0;
static ssize_t show_fan5405_access(struct device *dev,struct device_attribute *attr, char *buf)
{
    printk("[show_fan5405_access] 0x%x\n", g_reg_value_fan5405);
    return sprintf(buf, "%u\n", g_reg_value_fan5405);
}
static ssize_t store_fan5405_access(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    int ret=0;
    char *pvalue = NULL;
    unsigned int reg_value = 0;
    unsigned int reg_address = 0;
    
    printk("[store_fan5405_access] \n");
    
    if(buf != NULL && size != 0)
    {
        printk("[store_fan5405_access] buf is %s and size is %d \n",buf,size);
        reg_address = simple_strtoul(buf,&pvalue,16);
        
        if(size > 3)
        {        
            reg_value = simple_strtoul((pvalue+1),NULL,16);        
            printk("[store_fan5405_access] write fan5405 reg 0x%x with value 0x%x !\n",reg_address,reg_value);
            ret=fan5405_config_interface(reg_address, reg_value, 0xFF, 0x0);
        }
        else
        {    
            ret=fan5405_read_interface(reg_address, &g_reg_value_fan5405, 0xFF, 0x0);
            printk("[store_fan5405_access] read fan5405 reg 0x%x with value 0x%x !\n",reg_address,g_reg_value_fan5405);
            printk("[store_fan5405_access] Please use \"cat fan5405_access\" to get value\r\n");
        }        
    }    
    return size;
}
static DEVICE_ATTR(fan5405_access, 0664, show_fan5405_access, store_fan5405_access); //664

static int fan5405_user_space_probe(struct platform_device *dev)    
{    
    int ret_device_file = 0;

    printk("******** fan5405_user_space_probe!! ********\n" );
    
    ret_device_file = device_create_file(&(dev->dev), &dev_attr_fan5405_access);
    
    return 0;
}

struct platform_device fan5405_user_space_device = {
    .name   = "fan5405-user",
    .id     = -1,
};

static struct platform_driver fan5405_user_space_driver = {
    .probe      = fan5405_user_space_probe,
    .driver     = {
        .name = "fan5405-user",
    },
};

#define FAN5405_BUSNUM 6
static struct i2c_board_info __initdata i2c_fan5405 = { I2C_BOARD_INFO("fan5405", (0xd4>>1))};

static int __init fan5405_init(void)
{    
    int ret=0;
    
    printk("[fan5405_init] init start\n");
    
    i2c_register_board_info(FAN5405_BUSNUM, &i2c_fan5405, 1);

    if(i2c_add_driver(&fan5405_driver)!=0)
    {
        printk("[fan5405_init] failed to register fan5405 i2c driver.\n");
    }
    else
    {
        printk("[fan5405_init] Success to register fan5405 i2c driver.\n");
    }

    // fan5405 user space access interface
    ret = platform_device_register(&fan5405_user_space_device);
    if (ret) {
        printk("****[fan5405_init] Unable to device register(%d)\n", ret);
        return ret;
    }    
    ret = platform_driver_register(&fan5405_user_space_driver);
    if (ret) {
        printk("****[fan5405_init] Unable to register driver (%d)\n", ret);
        return ret;
    }
    
    return 0;        
}

static void __exit fan5405_exit(void)
{
    i2c_del_driver(&fan5405_driver);
}

module_init(fan5405_init);
module_exit(fan5405_exit);
   
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C fan5405 Driver");
MODULE_AUTHOR("James Lo<james.lo@mediatek.com>");
