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

//#include <common.h>
//#include <asm/arch/mt65xx.h>
#include <platform/mt_typedefs.h>
#include <platform/mt_rtc.h>
#include <platform/mtk_wdt.h>

extern kal_bool pmic_chrdet_status(void);
//<2013/07/02-26515-kevincheng,Add power off setting in CU
extern U32 pmic_config_interface (U32 RegNum, U32 val, U32 MASK, U32 SHIFT); //kevin
#ifndef NO_POWER_OFF
void mt6575_power_off(void)
{
        int pmic_ret = 0;

	pmic_ret = pmic_config_interface(0x0138/*INT_RSV*/, 0x0, 0x1, 0x7);//kevin
	printf("mt_power_off\n");

	/* pull PWRBB low */
	rtc_bbpu_power_down();

	while (1) {
		printf("mt_power_off : check charger\n");
		if (pmic_chrdet_status() == KAL_TRUE)
			mtk_arch_reset(0);
	}
}
#endif
//>2013/07/02-26515-kevincheng
