
#include <linux/kernel.h>
#include <asm/io.h>

#include <mach/mt_reg_base.h>
#include <mach/mt_emi_bm.h>
#include <mach/sync_write.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_mci.h>

static unsigned char g_cBWL;

void BM_Init(void)
{
    g_cBWL = 0;

    /*
    * make sure BW limiter counts consumed Soft-mode bandwidth of each master
    */
    if (readl(EMI_ARBA) & 0x00008000) {
        g_cBWL |= 1 << 0;
        writel(readl(EMI_ARBA) & ~0x00008000, EMI_ARBA);
    }

    if (readl(EMI_ARBC) & 0x00008000) {
        g_cBWL |= 1 << 2;
        writel(readl(EMI_ARBC) & ~0x00008000, EMI_ARBC);
    }

    if (readl(EMI_ARBD) & 0x00008000) {
        g_cBWL |= 1 << 3;
        writel(readl(EMI_ARBD) & ~0x00008000, EMI_ARBD);
    }

    if (readl(EMI_ARBE) & 0x00008000) {
        g_cBWL |= 1 << 4;
        writel(readl(EMI_ARBE) & ~0x00008000, EMI_ARBE);
    }

    if (readl(EMI_ARBF) & 0x00008000) {
        g_cBWL |= 1 << 5;
        writel(readl(EMI_ARBF) & ~0x00008000, EMI_ARBF);
    }

    if (readl(EMI_ARBG) & 0x00008000) {
        g_cBWL |= 1 << 6;
        writel(readl(EMI_ARBG) & ~0x00008000, EMI_ARBG);
    }
}

void BM_DeInit(void)
{
    if (g_cBWL & (1 << 0)) {
        g_cBWL &= ~(1 << 0);
        writel(readl(EMI_ARBA) | 0x00008000, EMI_ARBA);
    }

    if (g_cBWL & (1 << 2)) {
        g_cBWL &= ~(1 << 2);
        writel(readl(EMI_ARBC) | 0x00008000, EMI_ARBC);
    }

    if (g_cBWL & (1 << 3)) {
        g_cBWL &= ~(1 << 3);
        writel(readl(EMI_ARBD) | 0x00008000, EMI_ARBD);
    }

    if (g_cBWL & (1 << 4)) {
        g_cBWL &= ~(1 << 4);
        writel(readl(EMI_ARBE) | 0x00008000, EMI_ARBE);
    }

    if (g_cBWL & (1 << 5)) {
        g_cBWL &= ~(1 << 5);
        writel(readl(EMI_ARBF) | 0x00008000, EMI_ARBF);
    }

    if (g_cBWL & (1 << 6)) {
        g_cBWL &= ~(1 << 6);
        writel(readl(EMI_ARBG) | 0x00008000, EMI_ARBG);
    }
}

void BM_Enable(const unsigned int enable)
{
    const unsigned int value = readl(EMI_BMEN);

    mt65xx_reg_sync_writel((value & ~(BUS_MON_PAUSE | BUS_MON_EN)) | (enable ? BUS_MON_EN : 0), EMI_BMEN);
}

/*
void BM_Disable(void)
{
    const unsigned int value = readl(EMI_BMEN);

    mt65xx_reg_sync_writel(value & (~BUS_MON_EN), EMI_BMEN);
}
*/

void BM_Pause(void)
{
    const unsigned int value = readl(EMI_BMEN);

    mt65xx_reg_sync_writel(value | BUS_MON_PAUSE, EMI_BMEN);
}

void BM_Continue(void)
{
    const unsigned int value = readl(EMI_BMEN);

    mt65xx_reg_sync_writel(value & (~BUS_MON_PAUSE), EMI_BMEN);
}

unsigned int BM_IsOverrun(void)
{
    /*
    * return 0 if EMI_BCNT(bus cycle counts) or EMI_WACT(total word counts) is overrun,
    * otherwise return an !0 value
    */
    const unsigned int value = readl(EMI_BMEN);

    return (value & BC_OVERRUN);
}

void BM_SetReadWriteType(const unsigned int ReadWriteType)
{
    const unsigned int value = readl(EMI_BMEN);

    /*
    * ReadWriteType: 00/11 --> both R/W
    *                   01 --> only R
    *                   10 --> only W
    */
    mt65xx_reg_sync_writel((value & 0xFFFFFFCF) | (ReadWriteType << 4), EMI_BMEN);
}

int BM_GetBusCycCount(void)
{
    return BM_IsOverrun() ? BM_ERR_OVERRUN : readl(EMI_BCNT);
}

unsigned int BM_GetTransAllCount(void)
{    
    return readl(EMI_TACT);
}

int BM_GetTransCount(const unsigned int counter_num)
{
    unsigned int iCount;

    switch (counter_num) {
    case 1:
        iCount = readl(EMI_TSCT);
        break;

    case 2:
        iCount = readl(EMI_TSCT2);
        break;

    case 3:
        iCount = readl(EMI_TSCT3);
        break;

    default:
        return BM_ERR_WRONG_REQ;
    }

    return iCount;
}

int BM_GetWordAllCount(void)
{
    return BM_IsOverrun() ? BM_ERR_OVERRUN : readl(EMI_WACT);
}

int BM_GetWordCount(const unsigned int counter_num)
{
    unsigned int iCount;

    switch (counter_num) {
    case 1:
        iCount = readl(EMI_WSCT);
        break;

    case 2:
        iCount = readl(EMI_WSCT2);
        break;

    case 3:
        iCount = readl(EMI_WSCT3);
        break;

    case 4:
        iCount = readl(EMI_WSCT4);
        break;

    default:
        return BM_ERR_WRONG_REQ;
    }

    return iCount;
}

unsigned int BM_GetBandwidthWordCount(void)
{
    return readl(EMI_BACT);
}

unsigned int BM_GetOverheadWordCount(void)
{
    return readl(EMI_BSCT);
}

int BM_GetTransTypeCount(const unsigned int counter_num)
{
    return (counter_num < 1 || counter_num > BM_COUNTER_MAX) ? BM_ERR_WRONG_REQ : readl(EMI_TTYPE1 + (counter_num - 1) * 8);
}

int BM_SetMonitorCounter(const unsigned int counter_num, const unsigned int master, const unsigned int trans_type)
{
    unsigned int value, addr;
    const unsigned int iMask = 0xFF7F;

    if (counter_num < 1 || counter_num > BM_COUNTER_MAX) {
        return BM_ERR_WRONG_REQ;
    }

    if (counter_num == 1) {
        addr = EMI_BMEN;
        value = (readl(addr) & ~(iMask << 16)) | ((trans_type & 0xFF) << 24) | ((master & 0x7F) << 16);
    }else {
        addr = (counter_num <= 3) ? EMI_MSEL : (EMI_MSEL2 + (counter_num / 2 - 2) * 8);

        // clear master and transaction type fields
        value = readl(addr) & ~(iMask << ((counter_num % 2) * 16));

        // set master and transaction type fields
        value |= (((trans_type & 0xFF) << 8) | (master & 0x7F)) << ((counter_num % 2) * 16);
    }

    mt65xx_reg_sync_writel(value, addr);

    return BM_REQ_OK;
}

int BM_SetMaster(const unsigned int counter_num, const unsigned int master)
{
    unsigned int value, addr;
    const unsigned int iMask = 0x7F;

    if (counter_num < 1 || counter_num > BM_COUNTER_MAX) {
        return BM_ERR_WRONG_REQ;
    }

    if (counter_num == 1) {
        addr = EMI_BMEN;
        value = (readl(addr) & ~(iMask << 16)) | ((master & iMask) << 16);
    }else {
        addr = (counter_num <= 3) ? EMI_MSEL : (EMI_MSEL2 + (counter_num / 2 - 2) * 8);

        // clear master and transaction type fields
        value = readl(addr) & ~(iMask << ((counter_num % 2) * 16));

        // set master and transaction type fields
        value |= ((master & iMask) << ((counter_num % 2) * 16));
    }

    mt65xx_reg_sync_writel(value, addr);

    return BM_REQ_OK;
}

int BM_SetIDSelect(const unsigned int counter_num, const unsigned int id, const unsigned int enable)
{
    unsigned int value, addr, shift_num;

    if ((counter_num < 1 || counter_num > BM_COUNTER_MAX)
        || (id > 0xFF)
        || (enable > 1)) {
        return BM_ERR_WRONG_REQ;
    }

    addr = EMI_BMID0 + (counter_num - 1) / 4 * 8;

    // field's offset in the target EMI_BMIDx register
    shift_num = ((counter_num - 1) % 4) * 8;

    // clear SELx_ID field
    value = readl(addr) & ~(0xFF << shift_num);

    // set SELx_ID field
    value |= id << shift_num;

    mt65xx_reg_sync_writel(value, addr);

    value = (readl(EMI_BMEN2) & ~(1 << (counter_num - 1))) | (enable << (counter_num - 1));

    return BM_REQ_OK;
}

int BM_SetUltraHighFilter(const unsigned int counter_num, const unsigned int enable)
{
    unsigned int value;

    if ((counter_num < 1 || counter_num > BM_COUNTER_MAX)
        || (enable > 1)) {
        return BM_ERR_WRONG_REQ;
    }

    value = (readl(EMI_BMEN1) & ~(1 << (counter_num - 1))) | (enable << (counter_num - 1));

    mt65xx_reg_sync_writel(value, EMI_BMEN1);

    return BM_REQ_OK;
}

int BM_SetLatencyCounter(void)
{
  unsigned int value;
  value = readl(EMI_BMEN2) & ~(0b11 << 24);
  //emi_ttype1 -- emi_ttype7 change as total latencies for m0 -- m6, and emi_ttype9 -- emi_ttype15 change as total transaction counts for m0 -- m6
  value |= (0b10 << 24);
  mt65xx_reg_sync_writel(value, EMI_BMEN2);
  return BM_REQ_OK;
}

int BM_GetLatencyCycle(const unsigned int counter_num)
{
  unsigned int cycle_count;
  
  switch(counter_num)
  {
    case 1:
      cycle_count = readl(EMI_TTYPE1);
      break;
    case 3:
      cycle_count = readl(EMI_TTYPE3);
      break;  
    case 4:
      cycle_count = readl(EMI_TTYPE4);
      break;
    case 5:
      cycle_count = readl(EMI_TTYPE5);
      break;
    case 6:
      cycle_count = readl(EMI_TTYPE6);
      break;      
    case 7:
      cycle_count = readl(EMI_TTYPE7);
      break;  
    case 9:
      cycle_count = readl(EMI_TTYPE9);
      break;
    case 11:
      cycle_count = readl(EMI_TTYPE11);
      break;
    case 12:
      cycle_count = readl(EMI_TTYPE12);
      break;
    case 13:
      cycle_count = readl(EMI_TTYPE13);
      break;
    case 14:
      cycle_count = readl(EMI_TTYPE14);
      break;
    case 15:
      cycle_count = readl(EMI_TTYPE15);
      break;          
    default:
      return BM_ERR_WRONG_REQ;  
  }
  return cycle_count;
}

unsigned int DRAMC_GetPageHitCount(DRAMC_Cnt_Type CountType)
{
    unsigned int iCount;

    switch (CountType) {
    case DRAMC_R2R:
        iCount = readl(DRAMC_R2R_PAGE_HIT);
        break;

    case DRAMC_R2W:
        iCount = readl(DRAMC_R2W_PAGE_HIT);
        break;

    case DRAMC_W2R:
        iCount = readl(DRAMC_W2R_PAGE_HIT);
        break;

    case DRAMC_W2W:
        iCount = readl(DRAMC_W2W_PAGE_HIT);
        break;
    case DRAMC_ALL:
        iCount = readl(DRAMC_R2R_PAGE_HIT) + readl(DRAMC_R2W_PAGE_HIT) +
                 readl(DRAMC_W2R_PAGE_HIT) + readl(DRAMC_W2W_PAGE_HIT);
        break;
    default:
        return BM_ERR_WRONG_REQ;
    }

    return iCount;
}

unsigned int DRAMC_GetPageMissCount(DRAMC_Cnt_Type CountType)
{
    unsigned int iCount;

    switch (CountType) {
    case DRAMC_R2R:
        iCount = readl(DRAMC_R2R_PAGE_MISS);
        break;

    case DRAMC_R2W:
        iCount = readl(DRAMC_R2W_PAGE_MISS);
        break;

    case DRAMC_W2R:
        iCount = readl(DRAMC_W2R_PAGE_MISS);
        break;

    case DRAMC_W2W:
        iCount = readl(DRAMC_W2W_PAGE_MISS);
        break;
    case DRAMC_ALL:
        iCount = readl(DRAMC_R2R_PAGE_MISS) + readl(DRAMC_R2W_PAGE_MISS) +
                 readl(DRAMC_W2R_PAGE_MISS) + readl(DRAMC_W2W_PAGE_MISS);
        break;
    default:
        return BM_ERR_WRONG_REQ;
    }

    return iCount;
}

unsigned int DRAMC_GetInterbankCount(DRAMC_Cnt_Type CountType)
{
    unsigned int iCount;

    switch (CountType) {
    case DRAMC_R2R:
        iCount = readl(DRAMC_R2R_INTERBANK);
        break;

    case DRAMC_R2W:
        iCount = readl(DRAMC_R2W_INTERBANK);
        break;

    case DRAMC_W2R:
        iCount = readl(DRAMC_W2R_INTERBANK);
        break;

    case DRAMC_W2W:
        iCount = readl(DRAMC_W2W_INTERBANK);
        break;
    case DRAMC_ALL:
        iCount = readl(DRAMC_R2R_INTERBANK) + readl(DRAMC_R2W_INTERBANK) +
                 readl(DRAMC_W2R_INTERBANK) + readl(DRAMC_W2W_INTERBANK);
        break;
    default:
        return BM_ERR_WRONG_REQ;
    }

    return iCount;
}

unsigned int DRAMC_GetIdleCount(void)
{
    return readl(DRAMC_IDLE_COUNT);
}

void MCI_Mon_Enable(void)
{
  unsigned int mci_pmcr;
  unsigned int mci_event0_con;
  unsigned int mci_event1_con;
  
  mci_pmcr = readl(MCI_PMCR);  
  mci_pmcr |= 0x2; /*reset all counters to zero*/
  mt65xx_reg_sync_writel(mci_pmcr, MCI_PMCR);
  
  mci_pmcr = readl(MCI_PMCR);  
  mci_pmcr |= 0x1; /*enable all counters*/
  mt65xx_reg_sync_writel(mci_pmcr, MCI_PMCR);
  
  mci_event0_con = readl(MCI_EVENT0_CON);
  mci_event0_con |= 0x1; /*enable event0 counter*/
  mt65xx_reg_sync_writel(mci_event0_con, MCI_EVENT0_CON);
  
  mci_event1_con = readl(MCI_EVENT1_CON);
  mci_event1_con |= 0x1; /*enable event1 counter*/
  mt65xx_reg_sync_writel(mci_event1_con, MCI_EVENT1_CON);
}

void MCI_Mon_Disable(void)
{
  unsigned int mci_pmcr;
  unsigned int mci_event0_con;
  unsigned int mci_event1_con;        
  
  mci_event0_con = readl(MCI_EVENT0_CON);
  mci_event0_con &= ~0x1; /*enable event0 counter*/
  mt65xx_reg_sync_writel(mci_event0_con, MCI_EVENT0_CON);
  
  mci_event1_con = readl(MCI_EVENT1_CON);
  mci_event1_con &= ~0x1; /*enable event1 counter*/
  mt65xx_reg_sync_writel(mci_event1_con, MCI_EVENT1_CON);
  
  mci_pmcr = readl(MCI_PMCR);  
  mci_pmcr &= ~0x1; /*enable all counters*/
  mt65xx_reg_sync_writel(mci_pmcr, MCI_PMCR);
  
  mci_pmcr = readl(MCI_PMCR);  
  mci_pmcr &= ~0x2; /*reset all counters to zero*/
  mt65xx_reg_sync_writel(mci_pmcr, MCI_PMCR);
}

void MCI_Event_Set(unsigned int evt0, unsigned int evt1)
{
  writel(evt0, MCI_EVENT0_SEL);
  writel(evt1, MCI_EVENT1_SEL);
}


void MCI_Event_Read()
{
  printk("MCI_EVENT0 = %x\n", readl(MCI_EVENT0_SEL));
  printk("MCI_EVENT1 = %x\n", readl(MCI_EVENT1_SEL));
}

unsigned int MCI_GetEventCount(int evt_counter)
{
  unsigned int iCount;
  
  switch(evt_counter){
    case 0:
      iCount = readl(MCI_EVENT0_CNT);
      break;
    case 1:
      iCount = readl(MCI_EVENT1_CNT);
      break;
    default:
      return BM_ERR_WRONG_REQ;
  }  
  
  return iCount;
}
