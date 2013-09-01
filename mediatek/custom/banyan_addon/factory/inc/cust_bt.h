
#ifndef FTM_CUST_BT_H
#define FTM_CUST_BT_H

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

#endif /* FTM_CUST_BT_H */
