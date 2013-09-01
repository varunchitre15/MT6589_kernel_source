#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <ccif.h>
#include <ccci_cfg.h>
#include <ccci_layer.h>
#include <mach/irqs.h>
#include <ccci_common.h>



unsigned long long lg_ch_rx_debug_enable[MAX_MD_NUM] = {0ULL, 0ULL};
unsigned long long lg_ch_tx_debug_enable[MAX_MD_NUM] = {0ULL, 0ULL};

static int __init ccci_init(void)
{
	int ret = CCCI_ERR_MODULE_INIT_OK;
	unsigned int md_num = 1;	
	int i = 0;
	int md_en[MAX_MD_NUM] = {0, 0};
	
	//1. Get and set Support MD nmmber
	md_num = get_md_sys_max_num();
	set_md_sys_max_num(md_num);

	//2. Get and set MD enable table
	for(i = 0; i < md_num; i++) {
		if(is_md_enable(i)){
			md_en[i] = 1;
			set_md_enable(i, 1);
		} else {
			md_en[i] = 0;
			set_md_enable(i, 0);
		}
	}

	//3. Init ccci device table	
    ret = init_ccci_dev_node();
	if(ret) {
		CCCI_MSG("init_ccci_dev_node fail: %d\n", ret);
		ret = -CCCI_ERR_INIT_DEV_NODE_FAIL;
		goto out;
	}

	//4. Init ccci driver for each modem
	for(i = 0; i < md_num; i++)
	{
		if(md_en[i] == 0) {
			CCCI_MSG_INF(i, "cci", "md initial fail: md%d is not enabled\n", i+1);
			continue;
		}
		
		// 4.0 Init platform support function
		ret = platform_init(i, !md_en[i]);
		if (ret) {
			CCCI_MSG_INF(i, "cci", "platform_init fail: %d\n", ret);
			ret = -CCCI_ERR_INIT_PLATFORM_FAIL;
			goto platform_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "platform_init OK!\n");
		}

		// Make ccci device node
		ret = mk_ccci_dev_node(i);
		if (ret) {
			CCCI_MSG_INF(i, "cci", "mk_ccci_dev_node fail: %d\n", ret);
			ret = -CCCI_ERR_MK_DEV_NODE_FAIL;
			goto platform_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "mk_ccci_dev_node OK!\n");
		}

		// Statistics init
		statistics_init(i);

		// 4.1 Init ccci logical layer
		ret = ccci_logic_layer_init(i);
		if(ret) {
			CCCI_MSG_INF(i, "cci", "ccci logical layer init fail: %d\n", ret);
			ret = -CCCI_ERR_INIT_LOGIC_LAYER_FAIL;
			goto logic_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "ccci_logic_layer_init OK!\n");
		}
		
		// 4.2 Init md ctrl
		ret = ccci_md_ctrl_init(i);
		if(ret) {
			CCCI_MSG_INF(i, "cci", "ccci md ctrl init fail: %d\n", ret);
			ret = -CCCI_ERR_INIT_MD_CTRL_FAIL;
			goto mdctl_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "ccci_md_ctrl_init OK!\n");
		}
		
		// 4.3 Init char dev
		ret = ccci_chrdev_init(i);
		if (ret) {
			CCCI_MSG_INF(i, "cci", "ccci_chrdev_init fail: %d\n", ret);
			ret = -CCCI_ERR_INIT_CHAR_DEV_FAIL;
			goto char_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "ccci_chrdev_init OK!\n");
		}
		
		// 4.4 Init tty dev
		ret = ccci_tty_init(i);
		if (ret) {
			CCCI_MSG_INF(i, "cci", "ccci_tty_init fail: %d\n", ret);
			ret = -CCCI_ERR_INIT_TTY_FAIL;
			goto tty_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "ccci_tty_init OK!\n");
		}
		
		// 4.5 Init ipc dev
		ret = ccci_ipc_init(i);
		if (ret) {
			CCCI_MSG_INF(i, "cci", "ccci_ipc_init fail: %d\n", ret);
			ret = -CCCI_ERR_INIT_IPC_FAIL;
			goto ipc_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "ccci_ipc_init OK!\n");
		}
		
		// 4.6 Init rpc dev
		ret = ccci_rpc_init(i);
		if (ret) {
			CCCI_MSG_INF(i, "cci", "ccci_rpc_init fail: %d\n", ret);
			ret = -CCCI_ERR_INIT_RPC_FAIL;
			goto rpc_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "ccci_rpc_init OK!\n");
		}
		
		// 4.7 Init fs dev
		ret = ccci_fs_init(i);
		if (ret) {
			CCCI_MSG_INF(i, "cci", "ccci_fs_init fail: %d\n", ret);
			ret = -CCCI_ERR_INIT_FS_FAIL;
			goto fs_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "ccci_fs_init OK!\n");
		}
		
		// 4.8 Init ccmni dev
		ret = ccmni_init(i);
		if (ret) {
			CCCI_MSG_INF(i, "cci", "ccmni_init_v1 fail: %d\n", ret);
			ret = -CCCI_ERR_INIT_CCMNI_FAIL;
			goto ccmni_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "ccmni_init_v1 OK!\n");
		}
		
		// 4.9 Init pmic dev
		// ....
		
		// 4.10 Init vir char dev
		ret = ccci_vir_chrdev_init(i);
		if (ret) {
			CCCI_MSG_INF(i, "cci", "ccci_vir_chrdev_init fail: %d\n", ret);		
			ret = -CCCI_ERR_INIT_VIR_CHAR_FAIL;
			goto virchar_out;
		} else {
			CCCI_DBG_MSG(i, "cci", "ccci_vir_chrdev_init OK!\n");
		}

		// 4.11 Register IPO-H call back
		register_ccci_kern_func_by_md_id(i, ID_IPO_H_RESTORE_CB, ccci_ipo_h_restore);
		
		CCCI_MSG_INF(i, "cci", "md initial OK!\n");
	}

	// 5. Init common section
	ret = ccci_md_ctrl_common_init();
	
	goto out;


virchar_out:
	ccci_vir_chrdev_exit(i);
	
ccmni_out:
	ccmni_exit(i);

fs_out:
	ccci_fs_exit(i);
	
rpc_out:
	ccci_rpc_exit(i);

ipc_out:
	ccci_ipc_exit(i);
	
tty_out:
	ccci_tty_exit(i);
	
char_out:
	ccci_chrdev_exit(i);

mdctl_out:
	ccci_md_ctrl_exit(i);
	
logic_out:
	ccci_logic_layer_exit(i);
	
platform_out:
	platform_deinit(i);
	
out:
	if (i == MD_SYS2) {
		ccci_vir_chrdev_exit(MD_SYS1);
		ccmni_exit(MD_SYS1);
		ccci_fs_exit(MD_SYS1);
		ccci_rpc_exit(MD_SYS1);
		ccci_ipc_exit(MD_SYS1);
		ccci_tty_exit(MD_SYS1);
		ccci_chrdev_exit(MD_SYS1);
		ccci_md_ctrl_exit(MD_SYS1);
		ccci_logic_layer_exit(MD_SYS1);
		platform_deinit(MD_SYS1);
	}
	
	if (ret == CCCI_ERR_MODULE_INIT_OK)
		CCCI_MSG("ccci module init OK\n");
	else
		CCCI_MSG("ccci module init fail: %d\n", ret);
	
	return ret;
}

static void __exit ccci_exit(void)
{
	unsigned int md_num = 1, i;

	//1. Get Support MD nmmber
	md_num = get_md_sys_max_num();

	//2. Init ccci driver for each modem
	for(i=0; i<md_num; i++)
	{
		// 3.1 Init char dev
		// 3.2 Init tty dev
		// 3.3 Init ipc dev
		// 3.4 Init rpc dev
		// 3.5 Init fs dev
		// 3.6 Init ccmni dev
		// 3.7 Init pmic dev
		// 3.8 De-Init md ctrl
		// 3.9 Init ccci logical layer
		ccci_logic_layer_exit(i);
		// 3.10 Power down md sys
	}
}

module_init(ccci_init);
module_exit(ccci_exit);

MODULE_DESCRIPTION("CCIF Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MTK");
