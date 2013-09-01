


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_SIM_FILE.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *    header file of SIM config struct
 *
 * Author:
 * -------
 *    Liwen Chang (MTK02556)
 *
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 *    mtk02556
 * [DUMA00128675] [NVRAM] Add NVRAM SIM struct
 * NVRAM SIM new struct add
 *
 *
 *******************************************************************************/

#ifndef _CFG_SIM_FILE_H
#define _CFG_SIM_FILE_H

typedef struct
{	
	unsigned char sim2_ctl_flag;//0: AP side control, 1: MD side control;                                           
}ap_nvram_sim_config_struct;

#define CFG_FILE_SIM_CONFIG_SIZE    sizeof(ap_nvram_sim_config_struct)
#define CFG_FILE_SIM_CONFIG_TOTAL   1

#endif// _CFG_SIM_FILE_H


