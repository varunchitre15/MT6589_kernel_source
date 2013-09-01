
#ifndef __CUST_BT_H__
#define __CUST_BT_H__

#ifdef MTK_COMBO_SUPPORT
/* SERIAL PORT */
#define CUST_BT_SERIAL_PORT "/dev/stpbt"
/* BAUDRATE */
#define CUST_BT_BAUD_RATE   4000000 /* use 4M but is not controlled by bt directly */
#else
/* SERIAL PORT */
#define CUST_BT_SERIAL_PORT "/dev/ttyMT2"
/* BAUDRATE */
#define CUST_BT_BAUD_RATE   3250000
//#define CUST_BT_BAUD_RATE   921600
#endif

#endif /* __CUST_BT_H__ */
