#ifndef _DRIVER_EVENT_H_
#define _DRIVER_EVENT_H_
typedef struct tagRecvThreadParam
{
    int hDevcie;
    pthread_mutex_t	cmdLock;
    pthread_t recv_thread_fd;
    int recv_sock;
    int pipe_fd[2];
    int mhlState;
    uint8_t sendKeyCode;
    char dev_path[128];
}RecvThreadParam_t;
#define		MHL_TX_EVENT_NONE			0x00	
#define		MHL_TX_EVENT_DISCONNECTION	0x01	
#define		MHL_TX_EVENT_CONNECTION		0x02	
#define		MHL_TX_EVENT_RCP_READY		0x03	
#define		MHL_TX_EVENT_RCP_RECEIVED	0x04	
#define		MHL_TX_EVENT_RCPK_RECEIVED	0x05	
#define		MHL_TX_EVENT_RCPE_RECEIVED	0x06	
#define	    MHL_TX_EVENT_DCAP_CHG			0x07	
#define	    MHL_TX_EVENT_DSCR_CHG			0x08	
#define	    MHL_TX_EVENT_POW_BIT_CHG		0x09	
#define	    MHL_TX_EVENT_RGND_MHL			0x0A	
typedef struct {
	uint8_t	event;
	uint8_t	eventParam;
} mhlTxEventPacket_t;
int Sii_CreateThread(RecvThreadParam_t * pParam);
int Sii_StopRecvThread(RecvThreadParam_t * pParam);
void Sii_SendRCPKey(RecvThreadParam_t * pRecv);
#endif
