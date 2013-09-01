
#ifndef _VDEC_CONFIG_H_
#define  _VDEC_CONFIG_H_

#if 0
// Maximum PES Info number in ESM queue
#ifndef MPV_MAX_PES_NS_PER_QUEUE
#ifdef CC_MT5351_SLT
	#define MPV_MAX_PES_NS_PER_QUEUE    64
#else
	#define MPV_MAX_PES_NS_PER_QUEUE    112
#endif
#endif

// Message Queue Size
#define MPV_MSG_Q_SIZE				(MPV_MAX_PES_NS_PER_QUEUE + 16)

// Command Queue Size
#define MPV_CMD_Q_SIZE				5

// ESM Q Underflow Number
#define MPV_UDF_PES_NS				8

//Error Count Threshold
#define MPV_ERR_THRSD		   0

#define MPV_DETECT_REF_MISSING 0

//Error Concealment method
#define MPV_EC_METHOD     0   //0: 1389, 1: 5351, 2: 5351 no Intra
	#define MPV_EC_1389			0
	#define MPV_EC_5351_IPB	1
	#define MPV_EC_5351_PB		2


//#define MPV_LOG_NS     1800 //60*30
#define MPV_LOG_NS     900 //30*30

#define MPV_DTS_INVALID_DRIFT	45000 //0.5sec
#define MPV_DTS_DRIFT	9000 //0.1sec

#define MPV_PTS_INVALID_DRIFT	900000 ///10sec
#define MPV_PTS_DRIFT	18000


//#define MPV_PTS_INVALID_DRIFT	90000 ///check for wrap, 1sec

#define MPV_WAIT_DEC_TIME  100

#define MPV_WAIT_DISP_NO_AVSYNC
#if defined(MPV_WAIT_DISP)
#define MPV_WAIT_DISP_TIME	 			100				// 100 ms
#define MPV_AVSYNC_WAIT_DISP_TIME	 	500				// 500 ms
#elif defined(MPV_WAIT_DISP_NO_AVSYNC)
#define MPV_WAIT_DISP_TIME	 			200				// 200 ms
#define MPV_AVSYNC_WAIT_DISP_TIME	 	0				// 0 ms
#else
#define MPV_WAIT_DISP_TIME				0				// Don't wait display, push mode
#define MPV_AVSYNC_WAIT_DISP_TIME	 	0
#endif

#ifdef CC_MT5351_SLT
#undef MPV_AVSYNC_WAIT_DISP_TIME
#define MPV_AVSYNC_WAIT_DISP_TIME		0xFFFFFFFF
#endif

#define MPV_WAIT_DISP_UNLOCK_TIME		20				// 40 ms, wait display unlock when change fbg

#ifdef MPV_VIRTUAL_PRS
#undef MPV_WAIT_DISP_TIME
#undef MPV_AVSYNC_WAIT_DISP_TIME
#define MPV_WAIT_DISP_TIME				0xFFFFFFFF
#define MPV_AVSYNC_WAIT_DISP_TIME	 	0xFFFFFFFF
#endif

#define MPV_FIRST_MODIFY
//#define MPV_NO_PARSER
//#define MPV_DUMP_FBUF
#define NEW_FBM_SUPPORT
//#define NEW_FBM_DEBUG
//#define VDEC_MEM_ALLOC_SUPPORT
//#define MPV_DUMP_H264_DEC_REG
//#define MPV_CALLBACK_BY_VDEC
#define FBM_ALLOC_SUPPORT
//#define DRV_VDEC_TEMP_DEF
//#define WMV_SW_DEC_BP
#define FW_WRITE_QUANTIZATION_MATRIX
#define VDEC_EVENT_TRIGGER
#define VDEC_NO_FAC
#define VDEC_IBC_ASP_SUPPORT
//#define VDEC_LINE23_SUPPORT
#define VDEC_FORCE_DISP_LAST_PIC_SUPPORT
//#define MPV_DUMP_H264_CHKSUM
#define VDSCL_SIZE_LIMIT_SUPPORT

#define MPV_FIFO_CTRL_UDF				2
#define MPV_FIFO_CTRL_OVF				(MPV_MAX_PES_NS_PER_QUEUE)
#define MPV_FIFO_CTRL_MIN				0			// 0 ms
#define MPV_FIFO_CTRL_MAX				12000		// 133 ms
#define MPV_FIFO_CTRL_STEP				3000		// 33 ms
#define MPV_FIFO_CTRL_INIT				3000		// 33 ms
#define MPV_FIFO_MONITOR_NS				15			// 15 packet per monitor

#define MPV_FIFO_FULL_CNT_DOWN			60			// maintain 60 pictures

//After dectecting all Intra slice in No I bitstream, wait 1 P to display.
#define MPV_WAIT_P	1

#define MPV_CC_QSIZE  600	//1800/3

#define VDEC_8530_SUPPORT  1
#endif

#endif /* _MPV_CONFIG_H_ */


