#include <linux/module.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/stacktrace.h>
#include "ipanic.h"

unsigned ipanic_detail_start;
unsigned ipanic_detail_end;

#define MAX_STACK_TRACE_DEPTH 64
unsigned long ipanic_stack_entries[64];

struct ipanic_oops_header oops_header;

static u32 ipanic_iv = 0xaabbccdd;

struct ipanic_ops *ipanic_ops;

#if 1
void ipanic_block_scramble(u8 *buf, int buflen) 
{
	int i;
	u32 *p = (u32 *)buf;
	for (i = 0; i < buflen; i += 4, p++) {
		*p = *p ^ ipanic_iv;
	}
}
#else 

void ipanic_block_scramble(u8 *buf, int buflen) 
{
}

#endif

/*
 * Check if valid header is legitimate
 * return
 *  0: contain good panic data 
 *  1: no panic data
 *  2: contain bad panic data
 */
int ipanic_header_check(const struct ipanic_header *hdr) 
{
	if (hdr->magic != AEE_IPANIC_MAGIC) {
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: No panic data available [Magic header]\n");
		return 1;
	}

	if (hdr->version != AEE_IPANIC_PHDR_VERSION) {
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: Version mismatch (%d != %d)\n",
		       hdr->version, AEE_IPANIC_PHDR_VERSION);
		return 2;
	}

	if ((hdr->oops_header_length < 16) || (hdr->oops_header_length > AEE_IPANIC_DATALENGTH_MAX)) {
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: No panic data available   \
                [Invalid oops header length - %d]\n", hdr->oops_header_length);
		return 2;
	}

    /*panic() has zero detailed size into expdb*/
	if ( (hdr->oops_detail_length < 16 && hdr->oops_detail_length != 0) ||
            (hdr->oops_detail_length > AEE_IPANIC_DATALENGTH_MAX)) {
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: No panic data available \
                                        [Invalid oops detail length - %d]\n", hdr->oops_detail_length);
		return 2;
	}

	if ((hdr->console_length < 16) || (hdr->console_length > (1 << CONFIG_LOG_BUF_SHIFT))) {
		xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: No panic data available  \
                [Invalid oops console length - %d]\n", hdr->console_length);
		return 2;
	}
	return 0;
}

void ipanic_header_dump(const struct ipanic_header *hdr)
{
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "aee-ipanic: magic(%x) version(%d)\n",
		    hdr->magic, hdr->version);
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "\theader(%u, %u) detail(%u, %u) console(%u, %u)\n",
		    hdr->oops_header_offset, hdr->oops_header_length,
		    hdr->oops_detail_offset, hdr->oops_detail_length,
		    hdr->console_offset, hdr->console_length);
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "\tprocess(%u, %u)\n",
		    hdr->userspace_info_offset, hdr->userspace_info_length);
	xlog_printk(ANDROID_LOG_INFO, IPANIC_LOG_TAG, "\tandroid main(%u, %u), android system(%u, %u), android radio(%u, %u)\n",
		    hdr->android_main_offset, hdr->android_main_length, 
		    hdr->android_system_offset, hdr->android_system_length,
		    hdr->android_radio_offset, hdr->android_radio_length);
}

/*Exception stack is on top of task normal stack
* call panic() will save backtrace and other info 
* into db file
*/
void ipanic_save_current_tsk_info(void)
{
    struct stack_trace trace;
    int i, plen;
    struct task_struct *tsk;
   
	tsk = current_thread_info()->task;
    
    memset(&oops_header, 0, sizeof(struct ipanic_oops_header));

    /* Grab kernel task stack trace */
    trace.nr_entries	= 0;
    trace.max_entries	= MAX_STACK_TRACE_DEPTH;
    trace.entries		= ipanic_stack_entries;
    trace.skip		= 0;
    save_stack_trace_tsk(tsk, &trace);

    /* Skip the entries -  ipanic_save_current_tsk_info/save_stack_trace_tsk */
    for (i = 2; i < trace.nr_entries; i++) {
        int off = strlen(oops_header.backtrace);
        int plen = IPANIC_OOPS_HEADER_BACKTRACE_LENGTH - off;
        if (plen > 16) {
            snprintf(oops_header.backtrace + off, plen, "[<%p>] %pS\n",
                    (void *)ipanic_stack_entries[i], (void *)ipanic_stack_entries[i]);
        }
    }

    /* Current panic user tasks */
    plen = 0;
    while (tsk && (tsk->pid != 0) && (tsk->pid != 1)) {
        /* FIXME: Check overflow ? */
        plen += sprintf(oops_header.process_path + plen, "[%s, %d]", tsk->comm, tsk->pid);
        tsk = tsk->real_parent;
    }
}

void ipanic_oops_start()
{

	ipanic_detail_start = log_end;
}

void ipanic_oops_end(void)
{
	ipanic_detail_end = log_end;
}

void register_ipanic_ops(struct ipanic_ops *ops)
{
	ipanic_ops = ops;
}

struct aee_oops *ipanic_oops_copy(void)
{
	if (ipanic_ops) {
		return ipanic_ops->oops_copy();
	}
	else {
		return NULL;
	}
}

EXPORT_SYMBOL(ipanic_oops_copy);

void ipanic_oops_free(struct aee_oops *oops, int erase)
{
	if (ipanic_ops) {
		ipanic_ops->oops_free(oops, erase);
	}
}
EXPORT_SYMBOL(ipanic_oops_free);

