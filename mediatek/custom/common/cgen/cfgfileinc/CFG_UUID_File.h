


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_UUID_File.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *    header file of UUID CFG file
 *
 * Author:
 * -------
 *   MTK02556(Liwen Chang)
 *
 *
 *
 *******************************************************************************/


#ifndef _CFG_UUID_FILE_H
#define _CFG_UUID_FILE_H



// the record structure define of adc nvram file
typedef struct
{
 //   UINT32 uid[2];
      unsigned int uid[2];
} NVRAM_UUID_STRUCT;

///please define it according to your module
#define CFG_FILE_UUID_REC_SIZE    sizeof(NVRAM_UUID_STRUCT)
#define CFG_FILE_UUID_REC_TOTAL   1


#endif //_CFG_UUID_FILE_H


