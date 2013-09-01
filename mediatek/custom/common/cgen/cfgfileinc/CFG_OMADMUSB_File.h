


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_OMADMUSB_File.h
 *
 * Project:
 * --------
 *   YUSU
 *
 * Description:
 * ------------
 *    header file of OMADMUSB CFG file
 *
 * Author:
 * -------
 *   MTK80863(Hao Lin)
 *
 *
 *
 *******************************************************************************/


#ifndef _CFG_OMADMUSB_FILE_H
#define _CFG_OMADMUSB_FILE_H



// the record structure define of OMADM USB nvram file
typedef struct
{
    int iIsEnable;
    int iUsb;
    int iAdb;
    int iRndis;
}OMADMUSB_CFG_Struct;

///please define it according to your module
#define CFG_FILE_OMADMUSB_REC_SIZE    sizeof(OMADMUSB_CFG_Struct)
#define CFG_FILE_OMADMUSB_REC_TOTAL   1


#endif


