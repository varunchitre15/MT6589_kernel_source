#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <ccci.h>

static  struct init_mod  __refdata init_mod_array[]={
	{
		.init=ccif_module_init,
		.exit=ccif_module_exit,
	},
	{
		.init=ccci_md_init_mod_init,
		.exit=ccci_md_init_mod_exit,
	},
	{
		.init=ccci_tty_init,
		.exit=ccci_tty_exit,
	},	
	{
		.init=ccci_fs_init,
		.exit=ccci_fs_exit,
	},
	{
		.init=ccci_ipc_init,
		.exit=ccci_ipc_exit,
	},
	/*{
		.init=ccci_pmic_init,
		.exit=ccci_pmic_exit,
	},*/
	{
		.init=NULL,
		.exit=NULL,
	},
};

static int __init ccif_init(void)
{
	struct init_mod *mod_ptr=init_mod_array;
	int ret=0;
	while (mod_ptr->init)
	{
		ret=mod_ptr->init();
		if (ret)  
		{
			mod_ptr--;
			break;
		}
		mod_ptr++;
	}
	if (ret)
	{
		while (1)
		{
			mod_ptr->exit();
			if (mod_ptr==init_mod_array) break;
			mod_ptr--;
		}
	}
	return ret;
}

static void __init ccif_exit(void)
{	
	struct init_mod *mod_ptr=init_mod_array;
	while (mod_ptr->exit)
	{
		mod_ptr->exit();
		mod_ptr++;
	}

}

module_init(ccif_init);
module_exit(ccif_exit);

MODULE_DESCRIPTION("CCCI Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MTK");