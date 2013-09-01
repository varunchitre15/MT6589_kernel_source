#if !defined(_TRACE_INT_SCHED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_INT_SCHED_H

#include <linux/tracepoint_define.h>

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mtk_events 

TRACE_EVENT(new_trace_event,

	TP_PROTO(unsigned int irq,
		 int pid),

	TP_ARGS(irq, pid),

	TP_STRUCT__entry(
		__field(	unsigned int,	member1			)
		__field(	int,	member2			)
	),

	TP_fast_assign(
		__entry->member1	= irq;
		__entry->member2	= pid;
	),

	TP_printk("output string for new trace event %d,%d",
		__entry->member1,
		__entry->member2
	)
);

#endif /* _TRACE_SCHED_H */
#ifndef TRACE_EVENT
#undef _TRACEPOINT_DEFINE
#endif
/* This part must be outside protection */
#include <trace/define_trace.h>
