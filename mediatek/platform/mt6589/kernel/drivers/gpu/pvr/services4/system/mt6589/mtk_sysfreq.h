#ifndef MTK_SYSFREQ_H
#define MTK_SYSFREQ_H

#include "sgxdefs.h"
#include "services_headers.h"

#define TBLTYPE0        0x0
#define TBLTYPE1        0x1
#define TBLTYPE2        0x2
#define TBLTYPE3        0x3

typedef struct PVRSRV_BRIDGE_INPUT_TAG
{
    unsigned int freq;
    unsigned int type;
} PVRSRV_BRIDGE_INPUT;

typedef struct PVRSRV_BRIDGE_PWSRC_INPUT_TAG
{
    unsigned int in;
} PVRSRV_BRIDGE_PWSRC_INPUT;

typedef struct PVRSRV_BRIDGE_PWSRC_RETURN_TAG
{
    int powersrc;
} PVRSRV_BRIDGE_PWSRC_RETURN;

PVRSRV_ERROR MTKSetFreqInfo(unsigned int freq, unsigned int tbltype);


PVRSRV_ERROR MtkInitSetFreq(void);
void MtkInitSetFreqTbl(unsigned int tbltype);
void MtkSetKeepFreq(void);


#endif // MTK_SYSFREQ_H