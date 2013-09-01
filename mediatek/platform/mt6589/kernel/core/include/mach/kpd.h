#ifndef KPD_IO_H
#define KPD_IO_H
		  
#include <linux/ioctl.h>
#include <mtk_kpd.h>

#define KPD_AUTOTEST	KPD_YES
#define KPD_DEBUG	KPD_YES

struct kpd_ledctl {
	int onoff;
	int div;		/* 0 ~ 15 */
	int duty;	/* 0 ~ 31 */
};

#if KPD_AUTOTEST
#define PRESS_OK_KEY		_IO('k', 1)
#define RELEASE_OK_KEY		_IO('k', 2)
#define PRESS_MENU_KEY		_IO('k', 3)
#define RELEASE_MENU_KEY	_IO('k', 4)
#define PRESS_UP_KEY		_IO('k', 5)
#define RELEASE_UP_KEY		_IO('k', 6)
#define PRESS_DOWN_KEY		_IO('k', 7)
#define RELEASE_DOWN_KEY	_IO('k', 8)
#define PRESS_LEFT_KEY		_IO('k', 9)
#define RELEASE_LEFT_KEY	_IO('k', 10)
#define PRESS_RIGHT_KEY		_IO('k', 11)
#define RELEASE_RIGHT_KEY	_IO('k', 12)
#define PRESS_HOME_KEY		_IO('k', 13)
#define RELEASE_HOME_KEY	_IO('k', 14)
#define PRESS_BACK_KEY		_IO('k', 15)
#define RELEASE_BACK_KEY	_IO('k', 16)
#define PRESS_CALL_KEY		_IO('k', 17)
#define RELEASE_CALL_KEY	_IO('k', 18)
#define PRESS_ENDCALL_KEY	_IO('k', 19)
#define RELEASE_ENDCALL_KEY	_IO('k', 20)
#define PRESS_VLUP_KEY		_IO('k', 21)
#define RELEASE_VLUP_KEY	_IO('k', 22)
#define PRESS_VLDOWN_KEY	_IO('k', 23)
#define RELEASE_VLDOWN_KEY	_IO('k', 24)
#define PRESS_FOCUS_KEY		_IO('k', 25)
#define RELEASE_FOCUS_KEY	_IO('k', 26)
#define PRESS_CAMERA_KEY	_IO('k', 27)
#define RELEASE_CAMERA_KEY	_IO('k', 28)
#endif
#define SET_KPD_BACKLIGHT	_IOW('k', 29, struct kpd_ledctl)
#define SET_KPD_KCOL		_IO('k', 29)



#endif
