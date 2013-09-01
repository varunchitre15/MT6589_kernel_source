/*****************************************************************************
*                E X T E R N A L   R E F E R E N C E S
******************************************************************************
*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <linux/proc_fs.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>
#include <linux/io.h>

#include "AudioCommon.h"
#include "audio.h"
#include "AudioAfe.h"

#define AUDIO_NAME    "dvt-audio"


/*****************************************************************************
*                         F U N C T I O N S
******************************************************************************
*/

static long audio_ldvt_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    INT32 i4Result;
    i4Result=0;
    printk("[audio_ldvt]: ioctl cmd = %d  \n", cmd);
    switch(cmd)
    {
            /// AXI Interface
        case INTERNAL_SRAM_TEST:
            printk("[audio_ldvt]: INTERNAL_SRAM_TEST\n");
            if(!fgAudioMemTest()) i4Result=-1;
            break;

            /// Interface Test
        case MEMORY_UL_MONO_TEST:
            printk("[audio_ldvt]: MEMORY_UL_MONO_TEST\n");
            if(!fgAudioMemULMonoTest()) i4Result=-1;
            break;
        case MEMORY_LOOP1_TEST:
            printk("[audio_ldvt]: MEMORY_LOOP1_TEST\n");
            if(!fgAudioMemLoop1Test()) i4Result=-1;
            break;
        case MEMORY_LOOP2_TEST:
            printk("[audio_ldvt]: MEMORY_LOOP2_TEST\n");
            if(!fgAudioMemLoop2Test()) i4Result=-1;
            break;

            /// I2S Out (+ExtDAC), I2S_WLEN= 16/20 bit
        case I2S_DAC_OUT_16BIT:
            printk("[audio_ldvt]: I2S_DAC_OUT_16BIT\n");
            vDacI2sOut(I2S_16BIT);
            i4Result=-3;
            break;
        case I2S_DAC_OUT_20BIT:
            printk("[audio_ldvt]: I2S_DAC_OUT_20BIT\n");
            vDacI2sOut(I2S_32BIT);
            i4Result=-3;
            break;

            /// I2S In Slave and FOC
        case I2S_IN_SLAVE_FOC_ON:
            printk("[audio_ldvt]: I2S_IN_SLAVE_FOC_ON\n");
            vI2sIn(I2S_SLAVE, TRUE);
            i4Result=-3;
            break;
        case I2S_IN_SLAVE_FOC_OFF:
            printk("[audio_ldvt]: I2S_IN_SLAVE_FOC_OFF\n");
            vI2sIn(I2S_SLAVE, FALSE);
            i4Result=-3;
            break;

            /// I2S In Master
        case I2S_IN_MASTER:
            printk("[audio_ldvt]: I2S_IN_MASTER\n");
            vI2sIn(I2S_MASTER, FALSE);
            i4Result=-3;
            break;

            /// MOD_PCM
        case MOD_PCM_1_EXT_MD_MASTER:
            printk("[audio_ldvt]: MOD_PCM_1_EXT_MD_MASTER\n");
            vModPcm1ExtMasterMode();
            i4Result=-3;
            break;
        case MOD_PCM_1_EXT_MD_SLAVE_ASRC:
            printk("[audio_ldvt]: MOD_PCM_1_EXT_MD_SLAVE_ASRC\n");
            vModPcm1ExtSlaveModeASRC();
            i4Result=-3;
            break;
        case MOD_PCM_1_EXT_MD_SLAVE_ASYNC_FIFO:
            printk("[audio_ldvt]: MOD_PCM_1_EXT_MD_SLAVE_ASYNC_FIFO\n");
            vModPcm1ExtSlaveModeAsyncFIFO();
            i4Result=-3;
            break;
        case MOD_PCM_1_INT_MD_SLAVE_ASYNC_FIFO:
            printk("[audio_ldvt]: MOD_PCM_1_INT_MD_SLAVE_ASYNC_FIFO\n");
            vModPcm1IntSlaveModeAsyncFIFO();
            i4Result=-3;
            break;
        case MOD_PCM_2_INT_MD_SLAVE_ASYNC_FIFO:
            printk("[audio_ldvt]: MOD_PCM_2_INT_MD_SLAVE_ASYNC_FIFO\n");
            vModPcm2AsyncFIFO();
            i4Result=-3;
            break;
        case MOD_PCM_1_2_INT_MD_SLAVE_ASYNC_FIFO_VBT_16K:
            printk("[audio_ldvt]: MOD_PCM_1_2_INT_MD_SLAVE_ASYNC_FIFO_VBT_16K\n");
            if(!fgModPcmVbt16kMode()) i4Result=-1;
            break;

            /// Merge Interface
        case MERGE_IF_PCM_ONLY_08000:
            printk("[audio_ldvt]: MERGE_IF_PCM_ONLY_08000\n");
            if(!fgMergeIfPcmOnly()) i4Result=-1;
            break;
        case MERGE_IF_I2S_ONLY_44100:
            printk("[audio_ldvt]: MERGE_IF_I2S_ONLY_44100\n");
            vMergeIfI2sOnly(AFE_44100HZ);
            i4Result=-3;
            break;
        case MERGE_IF_PCM_WITH_I2S_32000:
            printk("[audio_ldvt]: MERGE_IF_PCM_WITH_I2S_32000\n");
            vMergeIfPcmWithI2s(AFE_32000HZ);
            i4Result=-3;
            break;
        case MERGE_IF_PCM_WITH_I2S_44100:
            printk("[audio_ldvt]: MERGE_IF_PCM_WITH_I2S_44100\n");
            vMergeIfPcmWithI2s(AFE_44100HZ);
            i4Result=-3;
            break;
        case MERGE_IF_PCM_WITH_I2S_48000:
            printk("[audio_ldvt]: MERGE_IF_PCM_WITH_I2S_48000\n");
            vMergeIfPcmWithI2s(AFE_48000HZ);
            i4Result=-3;
            break;
        case MERGE_IF_I2S_ONLY_CHANGE_FS:
            printk("[audio_ldvt]: MERGE_IF_I2S_ONLY_CHANGE_FS\n");
            vMergeIfI2sOnlyChangeFs();
            i4Result=-3;
            break;
        case MERGE_IF_I2S_FIRST_PCM_LATER:
            printk("[audio_ldvt]: MERGE_IF_I2S_FIRST_PCM_LATER\n");
            vMergeIfI2sFirstPcmLater();
            i4Result=-3;
            break;
        case MERGE_IF_PCM_FIRST_I2S_LATER:
            printk("[audio_ldvt]: MERGE_IF_PCM_FIRST_I2S_LATER\n");
            vMergeIfPcmFirstI2sLater();
            i4Result=-3;
            break;
        case MERGE_IF_WITH_I2S_IN_MASTER:
            printk("[audio_ldvt]: MERGE_IF_WITH_I2S_IN_MASTER\n");
            vMergeIfWithI2sIn(I2S_MASTER);
            i4Result=-3;
            break;
        case MERGE_IF_WITH_I2S_IN_SLAVE:
            printk("[audio_ldvt]: MERGE_IF_WITH_I2S_IN_SLAVE\n");
            vMergeIfWithI2sIn(I2S_SLAVE);
            i4Result=-3;
            break;

		case HW_GAIN_1:
            printk("[audio_ldvt]: HW_GAIN_1\n");
            vAudioHWGain1Test();
            i4Result=-3;
            break;

		case HW_GAIN_2:
            printk("[audio_ldvt]: HW_GAIN_2\n");
            vAudioHWGain2Test();
            i4Result=-3;
            break;

		case HW_GAIN_COMBINATION:
            printk("[audio_ldvt]: HW_GAIN_COMBINATION\n");
            vAudioHWGain1_2CombineTest(AFE_48000HZ);
            i4Result=-3;
            break;
			
        default:
            return -EINVAL;
    }
    return i4Result;
}

static INT32 audio_ldvt_dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations audio_ldvt_dev_fops =
{
    .owner          = THIS_MODULE,
    .unlocked_ioctl = audio_ldvt_dev_ioctl,
    .open           = audio_ldvt_dev_open,
};


static struct miscdevice audio_ldvt_dev =
{
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = AUDIO_NAME,
    .fops   = &audio_ldvt_dev_fops,
};

static int AudEmu_Read_Procmem(char *buf,char **start, off_t offset, int count , int *eof, void *data)
{
    INT32 i4len = 0;
    // AudioSys Register Setting
    printk("+AudEmu_Read_Procmem\n");
    i4len += sprintf(buf+ i4len, "Dram Buf Physical Addr = 0x%x\n",u4DramPhyBase);
    i4len += sprintf(buf+ i4len, "Dram Buf Virtual Addr = 0x%x\n",u4DramVirBase);
    return i4len;
}

void audio_local_init()
{
    WriteREG(FPGA_CFG0, 0x00001007);    // hopping 32m, MCLK : 3.072M

    u4DramVirBase = dma_alloc_coherent(0, AFE_EXTERNAL_DRAM_SIZE, &u4DramPhyBase, GFP_KERNEL);
    u4DramPhyEnd = u4DramPhyBase + AFE_EXTERNAL_DRAM_SIZE - 1;
    u4DramVirEnd = u4DramVirBase + AFE_EXTERNAL_DRAM_SIZE - 1;

    u4SramPhyBase = AFE_INTERNAL_SRAM_PHY_BASE;
    u4SramVirBase = AFE_INTERNAL_SRAM_VIR_BASE;
    u4SramPhyEnd = u4SramPhyBase + AFE_INTERNAL_SRAM_SIZE - 1;
    u4SramVirEnd = u4SramVirBase + AFE_INTERNAL_SRAM_SIZE - 1;
}

static INT32 __init audio_ldvt_mod_init(void)
{
    INT32 ret;

    ret = misc_register(&audio_ldvt_dev);
    if (ret)
    {
        printk("[audio_ldvt]: register driver failed (%d)\n", ret);
        return ret;
    }

    audio_local_init();
    printk("[audio_ldvt]: adc_udvt_local_init initialization\n");

    // cat /proc/Audio
    create_proc_read_entry("AudioEmu",
                           0,
                           NULL,
                           AudEmu_Read_Procmem,
                           NULL);
    return 0;
}

static void __exit audio_ldvt_mod_exit(void)
{
    INT32 ret;

    dma_free_coherent(0, AFE_EXTERNAL_DRAM_SIZE, u4DramVirBase, u4DramPhyBase);

    ret = misc_deregister(&audio_ldvt_dev);
    if(ret)
    {
        printk("[audio_ldvt]: unregister audio driver failed\n");
    }
}

module_init(audio_ldvt_mod_init);
module_exit(audio_ldvt_mod_exit);

MODULE_AUTHOR("mediatek");
MODULE_DESCRIPTION("MT6589 TS Driver for LDVT");
MODULE_LICENSE("GPL");


