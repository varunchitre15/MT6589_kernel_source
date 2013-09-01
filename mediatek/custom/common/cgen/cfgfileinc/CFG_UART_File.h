


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   cfg_uart_file.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *    header file of UART config struct
 *
 * Author:
 * -------
 *    LiChunhui (MTK80143)
 *
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 * Mar 24 2009 mtk80143
 * [DUMA00112375] [uart] UART EM mode
 * Add for UART EM mode
 *
 *
 *******************************************************************************/

#ifndef _CFG_UART_FILE_H
#define _CFG_UART_FILE_H


#if 1 //defined (__MT6516_AP__)
    #define UART_TOTAL_NUM  0x04
#elif defined (__TK6516_AP__)
    #define UART_TOTAL_NUM  0x03
#endif 

typedef struct
{	
	unsigned char uart_ctl_flag[UART_TOTAL_NUM];//0: AP side control, 1: MD side control; 
	                                            //uart_ctl_flag[0]<->UART1
	                                            //uart_ctl_flag[1]<->UART2
	                                            //uart_ctl_flag[2]<->UART3
	                                            //........................	                                            
}ap_nvram_uart_config_struct;

#define CFG_FILE_UART_CONFIG_SIZE    sizeof(ap_nvram_uart_config_struct)
#define CFG_FILE_UART_CONFIG_TOTAL   1

#endif


