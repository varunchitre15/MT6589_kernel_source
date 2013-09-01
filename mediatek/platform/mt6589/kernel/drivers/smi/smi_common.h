#ifndef __SMI_COMMON_H__
#define __SMI_COMMON_H__

#include <linux/xlog.h>
#include <linux/aee.h>


#define SMIMSG(string, args...)	xlog_printk(ANDROID_LOG_INFO, SMI_LOG_TAG, "[pid=%d]"string,current->tgid,##args)
#define SMITMP(string, args...)  xlog_printk(ANDROID_LOG_INFO, SMI_LOG_TAG, "[pid=%d]"string,current->tgid,##args)
#define SMIERR(string, args...) do{\
	xlog_printk(ANDROID_LOG_ERROR,  SMI_LOG_TAG, "error: "string, ##args); \
	aee_kernel_warning(SMI_LOG_TAG, "error: "string, ##args);  \
}while(0)

#define smi_aee_print(string, args...) do{\
    char smi_name[100];\
    snprintf(smi_name,100, "["SMI_LOG_TAG"]"string, ##args); \
  aee_kernel_warning(smi_name, "["SMI_LOG_TAG"]error:"string,##args);  \
}while(0)


#define MAU_ENTRY_NR    3
#define SMI_LARB_NR     5


extern unsigned int gLarbBaseAddr[SMI_LARB_NR];
extern char *smi_port_name[][15];


int larb_clock_on(int larb_id);
int larb_clock_off(int larb_id);

int mau_init(void);
void SMI_DBG_Init(void);


#endif

