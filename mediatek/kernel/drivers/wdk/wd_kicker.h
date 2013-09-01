#ifndef _WD_KICKER_H_
#define _WD_KICKER_H_

enum wk_wdt_mode {
	WK_WDT_NORMAL_MODE,
	WK_WDT_EXP_MODE,
};

enum wk_wdt_en {
	WK_WDT_DIS,
	WK_WDT_EN,
};


enum wk_wdt_type {
	WK_WDT_LOC_TYPE,
	WK_WDT_EXT_TYPE,
	WK_WDT_LOC_TYPE_NOLOCK,
	WK_WDT_EXT_TYPE_NOLOCK,
	
};

struct wk_wdt {
	int (*config)(enum wk_wdt_type, enum wk_wdt_mode, int timeout);
	void (*kick_wdt)(enum wk_wdt_type);
	int (*enable)(enum wk_wdt_en);
};

int wk_register_wdt(struct wk_wdt *wk_wdt);
unsigned int get_check_bit(void);
unsigned int get_kick_bit(void);



#endif
