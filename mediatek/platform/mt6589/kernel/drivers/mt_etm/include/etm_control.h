#ifndef _ETM_CONTROL_H_
#define _ETM_CONTROL_H_

#include "etm_event.h"

/** ETM counters control value */
struct etm_counter_control_t
{
	struct etm_event_t enabling_event;	/**< enabling event */
	int value;				/**< counter value */
	struct etm_event_t reload_event;	/**< reload event */
	int reload_value;			/**< reload value */
};

/** ETM control value */
struct etm_control_t
{
	int TE_SS_control; /**< trace start/stop control enable/disable */ //TODO:duplicate with TE_control_value
	int TE_start_control;
	int TE_stop_control;
	int TE_exclude_include_control; //TODO:duplicate with TE_control_value
	
	/**
	 * each bit indicates whether the related ARC is used, currently only
	 * bit[0-7] may be used
	 */ //TODO:duplicate with TE_control_value
	unsigned long TE_exclude_include_ARCs;
	unsigned int *TE_address_comparator_values; 
	int *TE_exclude_include_ARCs_ctxIDs;
	int *TE_exclude_include_ARCs_secure_levels;
	int *TE_exclude_include_ARCs_non_secure_levels;
	struct etm_event_t *TE_enabling_event;
	unsigned int TE_control_value;
	unsigned int main_control_value;
	struct etm_counter_control_t *counter_controls;
};

#endif

