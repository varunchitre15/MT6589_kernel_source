
#ifndef __DBG_FLICKER_PARAM_H__
#define __DBG_FLICKER_PARAM_H__

// Flicker debug info
#define FLICKER_DEBUG_TAG_VERSION (1)
#define FLICKER_DEBUG_TAG_SIZE 100 

typedef struct
{
    AAA_DEBUG_TAG_T Tag[FLICKER_DEBUG_TAG_SIZE];
} FLICKER_DEBUG_INFO_T;

// flicker Debug Tag
enum
{
	// flicker
	FLICKER_TAG_VERSION = 0,    
       FLICKER_TAG_MAX
};

#endif // __DBG_FLICKER_PARAM_H__

