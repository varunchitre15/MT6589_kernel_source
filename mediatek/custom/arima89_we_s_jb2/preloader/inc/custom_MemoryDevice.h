

#ifndef __CUSTOM_MEMORYDEVICE__
#define __CUSTOM_MEMORYDEVICE__

/*
 ****************************************************************************
 [README , VERY IMPORTANT NOTICE]
 --------------------------------
 After user configured this C header file, not only C compiler compile it but
 also auto-gen tool parse user's configure setting.
 Here are recommend configure convention to make both work fine.

 1. All configurations in this file form as #define MACRO_NAME MACRO_VALUE format.
    Note the #define must be the first non-space character of a line

 2. To disable the optional configurable item. Please use // before #define,
    for example: //#define MEMORY_DEVICE_TYPE

 3. Please don't use #if , #elif , #else , #endif conditional macro key word here.
    Such usage might cause compile result conflict with auto-gen tool parsing result.
    Auto-Gen tool will show error and stop.
    3.1.  any conditional keyword such as #if , #ifdef , #ifndef , #elif , #else detected.
          execpt this #ifndef __CUSTOM_MEMORYDEVICE__
    3.2.  any duplicated MACRO_NAME parsed. For example auto-gen tool got 
          2nd MEMORY_DEVICE_TYPE macro value.
 ****************************************************************************
*/

/*
 ****************************************************************************
 Step 1: Specify memory device type and its complete part number
         Possible memory device type: LPSDRAM (SDR, DDR)
 ****************************************************************************
*/

#define BOARD_ID                MT6589_EVB

//<2013/01/18-20549-stevenchen, Add memory device "H9TP32A8JDMCPR_KGM" & "H9TP32A8JDACPR_KGM".
#define CS_PART_NUMBER[0]       H9TP32A8JDMCPR_KGM
#define CS_PART_NUMBER[1]       H9TP32A8JDACPR_KGM
//>2013/01/18-20549-stevenchen 

//<2013/06/17-26010-stevenchen, [Pelican] [drv] Change 2nd source memory to KMK5W000VM_B312.
//<2013/03/13-22730-stevenchen, Add 2nd source memory, "KMK5U000VM_B309".
#define CS_PART_NUMBER[2]	KMK5W000VM_B312_MMD2
//>2013/03/13-22730-stevenchen
//>2013/06/17-26010-stevenchen

#endif /* __CUSTOM_MEMORYDEVICE__ */
