#ifndef __TVOUT_DVT_H__
#define __TVOUT_DVT_H__


#define MTK_IOW(num, dtype)     _IOW('O', num, dtype)
#define MTK_IOR(num, dtype)     _IOR('O', num, dtype)
#define MTK_IOWR(num, dtype)    _IOWR('O', num, dtype)
#define MTK_IO(num)             _IO('O', num)

// --------------------------------------------------------------------------
#define TV_DVT_TVROT_PATH_INIT      MTK_IOW(60, unsigned long)
#define TV_DVT_TVROT_PATH_CONFIG    MTK_IOW(61, unsigned long)
#define TV_DVT_TVROT_PATH_START     MTK_IOW(62, unsigned long)
#define TV_DVT_TVROT_PATH_STOP     	MTK_IOW(63, unsigned long)
#define TV_DVT_TVROT_PATH_WAIT      MTK_IOW(64, unsigned long)
#define TV_DVT_TVROT_PATH_DEINIT 	MTK_IOW(65, unsigned long)
#define TV_DVT_TVROT_UPDATE_SCREEN 	MTK_IOW(66, unsigned long)








#define TV_DVT_SET_REGISTERS_TVROT  0x10
#define TV_DVT_DUMP_REGISTERS_TVROT 0x20


void TVDVT_update_screen(void);
void TVDVT_update_src(void);

void TVDVT_prepare_source(unsigned int src);

void TVDVT_init_lcd(void);



#endif
