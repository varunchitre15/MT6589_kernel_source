/* linux/include/asm-arm/arch-mt6516/mt6516_bt.h
 *
 * Bluetooth Machine Related Functions & Configurations
 *
 * Copyright (C) 2008,2009 MediaTek <www.mediatek.com>
 * Authors: JinKwan Huang <jk.huang@mediatek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __ARCH_ARM_MACH_MT6573_INCLUDE_MACHINE_MT_COMBO_H
#define __ARCH_ARM_MACH_MT6573_INCLUDE_MACHINE_MT_COMBO_H

/*******************************************************************************
*                         C O M P I L E R   F L A G S
********************************************************************************
*/

/*******************************************************************************
*                E X T E R N A L   R E F E R E N C E S
********************************************************************************
*/
#include <linux/types.h>

/*******************************************************************************
*                          C O N S T A N T S
********************************************************************************
*/

/*******************************************************************************
*                         D A T A   T Y P E S
********************************************************************************
*/
typedef enum {
    COMBO_AUDIO_STATE_0 = 0, /* 0000: BT_PCM_OFF & FM analog (line in/out) */
    COMBO_AUDIO_STATE_1 = 1, /* 0001: BT_PCM_ON & FM analog (in/out) */
    COMBO_AUDIO_STATE_2 = 2, /* 0010: BT_PCM_OFF & FM digital (I2S) */
    COMBO_AUDIO_STATE_3 = 3, /* 0011: BT_PCM_ON & FM digital (I2S) (invalid in 73evb & 1.2 phone configuration) */
    COMBO_AUDIO_STATE_MAX = 4,
} COMBO_AUDIO_STATE;

typedef enum {
    COMBO_FUNC_TYPE_BT = 0,
    COMBO_FUNC_TYPE_FM = 1,
    COMBO_FUNC_TYPE_GPS = 2,
    COMBO_FUNC_TYPE_WIFI = 3,
    COMBO_FUNC_TYPE_WMT = 4,
    COMBO_FUNC_TYPE_STP = 5,
    COMBO_FUNC_TYPE_NUM = 6
} COMBO_FUNC_TYPE;

typedef enum {
    COMBO_IF_UART = 0,
    COMBO_IF_MSDC = 1,
    COMBO_IF_MAX,
} COMBO_IF;


/******************************************************************************
*                   F U N C T I O N   D E C L A R A T I O N S
*******************************************************************************
*/


/* [GeorgeKuo] Stub functions for other kernel built-in modules to call.
 * Keep them unchanged temporarily. Move mt_combo functions to mtk_wcn_combo.
 */
extern int mt_combo_audio_ctrl_ex(COMBO_AUDIO_STATE state, u32 clt_ctrl);
static inline int mt_combo_audio_ctrl(COMBO_AUDIO_STATE state) {
    return mt_combo_audio_ctrl_ex(state, 1);
}
extern int mt_combo_plt_enter_deep_idle(COMBO_IF src);
extern int mt_combo_plt_exit_deep_idle(COMBO_IF src);

/* Use new mtk_wcn_stub APIs instead of old mt_combo ones for kernel to control
 * function on/off.
 */
extern void mtk_wcn_cmb_stub_func_ctrl (unsigned int type, unsigned int on);
extern int board_sdio_ctrl (unsigned int sdio_port_num, unsigned int on);


#endif

