#ifndef _ETM_CONFIG_H_
#define _ETM_CONFIG_H_

/** ETM hardware configuration */
struct etm_config_t
{
	/* from Configuration Code Register */

	unsigned int AC_pairs:4; /**< number of pairs of address comparators */
	unsigned int :9; /* reserved */
	unsigned int counters:3; /**< number of counters */
	/** whether the sequencer is present */
	unsigned int has_sequencer:1;
	/** number of external inputs, supplied by the ASIC */
	unsigned int ext_inputs:3;
	/** number of external outputs, supplied by the ASIC */
	unsigned int ext_outputs:3;
	/** whether the FIFOFULL logic is present */
	unsigned int has_FIFOFULL:1;
	/** number of Context ID comparators implemented */
	unsigned int ctxID_comparators:2;
	/** whether the trace start/stop block is present */
	unsigned int has_trace_SS_block:1;
	unsigned int :1; /*TODO*/
	unsigned int :3; /* reserved */
	unsigned int :1; /*TODO*/

	/* from Configuration Code Extension Register */

	/** number of extended external input selectors */
	unsigned int extended_ext_input_selectors:3;
	/** size of extended external input bus */
	unsigned int extended_ext_input_bus_size:8;
	unsigned int :1; /*TODO*/
	unsigned int :1; /* reserved */
	/** number of Instrumentation resources supported */
	unsigned int instrument_resources:3;
	/** number of EmbeddedICE watchpoint comparator inputs implemented */
	unsigned int embeddedICE_watchpoint_comparator_inputs:4;
	/**
	 * whether trace Start/Stop block can use EmbeddedICE watchpoint
	 * comparator inputs
	 */
	unsigned int trace_SS_block_use_embeddedICE_input:1;
	/** whether EmbeddedICE Behavior Control Register implemented */
	unsigned int has_eibcr:1;
	/** whether timestamping implemented */
	unsigned int has_timestamping:1;
	unsigned int has_ret_stack:1; /**< whether return stack implemented */
	/** whether DMB and DSB operations are waypoint instructions */
	unsigned int dmb_is_waypoint:1;
	/** whether generate timestamps for DMB and DSB operations */
	unsigned int generate_TS_for_dmb:1;
	/** whether virtualization extensions implemented */
	unsigned int has_virtualization:1;
	/** whether counter 1 is implemented as a reduced function counter */
	unsigned int has_reduced_func_counter:1;
	unsigned int ts_encoding:1; /**< timestamp encoding */
	unsigned int ts_size:1; /**< timestamp size */
	unsigned int :2; /* reserved */

	/* from System Configuration Register */

	unsigned int :8; /* reserved */
	/** whether FIFOFULL is supported */
	unsigned int system_supports_FIFOFULL:1;
	unsigned int :3; /* reserved */
	/** number of supported processors minus 1 */
	unsigned int processors_minus_1:3;
	unsigned int :17; /* reserved */

	/* from ID Register */

	unsigned int revision:4; /**< implementation revision */
	unsigned int minor:4; /**< minor architecture version number */
	unsigned int major:4; /**< major architecture version number */
	unsigned int :6; /* reserved */
	/** whether support for 32-bit Thumb instructions */
	unsigned int supports_thumb:1;
	/** whether support for Security Extensions */
	unsigned int supports_secure_extension:1;
	unsigned int :4; /* reserved */
	unsigned int implementor:8; /**< implementor code */

	/* implementation defined feature */

	/** whether the FIFOFULL logic is implemented */
	unsigned int impl_defined_feature_FIFOFULL:1;
	/** whether branch broadcast is implemented */
	unsigned int impl_defined_feature_branch_broadcast:1;
	/** whether cycle-accurate tracing is implemented */
	unsigned int impl_defined_feature_cycle_accurate_tracing:1;
	/** whether context ID tracing is implemented */
	unsigned int impl_defined_feature_contextID_tracing:1;
	/** whether time stamp is implemented */
	unsigned int impl_defined_feature_timestamping:1;
	/** whether return stack is implemented */
	unsigned int impl_defined_feature_return_stack:1;
	/** whether VMID tracing is implemented */
	unsigned int impl_defined_feature_VMID_tracing:1;
	// whether monitor CPRT tracing is implemented 
	unsigned int impl_defined_feature_monitor_CPRT_tracing:1;
	// whether data access tracing is implemented 
	unsigned int impl_defined_feature_data_access_tracing:2;
	// whether suppress data tracing is implemented 
	unsigned int impl_defined_feature_suppress_data_tracing:1;
	// whether filter CPRT tracing is implemented 
	unsigned int impl_defined_feature_filter_CPRT_tracing:1;
	// whether data only mode tracing is implemented 
	unsigned int impl_defined_feature_data_only_mode_tracing:1;
} __attribute__((packed));


// only for PTM, not for ETM
/** whether current PTM is PFT version 1.0 */
#define IS_PFTv10(ctx, n) ((ctx)->etm_config.major == 3 && (ctx)->etm_config.minor == 0) //TODO:n
/** whether current PTM is PFT version 1.1 */
#define IS_PFTv11(ctx, n) ((ctx)->etm_config.major == 3 && (ctx)->etm_config.minor == 1)
/** whether current PTM implements Virtualization extension */

#define SUPPORTS_VIRTUAL_EXTENSION(ctx, n) (\
	(ctx)->etm_config.has_virtualization == 1)

#endif

