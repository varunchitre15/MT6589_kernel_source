#include <linux/aee.h>
#include <linux/atomic.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/io.h>


#define RC_CPU_COUNT NR_CPUS
#define RAM_CONSOLE_HEADER_STR_LEN 1024

static int mtk_cpu_num = 0;


struct ram_console_buffer {
	uint32_t    sig;
	uint32_t    start;
	uint32_t    size;

	uint8_t     hw_status;
	uint8_t	    fiq_step;
	uint8_t     __pad1;
	uint8_t     __pad2;
	uint8_t     __pad3;

	uint32_t    bin_log_count;

	uint32_t    last_irq_enter[RC_CPU_COUNT];
	uint64_t    jiffies_last_irq_enter[RC_CPU_COUNT];

	uint32_t    last_irq_exit[RC_CPU_COUNT];
	uint64_t    jiffies_last_irq_exit[RC_CPU_COUNT];

	uint64_t    jiffies_last_sched[RC_CPU_COUNT];
	char        last_sched_comm[RC_CPU_COUNT][TASK_COMM_LEN];

	uint8_t     hotplug_data1[RC_CPU_COUNT];
	uint8_t     hotplug_data2[RC_CPU_COUNT];

 	uint8_t     data[0];
};

#define RAM_CONSOLE_SIG (0x43474244) /* DBGC */
static int FIQ_log_size = sizeof(struct ram_console_buffer);


static char *ram_console_old_log_init_buffer = NULL;


static struct ram_console_buffer ram_console_old_header;
static char *ram_console_old_log;
static size_t ram_console_old_log_size;

static struct ram_console_buffer *ram_console_buffer;
static size_t ram_console_buffer_size;





static DEFINE_SPINLOCK(ram_console_lock);

static atomic_t rc_in_fiq = ATOMIC_INIT(0);


void aee_rr_rec_fiq_step(u8 i)
{
	if(ram_console_buffer)
	{
		ram_console_buffer->fiq_step = i;
	}
}

void aee_rr_rec_last_irq_enter(int cpu, int irq, u64 j)
{
	if ((ram_console_buffer != NULL) && (cpu >= 0) && (cpu < RC_CPU_COUNT)) {
		ram_console_buffer->last_irq_enter[cpu] = irq;
		ram_console_buffer->jiffies_last_irq_enter[cpu] = j;
	}
	mb();
}

void aee_rr_rec_last_irq_exit(int cpu, int irq, u64 j)
{
	if ((ram_console_buffer != NULL) && (cpu >= 0) && (cpu < RC_CPU_COUNT)) {
		ram_console_buffer->last_irq_exit[cpu] = irq;
		ram_console_buffer->jiffies_last_irq_exit[cpu] = j;
	}
	mb();
}

void aee_rr_rec_last_sched_jiffies(int cpu, u64 j, const char *comm)
{
	if ((ram_console_buffer != NULL) && (cpu >= 0) && (cpu < RC_CPU_COUNT)) {
		ram_console_buffer->jiffies_last_sched[cpu] = j;
		strlcpy(ram_console_buffer->last_sched_comm[cpu], comm, TASK_COMM_LEN);
	}
	mb();
}

void aee_rr_rec_hoplug(int cpu, u8 data1, u8 data2)
{
	if ((ram_console_buffer != NULL) && (cpu >= 0) && (cpu < RC_CPU_COUNT)) {
		ram_console_buffer->hotplug_data1[cpu] = data1;
		ram_console_buffer->hotplug_data2[cpu] = data2;
	}
}

void sram_log_save(const char *msg, int count)
{
	struct ram_console_buffer *buffer = ram_console_buffer;
	int rem;
	
	// count >= buffer_size, full the buffer
	if(count >= ram_console_buffer_size)
	{
		memcpy(buffer->data, msg + (count - ram_console_buffer_size), ram_console_buffer_size);
		buffer->start = 0;
		buffer->size = ram_console_buffer_size;
	}	
	else if(count > (ram_console_buffer_size - buffer->start)) // count > last buffer, full them and fill the head buffer
	{
		rem = ram_console_buffer_size - buffer->start;
		memcpy(buffer->data + buffer->start, msg, rem);
		memcpy(buffer->data, msg + rem, count - rem);
		buffer->start = count - rem;
		buffer->size = ram_console_buffer_size;
	}
	else	// count <=  last buffer, fill in free buffer
	{
		memcpy(buffer->data + buffer->start, msg, count); // count <= last buffer, fill them
		buffer->start += count;
		buffer->size += count;
		if(buffer->start >= ram_console_buffer_size)
		{
			buffer->start = 0;
		}
		if(buffer->size > ram_console_buffer_size)
		{
			buffer->size = ram_console_buffer_size;
		}
	}

}

void aee_sram_fiq_save_bin(const char *msg, size_t len)
{
	int delay = 100;
	char bin_buffer[4];
	struct ram_console_buffer *buffer = ram_console_buffer;	

	if(FIQ_log_size + len > CONFIG_MTK_RAM_CONSOLE_SIZE)
	{
		return;
	}
	
	if(len > 0xffff)
	{
		return;
	}

	if(len%4 !=0)
	{
		len -= len%4;
	}
	
	if(!atomic_read(&rc_in_fiq))
	{
		atomic_set(&rc_in_fiq, 1);
	}

	while ((delay > 0) && (spin_is_locked(&ram_console_lock))) {
		udelay(1);
		delay--;
	}

	// bin buffer flag 00ff
	bin_buffer[0] = 0x00;
	bin_buffer[1] = 0xff;
	// bin buffer size
	bin_buffer[2] = len/255;
	bin_buffer[3] = len%255;	
	
	sram_log_save(bin_buffer, 4);
	sram_log_save(msg, len);	
	FIQ_log_size = FIQ_log_size + len +4;
	buffer->bin_log_count += len;
	

}


void aee_disable_ram_console_write(void)
{

	atomic_set(&rc_in_fiq, 1);
	return;
}

void aee_sram_fiq_log(const char *msg)
{
	unsigned int count = strlen(msg);
	int delay = 100;
	
	if(FIQ_log_size + count > CONFIG_MTK_RAM_CONSOLE_SIZE)
	{
		return;
	}

	if(!atomic_read(&rc_in_fiq))
	{
		atomic_set(&rc_in_fiq, 1);
	}

	while ((delay > 0) && (spin_is_locked(&ram_console_lock))) {
		udelay(1);
		delay--;
	}

	sram_log_save(msg, count);
	FIQ_log_size += count;
}

void
ram_console_write(struct console *console, const char *s, unsigned int count)
{
	unsigned long flags;	

    if(atomic_read(&rc_in_fiq))
	{
        return;
    }

	spin_lock_irqsave(&ram_console_lock, flags);

	sram_log_save(s, count);

	spin_unlock_irqrestore(&ram_console_lock, flags);
}

static struct console ram_console = {
	.name	= "ram",
	.write	= ram_console_write,
	.flags	= CON_PRINTBUFFER | CON_ENABLED | CON_ANYTIME,
	.index	= -1,
};

void ram_console_enable_console(int enabled)
{
	if (enabled)
		ram_console.flags |= CON_ENABLED;
	else
		ram_console.flags &= ~CON_ENABLED;
}

static inline void bin_to_asc(char *buff, uint8_t num)
{
	if(num > 9)
	{
		*buff = num - 10 + 'a';
	}
	else
	{
		*buff = num + '0';
	}
	printk(KERN_ERR"buff %c, num %d.\n", *buff, num);
}

static void __init
ram_console_save_old(struct ram_console_buffer *buffer)
{
	size_t old_log_size = buffer->size;	
	size_t total_size = old_log_size;
	size_t bin_log_size =0;
	
	char *tmp;
	int i,n;
	int length;
	int point = 0;	
	
	if(buffer->bin_log_count == 0)
	{
		ram_console_old_log_init_buffer = kmalloc(total_size, GFP_KERNEL);
		if (ram_console_old_log_init_buffer == NULL)
		{
			printk(KERN_ERR
			       "ram_console: failed to allocate old buffer\n");
			return;
		}
		

		memcpy(&ram_console_old_header, buffer, sizeof(struct ram_console_buffer));

		ram_console_old_log = ram_console_old_log_init_buffer;
		ram_console_old_log_size = total_size;
		
		memcpy(ram_console_old_log_init_buffer,
		       &buffer->data[buffer->start], buffer->size - buffer->start);
		memcpy(ram_console_old_log_init_buffer + buffer->size - buffer->start,
		       &buffer->data[0], buffer->start);	
	}
	else
	{	
		bin_log_size = buffer->bin_log_count * 5 / 4; //bin: 12 34 56 78-->ascill: 78654321z
		
		ram_console_old_log_init_buffer = kmalloc(total_size + bin_log_size, GFP_KERNEL);
		if(ram_console_old_log_init_buffer == NULL)
		{
			printk(KERN_ERR
			       "ram_console: failed to allocate buffer\n");
			return;
		}
		
		tmp= kmalloc(total_size, GFP_KERNEL);
		if(tmp == NULL)
		{
			printk(KERN_ERR
			       "ram_console: failed to allocate tmp buffer\n");
			return;
		}
		
		memcpy(&ram_console_old_header, buffer, sizeof(struct ram_console_buffer));

		ram_console_old_log = ram_console_old_log_init_buffer;
//		ram_console_old_log_size = total_size + bin_log_size;
		
		memcpy(tmp,&buffer->data[buffer->start], buffer->size - buffer->start);
		memcpy(tmp + buffer->size - buffer->start, &buffer->data[0], buffer->start);

		for(i = 0; i < total_size;)
		{
			if((tmp[i] == 0x00) && (tmp[i+1] == 0xff))
			{
				length = tmp[i+2] * 0xff + tmp[i+3];
				i = i + 4;
				for(n = 0; n < length/4; n++)
				{										
					bin_to_asc(&ram_console_old_log_init_buffer[point], (uint8_t)(tmp[i+3] / 16));
					point++;
					bin_to_asc(&ram_console_old_log_init_buffer[point], (uint8_t)(tmp[i+3] % 16));
					point++;					
					bin_to_asc(&ram_console_old_log_init_buffer[point], (uint8_t)(tmp[i+2] / 16));
					point++;
					bin_to_asc(&ram_console_old_log_init_buffer[point], (uint8_t)(tmp[i+2] % 16));
					point++;
					bin_to_asc(&ram_console_old_log_init_buffer[point], (uint8_t)(tmp[i+1] / 16));
					point++;
					bin_to_asc(&ram_console_old_log_init_buffer[point], (uint8_t)(tmp[i+1] % 16));
					point++;					
					bin_to_asc(&ram_console_old_log_init_buffer[point], (uint8_t)(tmp[i] / 16));
					point++;
					bin_to_asc(&ram_console_old_log_init_buffer[point], (uint8_t)(tmp[i] % 16));
					point++;
					ram_console_old_log_init_buffer[point++] = 32;
					i = i + 4;					
				}
			}
			else
			{
				ram_console_old_log_init_buffer[point++] = tmp[i++];
			}
		}
		ram_console_old_log_size = point;
		kfree(tmp);
		
	}	
}

static int __init ram_console_init(struct ram_console_buffer *buffer,
				   size_t buffer_size)
{
	int i;
	
	ram_console_buffer = buffer;
	ram_console_buffer_size =
		buffer_size - sizeof(struct ram_console_buffer);	

	if (buffer->sig == RAM_CONSOLE_SIG) {
		if (buffer->size > ram_console_buffer_size
		    || buffer->start > buffer->size)
			printk(KERN_INFO "ram_console: found existing invalid "
			       "buffer, size %d, start %d\n",
			       buffer->size, buffer->start);
		else {
			printk(KERN_INFO "ram_console: found existing buffer, "
			       "size %d, start %d\n",
			       buffer->size, buffer->start);
			ram_console_save_old(buffer);
		}
	} else {
		printk(KERN_INFO "ram_console: no valid data in buffer "
		       "(sig = 0x%08x)\n", buffer->sig);
	}

	buffer->sig = RAM_CONSOLE_SIG;
	buffer->fiq_step = 0;
	buffer->start = 0;
	buffer->size = 0;
	buffer->hw_status = 0;
	buffer->bin_log_count = 0;

	for (i = 0; i < mtk_cpu_num; i++) {
		buffer->last_irq_enter[i] = 0;
		buffer->jiffies_last_irq_enter[i] = 0;
		buffer->last_irq_exit[i] = 0;
		buffer->jiffies_last_irq_exit[i] = 0;

		buffer->jiffies_last_sched[i] = 0;
		memset(buffer->last_sched_comm[i], i, TASK_COMM_LEN);
	}

	register_console(&ram_console);
	
	return 0;
}


static int __init ram_console_early_init(void)
{
	printk(KERN_ERR "ram_console: start: 0x%x, size: %d\r\n", CONFIG_MTK_RAM_CONSOLE_ADDR, CONFIG_MTK_RAM_CONSOLE_SIZE);
	mtk_cpu_num = num_present_cpus();
	
	return ram_console_init((struct ram_console_buffer *)
		CONFIG_MTK_RAM_CONSOLE_ADDR,
		CONFIG_MTK_RAM_CONSOLE_SIZE);
}


static ssize_t ram_console_read_old(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if(pos >= ram_console_old_log_size)
	{
		return 0;
	}
	
	count = min(len, (size_t)(ram_console_old_log_size - pos));
	if(copy_to_user(buf, ram_console_old_log + pos, count))
	{	
		return -EFAULT;
	}
	
	*offset += count;
	return count;
}

static const struct file_operations ram_console_file_ops = {
	.owner = THIS_MODULE,
	.read = ram_console_read_old,
};

static int __init ram_console_late_init(void)
{
	struct proc_dir_entry *entry;
	struct last_reboot_reason lrr;
	char *ram_console_header_buffer;
	int str_real_len = 0;
	int i = 0;

	printk(KERN_ERR"ram_console_late_init\r\n");
	
	if (ram_console_old_log == NULL)
	{
		printk(KERN_ERR"ram console olg log is null!\n");
		return 0;
	}

	memset(&lrr, 0, sizeof(struct last_reboot_reason));
	lrr.wdt_status = ram_console_old_header.hw_status;
	lrr.fiq_step = ram_console_old_header.fiq_step;

	for(i = 0; i < NR_CPUS; i++)
	{
		lrr.last_irq_enter[i] = ram_console_old_header.last_irq_enter[i];
		lrr.jiffies_last_irq_enter[i] = ram_console_old_header.jiffies_last_irq_enter[i];

		lrr.last_irq_exit[i] = ram_console_old_header.last_irq_exit[i];
		lrr.jiffies_last_irq_exit[i] = ram_console_old_header.jiffies_last_irq_exit[i];

		lrr.jiffies_last_sched[i] = ram_console_old_header.jiffies_last_sched[i];
		strlcpy(lrr.last_sched_comm[i], ram_console_old_header.last_sched_comm[i], TASK_COMM_LEN);

		lrr.hotplug_data1[i] = ram_console_old_header.hotplug_data1[i];
		lrr.hotplug_data2[i] = ram_console_old_header.hotplug_data2[i];
	}

	aee_rr_last(&lrr);

	ram_console_header_buffer = kmalloc(RAM_CONSOLE_HEADER_STR_LEN, GFP_KERNEL);
	if(ram_console_header_buffer == NULL)
	{
		printk(KERN_ERR
		       "ram_console: failed to allocate buffer for header buffer.\n");
		return 0;
	}


	str_real_len = sprintf(ram_console_header_buffer,"ram console header, hw_status: %u, fiq step %u.\n",
	ram_console_old_header.hw_status, ram_console_old_header.fiq_step);

	str_real_len += sprintf(ram_console_header_buffer + str_real_len,"bin log %d.\n", ram_console_old_header.bin_log_count);
	
	ram_console_old_log = kmalloc(ram_console_old_log_size + str_real_len, GFP_KERNEL);
	if(ram_console_old_log == NULL)
	{
		printk(KERN_ERR
		       "ram_console: failed to allocate buffer for old log\n");
		ram_console_old_log_size = 0;
		kfree(ram_console_header_buffer);
		return 0;
	}
	memcpy(ram_console_old_log, ram_console_header_buffer, str_real_len);
	memcpy(ram_console_old_log + str_real_len,
	       ram_console_old_log_init_buffer, ram_console_old_log_size);

	kfree(ram_console_header_buffer);
	kfree(ram_console_old_log_init_buffer);
	entry = create_proc_entry("last_kmsg", S_IFREG | S_IRUGO, NULL);
	if(!entry)
	{
		printk(KERN_ERR "ram_console: failed to create proc entry\n");
		kfree(ram_console_old_log);
		ram_console_old_log = NULL;
		return 0;
	}

	ram_console_old_log_size += str_real_len;
	entry->proc_fops = &ram_console_file_ops;
	entry->size = ram_console_old_log_size;
	return 0;
}


console_initcall(ram_console_early_init);
late_initcall(ram_console_late_init);


