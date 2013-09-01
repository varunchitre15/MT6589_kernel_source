#ifndef _CCCI_TTY_H
#define _CCCI_TTY_H

#define  CCCI_TTY_TX_BUFFER_SIZE      (16 * 1024)
#define  CCCI_TTY_RX_BUFFER_SIZE      (16 * 1024)

#define  CCCI_TTY_MODEM      0
#define  CCCI_TTY_META       1
#define  CCCI_TTY_IPC	     2
typedef struct
{
    unsigned read;
    unsigned write;
    unsigned length;
} buffer_control_tty_t;


typedef struct
{
    buffer_control_tty_t  rx_control;
    buffer_control_tty_t  tx_control;
    unsigned char     rx_buffer[CCCI_TTY_RX_BUFFER_SIZE];
    unsigned char     tx_buffer[CCCI_TTY_TX_BUFFER_SIZE];
} shared_mem_tty_t;

#if 0
extern int ccci_uart_setup(int port, int *addr_virt, int *addr_phy, int *len);
extern int ccci_uart_base_req(int port, void *addr_phy, int *len);
extern int ccci_fs_setup(int *addr_virt, int *addr_phy, int *len);
extern int ccci_fs_base_req(void *addr_phy, int *len);
extern int ccci_pmic_setup(int *addr_virt, int *addr_phy, int *len);
extern int ccci_pmic_base_req(void *addr_phy, int *len);
extern int ccci_reset_register(char *name);
extern int ccci_reset_request(int handle);
extern int ccci_reset_index(void);
extern int ccci_ipc_setup(int *addr_virt, int *addr_phy, int *len);
extern int ccci_ipc_base_req(void *addr_phy, int *len);
#endif
extern void ccci_reset_buffers(shared_mem_tty_t *shared_mem);
extern int __init ccci_tty_init(void);
extern void __exit ccci_tty_exit(void);


#define CCCI_TTY_SMEM_SIZE sizeof(shared_mem_tty_t)
#define CCCI_TTY_PORT_COUNT 6


#endif // !_CCCI_TTY_H
