#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <string.h>
#include "driver_event.h"
#include "mhl_linuxdrv_ioctl.h"
int gMhlDrv;
#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#define IEN     (1 << 8)
#define IDIS    (0 << 8)
#define PTU     (1 << 4)
#define PTD     (0 << 4)
#define EN      (1 << 3)
#define DIS     (0 << 3)
#define M0      0
#define M1      1
#define M2      2
#define M3      3
#define M4      4
#define M5      5
#define M6      6
#define M7      7
#define GPIO130_offset  0x128
int GPIO_MEM_RW() {
   int i;
  unsigned short x,old;
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    printf("Could not open memory\n");
    return 0;
  }
  volatile unsigned short  *pinconf;
  pinconf = (unsigned short *) mmap(NULL, 
                          0x10000, 
                          PROT_READ | PROT_WRITE, 
                          MAP_SHARED, 
                          fd,
			  0x48000000); 
  if (pinconf == MAP_FAILED) {
    printf("Pinconf Mapping failed\n");
    close(fd);
    return 0;
  } 
  else
    {
      printf("pinconf = 0x%x\n",&pinconf[0]);
    }
pinconf = pinconf + (0x2030 + 0x128)/2;
for(i = 0 ;i<10;i++)
{
  old = pinconf[i];
      if(i == 7 ||i ==2||i==0||i==6 )
      {
          x = (IEN | M4 );
      }
      else
      {
          x = (EN | PTD | M4 | IDIS);
      }
  pinconf[i] = x;
  printf("GPIO%d {0x%x}:0x%x => 0x%x ;\n",130+ i,&pinconf[i],old,x);
}
  close(fd);
}
static unsigned int  GetHex (const char *szComment)
{
    char     cmd[1024];
    printf("%s: 0x", szComment);
    scanf("%1023s",cmd);
    return (strtoul( cmd, (char**) NULL, 16 ));
}
static unsigned int  GetDec (const char *szComment)
{
    char     cmd[1024];
    printf("%s:", szComment);
    scanf("%1023s",cmd);
    return (strtoul( cmd, (char**) NULL, 10 ));
}
static void ShowRegisterPage(int index)
{
    int i=0,j=0;
    unsigned char reg;
    for(i=0;i<=0x10;i++)
	{	 
     for(j=0;j<=0x10;j++)       
    {
        if(i==0)
        {
            if(j==0)
                printf("   ");
            else
                printf(" %02x",j-1);   			
        }
        else 
        {      
			if(j==0)
				printf("%02X:",(i-1)*0x10);	
			else
			{
                Register_t RegisterInfo;
                int status;
                RegisterInfo.value = 0;
                RegisterInfo.dev_address = index;
                RegisterInfo.subaddr = j-1+(i-1)*0x10;
                status = ioctl(gMhlDrv,SII_IOCTRL_REGISTER_READ,&RegisterInfo);
                if(status < 0)
                {
                    printf(" XX"); 
                }
                else
                {
                    printf(" %02x",RegisterInfo.value);
                }
			}
        }
	 }
	 printf("\n");
    }
}
static int regMenu()
{
    int val;
    printf("============== regMenu ============== \n");
	printf("Command :\n");
	printf(" 0x1   ---->    read register \n");
	printf(" 0x2   ---->    write register \n");
	printf(" 0x3   ---->    dump register\n");
	printf(" 0x99   ---->   exit regMenu\n");
    printf("===================================== \n");
    val = GetHex("cmd");
    switch(val)
    {
    case 0x1:
            {
            Register_t RegisterInfo;
            int status;
            printf("read register begin\n");
            RegisterInfo.dev_address = GetHex("dev_address");
            RegisterInfo.subaddr = GetHex("subaddress");
            status = ioctl(gMhlDrv,SII_IOCTRL_REGISTER_READ,&RegisterInfo);
            if(status < 0)
            {
                printf("SII_REGISTER_READ ioctl failed, status: %d\n", status);
                return 0;
            }
            else
            {
                printf("page_0x%x[0x%x] = 0x%x \n",RegisterInfo.dev_address,RegisterInfo.subaddr,RegisterInfo.value);
                printf("read register finish\n");
            }
        }
        break;
    case 0x02:
        {
            Register_t RegisterInfo;
            int status;
            printf("write register begin\n");
            RegisterInfo.dev_address = GetHex("dev_address");
            RegisterInfo.subaddr = GetHex("subaddress");
            RegisterInfo.mask = GetHex("mask");
            RegisterInfo.value = GetHex("value");
            status = ioctl(gMhlDrv,SII_IOCTRL_REGISTER_WRITE,&RegisterInfo);
            if(status < 0)
            {
                printf("SII_REGISTER_WRITE ioctl failed, status: %d\n", status);
                return 0;
            }
            else
            {
                printf("write register finish\n");
           }
        }
        break;
    case 0x03:
        printf("dump register begin\n");
        ShowRegisterPage(GetHex("dev_address(0x72,0x7a,0x92,0xc8,...)"));
        printf("dump register finish\n");
        break; 
    case 0x04:
        break; 
    case 0x99:
        return 0;
    }
    return 0x01;
}
#define BIT0                    0x01
#define BIT1                    0x02
#define BIT2                    0x04
#define BIT3                    0x08
#define BIT4                    0x10
#define BIT5                    0x20
#define BIT6                    0x40
#define BIT7                    0x80
#define pinDbgMsgs_HIGH   BIT0
#define pinOverrideTiming_HIGH BIT1
#define pinDbgSw3_HIGH   BIT2
#define pinDbgSw4_HIGH BIT3
#define pinDbgSw5_HIGH   BIT4
#define pinDbgSw6_HIGH   BIT5
#define pinSw_HIGH   BIT6
#define pinPwSw1aEn_HIGH BIT7
static char *buildTime = "Build: " __DATE__"-" __TIME__ "\n";
static char *buildVersion = "0.80.4";
int main(int argc, char **argv) 
{
    int			status;
    char		*mhl_linuxdrv_name = "/dev/Sii-Iceman8338";
    int			cmd;
    int bexit = 0;
    unsigned int uDebugSW = (pinDbgMsgs_HIGH | pinDbgSw3_HIGH | pinDbgSw5_HIGH); 
    char strSystem[128];
    RecvThreadParam_t RecvThreadParam;
    UserControl_t user_control;
	printf("CP8338 Test appilication version: %s\n", buildVersion);
	printf("%s", buildTime);
    do
    {    
        printf("======== start Menu ==========\n");
        printf("0x1) run Default DebugSW \n");
        printf("0x2) Change DebugSW\n");
        printf("other) run Default DebugSW \n");
        printf("==============================\n");
        cmd = GetHex("\nchoice >");
    	switch (cmd)
    	{
        case 1:
        default:
             sprintf(strSystem,"insmod sii8338drv_rgb.ko DebugSW=%d",uDebugSW);
             bexit = 1;
             break;
        case 2:
            cmd = GetHex("  pinDbgMsgs			  // BIT0; \n"  
                         "  pinOverrideTiming	  // BIT1; \n"
                         "  pinDbgSw3			  // BIT2; \n"
                         "  pinDbgSw4	          // BIT3; \n"
                         "  pinDbgSw5			  // BIT4; \n"
                         "  pinDbgSw6	          // BIT5; \n"
                         "  pinSw				  // BIT6; \n"
                         "  pinPwSw1aEn		      // BIT7; \n"
                         "  DebugSW = >");
            uDebugSW = cmd &0xFF; 
            sprintf(strSystem,"insmod sii8338drv_rgb.ko DebugSW=%d ",uDebugSW);
            bexit = 1;			
            break;
        }
    }while(!bexit);
    printf("%s\n",strSystem);
    system(strSystem);
    usleep(1000);
    printf("Welcome to the Silicon Image Transmitter test application\nAttempting to open driver %s\n", mhl_linuxdrv_name);
    gMhlDrv = open(mhl_linuxdrv_name, O_RDONLY);
    if (gMhlDrv < 0)
    {
        printf("Error opening %s!\n", mhl_linuxdrv_name);
        return errno;
    }
    RecvThreadParam.hDevcie = gMhlDrv;
    status = Sii_CreateRecvThread(&RecvThreadParam);
    if(status < 0)
    {
        printf("create recv thread failed\n");
        return errno;
    }
    bexit = 0;
    do {
        printf("======== Main Menu ==========\n");
        printf("0x1) Access Registers\n");
        printf("0x2) trigger extern interrupt\n");
        printf("0x3) RCP key send\n");
        printf("0x4) On/Off  mhl interrupt\n");
        printf("0x5) Read Sink EDID\n");
        printf("0x6) test timer ,trigger MHL interrupt in timer CB\n");
        printf("0x7) reset MHL chip\n");
        printf("0x8) reinitialize mhl and ext chip\n");
        printf("0x80) show Main Menu\n");
        printf("0x99) Exit\n");
        printf("=============================\n");
        cmd = GetHex("\nsii8338Test >");
		switch (cmd)
		{
            case 0x99:
                printf("Exiting sii833xTest\n");
                bexit = 1;
                user_control.ControlID = USER_GPIO_SET;
                user_control.SubCommand.GpioCtrl.GpioIndex = 0x01;
                user_control.SubCommand.GpioCtrl.Value = 0;
                ioctl(gMhlDrv,SII_IOCTRL_USER,&user_control);
                user_control.ControlID = USER_GPIO_SET;
                user_control.SubCommand.GpioCtrl.GpioIndex = 0x01;
                user_control.SubCommand.GpioCtrl.Value = 1;
                ioctl(gMhlDrv,SII_IOCTRL_USER,&user_control);
                break;
            case 0x80:
                break;
            case 1:			
                while(regMenu());
                break;
            case 2:	
                user_control.ControlID = USER_TRIGGER_EXT_INT;
                ioctl(gMhlDrv,SII_IOCTRL_USER,&user_control);
                break;
            case 0x03:
                cmd = GetHex("input keycode");
                RecvThreadParam.sendKeyCode = (uint8_t)cmd&0xFF; 
                Sii_SendRCPKey(&RecvThreadParam);
                break;
            case 0x04:  
                user_control.ControlID = USER_ON_OFF_MHL_INT;
                user_control.SubCommand.iSubCommand = GetHex("1-enable;0-disable"); 
                ioctl(gMhlDrv,SII_IOCTRL_USER,&user_control);
                break;
            case 0x05:  
            {
                int i;
                user_control.ControlID = USER_READ_SINK_EDID;
                memset(user_control.SubCommand.EDID,0,256);
                ioctl(gMhlDrv,SII_IOCTRL_USER,&user_control);
                for (i = 0; i < 256; i++)
                {
                    printf("0x%2x ",user_control.SubCommand.EDID[i]);
                    if ((i + 1) % 16 == 0)
                        printf("\n");
                }
            }
                break;
            case 0x06:  
                cmd = GetDec("input delay(ms):");                 
                user_control.ControlID = USER_TRIGGER_MHL_INT;
                user_control.SubCommand.iSubCommand = cmd;
                ioctl(gMhlDrv,SII_IOCTRL_USER,&user_control);
                break;
            case 0x07:  
                user_control.ControlID = USER_GPIO_SET;
                user_control.SubCommand.GpioCtrl.GpioIndex = 0x01;
                user_control.SubCommand.GpioCtrl.Value = 0;
                ioctl(gMhlDrv,SII_IOCTRL_USER,&user_control);
                user_control.ControlID = USER_GPIO_SET;
                user_control.SubCommand.GpioCtrl.GpioIndex = 0x01;
                user_control.SubCommand.GpioCtrl.Value = 1;
                ioctl(gMhlDrv,SII_IOCTRL_USER,&user_control);
                break;
            case 0x08:                 
                user_control.ControlID  = USER_RESET_MHL_CHIP;
                ioctl(gMhlDrv,SII_IOCTRL_USER,&user_control);  
                break;
            default:
                break;
        }
    } while(!bexit);
    close(gMhlDrv);
 	system("rmmod sii8338drv_rgb.ko");
    Sii_StopRecvThread(&RecvThreadParam);
    return 0;
}
