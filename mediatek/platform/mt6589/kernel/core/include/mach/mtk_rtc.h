
#ifndef _MTK_RTC_H_
#define _MTK_RTC_H_

#include <linux/kernel.h>
#include <linux/rtc.h>

#define RTC_GPIO_USER_MASK	(((1U << 13) - 1) & 0xff00)

typedef enum {
	RTC_GPIO_USER_WIFI	= 8,
	RTC_GPIO_USER_GPS	= 9,
	RTC_GPIO_USER_BT	= 10,
	RTC_GPIO_USER_FM	= 11,
	RTC_GPIO_USER_PMIC	= 12,
} rtc_gpio_user_t;

#ifdef CONFIG_MTK_RTC

/*
 * NOTE:
 * 1. RTC_GPIO always exports 32K enabled by some user even if the phone is powered off
 */
extern void rtc_gpio_enable_32k(rtc_gpio_user_t user);
extern void rtc_gpio_disable_32k(rtc_gpio_user_t user);
extern bool rtc_gpio_32k_status(void);

/* for AUDIOPLL (deprecated) */
extern void rtc_enable_abb_32k(void);
extern void rtc_disable_abb_32k(void);

/* NOTE: used in Sleep driver to workaround Vrtc-Vore level shifter issue */
extern void rtc_enable_writeif(void);
extern void rtc_disable_writeif(void);

extern void rtc_mark_recovery(void);
#if defined (MTK_KERNEL_POWER_OFF_CHARGING)
extern void rtc_mark_kpoc(void);
#endif

extern u16 rtc_rdwr_uart_bits(u16 *val);

extern void rtc_bbpu_power_down(void);

extern void rtc_read_pwron_alarm(struct rtc_wkalrm *alm);


extern int get_rtc_spare_fg_value(void);
extern int set_rtc_spare_fg_value(int val);

extern void rtc_irq_handler(void);

extern bool crystal_exist_status(void);

#else

#define rtc_gpio_enable_32k(user)	do {} while (0)
#define rtc_gpio_disable_32k(user)	do {} while (0)
#define rtc_gpio_32k_status()		do {} while (0)
#define rtc_enable_abb_32k()		do {} while (0)
#define rtc_disable_abb_32k()		do {} while (0)
#define rtc_enable_writeif()		do {} while (0)
#define rtc_disable_writeif()		do {} while (0)
#define rtc_mark_recovery()		do {} while (0)
#define rtc_rdwr_uart_bits(val)		({ 0; })
#define rtc_bbpu_power_down()		do {} while (0)
#define rtc_read_pwron_alarm(alm)	do {} while (0)

#define get_rtc_spare_fg_value()	do {} while (0)
#define set_rtc_spare_fg_value(val)	do {} while (0)

#define rtc_irq_handler()			do {} while (0)

#define crystal_exist_status()		do {} while (0)
#endif

#endif
