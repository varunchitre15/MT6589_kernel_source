#ifndef _ETM_REGISTER_H_
#define _ETM_REGISTER_H_

/* registers */
#define ETMCR		(0x0<<2)	/**< Main control register */
#define ETMCCR		(0x1<<2)	/**< Configuration code register */
#define ETMTRIGGER	(0x2<<2)	/**< Trigger event register */
// no 0x3: ASIC control, ETMASICCR
#define ETMSR		(0x4<<2)	/**< Status register */
#define ETMSCR		(0x5<<2)	/**< System configuration register */
/** TraceEnable start/stop control register */
#define ETMTSSCR	(0x6<<2)
// no 0x7: T5raceEnable control 2, ETMTECR2
#define ETMTEEVR	(0x8<<2)	/**< TraceEnable event register */
#define ETMTECR1	(0x9<<2)	/**< TraceEnable control register */
// no 0xa: FIFOFULL Region, ETBFFRR
#define ETMFFLR		(0xb<<2)	/**< FIFOFULL level register */
// ViewData configuration
#define ETMVDEVR	(0xc<<2)	// ViewData event register
#define ETMVDCR1	(0xd<<2)	// ViewData control register 1
#define ETMVDCR2	(0xe<<2)	// ViewData control register 2
#define ETMVDCR3	(0xf<<2)	// ViewData control register 3
/** Address comparator value register 1 */
#define ETMACVR1	(0x10<<2)
/** Address comparator value register 2 */
#define ETMACVR2	(0x11<<2)
/** Address comparator value register 3 */
#define ETMACVR3	(0x12<<2)
/** Address comparator value register 4 */
#define ETMACVR4	(0x13<<2)
/** Address comparator value register 5 */
#define ETMACVR5	(0x14<<2)
/** Address comparator value register 6 */
#define ETMACVR6	(0x15<<2)
/** Address comparator value register 7 */
#define ETMACVR7	(0x16<<2)
/** Address comparator value register 8 */
#define ETMACVR8	(0x17<<2)
/** Address comparator value register 9 */
#define ETMACVR9	(0x18<<2)
/** Address comparator value register 10 */
#define ETMACVR10	(0x19<<2)
/** Address comparator value register 11 */
#define ETMACVR11	(0x1a<<2)
/** Address comparator value register 12 */
#define ETMACVR12	(0x1b<<2)
/** Address comparator value register 13 */
#define ETMACVR13	(0x1c<<2)
/** Address comparator value register 14 */
#define ETMACVR14	(0x1d<<2)
/** Address comparator value register 15 */
#define ETMACVR15	(0x1e<<2)
/** Address comparator value register 16 */
#define ETMACVR16	(0x1f<<2)
/** Address comparator access type register 1 */
#define ETMACTR1	(0x20<<2)
/** Address comparator access type register 2 */
#define ETMACTR2	(0x21<<2)
/** Address comparator access type register 3 */
#define ETMACTR3	(0x22<<2)
/** Address comparator access type register 4 */
#define ETMACTR4	(0x23<<2)
/** Address comparator access type register 5 */
#define ETMACTR5	(0x24<<2)
/** Address comparator access type register 6 */
#define ETMACTR6	(0x25<<2)
/** Address comparator access type register 7 */
#define ETMACTR7	(0x26<<2)
/** Address comparator access type register 8 */
#define ETMACTR8	(0x27<<2)
/** Address comparator access type register 9 */
#define ETMACTR9	(0x28<<2)
/** Address comparator access type register 10 */
#define ETMACTR10	(0x29<<2)
/** Address comparator access type register 11 */
#define ETMACTR11	(0x2a<<2)
/** Address comparator access type register 12 */
#define ETMACTR12	(0x2b<<2)
/** Address comparator access type register 13 */
#define ETMACTR13	(0x2c<<2)
/** Address comparator access type register 14 */
#define ETMACTR14	(0x2d<<2)
/** Address comparator access type register 15 */
#define ETMACTR15	(0x2e<<2)
/** Address comparator access type register 16 */
#define ETMACTR16	(0x2f<<2)
// Data value comparators (only the even-numbered registers can be implemented. The odd-numbered registers are reserved
#define ETMDCVR1	(0x30<<2)	// Data comparator value register 1
#define ETMDCVR2	(0x32<<2)	// Data comparator value register 2
#define ETMDCVR3	(0x34<<2)	// Data comparator value register 3
#define ETMDCVR4	(0x36<<2)	// Data comparator value register 4
#define ETMDCVR5	(0x38<<2)	// Data comparator value register 5
#define ETMDCVR6	(0x3a<<2)	// Data comparator value register 6
#define ETMDCVR7	(0x3c<<2)	// Data comparator value register 7
#define ETMDCVR8	(0x3e<<2)	// Data comparator value register 8

#define ETMDCMR1	(0x40<<2)	// Data comparator mask register 1
#define ETMDCMR2	(0x42<<2)	// Data comparator mask register 2
#define ETMDCMR3	(0x44<<2)	// Data comparator mask register 3
#define ETMDCMR4	(0x46<<2)	// Data comparator mask register 4
#define ETMDCMR5	(0x48<<2)	// Data comparator mask register 5
#define ETMDCMR6	(0x4a<<2)	// Data comparator mask register 6
#define ETMDCMR7	(0x4c<<2)	// Data comparator mask register 7
#define ETMDCMR8	(0x4e<<2)	// Data comparator mask register 8

// counters
#define ETMCNTRLDVR1	(0x50<<2)	/**< Counter reload value register 1 */
#define ETMCNTRLDVR2	(0x51<<2)	/**< Counter reload value register 2 */
#define ETMCNTRLDVR3	(0x52<<2)	/**< Counter reload value register 3 */
#define ETMCNTRLDVR4	(0x53<<2)	/**< Counter reload value register 4 */
#define ETMCNTENR1	(0x54<<2)	/**< Counter enable event register 1 */
#define ETMCNTENR2	(0x55<<2)	/**< Counter enable event register 2 */
#define ETMCNTENR3	(0x56<<2)	/**< Counter enable event register 3 */
#define ETMCNTENR4	(0x57<<2)	/**< Counter enable event register 4 */
#define ETMCNTRLDEVR1	(0x58<<2)	/**< Counter reload event register 1 */
#define ETMCNTRLDEVR2	(0x59<<2)	/**< Counter reload event register 2 */
#define ETMCNTRLDEVR3	(0x5a<<2)	/**< Counter reload event register 3 */
#define ETMCNTRLDEVR4	(0x5b<<2)	/**< Counter reload event register 4 */
#define ETMCNTVR1	(0x5c<<2)	/**< Counter value register 1 */
#define ETMCNTVR2	(0x5d<<2)	/**< Counter value register 2 */
#define ETMCNTVR3	(0x5e<<2)	/**< Counter value register 3 */
#define ETMCNTVR4	(0x5f<<2)	/**< Counter value register 4 */
/** Sequencer state transition event register 1 */
#define ETMSQabEVR1	(0x60<<2)
/** Sequencer state transition event register 2 */
#define ETMSQabEVR2	(0x61<<2)
/** Sequencer state transition event register 3 */
#define ETMSQabEVR3	(0x62<<2)
/** Sequencer state transition event register 4 */
#define ETMSQabEVR4	(0x63<<2)
/** Sequencer state transition event register 5 */
#define ETMSQabEVR5	(0x64<<2)
/** Sequencer state transition event register 6 */
#define ETMSQabEVR6	(0x65<<2)
#define ETMSQR		(0x67<<2)	/**< Current sequencer state register */
/** External output event register 1 */
#define ETMEXTOUTEVR1	(0x68<<2)
/** External output event register 2 */
#define ETMEXTOUTEVR2	(0x69<<2)
/** External output event register 3 */
#define ETMEXTOUTEVR3	(0x6a<<2)
/** External output event register 4 */
#define ETMEXTOUTEVR4	(0x6b<<2)
/** Context ID comparator value register 1 */
#define ETMCIDCVR1	(0x6c<<2)
/** Context ID comparator value register 2 */
#define ETMCIDCVR2	(0x6d<<2)
/** Context ID comparator value register 3 */
#define ETMCIDCVR3	(0x6e<<2)
/** Context ID comparator mask register */
#define ETMCIDCMR	(0x6f<<2)
/** Implementation specific register 0 */
#define ETMIMPSPEC0	(0x70<<2)
/** Implementation specific register 1 */
#define ETMIMPSPEC1	(0x71<<2)
/** Implementation specific register 2 */
#define ETMIMPSPEC2	(0x72<<2)
/** Implementation specific register 3 */
#define ETMIMPSPEC3	(0x73<<2)
/** Implementation specific register 4 */
#define ETMIMPSPEC4	(0x74<<2)
/** Implementation specific register 5 */
#define ETMIMPSPEC5	(0x75<<2)
/** Implementation specific register 6 */
#define ETMIMPSPEC6	(0x76<<2)
/** Implementation specific register 7 */
#define ETMIMPSPEC7	(0x77<<2)
/** Synchronization frequency register */
#define ETMSYNCFR	(0x78<<2)
#define ETMIDR		(0x79<<2)	/**< ID register */
/** Configuration code extension register */
#define ETMCCER		(0x7a<<2)
/** Extended external input selection register */
#define ETMEXTINSELR	(0x7b<<2)
/** TraceEnable start/stop embeddedICE control register */
#define ETMTESSEICR	(0x7c<<2)
/** EmbeddedICE behavior control register */
#define ETMEIBCR	(0x7d<<2)
#define ETMTSEVR	(0x7e<<2)	/**< Timestamp event register */
#define ETMAUXCR	(0x7f<<2)	/**< Auxiliary control register */
#define ETMTRACEIDR	(0x80<<2)	/**< CoreSight trace ID register */
#define ETMIDR2		(0x82<<2)	// ETM ID register 2
#define ETMVMIDCVR	(0x90<<2)	/**< VMID comparator value register */
#define ETMOSLAR	(0xc0<<2)	/**< OS lock access register */
#define ETMOSLSR	(0xc1<<2)	/**< OS lock status register */
#define ETMOSSRR	(0xc2<<2)	/**< OS save and restore register */
/** Device power-down control register */
#define ETMPDCR		(0xc4<<2)
/** Device power-down status register */
#define ETMPDSR		(0xc5<<2)
/** Integration mode control register */
#define ETMITCTRL	(0x3c0<<2)
#define ETMCLAIMSET	(0x3e8<<2)	/**< Claim tag set register */
#define ETMCLAIMCLR	(0x3e9<<2)	/**< Claim tag clear register */
#define ETMLAR		(0x3ec<<2)	/**< Lock access register */
#define ETMLSR		(0x3ed<<2)	/**< Lock status register */
#define ETMAUTHSTATUS	(0x3ee<<2)	/**< Authentication status register */
#define ETMDEVID	(0x3f2<<2)	/**< Device configuration register */
#define ETMDEVTYPE	(0x3f3<<2)	/**< Device type register */
#define ETMPIDR4	(0x3f4<<2)	/**< Peripheral ID register 4 */
#define ETMPIDR5	(0x3f5<<2)	/**< Peripheral ID register 5 */
#define ETMPIDR6	(0x3f6<<2)	/**< Peripheral ID register 6 */
#define ETMPIDR7	(0x3f7<<2)	/**< Peripheral ID register 7 */
#define ETMPIDR0	(0x3f8<<2)	/**< Peripheral ID register 0 */
#define ETMPIDR1	(0x3f9<<2)	/**< Peripheral ID register 1 */
#define ETMPIDR2	(0x3fa<<2)	/**< Peripheral ID register 2 */
#define ETMPIDR3	(0x3fb<<2)	/**< Peripheral ID register 3 */
#define ETMCIDR0	(0x3fc<<2)	/**< Component ID register 0 */
#define ETMCIDR1	(0x3fd<<2)	/**< Component ID register 1 */
#define ETMCIDR2	(0x3fe<<2)	/**< Component ID register 2 */
#define ETMCIDR3	(0x3ff<<2)	/**< Component ID register 3 */

/* control bits in registers */

/** magic number which enables further write access to ETM */
#define ETMLAR_UNLOCK_MAGIC	0xc5acce55
#define ETMOSLAR_LOCK_MAGIC	0xc5acce55

/** bits mask which indicates number of pairs of address comparators */
#define ETMCCR_AC_PAIRS		(0xf)
/** bits mask which indicates number of counters */
#define ETMCCR_COUNTERS		(7<<13)
/** bits mask which indicates whether the sequencer is present */
#define ETMCCR_SEQUENCER	(1<<16)
/** bits mask which indicates number of external inputs */
#define ETMCCR_EXT_INPUT	(7<<17)
/** bits mask which indicates number of external outputs */
#define ETMCCR_EXT_OUTPUT	(7<<20)
/** bits mask which indicates whether the FIFOFULL logic is present */
#define ETMCCR_FIFOFULL		(1<<23)
/** bits mask which indicates number of context ID comparators implemented */
#define ETMCCR_CIDC		(3<<24)
/** bits mask which indicates whether the trace start/stop block is present */
#define ETMCCR_TRACE_SS_BLOCK	(1<<26)

/** bits mask which indicates number of extended external input selectors */
#define ETMCCER_EXTEND_EXT_INPUT_SELECTOR	(7)
/** bits mask which indicates size of extended external input bus */
#define ETMCCER_EXTEND_EXT_INPUT_BUS		(0xff<<3)
/** bits mask which indicates number of instrumentation resources supported */
#define ETMCCER_INSTRUMENTATION_RES		(7<<13)
/**
 * bits mask which indicates number of EmbeddedICE watchpoint comparator inputs
 * implemented
 */
#define ETMCCER_EMBEDDED_ICE_WATCHPOINT_INPUT	(0xf<<16)
/**
 * bits mask which indicates whether trace start/stop block can use EmbeddedICE
 * watchpoint comparator inputs
 */
#define ETMCCER_SS_BLOCK_USE_EMBEDDED_ICE_INPUT	(1<<20)
/**
 * bits mask which indicates whether EmbeddedICE Behavior Control Register
 * implemented
 */
#define ETMCCER_EIBCR				(1<<21)
/** bits mask which indicates whether timestamping implemented */
#define ETMCCER_TIMESTAMP			(1<<22)
/** bits mask which indicates whether return stack implemented */
#define ETMCCER_RET_STACK			(1<<23)
/**
 * bits mask which indicates whether DMB and DSB operations are waypoint
 * instructions
 */
#define ETMCCER_DMB_IS_WAYPOINT			(1<<24)
/**
 * bits mask which indicates whether generates timestamps for DMB and DSB
 * operations
 */
#define ETMCCER_GENERATE_TIMESTAMP_FOR_DMB	(1<<25)
/** bits mask which indicates whether Virtualization Extensions implemented */
#define ETMCCER_VIRTUALIZATION_EXT		(1<<26)
/**
 * bits mask which indicates whether counter 1 is implemented as a reduced
 * function counter
 */
#define ETMCCER_REDUCED_FUNCTION_COUNTER	(1<<27)
/**
 * bits mask which specifies the encoding used for the timestamp value in the
 * timestamp packet
 */
#define ETMCCER_TIMESTAMP_ENCODE		(1<<28)
/**
 * bits mask which specifies the maximum size, in bits, of the timestamp for
 * the timestamp packet
 */
#define ETMCCER_TIMESTAMP_SIZE			(1<<29)

/** bits mask which indicates whether the FIFOFULL is supported by ETM */
#define ETMSCR_FIFOFULL			(1<<8)
/** bits mask which indicates number of supported processors minus 1 */
#define ETMSCR_PROCESSORS_MINUS_1	(7<<12)

/** bits mask which indicates implementation revision */
#define ETMIDR_REVISION		(0xf)
/** bits mask which indicates minor architecture version number */
#define ETMIDR_MINOR		(0xf<<4)
/** bits mask which indicates major architecture version number */
#define ETMIDR_MAJOR		(0xf<<8)
/** bits mask which indicates whether supports for 32-bit Thumb instructions */
#define ETMIDR_THUMB		(1<<18)
/** bits mask which indicates whether supports for security extensions */
#define ETMIDR_SECURITY		(1<<19)
/** bits mask which indicates whether supports for altervative branch address encoding */
#define ETMIDR_ALTERNATIVE		(1<<20)
/** bits mask which indicates implementer code */
#define ETMIDR_IMPLEMENTER	(0xff<<24)

/** bits mask which indicates include/exclude control */
#define ETMTECR1_TE_EXCLUDE_INCLUDE_CONTROL	(1<<24)
/** bits mask which indicates whether enable trace start/stop control */
#define ETMTECR1_TE_SS_ENABLE_CONTROL		(1<<25)

/** bits which indicates context ID comparator control */
#define ETMACTRn_CTXID_COMPARATOR_BIT	8
/** bits which indicates secure state comparison control */
#define ETMACTRn_SECURE_LEVEL_BIT1	10
/** bits which indicates non-secure state comparison control */
#define ETMACTRn_SECURE_LEVEL_BIT2	12

/** bits which indicates PowerDown control */
#define ETMCR_POWER_DOWN_BIT		0
/** bits mask which indicates PowerDown control */
#define ETMCR_POWER_DOWN		(1<<0)

// bits which indicates monitor CPRT
#define ETMCR_MONITOR_CPRT_BIT	1
// bit mask which indicates monitor CPRT control
#define ETMCR_MONITOR_CPRT	(1<<1)

// bits which indicates data access control
#define ETMCR_DATA_ACCESS_BIT		2
// bits mask whick indicates data access control
#define ETMCR_DATA_ACCESS		(3<<2)
// no data tracing
#define ETMCR_DATA_ACCESS_NO		0
// only data value portion
#define ETMCR_DATA_ACCESS_VALUE		1
// only data address portion
#define ETMCR_DATA_ACCESS_ADDR		2
// both data address and value
#define ETMCR_DATA_ACCESS_ADDR_VALUE		3

/**
 * bits which indicates whether enables the use of the FIFOFULL output to
 * stall the processor to prevent overflow
 */
#define ETMCR_STALL_PROCESSOR_BIT	7
/**
 * bits mask which indicates whether enables the use of the FIFOFULL output to
 * stall the processor to prevent overflow
 */
#define ETMCR_STALL_PROCESSOR		(1<<7)
/** bits which indicates whether enables branch broadcasting */
#define ETMCR_BRANCH_BROADCAST_BIT	8
/** bits mask which indicates whether enables branch broadcasting */
#define ETMCR_BRANCH_BROADCAST		(1<<8)
/**
 * bits which indicates whether enables a debugger to force the ARM processor
 * into Debug state
 */
#define ETMCR_DEBUG_REQUEST_BIT		9
/**
 * bits mask which indicates whether enables a debugger to force the ARM
 * processor into Debug state
 */
#define ETMCR_DEBUG_REQUEST		(1<<9)
/** bits which indicates whether enables programming bit */
#define ETMCR_PROGRAM_BIT		10
/** bits mask which indicates whether enables programming bit */
#define ETMCR_PROGRAM			(1<<10)
/** bits which indicates whether enables cycle-accurate tracing */
#define ETMCR_CYCLE_ACCURATE_BIT	12
/** bits mask which indicates whether enables external ETMEN pin */
#define ETMCR_ETMEN		(1<<11)
#define ETMCR_ETMEN_BIT	11
/** bits mask which indicates whether enables cycle-accurate tracing */
#define ETMCR_CYCLE_ACCURATE		(1<<12)
/** bits which specifies the number of bytes of traced context ID */
#define ETMCR_CONTEXTIDSIZE_BIT		14
/** bits mask which specifies the number of bytes of traced context ID */
#define ETMCR_CONTEXTIDSIZE		(3<<14)

//bits which indicates whether enables suppress data
#define ETMCR_SUPPRESS_DATA_BIT		18
// bits mask which indicates whether enables suppress data
#define ETMCR_SUPPRESS_DATA			(1<<18)
//bits which indicates whether enables filter CPRT
#define ETMCR_FILTER_CPRT_BIT		19
// bits mask which indicates whether enables filter CPRT
#define ETMCR_FILTER_CPRT			(1<<19)
//bits which indicates whether enables data only mode
#define ETMCR_DATA_ONLY_MODE_BIT		20
// bits mask which indicates whether enables data only mode
#define ETMCR_DATA_ONLY_MODE			(1<<20)

/**
 * bits which indicates whether the Instrumentation resources can only be
 * controlled when the processor is in a privileged mode
 */
#define ETMCR_INSTU_RESOURCE_ACCESS_BIT	24
/**
 * bits mask which indicates whether the Instrumentation resources can only be
 * controlled when the processor is in a privileged mode
 */
#define ETMCR_INSTU_RESOURCE_ACCESS	(1<<24)
/** bits which indicates whether enables timestamping */
#define ETMCR_TIMESTAMP_BIT		28
/** bits mask which indicates whether enables timestamping */
#define ETMCR_TIMESTAMP			(1<<28)
/** bits which indicates whether enables use of the return stack */
#define ETMCR_RETURN_STACK_BIT		29
/** bits mask which indicates whether enables use of the return stack */
#define ETMCR_RETURN_STACK		(1<<29)
/** bits which indicates whether enables VMID tracing */
#define ETMCR_VMID_BIT			30
/** bits mask which indicates whether enables VMID tracing */
#define ETMCR_VMID			(1<<30)

/** bits mask which indicates the current effective value of programming bit */
#define ETMSR_PROGRAM	(1<<1)

#endif

