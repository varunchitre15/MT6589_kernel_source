/*
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/mt65xx.h>
#include <asm/arch/mt65xx_typedefs.h>
#include <asm/arch/mt_rtc.h>
#include <asm/arch/mtk_wdt.h>

extern kal_bool pmic_chrdet_status(void);

#ifndef NO_POWER_OFF
void mt6575_power_off(void)
{
	printf("mt6575_power_off\n");

	/* pull PWRBB low */
	rtc_bbpu_power_down();

	while (1) {
		printf("mt6575_power_off : check charger\n");
		if (pmic_chrdet_status() == KAL_TRUE)
			mtk_arch_reset(0);
	}
}
#endif
