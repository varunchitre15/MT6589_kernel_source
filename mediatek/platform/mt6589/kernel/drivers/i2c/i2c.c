/*
 *
 * (C) Copyright 2011
 * MediaTek <www.mediatek.com>
 * Infinity Chen <infinity.chen@mediatek.com>
 *
 * mt8320 I2C Bus Controller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/wait.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <asm/scatterlist.h>
#include <linux/scatterlist.h>
#include <asm/io.h>
//#include <mach/dma.h>
#include <asm/system.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_gpio.h>
#include <mach/mt_pmic_wrap.h>
#include "mach/memory.h"

#define I2C_CLK_WRAPPER_RATE		36000		/* kHz for wrapper I2C work frequency */
#define I2C_PMIC_WR(addr, data)   pwrap_write((u32)addr, data)
#define I2C_PMIC_RD(addr)         ({ \
		u32 ext_data; \
		(pwrap_read((u32)addr,&ext_data) != 0)?-1:ext_data;})

#if (defined(CONFIG_MT6589_FPGA))
#define	CONFIG_MT_I2C_FPGA_ENABLE
#endif

#if (defined(CONFIG_MT_I2C_FPGA_ENABLE))
#define I2C_CLK_RATE				12000			/* kHz for FPGA I2C work frequency */
#else
#include <mach/mt_clkmgr.h>
#include <mach/upmu_common.h>
extern kal_uint32 mt_get_bus_freq(void);
#endif
/******************************************register operation***********************************/
enum I2C_REGS_OFFSET {
	OFFSET_DATA_PORT	= 0x0,		//0x0
	OFFSET_SLAVE_ADDR	= 0x04,		//0x04
	OFFSET_INTR_MASK	= 0x08,		//0x08
	OFFSET_INTR_STAT	= 0x0C,		//0x0C
	OFFSET_CONTROL		= 0x10,		//0X10
	OFFSET_TRANSFER_LEN = 0x14,		//0X14
	OFFSET_TRANSAC_LEN	= 0x18,		//0X18
	OFFSET_DELAY_LEN	= 0x1C,		//0X1C
	OFFSET_TIMING		= 0x20,		//0X20
	OFFSET_START		= 0x24,		//0X24
	OFFSET_EXT_CONF		= 0x28,
	OFFSET_FIFO_STAT	= 0x30,		//0X30
	OFFSET_FIFO_THRESH	= 0x34,		//0X34
	OFFSET_FIFO_ADDR_CLR= 0x38,		//0X38
	OFFSET_IO_CONFIG	= 0x40,		//0X40
	OFFSET_RSV_DEBUG	= 0x44,		//0X44
	OFFSET_HS			= 0x48,		//0X48
	OFFSET_SOFTRESET	= 0x50,		//0X50
	OFFSET_PATH_DIR		= 0x60,
	OFFSET_DEBUGSTAT	= 0x64,		//0X64
	OFFSET_DEBUGCTRL	= 0x68,		//0x68
};

#define I2C_HS_NACKERR				(1 << 2)
#define I2C_ACKERR					(1 << 1)
#define I2C_TRANSAC_COMP			(1 << 0)

#define I2C_FIFO_SIZE               8

#define MAX_ST_MODE_SPEED           100  /* khz */
#define MAX_FS_MODE_SPEED           400  /* khz */
#define MAX_HS_MODE_SPEED           3400 /* khz */

#define MAX_DMA_TRANS_SIZE          252 /* Max(255) aligned to 4 bytes = 252 */
#define MAX_DMA_TRANS_NUM           256

#define MAX_SAMPLE_CNT_DIV          8
#define MAX_STEP_CNT_DIV            64
#define MAX_HS_STEP_CNT_DIV         8

#define DMA_ADDRESS_HIGH				(0xC0000000)

//#define I2C_DEBUG
volatile U32 I2C_TIMING_REG_BACKUP[7]={0};

#ifdef I2C_DEBUG
#define I2C_BUG_ON(a) BUG_ON(a)
#else
#define I2C_BUG_ON(a)
#endif

enum DMA_REGS_OFFSET {
	OFFSET_INT_FLAG = 0x0,
	OFFSET_INT_EN = 0x04,
	OFFSET_EN = 0x08,
	OFFSET_CON = 0x18,
	OFFSET_TX_MEM_ADDR = 0x1c,
	OFFSET_RX_MEM_ADDR = 0x20,
	OFFSET_TX_LEN = 0x24,
	OFFSET_RX_LEN = 0x28,
};

enum i2c_trans_st_rs {
		I2C_TRANS_STOP = 0,
		I2C_TRANS_REPEATED_START,
};

enum {
     ST_MODE,
     FS_MODE,
     HS_MODE,
 };


enum mt_trans_op {
	I2C_MASTER_NONE = 0,
	I2C_MASTER_WR = 1,
	I2C_MASTER_RD,
	I2C_MASTER_WRRD,
};

//CONTROL
#define I2C_CONTROL_RS          (0x1 << 1)
#define I2C_CONTROL_DMA_EN      (0x1 << 2)
#define I2C_CONTROL_CLK_EXT_EN      (0x1 << 3)
#define I2C_CONTROL_DIR_CHANGE      (0x1 << 4)
#define I2C_CONTROL_ACKERR_DET_EN   (0x1 << 5)
#define I2C_CONTROL_TRANSFER_LEN_CHANGE (0x1 << 6)
#define I2C_CONTROL_WRAPPER          (0x1 << 0)


/***********************************end of register operation****************************************/

/***********************************I2C Param********************************************************/

#define I2C_DRV_NAME				"mt-i2c"
/****************************************************************************************************/

struct mt_trans_data {
	u16 trans_num;
	u16 data_size;
	u16 trans_len;
	u16 trans_auxlen;
};

struct mt_i2c {
	struct i2c_adapter	adap;		/* i2c host adapter */
	struct device		*dev;		/* the device object of i2c host adapter */
	u32					base;		/* i2c base addr */
	u16					id;
	u16					irqnr;		/* i2c interrupt number */
	u16					irq_stat;	/* i2c interrupt status */
	spinlock_t			lock;		/* for mt_i2c struct protection */
	wait_queue_head_t	wait;		/* i2c transfer wait queue */

	atomic_t			trans_err;	/* i2c transfer error */
	atomic_t			trans_comp;	/* i2c transfer completion */
	atomic_t			trans_stop;	/* i2c transfer stop */

	unsigned long		clk;		/* host clock speed in khz */
	unsigned long		sclk;		/* khz */
	int					pdn;		/*clock number*/

	unsigned char		master_code;/* master code in HS mode */
	unsigned char		mode;		/* ST/FS/HS mode */

	enum  i2c_trans_st_rs st_rs;
	bool                dma_en;
	u32                 pdmabase;
	u16                 delay_len;
	enum mt_trans_op op;
	bool                poll_en;
	struct mt_trans_data trans_data;
};

/*this field is only for 3d camera*/
static struct i2c_msg	g_msg[2];
static struct mt_i2c	*g_i2c[2];

//I2C GPIO debug
struct mt_i2c_gpio_t{
	u16 scl;
	u16 sda;
};
static struct mt_i2c_gpio_t mt_i2c_gpio_mode[7]={
	{GPIO119,GPIO118},
	{GPIO215,GPIO214},
	{GPIO217,GPIO216},
	{GPIO68,GPIO67},
	{GPIO267,GPIO268},
	{GPIO269,GPIO270},
	{GPIO271,GPIO272},
};
#include <mach/sync_write.h>
static inline void i2c_writel(struct mt_i2c * i2c, u8 offset, u16 value)
{

	//dev_err(i2c->dev, "before i2c_writel base=%x,offset=%x\n",i2c->base,offset);
	//__raw_writew(value, (i2c->base) + (offset));
	mt65xx_reg_sync_writel(value, (i2c->base) + (offset));
}

static inline u32 i2c_readl(struct mt_i2c * i2c, u8 offset)
{
	return __raw_readl((i2c->base) + (offset));
}
//#define I2C_DEBUG_FS
#ifdef I2C_DEBUG_FS
//#if 1
#define PORT_COUNT 7
#define MESSAGE_COUNT 16
#define I2C_T_DMA 1
#define I2C_T_TRANSFERFLOW 2
#define I2C_T_SPEED 3
/*7 ports,16 types of message*/
u8 i2c_port[ PORT_COUNT ][ MESSAGE_COUNT ];

#define I2C_INFO(i2c, type, format, arg...) do { \
	if ( type < MESSAGE_COUNT && type >= 0 ) { \
		if ( i2c_port[i2c->id][0] != 0 && ( i2c_port[i2c->id][type] != 0 || i2c_port[i2c->id][MESSAGE_COUNT - 1] != 0) ) { \
			dev_alert(i2c->dev, format, ## arg); \
		} \
	} \
} while (0)

static ssize_t show_config(struct device *dev, struct device_attribute *attr, char *buff)
{
	int i = 0;
	int j = 0;
	char *buf = buff;
	for ( i =0; i < PORT_COUNT; i++){
		for ( j=0;j < MESSAGE_COUNT; j++) i2c_port[i][j] += '0';
		strncpy(buf, (char *)i2c_port[i], MESSAGE_COUNT);
		buf += MESSAGE_COUNT;
		*buf = '\n';
		buf++;
		for ( j=0;j < MESSAGE_COUNT; j++) i2c_port[i][j] -= '0';
	}
	return (buf - buff);
}

static ssize_t set_config(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int port,type,status;

	if ( sscanf(buf, "%d %d %d", &port, &type, &status) ) {
		if ( port >= PORT_COUNT || port < 0 || type >= MESSAGE_COUNT || type < 0 ) {
			/*Invalid param*/
			dev_err(dev, "i2c debug system: Parameter overflowed!\n");
		} else {
			if ( status != 0 )
				i2c_port[port][type] = 1;
			else
				i2c_port[port][type] = 0;

			dev_alert(dev, "port:%d type:%d status:%s\ni2c debug system: Parameter accepted!\n", port, type, status?"on":"off");
		}
	} else {
		/*parameter invalid*/
		dev_err(dev, "i2c debug system: Parameter invalid!\n");
	}
	return count;
}

static DEVICE_ATTR(debug, S_IRUGO|S_IWUGO, show_config, set_config);

#else
#define I2C_INFO(mt_i2c, type, format, arg...)
#endif

static void	mt_i2c_clock_enable(struct mt_i2c *i2c)
{
#if (!defined(CONFIG_MT_I2C_FPGA_ENABLE))
	I2C_INFO(i2c, I2C_T_TRANSFERFLOW, "Before clock setting .....\n");
	enable_clock(i2c->pdn, "i2c");
	/*PMIC side clock control*/
	if(i2c->id == 4){
		upmu_set_rg_wrp_i2c0_pdn(0);
	}else if(i2c->id == 5){
		upmu_set_rg_wrp_i2c1_pdn(0);
	}else if(i2c->id == 6){
		upmu_set_rg_wrp_i2c2_pdn(0);
	}
	/*
	dev_err(i2c->dev, "Clock %s\n", (((__raw_readl(0xF0003018)>>26) | (__raw_readl(0xF000301c)&0x1 << 6)) & (1 << i2c->id))?"disable":"enable");
	if(i2c->id >=4)
		dev_err(i2c->dev, "Clock PMIC %s\n", ((I2C_PMIC_RD(0x011A) & 0x7) & (1 << (i2c->id - 4)))?"disable":"enable");
	*/
	I2C_INFO(i2c, I2C_T_TRANSFERFLOW, "clock setting done.....\n");
#endif
	return;
}
static void	mt_i2c_clock_disable(struct mt_i2c *i2c)
{
#if (!defined(CONFIG_MT_I2C_FPGA_ENABLE))
	disable_clock(i2c->pdn, "i2c");
	/*PMIC side clock control*/
	if(i2c->id == 4){
		upmu_set_rg_wrp_i2c0_pdn(1);
	}else if(i2c->id == 5){
		upmu_set_rg_wrp_i2c1_pdn(1);
	}else if(i2c->id == 6){
		upmu_set_rg_wrp_i2c2_pdn(1);
	}
	/*
	dev_err(i2c->dev, "Clock %s\n", (((__raw_readl(0xF0003018)>>26) | (__raw_readl(0xF000301c)&0x1 << 6)) & (1 << i2c->id))?"disable":"enable");
	if(i2c->id >=4)
		dev_err(i2c->dev, "Clock PMIC %s\n", ((I2C_PMIC_RD(0x011A) & 0x7) & (1 << (i2c->id - 4)))?"disable":"enable");
	*/
	I2C_INFO(i2c, I2C_T_TRANSFERFLOW, "clock disable done .....\n");
#endif
	return;
}
/*Set i2c port speed*/
static int mt_i2c_set_speed(struct mt_i2c *i2c,struct i2c_msg *msg)
{
	int ret = 0;
	int mode = 0;
	unsigned long khz = 0;
	//u32 base = i2c->base;
	unsigned short step_cnt_div = 0;
	unsigned short sample_cnt_div = 0;
	unsigned long tmp, sclk, hclk = i2c->clk;
	unsigned short max_step_cnt_div = 0;
	unsigned long diff, min_diff = i2c->clk;
	unsigned short sample_div = MAX_SAMPLE_CNT_DIV;
	unsigned short step_div = max_step_cnt_div;
    unsigned short i2c_timing_reg=0;
	//dev_err(i2c->dev, "mt_i2c_set_speed=================\n");
	if(0 == (msg->timing & 0xFFFF)){
			mode	= ST_MODE;
			khz		= MAX_ST_MODE_SPEED;
	}
	else{
		if ((msg->ext_flag & I2C_HS_FLAG)){
			mode	= HS_MODE;
			khz		= msg->timing & 0xFFFF;
		} else {
			mode	= FS_MODE;
			khz		= msg->timing & 0xFFFF;
		}
	}

	max_step_cnt_div = (mode == HS_MODE) ? MAX_HS_STEP_CNT_DIV : MAX_STEP_CNT_DIV;

	if((mode == FS_MODE && khz > MAX_FS_MODE_SPEED) || (mode == HS_MODE && khz > MAX_HS_MODE_SPEED)){
		dev_err(i2c->dev, "mt-i2c: the speed is too fast for this mode.\n");
		I2C_BUG_ON((mode == FS_MODE && khz > MAX_FS_MODE_SPEED) || (mode == HS_MODE && khz > MAX_HS_MODE_SPEED));
		ret = -EINVAL;
		goto end;
	}
	i2c_timing_reg=i2c_readl(i2c, OFFSET_TIMING);
	if((mode == i2c->mode) && (khz == i2c->sclk)&&(i2c_timing_reg==I2C_TIMING_REG_BACKUP[i2c->id])) {
		I2C_INFO(i2c, I2C_T_SPEED, "mt-i2c: set sclk to %ldkhz\n", i2c->sclk);
		return 0;
	}

	/*Find the best combination*/
	for (sample_cnt_div = 1; sample_cnt_div <= MAX_SAMPLE_CNT_DIV; sample_cnt_div++) {
			for (step_cnt_div = 1; step_cnt_div <= max_step_cnt_div; step_cnt_div++) {
				sclk = (hclk >> 1) / (sample_cnt_div * step_cnt_div);
				if (sclk > khz)
					continue;
				diff = khz - sclk;
				if (diff < min_diff) {
					min_diff = diff;
					sample_div = sample_cnt_div;
					step_div   = step_cnt_div;
				}
			}
		}
		sample_cnt_div = sample_div;
		step_cnt_div   = step_div;

	sclk = hclk / (2 * sample_cnt_div * step_cnt_div);
	if (sclk > khz) {
		dev_err(i2c->dev, "%s mode: unsupported speed (%ldkhz)\n",
	           (mode == HS_MODE) ? "HS" : "ST/FT", khz);
	  I2C_BUG_ON(sclk > khz);
		ret = -ENOTSUPP;
		goto end;
	}

	step_cnt_div--;
	sample_cnt_div--;

	spin_lock(&i2c->lock);

	if (mode == HS_MODE) {

		/*Set the hignspeed timing control register*/
		tmp	= i2c_readl(i2c, OFFSET_TIMING) & ~((0x7 << 8) | (0x3f << 0));
		tmp	= (0 & 0x7) << 8 | (16 & 0x3f) << 0 | tmp;
		i2c_writel(i2c, OFFSET_TIMING, tmp);
        I2C_TIMING_REG_BACKUP[i2c->id]=tmp;

		/*Set the hign speed mode register*/
		tmp	= i2c_readl(i2c, OFFSET_HS) & ~((0x7 << 12) | (0x7 << 8));
		tmp	= (sample_cnt_div & 0x7) << 12 | (step_cnt_div & 0x7) << 8 | tmp;
		/*Enable the hign speed transaction*/
		tmp	|= 0x0001;
		i2c_writel(i2c, OFFSET_HS, tmp);
	}
	else {
		/*Set non-highspeed timing*/
		tmp  = i2c_readl(i2c, OFFSET_TIMING) & ~((0x7 << 8) | (0x3f << 0));
		tmp  = (sample_cnt_div & 0x7) << 8 | (step_cnt_div & 0x3f) << 0 | tmp;
		i2c_writel(i2c, OFFSET_TIMING, tmp);
        I2C_TIMING_REG_BACKUP[i2c->id]=tmp;

		/*Disable the high speed transaction*/
		//dev_err(i2c->dev, "NOT HS_MODE============================1\n");
		tmp	= i2c_readl(i2c, OFFSET_HS) & ~(0x0001);
		//dev_err(i2c->dev, "NOT HS_MODE============================2\n");
		i2c_writel(i2c, OFFSET_HS, tmp);
		//dev_err(i2c->dev, "NOT HS_MODE============================3\n");
	}
	i2c->mode = mode;
	i2c->sclk = khz;
	spin_unlock(&i2c->lock);
	I2C_INFO(i2c, I2C_T_SPEED, "mt-i2c: set sclk to %ldkhz(orig:%ldkhz), sample=%d,step=%d\n", sclk, khz, sample_cnt_div, step_cnt_div);
end:
	return ret;
}

static void mt_i2c_post_isr(struct mt_i2c *i2c,struct i2c_msg *msg)
{
	//u16 addr = msg->addr;

	if (i2c->irq_stat & I2C_TRANSAC_COMP) {
		atomic_set(&i2c->trans_err, 0);
		atomic_set(&i2c->trans_comp, 1);
	}

	if (i2c->irq_stat & I2C_HS_NACKERR) {
		if (likely(!(msg->ext_flag & I2C_A_FILTER_MSG)))
			dev_err(i2c->dev, "I2C_HS_NACKERR\n");
	}

	if (i2c->irq_stat & I2C_ACKERR) {
		if (likely(!(msg->ext_flag & I2C_A_FILTER_MSG)))
			dev_err(i2c->dev, "I2C_ACKERR\n");
	}
	atomic_set(&i2c->trans_err, i2c->irq_stat & (I2C_HS_NACKERR | I2C_ACKERR));
}
static inline void mt_i2c_dump_info(struct mt_i2c *i2c)
{
	dev_err(i2c->dev, "I2C structure:\nMode %x\nSt_rs %x\nDma_en %x\nOp %x\n"
			"Poll_en %x\nTrans_len %x\nTrans_num %x\nTrans_auxlen %x\nData_size %x\n"
			"Irq_stat %x\nTrans_stop %u\nTrans_comp %u\nTrans_error %u\n",
		   i2c->mode,
		   i2c->st_rs,
		   i2c->dma_en,
		   i2c->op,
		   i2c->poll_en,
		   i2c->trans_data.trans_len,
		   i2c->trans_data.trans_num,
		   i2c->trans_data.trans_auxlen,
		   i2c->trans_data.data_size,
		   i2c->irq_stat,
		   atomic_read(&i2c->trans_stop),
		   atomic_read(&i2c->trans_comp),
		   atomic_read(&i2c->trans_err));
	dev_err(i2c->dev,"base address %x\n",i2c->base);
	dev_err(i2c->dev, "I2C register:\nSLAVE_ADDR %x\nINTR_MASK %x\nINTR_STAT %x\n"
			"CONTROL %x\nTRANSFER_LEN %x\nTRANSAC_LEN %x\nDELAY_LEN %x\nTIMING %x\n"
			"START %x\nFIFO_STAT %x\nIO_CONFIG %x\nHS %x\nDEBUGSTAT %x\nEXT_CONF %x\n"
			"PATH_DIR %x\n",
			(i2c_readl(i2c, OFFSET_SLAVE_ADDR)),
			(i2c_readl(i2c, OFFSET_INTR_MASK)),
			(i2c_readl(i2c, OFFSET_INTR_STAT)),
			(i2c_readl(i2c, OFFSET_CONTROL)),
			(i2c_readl(i2c, OFFSET_TRANSFER_LEN)),
			(i2c_readl(i2c, OFFSET_TRANSAC_LEN)),
			(i2c_readl(i2c, OFFSET_DELAY_LEN)),
			(i2c_readl(i2c, OFFSET_TIMING)),
			(i2c_readl(i2c, OFFSET_START)),
			(i2c_readl(i2c, OFFSET_FIFO_STAT)),
			(i2c_readl(i2c, OFFSET_IO_CONFIG)),
			(i2c_readl(i2c, OFFSET_HS)),
			(i2c_readl(i2c, OFFSET_DEBUGSTAT)),
			(i2c_readl(i2c, OFFSET_EXT_CONF)),
			(i2c_readl(i2c, OFFSET_PATH_DIR)));
	/*
	dev_err(i2c->dev, "DMA register:\nINT_FLAG %x\nCON %x\nTX_MEM_ADDR %x\nRX_MEM_ADDR %x\nTX_LEN %x\nRX_LEN %x\nINT_EN %x\nEN %x\n",
			(__raw_readl(i2c->pdmabase+OFFSET_INT_FLAG)),
			(__raw_readl(i2c->pdmabase+OFFSET_CON)),
			(__raw_readl(i2c->pdmabase+OFFSET_TX_MEM_ADDR)),
			(__raw_readl(i2c->pdmabase+OFFSET_RX_MEM_ADDR)),
			(__raw_readl(i2c->pdmabase+OFFSET_TX_LEN)),
			(__raw_readl(i2c->pdmabase+OFFSET_RX_LEN)),
			(__raw_readl(i2c->pdmabase+OFFSET_INT_EN)),
			(__raw_readl(i2c->pdmabase+OFFSET_EN)));
	*/
	/*6589 side and PMIC side clock*/
	dev_err(i2c->dev, "Clock %s\n",
			(((__raw_readl(0xF0003018)>>26) | (__raw_readl(0xF000301c)&0x1 << 6)) & (1 << i2c->id))?"disable":"enable");
	if(i2c->id >=4)
		dev_err(i2c->dev, "Clock PMIC %s\n",
			((I2C_PMIC_RD(0x011A) & 0x7) & (1 << (i2c->id - 4)))?"disable":"enable");
	dev_err(i2c->dev,"GPIO%d(SCL):%d,mode%d; GPIO%d(SDA):%d,mode%d\n",
			mt_i2c_gpio_mode[i2c->id].scl,
			mt_get_gpio_in(mt_i2c_gpio_mode[i2c->id].scl),
			mt_get_gpio_mode(mt_i2c_gpio_mode[i2c->id].scl),
			mt_i2c_gpio_mode[i2c->id].sda,
			mt_get_gpio_in(mt_i2c_gpio_mode[i2c->id].sda),
			mt_get_gpio_mode(mt_i2c_gpio_mode[i2c->id].sda));

	return;

}
static int mt_deal_result(struct mt_i2c *i2c, struct i2c_msg *msg)
{
	long tmo = i2c->adap.timeout;
	u16 data_size = 0;
	u8 *ptr = msg->buf;
	u16 addr = msg->addr;
	int ret = msg->len;
	u16 read = (msg->flags & I2C_M_RD);
	long tmo_poll = 0xffff;

	addr = read ? ((addr << 1) | 0x1) : ((addr << 1) & ~0x1);

	if(i2c->poll_en) { /*master read && poll mode*/
		for (;;) { /*check the interrupt status register*/
			i2c->irq_stat = i2c_readl(i2c, OFFSET_INTR_STAT);
			if(i2c->irq_stat & (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP)) {
				atomic_set(&i2c->trans_stop, 1);
				spin_lock(&i2c->lock);
				/*Clear interrupt status,write 1 clear*/
				i2c_writel(i2c, OFFSET_INTR_STAT, (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
				spin_unlock(&i2c->lock);
				break;
			}
			tmo_poll --;
			if(tmo_poll == 0) {
				tmo = 0;
				break;
			}
		}
	} else { /*Interrupt mode,wait for interrupt wake up*/
		tmo = wait_event_timeout(i2c->wait,atomic_read(&i2c->trans_stop), tmo);
	}

	/*Save register status to i2c struct*/
	mt_i2c_post_isr(i2c, msg);

	/*Check the transfer status*/
	if (!(tmo == 0 || atomic_read(&i2c->trans_err)) ) {/*Transfer success ,we need to get data from fifo*/
		if(!i2c->dma_en && (i2c->op == I2C_MASTER_RD || i2c->op == I2C_MASTER_WRRD) ) { /*only read mode or write_read mode and fifo mode need to get data*/
			ptr = msg->buf;
			data_size = (i2c_readl(i2c, OFFSET_FIFO_STAT) >> 4) & 0x000F;
			while (data_size--) {
				*ptr = i2c_readl(i2c, OFFSET_DATA_PORT);
				//dev_info(i2c->dev, "addr %.2x read byte = 0x%.2X\n", addr, *ptr);
				ptr++;
			}
		}
	} else { /*Timeout or ACKERR*/
		if ( tmo == 0 ){
			dev_err(i2c->dev, "addr: %.2x, transfer timeout\n", addr);
			ret = -ETIMEDOUT;
		} else {
			dev_err(i2c->dev, "addr: %.2x, transfer error\n", addr);
			ret = -EREMOTEIO;
		}

		if (likely(!(msg->ext_flag & I2C_A_FILTER_MSG))) { /*Dump i2c_struct & register*/
			mt_i2c_dump_info(i2c);
		}

		spin_lock(&i2c->lock);
		/*Reset i2c port*/
		i2c_writel(i2c, OFFSET_SOFTRESET, 0x0001);
		/*Set slave address*/
		i2c_writel( i2c, OFFSET_SLAVE_ADDR, 0x0000 );
		/*Clear interrupt status*/
		i2c_writel(i2c, OFFSET_INTR_STAT, (I2C_HS_NACKERR|I2C_ACKERR|I2C_TRANSAC_COMP));
		/*Clear fifo address*/
		i2c_writel(i2c, OFFSET_FIFO_ADDR_CLR, 0x0001);

		spin_unlock(&i2c->lock);
	}

	return ret;
}

static int mt_i2c_start_xfer(struct mt_i2c *i2c, struct i2c_msg *msg)
{

	u16 control = 0;
	u16 trans_num = 0;
	u16 data_size = 0;
	u16 trans_len = 0;
	u8 *ptr = msg->buf;
	u16 len = msg->len;
	int ret = len;
	u16 trans_auxlen = 0;
	u16 addr = msg->addr;
	u16 flags = msg->flags;
	u16 read = (flags & I2C_M_RD);


	/*Check param valid*/
	if(addr == 0){
		dev_err(i2c->dev, "mt-i2c: addr is invalid.\n");
		I2C_BUG_ON(addr == NULL);
		ret = -EINVAL;
		goto err;
	}

	if(msg->buf == NULL){
		dev_err(i2c->dev, "mt-i2c: data buffer is NULL.\n");
		I2C_BUG_ON(msg->buf == NULL);
		ret = -EINVAL;
		goto err;
	}
	//compatible with 77/75 driver
	if(msg->addr & 0xFF00){
		//dev_err(i2c->dev, "I2C Slave device driver parameter used warning ! please donot add any external flags at the high 8 bits of 16-bit address\n");
		msg->ext_flag |= msg->addr & 0xFF00;
		//I2C_BUG_ON(msg->addr & 0xFF00);

		/*go on...*/

	//	ret = -EINVAL;
	//	goto err;
	}

	if (g_i2c[0] == i2c || g_i2c[1] == i2c) {
		dev_err(i2c->dev, "mt-i2c%d: Current I2C Adapter is busy.\n", i2c->id);
		ret = -EINVAL;
		goto err;
	}

	I2C_INFO(i2c, I2C_T_TRANSFERFLOW, "Before i2c transfer .....\n");

	if (msg->ext_flag & I2C_DIRECTION_FLAG) {
		if(i2c->id <= 3){
			dev_err(i2c->dev, "the adapter do not support wrapper.\n");
			ret = -EINVAL;
			goto err;
		}
	//	dev_err(i2c->dev, "======enable wrapper=========\n");
		i2c_writel(i2c, OFFSET_PATH_DIR, I2C_CONTROL_WRAPPER); /*We must set wrapper bit before setting other register*/

		i2c->clk = I2C_CLK_WRAPPER_RATE;
	}else{
#if (defined(CONFIG_MT_I2C_FPGA_ENABLE))
		i2c->clk	= I2C_CLK_RATE;
#else
		i2c->clk	= mt_get_bus_freq()/16;
#endif
	}

		//dev_err(i2c->dev, "======set speed1=========\n");
	/*Set device speed*/
	if ( mt_i2c_set_speed(i2c, msg) < 0 ){
		ret =-EINVAL;
		//dev_err(i2c->dev, "======================\n");
		goto err;
	}
	/*set start condition */
	if((msg->timing & 0xFFFF) <= 100){
		i2c_writel(i2c,OFFSET_EXT_CONF, 0x8001);
	}
		//dev_err(i2c->dev, "======set speed2=========\n");
	/*prepare data*/
	if((msg->ext_flag & I2C_RS_FLAG))
		i2c->st_rs = I2C_TRANS_REPEATED_START;
	else
		i2c->st_rs = I2C_TRANS_STOP;

	if((msg->ext_flag & I2C_DMA_FLAG))
		i2c->dma_en = true;
	else
		i2c->dma_en = false;

  if(i2c->dma_en) {
	I2C_INFO(i2c, I2C_T_DMA, "DMA Transfer mode!\n");
  	if (i2c->pdmabase == 0) {
  			dev_err(i2c->dev, "mt-i2c: I2C%d doesnot support DMA mode!\n",i2c->id);
			I2C_BUG_ON(i2c->pdmabase == NULL);
			ret = -EINVAL;
			goto err;
		}
		if((u32)ptr > DMA_ADDRESS_HIGH){
			dev_err(i2c->dev, "mt-i2c: DMA mode should use physical buffer address!\n");
			I2C_BUG_ON((u32)ptr > DMA_ADDRESS_HIGH);
			ret = -EINVAL;
			goto err;
		}
	}

	i2c->delay_len = (msg->timing & 0xff0000) >> 16;

	if(0 == i2c->delay_len)
		i2c->delay_len = 2;

	if ((msg->ext_flag & I2C_WR_FLAG))
		i2c->op = I2C_MASTER_WRRD;
	else
	{
	   if(msg->flags & I2C_M_RD)
	   		i2c->op = I2C_MASTER_RD;
	   else
	   		i2c->op = I2C_MASTER_WR;
	}

	if(msg->ext_flag & I2C_POLLING_FLAG)
		i2c->poll_en = true;
	else
		i2c->poll_en = false;

	atomic_set(&i2c->trans_stop, 0);
	atomic_set(&i2c->trans_comp, 0);
	atomic_set(&i2c->trans_err, 0);
	i2c->irq_stat = 0;
	addr = read ? ((addr << 1) | 0x1) : ((addr << 1) & ~0x1);

	/*Get Transfer len and transaux len*/
	if(false == i2c->dma_en) { /*non-DMA mode*/
		if(I2C_MASTER_WRRD != i2c->op) {

			trans_len = (msg->len) & 0xFF;
			trans_num = (msg->len >> 8) & 0xFF;
			if(0 == trans_num)
				trans_num = 1;
			trans_auxlen = 0;
			data_size = trans_len*trans_num;

			if(!trans_len || !trans_num || trans_len*trans_num > 8) {
				dev_err(i2c->dev, "mt-i2c: non-WRRD transfer length is not right. trans_len=%x, tans_num=%x, trans_auxlen=%x\n", trans_len, trans_num, trans_auxlen);
				I2C_BUG_ON(!trans_len || !trans_num || trans_len*trans_num > 8);
				ret = -EINVAL;
				goto err;
			}
		} else {

			trans_len = (msg->len) & 0xFF;
			trans_auxlen = (msg->len >> 8) & 0xFF;
			trans_num = 2;
			data_size = trans_len;

			if(!trans_len || !trans_auxlen || trans_len > 8 || trans_auxlen > 8) {
				dev_err(i2c->dev, "mt-i2c: WRRD transfer length is not right. trans_len=%x, tans_num=%x, trans_auxlen=%x\n", trans_len, trans_num, trans_auxlen);
				I2C_BUG_ON(!trans_len || !trans_auxlen || trans_len > 8 || trans_auxlen > 8);
				ret = -EINVAL;
				goto err;
			}
		}
	} else { /*DMA mode*/
		if(I2C_MASTER_WRRD != i2c->op) {
			trans_len = (msg->len) & 0xFF;
			trans_num = (msg->len >> 8) & 0xFF;
			if(0 == trans_num)
				trans_num = 1;
			trans_auxlen = 0;
			data_size = trans_len*trans_num;

			if(!trans_len || !trans_num || trans_len > 255 || trans_num > 255) {
				dev_err(i2c->dev, "mt-i2c: DMA non-WRRD transfer length is not right. trans_len=%x, tans_num=%x, trans_auxlen=%x\n", trans_len, trans_num, trans_auxlen);
				I2C_BUG_ON(!trans_len || !trans_num || trans_len > 255 || trans_num > 255);
				ret = -EINVAL;
				goto err;
			}
		I2C_INFO(i2c, I2C_T_DMA, "DMA non-WRRD mode!trans_len=%x, tans_num=%x, trans_auxlen=%x\n",trans_len, trans_num, trans_auxlen);
		} else {
			trans_len = (msg->len) & 0xFF;
			trans_auxlen = (msg->len >> 8) & 0xFF;
			trans_num = 2;
			data_size = trans_len;
			if(!trans_len || !trans_auxlen || trans_len > 255 || trans_auxlen > 31) {
				dev_err(i2c->dev, "mt-i2c: DMA WRRD transfer length is not right. trans_len=%x, tans_num=%x, trans_auxlen=%x\n", trans_len, trans_num, trans_auxlen);
				I2C_BUG_ON(!trans_len || !trans_auxlen || trans_len > 255 || trans_auxlen > 31);
				ret = -EINVAL;
				goto err;
			}
		I2C_INFO(i2c, I2C_T_DMA, "DMA WRRD mode!trans_len=%x, tans_num=%x, trans_auxlen=%x\n",trans_len, trans_num, trans_auxlen);
		}
	}

	spin_lock(&i2c->lock);

	/*Set ioconfig*/
	if ((msg->ext_flag & I2C_PUSHPULL_FLAG)) {
			i2c_writel(i2c, OFFSET_IO_CONFIG, 0x0000);
		} else {
			i2c_writel(i2c, OFFSET_IO_CONFIG, 0x0003);
		}

	/*Set Control Register*/
	control = I2C_CONTROL_ACKERR_DET_EN | I2C_CONTROL_CLK_EXT_EN;
	if(i2c->dma_en) {
		control |= I2C_CONTROL_DMA_EN;
	}
	if(I2C_MASTER_WRRD == i2c->op)
		control |= I2C_CONTROL_DIR_CHANGE;

	if(HS_MODE == i2c->mode || (trans_num > 1 && I2C_TRANS_REPEATED_START == i2c->st_rs)) {
		control |= I2C_CONTROL_RS;
	}

//	if (msg->ext_flag & I2C_DIRECTION_FLAG) {
//		control |= I2C_CONTROL_WRAPPER; /*PMIC WRAPPER */
//	}

	i2c_writel(i2c, OFFSET_CONTROL, control);
	if(~control & I2C_CONTROL_RS){	// bit is set to 1, i.e.,use repeated stop
		i2c_writel(i2c, OFFSET_DELAY_LEN, i2c->delay_len);
	}

	/*Setup Registers*/

	/*Set slave address*/
	i2c_writel(i2c, OFFSET_SLAVE_ADDR, addr);
	/*Clear interrupt status*/
	i2c_writel(i2c, OFFSET_INTR_STAT, (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
	/*Clear fifo address*/
	i2c_writel(i2c, OFFSET_FIFO_ADDR_CLR, 0x0001);
	/*Setup the interrupt mask flag*/
	if(i2c->poll_en)
		i2c_writel(i2c, OFFSET_INTR_MASK, i2c_readl(i2c, OFFSET_INTR_MASK) & ~(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP)); /*Disable interrupt*/
	else
		i2c_writel(i2c, OFFSET_INTR_MASK, i2c_readl(i2c, OFFSET_INTR_MASK) | (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP)); /*Enable interrupt*/
	/*Set transfer len */
	i2c_writel(i2c, OFFSET_TRANSFER_LEN, ((trans_auxlen & 0x1F) << 8) | (trans_len & 0xFF));
	/*Set transaction len*/
	i2c_writel(i2c, OFFSET_TRANSAC_LEN, trans_num & 0xFF);

	/*Prepare buffer data to start transfer*/

	if(i2c->dma_en) {
		if (I2C_MASTER_RD == i2c->op) {
			mt65xx_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_INT_FLAG);
			mt65xx_reg_sync_writel(0x0001, i2c->pdmabase + OFFSET_CON);
			mt65xx_reg_sync_writel((u32)msg->buf, i2c->pdmabase + OFFSET_RX_MEM_ADDR);
			mt65xx_reg_sync_writel(data_size, i2c->pdmabase + OFFSET_RX_LEN);
		} else if (I2C_MASTER_WR == i2c->op) {
			mt65xx_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_INT_FLAG);
			mt65xx_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_CON);
			mt65xx_reg_sync_writel((u32)msg->buf, i2c->pdmabase + OFFSET_TX_MEM_ADDR);
			mt65xx_reg_sync_writel(data_size, i2c->pdmabase + OFFSET_TX_LEN);
		} else {
			mt65xx_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_INT_FLAG);
			mt65xx_reg_sync_writel(0x0000, i2c->pdmabase + OFFSET_CON);
			mt65xx_reg_sync_writel((u32)msg->buf, i2c->pdmabase + OFFSET_TX_MEM_ADDR);
			mt65xx_reg_sync_writel((u32)msg->buf, i2c->pdmabase + OFFSET_RX_MEM_ADDR);
			mt65xx_reg_sync_writel(trans_len, i2c->pdmabase + OFFSET_TX_LEN);
			mt65xx_reg_sync_writel(trans_auxlen, i2c->pdmabase + OFFSET_RX_LEN);
		}
		mb();
		mt65xx_reg_sync_writel(0x0001, i2c->pdmabase + OFFSET_EN);

	I2C_INFO(i2c, I2C_T_DMA, "addr %.2x dma %.2X byte\n", addr, data_size);
	I2C_INFO(i2c, I2C_T_DMA, "DMA Register:INT_FLAG:0x%x,CON:0x%x,TX_MEM_ADDR:0x%x, \
							  RX_MEM_ADDR:0x%x,TX_LEN:0x%x,RX_LEN:0x%x,EN:0x%x\n",
							  readl(i2c->pdmabase + OFFSET_INT_FLAG),
							  readl(i2c->pdmabase + OFFSET_CON),
							  readl(i2c->pdmabase + OFFSET_TX_MEM_ADDR),
							  readl(i2c->pdmabase + OFFSET_RX_MEM_ADDR),
							  readl(i2c->pdmabase + OFFSET_TX_LEN),
							  readl(i2c->pdmabase + OFFSET_RX_LEN),
							  readl(i2c->pdmabase + OFFSET_EN));

	} else {/*Set fifo mode data*/
		if (I2C_MASTER_RD == i2c->op) {
			/*do not need set fifo data*/
		} else { /*both write && write_read mode*/
			while (data_size--) {
				i2c_writel(i2c, OFFSET_DATA_PORT, *ptr);
				//dev_info(i2c->dev, "addr %.2x write byte = 0x%.2X\n", addr, *ptr);
				ptr++;
			}
		}
	}

	/*Set trans_data*/
	i2c->trans_data.trans_num = trans_num;
	i2c->trans_data.data_size = data_size;
	i2c->trans_data.trans_len = trans_len;
	i2c->trans_data.trans_auxlen = trans_auxlen;


	/*All register must be prepared before setting the start bit [SMP]*/
	mb();

	/*This is only for 3D CAMERA*/
	if (msg->ext_flag & I2C_3DCAMERA_FLAG) {

		spin_unlock(&i2c->lock);

		if (g_msg[0].buf == NULL)
			memcpy((void *)&g_msg[0], msg, sizeof(struct i2c_msg)); /*Save address infomation for 3d camera*/
		else
			memcpy((void *)&g_msg[1], msg, sizeof(struct i2c_msg)); /*Save address infomation for 3d camera*/

		if (g_i2c[0] == NULL)
			g_i2c[0] = i2c;
		else
			g_i2c[1] = i2c;

		goto end;
	}
	I2C_INFO(i2c, I2C_T_TRANSFERFLOW, "Before start .....\n");
		/*Start the transfer*/
		i2c_writel(i2c, OFFSET_START, 0x0001);
		spin_unlock(&i2c->lock);

		ret = mt_deal_result(i2c, msg);

	I2C_INFO(i2c, I2C_T_TRANSFERFLOW, "After i2c transfer .....\n");
err:

end:
	return ret;
}

/*Interrupt handler function*/
static irqreturn_t mt_i2c_irq(int irqno, void *dev_id)
{
	struct mt_i2c *i2c = (struct mt_i2c *)dev_id;
	//u32 base = i2c->base;

	I2C_INFO(i2c, I2C_T_TRANSFERFLOW, "i2c interrupt coming.....\n");

	/*Clear interrupt mask*/
	i2c_writel(i2c,OFFSET_INTR_MASK,i2c_readl(i2c,OFFSET_INTR_MASK) & ~(I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
	/*Save interrupt status*/
	i2c->irq_stat = i2c_readl(i2c, OFFSET_INTR_STAT);
	/*Clear interrupt status,write 1 clear*/
	i2c_writel(i2c,OFFSET_INTR_STAT, (I2C_HS_NACKERR | I2C_ACKERR | I2C_TRANSAC_COMP));
	//dev_info(i2c->dev, "I2C interrupt status 0x%04X\n", i2c->irq_stat);

	/*Wake up process*/
	atomic_set(&i2c->trans_stop, 1);
	wake_up(&i2c->wait);
	return IRQ_HANDLED;
}

static int mt_i2c_do_transfer(struct mt_i2c *i2c, struct i2c_msg *msgs, int num)
{
	int ret = 0;
	int left_num = num;

	while (left_num--) {
			ret = mt_i2c_start_xfer(i2c, msgs++);
			if ( ret < 0 ){
				if ( ret != -EINVAL ) /*We never try again when the param is invalid*/
					return -EAGAIN;
				else
					return -EINVAL;
			}
	}
	/*the return value is number of executed messages*/
	return num;
}

static int mt_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	int	ret = 0;
	int	retry;
	struct mt_i2c *i2c = i2c_get_adapdata(adap);
	mt_i2c_clock_enable(i2c);

	for (retry = 0; retry < adap->retries; retry++) {
		ret = mt_i2c_do_transfer(i2c, msgs, num);
		if (ret != -EAGAIN) {
			break;
		}
		//dev_info(i2c->dev, "Retrying transmission (%d)\n", retry);

		if ( retry < adap->retries - 1 )
			udelay(100);
	}
//	if (!(msg[0]->ext_flag & I2C_3DCAMERA_FLAG))
	mt_i2c_clock_disable(i2c);
	if (ret != -EAGAIN)
		return ret;
	else
		return -EREMOTEIO;
}

/*This function is only for 3d camera*/

int mt_wait4_i2c_complete(void)
{
	struct mt_i2c *i2c0 = g_i2c[0];
	struct mt_i2c *i2c1 = g_i2c[1];
	int result0, result1;
	int ret = 0;

	if ((i2c0 == NULL) ||(i2c1 == NULL)) {
		/*What's wrong?*/
		ret = -EINVAL;
		goto end;
	}

	result0 = mt_deal_result(i2c0, &g_msg[0]);
	result1 = mt_deal_result(i2c1, &g_msg[1]);

  if (result0 < 0 || result1 < 0) {
  	ret = -EINVAL;
  }

end:
	g_i2c[0] = NULL;
	g_i2c[1] = NULL;

	g_msg[0].buf = NULL;
	g_msg[1].buf = NULL;

	return ret;
}
EXPORT_SYMBOL(mt_wait4_i2c_complete);

static u32 mt_i2c_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_10BIT_ADDR | I2C_FUNC_SMBUS_EMUL;
}
static struct i2c_algorithm mt_i2c_algorithm = {
	.master_xfer   = mt_i2c_transfer,
	.smbus_xfer    = NULL,
	.functionality = mt_i2c_functionality,
};
static inline void mt_i2c_init_hw(struct mt_i2c *i2c)
{
	i2c_writel(i2c,OFFSET_SOFTRESET, 0x0001);
}

static void mt_i2c_free(struct mt_i2c *i2c)
{
	if (!i2c)
		return;

	free_irq(i2c->irqnr, i2c);
	i2c_del_adapter(&i2c->adap);
	kfree(i2c);
}

static int mt_i2c_probe(struct platform_device *pdev)
{
	int ret, irq;
	struct mt_i2c *i2c = NULL;
	struct resource *res;

	/* Request platform_device IO resource*/
	res		= platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq		= platform_get_irq(pdev, 0);
	if (res == NULL || irq < 0)
	     return -ENODEV;

	/* Request IO memory */
	if (!request_mem_region(res->start, resource_size(res), pdev->name)) {
		return -ENOMEM;
	}

	if (NULL == (i2c = kzalloc(sizeof(struct mt_i2c), GFP_KERNEL)))
		return -ENOMEM;

	/* initialize mt_i2c structure */
	i2c->id		= pdev->id;
	i2c->base	= IO_PHYS_TO_VIRT(res->start);
	//i2c->base	= 0x11011000;
	i2c->irqnr	= irq;
#if (defined(CONFIG_MT_I2C_FPGA_ENABLE))
	i2c->clk	= I2C_CLK_RATE;
#else
	i2c->clk	= mt_get_bus_freq()/16;
	//FIX me if clock manager add this Macro
	switch(i2c->id){
		case 0:
			i2c->pdn = MT_CG_PERI0_I2C0;
		break;
		case 1:
			i2c->pdn = MT_CG_PERI0_I2C1;
		break;
		case 2:
			i2c->pdn = MT_CG_PERI0_I2C2;
		break;
		case 3:
			i2c->pdn = MT_CG_PERI0_I2C3;
		break;
		case 4:
			i2c->pdn = MT_CG_PERI0_I2C4;
		break;
		case 5:
			i2c->pdn = MT_CG_PERI0_I2C5;
		break;
		case 6:
			i2c->pdn = MT_CG_PERI1_I2C6;
#ifdef I2C_DEBUG_FS
//			i2c_port[6][0xF] = 1;
//			i2c_port[6][0] = 1;
#endif
		break;
		default:
			dev_err(&pdev->dev, "Error id %d\n", i2c->id);
	}
#endif
	i2c->dev	= &i2c->adap.dev;

	i2c->adap.dev.parent	= &pdev->dev;
	i2c->adap.nr			= i2c->id;
	i2c->adap.owner			= THIS_MODULE;
	i2c->adap.algo			= &mt_i2c_algorithm;
	i2c->adap.algo_data		= NULL;
	i2c->adap.timeout		= 2 * HZ; /*2s*/
	i2c->adap.retries		= 1; /*DO NOT TRY*/

	snprintf(i2c->adap.name, sizeof(i2c->adap.name), I2C_DRV_NAME);

	i2c->pdmabase = AP_DMA_BASE + 0x300 + (0x80*(i2c->id));

	spin_lock_init(&i2c->lock);
	init_waitqueue_head(&i2c->wait);

	ret = request_irq(irq, mt_i2c_irq, IRQF_TRIGGER_LOW, I2C_DRV_NAME, i2c);

	if (ret){
		dev_err(&pdev->dev, "Can Not request I2C IRQ %d\n", irq);
		goto free;
	}

	mt_i2c_init_hw(i2c);
	i2c_set_adapdata(&i2c->adap, i2c);
	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret){
		dev_err(&pdev->dev, "failed to add i2c bus to i2c core\n");
		goto free;
	}
	platform_set_drvdata(pdev, i2c);

#ifdef I2C_DEBUG_FS
	ret = device_create_file(i2c->dev, &dev_attr_debug);
	if ( ret ){
		/*Do nothing*/
	}
#endif

	return ret;

free:
	mt_i2c_free(i2c);
	return ret;
}


static int mt_i2c_remove(struct platform_device *pdev)
{
	struct mt_i2c *i2c = platform_get_drvdata(pdev);
	if (i2c) {
		platform_set_drvdata(pdev, NULL);
		mt_i2c_free(i2c);
	}
	return 0;
}

#ifdef CONFIG_PM
static int mt_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
   // struct mt_i2c *i2c = platform_get_drvdata(pdev);
    //dev_dbg(i2c->dev,"[I2C %d] Suspend!\n", i2c->id);
    return 0;
}

static int mt_i2c_resume(struct platform_device *pdev)
{
   // struct mt_i2c *i2c = platform_get_drvdata(pdev);
   // dev_dbg(i2c->dev,"[I2C %d] Resume!\n", i2c->id);
    return 0;
}
#else
#define mt_i2c_suspend	NULL
#define mt_i2c_resume	NULL
#endif

static struct platform_driver mt_i2c_driver = {
	.probe	 = mt_i2c_probe,
	.remove	 = mt_i2c_remove,
	.suspend = mt_i2c_suspend,
	.resume	 = mt_i2c_resume,
	.driver  = {
        .name  = I2C_DRV_NAME,
        .owner = THIS_MODULE,
    },
};

static int __init mt_i2c_init(void)
{
	return platform_driver_register(&mt_i2c_driver);
}

static void __exit mt_i2c_exit(void)
{
	platform_driver_unregister(&mt_i2c_driver);
}

module_init(mt_i2c_init);
module_exit(mt_i2c_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek I2C Bus Driver");
MODULE_AUTHOR("Infinity Chen <infinity.chen@mediatek.com>");
