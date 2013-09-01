#ifndef FTM_CUST_MCARD_H
#define FTM_CUST_MCARD_H
	 
#ifdef MTK_EMMC_SUPPORT
#define CUST_EMMC_ID		(0) /*eMMC MSDC0*/
#endif

#ifdef MTK_MULTI_STORAGE_SUPPORT
#define CUST_MCARD_ID      (0)   /* MSDC0  (SD Card 1) */
								
#define CUST_MCARD_ID2		(1)    /*MSDC1 (SD Card 2)*/
#else
#define CUST_MCARD_ID      (0)   /* MSDC1*/
#endif
	 
#endif /* FTM_CUST_MCARD_H */


