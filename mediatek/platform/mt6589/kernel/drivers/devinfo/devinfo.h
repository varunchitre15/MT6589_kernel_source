
#ifndef DEVFINO_H
#define DEVFINO_H

 
 /***************************************************************************** 
 * MODULE DEFINITION 
 *****************************************************************************/
#define MODULE_NAME	    "[devinfo]"
#define DEV_NAME        "devmap"
#define MAJOR_DEV_NUM    196
 
 /***************************************************************************** 
 * IOCTL DEFINITION 
 *****************************************************************************/
#define DEV_IOC_MAGIC       'd'
#define READ_DEV_DATA       _IOR(DEV_IOC_MAGIC,  1, unsigned int)

#define DEV_IOC_MAXNR       (10)

/***************************************************************************** 
* EXPORT DEFINITION 
*****************************************************************************/
extern u32 g_devinfo_data_size;
extern u32 get_devinfo_with_index(u32 index);;


#endif /* end of DEVFINO_H */

