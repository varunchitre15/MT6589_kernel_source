


/*******************************************************************************
 *
 * Filename:
 * ---------
 *   CFG_ADC_File.h
 *
 * Project:
 * --------
 *   DUMA
 *
 * Description:
 * ------------
 *    header file of ADC CFG file
 *
 * Author:
 * -------
 *   MTK80198(Chunlei Wang)
 *
 *
 *
 *******************************************************************************/


#ifndef _CFG_AUXADC_FILE_H
#define _CFG_AUXADC_FILE_H



// the record structure define of adc nvram file
typedef struct
{
    int Slop[9];
    int Offset[9];
    int cal;
}AUXADC_CFG_Struct;

///please define it according to your module
#define CFG_FILE_AUXADC_REC_SIZE    sizeof(AUXADC_CFG_Struct)
#define CFG_FILE_AUXADC_REC_TOTAL   1


#endif


