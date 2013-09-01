#include "ion_priv.h"
#include "ion_debug.h"
#include <linux/slab.h>
#include <linux/stacktrace.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <linux/dma-buf.h>
#define BACKTRACE_LEVEL 10
ObjectTable gUserBtTable;
ObjectTable gKernelBtTable;
ObjectTable gUserMappingTable;
ObjectTable gKernelSymbolTable;
ObjectTable gBufferTable;
StringTable gMappingStringTable;
StringTable gKernelPathTable;
struct ion_client {
        struct rb_node node;
        struct ion_device *dev;
        struct rb_root handles;
        struct mutex lock;
        unsigned int heap_mask;
        const char *name;
        struct task_struct *task;
        pid_t pid;
        struct dentry *debug_root;
};

struct kmem_cache *ion_client_usage_cachep = NULL;
struct kmem_cache *ion_buffer_usage_cachep = NULL;
struct kmem_cache *ion_address_usage_cachep = NULL;
struct kmem_cache *ion_fd_usage_cachep = NULL;
struct kmem_cache *ion_list_buffer_cachep = NULL;
struct kmem_cache *ion_list_process_cachep = NULL;
unsigned int list_buffer_cache_created = false;
unsigned int list_process_cache_created = false;
unsigned int buffer_cache_created = false;
unsigned int fd_cache_created = false;
unsigned int address_cache_created = false;
unsigned int client_cache_created = false;
struct ion_client_usage_record *client_using_list = NULL;
struct ion_client_usage_record *client_freed_list = NULL;
struct mutex client_usage_mutex;
struct ion_buffer_record *buffer_created_list = NULL;
struct ion_buffer_record *buffer_destroyed_list = NULL;
unsigned int destroyed_buffer_count = 0;
struct mutex buffer_lifecycle_mutex;
struct ion_process_record *process_created_list = NULL;
struct ion_process_record *process_destroyed_list = NULL;
struct mutex process_lifecycle_mutex;
int ion_debug_show_backtrace(struct ion_record_basic_info *tracking_info,unsigned int show_backtrace_type)
{
	unsigned int i = 0;
	unsigned int backtrace_count = 0;
	ObjectEntry *tmp = NULL;
	unsigned int stringCount = KSYM_SYMBOL_LEN+30;	
	if(tracking_info == NULL)
	{
		return 0;		
	}
	if(show_backtrace_type == ALLOCATE_BACKTRACE_INFO)
	{
		tmp = (ObjectEntry *)tracking_info->allocate_backtrace;	
		if(tmp == NULL)
			return 0;
		backtrace_count = tmp->numEntries;
	}
	else if(show_backtrace_type == RELEASE_BACKTRACE_INFO)
	{
		tmp = (ObjectEntry *)tracking_info->release_backtrace;
		if(tmp == NULL)
			return 0;
                backtrace_count = tmp->numEntries;
	}

	for(i = 0;i < backtrace_count;i++)
	{
		char tmpString[stringCount];
		ion_get_backtrace_info(tracking_info,tmpString,stringCount,i,show_backtrace_type);
		printk("%s",tmpString);
	}
	return 1;	
}
void ion_debug_show_basic_info_record(struct ion_record_basic_info *tracking_info)
{
	if(tracking_info != NULL)
	{
        	printk("===recordID.pid : %d\n",tracking_info->recordID.pid);
        	printk("===recordID.group_pid : %d\n",tracking_info->recordID.group_pid); 
        	printk("===recordID.client_address : 0x%x\n",(unsigned int)tracking_info->recordID.client_address);
        	printk("===recordID.client: 0x%x\n",(unsigned int)tracking_info->recordID.client);
        	printk("===recordID.buffer : 0x%x\n",(unsigned int)tracking_info->recordID.buffer);
		printk("===record type  : %d\n",tracking_info->record_type);
		printk("===from_kernel  : %d\n",tracking_info->from_kernel);
		printk("===allocate_backtrace : 0x%x\n",(unsigned int)tracking_info->allocate_backtrace);
		printk("===allocate_map : 0x%x\n",(unsigned int)tracking_info->allocate_map);
		ion_debug_show_backtrace(tracking_info,ALLOCATE_BACKTRACE_INFO);
		printk("===release_backtrace : 0x%x\n",(unsigned int)tracking_info->release_backtrace);
		printk("===release_map : 0x%x\n",(unsigned int)tracking_info->release_map);
		ion_debug_show_backtrace(tracking_info,RELEASE_BACKTRACE_INFO);
	}
}
void ion_debug_show_buffer_usage_record(struct ion_buffer_usage_record *buffer_usage_record)
{
	if(buffer_usage_record != NULL)
	{
		printk("===========================================\n");
		printk("===buffer usage record : %x\n",(unsigned int)buffer_usage_record);
		printk("===buffer_usage_record.next : 0x%x\n",(unsigned int)buffer_usage_record->next);
		ion_debug_show_basic_info_record(&(buffer_usage_record->tracking_info));
		printk("===buffer_usage_record.handle : 0x%x\n",(unsigned int)buffer_usage_record->handle);	
		printk("===========================================\n");	
	}
}

void ion_debug_show_address_usage_record(struct ion_address_usage_record *address_usage_record)
{
	if(address_usage_record != NULL)
	{
        	printk("===========================================\n");
        	printk("===address usage record : %x\n",(unsigned int)address_usage_record);
        	printk("===address_usage_record.next : 0x%x\n",(unsigned int)address_usage_record->next);
        	ion_debug_show_basic_info_record(&(address_usage_record->tracking_info));
        	printk("===address_usage_record.address_type : %x\n",address_usage_record->address_type);
		printk("===address_usage_record.mapping_address : 0x%x\n",(unsigned int)address_usage_record->mapping_address);
        	printk("===address_usage_record.size : %x\n",address_usage_record->size);
        	printk("===address_usage_record.fd : 0x%x\n",address_usage_record->fd);
        	printk("===========================================\n");
	}
}

void ion_debug_show_fd_usage_record(struct ion_fd_usage_record *fd_usage_record)
{
	if(fd_usage_record != NULL)
	{
        	printk("===========================================\n");
        	printk("===fd usage record : %x\n",(unsigned int)fd_usage_record);
        	printk("===fd_usage_record.next : 0x%x\n",(unsigned int)fd_usage_record->next);
        	ion_debug_show_basic_info_record(&(fd_usage_record->tracking_info));
        	printk("===fd_usage_record.fd : 0x%d\n",fd_usage_record->fd);
        	printk("===========================================\n");
	}
}

void ion_debug_show_client_usage_record(struct ion_client_usage_record *client_usage_record)
{
	if(client_usage_record != NULL)
	{	
        	printk("===========================================\n");
        	printk("===client usage record : %x\n",(unsigned int)client_usage_record);
        	printk("===client_sage_record.next : 0x%x\n",(unsigned int)client_usage_record->next);
        	ion_debug_show_basic_info_record(&(client_usage_record->tracking_info));
        	printk("===client_usage_record.fd : 0x%d\n",client_usage_record->fd);
        	printk("===========================================\n");
	}
}
void ion_debug_show_process_record(struct ion_process_record *process_record)
{
	if(process_record != NULL)
	{
		printk("============================================\n");
		printk("===process : %x\n",(unsigned int)process_record);
		printk("===process.next : %x\n",(unsigned int)process_record->next);
		printk("===process.count : %d\n",process_record->count);
		printk("===process.pid : %d\n",process_record->pid);
		printk("===process.group_id : %d\n",process_record->group_id);
		printk("===process.address_using_list : %x\n",(unsigned int)process_record->address_using_list);
		printk("===process.address_freed_list : %x\n",(unsigned int)process_record->address_freed_list);
		//printk("===process.ion_address_usage_mutex : %d\n",process_record->ion_address_usage_mutex);
		printk("===process.fd_using_list : %x\n",(unsigned int)process_record->fd_using_list);
		printk("===process.fd_freed_list : %x\n",(unsigned int)process_record->fd_freed_list);
		//printk("===process.ion_fd_usage_mutex : %d\n",process_record->ion_fd_usage_mutex);
	}
}
void ion_debug_show_buffer_record(struct ion_buffer_record *buffer_record)
{
	if(buffer_record != NULL)
	{
        	printk("===========================================\n");
        	printk("===buffer_record : %x\n",(unsigned int)buffer_record);
        	printk("===buffer_record.next : 0x%x\n",(unsigned int)buffer_record->next);
		printk("===buffer_record.buffer : 0x%x\n",(unsigned int)buffer_record->buffer_address);
        	printk("===buffer_record.heap_type : 0x%d\n",(unsigned int)buffer_record->heap_type);
        	printk("===buffer_record.size : 0x%d\n",buffer_record->size);
        	printk("===buffer_record.buffer_using_list : 0x%x\n",(unsigned int)buffer_record->buffer_using_list);
        	printk("===buffer_record.buffer_freed_list : 0x%x\n",(unsigned int)buffer_record->buffer_freed_list);
		//printk("===buffer_record.buffer_usage_mutex : 0x%x\n",buffer_record->ion_buffer_usage_mutex);
        	printk("===buffer_record.address_using_list : 0x%x\n",(unsigned int)buffer_record->address_using_list);
        	printk("===buffer_record.address_freed_list : 0x%x\n",(unsigned int)buffer_record->address_freed_list);
		//printk("===buffer_record.address_usage_mutex : 0x%x\n",(unsigned int)buffer_record->ion_address_usage_mutex);
        	printk("===========================================\n");
	}
}
char *ion_get_backtrace_info(struct ion_record_basic_info *tracking_info,char *backtrace_string,unsigned int backtrace_string_len, unsigned int backtrace_index,unsigned int show_backtrace_type)
{
	ObjectEntry *tmpBacktrace = NULL;
	ObjectEntry *tmpMapping = NULL;
	unsigned int backtrace_info = BACKTRACE_MAX;
	unsigned int *backtrace = NULL;
	if(tracking_info == NULL)
	{
		printk("[ion_get_backtrace_info]ERROR input tracking_info is NULL\n");
		return NULL;
	}

	if(show_backtrace_type == ALLOCATE_BACKTRACE_INFO)		
	{
		tmpBacktrace = (ObjectEntry *)tracking_info->allocate_backtrace;
		tmpMapping = (ObjectEntry *)tracking_info->allocate_map;
		backtrace_info = tracking_info->allocate_backtrace_type;
	}
	else if(show_backtrace_type == RELEASE_BACKTRACE_INFO)
	{
		tmpBacktrace = (ObjectEntry *)tracking_info->release_backtrace;
		tmpMapping = (ObjectEntry *)tracking_info->release_map;
		backtrace_info = tracking_info->release_backtrace_type;
 	}

	if((tmpBacktrace == NULL) || (tmpBacktrace->numEntries <= 0))
        {
        	printk("[ion_get_backtrace_info]ERROR tmpBacktrace is NULL or tmpBacktrace->numEntries <=0\n");
		return NULL;
        }
	else
	{
		backtrace = (unsigned int *)tmpBacktrace->object;	
	}
	if(backtrace == NULL)
	{
		printk("[ion_get_backtrace_info]ERROR backtrace is NULL\n");
		return NULL;
	}
	if(backtrace_info == USER_BACKTRACE)
        {
		if(tmpMapping != NULL)
		{
                	struct mapping *backtrace_mapping = (struct mapping *)tmpMapping->object;
			//unsigned int end_address = 0;
			if(backtrace_mapping!= NULL)
			{
				//unsigned int end_address = 0;
				//end_address = backtrace_mapping[backtrace_index].address + backtrace_mapping[backtrace_index].size;
                		snprintf(backtrace_string,backtrace_string_len,"USERSPACE BACKTRACE[%d] address: 0x%x mapping address: 0x%x - 0x%x lib: %s\n",backtrace_index,backtrace[backtrace_index],backtrace_mapping[backtrace_index].address,(backtrace_mapping[backtrace_index].address + backtrace_mapping[backtrace_index].size),backtrace_mapping[backtrace_index].name);
				//snprintf(backtrace_string,backtrace_string_len,"USERSPACE BACKTRACE[%d] address %x \n",backtrace_index,backtrace[backtrace_index]);
				return backtrace_string;
			}	
			else
                	{
                       		printk("[ion_get_backtrace_info]ERROR backtrace_mapping is NULL\n");
                	}
		}
		else
		{
			printk("[ion_get_backtrace_info]ERROR tmpMapping is NULL\n");
		}
		snprintf(backtrace_string,backtrace_string_len,"USERSPACE BACKTRACE[%d] address: 0x%x\n",backtrace_index,backtrace[backtrace_index]);
        }
        else if(backtrace_info == KERNEL_BACKTRACE)
        {
		if(tmpMapping != NULL)
                {
			unsigned int *backtrace_symbol = (unsigned int *)tmpMapping->object;
			if(backtrace_symbol != NULL)	
			{
				snprintf(backtrace_string,backtrace_string_len,"KERNELSPACE BACKTRACE[%d]2 address: 0x%x symbol: %s\n",backtrace_index,backtrace[backtrace_index],(char *)*(backtrace_symbol+backtrace_index));
				//snprintf(backtrace_string,backtrace_string_len,"KERNELSPACE BACKTRACE[%d]2 address: 0x%x \n",backtrace_index,backtrace[backtrace_index]);
	
				return backtrace_string;
			}
			else
                        {
                                printk("[ion_get_backtrace_info]ERROR backtrace_mapping is NULL\n");
                        }
		}
		else
                {
                        printk("[ion_get_backtrace_info]ERROR tmpMapping is NULL\n");
                }
		snprintf(backtrace_string,backtrace_string_len,"KERNELSPACE BACKTRACE[%d] address: 0x%x ttt\n",backtrace_index,backtrace[backtrace_index]);
        }
	return NULL;
}
void *ion_get_list(unsigned int record_type ,void  *record, unsigned int list_type)
{

	if(record_type == LIST_BUFFER)
	{
		struct ion_buffer_record *buffer_record = (struct ion_buffer_record *)record;
		switch(list_type)
		{
	   		case BUFFER_ALLOCATION_LIST:
	   		{
				printk("[ion_get_list return]buffer using list %x\n",(unsigned int)buffer_record->buffer_using_list);
				ion_debug_show_buffer_usage_record(buffer_record->buffer_using_list);	
				return  buffer_record->buffer_using_list;
			}
			case BUFFER_FREE_LIST:
			{
				printk("[ion_get_list return]buffer freed list %x\n",(unsigned int)buffer_record->buffer_freed_list);
				ion_debug_show_buffer_usage_record(buffer_record->buffer_freed_list);
				return  buffer_record->buffer_freed_list;
     			}
        		case ADDRESS_ALLOCATION_LIST:
			{
				printk("[ion_get_list return] address using list %x\n",(unsigned int)buffer_record->address_using_list);
				ion_debug_show_address_usage_record(buffer_record->address_using_list);
				return  buffer_record->address_using_list;
       	   		}
        		case ADDRESS_FREE_LIST:
	   		{
				printk("[ion_get_list return] address freed list %x\n",(unsigned int)buffer_record->address_freed_list);
				ion_debug_show_address_usage_record(buffer_record->address_freed_list);
				return  buffer_record->address_freed_list;
	   		}
		}
	}
	else if(record_type == LIST_PROCESS)
	{
		struct ion_process_record *process_record = (struct ion_process_record *)record;
		switch(list_type)
		{
           		case FD_ALLOCATION_LIST:
	   		{
				printk("[ion_get_list return]process fd using list %x\n",(unsigned int)process_record->fd_using_list);
				ion_debug_show_fd_usage_record(process_record->fd_using_list);
                		return process_record->fd_using_list;
           		}
           		case FD_FREE_LIST:
	   		{
				printk("[ion_get_list return]process fd free list %x\n",(unsigned int)process_record->fd_freed_list);
				ion_debug_show_fd_usage_record(process_record->fd_freed_list);
				return process_record->fd_freed_list; 
           		}
		        case ADDRESS_ALLOCATION_LIST:
                	{
                       		printk("[ion_get_list return]process address using list %x\n",(unsigned int)process_record->address_using_list);
                        	ion_debug_show_address_usage_record(process_record->address_using_list);
                        	return  process_record->address_using_list;
                	}
                	case ADDRESS_FREE_LIST:
                	{
                        	printk("[ion_get_list return]process  address freed list %x\n",(unsigned int)process_record->address_freed_list);
                        	ion_debug_show_address_usage_record(process_record->address_freed_list);
                        	return  process_record->address_freed_list;
                	}

		}
	}
	printk("[ion_get_list_from_buffer]can't find corresponding record in list_type %d record_type %x\n",list_type,record_type );
	return NULL;
}
struct ion_buffer_record *ion_get_inuse_buffer_record(void)
{
       struct ion_buffer_record *tmp_buffer = buffer_created_list;
       printk("[ion_get_inuse_buffer_record]return buffer_record  %x\n",(unsigned int)tmp_buffer);
	if(tmp_buffer != NULL)
	{
		//printk("[ion_get_inuse_buffer_record]return buffer %x buffer_size %d\n",tmp_buffer->buffer,tmp_buffer->buffer->size);
		ion_debug_show_buffer_record(tmp_buffer);
	}
	return buffer_created_list;	
}
struct ion_buffer_record *ion_get_freed_buffer_record(void)
{
	struct ion_buffer_record *tmp_buffer = buffer_destroyed_list;
        printk("[ion_get_freed_buffer_record]return buffer_record  %x\n",(unsigned int)tmp_buffer);
        if(tmp_buffer != NULL)
        {
                //printk("[ion_get_freed_buffer_record]return buffer %x buffer_size %d\n",tmp_buffer->buffer,tmp_buffer->buffer->size);
		ion_debug_show_buffer_record(tmp_buffer);
        }

	return buffer_destroyed_list;
}
struct ion_process_record *ion_get_inuse_process_usage_record2(void)
{
       struct ion_process_record *tmp_process = process_created_list;
	printk("[ion_get_inuse_process_record3]\n");
        if(tmp_process != NULL)
        {
		printk("[ion_get_inuse_process_record]return process_record  %x pid %d\n",(unsigned int)tmp_process,tmp_process->pid);
                printk("[ion_get_inuse_process_record]return process %x \n",(unsigned int)tmp_process);
                //ion_debug_show_process_record(tmp_process);
        }
	else
	{
		printk("[ion_get_inuse_process_recoed3] tmp_process is null process_created_list is %x\n",(unsigned int)process_created_list);
	}
        return process_created_list;
}
struct ion_process_record *ion_get_freed_process_record(void)
{
       struct ion_process_record *tmp_process = process_destroyed_list;
       printk("[ion_get_freed_process_record]return process_record  %x pid %d\n",(unsigned int)tmp_process,tmp_process->pid);
        if(tmp_process != NULL)
        {
                //printk("[ion_get_freed_process_record]return process %x  %d\n",tmp_process);
                ion_debug_show_process_record(tmp_process);
        }
        return process_destroyed_list;
}
struct ion_client_usage_record *ion_get_inuse_client_record(void)
{
       struct ion_client_usage_record *tmp_client = client_using_list;
       printk("[ion_get_inuse_client_record]return client_record  %x\n",(unsigned int)tmp_client);
        if(tmp_client != NULL)
        {
                //printk("[ion_get_inuse_client_record]return client %x \n",tmp_client);
                ion_debug_show_client_usage_record(tmp_client);
        }
        return client_using_list;
}
struct ion_client_usage_record *ion_get_freed_client_record(void)
{
       struct ion_client_usage_record *tmp_client = client_freed_list;
       printk("[ion_get_inuse_client_record]return client_record  %x\n",(unsigned int)tmp_client);
        if(tmp_client != NULL)
        {
                //printk("[ion_get_inuse_client_record]return client %x \n",tmp_client);
                ion_debug_show_client_usage_record(tmp_client);
        }
        return client_freed_list;
}

unsigned int ion_get_data_from_record(void *record,unsigned int data_type)
{
	unsigned int *tmp_record = record;
	struct ion_record_basic_info *tracking_info = (struct ion_record_basic_info *)(tmp_record+1);
	switch(data_type)
	{
		case RECORD_ID:
		{
			return (unsigned int)&(tracking_info->recordID);
		}
		case RECORD_CLIENT:
		{
			return (unsigned int)tracking_info->recordID.client;
		}
		case RECORD_HANDLE:
		{
			if(tracking_info->record_type == NODE_BUFFER)
			{
				struct ion_buffer_usage_record *buffer_node = (struct ion_buffer_usage_record *)record;
				return (unsigned int)buffer_node->handle;
			}
		}
   		case RECORD_ALLOCATE_BACKTRACE_NUM:
		{
			ObjectEntry *tmp =(ObjectEntry *)tracking_info->allocate_backtrace;
			return tmp->numEntries;
		}
   		case RECORD_FREED_BACKTRACE_NUM:
		{
			ObjectEntry *tmp =(ObjectEntry *)tracking_info->release_backtrace;		
			return tmp->numEntries;
		}
   		case RECORD_ALLOCATE_MAPPING_NUM:
		{
			ObjectEntry *tmp =(ObjectEntry *)tracking_info->allocate_map;
			return tmp->numEntries;
		}
   		case RECORD_FREED_MAPPING_NUM:
		{
			ObjectEntry *tmp =(ObjectEntry *)tracking_info->release_map;	
			return tmp->numEntries;
		}
   		case RECORD_FD:
		{
			if(tracking_info->record_type == NODE_FD)
                        {
				struct ion_fd_usage_record *fd_node = (struct ion_fd_usage_record *)record;
				return (unsigned int)fd_node->fd;
			}
			else if(tracking_info->record_type == NODE_MMAP)
			{
				struct ion_address_usage_record *address_node = (struct ion_address_usage_record *)record;
                                return (unsigned int)address_node->fd;
			}
		}
   		case RECORD_ADDRESS:
		{
			if( tracking_info->record_type == NODE_MMAP)
                        {
				struct ion_address_usage_record *address_node = (struct ion_address_usage_record *)record;
				return (unsigned int)address_node->mapping_address;
			}
		}
   		case RECORD_SIZE:
		{
			if( tracking_info->record_type == NODE_MMAP)
                        {
				struct ion_address_usage_record *address_node = (struct ion_address_usage_record *)record;
				return (unsigned int)address_node->size;
			}
		}
		case RECORD_NEXT:
		{
			return *(tmp_record);
		}
		default:
		{
			printk("[ion_get_data_from_record]can't find data type (%d)error \n",data_type);
			return 0;
		}
	}
 printk("[ion_get_data_from_record]get data type %d but wrong record type %d\n",data_type,tracking_info->record_type);
 return 0;
}
struct ion_buffer_record * search_record_in_list(struct ion_buffer *buffer,struct ion_buffer_record *list,struct ion_buffer_record **previous_node)
{
	struct ion_buffer_record *tmp_buffer_record = list;
	
	if(buffer_created_list != NULL)
	{
		while(tmp_buffer_record !=NULL)
		{
			//printk("               tmp_buffer_record: 0x%x \n",tmp_buffer_record);
			if(tmp_buffer_record->buffer_address == buffer)
			{
				printk("               found record tmp_buffer_record: 0x%x *previous_node %x\n",(unsigned int)tmp_buffer_record,(unsigned int)*previous_node);
				return tmp_buffer_record;
			}
			*previous_node = tmp_buffer_record;
			tmp_buffer_record = tmp_buffer_record->next;
		}
		printk("	[search_record_in_list]can't get corresponding buffer %x in buffer list %x\n",(unsigned int)buffer,(unsigned int)list);
	}
	else
	{
		printk("	[search_record_in_list]buffer_created_list is null \n");
	}
	return NULL;
}
struct ion_process_record * search_process_in_list(pid_t pid,struct ion_process_record *list,struct ion_process_record **previous_node)
{
        struct ion_process_record *tmp_process_record = list;
        //*previous_node == NULL;
        if(process_created_list != NULL)
        {
                while(tmp_process_record !=NULL)
                {
                        printk("               tmp_process_record: 0x%x pid %d count %d\n",(unsigned int)tmp_process_record,tmp_process_record->pid,tmp_process_record->count);
                        if(tmp_process_record->pid == pid)
                        {
                                printk("               found record tmp_process_record: 0x%x *previous_node %x\n",(unsigned int)tmp_process_record,(unsigned int)*previous_node);
                                return tmp_process_record;
                        }
                        *previous_node = tmp_process_record;
                        tmp_process_record = tmp_process_record->next;
                }
                printk("        [search_record_in_list]can't get corresponding process %x in process list %x\n",(unsigned int)pid,(unsigned int)list);
        }
        else
        {
                printk("        [search_record_in_list]process_created_list is null \n");
        }
        return NULL;
}

void get_kernel_symbol(unsigned long *backtrace,unsigned int numEntries, unsigned int *kernel_symbol)
{
	unsigned int i = 0;
	char symbol[KSYM_SYMBOL_LEN];

	for(i = 0;i < numEntries;i++)	
	{
		sprint_symbol(symbol,*(backtrace+i));
		//printk("        [get_kernel_symbol]size = %d , %s\n",strlen(symbol),symbol);
		*(kernel_symbol+i) = (unsigned int)get_kernelString_from_hashTable(symbol,strlen(symbol));
		//printk("	[get_kernel_symbol]store string at : 0x[%x]\n",(kernel_symbol+i));
	}
}
unsigned int get_kernel_backtrace(unsigned long *backtrace)
{
	unsigned long stack_entries[BACKTRACE_LEVEL];
	unsigned int i = 0;
	char tmp[KSYM_SYMBOL_LEN];
	struct stack_trace trace = {
		.nr_entries = 0,
		.entries = &stack_entries[0],
		.max_entries = BACKTRACE_LEVEL,
		.skip = 3 
	};
	save_stack_trace(&trace);
	//printk("	[get_kernel_backtrace] backtrace num: [%d]\n",trace.nr_entries);
	if(trace.nr_entries > 0)
	{
		for(i= 0 ; i < trace.nr_entries; i++)
		{
			//printk("bactrace[%d] : %x  ",i, trace.entries[i]);
			sprint_symbol(tmp,trace.entries[i]);
			//printk("%s\n",tmp);
		}
		memcpy(backtrace,(unsigned long *)trace.entries,sizeof(unsigned int)*trace.nr_entries);
	}
	return trace.nr_entries; 
}
void insert_node_to_list(void **list,unsigned int *node)
{
	//printk("list is %x node is %x\n",list,node);
	//printk("*list is %x *node is %x\n",*list,*node);
	if(*list != NULL )
	{
		*node = (unsigned int)*list;
	}
	else
	{
		*node = 0;
	}
	*list = node;
}

void *find_node_in_list(pid_t pid, unsigned int client_address,unsigned int data,unsigned int **previous_node, unsigned int *list,unsigned int search_type,unsigned int node_type)
{
	unsigned int *prev_node = NULL;
	unsigned int *current_node = list;
	ion_record_basic_info_t *record_ID_tmp;
	struct ion_process_record *process_tmp = NULL;

	while(current_node != NULL)
	{
		if((search_type == SEARCH_PROCESS_PID) && (node_type == LIST_PROCESS)&& (client_address == 0))
		{
			process_tmp = (struct ion_process_record *)(current_node);	
			printk("            [find_node_in_list]curent_node is %x process_tmp->pid is %d process_tmp->count is %d\n",(unsigned int)current_node,process_tmp->pid,process_tmp->count);	
			if(process_tmp->pid == pid)
			{
				if(process_tmp->count ==1)
				{
					*previous_node = prev_node;
					return current_node;
				}
				else
				{
					process_tmp->count--;
					return NULL;
				}
			}
		}
		else if(search_type < SEARCH_PROCESS_PID)
		{
			record_ID_tmp =(ion_record_basic_info_t *)(current_node+1);
			printk("            [find_node_in_list]current_node is %x record_ID_tmp %x record_ID_tmp->pid %d record_ID_tmp->client_address %x\n",(unsigned int)current_node,(unsigned int)record_ID_tmp,record_ID_tmp->recordID.pid,record_ID_tmp->recordID.client_address);
			if((search_type == SEARCH_PID_CLIENT)
			&&(record_ID_tmp->recordID.pid == pid)
			&&(record_ID_tmp->recordID.client_address  == client_address))//FIXME client may use the same buffer twice?
			{
				*previous_node =  prev_node;
				return current_node;
			}
			else if((search_type == SEARCH_PID)&&(record_ID_tmp->recordID.pid == pid)) 
			{
				if(node_type == NODE_FD)
				{
					struct ion_fd_usage_record *tmp = (struct ion_fd_usage_record *)current_node;
					if(tmp->fd == data)
					{
						*previous_node =  prev_node;
                       		 		return current_node;
					}
				}
				else if(node_type == NODE_MMAP)
				{
					struct ion_address_usage_record *tmp = (struct ion_address_usage_record *)current_node;
					if(tmp->mapping_address == data)
					{
						 *previous_node =  prev_node;
		               		        return current_node;
					}	
				}
                	}
		}
		else
		{
			printk("            [find_node_in_list]Error!!!\n");
		}
		prev_node = current_node;
		current_node = (unsigned int *)*(current_node);
	}
	printk("              [find_node_in_list]can't find node in list. search_type %d node_type %d pid %d client_address %x\n",search_type,node_type,pid,(unsigned int)client_address);
	return NULL;
}
void *move_node_to_freelist(pid_t pid,unsigned int client_address,unsigned int data,unsigned int **from, unsigned int **to,unsigned int search_type,unsigned int node_type)
{
	unsigned int *previous_node = NULL;
	unsigned int *found_node;
	printk("          [move_node_to_freelist]pid %d client_address %x from %x to %x search_type %d node_type %d\n",(int)pid,(unsigned int)client_address,(unsigned int )*from,(unsigned int )*to,search_type,node_type);
	found_node = find_node_in_list(pid,client_address,data,(unsigned int **)&previous_node,(unsigned int *)*from,search_type,node_type);
	printk("          [move_node_to_freelist] found_node %x previous_node is %x *to %x\n",(unsigned int)found_node,(unsigned int)previous_node,(unsigned int)*to);
	if(found_node != NULL)
	{
		if(previous_node == NULL)
		{
			*from = (unsigned int *)*found_node;
		}
		else
		{
			*previous_node = *(found_node);
		}
		if(*to != NULL)
		{
			*found_node = (unsigned int)*to;
		}
		else
		{
			*found_node = 0;
		}
		*to = found_node;
		printk("          [move_node_to_free_list]from list is %x to list is %x found_node is %x\n",(unsigned int)*from,(unsigned int)*to,(unsigned int)found_node);
		return found_node;	
	}
	else
	{
		printk("          [move_node_to_freelist]can't found node in list %x: node info pid %d client address %x\n",(unsigned int)from,pid,(unsigned int)client_address);
		return NULL;
	}
}

void * allocate_record(unsigned int type)
{
	switch(type)
	{
		case LIST_BUFFER:
                {
                        if(!list_buffer_cache_created)
                        {
                                ion_list_buffer_cachep = kmem_cache_create("buffer_record",sizeof(struct ion_buffer_record),0,SLAB_HWCACHE_ALIGN,NULL);
                                list_buffer_cache_created = true;
                        }
                        if(ion_list_buffer_cachep != NULL)
                        {
                                return (void *)kmem_cache_alloc(ion_list_buffer_cachep,GFP_KERNEL);
                        }
                        break;
                }
                case LIST_PROCESS:
                {
                        if(!list_process_cache_created)
                        {
                                ion_list_process_cachep = kmem_cache_create("process_record",sizeof(struct ion_process_record),0,SLAB_HWCACHE_ALIGN,NULL);
                                list_process_cache_created = true;
                        }
                        if(ion_list_process_cachep != NULL)
                        {
                                return (void *)kmem_cache_alloc(ion_list_process_cachep,GFP_KERNEL);
                        }
                        break;
                }


	        case NODE_BUFFER:
		{
			if(!buffer_cache_created)
			{
				ion_buffer_usage_cachep = kmem_cache_create("buffer_usage_record",sizeof(ion_buffer_usage_record_t),0,SLAB_HWCACHE_ALIGN,NULL);
				buffer_cache_created = true;
			}
			if(ion_buffer_usage_cachep != NULL)
			{
				return (void *)kmem_cache_alloc(ion_buffer_usage_cachep,GFP_KERNEL);	
			}
			break;
		}
		case NODE_FD:
		{
			if(!fd_cache_created)
			{
				ion_fd_usage_cachep = kmem_cache_create("fd_record",sizeof(ion_fd_usage_record_t),0,SLAB_HWCACHE_ALIGN,NULL);	
				fd_cache_created = true;
			}
			if(ion_fd_usage_cachep != NULL)
			{
				return (void *)kmem_cache_alloc(ion_fd_usage_cachep,GFP_KERNEL);
			}
			break;
		}
		case NODE_CLIENT:
		{
			if(!client_cache_created)
			{
				ion_client_usage_cachep = kmem_cache_create("client_record",sizeof(ion_client_usage_record_t),0,SLAB_HWCACHE_ALIGN,NULL);
				client_cache_created = true;
			}
			if(ion_client_usage_cachep != NULL)
			{
				void *tmp= NULL;
				tmp = (void *)kmem_cache_alloc(ion_client_usage_cachep,GFP_KERNEL);
				//printk("allocate ion_client_usage_record_t (%d), tmp=%x tmp size = %d\n",sizeof(ion_client_usage_record_t),tmp,sizeof(tmp));
				return tmp;	
			}
			break;
		}
		case NODE_MMAP:
		{
			if(!address_cache_created)
			{
				ion_address_usage_cachep = kmem_cache_create("address_record",sizeof(ion_address_usage_record_t),0,SLAB_HWCACHE_ALIGN,NULL);
				address_cache_created = true;
			}
			if(ion_address_usage_cachep != NULL)
			{
				return (void *)kmem_cache_alloc(ion_address_usage_cachep,GFP_KERNEL);
			}
			break;
		}
		case NODE_MAX:
		default:
		{
			printk("[ERROR!!]allocate_record wrong type %d",type);
			break;
		} 

	}
	return NULL;
}
/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL
static inline unsigned int hash_32(unsigned int val, unsigned int bits)
{
        /* On some cpus multiply is faster, on others gcc will do shifts */
        unsigned int hash = val * GOLDEN_RATIO_PRIME_32;

        /* High bits are more random, so use them. */
        return hash >> (32 - bits);
}

static unsigned int RSHash(char* str, unsigned int len) 
{ 
    unsigned int b = 378551; 
    unsigned int a = 63689; 
    unsigned int hash = 0; 
    unsigned int i = 0; 

    for(i = 0; i < len; str++, i++) 
    { 
        hash = hash * a + (*str); 
        a    = a * b; 
    } 

    return hash;
}
static uint32_t get_hash(void* object, size_t numEntries)
{
    unsigned int *backtrace = NULL;
    unsigned int hash = 0;
    size_t i;
    backtrace = (unsigned int *)object;
    if (backtrace == NULL) return 0;
    for (i = 0 ; i < numEntries ; i++) {
        hash = (hash * 33) + (*(backtrace+i) >> 2);
    }
    return hash;
}
static uint32_t get_mapping_hash(struct mapping* object, size_t numEntries)
{
    unsigned int *mapping_address = NULL;
    unsigned int hash = 0;
    size_t i;
    if (object == NULL) return 0;
    for (i = 0 ; i < numEntries ; i++) {
	mapping_address = (unsigned int *)(object+i);
        hash = (hash * 33) + (*(mapping_address) >> 2);
    }
    return hash;
}

static ObjectEntry* find_entry(ObjectTable* table, unsigned int slot,void* object,unsigned int  numEntries)
{
    ObjectEntry* entry = table->slots[slot];
    while (entry != NULL) {
        if ( entry->numEntries == numEntries &&
                !memcmp(object, entry->object, numEntries * sizeof(unsigned int))) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static ObjectEntry* find_mapping_entry(ObjectTable* table, unsigned int slot,void* object,unsigned int  numEntries)
{
    ObjectEntry* entry = table->slots[slot];
    while (entry != NULL) {
        if ( entry->numEntries == numEntries &&
                !memcmp(object, entry->object, numEntries * sizeof(struct mapping))) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

static ObjectEntry* find_buffer_entry(ObjectTable* table, unsigned int slot,struct ion_buffer *object)
{
    ObjectEntry* entry = table->slots[slot];
    while (entry != NULL) {
	struct ion_buffer *tmp_buffer =(struct ion_buffer *)entry->object;
        if ( (tmp_buffer->size == object->size)&&(tmp_buffer->heap == object->heap) &&(entry->object == (void *)object))
        {
		if(object->heap->type != ION_HEAP_TYPE_CARVEOUT)
                {
                   if(tmp_buffer->priv_virt ==  object->priv_virt)
		   {
			return entry;	
		   }
                }
                else
                {
                   if(tmp_buffer->priv_phys ==  object->priv_phys)
		   {
			return entry;
		   }
                }
        }
        entry = entry->next;
    }
    return NULL;
}

static StringEntry* find_string_entry(StringTable* table, unsigned int slot,char *string_name,unsigned int string_len)
{
    StringEntry* entry = table->slots[slot];
    while (entry != NULL) {
        if ( entry->string_len == string_len &&
                !memcmp(string_name, entry->name, string_len)) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}
char *get_userString_from_hashTable(char *string_name,unsigned int len)
{
	return get_string(string_name,len,&gMappingStringTable);
}
char *get_kernelString_from_hashTable(char *string_name,unsigned int len)
{
	return get_string(string_name,len+1,&gKernelPathTable); //add 1 for '\0'
}
//get string form hash table or create new hash node in hash table
char *get_string(char *string_name,unsigned int len,StringTable *table)
{
    unsigned int hash;
    unsigned int slot;
    StringEntry *entry = NULL;
    hash = RSHash(string_name,len);
    slot = hash % OBJECT_TABLE_SIZE;
    entry = find_string_entry(table,slot,(void *)string_name,len);
    if(entry != NULL)
    {
	//printk("	[get_string] find string in string hash table : addres 0x[%x]%s\n",entry->name,entry->name,entry->string_len);
	return entry->name;
    }
    else
    {
	//printk("	[get_string]can't get string in string hash table \n");
	entry = kmalloc(sizeof(StringEntry),GFP_KERNEL);
	entry->name = kmalloc(len,GFP_KERNEL); 
	memcpy(entry->name,string_name,len);
    	entry->slot = slot;
	entry->string_len = len;
	entry->reference = 1;
        entry->prev = NULL;
        entry->next =  table->slots[slot];
        table->slots[slot] = entry;
        if(entry->next != NULL)
       	{ 
        	entry->next->prev = entry;
        }
        table->count++;
	//printk("        [get_string]create new node in string hash table: address 0x[%x]%s size %d\n",entry->name,entry->name,len);
	return entry->name;
    }
}
//get record from hash table or create new node from slab allocator
void *get_record(unsigned int type, ion_sys_record_t *param)
{
	ion_sys_record_t *tmp = param;
	ObjectEntry *entry = NULL;
	unsigned int hash;
	unsigned int slot;
	if(tmp != NULL)
	{	
		switch(type)
		{
			case HASH_NODE_CLIENT:
			{
				break;
			}
			case HASH_NODE_BUFFER:
			{
				hash = (unsigned int)hash_32(((unsigned int)param->buffer+param->buffer->size+param->buffer->heap->type), 16);
				slot = hash % OBJECT_TABLE_SIZE;
				entry = find_buffer_entry(&gBufferTable,slot ,param->buffer);
				if(entry != NULL)
				{
					//printk("        [get_record][BUFFER]find the same entry\n ");
					entry->reference++;
				}
				else
				{
					//printk("        [get_record][BUFFER] can't find entry in hash table and create new entry\n");
					entry = kmalloc(sizeof(ObjectEntry)+sizeof(struct ion_buffer),GFP_KERNEL);
                                        entry->reference = 1;
                                        entry->prev = NULL;
                                        entry->slot = slot;
                                        entry->next = gBufferTable.slots[slot];
                                        entry->numEntries = (unsigned int)param->buffer; 
                                        memcpy(entry->object,param->buffer, sizeof(struct ion_buffer));
                                        gBufferTable.slots[slot] = entry;
                                        if(entry->next != NULL)
                                        {
                                                entry->next->prev = entry;
                                        }
                                        gBufferTable.count++;
				}
				
				return entry->object;
			}
			case HASH_NODE_USER_BACKTRACE:
			{
				hash = get_hash(param->backtrace,param->backtrace_num);
				slot = hash % OBJECT_TABLE_SIZE;
				entry = find_entry(&gUserBtTable,slot,(void *)param->backtrace,param->backtrace_num);
				if(entry != NULL)
				{
					//printk("        [get_record][USER_BACKTRACE]find the same entry entry 0x%x entry num %d\n",entry->object,entry->numEntries);

					entry->reference++;
				}
				else
				{
					entry = kmalloc(sizeof(ObjectEntry)+(param->backtrace_num * sizeof(unsigned int)),GFP_KERNEL);
					entry->reference = 1;
					entry->prev = NULL;
					entry->slot = slot;
					entry->next = gUserBtTable.slots[slot];
					entry->numEntries = param->backtrace_num;

					memcpy(entry->object,&(param->backtrace[0]),entry->numEntries * sizeof(unsigned int));
					gUserBtTable.slots[slot] = entry;
					if(entry->next != NULL)
					{
						entry->next->prev = entry;
					}
					gUserBtTable.count++;
					// printk("        [get_record][USER_BACKTRACE]create new entry>object  0x%x entry num %d souce %x source2 %x\n",entry->object,entry->numEntries,&param->backtrace[0],&(param->backtrace[0]));

				}
				return entry;
			}
			case HASH_NODE_KERNEL_BACKTRACE:
			{
                                hash = get_hash(param->backtrace,param->backtrace_num);
                                slot = hash % OBJECT_TABLE_SIZE;
                                entry = find_entry(&gKernelBtTable,slot,(void *)param->backtrace,param->backtrace_num);
                                if(entry != NULL)
                                {
                                        //printk("        [get_record][KERNEL_BACKTRACE]find the same entry entry 0x%x entry num %d\n",entry->object,entry->numEntries);
                                        entry->reference++;
                                }
                                else
                                {
                                        entry = kmalloc(sizeof(ObjectEntry)+(param->backtrace_num * sizeof(unsigned int)),GFP_KERNEL);
                                        entry->reference = 1;
                                        entry->prev = NULL;
                                        entry->slot = slot;
                                        entry->next = gKernelBtTable.slots[slot];
                                        entry->numEntries = param->backtrace_num;
                                        memcpy(entry->object,param->backtrace,entry->numEntries * sizeof(unsigned int));
                                        gKernelBtTable.slots[slot] = entry;
                                        if(entry->next != NULL)
                                        {
                                                entry->next->prev = entry;
                                        }
                                        gKernelBtTable.count++;
					//printk("        [get_record][KERNEL_BACKTRACE]create new entry>object  0x%x entry num %d\n",entry->object,entry->numEntries);
                                }
                                return entry;
			}
			case HASH_NODE_USER_MAPPING:
			{
                                hash = get_mapping_hash(&(param->mapping_record[0]),param->backtrace_num);
                                slot = hash % OBJECT_TABLE_SIZE;
                                entry = find_mapping_entry(&gUserMappingTable,slot,(void *)&(param->mapping_record[0]),param->backtrace_num);
                                if(entry != NULL)
                                {
                                        //printk("        [get_record][USER_MAPPING_INFO]find the same entry entry 0x%x entry num %d\n",entry->object,entry->numEntries);
                                        entry->reference++;
                                }
                                else
                                {
                                        entry = kmalloc(sizeof(ObjectEntry)+(param->backtrace_num * sizeof(struct mapping)),GFP_KERNEL);
                                        entry->reference = 1;
                                        entry->prev = NULL;
                                        entry->slot = slot;
                                        entry->next = gUserMappingTable.slots[slot];
                                        entry->numEntries = param->backtrace_num;
                                        memcpy(entry->object,&(param->mapping_record[0]),entry->numEntries * sizeof(struct mapping));
                                        gUserMappingTable.slots[slot] = entry;
                                        if(entry->next != NULL)
                                        {
                                                entry->next->prev = entry;
                                        }
                                        gUserMappingTable.count++;
					//printk("        [get_record][USER_MAPPING]create new entry>object  0x%x entry num %d source %x source2 %x\n",entry->object,entry->numEntries,&param->backtrace[0],&(param->backtrace[0]));
                                }
                                return entry;

			}
			case HASH_NODE_KERNEL_SYMBOL:
			{
				hash = get_hash(param->kernel_symbol,param->backtrace_num);
                                slot = hash % OBJECT_TABLE_SIZE;
                                entry = find_entry(&gKernelSymbolTable,slot,(void *)param->kernel_symbol,param->backtrace_num);
                                if(entry != NULL)
                                {
                                        //printk("        [get_record][KERNEL_SYMBOL]find the same entry entry 0x%x entry num %d\n",entry->object,entry->numEntries);
                                        entry->reference++;
                                }
                                else
                                {
					//unsigned int *temp = NULL;
                                        entry = kmalloc(sizeof(ObjectEntry)+(param->backtrace_num * sizeof(unsigned int)),GFP_KERNEL);
                                        entry->reference = 1;
                                        entry->prev = NULL;
                                        entry->slot = slot;
                                        entry->next = gKernelSymbolTable.slots[slot];
                                        entry->numEntries = param->backtrace_num;
                                        memcpy(entry->object,param->kernel_symbol,entry->numEntries * sizeof(unsigned int));
                                        gKernelSymbolTable.slots[slot] = entry;
					//tmp = (unsigned int *)entry->object;

                                        if(entry->next != NULL)
                                        {
                                                entry->next->prev = entry;
                                        }
                                        gKernelSymbolTable.count++;
                                }
                                
                                return entry;

				break;
			}

			case HASH_NODE_MAX:
			default:
			{
				printk("        [get_record][ERROR] get_record error type %d",type);
				break;
			}
		}
	}
	return NULL;
}
void *create_new_record_into_process(ion_sys_record_t *record_param, struct ion_process_record *process_record,unsigned int node_type,unsigned int from_kernel)
{
	unsigned int *new_node = NULL;
        struct ion_client *tmp = NULL;
	new_node = (void *)allocate_record(node_type);
	printk("           [%d][%d][create_new_record_into_process(%d)] process_record 0x%x node_type %d new node %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_record,node_type,(unsigned int)new_node);
	
	if(new_node != NULL)
	{
		 struct ion_record_basic_info *tracking_info = NULL;
	         tmp = record_param->client;
            	 tracking_info = (struct ion_record_basic_info *)(new_node+1);

            	//assign data into new record
            	tracking_info->recordID.pid = record_param->pid;
            	tracking_info->recordID.group_pid = (pid_t)tmp->pid;
            	tracking_info->recordID.client_address = (unsigned int)record_param->client;
            	tracking_info->recordID.client = record_param->client; //FIXME it should be stored in hash table
            	tracking_info->recordID.process_record = process_record;
            	tracking_info->from_kernel = from_kernel;
            	tracking_info->allocate_backtrace = NULL;
            	tracking_info->allocate_map = NULL;
            	tracking_info->release_backtrace = NULL;
            	tracking_info->release_map = NULL;
            	tracking_info->release_backtrace_type = BACKTRACE_MAX;
		tracking_info->record_type = node_type;

            	if(!from_kernel)
            	{
                	tracking_info->allocate_backtrace_type = USER_BACKTRACE;
                	tracking_info->allocate_backtrace = (unsigned int *)get_record(HASH_NODE_USER_BACKTRACE,record_param);
               		tracking_info->allocate_map = (unsigned int *)get_record(HASH_NODE_USER_MAPPING,record_param);
            	}
            	else
            	{
                	tracking_info->allocate_backtrace_type = KERNEL_BACKTRACE;
                	tracking_info->allocate_backtrace = (unsigned int *)get_record(HASH_NODE_KERNEL_BACKTRACE,record_param);
               		tracking_info->allocate_map = (unsigned int *)get_record(HASH_NODE_KERNEL_SYMBOL,record_param);
            	}

		switch(node_type)
		{
			case NODE_MMAP:
			{
				struct ion_address_usage_record *tmp = (struct ion_address_usage_record *)new_node;
	                        if(from_kernel)
	     	                {
	                                tmp->address_type = record_param->address_type;
       		                }
                        	else
                        	{
                               		tmp->address_type = ADDRESS_USER_VIRTUAL;
                        	}
                        	tmp->fd = record_param->fd;
                        	tmp->mapping_address = record_param->address;
                        	tmp->size = record_param->length;
                        
				//add new node into allocate list
                        	mutex_lock(&process_record->ion_address_usage_mutex);
                        	insert_node_to_list((void **)&process_record->address_using_list,(unsigned int *)new_node);
                        	mutex_unlock(&process_record->ion_address_usage_mutex);

				break;
			}
			case NODE_FD:
			{
				struct ion_fd_usage_record *tmp = (struct ion_fd_usage_record *)new_node;
	                        tmp->fd = record_param->fd;
                        
				//add new node into allocate list
                        	mutex_lock(&process_record->ion_fd_usage_mutex);
                        	insert_node_to_list((void **)&process_record->fd_using_list,(unsigned int *)new_node);
                        	mutex_unlock(&process_record->ion_fd_usage_mutex);
				break;
			}
		}
		return new_node;
	}
	return NULL;
}
void *create_new_record_into_list(ion_sys_record_t *record_param,struct ion_buffer_record *buffer,unsigned int node_type,unsigned int from_kernel)
{
	unsigned int *new_node = NULL;
	struct ion_client *tmp = NULL;
	//assign data into buffer usage record
	new_node = (void *)allocate_record(node_type);
	if(new_node != NULL)
        {
	    struct ion_record_basic_info *tracking_info = NULL; 
	    tmp = record_param->client;	
	    tracking_info = (struct ion_record_basic_info *)(new_node+1);
            //assign data into new record
            tracking_info->recordID.pid = record_param->pid;
            tracking_info->recordID.group_pid = (pid_t)tmp->pid;
            tracking_info->recordID.client_address = (unsigned int)record_param->client; 
	    tracking_info->recordID.client = record_param->client; //FIXME it should be stored in hash table
	    tracking_info->recordID.buffer = buffer;
	    tracking_info->from_kernel = from_kernel;
            tracking_info->allocate_backtrace = NULL;
	    tracking_info->allocate_map = NULL;
	    tracking_info->release_backtrace = NULL;
	    tracking_info->release_map = NULL;
	    tracking_info->release_backtrace_type = BACKTRACE_MAX;
	    tracking_info->record_type = node_type;
	
	    if(!from_kernel)
	    {
		tracking_info->allocate_backtrace_type = USER_BACKTRACE;
		tracking_info->allocate_backtrace = (unsigned int *)get_record(HASH_NODE_USER_BACKTRACE,record_param);
            	tracking_info->allocate_map = (unsigned int *)get_record(HASH_NODE_USER_MAPPING,record_param);
            }
	    else
	    {
		tracking_info->allocate_backtrace_type = KERNEL_BACKTRACE;
		tracking_info->allocate_backtrace = (unsigned int *)get_record(HASH_NODE_KERNEL_BACKTRACE,record_param);
		tracking_info->allocate_map = (unsigned int *)get_record(HASH_NODE_KERNEL_SYMBOL,record_param);
	    }
	    switch(node_type)
	    {
		case NODE_BUFFER:
		{
			struct ion_buffer_usage_record *tmp = (struct ion_buffer_usage_record *)new_node;
            		tmp->handle = record_param->handle; //FIXME it should be stored in hash table
            		//printk("[create_new_record_into_list]new_node %x buffer_using_list%x\n",new_node,buffer->buffer_using_list);
            		//add new node into allocate list
            		mutex_lock(&buffer->ion_buffer_usage_mutex);
            		insert_node_to_list((void **)&buffer->buffer_using_list,(unsigned int *)new_node);
            		mutex_unlock(&buffer->ion_buffer_usage_mutex);
			break;
		}

		case NODE_MMAP:
		{
			struct ion_address_usage_record *tmp = (struct ion_address_usage_record *)new_node;
			if(from_kernel)
			{
				tmp->address_type = record_param->address_type;
			}
			else
			{
				tmp->address_type = ADDRESS_USER_VIRTUAL;
			}
                        tmp->fd = record_param->fd;
			tmp->mapping_address = record_param->address;
			tmp->size = record_param->length;
                        //printk("[create_new_record_into_list]new_node %x address__using_list%x\n",new_node,buffer->address_using_list);
                        //add new node into allocate list
                        mutex_lock(&buffer->ion_address_usage_mutex);
                        insert_node_to_list((void **)&buffer->address_using_list,(unsigned int *)new_node); //FIXME
                        mutex_unlock(&buffer->ion_address_usage_mutex);
			break;	
		}
		case NODE_CLIENT:
		{	
			struct ion_client_usage_record *tmp = (struct ion_client_usage_record *)new_node;
			//printk("[ION_FUNCTION_OPEN]new_node %x client_using_list%x\n",new_node,client_using_list);
			if(!from_kernel) {
  				tmp->fd = record_param->fd; 
			}
	                //add new node into allocate list
                        mutex_lock(&client_usage_mutex);
                        insert_node_to_list((void **)&client_using_list,(unsigned int *)new_node);
                        mutex_unlock(&client_usage_mutex);
			break;
		}
	    }
        }
        else
        {
            printk("[create_new_record_into_list] can't get new node type \n");
        }
	return (void *)new_node;
}
unsigned int record_release_backtrace(ion_sys_record_t *record_param,struct ion_record_basic_info *tracking_info,unsigned int from_kernel)
{
	if((record_param != NULL) && (tracking_info != NULL))
	{	
		if(!from_kernel)
		{
			tracking_info->release_backtrace_type = USER_BACKTRACE;
			tracking_info->release_backtrace = (unsigned int *)get_record(HASH_NODE_USER_BACKTRACE,record_param);
			tracking_info->release_map = (unsigned int *)get_record(HASH_NODE_USER_MAPPING,record_param);
		}
		else
		{
			tracking_info->release_backtrace_type = KERNEL_BACKTRACE;
			tracking_info->release_backtrace = (unsigned int *)get_record(HASH_NODE_KERNEL_BACKTRACE,record_param);
			tracking_info->release_map = (unsigned int *)get_record(HASH_NODE_KERNEL_SYMBOL,record_param);
		}
		return 1;
	}
	return 0;
}
int record_ion_info(int from_kernel,ion_sys_record_t *record_param)
{
	printk("  [%d][%d][FUNCTION(%d)_%d][record_ion_info]  backtrace_num %d\n",record_param->pid,record_param->group_id,from_kernel,record_param->action,record_param->backtrace_num);
        {
                //store userspace infomation    
                switch(record_param->action)
                {

                        case ION_FUNCTION_OPEN:
			case ION_FUNCTION_CREATE_CLIENT:
                        {

				struct ion_client_usage_record *new_node = NULL;
				struct ion_process_record *previous_process_record = NULL;
				struct ion_process_record *process_record = NULL;
				printk("    [%d][%d][ION_FUNCTION_OPEN(%d)] \n",record_param->pid,record_param->group_id,from_kernel);

				//init the mutex lock of buffer and client list
				if((client_using_list == NULL) && (client_freed_list == NULL))
				{
					mutex_init(&client_usage_mutex);	
					mutex_init(&buffer_lifecycle_mutex);
				}

				//create client node
				new_node = (struct ion_client_usage_record *)create_new_record_into_list(record_param,NULL,NODE_CLIENT,from_kernel);
				
				//find process record in list
				if(from_kernel == 0)
				{
					if(process_created_list == NULL)
					{
						mutex_init(&process_lifecycle_mutex);
					}
				
					//find process record in list
					mutex_lock(&process_lifecycle_mutex);
                               		process_record = search_process_in_list(record_param->pid, process_created_list, &previous_process_record);

					//If it can't find corresponding process record, we will create new record and insert new record into process_created_list
                               		if(process_record == NULL)
                               		{
						//create buffer node
                               			process_record = (struct ion_process_record *)allocate_record(LIST_PROCESS);
                               			if(process_record != NULL)
                               			{
                                       			//assign data into process_created_list
                                   	    		process_record->pid = record_param->pid;
							process_record->group_id = record_param->group_id;
                                       			mutex_init(&process_record->ion_fd_usage_mutex);
                                       			mutex_init(&process_record->ion_address_usage_mutex);
                                       			process_record->fd_using_list = NULL;
                                       			process_record->fd_freed_list = NULL;
                                       			process_record->address_using_list = NULL;
                                       			process_record->address_freed_list = NULL;
							process_record->count = 1;
                                       			if(process_created_list == NULL)
                                 		 	{
                                       			       	process_record->next = NULL;
                                       			}
                                       		 	else
                                       		 	{
                                       	        		process_record->next = process_created_list;
                                       		 	}
                                       		 	process_created_list = process_record;
                                		}
						printk("    [%d][%d][ION_FUNCTION_OPEN(%d)]create new process record 0x%x in process [%d] count %d process created list %x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_record,(int)record_param->pid,process_record->count,(unsigned int)process_created_list);
					#if 0	
						{
							struct ion_process_record *tmp =NULL;
							tmp = process_created_list;
							for(;tmp != NULL;tmp = tmp->next) 
							{
								printk("PROCESS_DUMP!! process %x process pid %d group id %d count %d\n",tmp,tmp->pid,tmp->group_id,tmp->count);	
							}
						}
					#endif
                                	}
					else
					{
						process_record->count++;
						printk("    [%d][%d][ION_FUNCTION_OPEN(%d)]found process record 0x%x in process [%d] process->count %d process created list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_record,record_param->pid,process_record->count,(unsigned int)process_created_list);

					}
					mutex_unlock(&process_lifecycle_mutex);
				}
		               	 printk("    [%d][%d][ION_FUNCTION_OPEN(%d)] DONE  new_node %x client_using_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node,(unsigned int)client_using_list);
                                break;
                        }
                        case ION_FUNCTION_CLOSE:
			case ION_FUNCTION_DESTROY_CLIENT:
                        {
				ion_client_usage_record_t *found_node = NULL;
				printk("    [%d][%d][ION_FUNCTION_CLOSE(%d)]  client_freed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)client_freed_list);
				//move client node from allocate list into free list
				mutex_lock(&client_usage_mutex);
				found_node = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,0,(unsigned int **)&client_using_list,(unsigned int **)&client_freed_list,SEARCH_PID_CLIENT,NODE_CLIENT);
				mutex_unlock(&client_usage_mutex);
				if(found_node != NULL)
				{
					record_release_backtrace(record_param,&(found_node->tracking_info),from_kernel);	
				}
				else
				{
					printk("    [%d][%d][ION_FUNCTION_CLOSE(%d)]ERROR can't find client: 0x%x in client_using_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)client_using_list);
					break;
				}
				if(from_kernel == 0)
				{
					//move process node to destroyed list if it referece count is 1	
					printk("    [%d][%d][ION_FUNCTION_CLOSE(%d)]  DONE prepare to move process %d into destroyed list %x\n",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_destroyed_list);
					mutex_lock(&process_lifecycle_mutex);
					move_node_to_freelist(record_param->pid,0,0,(unsigned int **)&process_created_list, (unsigned int **)&process_destroyed_list,SEARCH_PROCESS_PID,LIST_PROCESS);
					mutex_unlock(&process_lifecycle_mutex);
					printk("    [%d][%d][ION_FUNCTION_CLOSE(%d)]  DONE process_destroyed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_destroyed_list);	
					#if 0 
					{
                                                        struct ion_process_record *tmp =NULL;
                                                        tmp = process_created_list;
                                                        for(;tmp != NULL;tmp = tmp->next)
                                                        {
                                                                printk("PROCESS_DUMP!! process %x process pid %d group id %d count %d\n",tmp,tmp->pid,tmp->group_id,tmp->count);
                                                        }
                                         }
					#endif

				}
				printk("    [%d][%d][ION_FUNCTION_CLOSE(%d)]  DONE client_freed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)client_freed_list);

				break;
                        }
                        case ION_FUNCTION_ALLOC:
                        case ION_FUNCTION_ALLOC_MM:
                        case ION_FUNCTION_ALLOC_CONT:
                        {
				struct ion_buffer_record *new_buffer_record = NULL;
				struct ion_buffer_usage_record *new_node = NULL;
				printk("    [%d][%d][ION_FUNCTION_ALLOC(%d)_%d]buffer: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,record_param->action,(unsigned int)record_param->buffer);
#if 1
				if(record_param->buffer == NULL)
				{
					printk("    [%d][%d][ION_FUNCTION_ALLOC(%d)_%d]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,record_param->action,(unsigned int)record_param->buffer);
				}
				//create buffer node
				new_buffer_record = (struct ion_buffer_record *)allocate_record(LIST_BUFFER);
				if(new_buffer_record != NULL)
				{
					//assign data into buffer record
					new_buffer_record->buffer_address = record_param->buffer;
        				new_buffer_record->buffer = get_record(HASH_NODE_BUFFER,record_param);
					new_buffer_record->heap_type = record_param->buffer->heap->type;
					if(new_buffer_record->heap_type != ION_HEAP_TYPE_CARVEOUT)
					{
						new_buffer_record->priv_virt =  record_param->buffer->priv_virt;
					}
					else
					{
						new_buffer_record->priv_phys =  record_param->buffer->priv_phys;	
					}
					new_buffer_record->size = record_param->buffer->size;		
					mutex_init(&new_buffer_record->ion_buffer_usage_mutex);
					mutex_init(&new_buffer_record->ion_address_usage_mutex);
					new_buffer_record->buffer_using_list = NULL;
					new_buffer_record->buffer_freed_list = NULL;
					new_buffer_record->address_using_list = NULL;
                                        new_buffer_record->address_freed_list = NULL;
					mutex_lock(&buffer_lifecycle_mutex);
					if(buffer_created_list == NULL)
					{				
						new_buffer_record->next = NULL;
					}
					else
					{
						new_buffer_record->next = buffer_created_list;	
					}
					buffer_created_list = new_buffer_record;
					mutex_unlock(&buffer_lifecycle_mutex);
					
					//assign data into buffer usage record
					new_node = (struct ion_buffer_usage_record *)create_new_record_into_list(record_param,new_buffer_record,NODE_BUFFER,from_kernel);
				}
#endif
				printk("    [%d][%d][ION_FUNCTION_ALLOC(%d)_%d] DONE create buffer_usage_record : 0x%x buffer_created_list: 0x%x new buffer record: 0x%x buffer_using_list: 0x%x\n",record_param->pid,record_param->group_id,from_kernel,record_param->action,(unsigned int)new_node,(unsigned int)buffer_created_list,(unsigned int)new_buffer_record,(unsigned int)new_buffer_record->buffer_using_list);
                                break;
                        }
			case ION_FUNCTION_IMPORT:
			{
				//find buffer record in buffer created list
                                struct ion_buffer_record *buffer_record = NULL;
                                //struct ion_buffer_usage_record *found_node = NULL;
                                struct ion_buffer_record *previous_record = NULL;
				struct ion_buffer_usage_record *new_node = NULL;
				printk("    [%d][%d][ION_FUNCTION_IMPORT(%d)]input buffer: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
#if 1
				if(record_param->buffer == NULL)
                                {
                                        printk("    [%d][%d][ION_FUNCTION_IMPORT(%d)]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
                                }

                                mutex_lock(&buffer_lifecycle_mutex);
                                buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list,&previous_record);
                                mutex_unlock(&buffer_lifecycle_mutex);
                                if(buffer_record == NULL)
                                {
                                        printk("    [%d][%d][ION_FUNCTION_IMPORT(%d)]can't found corresponding buffer 0x%x in buffer created list %x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        break;
                                }
				//assign data into buffer usage record
                                new_node = (struct ion_buffer_usage_record *)create_new_record_into_list(record_param,buffer_record,NODE_BUFFER,from_kernel);
#endif
			 	printk("    [%d][%d][ION_FUNCTION_IMPORT(%d)]DONE new_node %x buffer_using_list%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node,(unsigned int)buffer_record->buffer_using_list);	
				break;
			}

                        case ION_FUNCTION_FREE:
                        {
				//find buffer record in buffer created list 
				struct ion_buffer_record *buffer_record = NULL;
				struct ion_buffer_usage_record *found_node = NULL;
				struct ion_buffer_record *previous_record = NULL;
				printk("    [%d][%d][ION_FUNCTION_FREE(%d)]input buffer: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
#if 1
				if(record_param->buffer == NULL)
                                {
                                        printk("    [%d][%d][ION_FUNCTION_FREE(%d)]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
                                }

				mutex_lock(&buffer_lifecycle_mutex);
				buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list,&previous_record);
				mutex_unlock(&buffer_lifecycle_mutex);
				if(buffer_record == NULL)
				{
					printk("    [%d][%d][ION_FUNCTION_FREE(%d)]can't found corresponding buffer 0x%x in buffer created list %x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
					break;
				}
				
                                //move buffer node from allocate list into free list
                                mutex_lock(&buffer_record->ion_buffer_usage_mutex);
                                found_node = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,0,(unsigned int **)&(buffer_record->buffer_using_list),(unsigned int **)&(buffer_record->buffer_freed_list),SEARCH_PID_CLIENT,NODE_BUFFER);
                                mutex_unlock(&buffer_record->ion_buffer_usage_mutex);
                                if(found_node != NULL)
                                {
					record_release_backtrace(record_param,&(found_node->tracking_info),from_kernel);	
                                }
				else
				{
					printk("    [%d][%d][ION_FUNCTION_FREE(%d)]can't found corresponding buffer usage record client 0x%x in buffer using list 0x%x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)buffer_record->buffer_using_list);
                                        break;
					
				}
			
				//move buffer structure from inusing buffer list to buffer free list
				if(buffer_record->buffer_using_list == NULL)
				{
					mutex_lock(&buffer_lifecycle_mutex);
					if(previous_record != NULL)
					{
						previous_record->next = buffer_record->next;
					}
					else
					{
						buffer_created_list = buffer_record->next;
					}
					buffer_record->next = buffer_destroyed_list;
					buffer_destroyed_list = buffer_record;
					mutex_unlock(&buffer_lifecycle_mutex);	
					printk("    [%d][%d][ION_FUNCTION_FREE(%d)]move buffer record %xfrom buffer_created_list %x to buffer_destroyed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)buffer_record,(unsigned int)buffer_created_list,(unsigned int)buffer_destroyed_list);

				}
				
				//count total buffer free list if buffer free list is full. remove olderest buffer record 
				destroyed_buffer_count++; //FIXME waiting for real case 
#endif
                               	printk("    [%d][%d][ION_FUNCTION_FREE(%d)] DONE buffer_using_list %x buffer_freed_list %x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)buffer_record->buffer_using_list,(unsigned int)buffer_record->buffer_freed_list);
 
				break;
                        }
                        case ION_FUNCTION_MMAP:
                        {
				//find buffer record in buffer created list
                                struct ion_buffer_record *buffer_record = NULL;
                                struct ion_buffer_record *previous_record = NULL;
				struct ion_process_record *process_record = NULL;
				struct ion_process_record *previous_process_record = NULL;
				struct ion_address_usage_record *new_node = NULL;
				struct ion_address_usage_record *new_node2 = NULL;
				printk("    [%d][%d][ION_FUNCTION_MMAP(%d)]input buffer: 0x%x mapping_address %x length %d\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,record_param->address,record_param->length);

				if((record_param->buffer == NULL) && (record_param->fd != 0) && !from_kernel)
				{
					struct dma_buf *dmabuf;
					dmabuf = dma_buf_get(record_param->fd);
 					if(dmabuf->priv != NULL);
					{
						record_param->buffer = dmabuf->priv;	
					}
					printk("    [%d][%d][ION_FUNCTION_MMAP(%d)]get buffer from fd %d input buffer: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,record_param->fd,(unsigned int)record_param->buffer);
				}
				if(record_param->buffer == NULL)
                                {
                                        printk("    [%d][%d][ION_FUNCTION_MMAP(%d)]ERROR!!! buffer: 0x%x is NULL\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer);
                                        break;
                                }
				if(from_kernel)
				{	
					//find corresponding buffer in created buffer list
                                	mutex_lock(&buffer_lifecycle_mutex);
                                	buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list,&previous_record);
                           		mutex_unlock(&buffer_lifecycle_mutex);
                   			if(buffer_record == NULL)
                                	{
                                       		printk("    [%d][%d][ION_FUNCTION_MMAP(%d)]ERROR !!! can't found corresponding buffer 0x%x in buffer created list %x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        	break;
                                	}
					//assign data into mmap record
                                	new_node = (struct ion_address_usage_record *)create_new_record_into_list(record_param,buffer_record,NODE_MMAP,from_kernel);
					printk("    [%d][%d][ION_FUNCTION_MMAP(%d)] DONE new node:  0x%x in buffer record address mapping 0x%x size %d address using list: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node,new_node->mapping_address,new_node->size,(unsigned int)buffer_record->address_using_list);
				}
				else
				{	
					//find corresponding process in created process list
					mutex_lock(&process_lifecycle_mutex);
					process_record = search_process_in_list(record_param->pid, process_created_list, &previous_process_record);	
					mutex_unlock(&process_lifecycle_mutex);	
 					if(process_record == NULL)
               	                	{
           	                        	printk("    [%d][%d][ION_FUNCTION_MMAP(%d)]ERROR !!! can't found corresponding process %d in process created list %x",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_created_list);
               	                        	break;
                                	}
					new_node2 = (struct ion_address_usage_record *)create_new_record_into_process(record_param,process_record,NODE_MMAP,from_kernel);
                                	printk("    [%d][%d][ION_FUNCTION_MMAP(%d)] DONE new node 0x%x in process record address mapping 0x%x size %d address using list 0x%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node2,(unsigned int)new_node2->mapping_address,new_node2->size,(unsigned int)process_record->address_using_list);
				}	 
                                break;
                        }
                        case ION_FUNCTION_MUNMAP:
                        {
				//find buffer record in buffer created list
                                struct ion_buffer_record *buffer_record = NULL;
                                struct ion_address_usage_record *found_node = NULL;
				struct ion_address_usage_record *found_node2 = NULL;
                                struct ion_buffer_record *previous_record = NULL;
				struct ion_process_record *process_record = NULL;
				//struct ion_address_uage_record *found_ndoe2 = NULL;
				struct ion_process_record *previous_process_record = NULL;
				printk("    [%d][%d][ION_FUNCTION_MUNMAP(%d)]input buffer: 0x%x address 0x%x size %d\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,record_param->address,record_param->length);
				
				if(from_kernel)
				{	
                    		        mutex_lock(&buffer_lifecycle_mutex);
                       	        	buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list,&previous_record);
                    	        	mutex_unlock(&buffer_lifecycle_mutex);
                    	        	if(buffer_record == NULL)
                                	{
                                        	printk("    [%d][%d][ION_FUNCTION_MUNMAP(%d)]ERROR !!! can't found corresponding buffer 0x%x in buffer created list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        	break;
                                	}
				
					//move buffer node from allocate list into free list
                                	mutex_lock(&buffer_record->ion_address_usage_mutex);
                                	found_node = move_node_to_freelist(record_param->pid,(unsigned int )record_param->client,record_param->address,(unsigned int **)&(buffer_record->address_using_list),(unsigned int **)&(buffer_record->address_freed_list),SEARCH_PID,NODE_MMAP);
                                	mutex_unlock(&buffer_record->ion_address_usage_mutex);
                                	if(found_node != NULL)
                                	{
                               			record_release_backtrace(record_param,&(found_node->tracking_info),from_kernel); 
					}
					else
					{
                                        	printk("    [%d][%d][ION_FUNCTION_MUNMAP(%d)]can't found corresponding buffer usage record client 0x%x in address using list 0x%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)buffer_record->address_using_list);
                                        	break;

                                	}
				}
				else
				{
					mutex_lock(&process_lifecycle_mutex);
                                	process_record  = search_process_in_list(record_param->pid,process_created_list,&previous_process_record);
                                	mutex_unlock(&process_lifecycle_mutex);
                                	if(process_record == NULL)
                                	{
                                        	printk("    [%d][%d][ION_FUNCTION_MUNMAP(%d)]ERROR !!! can't found corresponding process pid %d in process created list %x\n",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_created_list);
                                        	break;
                                	}

					//move buffer node from allocate list into free list
                                	mutex_lock(&process_record->ion_address_usage_mutex);
                                	found_node2 = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,record_param->address,(unsigned int **)&(process_record->address_using_list),(unsigned int **)&(process_record->address_freed_list),SEARCH_PID,NODE_MMAP);
                                	mutex_unlock(&process_record->ion_address_usage_mutex);
                                	if(found_node2 != NULL)
                                	{
                               			record_release_backtrace(record_param,&(found_node2->tracking_info),from_kernel); 
					}
                                	else
                                	{
                                        	printk("    [%d][%d][ION_FUNCTION_MUNMAP(%d)]can't found corresponding address usage record process 0x%d in address using list 0x%x\n",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_record->address_using_list);
                                        	break;
                                	}
                               		printk("    [%d][%d][ION_FUNCTION_MUNMAP(%d)] DONE client %x process->address_using_list %x process->address_free_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int )record_param->client,(unsigned int)process_record->address_using_list,(unsigned int)process_record->address_freed_list);
				} 
				break;
                        }
                        case ION_FUNCTION_SHARE:
                        {
				//create fd record 
				//add fd record into allocate list
				//find buffer record in buffer created list
                                struct ion_process_record *process_record = NULL;
                                struct ion_process_record *previous_process_record = NULL;
				struct ion_buffer_record *buffer_record = NULL;
				struct ion_buffer_record *previous_buffer_record = NULL;	
                                struct ion_fd_usage_record *new_node = NULL;
				struct ion_buffer_usage_record *buffer_new_node = NULL;
				printk("    [%d][%d][ION_FUNCTION_SHARE(%d)]input buffer: 0x%x fd %d\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,record_param->fd);
				#if 1
				mutex_lock(&buffer_lifecycle_mutex);
                                buffer_record = search_record_in_list(record_param->buffer,buffer_created_list,&previous_buffer_record);
                                mutex_unlock(&buffer_lifecycle_mutex);
				if(buffer_record == NULL)
                                {
                                        printk("    [%d][%d][ION_FUNCTION_SHARE(%d)]can't found corresponding buffer 0x%x in buffer created list %x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        break;
                                }
				buffer_new_node = (struct ion_buffer_usage_record *)create_new_record_into_list(record_param,buffer_record,NODE_BUFFER,from_kernel);
				printk("    [%d][%d][ION_FUNCTION_SHARE(%d)]DONE new_node %x buffer_using_list%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)buffer_new_node,(unsigned int)buffer_record->buffer_using_list);

                                mutex_lock(&process_lifecycle_mutex);
                                process_record = search_process_in_list(record_param->pid,process_created_list,&previous_process_record);
                                mutex_unlock(&process_lifecycle_mutex);
                                if(process_record == NULL)
                                {
                                        printk("    [%d][%d][ION_FUNCTION_SHARE(%d)]ERROR !!! can't found corresponding process %d in process created list %x",record_param->pid,record_param->group_id,from_kernel,record_param->pid,(unsigned int)process_created_list);
                                        break;
                                }
				#endif
				
                                //assign data into fd record
                                new_node = (struct ion_fd_usage_record *)create_new_record_into_process(record_param,process_record,NODE_FD,from_kernel);
                                printk("    [%d][%d][ION_FUNCTION_SHARE(%d)]done new process node is %x insert node into fd using list %x  \n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)new_node,(unsigned int)process_record->fd_using_list);
                                break;
                        }
                        case ION_FUNCTION_SHARE_CLOSE:
                        {
				//find buffer record in buffer created list
                                struct ion_buffer_record *buffer_record = NULL;
                                struct ion_fd_usage_record *found_node = NULL;
                                struct ion_buffer_record *previous_record = NULL;
				struct ion_buffer_usage_record *buffer_found_node = NULL;
				struct ion_process_record *process_record = NULL;
                                struct ion_process_record *previous_process_record = NULL;
				
				if((record_param->buffer == NULL) && (record_param->fd != 0) && !from_kernel)
                                {
                                        struct dma_buf *dmabuf;
                                        dmabuf = dma_buf_get(record_param->fd);
                                        if(dmabuf->priv != NULL);
                                        {
                                                record_param->buffer = dmabuf->priv;
                                        }
                                        printk("    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]get buffer from fd %d input buffer: 0x%x \n",record_param->pid,record_param->group_id,from_kernel,record_param->fd,(unsigned int)record_param->buffer);
                                }

				printk("    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]input buffer: 0x%x fd %d\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,record_param->fd);
				#if 1
                                mutex_lock(&process_lifecycle_mutex);
                                process_record  = search_process_in_list(record_param->pid,process_created_list,&previous_process_record);
                                mutex_unlock(&process_lifecycle_mutex);
                                if(process_record == NULL)
                                {
                                        printk("    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]ERROR !!!can't found corresponding process %d in process created list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->pid,(unsigned int)buffer_created_list);
                                        break;
                                }
				 //move fd node from allocate list into free list
                                mutex_lock(&process_record->ion_fd_usage_mutex);
                                found_node = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,record_param->fd,(unsigned int **)&(process_record->fd_using_list),(unsigned int **)&(process_record->fd_freed_list),SEARCH_PID,NODE_FD);
                                mutex_unlock(&process_record->ion_fd_usage_mutex);
                                if(found_node != NULL)
                                {
                               		record_release_backtrace(record_param,&(found_node->tracking_info),from_kernel); 
				}
				else
                                {
                                        printk("    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]can't found corresponding process  record client 0x%x in fd using list 0x%x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)process_record->fd_using_list);
                                        break;

                                }
				 //move fd record to free list
                                printk("    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)] client %x fd_using_list %x fd_freed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)process_record->fd_using_list,(unsigned int)process_record->fd_freed_list);

				//============================================================================================================================================
				 //move buffer node from allocate list into free list
				mutex_lock(&buffer_lifecycle_mutex);
                                buffer_record  = search_record_in_list(record_param->buffer,buffer_created_list,&previous_record);
                                mutex_unlock(&buffer_lifecycle_mutex);
                                if(buffer_record == NULL)
                                {
                                	printk("    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]ERROR !!! can't found corresponding buffer 0x%x in buffer created list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->buffer,(unsigned int)buffer_created_list);
                                        break;
                                } 

                                mutex_lock(&buffer_record->ion_buffer_usage_mutex);
                                buffer_found_node = move_node_to_freelist(record_param->pid,(unsigned int)record_param->client,0,(unsigned int **)&(buffer_record->buffer_using_list),(unsigned int **)&(buffer_record->buffer_freed_list),SEARCH_PID_CLIENT,NODE_BUFFER);
                                mutex_unlock(&buffer_record->ion_buffer_usage_mutex);
                                if(buffer_found_node != NULL)
                                {
                               		record_release_backtrace(record_param,&(buffer_found_node->tracking_info),from_kernel); 
				}
                                else
                                {
                                        printk("    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]can't found corresponding buffer usage record client 0x%x in buffer using list 0x%x",record_param->pid,record_param->group_id,from_kernel,(unsigned int)record_param->client,(unsigned int)buffer_record->buffer_using_list);
                                        break;

                                }

                                //move buffer structure from inusing buffer list to buffer free list
                                if(buffer_record->buffer_using_list == NULL)
                                {
                                        mutex_lock(&buffer_lifecycle_mutex);
                                        if(previous_record != NULL)
                                        {
                                                previous_record->next = buffer_record->next;
                                        }
                                        else
                                        {
                                                buffer_created_list = buffer_record->next;
                                        }
                                        buffer_record->next = buffer_destroyed_list;
                                        buffer_destroyed_list = buffer_record;
                                        mutex_unlock(&buffer_lifecycle_mutex);
                                        printk("    [%d][%d][ION_FUNCTION_SHARE_CLOSE(%d)]move buffer record %xfrom buffer_created_list %x to buffer_destroyed_list %x\n",record_param->pid,record_param->group_id,from_kernel,(unsigned int)process_record,(unsigned int)process_created_list,(unsigned int)process_destroyed_list);

                                }
				#endif
                                break;
                        }
                        default:
                                printk("[ERROR]record_ion_info error action\n");
                }

        }
	printk("  [%d][%d][FUNCTION(%d)_%d][record_ion_info] DONE\n",record_param->pid,record_param->group_id,from_kernel,record_param->action);
	return 1;
}

