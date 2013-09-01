#ifndef _VDEC_VERIFY_VDECODE_H_
#define _VDEC_VERIFY_VDECODE_H_

#include "vdec_verify_mpv_prov.h"
#include "../include/vdec_info_common.h"


void vMpvPlay(UINT32 u4InstID);
void vVerifyVDecIsrInit(UINT32 u4InstID);
void vVerifyVDecIsrStop(UINT32 u4InstID);
extern void vDrmaBusySet(UINT32  u4InstID);


#endif

