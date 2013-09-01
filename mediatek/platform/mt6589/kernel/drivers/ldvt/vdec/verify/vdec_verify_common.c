
#include "vdec_verify_common.h"
#include "vdec_verify_mpv_prov.h"


// *********************************************************************
// Function    : void vOutputPOCData(UINT32 dwDecOrder)
// Description : 
// Parameter  :
// Return      : void
// *********************************************************************
void vOutputPOCData(UINT32 dwDecOrder)
{

}


// *********************************************************************
// Function    : void ucVDecGetMinPOCFBuf()
// Description : Output 1 frm buff in DPB when DPB full
// Parameter   : 
// Return      : None
// *********************************************************************
UCHAR ucVDecGetMinPOCFBuf(UINT32 u4InstID,VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm, BOOL fgWithEmpty)
{
    UINT32 u4Idx;
    UINT32 u4MinPOCFBufIdx;
    INT32 i4MinPOC;

    i4MinPOC = 0x7fffffff;
    u4MinPOCFBufIdx = 0xFF;
    for(u4Idx=0; u4Idx < _tVerMpvDecPrm[u4InstID].SpecDecPrm.rVDecH264DecPrm.ucMaxFBufNum; u4Idx++)
    {
        if((_ptFBufInfo[u4InstID][u4Idx].eH264DpbStatus == H264_DPB_STATUS_OUTPUTTED)
            && fgIsNonRefFBuf(u4InstID, u4Idx))
        {
            vVerifyClrFBufInfo(u4InstID, u4Idx);
        }
        if(fgWithEmpty)
        {
		if((i4MinPOC != 0x80000001) && (_ptFBufInfo[u4InstID][u4Idx].eH264DpbStatus == H264_DPB_STATUS_EMPTY))
	    	{
			i4MinPOC = 0x80000001;
			u4MinPOCFBufIdx = u4Idx;
	    	}
		else if((i4MinPOC > _ptFBufInfo[u4InstID][u4Idx].i4POC) && (_ptFBufInfo[u4InstID][u4Idx].eH264DpbStatus == H264_DPB_STATUS_DECODED))
		{
	            i4MinPOC = _ptFBufInfo[u4InstID][u4Idx].i4POC;
	            u4MinPOCFBufIdx = u4Idx;
		}
            //break;
        }
        else // Flush
    	{
	        // Need to take care of field empty
	        if(i4MinPOC > _ptFBufInfo[u4InstID][u4Idx].i4POC)
	        {
	            i4MinPOC = _ptFBufInfo[u4InstID][u4Idx].i4POC;
	            u4MinPOCFBufIdx = u4Idx;
	        }
    	}
#if 0        
        if(fgWithEmpty
            && (i4MinPOC != 0x80000001)
            && (_ptFBufInfo[u4Idx].eH264DpbStatus == H264_DPB_STATUS_EMPTY))
        {
            i4MinPOC = 0x80000001;
            u4MinPOCFBufIdx = u4Idx;
            //break;
        }
        // Need to take care of field empty
        else if(i4MinPOC > _ptFBufInfo[u4Idx].iPOC)
        {
            i4MinPOC = _ptFBufInfo[u4Idx].iPOC;
            u4MinPOCFBufIdx = u4Idx;
        }
#endif        
    }  

    return u4MinPOCFBufIdx;
}


// *********************************************************************
// Function    : void vVerifyClrFBufInfo(UINT32 u4InstID, UINT32 u4FBufIdx)
// Description : flush selected Frm Buf info
// Parameter   : UINT32 u4FBufIdx: Selected frm buf idx
// Return      : None
// *********************************************************************
void vVerifyClrFBufInfo(UINT32 u4InstID, UINT32 u4FBufIdx)
{
  _ptFBufInfo[u4InstID][u4FBufIdx].fgNonExisting = FALSE;    
  _ptFBufInfo[u4InstID][u4FBufIdx].eH264DpbStatus = H264_DPB_STATUS_EMPTY;
  _ptFBufInfo[u4InstID][u4FBufIdx].ucFBufStatus = NO_PIC;    
  
  _ptFBufInfo[u4InstID][u4FBufIdx].ucBFldRefType = NREF_PIC;    
  _ptFBufInfo[u4InstID][u4FBufIdx].ucFBufRefType = NREF_PIC;
  _ptFBufInfo[u4InstID][u4FBufIdx].ucTFldRefType = NREF_PIC;
  
  _ptFBufInfo[u4InstID][u4FBufIdx].u4FrameNum = 0xffffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].i4FrameNumWrap = 0xefffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].i4PicNum = 0xefffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].i4TFldPicNum = 0xefffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].i4BFldPicNum = 0xefffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].u4LongTermFrameIdx = 0xffffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].u4TFldLongTermFrameIdx = 0xffffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].u4BFldLongTermFrameIdx = 0xffffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].i4LongTermPicNum = 0xefffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].i4TFldLongTermPicNum = 0xefffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].i4BFldLongTermPicNum = 0xefffffff;    

  _ptFBufInfo[u4InstID][u4FBufIdx].ucFBufStatus = 0;
  _ptFBufInfo[u4InstID][u4FBufIdx].u4TFldPara = 0;
  _ptFBufInfo[u4InstID][u4FBufIdx].u4BFldPara = 0;
  
  
  _ptFBufInfo[u4InstID][u4FBufIdx].i4POC = 0x7fffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].i4TFldPOC = 0x7fffffff;
  _ptFBufInfo[u4InstID][u4FBufIdx].i4BFldPOC = 0x7fffffff;
}


// *********************************************************************
// Function    : void vFlushDPB(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm, BOOL fgWithOutput)
// Description : Flush pictures in DPB
// Parameter   : None
// Return      : None
// *********************************************************************
void vFlushDPB(UINT32 u4InstID, VDEC_INFO_DEC_PRM_T *tVerMpvDecPrm, BOOL fgWithOutput)
{
  UINT32 u4MinPOCFBufIdx;

  do
  {
        u4MinPOCFBufIdx = ucVDecGetMinPOCFBuf(u4InstID, tVerMpvDecPrm, FALSE);
        if(u4MinPOCFBufIdx != 0xFF)
        {
            if(fgWithOutput
                && _ptFBufInfo[u4InstID][u4MinPOCFBufIdx].eH264DpbStatus == H264_DPB_STATUS_DECODED)
            {
                //_ptFBufInfo[u4MinPOCFBufIdx].eH264DpbStatus = H264_DPB_STATUS_OUTPUTTED;
            }
            // Force set outputted
            _ptFBufInfo[u4InstID][u4MinPOCFBufIdx].eH264DpbStatus = H264_DPB_STATUS_OUTPUTTED;
            _ptFBufInfo[u4InstID][u4MinPOCFBufIdx].ucFBufRefType = NREF_PIC;
            _ptFBufInfo[u4InstID][u4MinPOCFBufIdx].ucTFldRefType = NREF_PIC;
            _ptFBufInfo[u4InstID][u4MinPOCFBufIdx].ucBFldRefType = NREF_PIC;
            vOutputPOCData(_ptFBufInfo[u4InstID][u4MinPOCFBufIdx].u4DecOrder);
        }
  }while(u4MinPOCFBufIdx != 0xff);

}


