#ifndef __MT6589_VCODEC_H__
#define __MT6589_VCODEC_H__

#define MFV_IOC_MAGIC    'M'

//below is control message
#define MFV_TEST_CMD                            _IO(MFV_IOC_MAGIC,  0x00)
#define MFV_INIT_CMD                            _IO(MFV_IOC_MAGIC, 0x01)
#define MFV_DEINIT_CMD                          _IO(MFV_IOC_MAGIC, 0x02)
#define MFV_SET_CMD_CMD                         _IOW(MFV_IOC_MAGIC, 0x03, P_MFV_DRV_CMD_QUEUE_T)
#define MFV_SET_PWR_CMD                         _IOW(MFV_IOC_MAGIC, 0x04, HAL_POWER_T *)
#define MFV_SET_ISR_CMD                         _IOW(MFV_IOC_MAGIC, 0x05, HAL_ISR_T *)
#define MFV_ALLOC_MEM_CMD                       _IOW(MFV_IOC_MAGIC, 0x06, unsigned int)
#define MFV_FREE_MEM_CMD                        _IOW(MFV_IOC_MAGIC, 0x07, unsigned int)
#define MFV_MAKE_PMEM_TO_NONCACHED              _IOW(MFV_IOC_MAGIC, 0x08, unsigned int*)
#define MFV_ALLOC_INT_MEM_CMD                   _IOW(MFV_IOC_MAGIC, 0x09, VAL_INTMEM_T*)
#define MFV_FREE_INT_MEM_CMD                    _IOW(MFV_IOC_MAGIC, 0x0a, VAL_INTMEM_T*)
#define VCODEC_WAITISR                          _IOW(MFV_IOC_MAGIC, 0x0b, HAL_POWER_T *)
#define VCODEC_LOCKHW                           _IOW(MFV_IOC_MAGIC, 0x0d, VAL_HW_LOCK_T *)
#define VCODEC_PMEM_FLUSH                       _IOW(MFV_IOC_MAGIC, 0x10, HAL_POWER_T *)
#define VCODEC_PMEM_CLEAN                       _IOW(MFV_IOC_MAGIC, 0x11, HAL_POWER_T *)
#define VCODEC_INC_SYSRAM_USER                  _IOW(MFV_IOC_MAGIC, 0x13, VAL_UINT32_T *)
#define VCODEC_DEC_SYSRAM_USER                  _IOW(MFV_IOC_MAGIC, 0x14, VAL_UINT32_T *)
#define VCODEC_INC_ENC_EMI_USER                 _IOW(MFV_IOC_MAGIC, 0x15, VAL_UINT32_T *)
#define VCODEC_DEC_ENC_EMI_USER                 _IOW(MFV_IOC_MAGIC, 0x16, VAL_UINT32_T *)
#define VCODEC_INC_DEC_EMI_USER                 _IOW(MFV_IOC_MAGIC, 0x17, VAL_UINT32_T *)
#define VCODEC_DEC_DEC_EMI_USER                 _IOW(MFV_IOC_MAGIC, 0x18, VAL_UINT32_T *)
#define VCODEC_INITHWLOCK                       _IOW(MFV_IOC_MAGIC, 0x20, VAL_VCODEC_OAL_HW_REGISTER_T *)
#define VCODEC_DEINITHWLOCK                     _IOW(MFV_IOC_MAGIC, 0x21, VAL_VCODEC_OAL_HW_REGISTER_T *)
#define VCODEC_ALLOC_NON_CACHE_BUFFER           _IOW(MFV_IOC_MAGIC, 0x22, VAL_MEMORY_T *)
#define VCODEC_FREE_NON_CACHE_BUFFER            _IOW(MFV_IOC_MAGIC, 0x23, VAL_MEMORY_T *)
#define VCODEC_SET_THREAD_ID                    _IOW(MFV_IOC_MAGIC, 0x24, VAL_VCODEC_THREAD_ID_T *)
#define VCODEC_SET_SYSRAM_INFO                  _IOW(MFV_IOC_MAGIC, 0x25, VAL_INTMEM_T *)
#define VCODEC_GET_SYSRAM_INFO                  _IOW(MFV_IOC_MAGIC, 0x26, VAL_INTMEM_T *)
#define VCODEC_INC_PWR_USER                     _IOW(MFV_IOC_MAGIC, 0x27, HAL_POWER_T *)
#define VCODEC_DEC_PWR_USER                     _IOW(MFV_IOC_MAGIC, 0x28, HAL_POWER_T *)
#define VCODEC_GET_CPU_LOADING_INFO             _IOW(MFV_IOC_MAGIC, 0x29, VAL_VCODEC_CPU_LOADING_INFO_T *)
#define VCODEC_GET_CORE_LOADING                 _IOW(MFV_IOC_MAGIC, 0x30, VAL_VCODEC_CORE_LOADING_T *)
#define VCODEC_GET_CORE_NUMBER                  _IOW(MFV_IOC_MAGIC, 0x31, int *)
#define VCODEC_SET_CPU_OPP_LIMIT                _IOW(MFV_IOC_MAGIC, 0x32, VAL_VCODEC_CPU_OPP_LIMIT_T *)
#define VCODEC_UNLOCKHW                         _IOW(MFV_IOC_MAGIC, 0x33, VAL_HW_LOCK_T *)
#define VCODEC_MB                               _IOW(MFV_IOC_MAGIC, 0x34, VAL_UINT32_T *)


//#define MFV_GET_CACHECTRLADDR_CMD  _IOR(MFV_IOC_MAGIC, 0x06, int)

#endif //__MT6589_MFLEXVIDEO_H__
