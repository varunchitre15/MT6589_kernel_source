#ifndef _ETM_TRACE_CONTEXT_H_
#define _ETM_TRACE_CONTEXT_H_

//#include "etm_control.h"
#include "etm_config.h"
#include "etb_control.h"
//#include "pft_state.h"
#include "config.h"

#include <linux/mutex.h>	/* needed by mutex_* */
#include <linux/list.h>		/* needed by list_* */

/** trace context */
struct etm_trace_context_t { //****************** 2 cores
	void __iomem *etm_regs[ETM_NO]; 	/**< ETM registers base address *///TODO:4
	void __iomem *etb_regs;		/**< ETB registers base address */
	void __iomem *funnel_regs;	/**< Funnel registers base address */
	struct mutex mutex;		/**< operation mutex */
	struct etm_config_t etm_config;	/**< ETM hardware configuration */
	struct etm_control_t *etm_control;	/**< ETM control value */

#if 0
	unsigned char *trace_data;
	int trace_size;
	int decoded_trace_size;
#endif

	int state;
	/** each bit indicates whether the related ETM is used */
	unsigned long used_ETMs;
	atomic_t tracing_ETMs; /**< number of ETM which is tracing */

	int inst_set_state;
	unsigned int pre_inst_addr[ETM_NO]; //TODO:4 
	unsigned long long pre_timestamp[ETM_NO]; //TODO:4
	unsigned int data_addr[ETM_NO];
	long long data_value[ETM_NO];
	int sixty_four_bit_data; // may need for different ETMs
	int tag; // may need for different ETMs
	int contextID; // may need for different ETMs

	struct list_head block_list;
	struct etb_control_t etb_control;	/**< ETB control value */
	int etb_total_buf_size;
	char set;
	int log_level;
/*
	struct pft_state_t last_state;
	struct pft_state_t current_state;
*/
};

#endif

