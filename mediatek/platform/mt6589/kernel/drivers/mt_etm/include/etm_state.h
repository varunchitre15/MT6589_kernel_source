#ifndef _ETM_STATE_H_
#define _ETM_STATE_H_

enum {
	TRACE_STATE_STOP = 0,		/**< trace stopped */
	TRACE_STATE_TRACING ,		/**< tracing */
	TRACE_STATE_UNFORMATTING,	/**< unformatting frame */
	TRACE_STATE_UNFORMATTED,	/**< frame unformatted */
	TRACE_STATE_SYNCING,		/**< syncing to trace head */ //TODO
	TRACE_STATE_PARSING,		/**< decoding packet */
};

/** check whether we are in some state */
#define IS_IN_STATE(ctx, _state) ((ctx)->state == _state)

#endif

