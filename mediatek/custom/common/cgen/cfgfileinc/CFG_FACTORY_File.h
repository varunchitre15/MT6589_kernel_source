


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_FACTORY_FILE.h
 *
 * Project:
 * --------
 *   YUSU
 *
 * Description:
 * ------------
 *   Hheader file of FACTORY CFG file
 *
 * Author:
 * -------
 *   MTK02528(Koshi Chiu)
 *
 *
 *
 *******************************************************************************/


#ifndef _CFG_FACTORY_FILE_H
#define _CFG_FACTORY_FILE_H


///define meta nvram record
typedef struct
{
//    UINT32 rf_calabrated;
      unsigned int rf_calabrated;
} FACTORY_CFG_Struct;

//please define it according to your module
#define CFG_FILE_FACTORY_REC_SIZE    sizeof(FACTORY_CFG_Struct)
#define CFG_FILE_FACTORY_REC_TOTAL   1

#endif


