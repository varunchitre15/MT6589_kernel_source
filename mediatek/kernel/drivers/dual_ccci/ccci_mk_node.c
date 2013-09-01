#include <linux/module.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/kfifo.h>

#include <linux/firmware.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <ccci_common.h>

typedef struct _ccci_node
{
	char *name;
	char *type;
	int  idx;
	int  ext_num;
}ccci_node_t;

typedef struct _ccci_node_type
{
	char *type;
	int  major;
	int  minor_start;
	int  range;
}ccci_node_type_t;

typedef struct _ccci_node_type_table
{
	int major;
	ccci_node_type_t array[CCCI_NODE_TYPE_NUM];
}ccci_node_type_table_t;

static ccci_node_t ccci1_node_list[] = {
	//{"ccci_sys_rx",		"std chr",		2,		0},
	//{"ccci_sys_tx",		"std chr",		3,		0},
	{"ccci_pcm_rx",		"std chr",		4,		0},
	{"ccci_pcm_tx",		"std chr",		5,		0},
	{"ccci_uem_rx",		"std chr",		18,		0},
	{"ccci_uem_tx",		"std chr",		19,		0},
	{"ccci_md_log_rx",	"std chr",		42,		0},
	{"ccci_md_log_tx",	"std chr",		43,		0},

	{"ccci_ipc_1220_0",	"ipc",			0,		0},

	{"ccci_fs",			"fs",			0,		0},

	{"ccci_monitor",	"vir chr",		0,		0},
	{"ccci_ioctl",		"vir chr",		1,		2},
};

static ccci_node_t ccci2_node_list[] = {
	//{"ccci2_sys_rx",	"std chr",		2,		0},
	//{"ccci2_sys_tx",	"std chr",		3,		0},
	{"ccci2_pcm_rx",	"std chr",		4,		0},
	{"ccci2_pcm_tx",	"std chr",		5,		0},
	{"ccci2_uem_rx",	"std chr",		18,		0},
	{"ccci2_uem_tx",	"std chr",		19,		0},
	{"ccci2_md_log_rx",	"std chr",		42,		0}, 
	{"ccci2_md_log_tx",	"std chr",		43,		0},

	{"ccci2_ipc_",		"ipc",			0,		1}, 

	{"ccci2_fs",		"fs",			0,		0},

	{"ccci2_monitor",	"vir chr",		0,		0},
	{"ccci2_ioctl",		"vir chr",		1,		2},
};

static ccci_node_type_table_t	ccci_node_type_table[MAX_MD_NUM];
static void						*dev_class = NULL;




static void init_ccci_node_type_table(void)
{
	int i;
	int curr = 0;
	int major;
	ccci_node_type_table_t *curr_table = NULL;
	
	memset(ccci_node_type_table, 0, sizeof(ccci_node_type_table));

	for(i = 0; i < MAX_MD_NUM; i++){
		major = get_dev_major_for_md_sys(i);
		curr_table = &ccci_node_type_table[i];
		curr = 0;
		if(major < 0)
			continue;

		curr_table->major = major;

		curr_table->array[0].type = "std chr";
		curr_table->array[0].major = major;
		curr_table->array[0].minor_start = curr;
		curr_table->array[0].range = STD_CHR_DEV_NUM;
		curr += STD_CHR_DEV_NUM;

		curr_table->array[1].type = "ipc";
		curr_table->array[1].major = major;
		curr_table->array[1].minor_start = curr;
		curr_table->array[1].range = IPC_DEV_NUM;
		curr += IPC_DEV_NUM;

		curr_table->array[2].type = "fs";
		curr_table->array[2].major = major;
		curr_table->array[2].minor_start = curr;
		curr_table->array[2].range = FS_DEV_NUM;
		curr += FS_DEV_NUM;

		curr_table->array[3].type = "vir chr";
		curr_table->array[3].major = major;
		curr_table->array[3].minor_start = curr;
		curr_table->array[3].range = VIR_CHR_DEV_NUM;
		curr += VIR_CHR_DEV_NUM;

		curr_table->array[4].type = "tty";
		curr_table->array[4].major = major;
		curr_table->array[4].minor_start = curr;
		curr_table->array[4].range = TTY_DEV_NUM;
		curr += TTY_DEV_NUM;
	}
}


int get_md_id_by_dev_major(int dev_major)
{
	int i ;
	
	for(i = 0; i < MAX_MD_NUM; i++){
		if(ccci_node_type_table[i].major == dev_major)
			return i;
	}

	return -1;
}


int get_dev_id_by_md_id(int md_id, char node_name[], int *major, int* minor)
{
	int	i;
	ccci_node_type_table_t	*curr_table = NULL;
	
	curr_table = &ccci_node_type_table[md_id];

	for(i = 0; i < CCCI_NODE_TYPE_NUM; i++) {
		if(curr_table->array[i].type == NULL)
			break;
		if(strcmp(curr_table->array[i].type, node_name) == 0) {
			if(major != NULL)
				*major = curr_table->array[i].major;
			if(minor != NULL)
				*minor = curr_table->array[i].minor_start;
			return 0;
		}
	}

	return -1;
}


/***************************************************************************
 * Make device node helper function section
 ***************************************************************************/
static void* create_dev_class(struct module *owner, const char *name)
{
	int err = 0;
	
	struct class *dev_class = class_create(owner, name);
	if(IS_ERR(dev_class)){
		err = PTR_ERR(dev_class);
		CCCI_MSG("create %s class fail: %d\n", name, err);
		return NULL;
	}

	return dev_class;
}


/*
static void release_dev_class(void *dev_class)
{
	if(NULL != dev_class)
		class_destroy(dev_class);
}
*/


static int register_dev_node(void *dev_class, const char *name, int major_id, int minor_start_id, int index)
{
	int ret = 0;
	dev_t dev;
	struct device *devices;

	if(index >= 0) {
		dev = MKDEV(major_id, minor_start_id) + index;
		devices = device_create( (struct class *)dev_class, NULL, dev, NULL, "%s%d", name, index );
	} else {
		dev = MKDEV(major_id, minor_start_id);
		devices = device_create( (struct class *)dev_class, NULL, dev, NULL, "%s", name );
	}

	if(IS_ERR(devices)) {
		ret = PTR_ERR(devices);
	}
	
	return ret;
}


/*
static void release_dev_node(void *dev_class, int major_id, int minor_start_id, int index)
{
	device_destroy(dev_class,MKDEV(major_id, minor_start_id) + index);
}
*/


int init_ccci_dev_node(void)
{
	init_ccci_node_type_table();

	// Make device class
	dev_class = create_dev_class(THIS_MODULE, "ccci_node");
	if(dev_class == NULL)
		return -1;
	
	return 0;
}


int mk_ccci_dev_node(int md_id)
{
	int	i, j, major, minor, num;
	ccci_node_t *dev_node;
	int		ret = 0;

	if (md_id == MD_SYS1) {
		dev_node = ccci1_node_list;
		num = sizeof(ccci1_node_list)/sizeof(ccci_node_t);
	} else if (md_id == MD_SYS2) {
		dev_node = ccci2_node_list;
		num = sizeof(ccci2_node_list)/sizeof(ccci_node_t);
	} else {
		return -1;
	}

	for(i = 0; i < num; i++) {
		if(get_dev_id_by_md_id(md_id, dev_node[i].type, &major, &minor) < 0)
			break;

		minor = minor + dev_node[i].idx;
		if(dev_node[i].ext_num == 0) {
			ret = register_dev_node(dev_class, dev_node[i].name, major, minor, -1);
			if(ret < 0)
				CCCI_MSG_INF(md_id, "cci", "create %s device fail: %d\n", dev_node[i].name, ret);
		} else {
			for(j = 0; j < dev_node[i].ext_num; j++) {
				ret = register_dev_node(dev_class, dev_node[i].name, major, minor, j);
				if(ret < 0) {
					CCCI_MSG_INF(md_id, "cci", "create %s device fail: %d\n", dev_node[i].name, ret);
					return ret;
				}
			}
		}
	}

	return ret;
}

