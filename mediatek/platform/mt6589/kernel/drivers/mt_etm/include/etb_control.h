#ifndef _ETB_CONTROL_H_
#define _ETB_CONTROL_H_

/** formatter mode */
enum {
	FORMATTER_MODE_BYPASS = 0, /**< bypass mode */
	FORMATTER_MODE_NORMAL, /**< normal mode */
	FORMATTER_MODE_CONTINUOUS, /**< continuous mode */
};

/** ETB control value */ 
struct etb_control_t
{
	int formatter_mode; /**< ETB formattor mode */
	unsigned int formatter_control_value; /**< ETB formattor control value */
};

#endif

