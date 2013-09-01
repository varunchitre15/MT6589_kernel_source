#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/sched.h>
#include <ccci.h>
#include <linux/delay.h>

static int ccif_ch_init(struct physical_channel *pc_ch);
static int ccif_ch_destroy(struct physical_channel *pc_ch);
static int ccif_ch_xmit(struct physical_channel *pc_ch,CCCI_BUFF_T *,int force);
static void ccif_irq_mask(struct physical_channel *pc_ch);
static void ccif_irq_unmask(struct physical_channel *pc_ch);
static void ccif_disable(struct physical_channel *pc_ch,int);
static void ccif_enable(struct physical_channel *pc_ch);
static void ccif_reset(struct physical_channel *pc_ch);
static int ccif_manual_triger(struct physical_channel *pc_ch,CCCI_BUFF_T *);
static void ccif_debug_dump(struct physical_channel *pc_ch,DEBUG_INFO_T *);

extern volatile int md_boot_stage;
extern atomic_t md_reset_on_going;
extern volatile unsigned int md_slp_lock_ack;
extern volatile unsigned int md_slp_unlock_ack;

static int irq_cnt = 0;

#define ENABLE_ALL_RX_LOG (1ULL<<63)
unsigned long long lg_ch_tx_debug_enable = 0ULL;
//every bit indicates one logical channel, ex: bit43 -->CCCI_MD_LOG_TX
//bit43=1, enable TX debug log for CCCI_MD_LOG_TX; or else, disable debug log
unsigned long long lg_ch_rx_debug_enable = 0ULL;
//every bit indicates one logical channel, ex: bit42 -->CCCI_MD_LOG_RX
//bit42=1, enable RX debug log for CCCI_MD_LOG_RX; or else, disable debug log

typedef struct _xmit_log_unit{
	unsigned int data[4];
	struct timeval tv;
	unsigned int ref_jiffs;
	unsigned int retry_count;
}xmit_log_unit_t;
static xmit_log_unit_t *xmit_log;
static int curr_xmit_log_index= -1;
static int curr_xmit_log_num = -1;
static void xmit_history_dump(void);

char *lg_ch_str[CCCI_MAX_CHANNEL]={
	"Control_RX", "Control_TX",
	"System_RX", "System_TX",
	"Audio_RX", "Audio_TX",
	"Meta_RX", "Meta_RX_ACK", "Meta_TX", "Meta_TX_ACK",
	"Muxd_RX", "Muxd_RX_ACK", "Muxd_TX", "Muxd_TX_ACK",
	"MD_NVRAM_RX", "MD_NVRAM_TX",
	"PMIC_RX", "PMIC_TX",
	"UEM_RX", "UEM_TX",
	"Net_CCMNI1_RX", "Net_CCMNI1_RX_ACK","Net_CCMNI1_TX", "Net_CCMNI1_TX_ACK",
	"Net_CCMNI2_RX", "Net_CCMNI2_RX_ACK","Net_CCMNI2_TX", "Net_CCMNI2_TX_ACK",
	"Net_CCMNI3_RX", "Net_CCMNI3_RX_ACK","Net_CCMNI3_TX", "Net_CCMNI3_TX_ACK",
	"RPC_RX", "RPC_TX", 
	"AGPS_IPC_RX", "AGPS_IPC_RX_ACK", "AGPS_IPC_TX", "AGPS_IPC_TX_ACK",
	"AGPS_UART_RX", "AGPS_UART_RX_ACK", "AGPS_UART_TX", "AGPS_UART_TX_ACK",
	"Mdlogger_RX", "Mdlogger_TX", 
};


struct physical_channel  ccif_ch =
{
	.arb=1,
	.max_pc_num=CCIF_MAX_PHY,
	.pc_base_addr=CCIF_BASE,
	.irq_line=CCIF_IRQ_CODE,
	.pc_init=ccif_ch_init,
	.pc_xmit=ccif_ch_xmit,
	.manual_triger=ccif_manual_triger,
	.pc_destroy=ccif_ch_destroy,
	.mask=ccif_irq_mask,
	.unmask=ccif_irq_unmask,
	.disable=ccif_disable,
	.enable=ccif_enable,
	.reset=ccif_reset,
	.dump=ccif_debug_dump,
} ;


static irqreturn_t ccif_irq_handler(int irq,void *data)
{	
	struct physical_channel *pc_ch=(struct physical_channel *)data;
	int arb ;
	unsigned long flags;
	int i=0;
	int j=0;
	int max_ch;
	int rx_idx;
	int r_chnum;
	CCCI_BUFF_T buf;
	
	set_bit(CCCI_RUNNING,&pc_ch->flags);
	if (!test_bit(CCCI_ENABLED,&pc_ch->flags))
	{
		clear_bit(CCCI_RUNNING,&pc_ch->flags);
		CCCI_MSG_INF("cci", "ccci disabled!\n");
		return IRQ_NONE;
	}
	
	spin_lock_irqsave(&pc_ch->r_lock,flags);
	max_ch=pc_ch->max_pc_num;
	rx_idx=pc_ch->rx_idx;
	arb=pc_ch->arb;

	while( (r_chnum=*CCIF_RCHNUM(pc_ch->pc_base_addr))||arb==0)
	{
		if ((r_chnum&(r_chnum-1))!=0 && j++)
		{
			//CCCI_DEBUG("r_chnum=0x%x arb=%d j=%d\n",r_chnum,arb,j);
		}
		
		for(i=0;i<max_ch;i++)
		{
			int num=rx_idx%max_ch;
			
			if (r_chnum&(1<<num)||arb==0)
			{
				rx_idx++;
				if (unlikely(arb==0))
					ccci_buff_t_assign(&buf,(CCCI_BUFF_T *)CCIF_RXCHDATA(pc_ch->pc_base_addr)+r_chnum);	
				else
					ccci_buff_t_assign(&buf,(CCCI_BUFF_T *)CCIF_RXCHDATA(pc_ch->pc_base_addr)+num);
				
				if((lg_ch_rx_debug_enable & ENABLE_ALL_RX_LOG) || 
					(lg_ch_rx_debug_enable & (1<< buf.channel))) {
						CCCI_MSG_INF("cci", "[RX](%d): %08X, %08X, %02d, %08X (%02d)\n",
							irq_cnt, buf.data[0], buf.data[1], buf.channel, buf.reserved, num);
				}
				
				//check_data_connected(buf.channel);

				if(buf.channel == CCCI_SYSTEM_RX){
					if(buf.data[1] == MD_SLP_REQUEST) {
						if (buf.reserved == LOCK_MD_SLP) {
							md_slp_lock_ack = 1;
							//CCCI_MSG_INF("cci", "ack md lock sleep OK! \n");
						}
						else if (buf.reserved == UNLOCK_MD_SLP) {
							md_slp_unlock_ack = 1;
							//CCCI_MSG_INF("cci", "ack md unlock sleep OK! \n");
						}
						else if (buf.reserved == 0xFFFF) {
							CCCI_MSG_INF("cci", "request md sleep fail! \n");
						}
					}
					else if(buf.data[1] == MD_WDT_MONITOR)
						CCCI_MSG_INF("cci", "MD WDT monitor message\n");
				} 
				else if (pc_ch->lg_layer.process_data(&pc_ch->lg_layer,&buf,1,0)
					!=sizeof(CCCI_BUFF_T)) {
					//CCCI_MSG_INF("cci", "push data fail!\n");
					pc_ch->r_errors++;
				}
				*CCIF_ACK(pc_ch->pc_base_addr) = arb?(1 << num):(1<<r_chnum);
				if (arb==0) break;
				r_chnum &=~(1<<num);
			}	
			else 
			{
				if (r_chnum!=0)
				{
					CCCI_MSG_INF("cci", "rx channel data abnormal\n");
					rx_idx++;
				}
				break;
			}
			
		}
		if (arb==0)  break;
	}
	
	pc_ch->rx_idx=rx_idx;
	spin_unlock_irqrestore(&pc_ch->r_lock,flags);
	clear_bit(CCCI_RUNNING,&pc_ch->flags);

	if(irq_cnt)
		irq_cnt = 0;
	else
		irq_cnt = 1;
	
	return IRQ_HANDLED;
	
}

static void memory_dump(void *start_addr, int length)
{
	
	unsigned int *curr_p = (unsigned int *)start_addr;
	unsigned char *curr_ch_p;
	int _16_fix_num = length/16;
	int tail_num = length%16;
	char buf[16];
	int i,j;

	if(NULL == curr_p){
		CCCI_MSG_INF("cci", "NULL point!\n");
		return;
	}
	if(0 == length){
		CCCI_MSG_INF("cci", "Not need to dump\n");
		return;
	}

	CCCI_MSG_INF("cci", "Base: %08x\n", (unsigned int)start_addr);
	// Fix section
	for(i=0; i<_16_fix_num; i++){
		CCCI_MSG_INF("cci", "%03x: %08x %08x %08x %08x\n", 
				i*16, *curr_p, *(curr_p+1), *(curr_p+2), *(curr_p+3) );
		curr_p+=4;
	}

	// Tail section
	if(tail_num > 0){
		curr_ch_p = (unsigned char*)curr_p;
		for(j=0; j<tail_num; j++){
			buf[j] = *curr_ch_p;
			curr_ch_p++;
		}
		for(; j<16; j++)
			buf[j] = 0;
		curr_p = (unsigned int*)buf;
		CCCI_MSG_INF("cci", "%03x: %08x %08x %08x %08x\n", 
				i*16, *curr_p, *(curr_p+1), *(curr_p+2), *(curr_p+3) );

	}
}

void my_default_end(void)
{
	CCCI_MSG_INF("cci", "*********************END*******************\n");
}
void my_mem_dump(int *mem, int size,void (*end)(void),char *fmt,...)
{
	va_list v_arg;
	//int i=0;
	//int left=size%sizeof(int);
	va_start(v_arg,fmt);
	vprintk(fmt,v_arg);
	va_end(v_arg);

	CCCI_MSG_INF("cci", "\n");

	#if 0
	if ( mem && size>0)
	{
		for ( i=0;i<size/sizeof(int);i++ )
		{
			if (i%4==0)
			{
				CCCI_RAW("\n%3d-->%3d:",i,i+3);
			}
			CCCI_RAW(" %08x",*(mem+i));
		}
		if (left)
		{
			unsigned int val=*(mem+i)&((1<<(left*8))-1);
			if (i%4==0)
			{
				CCCI_RAW("\ntail:%08x\n",val);
			}
			else 
			{
				CCCI_RAW(" %08x\n",val);
			}
		}
		else
			CCCI_RAW("\n");
	}
	else 
		CCCI_RAW("No data to Dump(src:%p size:%d)\n",mem,size);
	#endif
	memory_dump(mem, size);
	if (end) end();
	mdelay(5);
}

#define EE_BUF_LEN      (256)
static void ccif_debug_dump(struct physical_channel *pc_ch,DEBUG_INFO_T *debug_info)	
{
	char ex_info[EE_BUF_LEN]="";
	int *tx_data=(int*)CCIF_TXCHDATA(pc_ch->pc_base_addr);
	set_bit(CCCI_RUNNING,&pc_ch->flags);

	CCCI_MSG_INF("cci", "exception type(%d):%s\n",debug_info->type,debug_info->name?:"Unknown");
	switch(debug_info->type)
	{
	case MD_EX_TYPE_ASSERT_DUMP:
	case MD_EX_TYPE_ASSERT:
		CCCI_MSG_INF("cci", "filename = %s\n", debug_info->assert.file_name);
		CCCI_MSG_INF("cci", "line = %d\n", debug_info->assert.line_num);
		CCCI_MSG_INF("cci", "para0 = %d, para1 = %d, para2 = %d\n", 
				debug_info->assert.parameters[0],
				debug_info->assert.parameters[1],
				debug_info->assert.parameters[2]);
		//snprintf(ex_info,128,"[%s] file:%s line:%d\n", debug_info->name, debug_info->assert.file_name,
		//		debug_info->assert.line_num);
		snprintf(ex_info,EE_BUF_LEN,"\n[%s] file:%s line:%d\np1:0x%08x\np2:0x%08x\np3:0x%08x\n",
				debug_info->name, 
				debug_info->assert.file_name,
				debug_info->assert.line_num, 
				debug_info->assert.parameters[0],
				debug_info->assert.parameters[1],
				debug_info->assert.parameters[2]);
		break;
	case MD_EX_TYPE_FATALERR_BUF:
	case MD_EX_TYPE_FATALERR_TASK:
		CCCI_MSG_INF("cci", "fatal error code 1 = %d\n", debug_info->fatal_error.err_code1);
		CCCI_MSG_INF("cci", "fatal error code 2 = %d\n", debug_info->fatal_error.err_code2);
		snprintf(ex_info,EE_BUF_LEN,"\n[%s] err_code1:%d err_code2:%d\n", debug_info->name, 
				debug_info->fatal_error.err_code1, debug_info->fatal_error.err_code2);
		break;
	case MD_EX_TYPE_EMI_CHECK:
		CCCI_MSG_INF("cci", "md_emi_check: %08X, %08X, %02d, %08X\n", 
				debug_info->data.data[0], debug_info->data.data[0],
				debug_info->data.channel, debug_info->data.reserved);
		snprintf(ex_info,EE_BUF_LEN,"\n[emi_chk] %08X, %08X, %02d, %08X\n", 
				debug_info->data.data[0], debug_info->data.data[0],
				debug_info->data.channel, debug_info->data.reserved);
		break;
	case DSP_EX_TYPE_ASSERT:
		CCCI_MSG_INF("cci", "filename = %s\n", debug_info->dsp_assert.file_name);
		CCCI_MSG_INF("cci", "line = %d\n", debug_info->dsp_assert.line_num);
		CCCI_MSG_INF("cci", "exec unit = %s\n", debug_info->dsp_assert.execution_unit);
		CCCI_MSG_INF("cci", "para0 = %d, para1 = %d, para2 = %d\n", 
				debug_info->dsp_assert.parameters[0],
				debug_info->dsp_assert.parameters[1],
				debug_info->dsp_assert.parameters[2]);
		snprintf(ex_info,EE_BUF_LEN,"\n[%s] file:%s line:%d\nexec:%s\np1:%d\np2:%d\np3:%d\n",
				debug_info->name, debug_info->assert.file_name, debug_info->assert.line_num,
				debug_info->dsp_assert.execution_unit, 
				debug_info->dsp_assert.parameters[0],
				debug_info->dsp_assert.parameters[1],
				debug_info->dsp_assert.parameters[2]);
		break;
	case DSP_EX_TYPE_EXCEPTION:
		CCCI_MSG_INF("cci", "exec unit = %s, code1:0x%08x\n", debug_info->dsp_exception.execution_unit,
				debug_info->dsp_exception.code1);
		snprintf(ex_info,EE_BUF_LEN,"\n[%s] exec:%s code1:0x%08x\n",
				debug_info->name, debug_info->dsp_exception.execution_unit,
				debug_info->dsp_exception.code1);
		break;
	case DSP_EX_FATAL_ERROR:
		CCCI_MSG_INF("cci", "exec unit = %s\n", debug_info->dsp_fatal_err.execution_unit);
		CCCI_MSG_INF("cci", "err_code0 = 0x%08x, err_code1 = 0x%08x\n", 
				debug_info->dsp_fatal_err.err_code[0],
				debug_info->dsp_fatal_err.err_code[1]);

		snprintf(ex_info,EE_BUF_LEN,"\n[%s] exec:%s err_code1:0x%08x err_code2:0x%08x\n",
				debug_info->name, debug_info->dsp_fatal_err.execution_unit, 
				debug_info->dsp_fatal_err.err_code[0],
				debug_info->dsp_fatal_err.err_code[1]);
		break;
	default: // Only display exception name
	        snprintf(ex_info,EE_BUF_LEN,"\n[%s]\n", debug_info->name);
	        break;
	}

	CCCI_MSG_INF("cci", "[CCCI REG_INFO] (arb=%d):\n",pc_ch->arb);
	CCCI_MSG_INF("cci", "CON(%p)=%08x, BUSY(%p)=%08x,START(%p)=%08x\n", 
		CCIF_CON(pc_ch->pc_base_addr), *CCIF_CON(pc_ch->pc_base_addr),
		 CCIF_BUSY(pc_ch->pc_base_addr), *CCIF_BUSY(pc_ch->pc_base_addr),
		 CCIF_START(pc_ch->pc_base_addr), *CCIF_START(pc_ch->pc_base_addr));
	CCCI_MSG_INF("cci", "[CCCI RX_errors:%d TX_errors:%d]\n",pc_ch->r_errors,pc_ch->s_errors);
	
	my_mem_dump(tx_data,0x100,my_default_end,"\n\n[CCCI_SHARE][%p][%dBytes]:",tx_data,0x100);
	
	my_mem_dump(debug_info->ext_mem, debug_info->ext_size,my_default_end,
		"\n\n[MODEM_EXP_LOG][%p][%dBytes]:",debug_info->ext_mem, debug_info->ext_size);

	my_mem_dump(debug_info->md_image, debug_info->md_size,my_default_end,
		"\n\n[CCCI_MD_IMG][%p][%dBytes]:",debug_info->md_image, debug_info->md_size);
	if (debug_info->platform_call)  debug_info->platform_call(debug_info->platform_data);
	if (pc_ch->lg_layer.dump)
		pc_ch->lg_layer.dump(&pc_ch->lg_layer,-1U);

	xmit_history_dump();
	clear_bit(CCCI_RUNNING,&pc_ch->flags);

	ccci_aed(CCCI_AED_DUMP_EX_MEM|CCCI_AED_DUMP_MD_IMG_MEM, ex_info);
#if 0
//#ifdef CONFIG_MTK_AEE_FEATURE 
	aed_md_exception1(debug_info->ext_mem, debug_info->ext_size, 
		debug_info->md_image, debug_info->md_size, debug_info->name,
		(debug_info->type==MD_EX_TYPE_ASSERT_DUMP||debug_info->type==MD_EX_TYPE_ASSERT)?debug_info->assert.file_name:NULL,
		(debug_info->type==MD_EX_TYPE_ASSERT_DUMP||debug_info->type==MD_EX_TYPE_ASSERT)?debug_info->assert.line_num:0,
		(debug_info->type==MD_EX_TYPE_FATALERR_BUF||debug_info->type==MD_EX_TYPE_FATALERR_TASK)?debug_info->fatal_error.err_code1:0,
		(debug_info->type==MD_EX_TYPE_FATALERR_BUF||debug_info->type==MD_EX_TYPE_FATALERR_TASK)?debug_info->fatal_error.err_code2:
		(debug_info->type==MD_EX_TYPE_EMI_CHECK?debug_info->data.reserved:0));

#endif

}


#if 0
static void ccif_tx_fifo_dump(void)
{
	int i;
	unsigned long addr = ccif_ch.pc_base_addr;
	
	CCCI_MSG_INF("cci", "[CCCI TX_REG_INFO]:\n");

	for(i = 0; i < ccif_ch.max_pc_num; i++) {
		CCCI_MSG_INF("cci", "[phy_ch:%02d] %08X, %08X, %02d, %08X\n", i,
			*CCIF_TXCHDATA(addr+4*4*i), *CCIF_TXCHDATA(addr+4*(4*i+1)), 
			*CCIF_TXCHDATA(addr+4*(4*i+2)), *CCIF_TXCHDATA(addr+4*(4*i+3)));
	}

	CCCI_MSG_INF("cci", "*********************END*******************\n");
}
#endif

extern int s_to_date(unsigned long seconds, unsigned long usec, unsigned int *ms,
					unsigned int *sec, unsigned int *min, unsigned int *hour,
					unsigned int *day, unsigned int *month, unsigned int *year);
static void xmit_history_dump(void)
{
	int dump_num;
	struct timeval ref_tv = {0};
	int i,j;
	int s_delta, us_delta;
	unsigned int us, sec, min, hour, day, month, year;

	if( curr_xmit_log_num > 1 ){
		//CCCI_MSG_INF("cci", "\n");
		CCCI_MSG_INF("cci", "dump CCIF TX history: \n");
		j = curr_xmit_log_index;
		if(curr_xmit_log_num < 16)
			dump_num = curr_xmit_log_num;
		else
			dump_num = 16; 
		ref_tv = xmit_log[j].tv;
		j++;
		j &= 0xF;

		for(i=1; i< dump_num; i++){
			if(xmit_log[j].tv.tv_sec == ref_tv.tv_sec){
				if(xmit_log[j].tv.tv_usec>=ref_tv.tv_usec){
					us_delta = (int)(xmit_log[j].tv.tv_usec - ref_tv.tv_usec);
					s_delta = 0;
				}else{
					us_delta = 0;
					s_delta = -1;
				}
			}else if(xmit_log[j].tv.tv_sec > ref_tv.tv_sec){
				if(xmit_log[j].tv.tv_usec>=ref_tv.tv_usec){
					us_delta = (int)(xmit_log[j].tv.tv_usec - ref_tv.tv_usec);
					s_delta = (int)(xmit_log[j].tv.tv_sec - ref_tv.tv_sec);
				}else{
					us_delta = (int)(xmit_log[j].tv.tv_usec - ref_tv.tv_usec + 1000*1000);
					s_delta = (int)(xmit_log[j].tv.tv_sec - ref_tv.tv_sec - 1);
				}
			}else{
				us_delta = 0;
				s_delta = -1;
			}
			s_to_date(xmit_log[j].tv.tv_sec, xmit_log[j].tv.tv_usec, &us, &sec, &min, 
					&hour, &day, &month, &year);
			CCCI_MSG_INF("cci", "%08X %08X %02d %08X   %d-%02d-%02d %02d:%02d:%02d.%06d   %03x   %d.%06ds\n", 
				xmit_log[j].data[0], xmit_log[j].data[1], xmit_log[j].data[2], xmit_log[j].data[3],
				year, month, day, hour, min, sec, us,
				xmit_log[j].retry_count, s_delta, us_delta);
			ref_tv = xmit_log[j].tv;
			mdelay(1);
			j++;
			j &= 0xF;
		}
	}
}

static void xmit_history_init(void)
{
	xmit_log=(xmit_log_unit_t*)kzalloc(sizeof(xmit_log_unit_t)*16,GFP_KERNEL);
	if(NULL != xmit_log){
		curr_xmit_log_index = 0;
		curr_xmit_log_num = 0;
	}

	return;
}

static void xmit_history_deinit(void)
{
	if(NULL != xmit_log){
		curr_xmit_log_index = 0;
		curr_xmit_log_num = 0;
		kfree(xmit_log);
	}

	return;
}

int get_md_wakeup_src(char *buf, unsigned int len)
{
	unsigned int i, rx, ch;
	CCCI_BUFF_T *data;
	unsigned int rx_ch[CCIF_MAX_PHY][2] = {{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0},{0,0}};
	char str[64];
	char log_buf[256] = "";
	int ret = 0;

	rx = *CCIF_RCHNUM(CCIF_BASE);
	if(rx == 0)
		return ret;
	
	for (i = 0; i < CCIF_MAX_PHY; i++) {
		if (rx&(1<<i)) {
			data = ((CCCI_BUFF_T *)CCIF_RXCHDATA(CCIF_BASE)+i);			
			for (ch = 0; ch < i; ch++)
				if (data->channel == rx_ch[ch][0])
					break;
			
			rx_ch[ch][0] = data->channel;
			rx_ch[ch][1]++;
		}
	}

	for (i = 0; i < CCIF_MAX_PHY ; i ++) {
		if (rx_ch[i][1]) {
			if ((rx_ch[i][0] >= 0) && (rx_ch[i][0] < CCCI_MAX_CHANNEL))
				sprintf(str,"%s(%d,%d) ", lg_ch_str[rx_ch[i][0]], rx_ch[i][0], rx_ch[i][1]);
			else 
				sprintf(str,"%s(%d,%d) ", "unknown", rx_ch[i][0], rx_ch[i][1]);
			strcat(log_buf, str);
		}
	}

	printk("CCIF_MD wakeup source: %s\n", log_buf);

	return ret;
}


static int ccif_ch_init(struct physical_channel *pc_ch)
{
	int ret;
	ret=ccci_logical_layer_init(pc_ch,&pc_ch->lg_layer,CCCI_MAX_CHANNEL);
	if (ret)
	{
		CCCI_MSG_INF("cci", "logical layer init fail \n");
		goto out;
	}
	spin_lock_init(&pc_ch->r_lock);
	spin_lock_init(&pc_ch->s_lock);
	pc_ch->r_errors=pc_ch->rx_idx=pc_ch->s_errors=pc_ch->tx_idx=0;
	pc_ch->flags=0;
	if (pc_ch->arb)
	*CCIF_CON(pc_ch->pc_base_addr) |=CCIF_CON_ARB;
	else 
	*CCIF_CON(pc_ch->pc_base_addr) =CCIF_CON_SEQ;
	//ccif_platform_irq_init(pc_ch->irq_line);
	ret=request_irq(pc_ch->irq_line,ccif_irq_handler, IRQF_TRIGGER_LOW, "CCIF", pc_ch);
	if (ret)
	{
		if(ret != -EEXIST){
			CCCI_MSG_INF("cci", "irq register fail \n");
		}
		goto error;		
	}
	//CCIF_UNMASK(pc_ch->irq_line);
	atomic_set(&(pc_ch->irq_cnt), 0);

	goto out;
error :
	ccci_logical_layer_destroy(&pc_ch->lg_layer);	
out:
	return ret;
}


static void ccif_irq_mask(struct physical_channel *pc_ch)
{
	if(atomic_read(&(pc_ch->irq_cnt)) == 0) {
		CCIF_MASK(pc_ch->irq_line);
		atomic_inc(&(pc_ch->irq_cnt));
	}
	//CCCI_MSG_INF("cci", "mask ccif irq:%d \n", pc_ch->irq_cnt.counter);
}

static void ccif_irq_unmask(struct physical_channel *pc_ch)
{
	if(atomic_read(&(pc_ch->irq_cnt))) {
		CCIF_UNMASK(pc_ch->irq_line);
		atomic_dec(&(pc_ch->irq_cnt));
	}
	//CCCI_MSG_INF("cci", "unmask ccif irq:%d \n", pc_ch->irq_cnt.counter);
}

static void ccif_disable(struct physical_channel *pc_ch,int sync)
{
	//CCIF_MASK(pc_ch->irq_line);
	ccif_irq_mask(pc_ch);
	clear_bit(CCCI_ENABLED,&pc_ch->flags);
	
	if (sync)
	{		
		if (WARN_ON(!preemptible()&&!current->exit_state))
		{
			//CCCI_DEBUG("Preempt count %x.\n",preempt_count());
		}
		while(test_bit(CCCI_RUNNING,&pc_ch->flags))
		{
			yield();
		}
	}
	WARN_ON(spin_is_locked(&pc_ch->r_lock));
	WARN_ON(spin_is_locked(&pc_ch->s_lock));
	return ;

}

static void ccif_enable(struct physical_channel *pc_ch)
{
	
	set_bit(CCCI_ENABLED,&pc_ch->flags);	
	//CCIF_UNMASK(pc_ch->irq_line);
	ccif_irq_unmask(pc_ch);
}

static int ccif_ch_destroy(struct physical_channel *pc_ch)
{
	int ret=0;
	pc_ch->disable(pc_ch,1);
	//CCIF_MASK(pc_ch->irq_line);
	free_irq(pc_ch->irq_line,pc_ch);
	ret=ccci_logical_layer_destroy(&pc_ch->lg_layer);
	if (ret)
		CCCI_MSG_INF("cci", "logical_layer_destroy fail \n");
	return ret;
	
}

static void ccif_reset(struct physical_channel *pc_ch)
{
	spin_lock_irq(&pc_ch->r_lock);
	pc_ch->rx_idx=pc_ch->r_errors=0;
	spin_unlock_irq(&pc_ch->r_lock);
	
	spin_lock_irq(&pc_ch->s_lock);
	pc_ch->tx_idx=pc_ch->s_errors=0;
	spin_unlock_irq(&pc_ch->s_lock);

	CCIF_CLEAR_PHY(pc_ch->pc_base_addr);
}

static int ccif_manual_triger(struct physical_channel *pc_ch,CCCI_BUFF_T *data)
{
	int ret=0;
	
	set_bit(CCCI_RUNNING,&pc_ch->flags);
	ret=pc_ch->lg_layer.process_data(&pc_ch->lg_layer,data,1,0);
	clear_bit(CCCI_RUNNING,&pc_ch->flags);
	
	return ret;
}

static int ccif_ch_xmit(struct physical_channel *pc_ch,CCCI_BUFF_T *data,int force)
{
	int ret=0;
	unsigned long flag;
	unsigned long  start_time=jiffies;
	int atomic=!preemptible();
	int irq_off=irqs_disabled();
	int can_sched=0;
	unsigned int busy;
	int tx_idx=0;
	int drop=0;
	int time=0;
	//struct timeval no_ch_tv;
	//unsigned int us, sec, min, hour, day, month, year;
	int lock_flag=0;
	
//	if (atomic==0 && irq_off==0) can_sched=1;
	if (irq_off)  start_time +=0;
	else if (atomic) start_time +=1;
	else can_sched=1;
	set_bit(CCCI_RUNNING,&pc_ch->flags);
	
retry:
	
	if((md_boot_stage != MD_BOOT_STAGE_2) &&
        (data->channel != CCCI_CONTROL_TX && data->channel != CCCI_FS_TX &&
	 data->channel != CCCI_MD_LOG_TX && data->channel != CCCI_RPC_TX))
	{
	
		if(atomic_read(&md_reset_on_going)) {
			CCCI_MSG_INF("cci", "forbid to send(lg_ch=%02d)at md reset\n", data->channel);
			ret = CCCI_MD_IN_RESET;
		}
		else {
			CCCI_MSG_INF("cci", "Not ready to send(lg_ch=%02d) at md_boot_%d\n",
				data->channel, md_boot_stage);
			ret = CCCI_MD_NOT_READY;
		}
		goto out;
	}

	if (!test_bit(CCCI_ENABLED,&pc_ch->flags))
	{
		CCCI_MSG_INF("cci", "Not enable (%s)!\n",current->comm);
		ret = CCCI_FAIL;
		goto out;
	}

	spin_lock_irqsave(&pc_ch->s_lock,flag);
	lock_flag = 1;
	busy=*CCIF_BUSY(pc_ch->pc_base_addr);
	tx_idx=pc_ch->tx_idx%pc_ch->max_pc_num;
	if (busy&(1<<tx_idx))
	{
		//CCCI_MSG_INF("cci", "No physical channel CH:%d Process:%s(%02d)\n",
		//	data->channel,current->comm,time);
		ret=CCCI_NO_PHY_CHANNEL;
		if (busy!=0xff)
			CCCI_MSG_INF("cci", "Wrong Busy value:0x%X\n",busy);
		if (can_sched)
		{
			spin_unlock_irqrestore(&pc_ch->s_lock,flag);
			lock_flag = 0;
			//yield();
			udelay(1);
			if (++time<1000)
				goto retry;
		}
		else {
			while (time_before(jiffies,start_time))
			{
				spin_unlock_irqrestore(&pc_ch->s_lock,flag);
				lock_flag = 0;
				goto retry;
			}
		}
		
		CCCI_MSG_INF("cci", "No physical channel(%s):%08X, %08X, %02d, %08X\n",
			current->comm, data->data[0], data->data[1],data->channel, data->reserved);
		//do_gettimeofday( &no_ch_tv );
		//s_to_date(no_ch_tv.tv_sec, no_ch_tv.tv_usec, &us, &sec, &min, 
		//			&hour, &day, &month, &year);

		//CCCI_MSG_INF("cci", "\n");
		//CCCI_MSG_INF("cci", "Process:%s @ %d-%02d-%02d %02d:%02d:%02d.%06d\n", 
		//		current->comm, year, month, day, hour, min, sec, us);

		//CCCI_MSG_INF("cci", "retry %d\n",time);
		//ccif_tx_fifo_dump();
		//xmit_history_dump();

		//ccci_channel_status(data->channel);
		drop=1;	
	}
	
	if (force)
	{
		if (drop==0)
			ret=sizeof(*data);
	}
	else 
	{		
		ret=pc_ch->lg_layer.process_data(&pc_ch->lg_layer,data,0,drop);
		if (drop)   
			ret=CCCI_NO_PHY_CHANNEL;
	}
	
	if (ret != sizeof(*data))
	{
		pc_ch->s_errors++;

		if(ret != CCCI_NO_PHY_CHANNEL)
			CCCI_MSG_INF("cci", "ccif_ch_xmit fail: %d\n",ret);
		goto out_unlock;
	}
	*CCIF_BUSY(pc_ch->pc_base_addr)=1<<tx_idx;
	//memcpy(((CCCI_BUFF_T*)(CCIF_TXCHDATA(pc_ch->pc_base_addr))+tx_idx),data,sizeof(*data));
	ccci_buff_t_assign((CCCI_BUFF_T*)CCIF_TXCHDATA(pc_ch->pc_base_addr)+tx_idx,data);
	*CCIF_TCHNUM(pc_ch->pc_base_addr)=tx_idx;
	pc_ch->tx_idx++;
	ret=sizeof(*data);

	// Log xmit history
	if( (curr_xmit_log_index>=0)&&(0==drop) ){
		xmit_log[curr_xmit_log_index].data[0] = data->data[0];
		xmit_log[curr_xmit_log_index].data[1] = data->data[1];
		xmit_log[curr_xmit_log_index].data[2] = data->channel;
		xmit_log[curr_xmit_log_index].data[3] = data->reserved;
		do_gettimeofday( &(xmit_log[curr_xmit_log_index].tv) );
		xmit_log[curr_xmit_log_index].ref_jiffs = jiffies;
		xmit_log[curr_xmit_log_index].retry_count = time;
		curr_xmit_log_index++;
		curr_xmit_log_index &= 0xF;
		if(curr_xmit_log_num < 16)
			curr_xmit_log_num++;
	}
	// Log xmit history done

out_unlock:
	if(lock_flag)
	spin_unlock_irqrestore(&pc_ch->s_lock,flag);
	
out:
	clear_bit(CCCI_RUNNING,&pc_ch->flags);
	
	if(lg_ch_tx_debug_enable & (1<< data->channel)) 
		CCCI_MSG_INF("cci", "[TX]: %08X, %08X, %02d, %08X (%02d)\n",
			 data->data[0], data->data[1], data->channel, data->reserved, tx_idx);

	return ret;
}

static void lg_default_dtor(struct logical_channel *lg_ch)
{
	WARN_ON(spin_is_locked(&lg_ch->lock));
	if (lg_ch->have_fifo)
		kfifo_free(&lg_ch->fifo);
	CCCI_CCIF_MSG("dtor channel:%s owner:%s\n",lg_ch->name,lg_ch->owner);
}

int pc_register(struct physical_channel *pc_ch,int ch_num, char *name,int buf_num,
		CCCI_CALLBACK call_back,void *private_data)
{
	struct logical_channel *lg_ch=NULL;
	int ret=0;
	
	ret=(*pc_ch->lg_layer.add_client)(&pc_ch->lg_layer,ch_num,buf_num,&lg_ch);
	if (ret)
	{
		if(ret != -EEXIST){
			CCCI_MSG_INF("cci", "register fail for %s: %d\n",name,ret);
		}
		return ret;
	}
	WARN_ON(lg_ch==NULL);
	spin_lock_init(&lg_ch->lock);
	lg_ch->name=name;
	lg_ch->have_fifo=0;
	snprintf(lg_ch->owner,sizeof(lg_ch->owner),current->comm);
#if 0
	ret=kfifo_alloc(&lg_ch->fifo,buf_num*sizeof(CCCI_BUFF_T),GFP_KERNEL);
	if (ret)
	{
		CCCI_DEBUG("kfifo alloc failed(ret=%d).\n",ret);
		goto err_out;
	}
#endif	
	lg_ch->callback=call_back;
	lg_ch->private_data=private_data;
	lg_ch->dtor=lg_default_dtor;
	CCCI_CCIF_MSG("CH:%d Name:%s Process:%s \n",ch_num,name,current->comm);
	return ret;
}

int pc_unregister(struct physical_channel *pc_ch,int ch_num)
{
	return pc_ch->lg_layer.remove_client(&pc_ch->lg_layer,ch_num);
}


void ccif_send_wakeup_md_msg(void)
{
	unsigned long flag=0;
	unsigned int busy;
	int tx_idx;
	CCCI_BUFF_T msg={.data[0]=0xFFFFFFFF, .data[1]=MD_WDT_MONITOR, .channel=CCCI_SYSTEM_TX, .reserved=0};

	spin_lock_irqsave(&ccif_ch.s_lock,flag);
	busy=*CCIF_BUSY(ccif_ch.pc_base_addr);
	if(busy == 0){
		CCCI_MSG_INF("cci", "send wake up message to md\n");
		tx_idx=ccif_ch.tx_idx%ccif_ch.max_pc_num;
		*CCIF_BUSY(ccif_ch.pc_base_addr)=1<<tx_idx;
		ccci_buff_t_assign((CCCI_BUFF_T*)CCIF_TXCHDATA(ccif_ch.pc_base_addr)+tx_idx,&msg);
		*CCIF_TCHNUM(ccif_ch.pc_base_addr)=tx_idx;
		ccif_ch.tx_idx++;
	}
	spin_unlock_irqrestore(&ccif_ch.s_lock,flag);
}

int __init ccif_module_init(void)
{
	int ret=0;	
	ret=ccif_ch.pc_init(&ccif_ch);
	if (ret)
		CCCI_MSG_INF("cci", "Init fail: %d\n",ret);

	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
	ccif_ch.enable(&ccif_ch);
	#else
	//every irq is default unmask on linux3.0, if unmask it again, kernel warning of irq unbalance happens 
	set_bit(CCCI_ENABLED,&ccif_ch.flags);
	#endif
	
	ret=ccci_chrdev_init();
	xmit_history_init();

	//register the API of get CCCI_MD wakeup source
	register_ccci_kern_func(ID_GET_MD_WAKEUP_SRC, get_md_wakeup_src);
	
	return ret;
}


void __exit ccif_module_exit(void)
{
	ccci_chrdev_exit();
	ccif_ch.pc_destroy(&ccif_ch);
	xmit_history_deinit();
}

#if 0
module_init(ccif_module_init);
module_exit(ccif_module_exit);
#endif


EXPORT_SYMBOL(get_md_wakeup_src);
EXPORT_SYMBOL(pc_unregister);
EXPORT_SYMBOL(pc_register);
EXPORT_SYMBOL(ccif_ch);
EXPORT_SYMBOL(lg_ch_tx_debug_enable);
EXPORT_SYMBOL(lg_ch_rx_debug_enable);
