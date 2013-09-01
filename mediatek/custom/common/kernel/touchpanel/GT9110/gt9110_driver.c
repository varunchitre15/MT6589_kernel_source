#include "tpd.h"
#include "tpd_custom_gt9110.h"
#ifndef TPD_NO_GPIO
#include "cust_gpio_usage.h"
#endif
#ifdef TPD_PROXIMITY
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
#endif

extern struct tpd_device *tpd;

static int tpd_flag = 0;
static int tpd_halt = 0;
static struct task_struct *thread = NULL;
static DECLARE_WAIT_QUEUE_HEAD(waiter);

#ifdef TPD_HAVE_BUTTON
static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

#if GTP_HAVE_TOUCH_KEY
const u16 touch_key_array[] = { KEY_MENU, KEY_HOMEPAGE, KEY_BACK, KEY_SEARCH };
#define GTP_MAX_KEY_NUM ( sizeof( touch_key_array )/sizeof( touch_key_array[0] ) )
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
//static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_calmat_driver[8]            = {0};
static int tpd_def_calmat_local_normal[8]  = TPD_CALIBRATION_MATRIX_ROTATION_NORMAL;
static int tpd_def_calmat_local_factory[8] = TPD_CALIBRATION_MATRIX_ROTATION_FACTORY;
#endif

s32 gtp_send_cfg(struct i2c_client *client);
static void tpd_eint_interrupt_handler(void);
static int touch_event_handler(void *unused);
static int tpd_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info);
static int tpd_i2c_remove(struct i2c_client *client);
extern void mt65xx_eint_unmask(unsigned int line);
extern void mt65xx_eint_mask(unsigned int line);
extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
                                     kal_bool auto_umask);

#if GTP_CREATE_WR_NODE
extern s32 init_wr_node(struct i2c_client *);
extern void uninit_wr_node(void);
#endif

#if GTP_ESD_PROTECT
#define TPD_ESD_CHECK_CIRCLE        2000
static struct delayed_work gtp_esd_check_work;
static struct workqueue_struct *gtp_esd_check_workqueue = NULL;
static void gtp_esd_check_func(struct work_struct *);
#endif

#ifdef TPD_PROXIMITY
#define TPD_PROXIMITY_VALID_REG                   0x814E
#define TPD_PROXIMITY_ENABLE_REG                  0x8042
static u8 tpd_proximity_flag = 0;
static u8 tpd_proximity_detect = 1;//0-->close ; 1--> far away
#endif

struct i2c_client *i2c_client_point = NULL;
static const struct i2c_device_id tpd_i2c_id[] = {{"gt9110", 0}, {}};
static unsigned short force[] = {0, 0xBA, I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short *const forces[] = { force, NULL };
//static struct i2c_client_address_data addr_data = { .forces = forces,};
static struct i2c_board_info __initdata i2c_tpd = { I2C_BOARD_INFO("gt9110", (0xBA >> 1))};
static struct i2c_driver tpd_i2c_driver =
{
    .probe = tpd_i2c_probe,
    .remove = tpd_i2c_remove,
    .detect = tpd_i2c_detect,
    .driver.name = "gt9110",
    .id_table = tpd_i2c_id,
    .address_list = (const unsigned short *) forces,
};

static u8 config[GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH]
    = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};
#pragma pack(1)
typedef struct
{
    u16 pid;                 //product id   //
    u16 vid;                 //version id   //
} st_tpd_info;
#pragma pack()

st_tpd_info tpd_info;
u8 int_type = 0;
u32 abs_x_max = 0;
u32 abs_y_max = 0;
u8 gtp_rawdiff_mode = 0;
u8 cfg_len = 0;

/* proc file system */
s32 i2c_read_bytes(struct i2c_client *client, u16 addr, u8 *rxbuf, int len);
s32 i2c_write_bytes(struct i2c_client *client, u16 addr, u8 *txbuf, int len);
static struct proc_dir_entry *gt9110_config_proc = NULL;

#define VELOCITY_CUSTOM
#ifdef VELOCITY_CUSTOM
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>

#ifndef TPD_VELOCITY_CUSTOM_X
#define TPD_VELOCITY_CUSTOM_X 10
#endif
#ifndef TPD_VELOCITY_CUSTOM_Y
#define TPD_VELOCITY_CUSTOM_Y 10
#endif

// for magnify velocity********************************************
#define TOUCH_IOC_MAGIC 'A'

#define TPD_GET_VELOCITY_CUSTOM_X _IO(TOUCH_IOC_MAGIC,0)
#define TPD_GET_VELOCITY_CUSTOM_Y _IO(TOUCH_IOC_MAGIC,1)

int g_v_magnify_x = TPD_VELOCITY_CUSTOM_X;
int g_v_magnify_y = TPD_VELOCITY_CUSTOM_Y;
static int tpd_misc_open(struct inode *inode, struct file *file)
{
    return nonseekable_open(inode, file);
}

static int tpd_misc_release(struct inode *inode, struct file *file)
{
    return 0;
}

static long tpd_unlocked_ioctl(struct file *file, unsigned int cmd,
                               unsigned long arg)
{
    //char strbuf[256];
    void __user *data;

    long err = 0;

    if (_IOC_DIR(cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
    {
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }

    if (err)
    {
        printk("tpd: access error: %08X, (%2d, %2d)\n", cmd, _IOC_DIR(cmd), _IOC_SIZE(cmd));
        return -EFAULT;
    }

    switch (cmd)
    {
        case TPD_GET_VELOCITY_CUSTOM_X:
            data = (void __user *) arg;

            if (data == NULL)
            {
                err = -EINVAL;
                break;
            }

            if (copy_to_user(data, &g_v_magnify_x, sizeof(g_v_magnify_x)))
            {
                err = -EFAULT;
                break;
            }

            break;

        case TPD_GET_VELOCITY_CUSTOM_Y:
            data = (void __user *) arg;

            if (data == NULL)
            {
                err = -EINVAL;
                break;
            }

            if (copy_to_user(data, &g_v_magnify_y, sizeof(g_v_magnify_y)))
            {
                err = -EFAULT;
                break;
            }

            break;

        default:
            printk("tpd: unknown IOCTL: 0x%08x\n", cmd);
            err = -ENOIOCTLCMD;
            break;

    }

    return err;
}


static struct file_operations tpd_fops =
{
//	.owner = THIS_MODULE,
    .open = tpd_misc_open,
    .release = tpd_misc_release,
    .unlocked_ioctl = tpd_unlocked_ioctl,
};
/*----------------------------------------------------------------------------*/
static struct miscdevice tpd_misc_device =
{
    .minor = MISC_DYNAMIC_MINOR,
    .name = "touch",
    .fops = &tpd_fops,
};

//**********************************************
#endif

static int tpd_i2c_detect(struct i2c_client *client, struct i2c_board_info *info)
{
    strcpy(info->type, "mtk-tpd");
    return 0;
}

#ifdef TPD_PROXIMITY
static s32 tpd_get_ps_value(void)
{
    return tpd_proximity_detect;
}

static s32 tpd_enable_ps(s32 enable)
{
    u8  state;
    s32 ret = -1;

    if (enable)
    {
        state = 1;
        tpd_proximity_flag = 1;
        GTP_INFO("TPD proximity function to be on.");
    }
    else
    {
        state = 0;
        tpd_proximity_flag = 0;
        GTP_INFO("TPD proximity function to be off.");
    }

    ret = i2c_write_bytes(i2c_client_point, TPD_PROXIMITY_ENABLE_REG, &state, 1);

    if (ret < 0)
    {
        GTP_ERROR("TPD %s proximity cmd failed.", state ? "enable" : "disable");
        return ret;
    }

    GTP_INFO("TPD proximity function %s success.", state ? "enable" : "disable");
    return 0;
}

s32 tpd_ps_operate(void *self, u32 command, void *buff_in, s32 size_in,
                   void *buff_out, s32 size_out, s32 *actualout)
{
    s32 err = 0;
    s32 value;
    hwm_sensor_data *sensor_data;

    switch (command)
    {
        case SENSOR_DELAY:
            if ((buff_in == NULL) || (size_in < sizeof(int)))
            {
                GTP_ERROR("Set delay parameter error!");
                err = -EINVAL;
            }

            // Do nothing
            break;

        case SENSOR_ENABLE:
            if ((buff_in == NULL) || (size_in < sizeof(int)))
            {
                GTP_ERROR("Enable sensor parameter error!");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                err = tpd_enable_ps(value);
            }

            break;

        case SENSOR_GET_DATA:
            if ((buff_out == NULL) || (size_out < sizeof(hwm_sensor_data)))
            {
                GTP_ERROR("Get sensor data parameter error!");
                err = -EINVAL;
            }
            else
            {
                sensor_data = (hwm_sensor_data *)buff_out;
                sensor_data->values[0] = tpd_get_ps_value();
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }

            break;

        default:
            GTP_ERROR("proxmy sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}
#endif

static int gt9110_config_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    char *ptr = page;
    char temp_data[GTP_CONFIG_MAX_LENGTH + 2] = {0};
    int i;

    ptr += sprintf(ptr, "==== GT9110 config init value====\n");

    for (i = 0 ; i < GTP_CONFIG_MAX_LENGTH ; i++)
    {
        ptr += sprintf(ptr, "0x%02X ", config[i + 2]);

        if (i % 8 == 7)
            ptr += sprintf(ptr, "\n");
    }

    ptr += sprintf(ptr, "\n");

    ptr += sprintf(ptr, "==== GT9110 config real value====\n");
    i2c_read_bytes(i2c_client_point, GTP_REG_CONFIG_DATA, temp_data, GTP_CONFIG_MAX_LENGTH);

    for (i = 0 ; i < GTP_CONFIG_MAX_LENGTH ; i++)
    {
        ptr += sprintf(ptr, "0x%02X ", temp_data[i]);

        if (i % 8 == 7)
            ptr += sprintf(ptr, "\n");
    }
    
    i2c_read_bytes(i2c_client_point, GTP_REG_VERSION, temp_data, 6);
    ptr += sprintf(ptr, "PID: %c%c%c%c VID: 0x%02X%02X\n", temp_data[0],temp_data[1],temp_data[2],temp_data[3],temp_data[5],temp_data[4]);   
    
    i2c_read_bytes(i2c_client_point, 0x800D, temp_data, 4);
    ptr += sprintf(ptr, "Real VID: 0x%c%c%c%c\n", temp_data[0],temp_data[1],temp_data[2],temp_data[3]);

    *eof = 1;
    return (ptr - page);
}

static int gt9110_config_write_proc(struct file *file, const char *buffer, unsigned long count, void *data)
{
    s32 ret = 0;

    GTP_DEBUG("write count %ld\n", count);

    if (count > GTP_CONFIG_MAX_LENGTH)
    {
        GTP_ERROR("size not match [%d:%ld]\n", GTP_CONFIG_MAX_LENGTH, count);
        return -EFAULT;
    }

    if (copy_from_user(&config[2], buffer, count))
    {
        GTP_ERROR("copy from user fail\n");
        return -EFAULT;
    }

    ret = gtp_send_cfg(i2c_client_point);
    abs_x_max = (config[RESOLUTION_LOC + 1] << 8) + config[RESOLUTION_LOC];
    abs_y_max = (config[RESOLUTION_LOC + 3] << 8) + config[RESOLUTION_LOC + 2];
    int_type = (config[TRIGGER_LOC]) & 0x03;

    if (ret < 0)
    {
        GTP_ERROR("send config failed.");
    }

    return count;
}

int i2c_read_bytes(struct i2c_client *client, u16 addr, u8 *rxbuf, int len)
{
    u8 buffer[GTP_ADDR_LENGTH];
    u8 retry;
    u16 left = len;
    u16 offset = 0;

    struct i2c_msg msg[2] =
    {
        {            
            .addr = (client->addr & I2C_MASK_FLAG),
            //.addr = ((client->addr &I2C_MASK_FLAG) | (I2C_PUSHPULL_FLAG)),
            .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG),
            .flags = 0,
            .buf = buffer,
            .len = GTP_ADDR_LENGTH,
            .timing = I2C_MASTER_CLOCK
        },
        {
            .addr = (client->addr & I2C_MASK_FLAG),
            //.addr = ((client->addr &I2C_MASK_FLAG) | (I2C_PUSHPULL_FLAG)),
            .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG),
            .flags = I2C_M_RD,
            .timing = I2C_MASTER_CLOCK
        },
    };

    if (rxbuf == NULL)
        return -1;

    GTP_DEBUG("i2c_read_bytes to device %02X address %04X len %d\n", client->addr, addr, len);

    while (left > 0)
    {
        buffer[0] = ((addr + offset) >> 8) & 0xFF;
        buffer[1] = (addr + offset) & 0xFF;

        msg[1].buf = &rxbuf[offset];

        if (left > MAX_TRANSACTION_LENGTH)
        {
            msg[1].len = MAX_TRANSACTION_LENGTH;
            left -= MAX_TRANSACTION_LENGTH;
            offset += MAX_TRANSACTION_LENGTH;
        }
        else
        {
            msg[1].len = left;
            left = 0;
        }

        retry = 0;

        while (i2c_transfer(client->adapter, &msg[0], 2) != 2)
        {
            retry++;

            if (retry == 20)
            {
                GTP_ERROR("I2C read 0x%X length=%d failed\n", addr + offset, len);
                return -1;
            }
        }
    }

    return 0;
}

s32 gtp_i2c_read(struct i2c_client *client, u8 *buf, s32 len)
{
    s32 ret = -1;
    u16 addr = (buf[0] << 8) + buf[1];

    ret = i2c_read_bytes(client, addr, &buf[2], len - 2);

    if (!ret)
    {
        return 2;
    }
    else
    {
        gtp_reset_guitar(client, 20);
        return ret;
    }
}

int i2c_write_bytes(struct i2c_client *client, u16 addr, u8 *txbuf, int len)
{
    u8 buffer[MAX_TRANSACTION_LENGTH];
    u16 left = len;
    u16 offset = 0;
    u8 retry = 0;

    struct i2c_msg msg =
    {
        .addr = (client->addr & I2C_MASK_FLAG),
        //.addr = ((client->addr &I2C_MASK_FLAG) | (I2C_PUSHPULL_FLAG)),
        .ext_flag = (client->ext_flag | I2C_ENEXT_FLAG),
        .flags = 0,
        .buf = buffer,
        .timing = I2C_MASTER_CLOCK,
    };


    if (txbuf == NULL)
        return -1;

    GTP_DEBUG("i2c_write_bytes to device %02X address %04X len %d\n", client->addr, addr, len);

    while (left > 0)
    {
        retry = 0;

        buffer[0] = ((addr + offset) >> 8) & 0xFF;
        buffer[1] = (addr + offset) & 0xFF;

        if (left > MAX_I2C_TRANSFER_SIZE)
        {
            memcpy(&buffer[GTP_ADDR_LENGTH], &txbuf[offset], MAX_I2C_TRANSFER_SIZE);
            msg.len = MAX_TRANSACTION_LENGTH;
            left -= MAX_I2C_TRANSFER_SIZE;
            offset += MAX_I2C_TRANSFER_SIZE;
        }
        else
        {
            memcpy(&buffer[GTP_ADDR_LENGTH], &txbuf[offset], left);
            msg.len = left + GTP_ADDR_LENGTH;
            left = 0;
        }

        //GTP_DEBUG("byte left %d offset %d\n", left, offset);

        while (i2c_transfer(client->adapter, &msg, 1) != 1)
        {
            retry++;

            if (retry == 20)
            {
                GTP_ERROR("I2C write 0x%X%X length=%d failed\n", buffer[0], buffer[1], len);
                return -1;
            }


        }
    }

    return 0;
}

s32 gtp_i2c_write(struct i2c_client *client, u8 *buf, s32 len)
{
    s32 ret = -1;
    u16 addr = (buf[0] << 8) + buf[1];

    ret = i2c_write_bytes(client, addr, &buf[2], len - 2);

    if (!ret)
    {
        return 1;
    }
    else
    {
        gtp_reset_guitar(client, 20);
        return ret;
    }
}



/*******************************************************
Function:
	Send config Function.

Input:
	client:	i2c client.

Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
s32 gtp_send_cfg(struct i2c_client *client)
{
    s32 ret = 0;
#if GTP_DRIVER_SEND_CFG
    s32 retry = 0;

    for (retry = 0; retry < 5; retry++)
    {
        ret = gtp_i2c_write(client, config , GTP_CONFIG_MAX_LENGTH + GTP_ADDR_LENGTH);

        if (ret > 0)
        {
            break;
        }
    }

#endif

    return ret;
}


/*******************************************************
Function:
	Read goodix touchscreen version function.

Input:
	client:	i2c client struct.
	version:address to store version info

Output:
	Executive outcomes.0---succeed.
*******************************************************/
s32 gtp_read_version(struct i2c_client *client, u16 *version)
{
    s32 ret = -1;
    s32 i;
    u8 buf[8] = {GTP_REG_VERSION >> 8, GTP_REG_VERSION & 0xff};

    GTP_DEBUG_FUNC();

    ret = gtp_i2c_read(client, buf, sizeof(buf) + GTP_ADDR_LENGTH);

    if (ret < 0)
    {
        GTP_ERROR("GTP read version failed");
        return ret;
    }

    if (version)
    {
        *version = (buf[7] << 8) | buf[6];
    }

    tpd_info.vid = *version;
    tpd_info.pid = 0x00;

    for (i = 0; i < 4; i++)
    {
        if (buf[i + 2] < 0x30)break;

        tpd_info.pid |= ((buf[i + 2] - 0x30) << ((3 - i) * 4));
    }

    GTP_INFO("IC VERSION:%c%c%c%c_%02x%02x",
             buf[2], buf[3], buf[4], buf[5], buf[7], buf[6]);

    return ret;
}
/*******************************************************
Function:
	GTP initialize function.

Input:
	client:	i2c client private struct.

Output:
	Executive outcomes.0---succeed.
*******************************************************/
static s32 gtp_init_panel(struct i2c_client *client)
{
    s32 ret = -1;

#if GTP_DRIVER_SEND_CFG
    s32 i;
    u8 check_sum = 0;
    u8 rd_cfg_buf[16];

    u8 cfg_info_group1[] = CTP_CFG_GROUP1;
    u8 cfg_info_group2[] = CTP_CFG_GROUP2;
    u8 cfg_info_group3[] = CTP_CFG_GROUP3;
    u8 *send_cfg_buf[3] = {cfg_info_group1, cfg_info_group2, cfg_info_group3};
    u8 cfg_info_len[3] = {sizeof(cfg_info_group1) / sizeof(cfg_info_group1[0]),
                          sizeof(cfg_info_group2) / sizeof(cfg_info_group2[0]),
                          sizeof(cfg_info_group3) / sizeof(cfg_info_group3[0])
                         };

    for (i = 0; i < 3; i++)
    {
        if (cfg_info_len[i] > cfg_len)
        {
            cfg_len = cfg_info_len[i];
        }
    }

    GTP_DEBUG("len1=%d,len2=%d,len3=%d,get_len=%d", cfg_info_len[0], cfg_info_len[1], cfg_info_len[2], cfg_len);

    if ((!cfg_info_len[1]) && (!cfg_info_len[2]))
    {
        rd_cfg_buf[GTP_ADDR_LENGTH] = 0;
    }
    else
    {
        rd_cfg_buf[0] = GTP_REG_SENSOR_ID >> 8;
        rd_cfg_buf[1] = GTP_REG_SENSOR_ID & 0xff;
        ret = gtp_i2c_read(client, rd_cfg_buf, 3);

        if (ret < 0)
        {
            GTP_ERROR("Read SENSOR ID failed,default use group1 config!");
            rd_cfg_buf[GTP_ADDR_LENGTH] = 0;
        }

        rd_cfg_buf[GTP_ADDR_LENGTH] &= 0x03;
    }

    GTP_INFO("SENSOR ID:%d", rd_cfg_buf[GTP_ADDR_LENGTH]);
    memset(&config[GTP_ADDR_LENGTH], 0, GTP_CONFIG_MAX_LENGTH);
    memcpy(&config[GTP_ADDR_LENGTH], send_cfg_buf[rd_cfg_buf[GTP_ADDR_LENGTH]], cfg_len);

#if GTP_CUSTOM_CFG
    config[RESOLUTION_LOC]     = (u8)GTP_MAX_WIDTH;
    config[RESOLUTION_LOC + 1] = (u8)(GTP_MAX_WIDTH >> 8);
    config[RESOLUTION_LOC + 2] = (u8)GTP_MAX_HEIGHT;
    config[RESOLUTION_LOC + 3] = (u8)(GTP_MAX_HEIGHT >> 8);

    if (GTP_INT_TRIGGER == 0)  //RISING
    {
        config[TRIGGER_LOC] &= 0xfe;
    }
    else if (GTP_INT_TRIGGER == 1)  //FALLING
    {
        config[TRIGGER_LOC] |= 0x01;
    }

#endif  //endif GTP_CUSTOM_CFG

    check_sum = 0;

    for (i = GTP_ADDR_LENGTH; i < cfg_len; i++)
    {
        check_sum += config[i];
    }

    config[cfg_len] = (~check_sum) + 1;

#else //else DRIVER NEED NOT SEND CONFIG

    if (cfg_len == 0)
    {
        cfg_len = GTP_CONFIG_MAX_LENGTH;
    }

    ret = gtp_i2c_read(client, config, cfg_len + GTP_ADDR_LENGTH);

    if (ret < 0)
    {
        GTP_ERROR("GTP read resolution & max_touch_num failed, use default value!");
        abs_x_max = GTP_MAX_WIDTH;
        abs_y_max = GTP_MAX_HEIGHT;
        int_type = GTP_INT_TRIGGER;
    }

#endif //endif GTP_DRIVER_SEND_CFG

    abs_x_max = (config[RESOLUTION_LOC + 1] << 8) + config[RESOLUTION_LOC];
    abs_y_max = (config[RESOLUTION_LOC + 3] << 8) + config[RESOLUTION_LOC + 2];
    int_type = (config[TRIGGER_LOC]) & 0x03;

    if ((!abs_x_max) || (!abs_y_max))
    {
        GTP_ERROR("GTP resolution & max_touch_num invalid, use default value!");
        abs_x_max = GTP_MAX_WIDTH;
        abs_y_max = GTP_MAX_HEIGHT;
    }

    ret = gtp_send_cfg(client);

    if (ret < 0)
    {
        GTP_ERROR("Send config error.");
    }

    GTP_DEBUG("X_MAX = %d,Y_MAX = %d,TRIGGER = 0x%02x", abs_x_max, abs_y_max, int_type);

    msleep(10);

    return 0;
}

static s8 gtp_i2c_test(struct i2c_client *client)
{

    u8 retry = 0;
    s8 ret = -1;
    u32 hw_info = 0;

    GTP_DEBUG_FUNC();

    while (retry++ < 5)
    {
        ret = i2c_read_bytes(client, GTP_REG_HW_INFO, (u8 *)&hw_info, sizeof(hw_info));

        if ((!ret) & (hw_info == 0x00900600))
        {
            return ret;
        }

        GTP_ERROR("GTP_REG_HW_INFO : %08X", hw_info);
        GTP_ERROR("GTP i2c test failed time %d.", retry);
        msleep(10);
    }

    return -1;
}

/*******************************************************
Function:
	Set INT pin as input for FW sync.

Note:
  If the INT is high, It means there is pull up resistor attached on the INT pin.
  Pull low the INT pin manaully for FW sync.
*******************************************************/
void gtp_int_sync()
{
    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
    msleep(50);
    GTP_GPIO_AS_INT(GTP_INT_PORT);
}

void gtp_reset_guitar(struct i2c_client *client, s32 ms)
{
    GTP_INFO("GTP RESET!\n");
    GTP_GPIO_OUTPUT(GTP_RST_PORT, 0);
    msleep(ms);
    GTP_GPIO_OUTPUT(GTP_INT_PORT, client->addr == 0x14);

    msleep(2);
    GTP_GPIO_OUTPUT(GTP_RST_PORT, 1);

    msleep(6);
    gtp_int_sync();
}

static int tpd_power_on(struct i2c_client *client)
{
    int ret = 0;
    int reset_count = 0;

reset_proc:
#ifdef MT6573
    // power on CTP
    //mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
    //mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
    //mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
#endif
#ifdef MT6575
    //power on, need confirm with SA
    //hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
    //hwPowerOn(MT65XX_POWER_LDO_VGP, VOL_1800, "TP");
#endif
#ifdef MT6577
    //power on, need confirm with SA
    //hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
    //hwPowerOn(MT65XX_POWER_LDO_VGP, VOL_1800, "TP");
#endif

    //MT6589
    //Power on
    hwPowerOn(MT65XX_POWER_LDO_VGP5, VOL_2800, "TP");

    gtp_reset_guitar(client, 20);

    ret = gtp_i2c_test(client);

    if (ret < 0)
    {
        GTP_ERROR("I2C communication ERROR!");

        if (reset_count < TPD_MAX_RESET_COUNT)
        {
            reset_count++;
            goto reset_proc;
        }
    }

#if GTP_FW_DOWNLOAD
    ret = gup_init_fw_proc(client);

    if (ret < 0)
    {
        GTP_ERROR("Create fw download thread error.");
    }

#endif
    return ret;
}

static s32 tpd_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    s32 err = 0;
    s32 ret = 0;

    //u16 version_info;
#if GTP_HAVE_TOUCH_KEY
    s32 idx = 0;
#endif
#ifdef TPD_PROXIMITY
    struct hwmsen_object obj_ps;
#endif

    i2c_client_point = client;
    ret = tpd_power_on(client);

    if (ret < 0)
    {
        GTP_ERROR("I2C communication ERROR!");
    }

#if GTP_AUTO_UPDATE
    ret = gup_init_update_proc(client);

    if (ret < 0)
    {
        GTP_ERROR("Create update thread error.");
    }

#endif



#ifdef VELOCITY_CUSTOM

    if ((err = misc_register(&tpd_misc_device)))
    {
        printk("mtk_tpd: tpd_misc_device register failed\n");
    }

#endif

    ret = gtp_init_panel(client);

    if (ret < 0)
    {
        GTP_ERROR("GTP init panel failed.");
    }

    /*    
    ret = gtp_read_version(client, &version_info);

    if (ret < 0)
    {
        GTP_ERROR("Read version failed.");
    }
    */

    // Create proc file system
    gt9110_config_proc = create_proc_entry(GT9110_CONFIG_PROC_FILE, 0666, NULL);

    if (gt9110_config_proc == NULL)
    {
        GTP_ERROR("create_proc_entry %s failed\n", GT9110_CONFIG_PROC_FILE);
    }
    else
    {
        gt9110_config_proc->read_proc = gt9110_config_read_proc;
        gt9110_config_proc->write_proc = gt9110_config_write_proc;
    }

#if GTP_CREATE_WR_NODE
    init_wr_node(client);
#endif

    thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);

    if (IS_ERR(thread))
    {
        err = PTR_ERR(thread);
        GTP_INFO(TPD_DEVICE " failed to create kernel thread: %d\n", err);
    }

#if GTP_HAVE_TOUCH_KEY

    for (idx = 0; idx < GTP_MAX_KEY_NUM; idx++)
    {
        input_set_capability(tpd->dev, EV_KEY, touch_key_array[idx]);
    }

#endif

    /*
    #ifndef TPD_RESET_ISSUE_WORKAROUND
        mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
        msleep(10);
    #endif
    */

    // set INT mode
    mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
    mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
    mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_DISABLE);

    msleep(50);

    mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
    mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);

    if (!int_type)
    {
        mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_POLARITY_HIGH, tpd_eint_interrupt_handler, 1);
    }
    else
    {
        mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, tpd_eint_interrupt_handler, 1);
    }

    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    /*
    #ifndef TPD_RESET_ISSUE_WORKAROUND
        mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ONE);
    #endif
    */

#ifdef TPD_PROXIMITY
    //obj_ps.self = cm3623_obj;
    obj_ps.polling = 0;         //0--interrupt mode;1--polling mode;
    obj_ps.sensor_operate = tpd_ps_operate;

    if ((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
    {
        GTP_ERROR("hwmsen attach fail, return:%d.", err);
    }

#endif

#if GTP_ESD_PROTECT
    INIT_DELAYED_WORK(&gtp_esd_check_work, gtp_esd_check_func);
    gtp_esd_check_workqueue = create_workqueue("gtp_esd_check");
    queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
#endif

    tpd_load_status = 1;

    return 0;
}

static void tpd_eint_interrupt_handler(void)
{
    TPD_DEBUG_PRINT_INT;
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
}
static int tpd_i2c_remove(struct i2c_client *client)
{
#if GTP_CREATE_WR_NODE
    uninit_wr_node();
#endif

#if GTP_ESD_PROTECT
    destroy_workqueue(gtp_esd_check_workqueue);
#endif

    return 0;
}

#if GTP_ESD_PROTECT
static void force_reset_guitar(void)
{
    s32 i;
    s32 ret;

    GTP_INFO("force_reset_guitar\n");

    //Power off TP
#if (defined(MT6575) || defined(mt6577))
    hwPowerDown(MT65XX_POWER_LDO_VGP2, "TP");
    msleep(30);
    //Power on TP
    hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
    msleep(30);
#else
    //Power off TP
    mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ZERO);
    msleep(30);
    //Power on TP
    mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
    msleep(30);
#endif

    for (i = 0; i < 5; i++)
    {
        //Reset Guitar
        gtp_reset_guitar(i2c_client_point, 20);

        //Send config
        ret = gtp_send_cfg(i2c_client_point);

        if (ret < 0)
        {
            continue;
        }

        break;
    }

}

static void gtp_esd_check_func(struct work_struct *work)
{
    int i;
    int ret = -1;
    u8 test[3] = {GTP_REG_CONFIG_DATA >> 8, GTP_REG_CONFIG_DATA & 0xff};

    if (tpd_halt)
    {
        return;
    }

    for (i = 0; i < 3; i++)
    {
        ret = gtp_i2c_read(i2c_client_point, test, 3);

        if (ret > 0)
        {
            break;
        }
    }

    if (i >= 3)
    {
        force_reset_guitar();
    }

    if (!tpd_halt)
    {
        queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
    }

    return;
}
#endif

static void tpd_down(s32 x, s32 y, s32 size, s32 id)
{
    if ((!size) && (!id))
    {
        input_report_abs(tpd->dev, ABS_MT_PRESSURE, 1);
        input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 1);
    }
    else
    {
        input_report_abs(tpd->dev, ABS_MT_PRESSURE, size);
        input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, size);
        /* track id Start 0 */
        input_report_abs(tpd->dev, ABS_MT_TRACKING_ID, id);
    }

    input_report_key(tpd->dev, BTN_TOUCH, 1);
    input_report_abs(tpd->dev, ABS_MT_POSITION_X, x);
    input_report_abs(tpd->dev, ABS_MT_POSITION_Y, y);
    input_mt_sync(tpd->dev);
    TPD_EM_PRINT(x, y, x, y, id, 1);

#if (defined(MT6575)||defined(MT6577))

    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
    {
        tpd_button(x, y, 1);
    }

#endif
}

static void tpd_up(s32 x, s32 y, s32 id)
{
    //input_report_abs(tpd->dev, ABS_MT_PRESSURE, 0);
    input_report_key(tpd->dev, BTN_TOUCH, 0);
    //input_report_abs(tpd->dev, ABS_MT_TOUCH_MAJOR, 0);
    input_mt_sync(tpd->dev);
    TPD_EM_PRINT(x, y, x, y, id, 0);

#if (defined(MT6575)||defined(MT6577))

    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
    {
        tpd_button(x, y, 0);
    }

#endif
}

/*Coordination mapping*/
static void tpd_calibrate_driver(int *x, int *y)
{
    int tx;
    
    GTP_DEBUG("Call tpd_calibrate of this driver ..\n");
    
    tx = ( (tpd_calmat_driver[0] * (*x)) + (tpd_calmat_driver[1] * (*y)) + (tpd_calmat_driver[2]) ) >> 12;
    *y = ( (tpd_calmat_driver[3] * (*x)) + (tpd_calmat_driver[4] * (*y)) + (tpd_calmat_driver[5]) ) >> 12;
    *x = tx;
}

static int touch_event_handler(void *unused)
{
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    u8  end_cmd[3] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF, 0};
    u8  point_data[2 + 1 + 8 * GTP_MAX_TOUCH + 1] = {GTP_READ_COOR_ADDR >> 8, GTP_READ_COOR_ADDR & 0xFF};
    u8  touch_num = 0;
    u8  finger = 0;
    static u8 pre_touch = 0;
    static u8 pre_key = 0;
    u8  key_value = 0;
    u8 *coor_data = NULL;
    s32 input_x = 0;
    s32 input_y = 0;
    s32 input_w = 0;
    s32 id = 0;
    s32 i  = 0;
    s32 ret = -1;
#ifdef TPD_PROXIMITY
    s32 err = 0;
    hwm_sensor_data sensor_data;
    u8 proximity_status;
#endif

    sched_setscheduler(current, SCHED_RR, &param);

    do
    {
        set_current_state(TASK_INTERRUPTIBLE);

        while (tpd_halt)
        {
            tpd_flag = 0;
            msleep(20);
        }

        wait_event_interruptible(waiter, tpd_flag != 0);
        tpd_flag = 0;
        TPD_DEBUG_SET_TIME;
        set_current_state(TASK_RUNNING);

        ret = gtp_i2c_read(i2c_client_point, point_data, 12);

        if (ret < 0)
        {
            GTP_ERROR("I2C transfer error. errno:%d\n ", ret);
            goto exit_work_func;
        }

        finger = point_data[GTP_ADDR_LENGTH];

        if ((finger & 0x80) == 0)
        {
            goto exit_work_func;
        }

#ifdef TPD_PROXIMITY

        if (tpd_proximity_flag == 1)
        {
            proximity_status = point_data[GTP_ADDR_LENGTH];
            GTP_DEBUG("REG INDEX[0x814E]:0x%02X\n", proximity_status);

            if (proximity_status & 0x60)                //proximity or large touch detect,enable hwm_sensor.
            {
                tpd_proximity_detect = 0;
                //sensor_data.values[0] = 0;
            }
            else
            {
                tpd_proximity_detect = 1;
                //sensor_data.values[0] = 1;
            }

            //get raw data
            GTP_DEBUG(" ps change\n");
            GTP_DEBUG("PROXIMITY STATUS:0x%02X\n", tpd_proximity_detect);
            //map and store data to hwm_sensor_data
            sensor_data.values[0] = tpd_get_ps_value();
            sensor_data.value_divide = 1;
            sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
            //report to the up-layer
            ret = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data);

            if (ret)
            {
                GTP_ERROR("Call hwmsen_get_interrupt_data fail = %d\n", err);
            }
        }

#endif

        touch_num = finger & 0x0f;

        if (touch_num > GTP_MAX_TOUCH)
        {
            goto exit_work_func;
        }

        if (touch_num > 1)
        {
            u8 buf[8 * GTP_MAX_TOUCH] = {(GTP_READ_COOR_ADDR + 10) >> 8, (GTP_READ_COOR_ADDR + 10) & 0xff};

            ret = gtp_i2c_read(i2c_client_point, buf, 2 + 8 * (touch_num - 1));
            memcpy(&point_data[12], &buf[2], 8 * (touch_num - 1));
        }

#if GTP_HAVE_TOUCH_KEY
        key_value = point_data[3 + 8 * touch_num];

        if (key_value || pre_key)
        {
            for (i = 0; i < GTP_MAX_KEY_NUM; i++)
            {
                input_report_key(tpd->dev, touch_key_array[i], key_value & (0x01 << i));
            }

            touch_num = 0;
            pre_touch = 0;
        }

#endif
        pre_key = key_value;

        GTP_DEBUG("pre_touch:%02x, finger:%02x.", pre_touch, finger);

        if (touch_num)
        {
            for (i = 0; i < touch_num; i++)
            {
                coor_data = &point_data[i * 8 + 3];

                id = coor_data[0];
                input_x  = coor_data[1] | coor_data[2] << 8;
                input_y  = coor_data[3] | coor_data[4] << 8;
                input_w  = coor_data[5] | coor_data[6] << 8;
                
                GTP_DEBUG("Original touch point : [X:%04d, Y:%04d]", input_x, input_y);

                input_x = TPD_WARP_X(abs_x_max, input_x);
                input_y = TPD_WARP_Y(abs_y_max, input_y);                
                tpd_calibrate_driver(&input_x, &input_y);
                
                GTP_DEBUG("Touch point after calibration: [X:%04d, Y:%04d]", input_x, input_y);
                
                tpd_down(input_x, input_y, input_w, id);
            }
        }
        else if (pre_touch)
        {
            GTP_DEBUG("Touch Release!");
            tpd_up(0, 0, 0);
        }

        pre_touch = touch_num;
        input_report_key(tpd->dev, BTN_TOUCH, (touch_num || key_value));

        if (tpd != NULL && tpd->dev != NULL)
        {
            input_sync(tpd->dev);
        }

exit_work_func:

        if (!gtp_rawdiff_mode)
        {
            ret = gtp_i2c_write(i2c_client_point, end_cmd, 3);

            if (ret < 0)
            {
                GTP_INFO("I2C write end_cmd  error!");
            }
        }

    }
    while (!kthread_should_stop());

    return 0;
}

static int tpd_local_init(void)
{

    if (i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        GTP_INFO("unable to add i2c driver.\n");
        return -1;
    }

    if (tpd_load_status == 0) //if(tpd_load_status == 0) // disable auto load touch driver for linux3.0 porting
    {
        GTP_INFO("add error touch panel driver.\n");
        i2c_del_driver(&tpd_i2c_driver);
        return -1;
    }

#ifdef TPD_HAVE_BUTTON
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif

#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    if ( FACTORY_BOOT == get_boot_mode() )
	{
		TPD_DEBUG("Factory mode is detected! \n");		
		memcpy(tpd_calmat_driver, tpd_def_calmat_local_factory, sizeof(tpd_calmat_driver));	    
	}
	else
	{
		TPD_DEBUG("Normal mode is detected! \n");
		memcpy(tpd_calmat_driver, tpd_def_calmat_local_normal, sizeof(tpd_calmat_driver));
	}
#endif

    // set vendor string
    tpd->dev->id.vendor = 0x00;
    tpd->dev->id.product = tpd_info.pid;
    tpd->dev->id.version = tpd_info.vid;

    GTP_INFO("end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;

    return 0;
}


/*******************************************************
Function:
	Eter sleep function.

Input:
	client:i2c_client.

Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_enter_sleep(struct i2c_client *client)
{
    s8 ret = -1;
    s8 retry = 0;
    u8 i2c_control_buf[3] = {(u8)(GTP_REG_SLEEP >> 8), (u8)GTP_REG_SLEEP, 5};
#if GTP_POWER_CTRL_SLEEP
    GTP_GPIO_OUTPUT(GTP_RST_PORT, 0);
    msleep(5);

    mt_set_gpio_mode(GPIO118, GPIO_MODE_00);
    mt_set_gpio_mode(GPIO119, GPIO_MODE_00);
    
    //MT6589 power down
    hwPowerDown(MT65XX_POWER_LDO_VGP5, "TP");
    //hwPowerDown(MT65XX_POWER_LDO_VGP, "TP");
    GTP_INFO("GTP enter sleep!");
    return 0;
#else
    GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
    msleep(5);

    while (retry++ < 5)
    {
        ret = gtp_i2c_write(client, i2c_control_buf, 3);

        if (ret > 0)
        {
            GTP_INFO("GTP enter sleep!");
            return ret;
        }

        msleep(10);
    }

#endif
    GTP_ERROR("GTP send sleep cmd failed.");
    return ret;
}

/*******************************************************
Function:
	Wakeup from sleep mode Function.

Input:
	client:i2c_client.

Output:
	Executive outcomes.0--success,non-0--fail.
*******************************************************/
static s8 gtp_wakeup_sleep(struct i2c_client *client)
{
    u8 retry = 0;
    s8 ret = -1;


    GTP_INFO("GTP wakeup begin.");
#if GTP_POWER_CTRL_SLEEP

    mt_set_gpio_mode(GPIO118, GPIO_MODE_01);
    mt_set_gpio_mode(GPIO119, GPIO_MODE_01);
    
    while (retry++ < 5)
    {
        ret = tpd_power_on(client);

        if (ret < 0)
        {
            GTP_ERROR("I2C Power on ERROR!");
        }

        ret = gtp_send_cfg(client);

        if (ret > 0)
        {
            GTP_DEBUG("Wakeup sleep send config success.");
            return ret;
        }
    }

#else

    while (retry++ < 10)
    {
        GTP_GPIO_OUTPUT(GTP_INT_PORT, 1);
        msleep(5);
        GTP_GPIO_OUTPUT(GTP_INT_PORT, 0);
        msleep(5);
        ret = gtp_i2c_test(client);

        if (ret >= 0)
        {
            GTP_INFO("GTP wakeup sleep.");
            gtp_int_sync();
            return ret;
        }

        gtp_reset_guitar(client, 20);
    }

#endif

    GTP_ERROR("GTP wakeup sleep failed.");
    return ret;
}
/* Function to manage low power suspend */
static void tpd_suspend(struct early_suspend *h)
{
    s32 ret = -1;

#if GTP_ESD_PROTECT
    cancel_delayed_work_sync(&gtp_esd_check_work);
#endif

#ifdef TPD_PROXIMITY

    if (tpd_proximity_flag == 1)
    {
        return ;
    }

#endif

    ret = gtp_enter_sleep(i2c_client_point);

    if (ret < 0)
    {
        GTP_ERROR("GTP early suspend failed.");
    }

    tpd_halt = 1;
    mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
}

/* Function to manage power-on resume */
static void tpd_resume(struct early_suspend *h)
{
    s32 ret = -1;
#ifdef TPD_PROXIMITY

    if (tpd_proximity_flag == 1)
    {
        return ;
    }

#endif

    ret = gtp_wakeup_sleep(i2c_client_point);

    if (ret < 0)
    {
        GTP_ERROR("GTP later resume failed.");
    }

    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    tpd_halt = 0;

#if GTP_ESD_PROTECT
    queue_delayed_work(gtp_esd_check_workqueue, &gtp_esd_check_work, TPD_ESD_CHECK_CIRCLE);
#endif

}

static struct tpd_driver_t tpd_device_driver =
{
    .tpd_device_name = "gt9110",
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
    .tpd_have_button = 1,
#else
    .tpd_have_button = 0,
#endif
};

/* called when loaded into kernel */
static int __init tpd_driver_init(void)
{
    GTP_INFO("MediaTek gt9110 touch panel driver init\n");
    i2c_register_board_info(0, &i2c_tpd, 1);

    if (tpd_driver_add(&tpd_device_driver) < 0)
        GTP_INFO("add generic driver failed\n");

    return 0;
}

/* should never be called */
static void __exit tpd_driver_exit(void)
{
    GTP_INFO("MediaTek gt9110 touch panel driver exit\n");
    //input_unregister_device(tpd->dev);
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);
