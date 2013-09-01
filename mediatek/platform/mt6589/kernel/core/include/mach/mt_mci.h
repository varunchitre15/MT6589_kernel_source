#ifndef __MCI_H__
#define __MCI_H__

/*
 * Define hardware registers.
 */
#define MCI_CON_OVRD        (MCI_BASE + 0x0000) 
#define MCI_SFETCH_CON      (MCI_BASE + 0x0004) 
#define MCI_SEC_CON         (MCI_BASE + 0x0008) 
#define MCI_ERR_REG         (MCI_BASE + 0x0010) 
#define MCI_PMCR            (MCI_BASE + 0x0014) 
#define MCI_SCR_S0          (MCI_BASE + 0x0100) 
#define MCI_SOR_S0          (MCI_BASE + 0x0104) 
#define MCI_RQOS_OVRD_S0    (MCI_BASE + 0x0108) 
#define MCI_WQOS_OVRD_S0    (MCI_BASE + 0x010C) 
#define MCI_SCR_S1          (MCI_BASE + 0x0200) 
#define MCI_SOR_S1          (MCI_BASE + 0x0204) 
#define MCI_RQOS_OVRD_S1    (MCI_BASE + 0x0208) 
#define MCI_WQOS_OVRD_S1    (MCI_BASE + 0x020C) 
#define MCI_CCR             (MCI_BASE + 0x0300) 
#define MCI_CCR_CON         (MCI_BASE + 0x0304) 
#define MCI_CCR_OVFL        (MCI_BASE + 0x0308) 
#define MCI_EVENT0_SEL      (MCI_BASE + 0x0400) 
#define MCI_EVENT0_CNT      (MCI_BASE + 0x0404) 
#define MCI_EVENT0_CON      (MCI_BASE + 0x0408) 
#define MCI_EVENT0_OVFL     (MCI_BASE + 0x040C) 
#define MCI_EVENT1_SEL      (MCI_BASE + 0x0500) 
#define MCI_EVENT1_CNT      (MCI_BASE + 0x0504) 
#define MCI_EVENT1_CON      (MCI_BASE + 0x0508) 
#define MCI_EVENT1_OVFL     (MCI_BASE + 0x050C) 



/*
 * Define constants.
 */


/*
 * Define function prototypes.
 */
   #if defined(CONFIG_MTK_MCI)
   void mci_snoop_sleep();
   void mci_snoop_restore();
   void mci_restore();
   void mci_sleep();
   #else
   #define mci_snoop_sleep() do {} while(0)
   #define mci_snoop_restore() do {} while(0)
   #define mci_sleep() do {} while(0)
   #define mci_restore() do {} while(0)
   #endif//CONFIG_MTK_MCI
#endif  /*!__MCI_H__ */
