

#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/aee.h>
#include <linux/timer.h>
#include <linux/disp_assert_layer.h>
#include <linux/xlog.h>
#include <linux/fs.h>

#include <asm/mach/map.h>
#include <mach/sync_write.h>
#include <mach/mt_irq.h>
#include <mach/mt_clkmgr.h>
#include <mach/irqs.h>
#include <mach/mt_boot.h>
#include <asm/cacheflush.h>
#include <asm/system.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/printk.h>

#include <mach/m4u.h>
#include <mach/mt_smi.h>
#include "m4u_reg.h"
#include "../smi/smi_common.h"
#include <ddp_reg.h>

#include <linux/m4u_profile.h>


#define M4U_ASSERT(x) if(!(x)){xlog_printk(ANDROID_LOG_ERROR, "M4U", "assert fail, file:%s, line:%d", __FILE__, __LINE__);}

//#define MTK_M4U_DBG
#ifdef MTK_M4U_DBG
#define M4UDBG(string, args...)	xlog_printk(ANDROID_LOG_INFO, "M4U", "[pid=%d]"string,current->tgid,##args);

bool gM4uLogFlag = false;
#define M4ULOG(string, args...) xlog_printk(ANDROID_LOG_INFO, "M4U", "[pid=%d] "string,current->tgid,##args)

#else

#define M4UDBG(string, args...)

bool gM4uLogFlag = false;

#define M4ULOG(string, args...) do { \
	if(gM4uLogFlag){ \
	xlog_printk(ANDROID_LOG_INFO, "M4U", "[pid=%d] "string,current->tgid,##args); } \
}while(0)

#endif

#define M4UMSG(string, args...)	xlog_printk(ANDROID_LOG_INFO, "M4U", string,##args)
#define M4UINFO(string, args...) xlog_printk(ANDROID_LOG_DEBUG, "M4U", string,##args)
//#define M4UINFO(string, args...) xlog_printk(ANDROID_LOG_INFO, "M4U", string,##args)


#define M4UTMP(string, args...)  xlog_printk(ANDROID_LOG_INFO, "M4U", string,##args)

#define M4UERR(string, args...) do {\
	xlog_printk(ANDROID_LOG_ERROR, "M4U", "error: "string,##args);  \
	aee_kernel_exception("M4U", "[M4U] error:"string,##args);  \
}while(0)

static char m4u_name[100];
#define m4u_aee_print(string, args...) do{\
    snprintf(m4u_name,100, "[M4U]"string, ##args); \
    aee_kernel_warning_api(__FILE__, __LINE__, DB_OPT_MMPROFILE_BUFFER, m4u_name, "[M4U]"string, ##args);  \
}while(0)
    // aee_kernel_warning(m4u_name, "[M4U] error:"string,##args);  \

#define PFNMAP_FLAG_SET 0x00555555

//#define M4U_USE_ONE_PAGETABLE
#define M4U_COPY_NONSEC_PT_TO_SEC


// garbage collect related
#define MVA_REGION_FLAG_NONE 0x0
#define MVA_REGION_HAS_TLB_RANGE 0x1
#define MVA_REGION_REGISTER    0x2

// list element, each element record mva's size, start addr info
// if user process dose not call mva_alloc() and mva_dealloc() in pair
// we will help to call mva_dealloc() according to elements' info
typedef struct
{
    struct list_head link;
    unsigned int bufAddr;
    unsigned int mvaStart;
    unsigned int size;
    M4U_MODULE_ID_ENUM eModuleId;
    unsigned int flags;    
    int security;
    int cache_coherent;

} mva_info_t;

// per-file-handler structure, allocated in M4U_Open, used to 
// record calling of mva_alloc() and mva_dealloc()    
typedef struct
{
    struct mutex dataMutex;
    pid_t open_pid;
    pid_t open_tgid;
    unsigned int OwnResource;
    struct list_head mvaList;
    int isM4uDrvConstruct;
    int isM4uDrvDeconstruct;
} garbage_node_t;

static mva_info_t gMvaNode_unkown = 
{
    .bufAddr = 0,
    .mvaStart = 0,
    .size = 0,
    .eModuleId = M4U_CLNTMOD_UNKNOWN,
};


//------------------------------------Defines & Data for alloc mva-------------
//----------------------------------------------------------------------
/// macros to handle M4u Page Table processing
#define M4U_MVA_MAX 0x3fffffff   // 1G 
#define M4U_PAGE_MASK 0xfff
#define M4U_PAGE_SIZE   0x1000 //4KB
#define DEFAULT_PAGE_SIZE   0x1000 //4KB
#define M4U_PTE_MAX (M4U_GET_PTE_OFST_TO_PT_SA(TOTAL_MVA_RANGE-1))
#define mva_pteAddr_nonsec(mva) ((unsigned int *)pPT_nonsec+((mva) >> 12))
#define mva_pteAddr_sec(mva) ((unsigned int *)pPT_sec+((mva) >> 12))
#define mva_pteAddr(mva) mva_pteAddr_nonsec(mva)

//  ((va&0xfff)+size+0xfff)>>12
#define M4U_GET_PAGE_NUM(va,size) ((((va)&(M4U_PAGE_SIZE-1))+(size)+(M4U_PAGE_SIZE-1))>>12)

#define mva_pageOffset(mva) ((mva)&0xfff)

#define MVA_BLOCK_SIZE_ORDER     18     //256K
#define MVA_MAX_BLOCK_NR        4095    //1GB

#define MVA_BLOCK_SIZE      (1<<MVA_BLOCK_SIZE_ORDER)  //0x40000 
#define MVA_BLOCK_ALIGN_MASK (MVA_BLOCK_SIZE-1)        //0x3ffff
#define MVA_BLOCK_NR_MASK   (MVA_MAX_BLOCK_NR)      //0xfff
#define MVA_BUSY_MASK       (1<<15)                 //0x8000

#define MVA_IS_BUSY(index) ((mvaGraph[index]&MVA_BUSY_MASK)!=0)
#define MVA_SET_BUSY(index) (mvaGraph[index] |= MVA_BUSY_MASK)
#define MVA_SET_FREE(index) (mvaGraph[index] & (~MVA_BUSY_MASK))
#define MVA_GET_NR(index)   (mvaGraph[index] & MVA_BLOCK_NR_MASK)

#define MVAGRAPH_INDEX(mva) (mva>>MVA_BLOCK_SIZE_ORDER)


static short mvaGraph[MVA_MAX_BLOCK_NR+1];
static mva_info_t* mvaInfoGraph[MVA_MAX_BLOCK_NR+1];


//#define M4U_MVA_ALLOC_DEBUG
#ifdef M4U_MVA_ALLOC_DEBUG
#define M4U_MVA_DBUG(string, args...) printk("[M4U_K][MVA]"string,##args)
#define M4U_mvaGraph_dump_DBG() m4u_mvaGraph_dump()
#else
#define M4U_MVA_DBUG(string, args...)
#define M4U_mvaGraph_dump_DBG() 
#endif



static DEFINE_SPINLOCK(gMvaGraph_lock);

static void m4u_mvaGraph_init(void);
void m4u_mvaGraph_dump_raw(void);
void m4u_mvaGraph_dump(void);
static int m4u_dealloc_mva_dynamic(M4U_MODULE_ID_ENUM eModuleID, 
									const unsigned int BufAddr, 
									const unsigned int BufSize,
									unsigned int mvaRegionAddr);
static unsigned int m4u_do_mva_alloc(M4U_MODULE_ID_ENUM eModuleID, 
								  const unsigned int BufAddr, 
								  const unsigned int BufSize,
								  mva_info_t *pMvaInfo);
static int m4u_do_mva_free(M4U_MODULE_ID_ENUM eModuleID, 
                                const unsigned int BufAddr,
								const unsigned int BufSize,
								unsigned int mvaRegionStart) ;
static int __m4u_alloc_mva(mva_info_t *pMvaInfo);
static M4U_MODULE_ID_ENUM mva2module(unsigned int mva);
int m4u_invalid_seq_range_by_mva(int m4u_index, unsigned int MVAStart, unsigned int MVAEnd);
int m4u_dump_pagetable(M4U_MODULE_ID_ENUM eModuleID);
int m4u_confirm_range_invalidated(int m4u_index, unsigned int MVAStart, unsigned int MVAEnd);

static bool m4u_struct_init(void);
int m4u_hw_init(void);
static int m4u_get_pages(M4U_MODULE_ID_ENUM eModuleID,
                    unsigned int BufAddr, 
                    unsigned int BufSize, 
                    unsigned int* pPageTableAddr);
                               
static int m4u_release_pages(M4U_MODULE_ID_ENUM eModuleID,
                    unsigned int BufAddr, 
                    unsigned int BufSize,
                    unsigned int MVA);

static M4U_DMA_DIR_ENUM m4u_get_dir_by_module(M4U_MODULE_ID_ENUM eModuleID);
static void m4u_clear_intr(unsigned int m4u_base);
static int m4u_port_2_m4u_id(M4U_PORT_ID_ENUM portID);
static void m4u_memory_usage(bool bPrintAll);
void m4u_print_active_port(unsigned int m4u_index);
static M4U_MODULE_ID_ENUM m4u_port_2_module(M4U_PORT_ID_ENUM portID);
static char* m4u_get_port_name(M4U_PORT_ID_ENUM portID);
static char* m4u_get_module_name(M4U_MODULE_ID_ENUM moduleID);
void m4u_get_power_status(void);
unsigned int m4u_get_pa_by_mva(unsigned int mva);
int m4u_dump_user_addr_register(unsigned int m4u_index);
static int m4u_free_garbage_list(mva_info_t *pList);
static mva_info_t* m4u_alloc_garbage_list(   unsigned int mvaStart, 
                                          unsigned int bufSize,
                                          M4U_MODULE_ID_ENUM eModuleID,
                                          unsigned int va,
                                          unsigned int flags,
                                          int security,
                                          int cache_coherent);
static int m4u_add_to_garbage_list(struct file * a_pstFile,mva_info_t *pList);
static mva_info_t* m4u_delete_from_garbage_list(M4U_MOUDLE_STRUCT* p_m4u_module, struct file * a_pstFile);
M4U_PORT_ID_ENUM m4u_get_error_port(unsigned int m4u_index, unsigned int mva);
int m4u_dump_mva_info(void);
int m4u_get_write_mode_by_module(M4U_MODULE_ID_ENUM moduleID);
void m4u_dump_pagetable_range(unsigned int vaStart, unsigned int nr);
void m4u_print_mva_list(struct file *filep, const char *pMsg);
int m4u_dma_cache_flush_all(void);
void mlock_vma_page(struct page *page);
void munlock_vma_page(struct page *page);
int m4u_dump_main_tlb_tags(int m4u_id) ;
int m4u_dump_main_tlb_des(int m4u_id); 
int m4u_dump_pfh_tlb_tags(int m4u_id);
int m4u_dump_pfh_tlb_des(int m4u_id);
int m4u_enable_error_hang(int m4u_id);
int m4u_disable_error_hang(int m4u_id);
int m4u_search_main_invalid(int m4u_id);


//-------------------------------------Global variables------------------------------------------------//

#define MAX_BUF_SIZE_TO_GET_USER_PAGE (200*1024*1024)  //200MB at most for single time alloc

extern unsigned char *pMlock_cnt;
extern unsigned int mlock_cnt_size;
// record memory usage
int* pmodule_max_size=NULL;
int* pmodule_current_size=NULL;
int* pmodule_locked_pages=NULL;

unsigned int gM4UBaseAddr[TOTAL_M4U_NUM] = {M4U_BASE0, M4U_BASE1};
unsigned int g4M4UTagCount[TOTAL_M4U_NUM]  = {M4U_MAIN_TLB_NR, M4U_MAIN_TLB_NR};
unsigned int g4M4UWrapCount[TOTAL_M4U_NUM] = {M4U_WRAP_NR, M4U_WRAP_NR};
unsigned int g4M4UWrapOffset[TOTAL_M4U_NUM]= {0, M4U_WRAP_NR};
static volatile unsigned int FreeSEQRegs[TOTAL_M4U_NUM] = {M4U_SEQ_NR, M4U_SEQ_NR};
static volatile unsigned int FreeWrapRegs[TOTAL_M4U_NUM]= {M4U_WRAP_NR, M4U_WRAP_NR};


unsigned int m4u_index_of_larb[SMI_LARB_NR] = {0,0,0,0,1};
unsigned int smi_port0_in_larbx[SMI_LARB_NR+1] = {0, 10, 17, 29, 44 ,56};
unsigned int m4u_port0_in_larbx[SMI_LARB_NR+1] = {0, 10, 17, 29, 43 ,53};
unsigned int m4u_port_size_limit[M4U_PORT_NR] = {};

int gM4U_L2_enable = 1;

unsigned int pt_pa_nonsec;    //Page Table Physical Address, 64K align
unsigned int *pPT_nonsec;
unsigned int pt_pa_sec;
unsigned int *pPT_sec;


#define TF_PROTECT_BUFFER_SIZE 128
unsigned int ProtectPA = 0;
unsigned int *pProtectVA_nonCache = NULL; 

//unsigned int gM4U_align_page_va = 0;
unsigned int gM4U_align_page_pa = 0;

//#define BACKUP_REG_SIZE (M4U_REG_SIZE*TOTAL_M4U_NUM)
#define BACKUP_REG_SIZE 640
unsigned int* pM4URegBackUp = 0;

static M4U_RANGE_DES_T *pRangeDes = NULL;
static M4U_WRAP_DES_T *pWrapDes = 0;
#define RANGE_DES_ADDR 0x11

static int g_debug_make_translation_fault=0;
static int g_debug_print_detail_in_isr=1;
static int g_debug_enable_error_hang=0;
static int g_debug_recover_pagetable_TF=0;
static int g_debug_dump_rs_in_isr=0;

spinlock_t gM4u_reg_lock;
static DEFINE_MUTEX(gM4uMutex);
static DEFINE_MUTEX(gM4uMutexPower);

#define MTK_M4U_DEV_MAJOR_NUMBER 188
static struct cdev * g_pMTKM4U_CharDrv = NULL;
static dev_t g_MTKM4Udevno = MKDEV(MTK_M4U_DEV_MAJOR_NUMBER,0);
#define M4U_DEVNAME "M4U_device"

extern void init_mlock_cnt(void);

extern unsigned int m4u_user_v2p(unsigned int va);

extern int is_pmem_range(unsigned long* base, unsigned long size);
extern int m4u_get_user_pages(int eModuleID, struct task_struct *tsk, struct mm_struct *mm, unsigned long start, int nr_pages, int write, int force, struct page **pages, struct vm_area_struct **vmas);
extern void  smp_inner_dcache_flush_all(void);



unsigned int gModuleMaxMVASize[M4U_CLIENT_MODULE_NUM] = {
    M4U_CLNTMOD_SZ_VENC    ,
    M4U_CLNTMOD_SZ_VDEC    ,
    M4U_CLNTMOD_SZ_ROT     ,
    M4U_CLNTMOD_SZ_OVL     ,
    M4U_CLNTMOD_SZ_WDMA    ,
    M4U_CLNTMOD_SZ_RDMA    ,
    M4U_CLNTMOD_SZ_CMDQ    ,
    M4U_CLNTMOD_SZ_DBI     ,
    M4U_CLNTMOD_SZ_G2D     ,
    M4U_CLNTMOD_SZ_JPGDEC  ,
    M4U_CLNTMOD_SZ_JPGENC  ,
    M4U_CLNTMOD_SZ_VIP     ,
    M4U_CLNTMOD_SZ_DISP    ,
    M4U_CLNTMOD_SZ_VID     ,
    M4U_CLNTMOD_SZ_GDMA    ,
    M4U_CLNTMOD_SZ_IMG     ,
    M4U_CLNTMOD_SZ_LSCI    ,
    M4U_CLNTMOD_SZ_FLKI    ,
    M4U_CLNTMOD_SZ_LCEI    ,
    M4U_CLNTMOD_SZ_LCSO    ,
    M4U_CLNTMOD_SZ_ESFKO   ,
    M4U_CLNTMOD_SZ_AAO     ,
    M4U_CLNTMOD_SZ_AUDIO   ,    
    M4U_CLNTMOD_SZ_LCDC_UI ,    
    M4U_CLNTMOD_SZ_RESERVED 
};


typedef enum
{
	M4U_TEST_LEVEL_USER = 0,  // performance best, least verification
	M4U_TEST_LEVEL_ENG = 1,   // SQC used, more M4UMSG and M4UERR
	M4U_TEST_LEVEL_STRESS= 2  // stricker verification ,may use M4UERR instead M4UMSG sometimes, used for our own internal test
} M4U_TEST_LEVEL_ENUM;
M4U_TEST_LEVEL_ENUM gTestLevel = M4U_TEST_LEVEL_ENG;    

#define M4U_POW_ON_TRY(eModuleID) 
#define M4U_POW_OFF_TRY(eModuleID) 

//--------------------------------------Functions-----------------------------------------------------//

int m4u_module_2_larb(M4U_MODULE_ID_ENUM eModuleID)
{
    unsigned int larb = 0;
    switch(eModuleID)
    {
        case M4U_CLNTMOD_VENC      :
            larb = 0;
            break;
        case M4U_CLNTMOD_VDEC      :
            larb = 1;
            break;
        case M4U_CLNTMOD_ROT       :
        case M4U_CLNTMOD_OVL       :
        case M4U_CLNTMOD_LCDC_UI       :
        case M4U_CLNTMOD_WDMA      :
        case M4U_CLNTMOD_RDMA      :
        case M4U_CLNTMOD_CMDQ      :
        case M4U_CLNTMOD_DBI       :
        case M4U_CLNTMOD_G2D       :
            larb = 2;
            break;
        case M4U_CLNTMOD_JPGDEC    :
        case M4U_CLNTMOD_JPGENC    :
        case M4U_CLNTMOD_VIP       :
        case M4U_CLNTMOD_DISP      :
        case M4U_CLNTMOD_VID       :
        case M4U_CLNTMOD_GDMA      :
            larb = 3;
            break;
        case M4U_CLNTMOD_IMG       :
        case M4U_CLNTMOD_LSCI      :
        case M4U_CLNTMOD_FLKI      :
        case M4U_CLNTMOD_LCEI      :
        case M4U_CLNTMOD_LCSO      :
        case M4U_CLNTMOD_ESFKO     :
        case M4U_CLNTMOD_AAO       :
            larb = 4;
            break;
        case M4U_CLNTMOD_AUDIO     :
            larb = 5;
            break;


        default:
            M4UMSG("m4u_module_2_larb() fail, invalid moduleID=%d", eModuleID);
    }
    // M4UDBG("get_index_by_module() moduleID=%d, index=%d \n", moduleID, m4u_index);
    return larb;
}


static M4U_MODULE_ID_ENUM m4u_port_2_module(M4U_PORT_ID_ENUM portID)
{
    M4U_MODULE_ID_ENUM moduleID = M4U_CLNTMOD_UNKNOWN;
    switch(portID)
    {
        case M4U_PORT_VENC_RCPU               :
        case M4U_PORT_VENC_REF_LUMA           :
        case M4U_PORT_VENC_REF_CHROMA         :
        case M4U_PORT_VENC_DB_READ            :
        case M4U_PORT_VENC_DB_WRITE           :
        case M4U_PORT_VENC_CUR_LUMA           :
        case M4U_PORT_VENC_CUR_CHROMA         :
        case M4U_PORT_VENC_RD_COMV            :
        case M4U_PORT_VENC_SV_COMV            :
        case M4U_PORT_VENC_BSDMA              :
            moduleID = M4U_CLNTMOD_VENC;
            break;
        
        case M4U_PORT_HW_VDEC_MC_EXT          :
        case M4U_PORT_HW_VDEC_PP_EXT          :
        case M4U_PORT_HW_VDEC_AVC_MV_EXT      :
        case M4U_PORT_HW_VDEC_PRED_RD_EXT     :
        case M4U_PORT_HW_VDEC_PRED_WR_EXT     :
        case M4U_PORT_HW_VDEC_VLD_EXT         :
        case M4U_PORT_HW_VDEC_VLD2_EXT        :
            moduleID = M4U_CLNTMOD_VDEC;
            break;

        
        case M4U_PORT_ROT_EXT                 :
            moduleID = M4U_CLNTMOD_ROT;
            break;
        case M4U_PORT_OVL_CH0                 :
        case M4U_PORT_OVL_CH1                 :
        case M4U_PORT_OVL_CH2                 :
        case M4U_PORT_OVL_CH3                 :
            moduleID = M4U_CLNTMOD_OVL;
            break;
        case M4U_PORT_WDMA0                   :
        case M4U_PORT_WDMA1                   :
            moduleID = M4U_CLNTMOD_WDMA;
            break;
        case M4U_PORT_RDMA0                   :
        case M4U_PORT_RDMA1                   :
            moduleID = M4U_CLNTMOD_RDMA;
            break;
        case M4U_PORT_CMDQ                    :
            moduleID = M4U_CLNTMOD_CMDQ;
            break;
        case M4U_PORT_DBI                     :
            moduleID = M4U_CLNTMOD_DBI;
            break;
        case M4U_PORT_G2D                     :
            moduleID = M4U_CLNTMOD_G2D;
            break;

        case M4U_PORT_JPGDEC_BSDMA            :
        case M4U_PORT_JPGDEC_WDMA             :
            moduleID = M4U_CLNTMOD_JPGDEC;
            break;
        case M4U_PORT_JPGENC_BSDMA            :
        case M4U_PORT_JPGENC_RDMA             :
            moduleID = M4U_CLNTMOD_JPGENC;
            break;
        case M4U_PORT_VIPI                    :
        case M4U_PORT_VIP2I                   :
            moduleID = M4U_CLNTMOD_VIP;
            break;
        case M4U_PORT_DISPO                   :
        case M4U_PORT_DISPCO                  :
        case M4U_PORT_DISPVO                  :
            moduleID = M4U_CLNTMOD_DISP;
            break;
        case M4U_PORT_VIDO                    :
        case M4U_PORT_VIDCO                   :
        case M4U_PORT_VIDVO                   :
            moduleID = M4U_CLNTMOD_VID;
            break;
        case M4U_PORT_GDMA_SMI_RD             :
        case M4U_PORT_GDMA_SMI_WR             :
            moduleID = M4U_CLNTMOD_GDMA;
            break;

        case M4U_PORT_IMGI                    :
        case M4U_PORT_IMGCI                   :
        case M4U_PORT_IMGO                    :
        case M4U_PORT_IMG2O                   :
            moduleID = M4U_CLNTMOD_IMG;
            break;
        case M4U_PORT_LSCI                    :
            moduleID = M4U_CLNTMOD_LSCI;
            break;
        case M4U_PORT_FLKI                    :
            moduleID = M4U_CLNTMOD_FLKI;
            break;
        case M4U_PORT_LCEI                    :
            moduleID = M4U_CLNTMOD_LCEI;
            break;
        case M4U_PORT_LCSO                    :
            moduleID = M4U_CLNTMOD_LCSO;
            break;
        case M4U_PORT_ESFKO                   :
            moduleID = M4U_CLNTMOD_ESFKO;
            break;
        case M4U_PORT_AAO                     :
            moduleID = M4U_CLNTMOD_AAO;
            break;
        case M4U_PORT_AUDIO                   :
            moduleID = M4U_CLNTMOD_AUDIO;
            break;
        default:
        	M4UMSG("m4u_port_2_module() fail, invalid portID=%d", portID);
    }	
  
    return moduleID;    
}

int m4u_module_2_m4u_id(M4U_MODULE_ID_ENUM emoduleID)
{
    return larb_2_m4u_id(m4u_module_2_larb(emoduleID));
}


int m4u_invalid_tlb(M4U_ID_ENUM m4u_id,int L2_en, int isInvAll, unsigned int mva_start, unsigned int mva_end)
{
    unsigned int reg = 0;

    if(!isInvAll && (mva_end<mva_start))
    {
        m4u_aee_print("invalid tlb fail: s=0x%x,e=0x%x\n", mva_start, mva_end);
        isInvAll = 1;
    }

    if(L2_en)
        reg = F_MMUg_CTRL_INV_EN2;

    if(m4u_id == M4U_ID_0)
        reg |= F_MMUg_CTRL_INV_EN0;
    else if(m4u_id == M4U_ID_1)
        reg |= F_MMUg_CTRL_INV_EN1;
    else
    {
        reg |= F_MMUg_CTRL_INV_EN0;
        reg |= F_MMUg_CTRL_INV_EN1;
    }
    COM_WriteReg32(REG_MMUg_CTRL, reg); 
    
    if(isInvAll)
    {
        COM_WriteReg32(REG_MMUg_INVLD, F_MMUg_INV_ALL);
    }
    else 
    {
        COM_WriteReg32(REG_MMUg_INVLD_SA ,mva_start & (~0xfff));
        COM_WriteReg32(REG_MMUg_INVLD_EA, mva_end&(~0xfff));
        COM_WriteReg32(REG_MMUg_INVLD, F_MMUg_INV_RANGE);
    }
    
    if(L2_en)
    {
        if(isInvAll)
        {
            unsigned int event = 0;
            while(!(event=m4uHw_get_field_by_mask(0, REG_L2_GDC_STATE, F_L2_GDC_ST_EVENT_MSK)));
            m4uHw_set_field_by_mask(0, REG_L2_GDC_STATE, F_L2_GDC_ST_EVENT_MSK, 0);
        }
        else
        {
            while(!(m4uHw_get_field_by_mask(0, REG_L2_GPE_STATUS, F_L2_GPE_ST_RANGE_INV_DONE)));
            m4uHw_set_field_by_mask(0, REG_L2_GPE_STATUS, F_L2_GPE_ST_RANGE_INV_DONE, 0);
        }
    }

    return 0;
  
}

void m4u_invalid_tlb_all(M4U_ID_ENUM m4u_id, int L2_en)
{
    m4u_invalid_tlb(m4u_id, L2_en, 1, 0, 0);
}

void m4u_invalid_tlb_by_range(M4U_ID_ENUM m4u_id,
                                    int L2_en,
                                    unsigned int mva_start, 
                                    unsigned int mva_end)
{
    m4u_invalid_tlb(m4u_id, L2_en, 0, mva_start, mva_end);
}


void m4u_invalid_tlb_sec_by_range(M4U_ID_ENUM m4u_id,
                                    int L2_en,
                                    unsigned int mva_start, 
                                    unsigned int mva_end)
{
    unsigned int reg = 0;
    if(L2_en)
        reg = F_MMUg_CTRL_SEC_INV_EN2;

    if(m4u_id == M4U_ID_0)
        reg |= F_MMUg_CTRL_SEC_INV_EN0;
    else if(m4u_id == M4U_ID_1)
        reg |= F_MMUg_CTRL_SEC_INV_EN1;
    else
    {
        reg |= F_MMUg_CTRL_SEC_INV_EN0;
        reg |= F_MMUg_CTRL_SEC_INV_EN1;
    }

    m4uHw_set_field_by_mask(0, REG_MMUg_CTRL_SEC, ~F_MMUg_CTRL_SEC_DBG, reg);
    
    COM_WriteReg32(REG_MMUg_INVLD_SA_SEC,mva_start & (~0xfff));
    COM_WriteReg32(REG_MMUg_INVLD_EA_SEC, mva_end&(~0xfff));
    COM_WriteReg32(REG_MMUg_INVLD_SEC, F_MMUg_INV_SEC_RANGE);
    
    if(L2_en)
    {
        while(!(m4uHw_get_field_by_mask(0, REG_L2_GPE_STATUS_SEC, F_L2_GPE_ST_RANGE_INV_DONE_SEC)));
        m4uHw_set_field_by_mask(0, REG_L2_GPE_STATUS_SEC, F_L2_GPE_ST_RANGE_INV_DONE_SEC, 0);
    }
  
}

void m4u_L2_prefetch(unsigned int start, unsigned int end, int lock)
{
    unsigned int reg;
    COM_WriteReg32(REG_MMUg_INVLD_SA ,start & (~0xfff));
    COM_WriteReg32(REG_MMUg_INVLD_EA, end&(~0xfff));
    mb();
    reg = F_MMUg_CTRL_INV_EN2 | F_MMUg_CTRL_PRE_EN | (F_MMUg_CTRL_PRE_LOCK(!!lock));
    COM_WriteReg32(REG_MMUg_CTRL, reg);  
    
    while(!(m4uHw_get_field_by_mask(0, REG_L2_GPE_STATUS, F_L2_GPE_ST_PREFETCH_DONE)));
    m4uHw_set_field_by_mask(0, REG_L2_GPE_STATUS, F_L2_GPE_ST_PREFETCH_DONE, 0);
}


static int m4u_dump_maps(unsigned int addr)
{
    struct vm_area_struct *vma;
    
	M4UMSG("addr=0x%x, name=%s,pid=0x%x,", addr, current->comm, current->pid);

    vma = find_vma(current->mm, addr);
    if(vma == NULL)
    {
        M4UMSG("dump_maps fail: find_vma return NULL\n");
        return -1;
    }

	M4UMSG("find vma: 0x%08x-0x%08x\n", (unsigned int)(vma->vm_start), (unsigned int)(vma->vm_end));

	return 0;
}




//file operations
static int MTK_M4U_open(struct inode * a_pstInode, struct file * a_pstFile)
{
    garbage_node_t * pNode;

    M4UDBG("enter MTK_M4U_open() process:%s\n",current->comm);

    //Allocate and initialize private data
    a_pstFile->private_data = kmalloc(sizeof(garbage_node_t) , GFP_ATOMIC);

    if(NULL == a_pstFile->private_data)
    {
        M4UMSG("Not enough entry for M4U open operation\n");
        return -ENOMEM;
    }

    pNode = (garbage_node_t *)a_pstFile->private_data;
    mutex_init(&(pNode->dataMutex));
    mutex_lock(&(pNode->dataMutex));
    pNode->open_pid = current->pid;
    pNode->open_tgid = current->tgid;
    pNode->OwnResource = 0;
    pNode->isM4uDrvConstruct = 0;
    pNode->isM4uDrvDeconstruct = 0;    
    INIT_LIST_HEAD(&(pNode->mvaList));
    mutex_unlock(&(pNode->dataMutex));

    return 0;
}

static int MTK_M4U_release(struct inode * a_pstInode, struct file * a_pstFile)
{
    struct list_head *pListHead, *ptmp;
    garbage_node_t *pNode = a_pstFile->private_data;
    mva_info_t *pList;
    M4UDBG("enter MTK_M4U_release() process:%s\n",current->comm);

    mutex_lock(&(pNode->dataMutex));

    if(pNode->isM4uDrvConstruct==0 || pNode->isM4uDrvDeconstruct==0)
    {
        M4UMSG("warning on close: construct=%d, deconstruct=%d, open_pid=%d, cur_pid=%d\n",
            pNode->isM4uDrvConstruct, pNode->isM4uDrvDeconstruct,
            pNode->open_pid, current->pid);
        M4UMSG("open->tgid=%d, cur->tgid=%d, cur->mm=0x%x\n",
            pNode->open_tgid, current->tgid, current->mm);
    }
    
    pListHead = pNode->mvaList.next;
    while(pListHead!= &(pNode->mvaList))
    {
        ptmp = pListHead;
        pListHead = pListHead->next;
        pList = container_of(ptmp, mva_info_t, link);
        M4UMSG("warnning: clean garbage at m4u close: module=%s,va=0x%x,mva=0x%x,size=%d\n",
            m4u_get_module_name(pList->eModuleId),pList->bufAddr,pList->mvaStart,pList->size);

        list_del(ptmp);
        //kfree(pList); notes: m4u_dealloc_mva will help to free listp

        //if registered but never has chance to query this buffer (we will allocate mva in query_mva)
        //then the mva will be 0, and MVA_REGION_REGISTER flag will be set.
        //we don't call deallocate for this mva, because it's 0 ...
        if(pList->mvaStart != 0)        
        {
            int ret;
            ret = m4u_dealloc_mva(pList->eModuleId, pList->bufAddr, pList->size, pList->mvaStart);
            if(ret)
                m4u_free_garbage_list(pList);
        }
        else
        {
            if(!(pList->flags&MVA_REGION_REGISTER))
                M4UERR("warning: in garbage reclaim: mva==0, but MVA_REGION_REGISTER is not set!! flag=0x%x\n", pList->flags);
        }
    }

    mutex_unlock(&(pNode->dataMutex));
    
    if(NULL != a_pstFile->private_data)
    {
        kfree(a_pstFile->private_data);
        a_pstFile->private_data = NULL;
    }
        
    return 0;
}

static int MTK_M4U_flush(struct file * a_pstFile , fl_owner_t a_id)
{
    M4UDBG("enter MTK_M4U_flush() process:%s\n", current->comm); 
    return 0;
}


static long MTK_M4U_ioctl(struct file * a_pstFile,
								unsigned int a_Command,
								unsigned long a_Param)
{
    int ret = 0;
    M4U_MOUDLE_STRUCT m4u_module;
    M4U_PORT_STRUCT m4u_port;
    M4U_PORT_STRUCT_ROTATOR m4u_port_rotator;
    M4U_PORT_ID_ENUM PortID;
    M4U_MODULE_ID_ENUM ModuleID;
    M4U_WRAP_DES_T m4u_wrap_range;
    M4U_CACHE_STRUCT m4u_cache_data;
    garbage_node_t *pNode = a_pstFile->private_data;

    switch(a_Command)
    {
        case MTK_M4U_T_POWER_ON :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&ModuleID, (void*)a_Param , sizeof(unsigned int));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_POWER_ON, copy_from_user failed, %d\n", ret);
            	return -EFAULT;
            }  
            ret = m4u_power_on(ModuleID);
        break;

        case MTK_M4U_T_POWER_OFF :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&ModuleID, (void*)a_Param , sizeof(unsigned int));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_POWER_OFF, copy_from_user failed, %d\n", ret);
            	return -EFAULT;
            }  
            ret = m4u_power_off(ModuleID);
        break;

        case MTK_M4U_T_ALLOC_MVA :			
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_module, (void*)a_Param , sizeof(M4U_MOUDLE_STRUCT));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_ALLOC_MVA, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            }  

            if(m4u_module.MVAStart == -1) //work around for wrap layer
            {
                m4u_module.MVAStart = m4u_user_v2p(m4u_module.BufAddr);
                M4UMSG("alloc_mva_pmem: module=%d,va=0x%x, pa=0x%x\n",
                    m4u_module.eModuleID, m4u_module.BufAddr, m4u_module.MVAStart);
                ret = 0;
            }
            else
            {
                mva_info_t *pList = NULL;
                unsigned int query_mva = 0;
                pList = m4u_alloc_garbage_list(0, 
                        m4u_module.BufSize, m4u_module.eModuleID, m4u_module.BufAddr, 
                        MVA_REGION_FLAG_NONE, m4u_module.security, m4u_module.cache_coherent);

                m4u_query_mva(m4u_module.eModuleID, 
            			  m4u_module.BufAddr, 
            			  m4u_module.BufSize, 
            			  &(query_mva),
            			  a_pstFile); 
                
                ret = __m4u_alloc_mva(pList); 

                if(query_mva != 0)
                {
                    M4UDBG("warning: alloc mva while exists, module=%d, BufAddr=0x%x, BufSize=%d, query=0x%x, mva=0x%x\n",
                        m4u_module.eModuleID, m4u_module.BufAddr, m4u_module.BufSize, query_mva, pList->mvaStart);			
                }
                

                if(ret)
                {
                    m4u_module.MVAStart = 0;
                    //notes: mva_info node will be freed in __m4u_alloc_mva if failed.
                	M4UMSG(" MTK_M4U_T_ALLOC_MVA, m4u_alloc_mva failed: %d\n", ret);
                	return -EFAULT;
                }  
                else
                {
                    m4u_module.MVAStart = pList->mvaStart;
                    m4u_add_to_garbage_list(a_pstFile, pList);      
                }

            }
            
            ret = copy_to_user(&(((M4U_MOUDLE_STRUCT*)a_Param)->MVAStart), &(m4u_module.MVAStart) , sizeof(unsigned int));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_ALLOC_MVA, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            }  
        break;

        case MTK_M4U_T_QUERY_MVA :			
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_module, (void*)a_Param , sizeof(M4U_MOUDLE_STRUCT));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_QUERY_MVA, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            }  
            M4UDBG("-MTK_M4U_T_QUERY_MVA, module_id=%d, BufAddr=0x%x, BufSize=%d \r\n",
            		m4u_module.eModuleID, m4u_module.BufAddr, m4u_module.BufSize );			
            
            m4u_query_mva(m4u_module.eModuleID, 
            			  m4u_module.BufAddr, 
            			  m4u_module.BufSize, 
            			  &(m4u_module.MVAStart),
            			  a_pstFile); 
                       
            ret = copy_to_user(&(((M4U_MOUDLE_STRUCT*)a_Param)->MVAStart), &(m4u_module.MVAStart) , sizeof(unsigned int));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_QUERY_MVA, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            }  
            M4UDBG("MTK_M4U_T_QUERY_MVA,  m4u_module.MVAStart=0x%x \n", m4u_module.MVAStart);
        break;
        
        case MTK_M4U_T_DEALLOC_MVA :
        {
            mva_info_t *pMvaInfo;
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_module, (void*)a_Param , sizeof(M4U_MOUDLE_STRUCT));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_DEALLOC_MVA, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            } 
            M4UDBG("MTK_M4U_T_DEALLOC_MVA, eModuleID:%d, VABuf:0x%x, Length:%d, MVAStart=0x%x \r\n",
            	m4u_module.eModuleID, m4u_module.BufAddr, m4u_module.BufSize, m4u_module.MVAStart); 


            pMvaInfo = m4u_delete_from_garbage_list(&m4u_module, a_pstFile);
            
            if(pMvaInfo==NULL)
            {
                M4UMSG("error to dealloc mva: id=%s,va=0x%x,size=%d,mva=0x%x\n", 
                    m4u_get_module_name(m4u_module.eModuleID), m4u_module.BufAddr,
                    m4u_module.BufSize, m4u_module.MVAStart);
                m4u_print_mva_list(a_pstFile, "in deallocate");
            }
            else
            {
                //if user register a buffer without query it,
                //then we never allocated a real mva for it,
                //when deallocate, m4u_module.MVAStart==0, we think this is right.
                if(m4u_module.MVAStart!=0)
                {
                    m4u_dealloc_mva(m4u_module.eModuleID, 
                    				m4u_module.BufAddr, 
                    				m4u_module.BufSize,
                    				m4u_module.MVAStart);
                }
                else
                {
                    M4UMSG("warning: deallocat a registered buffer, before any query!\n");
                    M4UMSG("error to dealloc mva: id=%s,va=0x%x,size=%d,mva=0x%x\n", 
                        m4u_get_module_name(m4u_module.eModuleID), m4u_module.BufAddr,
                        m4u_module.BufSize, m4u_module.MVAStart);
                }
                //m4u_free_garbage_list(pMvaInfo);
            }

        }				
        break;
            
        case MTK_M4U_T_MANUAL_INSERT_ENTRY :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_module, (void*)a_Param , sizeof(M4U_MOUDLE_STRUCT));
            if(ret)
            {
            	M4UERR(" MTK_M4U_Manual_Insert_Entry, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            } 
            M4UDBG(" ManualInsertTLBEntry, eModuleID:%d, Entry_MVA:0x%x, locked:%d\r\n", 
            	m4u_module.eModuleID, m4u_module.EntryMVA, m4u_module.Lock);
            
            ret = m4u_manual_insert_entry(m4u_module.eModuleID,
											m4u_module.EntryMVA,
											!!(m4u_module.security),
											m4u_module.Lock);
        break;

        case MTK_M4U_T_INSERT_TLB_RANGE :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_module, (void*)a_Param , sizeof(M4U_MOUDLE_STRUCT));
            if(ret)
            {
            	M4UERR("m4u_insert_seq_range , copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            } 
            M4UDBG("m4u_insert_seq_range , eModuleID:%d, MVAStart:0x%x, MVAEnd:0x%x, ePriority=%d \r\n", 
            	m4u_module.eModuleID, m4u_module.MVAStart, m4u_module.MVAEnd, m4u_module.ePriority); 
            
            ret = m4u_insert_seq_range(m4u_module.eModuleID, 
            				  m4u_module.MVAStart, 
            				  m4u_module.MVAEnd, 
            				  m4u_module.ePriority,
            				  m4u_module.entryCount);
        break;

        case MTK_M4U_T_INVALID_TLB_RANGE :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_module, (void*)a_Param , sizeof(M4U_MOUDLE_STRUCT));
            if(ret)
            {
            	M4UERR(" MTK_M4U_Invalid_TLB_Range, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            } 
            M4UDBG("MTK_M4U_Invalid_TLB_Range(), eModuleID:%d, MVAStart=0x%x, MVAEnd=0x%x \n", 
            		m4u_module.eModuleID, m4u_module.MVAStart, m4u_module.MVAEnd);
                      	
            ret = m4u_invalid_seq_range(m4u_module.eModuleID,
                                            m4u_module.MVAStart, 
                							 m4u_module.MVAEnd);
        break;

        case MTK_M4U_T_INVALID_TLB_ALL :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&ModuleID, (void*)a_Param , sizeof(unsigned int));
            if(ret)
            {
            	M4UERR(" MTK_M4U_Invalid_TLB_Range, copy_from_user failed, %d\n", ret);
            	return -EFAULT;
            }           		
            //ret = m4u_invalid_tlb_all(ModuleID);
        break;

        case MTK_M4U_T_DUMP_REG :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&ModuleID, (void*)a_Param , sizeof(unsigned int));
            if(ret)
            {
            	M4UERR(" MTK_M4U_Invalid_TLB_Range, copy_from_user failed, %d\n", ret);
            	return -EFAULT;
            } 
            m4u_dump_main_tlb_tags(m4u_module_2_m4u_id(ModuleID));
            ret = m4u_dump_reg(ModuleID);
            
        break;

        case MTK_M4U_T_DUMP_INFO :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&ModuleID, (void*)a_Param , sizeof(unsigned int));
            if(ret)
            {
            	M4UERR(" MTK_M4U_Invalid_TLB_Range, copy_from_user failed, %d\n", ret);
            	return -EFAULT;
            } 
            ret = m4u_dump_info(m4u_module_2_m4u_id(ModuleID));
            m4u_dump_pagetable(ModuleID);
            
        break;

        case MTK_M4U_T_CACHE_SYNC :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_cache_data, (void*)a_Param , sizeof(M4U_CACHE_STRUCT));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_CACHE_INVALID_AFTER_HW_WRITE_MEM, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            } 
            M4UDBG("MTK_M4U_T_CACHE_INVALID_AFTER_HW_WRITE_MEM(), moduleID=%d, eCacheSync=%d, buf_addr=0x%x, buf_length=0x%x \n", 
            		m4u_cache_data.eModuleID, m4u_cache_data.eCacheSync, m4u_cache_data.BufAddr, m4u_cache_data.BufSize);

            switch(m4u_cache_data.eCacheSync)  
            {
            	case M4U_CACHE_FLUSH_BEFORE_HW_WRITE_MEM:
                case M4U_CACHE_FLUSH_BEFORE_HW_READ_MEM:
                		ret = m4u_dma_cache_maint(m4u_cache_data.eModuleID, (unsigned int*)(m4u_cache_data.BufAddr), m4u_cache_data.BufSize, M4U_DMA_READ_WRITE);
                		break;

                case M4U_CACHE_CLEAN_BEFORE_HW_READ_MEM:
                		ret = m4u_dma_cache_maint(m4u_cache_data.eModuleID, (unsigned int*)(m4u_cache_data.BufAddr), m4u_cache_data.BufSize, M4U_DMA_READ);
                		break;
                		
                case M4U_CACHE_INVALID_AFTER_HW_WRITE_MEM:
                		ret = m4u_dma_cache_maint(m4u_cache_data.eModuleID, (unsigned int*)(m4u_cache_data.BufAddr), m4u_cache_data.BufSize, M4U_DMA_WRITE);
                		break;
                default:
                	M4UMSG("error: MTK_M4U_T_CACHE_SYNC, invalid eCacheSync=%d, module=%d \n", m4u_cache_data.eCacheSync, m4u_cache_data.eModuleID);  
            }            
        break;

        case MTK_M4U_T_CONFIG_PORT :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_port, (void*)a_Param , sizeof(M4U_PORT_STRUCT));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_CONFIG_PORT, copy_from_user failed: %d \n", ret);
            	return -EFAULT;
            } 
            M4UDBG("ePortID=%d, Virtuality=%d, Security=%d, Distance=%d, Direction=%d \n",
                m4u_port.ePortID, m4u_port.Virtuality, m4u_port.Security, m4u_port.Distance, m4u_port.Direction);
            
            ret = m4u_config_port(&m4u_port);
        break;                                

        case MTK_M4U_T_CONFIG_PORT_ROTATOR:
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_port_rotator, (void*)a_Param , sizeof(M4U_PORT_STRUCT_ROTATOR));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_CONFIG_PORT_ROTATOR, copy_from_user failed: %d \n", ret);
            	return -EFAULT;
            } 
            ret = m4u_config_port_rotator(&m4u_port_rotator);
        break; 
      
        case MTK_M4U_T_CONFIG_ASSERT :
            // todo
        break;

        case MTK_M4U_T_INSERT_WRAP_RANGE :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_wrap_range, (void*)a_Param , sizeof(M4U_WRAP_DES_T));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_INSERT_WRAP_RANGE, copy_from_user failed: %d \n", ret);
            	return -EFAULT;
            } 
            M4UDBG("PortID=%d, eModuleID=%d, MVAStart=0x%x, MVAEnd=0x%x \n",
                    m4u_wrap_range.ePortID, 
                    m4u_wrap_range.eModuleID,
                    m4u_wrap_range.MVAStart, 
                    m4u_wrap_range.MVAEnd );
            
            ret = m4u_insert_wrapped_range(m4u_wrap_range.eModuleID,
                                  m4u_wrap_range.ePortID, 
                                  m4u_wrap_range.MVAStart, 
                                  m4u_wrap_range.MVAEnd);
        break;   

        case MTK_M4U_T_MONITOR_START :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&PortID, (void*)a_Param , sizeof(unsigned int));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_MONITOR_START, copy_from_user failed, %d\n", ret);
            	return -EFAULT;
            } 
           	ret = m4u_monitor_start(PortID);

        break;

        case MTK_M4U_T_MONITOR_STOP :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&PortID, (void*)a_Param , sizeof(unsigned int));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_MONITOR_STOP, copy_from_user failed, %d\n", ret);
            	return -EFAULT;
            } 
            ret = m4u_monitor_stop(PortID);
        break;

        case MTK_M4U_T_RESET_MVA_RELEASE_TLB :
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&ModuleID, (void*)a_Param , sizeof(ModuleID));
            if(ret)
            {
              M4UERR(" MTK_M4U_T_RESET_MVA_RELEASE_TLB, copy_from_user failed: %d\n", ret);
              return -EFAULT;
            }             
            ret = m4u_reset_mva_release_tlb(ModuleID);            
        break;

        case MTK_M4U_T_M4UDrv_CONSTRUCT:
            mutex_lock(&(pNode->dataMutex));
            pNode->isM4uDrvConstruct = 1;
            mutex_unlock(&(pNode->dataMutex));

        break;
        
        case MTK_M4U_T_M4UDrv_DECONSTRUCT:
            mutex_lock(&(pNode->dataMutex));
            pNode->isM4uDrvDeconstruct = 1;
            mutex_unlock(&(pNode->dataMutex));
        break;

        case MTK_M4U_T_DUMP_PAGETABLE:
        do{
            unsigned int mva, va, page_num, size, i;

            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_module, (void*)a_Param , sizeof(M4U_MOUDLE_STRUCT));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_ALLOC_MVA, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            }  
            mva = m4u_module.MVAStart;
            va = m4u_module.BufAddr;
            size = m4u_module.BufSize;
            page_num = (size + (va&0xfff))/DEFAULT_PAGE_SIZE;

            M4UMSG("M4U dump pagetable in ioctl: mva=0x%x, size=0x%x===>\n", mva,size);
            m4u_dump_pagetable_range(mva, page_num);
            printk("\n");
            
            M4UMSG("M4U dump PA by VA in ioctl: va=0x%x, size=0x%x===>\n", va,size);
            printk("0x%08x: ", va);
            for(i=0; i<page_num; i++)
            {
                printk("0x%08x, ", m4u_user_v2p(va+i*M4U_PAGE_SIZE));
                if((i+1)%8==0)
                {
                	 printk("\n 0x%08x: ", (va+((i+1)<<12)));
                }
            }
            printk("\n"); 


            M4UMSG("=========  compare these automaticly =======>\n");
            for(i=0; i<page_num; i++)
            {
                unsigned int pa, entry;
                pa = m4u_user_v2p(va+i*M4U_PAGE_SIZE);
                entry = *(unsigned int*)mva_pteAddr_nonsec((mva+i*M4U_PAGE_SIZE));

                if((pa&(~0xfff)) != (pa&(~0xfff)))
                {
                    M4UMSG("warning warning!! va=0x%x,mva=0x%x, pa=0x%x,entry=0x%x\n",
                        va+i*M4U_PAGE_SIZE, mva+i*M4U_PAGE_SIZE, pa, entry);
                }
            }

        }while(0);
            
        break;
        
        case MTK_M4U_T_REGISTER_BUFFER:
        {
            mva_info_t *pMvaInfo;
            M4U_ASSERT(a_Param);
            ret = copy_from_user(&m4u_module, (void*)a_Param , sizeof(M4U_MOUDLE_STRUCT));
            if(ret)
            {
            	M4UERR(" MTK_M4U_T_ALLOC_MVA, copy_from_user failed: %d\n", ret);
            	return -EFAULT;
            }  
            M4ULOG("-MTK_M4U_T_REGISTER_BUF, module_id=%d, BufAddr=0x%x, BufSize=%d \r\n",
            		m4u_module.eModuleID, m4u_module.BufAddr, m4u_module.BufSize );			
            pMvaInfo->bufAddr = m4u_module.BufAddr;
            pMvaInfo->mvaStart = 0;
            pMvaInfo->size = m4u_module.BufSize;
            pMvaInfo->eModuleId = m4u_module.eModuleID;
            pMvaInfo->flags = MVA_REGION_REGISTER;
            pMvaInfo->security = m4u_module.security;
            pMvaInfo->cache_coherent = m4u_module.cache_coherent;
            m4u_add_to_garbage_list(a_pstFile, pMvaInfo);
        }
        break;

        case MTK_M4U_T_CACHE_FLUSH_ALL:
            m4u_dma_cache_flush_all();
        break;

        case MTK_M4U_T_REG_GET:
        {
            unsigned int para[2];
            M4U_ASSERT(a_Param);
            ret = copy_from_user(para, (void*)a_Param , 2*sizeof(unsigned int));
            
            para[1] = COM_ReadReg32(para[0]);

            ret=copy_to_user((void*)a_Param, para, 2*sizeof(unsigned int));
        }
        break;


        case MTK_M4U_T_REG_SET:
        {
            unsigned int para[2];
            M4U_ASSERT(a_Param);
            ret = copy_from_user(para, (void*)a_Param , 2*sizeof(unsigned int));
            
            COM_WriteReg32(para[0], para[1]);
        }

        break;

        default :
            M4UMSG("MTK M4U ioctl : No such command!!\n");
            ret = -EINVAL;
        break;        
    }

    return ret;
}

static int MTK_M4U_mmap(struct file * a_pstFile, struct vm_area_struct * a_pstVMArea)
{
    garbage_node_t * pstLog;
    pstLog = (garbage_node_t *)a_pstFile->private_data;

    if(NULL == pstLog)
    {
        M4UMSG("Private data is null in mmap operation. HOW COULD THIS HAPPEN ??\n");
        //return -1;
    }
    M4UMSG("MTK_M4U_Mmap, a_pstVMArea=0x%x, vm_start=0x%x, vm_pgoff=0x%x, size=0x%x, vm_page_prot=0x%x \n", 
                       (unsigned int)a_pstVMArea , 
                       (unsigned int)a_pstVMArea->vm_start , 
                       (unsigned int)a_pstVMArea->vm_pgoff , 
                       (unsigned int)(a_pstVMArea->vm_end - a_pstVMArea->vm_start) , 
                       (unsigned int)a_pstVMArea->vm_page_prot );

    {
        a_pstVMArea->vm_page_prot = pgprot_noncached(a_pstVMArea->vm_page_prot);
        if(remap_pfn_range(a_pstVMArea , 
	                   a_pstVMArea->vm_start , 
	                   a_pstVMArea->vm_pgoff , 
	                   (a_pstVMArea->vm_end - a_pstVMArea->vm_start) , 
	                   a_pstVMArea->vm_page_prot)<0)
        {
            M4UMSG("MMAP failed!!\n");
            return -1;
        }
    }	

    return 0;
}

static const struct file_operations g_stMTK_M4U_fops = 
{
	.owner = THIS_MODULE,
	.open = MTK_M4U_open,
	.release = MTK_M4U_release,
	.flush = MTK_M4U_flush,
	.unlocked_ioctl = MTK_M4U_ioctl,
	.mmap = MTK_M4U_mmap
};


volatile static int gM4u_L2_invalid_range_done = 0;
volatile static int gM4u_L2_invalid_all_done = 0;

static irqreturn_t MTK_M4U_L2_isr(int irq, void *dev_id)
{
    unsigned int regval = COM_ReadReg32(REG_L2_GPE_STATUS);
    M4UMSG("L2 interrupt happens!!! irq=%d, status=0x%x\n", irq, regval);

    //clear interrupt
    COM_WriteReg32(REG_L2_GPE_STATUS, 0);

    return IRQ_HANDLED;
}

static int __m4u_dump_rs_info(unsigned int va[], unsigned int pa[], unsigned int st[])
{
    int i;

    M4UINFO("m4u dump RS information =====>\n");
    M4UINFO("mva   valid   port-id   pa   larb-id  write  other-status  \n");
    for(i=0; i<MMU_TOTAL_RS_NR; i++)
    {
        M4UINFO("0x%-8x %d %-2d 0x%-8x %d %d 0x%-8x",
            F_MMU_RSx_VA_GET(va[i]), F_MMU_RSx_VA_VALID(va[i]),
            F_MMU_RSx_VA_PID(va[i]), pa[i], F_MMU_RSx_ST_LID(st[i]),
            F_MMU_RSx_ST_WRT(st[i]), F_MMU_RSx_ST_OTHER(st[i])
        );
    }
    M4UINFO("m4u dump RS information done =====>\n");
    return 0;
}


static int m4u_dump_rs_info(int m4u_index)
{
    unsigned int m4u_base = gM4UBaseAddr[m4u_index];
    int i;
    unsigned int va[MMU_TOTAL_RS_NR], pa[MMU_TOTAL_RS_NR], st[MMU_TOTAL_RS_NR];

    for(i=0; i<MMU_TOTAL_RS_NR; i++)
    {
        va[i] = ioread32(m4u_base+REG_MMU_RSx_VA(i));
        pa[i] = ioread32(m4u_base+REG_MMU_RSx_PA(i));
        st[i] = ioread32(m4u_base+REG_MMU_RSx_ST(i));
    }
    mb();  

    __m4u_dump_rs_info(va, pa, st);
    return 0;
}

static irqreturn_t MTK_M4U_isr(int irq, void *dev_id)
{
    unsigned int m4u_base, m4u_index;
    unsigned int IntrSrc, faultMva, port_regval, i;	
    int portID, larbID;
    unsigned int main_tags[M4U_MAIN_TLB_NR];
    unsigned int pfh_tags[M4U_MAIN_TLB_NR];
    unsigned int rs_va[MMU_TOTAL_RS_NR], rs_pa[MMU_TOTAL_RS_NR], rs_st[MMU_TOTAL_RS_NR];
    
    m4u_index = irq-MT6589_MMU0_IRQ_ID;
    m4u_base = gM4UBaseAddr[m4u_index];
    
    IntrSrc = M4U_ReadReg32(m4u_base, REG_MMU_FAULT_ST) & 0xFF; 		
    faultMva = M4U_ReadReg32(m4u_base, REG_MMU_FAULT_VA);
    port_regval = M4U_ReadReg32(m4u_base, REG_MMU_INT_ID);

    for(i=0; i<M4U_MAIN_TLB_NR; i++)
    {
        main_tags[i] = ioread32(m4u_base+REG_MMU_MAIN_TAG(i));
        pfh_tags[i] = ioread32(m4u_base+REG_MMU_PFH_TAG(i));
    }

    if(g_debug_dump_rs_in_isr)
    {
        for(i=0; i<MMU_TOTAL_RS_NR; i++)
        {
            rs_va[i] = ioread32(m4u_base+REG_MMU_RSx_VA(i));
            rs_pa[i] = ioread32(m4u_base+REG_MMU_RSx_PA(i));
            rs_st[i] = ioread32(m4u_base+REG_MMU_RSx_ST(i));
        }
    }
    mb();

    m4u_disable_error_hang(m4u_index);

    if(0==IntrSrc)
    {
        M4UMSG("warning: MTK_M4U_isr, larbID=%d, but REG_MMU_FAULT_ST=0x0 \n", m4u_index);
        m4u_clear_intr(m4u_base);
        return IRQ_HANDLED;
    }
            
    if(IntrSrc&F_INT_TRANSLATION_FAULT)
    {
        unsigned int *faultPTE;
        M4U_PORT_ID_ENUM m4u_port;
        if(g_debug_recover_pagetable_TF)
        {
            if(faultMva<M4U_MVA_MAX )
            {
                faultPTE = mva_pteAddr(faultMva);
                if(!(*faultPTE & F_DESC_VALID)) 
                    *faultPTE = gM4U_align_page_pa|F_DESC_VALID;

                //to-do: add secure solution
                m4u_invalid_tlb_all(m4u_index, gM4U_L2_enable);            	  
            }
        }        
        
        portID = F_INT_ID_TF_PORT_ID(port_regval);
        larbID = 6-F_INT_ID_TF_LARB_ID(port_regval);

        m4u_port = larb_port_2_m4u_port(larbID, portID);
        if(m4u_port != M4U_PORT_HW_VDEC_AVC_MV_EXT)
        {            
            MMProfileLogEx(M4U_MMP_Events[PROFILE_M4U_ERROR], MMProfileFlagPulse, m4u_port, faultMva);
            
            m4u_aee_print("translation fault: larb=%d,module=%s,port=%s,mva=0x%x\n", 
                larbID, m4u_get_module_name(m4u_port_2_module(m4u_port)),
                m4u_get_port_name(m4u_port),
                faultMva);
        }

        M4UMSG("translation fault: larb=%d,port=%s, fault_mva=0x%x\n", 
            larbID, m4u_get_port_name(larb_port_2_m4u_port(larbID, portID)), faultMva);

        if(faultMva<M4U_MVA_MAX-0x1000 && faultMva>0x40000)
        {
            unsigned int *pteStart;
            pteStart = mva_pteAddr(faultMva);
            
            M4UINFO("pagetable @ 0x%x: 0x%x,0x%x,0x%x\n",faultMva,pteStart[-1], pteStart[0],pteStart[1]);
        }

        m4u_dump_user_addr_register(m4u_port);  
        
        //search invalid main tlb
        {
            unsigned int mva;// des;
            M4UINFO("search main tlb=>\n");
            for(i=0;i<M4U_MAIN_TLB_NR;i++)
            {
                mva = main_tags[i];
                if((mva&(F_MAIN_TLB_VALID_BIT|F_MAIN_TLB_INV_DES_BIT))
                    == (F_MAIN_TLB_VALID_BIT|F_MAIN_TLB_INV_DES_BIT) )
                {
                    //des = m4u_get_main_descriptor(m4u_base,i);
                    printk(KERN_INFO"%d:0x%x ", i,mva&F_MAIN_TLB_VA_MSK);
                }
            }
            //printk(KERN_INFO"\n");
        }

        //search invalid pfh tlb
        {
            unsigned int tag,tag_valid,des_invalid;
            M4UINFO("search pfh tlb=>\n");
            for(i=0;i<M4U_MAIN_TLB_NR;i++)
            {
                tag = pfh_tags[i];
                tag_valid = F_PFH_TAG_VALID(tag);
                des_invalid = F_PFH_TAG_DESC_VALID(tag);
                
                if((tag_valid & des_invalid)!=0)
                {
                    //des = m4u_get_main_descriptor(m4u_base,i);
                    printk(KERN_INFO"%d:0x%x ", i,tag);
                }
            }
            //printk(KERN_INFO"\n");
        }
        
        if(g_debug_dump_rs_in_isr)
            __m4u_dump_rs_info(rs_va, rs_pa, rs_st);
        
    } 
    if(IntrSrc&F_INT_TLB_MULTI_HIT_FAULT)
    {
        m4u_dump_main_tlb_des(m4u_index);
        m4u_dump_pfh_tlb_des(m4u_index);
        M4UERR("multi-hit error! \n");
    } 
    if(IntrSrc&F_INT_INVALID_PHYSICAL_ADDRESS_FAULT)
    {
        if(!(IntrSrc&F_INT_TRANSLATION_FAULT))
        {
            if(faultMva<M4U_MVA_MAX-0x1000 && faultMva>0x40000)
            {
                unsigned int *pteStart;
                pteStart = mva_pteAddr(faultMva);
                M4UINFO("pagetable @ 0x%x: 0x%x,0x%x,0x%x\n",faultMva,pteStart[-1], pteStart[0],pteStart[1]);
            }

            m4u_aee_print("invalid PA:0x%x->0x%x\n", faultMva, M4U_ReadReg32(m4u_base, REG_MMU_INVLD_PA)); 
            m4u_dump_main_tlb_des(m4u_index);
            m4u_dump_pfh_tlb_des(m4u_index);
        }
        else
        {
           M4UMSG("invalid PA:0x%x->0x%x\n", faultMva, M4U_ReadReg32(m4u_base, REG_MMU_INVLD_PA)); 
        }
    } 
    if(IntrSrc&F_INT_ENTRY_REPLACEMENT_FAULT)
    {
        unsigned char lock_cnt[M4U_CLNTMOD_MAX] = {0};
        M4UERR("error: Entry replacement fault! No free TLB, TLB are locked by: ");
        for(i=0;i<g4M4UTagCount[m4u_index];i++)
        {
           lock_cnt[mva2module(M4U_ReadReg32(m4u_base, REG_MMU_MAIN_TAG(i))&(~0xfff))]++;
        } 
        for(i=0;i<M4U_CLNTMOD_MAX;i++)
        {
           if(0!=lock_cnt[i])
           {
               printk("%s(lock=%d), ", m4u_get_module_name(i), lock_cnt[i]);	
           }
        }   
        printk("\n");       
    } 
    if(IntrSrc&F_INT_TABLE_WALK_FAULT)
    {
        M4UERR("error:  Table walk fault! pageTable start addr:0x%x, 0x%x\n", pt_pa_nonsec, pt_pa_sec);        	    
    } 
    if(IntrSrc&F_INT_TLB_MISS_FAULT)
    {
        M4UERR("error:  TLB miss fault! \n");        	  	
    } 
    if(IntrSrc&F_INT_PFH_DMA_FIFO_OVERFLOW)
    {
        M4UERR("error:  Prefetch DMA fifo overflow fault! \n");
    } 

    //m4u_print_active_port(m4u_index);
    if(g_debug_print_detail_in_isr)
        m4u_dump_mva_info();
    
    m4u_invalid_tlb_all(m4u_index, gM4U_L2_enable);            	  

    m4u_clear_intr(m4u_base); 
    if(g_debug_enable_error_hang)
        m4u_enable_error_hang(m4u_index);
    
    return IRQ_HANDLED;
}


unsigned int SMI_reg_init(void)
{
    return 0;
}

static struct class *pM4uClass = NULL;
static struct device* m4uDevice = NULL;
static int m4u_probe(struct platform_device *pdev)
{
    int ret;

    M4UMSG("MTK_M4U_Init\n");

    ret = register_chrdev_region(g_MTKM4Udevno, 1, M4U_DEVNAME);	
    if(ret)
        M4UMSG("error: can't get major number for m4u device\n");
    else
        M4UMSG("Get M4U Device Major number (%d)\n", ret);

    g_pMTKM4U_CharDrv = cdev_alloc();
    g_pMTKM4U_CharDrv->owner = THIS_MODULE;
    g_pMTKM4U_CharDrv->ops = &g_stMTK_M4U_fops;
    ret = cdev_add(g_pMTKM4U_CharDrv, g_MTKM4Udevno, 1);	

    //create /dev/M4U_device automaticly
    pM4uClass = class_create(THIS_MODULE, M4U_DEVNAME);
    if (IS_ERR(pM4uClass)) {
        int ret = PTR_ERR(pM4uClass);
        M4UMSG("Unable to create class, err = %d", ret);
        return ret;
    }
    m4uDevice = device_create(pM4uClass, NULL, g_MTKM4Udevno, NULL, M4U_DEVNAME);

    pmodule_current_size = (int*)kmalloc(M4U_CLIENT_MODULE_NUM*4, GFP_KERNEL|__GFP_ZERO);
    pmodule_max_size = (int*)kmalloc(M4U_CLIENT_MODULE_NUM*4, GFP_KERNEL|__GFP_ZERO);
    pmodule_locked_pages = (int*)kmalloc(M4U_CLIENT_MODULE_NUM*4, GFP_KERNEL|__GFP_ZERO);
    
    m4u_struct_init(); //init related structures

    m4u_mvaGraph_init();
        
    // add SMI reg init here
    SMI_reg_init();
    
    spin_lock_init(&gM4u_reg_lock);
    
    //Set IRQ   
    if(request_irq(MT6589_MMU0_IRQ_ID , MTK_M4U_isr, IRQF_TRIGGER_LOW, M4U_DEVNAME , NULL))
    {
        M4UERR("request M4U0 IRQ line failed\n");
        return -ENODEV;
    }

    if(request_irq(MT6589_MMU1_IRQ_ID , MTK_M4U_isr, IRQF_TRIGGER_LOW, M4U_DEVNAME , NULL))
    {
        M4UERR("request M4U1 IRQ line failed\n");
        return -ENODEV;
    }
    
    if(request_irq(MT6589_MMU_L2_IRQ_ID , MTK_M4U_L2_isr, IRQF_TRIGGER_LOW, M4U_DEVNAME , NULL))
    {
        M4UERR("request M4U2 IRQ line failed\n");
        return -ENODEV;
    }
    disable_irq(MT6589_MMU_L2_IRQ_ID);

    if(request_irq(MT6589_MMU_L2_SEC_IRQ_ID , MTK_M4U_L2_isr, IRQF_TRIGGER_LOW, M4U_DEVNAME , NULL))
    {
        M4UERR("request M4U2 IRQ line failed\n");
        return -ENODEV;
    }
    disable_irq(MT6589_MMU_L2_SEC_IRQ_ID);

    M4UMSG("init done\n");

    MMP_Event M4U_Event;
    M4U_Event = MMProfileRegisterEvent(MMP_RootEvent, "M4U");
    M4U_MMP_Events[PROFILE_M4U_ERROR] = MMProfileRegisterEvent(M4U_Event, "M4U ERROR");
    M4U_MMP_Events[PROFILE_ALLOC_MVA] = MMProfileRegisterEvent(M4U_Event, "Alloc MVA");
    M4U_MMP_Events[PROFILE_DEALLOC_MVA] = MMProfileRegisterEvent(M4U_Event, "DeAlloc MVA");
    MMProfileEnableEvent(M4U_MMP_Events[PROFILE_M4U_ERROR], 1);
    MMProfileEnableEvent(M4U_MMP_Events[PROFILE_ALLOC_MVA], 1);
    MMProfileEnableEvent(M4U_MMP_Events[PROFILE_DEALLOC_MVA], 1);
    
    return 0;


}


static int m4u_remove(struct platform_device *pdev)
{
    M4UDBG("MT6577_M4U_Exit() \n");
    
    cdev_del(g_pMTKM4U_CharDrv);
    unregister_chrdev_region(g_MTKM4Udevno, 1);

    //Release IRQ
    free_irq(MT6589_MMU0_IRQ_ID , NULL);
    free_irq(MT6589_MMU1_IRQ_ID , NULL);
    free_irq(MT6589_MMU_L2_IRQ_ID , NULL);
        
    return 0;
}


///> for kernel driver call directly
///> or user driver call through M4U library's ioctl
int m4u_dump_reg(int m4u_index) 
{
    // M4U related
    unsigned int i=0;
    unsigned int m4u_base;

    m4u_base = gM4UBaseAddr[m4u_index];
    
    M4U_POW_ON_TRY(eModuleID);
    M4UMSG(" M4U Register Start ======= index=%d \n", m4u_index);
    for(i=0;i<M4U_REG_SIZE/8;i+=4)
    {
    	M4UDBG("+0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x \n", 8*i, 
    	M4U_ReadReg32(m4u_base, 8*i + 4*0), M4U_ReadReg32(m4u_base, 8*i + 4*1),
    	M4U_ReadReg32(m4u_base, 8*i + 4*2), M4U_ReadReg32(m4u_base, 8*i + 4*3),
    	M4U_ReadReg32(m4u_base, 8*i + 4*4), M4U_ReadReg32(m4u_base, 8*i + 4*5),
    	M4U_ReadReg32(m4u_base, 8*i + 4*6), M4U_ReadReg32(m4u_base, 8*i + 4*7));
    }
    M4UMSG(" M4U Register End ========== \n");
    M4UMSG("(0x0  )=0x%x ", M4U_ReadReg32(0, REG_MMUg_CTRL));                
    M4UMSG("(0x4  )=0x%x ", M4U_ReadReg32(0, REG_MMUg_INVLD));
    M4UMSG("(0x8  )=0x%x ", M4U_ReadReg32(0, REG_MMUg_INVLD_SA));
    M4UMSG("(0xC  )=0x%x ", M4U_ReadReg32(0, REG_MMUg_INVLD_EA));
    M4UMSG("(0x10 )=0x%x ", M4U_ReadReg32(0, REG_MMUg_PT_BASE    ));
    M4UMSG("(0x18 )=0x%x \n", M4U_ReadReg32(0, REG_MMUg_L2_SEL	          ));
    M4UMSG("(0x1C )=0x%x ", M4U_ReadReg32(0, REG_MMUg_DCM              ));
    M4UMSG("(0x20 )=0x%x ", M4U_ReadReg32(0, REG_MMUg_CTRL_SEC         ));
    M4UMSG("(0x24 )=0x%x ", M4U_ReadReg32(0, REG_MMUg_INVLD_SEC	  ));
    M4UMSG("(0x28 )=0x%x ", M4U_ReadReg32(0, REG_MMUg_INVLD_SA_SEC));
    M4UMSG("(0x2c )=0x%x ", M4U_ReadReg32(0, REG_MMUg_INVLD_EA_SEC  ));
    M4UMSG("(0x30 )=0x%x \n", M4U_ReadReg32(0, REG_MMUg_PT_BASE_SEC ));


    M4UMSG("(0x100)=0x%x ", M4U_ReadReg32(0, REG_L2_GDC_STATE        ));
    M4UMSG("(0x104)=0x%x ", M4U_ReadReg32(0, REG_L2_GDC_OP	          ));
    M4UMSG("(0x108)=0x%x ", M4U_ReadReg32(0, REG_L2_GDC_PERF0        ));
    M4UMSG("(0x10c)=0x%x ", M4U_ReadReg32(0, REG_L2_GDC_PERF1        ));
    M4UMSG("(0x118)=0x%x \n", M4U_ReadReg32(0, REG_L2_GPE_STATUS         ));
    M4UMSG(" M4U Global Reg  End====== \n"); 	

    
    M4U_POW_OFF_TRY(eModuleID);
    M4UMSG(" SMI Register End====== \n"); 	
    
      return 0;
}

int m4u_confirm_range_invalidated(int m4u_index, unsigned int MVAStart, unsigned int MVAEnd)
{
    unsigned int i;
    unsigned int regval;
    unsigned int m4u_base = gM4UBaseAddr[m4u_index];
    int result = 0;


    M4ULOG("m4u_confirm_range_invalidated, idx=0x%x, MVAStart=0x%x, MVAEnd=0x%x \n",
        m4u_index, MVAStart, MVAEnd);    

    if(gTestLevel==M4U_TEST_LEVEL_USER)
    {
    	  return 0;    	  
    }    

    ///> check Main TLB part
    for(i=0;i<g4M4UTagCount[m4u_index];i++)
    {
        regval = M4U_ReadReg32(m4u_base, REG_MMU_MAIN_TAG(i));
        
        if(regval & (F_MAIN_TLB_VALID_BIT))
        {
            unsigned int mva = regval & F_MAIN_TLB_VA_MSK;
            unsigned int sa_align = MVAStart & F_MAIN_TLB_VA_MSK;
            unsigned int ea_align = MVAEnd & F_MAIN_TLB_VA_MSK;

            if(mva>=sa_align && mva<=ea_align)
            {
                if(gTestLevel==M4U_TEST_LEVEL_STRESS)
                {
                    M4UERR("main: i=%d, idx=0x%x, MVAStart=0x%x, MVAEnd=0x%x, RegValue=0x%x\n",
                        i, m4u_index, MVAStart, MVAEnd, regval);
                    m4u_dump_reg(m4u_index); 
                }
                else if(gTestLevel==M4U_TEST_LEVEL_ENG)
                {
                    M4UMSG("main: i=%d, idx=0x%x, MVAStart=0x%x, MVAEnd=0x%x, RegValue=0x%x \n",
                        i, m4u_index, MVAStart, MVAEnd, regval);
                }
                result = -1;
            }
        }
    }


    if(result < 0)
        return result;

    ///> check Prefetch TLB part
    for(i=0;i<g4M4UTagCount[m4u_index];i++)
    {
        regval = M4U_ReadReg32(m4u_base, REG_MMU_PFH_TAG(i));
        
        if(regval & F_PFH_TAG_VALID_MSK)  ///> a valid Prefetch TLB entry
        {

            unsigned int mva = regval & F_PFH_TAG_VA_MSK;
            unsigned int sa_align = MVAStart& F_PFH_TAG_VA_MSK;
            unsigned int ea_align = MVAEnd & F_PFH_TAG_VA_MSK;
            
            if(mva>=sa_align && mva<=ea_align)
            {
                if(gTestLevel==M4U_TEST_LEVEL_STRESS)
                {
                    M4UERR("prefetch: i=%d, idx=0x%x, MVAStart=0x%x, MVAEnd=0x%x, RegValue=0x%x\n",
                        i, m4u_index, MVAStart, MVAEnd, regval);
                    m4u_dump_reg(m4u_index); 
                }
                else if(gTestLevel==M4U_TEST_LEVEL_ENG)
                {
                    M4UMSG("prefetch: i=%d, idx=0x%x, MVAStart=0x%x, MVAEnd=0x%x, RegValue=0x%x\n",
                        i, m4u_index, MVAStart, MVAEnd, regval);
                }
                result = -1;
            }
            
        }
    }
    
    return result;
}


/**
 * @brief ,             
 * @param , tlbSelect 0:main tlb, 1:pre-fetch tlb LSB, 2:pre-fetch tlb MSB
 * @return 
 */
unsigned int m4u_get_main_descriptor(unsigned int m4u_base, unsigned int idx)
{
    unsigned int regValue=0;
    regValue = F_READ_ENTRY_TLB_SEL_MAIN \
             | F_READ_ENTRY_INDEX_VAL(idx)\
             | F_READ_ENTRY_READ_EN_BIT;
    
    M4U_WriteReg32(m4u_base, REG_MMU_READ_ENTRY, regValue);
    return M4U_ReadReg32(m4u_base, REG_MMU_DES_RDATA);
}

unsigned int m4u_get_pfh_descriptor(unsigned int m4u_base, int tlbIndex, int tlbSelect)
{
    unsigned regValue=0;
    M4U_ASSERT(tlbSelect<4);
 
    regValue = F_READ_ENTRY_TLB_SEL_PFH \
             | F_READ_ENTRY_INDEX_VAL(tlbIndex)\
             | F_READ_ENTRY_PFH_IDX(tlbSelect)\
             | F_READ_ENTRY_READ_EN_BIT;
   
    M4U_WriteReg32(m4u_base, REG_MMU_READ_ENTRY, regValue);
    return M4U_ReadReg32(m4u_base, REG_MMU_DES_RDATA);
}


int m4u_search_main_invalid(int m4u_id)
{
    unsigned int i=0;
    unsigned int m4u_base = gM4UBaseAddr[m4u_id];
    unsigned int mva;// des;
    
    M4UINFO("search main tlb=>\n");
    for(i=0;i<M4U_MAIN_TLB_NR;i++)
    {
        mva = M4U_ReadReg32(m4u_base, REG_MMU_MAIN_TAG(i));
        if((mva&(F_MAIN_TLB_VALID_BIT|F_MAIN_TLB_INV_DES_BIT))
            == (F_MAIN_TLB_VALID_BIT|F_MAIN_TLB_INV_DES_BIT) )
        {
            //des = m4u_get_main_descriptor(m4u_base,i);
            printk(KERN_INFO"%d:0x%x ", i,mva);
        }
    }
    return 0;
}


int m4u_dump_main_tlb_des(int m4u_id) 
{
    // M4U related
    unsigned int i=0;
    unsigned int m4u_base = gM4UBaseAddr[m4u_id];
    M4UINFO("dump main tlb=======>\n");
    for(i=0;i<M4U_MAIN_TLB_NR;i++)
    {
        printk(KERN_INFO"%d:0x%x:0x%x  ", i,
            M4U_ReadReg32(m4u_base, REG_MMU_MAIN_TAG(i)),
            m4u_get_main_descriptor(m4u_base,i));
        
        if((i+1)%8==0)
            printk(KERN_INFO"\n");
    }
    
    return 0;
}

int m4u_dump_main_tlb_tags(int m4u_id) 
{
    // M4U related
    unsigned int i=0;
    unsigned int m4u_base = gM4UBaseAddr[m4u_id];
    M4UINFO("dump main tlb=======>\n");
    for(i=0;i<M4U_MAIN_TLB_NR;i++)
    {
        M4UINFO("0x%x  ", M4U_ReadReg32(m4u_base, REG_MMU_MAIN_TAG(i)));
        
        if((i+1)%8==0)
            printk(KERN_INFO"\n");
    }
    
    return 0;
}

int m4u_dump_pfh_tlb_tags(int m4u_id)
{
    unsigned int i=0;
    unsigned int m4u_base = gM4UBaseAddr[m4u_id];

    M4UINFO("dump pfh tags=======>\n");
    for(i=0;i<M4U_PRE_TLB_NR;i++)
    {
        printk(KERN_INFO"0x%x ", M4U_ReadReg32(m4u_base, REG_MMU_PFH_TAG(i)));
        if((i+1)%8==0)
            printk(KERN_INFO"\n");
    }
    printk(KERN_INFO"\n");
    return 0;
}


int m4u_dump_pfh_tlb_des(int m4u_id) 
{
    // M4U related
    unsigned int i=0;
    unsigned int m4u_base = gM4UBaseAddr[m4u_id];
    M4UINFO("dump main tlb=======>\n");
    for(i=0;i<M4U_PRE_TLB_NR;i++)
    {
        printk(KERN_INFO"%d:0x%x:0x%x,0x%x,0x%x,0x%x  ", i,
            M4U_ReadReg32(m4u_base, REG_MMU_PFH_TAG(i)),
            m4u_get_pfh_descriptor(m4u_base,i, 0),
            m4u_get_pfh_descriptor(m4u_base,i, 1),
            m4u_get_pfh_descriptor(m4u_base,i, 2),
            m4u_get_pfh_descriptor(m4u_base,i, 3)
            );
        
        if((i+1)%4==0)
            printk(KERN_INFO"\n");
    }
    
    return 0;
}



void m4u_dump_pagetable_range(unsigned int mvaStart, unsigned int nr)
{
    unsigned int *pteStart;
    int i;

    pteStart = mva_pteAddr(mvaStart);
    mvaStart &= ~0xfff;

    // printk("m4u dump pagetable by range: start=0x%x, nr=%d ==============>\n", vaStart, nr);
    // printk("index : mva : PTE\n");
    printk(KERN_INFO"\n 0x%08x: ", mvaStart);
    for(i=0; i<nr; i++)
    {
        printk(KERN_INFO"0x%08x, ", pteStart[i]);
        if((i+1)%8==0)
        {
        	 printk(KERN_INFO"\n 0x%08x: ", (mvaStart+((i+1)<<12)));
        }
    }
    printk(KERN_INFO"\n");
    // printk("m4u dump pagetable done : start=0x%x, nr=%d ==============<\n", vaStart, nr);
}



int m4u_dump_pagetable(M4U_MODULE_ID_ENUM eModuleID)
{
    unsigned int addr=0;
    short index=1, nr=0;
    unsigned long irq_flags;

    
    printk("[M4U_K] dump pagetable by module: %s, page_num=%d ========>\n", 
        m4u_get_module_name(eModuleID), pmodule_locked_pages[eModuleID]);

//  this function may be called in ISR
    spin_lock_irqsave(&gMvaGraph_lock, irq_flags);
    for(index=1; index<MVA_MAX_BLOCK_NR+1; index += nr)
    {
        addr = index << MVA_BLOCK_SIZE_ORDER;
        nr = MVA_GET_NR(index);
        if(MVA_IS_BUSY(index) && (mvaInfoGraph[index]->eModuleId) == eModuleID)
        {
            // printk("start a mva region for module %d===>\n", eModuleID);
            m4u_dump_pagetable_range(addr, ((nr<<MVA_BLOCK_SIZE_ORDER)>>12));
        }
    }

    spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);
    printk("[M4U_K]  dump pagetable by module done =========================<\n");

    return 0;
}


int m4u_dump_mva_info()
{
    short index=1, nr=0;
    unsigned int addr=0;
    mva_info_t *pMvaInfo;
    unsigned long irq_flags;

    spin_lock_irqsave(&gMvaGraph_lock, irq_flags);
    
    M4UINFO(" dump mva allocated info ========>\n");
    M4UINFO("mva_start   mva_end          va       size     block_end  module block_num sec snoop \n");
    for(index=1; index<MVA_MAX_BLOCK_NR+1; index += nr)
    {
        addr = index << MVA_BLOCK_SIZE_ORDER;
        nr = MVA_GET_NR(index);
        if(MVA_IS_BUSY(index))
        {            
            pMvaInfo = mvaInfoGraph[index];
            M4UINFO("0x%-8x, 0x%-8x, 0x%-8x, 0x%-8x, 0x%-8x, %s, %d, %d, %d\n", 
                pMvaInfo->mvaStart, pMvaInfo->mvaStart+pMvaInfo->size-1,
                pMvaInfo->bufAddr, pMvaInfo->size, addr+nr*MVA_BLOCK_SIZE,
                m4u_get_module_name(pMvaInfo->eModuleId), nr,
                pMvaInfo->security, pMvaInfo->cache_coherent);
        }
        else
        {
            M4UINFO("%s, 0x%-8x, 0x%-8x, %d\n", 
                "free", addr, addr+nr*MVA_BLOCK_SIZE, nr);
        }
    }
    M4UINFO(" dump mva allocated info done ========>\n");
    
    spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);
    return 0;
}


int m4u_power_on(int m4u_index)
{
    return 0;
}

int m4u_power_off(int m4u_index)
{
    return 0;
}


int m4u_clock_on(void)
{
    enable_clock(MT_CG_INFRA_M4U, "m4u");
    enable_clock(MT_CG_INFRA_SMI, "m4u");
    return 0;
}

int m4u_clock_off(void)
{
    disable_clock(MT_CG_INFRA_M4U, "m4u");
    disable_clock(MT_CG_INFRA_SMI, "m4u");
    return 0;
}



static void m4u_mvaGraph_init(void)
{
    unsigned long irq_flags;
    spin_lock_irqsave(&gMvaGraph_lock, irq_flags);
    memset(mvaGraph, 0, sizeof(short)*(MVA_MAX_BLOCK_NR+1));
    memset(mvaInfoGraph, 0, sizeof(mva_info_t*)*(MVA_MAX_BLOCK_NR+1));
    mvaGraph[0] = 1|MVA_BUSY_MASK;
    mvaInfoGraph[0] = &gMvaNode_unkown;
    mvaGraph[1] = MVA_MAX_BLOCK_NR;
    mvaInfoGraph[1] = &gMvaNode_unkown;
    mvaGraph[MVA_MAX_BLOCK_NR] = MVA_MAX_BLOCK_NR;
    mvaInfoGraph[MVA_MAX_BLOCK_NR] = &gMvaNode_unkown;
    
    spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);
}

void m4u_mvaGraph_dump_raw(void)
{
    int i;
    unsigned long irq_flags;
    spin_lock_irqsave(&gMvaGraph_lock, irq_flags);
    printk("[M4U_K] dump raw data of mvaGraph:============>\n");
    for(i=0; i<MVA_MAX_BLOCK_NR+1; i++)
        printk("0x%4x: 0x%08x   ID:%d\n", i, mvaGraph[i], mvaInfoGraph[i]->eModuleId); 
    spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);
}


void m4u_mvaGraph_dump(void)
{
    unsigned int addr=0, size=0;
    short index=1, nr=0;
    M4U_MODULE_ID_ENUM moduleID;
    char *pMvaFree = "FREE";
    char *pErrorId = "ERROR";
    char *pOwner = NULL;
    int i,max_bit;    
    short frag[12] = {0};
    short nr_free=0, nr_alloc=0;
    unsigned long irq_flags;
    
    printk("[M4U_K] mva allocation info dump:====================>\n");
    printk("start      size     blocknum    owner       \n");

    spin_lock_irqsave(&gMvaGraph_lock, irq_flags);
    for(index=1; index<MVA_MAX_BLOCK_NR+1; index += nr)
    {
        addr = index << MVA_BLOCK_SIZE_ORDER;
        nr = MVA_GET_NR(index);
        size = nr << MVA_BLOCK_SIZE_ORDER;
        if(MVA_IS_BUSY(index))
        {
            moduleID = mvaInfoGraph[index]->eModuleId;
            if(moduleID > M4U_CLIENT_MODULE_NUM-1)
                pOwner = pErrorId;
            else
                pOwner = m4u_get_module_name(moduleID);
            nr_alloc += nr;
        }
        else    // mva region is free
        {
            pOwner = pMvaFree;
            nr_free += nr;

            max_bit=0;
            for(i=0; i<12; i++)
            {
                if(nr & (1<<i))
                    max_bit = i;
            }
            frag[max_bit]++; 
        }

        printk("0x%08x  0x%08x  %4d    %s\n", addr, size, nr, pOwner);

     }

    spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);

    printk("\n");
    printk("[M4U_K] mva alloc summary: (unit: blocks)========================>\n");
    printk("free: %d , alloc: %d, total: %d \n", nr_free, nr_alloc, nr_free+nr_alloc);
    printk("[M4U_K] free region fragments in 2^x blocks unit:===============\n");
    printk("  0     1     2     3     4     5     6     7     8     9     10    11    \n");
    printk("%4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d  %4d  \n",
        frag[0],frag[1],frag[2],frag[3],frag[4],frag[5],frag[6],frag[7],frag[8],frag[9],frag[10],frag[11]);
    printk("[M4U_K] mva alloc dump done=========================<\n");
    
}

static M4U_MODULE_ID_ENUM mva2module(unsigned int mva)
{

    M4U_MODULE_ID_ENUM eModuleId = M4U_CLNTMOD_UNKNOWN;
    int index;
    unsigned long irq_flags;

    index = MVAGRAPH_INDEX(mva);
    if(index==0 || index>MVA_MAX_BLOCK_NR)
    {
        M4UMSG("mvaGraph index is 0. mva=0x%x\n", mva);
        return M4U_CLNTMOD_UNKNOWN;
    }
    
    spin_lock_irqsave(&gMvaGraph_lock, irq_flags);

    //find prev head/tail of this region
    while(mvaGraph[index]==0)
        index--;

    if(MVA_IS_BUSY(index))
    {
        eModuleId = mvaInfoGraph[index]->eModuleId;
        goto out;
    }
    else
    {
        eModuleId = M4U_CLNTMOD_UNKNOWN;
        goto out;
    }                    

out:    
    spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);
    return eModuleId;
    
}


static unsigned int m4u_do_mva_alloc(M4U_MODULE_ID_ENUM eModuleID, 
								  const unsigned int BufAddr, 
								  const unsigned int BufSize,
								  mva_info_t *pMvaInfo)
{
    short s,end;
    short new_start, new_end;
    short nr = 0;
    unsigned int mvaRegionStart;
    unsigned int startRequire, endRequire, sizeRequire; 
    unsigned long irq_flags;

    if(BufSize == 0) return 0;

    MMProfileLogEx(M4U_MMP_Events[PROFILE_ALLOC_MVA_REGION], MMProfileFlagStart, eModuleID, BufAddr);
    ///-----------------------------------------------------
    ///calculate mva block number
    startRequire = BufAddr & (~M4U_PAGE_MASK);
    endRequire = (BufAddr+BufSize-1)| M4U_PAGE_MASK;
    sizeRequire = endRequire-startRequire+1;
    nr = (sizeRequire+MVA_BLOCK_ALIGN_MASK)>>MVA_BLOCK_SIZE_ORDER;//(sizeRequire>>MVA_BLOCK_SIZE_ORDER) + ((sizeRequire&MVA_BLOCK_ALIGN_MASK)!=0);

    spin_lock_irqsave(&gMvaGraph_lock, irq_flags);

    ///-----------------------------------------------
    ///find first match free region
    for(s=1; (s<(MVA_MAX_BLOCK_NR+1))&&(mvaGraph[s]<nr); s+=(mvaGraph[s]&MVA_BLOCK_NR_MASK))
        ;
    if(s > MVA_MAX_BLOCK_NR)
    {
        spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);
        M4UMSG("mva_alloc error: no available MVA region for %d blocks!\n", nr);
        return 0;
    }

    ///-----------------------------------------------
    ///alloc a mva region
    end = s + mvaGraph[s] - 1;

    if(unlikely(nr == mvaGraph[s]))
    {
        MVA_SET_BUSY(s);
        MVA_SET_BUSY(end);
        mvaInfoGraph[s] = pMvaInfo;
        mvaInfoGraph[end] = pMvaInfo;
    }
    else
    {
        new_end = s + nr - 1;
        new_start = new_end + 1;
        //note: new_start may equals to end
        mvaGraph[new_start] = (mvaGraph[s]-nr);
        mvaGraph[new_end] = nr | MVA_BUSY_MASK;
        mvaGraph[s] = mvaGraph[new_end];
        mvaGraph[end] = mvaGraph[new_start];

        mvaInfoGraph[s] = pMvaInfo;
        mvaInfoGraph[new_end] = pMvaInfo;
    }

    spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);

    mvaRegionStart = (unsigned int)s;
    
    MMProfileLogEx(M4U_MMP_Events[PROFILE_ALLOC_MVA_REGION], MMProfileFlagEnd, eModuleID, BufSize);
    return (mvaRegionStart<<MVA_BLOCK_SIZE_ORDER) + mva_pageOffset(BufAddr);

}



#define RightWrong(x) ( (x) ? "correct" : "error")
static int m4u_do_mva_free(M4U_MODULE_ID_ENUM eModuleID, 
                                const unsigned int BufAddr,
								const unsigned int BufSize,
								unsigned int mvaRegionStart) 
{
    short startIdx = mvaRegionStart >> MVA_BLOCK_SIZE_ORDER;
    short nr = mvaGraph[startIdx] & MVA_BLOCK_NR_MASK;
    short endIdx = startIdx + nr - 1;
    unsigned int startRequire, endRequire, sizeRequire;
    short nrRequire;
    mva_info_t * pMvaInfo = NULL;
    unsigned long irq_flags;

    spin_lock_irqsave(&gMvaGraph_lock, irq_flags);
    ///--------------------------------
    ///check the input arguments
    ///right condition: startIdx is not NULL && region is busy && right module && right size 
    startRequire = BufAddr & (~M4U_PAGE_MASK);
    endRequire = (BufAddr+BufSize-1)| M4U_PAGE_MASK;
    sizeRequire = endRequire-startRequire+1;
    nrRequire = (sizeRequire+MVA_BLOCK_ALIGN_MASK)>>MVA_BLOCK_SIZE_ORDER;//(sizeRequire>>MVA_BLOCK_SIZE_ORDER) + ((sizeRequire&MVA_BLOCK_ALIGN_MASK)!=0);
    if(!(   startIdx != 0           //startIdx is not NULL
            && MVA_IS_BUSY(startIdx)               // region is busy
            && (mvaInfoGraph[startIdx]->eModuleId==eModuleID) //right module
            && (nr==nrRequire)       //right size
          )
       )
    {

        spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);
        m4u_aee_print("free mva error, larb=%d, module=%s\n", m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID));
        M4UMSG("error to free mva========================>\n");
        M4UMSG("ModuleID=%s (expect %s) [%s]\n", 
                m4u_get_module_name(eModuleID), m4u_get_module_name(mvaInfoGraph[startIdx]->eModuleId),
                RightWrong(eModuleID==(mvaInfoGraph[startIdx]->eModuleId)));
        M4UMSG("BufSize=%d(unit:0x%xBytes) (expect %d) [%s]\n", 
                nrRequire, MVA_BLOCK_SIZE, nr, RightWrong(nrRequire==nr));
        M4UMSG("mva=0x%x, (IsBusy?)=%d (expect %d) [%s]\n",
                mvaRegionStart, MVA_IS_BUSY(startIdx),1, RightWrong(MVA_IS_BUSY(startIdx)));
        m4u_mvaGraph_dump();
        //m4u_mvaGraph_dump_raw();
        return -1;
    }

    pMvaInfo = mvaInfoGraph[startIdx];
    mvaInfoGraph[startIdx] = NULL;
    mvaInfoGraph[endIdx] = NULL;

    ///--------------------------------
    ///merge with followed region
    if( (endIdx+1 <= MVA_MAX_BLOCK_NR)&&(!MVA_IS_BUSY(endIdx+1)))
    {
        nr += mvaGraph[endIdx+1];
        mvaGraph[endIdx] = 0;
        mvaGraph[endIdx+1] = 0;
    }

    ///--------------------------------
    ///merge with previous region
    if( (startIdx-1>0)&&(!MVA_IS_BUSY(startIdx-1)) )
    {
        int pre_nr = mvaGraph[startIdx-1];
        mvaGraph[startIdx] = 0;
        mvaGraph[startIdx-1] = 0;
        startIdx -= pre_nr;
        nr += pre_nr;
    }
    ///--------------------------------
    ///set region flags
    mvaGraph[startIdx] = nr;
    mvaGraph[startIdx+nr-1] = nr;

    spin_unlock_irqrestore(&gMvaGraph_lock, irq_flags);

    if(pMvaInfo!=NULL)
        m4u_free_garbage_list(pMvaInfo);
    
    return 0;    

}

void m4u_profile_init(void)
{
    MMP_Event M4U_Event;
    M4U_Event = MMProfileRegisterEvent(MMP_RootEvent, "M4U");
    M4U_MMP_Events[PROFILE_ALLOC_MVA] = MMProfileRegisterEvent(M4U_Event, "Alloc MVA");
    M4U_MMP_Events[PROFILE_ALLOC_MVA_REGION] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_ALLOC_MVA], "Alloc MVA Region");
    M4U_MMP_Events[PROFILE_GET_PAGES] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_ALLOC_MVA], "Get Pages");
    M4U_MMP_Events[PROFILE_FOLLOW_PAGE] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_GET_PAGES], "Follow Page");
    M4U_MMP_Events[PROFILE_FORCE_PAGING] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_GET_PAGES], "Force Paging");
    M4U_MMP_Events[PROFILE_MLOCK] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_GET_PAGES], "MLock");
    M4U_MMP_Events[PROFILE_ALLOC_FLUSH_TLB] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_ALLOC_MVA], "Alloc Flush TLB");
    M4U_MMP_Events[PROFILE_DEALLOC_MVA] = MMProfileRegisterEvent(M4U_Event, "DeAlloc MVA");
    M4U_MMP_Events[PROFILE_RELEASE_PAGES] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_DEALLOC_MVA], "Release Pages");
    M4U_MMP_Events[PROFILE_MUNLOCK] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_RELEASE_PAGES], "MUnLock");
    M4U_MMP_Events[PROFILE_PUT_PAGE] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_RELEASE_PAGES], "Put Page");
    M4U_MMP_Events[PROFILE_RELEASE_MVA_REGION] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_DEALLOC_MVA], "Release MVA Region");
    M4U_MMP_Events[PROFILE_QUERY] = MMProfileRegisterEvent(M4U_Event, "Query MVA");
    M4U_MMP_Events[PROFILE_INSERT_TLB] = MMProfileRegisterEvent(M4U_Event, "Insert TLB");
    M4U_MMP_Events[PROFILE_DMA_MAINT_ALL] = MMProfileRegisterEvent(M4U_Event, "Cache Maintain");
    M4U_MMP_Events[PROFILE_DMA_CLEAN_RANGE] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_DMA_MAINT_ALL], "Clean Range");
    M4U_MMP_Events[PROFILE_DMA_CLEAN_ALL] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_DMA_MAINT_ALL], "Clean All");
    M4U_MMP_Events[PROFILE_DMA_INVALID_RANGE] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_DMA_MAINT_ALL], "Invalid Range");
    M4U_MMP_Events[PROFILE_DMA_INVALID_ALL] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_DMA_MAINT_ALL], "Invalid All");
    M4U_MMP_Events[PROFILE_DMA_FLUSH_RANGE] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_DMA_MAINT_ALL], "Flush Range");
    M4U_MMP_Events[PROFILE_DMA_FLUSH_ALL] = MMProfileRegisterEvent(M4U_MMP_Events[PROFILE_DMA_MAINT_ALL], "Flush All");
    M4U_MMP_Events[PROFILE_CACHE_FLUSH_ALL] = MMProfileRegisterEvent(M4U_Event, "Cache Flush All");
    M4U_MMP_Events[PROFILE_CONFIG_PORT] = MMProfileRegisterEvent(M4U_Event, "Config Port");
    M4U_MMP_Events[PROFILE_MAIN_TLB_MON] = MMProfileRegisterEvent(M4U_Event, "Main TLB Monitor");
    M4U_MMP_Events[PROFILE_PREF_TLB_MON] = MMProfileRegisterEvent(M4U_Event, "PreFetch TLB Monitor");
    M4U_MMP_Events[PROFILE_M4U_REG] = MMProfileRegisterEvent(M4U_Event, "M4U Registers");
    M4U_MMP_Events[PROFILE_M4U_ERROR] = MMProfileRegisterEvent(M4U_Event, "M4U ERROR");
}

// query mva by va
int m4u_query_mva(M4U_MODULE_ID_ENUM eModuleID, 
								  const unsigned int BufAddr, 
								  const unsigned int BufSize, 
								  unsigned int *pRetMVABuf,
								  struct file * a_pstFile) 
{
    struct list_head *pListHead;
    mva_info_t *pList = NULL;
    garbage_node_t *pNode = (garbage_node_t*)(a_pstFile->private_data);
    unsigned int query_start = BufAddr;
    unsigned int query_end = BufAddr + BufSize - 1;
    unsigned int s,e;
    int ret, err = 0;
    
    *pRetMVABuf = 0;                 
    
    if(pNode==NULL)
    {
        M4UMSG("error: m4u_query_mva, pNode is NULL, va=0x%x, module=%s! \n", BufAddr, m4u_get_module_name(eModuleID));
        return -1;
    }  

    MMProfileLogEx(M4U_MMP_Events[PROFILE_QUERY], MMProfileFlagStart, eModuleID, BufAddr);
    mutex_lock(&(pNode->dataMutex));              
    list_for_each(pListHead, &(pNode->mvaList))
    {
        pList = container_of(pListHead, mva_info_t, link);
        s = pList->bufAddr;
        e = s + pList->size - 1;

        if((pList->eModuleId==eModuleID) &&
        	 (query_start>=s && query_end<=e))
        {
            if(pList->mvaStart > 0) //here we have allocated mva for this buffer
            {
                *pRetMVABuf = pList->mvaStart + (query_start-s);
            }
            else    // here we have not allocated mva (this buffer is registered, and query for first time)
            {
                mva_info_t *pMvaInfo;
                M4U_ASSERT(pList->flags&MVA_REGION_REGISTER);
                //we should allocate mva for this buffer
                //allocate another mva_info node for allocate mva 
                //because allocate mva function will free the list if failed !!!
                pMvaInfo = m4u_alloc_garbage_list(pList->mvaStart, pList->size,
                            pList->eModuleId, pList->bufAddr, pList->flags, 
                            pList->security, pList->cache_coherent);

                ret = __m4u_alloc_mva(pMvaInfo);
                if(ret)
                {
                	M4UMSG("m4u_alloc_mva failed when query for it: %d\n", ret);
                	err = -EFAULT;
                } 
                else
                {
                    pList->flags &= ~(MVA_REGION_REGISTER);
                    pList->mvaStart = pMvaInfo->mvaStart;
                    *pRetMVABuf = pList->mvaStart + (query_start-s);
                }
                M4ULOG("allocate for first query: id=%s, addr=0x%08x, size=%d, mva=0x%x \n", 
                      m4u_get_module_name(eModuleID), BufAddr,  BufSize, *pRetMVABuf);
            }
    		break;
        }
    }
    mutex_unlock(&(pNode->dataMutex));
    MMProfileLogEx(M4U_MMP_Events[PROFILE_QUERY], MMProfileFlagEnd, eModuleID, BufSize);

    M4ULOG("m4u_query_mva: id=%s, addr=0x%08x, size=%d, mva=0x%x \n", 
                    m4u_get_module_name(eModuleID), BufAddr,  BufSize, *pRetMVABuf);

    return err;

}





#define TVC_MVA_SAFE_MARGIN 0 //(4*1024*50)
static int m4u_invalidate_and_check(unsigned int m4u_index, unsigned int start, unsigned int end)
{
    m4u_invalid_tlb_by_range(m4u_index, gM4U_L2_enable, start, end);
    if(0!=m4u_confirm_range_invalidated(m4u_index, start, end)) // first time fail, invalidate range again
    {
        m4u_invalid_tlb_by_range(m4u_index, gM4U_L2_enable, start, end);
    	if(0!=m4u_confirm_range_invalidated(m4u_index, start, end)) // again failed, invalidate all
    	{
    		M4UMSG("invalidate range twice, also fail! \n");
            m4u_invalid_tlb_all(m4u_index, gM4U_L2_enable);
            if(0!=m4u_confirm_range_invalidated(m4u_index, start, end)) // invalidate all failed, die
            {
                M4UMSG("invalidate all fail! \n");
            }
    	}
    }
    return 0;	
}


static int m4u_dealloc_mva_dynamic(M4U_MODULE_ID_ENUM eModuleID, 
									const unsigned int BufAddr, 
									const unsigned int BufSize,
									unsigned int mvaRegionAddr) 
{			
    int ret;
    unsigned int pteStart, pteNr;
    unsigned int align_page_num, page_num;
    unsigned int prefetch_distance = 1;

    M4ULOG("mva dealloc: ID=%s, VA=0x%x, size=%d, mva=0x%x\n", m4u_get_module_name(eModuleID), BufAddr, BufSize, mvaRegionAddr);

    page_num = M4U_GET_PAGE_NUM(BufAddr, BufSize);
    align_page_num = ((4-(page_num&(4-1)))&(4-1)) + 4*prefetch_distance;

    mutex_lock(&gM4uMutex);

    MMProfileLogEx(M4U_MMP_Events[PROFILE_RELEASE_PAGES], MMProfileFlagStart, eModuleID, BufAddr);
    m4u_release_pages(eModuleID,BufAddr,BufSize,mvaRegionAddr);

    //==================================
    // fill pagetable with 0
    {
        pteStart= (unsigned int)mva_pteAddr_nonsec(mvaRegionAddr); // get offset in the page table  
        pteNr = ((BufSize+(BufAddr&0xfff))/DEFAULT_PAGE_SIZE) + (((BufAddr+BufSize)&0xfff)!=0);
        pteNr += align_page_num;
        memset((void*)pteStart, 0, pteNr<<2);


        spin_lock(&gM4u_reg_lock);
        if((eModuleID != M4U_CLNTMOD_IMG)&&(eModuleID != M4U_CLNTMOD_GDMA))// this 2 module is in both larb3&4
        {
            m4u_invalidate_and_check(m4u_module_2_m4u_id(eModuleID), mvaRegionAddr, mvaRegionAddr+BufSize+align_page_num*0x1000-1);
        }
        else
        {
            m4u_invalid_tlb_by_range(M4U_ID_ALL, gM4U_L2_enable, mvaRegionAddr, mvaRegionAddr+BufSize+align_page_num*0x1000-1);
        }
        spin_unlock(&gM4u_reg_lock);
    }
    MMProfileLogEx(M4U_MMP_Events[PROFILE_RELEASE_PAGES], MMProfileFlagEnd, eModuleID, BufSize);
    mutex_unlock(&gM4uMutex);


    MMProfileLogEx(M4U_MMP_Events[PROFILE_RELEASE_MVA_REGION], MMProfileFlagStart, eModuleID, BufAddr);

    {
    	ret = m4u_do_mva_free(eModuleID, BufAddr, BufSize+align_page_num*0x1000, mvaRegionAddr);
    }
    
    MMProfileLogEx(M4U_MMP_Events[PROFILE_RELEASE_MVA_REGION], MMProfileFlagEnd, eModuleID, BufSize);
    M4U_mvaGraph_dump_DBG();

    return ret;
}



int m4u_fill_pagetable(M4U_MODULE_ID_ENUM eModuleID, unsigned int BufAddr, 
                unsigned int BufSize, unsigned int mvaStart, unsigned int entry_flag)
{
    int i;
    int page_num;
    
    unsigned int *pPagetable_nonsec;
    unsigned int *pPagetable_sec;
    unsigned int *pPhys;

    MMProfileLogEx(M4U_MMP_Events[PROFILE_GET_PAGES], MMProfileFlagStart, eModuleID, BufAddr);
    
    pPagetable_nonsec = mva_pteAddr_nonsec(mvaStart);
    pPagetable_sec = mva_pteAddr_sec(mvaStart);
    page_num = M4U_GET_PAGE_NUM(BufAddr, BufSize);
    pPhys = (unsigned int*)vmalloc(page_num*sizeof(unsigned int*));
    if(pPhys == NULL)
    {
        MMProfileLogEx(M4U_MMP_Events[PROFILE_GET_PAGES], MMProfileFlagEnd, eModuleID, BufSize);
        M4UMSG("m4u_fill_pagetable: error to vmalloc %d*4 size\n", page_num);
        return 0;
    }

    page_num = m4u_get_pages(eModuleID, BufAddr, BufSize, pPhys);
    if(page_num<=0)
    {
        MMProfileLogEx(M4U_MMP_Events[PROFILE_GET_PAGES], MMProfileFlagEnd, eModuleID, BufSize);
        M4UDBG("Error: m4u_get_pages failed \n");
        return 0;
    }
	
    mutex_lock(&gM4uMutex);    

    //fill page table
    for(i=0;i<page_num;i++)
    {
        unsigned int pa = pPhys[i];
        
        pa |= entry_flag;
        
    #ifdef M4U_USE_ONE_PAGETABLE
        *(pPagetable_nonsec+i) = pa;
    #else
        if(!(entry_flag&0x8))
        {
            *(pPagetable_sec+i) = pa;
        }
        else
        {
            #ifdef M4U_COPY_NONSEC_PT_TO_SEC
                *(pPagetable_nonsec+i) = pa;
                *(pPagetable_sec+i) = pa;
            #else
                *(pPagetable_nonsec+i) = pa;
            #endif
        }
    #endif
    } 


    vfree(pPhys);


    mb();
    MMProfileLogEx(M4U_MMP_Events[PROFILE_GET_PAGES], MMProfileFlagEnd, eModuleID, BufSize);
        
    ///-------------------------------------------------------
    ///flush tlb entries in this mva range
    MMProfileLogEx(M4U_MMP_Events[PROFILE_ALLOC_FLUSH_TLB], MMProfileFlagStart, eModuleID, BufAddr);
    spin_lock(&gM4u_reg_lock);
    if((eModuleID != M4U_CLNTMOD_IMG)&&(eModuleID != M4U_CLNTMOD_GDMA))// this 2 module is in both larb3&4
    {
        m4u_invalidate_and_check(m4u_module_2_m4u_id(eModuleID), mvaStart, mvaStart+BufSize-1);
    }
    else
    {
        m4u_invalid_tlb_by_range(M4U_ID_ALL, gM4U_L2_enable, mvaStart, mvaStart+BufSize-1);
    }
    spin_unlock(&gM4u_reg_lock);
    MMProfileLogEx(M4U_MMP_Events[PROFILE_ALLOC_FLUSH_TLB], MMProfileFlagEnd, eModuleID, BufSize);
    

    // record memory usage
    pmodule_current_size[eModuleID] += BufSize;
    if(pmodule_current_size[eModuleID]>gModuleMaxMVASize[eModuleID])
    {    	
        m4u_aee_print("overflow: larb=%d, module=%s, mvaSize=%d(max=%d)\n", 
            m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID), 
            pmodule_current_size[eModuleID], gModuleMaxMVASize[eModuleID]);
        M4UMSG("hint: alloc mva but forget to free it!!\n");
        m4u_dump_mva_info();
    }
    if(pmodule_current_size[eModuleID]> pmodule_max_size[eModuleID])
    {
        pmodule_max_size[eModuleID] = pmodule_current_size[eModuleID];
    }

    mutex_unlock(&gM4uMutex);

    return page_num;
    
}



static int __m4u_alloc_mva(mva_info_t *pMvaInfo)
{

    M4U_MODULE_ID_ENUM eModuleID = pMvaInfo->eModuleId;
    const unsigned int BufAddr = pMvaInfo->bufAddr;
    const unsigned int BufSize = pMvaInfo->size;
    int security = pMvaInfo->security;
    int cache_coherent = pMvaInfo->cache_coherent;
    
    unsigned int page_num, align_page_num;
    unsigned int mvaStart;
    unsigned int i;
    unsigned int entry_flag = F_DESC_VALID | F_DESC_NONSEC(!security) | F_DESC_SHARE(!!cache_coherent);
    int prefetch_distance = 1;

    MMProfileLogEx(M4U_MMP_Events[PROFILE_ALLOC_MVA], MMProfileFlagStart, eModuleID, BufAddr);

    page_num = M4U_GET_PAGE_NUM(BufAddr, BufSize);
    align_page_num = ((4-(page_num&(4-1)))&(4-1)) + 4*prefetch_distance;

    mvaStart= m4u_do_mva_alloc(eModuleID, BufAddr, BufSize+align_page_num*0x1000, pMvaInfo);     	
    if(mvaStart == 0)
    {
        m4u_aee_print("alloc mva fail: larb=%d,module=%s,size=%d\n", 
            m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID), BufSize);
            
        M4UMSG("mva_alloc error: no available MVA region for %d bytes!\n", BufSize);
        
        m4u_mvaGraph_dump();

        m4u_free_garbage_list(pMvaInfo);
        
        return -1;
    }

    page_num = m4u_fill_pagetable(eModuleID, BufAddr, BufSize, mvaStart, entry_flag);
    if(page_num==0)
    {
        M4UMSG("alloc_mva error: id=%s, addr=0x%08x, size=%d, sec=%d\n", 
                    m4u_get_module_name(eModuleID), BufAddr,  BufSize, security);
        goto error_alloc_mva;
    }
    //for m4u bug in mt6589: we need our entry_start and entry_end align at 128 bits (pfh tlb is 128bits align)
    //this aims to ensure that no invalid tlb will be fetched into tlb 
    //TODO: now prefetch_distance is 1 for most engine. if prefetch is not 1, we should modify this too
    
    {
        unsigned int *pPagetable_nonsec;
        unsigned int *pPagetable_sec;
        unsigned int pa = gM4U_align_page_pa|entry_flag;

        pPagetable_sec = mva_pteAddr_sec(mvaStart)+page_num;
        pPagetable_nonsec = mva_pteAddr_nonsec(mvaStart)+page_num;
        for(i=0; i<align_page_num; i++)
        {
            #ifdef M4U_USE_ONE_PAGETABLE
                *(pPagetable_nonsec+i) = pa;
            #else
                if(security)
                {
                    *(pPagetable_sec+i) = pa;
                }
                else
                {
                    #ifdef M4U_COPY_NONSEC_PT_TO_SEC
                        *(pPagetable_nonsec+i) = pa;
                        *(pPagetable_sec+i) = pa;
                    #else
                        *(pPagetable_nonsec+i) = pa;
                    #endif
                }
            #endif
        }
    }

    if(g_debug_make_translation_fault == 1)
    {
        unsigned int *pPagetable_nonsec;
        unsigned int *pPagetable_sec;
        pPagetable_sec = mva_pteAddr_sec(mvaStart);
        pPagetable_nonsec = mva_pteAddr_nonsec(mvaStart);
        *pPagetable_sec = 0;
        *pPagetable_nonsec = 0;
    }

    
    pMvaInfo->mvaStart = mvaStart;

    MMProfileLogEx(M4U_MMP_Events[PROFILE_ALLOC_MVA], MMProfileFlagEnd, mvaStart, BufSize);
    M4ULOG("alloc_mva_dynamic: id=%s, addr=0x%08x, size=%d,sec=%d, mva=0x%x, mva_end=0x%x\n", 
                    m4u_get_module_name(eModuleID), BufAddr,  BufSize, security, pMvaInfo->mvaStart, pMvaInfo->mvaStart+BufSize-1);

    return 0;

error_alloc_mva:
    m4u_do_mva_free(eModuleID, BufAddr, BufSize+align_page_num*0x1000, mvaStart);
    M4ULOG("alloc_mva error: id=%s, addr=0x%08x, size=%d, sec=%d\n", 
                    m4u_get_module_name(eModuleID), BufAddr,  BufSize, security);

    return -1;
}


int m4u_alloc_mva(M4U_MODULE_ID_ENUM eModuleID, 
								  const unsigned int BufAddr, 
								  const unsigned int BufSize, 
								  int security,
								  int cache_coherent,
								  unsigned int *pRetMVABuf)
{
    mva_info_t *pMvaInfo = NULL;
    int ret;
    pMvaInfo=m4u_alloc_garbage_list(0,BufSize,eModuleID,BufAddr,0,security,cache_coherent);
    ret = __m4u_alloc_mva(pMvaInfo);

    if(ret == 0)
    {
        *pRetMVABuf = pMvaInfo->mvaStart;
    }
    else
    {
        *pRetMVABuf = 0;
    }
    return ret;
}



#define MVA_PROTECT_BUFFER_SIZE 1024*1024
int m4u_dealloc_mva(M4U_MODULE_ID_ENUM eModuleID, 
									const unsigned int BufAddr, 
									const unsigned int BufSize, 
									const unsigned int MVA) 
{									

    int ret;
    
    M4ULOG("m4u_dealloc_mva, module=%s, addr=0x%x, size=0x%x, MVA=0x%x, mva_end=0x%x\n",
        m4u_get_module_name(eModuleID), BufAddr, BufSize, MVA, MVA+BufSize-1 );


    MMProfileLogEx(M4U_MMP_Events[PROFILE_DEALLOC_MVA], MMProfileFlagStart, eModuleID, BufAddr);
    //if(eModuleID!=M4U_CLNTMOD_RDMA_GENERAL && eModuleID!=M4U_CLNTMOD_ROT_GENERAL)
    {
        if(m4u_invalid_seq_range_by_mva(m4u_module_2_m4u_id(eModuleID), MVA, MVA+BufSize-1)==0)
        {
            M4UMSG("warning: dealloc mva without invalid tlb range!! id=%s,add=0x%x,size=0x%x,mva=0x%x\n",
                m4u_get_module_name(eModuleID), BufAddr, BufSize, MVA);
        }
    }
    
    ret = m4u_dealloc_mva_dynamic(eModuleID, BufAddr, BufSize, MVA);

    MMProfileLogEx(M4U_MMP_Events[PROFILE_DEALLOC_MVA], MMProfileFlagEnd, MVA, BufSize);
    return ret;

}


int m4u_invalid_seq_all(M4U_MODULE_ID_ENUM eModuleID) 
{
    unsigned int i;
    unsigned int m4u_index = m4u_module_2_m4u_id(eModuleID);
    unsigned int m4u_base = gM4UBaseAddr[m4u_index];
    unsigned int m4u_index_offset = (SEQ_RANGE_NUM)*m4u_index;
    
  
    M4ULOG("m4u_invalid_tlb_all, module:%s \n", m4u_get_module_name(eModuleID)); 
    M4U_POW_ON_TRY(eModuleID);

    spin_lock(&gM4u_reg_lock);

    if(FreeSEQRegs[m4u_index] < SEQ_RANGE_NUM)
    {
        for(i=0;i<SEQ_RANGE_NUM;i++)
        {
            if(pRangeDes[i+m4u_index_offset].Enabled == 1)
            {
                 pRangeDes[i].Enabled = 0;
                 M4U_WriteReg32(m4u_base, REG_MMU_SQ_START(i), 0);
                 M4U_WriteReg32(m4u_base, REG_MMU_SQ_END(i), 0);
                 FreeSEQRegs[m4u_index]++;
            }
        }
    }
    m4u_invalid_tlb_all(m4u_index, 0);
    
    M4U_POW_OFF_TRY(eModuleID);
    spin_unlock(&gM4u_reg_lock);
    
    return 0;

}

static inline int mva_owner_match(M4U_MODULE_ID_ENUM id, M4U_MODULE_ID_ENUM owner)
{
    if(owner == id)
        return 1;

#if 0
    if(owner==M4U_CLNTMOD_RDMA_GENERAL &&
       (id==M4U_CLNTMOD_RDMA0||id==M4U_CLNTMOD_RDMA1) 
       )
    {
        return 1;
    }
    if(owner==M4U_CLNTMOD_ROT_GENERAL &&
        (id==M4U_CLNTMOD_VDO_ROT0||
        id==M4U_CLNTMOD_RGB_ROT0||
        id==M4U_CLNTMOD_RGB_ROT1||
        id==M4U_CLNTMOD_VDO_ROT1||
        id==M4U_CLNTMOD_RGB_ROT2)
        )
    {
        return 1;
    }
#endif

    return 0;
}


int m4u_manual_insert_entry(M4U_PORT_ID_ENUM eModuleID,
									unsigned int EntryMVA, 
									int secure_pagetable,
									int Lock) 
{ 
    unsigned int *pPageAddr = 0;
    unsigned int EntryPA;
    unsigned int m4u_base = gM4UBaseAddr[m4u_port_2_m4u_id(eModuleID)];

    M4UDBG("m4u_manual_insert_entry, module:%s, EntryMVA:0x%x,secure:%d, Lock:%d \r\n", 
        m4u_get_port_name(eModuleID), EntryMVA, secure_pagetable, Lock);

    if(secure_pagetable)
    {
        pPageAddr = mva_pteAddr_sec(EntryMVA);
    }       
    else
    {
        pPageAddr = mva_pteAddr_nonsec(EntryMVA);
    }
    
    EntryPA = *pPageAddr;  
//    EntryPA &= 0xFFFFF000;  //clear bit0~11

    EntryMVA &= 0xFFFFF000;	//clear bit0~11

    if(Lock)
    {
        EntryMVA |= F_PROG_VA_LOCK_BIT; 
    }

    if(secure_pagetable && (!(EntryPA&F_DESC_NONSEC(1))))
    {
        EntryMVA |= F_PROG_VA_SECURE_BIT; 
    }
    

    M4U_WriteReg32(m4u_base, REG_MMU_PROG_VA, EntryMVA);
    M4U_WriteReg32(m4u_base, REG_MMU_PROG_DSC, EntryPA);
    M4U_WriteReg32(m4u_base, REG_MMU_PROG_EN, F_MMU_PROG_EN);

	  return 0;
}



// #define M4U_PRINT_RANGE_DETAIL  // dump range infro when no available range can be found
#define M4U_INVALID_ID 0x5555
int m4u_do_insert_seq_range(M4U_PORT_ID_ENUM eModuleID, 
                             unsigned int MVAStart, 
                             unsigned int MVAEnd, 
                             unsigned int entryCount)
{
    //causion: we should hold m4u global 
    unsigned int i;
    unsigned int RangeReg_ID = M4U_INVALID_ID;
    unsigned int m4u_index = m4u_module_2_m4u_id(eModuleID);
    unsigned int m4u_base = gM4UBaseAddr[m4u_index];
    unsigned int m4u_index_offset = (SEQ_RANGE_NUM)*m4u_index;

    if(entryCount!=1 && entryCount!=2 && entryCount!=4 && entryCount!=8 && entryCount!=16)
        entryCount = 1;

    if(mva2module(MVAStart)>=M4U_CLNTMOD_UNKNOWN)
    {
        m4u_aee_print("insert range fail: larb=%d,module=%s\n", 
            m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID));
        M4UMSG(" m4u_insert_seq_range module=%s, MVAStart=0x%x is %s, MVAEnd=0x%x is %s\n", 
    	    m4u_get_module_name(eModuleID), MVAStart, m4u_get_module_name(mva2module(MVAStart)),
    	    MVAEnd, m4u_get_module_name(mva2module(MVAEnd)));
        m4u_mvaGraph_dump();
    }

    M4ULOG("m4u_insert_seq_range , module:%s, MVAStart:0x%x, MVAEnd:0x%x, entryCount=%d \r\n", 
            m4u_get_module_name(eModuleID), MVAStart, MVAEnd,  entryCount);

//==================================
//no seq range error
    if(FreeSEQRegs[m4u_index] == 0)
    {
        M4ULOG("No seq range found. module=%s \n", m4u_get_module_name(eModuleID));
#ifdef M4U_PRINT_RANGE_DETAIL
        M4UMSG("m4u_insert_seq_range , module:%s, MVAStart:0x%x, MVAEnd:0x%x, entryCount=%d \r\n", 
                m4u_get_module_name(eModuleID), MVAStart, MVAEnd,  entryCount);
        M4UMSG(" Curent Range Info: \n");
        for(i=0;i<TOTAL_RANGE_NUM;i++)
        {
            if(1==pRangeDes[i].Enabled)
            {
                M4UMSG("pRangeDes[%d]: Enabled=%d, module=%s, MVAStart=0x%x, MVAEnd=0x%x \n", 
                    i, pRangeDes[i].Enabled, m4u_get_module_name(pRangeDes[i].eModuleID), 
                    pRangeDes[i].MVAStart, pRangeDes[i].MVAEnd);
            }
        } 
#endif        
        return 0;
    }

//===============================================
    //every seq range has to align to 256K Bytes
    MVAStart &= ~M4U_SEQ_ALIGN_MSK;
    MVAEnd |= M4U_SEQ_ALIGN_MSK;

//==================================================================    
    // check if the range is overlap with previous ones
    for(i=m4u_index_offset;i<m4u_index_offset+SEQ_RANGE_NUM;i++)
    {
        if(1==pRangeDes[i].Enabled)
        {
            if(MVAEnd<pRangeDes[i].MVAStart || MVAStart>pRangeDes[i].MVAEnd) //no overlap
            {
            	  continue;
            }
            else
            {
                M4UMSG("insert range overlap!: larb=%d,module=%s\n", 
                     m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID));

                M4UMSG("error: insert tlb range is overlapped with previous ranges, current process=%s,!\n",  current->comm);	
                M4UMSG("module=%s, mva_start=0x%x, mva_end=0x%x \n", m4u_get_module_name(eModuleID), MVAStart, MVAEnd);
                M4UMSG("overlapped range id=%d, module=%s, mva_start=0x%x, mva_end=0x%x \n", 
                        i, m4u_get_module_name(pRangeDes[i].eModuleID), pRangeDes[i].MVAStart, pRangeDes[i].MVAEnd);
                return 0;
            }
        }
    }
//========================================
    //find a free seq range
    
    if(FreeSEQRegs[m4u_index]>0) ///> first search in low priority
    {
        for(i=m4u_index_offset;i<m4u_index_offset+SEQ_RANGE_NUM;i++)
        {
            if(pRangeDes[i].Enabled == 0)
            {
                RangeReg_ID = i;
                FreeSEQRegs[m4u_index]--;
                break;
            }
        }
    }
    
    if(RangeReg_ID == M4U_INVALID_ID)
    {
        M4ULOG("error: can not find available range \n");
        return 0;  // do not have to return erro to up-layer, nothing will happen even insert tlb range fails
    }

//======================================================
    // write register to insert seq range

    ///> record range information in array
    pRangeDes[RangeReg_ID].Enabled = 1;
    pRangeDes[RangeReg_ID].eModuleID = eModuleID;
    pRangeDes[RangeReg_ID].MVAStart = MVAStart;
    pRangeDes[RangeReg_ID].MVAEnd = MVAEnd;
    pRangeDes[RangeReg_ID].entryCount = entryCount;
    
    ///> set the range register
    MVAStart &= F_SQ_VA_MASK;
    MVAStart |= F_SQ_MULTI_ENTRY_VAL(entryCount-1);
    MVAStart |= F_SQ_EN_BIT;
    //align mvaend to 256K
    MVAEnd |= ~F_SQ_VA_MASK;
    
    spin_lock(&gM4u_reg_lock);
    {
        M4U_POW_ON_TRY(eModuleID);
        M4U_WriteReg32(m4u_base, REG_MMU_SQ_START(RangeReg_ID-m4u_index_offset), MVAStart);
        M4U_WriteReg32(m4u_base, REG_MMU_SQ_END(RangeReg_ID-m4u_index_offset), MVAEnd);
        M4U_POW_OFF_TRY(eModuleID);
    }
    
    spin_unlock(&gM4u_reg_lock);
    
    return 0;
}  //end of vM4USetUniupdateRangeInTLB()


int m4u_insert_seq_range(M4U_MODULE_ID_ENUM eModuleID, 
                             unsigned int MVAStart, 
                             unsigned int MVAEnd, 
                             M4U_RANGE_PRIORITY_ENUM ePriority,
                             unsigned int entryCount) //0:disable multi-entry, 1,2,4,8,16: enable multi-entry
{

    int ret;
    
    MMProfileLogEx(M4U_MMP_Events[PROFILE_INSERT_TLB], MMProfileFlagStart, eModuleID, MVAStart);
    mutex_lock(&gM4uMutex);
    ret = m4u_do_insert_seq_range(eModuleID, MVAStart, MVAEnd, entryCount);
    mutex_unlock(&gM4uMutex);
    MMProfileLogEx(M4U_MMP_Events[PROFILE_INSERT_TLB], MMProfileFlagEnd, eModuleID, MVAEnd-MVAStart+1);
    
    return ret;
    
}

int m4u_invalid_seq_range_by_mva(int m4u_index, unsigned int MVAStart, unsigned int MVAEnd)
{
    unsigned int i;
    unsigned int m4u_base = gM4UBaseAddr[m4u_index];
    unsigned int m4u_index_offset = SEQ_RANGE_NUM*m4u_index;
    int ret=-1;

    MVAStart &= ~M4U_SEQ_ALIGN_MSK;
    MVAEnd |= M4U_SEQ_ALIGN_MSK;
    
    M4UDBG("m4u_invalid_tlb_range_by_mva,  MVAStart:0x%x, MVAEnd:0x%x \r\n", MVAStart, MVAEnd);
	      
    spin_lock(&gM4u_reg_lock); 
    M4U_POW_ON_TRY(m4u_index);

    if(FreeSEQRegs[m4u_index] < SEQ_RANGE_NUM)
    {
        for(i=m4u_index_offset;i<m4u_index_offset+SEQ_RANGE_NUM;i++)
        {
            if(pRangeDes[i].Enabled == 1 &&
                pRangeDes[i].MVAStart>=MVAStart && 
                pRangeDes[i].MVAEnd<=MVAEnd)
            {
                 pRangeDes[i].Enabled = 0;
                 M4U_WriteReg32(m4u_base, REG_MMU_SQ_START(i-m4u_index_offset), 0);
                 M4U_WriteReg32(m4u_base, REG_MMU_SQ_END(i-m4u_index_offset), 0);
                 mb();
                 FreeSEQRegs[m4u_index]++;
                 break;
            }
        }
    }

    
    //trig invalidate register, 6589 invalid is moved to M4U global register area
    m4u_invalid_tlb_by_range(m4u_index, gM4U_L2_enable, MVAStart, MVAEnd);
    spin_unlock(&gM4u_reg_lock);

    return ret;

}



int m4u_invalid_seq_range(M4U_MODULE_ID_ENUM eModuleID, unsigned int MVAStart, unsigned int MVAEnd)
{
//    int ret;
	  
    M4ULOG(" m4u_invalid_seq_range, module:%s, MVAStart:0x%x, MVAEnd:0x%x \r\n", m4u_get_module_name(eModuleID), MVAStart, MVAEnd);
	      
    if(mva2module(MVAStart)>= M4U_CLNTMOD_UNKNOWN)
    {
        m4u_aee_print("invalid range fail: larb=%d,module=%s\n", 
             m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID));
        M4UMSG(" m4u_invalid_seq_rangemodule=%s, MVAStart=0x%x is %s, MVAEnd=0x%x is %s\n", 
    	    m4u_get_module_name(eModuleID), MVAStart, m4u_get_module_name(mva2module(MVAStart)),
    	    MVAEnd, m4u_get_module_name(mva2module(MVAEnd)));
        m4u_mvaGraph_dump();
    }
    mutex_lock(&gM4uMutex);
    m4u_invalid_seq_range_by_mva(m4u_module_2_m4u_id(eModuleID), MVAStart, MVAEnd);
    mutex_unlock(&gM4uMutex);
    return 0;

}



int m4u_insert_wrapped_range(M4U_MODULE_ID_ENUM eModuleID, 
                             M4U_PORT_ID_ENUM portID, 
                             unsigned int MVAStart, 
                             unsigned int MVAEnd)
{
    unsigned int i;
    unsigned int WrapRangeID = M4U_INVALID_ID;
    unsigned int RegVal;
    unsigned int m4u_index = m4u_port_2_m4u_id(portID);
    unsigned int m4u_base = gM4UBaseAddr[m4u_index];
    unsigned int m4u_index_offset = g4M4UWrapOffset[m4u_index];
    
    M4ULOG("m4u_insert_wrapped_range, module:%s, port:%s, MVAStart:0x%x, MVAEnd:0x%x \r\n", 
            m4u_get_module_name(eModuleID), m4u_get_port_name(portID), MVAStart, MVAEnd);
	  
            
    if(FreeWrapRegs[m4u_index] == 0)
    {
        M4UMSG("warning: m4u_insert_wrapped_range, no available wrap range found.\n");
        return -1;
    }    	      
    

    if(mva2module(MVAStart)!=eModuleID || mva2module(MVAEnd)!=eModuleID)
    {
        m4u_aee_print("insert wrap fail!: larb=%d,module=%s\n", 
             m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID));
        M4UMSG("m4u_insert_wrapped_range module=%s, MVAStart=0x%x is %s, MVAEnd=0x%x is %s\n", 
    	    m4u_get_module_name(eModuleID), MVAStart, m4u_get_module_name(mva2module(MVAStart)),
    	    MVAEnd, m4u_get_module_name(mva2module(MVAEnd)));
        m4u_mvaGraph_dump();
        return -1;
    }
    
        
    // check if the range is overlap with previous ones
    for(i=m4u_index_offset;i<m4u_index_offset+g4M4UWrapCount[m4u_index];i++)
    {
        if(1==pWrapDes[i].Enabled)
        {
            if(MVAEnd<pWrapDes[i].MVAStart || MVAStart>pWrapDes[i].MVAEnd) //no overlap
            {
            	  continue;
            }
            else
            {
                M4UMSG("error: insert wrap range is overlapped with previous ranges, current!\n");	
                M4UMSG("module=%s, mva_start=0x%x, mva_end=0x%x \n", m4u_get_module_name(eModuleID), MVAStart, MVAEnd);
                M4UMSG("overlapped range id=%d, module=%s, mva_start=0x%x, mva_end=0x%x \n", 
                        i, m4u_get_module_name(pWrapDes[i].eModuleID), pWrapDes[i].MVAStart, pWrapDes[i].MVAEnd);
                return -1;        
            }
        }
    }

    
    spin_lock(&gM4u_reg_lock);        
    ///> look for an inactive range register
    for(i=m4u_index_offset;i<m4u_index_offset+g4M4UWrapCount[m4u_index];i++)
    {
        if(pWrapDes[i].Enabled == 0)
        {
            M4UDBG("wrap range found. rangeID=%d \n", i);
            WrapRangeID = i;
            FreeWrapRegs[m4u_index]--;
            break;
        }
    }
            
    if(WrapRangeID == M4U_INVALID_ID)
    {
        M4U_ASSERT(0);
        M4UDBG("can not find available wrap range \n");
        spin_unlock(&gM4u_reg_lock);
        return -1;
    }
    
    pWrapDes[WrapRangeID].Enabled = 1;
    pWrapDes[WrapRangeID].eModuleID = eModuleID;
    pWrapDes[WrapRangeID].ePortID = portID;
    pWrapDes[WrapRangeID].MVAStart = MVAStart;
    pWrapDes[WrapRangeID].MVAEnd = MVAEnd;

    M4U_POW_ON_TRY(eModuleID);
    // write registers    
    M4U_WriteReg32(m4u_base, REG_MMU_WRAP_SA(WrapRangeID-m4u_index_offset), MVAStart&(~0xfff));
    M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EA(WrapRangeID-m4u_index_offset), MVAEnd&(~0xfff));

    RegVal = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EN(portID));
    RegVal &= ~(F_MMU_WRAP_SEL_VAL(portID, 0xffffff)); //clear sel field
    RegVal |= F_MMU_WRAP_SEL_VAL(portID, WrapRangeID-m4u_index_offset+1);
    M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EN(portID), RegVal);
    mb();

    M4U_POW_OFF_TRY(eModuleID);
  
    spin_unlock(&gM4u_reg_lock);
    
    return 0;
}

int m4u_invalid_wrapped_range(M4U_MODULE_ID_ENUM eModuleID, 
                              M4U_PORT_ID_ENUM portID,
                              unsigned int MVAStart, 
                              unsigned int MVAEnd)
{
    unsigned int i;
    unsigned int WrapRangeID = M4U_INVALID_ID;
    unsigned int RegVal;
    unsigned int m4u_index = m4u_module_2_m4u_id(eModuleID);
    unsigned int m4u_base = gM4UBaseAddr[m4u_index];
    unsigned int m4u_index_offset = g4M4UWrapOffset[m4u_index];
	  
    M4ULOG("m4u_invalid_wrapped_range, module:%s, MVAStart:0x%x, MVAEnd:0x%x \r\n", m4u_get_module_name(eModuleID), MVAStart, MVAEnd);
	      

    if(!mva_owner_match(eModuleID, mva2module(MVAStart)))
    {
        m4u_aee_print("invalid wrap fail: larb=%d,module=%s\n", 
             m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID));
        M4UMSG("m4u_invalid_wrapped_range module=%s, MVAStart=0x%x is %s, MVAEnd=0x%x is %s\n",  
    	    m4u_get_module_name(eModuleID), MVAStart, m4u_get_module_name(mva2module(MVAStart)),
    	    MVAEnd, m4u_get_module_name(mva2module(MVAEnd)));
        m4u_mvaGraph_dump();
        return -1;
    }
        
    spin_lock(&gM4u_reg_lock);
    
    if(FreeWrapRegs[m4u_index] < g4M4UWrapCount[m4u_index])
    {
        for(i=m4u_index_offset;i<m4u_index_offset+g4M4UWrapCount[m4u_index];i++)
        {
            if(pWrapDes[i].Enabled == 1 &&
                pWrapDes[i].MVAStart>=MVAStart && 
                pWrapDes[i].MVAEnd<=MVAEnd)
            {
                pWrapDes[i].Enabled = 0;
                WrapRangeID = i;
                FreeWrapRegs[m4u_index]++;
            }
        }
    }

    if(WrapRangeID == M4U_INVALID_ID)
    {
        M4UDBG("warning: m4u_invalid_wrapped_range(), does not find wrap range \n");
        spin_unlock(&gM4u_reg_lock);
        return 0;
    }
    
    pWrapDes[WrapRangeID].Enabled = 0;
    pWrapDes[WrapRangeID].eModuleID = 0;
    pWrapDes[WrapRangeID].ePortID = 0;
    pWrapDes[WrapRangeID].MVAStart = 0;
    pWrapDes[WrapRangeID].MVAEnd = 0;

    M4U_POW_ON_TRY(eModuleID);
    //write register 
    M4U_WriteReg32(m4u_base, REG_MMU_WRAP_SA(WrapRangeID-m4u_index_offset), 0);
    M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EA(WrapRangeID-m4u_index_offset), 0);

    RegVal = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EN(portID));
    RegVal &= ~(F_MMU_WRAP_SEL_VAL(portID, 0xffffff)); //clear sel field
    M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EN(portID), RegVal);
    mb();

    M4U_POW_OFF_TRY(eModuleID);
	spin_unlock(&gM4u_reg_lock);
	  
    return 0;

}



int m4u_config_port(M4U_PORT_STRUCT* pM4uPort) //native
{

    M4U_PORT_ID_ENUM PortID = (pM4uPort->ePortID);
    unsigned int m4u_base = gM4UBaseAddr[m4u_port_2_m4u_id(PortID)];
    M4U_MODULE_ID_ENUM eModuleID = m4u_port_2_module(PortID);
    unsigned int sec_con_val;

    pM4uPort->Distance = 1;
    pM4uPort->Direction = 0;
    
    M4ULOG("m4u_config_port(), port=%s, Virtuality=%d, Security=%d, Distance=%d, Direction=%d \n", 
        m4u_get_port_name(pM4uPort->ePortID), pM4uPort->Virtuality, pM4uPort->Security, pM4uPort->Distance, pM4uPort->Direction);

    MMProfileLogEx(M4U_MMP_Events[PROFILE_CONFIG_PORT], MMProfileFlagStart, eModuleID, pM4uPort->ePortID);
    spin_lock(&gM4u_reg_lock);
    M4U_POW_ON_TRY(eModuleID);

    // Direction, one bit for each port, 1:-, 0:+
    m4uHw_set_field_by_mask(m4u_base, REG_MMU_PFH_DIR(PortID),\
                F_MMU_PFH_DIR(PortID, 1), F_MMU_PFH_DIR(PortID, pM4uPort->Direction));
        
    // Distance
    if(pM4uPort->Distance>16)
    {
        M4ULOG("m4u_config_port() error, port=%s, Virtuality=%d, Security=%d, Distance=%d, Direction=%d \n", 
            m4u_get_port_name(pM4uPort->ePortID), pM4uPort->Virtuality, pM4uPort->Security, pM4uPort->Distance, pM4uPort->Direction);
    }

    m4uHw_set_field_by_mask(m4u_base, REG_MMU_PFH_DIST(PortID),\
                F_MMU_PFH_DIST_MASK(PortID), F_MMU_PFH_DIST_VAL(PortID,pM4uPort->Distance));

    // Virtuality, 1:V, 0:P
    sec_con_val = 0;
    if(pM4uPort->Virtuality)
    { 
        sec_con_val |= F_SMI_SECUR_CON_VIRTUAL(PortID);
    }
    if(pM4uPort->Security)
    {
        sec_con_val |= F_SMI_SECUR_CON_SECURE(PortID);
    }
    
    sec_con_val |= F_SMI_SECUR_CON_DOMAIN(PortID, 3);//pM4uPort->domain);

    m4uHw_set_field_by_mask(0, REG_SMI_SECUR_CON_OF_PORT(PortID),\
                F_SMI_SECUR_CON_MASK(PortID), sec_con_val);
    
    M4U_POW_OFF_TRY(eModuleID); 
    spin_unlock(&gM4u_reg_lock);

    MMProfileLogEx(M4U_MMP_Events[PROFILE_CONFIG_PORT], MMProfileFlagEnd, pM4uPort->Virtuality, pM4uPort->ePortID);
    return 0;

}


// config rotator port, need several parameter to improve performance
#define MULTI_ENTRY_VALUE 4
// #define M4U_ENABLE_AUTO_SET_WRAPPED_RANGE_MULTI_ENTRY
int m4u_config_port_rotator(M4U_PORT_STRUCT_ROTATOR *pM4uPort)
{ 
#if 0   
    M4U_PORT_STRUCT portStruct;
    unsigned int direction = 0;
    unsigned int distance = 0;
    unsigned int page_num;
    //unsigned int entryCount = 0;
    //unsigned int i=0;
    M4U_MODULE_ID_ENUM eModuleID = m4u_port_2_module(pM4uPort->ePortID);
                
    M4ULOG("m4u_config_port_rotator(), module=%s, port=%s, Virtuality=%d, Security=%d, MVAStart=0x%x, BufAddr=0x%x, BufSize=0x%x, angle=%d \n", 
            m4u_get_module_name(eModuleID),
            m4u_get_port_name(pM4uPort->ePortID), 
            pM4uPort->Virtuality, 
            pM4uPort->Security, 
            pM4uPort->MVAStart, 
            pM4uPort->BufAddr,
            pM4uPort->BufSize,
            pM4uPort->angle);    

    if(pM4uPort->ePortID!=M4U_PORT_VDO_ROT0_OUT0 &&
    	 pM4uPort->ePortID!=M4U_PORT_RGB_ROT0_OUT0 &&
    	 pM4uPort->ePortID!=M4U_PORT_RGB_ROT1_OUT0 &&
    	 pM4uPort->ePortID!=M4U_PORT_VDO_ROT1_OUT0 &&
    	 pM4uPort->ePortID!=M4U_PORT_RGB_ROT2_OUT0 &&
    	 pM4uPort->ePortID!=M4U_PORT_TV_ROT_OUT0 &&
    	 pM4uPort->ePortID!=M4U_PORT_G2D_W)
    {
    	  M4UERR("error, only rotator port should call m4u_config_port_rotator(), port=%s \n", m4u_get_port_name(pM4uPort->ePortID));
    	  return -1;
    }
    
    page_num = (pM4uPort->BufSize + (pM4uPort->BufAddr&0xfff))/DEFAULT_PAGE_SIZE;
    if((pM4uPort->BufAddr+pM4uPort->BufSize)&0xfff)
    {
        page_num++;
    } 
        
    // 1. direction
    if(M4U_PORT_G2D_W==pM4uPort->ePortID)
    {
        if(pM4uPort->angle==ROTATE_0 || 
           pM4uPort->angle==ROTATE_90 ||
           pM4uPort->angle==ROTATE_HFLIP_90||
           pM4uPort->angle==ROTATE_HFLIP_0)
        {
            direction = 0;
        }
        else if(pM4uPort->angle==ROTATE_HFLIP_270 || 
                pM4uPort->angle==ROTATE_270||
                pM4uPort->angle==ROTATE_HFLIP_180||
                pM4uPort->angle==ROTATE_180)
        {
            direction = 1;        
        }
        else
        {
            M4UERR("G2D rotate angel is invalid:%d \n", pM4uPort->angle);
            return -1;
        }
    }
    else // for MDP and TVROT
    {
        if(pM4uPort->angle==ROTATE_0 || pM4uPort->angle==ROTATE_90)
        {
            direction = 0;
        }
        else if(pM4uPort->angle==ROTATE_180 || pM4uPort->angle==ROTATE_270)
        {
            direction = 1;
        }
        else
        {
            M4UERR("rotate angel is invalid:%d \n", pM4uPort->angle);
            return -1;
        }
    }
    
    // 2. distance
    if(pM4uPort->angle==ROTATE_90 || pM4uPort->angle==ROTATE_270)
    {
        distance = MULTI_ENTRY_VALUE;
    }
    else
    {
        distance = 1;
    }
    
    // 3. config port    
    portStruct.ePortID    =pM4uPort->ePortID;
    portStruct.Virtuality =pM4uPort->Virtuality;	
    portStruct.Security   =pM4uPort->Security;   
    portStruct.Distance   =distance;   
    portStruct.Direction  =direction;  
    m4u_config_port(&portStruct);    

#ifdef M4U_ENABLE_AUTO_SET_WRAPPED_RANGE_MULTI_ENTRY
    // auto set wrapped range and multi-entry according to rotator's parameters
    if(1==portStruct.Virtuality)    
    {
        if(page_num%16==0 || page_num%16>8)
        {
            m4u_insert_wrapped_range(eModuleID, pM4uPort->ePortID, pM4uPort->MVAStart, pM4uPort->MVAStart+pM4uPort->BufSize-1);
        }
        else
        {
            // lock first (page_num%16) pages
            for(i=0;i<page_num%16;i++)
            {
                m4u_manual_insert_entry(eModuleID, pM4uPort->MVAStart+DEFAULT_PAGE_SIZE*i, true);            	
            }
            m4u_insert_wrapped_range(eModuleID, pM4uPort->ePortID, pM4uPort->MVAStart+(page_num%16)*DEFAULT_PAGE_SIZE, pM4uPort->MVAStart+pM4uPort->BufSize-1);
        }
        
        // 5. set multi-entry
        if(pM4uPort->angle==ROTATE_90 || pM4uPort->angle==ROTATE_270)
        {
            entryCount = MULTI_ENTRY_VALUE;
        }
        else
        {
            entryCount = 1;
        }
         m4u_insert_seq_range(eModuleID, pM4uPort->MVAStart, pM4uPort->MVAStart+pM4uPort->BufSize-1, RT_RANGE_HIGH_PRIORITY, entryCount);    
    }
    else
    {
        if(page_num%16==0 || page_num%16>8)
        {
            m4u_invalid_wrapped_range(eModuleID, pM4uPort->ePortID, pM4uPort->MVAStart, pM4uPort->MVAStart+pM4uPort->BufSize-1);
        }
        else
        {
            m4u_invalid_wrapped_range(eModuleID, pM4uPort->ePortID, pM4uPort->MVAStart+(page_num%16)*DEFAULT_PAGE_SIZE, pM4uPort->MVAStart+pM4uPort->BufSize-1);
        }
         m4u_invalid_seq_range(eModuleID, pM4uPort->MVAStart, pM4uPort->MVAStart+pM4uPort->BufSize-1);       	
    }
#endif

#endif
    
    return 0;
}




void m4u_get_perf_counter(int m4u_index, M4U_PERF_COUNT *pM4U_perf_count)
{
    unsigned int m4u_base = gM4UBaseAddr[m4u_index];
    pM4U_perf_count->transaction_cnt= M4U_ReadReg32(m4u_base, REG_MMU_ACC_CNT);    ///> Transaction access count
    pM4U_perf_count->main_tlb_miss_cnt= M4U_ReadReg32(m4u_base, REG_MMU_MAIN_MSCNT); ///> Main TLB miss count
    pM4U_perf_count->pfh_tlb_miss_cnt= M4U_ReadReg32(m4u_base, REG_MMU_PF_MSCNT);   ///> Prefetch TLB miss count
    pM4U_perf_count->pfh_cnt = M4U_ReadReg32(m4u_base, REG_MMU_PF_CNT);     ///> Prefetch count
}


int m4u_monitor_start(int m4u_id)
{
    unsigned int m4u_base = gM4UBaseAddr[m4u_id];
    
    M4ULOG("start monitor, id=%d \n", m4u_id);
    
    M4U_POW_ON_TRY(m4u_id);
    //clear GMC performance counter
    m4uHw_set_field_by_mask(m4u_base, REG_MMU_CTRL_REG, 
                F_MMU_CTRL_MONITOR_CLR(1), F_MMU_CTRL_MONITOR_CLR(1));
    m4uHw_set_field_by_mask(m4u_base, REG_MMU_CTRL_REG, 
                F_MMU_CTRL_MONITOR_CLR(1), F_MMU_CTRL_MONITOR_CLR(0));

    //enable GMC performance monitor
    m4uHw_set_field_by_mask(m4u_base, REG_MMU_CTRL_REG, 
                F_MMU_CTRL_MONITOR_EN(1), F_MMU_CTRL_MONITOR_EN(1));

    M4U_POW_OFF_TRY(m4u_id);


    return 0;
}

/**
 * @brief ,             
 * @param 
 * @return 
 */
int m4u_monitor_stop(int m4u_id)
{
    M4U_PERF_COUNT cnt;
    unsigned int m4u_base = gM4UBaseAddr[m4u_id];
    
    M4ULOG("stop monitor, id=%d \n", m4u_id);

    M4U_POW_ON_TRY(m4u_id);
    //disable GMC performance monitor
    m4uHw_set_field_by_mask(m4u_base, REG_MMU_CTRL_REG, 
                F_MMU_CTRL_MONITOR_EN(1), F_MMU_CTRL_MONITOR_EN(0));

    m4u_get_perf_counter(m4u_id, &cnt);
    //read register get the count

    MMProfileLogEx(M4U_MMP_Events[PROFILE_MAIN_TLB_MON], MMProfileFlagStart, (unsigned int) m4u_id, cnt.transaction_cnt);
    MMProfileLogEx(M4U_MMP_Events[PROFILE_PREF_TLB_MON], MMProfileFlagStart, (unsigned int) m4u_id, cnt.pfh_cnt);
    MMProfileLogEx(M4U_MMP_Events[PROFILE_MAIN_TLB_MON], MMProfileFlagEnd, (unsigned int) m4u_id, cnt.main_tlb_miss_cnt);
    MMProfileLogEx(M4U_MMP_Events[PROFILE_PREF_TLB_MON], MMProfileFlagEnd, (unsigned int) m4u_id, cnt.pfh_tlb_miss_cnt);
    M4UMSG("[M4U] total:%d, main miss:%d, pfh miss(walk):%d, auto pfh:%d\n", 
        cnt.transaction_cnt, cnt.main_tlb_miss_cnt, cnt.pfh_tlb_miss_cnt,cnt.pfh_cnt);
    
    
    if(0!=cnt.transaction_cnt)
    {
        M4UMSG("main miss:%d%%, pfh miss:%d%%\n", 
            100*cnt.main_tlb_miss_cnt/cnt.transaction_cnt,
            100*cnt.pfh_tlb_miss_cnt/cnt.transaction_cnt);
    }
    else
    {
        M4UMSG("[M4U] no transaction happened! \r\n");
    }
    M4U_POW_OFF_TRY(m4u_id);
    
    return 0;
}



void m4u_L2_monitor_start(unsigned perf_msk)
{
    unsigned int regval, mask;
    regval = F_L2_GDC_PERF_MASK(perf_msk) | F_L2_GDC_PERF_EN(1);
    mask = F_L2_GDC_PERF_MASK(0xffff) | F_L2_GDC_PERF_EN(1);
    
    m4uHw_set_field_by_mask(0, REG_L2_GDC_OP, mask, regval);
}

void m4u_L2_monitor_stop(unsigned int perf_cnt[2])
{
    unsigned int perf_msk, regval;
    unsigned int cnt0, cnt1;
    regval = COM_ReadReg32(REG_L2_GDC_OP);
    cnt0 = COM_ReadReg32(REG_L2_GDC_PERF0);
    cnt1 = COM_ReadReg32(REG_L2_GDC_PERF1);
    
    perf_msk = F_GET_L2_GDC_PERF_MASK(regval);

    M4UMSG("L2 cache monitor stop: \n");

    if(perf_msk == GDC_PERF_MASK_HIT_MISS)
    {
        M4UMSG("hit/miss counter: \n");
        M4UMSG("total:%d, hit:%d, miss:%d, miss_rate:%d%%\n",
            cnt0+cnt1, cnt0, cnt1, 100*cnt1/(cnt0+cnt1));
    }
    else if(perf_msk == GDC_PERF_MASK_RI_RO)
    {
        M4UMSG("ri/ro counter: \n");
        M4UMSG("total:%d, ri:%d, ro:%d, ro_rate:%d%%\n",
            cnt0+cnt1, cnt0, cnt1, 100*cnt1/(cnt0+cnt1));
    }
    else if(perf_msk == GDC_PERF_MASK_BUSY_CYCLE)
    {
        M4UMSG("busy cycle counter: high32:0x%x, low32:0x%x\n", cnt1, cnt0);
    }
    else if(perf_msk == GDC_PERF_MASK_READ_OUTSTAND_FIFO)
    {
        M4UMSG("READ_OUTSTAND_FIFO: cnt0:0x%x, cnt1:0x%x\n", cnt0, cnt1);
    }
    else
    {
        M4UMSG("wrong performance mask of L2 cache %d\n", perf_msk);
        M4U_ASSERT(0);
    }

    if(perf_cnt)
    {
        perf_cnt[0] = cnt0;
        perf_cnt[1] = cnt1;
    }

    m4uHw_set_field_by_mask(0, REG_L2_GDC_OP, F_L2_GDC_PERF_EN(1), F_L2_GDC_PERF_EN(0));

    

}



void m4u_print_perf_counter(int m4u_index, const char *msg)
{
    M4U_PERF_COUNT cnt;
    m4u_get_perf_counter(m4u_index, &cnt);
    M4UMSG("====m4u performance count for %s======\n", msg);
    M4UMSG("total trans=%d, main_miss=%d, pfh_miss=%d, pfh_cnt=%d\n",
        cnt.transaction_cnt, cnt.main_tlb_miss_cnt, cnt.pfh_tlb_miss_cnt, cnt.pfh_cnt);
}

#define M4U_ERR_PAGE_UNLOCKED -101

int m4u_put_unlock_page(struct page* page)
{
    unsigned int pfn;
    int ret = 0;
    pfn = page_to_pfn(page);
    MMProfileLogEx(M4U_MMP_Events[PROFILE_MUNLOCK], MMProfileFlagStart, 0, (unsigned int)(pfn<<12));

    if(pMlock_cnt[pfn])
    {
        if(!PageMlocked(page))
        {
            ret = M4U_ERR_PAGE_UNLOCKED;
        }
        
        pMlock_cnt[pfn]--;
        if(pMlock_cnt[pfn] == 0)
        {
            if (trylock_page(page)) {
                munlock_vma_page(page);
                unlock_page(page);
            }
            if(PageMlocked(page)==1)
            {
                M4UMSG(" Can't munlock page: \n");
                dump_page(page);
            }
        }
    }
    else
    {
        M4UMSG("warning pMlock_cnt[%d]==0 !! \n", pfn);
        ret = M4U_ERR_PAGE_UNLOCKED;
    }
    MMProfileLogEx(M4U_MMP_Events[PROFILE_MUNLOCK], MMProfileFlagEnd, 0, 0x1000);
    MMProfileLogEx(M4U_MMP_Events[PROFILE_PUT_PAGE], MMProfileFlagStart, 0, pfn<<12);
    put_page(page);
    MMProfileLogEx(M4U_MMP_Events[PROFILE_PUT_PAGE], MMProfileFlagEnd, 0, 0x1000);

    return ret;
    
}

///> m4u driver internal use function
///> should not be called outside m4u kernel driver
static int m4u_get_pages(M4U_MODULE_ID_ENUM eModuleID, unsigned int BufAddr, unsigned int BufSize, unsigned int* pPhys)
{
    int ret,i;
    int page_num;
    unsigned int start_pa;    
    unsigned int write_mode = 0;
    struct vm_area_struct *vma = NULL;
    
    
    M4ULOG("^ m4u_get_pages: module=%s, BufAddr=0x%x, BufSize=%d \n", m4u_get_module_name(eModuleID), BufAddr, BufSize);
    
    // caculate page number
    page_num = (BufSize + (BufAddr&0xfff))/DEFAULT_PAGE_SIZE;
    if((BufAddr+BufSize)&0xfff)
    {
        page_num++;
    }        

    if(M4U_CLNTMOD_LCDC_UI==eModuleID)
    {
        for(i=0;i<page_num;i++)
        {
            pPhys[i] = (BufAddr&0xfffff000) + i*DEFAULT_PAGE_SIZE;
        } 
       
    }  
    else if(BufAddr<PAGE_OFFSET)  // from user space
    {
        start_pa = m4u_user_v2p(BufAddr);
        if(0==start_pa)
        {
        	  M4UDBG("m4u_user_v2p=0 in m4u_get_pages() \n");
        }
        if(is_pmem_range((unsigned long*)start_pa, BufSize))
        {
            M4UMSG("warning: m4u_get_pages virtual addr from pmem! start_pa=0x%x\n", start_pa);
            for(i=0;i<page_num;i++)
            {
                *(pPhys+i) = m4u_user_v2p((BufAddr&0xfffff000) + i*DEFAULT_PAGE_SIZE);
            }   
        }    
        else 
        {
            if(BufSize>MAX_BUF_SIZE_TO_GET_USER_PAGE)
            {
                m4u_aee_print("alloc mva fail: larb=%d,module=%s,size=%d\n", 
                    m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID), BufSize);
            	  M4UMSG("m4u_get_pages(), single time alloc size=0x%x, bigger than limit=0x%x \n", BufSize, MAX_BUF_SIZE_TO_GET_USER_PAGE);
            	  return -EFAULT;
            } 

            down_read(&current->mm->mmap_sem);
            
            vma = find_vma(current->mm, BufAddr);
            if(vma == NULL)
            {
                M4UMSG("cannot find vma: module=%s, va=0x%x, size=0x%x\n", 
                    m4u_get_module_name(eModuleID), BufAddr, BufSize);
                m4u_dump_maps(BufAddr);
                
                return -1;
            }
            write_mode = (vma->vm_flags&VM_WRITE)?1:0;

            ret = m4u_get_user_pages(
            	eModuleID,
            	current,
            	current->mm,
            	BufAddr,
            	page_num,
            	write_mode, //m4u_get_write_mode_by_module(eModuleID),	// 1 /* write */
            	0,	/* force */
            	(struct page**)pPhys,
            	NULL);
    
            up_read(&current->mm->mmap_sem);
            
            if(ret<page_num)
            {
            	  // release pages first
            	for(i=0;i<ret;i++)
                {
                    m4u_put_unlock_page((struct page*)(*(pPhys+i)));
                }
                
                if(unlikely(fatal_signal_pending(current)))
                {
                    M4UMSG("error: receive sigkill during get_user_pages(),  page_num=%d, return=%d, module=%s, current_process:%s \n", 
                        page_num, ret, m4u_get_module_name(eModuleID), current->comm);
                }
                else
                {
                    if(ret>0) //return value bigger than 0 but smaller than expected, trigger red screen
                    {
                        M4UMSG("error: page_num=%d, get_user_pages return=%d, module=%s, current_process:%s \n", 
                            page_num, ret, m4u_get_module_name(eModuleID), current->comm);                    	
                        M4UMSG("error hint: maybe the allocated VA size is smaller than the size configured to m4u_alloc_mva()!");
                    }
                    else  // return vaule is smaller than 0, maybe the buffer is not exist, just return error to up-layer
                    {                    	                    
                        M4UMSG("error: page_num=%d, get_user_pages return=%d, module=%s, current_process:%s \n", 
                            page_num, ret, m4u_get_module_name(eModuleID), current->comm);                    	
                        M4UMSG("error hint: maybe the VA is deallocated before call m4u_alloc_mva(), or no VA has be ever allocated!");
                    }
                    m4u_dump_maps(BufAddr);
                }
            
                return -EFAULT;                
            }

            // add locked pages count, used for debug whether there is memory leakage
            pmodule_locked_pages[eModuleID] += page_num;
                    
            for(i=0;i<page_num;i++)
            {
                *(pPhys+i) = page_to_phys((struct page*)(*(pPhys+i)));
            }		
    
            M4UDBG("\n [user verify] BufAddr_sv=0x%x, BufAddr_sp=0x%x, BufAddr_ev=0x%x, BufAddr_ep=0x%x \n",
                        BufAddr, 
                        m4u_user_v2p(BufAddr), 
                        BufAddr+BufSize-1, 
                        m4u_user_v2p(BufAddr+BufSize-4));                    
        }
    }
    else // from kernel space
    {
        if(BufAddr>=VMALLOC_START && BufAddr<=VMALLOC_END) // vmalloc
        {
            struct page * ppage;
            for(i=0;i<page_num;i++)
            {          	
                ppage=vmalloc_to_page((unsigned int *)(BufAddr + i*DEFAULT_PAGE_SIZE));            
                *(pPhys+i) = page_to_phys(ppage) & 0xfffff000 ;
            }
        }
        else // kmalloc
        {
            for(i=0;i<page_num;i++)
            {
                *(pPhys+i) = virt_to_phys((void*)((BufAddr&0xfffff000) + i*DEFAULT_PAGE_SIZE));
            }        	
        }
        
        M4UDBG("\n [kernel verify] BufAddr_sv=0x%x, BufAddr_sp=0x%x, BufAddr_ev=0x%x, BufAddr_ep=0x%x \n",
                    BufAddr, 
                    virt_to_phys((void*)BufAddr), 
                    BufAddr+BufSize-1, 
                    virt_to_phys(BufAddr+BufSize-4));

    }

    return page_num;
}

int m4u_release_pages(M4U_MODULE_ID_ENUM eModuleID, unsigned int BufAddr, unsigned int BufSize, unsigned int MVA)
{
    unsigned int page_num=0, i=0;
    unsigned int start_pa;
    struct page *page;
    int put_page_err = 0, tmp;
    M4ULOG("m4u_release_pages(),  module=%s, BufAddr=0x%x, BufSize=0x%x\n", m4u_get_module_name(eModuleID), BufAddr, BufSize);

    if(!mva_owner_match(eModuleID, mva2module(MVA)))
    {
        m4u_aee_print("release page fail!: larb=%d,module=%s\n", 
             m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID));
        M4UMSG("m4u_release_pages module=%s, MVA=0x%x, expect module is %s \n", 
    	    m4u_get_module_name(eModuleID), MVA, m4u_get_module_name(mva2module(MVA)));
        m4u_mvaGraph_dump();
    }

    if(M4U_CLNTMOD_LCDC_UI==eModuleID)
    {
        goto RELEASE_FINISH;	
    }

    if(BufAddr<PAGE_OFFSET)  // from user space
    {	

        // put page by finding PA in pagetable
        unsigned int* pPageTableAddr = mva_pteAddr(MVA);
    
        page_num = (BufSize + (BufAddr&0xfff))/DEFAULT_PAGE_SIZE;
        if((BufAddr+BufSize)&0xfff)
        {
            page_num++;
        } 

        for(i=0;i<page_num;i++)
        {
            start_pa = *(pPageTableAddr+i);
            if((start_pa&0x02)==0)
            {
                continue;
            }
            page = pfn_to_page(__phys_to_pfn(start_pa));
    	  
            //we should check page count before call put_page, because m4u_release_pages() may fail in the middle of buffer
            //that is to say, first several pages may be put successfully in m4u_release_pages()
            if(page_count(page)>0) 
            {
                //to avoid too much log, we only save tha last err here.
                if((tmp=m4u_put_unlock_page(page)))
                    put_page_err = tmp;
            }         
            pmodule_locked_pages[eModuleID]--;   
            *(pPageTableAddr+i) &= (~0x2); 
        }
        if(put_page_err == M4U_ERR_PAGE_UNLOCKED)
        {
            M4UMSG("warning: in m4u_release_page: module=%s, va=0x%x, size=0x%x,mva=0x%x (page is unlocked before put page)\n", 
                m4u_get_module_name(eModuleID), BufAddr, BufSize, MVA);
        }
    } //end of "if(BufAddr<PAGE_OFFSET)"

RELEASE_FINISH:
    // record memory usage
    if(pmodule_current_size[eModuleID]<BufSize)
    {
        pmodule_current_size[eModuleID] = 0;
        M4UMSG("error pmodule_current_size is less than BufSize, module=%s, current_size=%d, BufSize=%d \n", 
           m4u_get_module_name(eModuleID), pmodule_current_size[eModuleID], BufSize);
    }
    else
    {
        pmodule_current_size[eModuleID] -= BufSize;
    }

    return 0;
}


// Refer to dma_cache_maint().
// The function works for user virtual addr 
#define BUFFER_SIZE_FOR_FLUSH_ALL (864*480*2)
int L1_CACHE_SYNC_BY_RANGE_ONLY = 1;



int m4u_dma_cache_maint(M4U_MODULE_ID_ENUM eModuleID, const void *start, size_t size, int direction)
{
    void (*outer_op)(phys_addr_t start, phys_addr_t end);
//	void (*outer_op)(unsigned long, unsigned long);
	void (*outer_op_all)(void);
	unsigned int page_start, page_num;
    unsigned int *pPhy = NULL;
    int i, ret=0;
    PROFILE_TYPE ptype=PROFILE_DMA_MAINT_ALL;
    switch (direction) {
	case DMA_FROM_DEVICE:
        if(size < BUFFER_SIZE_FOR_FLUSH_ALL)
            ptype = PROFILE_DMA_INVALID_RANGE;
        else
            ptype = PROFILE_DMA_INVALID_ALL;
		break;
	case DMA_TO_DEVICE:
        if(size < BUFFER_SIZE_FOR_FLUSH_ALL)
            ptype = PROFILE_DMA_CLEAN_RANGE;
        else
            ptype = PROFILE_DMA_CLEAN_ALL;
        break;
	case DMA_BIDIRECTIONAL:
        if(size < BUFFER_SIZE_FOR_FLUSH_ALL)
            ptype = PROFILE_DMA_FLUSH_RANGE;
        else
            ptype = PROFILE_DMA_FLUSH_ALL;
		break;
	default:
        break;
	}
    MMProfileLogEx(M4U_MMP_Events[ptype], MMProfileFlagStart, eModuleID, (unsigned int)start);

    M4ULOG(" m4u_dma_cache_maint():  module=%s, start=0x%x, size=%d, direction=%d \n",
          m4u_get_module_name(eModuleID), (unsigned int)start, size, direction);

    if(0==start)
    {
        m4u_aee_print("cache sync fail!: larb=%d,module=%s\n", 
             m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID));
        M4UMSG(" m4u_dma_cache_maint():  module=%s, start=0x%x, size=%d, direction=%d \n", 
          m4u_get_module_name(eModuleID), (unsigned int)start, size, direction);
  	  return -1;
    }         

    mutex_lock(&gM4uMutex);
   
    //To avoid non-cache line align cache corruption, user should make sure
    //cache start addr and size both cache-line-bytes align
    //we check start addr here but size should be checked in memory allocator
    //Rotdma memory is allocated by surfacefligner, address is not easy to modify
    //so do not check them now, should followup after MP
    if( m4u_get_dir_by_module(eModuleID)== M4U_DMA_WRITE &&
        (((unsigned int)start%L1_CACHE_BYTES!=0) || (size%L1_CACHE_BYTES)!=0)
       )
    {
        if(1) //screen red in debug mode
        {
            m4u_aee_print("Buffer align error: larb=%d,module=%s,addr=0x%x,size=%d,align=%d\n", 
                 m4u_module_2_larb(eModuleID), m4u_get_module_name(eModuleID), 
                 (unsigned int)start, size, L1_CACHE_BYTES);
      		M4UMSG("error: addr un-align, module=%s, addr=0x%x, size=0x%x, process=%s, align=0x%x\n",  m4u_get_module_name(eModuleID), 
      	        m4u_get_module_name(eModuleID), (unsigned int)start, size, current->comm, L1_CACHE_BYTES);
      	}
      	else
      	{
      		M4UMSG("error: addr un-align, module=%s, addr=0x%x, size=0x%x, process=%s, align=0x%x\n", 
      	        m4u_get_module_name(eModuleID), (unsigned int)start, size, current->comm, L1_CACHE_BYTES);
      	}
    }
          
	switch (direction) {
	case DMA_FROM_DEVICE:		/* invalidate only, HW write to memory */
        //M4UMSG("error: someone call cache maint with DMA_FROM_DEVICE, module=%s\n",m4u_get_module_name(eModuleID));
		outer_op = outer_inv_range;
		outer_op_all = outer_inv_all;  
		break;
	case DMA_TO_DEVICE:		/* writeback only, HW read from memory */
		outer_op = outer_clean_range;
		outer_op_all = outer_flush_all;
		break;
	case DMA_BIDIRECTIONAL:		/* writeback and invalidate */
		outer_op = outer_flush_range;
		outer_op_all = outer_flush_all;
		break;
	default:
		M4UERR("m4u_dma_cache_maint, direction=%d is invalid \n", direction);
        return -1;
	}


//<===========================================================================
//< check wether input buffer is valid (has physical pages allocated)
	page_start = (unsigned int)start & 0xfffff000;
	page_num = (size + ((unsigned int)start & 0xfff)) / DEFAULT_PAGE_SIZE;
	if(((unsigned int)start + size) & 0xfff)
		page_num++;

    if(size < BUFFER_SIZE_FOR_FLUSH_ALL)
    {
        pPhy = kmalloc(sizeof(int)*page_num, GFP_KERNEL);
        if(pPhy == NULL)
        {
            M4UMSG("error to kmalloc in m4u_cache_maint: module=%s, start=0x%x, size=%d, direction=%d \n", 
                m4u_get_module_name(eModuleID), (unsigned int)start, size, direction);
            goto out;
        }

        if((unsigned int)start<PAGE_OFFSET)  // from user space
        {
            for(i=0; i<page_num; i++,page_start+=DEFAULT_PAGE_SIZE)
            {
                struct page* page;
                pPhy[i] = m4u_user_v2p(page_start);
                page = phys_to_page(pPhy[i]);
                
                if((pPhy[i]==0) || (!PageMlocked(page))) 
                {
                    ret=-1;
                    M4UMSG("error: cache_maint() fail, module=%s, start=0x%x, page_start=0x%x, size=%d, pPhy[i]=0x%x\n", 
                            m4u_get_module_name(eModuleID), (unsigned int)start, (unsigned int)page_start, size, pPhy[i]);
                    dump_page(page);
                    m4u_dump_maps((unsigned int)start);
                    goto out;
                }
            }
        }
        else if((unsigned int)start>=VMALLOC_START && (unsigned int)start<=VMALLOC_END) // vmalloc
        {

            struct page * ppage;

            for(i=0; i<page_num; i++,page_start+=DEFAULT_PAGE_SIZE)
            {
                ppage=vmalloc_to_page((void *)page_start); 
                if(ppage == NULL) 
                {
                    ret=-1;
                    M4UMSG("error: ppage is 0 in cache_maint of vmalloc!, module=%s, start=0x%x, pagestart=0x%x\n", 
                            m4u_get_module_name(eModuleID), (unsigned int)start,page_start);
                    goto out;
                }
                pPhy[i] = page_to_phys(ppage);
            }
        }
        else // kmalloc
        {
            for(i=0; i<page_num; i++,page_start+=DEFAULT_PAGE_SIZE)
            {
                pPhy[i] = virt_to_phys((void*)page_start);
            }        	
        }
        
    }

//=====================================================================================
// L1 cache clean before hw read
    if(L1_CACHE_SYNC_BY_RANGE_ONLY)
    {
    	if (direction == DMA_TO_DEVICE) 
    	{
            dmac_map_area(start, size, direction);
    	}

    	if (direction == DMA_BIDIRECTIONAL) 
    	{
            dmac_flush_range(start, start+size-1);
    	}

    }
    else
    {
        smp_inner_dcache_flush_all();
    }

//=============================================================================================
	// L2 cache maintenance by physical pages
    if(size<BUFFER_SIZE_FOR_FLUSH_ALL)
    {
        for (i=0; i<page_num; i++) 
        {
    		outer_op(pPhy[i], pPhy[i]+ DEFAULT_PAGE_SIZE);
    	}
    }
    else 
    {
        outer_op_all();
    }
//=========================================================================================      
	// L1 cache invalidate after hw write to memory
    if(L1_CACHE_SYNC_BY_RANGE_ONLY)
    {
    	if (direction == DMA_FROM_DEVICE) 
        {
    	    dmac_unmap_area(start, size, direction);
        }
    }
  
out:
    if(pPhy != NULL)
        kfree(pPhy);

    MMProfileLogEx(M4U_MMP_Events[ptype], MMProfileFlagEnd, eModuleID, size);

    mutex_unlock(&gM4uMutex);
        
    return ret;
}



int m4u_dma_cache_flush_all()
{

   // M4UMSG("cache flush all!!\n")
    mutex_lock(&gM4uMutex);

    // L1 cache clean before hw read
    smp_inner_dcache_flush_all();
     
	// L2 cache maintenance by physical pages
    outer_flush_all();
    
    mutex_unlock(&gM4uMutex);
   
    return 0;
}


static M4U_DMA_DIR_ENUM m4u_get_dir_by_module(M4U_MODULE_ID_ENUM eModuleID)
{
    
    M4U_DMA_DIR_ENUM dir;
    switch(eModuleID)  // from user space
    {
        case M4U_CLNTMOD_G2D:
          dir = M4U_DMA_READ_WRITE;
          break;                   

        case M4U_CLNTMOD_AUDIO:
          dir = M4U_DMA_READ;
          break;                

        case M4U_CLNTMOD_JPGDEC: 
          dir = M4U_DMA_WRITE;
          break;

        default:
            //M4UMSG("warning: can not get port's direction, module=%s \n", m4u_get_module_name(eModuleID));
            dir = M4U_DMA_READ_WRITE;
            break;
    }

    return dir;
}



#define M4U_PAGE_TABLE_ALIGN (PT_TOTAL_ENTRY_NUM*sizeof(unsigned int) - 1) // page table addr should (2^16)x align
#define M4U_PROTECT_BUF_OFFSET (128-1)    // protect buffer should be 128x align
static bool m4u_struct_init(void)
{
    unsigned int *pProtectVA=NULL;  //Page Table virtual Address
    struct page* tmp_page = NULL;

    //======= alloc pagetable=======================
    pPT_nonsec= dma_alloc_coherent(NULL, PT_TOTAL_ENTRY_NUM * sizeof(unsigned int), &pt_pa_nonsec, GFP_KERNEL);
    if(!pPT_nonsec)
    {
        M4UMSG("dma_alloc_coherent error!  dma memory not available.\n");
        return false;
    }
    if((pt_pa_nonsec&M4U_PAGE_TABLE_ALIGN)!=0)
    {
        unsigned int tmp;
        M4UMSG("dma_alloc_coherent memory not align. PageTablePA=0x%x we will try again \n", pt_pa_nonsec);
        dma_free_coherent(NULL, PT_TOTAL_ENTRY_NUM * sizeof(unsigned int), pPT_nonsec, pt_pa_nonsec);
        tmp = (unsigned int)dma_alloc_coherent(NULL, PT_TOTAL_ENTRY_NUM * sizeof(unsigned int)+M4U_PAGE_TABLE_ALIGN, &pt_pa_nonsec, GFP_KERNEL);
        if(!tmp)
        {
            M4UMSG("dma_alloc_coherent error!  dma memory not available.\n");
            return false;
        }
        pPT_nonsec = (unsigned int*)((tmp+M4U_PAGE_TABLE_ALIGN)&(~M4U_PAGE_TABLE_ALIGN));
        pt_pa_nonsec += (unsigned int)pPT_nonsec - tmp;
    }
    
    M4UMSG("dma_alloc_coherent success! pagetable_va=0x%x, pagetable_pa=0x%x.\n", (unsigned int)pPT_nonsec, (unsigned int)pt_pa_nonsec);
    memset((void*)pPT_nonsec, 0, PT_TOTAL_ENTRY_NUM * sizeof(unsigned int));
    //======= alloc pagetable done=======================

#ifdef M4U_USE_ONE_PAGETABLE
    pPT_sec = pPT_nonsec;
    pt_pa_sec = pt_pa_nonsec;
#else

    //======= alloc pagetable for security pt=======================
    pPT_sec= dma_alloc_coherent(NULL, PT_TOTAL_ENTRY_NUM * sizeof(unsigned int), &pt_pa_sec, GFP_KERNEL);
    if(!pPT_sec)
    {
        M4UMSG("dma_alloc_coherent error for sec pt!  dma memory not available.\n");
        return false;
    }
    if((pt_pa_sec&M4U_PAGE_TABLE_ALIGN)!=0)
    {
        unsigned int tmp;
        M4UMSG("dma_alloc_coherent memory not align. PageTablePA=0x%x we will try again \n", pt_pa_sec);
        dma_free_coherent(NULL, PT_TOTAL_ENTRY_NUM * sizeof(unsigned int), pPT_sec, pt_pa_sec);
        tmp = (unsigned int)dma_alloc_coherent(NULL, PT_TOTAL_ENTRY_NUM * sizeof(unsigned int)+M4U_PAGE_TABLE_ALIGN, &pt_pa_sec, GFP_KERNEL);
        if(!tmp)
        {
            M4UMSG("dma_alloc_coherent error!  dma memory not available.\n");
            return false;
        }
        pPT_sec = (unsigned int*)((tmp+M4U_PAGE_TABLE_ALIGN)&(~M4U_PAGE_TABLE_ALIGN));
        pt_pa_sec += (unsigned int)pPT_sec - tmp;
    }
    
    M4UMSG("dma_alloc_coherent success! pagetable_va=0x%x, pagetable_pa=0x%x.\n", (unsigned int)pPT_sec, (unsigned int)pt_pa_sec);
    memset((void*)pPT_sec, 0, PT_TOTAL_ENTRY_NUM * sizeof(unsigned int));
    //======= alloc pagetable done=======================
#endif


    init_mlock_cnt();
    if(NULL==pMlock_cnt)
        return false;
          
    // allocate 128 byte for translation fault protection
    // when TF occurs, M4U will translate the physical address to ProtectPA
    pProtectVA = (unsigned int*) kmalloc(TF_PROTECT_BUFFER_SIZE*TOTAL_M4U_NUM+M4U_PROTECT_BUF_OFFSET, GFP_KERNEL|__GFP_ZERO);
    if(NULL==pProtectVA)
    {
        
        M4UMSG("Physical memory not available.\n");
        return false;
    }
    pProtectVA = (unsigned int*)(((unsigned int)pProtectVA+M4U_PROTECT_BUF_OFFSET)&(~M4U_PROTECT_BUF_OFFSET));
    ProtectPA = virt_to_phys(pProtectVA);
    if((ProtectPA&0x7f)!=0)
    {        
        M4UERR("Physical memory not align. ProtectPA=0x%x \n", ProtectPA);
    }  
    pProtectVA_nonCache = pProtectVA;
    memset((unsigned char*)pProtectVA_nonCache, 0x55, TF_PROTECT_BUFFER_SIZE*TOTAL_M4U_NUM);

    tmp_page = alloc_page(GFP_KERNEL|__GFP_ZERO);
//    gM4U_align_page_va = (unsigned int)page_address(tmp_page);
    gM4U_align_page_pa = (unsigned int)page_to_phys(tmp_page);

    M4UMSG("gM4U_align_page_pa is 0x%x\n", gM4U_align_page_pa);

    M4UDBG("ProtectTablePA:0x%x, ProtectTableVA:0x%x, pProtectVA_nonCache:0x%x \n", 
        ProtectPA, (unsigned int)pProtectVA, (unsigned int)pProtectVA_nonCache);
           
    //initialize global variables
    pRangeDes = kmalloc(sizeof(M4U_RANGE_DES_T) * TOTAL_RANGE_NUM, GFP_KERNEL|__GFP_ZERO);
    if(NULL==pRangeDes)
    {
        
        M4UMSG("Physical memory not available.\n");
        return false;
    }
    
    pWrapDes = kmalloc(sizeof(M4U_WRAP_DES_T) * TOTAL_WRAP_NUM, GFP_KERNEL|__GFP_ZERO);
    if(NULL==pWrapDes)
    {
        
        M4UMSG("Physical memory not available.\n");
        return false;
    }

    pM4URegBackUp = (unsigned int*)kmalloc(BACKUP_REG_SIZE, GFP_KERNEL|__GFP_ZERO);
    if(pM4URegBackUp==NULL)
    {
    	  M4UERR("pM4URegBackUp kmalloc fail \n");
    }    

    m4u_hw_init();
	 
    gM4uLogFlag = false; 

    return 0;
}


/**
 * @brief ,     system power on / return from power resume        
 * @param 
 * @return 
 */
int m4u_hw_init(void)
{
    unsigned int i;
    unsigned regval;
    M4UDBG("m4u_hw_init() \n");

    m4u_clock_on();

//=============================================
// SMI registers
//=============================================
    //bus sel 
    regval = F_SMI_BUS_SEL_larb0(larb_2_m4u_id(0))  \
            |F_SMI_BUS_SEL_larb1(larb_2_m4u_id(1))  \
            |F_SMI_BUS_SEL_larb2(larb_2_m4u_id(2))  \
            |F_SMI_BUS_SEL_larb3(larb_2_m4u_id(3))  \
            |F_SMI_BUS_SEL_larb4(larb_2_m4u_id(4))  ;

    M4UDBG("regval = 0x%x\n", regval);

    COM_WriteReg32(REG_SMI_BUS_SEL, regval);
    M4UMSG("bus = 0x%x\n", COM_ReadReg32(REG_SMI_BUS_SEL));
    
    // secure register: 
    // all use physical (bypass m4u); domain(3); secure(0)
    for(i=0; i<7; i++)
        COM_WriteReg32(REG_SMI_SECUR_CON(i), 0x66666666);

    //config G3D route to EMI (MCI in default)
    m4uHw_set_field_by_mask(0, REG_SMI_L1LEN, F_SMI_L1LEN_AXROUTE_G3D_EMI(1), F_SMI_L1LEN_AXROUTE_G3D_EMI(1));
    //config AUDIO route to EMI (MCI in default)
    m4uHw_set_field_by_mask(0, REG_SMI_L1LEN, F_SMI_L1LEN_AXROUTE_AUDIO_EMI(1), F_SMI_L1LEN_AXROUTE_AUDIO_EMI(1));

//=============================================
//  m4u global registers
//============================================
    //set m4u pagetable base address
    COM_WriteReg32(REG_MMUg_PT_BASE, (unsigned int)pt_pa_nonsec);
    COM_WriteReg32(REG_MMUg_PT_BASE_SEC, (unsigned int)pt_pa_sec);

    //REG_MMUg_L2_SEL
    regval = F_MMUg_L2_SEL_FLUSH_EN(1)  \
            |F_MMUg_L2_SEL_L2_ULTRA(1)  \
            |F_MMUg_L2_SEL_L2_SHARE(0)  \
            |F_MMUg_L2_SEL_L2_BUS_SEL(1);
    COM_WriteReg32(REG_MMUg_L2_SEL, regval);

    COM_WriteReg32(REG_MMUg_DCM, F_MMUg_DCM_ON(1));


//=============================================
// L2 registers
//=============================================
    if(gM4U_L2_enable)
    {
        regval = F_L2_GDC_BYPASS(0);
    }
    else
    {
        regval = F_L2_GDC_BYPASS(1);
    }
    
    regval |=   F_L2_GDC_PERF_MASK(GDC_PERF_MASK_HIT_MISS) \
                |F_L2_GDC_LOCK_ALERT_DIS(0) \
                |F_L2_GDC_LOCK_TH(3)    \
                |F_L2_GDC_PAUSE_OP(GDC_NO_PAUSE);
    COM_WriteReg32(REG_L2_GDC_OP, regval);


//===============================
// LARB
//===============================
    /* cache coherent operations:

        SMI_SHARE_EN    SMI_ROUTE_ENABLE    coherent_EN+pagetable_coherent  operation
            0               1                       0                       EMI
            0               1                       1                       MCI+snoop
            0               0                       0                       MCI
            0               0                       1                       MCI+snoop
            1               1                       0                       MCI+snoop
            1               1                       1                       MCI+snoop
            1               0                       0                       MCI+snoop
            1               0                       1                       MCI+snoop
    */
    
    {
        int i;
        for(i=0; i<SMI_LARB_NR; i++)
        {
            larb_clock_on(i);
            //set SMI_SHARE_EN to 0
            M4U_WriteReg32(gLarbBaseAddr[i], SMI_SHARE_EN, 0x0);
            //set SMI_ROUTE_SEL to 1
            if(i==4)
                M4U_WriteReg32(gLarbBaseAddr[i], SMI_ROUTE_SEL, 0x0);
            else
                M4U_WriteReg32(gLarbBaseAddr[i], SMI_ROUTE_SEL, 0xffffffff);

            M4UMSG("larb clock on %d\n", i);

            larb_clock_off(i);
        }

    }

//=============================================
// m4u registers
//=============================================

    for(i=0;i<TOTAL_M4U_NUM;i++)
    {
        regval = F_MMU_CTRL_PFH_DIS(0)         \
                |F_MMU_CTRL_TLB_WALK_DIS(0)    \
                |F_MMU_CTRL_MONITOR_EN(0)       \
                |F_MMU_CTRL_MONITOR_CLR(0)     \
                |F_MMU_CTRL_PFH_RT_RPL_MODE(0) \
                |F_MMU_CTRL_TF_PROT_VAL(2)    \
                |F_MMU_CTRL_COHERE_EN(1)       ;
        
        if(g_debug_enable_error_hang)
            regval |= F_MMU_CTRL_INT_HANG_en(1);
        
        M4U_WriteReg32(gM4UBaseAddr[i], REG_MMU_CTRL_REG, regval);

    //    M4UMSG("ctl = 0x%x\n", M4U_ReadReg32(gM4UBaseAddr[i], REG_MMU_CTRL_REG));
        
        //enable interrupt control except "Same VA-to-PA test"
        M4U_WriteReg32(gM4UBaseAddr[i], REG_MMU_INT_CONTROL, 0x7F);
        
        //disable non-blocking mode
        //M4U_WriteReg32(gM4UBaseAddr[i], REG_MMU_NON_BLOCKING_DIS, F_MMU_NON_BLOCK_DISABLE_BIT);
        
        M4U_WriteReg32(gM4UBaseAddr[i], REG_MMU_IVRP_PADDR, (unsigned int)ProtectPA);

        if(g_debug_dump_rs_in_isr)
            m4u_monitor_start(i);


        M4UDBG("init hw OK: %d \n",i);
    }
    //invalidate all TLB entry
    m4u_invalid_tlb_all(M4U_ID_ALL, gM4U_L2_enable);

    return 1;
}


static void m4u_clear_intr(unsigned int m4u_base)
{
    unsigned int Temp;
    Temp = M4U_ReadReg32(m4u_base, REG_MMU_INT_CONTROL) | F_INT_CLR_BIT;
    M4U_WriteReg32(m4u_base, REG_MMU_INT_CONTROL, Temp);   
}

int smi_reg_backup(void)
{
    unsigned int* pReg = pM4URegBackUp;
    int m4u_id;
    int i;

    //flag (for debug)
    *(pReg++) = COM_ReadReg32(REG_MMUg_PT_BASE);

    //m4u reg backup
    for(m4u_id=0; m4u_id<2; m4u_id++)
    {
        unsigned int m4u_base = gM4UBaseAddr[m4u_id];
        
        for(i=0; i<M4U_SEQ_NR; i++)
        {
            *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_SQ_START(i));
            *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_SQ_END(i));
        }
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIST0);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIST1);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIST2);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIST3);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIST4);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIST5);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIST6);

        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIST16_0);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DISTS16_1);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIR0);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFH_DIR1);

        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_CTRL_REG);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_IVRP_PADDR);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_INT_CONTROL);

        for(i=0; i<M4U_WRAP_NR; i++)
        {
            *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_SA(i));
            *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EA(i));
        }
        
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EN0);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EN1);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EN2);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EN3);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EN4);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EN5);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_WRAP_EN6);
        
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_PFQ_BROADCAST_EN);
        *(pReg++) = M4U_ReadReg32(m4u_base, REG_MMU_NON_BLOCKING_DIS);
        
    }

    //M4U top registers
    *(pReg++) = COM_ReadReg32(REG_MMUg_CTRL);
    *(pReg++) = COM_ReadReg32(REG_MMUg_PT_BASE);
    *(pReg++) = COM_ReadReg32(REG_MMUg_L2_SEL);
    *(pReg++) = COM_ReadReg32(REG_MMUg_DCM);
    *(pReg++) = COM_ReadReg32(REG_MMUg_CTRL_SEC);
    *(pReg++) = COM_ReadReg32(REG_MMUg_PT_BASE_SEC);
    *(pReg++) = COM_ReadReg32(REG_MMUg_INFA_CTRL);

    //L2 cache registers
    *(pReg++) = COM_ReadReg32(REG_L2_GDC_OP);
    *(pReg++) = COM_ReadReg32(REG_L2_GDC_PERF0);
    *(pReg++) = COM_ReadReg32(REG_L2_GDC_PERF1);
    *(pReg++) = COM_ReadReg32(REG_L2_GPE_STATUS_SEC);


    
    //SMI registers
    *(pReg++) = COM_ReadReg32(REG_SMI_L1LEN);
    *(pReg++) = COM_ReadReg32(REG_SMI_BUS_SEL);
    for(i=0; i<7; i++)
        *(pReg++) = COM_ReadReg32(REG_SMI_SECUR_CON(i));

    //M4UMSG("register backup buffer needs: %d \n", (unsigned int)pReg-(unsigned int)pM4URegBackUp);

    if(pt_pa_nonsec !=*pM4URegBackUp)
    {
        M4UERR("PT_BASE in memory is error after backup! expect PTPA=0x%x, backupReg=0x%x\n", 
            pt_pa_nonsec, *pM4URegBackUp);
    }


    return 0;
}

int smi_reg_restore(void)
{

    unsigned int* pReg = pM4URegBackUp;
    int m4u_id;
    int i;

    //flag (for debug)
    COM_WriteReg32(REG_MMUg_PT_BASE, *(pReg++));

    //m4u reg backup
    for(m4u_id=0; m4u_id<2; m4u_id++)
    {
        unsigned int m4u_base = gM4UBaseAddr[m4u_id];
        
        for(i=0; i<M4U_SEQ_NR; i++)
        {
            M4U_WriteReg32(m4u_base, REG_MMU_SQ_START(i), *(pReg++));
            M4U_WriteReg32(m4u_base, REG_MMU_SQ_END(i)  , *(pReg++));
        }
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIST0      , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIST1     , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIST2     , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIST3     , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIST4     , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIST5     , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIST6     , *(pReg++) );
                                                                    
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIST16_0  , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DISTS16_1 , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIR0      , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_PFH_DIR1      , *(pReg++) );
                                                                   
        M4U_WriteReg32(m4u_base, REG_MMU_CTRL_REG      , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_IVRP_PADDR    , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_INT_CONTROL   , *(pReg++) );

        for(i=0; i<M4U_WRAP_NR; i++)
        {
            M4U_WriteReg32(m4u_base, REG_MMU_WRAP_SA(i), *(pReg++) );
            M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EA(i), *(pReg++) );
        }
        
        M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EN0        , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EN1        , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EN2        , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EN3        , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EN4        , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EN5        , *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_WRAP_EN6        , *(pReg++) );
                                                                     
        M4U_WriteReg32(m4u_base, REG_MMU_PFQ_BROADCAST_EN, *(pReg++) );
        M4U_WriteReg32(m4u_base, REG_MMU_NON_BLOCKING_DIS, *(pReg++) );
                                                                     
    }                                                                
                                                                     
    //M4U top registers                                              
    COM_WriteReg32(REG_MMUg_CTRL                         , *(pReg++) );         
    COM_WriteReg32(REG_MMUg_PT_BASE                      , *(pReg++) );         
    COM_WriteReg32(REG_MMUg_L2_SEL                       , *(pReg++) );
    COM_WriteReg32(REG_MMUg_DCM                          , *(pReg++) );
    COM_WriteReg32(REG_MMUg_CTRL_SEC                     , *(pReg++) );
    COM_WriteReg32(REG_MMUg_PT_BASE_SEC                  , *(pReg++) );
    COM_WriteReg32(REG_MMUg_INFA_CTRL                    , *(pReg++) );
                                                                     
    //L2 cache registers                                             
    COM_WriteReg32(REG_L2_GDC_OP                         , *(pReg++) );
    COM_WriteReg32(REG_L2_GDC_PERF0                      , *(pReg++) );
    COM_WriteReg32(REG_L2_GDC_PERF1                      , *(pReg++) );
    COM_WriteReg32(REG_L2_GPE_STATUS_SEC                 , *(pReg++) );

    //SMI registers
    COM_WriteReg32(REG_SMI_L1LEN, *(pReg++) );
    COM_WriteReg32(REG_SMI_BUS_SEL, *(pReg++) );
    for(i=0; i<7; i++)
        COM_WriteReg32(REG_SMI_SECUR_CON(i), *(pReg++) );


    if(COM_ReadReg32(REG_MMUg_PT_BASE) != pt_pa_nonsec)
    {
    	  M4UERR("PT_BASE is error after restore! 0x%x != 0x%x\n",
            COM_ReadReg32(REG_MMUg_PT_BASE), pt_pa_nonsec);
    }
    
    return 0;
}



static char* m4u_get_module_name(M4U_MODULE_ID_ENUM moduleID)
{
    switch(moduleID)
    {
        case M4U_CLNTMOD_VENC      :  return "VENC";
        case M4U_CLNTMOD_VDEC      :  return "VDEC";
        case M4U_CLNTMOD_ROT       :  return "ROT";
        case M4U_CLNTMOD_OVL       :  return "OVL";
        case M4U_CLNTMOD_WDMA      :  return "WDMA";
        case M4U_CLNTMOD_RDMA      :  return "RDMA";
        case M4U_CLNTMOD_CMDQ      :  return "CMDQ";
        case M4U_CLNTMOD_DBI       :  return "DBI";
        case M4U_CLNTMOD_G2D       :  return "G2D";
        case M4U_CLNTMOD_JPGDEC    :  return "JPGDEC";
        case M4U_CLNTMOD_JPGENC    :  return "JPGENC";
        case M4U_CLNTMOD_VIP       :  return "VIP";
        case M4U_CLNTMOD_DISP      :  return "DISP";
        case M4U_CLNTMOD_VID       :  return "VID";
        case M4U_CLNTMOD_GDMA      :  return "GDMA";
        case M4U_CLNTMOD_IMG       :  return "IMG";
        case M4U_CLNTMOD_LSCI      :  return "LSCI";
        case M4U_CLNTMOD_FLKI      :  return "FLKI";
        case M4U_CLNTMOD_LCEI      :  return "LCEI";
        case M4U_CLNTMOD_LCSO      :  return "LCSO";
        case M4U_CLNTMOD_ESFKO     :  return "ESFKO";
        case M4U_CLNTMOD_AAO       :  return "AAO";
        case M4U_CLNTMOD_AUDIO     :  return "AUDIO";
        case M4U_CLNTMOD_UNKNOWN   :  return "UNKNOWN";
        case M4U_CLNTMOD_LCDC_UI   :  return "LCDC_UI";        
        default:
             M4UMSG("invalid module id=%d", moduleID);
             return "UN-KNOWN";    	
    	
    }
}
                  
static char* m4u_get_port_name(M4U_PORT_ID_ENUM portID)
{
    switch(portID)
    {
        case M4U_PORT_VENC_RCPU           : return "VENC_RCPU";            
        case M4U_PORT_VENC_REF_LUMA       : return "VENC_REF_LUMA";            
        case M4U_PORT_VENC_REF_CHROMA     : return "VENC_REF_CHROMA";            
        case M4U_PORT_VENC_DB_READ        : return "VENC_DB_READ";            
        case M4U_PORT_VENC_DB_WRITE       : return "VENC_DB_WRITE";            
        case M4U_PORT_VENC_CUR_LUMA       : return "VENC_CUR_LUMA";            
        case M4U_PORT_VENC_CUR_CHROMA     : return "VENC_CUR_CHROMA";            
        case M4U_PORT_VENC_RD_COMV        : return "VENC_RD_COMV";            
        case M4U_PORT_VENC_SV_COMV        : return "VENC_SV_COMV";            
        case M4U_PORT_VENC_BSDMA          : return "VENC_BSDMA";            
        case M4U_PORT_HW_VDEC_MC_EXT      : return "VDEC_MC_EXT";            
        case M4U_PORT_HW_VDEC_PP_EXT      : return "VDEC_PP_EXT";            
        case M4U_PORT_HW_VDEC_AVC_MV_EXT  : return "VDEC_AVC_MV_EXT";            
        case M4U_PORT_HW_VDEC_PRED_RD_EXT : return "VDEC_PRED_RD_EXT";            
        case M4U_PORT_HW_VDEC_PRED_WR_EXT : return "VDEC_PRED_WR_EXT";            
        case M4U_PORT_HW_VDEC_VLD_EXT     : return "VDEC_VLD_EXT";
        case M4U_PORT_HW_VDEC_VLD2_EXT    : return "VDEC_VLD2_EXT";
        case M4U_PORT_ROT_EXT             : return "ROT_EXT";
        case M4U_PORT_OVL_CH0             : return "OVL_CH0";
        case M4U_PORT_OVL_CH1             : return "OVL_CH1";
        case M4U_PORT_OVL_CH2             : return "OVL_CH2";
        case M4U_PORT_OVL_CH3             : return "OVL_CH3";
        case M4U_PORT_WDMA0               : return "WDMA0";
        case M4U_PORT_WDMA1               : return "WDMA1";
        case M4U_PORT_RDMA0               : return "RDMA0";
        case M4U_PORT_RDMA1               : return "RDMA1";
        case M4U_PORT_CMDQ                : return "CMDQ";
        case M4U_PORT_DBI                 : return "DBI";
        case M4U_PORT_G2D                 : return "G2D";
        case M4U_PORT_JPGDEC_WDMA         : return "JPGDEC_WDMA";
        case M4U_PORT_JPGENC_RDMA         : return "JPGENC_RDMA";
        case M4U_PORT_VIPI                : return "VIPI";
        case M4U_PORT_VIP2I               : return "VIP2I";
        case M4U_PORT_DISPO               : return "DISPO";
        case M4U_PORT_DISPCO              : return "DISPCO";
        case M4U_PORT_DISPVO              : return "DISPVO";
        case M4U_PORT_VIDO                : return "VIDO";
        case M4U_PORT_VIDCO               : return "VIDCO";
        case M4U_PORT_VIDVO               : return "VIDVO";
        case M4U_PORT_GDMA_SMI_RD         : return "GDMA_SMI_RD";
        case M4U_PORT_GDMA_SMI_WR         : return "GDMA_SMI_WR";
        case M4U_PORT_JPGDEC_BSDMA        : return "JPGDEC_BSDMA";
        case M4U_PORT_JPGENC_BSDMA        : return "JPGENC_BSDMA";
        case M4U_PORT_IMGI                : return "IMGI";
        case M4U_PORT_IMGCI               : return "IMGCI";
        case M4U_PORT_IMGO                : return "IMGO";
        case M4U_PORT_IMG2O               : return "IMG2O";
        case M4U_PORT_LSCI                : return "LSCI";
        case M4U_PORT_FLKI                : return "FLKI";
        case M4U_PORT_LCEI                : return "LCEI";
        case M4U_PORT_LCSO                : return "LCSO";
        case M4U_PORT_ESFKO               : return "ESFKO";
        case M4U_PORT_AAO                 : return "AAO";
        case M4U_PORT_AUDIO               : return "AUDIO";
        case M4U_PORT_UNKNOWN             : return "UNKNOWN";
        default:
            M4UMSG("invalid port id=%d", portID);
            return "UN-KNOWN";    	
    	
    }
}

#if 0
unsigned int m4u_get_pa_by_mva(unsigned int mva)
{
    unsigned int * pPageTableAddr = 0;
    pPageTableAddr = (unsigned int*)((unsigned int) pPageTableVA_nonCache + M4U_GET_PTE_OFST_TO_PT_SA(mva)); 
    if( (*pPageTableAddr & 0x2) !=0)
    {
        return *pPageTableAddr;
    }
    else
    {
        M4UMSG("error: pa is invalid, mva=0x%x, pa=0x%x \n", mva, *pPageTableAddr);
        return 0;
    }
}
#endif

static void m4u_memory_usage(bool bPrintAll)
{
    unsigned int i=0;
    for(i=0;i<M4U_CLIENT_MODULE_NUM;i++)
    {
        M4UMSG("id=%-2d, name=%-10s, max=%-5dKB, current=%-5dKB, locked_page=%-3d \n",
            i, m4u_get_module_name(i), pmodule_max_size[i]/1024, pmodule_current_size[i]/1024, 
            pmodule_locked_pages[i]);
    }    	
}    

void m4u_print_active_port(unsigned int m4u_index)
{
    unsigned int i=0;
    unsigned int regval;

    M4UINFO("active ports: ");
    {
        for(i=0;i<M4U_PORT_NR;i++)
        {
            
            regval = m4uHw_get_field_by_mask(0, REG_SMI_SECUR_CON_OF_PORT(i), F_SMI_SECUR_CON_VIRTUAL(i));
            if(regval)
            {
                printk(KERN_INFO"%s(%d), ", m4u_get_port_name(i),i);
            }
        }
        printk(KERN_INFO"\n");
    }
} 



// used to clear all TLB resource occupied by the module
int m4u_reset_mva_release_tlb(M4U_MODULE_ID_ENUM eModuleID) 
{	
    //todo: implement this func in dynamic alloc mode
    M4ULOG("Have not implemented m4u_reset_mva_release_tlb() in dynamic mva alloc mode! \n");
    
    return 0;
}


static int m4u_dump_seq_range_info(void)
{
    unsigned int i=0;

    M4UMSG(" MVA Range Info: \n");
    for(i=0;i<TOTAL_RANGE_NUM;i++)
    {
        if(1==pRangeDes[i].Enabled)
        {
            M4UMSG("pRangeDes[%d]: Enabled=%d, module=%s, MVAStart=0x%x, MVAEnd=0x%x, entrycount=%d \n", 
                i, pRangeDes[i].Enabled, m4u_get_module_name(pRangeDes[i].eModuleID), 
                pRangeDes[i].MVAStart, pRangeDes[i].MVAEnd, pRangeDes[i].entryCount);
        }
    }    
    
    return 0;	
}

int m4u_dump_info(int m4u_index) 
{
    unsigned int i=0;
	  
    M4UMSG(" MVA Range Info: \n");
    m4u_dump_seq_range_info();

    M4UMSG(" Wrap Range Info: \n");
    for(i=0;i<TOTAL_WRAP_NUM;i++)
    {
        if(1==pWrapDes[i].Enabled)
        {
            M4UMSG("pWrapDes[%d]: Enabled=%d, module=%s, MVAStart=0x%x, MVAEnd=0x%x \n", 
                i, pWrapDes[i].Enabled, m4u_get_port_name(pWrapDes[i].eModuleID), 
                pWrapDes[i].MVAStart, pWrapDes[i].MVAEnd);
        }
    } 

    m4u_dump_mva_info();
        
    return 0;
}


void m4u_get_power_status(void)
{
    
}

int m4u_log_on(void)
{
	unsigned int i=0;
	  
    M4UMSG("m4u_log_on is called! \n");  
    gM4uLogFlag = true;
    m4u_get_power_status();
    m4u_dump_mva_info();
    m4u_memory_usage(true);
    for(i=0;i<TOTAL_M4U_NUM;i++)
    {
        m4u_dump_info(i);
        m4u_print_active_port(i);
    }
    
    M4UMSG("m4u pagetable info: \n");
    m4u_mvaGraph_dump();
    
    return 0;
}

int m4u_log_off(void)
{
    M4UMSG("m4u_log_off is called! \n");  
    gM4uLogFlag = false;
    return 0;  	
}


int m4u_enable_prefetch(M4U_PORT_ID_ENUM PortID)
{
    unsigned int m4u_base = gM4UBaseAddr[m4u_port_2_m4u_id(PortID)];
    m4uHw_set_field_by_mask(m4u_base, REG_MMU_CTRL_REG, 
                  F_MMU_CTRL_PFH_DIS(1), F_MMU_CTRL_PFH_DIS(0));
    return 0;
}

int m4u_disable_prefetch(M4U_PORT_ID_ENUM PortID)
{
    unsigned int m4u_base = gM4UBaseAddr[m4u_port_2_m4u_id(PortID)];
    m4uHw_set_field_by_mask(m4u_base, REG_MMU_CTRL_REG, 
                  F_MMU_CTRL_PFH_DIS(1), F_MMU_CTRL_PFH_DIS(1));

    return 0;
}

int m4u_enable_error_hang(int m4u_id)
{
    unsigned int m4u_base = gM4UBaseAddr[m4u_id];
    m4uHw_set_field_by_mask(m4u_base, REG_MMU_CTRL_REG, 
                  F_MMU_CTRL_INT_HANG_en(1), F_MMU_CTRL_INT_HANG_en(1));

    return 0;
}

int m4u_disable_error_hang(int m4u_id)
{
    unsigned int m4u_base = gM4UBaseAddr[m4u_id];
    m4uHw_set_field_by_mask(m4u_base, REG_MMU_CTRL_REG, 
                  F_MMU_CTRL_INT_HANG_en(1), F_MMU_CTRL_INT_HANG_en(0));

    return 0;
}

int m4u_enable_L2_cache(void)
{
    unsigned int regval;
    regval = F_L2_GDC_BYPASS(0) \
            |F_L2_GDC_PERF_MASK(GDC_PERF_MASK_HIT_MISS) \
            |F_L2_GDC_LOCK_ALERT_DIS(0) \
            |F_L2_GDC_LOCK_TH(3)    \
            |F_L2_GDC_PAUSE_OP(GDC_NO_PAUSE);
    COM_WriteReg32(REG_L2_GDC_OP, regval);

    gM4U_L2_enable = 1;
    return 0;

}


int m4u_disable_L2_cache(void)
{
    unsigned int regval;
    regval = F_L2_GDC_BYPASS(1) \
            |F_L2_GDC_PERF_MASK(GDC_PERF_MASK_HIT_MISS) \
            |F_L2_GDC_LOCK_ALERT_DIS(0) \
            |F_L2_GDC_LOCK_TH(3)    \
            |F_L2_GDC_PAUSE_OP(GDC_NO_PAUSE);
    COM_WriteReg32(REG_L2_GDC_OP, regval);

    gM4U_L2_enable = 0;
    return 0;

}

int m4u_L2_enable_lock_alert(void)
{
    m4uHw_set_field_by_mask(0, REG_L2_GDC_OP, 
        F_L2_GDC_LOCK_ALERT_DIS(1), F_L2_GDC_LOCK_ALERT_DIS(0));
    return 0;
}

int m4u_l2_disable_lock_alert(void)
{
    m4uHw_set_field_by_mask(0, REG_L2_GDC_OP, 
            F_L2_GDC_LOCK_ALERT_DIS(1), F_L2_GDC_LOCK_ALERT_DIS(1));
    m4uHw_set_field_by_mask(0, REG_L2_GDC_OP, 
            F_L2_GDC_LOCK_ALERT_DIS(1), F_L2_GDC_LOCK_ALERT_DIS(0));
    return 0;
}

//======================================== for MAU IT
// find all active M4U ports' MVA range to check whether the pagetable contain PA inside [start_addr, end_addr]
int m4u_mau_check_pagetable(unsigned int start_addr, unsigned int end_addr)
{
    unsigned int mva;
    int found=0;
    M4UMSG("m4u_check_pagetable, start_addr=0x%x, end_addr=0x%x \n", 
        start_addr, end_addr);

    start_addr &= ~(M4U_PAGE_SIZE-1);
    end_addr &= ~(M4U_PAGE_SIZE-1);

    
    for(mva=0; mva<=M4U_MVA_MAX; mva+=M4U_PAGE_SIZE)
    {
        unsigned int pa = *(unsigned int*)(mva_pteAddr(mva));
        if(pa & 0x2)
        {
            pa &= ~(M4U_PAGE_SIZE-1);
            if((pa>=start_addr) && (pa<=end_addr))
            {
                M4UMSG("pa found in pagetable: pa=0x%x, module=%s\n",pa, m4u_get_module_name(mva2module(mva)));
                found=1;
            }
        }
    }
    
    if(found)
    {
        M4UMSG("m4u_check_pagetable found pa! \n");
    }
    else
    {
        M4UMSG("m4u_check_pagetable cannot found pa\n");
    }

    return 0;	
}

int m4u_mva_map_kernel(unsigned int mva, unsigned int size, int sec,
                        unsigned int* map_va, unsigned int* map_size)
{
    struct page **pages;
    unsigned int page_num, map_page_num;
    unsigned int kernel_va, kernel_size;

    kernel_va = 0;
    kernel_size = 0;

    page_num = M4U_GET_PAGE_NUM(mva, size);
    pages = vmalloc(sizeof(struct page*)*page_num);
    if(pages == NULL)
    {
        M4UMSG("mva_map_kernel: error to vmalloc for %d\n", sizeof(struct page*)*page_num);
    }

    for(map_page_num=0; map_page_num<page_num; map_page_num++)
    {
        unsigned int pa;
        if(sec)
            pa = *(unsigned int*)mva_pteAddr_sec(mva+map_page_num*M4U_PAGE_SIZE);
        else
            pa = *(unsigned int*)mva_pteAddr_nonsec(mva+map_page_num*M4U_PAGE_SIZE);

        if((pa&F_DESC_VALID) != F_DESC_VALID)
        {
            break;
        }

        pages[map_page_num] = phys_to_page(pa);
    }

    if(map_page_num != page_num)
    {
        M4UMSG("mva_map_kernel: only get %d pages: mva=0x%x, size=0x%x\n", 
            map_page_num, mva, size);
        goto error_out;
    }
    
    kernel_va = vmap(pages, map_page_num, VM_MAP, PAGE_KERNEL);
    if(kernel_va == 0)
    {
        M4UMSG("mva_map_kernel: vmap fail: page_num=%d\n", map_page_num);
        goto error_out;
    }

    kernel_va += mva & (M4U_PAGE_MASK);
    
    *map_va = kernel_va;
    *map_size = size;

error_out:
    vfree(pages);
    M4ULOG("mva_map_kernel: mva=0x%x,size=0x%x,sec=0x%x,map_va=0x%x,map_size=0x%x\n",
        mva, size, sec, *map_va, *map_size);
    return 0;
    
}

EXPORT_SYMBOL(m4u_mva_map_kernel);

int m4u_mva_unmap_kernel(unsigned int mva, unsigned int size, unsigned int va)
{
    M4ULOG("mva_unmap_kernel: mva=0x%x,size=0x%x,va=0x%x\n", mva, size, va);
    vunmap(va&(~M4U_PAGE_MASK));
    return 0;
}
EXPORT_SYMBOL(m4u_mva_unmap_kernel);


int m4u_dump_user_addr_register(M4U_PORT_ID_ENUM port)
{
    int i;
    M4UINFO("dump user register: %d\n", port);
    switch(port)
    {
        case M4U_PORT_VENC_RCPU:
            M4UINFO("f700208C=0x%x\n", COM_ReadReg32(0xf700208C));
            M4UINFO("f7002090=0x%x\n", COM_ReadReg32(0xf7002090));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));

            break;
        case M4U_PORT_VENC_REF_LUMA:
            M4UINFO("f7002064=0x%x\n", COM_ReadReg32(0xf7002064));
            M4UINFO("f70020E0=0x%x\n", COM_ReadReg32(0xf70020E0));
            M4UINFO("f7002660=0x%x\n", COM_ReadReg32(0xf7002660));
            M4UINFO("f7002068=0x%x\n", COM_ReadReg32(0xf7002068));
            M4UINFO("f7002098=0x%x\n", COM_ReadReg32(0xf7002098));
            M4UINFO("f70020E4=0x%x\n", COM_ReadReg32(0xf70020E4));
            M4UINFO("f70020E8=0x%x\n", COM_ReadReg32(0xf70020E8));
            M4UINFO("f7002680=0x%x\n", COM_ReadReg32(0xf7002680));
            M4UINFO("f7002684=0x%x\n", COM_ReadReg32(0xf7002684));
            M4UINFO("f7002688=0x%x\n", COM_ReadReg32(0xf7002688));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));


            break;
        case M4U_PORT_VENC_REF_CHROMA:
            M4UINFO("f700207C=0x%x\n", COM_ReadReg32(0xf700207C));
            M4UINFO("f7002080=0x%x\n", COM_ReadReg32(0xf7002080));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));

            break;
        case M4U_PORT_VENC_DB_READ:
            M4UINFO("f700207C=0x%x\n", COM_ReadReg32(0xf700207C));
            M4UINFO("f7002080=0x%x\n", COM_ReadReg32(0xf7002080));
            M4UINFO("f7002648=0x%x\n", COM_ReadReg32(0xf7002648));
            M4UINFO("f700264C=0x%x\n", COM_ReadReg32(0xf700264C));
            M4UINFO("f7002650=0x%x\n", COM_ReadReg32(0xf7002650));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));

            break;
        case M4U_PORT_VENC_DB_WRITE:
            M4UINFO("f7002088=0x%x\n", COM_ReadReg32(0xf7002088));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));

            break;
        case M4U_PORT_VENC_CUR_LUMA:
            M4UINFO("f7002084=0x%x\n", COM_ReadReg32(0xf7002084));
            M4UINFO("f7002638=0x%x\n", COM_ReadReg32(0xf7002638));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));

            break;
        case M4U_PORT_VENC_CUR_CHROMA:
            M4UINFO("f7002070=0x%x\n", COM_ReadReg32(0xf7002070));
            M4UINFO("f7002130=0x%x\n", COM_ReadReg32(0xf7002130));
            M4UINFO("f700263C=0x%x\n", COM_ReadReg32(0xf700263C));
            M4UINFO("f7002640=0x%x\n", COM_ReadReg32(0xf7002640));
            M4UINFO("f7002644=0x%x\n", COM_ReadReg32(0xf7002644));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));

            break;
        case M4U_PORT_VENC_RD_COMV:
            M4UINFO("f7002078=0x%x\n", COM_ReadReg32(0xf7002078));
            M4UINFO("f7002638=0x%x\n", COM_ReadReg32(0xf7002638));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));

            break;
        case M4U_PORT_VENC_SV_COMV:
            M4UINFO("f700206C=0x%x\n", COM_ReadReg32(0xf700206C));
            M4UINFO("f700263C=0x%x\n", COM_ReadReg32(0xf700263C));
            M4UINFO("f7002640=0x%x\n", COM_ReadReg32(0xf7002640));
            M4UINFO("f7002644=0x%x\n", COM_ReadReg32(0xf7002644));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));

            break;
        case M4U_PORT_VENC_BSDMA:
            M4UINFO("f7002074=0x%x\n", COM_ReadReg32(0xf7002074));
            M4UINFO("f7002654=0x%x\n", COM_ReadReg32(0xf7002654));
            M4UINFO("f7002658=0x%x\n", COM_ReadReg32(0xf7002658));
            M4UINFO("f700265C=0x%x\n", COM_ReadReg32(0xf700265C));
            M4UINFO("f70020ec=0x%x\n", COM_ReadReg32(0xf70020ec));
            M4UINFO("f70020f4=0x%x\n", COM_ReadReg32(0xf70020f4));
            M4UINFO("f7002094=0x%x\n", COM_ReadReg32(0xf7002094));

            break;

            
        case M4U_PORT_HW_VDEC_MC_EXT:
            for(i=0; i<=0xa0; i+=4)
                M4UINFO("0xf6022000+%d = 0x%x\n", i, COM_ReadReg32(0xf6022000+i));
            M4UINFO("0xf60223e0: 0x%x,0x%x,0x%x\n", 
                COM_ReadReg32(0xf60223e0),COM_ReadReg32(0xf60223e4),COM_ReadReg32(0xf60223e8));
            M4UINFO("0xf6022560 = 0x%x", COM_ReadReg32(0xf6022560));
            for(i=0x3dc; i<=0x458; i+=4)
                M4UINFO("0xf6022000+%d = 0x%x\n", i, COM_ReadReg32(0xf6022000+i));
            for(i=0x45c; i<=0x558; i+=4)
                M4UINFO("0xf6022000+%d = 0x%x\n", i, COM_ReadReg32(0xf6022000+i));
            
            break;
        case M4U_PORT_HW_VDEC_PP_EXT:
            M4UINFO("0xf6025224: 0x%x,0x%x\n", COM_ReadReg32(0xf6025224),COM_ReadReg32(0xf6025228));
            M4UINFO("0xf6025a24: 0x%x\n",COM_ReadReg32(0xf6025a24));
            M4UINFO("0xf60252c8: 0x%x\n",COM_ReadReg32(0xf60252c8));
            M4UINFO("0xf60252cc: 0x%x\n",COM_ReadReg32(0xf60252cc));
            break;
        case M4U_PORT_HW_VDEC_AVC_MV_EXT:
            M4UINFO("0xf602420c: 0x%x\n",COM_ReadReg32(0xf602420c));
            break;
        case M4U_PORT_HW_VDEC_PRED_RD_EXT:
            M4UINFO("0xf6021828: 0x%x\n",COM_ReadReg32(0xf6021828));
            M4UINFO("0xf60212d4: 0x%x\n",COM_ReadReg32(0xf60212d4));
            M4UINFO("0xf60212d8: 0x%x\n",COM_ReadReg32(0xf60212d8));
            M4UINFO("0xf602129c: 0x%x\n",COM_ReadReg32(0xf602129c));
            M4UINFO("0xf60212a0: 0x%x\n",COM_ReadReg32(0xf60212a0));
            M4UINFO("0xf60212a4: 0x%x\n",COM_ReadReg32(0xf60212a4));
            M4UINFO("0xf6022a20: 0x%x\n",COM_ReadReg32(0xf6022a20));
            M4UINFO("0xf602183c: 0x%x\n",COM_ReadReg32(0xf602183c));
            break;
        case M4U_PORT_HW_VDEC_PRED_WR_EXT:
            M4UINFO("0xf6021828: 0x%x\n",COM_ReadReg32(0xf6021828));
            M4UINFO("0xf6022a20: 0x%x\n",COM_ReadReg32(0xf6022a20));
            M4UINFO("0xf602183c: 0x%x\n",COM_ReadReg32(0xf602183c));
            break;
        case M4U_PORT_HW_VDEC_VLD_EXT:
            M4UINFO("0xf60210b0: 0x%x\n",COM_ReadReg32(0xf60210b0));
            M4UINFO("0xf6021110: 0x%x\n",COM_ReadReg32(0xf6021110));
            M4UINFO("0xf60210b4: 0x%x\n",COM_ReadReg32(0xf60210b4));
            M4UINFO("0xf60210b8: 0x%x\n",COM_ReadReg32(0xf60210b8));
            break;
        case M4U_PORT_HW_VDEC_VLD2_EXT:
            M4UINFO("0xf60278b4: 0x%x\n",COM_ReadReg32(0xf60278b4));
            M4UINFO("0xf60278b8: 0x%x\n",COM_ReadReg32(0xf60278b8));
            break;

            
        case M4U_PORT_ROT_EXT:
            M4UINFO("DISP_REG_ROT_SRC_BASE_0: 0x%x\n",COM_ReadReg32(DISP_REG_ROT_SRC_BASE_0));
            M4UINFO("DISP_REG_ROT_SRC_BASE_1: 0x%x\n",COM_ReadReg32(DISP_REG_ROT_SRC_BASE_1));
            M4UINFO("DISP_REG_ROT_SRC_BASE_2: 0x%x\n",COM_ReadReg32(DISP_REG_ROT_SRC_BASE_2));
            M4UINFO("DISP_REG_ROT_MF_SRC_SIZE: 0x%x\n",COM_ReadReg32(DISP_REG_ROT_MF_SRC_SIZE));
            M4UINFO("DISP_REG_ROT_MF_CLIP_SIZE: 0x%x\n",COM_ReadReg32(DISP_REG_ROT_MF_CLIP_SIZE));
            M4UINFO("DISP_REG_ROT_EN: 0x%x\n",COM_ReadReg32(DISP_REG_ROT_EN));
            M4UINFO("DISP_REG_ROT_SRC_CON: 0x%x\n",COM_ReadReg32(DISP_REG_ROT_SRC_CON));
            M4UINFO("DISP_REG_ROT_CON: 0x%x\n",COM_ReadReg32(DISP_REG_ROT_CON));

            break;
        case M4U_PORT_OVL_CH0:
            M4UINFO("DISP_REG_OVL_L0_ADDR: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L0_ADDR));
            M4UINFO("DISP_REG_OVL_L0_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L0_CON));
            M4UINFO("DISP_REG_OVL_L0_SRC_SIZE: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L0_SRC_SIZE));
            M4UINFO("DISP_REG_OVL_EN: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_EN));
            M4UINFO("DISP_REG_OVL_DATAPATH_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_DATAPATH_CON));
            M4UINFO("DISP_REG_OVL_SRC_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_SRC_CON));
            M4UINFO("DISP_REG_OVL_RDMA0_CTRL: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_RDMA0_CTRL));

            break;
        case M4U_PORT_OVL_CH1:
            M4UINFO("DISP_REG_OVL_L1_ADDR: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L1_ADDR));
            M4UINFO("DISP_REG_OVL_L1_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L1_CON));
            M4UINFO("DISP_REG_OVL_L1_SRC_SIZE: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L1_SRC_SIZE));
            M4UINFO("DISP_REG_OVL_EN: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_EN));
            M4UINFO("DISP_REG_OVL_DATAPATH_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_DATAPATH_CON));
            M4UINFO("DISP_REG_OVL_SRC_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_SRC_CON));
            M4UINFO("DISP_REG_OVL_RDMA1_CTRL: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_RDMA1_CTRL));
            break;
        case M4U_PORT_OVL_CH2:
            M4UINFO("DISP_REG_OVL_L2_ADDR: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L2_ADDR));
            M4UINFO("DISP_REG_OVL_L2_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L2_CON));
            M4UINFO("DISP_REG_OVL_L2_SRC_SIZE: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L2_SRC_SIZE));
            M4UINFO("DISP_REG_OVL_EN: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_EN));
            M4UINFO("DISP_REG_OVL_DATAPATH_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_DATAPATH_CON));
            M4UINFO("DISP_REG_OVL_SRC_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_SRC_CON));
            M4UINFO("DISP_REG_OVL_RDMA2_CTRL: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_RDMA2_CTRL));
            break;
        case M4U_PORT_OVL_CH3:
            M4UINFO("DISP_REG_OVL_L3_ADDR: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L3_ADDR));
            M4UINFO("DISP_REG_OVL_L3_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L3_CON));
            M4UINFO("DISP_REG_OVL_L3_SRC_SIZE: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_L3_SRC_SIZE));
            M4UINFO("DISP_REG_OVL_EN: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_EN));
            M4UINFO("DISP_REG_OVL_DATAPATH_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_DATAPATH_CON));
            M4UINFO("DISP_REG_OVL_SRC_CON: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_SRC_CON));
            M4UINFO("DISP_REG_OVL_RDMA3_CTRL: 0x%x\n",COM_ReadReg32(DISP_REG_OVL_RDMA3_CTRL));
            break;
        case M4U_PORT_WDMA0:
            M4UINFO("DISP_REG_WDMA_DST_ADDR: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_DST_ADDR));
            M4UINFO("DISP_REG_WDMA_DST_U_ADDR: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_DST_U_ADDR));
            M4UINFO("DISP_REG_WDMA_DST_UV_PITCH: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_DST_UV_PITCH));
            M4UINFO("DISP_REG_WDMA_SRC_SIZE: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_SRC_SIZE));
            M4UINFO("DISP_REG_WDMA_CLIP_SIZE: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_CLIP_SIZE));
            M4UINFO("DISP_REG_WDMA_EN: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_EN));
            M4UINFO("DISP_REG_WDMA_CFG: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_CFG));
            break;
        case M4U_PORT_WDMA1:
            M4UINFO("DISP_REG_WDMA_DST_ADDR+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_DST_ADDR+0x1000));
            M4UINFO("DISP_REG_WDMA_DST_U_ADDR+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_DST_U_ADDR+0x1000));
            M4UINFO("DISP_REG_WDMA_DST_UV_PITCH+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_DST_UV_PITCH+0x1000));
            M4UINFO("DISP_REG_WDMA_SRC_SIZE+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_SRC_SIZE+0x1000));
            M4UINFO("DISP_REG_WDMA_CLIP_SIZE+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_CLIP_SIZE+0x1000));
            M4UINFO("DISP_REG_WDMA_EN+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_EN+0x1000));
            M4UINFO("DISP_REG_WDMA_CFG+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_WDMA_CFG+0x1000));
            break;
        case M4U_PORT_RDMA0:
            M4UINFO("DISP_REG_RDMA_MEM_START_ADDR: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_MEM_START_ADDR));
            M4UINFO("DISP_REG_RDMA_SIZE_CON_0: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_SIZE_CON_0));
            M4UINFO("DISP_REG_RDMA_SIZE_CON_1: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_SIZE_CON_1));
            M4UINFO("DISP_REG_RDMA_GLOBAL_CON: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_GLOBAL_CON));
            M4UINFO("DISP_REG_RDMA_MEM_CON: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_MEM_CON));
            break;
        case M4U_PORT_RDMA1:
            M4UINFO("DISP_REG_RDMA_MEM_START_ADDR+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_MEM_START_ADDR+0x1000));
            M4UINFO("DISP_REG_RDMA_SIZE_CON_0+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_SIZE_CON_0+0x1000));
            M4UINFO("DISP_REG_RDMA_SIZE_CON_1+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_SIZE_CON_1+0x1000));
            M4UINFO("DISP_REG_RDMA_GLOBAL_CON+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_GLOBAL_CON+0x1000));
            M4UINFO("DISP_REG_RDMA_MEM_CON+0x1000: 0x%x\n",COM_ReadReg32(DISP_REG_RDMA_MEM_CON+0x1000));
            break;
        case M4U_PORT_CMDQ:
            for(i=0;i<2;i++)
            {
                M4UINFO("DISP_REG_CMDQ_THRx_PC(i): 0x%x\n",COM_ReadReg32(DISP_REG_CMDQ_THRx_PC(i)));
                M4UINFO("DISP_REG_CMDQ_THRx_END_ADDR(i): 0x%x\n",COM_ReadReg32(DISP_REG_CMDQ_THRx_END_ADDR(i)));
                M4UINFO("DISP_REG_CMDQ_THRx_EXEC_CMDS_CNT(i): 0x%x\n",COM_ReadReg32(DISP_REG_CMDQ_THRx_EXEC_CMDS_CNT(i)));
            }
            break;
        case M4U_PORT_DBI:
            
            break;
        case M4U_PORT_G2D:
            
            break;

            
        case M4U_PORT_JPGDEC_WDMA:
            
            break;
        case M4U_PORT_JPGENC_RDMA:

            break;
        case M4U_PORT_VIPI:
            M4UINFO("0xF50042C0 = 0x%x\n", COM_ReadReg32(0xF50042C0));
            M4UINFO("0xF50042C4 = 0x%x\n", COM_ReadReg32(0xF50042C4));
            M4UINFO("0xF50042C8 = 0x%x\n", COM_ReadReg32(0xF50042C8));
            M4UINFO("0xF50042CC = 0x%x\n", COM_ReadReg32(0xF50042CC));
            M4UINFO("0xF50042D0 = 0x%x\n", COM_ReadReg32(0xF50042D0));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        case M4U_PORT_IMGI:
            M4UINFO("0xF5004230 = 0x%x\n", COM_ReadReg32(0xF5004230));
            M4UINFO("0xF5004234 = 0x%x\n", COM_ReadReg32(0xF5004234));
            M4UINFO("0xF5004238 = 0x%x\n", COM_ReadReg32(0xF5004238));
            M4UINFO("0xF500423C = 0x%x\n", COM_ReadReg32(0xF500423C));
            M4UINFO("0xF5004240 = 0x%x\n", COM_ReadReg32(0xF5004240));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        case M4U_PORT_DISPO:
            M4UINFO("0xF5004D40 = 0x%x\n", COM_ReadReg32(0xF5004D40));
            M4UINFO("0xF5004D64 = 0x%x\n", COM_ReadReg32(0xF5004D64));
            M4UINFO("0xF5004D68 = 0x%x\n", COM_ReadReg32(0xF5004D68));
            M4UINFO("0xF5004D6C = 0x%x\n", COM_ReadReg32(0xF5004D6C));
            M4UINFO("0xF5004D70 = 0x%x\n", COM_ReadReg32(0xF5004D70));
            M4UINFO("0xF5004D74 = 0x%x\n", COM_ReadReg32(0xF5004D74));
            M4UINFO("0xF5004D78 = 0x%x\n", COM_ReadReg32(0xF5004D78));
            M4UINFO("0xF5004D7C = 0x%x\n", COM_ReadReg32(0xF5004D7C));
            M4UINFO("0xF5004DA4 = 0x%x\n", COM_ReadReg32(0xF5004DA4));
            M4UINFO("0xF5004DA8 = 0x%x\n", COM_ReadReg32(0xF5004DA8));
            M4UINFO("0xF5004DAC = 0x%x\n", COM_ReadReg32(0xF5004DAC));
            M4UINFO("0xF5004054 = 0x%x\n", COM_ReadReg32(0xF5004054));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF5004008 = 0x%x\n", COM_ReadReg32(0xF5004008));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            M4UINFO("0xF5004010 = 0x%x\n", COM_ReadReg32(0xF5004010));
            M4UINFO("0xF5004234 = 0x%x\n", COM_ReadReg32(0xF5004234));
            M4UINFO("0xF5004238 = 0x%x\n", COM_ReadReg32(0xF5004238));
            M4UINFO("0xF500423C = 0x%x\n", COM_ReadReg32(0xF500423C));
            M4UINFO("0xF5004240 = 0x%x\n", COM_ReadReg32(0xF5004240));
            COM_WriteReg32(0xF5004160,0x6000);
            M4UINFO("(CQ)0xF5004164 = 0x%x\n", COM_ReadReg32(0xF5004164));
            break;
        case M4U_PORT_DISPCO:
            M4UINFO("0xF5004D40 = 0x%x\n", COM_ReadReg32(0xF5004D40));
            M4UINFO("0xF5004D64 = 0x%x\n", COM_ReadReg32(0xF5004D64));
            M4UINFO("0xF5004D68 = 0x%x\n", COM_ReadReg32(0xF5004D68));
            M4UINFO("0xF5004D6C = 0x%x\n", COM_ReadReg32(0xF5004D6C));
            M4UINFO("0xF5004D70 = 0x%x\n", COM_ReadReg32(0xF5004D70));
            M4UINFO("0xF5004D74 = 0x%x\n", COM_ReadReg32(0xF5004D74));
            M4UINFO("0xF5004D78 = 0x%x\n", COM_ReadReg32(0xF5004D78));
            M4UINFO("0xF5004D7C = 0x%x\n", COM_ReadReg32(0xF5004D7C));
            M4UINFO("0xF5004DA4 = 0x%x\n", COM_ReadReg32(0xF5004DA4));
            M4UINFO("0xF5004DA8 = 0x%x\n", COM_ReadReg32(0xF5004DA8));
            M4UINFO("0xF5004DAC = 0x%x\n", COM_ReadReg32(0xF5004DAC));
            M4UINFO("0xF5004054 = 0x%x\n", COM_ReadReg32(0xF5004054));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF5004008 = 0x%x\n", COM_ReadReg32(0xF5004008));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            M4UINFO("0xF5004010 = 0x%x\n", COM_ReadReg32(0xF5004010));
            M4UINFO("0xF5004234 = 0x%x\n", COM_ReadReg32(0xF5004234));
            M4UINFO("0xF5004238 = 0x%x\n", COM_ReadReg32(0xF5004238));
            M4UINFO("0xF500423C = 0x%x\n", COM_ReadReg32(0xF500423C));
            M4UINFO("0xF5004240 = 0x%x\n", COM_ReadReg32(0xF5004240));
            COM_WriteReg32(0xF5004160,0x6000);
            M4UINFO("(CQ)0xF5004164 = 0x%x\n", COM_ReadReg32(0xF5004164));
            break;
        case M4U_PORT_DISPVO:
            M4UINFO("0xF5004D40 = 0x%x\n", COM_ReadReg32(0xF5004D40));
            M4UINFO("0xF5004D64 = 0x%x\n", COM_ReadReg32(0xF5004D64));
            M4UINFO("0xF5004D68 = 0x%x\n", COM_ReadReg32(0xF5004D68));
            M4UINFO("0xF5004D6C = 0x%x\n", COM_ReadReg32(0xF5004D6C));
            M4UINFO("0xF5004D70 = 0x%x\n", COM_ReadReg32(0xF5004D70));
            M4UINFO("0xF5004D74 = 0x%x\n", COM_ReadReg32(0xF5004D74));
            M4UINFO("0xF5004D78 = 0x%x\n", COM_ReadReg32(0xF5004D78));
            M4UINFO("0xF5004D7C = 0x%x\n", COM_ReadReg32(0xF5004D7C));
            M4UINFO("0xF5004DA4 = 0x%x\n", COM_ReadReg32(0xF5004DA4));
            M4UINFO("0xF5004DA8 = 0x%x\n", COM_ReadReg32(0xF5004DA8));
            M4UINFO("0xF5004DAC = 0x%x\n", COM_ReadReg32(0xF5004DAC));
            M4UINFO("0xF5004054 = 0x%x\n", COM_ReadReg32(0xF5004054));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF5004008 = 0x%x\n", COM_ReadReg32(0xF5004008));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            M4UINFO("0xF5004010 = 0x%x\n", COM_ReadReg32(0xF5004010));
            M4UINFO("0xF5004234 = 0x%x\n", COM_ReadReg32(0xF5004234));
            M4UINFO("0xF5004238 = 0x%x\n", COM_ReadReg32(0xF5004238));
            M4UINFO("0xF500423C = 0x%x\n", COM_ReadReg32(0xF500423C));
            M4UINFO("0xF5004240 = 0x%x\n", COM_ReadReg32(0xF5004240));
            COM_WriteReg32(0xF5004160,0x6000);
            M4UINFO("(CQ)0xF5004164 = 0x%x\n", COM_ReadReg32(0xF5004164));
            break;
        case M4U_PORT_VIDO:
            M4UINFO("0xF5004CE4 = 0x%x\n", COM_ReadReg32(0xF5004CE4));
            M4UINFO("0xF5004CE8 = 0x%x\n", COM_ReadReg32(0xF5004CE8));
            M4UINFO("0xF5004CEC = 0x%x\n", COM_ReadReg32(0xF5004CEC));
            M4UINFO("0xF5004CF0 = 0x%x\n", COM_ReadReg32(0xF5004CF0));
            M4UINFO("0xF5004CF4 = 0x%x\n", COM_ReadReg32(0xF5004CF4));
            M4UINFO("0xF5004CF8 = 0x%x\n", COM_ReadReg32(0xF5004CF8));
            M4UINFO("0xF5004CFC = 0x%x\n", COM_ReadReg32(0xF5004CFC));
            M4UINFO("0xF5004D24 = 0x%x\n", COM_ReadReg32(0xF5004D24));
            M4UINFO("0xF5004D28 = 0x%x\n", COM_ReadReg32(0xF5004D28));
            M4UINFO("0xF5004D2C = 0x%x\n", COM_ReadReg32(0xF5004D2C));
            M4UINFO("0xF5004054 = 0x%x\n", COM_ReadReg32(0xF5004054));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF5004008 = 0x%x\n", COM_ReadReg32(0xF5004008));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            M4UINFO("0xF5004010 = 0x%x\n", COM_ReadReg32(0xF5004010));
            M4UINFO("0xF5004234 = 0x%x\n", COM_ReadReg32(0xF5004234));
            M4UINFO("0xF5004238 = 0x%x\n", COM_ReadReg32(0xF5004238));
            M4UINFO("0xF500423C = 0x%x\n", COM_ReadReg32(0xF500423C));
            M4UINFO("0xF5004240 = 0x%x\n", COM_ReadReg32(0xF5004240));
            COM_WriteReg32(0xF5004160,0x6000);
            M4UINFO("(CQ)0xF5004164 = 0x%x\n", COM_ReadReg32(0xF5004164));
            break;
        case M4U_PORT_VIDCO:
            M4UINFO("0xF5004CE4 = 0x%x\n", COM_ReadReg32(0xF5004CE4));
            M4UINFO("0xF5004CE8 = 0x%x\n", COM_ReadReg32(0xF5004CE8));
            M4UINFO("0xF5004CEC = 0x%x\n", COM_ReadReg32(0xF5004CEC));
            M4UINFO("0xF5004CF0 = 0x%x\n", COM_ReadReg32(0xF5004CF0));
            M4UINFO("0xF5004CF4 = 0x%x\n", COM_ReadReg32(0xF5004CF4));
            M4UINFO("0xF5004CF8 = 0x%x\n", COM_ReadReg32(0xF5004CF8));
            M4UINFO("0xF5004CFC = 0x%x\n", COM_ReadReg32(0xF5004CFC));
            M4UINFO("0xF5004D24 = 0x%x\n", COM_ReadReg32(0xF5004D24));
            M4UINFO("0xF5004D28 = 0x%x\n", COM_ReadReg32(0xF5004D28));
            M4UINFO("0xF5004D2C = 0x%x\n", COM_ReadReg32(0xF5004D2C));
            M4UINFO("0xF5004054 = 0x%x\n", COM_ReadReg32(0xF5004054));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF5004008 = 0x%x\n", COM_ReadReg32(0xF5004008));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            M4UINFO("0xF5004010 = 0x%x\n", COM_ReadReg32(0xF5004010));
            M4UINFO("0xF5004234 = 0x%x\n", COM_ReadReg32(0xF5004234));
            M4UINFO("0xF5004238 = 0x%x\n", COM_ReadReg32(0xF5004238));
            M4UINFO("0xF500423C = 0x%x\n", COM_ReadReg32(0xF500423C));
            M4UINFO("0xF5004240 = 0x%x\n", COM_ReadReg32(0xF5004240));
            COM_WriteReg32(0xF5004160,0x6000);
            M4UINFO("(CQ)0xF5004164 = 0x%x\n", COM_ReadReg32(0xF5004164));
            break;
        case M4U_PORT_VIDVO:
            M4UINFO("0xF5004CE4 = 0x%x\n", COM_ReadReg32(0xF5004CE4));
            M4UINFO("0xF5004CE8 = 0x%x\n", COM_ReadReg32(0xF5004CE8));
            M4UINFO("0xF5004CEC = 0x%x\n", COM_ReadReg32(0xF5004CEC));
            M4UINFO("0xF5004CF0 = 0x%x\n", COM_ReadReg32(0xF5004CF0));
            M4UINFO("0xF5004CF4 = 0x%x\n", COM_ReadReg32(0xF5004CF4));
            M4UINFO("0xF5004CF8 = 0x%x\n", COM_ReadReg32(0xF5004CF8));
            M4UINFO("0xF5004CFC = 0x%x\n", COM_ReadReg32(0xF5004CFC));
            M4UINFO("0xF5004D24 = 0x%x\n", COM_ReadReg32(0xF5004D24));
            M4UINFO("0xF5004D28 = 0x%x\n", COM_ReadReg32(0xF5004D28));
            M4UINFO("0xF5004D2C = 0x%x\n", COM_ReadReg32(0xF5004D2C));
            M4UINFO("0xF5004054 = 0x%x\n", COM_ReadReg32(0xF5004054));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF5004008 = 0x%x\n", COM_ReadReg32(0xF5004008));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            M4UINFO("0xF5004010 = 0x%x\n", COM_ReadReg32(0xF5004010));
            M4UINFO("0xF5004234 = 0x%x\n", COM_ReadReg32(0xF5004234));
            M4UINFO("0xF5004238 = 0x%x\n", COM_ReadReg32(0xF5004238));
            M4UINFO("0xF500423C = 0x%x\n", COM_ReadReg32(0xF500423C));
            M4UINFO("0xF5004240 = 0x%x\n", COM_ReadReg32(0xF5004240));
            COM_WriteReg32(0xF5004160,0x6000);
            M4UINFO("(CQ)0xF5004164 = 0x%x\n", COM_ReadReg32(0xF5004164));
            break;
        case M4U_PORT_VIP2I:
            M4UINFO("0xF50042E0 = 0x%x\n", COM_ReadReg32(0xF50042E0));
            M4UINFO("0xF50042E4 = 0x%x\n", COM_ReadReg32(0xF50042E4));
            M4UINFO("0xF50042E8 = 0x%x\n", COM_ReadReg32(0xF50042E8));
            M4UINFO("0xF50042EC = 0x%x\n", COM_ReadReg32(0xF50042EC));
            M4UINFO("0xF50042F0 = 0x%x\n", COM_ReadReg32(0xF50042F0));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            COM_WriteReg32(0xF5004160,0x6000);
            M4UINFO("(CQ)0xF5004164 = 0x%x\n", COM_ReadReg32(0xF5004164));
            break;
        case M4U_PORT_GDMA_SMI_WR:
            break;
        case M4U_PORT_JPGDEC_BSDMA:
            
            break;
        case M4U_PORT_JPGENC_BSDMA:
            
            break;

            
        case M4U_PORT_GDMA_SMI_RD:

            break;
        case M4U_PORT_IMGCI:
            M4UINFO("0xF5004250 = 0x%x\n", COM_ReadReg32(0xF5004250));
            M4UINFO("0xF5004254 = 0x%x\n", COM_ReadReg32(0xF5004254));
            M4UINFO("0xF5004258 = 0x%x\n", COM_ReadReg32(0xF5004258));
            M4UINFO("0xF500425C = 0x%x\n", COM_ReadReg32(0xF500425C));
            M4UINFO("0xF5004260 = 0x%x\n", COM_ReadReg32(0xF5004260));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        case M4U_PORT_IMGO:
            M4UINFO("0xF5004300 = 0x%x\n", COM_ReadReg32(0xF5004300));
            M4UINFO("0xF5004304 = 0x%x\n", COM_ReadReg32(0xF5004304));
            M4UINFO("0xF5004308 = 0x%x\n", COM_ReadReg32(0xF5004308));
            M4UINFO("0xF500430C = 0x%x\n", COM_ReadReg32(0xF500430C));
            M4UINFO("0xF5004310 = 0x%x\n", COM_ReadReg32(0xF5004310));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        case M4U_PORT_IMG2O:
            M4UINFO("0xF5004320 = 0x%x\n", COM_ReadReg32(0xF5004320));
            M4UINFO("0xF5004324 = 0x%x\n", COM_ReadReg32(0xF5004324));
            M4UINFO("0xF5004328 = 0x%x\n", COM_ReadReg32(0xF5004328));
            M4UINFO("0xF500432C = 0x%x\n", COM_ReadReg32(0xF500432C));
            M4UINFO("0xF5004330 = 0x%x\n", COM_ReadReg32(0xF5004330));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        case M4U_PORT_LSCI:
            M4UINFO("0xF500426C = 0x%x\n", COM_ReadReg32(0xF500426C));
            M4UINFO("0xF5004270 = 0x%x\n", COM_ReadReg32(0xF5004270));
            M4UINFO("0xF5004274 = 0x%x\n", COM_ReadReg32(0xF5004274));
            M4UINFO("0xF5004278 = 0x%x\n", COM_ReadReg32(0xF5004278));
            M4UINFO("0xF500427C = 0x%x\n", COM_ReadReg32(0xF500427C));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        case M4U_PORT_FLKI:
            M4UINFO("0xF5004288 = 0x%x\n", COM_ReadReg32(0xF5004288));
            M4UINFO("0xF500428C = 0x%x\n", COM_ReadReg32(0xF500428C));
            M4UINFO("0xF5004290 = 0x%x\n", COM_ReadReg32(0xF5004290));
            M4UINFO("0xF5004294 = 0x%x\n", COM_ReadReg32(0xF5004294));
            M4UINFO("0xF5004298 = 0x%x\n", COM_ReadReg32(0xF5004298));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        case M4U_PORT_LCEI:
            M4UINFO("0xF50042A4 = 0x%x\n", COM_ReadReg32(0xF50042A4));
            M4UINFO("0xF50042A8 = 0x%x\n", COM_ReadReg32(0xF50042A8));
            M4UINFO("0xF50042AC = 0x%x\n", COM_ReadReg32(0xF50042AC));
            M4UINFO("0xF50042B0 = 0x%x\n", COM_ReadReg32(0xF50042B0));
            M4UINFO("0xF50042B4 = 0x%x\n", COM_ReadReg32(0xF50042B4));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        case M4U_PORT_LCSO:
            M4UINFO("0xF5004340 = 0x%x\n", COM_ReadReg32(0xF5004340));
            M4UINFO("0xF5004344 = 0x%x\n", COM_ReadReg32(0xF5004344));
            M4UINFO("0xF5004348 = 0x%x\n", COM_ReadReg32(0xF5004348));
            M4UINFO("0xF500434C = 0x%x\n", COM_ReadReg32(0xF500434C));
            M4UINFO("0xF5004350 = 0x%x\n", COM_ReadReg32(0xF5004350));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        case M4U_PORT_ESFKO:
            M4UINFO("0xF5004364 = 0x%x\n", COM_ReadReg32(0xF5004364));
            M4UINFO("0xF500435C = 0x%x\n", COM_ReadReg32(0xF500435C));
            M4UINFO("0xF500436C = 0x%x\n", COM_ReadReg32(0xF500436C));
            M4UINFO("0xF5004360 = 0x%x\n", COM_ReadReg32(0xF5004360));
            M4UINFO("0xF5004368 = 0x%x\n", COM_ReadReg32(0xF5004368));
            M4UINFO("0xF5004370 = 0x%x\n", COM_ReadReg32(0xF5004370));
            M4UINFO("0xF5004374 = 0x%x\n", COM_ReadReg32(0xF5004374));
            M4UINFO("0xF5004378 = 0x%x\n", COM_ReadReg32(0xF5004378));
            M4UINFO("0xF500437C = 0x%x\n", COM_ReadReg32(0xF500437C));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            COM_WriteReg32(0xF5004160,0x6000);
            M4UINFO("(CQ)0xF5004164 = 0x%x\n", COM_ReadReg32(0xF5004164));
            break;
        case M4U_PORT_AAO:
            M4UINFO("0xF5004388 = 0x%x\n", COM_ReadReg32(0xF5004388));
            M4UINFO("0xF500438C = 0x%x\n", COM_ReadReg32(0xF500438C));
            M4UINFO("0xF5004390 = 0x%x\n", COM_ReadReg32(0xF5004390));
            M4UINFO("0xF5004394 = 0x%x\n", COM_ReadReg32(0xF5004394));
            M4UINFO("0xF5004398 = 0x%x\n", COM_ReadReg32(0xF5004398));
            M4UINFO("0xF5004000 = 0x%x\n", COM_ReadReg32(0xF5004000));
            M4UINFO("0xF5004004 = 0x%x\n", COM_ReadReg32(0xF5004004));
            M4UINFO("0xF500400C = 0x%x\n", COM_ReadReg32(0xF500400C));
            break;
        default:  M4UINFO("port id is wrong %d\n", port);

    }

    
    return 0;
}

int m4u_debug_command(unsigned int command)
{

    M4UMSG("m4u_debug_command, command=0x%x \n", command);
    
    switch(command)
    {
    	  case 0:
            g_debug_make_translation_fault = 0;
    	  	break;
    	  	
    	  case 1:
            g_debug_make_translation_fault = 1;
    	  	break;
    	  
    	  case 2:
            g_debug_print_detail_in_isr = 0;
    	  	break;

    	  case 3:
            g_debug_print_detail_in_isr = 1;
    	  	break;

    	  case 4:
            m4u_enable_error_hang(0);
            g_debug_enable_error_hang= 1;
    	  	break;
        
    	  case 5:
            m4u_disable_error_hang(0);
            g_debug_enable_error_hang= 0;
    	  	break; 

    	  case 6:
            g_debug_recover_pagetable_TF = 0;
    	  	break; 

          case 7:   //start dynamic profile
            g_debug_recover_pagetable_TF = 1;
            break;

          case 8:  // get profile report
              {
                  //int i;
                  //for(i=0;i<TOTAL_M4U_NUM;i++)
                  //{
                  //    m4u_monitor_start(m4u_get_port_by_index(i));  // start to count performance for next 1 second  
                  //}
              }
            break;

          case 9: //stop profile and get report
              {
                  //int i;
                  //for(i=0;i<TOTAL_M4U_NUM;i++)
                  //{    
                  //    m4u_monitor_stop(m4u_get_port_by_index(i));   // print performance in last 1 second    
                  //}
              }
            break;

          case 10: 
            m4u_monitor_start(0);
            m4u_monitor_start(1);
            g_debug_dump_rs_in_isr=1;
            break;
          case 11: 
            m4u_monitor_stop(0);
            m4u_monitor_stop(1);
            g_debug_dump_rs_in_isr=0;
            break;

          case 12: 
            M4UMSG("debug 12: dump mva info\n");
            m4u_dump_mva_info();
            break;

          case 13: 
            M4UMSG("debug 13: L1 enable cache flush all\n");
            L1_CACHE_SYNC_BY_RANGE_ONLY = 0;
            break;
            
          case 14: 
            M4UMSG("debug 14: L1 cache flush by range only\n");
            L1_CACHE_SYNC_BY_RANGE_ONLY = 1;
            break;
         	  
         case 15:
         	  M4UMSG("debug 15: set level to user \n");
         	  gTestLevel = M4U_TEST_LEVEL_USER;
         	  break;  

         case 16:
         	  M4UMSG("debug 16: set level to eng \n");
         	  gTestLevel = M4U_TEST_LEVEL_ENG;
         	  break;  

         case 17:
         	  M4UMSG("debug 17: set level to stress \n");
         	  gTestLevel = M4U_TEST_LEVEL_STRESS;
         	  break;  

          case 18: 
            {
                unsigned int size = 0x1000*1024+0x128;
                unsigned int orignal_va, va, mva, map_va, map_size;
                unsigned int page_num, i;

                M4UMSG("debug 18: mva map kernel test\n");
                
                orignal_va = vmalloc(size+0x104);
                va = orignal_va + 0x104;
                m4u_alloc_mva(M4U_CLNTMOD_DBI, va, size, 0, 0, &mva);
                page_num = M4U_GET_PAGE_NUM(va, size);
                M4UMSG("orignal_va=0x%x, va=0x%x, mva=0x%x, size=0x%x, page_num=0x%x\n", 
                    orignal_va, va, mva, size, page_num);

                for(i=0; i<size; i+=4)
                    *(unsigned int *)(va+i) = i;
                
                m4u_dma_cache_flush_all();

                m4u_mva_map_kernel(mva, size, 0, &map_va, &map_size);
                M4UMSG("map_va=0x%x, map_size=0x%x\n", map_va, map_size);

                for(i=0; i<page_num; i++)
                {
                    struct page *p1, *p2;
                    p1 = vmalloc_to_page(va+i*0x1000);
                    p2 = vmalloc_to_page(map_va+i*0x1000);
                    if(p1 != p2)
                        M4UMSG("error: p1=0x%x, p2=0x%x\n", p1, p2);
                }
                {//check result 
                    unsigned int tmp = 0;

                    for(i=0; i<size; i+=4)
                    {
                        unsigned int x = *(unsigned int *)(map_va+i);
                        if(x != i)
                        {
                            tmp++;
                        }
                    }
                    if(tmp != 0)
                        M4UMSG("error!: %d data error!", tmp);
                }

                m4u_mva_unmap_kernel(mva, size, map_va);
                m4u_dealloc_mva(M4U_CLNTMOD_DBI, va, size, mva);

                vfree(orignal_va);
                M4UMSG("mva map kernel test done!!! \n");
            }
            
            
            break;
         
    	  default:
    	  	M4UMSG("undefined command: %d! \n", command);
    }
    
    return 0;	
}

void m4u_print_mva_list(struct file *filep, const char *pMsg)
{
    garbage_node_t *pNode = filep->private_data;
    mva_info_t *pList;
    struct list_head *pListHead;

    M4UMSG("print mva list [%s] ================================>\n", pMsg);
    mutex_lock(&(pNode->dataMutex));
    list_for_each(pListHead, &(pNode->mvaList))
    {
        pList = container_of(pListHead, mva_info_t, link);
        M4UMSG("module=%s, va=0x%x, size=0x%x, mva=0x%x, flags=%d\n", 
            m4u_get_module_name(pList->eModuleId), pList->bufAddr, pList->size, pList->mvaStart, pList->flags);
    }
    mutex_unlock(&(pNode->dataMutex));

    M4UMSG("print mva list done ==========================>\n");
}


static mva_info_t* m4u_alloc_garbage_list(   unsigned int mvaStart, 
                                          unsigned int bufSize,
                                          M4U_MODULE_ID_ENUM eModuleID,
                                          unsigned int va,
                                          unsigned int flags,
                                          int security,
                                          int cache_coherent)
{
    mva_info_t *pList = NULL;
    pList = (mva_info_t*)kmalloc(sizeof(mva_info_t), GFP_KERNEL);
    if(pList==NULL)
    {
        M4UERR("m4u_add_to_garbage_list(), pList=0x%x\n", (unsigned int)pList);
        return NULL;
    }

    pList->mvaStart = mvaStart;
    pList->size = bufSize;
    pList->eModuleId = eModuleID;
    pList->bufAddr = va;
    pList->flags = flags;
    pList->security = security;
    pList->cache_coherent = cache_coherent;

    return pList;
}

static int m4u_free_garbage_list(mva_info_t *pList)
{
    kfree(pList);
    return 0;
}


static int m4u_add_to_garbage_list(struct file * a_pstFile,mva_info_t *pList)
{
    garbage_node_t *pNode = (garbage_node_t*)(a_pstFile->private_data);
    mutex_lock(&(pNode->dataMutex));
    list_add(&(pList->link), &(pNode->mvaList));
    mutex_unlock(&(pNode->dataMutex));
    
    return 0;	
}


static mva_info_t* m4u_delete_from_garbage_list(M4U_MOUDLE_STRUCT* p_m4u_module, struct file * a_pstFile)
{
    struct list_head *pListHead;
    mva_info_t *pList = NULL;
    garbage_node_t *pNode = (garbage_node_t*)(a_pstFile->private_data);
    mva_info_t* ret=NULL;

    if(pNode==NULL)
    {
        M4UERR("m4u_delete_from_garbage_list(), pNode is NULL! \n");
        return NULL;
    }

    mutex_lock(&(pNode->dataMutex));
    list_for_each(pListHead, &(pNode->mvaList))
    {
        pList = container_of(pListHead, mva_info_t, link);
        if((pList->mvaStart== p_m4u_module->MVAStart))
        {
            if(    (pList->bufAddr== p_m4u_module->BufAddr)
                && (pList->size == p_m4u_module->BufSize)
                && (pList->eModuleId == p_m4u_module->eModuleID) )
            {                    
                list_del(pListHead);
                ret = pList;
                break;
            }
            else
            {
                ret=NULL;
            	M4UMSG("error: input argument isn't valid, can't find the node at garbage list\n");
            }
        }
    }
    if(pListHead == &(pNode->mvaList))
    {
        ret=NULL;
        M4UMSG("error: input argument isn't valid, can't find the node at garbage list\n");
    }
    mutex_unlock(&(pNode->dataMutex));
    
    return ret;	
}


static int m4u_suspend(struct platform_device *pdev, pm_message_t mesg)
{
    smi_reg_backup();
    M4ULOG("SMI backup in suspend \n");
    
    return 0;
}

static int m4u_resume(struct platform_device *pdev)
{
    smi_reg_restore();
    M4UMSG("SMI restore in resume \n");
    return 0;
}

/*---------------------------------------------------------------------------*/
#ifdef CONFIG_PM
/*---------------------------------------------------------------------------*/
int m4u_pm_suspend(struct device *device)
{
    M4ULOG("calling %s()\n", __func__);

	struct platform_device *pdev = to_platform_device(device);
    BUG_ON(pdev == NULL);

    return m4u_suspend(pdev, PMSG_SUSPEND);
}

int m4u_pm_resume(struct device *device)
{
    M4ULOG("calling %s()\n", __func__);

	struct platform_device *pdev = to_platform_device(device);
    BUG_ON(pdev == NULL);

    return m4u_resume(pdev);
}

extern void mt_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt_irq_set_polarity(unsigned int irq, unsigned int polarity);
int m4u_pm_restore_noirq(struct device *device)
{
    M4ULOG("calling %s()\n", __func__);

    // m4u related irqs
    mt_irq_set_sens(MT6589_MMU0_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
	mt_irq_set_polarity(MT6589_MMU0_IRQ_ID, MT65xx_POLARITY_LOW);

    mt_irq_set_sens(MT6589_MMU1_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
	mt_irq_set_polarity(MT6589_MMU1_IRQ_ID, MT65xx_POLARITY_LOW);

    mt_irq_set_sens(MT6589_MMU_L2_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
	mt_irq_set_polarity(MT6589_MMU_L2_IRQ_ID, MT65xx_POLARITY_LOW);

    mt_irq_set_sens(MT6589_MMU_L2_SEC_IRQ_ID, MT65xx_LEVEL_SENSITIVE);
	mt_irq_set_polarity(MT6589_MMU_L2_SEC_IRQ_ID, MT65xx_POLARITY_LOW);

    return 0;

}
/*---------------------------------------------------------------------------*/
#else /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
#define m4u_pm_suspend NULL
#define m4u_pm_resume  NULL
#define m4u_pm_restore_noirq NULL
/*---------------------------------------------------------------------------*/
#endif /*CONFIG_PM*/
/*---------------------------------------------------------------------------*/
struct dev_pm_ops m4u_pm_ops = {
    .suspend = m4u_pm_suspend,
    .resume = m4u_pm_resume,
    .freeze = m4u_pm_suspend,
    .thaw = m4u_pm_resume,
    .poweroff = m4u_pm_suspend,
    .restore = m4u_pm_resume,
    .restore_noirq = m4u_pm_restore_noirq,
};

static struct platform_driver m4uDrv = {
    .probe	= m4u_probe,
    .remove	= m4u_remove,
    .suspend= m4u_suspend,
    .resume	= m4u_resume,
    .driver	= {
    .name	= M4U_DEVNAME,
#ifdef CONFIG_PM
    .pm     = &m4u_pm_ops,
#endif
    .owner	= THIS_MODULE,
    }
};


static int __init MTK_M4U_Init(void)
{
    if(platform_driver_register(&m4uDrv)){
        M4UMSG("failed to register MAU driver");
        return -ENODEV;
    }

	return 0;
}

static void __exit MTK_M4U_Exit(void)
{
    platform_driver_unregister(&m4uDrv);
}


EXPORT_SYMBOL(m4u_dump_reg);
EXPORT_SYMBOL(m4u_dump_info);
EXPORT_SYMBOL(m4u_alloc_mva);
EXPORT_SYMBOL(m4u_dealloc_mva);
EXPORT_SYMBOL(m4u_insert_seq_range);
EXPORT_SYMBOL(m4u_invalid_seq_range);
EXPORT_SYMBOL(m4u_invalid_tlb_all);
EXPORT_SYMBOL(m4u_manual_insert_entry);
EXPORT_SYMBOL(m4u_config_port);
EXPORT_SYMBOL(m4u_monitor_start);
EXPORT_SYMBOL(m4u_monitor_stop);
EXPORT_SYMBOL(m4u_dma_cache_maint);
EXPORT_SYMBOL(m4u_log_on);
EXPORT_SYMBOL(m4u_log_off);
EXPORT_SYMBOL(m4u_debug_command);
EXPORT_SYMBOL(m4u_mau_check_pagetable);
EXPORT_SYMBOL(m4u_dump_pagetable_range);
EXPORT_SYMBOL(m4u_dump_main_tlb_des);
EXPORT_SYMBOL(m4u_dump_pfh_tlb_des);
EXPORT_SYMBOL(m4u_print_active_port);


module_init(MTK_M4U_Init);
module_exit(MTK_M4U_Exit);
                      

MODULE_DESCRIPTION("MTK M4U driver");
MODULE_AUTHOR("MTK80347 <Xiang.Xu@mediatek.com>");
MODULE_LICENSE("GPL");



#if 0
static int __dump_reg_range(unsigned int start, unsigned int end)
{
    int i;
    
    for(i=0; i<=end-start; i+=4)
    {
        printk("0x%x,", ioread32(start+i));
        if((i+4)%32==0)
            printk("\n");
    }
    if(i%32!=0)
        printk("\n");
    return 0;
}

int __dump_memory_range(unsigned int start, unsigned int end)
{
    int i;
    
    for(i=0; i<=end-start; i+=4)
    {
        printk("0x%x,", *(volatile unsigned int *)(start+i));
        if((i+4)%32==0)
            printk("\n");
    }
    if(i%32!=0)
        printk("\n");
    return 0;
}


#endif



