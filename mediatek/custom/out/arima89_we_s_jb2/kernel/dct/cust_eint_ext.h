
#ifndef __CUST_EINT_EXTH
#define __CUST_EINT_EXTH

#ifdef __cplusplus
extern "C" {
#endif

#define CUST_EINT_POLARITY_LOW              0
#define CUST_EINT_POLARITY_HIGH             1
#define CUST_EINT_DEBOUNCE_DISABLE          0
#define CUST_EINT_DEBOUNCE_ENABLE           1
#define CUST_EINT_EDGE_SENSITIVE            0
#define CUST_EINT_LEVEL_SENSITIVE           1

//////////////////////////////////////////////////////////////////////////////


#define CUST_EINT_EXT_NFC_NUM              195
#define CUST_EINT_EXT_NFC_DEBOUNCE_CN      0
#define CUST_EINT_EXT_NFC_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_EXT_NFC_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_EXT_NFC_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE

#define CUST_EINT_EXT_CHG_STAT_NUM              212
#define CUST_EINT_EXT_CHG_STAT_DEBOUNCE_CN      0
#define CUST_EINT_EXT_CHG_STAT_POLARITY         CUST_EINT_POLARITY_LOW
#define CUST_EINT_EXT_CHG_STAT_SENSITIVE        CUST_EINT_LEVEL_SENSITIVE
#define CUST_EINT_EXT_CHG_STAT_DEBOUNCE_EN      CUST_EINT_DEBOUNCE_DISABLE



//////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
}

#endif
#endif //_CUST_EINT__EXT_H


