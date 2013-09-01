
#ifndef _CUST_KPD_H_
#define _CUST_KPD_H_
#include <linux/input.h>
//#include <cust_eint.h>
#define KPD_YES		1
#define KPD_NO		0
/* available keys (Linux keycodes) */
#define KEY_CALL	KEY_SEND
#define KEY_ENDCALL	KEY_END
#undef KEY_OK
#define KEY_OK		KEY_REPLY	/* DPAD_CENTER */
#define KEY_FOCUS	KEY_HP
#define KEY_AT		KEY_EMAIL
#define KEY_POUND	228	//KEY_KBDILLUMTOGGLE
#define KEY_STAR	227	//KEY_SWITCHVIDEOMODE
#define KEY_DEL 	KEY_BACKSPACE
#define KEY_SYM		KEY_COMPOSE
/* KEY_HOME */
/* KEY_BACK */
/* KEY_VOLUMEDOWN */
/* KEY_VOLUMEUP */
/* KEY_MUTE */
/* KEY_MENU */
/* KEY_UP */
/* KEY_DOWN */
/* KEY_LEFT */
/* KEY_RIGHT */
/* KEY_CAMERA */
/* KEY_POWER */
/* KEY_TAB */
/* KEY_ENTER */
/* KEY_LEFTSHIFT */
/* KEY_COMMA */
/* KEY_DOT */		/* PERIOD */
/* KEY_SLASH */
/* KEY_LEFTALT */
/* KEY_RIGHTALT */
/* KEY_SPACE */
/* KEY_SEARCH */
/* KEY_0 ~ KEY_9 */
/* KEY_A ~ KEY_Z */
/*
* Power key's HW keycodes are 8, 17, 26, 35, 44, 53, 62, 71.  Only [8] works
* for Power key in Keypad driver, so we set KEY_ENDCALL in [8] because
* EndCall key is Power key in Android.  If KPD_PWRKEY_USE_EINT is YES, these
* eight keycodes will not work for Power key.
*/


#define KPD_KEY_DEBOUNCE  1024      /* (val / 32) ms */
#define KPD_PWRKEY_MAP    KEY_ENDCALL

#define KPD_PWRKEY_USE_EINT       KPD_NO
#define KPD_PWRKEY_EINT           CUST_EINT_KPD_PWRKEY_NUM
#define KPD_PWRKEY_DEBOUNCE       CUST_EINT_KPD_PWRKEY_DEBOUNCE_CN
#define KPD_PWRKEY_POLARITY       CUST_EINT_KPD_PWRKEY_POLARITY
#define KPD_PWRKEY_SENSITIVE      CUST_EINT_KPD_PWRKEY_SENSITIVE

/* HW keycode [0 ~ 71] -> Linux keycode */
#define KPD_INIT_KEYMAP()	\
{	\
	[0] = KEY_Z,		\
	[1] = KEY_X,		\
	[2] = KEY_C,		\
	[3] = KEY_V,		\
	[4] = KEY_B,		\
	[5] = KEY_N,		\
	[6] = KEY_M,		\
	[7] = KEY_AT,		\
	[9] = KEY_O,		\
	[10] = KEY_P,		\
	[11] = KEY_L,		\
	[12] = KEY_DEL,		\
	[13] = KEY_OK,		\
	[14] = KEY_SPACE,		\
	[15] = KEY_TAB,		\
	[16] = KEY_SYM,		\
	[18] = KEY_1,		\
	[19] = KEY_2,		\
	[20] = KEY_3,		\
	[21] = KEY_CALL,		\
	[22] = KEY_LEFTALT,		\
	[23] = KEY_VOLUMEUP,		\
	[24] = KEY_FOCUS,		\
	[25] = KEY_COMMA,		\
	[27] = KEY_4,		\
	[28] = KEY_5,		\
	[29] = KEY_6,		\
	[30] = KEY_ENDCALL,		\
	[31] = KEY_SEARCH,		\
	[32] = KEY_VOLUMEDOWN,		\
	[33] = KEY_CAMERA,		\
	[34] = KEY_DOT,		\
	[36] = KEY_7,		\
	[37] = KEY_8,		\
	[38] = KEY_9,		\
	[39] = KEY_HOME,		\
	[40] = KEY_UP,		\
	[41] = KEY_BACK,		\
	[42] = KEY_MENU,		\
	[43] = KEY_SLASH,		\
	[45] = KEY_STAR,		\
	[46] = KEY_0,		\
	[47] = KEY_POUND,		\
	[48] = KEY_LEFT,		\
	[49] = KEY_DOWN,		\
	[50] = KEY_RIGHT,		\
	[51] = KEY_LEFTSHIFT,		\
	[52] = KEY_ENTER,		\
	[54] = KEY_Q,		\
	[55] = KEY_W,		\
	[56] = KEY_E,		\
	[57] = KEY_R,		\
	[58] = KEY_T,		\
	[59] = KEY_Y,		\
	[60] = KEY_U,		\
	[61] = KEY_I,		\
	[63] = KEY_A,		\
	[64] = KEY_S,		\
	[65] = KEY_D,		\
	[66] = KEY_F,		\
	[67] = KEY_G,		\
	[68] = KEY_H,		\
	[69] = KEY_J,		\
	[70] = KEY_K,		\
}	 

#endif
