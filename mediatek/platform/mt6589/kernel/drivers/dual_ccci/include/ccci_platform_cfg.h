#ifndef __CCCI_PLATFORM_CFG_H__
#define __CCCI_PLATFORM_CFG_H__

//-------------md share memory configure----------------//
//Common configuration
#define MD_EX_LOG_SIZE					(2*1024)
#define CCCI_MISC_INFO_SMEM_SIZE		(1*1024)
#define CCCI_SHARED_MEM_SIZE 			UL(0x200000) // 2M align for share memory

#define MD_IMG_DUMP_SIZE				(1<<8)
#define DSP_IMG_DUMP_SIZE				(1<<9)

#define CCMNI_V2_PORT_NUM               (3) 		 // For V2 CCMNI


// MD SYS1 configuration
#define CCCI1_PCM_SMEM_SIZE				(16 * 2 * 1024)				// PCM
#define CCCI1_MD_LOG_SIZE				(137*1024*4+64*1024+112*1024)	// MD Log

#define RPC1_MAX_BUF_SIZE				2048 //(6*1024)
#define RPC1_REQ_BUF_NUM				2 			 //support 2 concurrently request	

#define CCCI1_TTY_BUF_SIZE			    (16 * 1024)
#define CCCI1_CCMNI_BUF_SIZE			(16*1024)
#define CCCI1_TTY_PORT_NUM    			(3)
#define CCCI1_CCMNI_V1_PORT_NUM			(3) 		 // For V1 CCMNI


// MD SYS2 configuration
#define CCCI2_PCM_SMEM_SIZE				(16 * 2 * 1024)					// PCM 
#define CCCI2_MD_LOG_SIZE				(137*1024*4+64*1024+112*1024)	// MD Log

#define RPC2_MAX_BUF_SIZE				2048 //(6*1024)
#define RPC2_REQ_BUF_NUM				2 			 //support 2 concurrently request	

#define CCCI2_TTY_BUF_SIZE			    (16 * 1024)
#define CCCI2_CCMNI_BUF_SIZE			(16*1024)
#define CCCI2_TTY_PORT_NUM  			(3)
#define CCCI2_CCMNI_V1_PORT_NUM			(3) 		 // For V1 CCMNI


//-------------feature enable/disable configure----------------//
//security feature configure
#define CURR_SEC_CCCI_SYNC_VER			(1)	// Note: must sync with sec lib, if ccci and sec has dependency change


#define CCCI_STATIC_SHARED_MEM           //using ioremap to allocate share memory, not dma_alloc_coherent
//#define  MD_IMG_SIZE_ADJUST_BY_VER        //md region can be adjusted by 2G/3G, ex, 2G: 10MB for md+dsp, 3G: 22MB for md+dsp

//#define  ENCRYPT_DEBUG                            //enable debug log for SECURE_ALGO_OP
#define  ENABLE_MD_IMG_SECURITY_FEATURE 
//#define  ENABLE_CHIP_VER_CHECK
#define  ENABLE_EMI_PROTECTION
//#define  ENABLE_MAU_PROTECTION
#define  ENABLE_MD_WDT_PROCESS
#define  ENABLE_MEM_REMAP_HW
//#define  ENABLE_2G_3G_CHECK
//#define  ENABLE_LOCK_MD_SLP_FEATURE


#endif

