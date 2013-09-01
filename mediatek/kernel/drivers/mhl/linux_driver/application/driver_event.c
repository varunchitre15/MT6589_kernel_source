#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/prctl.h>
#include "mhl_linuxdrv_ioctl.h"
#include "driver_event.h"
#include "pthread.h"
void* recv_thread(void *ptr);
unsigned char send_RCP_ACKE = 0;
unsigned char send_RCP_ACK = 1;
int init_recv_sock(void)
{
    struct sockaddr_nl snl;
    const int buffersize = 1024*64;
    int retval;
    memset(&snl, 0x00, sizeof(struct sockaddr_nl));
    snl.nl_family = AF_NETLINK;
    snl.nl_pid = getpid();
    snl.nl_groups = 1;
    int recv_sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (recv_sock == -1)
    {
        printf("error getting socket: %s", strerror(errno));
        return -1;
    }
    setsockopt(recv_sock, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));
    retval = bind(recv_sock, (struct sockaddr *) &snl, sizeof(struct sockaddr_nl));
    if (retval < 0) {
        printf("bind failed: %s", strerror(errno));
        close(recv_sock);
        recv_sock = -1;
        return -1;
    }
    return recv_sock;
}
#define MHL_STATE_CONNECTED	0x01
#define MHL_STATE_RCP_READY	0x02
#define MHL_STATE_RCP_IP		0x04
#define MHL_STATE_RCP_SEND_ACK	0x08
#define MHL_STATE_RCP_ACK		0x10	
#define MHL_STATE_RCP_NAK		0x20	
static int GetMhlState(RecvThreadParam_t * pRecv)
{
    int fd;
    char file_name[128];
    char data[64];
    int len;
    if(strlen(pRecv->dev_path)==0)
    {
        printf("dev_path is empty\n");
        return -1;
    }
    strcpy(file_name,"/sys");
    strcat(file_name,pRecv->dev_path);
    strcat(file_name,"/connection_state");
    fd = open(file_name,O_RDONLY);
    if(fd < 0)
    {
        printf("open %s error \n",file_name);
        return -1;
    }
    len = read(fd,data,64);
    if(len)
    { 
        data[len] = '\0';
        printf("%s \n",data);
    }
    close(fd);
    if (!strncmp(data, "connected rcp_ready", strlen("connected rcp_ready"))) 
    {
        printf("%s \n",data);
        return MHL_STATE_RCP_READY;
    }
    else
    {
        return 0;
    }
}
static int SendRCPKeyCode(RecvThreadParam_t * pRecv)
{
    int fd;
    char file_name[128];
    char data[16];
    if(strlen(pRecv->dev_path)==0)
    {
        printf("dev_path is empty\n");
        return -1;
    }
    strcpy(file_name,"/sys");
    strcat(file_name,pRecv->dev_path);
    strcat(file_name,"/rcp_keycode");
    sprintf(data,"0x%02x",(int)pRecv->sendKeyCode&0xff);
    fd = open(file_name,O_WRONLY | O_TRUNC);
    if(fd < 0)
    {
        printf("open %s error \n",file_name);
        return -1;
    }
    write(fd,data,strlen(data)+1);
    close(fd);
    return 0;
}
static int SendRCPACK(RecvThreadParam_t * pRecv ,uint8_t keycode, uint8_t bAck)
{
    int fd;
    char file_name[128];
    char data[128];
    char str_keycode[32];
    char str_errorcode[32];
    if(strlen(pRecv->dev_path)==0)
    {
        printf("dev_path is empty\n");
        return -1;
    }
    strcpy(file_name,"/sys");
    strcat(file_name,pRecv->dev_path);
    strcat(file_name,"/rcp_ack");
    fd = open(file_name,O_WRONLY | O_TRUNC);
    if(fd < 0)
    {
        printf("open %s error \n",file_name);
        return -1;
    }
    sprintf(str_keycode,"keycode=0x%02x",(int)keycode&0xff);
    printf("send RCPACK(%s,",str_keycode);
    sprintf(str_errorcode,"errorcode=0x%02x",bAck ? 0 : 1);
    printf("%s)\n",str_errorcode);
    memset(data,0,128);
    memcpy(data,str_keycode,strlen(str_keycode));
    data[strlen(str_keycode)] = 0;
    memcpy(data + strlen(str_keycode) + 1,str_errorcode,strlen(str_errorcode));
    data[strlen(str_keycode) +strlen(str_errorcode) + 1] = 0;
    write(fd,data,strlen(str_keycode)+1 + strlen(str_errorcode) + 1);
    close(fd);
    return 0;
}
void Sii_SendRCPKey(RecvThreadParam_t * pRecvThreadParam )
{
    pthread_mutex_lock(&pRecvThreadParam->cmdLock);
    if((pRecvThreadParam->mhlState & MHL_STATE_RCP_READY) !=MHL_STATE_RCP_READY)
    {
        printf("MHL_STATE_RCP_READY  not recieved , CAN NOT send key %x\n",pRecvThreadParam->sendKeyCode);
        pthread_mutex_unlock(&pRecvThreadParam->cmdLock);
        return ;
    }
    if((pRecvThreadParam->mhlState & MHL_STATE_RCP_IP) == MHL_STATE_RCP_IP)
    {
        printf(" in MHL_STATE_RCP_IP  not recieved , CAN NOT send key %x\n",pRecvThreadParam->sendKeyCode);
        pthread_mutex_unlock(&pRecvThreadParam->cmdLock);
        return ;
    }
    if(pRecvThreadParam->sendKeyCode != 0xFF)
    {  
        int status;
        status = SendRCPKeyCode(pRecvThreadParam);
        if(status < 0)
        {
            printf("SII_HDMI_RCP_SEND ioctl failed, status: %d\n", status);
        }
        else
        {
            pRecvThreadParam->mhlState |= MHL_STATE_RCP_IP;
        }
    }
    pthread_mutex_unlock(&pRecvThreadParam->cmdLock);
}
static void EventsHandler(RecvThreadParam_t * pRecv, mhlTxEventPacket_t  * pEventPacket)
{
	mhlTxEventPacket_t	mhlEventPacket = *pEventPacket;
	int					status;
    printf("mhlState = %d\n",pRecv->mhlState);
    switch(mhlEventPacket.event)
    {
        case MHL_TX_EVENT_NONE:				
            break;
        case MHL_TX_EVENT_DISCONNECTION:	
            printf("Disconnection event received\n");
            pRecv->mhlState = 0;
            break;
        case MHL_TX_EVENT_CONNECTION:		
            printf("Connection event received\n");
			pRecv->mhlState |= MHL_STATE_CONNECTED;
            break;
        case MHL_TX_EVENT_RCP_READY:		
            printf("Connection ready for RCP event received\n");
            pRecv->mhlState |= MHL_STATE_RCP_READY;
            break;
        case MHL_TX_EVENT_RCP_RECEIVED:		
            printf("RCP event received, key code: 0x%02x\n",
                    mhlEventPacket.eventParam);
            pRecv->mhlState |= MHL_STATE_RCP_SEND_ACK;
            break;
        case MHL_TX_EVENT_RCPK_RECEIVED:	
            if((pRecv->mhlState & MHL_STATE_RCP_IP) &&
                (mhlEventPacket.eventParam == pRecv->sendKeyCode)
             )
            {
                printf("RCPK received for sent key code: 0x%02x\n",
                        mhlEventPacket.eventParam);
                pRecv->mhlState &= ~MHL_STATE_RCP_IP;
                pRecv->sendKeyCode = 0xFF;
            }
            else if(pRecv->mhlState & MHL_STATE_RCP_NAK)
            {
                printf("RCPK is recieved after RCPE: 0x%02x\n",
                        mhlEventPacket.eventParam);
                pRecv->mhlState &= ~MHL_STATE_RCP_NAK;
                pRecv->sendKeyCode = 0xFF;
            }
            break;          
        case MHL_TX_EVENT_RCPE_RECEIVED:	
            if(pRecv->mhlState & MHL_STATE_RCP_IP)
            {
                printf("RCPE received for sent key code , error code = 0x%02x\n",
                        mhlEventPacket.eventParam);
                pRecv->mhlState &= ~MHL_STATE_RCP_IP;
                pRecv->mhlState |= MHL_STATE_RCP_NAK;
                pRecv->sendKeyCode = 0xFF;
            }
            else
            {
                printf("Unexpected RCPE event received\n");
            }
            break;
		case 	MHL_TX_EVENT_DCAP_CHG:				
			break;
		case 	MHL_TX_EVENT_DSCR_CHG:				
			break;
		case 	MHL_TX_EVENT_POW_BIT_CHG:			
			break;
		case 	MHL_TX_EVENT_RGND_MHL:				
			break;
        default:
            printf("Unknown event code: %d \n", mhlEventPacket.event);
    }
    if(pRecv->mhlState & MHL_STATE_RCP_SEND_ACK)
    {
        if(send_RCP_ACK)
        {
            status = SendRCPACK(pRecv,mhlEventPacket.eventParam,1);
        }
        if(status < 0)
        {
            printf("send ACK failed, status: %d\n", status);
        }
        if(send_RCP_ACK && send_RCP_ACKE)
        {
            status = SendRCPACK(pRecv,mhlEventPacket.eventParam,0);
        }
        if(status < 0)
        {
            printf("send ACKE  failed, status: %d\n", status);
        }
        pRecv->mhlState &= ~MHL_STATE_RCP_SEND_ACK;
    }
}
void* recv_thread(void *ptr)
{
#define UEVENT_MSG_LEN 4096
    char buf[UEVENT_MSG_LEN + 2]={0};
    char *msg;
    int size;
    RecvThreadParam_t *pRecv = (RecvThreadParam_t *)ptr;
    int recv_sock = pRecv->recv_sock;
    mhlTxEventPacket_t mhlTxEventPacket;
    int maxfd;
    fd_set readfd;
    if(recv_sock < 0 )
    {
        printf("init recv socket error\n");
        return NULL;
    }
    memset(&mhlTxEventPacket,0,sizeof(mhlTxEventPacket_t));
    maxfd = recv_sock > pRecv->pipe_fd[0] ? (recv_sock + 1) : (pRecv->pipe_fd[0] + 1);
    while(1)
    { 
        FD_ZERO(&readfd);
        FD_SET(recv_sock,&readfd);
        FD_SET(pRecv->pipe_fd[0],&readfd);
        if(select(maxfd,&readfd,NULL,NULL,NULL)<=0)
        {
            printf("select return error\n");
            break;
        }
        if(FD_ISSET(pRecv->pipe_fd[0],&readfd))
        {
            break;
        }
        else if(!FD_ISSET(recv_sock,&readfd))
        {
            printf("NOT match \n");
            continue;
        }
        size = recv(recv_sock,&buf,UEVENT_MSG_LEN,0);
        if(size == -1)
        {
            printf("error\n");
            break;
        }
        else if(size == UEVENT_MSG_LEN) 
            continue;
        buf[size] = '\0';
#if 0
        msg = buf;
        int count=0,str_len = 0;
        while (*msg) 
        {
             printf("no(%d) = %s\n", ++count,msg);
             str_len+=strlen(msg);
            while(*msg++);
        }
        printf("str_len = %d, recv size = %d", str_len,size);
#endif
        msg = buf;
        while (*msg) 
        {
            if (!strncmp(msg, "MHLEVENT=", strlen("MHLEVENT="))) 
            {
                printf("======<< %s >>======\n", msg);
            }
            mhlTxEventPacket.event = MHL_TX_EVENT_NONE;
            if (!strncmp(msg, "MHLEVENT=connected", strlen("MHLEVENT=connected"))) 
            {
                msg += strlen("MHLEVENT=connected");
                mhlTxEventPacket.event = MHL_TX_EVENT_CONNECTION;
            } else if (!strncmp(msg, "MHLEVENT=disconnected", strlen("MHLEVENT=disconnected")))
            {
                msg += strlen("MHLEVENT=disconnected");
                mhlTxEventPacket.event = MHL_TX_EVENT_DISCONNECTION;
            }else  if (!strncmp(msg, "MHLEVENT=rcp_ready", strlen("MHLEVENT=rcp_ready")))
            {
                msg += strlen("MHLEVENT=rcp_ready");
                mhlTxEventPacket.event = MHL_TX_EVENT_RCP_READY;
            }else if (!strncmp(msg, "MHLEVENT=received_RCPK key code=0x", strlen("MHLEVENT=received_RCPK key code=0x")))
            {
                char str[4]="";
                msg += strlen("MHLEVENT=received_RCPK key code=0x");
                strncpy(str,msg,2);
                msg +=2;
                mhlTxEventPacket.eventParam = strtoul( str, (char**) NULL, 16 );
                mhlTxEventPacket.event = MHL_TX_EVENT_RCPK_RECEIVED;
            }else  if (!strncmp(msg, "MHLEVENT=received_RCPE error code=0x", strlen("MHLEVENT=received_RCPE error code=0x"))) 
            {
                char str[4]="";
                msg += strlen("MHLEVENT=received_RCPE error code=0x");
                strncpy(str,msg,2);
                msg +=2;
                mhlTxEventPacket.eventParam = strtoul( str, (char**) NULL, 16 );
                mhlTxEventPacket.event = MHL_TX_EVENT_RCPE_RECEIVED;
            }else  if (!strncmp(msg, "MHLEVENT=received_RCP key code=0x", strlen("MHLEVENT=received_RCP key code=0x")))
            {
                char str[4]="";
                msg += strlen("MHLEVENT=received_RCP key code=0x");
                strncpy(str,msg,2);
                msg +=2;
                mhlTxEventPacket.eventParam = strtoul( str, (char**) NULL, 16 );
                mhlTxEventPacket.event = MHL_TX_EVENT_RCP_RECEIVED;
            }else  if (!strncmp(msg, "MHLEVENT=DEVCAP change", strlen("MHLEVENT=DEVCAP change")))
            {
                msg += strlen("MHLEVENT=DEVCAP change");
                mhlTxEventPacket.event = MHL_TX_EVENT_DCAP_CHG;
            }else  if (!strncmp(msg, "MHLEVENT=SCRATCHPAD change", strlen("MHLEVENT=SCRATCHPAD change")))
            {
                msg += strlen("MHLEVENT=SCRATCHPAD change");
                mhlTxEventPacket.event = MHL_TX_EVENT_DSCR_CHG;
            }else  if (!strncmp(msg, "MHLEVENT=MHL VBUS power OFF", strlen("MHLEVENT=MHL VBUS power OFF")))
            {
                msg += strlen("MHLEVENT=MHL VBUS power OFF");
                mhlTxEventPacket.eventParam = 1;
                mhlTxEventPacket.event = MHL_TX_EVENT_POW_BIT_CHG;
			}else  if (!strncmp(msg, "MHLEVENT=MHL VBUS power ON", strlen("MHLEVENT=MHL VBUS power ON")))
            {
                msg += strlen("MHLEVENT=MHL VBUS power ON");
                mhlTxEventPacket.eventParam = 0;
                mhlTxEventPacket.event = MHL_TX_EVENT_POW_BIT_CHG;
            }else  if (!strncmp(msg, "MHLEVENT=MHL device detected", strlen("MHLEVENT=MHL device detected")))
            {
                msg += strlen("MHLEVENT=MHL device detected");
                mhlTxEventPacket.event = MHL_TX_EVENT_RGND_MHL;
            }
            if (!strncmp(msg, "DEVPATH=",strlen("DEVPATH="))) 
            {
                msg += strlen("DEVPATH=");
                strcpy(pRecv->dev_path,msg);
            }
            if(mhlTxEventPacket.event != MHL_TX_EVENT_NONE)
            {
                EventsHandler(pRecv,&mhlTxEventPacket);
            }
            while(*msg++);
        }
    }
    return NULL;
}
int Sii_CreateRecvThread(RecvThreadParam_t * pRecvThreadParam)
{
    if(pipe(pRecvThreadParam->pipe_fd) < 0)
    {
        printf("create pipe failed \n");
        return -1 ;
    }
    pRecvThreadParam->recv_sock =  init_recv_sock();
    if(pRecvThreadParam->recv_sock == -1)
    {
        return -1;
    }
    pthread_mutex_init(&pRecvThreadParam->cmdLock, NULL); 
    strcpy(pRecvThreadParam->dev_path,"/devices/virtual/mhl/Sii-Iceman8338");
    pRecvThreadParam->mhlState = GetMhlState(pRecvThreadParam);
    prctl(PR_SET_NAME, "EventRecv_Thread", 0, 0, 0);
    return pthread_create(&(pRecvThreadParam->recv_thread_fd), NULL, recv_thread, (void *)pRecvThreadParam);
}
int Sii_StopRecvThread(RecvThreadParam_t * pRecvThreadParam )
{
    char data[40] ="";
    strcpy(data,"exit");
	write(pRecvThreadParam->pipe_fd[1], "exit", strlen("exit")); 
    pthread_join(pRecvThreadParam->recv_thread_fd, NULL);
    printf("stop thread \n");
    close(pRecvThreadParam->recv_sock);
    close(pRecvThreadParam->pipe_fd[0]);
    close(pRecvThreadParam->pipe_fd[1]);
    return 1;
}
