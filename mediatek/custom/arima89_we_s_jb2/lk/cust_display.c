

//#include <common.h>
//#include <stdarg.h>

//#include <linux/types.h>

//#include <asm/arch/mt65xx.h>
//#include <asm/arch/mt65xx_typedefs.h>
//#include <asm/arch/mt65xx_disp_drv.h>
//#include <asm/arch/mt65xx_logo.h>

#include <platform/mt_typedefs.h>
#include <platform/mt_disp_drv.h>
#include <platform/mt_logo.h>

#include <target/board.h>

// this is a sample code for how to draw the logo by yourself.
void cust_show_boot_logo(void)
{
    void  *fb_addr = mt_get_fb_addr();
    UINT32 fb_size = mt_get_fb_size();
    void  *db_addr = mt_get_logo_db_addr();

    memcpy(fb_addr, (UINT32)db_addr + fb_size * 0, fb_size);

    mt_disp_update(0, 0, CFG_DISPLAY_WIDTH, CFG_DISPLAY_HEIGHT);
}

const LOGO_CUST_IF *LOGO_GetCustomIF(void)
{
    static LOGO_CUST_IF CUST_IF;

    memset(&CUST_IF, 0, sizeof(LOGO_CUST_IF));
   
    // the CUST_IF is null by default.
    // if you want to use your routine, please replace the callback like the code below shows:
    //CUST_IF.show_boot_logo = cust_show_boot_logo;

    return &CUST_IF;
}


