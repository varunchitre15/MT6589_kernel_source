#include "vdec_hw_common.h"
#include "vdec_hw_vp6.h"
#include "vdec_hal_if_vp6.h"
#include "vdec_hal_errcode.h"
//#include "x_hal_ic.h"
//#include "x_hal_1176.h"
#include "../include/drv_common.h"
#if CONFIG_DRV_VERIFY_SUPPORT
#include "../verify/vdec_verify_general.h"
#include "../verify/vdec_verify_mpv_prov.h"
#include <linux/string.h>

#if (!CONFIG_DRV_LINUX)
#include <stdio.h>
#include <string.h>
#endif

#if VMMU_SUPPORT
#include "vdec_hal_if_common.h"
#endif


extern void vVDecOutputDebugString(const CHAR * format, ...);
extern BOOL fgWrMsg2PC(void* pvAddr, UINT32 u4Size, UINT32 u4Mode, VDEC_INFO_VERIFY_FILE_INFO_T *pFILE_INFO);
#endif


//MULTI-STREAM PANDA
static const UINT8 vp6_huff_coeff_map[] = {
    13, 14, 11, 0, 1, 15, 16, 18, 2, 17, 3, 4, 19, 20, 5, 6, 21, 22, 7, 8, 9, 10
};

static const UINT8 vp6_huff_run_map[] = {
    10, 13, 11, 12, 0, 1, 2, 3, 14, 8, 15, 16, 4, 5, 6, 7
};

static const HUFF_CODE eob_huff_code[] = {
    {0x0, 0x2}, {0x400, 0x2}, {0x800, 0x4}, {0x900, 0x4}, {0xa00, 0x4}, {0xb00, 0x4},
    {0xc00, 0x5}, {0xc80, 0x5}, {0xd00, 0x5}, {0xd80, 0x5}, {0xe00, 0x3}, {0x0, 0x0}};



    static void vBoolTreeToHuffCodes(UINT8 *coeff_model, const UINT8 *map, UINT32 size, UINT32 *huff_probs)
    {
        int a, b, i;
        UINT32 *tmp;

        if (!coeff_model || !map || !huff_probs)
        {
            return;
        }

        tmp = &huff_probs[size];

        /* first compute probabilities from model */
        tmp[0] = 256;
        for (i = 0; i < size - 1; i++) 
        {
            a = tmp[i] *        coeff_model[i]  >> 8;
            b = tmp[i] * (255 - coeff_model[i]) >> 8;
            huff_probs[map[2*i  ]] = a + !a;
            huff_probs[map[2*i+1]] = b + !b;
        }
    }

    static void vInsertSorted (SORTNODE *sn, INT32 node, INT32 *startnode)
    {
        INT32 which;
        INT32 prior;

        if (!sn || !startnode)
        {
            return;
        }

        which = *startnode;
        prior = *startnode;

        // find the position at which to insert the node
        while ((which != -1) && (sn[node].freq > sn[which].freq))
        {
            prior = which;
            which = sn[which].next;
        }

        if(which == *startnode)
        {
            *startnode = node;
            sn[node].next = which;
        }
        else
        {
            sn[prior].next = node;
            sn[node].next = which;
        }
    }

    UINT16 get_huff_code(HUFF_NODE *huffNode, UINT16 *pcode, UINT16 *len)
    {
        UINT16 *hn;
        UINT16 len_cnt = 0;
        UINT16 torp = 0;
        UINT16 hcode = 0;
        UINT16 bit;

        if (!huffNode || !pcode || !len)
        {
            return 0;
        }

        hn = (UINT16 *)huffNode;

        do
        {
            bit = ((*pcode) & (1 << (*len - len_cnt - 1))) ? 1 : 0;
            hcode = (hcode << 1) | bit;
            torp =  torp + bit;
            torp = hn[torp];
            len_cnt ++;
        }
        while (!(torp & 1));

        *len    = len_cnt;
        *pcode  = hcode;

        return torp;
    }

    static INT32 _pow(INT32 i4Base, INT32 i4Exp)
    {
        INT32 val, i;

        if(i4Exp == 0)
        {
            return 1;
        }

        val= i4Base;
        for(i=1; i<i4Exp; i++)
        {
            val *= i4Base;
        }

        return val;
    }

    static void vp6_build_huff_tree(HUFF_NODE *hn, UINT32 *counts, INT32 values, HUFF_CODE *pcode)
    {
        INT32 i;
        SORTNODE sn[256];
        INT32 sncount   = 0;
        INT32 startnode = 0;

        // NOTE:
        // Create huffman tree in reverse order so that the root will always be 0
        INT32 huffptr = values - 1;

        if (!hn || !counts || !pcode)
        {
            return;
        }

        // Set up sorted linked list of values/pointers into the huffman tree
        for (i = 0; i < values; i++)
        {
            sn[i].value = (i << 1) | 1;
            if (counts[i] == 0)
            {
                counts[i] = 1;
            }
            sn[i].freq = counts[i];
            sn[i].next = -1;
        }

        sncount = values;

        // Connect above list into a linked list
        for (i = 1; i < values; i++)
        {
            vInsertSorted (sn, i, &startnode);
        }

        // while there is more than one node in our linked list
        while (sn[startnode].next != -1)
        {
            int first   = startnode;
            int second  = sn[startnode].next;
            int sumfreq = sn[first].freq + sn[second].freq;

            // set-up new merged huffman node
            --huffptr;
            // setup new merged huffman node
            hn[huffptr].left    = sn[first].value;
            hn[huffptr].right   = sn[second].value;

            // set up new merged sort node pointing to our huffnode
            sn[sncount].value   = (huffptr << 1) | 0;
            sn[sncount].freq    = sumfreq;
            sn[sncount].next    = -1;


            // remove the two nodes we just merged from the linked list
            startnode = sn[second].next;

            // insert the new sort node into the proper location
            vInsertSorted(sn, sncount, &startnode);

            // account for new nodes
            sncount++;
        }

        { 
            unsigned short max_len = values;
            unsigned short len = values;
            int stop = 0;
            unsigned short hcode = 0;
            unsigned short torp;

            do
            {            
                torp = get_huff_code(hn, &hcode, &len);
                if(torp & 1)
                {
                    pcode[(torp>>1)].hcode  = hcode << (12 - len);
                    pcode[(torp>>1)].len    = len;
                    if(hcode == (_pow(2,len) - 1))
                    {
                        stop = 1;
                    }
                    hcode ++;
                }
                else
                {
                    hcode = hcode << (max_len - len);
                    len = max_len;
                }
            }
            while(stop != 1);
        }

        return;
    }

    void vVDEC_HAL_VP6_InitCtx(UINT32 u4VDecID, UINT32 u4BSID, VDEC_INFO_VP6_FRM_HDR_T *prFrmHdr)
    {
        UINT32 i, j, u4Cnt, u4Addr, u4Addr2, u4Tmp;
        UINT32 coeff_model[4];
        printk("//<vdec> entry %s\n", __FUNCTION__);

        if (!prFrmHdr)
        {
            return;
        }

        if (prFrmHdr->fgUseHuffman)
        {
            UINT32 huff_prob[23];
            HUFF_NODE huff_node[11];
            HUFF_CODE huff_code[12];
            UINT32 code0, code1;

            // Trigger Prob Update
            u4Cnt = 0;
            vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_PROB_UPD, (u4VDecReadVP6VLD(u4VDecID, RW_VP6_VLD_PROB_UPD) | 0x1), u4BSID);
            while(u4Cnt++ < WAIT_THRD)
            {
                if ((u4VDecReadVP6VLD(u4VDecID, RW_VP6_VLD_PROB_UPD) & 0x1) == 0x1)
                {
                    break;
                }
            }
            ASSERT(u4Cnt < WAIT_THRD);

            // AC Coeff
            printk("init AC Huff Tree\n");
            u4Addr2 = VP6_CTX_WARR_HUFFMAN | VP6_CTX_WARR_WRITE | (CTX_HUFF_AC_BASE);
            for (j = 0; j < 36; j++)
            {
                if ((j % 6 == 4) || (j % 6 == 5))
                {
                    continue;
                }

                for (i = 0; i < 4; i++)
                {
                    u4Addr = MC_VLD_WRAPPER_READ | ((VLD_WRAPPER_AC_BASE + j) << 2) | (3 - i);
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, u4Addr);
                    u4Tmp = u4VDecReadMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA);
                    coeff_model[i] =    ((u4Tmp & 0x000000ff) << 24) | 
                        ((u4Tmp & 0x0000ff00) << 8)  | 
                        ((u4Tmp & 0x00ff0000) >> 8)  | 
                        ((u4Tmp & 0xff000000) >> 24);
                }
                memset(huff_node, 0, sizeof(huff_node));
                memset(huff_code, 0, sizeof(huff_code));
                memset(huff_prob, 0, sizeof(huff_prob));
                vBoolTreeToHuffCodes((UINT8*)coeff_model, vp6_huff_coeff_map, 12, huff_prob);
                vp6_build_huff_tree(huff_node, huff_prob, 12, huff_code);

                vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Addr2, u4BSID);
                for (i = 0; i < 6; i++)
                {
                    code0 = (huff_code[2*i].len << 12)   | huff_code[2*i].hcode;
                    code1 = (huff_code[2*i+1].len << 12) | huff_code[2*i+1].hcode;
                    vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1 + 4 * i, ((code0<<16)|code1), u4BSID);
                }
                u4Addr2++;
            }

            // DC Coeff & Run Coeff
            printk("init DC Huff Tree and Zero Run Huff Tree\n");
            for (j = 0; j < 4; j++)
            {
                for (i = 0; i < 4; i++)
                {
                    u4Addr = MC_VLD_WRAPPER_READ | ((VLD_WRAPPER_DC_Y + j) << 2) | (3 - i);
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, u4Addr);
                    u4Tmp = u4VDecReadMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA);
                    coeff_model[i] =    ((u4Tmp & 0x000000ff) << 24) | 
                        ((u4Tmp & 0x0000ff00) << 8)  | 
                        ((u4Tmp & 0x00ff0000) >> 8)  | 
                        ((u4Tmp & 0xff000000) >> 24);
                }
                memset(huff_node, 0, sizeof(huff_node));
                memset(huff_code, 0, sizeof(huff_code));
                memset(huff_prob, 0, sizeof(huff_prob));
                if (j < 2)
                {
                    vBoolTreeToHuffCodes((UINT8*)coeff_model, vp6_huff_coeff_map, 12, huff_prob);
                    vp6_build_huff_tree(huff_node, huff_prob, 12, huff_code);
                }
                else
                {
                    vBoolTreeToHuffCodes((UINT8*)coeff_model, vp6_huff_run_map, 9, huff_prob);
                    vp6_build_huff_tree(huff_node, huff_prob, 9, huff_code);
                }

                u4Addr = VP6_CTX_WARR_HUFFMAN | VP6_CTX_WARR_WRITE | (CTX_HUFF_DC_Y + j);
                vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Addr, u4BSID);
                for (i = 0; i < 6; i++)
                {
                    code0 = (huff_code[2*i].len << 12)   | huff_code[2*i].hcode;
                    code1 = (huff_code[2*i+1].len << 12) | huff_code[2*i+1].hcode;
                    vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1 + 4 * i, ((code0<<16)|code1), u4BSID);
                }
            }

            // EOB Run
            printk("init EOB Run Huff Tree\n");
            u4Addr = VP6_CTX_WARR_HUFFMAN | VP6_CTX_WARR_WRITE | (CTX_HUFF_EOB_RUN);
            vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Addr, u4BSID);
            for (i = 0; i < 6; i++)
            {
                code0 = (eob_huff_code[2*i].len << 12)   | eob_huff_code[2*i].hcode;
                code1 = (eob_huff_code[2*i+1].len << 12) | eob_huff_code[2*i+1].hcode;
                vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1 + 4 * i, ((code0<<16)|code1), u4BSID);
            }
        }
        else
        {
            for (i = 0; i < 40; i++)
            {
                for (j = 0; j < 4; j++)
                {
                    u4Addr = MC_VLD_WRAPPER_READ | ((VLD_WRAPPER_DC_Y + i) << 2) | (3 - j);
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, u4Addr);
                    coeff_model[j] = u4VDecReadMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA);
                }

                u4Addr = VP6_CTX_WARR_BOOL | VP6_CTX_WARR_WRITE | (CTX_BOOL_DC_Y + i);
                vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Addr, u4BSID);
                for (j = 0; j < 4; j++)
                {
                    vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1 + 4 * j, coeff_model[j], u4BSID);
                }
            }
        }
    }
    //~MULTI-STREAM PANDA

    // **************************************************************************
    // Function : INT32 i4VDEC_HAL_VP6_InitVDecHW(UINT32 u4Handle, VDEC_INFO_VP6_VFIFO_PRM_T *prVp6VFifoInitPrm);
    // Description :Initialize video decoder hardware only for VP6
    // Parameter : u4VDecID : video decoder hardware ID
    //                  prVp6VFifoInitPrm : pointer to VFIFO info struct
    // Return      : =0: success.
    //                  <0: fail.
    // **************************************************************************
    INT32 i4VDEC_HAL_VP6_InitVDecHW(UINT32 u4VDecID, VDEC_INFO_VP6_VFIFO_PRM_T *prVp6VFifoInitPrm)
    {
        printk("<vdec> entry %s\n", __FUNCTION__);

#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8555)
        vVDecResetHW(u4VDecID);
#else
        vVDecResetHW(u4VDecID, VDEC_UNKNOWN);
#endif

        vVDecSetVLDVFIFO(0, u4VDecID, PHYSICAL((UINT32) prVp6VFifoInitPrm->u4VFifoSa), PHYSICAL((UINT32) prVp6VFifoInitPrm->u4VFifoEa));
        return HAL_HANDLE_OK;
    }

    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
    // Description :Read barrel shifter after shifting
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4ShiftBits : shift bits number
    // Return      : Value of barrel shifter input window after shifting
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_ShiftGetBitStream(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
    {
        UINT32 dRegVal;

        dRegVal = u4VDecVP6VLDGetBits(u4BSID, u4VDecID, u4ShiftBits);

        return(dRegVal);
    }

    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
    // Description :Read Barrel Shifter before shifting
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4ShiftBits : shift bits number
    // Return      : Value of barrel shifter input window before shifting
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_GetBitStreamShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
    {
        UINT32 u4RegVal0;

#ifdef VDEC_SIM_DUMP
        printk("<vdec> VP6_VLD_SHIFT_BIT(%d);\n", u4ShiftBits);
#endif

        u4RegVal0 = u4VDecVP6VLDGetBits(u4BSID, u4VDecID, 0);
        u4VDecVP6VLDGetBits(u4BSID, u4VDecID, u4ShiftBits);

        return(u4RegVal0);
    }

    // **************************************************************************
    // Function : INT32 i4VDEC_HAL_VP6_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_VP6_BS_INIT_PRM_T *prVp6BSInitPrm, BOOL fgIsVC1);
    // Description :Initialize barrel shifter with byte alignment
    // Parameter : u4ReadPointer : set read pointer value
    //                 u4WrtePointer : set write pointer value
    //                 u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    // Return      : =0: success.
    //                  <0: fail.
    // **************************************************************************
    INT32 i4VDEC_HAL_VP6_InitBarrelShifter(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_VP6_BS_INIT_PRM_T *prVp6BSInitPrm)
    {
        BOOL fgFetchOK = FALSE;
        INT32 i,j;
        //UINT32 u4VLDByte, u4VLDBit;
        UINT32 u4VLDRemainByte;

        printk("<vdec> entry %s\n", __FUNCTION__);

#if (CONFIG_DRV_VERIFY_SUPPORT) && (CONFIG_DRV_LINUX)    
        //HalFlushInvalidateDCache();
#endif

        //#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580) 
#if 0
        vVDecWriteVLD(u4VDecID,RW_VLD_RDY_SWTICH + (u4BSID << 10), READY_TO_RISC_1);

        //polling
        //if((u4VDecReadVLD(u4VDecID,RO_VLD_SRAMCTRL) & (0x1<<15)))
        printk("if(`VDEC_PROCESS_FLAG == 1) wait(`VDEC_BITS_PROC_NOP == 1);\n");
        if((u4VDecReadVLD(u4VDecID,RO_VLD_SRAMCTRL) & (PROCESS_FLAG)))
            while (!(u4VDecReadVLD(u4VDecID,RO_VLD_SRAMCTRL) & 1)); 

        vVDecSetVLDVFIFO(u4BSID, u4VDecID, PHYSICAL((UINT32) prVp6BSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prVp6BSInitPrm->u4VFifoEa));

        u4VLDRemainByte =  ((prVp6BSInitPrm->u4ReadPointer)) & 0xf;     

        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR, u4VDecReadVLD(u4VDecID, WO_VLD_WPTR) | VLD_CLEAR_PROCESS_EN);
        vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL((UINT32) prVp6BSInitPrm->u4ReadPointer));
        vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10),PHYSICAL((UINT32) prVp6BSInitPrm->u4WritePointer));
        //     vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC + (u4BSID << 10), u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);//mark

        //Reset async fifo for mt8580 new design
        vVDecWriteVLD(u4VDecID, WO_VLD_SRST , (0x1 << 8));
        vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 0);


        // start to fetch data
        vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);

        if (fgVDecWaitVldFetchOk(u4BSID, u4VDecID))
        {
            fgFetchOK = TRUE;
            //break;
        }

#else
        vVDecWriteVLD(u4VDecID,RW_VLD_RDY_SWTICH + (u4BSID << 10), READY_TO_RISC_1);

        vVDecSetVLDVFIFO(u4BSID, u4VDecID, PHYSICAL((UINT32) prVp6BSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prVp6BSInitPrm->u4VFifoEa));

        u4VLDRemainByte =  ((prVp6BSInitPrm->u4ReadPointer)) & 0xf;

        // prevent initialize barrel fail
        for (i = 0; i < 5; i++)
        {
#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8580)
            UINT32 u4Cnt;
            u4Cnt = 50000;
            if (u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL) & (1<<15))
            {
                while((!(u4VDecReadVLD(u4VDecID, RO_VLD_SRAMCTRL)&0x1)) && (u4Cnt--));
            }
#endif
            vVDecWriteVLD(u4VDecID, WO_VLD_WPTR, u4VDecReadVLD(u4VDecID, WO_VLD_WPTR) | VLD_CLEAR_PROCESS_EN);
            vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL((UINT32) prVp6BSInitPrm->u4ReadPointer));
            vVDecWriteVLD(u4VDecID, RW_VLD_RPTR + (u4BSID << 10), PHYSICAL((UINT32) prVp6BSInitPrm->u4ReadPointer));
            vVDecWriteVLD(u4VDecID, WO_VLD_WPTR + (u4BSID << 10),PHYSICAL((UINT32) prVp6BSInitPrm->u4WritePointer));
            //vVDecWriteVLD(u4VDecID, RW_VLD_ASYNC + (u4BSID << 10), u4VDecReadVLD(u4VDecID, RW_VLD_ASYNC) | VLD_WR_ENABLE);    //PANDA

            //Reset async fifo for mt8580 new design
            vVDecWriteVLD(u4VDecID, WO_VLD_SRST , (0x1 << 8));
            vVDecWriteVLD(u4VDecID, WO_VLD_SRST , 0);

            // start to fetch data
            vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIFET);

            if (fgVDecWaitVldFetchOk(u4BSID, u4VDecID))
            {
                fgFetchOK = TRUE;
                break;
            }
        }
#endif
        if (!fgFetchOK)
        {
            return(INIT_BARRELSHIFTER_FAIL);
        }

        vVDecWriteVLD(u4VDecID, RW_VLD_PROC + (u4BSID << 10), VLD_INIBR);           

        vVDecWriteVP6VLD(u4VDecID, RW_VP6_PCI_PAR + (u4BSID << 10), RW_VP6_FLAG, u4BSID);

        for (j=0;j<u4VLDRemainByte;j++)
        {
            u4VDecVP6VLDGetBits(u4BSID, u4VDecID, 8);
        }

        //u4VLDByte = u4VDecReadVldRPtr(u4BSID, u4VDecID, &u4VLDBit, PHYSICAL((UINT32) prVp6BSInitPrm->u4VFifoSa));
        return HAL_HANDLE_OK;
    }

    // **************************************************************************
    // Function : INT32 i4VDEC_HAL_VP6_InitBarrelShifter2
    // Description :Initialize barrel shifter 2 with byte alignment
    // Parameter : u4ReadPointer : set read pointer value
    //                 u4WrtePointer : set write pointer value
    //                 u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    // Return      : =0: success.
    //                  <0: fail.
    // **************************************************************************
    INT32 i4VDEC_HAL_VP6_InitBarrelShifter2(UINT32 u4BSID, UINT32 u4VDecID, VDEC_INFO_VP6_BS_INIT_PRM_T *prVp6BSInitPrm, VDEC_INFO_VP6_FRM_HDR_T *prVDecVp6FrmHdr)
    {
        //BOOL fgFetchOK = FALSE;
        INT32 i;
        UINT32 u4VLDRemainByte;
        UINT32 u4Cnt = 0;
        printk("<vdec> entry %s\n", __FUNCTION__);
#if (CONFIG_DRV_VERIFY_SUPPORT) && (CONFIG_DRV_LINUX)    
        //HalFlushInvalidateDCache();
#endif

        //vVDecVP6WriteVLD2(RW_VLD_RDY_SWTICH, READY_TO_RISC_1);

        vVDecVP6SetVLD2VFIFO(PHYSICAL((UINT32) prVp6BSInitPrm->u4VFifoSa), PHYSICAL((UINT32) prVp6BSInitPrm->u4VFifoEa));

        if(u4VDecVP6ReadVLD2(RO_VLD_SRAMCTRL) & PROCESS_FLAG)
        {
            while((u4VDecVP6ReadVLD2(RO_VLD_SRAMCTRL) & AA_FIT_TARGET_SCLK) == 0)
            {
                u4Cnt++;
                if(u4Cnt >= WAIT_THRD)
                {
                    printk("<vdec> [VP6] BARRELSHIFTER 2 INITIL FAIL\n");
                    break;
                }
            }
        }
        u4VLDRemainByte =  ((prVp6BSInitPrm->u4ReadPointer)) & 0xf;

        vVDecVP6WriteVLD2(RW_VLD_RPTR, PHYSICAL((UINT32) prVp6BSInitPrm->u4ReadPointer));
        vVDecVP6SetVLD2Wptr(prVp6BSInitPrm->u4WritePointer);
        vVDecVP6WriteVLD2(WO_VLD_SRST, 0x100);
        vVDecVP6WriteVLD2(WO_VLD_SRST, 0x0);

        // start to fetch data
        vVDecVP6WriteVLD2(RW_VLD_PROC, VLD_INIFET);

        if (!fgVDecWaitVld2FetchOk(u4VDecID))
        {
            printk("<vdec> [VP6] BARRELSHIFTER 2 FETCH FAIL\n");
            return FALSE;
        }

        vVDecVP6WriteVLD2(RW_VLD_PROC, VLD_INIBR);
        if (prVDecVp6FrmHdr)
        {
            UINT32 u4KeyFrame, u4Huffman, u4Multi;
            u4KeyFrame = (prVDecVp6FrmHdr->ucFrameType == VP6_I_FRM)? RW_VP6_KEY_FRM : 0;
            u4Huffman = (prVDecVp6FrmHdr->fgUseHuffman == TRUE) ? RW_VP6_HUFFMAN : 0;
            u4Multi = (prVDecVp6FrmHdr->fgMultiStream == TRUE) ? RW_VP6_MULTI : 0;

            vVDecWriteVP6VLD(u4VDecID, RW_VP6_PCI_PAR , (u4KeyFrame | u4Huffman | u4Multi | RW_VP6_FLAG), u4BSID);
        }
        else
        {
            vVDecWriteVP6VLD(u4VDecID, RW_VP6_PCI_PAR, RW_VP6_FLAG, u4BSID);
        }

        for (i=0;i<u4VLDRemainByte;i++)
        {
            u4VDecVP6VLD2GetBits(8);
        }

        return HAL_HANDLE_OK;
    }

    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits);
    // Description :Read current read pointer
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4VFIFOSa : video FIFO start address
    //                 pu4Bits : read pointer value with remained bits
    // Return      : Read pointer value with byte alignment
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_ReadRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits)
    {
        return u4VDecReadVldRPtr(u4BSID, u4VDecID, pu4Bits, PHYSICAL((UINT32) u4VFIFOSa));
    }

    // **************************************************************************
    // Function : void v4VDEC_HAL_VP6_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 u4AlignType);
    // Description :Align read pointer to byte,word or double word
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4VFIFOSa : video FIFO start address
    //                 u4AlignType : read pointer align type
    // Return      : None
    // **************************************************************************
    void vVDEC_HAL_VP6_AlignRdPtr(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 u4AlignType)
    {
        INT32 i;
        UINT32 u4VLDByte,u4VLDbits;

        u4VLDByte= u4VDecReadVldRPtr(u4BSID, u4VDecID, &u4VLDbits, PHYSICAL(u4VFIFOSa));
        if (u4VLDbits != 0)
        {
            u4VDecVLDGetBitS(u4BSID, u4VDecID, 8 - u4VLDbits);
            u4VLDByte++;
        }
        if (u4AlignType == WORD_ALIGN)
        {
            for (i=0;i<(u4VLDByte & 1);i++)
            {
                u4VDecVLDGetBitS(u4BSID, u4VDecID, 8);
            }
        }
        else if (u4AlignType == DWRD_ALIGN)
        {
            if ((u4VLDByte & 3) != 0)
            {
                for (i=0;i<(4- (u4VLDByte & 3));i++)
                {
                    u4VDecVLDGetBitS(u4BSID, u4VDecID, 8);
                }
            }
        }
        return;
    }


    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID);
    // Description :Read barrel shifter bitcount after initializing 
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    // Return      : Current bit count
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_GetBitcount(UINT32 u4BSID, UINT32 u4VDecID)
    {
        return HAL_HANDLE_OK;
    }

    // **************************************************************************
    // Function : void v4VDEC_HAL_VP6_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby);
    // Description :Read current decoded mbx and mby
    // Parameter : u4VDecID : video decoder hardware ID
    //                 pu4Mbx : macroblock x value
    //                 pu4Mby : macroblock y value
    // Return      : None
    // **************************************************************************
    void vVDEC_HAL_VP6_GetMbxMby(UINT32 u4VDecID, UINT32 *pu4Mbx, UINT32 *pu4Mby)
    {
        *pu4Mbx = u4VDecReadMC(u4VDecID, RO_MC_MBX);
        *pu4Mby = u4VDecReadMC(u4VDecID, RO_MC_MBY);
    }

    // **************************************************************************
    // Function : void v4VDEC_HAL_VP6_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_VP6_ERR_INFO_T *prVp6ErrInfo);
    // Description :Read error count after decoding end
    // Parameter : u4VDecID : video decoder hardware ID
    //                 prVp6ErrInfo : pointer to Vp6 error info struct
    // Return      : None
    // **************************************************************************
    void vVDEC_HAL_VP6_GetErrInfo(UINT32 u4VDecID, VDEC_INFO_VP6_ERR_INFO_T *prVp6ErrInfo)
    {
        UINT32 u4RegVal = 0;

        u4RegVal = u4VDecReadVLD(u4VDecID, RO_VP6_VLD_ERR);
        prVp6ErrInfo->u4Vp6ErrCnt = 0;
        prVp6ErrInfo->u4Vp6ErrRow = 0;
        prVp6ErrInfo->u4Vp6ErrType = (u4RegVal & 0xF); 
        prVp6ErrInfo->u2Vp6MBErrCnt =0 ;

        return;
    }

    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_GetErrType(UINT32 u4VDecID);
    // Description :Read Vp6 error type after decoding end
    // Parameter : u4VDecID : video decoder hardware ID
    // Return      : Vp6 decode error type value
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_GetErrType(UINT32 u4VDecID)
    {
        UINT32 u4RegVal = 0;
        return u4RegVal;
    }

    //extern BOOL _VDecNeedDumpRegister(UINT32 u4VDecID);

    // **************************************************************************
    // Function : INT32 i4VDEC_HAL_VP6_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
    // Description :Set video decoder hardware registers to decode for Vp6
    // Parameter : prDecVp6Prm : pointer to Vp6 decode info struct
    // Return      : =0: success.
    //                  <0: fail.
    // **************************************************************************
    INT32 i4VDEC_HAL_VP6_DecStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
    {    
#if (CONFIG_DRV_VERIFY_SUPPORT) && (!VDEC_DRV_PARSER)
        VDEC_INFO_VP6_DEC_PRM_T *prVDecVP6DecPrm = (VDEC_INFO_VP6_DEC_PRM_T *) &(prDecPrm->SpecDecPrm.rVDecVP6DecPrm);
#else
        VDEC_INFO_VP6_DEC_PRM_T *prVDecVP6DecPrm = (VDEC_INFO_VP6_DEC_PRM_T *)prDecPrm->prVDecCodecHalPrm;
#endif

#if VDEC_DDR3_SUPPORT
        UINT32 u4DDR3_PicWdith;
        UINT32 aMc406;
#endif          

        UINT32 u4BSID = 0;
        UINT32 u4Vp68C, u4VertMb, u4HoriMb; //, ucKeyFrame;
        UINT32 u4Mc85c = 0;
        UINT32 u4Mc860, u4Mc864, u4Mc868;
        UINT32 u4TurnOffDeblock;
        UINT32 u4Pp200;
        UINT32 u4MBqp = 0;
        UINT32 u4KeyFrame, u4Huffman, u4Multi;
#if (CONFIG_DRV_VERIFY_SUPPORT)
        UINT32 u4TargFrm = 10000;
#endif

        //#if VDEC_VER_COMPARE_CRC
#if 0
        UINT32 u4CRCSrc = 0x1; 
        u4CRCSrc = (VDEC_CRC_EN | VDEC_CRC_SRC_MC);   //CRC input from MC
        //u4CRCSrc = (VDEC_CRC_EN | VDEC_CRC_SRC_PP);  //CRC input from PP
        vVDecWriteCRC(u4VDecID, 0x4, u4CRCSrc);
#endif
        printk("<vdec> entry %s\n", __FUNCTION__);
        // set video down scaler parameter
        vVDECSetDownScalerPrm(u4VDecID, &prDecPrm->rDownScalerPrm);

#if ((CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8560) && CONFIG_DRV_FTS_SUPPORT)
        // set letterbox detection parameter
        //vVDECSetLetetrBoxDetPrm(u4VDecID, &prDecPrm->rLBDPrm);
#endif    

        //set picture parameter
        u4VertMb = prVDecVP6DecPrm->prFrmHdr->u2VFragments;
        u4HoriMb = prVDecVP6DecPrm->prFrmHdr->u2HFragments;
        //ucKeyFrame = (prVDecVP6DecPrm->prFrmHdr->ucFrameType ==VP6_I_FRM)? 1: 0;
        //u4Vp68C = ((ucKeyFrame << 1) | (RW_VP6_FLAG));          
        u4KeyFrame = (prVDecVP6DecPrm->prFrmHdr->ucFrameType == VP6_I_FRM)? RW_VP6_KEY_FRM : 0;
        u4Huffman = (prVDecVP6DecPrm->prFrmHdr->fgUseHuffman == TRUE) ? RW_VP6_HUFFMAN : 0;
        u4Multi = (prVDecVP6DecPrm->prFrmHdr->fgMultiStream == TRUE) ? RW_VP6_MULTI : 0;
        u4Vp68C     = (u4KeyFrame | u4Huffman | u4Multi | RW_VP6_FLAG /*| RW_VP6_RTN_ERR*/);

        vVDecWriteVP6VLD(u4VDecID, RW_VP6_PCI_PAR + (u4BSID << 10), u4Vp68C, u4BSID);

        vVDecWriteVLDTOP(u4VDecID, RW_PIC_MB_SIZE_M1, (((u4HoriMb-1)&0x3FF) | (((u4VertMb-1)&0x3FF) << 16)));
        vVDecWriteVLDTOP(u4VDecID, RW_PIC_PIX_SIZE, (((u4HoriMb << 4)&0x3FFF) | (((u4VertMb << 4)&0x3FFF) << 16)));


        //Write IQ_QUANT
        u4Pp200 = ((prVDecVP6DecPrm->prFrmHdr->u4DQuant_Ac << 8) | prVDecVP6DecPrm->prFrmHdr->u4DQuant_Dc);
        vVDecWriteVP6PP(u4VDecID, RW_VP6_QUANT_REG, u4Pp200, u4BSID);

        u4TurnOffDeblock = (prVDecVP6DecPrm->prFrmHdr->u2LoopFilter == 0)? 1 : 0;
        u4Mc860 = ((u4TurnOffDeblock << 8)   | (prVDecVP6DecPrm->prFrmHdr->u2Vp56_Filter_Threshold));
        vVDecWriteMC(u4VDecID, RW_VP6_LOOPDBK, u4Mc860);


#if 0
        u4Mc864 = ((prVDecVP6DecPrm->prFrmHdr->u4Mv_Thr_En << 16)   | (prVDecVP6DecPrm->prFrmHdr->u4Max_Vector_Length));
        vVDecWriteMC(u4VDecID, RW_VP6_MV_TH, u4Mc864);

        u4Mc868 = ((prVDecVP6DecPrm->prFrmHdr->u4BilinearFilter << 28)   | (prVDecVP6DecPrm->prFrmHdr->u4Var_Thr_En << 20) 
            |(prVDecVP6DecPrm->prFrmHdr->u4Sample_Variance_Threshold) );
        vVDecWriteMC(u4VDecID, RW_VP6_AUTOSEL, u4Mc868);
#endif

        if (prVDecVP6DecPrm->prFrmHdr->ucFilter_Mode != 2)
        {
            if (prVDecVP6DecPrm->prFrmHdr->ucFilter_Mode == 0)
            {
                vVDecWriteMC(u4VDecID, RW_VP6_AUTOSEL, (0x1 << 28));
            }
            else
            {
                vVDecWriteMC(u4VDecID, RW_VP6_AUTOSEL, (0x0 << 28));
            }
        }
        else
        {
            u4Mc868 = ((prVDecVP6DecPrm->prFrmHdr->u4Var_Thr_En << 20) | (prVDecVP6DecPrm->prFrmHdr->u4Sample_Variance_Threshold) );
            vVDecWriteMC(u4VDecID, RW_VP6_AUTOSEL, u4Mc868);

            u4Mc864 = ((prVDecVP6DecPrm->prFrmHdr->u4Mv_Thr_En << 16)   | (prVDecVP6DecPrm->prFrmHdr->u4Max_Vector_Length));
            vVDecWriteMC(u4VDecID, RW_VP6_MV_TH, u4Mc864);
        }


        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_ETC, ETC_COEFF_ERROR|ETC_MC_TIMEOUT|ETC_MV_TIMEOUT|ETC_DCAC_TIMEOUT|ETC_HUFF_ERR, u4BSID);


        //-------------------------------------------------
        // global setting 只要一開始設一次即可
        //-------------------------------------------------

        #if VMMU_SUPPORT
        {
        UINT32 u4Page_addr = 0;
        
        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa)/(4*1024))*4);
        printk("[VP6] i4VDEC_HAL_VP6_DecStart, 1, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa), PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa) + PIC_Y_SZ);
        vVDecWriteMC(u4VDecID, RW_VP6_MC_RY, ((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa) >> prVDecVP6DecPrm->i4MemBase >> 9); 

        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa)/(4*1024))*4);
        printk("[VP6] i4VDEC_HAL_VP6_DecStart, 2, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa), PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa) + PIC_C_SZ);
        vVDecWriteMC(u4VDecID, RW_VP6_MC_RC, ((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa) >> prVDecVP6DecPrm->i4MemBase >> 8); 

        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa)/(4*1024))*4);
        printk("[VP6] i4VDEC_HAL_VP6_DecStart, 3, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa), PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa) + PIC_Y_SZ);
        vVDecWriteMC(u4VDecID, RW_VP6_MC_GY, ((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa) >> prVDecVP6DecPrm->i4MemBase >> 9); 

        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa)/(4*1024))*4);
        printk("[VP6] i4VDEC_HAL_VP6_DecStart, 4, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa), PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa) + PIC_C_SZ);
        vVDecWriteMC(u4VDecID, RW_VP6_MC_GC, ((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa) >> prVDecVP6DecPrm->i4MemBase >> 8); 

        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa)/(4*1024))*4);
        printk("[VP6] i4VDEC_HAL_VP6_DecStart, 5, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa), PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa) + PIC_Y_SZ);
        vVDecWriteMC(u4VDecID, RW_VP6_MC_DY, ((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa) >> prVDecVP6DecPrm->i4MemBase >> 9); 

        u4Page_addr = (UINT32)_pucVMMUTable[u4VDecID] + ((((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa)/(4*1024))*4);
        printk("[VP6] i4VDEC_HAL_VP6_DecStart, 6, Page Addr = 0x%x\n", u4Page_addr);    
        vPage_Table(u4VDecID, u4Page_addr, PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa), PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa) + PIC_C_SZ);
        vVDecWriteMC(u4VDecID, RW_VP6_MC_DC, ((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa) >> prVDecVP6DecPrm->i4MemBase >> 8); 
        
        printk("[VP6] i4VDEC_HAL_VP6_DecStart, VMMUTable:[0x%x, 0x%x]\n", ((UINT32)_pucVMMUTable[u4VDecID]), PHYSICAL((UINT32)_pucVMMUTable[u4VDecID]));         
        vVDecVMMUEnable(PHYSICAL((UINT32)_pucVMMUTable[u4VDecID]));
        }
        #else
        //Previoud Frame Buffer 
        vVDecWriteMC(u4VDecID, RW_VP6_MC_RY, (PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1YSa) >> prVDecVP6DecPrm->i4MemBase) >> 9); // div 512
        vVDecWriteMC(u4VDecID, RW_VP6_MC_RC, (PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic1CSa) >> prVDecVP6DecPrm->i4MemBase) >> 8); // div 256
        //Golden Frame Buffer 
        vVDecWriteMC(u4VDecID, RW_VP6_MC_GY, (PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2YSa) >> prVDecVP6DecPrm->i4MemBase) >> 9); // div 512
        vVDecWriteMC(u4VDecID, RW_VP6_MC_GC, (PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic2CSa) >> prVDecVP6DecPrm->i4MemBase) >> 8); // div 256

        //
        vVDecWriteMC(u4VDecID, RW_VP6_MC_DY, (PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0YSa) >> prVDecVP6DecPrm->i4MemBase) >> 9); // div 512
        vVDecWriteMC(u4VDecID, RW_VP6_MC_DC, (PHYSICAL((UINT32) prVDecVP6DecPrm->rVp6FrameBufSa.u4Pic0CSa) >> prVDecVP6DecPrm->i4MemBase) >> 8); // div 256
        #endif
        
        // addr swap mode
        vVDecWriteMC(u4VDecID, RW_MC_ADDRSWAP, prDecPrm->ucAddrSwapMode);

#if (CONFIG_DRV_FPGA_BOARD)
        vVDecWriteMC(u4VDecID, RW_MC_MODE_CTL, MC_QIU_BANK4|MC_QIU_BANK8|MC_DRAM_REQ_DELAY_1T|MC_DRAM_REQ_MERGE_OFF|MC_MV_MERGE_OFF);
#endif

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
#if (CONFIG_CHIP_VER_CURR < CONFIG_CHIP_VER_MT8550)
        vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));  
#else
#if VDEC_DDR3_SUPPORT   
        vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prDecPrm->u4PicBW + 63)>> 4));  
#else
        //vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, ((prDecPrm->u4PicBW + 15)>> 4));       //MULTI-STREAM PANDA
#endif
#endif
#endif


        vVDecWriteVLD(u4VDecID, RW_VLD_MBDRAM_SEL, u4VDecReadVLD(u4VDecID, RW_VLD_MBDRAM_SEL) | (1 << 16));
        //------------------------------
        //   MC RISC WRITE PATTERN
        //------------------------------
        //----------------------------------------------------------
        // MC_reg_9 : MC output buffer setting
        //            (0:ref_pic1_buf  1:ref_pic2_buf  4:prg_B_pic1)
        //----------------------------------------------------------
        //I, P

        vVDecWriteMC(u4VDecID, RW_MC_OPBUF, 4);

        vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_WIDTH, prVDecVP6DecPrm->prFrmHdr->u2WidthDec);
        vVDecWriteMC(u4VDecID, RW_MC_UMV_PIC_HEIGHT, prVDecVP6DecPrm->prFrmHdr->u2HeightDec);


        if (prVDecVP6DecPrm->prFrmHdr->u2WidthDec> 1920)
        {
            vVDecWriteMC(u4VDecID, RW_MC_WRAPPER_SWITCH, 1);
        }
        else
        {
            vVDecWriteMC(u4VDecID, RW_MC_WRAPPER_SWITCH, 0);
        }


        if (prVDecVP6DecPrm->rVp6PpInfo.fgPpEnable)
        {                
            vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 1);
            vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, PHYSICAL(prVDecVP6DecPrm->rVp6PpInfo.u4PpYBufSa) >> 9);
            vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, PHYSICAL(prVDecVP6DecPrm->rVp6PpInfo.u4PpCBufSa) >> 8);
            //ddr3 -> 4x
            vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, (prDecPrm->u4PicW + 15) >> 4);


            u4MBqp = (prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[0] & 0x1F) | ((UINT32)(prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[1] & 0x1F) << 8) \
                | ((UINT32)(prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[2] & 0x1F) << 16) | ((UINT32)(prVDecVP6DecPrm->rVp6PpInfo.au1MBqp[3] & 0x1F) << 24);        
            vVDecWriteMC(u4VDecID, RW_VP6_MC_PP_QP_TYPE, u4MBqp);        
            vVDecWriteMC(u4VDecID, RW_MC_PP_DBLK_MODE, DBLK_Y+DBLK_C);
            vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, ((prDecPrm->u4PicW + 15) >> 4) - 1);
            vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, (((prDecPrm->u4PicH + 15) >> 4) >> (prDecPrm->ucPicStruct != FRM_PIC)) - 1);
        }
        else
        {
            vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, 0);
            //------------------------------------------
            // MC_reg_139 : picture width in MB
            //------------------------------------------
            vVDecWriteMC(u4VDecID, RW_MC_PP_MB_WIDTH, prVDecVP6DecPrm->prFrmHdr->u2HFragments-1); 
        }

        if (prVDecVP6DecPrm->fgAdobeMode)
        {
            u4Mc85c = (VP6_ADOBE_EN |VP6_2FLIMIT_ZERO | VP6_CBCR_MAKE_RND);
            vVDecWriteMC(u4VDecID, RW_VP6_BKDOOR, u4Mc85c);
        }

#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8530)
        vVDecWriteMC(0, 0x5E4, (u4VDecReadMC(0, 0x5E4) |(0x1 <<12)) );
        //vVDecWriteMC(0, 0x660, (u4VDecReadMC(0, 0x660) |(0x80000000)) );    

#ifndef VDEC_PIP_WITH_ONE_HW
        vVDecWriteMC(1, 0x5E4, (u4VDecReadMC(1, 0x5E4) |(0x1 <<12)) );
        //vVDecWriteMC(1, 0x660, (u4VDecReadMC(1, 0x660) |(0x80000000)) );    
#endif
#endif


#if (CONFIG_CHIP_VER_CURR >= CONFIG_CHIP_VER_MT8550)
        //Set NBM address swap mode
        vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL, 
            ((u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL)  & 0xFFFFFFF8) |prDecPrm->ucAddrSwapMode));

#if VDEC_MC_NBM_OFF
        //Turn off NBM address swap mode
        vVDecWriteMC(u4VDecID, RW_MC_NBM_CTRL, 
            (u4VDecReadMC(u4VDecID, RW_MC_NBM_CTRL) | (RW_MC_NBM_OFF)  ));
#endif

#if VDEC_DDR3_SUPPORT
        u4DDR3_PicWdith = (((prDecPrm->u4PicBW + 63) >> 6) << 2);
        vVDecWriteMC(u4VDecID,  RW_MC_PP_MB_WIDTH, u4DDR3_PicWdith);  
        vVDecWriteVLD(u4VDecID, RW_VLD_PIC_W_MB, u4DDR3_PicWdith);                 

        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_ADDR, PHYSICAL(prVDecVP6DecPrm->rVp6PpInfo.u4PpYBufSa) >> 9);
        vVDecWriteMC(u4VDecID, RW_MC_PP_C_ADDR, PHYSICAL(prVDecVP6DecPrm->rVp6PpInfo.u4PpCBufSa) >> 8);

        vVDecWriteMC(u4VDecID, RW_MC_PP_X_RANGE, ((prDecPrm->u4PicW + 15) >> 4) - 1);
        vVDecWriteMC(u4VDecID, RW_MC_PP_Y_RANGE, (((prDecPrm->u4PicH + 15) >> 4) >> (prDecPrm->ucPicStruct != FRM_PIC)) - 1);


        //Turn on DDR3 mode
        vVDecWriteMC(u4VDecID, RW_MC_DDR_CTRL0, 
            ((u4VDecReadMC(u4VDecID, RW_MC_DDR_CTRL0)  & 0xFFFFFFFE) |0x1));

        vVDecWriteMC(u4VDecID, RW_MC_DDR_CTRL1, 
            ((u4VDecReadMC(u4VDecID, RW_MC_DDR_CTRL1)  & 0xFFFFFFFE) |0x1));

        aMc406 = u4VDecReadMC(u4VDecID, (406<<2));
        aMc406 &= 0xFFFFFFEF;
        vVDecWriteMC(u4VDecID, (406<<2), aMc406);

        //Turn-on DDR3, Set 0x834[0] = 0
        vVDecWriteMC(u4VDecID, RW_MC_DDR3_EN, (u4VDecReadMC(u4VDecID, RW_MC_DDR3_EN)  & 0xFFFFFFFE));

        //DDR3 Output Selector:
        //a. MC Only:  RW_MC_PP_ENABLE = 0 && RW_MC_PP_WB_BY_POST = 0
        //b. MC+PP:    RW_MC_PP_ENABLE = 1 && RW_MC_PP_WB_BY_POST = 0
        //c. PP Only:   RW_MC_PP_ENABLE = 1 && RW_MC_PP_WB_BY_POST = 1

        //Post-process enable
        vVDecWriteMC(u4VDecID, RW_MC_PP_ENABLE, (u4VDecReadMC(u4VDecID, RW_MC_PP_ENABLE)  | 0x1));

        //Writeback by PP
        //if RW_MC_PP_WB_BY_POST =1, then Only output to PP Buffer.
        //if RW_MC_PP_WB_BY_POST = 0,then ouput to both PP & MC
        //vVDecWriteMC(u4VDecID, RW_MC_PP_WB_BY_POST, 0x00000001);
#endif

#endif

        //MULTI-STREAM PANDA
        //if(prVDecVP6DecPrm->prFrmHdr->u2Buff2Offset /*or alpha enabled*/)
        if(prVDecVP6DecPrm->prFrmHdr->u2Buff2Offset || (prVDecVP6DecPrm->u1AlphaFlag & VP6_ALPHA_ENABLE))
        {
            vVDEC_HAL_VP6_InitCtx(u4VDecID, u4BSID, prVDecVP6DecPrm->prFrmHdr);
        }

        vVDecWriteMC(u4VDecID, RW_MC_PIC_W_MB, ((prDecPrm->u4PicW + 15) >> 4));

#if (CONFIG_DRV_VERIFY_SUPPORT) && (CONFIG_DRV_LINUX)    
        // HalFlushInvalidateDCache();
#endif



#if (CONFIG_DRV_VERIFY_SUPPORT)
        //For Debug Only

        //u4VDEC_HAL_VP6_Read_QMatrix(u4BSID, u4VDecID);
        if (_u4FileCnt[u4VDecID] == u4TargFrm)
        {
            //u4VDEC_HAL_VP6_Write_SRAMData1(u4BSID, u4VDecID);
            u4VDEC_HAL_VP6_Read_SRAMData1(u4BSID, u4VDecID);
        }
#endif

        {
            printk("<vdec> Input window is 0x%x (%s, %d)\n", u4VDecReadVP6VLD(u4VDecID, 0xF0), __FUNCTION__, __LINE__);
            //u4VDecReadVldRPtr(u4BSID, u4VDecID, &u4VldBit, PHYSICAL((UINT32)_pucVFifo[u4VDecID]));
        }

        vVDecWriteVP6VLD(u4VDecID, RW_VLD_VP6DEC, VP6_DEC_START, u4BSID);
        vVDecWriteVLD(u4VDecID, RW_VLD_VP6DEC, 0x00000000);

        return HAL_HANDLE_OK;
    }


    // **************************************************************************
    // Function : INT32 i4VDEC_HAL_VP6_VPStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm);
    // Description :Set video decoder hardware registers to vp for Vp6
    // Parameter : prDecVp6Prm : pointer to Vp6 decode info struct
    // Return      : =0: success.
    //                  <0: fail.
    // **************************************************************************
    INT32 i4VDEC_HAL_VP6_VPStart(UINT32 u4VDecID, VDEC_INFO_DEC_PRM_T *prDecPrm)
    {


        return HAL_HANDLE_OK;
    }


    // **************************************************************************
    // Function : INT32 i4VDEC_HAL_VP6_VPStart(UINT32 u4VDecID);
    // Description :Set video decoder hardware registers to vp for Vp6
    // Parameter : prDecVp6Prm : pointer to Vp6 decode info struct
    // Return      : =0: success.
    //                  <0: fail.
    // **************************************************************************
    /// Read DDR3 Decode Finish Flag
    /// \return 
    INT32 i4VDEC_HAL_VP6_DDR3_DecFinish(UINT32 u4VDecID)
    {
        UINT32 u4RegVal = 0;

        u4RegVal = u4VDecReadVLD(u4VDecID, RO_VP6_VLD_DDR3_Finish);

        if (u4RegVal & VP6_DDR3_FINISH)
            return TRUE;
        else
            return FALSE;
    }

    // The following functions are only for verify
    void vVDEC_HAL_VP6_DecEndProcess(UINT32 u4VDecID)
    {

    }


    void vVDEC_HAL_VP6_VDec_DumpReg(UINT32 u4VDecID, BOOL fgBefore)
    {
        INT32 i; 
        INT32 i4VldStart = 42;
        INT32 i4VldEnd = 256;
        INT32 i4McStart = 0;
        INT32 i4McEnd = 400;
        UINT32 u4Data;

        printk("VP6 Dump Register: ");
        if (fgBefore == TRUE)
        {
            printk("Before Decode\n");
        }
        else
        {
            printk("After Decode\n");
        }

        printk("VLD Registers\n");
        for (i= i4VldStart; i<i4VldEnd; i++)
        {
            u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
            printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
        }

        printk("MC Registers\n");
        for (i=i4McStart; i<i4McEnd; i++)
        {
            u4Data = u4VDecReadMC(u4VDecID, (i<<2));
            printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);
        }

        i = 147;
        u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
        printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

        i = 148;
        u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
        printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

        i = 147;
        u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
        printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

        i = 148;
        u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
        printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

        i = 147;
        u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
        printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

        i = 148;
        u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
        printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

        i = 147;
        u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
        printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

        i = 148;
        u4Data = u4VDecReadVLD(u4VDecID, (i<<2));
        printk("%04d (0x%04X) = 0x%08X\n", i, (i<<2), u4Data);

    }


    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_GetBoolCoderShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
    // Description :Read Barrel Shifter before shifting
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4ShiftBits : shift bits number
    // Return      : Value of barrel shifter input window before shifting
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_GetBoolCoderShift(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
    {
        UINT32 u4RegVal0;

        printk("<vdec> VP6_VLD_READ_BOOL(%d)\n", u4ShiftBits);
        u4RegVal0 = u4VDecVP6BOOLGetBits(u4BSID, u4VDecID, u4ShiftBits);

        return(u4RegVal0);
    }


    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_InitBoolCoder(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits);
    // Description :Read Barrel Shifter before shifting
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4ShiftBits : shift bits number
    // Return      : Value of barrel shifter input window before shifting
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_InitBoolCoder(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4ShiftBits)
    {  
        //UINT32 u4Tmp;
        //UINT32 u4TryCnt = 0;

        //RISCWrite(`VP6_ADDR + 4*33, 1);
        if(u4BSID == 0)
            vVDecWriteVP6VLD(u4VDecID, RW_VP6_INIT_BOOL, RW_VP6_BOOL_EN, u4BSID);
        else
            vVDecVP6WriteVLD2Shift(RW_VP6_INIT_BOOL, RW_VP6_BOOL_EN);

        //wait(`VDEC_FULL.vld.vp6_vld_main.vp6_vld_core.get_bits_ready);
        //Polling 0x94[0]
        //u4VDecWaitVP6GetBitsReady(u4VDecID, u4BSID);

        return HAL_HANDLE_OK;
    }


    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_Default_Models_Init(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits);
    // Description :Read current read pointer
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4VFIFOSa : video FIFO start address
    //                 pu4Bits : read pointer value with remained bits
    // Return      : Read pointer value with byte alignment
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_Default_Models_Init(UINT32 u4BSID, UINT32 u4VDecID)
    {
        UINT32 pred_range, mbt_range, mvd_range, cfm_range;
        UINT32 i, j, u4Val;
        UINT32 u4Tmp0, u4Tmp1, u4Tmp2, u4Tmp3;
        UCHAR vector_model_dct[2];
        UCHAR vector_model_sig[2];
        UCHAR vp56_def_mb_types_stats[3][10][2] = 
        {
            { {  69, 42 }, {   1,  2 }, {  1,   7 }, {  44, 42 }, {  6, 22 },
            {   1,  3 }, {   0,  2 }, {  1,   5 }, {   0,  1 }, {  0,  0 }, },
            { { 229,  8 }, {   1,  1 }, {  0,   8 }, {   0,  0 }, {  0,  0 },
            {   1,  2 }, {   0,  1 }, {  0,   0 }, {   1,  1 }, {  0,  0 }, },
            { { 122, 35 }, {   1,  1 }, {  1,   6 }, {  46, 34 }, {  0,  0 },
            {   1,  2 }, {   0,  1 }, {  0,   1 }, {   1,  1 }, {  0,  0 }, },
        };

        UCHAR vp6_def_pdv_vector_model[2][7] = 
        {    
            { 225, 146, 172, 147, 214,  39, 156 },   
            { 204, 170, 119, 235, 140, 230, 228 },
        };

        UCHAR vp6_def_fdv_vector_model[2][8] = 
        {    
            { 247, 210, 135, 68, 138, 220, 239, 246 },
            { 244, 184, 201, 44, 173, 221, 239, 253 },
        };

        UCHAR vp6_def_runv_coeff_model[2][14] = 
        {    { 198, 197, 196, 146, 198, 204, 169, 142, 130, 136, 149, 149, 191, 249 },
        { 135, 201, 181, 154,  98, 117, 132, 126, 146, 169, 184, 240, 246, 254 },
        };

#ifdef VP6_MULTI_STREAM
        UINT32 u4Addr;
#else
        UINT32 Log_def_mb_types_stats_50[3][2];
        UINT32 Log_def_mb_types_stats_51[3][2];
        UINT32 Log_def_mb_types_stats_52[3][2];
        UINT32 Log_def_mb_types_stats_53[3][2];
        UINT32 Log_def_mb_types_stats_54[3][2];

        UINT32 Log_vector_model_dct[5];
        UINT32 Log_def_pdv_vector_model[5];              
        UINT32 Log_def_fdv_vector_model[5];       
        UINT32 Log_def_runv0_coeff_model[5];
        UINT32 Log_def_runv1_coeff_model[5];
#endif

        vector_model_dct[0] = 0xA2;
        vector_model_dct[1] = 0xA4;
        vector_model_sig[0] = 0x80;
        vector_model_sig[1] = 0x80;

        //    MEMCPY_ALIGNED(s->mb_types_stats, vp56_def_mb_types_stats, sizeof(s->mb_types_stats));
        //    MEMCPY_ALIGNED(s->vector_model_fdv, vp6_def_fdv_vector_model, sizeof(s->vector_model_fdv));
        //    MEMCPY_ALIGNED(s->vector_model_pdv, vp6_def_pdv_vector_model, sizeof(s->vector_model_pdv));
        //    MEMCPY_ALIGNED(s->coeff_model_runv, vp6_def_runv_coeff_model, sizeof(s->coeff_model_runv));
        //    MEMCPY_ALIGNED(s->coeff_reorder, vp6_def_coeff_reorder, sizeof(s->coeff_reorder));

        //   vp6_coeff_order_table_init(s);
        printk("<vdec> entry %s\n", __FUNCTION__);

        pred_range = 840;
        mbt_range = 102;
        mvd_range = 3;
        cfm_range = 40;

#ifdef VP6_MULTI_STREAM
        for (i=0; i<3; i++)
        {
            for (j=0; j<2; j++)
            {
                u4Addr = MC_VLD_WRAPPER_WRITE | ((pred_range+96+i*2+j) << 2);

                u4Tmp0 = vp56_def_mb_types_stats[i][0][j];
                u4Tmp1 = vp56_def_mb_types_stats[i][1][j];
                u4Tmp2 = vp56_def_mb_types_stats[i][2][j];
                u4Tmp3 = vp56_def_mb_types_stats[i][3][j];
                u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x3));
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

                u4Tmp0 = vp56_def_mb_types_stats[i][4][j];
                u4Tmp1 = vp56_def_mb_types_stats[i][5][j];
                u4Tmp2 = vp56_def_mb_types_stats[i][6][j];
                u4Tmp3 = vp56_def_mb_types_stats[i][7][j];
                u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x2));
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

                u4Tmp0 = vp56_def_mb_types_stats[i][8][j];
                u4Tmp1 = vp56_def_mb_types_stats[i][9][j];
                u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x1));
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

                u4Val = 0;
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x0));
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);
            }
        }

        ////////////////////
        u4Addr = MC_VLD_WRAPPER_WRITE | ((pred_range+mbt_range) << 2);

        u4Tmp0 = vector_model_dct[0];
        u4Tmp1 = vector_model_sig[0];
        u4Tmp2 = vector_model_dct[1];
        u4Tmp3 = vector_model_sig[1];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Val = 0;
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x2));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Val = 0;
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x1));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Val = 0;
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x0));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        ////////////////////
        u4Addr = MC_VLD_WRAPPER_WRITE | ((pred_range+mbt_range+1) << 2);

        u4Tmp0 = vp6_def_pdv_vector_model[0][0];
        u4Tmp1 = vp6_def_pdv_vector_model[0][1];
        u4Tmp2 = vp6_def_pdv_vector_model[0][2];
        u4Tmp3 = vp6_def_pdv_vector_model[0][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_pdv_vector_model[0][4];
        u4Tmp1 = vp6_def_pdv_vector_model[0][5];
        u4Tmp2 = vp6_def_pdv_vector_model[0][6];
        u4Tmp3 = vp6_def_pdv_vector_model[1][0];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x2));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_pdv_vector_model[1][1];
        u4Tmp1 = vp6_def_pdv_vector_model[1][2];
        u4Tmp2 = vp6_def_pdv_vector_model[1][3];
        u4Tmp3 = vp6_def_pdv_vector_model[1][4];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x1));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_pdv_vector_model[1][5];
        u4Tmp1 = vp6_def_pdv_vector_model[1][6];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x0));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        ////////////////////
        u4Addr = MC_VLD_WRAPPER_WRITE | ((pred_range+mbt_range+2) << 2);

        u4Tmp0 = vp6_def_fdv_vector_model[0][0];
        u4Tmp1 = vp6_def_fdv_vector_model[0][1];
        u4Tmp2 = vp6_def_fdv_vector_model[0][2];
        u4Tmp3 = vp6_def_fdv_vector_model[0][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_fdv_vector_model[0][4];
        u4Tmp1 = vp6_def_fdv_vector_model[0][5];
        u4Tmp2 = vp6_def_fdv_vector_model[0][6];
        u4Tmp3 = vp6_def_fdv_vector_model[0][7];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x2));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_fdv_vector_model[1][0];
        u4Tmp1 = vp6_def_fdv_vector_model[1][1];
        u4Tmp2 = vp6_def_fdv_vector_model[1][2];
        u4Tmp3 = vp6_def_fdv_vector_model[1][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x1));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_fdv_vector_model[1][4];
        u4Tmp1 = vp6_def_fdv_vector_model[1][5];
        u4Tmp2 = vp6_def_fdv_vector_model[1][6];
        u4Tmp3 = vp6_def_fdv_vector_model[1][7];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x0));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        ////////////////////
        u4Addr = MC_VLD_WRAPPER_WRITE | ((pred_range+mbt_range+mvd_range+2) << 2);

        u4Tmp0 = vp6_def_runv_coeff_model[0][0];
        u4Tmp1 = vp6_def_runv_coeff_model[0][1];
        u4Tmp2 = vp6_def_runv_coeff_model[0][2];
        u4Tmp3 = vp6_def_runv_coeff_model[0][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_runv_coeff_model[0][4];
        u4Tmp1 = vp6_def_runv_coeff_model[0][5];
        u4Tmp2 = vp6_def_runv_coeff_model[0][6];
        u4Tmp3 = vp6_def_runv_coeff_model[0][7];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x2));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_runv_coeff_model[0][8];
        u4Tmp1 = vp6_def_runv_coeff_model[0][9];
        u4Tmp2 = vp6_def_runv_coeff_model[0][10];
        u4Tmp3 = vp6_def_runv_coeff_model[0][11];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x1));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_runv_coeff_model[0][12];
        u4Tmp1 = vp6_def_runv_coeff_model[0][13];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x0));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        ////////////////////
        u4Addr = MC_VLD_WRAPPER_WRITE | ((pred_range+mbt_range+mvd_range+3) << 2);

        u4Tmp0 = vp6_def_runv_coeff_model[1][0];
        u4Tmp1 = vp6_def_runv_coeff_model[1][1];
        u4Tmp2 = vp6_def_runv_coeff_model[1][2];
        u4Tmp3 = vp6_def_runv_coeff_model[1][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_runv_coeff_model[1][4];
        u4Tmp1 = vp6_def_runv_coeff_model[1][5];
        u4Tmp2 = vp6_def_runv_coeff_model[1][6];
        u4Tmp3 = vp6_def_runv_coeff_model[1][7];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x2));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_runv_coeff_model[1][8];
        u4Tmp1 = vp6_def_runv_coeff_model[1][9];
        u4Tmp2 = vp6_def_runv_coeff_model[1][10];
        u4Tmp3 = vp6_def_runv_coeff_model[1][11];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x1));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

        u4Tmp0 = vp6_def_runv_coeff_model[1][12];
        u4Tmp1 = vp6_def_runv_coeff_model[1][13];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x0));
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

#else

        //fprintf(risc_out, "//---------- mbt%d model initial value ----------\n", i*2+j);
        for (i=0; i<3; i++)
        {
            for (j=0; j<2; j++)
            {
                //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*50, 32'd%d);\n", (pred_range+96+i*2+j));
                u4Val = pred_range+96+i*2+j;
                vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Val, u4BSID);
                Log_def_mb_types_stats_50[i][j] = u4Val;

                //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
                //vp56_def_mb_types_stats[i][0][j],
                //vp56_def_mb_types_stats[i][1][j],
                //vp56_def_mb_types_stats[i][2][j],
                //vp56_def_mb_types_stats[i][3][j]);
                u4Tmp0 = vp56_def_mb_types_stats[i][0][j];
                u4Tmp1 = vp56_def_mb_types_stats[i][1][j];
                u4Tmp2 = vp56_def_mb_types_stats[i][2][j];
                u4Tmp3 = vp56_def_mb_types_stats[i][3][j];
                u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
                vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1, u4Val, u4BSID);
                Log_def_mb_types_stats_51[i][j] = u4Val;

                //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
                //vp56_def_mb_types_stats[i][4][j],
                //vp56_def_mb_types_stats[i][5][j],
                //vp56_def_mb_types_stats[i][6][j],
                //vp56_def_mb_types_stats[i][7][j]);
                u4Tmp0 = vp56_def_mb_types_stats[i][4][j];
                u4Tmp1 = vp56_def_mb_types_stats[i][5][j];
                u4Tmp2 = vp56_def_mb_types_stats[i][6][j];
                u4Tmp3 = vp56_def_mb_types_stats[i][7][j];
                u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));                        
                vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR2, u4Val, u4BSID);
                Log_def_mb_types_stats_52[i][j] = u4Val;

                //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*53, 32'h%.2x%.2x0000);\n",
                //vp56_def_mb_types_stats[i][8][j],
                //vp56_def_mb_types_stats[i][9][j]);    
                u4Tmp0 = vp56_def_mb_types_stats[i][8][j];
                u4Tmp1 = vp56_def_mb_types_stats[i][9][j];
                u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));                       
                vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR3, u4Val, u4BSID);
                Log_def_mb_types_stats_53[i][j] = u4Val;

                //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*54, 32'h0);\n");
                u4Val = 0;
                vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR4, u4Val, u4BSID);
                Log_def_mb_types_stats_54[i][j] = u4Val;
            }
        }


        //fprintf(risc_out, "//---------- dct-sig model initial value ----------\n");
        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*50, 32'd%d);\n", (pred_range+mbt_range));   
        u4Val = pred_range+mbt_range;
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Val, u4BSID);
        Log_vector_model_dct[0] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
        //s->vector_model_dct[0],
        //s->vector_model_sig[0],
        //s->vector_model_dct[1],
        //s->vector_model_sig[1]);
        u4Tmp0 = vector_model_dct[0];
        u4Tmp1 = vector_model_sig[0];
        u4Tmp2 = vector_model_dct[1];
        u4Tmp3 = vector_model_sig[1];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1, u4Val, u4BSID);
        Log_vector_model_dct[1] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*52, 32'h0);\n");
        u4Val = 0;
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR2, u4Val, u4BSID);
        Log_vector_model_dct[2] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*53, 32'h0);\n");
        u4Val = 0;
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR3, u4Val, u4BSID);
        Log_vector_model_dct[3] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*54, 32'h0);\n");
        u4Val = 0;
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR4, u4Val, u4BSID);
        Log_vector_model_dct[4] = u4Val;

        //fprintf(risc_out, "//---------- pdv model initial value ----------\n");
        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*50, 32'd%d);\n", (pred_range+mbt_range+1)); 
        u4Val = pred_range+mbt_range+1;
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Val, u4BSID);
        Log_def_pdv_vector_model[0] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_pdv_vector_model[0][0],
        //vp6_def_pdv_vector_model[0][1],
        //vp6_def_pdv_vector_model[0][2],
        //vp6_def_pdv_vector_model[0][3]);
        u4Tmp0 = vp6_def_pdv_vector_model[0][0];
        u4Tmp1 = vp6_def_pdv_vector_model[0][1];
        u4Tmp2 = vp6_def_pdv_vector_model[0][2];
        u4Tmp3 = vp6_def_pdv_vector_model[0][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1, u4Val, u4BSID);
        Log_def_pdv_vector_model[1] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_pdv_vector_model[0][4],
        //vp6_def_pdv_vector_model[0][5],
        //vp6_def_pdv_vector_model[0][6],
        //vp6_def_pdv_vector_model[1][0]);
        u4Tmp0 = vp6_def_pdv_vector_model[0][4];
        u4Tmp1 = vp6_def_pdv_vector_model[0][5];
        u4Tmp2 = vp6_def_pdv_vector_model[0][6];
        u4Tmp3 = vp6_def_pdv_vector_model[1][0];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR2, u4Val, u4BSID);
        Log_def_pdv_vector_model[2] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*53, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_pdv_vector_model[1][1],
        //vp6_def_pdv_vector_model[1][2],
        //vp6_def_pdv_vector_model[1][3],
        //vp6_def_pdv_vector_model[1][4]);
        u4Tmp0 = vp6_def_pdv_vector_model[1][1];
        u4Tmp1 = vp6_def_pdv_vector_model[1][2];
        u4Tmp2 = vp6_def_pdv_vector_model[1][3];
        u4Tmp3 = vp6_def_pdv_vector_model[1][4];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR3, u4Val, u4BSID);
        Log_def_pdv_vector_model[3] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*54, 32'h%.2x%.2x0000);\n",
        //vp6_def_pdv_vector_model[1][5],
        //vp6_def_pdv_vector_model[1][6]);
        u4Tmp0 = vp6_def_pdv_vector_model[1][5];
        u4Tmp1 = vp6_def_pdv_vector_model[1][6];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR4, u4Val, u4BSID);
        Log_def_pdv_vector_model[4] = u4Val;

        //fprintf(risc_out, "//---------- fdv model initial value ----------\n");
        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*50, 32'd%d);\n", (pred_range+mbt_range+2)); 
        u4Val = pred_range+mbt_range+2;
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Val, u4BSID);
        Log_def_fdv_vector_model[0] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_fdv_vector_model[0][0],
        //vp6_def_fdv_vector_model[0][1],
        //vp6_def_fdv_vector_model[0][2],
        //vp6_def_fdv_vector_model[0][3]);
        u4Tmp0 = vp6_def_fdv_vector_model[0][0];
        u4Tmp1 = vp6_def_fdv_vector_model[0][1];
        u4Tmp2 = vp6_def_fdv_vector_model[0][2];
        u4Tmp3 = vp6_def_fdv_vector_model[0][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1, u4Val, u4BSID);
        Log_def_fdv_vector_model[1] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_fdv_vector_model[0][4],
        //vp6_def_fdv_vector_model[0][5],
        //vp6_def_fdv_vector_model[0][6],
        //vp6_def_fdv_vector_model[0][7]);
        u4Tmp0 = vp6_def_fdv_vector_model[0][4];
        u4Tmp1 = vp6_def_fdv_vector_model[0][5];
        u4Tmp2 = vp6_def_fdv_vector_model[0][6];
        u4Tmp3 = vp6_def_fdv_vector_model[0][7];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR2, u4Val, u4BSID);
        Log_def_fdv_vector_model[2] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*53, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_fdv_vector_model[1][0],
        //vp6_def_fdv_vector_model[1][1],
        //vp6_def_fdv_vector_model[1][2],
        //vp6_def_fdv_vector_model[1][3]);
        u4Tmp0 = vp6_def_fdv_vector_model[1][0];
        u4Tmp1 = vp6_def_fdv_vector_model[1][1];
        u4Tmp2 = vp6_def_fdv_vector_model[1][2];
        u4Tmp3 = vp6_def_fdv_vector_model[1][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR3, u4Val, u4BSID);
        Log_def_fdv_vector_model[3] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*54, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_fdv_vector_model[1][4],
        //vp6_def_fdv_vector_model[1][5],
        //vp6_def_fdv_vector_model[1][6],
        //vp6_def_fdv_vector_model[1][7]);
        u4Tmp0 = vp6_def_fdv_vector_model[1][4];
        u4Tmp1 = vp6_def_fdv_vector_model[1][5];
        u4Tmp2 = vp6_def_fdv_vector_model[1][6];
        u4Tmp3 = vp6_def_fdv_vector_model[1][7];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR4, u4Val, u4BSID);
        Log_def_fdv_vector_model[4] = u4Val;

        //fprintf(risc_out, "//---------- runv0 model initial value ----------\n");
        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*50, 32'd%d);\n", (pred_range+mbt_range+mvd_range+2));   
        u4Val = pred_range+mbt_range+mvd_range+2;
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Val, u4BSID);
        Log_def_runv0_coeff_model[0] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_runv_coeff_model[0][0],
        //vp6_def_runv_coeff_model[0][1],
        //vp6_def_runv_coeff_model[0][2],
        //vp6_def_runv_coeff_model[0][3]);
        u4Tmp0 = vp6_def_runv_coeff_model[0][0];
        u4Tmp1 = vp6_def_runv_coeff_model[0][1];
        u4Tmp2 = vp6_def_runv_coeff_model[0][2];
        u4Tmp3 = vp6_def_runv_coeff_model[0][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1, u4Val, u4BSID);
        Log_def_runv0_coeff_model[1] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_runv_coeff_model[0][4],
        //vp6_def_runv_coeff_model[0][5],
        //vp6_def_runv_coeff_model[0][6],
        //vp6_def_runv_coeff_model[0][7]);
        u4Tmp0 = vp6_def_runv_coeff_model[0][4];
        u4Tmp1 = vp6_def_runv_coeff_model[0][5];
        u4Tmp2 = vp6_def_runv_coeff_model[0][6];
        u4Tmp3 = vp6_def_runv_coeff_model[0][7];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR2, u4Val, u4BSID);
        Log_def_runv0_coeff_model[2] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*53, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_runv_coeff_model[0][8],
        //vp6_def_runv_coeff_model[0][9],
        //vp6_def_runv_coeff_model[0][10],
        //vp6_def_runv_coeff_model[0][11]);
        u4Tmp0 = vp6_def_runv_coeff_model[0][8];
        u4Tmp1 = vp6_def_runv_coeff_model[0][9];
        u4Tmp2 = vp6_def_runv_coeff_model[0][10];
        u4Tmp3 = vp6_def_runv_coeff_model[0][11];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR3, u4Val, u4BSID);
        Log_def_runv0_coeff_model[3] = u4Val;

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*54, 32'h%.2x%.2x0000);\n",
        //vp6_def_runv_coeff_model[0][12],
        //vp6_def_runv_coeff_model[0][13]);
        u4Tmp0 = vp6_def_runv_coeff_model[0][12];
        u4Tmp1 = vp6_def_runv_coeff_model[0][13];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR4, u4Val, u4BSID);
        Log_def_runv0_coeff_model[4] = u4Val;

        //fprintf(risc_out, "//---------- runv1 model initial value ----------\n");
        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*50, 32'd%d);\n",(pred_range+mbt_range+mvd_range+3));
        u4Val = pred_range+mbt_range+mvd_range+3;
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Val, u4BSID);
        Log_def_runv1_coeff_model[0] = u4Val;   

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_runv_coeff_model[1][0],
        //vp6_def_runv_coeff_model[1][1],
        //vp6_def_runv_coeff_model[1][2],
        //vp6_def_runv_coeff_model[1][3]);
        u4Tmp0 = vp6_def_runv_coeff_model[1][0];
        u4Tmp1 = vp6_def_runv_coeff_model[1][1];
        u4Tmp2 = vp6_def_runv_coeff_model[1][2];
        u4Tmp3 = vp6_def_runv_coeff_model[1][3];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1, u4Val, u4BSID);
        Log_def_runv1_coeff_model[1] = u4Val;   

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_runv_coeff_model[1][4],
        //vp6_def_runv_coeff_model[1][5],
        //vp6_def_runv_coeff_model[1][6],
        //vp6_def_runv_coeff_model[1][7]);
        u4Tmp0 = vp6_def_runv_coeff_model[1][4];
        u4Tmp1 = vp6_def_runv_coeff_model[1][5];
        u4Tmp2 = vp6_def_runv_coeff_model[1][6];
        u4Tmp3 = vp6_def_runv_coeff_model[1][7];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR2, u4Val, u4BSID);
        Log_def_runv1_coeff_model[2] = u4Val;   

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*53, 32'h%.2x%.2x%.2x%.2x);\n",
        //vp6_def_runv_coeff_model[1][8],
        //vp6_def_runv_coeff_model[1][9],
        //vp6_def_runv_coeff_model[1][10],
        //vp6_def_runv_coeff_model[1][11]);
        u4Tmp0 = vp6_def_runv_coeff_model[1][8];
        u4Tmp1 = vp6_def_runv_coeff_model[1][9];
        u4Tmp2 = vp6_def_runv_coeff_model[1][10];
        u4Tmp3 = vp6_def_runv_coeff_model[1][11];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR3, u4Val, u4BSID);
        Log_def_runv1_coeff_model[3] = u4Val;   

        //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*54, 32'h%.2x%.2x0000);\n",
        //vp6_def_runv_coeff_model[1][12],
        //vp6_def_runv_coeff_model[1][13]);
        u4Tmp0 = vp6_def_runv_coeff_model[1][12];
        u4Tmp1 = vp6_def_runv_coeff_model[1][13];
        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR4, u4Val, u4BSID);
        Log_def_runv1_coeff_model[4] = u4Val;   

#endif

        //fprintf(risc_out, "//---------- model ini finial ----------\n");
        ////fprintf(risc_ini_out,"RISCWrite(`VP6_ADDR + 4*55, 32'h1);\n");

        //PANDA
        //u4Val = 1;
        //vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WWFR, u4Val, u4BSID);
        //u4Val = WWFR_COEFF_ERROR|WWFR_MC_TIMEOUT|WWFR_MV_TIMEOUT|WWFR_DCAC_TIMEOUT|WWFR_HUFF_ERR;
        //vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WWFR, u4Val, u4BSID);

        return HAL_HANDLE_OK;
    }


    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_Parse_Mb_Type_Models(UINT32 u4BSID, UINT32 u4VDecID)
    // Description :Read current read pointer
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4VFIFOSa : video FIFO start address
    //                 pu4Bits : read pointer value with remained bits
    // Return      : Read pointer value with byte alignment
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_Parse_Mb_Type_Models(UINT32 u4BSID, UINT32 u4VDecID)
    {    
        UCHAR    vp56_pre_def_mb_type_stats[16][3][10][2] = 
        {
            { 
                { {   9, 15 }, {  32, 25 }, {  7,  19 }, {   9, 21 }, {  1, 12 },
                {  14, 12 }, {   3, 18 }, { 14,  23 }, {   3, 10 }, {  0,  4 }, },
                { {  41, 22 }, {   1,  0 }, {  1,  31 }, {   0,  0 }, {  0,  0 },
                {   0,  1 }, {   1,  7 }, {  0,   1 }, {  98, 25 }, {  4, 10 }, },
                { {   2,  3 }, {   2,  3 }, {  0,   2 }, {   0,  2 }, {  0,  0 },
                {  11,  4 }, {   1,  4 }, {  0,   2 }, {   3,  2 }, {  0,  4 }, }, 
            },
            { 
                { {  48, 39 }, {   1,  2 }, { 11,  27 }, {  29, 44 }, {  7, 27 },
                {   1,  4 }, {   0,  3 }, {  1,   6 }, {   1,  2 }, {  0,  0 }, },
                { { 123, 37 }, {   6,  4 }, {  1,  27 }, {   0,  0 }, {  0,  0 },
                {   5,  8 }, {   1,  7 }, {  0,   1 }, {  12, 10 }, {  0,  2 }, },
                { {  49, 46 }, {   3,  4 }, {  7,  31 }, {  42, 41 }, {  0,  0 },
                {   2,  6 }, {   1,  7 }, {  1,   4 }, {   2,  4 }, {  0,  1 }, }, 
            },
            {
                { {  21, 32 }, {   1,  2 }, {  4,  10 }, {  32, 43 }, {  6, 23 },
                {   2,  3 }, {   1, 19 }, {  1,   6 }, {  12, 21 }, {  0,  7 }, },
                { {  26, 14 }, {  14, 12 }, {  0,  24 }, {   0,  0 }, {  0,  0 },
                {  55, 17 }, {   1,  9 }, {  0,  36 }, {   5,  7 }, {  1,  3 }, },
                { {  26, 25 }, {   1,  1 }, {  2,  10 }, {  67, 39 }, {  0,  0 },
                {   1,  1 }, {   0, 14 }, {  0,   2 }, {  31, 26 }, {  1,  6 }, }, 
            },
            {
                { {  69, 83 }, {   0,  0 }, {  0,   2 }, {  10, 29 }, {  3, 12 },
                {   0,  1 }, {   0,  3 }, {  0,   3 }, {   2,  2 }, {  0,  0 }, },
                { { 209,  5 }, {   0,  0 }, {  0,  27 }, {   0,  0 }, {  0,  0 },
                {   0,  1 }, {   0,  1 }, {  0,   1 }, {   0,  0 }, {  0,  0 }, },
                { { 103, 46 }, {   1,  2 }, {  2,  10 }, {  33, 42 }, {  0,  0 },
                {   1,  4 }, {   0,  3 }, {  0,   1 }, {   1,  3 }, {  0,  0 }, }, 
            },
            {
                { {  11, 20 }, {   1,  4 }, { 18,  36 }, {  43, 48 }, { 13, 35 },
                {   0,  2 }, {   0,  5 }, {  3,  12 }, {   1,  2 }, {  0,  0 }, },
                { {   2,  5 }, {   4,  5 }, {  0, 121 }, {   0,  0 }, {  0,  0 },
                {   0,  3 }, {   2,  4 }, {  1,   4 }, {   2,  2 }, {  0,  1 }, },
                { {  14, 31 }, {   9, 13 }, { 14,  54 }, {  22, 29 }, {  0,  0 },
                {   2,  6 }, {   4, 18 }, {  6,  13 }, {   1,  5 }, {  0,  1 }, }, 
            },
            { 
                { {  70, 44 }, {   0,  1 }, {  2,  10 }, {  37, 46 }, {  8, 26 },
                {   0,  2 }, {   0,  2 }, {  0,   2 }, {   0,  1 }, {  0,  0 }, },
                { { 175,  5 }, {   0,  1 }, {  0,  48 }, {   0,  0 }, {  0,  0 },
                {   0,  2 }, {   0,  1 }, {  0,   2 }, {   0,  1 }, {  0,  0 }, },
                { {  85, 39 }, {   0,  0 }, {  1,   9 }, {  69, 40 }, {  0,  0 },
                {   0,  1 }, {   0,  3 }, {  0,   1 }, {   2,  3 }, {  0,  0 }, }, 
            },
            { 
                { {   8, 15 }, {   0,  1 }, {  8,  21 }, {  74, 53 }, { 22, 42 },
                {   0,  1 }, {   0,  2 }, {  0,   3 }, {   1,  2 }, {  0,  0 }, },
                { {  83,  5 }, {   2,  3 }, {  0, 102 }, {   0,  0 }, {  0,  0 },
                {   1,  3 }, {   0,  2 }, {  0,   1 }, {   0,  0 }, {  0,  0 }, },
                { {  31, 28 }, {   0,  0 }, {  3,  14 }, { 130, 34 }, {  0,  0 },
                {   0,  1 }, {   0,  3 }, {  0,   1 }, {   3,  3 }, {  0,  1 }, }, 
            },
            { 
                { { 141, 42 }, {   0,  0 }, {  1,   4 }, {  11, 24 }, {  1, 11 },
                {   0,  1 }, {   0,  1 }, {  0,   2 }, {   0,  0 }, {  0,  0 }, },
                { { 233,  6 }, {   0,  0 }, {  0,   8 }, {   0,  0 }, {  0,  0 },
                {   0,  1 }, {   0,  1 }, {  0,   0 }, {   0,  1 }, {  0,  0 }, },
                { { 171, 25 }, {   0,  0 }, {  1,   5 }, {  25, 21 }, {  0,  0 },
                {   0,  1 }, {   0,  1 }, {  0,   0 }, {   0,  0 }, {  0,  0 }, },
            },
            {
                { {   8, 19 }, {   4, 10 }, { 24,  45 }, {  21, 37 }, {  9, 29 },
                {   0,  3 }, {   1,  7 }, { 11,  25 }, {   0,  2 }, {  0,  1 }, },
                { {  34, 16 }, { 112, 21 }, {  1,  28 }, {   0,  0 }, {  0,  0 },
                {   6,  8 }, {   1,  7 }, {  0,   3 }, {   2,  5 }, {  0,  2 }, },
                { {  17, 21 }, {  68, 29 }, {  6,  15 }, {  13, 22 }, {  0,  0 },
                {   6, 12 }, {   3, 14 }, {  4,  10 }, {   1,  7 }, {  0,  3 }, }, 
            },
            {
                { {  46, 42 }, {   0,  1 }, {  2,  10 }, {  54, 51 }, { 10, 30 },
                {   0,  2 }, {   0,  2 }, {  0,   1 }, {   0,  1 }, {  0,  0 }, },
                { { 159, 35 }, {   2,  2 }, {  0,  25 }, {   0,  0 }, {  0,  0 },
                {   3,  6 }, {   0,  5 }, {  0,   1 }, {   4,  4 }, {  0,  1 }, },
                { {  51, 39 }, {   0,  1 }, {  2,  12 }, {  91, 44 }, {  0,  0 },
                {   0,  2 }, {   0,  3 }, {  0,   1 }, {   2,  3 }, {  0,  1 }, }, 
            },
            {   
                { {  28, 32 }, {   0,  0 }, {  3,  10 }, {  75, 51 }, { 14, 33 },
                {   0,  1 }, {   0,  2 }, {  0,   1 }, {   1,  2 }, {  0,  0 }, },
                { {  75, 39 }, {   5,  7 }, {  2,  48 }, {   0,  0 }, {  0,  0 },
                {   3, 11 }, {   2, 16 }, {  1,   4 }, {   7, 10 }, {  0,  2 }, },
                { {  81, 25 }, {   0,  0 }, {  2,   9 }, { 106, 26 }, {  0,  0 },
                {   0,  1 }, {   0,  1 }, {  0,   1 }, {   1,  1 }, {  0,  0 }, }, 
            },
            {   
                { { 100, 46 }, {   0,  1 }, {  3,   9 }, {  21, 37 }, {  5, 20 },
                {   0,  1 }, {   0,  2 }, {  1,   2 }, {   0,  1 }, {  0,  0 }, },
                { { 212, 21 }, {   0,  1 }, {  0,   9 }, {   0,  0 }, {  0,  0 },
                {   1,  2 }, {   0,  2 }, {  0,   0 }, {   2,  2 }, {  0,  0 }, },
                { { 140, 37 }, {   0,  1 }, {  1,   8 }, {  24, 33 }, {  0,  0 },
                {   1,  2 }, {   0,  2 }, {  0,   1 }, {   1,  2 }, {  0,  0 }, },
            },
            { 
                { {  27, 29 }, {   0,  1 }, {  9,  25 }, {  53, 51 }, { 12, 34 },
                {   0,  1 }, {   0,  3 }, {  1,   5 }, {   0,  2 }, {  0,  0 }, },
                { {   4,  2 }, {   0,  0 }, {  0, 172 }, {   0,  0 }, {  0,  0 },
                {   0,  1 }, {   0,  2 }, {  0,   0 }, {   2,  0 }, {  0,  0 }, },
                { {  14, 23 }, {   1,  3 }, { 11,  53 }, {  90, 31 }, {  0,  0 },
                {   0,  3 }, {   1,  5 }, {  2,   6 }, {   1,  2 }, {  0,  0 }, }, 
            },
            { 
                { {  80, 38 }, {   0,  0 }, {  1,   4 }, {  69, 33 }, {  5, 16 },
                {   0,  1 }, {   0,  1 }, {  0,   0 }, {   0,  1 }, {  0,  0 }, },
                { { 187, 22 }, {   1,  1 }, {  0,  17 }, {   0,  0 }, {  0,  0 },
                {   3,  6 }, {   0,  4 }, {  0,   1 }, {   4,  4 }, {  0,  1 }, },
                { { 123, 29 }, {   0,  0 }, {  1,   7 }, {  57, 30 }, {  0,  0 },
                {   0,  1 }, {   0,  1 }, {  0,   1 }, {   0,  1 }, {  0,  0 }, }, 
                },
            { 
                { {  16, 20 }, {   0,  0 }, {  2,   8 }, { 104, 49 }, { 15, 33 },
                {   0,  1 }, {   0,  1 }, {  0,   1 }, {   1,  1 }, {  0,  0 }, },
                { { 133,  6 }, {   1,  2 }, {  1,  70 }, {   0,  0 }, {  0,  0 },
                {   0,  2 }, {   0,  4 }, {  0,   3 }, {   1,  1 }, {  0,  0 }, },
                { {  13, 14 }, {   0,  0 }, {  4,  20 }, { 175, 20 }, {  0,  0 },
                {   0,  1 }, {   0,  1 }, {  0,   1 }, {   1,  1 }, {  0,  0 }, }, 
            },
            {
                { { 194, 16 }, {   0,  0 }, {  1,   1 }, {   1,  9 }, {  1,  3 },
                {   0,  0 }, {   0,  1 }, {  0,   1 }, {   0,  0 }, {  0,  0 }, },
                { { 251,  1 }, {   0,  0 }, {  0,   2 }, {   0,  0 }, {  0,  0 },
                {   0,  0 }, {   0,  0 }, {  0,   0 }, {   0,  0 }, {  0,  0 }, },
                { { 202, 23 }, {   0,  0 }, {  1,   3 }, {   2,  9 }, {  0,  0 },
                {   0,  1 }, {   0,  1 }, {  0,   1 }, {   0,  0 }, {  0,  0 }, }, 
            },
        };
        UINT32    pred_range = 840;
        //UINT32    mbt_range = 102;
        //UINT32    mvd_range = 3;
        //UINT32    cfm_range = 40;
        UINT32    idx, ctx, i;
        UINT32    u4Val, u4Tmp0, u4Tmp1, u4Tmp2, u4Tmp3;
#ifdef VP6_MULTI_STREAM
        UINT32 u4Addr;
        printk("<vdec> entry %s\n", __FUNCTION__);

        for (idx=0; idx<16; idx++) 
        {
            for (ctx=0; ctx<3; ctx++) 
            {
                for(i=0; i<2; i++)
                {
                    u4Addr = MC_VLD_WRAPPER_WRITE | ((idx*3*2 +ctx*2 +i+pred_range) << 2);

                    u4Tmp0 = vp56_pre_def_mb_type_stats[idx][ctx][0][i],
                        u4Tmp1 = vp56_pre_def_mb_type_stats[idx][ctx][1][i],
                        u4Tmp2 = vp56_pre_def_mb_type_stats[idx][ctx][2][i],
                        u4Tmp3 = vp56_pre_def_mb_type_stats[idx][ctx][3][i],
                        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x3));
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

                    u4Tmp0 = vp56_pre_def_mb_type_stats[idx][ctx][4][i],
                        u4Tmp1 = vp56_pre_def_mb_type_stats[idx][ctx][5][i],
                        u4Tmp2 = vp56_pre_def_mb_type_stats[idx][ctx][6][i],
                        u4Tmp3 = vp56_pre_def_mb_type_stats[idx][ctx][7][i],
                        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x2));
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

                    u4Tmp0 = vp56_pre_def_mb_type_stats[idx][ctx][8][i],
                        u4Tmp1 = vp56_pre_def_mb_type_stats[idx][ctx][9][i],
                        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x1));
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);

                    u4Val = 0;
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, (u4Addr | 0x0));
                    vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, u4Val);
                }
            }
        }
#else
        UINT32    u4Log50 [16][3][2] = 
        {
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
        };
        UINT32    u4Log51 [16][3][2] =
        {
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
        };
        UINT32    u4Log52 [16][3][2] =
        {
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
        };
        UINT32    u4Log53 [16][3][2] =
        {
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
        };
        UINT32    u4Log54 [16][3][2] =
        {
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
            {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}}, {{0,0}, {0,0}, {0,0}},
        };

        for (idx=0; idx<16; idx++) 
        {
            for (ctx=0; ctx<3; ctx++) 
            {
                for(i=0; i<2; i++)
                {
                    //fprintf(risc_out, "//---------- idx = %d ----------\n", idx);
                    //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*50, 32'h%x);\n", (idx*3*2 +ctx*2 +i+pred_range));
                    u4Val = (idx*3*2 +ctx*2 +i+pred_range);
                    vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_WARR, u4Val, u4BSID);
                    u4Log50[idx][ctx][i] = u4Val;

                    //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*51, 32'h%.2x%.2x%.2x%.2x);\n",
                    //vp56_pre_def_mb_type_stats[idx][ctx][0][i],
                    //vp56_pre_def_mb_type_stats[idx][ctx][1][i],
                    //vp56_pre_def_mb_type_stats[idx][ctx][2][i],
                    //vp56_pre_def_mb_type_stats[idx][ctx][3][i]);
                    u4Tmp0 = vp56_pre_def_mb_type_stats[idx][ctx][0][i],
                        u4Tmp1 = vp56_pre_def_mb_type_stats[idx][ctx][1][i],
                        u4Tmp2 = vp56_pre_def_mb_type_stats[idx][ctx][2][i],
                        u4Tmp3 = vp56_pre_def_mb_type_stats[idx][ctx][3][i],
                        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
                    vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR1, u4Val, u4BSID);
                    u4Log51[idx][ctx][i] = u4Val;

                    //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*52, 32'h%.2x%.2x%.2x%.2x);\n",
                    //vp56_pre_def_mb_type_stats[idx][ctx][4][i],
                    //vp56_pre_def_mb_type_stats[idx][ctx][5][i],
                    //vp56_pre_def_mb_type_stats[idx][ctx][6][i],
                    //vp56_pre_def_mb_type_stats[idx][ctx][7][i]);
                    u4Tmp0 = vp56_pre_def_mb_type_stats[idx][ctx][4][i],
                        u4Tmp1 = vp56_pre_def_mb_type_stats[idx][ctx][5][i],
                        u4Tmp2 = vp56_pre_def_mb_type_stats[idx][ctx][6][i],
                        u4Tmp3 = vp56_pre_def_mb_type_stats[idx][ctx][7][i],
                        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
                    vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR2, u4Val, u4BSID);
                    u4Log52[idx][ctx][i] = u4Val;

                    //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*53, 32'h%.2x%.2x0000);\n",
                    //vp56_pre_def_mb_type_stats[idx][ctx][8][i],
                    //vp56_pre_def_mb_type_stats[idx][ctx][9][i]);
                    u4Tmp0 = vp56_pre_def_mb_type_stats[idx][ctx][8][i],
                        u4Tmp1 = vp56_pre_def_mb_type_stats[idx][ctx][9][i],
                        u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16));
                    vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR3, u4Val, u4BSID);
                    u4Log53[idx][ctx][i] = u4Val;

                    //fprintf(risc_out,"RISCWrite(`VP6_ADDR + 4*54, 32'h0);\n");
                    u4Val = 0;
                    vVDecWriteVP6VLD(u4VDecID, RW_VP6_VLD_FCVR4, u4Val, u4BSID);
                    u4Log54[idx][ctx][i] = u4Val;
                }
            }
        }
#endif
                return 0;
    }

    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_Load_QMatrix(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4VFIFOSa, UINT32 *pu4Bits);
    // Description :Read current read pointer
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4VFIFOSa : video FIFO start address
    //                 pu4Bits : read pointer value with remained bits
    // Return      : Read pointer value with byte alignment
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_Load_QMatrix(UINT32 u4BSID, UINT32 u4VDecID)
    {
        UINT32 i;
        UINT32 u4Tmp0, u4Tmp1, u4Tmp2, u4Tmp3;
        UINT32 u4Val;
        UCHAR vp6_def_coeff_reorder[] = 
        {
            0,  0,  1,  1,  1,  2,  2,  2,
            2,  2,  2,  3,  3,  4,  4,  4,
            5,  5,  5,  5,  6,  6,  7,  7,
            7,  7,  7,  8,  8,  9,  9,  9,
            9,  9,  9, 10, 10, 11, 11, 11,
            11, 11, 11, 12, 12, 12, 12, 12,
            12, 13, 13, 13, 13, 13, 14, 14,
            14, 14, 15, 15, 15, 15, 15, 15,
        };
        printk("<vdec> entry %s\n", __FUNCTION__);
        for (i=0; i<64; i+=4) 
        {
            //fprintf(risc_out,"RISCWrite(`VLD_ADDR_1 +4*152, (0<<8) + %d);\n", i);
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, ((0<<8)+i) );

            //fprintf(risc_out,"RISCWrite(`VLD_ADDR_1 +4*153, (%d<<24) + (%d<<16) + (%d<<8) + (%d<<0));\n", 
            //s->coeff_reorder[i], 
            //s->coeff_reorder[i+1], 
            //s->coeff_reorder[i+2], 
            //s->coeff_reorder[i+3]);

            u4Tmp0 = vp6_def_coeff_reorder[i];
            u4Tmp1 = vp6_def_coeff_reorder[i+1];
            u4Tmp2 = vp6_def_coeff_reorder[i+2];
            u4Tmp3 = vp6_def_coeff_reorder[i+3];      
            u4Val = ((u4Tmp0 << 24) |(u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_DATA, u4Val);
        }

        return HAL_HANDLE_OK;
    }


    UINT32 u4VDEC_HAL_VP6_Read_QMatrix(UINT32 u4BSID, UINT32 u4VDecID)
    {
        UINT32 i;
        UINT32 u4Val;
        UCHAR rd_vp6_def_coeff_reorder[64] = 
        {
            0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,
            0,  0,  0,  0,  0,  0,  0,  0,
        };
        printk("<vdec> entry %s\n", __FUNCTION__);
        for (i=0; i<64; i+=4) 
        {
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, ((1<<8)+i) );
            u4Val = u4VDecReadVLD(u4VDecID, RW_VLD_SCL_DATA);

            rd_vp6_def_coeff_reorder[i]     = ((u4Val & 0xFF000000)>> 24);
            rd_vp6_def_coeff_reorder[i+1] = ((u4Val & 0x00FF0000) >> 16);
            rd_vp6_def_coeff_reorder[i+2] = ((u4Val & 0x0000FF00) >> 8);
            rd_vp6_def_coeff_reorder[i+3] = ((u4Val & 0x000000FF) >> 0);
        }

        return HAL_HANDLE_OK;
    }

    // **************************************************************************
    // Function : UINT32 u4VDEC_HAL_VP6_Load_Filter_Coef(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4Select);
    // Description :Read current read pointer
    // Parameter : u4BSID  : barrelshifter ID
    //                 u4VDecID : video decoder hardware ID
    //                 u4VFIFOSa : video FIFO start address
    //                 pu4Bits : read pointer value with remained bits
    // Return      : Read pointer value with byte alignment
    // **************************************************************************
    UINT32 u4VDEC_HAL_VP6_Load_Filter_Coef(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4Select)
    {
        INT16 vp6_block_copy_filter[16][8][4] = 
        {
            { {   0, 128,   0,   0  },  /* 0 */
            {  -3, 122,   9,   0  },
            {  -4, 109,  24,  -1  },
            {  -5,  91,  45,  -3  },
            {  -4,  68,  68,  -4  },
            {  -3,  45,  91,  -5  },
            {  -1,  24, 109,  -4  },
            {   0,   9, 122,  -3  } },

            { {   0, 128,   0,   0  },  /* 1 */
            {  -4, 124,   9,  -1  },
            {  -5, 110,  25,  -2  },
            {  -6,  91,  46,  -3  },
            {  -5,  69,  69,  -5  },
            {  -3,  46,  91,  -6  },
            {  -2,  25, 110,  -5  },
            {  -1,   9, 124,  -4  } },

            { {   0, 128,   0,   0  },  /* 2 */
            {  -4, 123,  10,  -1  },
            {  -6, 110,  26,  -2  },
            {  -7,  92,  47,  -4  },
            {  -6,  70,  70,  -6  },
            {  -4,  47,  92,  -7  },
            {  -2,  26, 110,  -6  },
            {  -1,  10, 123,  -4  } },

            { {   0, 128,   0,   0  },  /* 3 */
            {  -5, 124,  10,  -1  },
            {  -7, 110,  27,  -2  },
            {  -7,  91,  48,  -4  },
            {  -6,  70,  70,  -6  },
            {  -4,  48,  92,  -8  },
            {  -2,  27, 110,  -7  },
            {  -1,  10, 124,  -5  } },

            { {   0, 128,   0,   0  },  /* 4 */
            {  -6, 124,  11,  -1  },
            {  -8, 111,  28,  -3  },
            {  -8,  92,  49,  -5  },
            {  -7,  71,  71,  -7  },
            {  -5,  49,  92,  -8  },
            {  -3,  28, 111,  -8  },
            {  -1,  11, 124,  -6  } },

            { {  0,  128,   0,   0  },  /* 5 */
            {  -6, 123,  12,  -1  },
            {  -9, 111,  29,  -3  },
            {  -9,  93,  50,  -6  },
            {  -8,  72,  72,  -8  },
            {  -6,  50,  93,  -9  },
            {  -3,  29, 111,  -9  },
            {  -1,  12, 123,  -6  } },

            { {   0, 128,   0,   0  },  /* 6 */
            {  -7, 124,  12,  -1  },
            { -10, 111,  30,  -3  },
            { -10,  93,  51,  -6  },
            {  -9,  73,  73,  -9  },
            {  -6,  51,  93, -10  },
            {  -3,  30, 111, -10  },
            {  -1,  12, 124,  -7  } },

            { {   0, 128,   0,   0  },  /* 7 */
            {  -7, 123,  13,  -1  },
            { -11, 112,  31,  -4  },
            { -11,  94,  52,  -7  },
            { -10,  74,  74, -10  },
            {  -7,  52,  94, -11  },
            {  -4,  31, 112, -11  },
            {  -1,  13, 123,  -7  } },

            { {   0, 128,   0,  0  },  /* 8 */
            {  -8, 124,  13,  -1  },
            { -12, 112,  32,  -4  },
            { -12,  94,  53,  -7  },
            { -10,  74,  74, -10  },
            {  -7,  53,  94, -12  },
            {  -4,  32, 112, -12  },
            {  -1,  13, 124,  -8  } },

            { {   0, 128,   0,   0  },  /* 9 */
            {  -9, 124,  14,  -1  },
            { -13, 112,  33,  -4  },
            { -13,  95,  54,  -8  },
            { -11,  75,  75, -11  },
            {  -8,  54,  95, -13  },
            {  -4,  33, 112, -13  },
            {  -1,  14, 124,  -9  } },
            { {   0, 128,   0,   0  },  /* 10 */
            {  -9, 123,  15,  -1  },
            { -14, 113,  34,  -5  },
            { -14,  95,  55,  -8  },
            { -12,  76,  76, -12  },
            {  -8,  55,  95, -14  },
            {  -5,  34, 112, -13  },
            {  -1,  15, 123,  -9  } },

            { {   0, 128,   0,   0  },  /* 11 */
            { -10, 124,  15,  -1  },
            { -14, 113,  34,  -5  },
            { -15,  96,  56,  -9  },
            { -13,  77,  77, -13  },
            {  -9,  56,  96, -15  },
            {  -5,  34, 113, -14  },
            {  -1,  15, 124, -10  } },

            { {   0, 128,   0,   0  },  /* 12 */
            { -10, 123,  16,  -1  },
            { -15, 113,  35,  -5  },
            { -16,  98,  56, -10  },
            { -14,  78,  78, -14  },
            { -10,  56,  98, -16  },
            {  -5,  35, 113, -15  },
            {  -1,  16, 123, -10  } },

            { {   0, 128,   0,   0  },  /* 13 */
            { -11, 124,  17,  -2  },
            { -16, 113,  36,  -5  },
            { -17,  98,  57, -10  },
            { -14,  78,  78, -14  },
            { -10,  57,  98, -17  },
            {  -5,  36, 113, -16  },
            {  -2,  17, 124, -11  } },

            { {   0, 128,   0,   0  },  /* 14 */
            { -12, 125,  17,  -2  },
            { -17, 114,  37,  -6  },
            { -18,  99,  58, -11  },
            { -15,  79,  79, -15  },
            { -11,  58,  99, -18  },
            {  -6,  37, 114, -17  },
            {  -2,  17, 125, -12  } },

            { {   0, 128,   0,   0  },  /* 15 */   
            { -12, 124,  18,  -2  },
            { -18, 114,  38,  -6  },
            { -19,  99,  59, -11  },
            { -16,  80,  80, -16  },
            { -11,  59,  99, -19  },
            {  -6,  38, 114, -18  },
            {  -2,  18, 124, -12  } },
        };

        INT32 u4Tmp0, u4Tmp1, u4Tmp2, u4Tmp3;
        INT32 u4Val;//, i ,j;
        //     Tap0 : -4, -5, -6, -5, -3, -2, -1
        //     ==> 0x86C [28:24] = 4
        //     ==> 0x86C [20:16] = 5
        //     ==> 0x86C [12:8] = 6
        //     ==> 0x86C [4:0] = 5
        //     ==> 0x870 [28:24] = 3
        //     ==> 0x870 [20:16] = 2
        //     ==> 0x870 [12:8] = 1
        printk("<vdec> entry %s\n", __FUNCTION__);

        u4Tmp0 = (vp6_block_copy_filter[u4Select][1][0] * -1);
        u4Tmp1 = (vp6_block_copy_filter[u4Select][2][0] * -1);
        u4Tmp2 = (vp6_block_copy_filter[u4Select][3][0] * -1);
        u4Tmp3 = (vp6_block_copy_filter[u4Select][4][0] * -1);
        u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_VP6_COEF0_P1, u4Val);

        u4Tmp0 = (vp6_block_copy_filter[u4Select][5][0] * -1);
        u4Tmp1 = (vp6_block_copy_filter[u4Select][6][0] * -1);
        u4Tmp2 = (vp6_block_copy_filter[u4Select][7][0] * -1);
        u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8));
        vVDecWriteMC(u4VDecID, RW_VP6_COEF0_P2, u4Val);


        u4Tmp0 = (vp6_block_copy_filter[u4Select][1][1]);
        u4Tmp1 = (vp6_block_copy_filter[u4Select][2][1]);
        u4Tmp2 = (vp6_block_copy_filter[u4Select][3][1]);
        u4Tmp3 = (vp6_block_copy_filter[u4Select][4][1]);
        u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_VP6_COEF1_P1, u4Val);

        u4Tmp0 = (vp6_block_copy_filter[u4Select][5][1]);
        u4Tmp1 = (vp6_block_copy_filter[u4Select][6][1]);
        u4Tmp2 = (vp6_block_copy_filter[u4Select][7][1]);
        u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8));
        vVDecWriteMC(u4VDecID, RW_VP6_COEF1_P2, u4Val);

        u4Tmp0 = (vp6_block_copy_filter[u4Select][1][2]);
        u4Tmp1 = (vp6_block_copy_filter[u4Select][2][2]);
        u4Tmp2 = (vp6_block_copy_filter[u4Select][3][2]);
        u4Tmp3 = (vp6_block_copy_filter[u4Select][4][2]);
        u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_VP6_COEF2_P1, u4Val);

        u4Tmp0 = (vp6_block_copy_filter[u4Select][5][2]);
        u4Tmp1 = (vp6_block_copy_filter[u4Select][6][2]);
        u4Tmp2 = (vp6_block_copy_filter[u4Select][7][2]);
        u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8));
        vVDecWriteMC(u4VDecID, RW_VP6_COEF2_P2, u4Val);

        u4Tmp0 = (vp6_block_copy_filter[u4Select][1][3] * -1);
        u4Tmp1 = (vp6_block_copy_filter[u4Select][2][3] * -1);
        u4Tmp2 = (vp6_block_copy_filter[u4Select][3][3] * -1);
        u4Tmp3 = (vp6_block_copy_filter[u4Select][4][3] * -1);
        u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8) | (u4Tmp3));
        vVDecWriteMC(u4VDecID, RW_VP6_COEF3_P1, u4Val);

        u4Tmp0 = (vp6_block_copy_filter[u4Select][5][3] * -1);
        u4Tmp1 = (vp6_block_copy_filter[u4Select][6][3] * -1);
        u4Tmp2 = (vp6_block_copy_filter[u4Select][7][3] * -1);
        u4Val = ((u4Tmp0 << 24) | (u4Tmp1 << 16) | (u4Tmp2 << 8));
        vVDecWriteMC(u4VDecID, RW_VP6_COEF3_P2, u4Val);      

        return HAL_HANDLE_OK;
    }


    UINT32 u4VDEC_HAL_VP6_Write_SRAMData1(UINT32 u4BSID, UINT32 u4VDecID)
    {
        printk("<vdec> entry %s\n", __FUNCTION__);

        //---------- ctx = 0 ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50, 0x3a8, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x4501012c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x06010001, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0x00000000, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ctx = 0 ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50, 0x3a9, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x2a02072a, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x16030205, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ctx = 1 ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50, 0x3aa, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xe5010000, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x00010000, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ctx = 1 ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50, 0x3ab, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x08010800, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x00020100, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ctx = 2 ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50, 0x3ac, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x7a01012e, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x00010000, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ctx = 2 ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50, 0x3ad, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x23010622, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x00020101, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0x01000000, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- dct-sig model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  942, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xa280a480, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x0, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0x0, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- pdv model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  943, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xe192ac93, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xd6279ccc, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xaa77eb8c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0xe6e40000, u4BSID);
        //---------- fdv model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  944, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xf7d28744, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x8adceff6, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xf4b8c92c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0xadddeffd, u4BSID);
        //---------- dccv0 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  945, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xfe011c4a, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x5e806280, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xa680be00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- runv0 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  947, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xe8ded892, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xfaf6a98e, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0x82889595, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0xbff90000, u4BSID);
        //---------- dccv1 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  946, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xfe0150aa, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x80809e80, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xa680be00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- runv1 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  948, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xc8c9b59a, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x9892847e, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0x92a9b8f0, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0xf6fe0000, u4BSID);
        //---------- ract0 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  949, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xfefe50aa, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x80809e80, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xa680be00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract1 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  950, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01b650aa, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x80809e80, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xa680be00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract2 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  951, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01b6508c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x80809e80, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xea80be00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract3 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  952, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01b638ae, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x80b09e80, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xea80be00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract4 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  953, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01b638ae, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x80b0d080, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract5 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  954, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01b65ac6, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e0fec0, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract6 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  955, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xfefe5ac6, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e0fec0, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract7 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  956, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01ee5a72, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e09636, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract8 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  957, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01ee5a72, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e096ca, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract9 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  958, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01ee2472, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e0f060, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract10 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  959, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01ee24da, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e0f060, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract11 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  960, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x01ee24da, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e0f060, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract12 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  961, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x98bc66da, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e05c60, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract13 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  962, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xacd266da, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e05c60, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract14 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  963, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xac866674, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50e05c60, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract15 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  964, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x7a3e66b0, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x50945c60, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract16 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  965, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x7a3e66b0, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract17 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  966, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xae6aaef8, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract18 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  967, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xf0e6aef8, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract19 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  968, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xa8e6ae48, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract20 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  969, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xa87aae48, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract21 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  970, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xa87aae48, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract22 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  971, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xa87aae48, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract23 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  972, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xa87aae48, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0xa294e860, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xeaecbe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract24 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  973, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x4e7a326e, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x6a946e60, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xea96be00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract25 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  974, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x4e7a324c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x6a946e60, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xb696be00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract26 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  975, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x3c1a1c4c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x6a946e60, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xde96fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract27 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  976, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x3c0a1c66, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x6a94967c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xfa96fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract28 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  977, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x521e1c82, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x6ac2c07c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract29 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  978, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x8e361ca4, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x40eaee7c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract30 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  979, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0xd6ae5aa4, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x40eaee7c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract31 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  980, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x704c1c54, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract32 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  981, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x704c1c54, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract33 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  982, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x70081c94, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract34 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  983, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x9c281c94, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        //---------- ract35 model initial value ----------
        vVDecWriteVP6VLD(u4VDecID, 4*50,  984, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*51, 0x9c281c94, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*52, 0x40ea9e7c, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*53, 0xfae4fe00, u4BSID);
        vVDecWriteVP6VLD(u4VDecID, 4*54, 0x0, u4BSID);
        return 0;
    }

    UINT32 u4VDEC_HAL_VP6_Read_SRAMData1(UINT32 u4BSID, UINT32 u4VDecID)
    {
        //read start from 3A8~3D8
        UINT32 u4Start = 936;
        UINT32 i = u4Start;
        UINT32 j = 0;
        UINT32 u4Reg76 [50] = {0};
        UINT32 u4Reg77 [50] = {0};
        UINT32 u4Reg78 [50] = {0};
        UINT32 u4Reg79 [50] = {0};
        printk("<vdec> entry %s\n", __FUNCTION__);
        for ( i = 936; i <= 984; i++)
        {
            //RISCWrite(`VP6_ADDR + 4*75, 32'h3a8);
            //RISCRead(`VP6_ADDR + 4*76);
            //RISCRead(`VP6_ADDR + 4*77);
            //RISCRead(`VP6_ADDR + 4*78);
            //RISCRead(`VP6_ADDR + 4*79);
            vVDecWriteVP6VLD(u4VDecID, 0x12C, i, u4BSID);

            u4Reg76[j] = u4VDecReadVP6VLD(u4VDecID, 0x130);
            u4Reg77[j] = u4VDecReadVP6VLD(u4VDecID, 0x134);
            u4Reg78[j] = u4VDecReadVP6VLD(u4VDecID, 0x138);
            u4Reg79[j] = u4VDecReadVP6VLD(u4VDecID, 0x13C);         
            j++;
        }
        return 0;
    }

    UINT32 u4VDEC_HAL_VP6_VDec_ReadFinishFlag(UINT32 u4VDecID)
    {
        return u4VDecReadVP6VLD(u4VDecID, RO_VP6_PIC_DEC_END);
    }

    UINT32 u4VDEC_HAL_VP6_VDec_SetByteCount(UINT32 u4BSID, UINT32 u4VDecID, UINT32 u4Ofst)
    {
        if(u4BSID == 0)
        {
            vVDecWriteVP6VLD(u4VDecID, RW_VP6_PAD_ZERO_MODE, 1, u4BSID);
            vVDecWriteVP6VLD(u4VDecID, RW_VP6_PIC_BYTE_CNT, u4Ofst, u4BSID);
        }
        else
        {
            vVDecWriteVP6VLD(u4VDecID, RW_VP6_PAD_ZERO_MODE2, 1, u4BSID);
            vVDecWriteVP6VLD(u4VDecID, RW_VP6_PIC_BYTE_CNT2, u4Ofst, u4BSID);
        }
        return HAL_HANDLE_OK;
    }

    UINT32 u4VDEC_HAL_VP6_VDec_SetWorkspace(UINT32 u4VDecID, UINT32 u4VLDWrapperWorkspace, UINT32 u4PPWrapperWorkspace)
    {
#if 0
        vVDecWriteVP6MC(u4VDecID, RW_MC_VLD_WRAPPER,PHYSICAL(u4VLDWrapperWorkspace));
        vVDecWriteVP6MC(u4VDecID, RW_MC_PP_WRAPPER,PHYSICAL(u4PPWrapperWorkspace));
#else
        vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER,PHYSICAL(u4VLDWrapperWorkspace));
        vVDecWriteMC(u4VDecID, RW_MC_PP_WRAPPER,PHYSICAL(u4PPWrapperWorkspace));
#endif

    }

    UINT32 u4VDec_HAL_VP6_VDec_BackupSram(UINT32 u4VDecID, VDEC_INFO_VP6_DEC_PRM_T *prDecPrm)
    {
        UINT32 i, j, u4Cnt, u4Addr, *pBuf;

        printk("<vdec> entry %s\n", __FUNCTION__);

        if (!prDecPrm)
        {
            return INVLAID_HANDLE_VALUE;
        }

        // backup VLD wrapper
        pBuf = &(prDecPrm->au4VldWrapper[0]);

        u4Cnt = 0;
        for (i = 0; i < (VLD_WRAPPER_AC_BASE_END - VLD_WRAPPER_UPD_MP_TYPE + 1); i++)
        {
            for (j = 0; j < 4; j++)
            {
                u4Addr = MC_VLD_WRAPPER_READ | ((VLD_WRAPPER_UPD_MP_TYPE + i) << 2) | (3 - j);
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, u4Addr);
                pBuf[u4Cnt] = u4VDecReadMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA);
                u4Cnt++;
            }
        }

        // backup reorder coeff
        pBuf = &(prDecPrm->au4Reorder[0]);

        for (i = 0; i < 16; i++)
        {
            //vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, (SCL_READ | (i * 4)));
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, ((1 << 8) | (i * 4)));
            pBuf[i] = u4VDecReadVLD(u4VDecID, RW_VLD_SCL_DATA);
        }

        return HAL_HANDLE_OK;
    }

    UINT32 u4VDec_HAL_VP6_VDec_RestoreSram(UINT32 u4VDecID, VDEC_INFO_VP6_DEC_PRM_T *prDecPrm)
    {
        UINT32 i, j, u4Cnt, u4Addr, *pBuf;

        printk("<vdec> entry %s\n", __FUNCTION__);

        if (!prDecPrm)
        {
            return INVLAID_HANDLE_VALUE;
        }

        // restore VLD wrapper
        pBuf = &(prDecPrm->au4VldWrapper[0]);

        u4Cnt = 0;
        for (i = 0; i < (VLD_WRAPPER_AC_BASE_END - VLD_WRAPPER_UPD_MP_TYPE + 1); i++)
        {
            for (j = 0; j < 4; j++)
            {
                u4Addr = MC_VLD_WRAPPER_WRITE | ((VLD_WRAPPER_UPD_MP_TYPE + i) << 2) | (3 - j);
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_ADDR, u4Addr);
                vVDecWriteMC(u4VDecID, RW_MC_VLD_WRAPPER_DATA, pBuf[u4Cnt]);
                u4Cnt++;
            }
        }

        // restore reorder coeff
        vVDecWriteVP6VLD(u4VDecID, RW_VP6_PCI_PAR, RW_VP6_FLAG, 0);

        pBuf = &(prDecPrm->au4Reorder[0]);

        for (i = 0; i < 16; i++) 
        {
            //vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, (SCL_WRITE | (i * 4)));
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_ADDR, ((0 << 8) | (i * 4)));
            vVDecWriteVLD(u4VDecID, RW_VLD_SCL_DATA, pBuf[i]);
        }

        return HAL_HANDLE_OK;
    }


#if CONFIG_DRV_VERIFY_SUPPORT
    void vVDEC_HAL_VP6_VDec_ReadCheckSum(UINT32 u4VDecID, UINT32 *pu4DecCheckSum)
    {
        UINT32  u4Temp;

        u4Temp = 0;
        // DCAC
        *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x3AC);
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x3B0);    
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x3B4);    
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x3B8);        
        pu4DecCheckSum ++;
        u4Temp ++;
        // VLD
        *pu4DecCheckSum = u4VDecReadVLD(u4VDecID, 0x2F4);        
        pu4DecCheckSum ++;
        u4Temp ++;
        // MC
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5E8);
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5EC);    
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5F0);    
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5F4);        
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5F8);
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x5FC);    
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x600);    
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x604);        
        pu4DecCheckSum ++;
        u4Temp ++;
        // PP
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x608);    
        pu4DecCheckSum ++;
        u4Temp ++;
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x60C);        
        pu4DecCheckSum ++;
        u4Temp ++;       
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x610);        
        pu4DecCheckSum ++;
        u4Temp ++; 
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x614);        
        pu4DecCheckSum ++;
        u4Temp ++; 
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x618);        
        pu4DecCheckSum ++;
        u4Temp ++; 
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x61C);        
        pu4DecCheckSum ++;
        u4Temp ++; 
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x620);        
        pu4DecCheckSum ++;
        u4Temp ++; 
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x624);        
        pu4DecCheckSum ++;
        u4Temp ++; 
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x628);        
        pu4DecCheckSum ++;
        u4Temp ++; 
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x62C);        
        pu4DecCheckSum ++;
        u4Temp ++; 
        *pu4DecCheckSum = u4VDecReadMC(u4VDecID, 0x630);        
        pu4DecCheckSum ++;
        u4Temp ++; 

        while(u4Temp < MAX_CHKSUM_NUM)
        {
            *pu4DecCheckSum = 0;            
            pu4DecCheckSum ++;   
            u4Temp ++;
        }  
    }

    BOOL fgVDEC_HAL_VP6_VDec_CompCheckSum(UINT32 *pu4DecCheckSum, UINT32 *pu4GoldenCheckSum)
    {
        UINT32 i;
        // DCAC
        for (i = 0; i < 4; i ++)
        {
            if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
            {
                vVDecOutputDebugString("\n!!!!!!!!! DCAC Check Sum Compare Error  !!!!!!\n");
                return (FALSE);
            }
            pu4GoldenCheckSum ++;
            pu4DecCheckSum ++;
        }
        // VLD
        if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
        {
            vVDecOutputDebugString("\n!!!!!!!!! VLD Check Sum Compare Error  !!!!!!\n");
            return (FALSE);
        }
        pu4GoldenCheckSum ++;
        pu4DecCheckSum ++;
        // MC
        for (i = 0; i < 8; i ++)
        {
            if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
            {
                vVDecOutputDebugString("\n!!!!!!!!! MC Check Sum Compare Error  !!!!!!\n");
                return (FALSE);
            }
            pu4GoldenCheckSum ++;
            pu4DecCheckSum ++;
        }
        // PP
        for (i = 0; i < 2; i ++)
        {
            if((*pu4GoldenCheckSum) != (*pu4DecCheckSum))
            {
                vVDecOutputDebugString("\n!!!!!!!!! MC Check Sum Compare Error  !!!!!!\n");
                return (FALSE);
            }
            pu4GoldenCheckSum ++;
            pu4DecCheckSum ++;
        }

        return (TRUE);
    }

#endif

