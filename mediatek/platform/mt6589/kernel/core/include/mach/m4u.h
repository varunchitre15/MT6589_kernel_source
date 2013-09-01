

#ifndef __M4U_H__
#define __M4U_H__
#include <linux/ioctl.h>
#include <linux/fs.h>


#define M4U_PAGE_SIZE    0x1000                                  //4KB

#define M4U_CLIENT_MODULE_NUM        M4U_CLNTMOD_MAX
#define TOTAL_MVA_RANGE      0x40000000                              //total virtual address range: 1GB


#define ACCESS_TYPE_TRANSLATION_FAULT  0
#define ACCESS_TYPE_64K_PAGE           1
#define ACCESS_TYPE_4K_PAGE            2
#define ACCESS_TYPE_4K_EXTEND_PAGE     3


#define PT_TOTAL_ENTRY_NUM    (1*1024*1024) //(TOTAL_MVA_RANGE/DEFAULT_PAGE_SIZE)              //total page table entries


#define M4U_GET_PTE_OFST_TO_PT_SA(MVA)    (((MVA) >> 12) << 2)
#define M4U_PTE_MAX (M4U_GET_PTE_OFST_TO_PT_SA(TOTAL_MVA_RANGE-1))


//==========================
//hw related
//=========================
//m4u global
#define NORMAL_M4U_NUMBER 2
#define TOTAL_M4U_NUM         2
#define M4U_REG_SIZE        0x5e0
#define M4U_PORT_NR         54
#define M4U_L2_CACHE_EN      0

//SMI related
#define SMI_LARB_NR         5
#define SMI_PORT_NR         57

//tlb related
#define M4U_MAIN_TLB_NR   48
#define M4U_PRE_TLB_NR    48

//wrap range related
#define M4U_WRAP_NR       4
#define TOTAL_WRAP_NUM    ((M4U_WRAP_NR)*(TOTAL_M4U_NUM))

//IRQ related
#define MT6589_MMU0_IRQ_ID          (105+32)
#define MT6589_MMU1_IRQ_ID          (106+32)
#define MT6589_MMU_L2_IRQ_ID        (107+32)
#define MT6589_MMU_L2_SEC_IRQ_ID    (108+32)

//seq range related
#define M4U_SEQ_NR          16
#define SEQ_RANGE_NUM       M4U_SEQ_NR
#define TOTAL_RANGE_NUM       (M4U_SEQ_NR)*TOTAL_M4U_NUM
#define M4U_SEQ_ALIGN_MSK   (0x40000-1)
#define M4U_SEQ_ALIGN_SIZE  0x40000


//====================================
// about portid
//====================================
#define M4U_LARB0_PORTn(n)      ((n)+0)
#define M4U_LARB1_PORTn(n)      ((n)+10)
#define M4U_LARB2_PORTn(n)      ((n)+17)
#define M4U_LARB3_PORTn(n)      ((n)+29)
#define M4U_LARB4_PORTn(n)      ((n)+43)
#define M4U_LARB5_PORTn(n)      ((n)+53)

typedef enum
{
    M4U_PORT_VENC_RCPU             =  M4U_LARB0_PORTn(0)   ,
    M4U_PORT_VENC_REF_LUMA         =  M4U_LARB0_PORTn(1)   ,
    M4U_PORT_VENC_REF_CHROMA       =  M4U_LARB0_PORTn(2)   ,
    M4U_PORT_VENC_DB_READ          =  M4U_LARB0_PORTn(3)   ,
    M4U_PORT_VENC_DB_WRITE         =  M4U_LARB0_PORTn(4)   ,
    M4U_PORT_VENC_CUR_LUMA         =  M4U_LARB0_PORTn(5)   ,
    M4U_PORT_VENC_CUR_CHROMA       =  M4U_LARB0_PORTn(6)   ,
    M4U_PORT_VENC_RD_COMV          =  M4U_LARB0_PORTn(7)   ,
    M4U_PORT_VENC_SV_COMV          =  M4U_LARB0_PORTn(8)   ,
    M4U_PORT_VENC_BSDMA            =  M4U_LARB0_PORTn(9)   ,
                                                           
    M4U_PORT_HW_VDEC_MC_EXT        =  M4U_LARB1_PORTn(0)   ,
    M4U_PORT_HW_VDEC_PP_EXT        =  M4U_LARB1_PORTn(1)   ,
    M4U_PORT_HW_VDEC_AVC_MV_EXT    =  M4U_LARB1_PORTn(2)   ,
    M4U_PORT_HW_VDEC_PRED_RD_EXT   =  M4U_LARB1_PORTn(3)   ,
    M4U_PORT_HW_VDEC_PRED_WR_EXT   =  M4U_LARB1_PORTn(4)   ,
    M4U_PORT_HW_VDEC_VLD_EXT       =  M4U_LARB1_PORTn(5)   ,
    M4U_PORT_HW_VDEC_VLD2_EXT      =  M4U_LARB1_PORTn(6)   ,
                                                           
    M4U_PORT_ROT_EXT               =  M4U_LARB2_PORTn(0)   ,
    M4U_PORT_OVL_CH0               =  M4U_LARB2_PORTn(1)   ,
    M4U_PORT_OVL_CH1               =  M4U_LARB2_PORTn(2)   ,
    M4U_PORT_OVL_CH2               =  M4U_LARB2_PORTn(3)   ,
    M4U_PORT_OVL_CH3               =  M4U_LARB2_PORTn(4)   ,
    M4U_PORT_WDMA0                 =  M4U_LARB2_PORTn(5)   ,
    M4U_PORT_WDMA1                 =  M4U_LARB2_PORTn(6)   ,
    M4U_PORT_RDMA0                 =  M4U_LARB2_PORTn(7)   ,
    M4U_PORT_RDMA1                 =  M4U_LARB2_PORTn(8)   ,
    M4U_PORT_CMDQ                  =  M4U_LARB2_PORTn(9)   ,
    M4U_PORT_DBI                   =  M4U_LARB2_PORTn(10)  ,
    M4U_PORT_G2D                   =  M4U_LARB2_PORTn(11)  ,
                                                           
    M4U_PORT_JPGDEC_WDMA           =  M4U_LARB3_PORTn(0)   ,
    M4U_PORT_JPGENC_RDMA           =  M4U_LARB3_PORTn(1)   ,
    M4U_PORT_VIPI                  =  M4U_LARB3_PORTn(2)   ,
    M4U_PORT_IMGI                  =  M4U_LARB3_PORTn(3)   ,
    M4U_PORT_DISPO                 =  M4U_LARB3_PORTn(4)   ,
    M4U_PORT_DISPCO                =  M4U_LARB3_PORTn(5)   ,
    M4U_PORT_DISPVO                =  M4U_LARB3_PORTn(6)   ,
    M4U_PORT_VIDO                  =  M4U_LARB3_PORTn(7)   ,
    M4U_PORT_VIDCO                 =  M4U_LARB3_PORTn(8)   ,
    M4U_PORT_VIDVO                 =  M4U_LARB3_PORTn(9)   ,
    M4U_PORT_VIP2I                 =  M4U_LARB3_PORTn(10)  ,
    M4U_PORT_GDMA_SMI_WR           =  M4U_LARB3_PORTn(11)  ,
    M4U_PORT_JPGDEC_BSDMA          =  M4U_LARB3_PORTn(12)  ,
    M4U_PORT_JPGENC_BSDMA          =  M4U_LARB3_PORTn(13)  ,
                                                           
    M4U_PORT_GDMA_SMI_RD           =  M4U_LARB4_PORTn(0)   ,
    M4U_PORT_IMGCI                 =  M4U_LARB4_PORTn(1)   ,
    M4U_PORT_IMGO                  =  M4U_LARB4_PORTn(2)   ,
    M4U_PORT_IMG2O                 =  M4U_LARB4_PORTn(3)   ,
    M4U_PORT_LSCI                  =  M4U_LARB4_PORTn(4)   ,
    M4U_PORT_FLKI                  =  M4U_LARB4_PORTn(5)   ,
    M4U_PORT_LCEI                  =  M4U_LARB4_PORTn(6)   ,
    M4U_PORT_LCSO                  =  M4U_LARB4_PORTn(7)   ,
    M4U_PORT_ESFKO                 =  M4U_LARB4_PORTn(8)   ,
    M4U_PORT_AAO                   =  M4U_LARB4_PORTn(9)   ,
                                                           
    M4U_PORT_AUDIO                 =  M4U_LARB5_PORTn(0)   ,

    M4U_PORT_NUM,

    M4U_PORT_UNKNOWN         = 1000

} M4U_PORT_ID_ENUM;

extern unsigned int m4u_index_of_larb[SMI_LARB_NR];
extern unsigned int smi_port0_in_larbx[SMI_LARB_NR+1];
extern unsigned int m4u_port0_in_larbx[SMI_LARB_NR+1];

static inline int m4u_port_2_larb_port(M4U_PORT_ID_ENUM port)
{
    int i;
    for(i=SMI_LARB_NR-1; i>=0; i--)
    {
        if(port >= m4u_port0_in_larbx[i])
            return (port-m4u_port0_in_larbx[i]);
    }
    return 0;
}


static inline int m4u_port_2_larb_id(M4U_PORT_ID_ENUM port)
{
    int i;
    for(i=SMI_LARB_NR-1; i>=0; i--)
    {
        if(port >= m4u_port0_in_larbx[i])
            return i;
    }
    return 0;
}

static inline int larb_2_m4u_id(int larb)
{
    return m4u_index_of_larb[larb];
}


static inline int m4u_port_2_m4u_id(M4U_PORT_ID_ENUM port)
{
    return larb_2_m4u_id(m4u_port_2_larb_id(port));
}

static inline int m4u_port_2_smi_port(M4U_PORT_ID_ENUM port)
{
    int larb = m4u_port_2_larb_id(port);
    int local_port = m4u_port_2_larb_port(port);
    return smi_port0_in_larbx[larb]+local_port;

}

static inline M4U_PORT_ID_ENUM larb_port_2_m4u_port(unsigned int larb, unsigned int local_port)
{
    return m4u_port0_in_larbx[larb]+local_port;
}


typedef enum
{
    M4U_CLNTMOD_VENC    = 0,	//0
                             
    M4U_CLNTMOD_VDEC       ,
                             
    M4U_CLNTMOD_ROT        ,
    M4U_CLNTMOD_OVL        ,
    M4U_CLNTMOD_WDMA       ,
    M4U_CLNTMOD_RDMA       ,
    M4U_CLNTMOD_CMDQ       ,
    M4U_CLNTMOD_DBI        ,
    M4U_CLNTMOD_G2D        ,
                                 
    M4U_CLNTMOD_JPGDEC     ,
    M4U_CLNTMOD_JPGENC     ,
    M4U_CLNTMOD_VIP        ,
    M4U_CLNTMOD_DISP       ,
    M4U_CLNTMOD_VID        ,
    M4U_CLNTMOD_GDMA       ,
                           
    M4U_CLNTMOD_IMG        ,
    M4U_CLNTMOD_LSCI       ,
    M4U_CLNTMOD_FLKI       ,
    M4U_CLNTMOD_LCEI       ,
    M4U_CLNTMOD_LCSO       ,
    M4U_CLNTMOD_ESFKO      ,
    M4U_CLNTMOD_AAO        ,
                           
    M4U_CLNTMOD_AUDIO      ,
    
    M4U_CLNTMOD_LCDC_UI,
    M4U_CLNTMOD_UNKNOWN,   

    M4U_CLNTMOD_MAX,



} M4U_MODULE_ID_ENUM;



// enlarge all MDP engine's limit to 200MB, because in some pattern, like jpeg decode
// the size maybe 9054*3072*RGB8888=117MB, maybe even bigger
#define    M4U_CLNTMOD_SZ_VENC        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_VDEC        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_ROT         (200*0x00100000)
#define    M4U_CLNTMOD_SZ_OVL         (200*0x00100000)
#define    M4U_CLNTMOD_SZ_WDMA        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_RDMA        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_CMDQ        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_DBI         (200*0x00100000)
#define    M4U_CLNTMOD_SZ_G2D         (200*0x00100000)
#define    M4U_CLNTMOD_SZ_JPGDEC      (200*0x00100000)
#define    M4U_CLNTMOD_SZ_JPGENC      (200*0x00100000)
#define    M4U_CLNTMOD_SZ_VIP         (200*0x00100000)
#define    M4U_CLNTMOD_SZ_DISP        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_VID         (200*0x00100000)
#define    M4U_CLNTMOD_SZ_GDMA        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_IMG         (350*0x00100000)
#define    M4U_CLNTMOD_SZ_LSCI        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_FLKI        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_LCEI        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_LCSO        (200*0x00100000)
#define    M4U_CLNTMOD_SZ_ESFKO       (200*0x00100000)
#define    M4U_CLNTMOD_SZ_AAO         (200*0x00100000)
#define    M4U_CLNTMOD_SZ_AUDIO       (200*0x00100000)
#define    M4U_CLNTMOD_SZ_LCDC_UI     (200*0x00100000)
#define    M4U_CLNTMOD_SZ_RESERVED    (200*0x00100000)

typedef enum
{
    M4U_ID_0 = 0,
    M4U_ID_1,
    M4U_ID_ALL
}M4U_ID_ENUM;

typedef struct _M4U_RANGE_DES  //sequential entry range
{
    unsigned int Enabled;
    M4U_MODULE_ID_ENUM eModuleID;
    unsigned int MVAStart;
    unsigned int MVAEnd;
    unsigned int entryCount;
} M4U_RANGE_DES_T;

typedef struct _M4U_WRAP_DES
{
    unsigned int Enabled;
    M4U_MODULE_ID_ENUM eModuleID;
    M4U_PORT_ID_ENUM ePortID;    
    unsigned int MVAStart;
    unsigned int MVAEnd;
} M4U_WRAP_DES_T;



typedef enum
{
	RT_RANGE_HIGH_PRIORITY=0,
	SEQ_RANGE_LOW_PRIORITY=1
} M4U_RANGE_PRIORITY_ENUM;

typedef enum
{
	M4U_DMA_READ_WRITE = 0,
	M4U_DMA_READ = 1,
	M4U_DMA_WRITE = 2,
	M4U_DMA_NONE_OP = 3,

} M4U_DMA_DIR_ENUM;

// port related: virtuality, security, distance
typedef struct _M4U_PORT
{  
	M4U_PORT_ID_ENUM ePortID;		   //hardware port ID, defined in M4U_PORT_ID_ENUM
	unsigned int Virtuality;						   
	unsigned int Security;
    unsigned int domain;            //domain : 0 1 2 3
	unsigned int Distance;
	unsigned int Direction;         //0:- 1:+
}M4U_PORT_STRUCT;

typedef enum
{
	ROTATE_0=0,
	ROTATE_90,
	ROTATE_180,
	ROTATE_270,
	ROTATE_HFLIP_0,
	ROTATE_HFLIP_90,
	ROTATE_HFLIP_180,
	ROTATE_HFLIP_270
} M4U_ROTATOR_ENUM;

typedef struct _M4U_PORT_ROTATOR
{  
	M4U_PORT_ID_ENUM ePortID;		   // hardware port ID, defined in M4U_PORT_ID_ENUM
	unsigned int Virtuality;						   
	unsigned int Security;
	// unsigned int Distance;      // will be caculated actomatically inside M4U driver
	// unsigned int Direction;
  unsigned int MVAStart; 
  unsigned int BufAddr;
  unsigned int BufSize;  
  M4U_ROTATOR_ENUM angle;	
}M4U_PORT_STRUCT_ROTATOR;

// module related:  alloc/dealloc MVA buffer
typedef struct _M4U_MOUDLE
{
	// MVA alloc / dealloc
	M4U_MODULE_ID_ENUM eModuleID;	// module ID used inside M4U driver, defined in M4U_PORT_MODULE_ID_ENUM
	unsigned int BufAddr;				// buffer virtual address
	unsigned int BufSize;				// buffer size in byte

	// TLB range invalidate or set uni-upadte range
	unsigned int MVAStart;						 // MVA start address
	unsigned int MVAEnd;							 // MVA end address
	M4U_RANGE_PRIORITY_ENUM ePriority;						 // range priority, 0:high, 1:normal
	unsigned int entryCount;

    // manually insert page entry
	unsigned int EntryMVA;						 // manual insert entry MVA
	unsigned int Lock;							 // manual insert lock or not
	int security;
    int cache_coherent;
}M4U_MOUDLE_STRUCT;

typedef enum
{
    M4U_CACHE_FLUSH_BEFORE_HW_READ_MEM = 0,  // optimized, recommand to use
    M4U_CACHE_FLUSH_BEFORE_HW_WRITE_MEM = 1, // optimized, recommand to use
    M4U_CACHE_CLEAN_BEFORE_HW_READ_MEM = 2,
    M4U_CACHE_INVALID_AFTER_HW_WRITE_MEM = 3,
    M4U_NONE_OP = 4,
} M4U_CACHE_SYNC_ENUM;

typedef struct _M4U_CACHE
{
    // MVA alloc / dealloc
    M4U_MODULE_ID_ENUM eModuleID;             // module ID used inside M4U driver, defined in M4U_MODULE_ID_ENUM
    M4U_CACHE_SYNC_ENUM eCacheSync;
    unsigned int BufAddr;                  // buffer virtual address
    unsigned int BufSize;                     // buffer size in byte
}M4U_CACHE_STRUCT;

typedef struct _M4U_PERF_COUNT
{
    unsigned int transaction_cnt;
    unsigned int main_tlb_miss_cnt;
    unsigned int pfh_tlb_miss_cnt;
    unsigned int pfh_cnt;
}M4U_PERF_COUNT;

//IOCTL commnad
#define MTK_M4U_MAGICNO 'g'
#define MTK_M4U_T_POWER_ON            _IOW(MTK_M4U_MAGICNO, 0, int)
#define MTK_M4U_T_POWER_OFF           _IOW(MTK_M4U_MAGICNO, 1, int)
#define MTK_M4U_T_DUMP_REG            _IOW(MTK_M4U_MAGICNO, 2, int)
#define MTK_M4U_T_DUMP_INFO           _IOW(MTK_M4U_MAGICNO, 3, int)
#define MTK_M4U_T_ALLOC_MVA           _IOWR(MTK_M4U_MAGICNO,4, int)
#define MTK_M4U_T_DEALLOC_MVA         _IOW(MTK_M4U_MAGICNO, 5, int)
#define MTK_M4U_T_INSERT_TLB_RANGE    _IOW(MTK_M4U_MAGICNO, 6, int)
#define MTK_M4U_T_INVALID_TLB_RANGE   _IOW(MTK_M4U_MAGICNO, 7, int)
#define MTK_M4U_T_INVALID_TLB_ALL     _IOW(MTK_M4U_MAGICNO, 8, int)
#define MTK_M4U_T_MANUAL_INSERT_ENTRY _IOW(MTK_M4U_MAGICNO, 9, int)
#define MTK_M4U_T_CACHE_SYNC          _IOW(MTK_M4U_MAGICNO, 10, int)
#define MTK_M4U_T_CONFIG_PORT         _IOW(MTK_M4U_MAGICNO, 11, int)
#define MTK_M4U_T_CONFIG_ASSERT       _IOW(MTK_M4U_MAGICNO, 12, int)
#define MTK_M4U_T_INSERT_WRAP_RANGE   _IOW(MTK_M4U_MAGICNO, 13, int)
#define MTK_M4U_T_MONITOR_START       _IOW(MTK_M4U_MAGICNO, 14, int)
#define MTK_M4U_T_MONITOR_STOP        _IOW(MTK_M4U_MAGICNO, 15, int)
#define MTK_M4U_T_RESET_MVA_RELEASE_TLB  _IOW(MTK_M4U_MAGICNO, 16, int)
#define MTK_M4U_T_CONFIG_PORT_ROTATOR _IOW(MTK_M4U_MAGICNO, 17, int)
#define MTK_M4U_T_QUERY_MVA           _IOW(MTK_M4U_MAGICNO, 18, int)
#define MTK_M4U_T_M4UDrv_CONSTRUCT    _IOW(MTK_M4U_MAGICNO, 19, int)
#define MTK_M4U_T_M4UDrv_DECONSTRUCT  _IOW(MTK_M4U_MAGICNO, 20, int)
#define MTK_M4U_T_DUMP_PAGETABLE      _IOW(MTK_M4U_MAGICNO, 21, int)
#define MTK_M4U_T_REGISTER_BUFFER     _IOW(MTK_M4U_MAGICNO, 22, int)
#define MTK_M4U_T_CACHE_FLUSH_ALL     _IOW(MTK_M4U_MAGICNO, 23, int)
#define MTK_M4U_T_REG_SET             _IOW(MTK_M4U_MAGICNO, 24, int)
#define MTK_M4U_T_REG_GET             _IOW(MTK_M4U_MAGICNO, 25, int)




// for kernel direct call --------------------------------------------
int m4u_dump_reg(int m4u_index);
int m4u_dump_info(int m4u_index);
int m4u_dump_pagetable(M4U_MODULE_ID_ENUM eModuleID);
int m4u_dump_pagetable_nearby(M4U_MODULE_ID_ENUM eModuleID, unsigned int mva_addr);
int m4u_power_on(int m4u_index);
int m4u_power_off(int m4u_index);

int m4u_alloc_mva(M4U_MODULE_ID_ENUM eModuleID, 
                    const unsigned int BufAddr, 
                    const unsigned int BufSize, 
                    int security,
                    int cache_coherent,
                    unsigned int *pRetMVABuf);

int m4u_dealloc_mva(M4U_MODULE_ID_ENUM eModuleID, 
                    const unsigned int BufAddr, 
                    const unsigned int BufSize,
                    const unsigned int MVA);	

int m4u_insert_wrapped_range(M4U_MODULE_ID_ENUM eModuleID, 
                             M4U_PORT_ID_ENUM portID, 
                             unsigned int MVAStart, 
                             unsigned int MVAEnd);

int m4u_invalid_wrapped_range(M4U_MODULE_ID_ENUM eModuleID, 
                              M4U_PORT_ID_ENUM portID,
                              unsigned int MVAStart, 
                              unsigned int MVAEnd);
                                                                                             
int m4u_insert_seq_range(M4U_MODULE_ID_ENUM eModuleID, 
                             unsigned int MVAStart, 
                             unsigned int MVAEnd, 
                             M4U_RANGE_PRIORITY_ENUM ePriority,
                             unsigned int entryCount); //0:disable multi-entry, 1,2,4,8,16: enable multi-entry
                      
int m4u_invalid_seq_range(M4U_MODULE_ID_ENUM eModuleID,
                    unsigned int MVAStart,
                    unsigned int MVAEnd);
                    
void m4u_invalid_tlb_all(M4U_ID_ENUM m4u_id, int L2_en);
int m4u_manual_insert_entry(M4U_PORT_ID_ENUM eModuleID,
									unsigned int EntryMVA, 
									int secure_pagetable,
									int Lock);
int m4u_config_port_rotator(M4U_PORT_STRUCT_ROTATOR *pM4uPort);

int m4u_config_port(M4U_PORT_STRUCT* pM4uPort); //native
//int m4u_config_port_rotator(M4U_PORT_STRUCT_ROTATOR *pM4uPort);
int m4u_monitor_start(int m4u_id);
int m4u_monitor_stop(int m4u_id);


int m4u_dma_cache_maint(M4U_MODULE_ID_ENUM eModuleID,
    const void *va,
    size_t size, 
    int direction);

int m4u_reg_backup(int m4u_index);
int m4u_reg_restore(int m4u_index);
int m4u_reset_mva_release_tlb(M4U_MODULE_ID_ENUM eModuleID); 

int m4u_mau_check_pagetable(unsigned int start_addr, unsigned int end_addr);
int m4u_mau_get_physical_port(unsigned int* engineMask);

// used for those looply used buffer
// will check link list for mva rather than re-build pagetable by get_user_pages()
// if can not find the VA in link list, will call m4u_alloc_mva() internally
int m4u_query_mva(M4U_MODULE_ID_ENUM eModuleID, 
								  const unsigned int BufAddr, 
								  const unsigned int BufSize, 
								  unsigned int *pRetMVABuf,
								  struct file * a_pstFile);
								  
int m4u_log_on(void);
int m4u_log_off(void);
int m4u_debug_command(unsigned int command);
int m4u_mva_map_kernel(unsigned int mva, unsigned int size, int sec,
                        unsigned int* map_va, unsigned int* map_size);
int m4u_mva_unmap_kernel(unsigned int mva, unsigned int size, unsigned int va);

// m4u driver internal use ---------------------------------------------------
//

#endif

