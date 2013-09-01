/*
 * MD218A voice coil motor driver
 *
 *
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include "IMX111AF.h"
#include "../camera/kd_camera_hw.h"

#define LENS_I2C_BUSNUM 1

static struct i2c_board_info __initdata kd_lens_dev={ I2C_BOARD_INFO("IMX111AF", 0x18)};


#define IMX111AF_DRVNAME "IMX111AF"

#define IMX111AF_VCM_WRITE_ID           0x18

#define IMX111AF_DEBUG
#ifdef IMX111AF_DEBUG
#define IMX111AFDB printk
#else
#define IMX111AFDB(x,...)
#endif

static spinlock_t g_IMX111AF_SpinLock;

static struct i2c_client * g_pstIMX111AF_I2Cclient = NULL;

static dev_t g_IMX111AF_devno;
static struct cdev * g_pIMX111AF_CharDrv = NULL;
static struct class *actuator_class = NULL;

static int  g_s4IMX111AF_Opened = 0;
static long g_i4MotorStatus = 0;
static long g_i4Dir = 0;
static unsigned long g_u4IMX111AF_INF = 0;
static unsigned long g_u4IMX111AF_MACRO = 1023;
static unsigned long g_u4TargetPosition = 0;
static unsigned long g_u4CurrPosition   = 0;

static int g_sr = 15;

extern s32 mt_set_gpio_mode(u32 u4Pin, u32 u4Mode);
extern s32 mt_set_gpio_out(u32 u4Pin, u32 u4PinOut);
extern s32 mt_set_gpio_dir(u32 u4Pin, u32 u4Dir);

//<2013/05/02-24529-alberthsiao, Update camera parameters for imx111
char T_SRC[36]={0x10,0x14,0x19,0x1d,0x02,0x08,0x12,0x14,0x16,0x19,0x1b,0x1d,0x00,0x02,0x05,0x08,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x00,0x01,0x02,0x04,0x05,0x06,0x08};
char SS[36]={0x07,0x07,0x07,0x07,0x07,0x07,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x06,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05,0x05};

static int s4IMX111AF_ReadReg(unsigned short * a_pu2Result)
{
    int  i4RetValue = 0;
    char pBuff[2];

    i4RetValue = i2c_master_recv(g_pstIMX111AF_I2Cclient, pBuff, 2);

    if (i4RetValue < 0) 
    {
        IMX111AFDB("[IMX111AF] I2C read failed!! \n");
        return -1;
    }

    *a_pu2Result = (((u16)pBuff[0]) << 4) + (pBuff[1] >> 4);

    return 0;
}

static int s4IMX111AF_WriteReg(u16 a_u2Data)
{
    int  i4RetValue = 0;

    char puSendCmd[2] = {(char)(a_u2Data >> 4) , (char)(((a_u2Data & 0xF) << 4)+g_sr)};

    IMX111AFDB("[IMX111AF] g_sr %d, write %d \n", g_sr, a_u2Data);
    g_pstIMX111AF_I2Cclient->ext_flag |= I2C_A_FILTER_MSG;
    i4RetValue = i2c_master_send(g_pstIMX111AF_I2Cclient, puSendCmd, 2);
	
    if (i4RetValue < 0) 
    {
        IMX111AFDB("[IMX111AF] I2C send failed!! \n");
        return -1;
    }

    return 0;
}

static int s4IMX111AF_WriteData(u16 a_u2Data)
{
    int  i4RetValue = 0;

    char puSendCmd[2] = {(char)(a_u2Data >> 8) , (char)(a_u2Data & 0xFF)};

	IMX111AFDB("[IMX111AF] Byte1: %d, Byte2: %d \n", puSendCmd[0], puSendCmd[1]);
    g_pstIMX111AF_I2Cclient->ext_flag |= I2C_A_FILTER_MSG;
    i4RetValue = i2c_master_send(g_pstIMX111AF_I2Cclient, puSendCmd, 2);
	
    if (i4RetValue < 0) 
    {
        IMX111AFDB("[IMX111AF] I2C send failed!! \n");
        return -1;
    }

    return 0;
}

inline static int getIMX111AFInfo(__user stIMX111AF_MotorInfo * pstMotorInfo)
{
    stIMX111AF_MotorInfo stMotorInfo;
    stMotorInfo.u4MacroPosition   = g_u4IMX111AF_MACRO;
    stMotorInfo.u4InfPosition     = g_u4IMX111AF_INF;
    stMotorInfo.u4CurrentPosition = g_u4CurrPosition;
    stMotorInfo.bIsSupportSR      = TRUE;

	if (g_i4MotorStatus == 1)	{stMotorInfo.bIsMotorMoving = 1;}
	else						{stMotorInfo.bIsMotorMoving = 0;}

	if (g_s4IMX111AF_Opened >= 1)	{stMotorInfo.bIsMotorOpen = 1;}
	else						{stMotorInfo.bIsMotorOpen = 0;}

    if(copy_to_user(pstMotorInfo , &stMotorInfo , sizeof(stIMX111AF_MotorInfo)))
    {
        IMX111AFDB("[IMX111AF] copy to user failed when getting motor information \n");
    }

    return 0;
}

inline static int moveIMX111AF(unsigned long a_u4Position)
{
    int ret = 0, dac=0;
    char index=0;

    if((a_u4Position > g_u4IMX111AF_MACRO) || (a_u4Position < g_u4IMX111AF_INF))
    {
        IMX111AFDB("[IMX111AF] out of range \n");
        return -EINVAL;
    }

    if (g_s4IMX111AF_Opened == 1)
    {
        unsigned short InitPos;
		ret = s4IMX111AF_ReadReg(&InitPos); // MTK Reference Design
	    
        spin_lock(&g_IMX111AF_SpinLock);
        if(ret == 0)
        {
            IMX111AFDB("[IMX111AF] Init Pos %6d \n", InitPos);
            g_u4CurrPosition = (unsigned long)InitPos;
        }
        else
        {		
            g_u4CurrPosition = 0;
        }
        g_s4IMX111AF_Opened = 2;
        spin_unlock(&g_IMX111AF_SpinLock);
    }

    if (g_u4CurrPosition < a_u4Position)
    {
        spin_lock(&g_IMX111AF_SpinLock);	
        g_i4Dir = 1;
        spin_unlock(&g_IMX111AF_SpinLock);	
		dac= a_u4Position - g_u4CurrPosition;
    }
    else if (g_u4CurrPosition > a_u4Position)
    {
        spin_lock(&g_IMX111AF_SpinLock);	
        g_i4Dir = -1;
        spin_unlock(&g_IMX111AF_SpinLock);	
		dac=  g_u4CurrPosition - a_u4Position;
    }
    else										{return 0;}
#if 1
//<2013/06/30-26423-alberthsiao,[ATS00160169]Update camera parameters for imx111 // to improve AF satability
//<2013/06/27-26367-alberthsiao,[Coverity]90096,90097
    //if (dac <= 5)
    if (dac < 5) 
		index=0;
	else if (dac <= 40)
		index = dac-5;
	else if (dac <= 80)
        index = dac/2 -5;
	else if (dac <= 160)
		index = dac/4 -5;
	else if (dac <= 320)
		index = dac/8 -5;
	else 
		index = dac/16 -5;
		
 	if (index>35)
     index=35;
//>2013/06/27-26367-alberthsiao     
	msleep(12);
	s4IMX111AF_WriteData(0xECA3);//Protection OFF
	s4IMX111AF_WriteData(0xA104);//MCLK setting
	s4IMX111AF_WriteData(0xF200 | (T_SRC[index]<<3));//T_SRC setting
	s4IMX111AF_WriteData(0xDC51);//Protection ON
	//s4IMX111AF_WriteData((dac<<4) + SS[index]);//D[9:0],S[3:2],S[1:0] setting
	s4IMX111AF_WriteData((a_u4Position<<4) + SS[index]);
	//msleep(12);
//>2013/06/27-26423-alberthsiao
#endif
    spin_lock(&g_IMX111AF_SpinLock);    
    g_u4TargetPosition = a_u4Position;
    spin_unlock(&g_IMX111AF_SpinLock);	

	IMX111AFDB("[IMX111AF] move [curr] %d [target] %d\n", g_u4CurrPosition, g_u4TargetPosition);

            spin_lock(&g_IMX111AF_SpinLock);
            g_sr = 15;
            g_i4MotorStatus = 0;
            spin_unlock(&g_IMX111AF_SpinLock);	
		
            if(s4IMX111AF_WriteReg((unsigned short)g_u4TargetPosition) == 0) // MTK Reference Design	
            {
                spin_lock(&g_IMX111AF_SpinLock);		
                g_u4CurrPosition = (unsigned long)g_u4TargetPosition;
                spin_unlock(&g_IMX111AF_SpinLock);				
            }
            else
            {
                IMX111AFDB("[IMX111AF] set I2C failed when moving the motor \n");			
                spin_lock(&g_IMX111AF_SpinLock);
                g_i4MotorStatus = -1;
                spin_unlock(&g_IMX111AF_SpinLock);				
            }
#if 0
	if (dac <= 40)
		msleep(12);
	else if (dac <= 80)
        msleep(23);
	else if (dac <= 160)
		msleep(45);
	else if (dac <= 320)
		msleep(89);
	else 
		msleep(168);
#endif
//>2013/05/02-24529-alberthsiao
    return 0;
}

inline static int setIMX111AFInf(unsigned long a_u4Position)
{
    spin_lock(&g_IMX111AF_SpinLock);
    g_u4IMX111AF_INF = a_u4Position;
    spin_unlock(&g_IMX111AF_SpinLock);	
    return 0;
}

inline static int setIMX111AFMacro(unsigned long a_u4Position)
{
    spin_lock(&g_IMX111AF_SpinLock);
    g_u4IMX111AF_MACRO = a_u4Position;
    spin_unlock(&g_IMX111AF_SpinLock);	
    return 0;	
}

////////////////////////////////////////////////////////////////
static long IMX111AF_Ioctl(
struct file * a_pstFile,
unsigned int a_u4Command,
unsigned long a_u4Param)
{
    long i4RetValue = 0;

    switch(a_u4Command)
    {
        case IMX111AFIOC_G_MOTORINFO :
            i4RetValue = getIMX111AFInfo((__user stIMX111AF_MotorInfo *)(a_u4Param));
        break;

        case IMX111AFIOC_T_MOVETO :
            i4RetValue = moveIMX111AF(a_u4Param);
        break;
 
        case IMX111AFIOC_T_SETINFPOS :
            i4RetValue = setIMX111AFInf(a_u4Param);
        break;

        case IMX111AFIOC_T_SETMACROPOS :
            i4RetValue = setIMX111AFMacro(a_u4Param);
        break;
		
        default :
      	    IMX111AFDB("[IMX111AF] No CMD \n");
            i4RetValue = -EPERM;
        break;
    }

    return i4RetValue;
}

//Main jobs:
// 1.check for device-specified errors, device not ready.
// 2.Initialize the device if it is opened for the first time.
// 3.Update f_op pointer.
// 4.Fill data structures into private_data
//CAM_RESET
static int IMX111AF_Open(struct inode * a_pstInode, struct file * a_pstFile)
{
    IMX111AFDB("[IMX111AF] IMX111AF_Open - Start\n");
	
    spin_lock(&g_IMX111AF_SpinLock);

    if(g_s4IMX111AF_Opened)
    {
        spin_unlock(&g_IMX111AF_SpinLock);
        IMX111AFDB("[IMX111AF] the device is opened \n");
        return -EBUSY;
    }

    g_s4IMX111AF_Opened = 1;
		
    spin_unlock(&g_IMX111AF_SpinLock);

 	IMX111AFDB("[IMX111]IMX111AF_Open-End\n");	
    return 0;
}

//Main jobs:
// 1.Deallocate anything that "open" allocated in private_data.
// 2.Shut down the device on last close.
// 3.Only called once on last time.
// Q1 : Try release multiple times.
static int IMX111AF_Release(struct inode * a_pstInode, struct file * a_pstFile)
{
    IMX111AFDB("[IMX111AF] IMX111AF_Release - Start\n");

    if (g_s4IMX111AF_Opened)
    {
        IMX111AFDB("[IMX111AF] feee \n");
        g_sr = 15;
	    s4IMX111AF_WriteReg(200);
        msleep(10);
	    s4IMX111AF_WriteReg(100);
        msleep(10);
            	            	    	    
        spin_lock(&g_IMX111AF_SpinLock);
        g_s4IMX111AF_Opened = 0;
        spin_unlock(&g_IMX111AF_SpinLock);

    }

    IMX111AFDB("[IMX111AF] IMX111AF_Release - End\n");

    return 0;
}

static const struct file_operations g_stIMX111AF_fops = 
{
    .owner = THIS_MODULE,
    .open = IMX111AF_Open,
    .release = IMX111AF_Release,
    .unlocked_ioctl = IMX111AF_Ioctl
};

inline static int Register_IMX111AF_CharDrv(void)
{
    struct device* vcm_device = NULL;

    IMX111AFDB("[IMX111AF] Register_IMX111AF_CharDrv - Start\n");

    //Allocate char driver no.
    if( alloc_chrdev_region(&g_IMX111AF_devno, 0, 1,IMX111AF_DRVNAME) )
    {
        IMX111AFDB("[IMX111AF] Allocate device no failed\n");

        return -EAGAIN;
    }

    //Allocate driver
    g_pIMX111AF_CharDrv = cdev_alloc();

    if(NULL == g_pIMX111AF_CharDrv)
    {
        unregister_chrdev_region(g_IMX111AF_devno, 1);

        IMX111AFDB("[IMX111AF] Allocate mem for kobject failed\n");

        return -ENOMEM;
    }

    //Attatch file operation.
    cdev_init(g_pIMX111AF_CharDrv, &g_stIMX111AF_fops);

    g_pIMX111AF_CharDrv->owner = THIS_MODULE;

    //Add to system
    if(cdev_add(g_pIMX111AF_CharDrv, g_IMX111AF_devno, 1))
    {
        IMX111AFDB("[IMX111AF] Attatch file operation failed\n");

        unregister_chrdev_region(g_IMX111AF_devno, 1);

        return -EAGAIN;
    }

    actuator_class = class_create(THIS_MODULE, "actuatordrv");
    if (IS_ERR(actuator_class)) {
        int ret = PTR_ERR(actuator_class);
        IMX111AFDB("Unable to create class, err = %d\n", ret);
        return ret;            
    }

    vcm_device = device_create(actuator_class, NULL, g_IMX111AF_devno, NULL, IMX111AF_DRVNAME);

    if(NULL == vcm_device)
    {
        return -EIO;
    }
    
    IMX111AFDB("[IMX111AF] Register_IMX111AF_CharDrv - End\n");    
    return 0;
}

inline static void Unregister_IMX111AF_CharDrv(void)
{
    IMX111AFDB("[IMX111AF] Unregister_IMX111AF_CharDrv - Start\n");

    //Release char driver
    cdev_del(g_pIMX111AF_CharDrv);

    unregister_chrdev_region(g_IMX111AF_devno, 1);
    
    device_destroy(actuator_class, g_IMX111AF_devno);

    class_destroy(actuator_class);
}

//////////////////////////////////////////////////////////////////////

static int IMX111AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int IMX111AF_i2c_remove(struct i2c_client *client);
static const struct i2c_device_id IMX111AF_i2c_id[] = {{IMX111AF_DRVNAME,0},{}};   
struct i2c_driver IMX111AF_i2c_driver = {                       
    .probe = IMX111AF_i2c_probe,                                   
    .remove = IMX111AF_i2c_remove,                           
    .driver.name = IMX111AF_DRVNAME,                 
    .id_table = IMX111AF_i2c_id,                             
};  

#if 0 
static int IMX111AF_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info) {         
    strcpy(info->type, IMX111AF_DRVNAME);                                                         
    return 0;                                                                                       
}      
#endif 
static int IMX111AF_i2c_remove(struct i2c_client *client) {
    return 0;
}

/* Kirby: add new-style driver {*/
static int IMX111AF_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int i4RetValue = 0;

    IMX111AFDB("[IMX111AF] Attach I2C \n");

    /* Kirby: add new-style driver { */
    g_pstIMX111AF_I2Cclient = client;
    
    g_pstIMX111AF_I2Cclient->addr = g_pstIMX111AF_I2Cclient->addr >> 1;
    
    //Register char driver
    i4RetValue = Register_IMX111AF_CharDrv();

    if(i4RetValue){

        IMX111AFDB("[IMX111AF] register char device failed!\n");

        return i4RetValue;
    }

    spin_lock_init(&g_IMX111AF_SpinLock);

    IMX111AFDB("[IMX111AF] Attached!! \n");

    return 0;
}

static int IMX111AF_probe(struct platform_device *pdev)
{
    return i2c_add_driver(&IMX111AF_i2c_driver);
}

static int IMX111AF_remove(struct platform_device *pdev)
{
    i2c_del_driver(&IMX111AF_i2c_driver);
    return 0;
}

static int IMX111AF_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    return 0;
}

static int IMX111AF_resume(struct platform_device *pdev)
{
    return 0;
}

// platform structure
static struct platform_driver g_stIMX111AF_Driver = {
    .probe		= IMX111AF_probe,
    .remove	= IMX111AF_remove,
    .suspend	= IMX111AF_suspend,
    .resume	= IMX111AF_resume,
    .driver		= {
        .name	= "lens_actuator",
        .owner	= THIS_MODULE,
    }
};

static int __init IMX111AF_i2C_init(void)
{
    i2c_register_board_info(LENS_I2C_BUSNUM, &kd_lens_dev, 1);
	
    if(platform_driver_register(&g_stIMX111AF_Driver)){
        IMX111AFDB("failed to register IMX111AF driver\n");
        return -ENODEV;
    }
 IMX111AFDB("init IMX111AF driver\n");
    return 0;
}

static void __exit IMX111AF_i2C_exit(void)
{
	platform_driver_unregister(&g_stIMX111AF_Driver);
}

module_init(IMX111AF_i2C_init);
module_exit(IMX111AF_i2C_exit);

MODULE_DESCRIPTION("IMX111AF lens module driver");
MODULE_AUTHOR("KY Chen <ky.chen@Mediatek.com>");
MODULE_LICENSE("GPL");


