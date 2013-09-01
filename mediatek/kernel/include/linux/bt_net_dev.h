#ifndef __BT_NET_DEV_H__
#define __BT_NET_DEV_H__


#include <linux/types.h>
#include <linux/compiler.h>

#define BTNIOCNEWUNIT_IP	_IOWR('B', 1, int)	/* create new btn unit */
#define BTNIOCNEWUNIT_ETH	_IOWR('B', 2, int)	/* create new btn unit */
#define BTNIOCSETMAC	_IOWR('B', 3, int)	/* set device mac */


#endif
