#ifndef _VDEC_VERIFY_VPARSER_H264_H_
#define _VDEC_VERIFY_VPARSER_H264_H_

#include <mach/mt_typedefs.h>

typedef enum _H264_DPB_SIZE_T
{
    H264_LEVEL_1_0 = 10,
    H264_LEVEL_1_1 = 11,
    H264_LEVEL_1_2 = 12,    
    H264_LEVEL_1_3 = 13,
    H264_LEVEL_2_0 = 20,
    H264_LEVEL_2_1 = 21,
    H264_LEVEL_2_2 = 22,
    H264_LEVEL_3_0 = 30,
    H264_LEVEL_3_1 = 31,
    H264_LEVEL_3_2 = 32,
    H264_LEVEL_4_0 = 40,
    H264_LEVEL_4_1 = 41,
}H264_DPB_SIZE;

void vVerifyVDecSetPicInfo(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *ptVerMpvDecPrm);
void vPrepareRefPiclist(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm);
void vAssignQuantParam(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *ptVerMpvDecPrm);
void vVerifyFlushBufRefInfo(UINT32 u4InstID);
void vVerifyPrepareFBufInfo(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm);
void vVDecSetPRefPicList(UINT32 u4InstID);
void vVDecSetBRefPicList(UINT32 u4InstID);
void vSetupBRefPicList(UINT32 u4InstID, UINT32 *pu4RefIdx, UINT32 u4TFldListIdx, UINT32 u4BFldListIdx, BOOL *fgDiff);
void vSetupPRefPicList(UINT32 u4InstID, UINT32 *pu4RefIdx, UINT32 u4TFldListIdx, UINT32 u4BFldListIdx);
void vPartitionDPB(UINT32 u4InstID);
void vFillFrameNumGap(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm);
void vAllocateFBuf(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm, BOOL fgFillCurrFBuf);
void vSetCurrFBufIdx(UINT32 u4InstID, UINT32 u4DecFBufIdx);
void vSearchRealPic(UINT32 u4InstID);
void vVerifySlidingWindowProce(UINT32 u4InstID);

void vSetupInterViewRefPicList(UINT32 u4InstID, UINT32 u4BaseViewDpbId, VDEC_INFO_H264_FBUF_INFO_T* ptInterViewDpbInfo, UCHAR ucRefListId, UINT32 *pu4RefIdx);
void vAppendInterviewRefPicList(UINT32 u4InstID,  UINT32* pu4RefIdx, UINT32 u4PicListIdx);
void vPrefix_Nal_Unit_Rbsp_Verify(UINT32 u4InstID);
void vSubset_Seq_Parameter_Set_Rbsp_Verify(UINT32 u4VDecID);

#endif // _PR_EMU_H_

