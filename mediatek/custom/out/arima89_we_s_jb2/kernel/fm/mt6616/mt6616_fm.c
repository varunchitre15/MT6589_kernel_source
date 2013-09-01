#include "mt6616_fm.h"

/******************************************************************************
 * CONSTANT DEFINITIONS
 *****************************************************************************/
//addr = read ? (addr | 0x1) : (addr & ~0x1);
#define MT6616_SLAVE_ADDR    0xE0  //0x70 7-bit address

//Global data for MT6616
#define MT6616_MAX_COUNT     100
#define MT6616_SCANTBL_SIZE  16 //16*uinit16_t

//FM_DAC_CON1(0x83)
#define FM_VOL_MAX             0x2B // 43 volume(0-15)


#define AFC_ON  0x01
#if AFC_ON
#define FM_MAIN_CTRL_INIT  0x480
#else
#define FM_MAIN_CTRL_INIT  0x080
#endif

uint32_t g_dbg_level = 0xfffffff4;
uint32_t g_stop_timer = 1;

static struct proc_dir_entry *g_fm_proc = NULL;
#define FM_PROC_FILE "fm"
//The following define need sync with board-xxx.c
enum {
    MT6616_BT = 0,
    MT6616_FM,
    MT6616_MAX
};

extern void mt6616_poweron(int idx);
extern void mt6616_poweroff(int idx);
extern void MT65XX_EINT_IRQ_UNMASK(unsigned int line);
extern void MT65XX_EINT_IRQ_MASK(unsigned int line);
extern void MT65XX_EINT_SET_HW_DEBOUNCE(kal_uint8 eintno, kal_uint32 ms);
extern kal_uint32 MT65XX_EINT_SET_SENSITIVITY(kal_uint8 eintno, kal_bool sens);
extern void MT65XX_EINT_REGISTRATION(kal_uint8 eintno, kal_bool Dbounce_En, 
                                     kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void), 
                                     kal_bool auto_umask);

struct ctrl_word_operation{
    uint16_t addr;
    uint16_t and;
    uint16_t or;
};

//USE interrupt for SEEK/SCAN/TUNE done
#define USE_INTERRUPT  1  //internal flag, but if disable interrupt mode,RDS also can't work.

/******************************************************************************
 * FUNCTION PROTOTYPES
 *****************************************************************************/
void  Delayms(uint32_t data);
void  Delayus(uint32_t data);
int   MT6616_read(struct i2c_client *client, uint8_t addr, uint16_t *val);
int   MT6616_write(struct i2c_client *client, uint8_t addr, uint16_t val);
int   MT6616_set_bits(struct i2c_client *client, uint8_t addr,
                               uint16_t bits, uint16_t mask);
static int  MT6616_Mute(struct i2c_client *client, bool mute);
static void MT6616_Tune(struct i2c_client *client, bool tune); //only HW tune
static void MT6616_RampDown(struct i2c_client *client);
static void MT6616_ADPLL_PowerUp(struct i2c_client *client);
static void MT6616_ADPLL_PowerDown(struct i2c_client *client);
static void MT6616_Freq_Avoid(struct i2c_client *client, uint16_t freq);
static bool MT6616_SetFreq(struct i2c_client *client, uint32_t freq); // functionality tune
static void MT6616_Tune_HiLo(struct i2c_client *client, 
                            uint16_t freq, uint16_t band, 
                            uint16_t space);
static bool MT6616_Seek(struct i2c_client *client, 
                        uint16_t min_freq, uint16_t max_freq, 
                        uint16_t *pFreq, 
                        uint16_t seekdir, 
                        uint16_t space);
static bool MT6616_Scan(struct i2c_client *client, 
                        uint16_t min_freq, uint16_t max_freq,
                        uint16_t *pFreq, //get the valid freq after scan
                        uint16_t *pScanTBL, 
                        uint16_t *ScanTBLsize, 
                        uint16_t scandir, 
                        uint16_t space);
static void MT6616_ScanForceStop(void);
static void MT6616_GetCurRSSI(struct i2c_client *client, uint16_t *pRSSI);
static void MT6616_SetVol(struct i2c_client *client, uint8_t vol);
static void MT6616_GetVol(struct i2c_client *client, uint8_t *pVol);
#if 0//def FMDEBUG
static void MT6616_dump_reg(struct i2c_client *client);
#endif
static bool MT6616_GetMonoStereo(struct i2c_client *client, uint16_t *pMonoStereo);
static bool MT6616_GetCurPamd(struct i2c_client *client, uint16_t *pPamdLevl);
static bool MT6616_em_test(struct i2c_client *client, uint16_t group_idx, uint16_t item_idx, uint32_t item_value);

#if USE_INTERRUPT
static void fm_enable_eint(void);
static void fm_disable_eint(void);
static void fm_eint_handler(void);
static void fm_request_eint(void *data);  
static void fm_workqueue_func(struct work_struct *work);

#define MT6616_FM_STC_DONE_TIMEOUT 12  //second
static bool fm_stc_done = false;
static wait_queue_head_t fm_stc_done_wait;
static bool fm_wait_stc_done(uint32_t sec);
#endif

#if FM_RDS_ENABLE
extern uint32_t gBLER_CHK_INTERVAL;
extern uint16_t GOOD_BLK_CNT, BAD_BLK_CNT;
extern uint8_t  BAD_BLK_RATIO;
static struct   timer_list fm_rds_reset_timer;
static void fm_rds_reset_func(unsigned long data);
static void fm_enable_rds_BlerCheck(struct fm *fm);
static void fm_disable_rds_BlerCheck();
static void fm_rds_reset_workqueue_func(struct work_struct *work);
#endif   
                           
static int  fm_setup_cdev(struct fm *fm);
static int  fm_ops_ioctl(struct inode *inode, struct file *filp,
                           unsigned int cmd, unsigned long arg);
static loff_t fm_ops_lseek(struct file *filp, loff_t off, int whence);
static ssize_t fm_ops_read(struct file *filp, char *buf, size_t len, loff_t *off);
static ssize_t fm_ops_write(struct file *filp, const char *buf, size_t len, loff_t *off);
static int  fm_ops_open(struct inode *inode, struct file *filp);
static int  fm_ops_release(struct inode *inode, struct file *filp);
            
static int  fm_init(struct i2c_client *client);
static int  fm_destroy(struct fm *fm);
static int  fm_powerup(struct fm *fm, struct fm_tune_parm *parm);
static int  fm_powerdown(struct fm *fm);
static int  fm_tune(struct fm *fm, struct fm_tune_parm *parm);
static int  fm_seek(struct fm *fm, struct fm_seek_parm *parm);
static int  fm_scan(struct fm *fm, struct fm_scan_parm *parm);
static int  fm_setvol(struct fm *fm, uint32_t vol);
static int  fm_getvol(struct fm *fm, uint32_t *vol);
static int  fm_getrssi(struct fm *fm, uint32_t *rssi);
static int fm_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data);
static int fm_proc_write(struct file *file, const char *buffer, unsigned long count, void *data);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))          
static int  fm_i2c_attach_adapter(struct i2c_adapter *adapter);
static int  fm_i2c_detect(struct i2c_adapter *adapter, int addr, int kind);
static int  fm_i2c_detach_client(struct i2c_client *client);
#else
static int fm_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int fm_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int fm_i2c_remove(struct i2c_client *client);
#endif

/******************************************************************************
 * GLOBAL DATA
 *****************************************************************************/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
/* Addresses to scan */
static unsigned short normal_i2c[] = {MT6616_SLAVE_ADDR, I2C_CLIENT_END};
static unsigned short ignore = I2C_CLIENT_END;

static struct i2c_client_address_data mt6616_addr_data = {
    .normal_i2c = normal_i2c,
    .probe = &ignore,
    .ignore = &ignore,
};
#else
static const struct i2c_device_id fm_i2c_id =
{MT6616_DEV, 0};
static unsigned short force[] = {MT6616_I2C_PORT, MT6616_SLAVE_ADDR, I2C_CLIENT_END, I2C_CLIENT_END};
static const unsigned short * const forces[] = {force, NULL};
static struct i2c_client_address_data addr_data = {.forces = forces};
#endif


static struct i2c_driver MT6616_driver = {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
    .driver = {
        .owner = THIS_MODULE,
        .name = MT6616_DEV,
    },
    .attach_adapter = fm_i2c_attach_adapter,
    .detach_client = fm_i2c_detach_client,
#else
    .probe = fm_i2c_probe,
    .remove = fm_i2c_remove,
    .detect = fm_i2c_detect,
    .driver.name = MT6616_DEV,
    .id_table = &fm_i2c_id,
    .address_data = &addr_data,
#endif
};



/* FM Receiver Power Up */
static struct ctrl_word_operation PowerOnSetting[] = {
//A00:Switch to FM LDO
#if 0 // defined(ext_clk)
    { 0x28,   0, 0x2000 },
    { 0xFFFB, 0, 0x000A }, //wait 10ms
    { 0x28,   0, 0x3000 },
#endif
    { 0x25,   0, 0x1A00 },
    { 0xFFFA, 0, 0x0001 }, //wait 1us
    { 0x25,   0, 0x1A10 },
//A0:Turn on 4.3V Bandgap and 4.3V to 2.8V LDO
    { 0x25,   0, 0x1A90 },
    { 0x25,   0, 0x1AD0 },
    { 0xFFFA, 0, 0x001E },  //wait 30us
    { 0x25,   0, 0x1A90 },
    { 0xFFFB, 0, 0x0001 }, //wait 1ms
//A1:Turn on BIAS_CORE circuit and output bandgap voltage
    { 0x0A,   0, 0xF614 },
    { 0xFFFA, 0, 0x001E },  //wait 30us
    { 0x01,   0, 0x0002 },
    { 0xFFFA, 0, 0x0032 },  //wait 50us
//A2:Turn on all the LDOS of FM circuits
    { 0x01,   0, 0x804A },
    { 0xFFFA, 0, 0x000A },  //wait 10us
//A3:Turn on all the FM receiver circuits
    { 0x00,   0, 0x01F6 },
//A5:FM RF Optimize setting
    { 0x09,   0, 0x7B56 },
    { 0x02,   0, 0x6CBE },
    { 0x07,   0, 0xD968 },
    { 0x24,   0, 0x5080 },
    { 0x0B,   0, 0x80E5 },
    { 0x05,   0, 0xC14B },
    { 0x02,   0, 0x6CBF },
    { 0x04,   0, 0x1238 },
    { 0x25,   0, 0x5A96 },
    { 0x0E,   0, 0x079C },
    { 0x10,   0, 0xFD90 },
    { 0x11,   0, 0x0270 },
    { 0x12,   0, 0xFD90 },
    { 0x13,   0, 0x0270 },
    { 0x0E,   0, 0x0794 },
//FM 32K Power Up 
#if defined(ext_clk)
    { 0x24,   0, 0x50C0 },
    { 0x0B,   0, 0x80E4 },
#else
    { 0x24,   0, 0x5040 },
    { 0x01,   0, 0x806A },
    { 0xFFFA, 0, 0x000A }, //wait 10us
    { 0x00,   0, 0x09F6 },
    { 0xFFFB, 0, 0x03E8 }, // delay 1000ms
    { 0x00,   0, 0x0DF6 },
    { 0xFFFB, 0, 0x0001 }, // delay 1ms
#endif
//FM ADPLL Power Up
#if defined(APLL_Cal_Open_Loop_Mode)
#else
    { 0x21,   0, 0x8880 },
    { 0x1D,   0, 0x0861 },
    { 0x1D,   0, 0x0863 },
    { 0xFFFB, 0, 0x0003 }, /// delay 3ms
    { 0xFFFF, 0x0022, 0x8000 }, /// Polling FMR2D_DCO_CAL_STATUS=1
    { 0x1D,   0, 0x0861 },
    { 0x1D,   0, 0x0865 },
    { 0xFFFB, 0, 0x0003 }, /// delay 3ms
    { 0xFFFF, 0x0022, 0x8000 }, /// Polling FMR2D_DCO_CAL_STATUS=1
    { 0x1D,   0, 0x0861 },
    { 0x1D,   0, 0x0871 },
    { 0xFFFB, 0, 0x0064 }, /// delay 500ms->100ms, modify by lichunhui
    { 0x28,   0, 0x3E00 },
#endif
//FM RC Calibration
    { 0x00,   0, 0x03F6 },
    { 0xFFFA, 0, 0x0001 },  //wait 1us
    { 0x19,   0, 0x00CB },
    { 0xFFFA, 0, 0x00C8 },  //wait 200us
    { 0x19,   0, 0x00CA },
#if defined(ext_clk)
    { 0x00,   0, 0x01F6 },
#else
    { 0x00,   0, 0x0DF6 },
#endif
//FM VCO Enable
#if defined(ext_clk)
    { 0x00,   0, 0x81F6 },
#else
    { 0x00,   0, 0x8DF6 },
#endif
    { 0xFFFB, 0, 0x0002 }, /// delay 2ms
#if defined(ext_clk)
    { 0x01,   0, 0x805E },
    { 0x00,   0, 0x91F6 },
#else
    { 0x01,   0, 0x807E },
    { 0x00,   0, 0x9DF6 },
#endif
    { 0xFFFA, 0, 0x000A },  //wait 10us
#if defined(ext_clk)
    { 0x00,   0, 0xF1F6 },
#else
    { 0x00,   0, 0xFDF6 },
#endif
//FM DCOC Calibration
    { 0x30,   0, 0x0006 },
    { 0x34,   0, 0x0001 }, /// delay 23ms
    { 0x33,   0, 0x0020 },
    { 0x83,   0, 0x0AC3 },
    { 0x3A,   0, 0x0001 },
    { 0x3B,   0, 0x0001 },
    { 0x0C,   0, 0x1A08 },
    { 0x33,   0, 0x0021 },
    { 0xFFFF, 0x39, 0x0001 }, /// Polling fm_intr_stc_done=1
    { 0x39,   0, 0x0001 },
    { 0x33,   0, 0x0000 },
    { 0xFFFE, 0x3F, 0x0001 }, /// Polling stc_done=0
//FM Digital Init Part 1
    { 0x30,   0, 0x0006 },
    { 0xFFFD, 0x32, 0   }, ///Read fm_hw_ver
    { 0x34,   0, 0x0001 },
    { 0x33,   0, 0x0480 },
    { 0x83,   0, 0x0AC3 },
    { 0x37,   0, 0x0000 }, /// CW55  [9:0] lower bound
    { 0x36,   0, 0x2800 }, /// CW54  [9:0] upper bound
    { 0x3A,   0, 0x0002 },
    { 0x3B,   0, 0x0002 },
    { 0x1C,   0, 0x285B },
//Enable IQ Cal Tone for low channel
    { 0x28,   0, 0x3E01 }, /// CW40  [0] 1
    { 0xFFFB, 0, 0x0002 }, /// delay 2ms
    { 0x28,   0, 0x3E83 }, /// CW40  [7] 1,[1] 1
    { 0xFFFB, 0, 0x0001 }, /// delay 1ms
    { 0x28,   0, 0x3EA3 }, /// CW40  [5] 1,[4] 0
    { 0x28,   0, 0x3EAB }, /// CW40  [3] 1
    { 0x26,   0, 0x7C02 }, /// CW38  [14:10] 1 [1] 1
    { 0x00,   0, 0xF1FE }, /// CW0  [3] 1
    { 0xFFFB, 0, 0x000A }, /// delay 10ms
//Power-on IQ cal for low channel
    { 0x0C,   0, 0x1980 }, /// CW12  Set CHAN 83.2KHZ
    { 0x33,   0, 0x0488 }, /// CW51  [3] 1
    { 0xFFFF, 0x39, 0x0002 }, /// Polling IQCAL_DONE status=1
    { 0x39,   0, 0x0002 }, /// Clear IQCAL_DONE status flag
    { 0x2F,   0, 0x0001 }, /// CW47  [1:0] 1 Page Num=1
    { 0xFFFD, 0x0048, 0x0042 }, /// Readback eps_opt and phi_opt, write to seg1_eps&seg1_phi
    { 0x45,   0, 0x4000 }, /// [14] 1,Set rgf_iqcal_clr = 1
    { 0x45,   0, 0x0000 }, /// [14] 0,Set rgf_iqcal_clr = 0
    { 0x2F,   0, 0x0000 }, /// CW47  [1:0] 0 Page Num=0
    { 0x33,   0, 0x0480 }, /// CW51  [3] 0
//Enable IQ Cal Tone for high channel
    { 0x28,   0, 0x3E83 }, /// CW40  [5] 0,[3] 0
    { 0x28,   0, 0x3EB3 }, /// CW40  [5] 1,[4] 1
    { 0x28,   0, 0x3EBB }, /// CW40  [3] 1
//Power-on IQ cal for high channel
    { 0x0C,   0, 0x1B20 }, /// CW12  Set CHAN 104KHZ
    { 0x33,   0, 0x0488 }, /// CW51  [3] 1
    { 0xFFFF, 0x39, 0x0002 }, /// Polling IQCAL_DONE status=1
    { 0x39,   0, 0x0002 }, /// Clear IQCAL_DONE status flag
    { 0x2F,   0, 0x0001 }, /// CW47  [1:0] 1 Page Num=1
    { 0xFFFD, 0x0048, 0x0043 }, /// Readback eps_opt and phi_opt, write to seg2_eps&seg2_phi
    { 0x45,   0, 0x4000 }, /// [14] 1,Set rgf_iqcal_clr = 1
    { 0x45,   0, 0x0000 }, /// [14] 0,Set rgf_iqcal_clr = 0
    { 0x2F,   0, 0x0000 }, /// CW47  [1:0] 0 Page Num=0
    { 0x33,   0, 0x0480 }, /// CW51  [3] 0
//Disable IQ Cal Tone
    { 0x28,   0, 0x3E83 }, /// CW40  [5] 0,[3] 0
    { 0x28,   0, 0x3E01 }, /// CW40  [7] 0,[1] 0
    { 0xFFFB, 0, 0x0001 }, /// delay 1ms
    { 0x28,   0, 0x3E00 }, /// CW40  [0] 0
    { 0x26,   0, 0x0000 }, /// CW38  [15:0] 0
 #if defined(ext_clk)
    { 0x00,   0, 0xF1F7 }, /// CW00  [3] 0
 #else
    { 0x00,   0, 0xFDF7 }, /// CW00  [3] 0
 #endif
    { 0xFFFB, 0, 0x0032}, /// delay 50ms
//FM Digital Init: fm_rgf_maincon
    { 0x3A,   0, 0x0    },
    { 0x3B,   0, 0x0    },
    { 0x2F,   0, 0x0001 }, /// CW47  [1:0] 1 Page Num=1
    { 0x42,   0x7FFF, 0x8000 }, /// CW66  [15] 1
    { 0x44,   0, 0xF208 }, /// CW68  [9:0] 0X208=90MHZ
    { 0x2F,   0, 0x0000 }, /// CW47  [1:0] 1 Page Num=1
    { 0x33,   0, FM_MAIN_CTRL_INIT }, /// CW51  [6] 1
    { 0x4F,   0, 0x0000 }, /// CW79  [2] 0
    { 0x0A,   0, 0xF414 }, /// CW10  [9] 0
    { 0x3D,   0, 0x1AB2 }, /// CW51  [6] 1
//FM Digital Init: fm_rgf_dac
    { 0x2F,   0, 0x0000 }, /// CW47  [1:0] 1 Page Num=1
    { 0x83,   0, 0x1AEB }, /// CW51  [6] 1
//FM Digital Init: fm_rgf_front
    { 0x2F,   0, 0x0000 }, /// CW47  [1:0] 1 Page Num=1
    { 0x40,   0, 0x1A44 },
    { 0x5A,   0, 0x0CF4 },
    { 0x51,   0, 0x002B },
    { 0x60,   0, 0x0301 },
    { 0x63,   0, 0x0028 },
    { 0x81,   0, 0xFF00 },
    { 0x41,   0, 0x2E2E },
    { 0x61,   0, 0x03E7 },
    { 0x79,   0, 0x1761 },  
    { 0x26,   0, 0x0002 },
    { 0x4F,   0, 0x0010 },
    { 0x49,   0, 0x006A },
    { 0x4A,   0, 0x0015 },
    { 0x4B,   0, 0x03C2 },
    { 0x4C,   0, 0x0069 },
    { 0x4D,   0, 0x001A },
    { 0x4E,   0, 0x03C1 },
    { 0x73,   0, 0x0001 },
    { 0x48,   0, 0x0030 },
    { 0x58,   0, 0x2302 },
    { 0x54,   0, 0x007D },
    { 0x50,   0, 0x0886 },
    { 0x65,   0, 0x0806 },
    { 0x66,   0, 0x0C0A },
    { 0x67,   0, 0x100E },
    { 0x68,   0, 0x1412 },
    { 0x69,   0, 0xB416 },
    { 0x59,   0, 0x0005 },
    { 0x55,   0, 0x1FF2 },
    { 0x62,   0, 0x0394 },
    { 0x64,   0, 0x03DC },
    { 0x56,   0, 0x01FF },
    { 0x57,   0, 0x00BC },
    { 0x47,   0, 0x0016 },
    { 0x2F,   0, 0x0001 },
    { 0x7F,   0, 0x03E7 },
    { 0x80,   0, 0x047B },
    { 0x62,   0, 0x0192 },
    { 0x63,   0, 0x0086 },
    { 0x68,   0, 0x0100 },
    { 0x69,   0, 0x0055 },
    { 0x6E,   0, 0x0100 },
    { 0x6F,   0, 0x0055 },
    { 0x72,   0, 0x5A00 },
    { 0x73,   0, 0xBE9B },
    { 0x64,   0, 0x0302 },
    { 0x6A,   0, 0x03DB },
    { 0x70,   0, 0x03DB },
    { 0x76,   0, 0x3888 },
    { 0x75,   0, 0x0002 },
    { 0x65,   0, 0x0003 },
    { 0x6B,   0, 0x03F8 },
    { 0x71,   0, 0x03EE },
    { 0x61,   0, 0x0024 },
    { 0x67,   0, 0x0025 },
    { 0x6D,   0, 0x03DC },
    { 0x81,   0, 0x00CD },
    { 0x82,   0, 0x01B1 },
    { 0x60,   0, 0x007D },
    { 0x66,   0, 0x007D },
    { 0x6C,   0, 0x007D },
//FM Digital Init: fm_rgf_fmx
    { 0x2F,   0, 0x0000},
    { 0xEF,   0, 0x0031 },
    { 0xF0,   0, 0x0002 },
    { 0xF5,   0, 0x00D6 },
    { 0xDD,   0, 0x3C2F },
    { 0xEA,   0, 0x3BCF },
    { 0xE3,   0, 0x0D9C },
    { 0xEB,   0, 0x0B6A },
    { 0xEC,   0, 0x0B74 },
    { 0xF7,   0, 0x1000 },
    { 0xE7,   0, 0x0400 },
    { 0x2F,   0, 0x0001 },
    { 0xBF,   0, 0xD948 },
    { 0xD1,   0, 0x3823 },
    { 0xD3,   0, 0x034F },
    { 0x2F,   0, 0x0003 },
    { 0xE7,   0, 0x27D2 },
    { 0xD1,   0, 0x0BA1 },
    { 0xD0,   0, 0x0B9E },
    { 0xCF,   0, 0x0B9C },
    { 0xCE,   0, 0x0B9A },
    { 0xCD,   0, 0x0B98 },
    { 0xCC,   0, 0x0B96 },
    { 0xCB,   0, 0x0B94 },
    { 0xCA,   0, 0x0B92 },
    { 0xC9,   0, 0x0B90 },
    { 0xC8,   0, 0x0B8E },
    { 0xC7,   0, 0x0B8C },
    { 0xC6,   0, 0x0B8A },
    { 0xC5,   0, 0x0B88 },
    { 0xC4,   0, 0x0B86 },
    { 0xC3,   0, 0x0B84 },
    { 0xC2,   0, 0x0B82 },
    { 0xC1,   0, 0x0B80 },
    { 0xC0,   0, 0x0B7E },
    { 0xBF,   0, 0x0B7C },
    { 0xBE,   0, 0x0B7A },
    { 0xBD,   0, 0x0B78 },
    { 0x2F,   0, 0x0000 },
    { 0xE9,   0, 0x0101 },
    { 0xBD,   0, 0x0562 },
    { 0x2F,   0, 0x0001 },
    { 0xD8,   0, 0x0000 },
//pamd offset table for blend
    { 0x2F,   0, 0x0002 },
    { 0xBD,   0, 0x00E8 },
    { 0xBE,   0, 0xFF28 },
    { 0xBF,   0, 0xFF28 },
    { 0xC0,   0, 0x00B8 },
    { 0xC1,   0, 0x0032 },
    { 0xC2,   0, 0x0032 },
    { 0xC3,   0, 0x0099 },
    { 0xC4,   0, 0x0019 },
    { 0xC5,   0, 0x0019 },
    { 0xC6,   0, 0x006D },
    { 0xC7,   0, 0x0035 },
    { 0xC8,   0, 0x0035 },
//pamd offset table for hcc
    { 0x2F,   0, 0x0002 },
    { 0xD5,   0, 0x0098 },
    { 0xD6,   0, 0xFF4E },
    { 0xD7,   0, 0xFF4E },
    { 0xD8,   0, 0x006D },
    { 0xD9,   0, 0x0040 },
    { 0xDA,   0, 0x0040 },
    { 0xDB,   0, 0x000B },
    { 0xDC,   0, 0x001E },
    { 0xDD,   0, 0x001E },
    { 0xDE,   0, 0x0028 },
    { 0xDF,   0, 0x0006 },
//dylim gain (set to defaul value)
    { 0xF0,   0, 0x2AC4 },
    { 0xE0,   0, 0x0006 },
    { 0xE1,   0, 0x014A },
    { 0xE2,   0, 0xFF83 },
    { 0xE3,   0, 0xFC75 },
    { 0xE4,   0, 0x0129 },
    { 0xE5,   0, 0xFFFC },
    { 0xE6,   0, 0xFE67 },
    { 0xE7,   0, 0x0119 },
    { 0xE8,   0, 0x0005 },
    { 0xE9,   0, 0xFED3 },
//rssi offset table for blend
    { 0x2F,   0, 0x0001 },
    { 0xC4,   0, 0xFFCB },
    { 0xC5,   0, 0xFFCB },
    { 0xC6,   0, 0x00FD },
    { 0xC9,   0, 0xFFF7 },
    { 0xCA,   0, 0x0008 },
    { 0xCB,   0, 0x0008 },
    { 0xCC,   0, 0xFFF6 },
//rssi offset table for hcc
    { 0x2F,   0, 0x0001 },
    { 0xDB,   0, 0x0008 },
    { 0xDC,   0, 0xFFD2 },
    { 0xDD,   0, 0xFFD2 },
    { 0xDE,   0, 0x0003 },
    { 0xDF,   0, 0xFFFD },
    { 0xE0,   0, 0xFFFD },
    { 0xE1,   0, 0x0002 },
    { 0xE2,   0, 0x0003 },
    { 0xE3,   0, 0x0003 },
    { 0xE4,   0, 0xFFFC },
    { 0xE7,   0, 0x0013 },
    { 0xE8,   0, 0x0013 },
    { 0xE9,   0, 0x0013 },
    { 0xEA,   0, 0x001C },
    { 0xEB,   0, 0x001C },
    { 0xEC,   0, 0x001C },
    { 0xED,   0, 0x0002 },
    { 0xEE,   0, 0x0002 },
    { 0xEF,   0, 0x0002 },
//last softmute gain
    { 0xD1,   0, 0x3823 },
//softmute threshold
    { 0x2F,   0, 0x0003 },
    { 0xD2,   0, 0x3FFF },
    { 0xD3,   0, 0x3FFF },
    { 0xD4,   0, 0x3FFF },
    { 0xD5,   0, 0x3FFF },
    { 0xD6,   0, 0x3FFF },
    { 0xD7,   0, 0x3FFF },
    { 0xD8,   0, 0x3FFF },
    { 0xD9,   0, 0x3CE6 },
    { 0xDA,   0, 0x3CDE },
    { 0xDB,   0, 0x3CD6 },
    { 0xDC,   0, 0x3CCE },
    { 0xDD,   0, 0x3CC6 },
    { 0xDE,   0, 0x3CBE },
    { 0xDF,   0, 0x3CB6 },
    { 0xE0,   0, 0x3CAE },
    { 0xE1,   0, 0x3CA6 },
    { 0xE2,   0, 0x3C9E },
    { 0xE3,   0, 0x3C96 },
    { 0xE4,   0, 0x3C8E },
    { 0xE5,   0, 0x3C86 },
    { 0xE6,   0, 0x3C7f },
//solve SSR variation issue
    { 0x2F,   0, 0x0000 },
    //{ 0xE2, 0, 0x3E3C },
    { 0xF1,   0, 0x1F42 },
    { 0xF2,   0, 0x218F },
    //{ 0x2F, 0, 0x0001 },
    //{ 0xFE, 0, 0x313D },
//Set search threshold
    { 0x2F,   0, 0x0001 },
    { 0xD1,   0, 0x3810 }, //resolve invalid channel voice.
    { 0x7F,   0, 0x03e9 },
    { 0x2F,   0, 0x0000 },
    { 0x61,   0, 0x03e9 },  
//FM Digital Init: fm_rgf_rds
    { 0x2F,   0, 0x0000 },
    { 0x9A,   0, 0x0080 },
    { 0x97,   0, 0x0001 },
}; 

/* FM Receiver Power down */
static struct ctrl_word_operation PowerOffProc[] = {
//Digital Modem Power Down
    {0x38,    0, 0x0000 }, /// CW56 [1:0] 0
    {0x2F,    0, 0x0001 }, /// CW51 [3:0]    0
    {0x4A,    0, 0x4000 }, /// CW48   [0]    1
    {0x2F,    0, 0x0    }, /// CW62  [1:0]  0
    {0x33,    0, 0x0    }, /// CW62  [1:0]  0
    {0x32,    0, 0x0007 }, /// CW62  [1:0]  0
    {0x3E,    0xFFFC, 0 }, /// CW62  [1:0]  0
    {0x3E,    0xFFFC, 0 }, /// CW62  [1:0]  0
    {0x3E,    0xFFFC, 0 }, /// CW62  [1:0]  0
    {0x3E,    0xFFFC, 0 }, /// CW62  [1:0]  0
    {0x28,    0, 0x3000 }, /// CW62  [1:0]  0
    {0x1D,    0, 0x0860 }, /// CW40 [9] 0
    {0x1E,    0, 0x0400 }, /// CW29 [7][4][0] 0
    {0x1E,    0, 0x0480 }, /// CW30 [7] 0
    {0x00,    0, 0x0000 }, /// CW30 [7] 1
    {0x01,    0, 0x0000 }, /// CW30 [7] 1
    {0x0A,    0, 0x7205 }, /// CW00 [15] 0
    {0x25,    0, 0x1A10 }, /// CW00 [15] 0
#if defined(ext_clk)
    {0x28,    0, 0x1000 }, /// CW00 [15] 0
    {0xFFFB,  0, 0x000A }, /// delay 10ms
    {0x28,    0, 0x3000 }, /// CW00 [15] 0
#endif
    {0x25,    0, 0x1A00 }, /// CW00 [15] 0
    {0xFFFA,  0, 0x0001 },  //wait 1us , LCH modify
    {0x25,    0, 0x1E00 }, /// CW00 [15] 0
};

#define POWER_ON_COMMAND_COUNT (sizeof(PowerOnSetting)/sizeof(PowerOnSetting[0]))
#define POWER_OFF_COMMAND_COUNT (sizeof(PowerOffProc)/sizeof(PowerOffProc[0]))
int16_t _current_frequency = 0xffff;
static volatile bool g_bStopScan = false;
static struct fm *g_fm_struct = NULL;


static struct file_operations fm_ops = {
    .owner = THIS_MODULE,
    .ioctl = fm_ops_ioctl,
    .llseek = fm_ops_lseek,
    .read = fm_ops_read,
	.write = fm_ops_write,
    .open = fm_ops_open,
    .release = fm_ops_release,
};

static DECLARE_MUTEX(fm_ops_mutex);
static DECLARE_MUTEX(fm_read_mutex);
static DECLARE_MUTEX(fm_timer_mutex);

/******************************************************************************
 *****************************************************************************/

/*
static void _open_audio_path(void)
{

    volatile uint *ANA_VOL = ( volatile uint *)0xF0060200;
    volatile uint *ANA_REG = ( volatile uint *)0xF0060204;
    volatile uint *ANA_PWR = ( volatile uint *)0xF0060208;

    FM_DEBUG("<= 0x%X=0x%X,0x%X=0x%X, 0x%X=0x%X\n",
        (uint)ANA_REG, *ANA_REG,
        (uint)ANA_VOL, *ANA_VOL,
        (uint)ANA_PWR, *ANA_PWR);

    *ANA_REG = 0x480; // set audio path to wired headset
    *ANA_PWR = 0x1F; // power all on

    FM_DEBUG("=> 0x%X=0x%X,0x%X=0x%X, 0x%X=0x%X\n",
        (uint)ANA_REG, *ANA_REG,
        (uint)ANA_VOL, *ANA_VOL,
        (uint)ANA_PWR, *ANA_PWR);
}
*/

/******************************************************************************
 *****************************************************************************/

/*
 *  delay ms
 */
void Delayms(uint32_t data)
{    
    //msleep(data);
    udelay(data*1000);
}

/*
 *  delay us
 */
void Delayus(uint32_t data)
{
    udelay(data);   
}

/*
 *  MT6616_read
 */
int MT6616_read(struct i2c_client *client, uint8_t addr, uint16_t *val)
{
    int n;
    char b[2] = {0};

    // first, send addr to MT6616
    n = i2c_master_send(client, (char*)&addr, 1);
    if (n < 0)
    {
        WCN_DBG(L3|D_ALL, "MT6616_read send:0x%X err:%d\n", addr, n);
        return -1;
    }

    // second, receive two byte from MT6616
    n = i2c_master_recv(client, b, 2);
    if (n < 0)
    {
         WCN_DBG(L3|D_ALL, "MT6616_read recv:0x%X err:%d\n", addr, n);
        return -1;
    }

    *val = ((uint16_t)b[0] << 8 | (uint16_t)b[1]);

    return 0;
}

/*
 *  MT6616_write
 */
int MT6616_write(struct i2c_client *client, uint8_t addr, uint16_t val)
{
    int n;
    char b[3];

    b[0] = addr;
    b[1] = (char)(val >> 8);
    b[2] = (char)(val & 0xFF);

    n = i2c_master_send(client, b, 3);
    if (n < 0)
    {
         WCN_DBG(L3|D_ALL, "MT6616_write send:0x%X err:%d\n", addr, n);
        return -1;
    }

    return 0;
}

int MT6616_set_bits(struct i2c_client *client, uint8_t addr,
                             uint16_t bits, uint16_t mask)
{
    int err;
    uint16_t val;

    err = MT6616_read(client, addr, &val);
    if (err)
        return err;

    val = ((val & (mask)) | bits);

    err = MT6616_write(client, addr, val);
    if (err)
        return err;

    return 0;
}

static int MT6616_Mute(struct i2c_client *client, bool mute)
{
    int ret = 0;
    
    if(mute) {
        ret = MT6616_set_bits(client, 0x0A, 0x0200, 0xFFFF);
    }
    else
    {
        ret = MT6616_set_bits(client, 0x0A, 0x0, 0xFDFF);
    }
    return ret;
}

static void MT6616_Tune(struct i2c_client *client, bool tune)
{
    
    if(tune) {
        MT6616_set_bits(client, FM_MAIN_CTRL, 0x0001, 0xFFFE);
    }
    else
    {
        MT6616_set_bits(client, FM_MAIN_CTRL, 0x0, 0xFFFE);
    }
}


static void MT6616_RampDown(struct i2c_client *client)
{
    int cnt = 0; 
    uint16_t tmp_reg;

    MT6616_set_bits(client, FM_MAIN_CTRL, 0x0, 0xFFF0);	//clear rgf_tune/seek/scan/rxcal

    MT6616_write(client, FM_MAIN_PGSEL, 0x01);
	MT6616_read(client, FM_CHNLSCAN_STAT, &tmp_reg);
  	while(tmp_reg & 0xFF00)
  	{
		MT6616_read(client, FM_CHNLSCAN_STAT, &tmp_reg);
	    Delayms(1);
	    cnt++;
	    if(cnt > MT6616_MAX_COUNT)
	    {
	        WCN_DBG(L3|D_ALL, "MT6616_RampDown failed\n");
	        break;
        }		
  	}
  	MT6616_write(client, FM_MAIN_PGSEL, 0x0);

	//Enable mute
	MT6616_set_bits(client, FM_DAC_CON1, 0x0003, 0xFFFF);
    //Set softmute to fast mode
    MT6616_write(client, FM_MAIN_PGSEL, 0x0);
    MT6616_write(client, 0xE9, 0x0101);
    MT6616_write(client, 0xBD, 0x0562);
    MT6616_write(client, FM_MAIN_PGSEL, 0x0001);
    MT6616_write(client, 0xD8, 0x0000);
    MT6616_write(client, FM_MAIN_PGSEL, 0x0);
    
}

static void MT6616_ADPLL_PowerUp(struct i2c_client *client)
{
    int cnt = 0;
    uint16_t tmp_reg, f16mode_enable;
     
    // change DLF loop gain  
    // Set FMCR_DLF_GAIN_A = "8"
    // Set FMCR_DLF_GAIN_B = "8"
    MT6616_write(client, 0x21, 0x8880);
    
    // C1. Enable ADPLL DCO	Set
    // FMCR_DCO_EN = 
    MT6616_read(client, 0x1D, &tmp_reg);
    f16mode_enable = tmp_reg&0x0200;
    MT6616_write(client, 0x1D, (f16mode_enable|0x0861)); 
    
    // C2. Turn on coarse calibration
    // Set FMCR_COARSE_EN = 
    MT6616_write(client, 0x1D, (f16mode_enable|0x0863));    
    // wait 10ms 
    Delayms(10);     

    // C3. Check DCO calibration status	
    // Poll FMR2D_DCO_CAL_STATUS = "1"
    cnt = 0;
    do
    {        
        MT6616_read(client, 0x22, &tmp_reg);
        if(0 == (tmp_reg&0x8000))
        {
            cnt++;
	        if(cnt > MT6616_MAX_COUNT) {
	             WCN_DBG(L3|D_ALL, "MT6616_ADPLL_PowerUp failed\n");
	            break;
            }
        }		
    }while (0 == (tmp_reg&0x8000));
    
    // C4. Turn on fine_A calibration
    // Set FMCR_CAL_COARSE_EN = "0"
    MT6616_write(client, 0x1D, f16mode_enable|0x0861);
    // Set FMCR_FINE_A_EN = "1"
    MT6616_write(client, 0x1D, f16mode_enable|0x0865);
    // wait 10ms 
    Delayms(10);
    
    // C5. Check DCO calibration status
    // Poll FMR2D_DCO_CAL_STATUS = "1"  
    cnt = 0;  
    do
    {        
        MT6616_read(client, 0x22, &tmp_reg);
        if(0 == (tmp_reg&0x8000))
        {
            cnt++;
	        if(cnt > MT6616_MAX_COUNT) {
	             WCN_DBG(L3|D_ALL, "MT6616_ADPLL_PowerUp failed\n");
	            break;
            }
        }	
    }while (0x0 == (tmp_reg&0x8000));
    
    // C6. Enable Close-loop mode
    // Set FMCR_FINE_A_EN = "0"
    MT6616_write(client, 0x1D, f16mode_enable|0x0861);
    // Set FMCR_PLL_EN = "1"
    MT6616_write(client, 0x1D, f16mode_enable|0x0871);
    
    // C9. Disable fm adc ck top clock gating
    // Set rgfrf_top_ck = "1"
    MT6616_write(client, 0x28, 0x3200); 
    
    
}

static void MT6616_ADPLL_PowerDown(struct i2c_client *client)
{
    uint16_t tmp_reg;
    
    // 2. ADPLL Power Off Sequence
    // Set rgfrf_top_ck = "0"
    MT6616_set_bits(client, 0x28, 0x0, (~0x0200));    
    
    // Set FMCR_OPEN_LOOP_EN = "0"
    // Set FMCR_PLL_EN = "0"
    // Set FMCR_DCO_EN = "0"
    MT6616_set_bits(client, 0x1D, 0x0, (~0x0091));
          
    // Set rgfrf_adpll_reset_n = "0"
    MT6616_set_bits(client, 0x1E, 0x0, (~0x0080));
    MT6616_read(client, 0x1E, &tmp_reg);
    tmp_reg &= ~0x0080;        
    MT6616_write(client, 0x1E, tmp_reg);               
    // Set rgfrf_adpll_reset_n = "1"
    tmp_reg |= 0x0080;                
    MT6616_write(client, 0x1E, tmp_reg);  
}

static void MT6616_Freq_Avoid(struct i2c_client *client, uint16_t freq)
{
    bool isADPLL_16M = 0;
    uint16_t tmp_reg, indx;
    static uint16_t Avoid_Channels[] = {
                                      767, 768, 769, 770, 806, 807, 808, 844, 845, 846,
                                      883, 884, 920, 921, 922, 923, 960, 961, 998, 999,
                                      1000, 1036, 1037, 1038, 1074, 1075, 1076, 1077};

    for(indx = 0; indx < sizeof(Avoid_Channels)/sizeof(Avoid_Channels[0]); indx++)
    {
  	    if(Avoid_Channels[indx] == freq)
  	    {
		    isADPLL_16M = true;
            break;
        }
    }
    
    //isADPLL_16M = 1;
    MT6616_read(client, 0x1D, &tmp_reg);
    if(((tmp_reg&0x0200)&&isADPLL_16M) || (!(tmp_reg&0x0200)&&(!isADPLL_16M)))
    { 
        return;
    }
    //Set rgf_fm_mclk_sel = 0
    MT6616_write(client, 0x38, 0);
  
    //Set rgf_auto_mclk_sel_en = 0 (only if muclk desense option is enabled)
    /*MT6616_write(client, FM_MAIN_PGSEL, 0x01);
      MT6616_set_bits(client, 0x4A, 0x0, 0xF7FF); 
      MT6616_write(client, FM_MAIN_PGSEL, 0x0);*/
  
    // Set rgf_pdn_fm = 1
    MT6616_set_bits(client, FM_MAIN_CG, 0x0, 0x0007); 
   
    //Set rgfrf_enable_top_ck ="0"
    MT6616_set_bits(client, 0x28, 0x0, 0xFDFF);
  
    // Disable ADPLL
    MT6616_ADPLL_PowerDown(client);
  
    //Set FMCR_DCO_CK_SEL = ? (default = 0, 15.36)
    MT6616_read(client, 0x1D, &tmp_reg);
    if(isADPLL_16M)
    {		
  	    MT6616_write(client, 0x1D, (tmp_reg|0x0200));
    }
    else
    {
  	    MT6616_write(client, 0x1D, (tmp_reg&0xFDFF));
    }
    
    // Ensable ADPLL
    MT6616_ADPLL_PowerUp(client);    
    Delayms(100);
    
    //Set rgfrf_enable_top_ck ="1"
    MT6616_set_bits(client, 0x28, 0x0200, 0xFFFF);
    
    if(isADPLL_16M)
    {		
  	    //Set rgf_f16mode_en = X	
  	    MT6616_write(client, 0x31, 3);
  	    //Set rgf_cnt_resync_b = 0    
 	    MT6616_write(client, 0x31, 1);      
        //Set rgf_cnt_resync_b = 1    
 	    MT6616_write(client, 0x31, 3);
    }
    else
    {
  	    //Set rgf_f16mode_en = X  		
  	    MT6616_write(client, 0x31, 2);
        //Set rgf_cnt_resync_b = 0
 	    MT6616_write(client, 0x31, 0);          
        //Set rgf_cnt_resync_b = 1
  	    MT6616_write(client, 0x31, 2);
    } 
 
    // Power up FM modem clk
    MT6616_write(client, 0x30, 0x0006);
}

static bool MT6616_SetFreq(struct i2c_client *client, uint32_t freq)
{
    uint32_t CHAN = 0x0000;
	uint16_t tmp_reg;
#if (USE_INTERRUPT == 0)		
	uint16_t cnt = 0;
#endif
    WCN_DBG(L7|D_IOCTL, "MT6616_SetFreq freq:%d\n", freq);  

	CHAN = (freq - 640)<<1;
	MT6616_read(client,  0x0C, &tmp_reg);
	MT6616_write(client, 0x0C, (tmp_reg&0xFC00)|CHAN);

	WCN_DBG(L7|D_IOCTL, "+MT6616_SetFreq ticks:%d\n", jiffies_to_msecs(jiffies));
    	
#if USE_INTERRUPT
    MT6616_set_bits(client, FM_MAIN_EXTINTRMASK, FM_EXT_STC_DONE_MASK, 0xFFFE); 
#endif
    MT6616_Tune(client, true);	

#if USE_INTERRUPT
    if (fm_wait_stc_done(MT6616_FM_STC_DONE_TIMEOUT) == false)
    {
        WCN_DBG(L4|D_ALL, "MT6616_SetFreq get stc done failed\n");
        return false;        
    } 
    WCN_DBG(L7|D_IOCTL, "-MT6616_SetFreq ticks:%d\n", jiffies_to_msecs(jiffies));
    
    _current_frequency = freq;
    //Disable STD done intr
    MT6616_set_bits(client, FM_MAIN_EXTINTRMASK, 0, 0xFFFE);   
#else
	do {
      	MT6616_read(client, FM_MAIN_INTR, &tmp_reg);
		if((tmp_reg&FM_INTR_STC_DONE) == 0)
		{
            Delayms(10);
		    cnt++;
		}
	} while (((tmp_reg&FM_INTR_STC_DONE) == 0) && (cnt < MT6616_MAX_COUNT));
	
	//clear status flag
   	MT6616_set_bits(client, FM_MAIN_INTR, 0x0001, 0xFFFF);
#endif


   	Delayms(125);
  	//Set softmute to normal mode
  	MT6616_write(client, FM_MAIN_PGSEL, 0x0);
  	MT6616_write(client, 0xE9, 0x0232);
  	MT6616_write(client, 0xBD, 0x4562);
  	MT6616_write(client, FM_MAIN_PGSEL, 0x0001);
  	MT6616_write(client, 0xD8, 0x008B);
  	MT6616_write(client, FM_MAIN_PGSEL, 0x0);
  	//Disable mute
  	MT6616_set_bits(client, FM_DAC_CON1, 0x0, 0xFFFC);
  	
#if USE_INTERRUPT
    return true;
#else
    if(cnt == MT6616_MAX_COUNT) {
        WCN_DBG(L4|D_ALL, "MT6616_SetFreq failed: can't get STC_DONE\n");  
        return false;    
    }
    else 
    {
        _current_frequency = freq;
  	    return true;
  	}
#endif   
}

static void MT6616_Tune_HiLo(struct i2c_client *client, 
                            uint16_t freq, uint16_t band, 
                            uint16_t space)
{
    int16_t rssi;
	uint16_t tmp_reg;
	
	MT6616_Mute(client, true);      
    MT6616_RampDown(client);		
    
    //set space(100k/200k)and band 	
    MT6616_set_bits(client, 0x03, (space<<13|band), 0xC7FF);
        
    //Read Low-Side LO Injection
  	//R11 --> clear  D15,  clear D0/D2,  D3 is the same as default
  	MT6616_set_bits(client, 0x0B, 0, 0x7FFA);
  	
  	if(false == MT6616_SetFreq(client, freq)) 
    {
        WCN_DBG(L4|D_ALL, "MT6616_Tune_HiLo set freq failed+\n");  
    }    
      
    MT6616_read(client, FM_ADDR_PAMD, &tmp_reg);
  	rssi = (int16_t)(tmp_reg&FM_RSSI_MASK);
  	
  	//Read Hi-Side LO Injection
  	// R11-->set D15, set D0/D2,  D3 is the same as default
  	MT6616_set_bits(client, 0x0B, 0x8005, 0xFFFF);
  	if(false == MT6616_SetFreq(client, freq)) 
    {
       	WCN_DBG(L4|D_ALL, "MT6616_Tune_HiLo set freq failed++\n");  
    }  
      	
    MT6616_read(client, FM_ADDR_PAMD, &tmp_reg);
    rssi = rssi - (int16_t)(tmp_reg&FM_RSSI_MASK);  
      	
    if(rssi < 0) //errata in 0.82
  	{ 	
		// LO
		// R11--> clear D15, set D0/D2, D3 is the same as default
		MT6616_set_bits(client, 0x0B, 0x0005, 0x7FFF);
  	}else{ 
		//HI
		// R11--> set D15, clear D0/D2, D3 is the same as default
		MT6616_set_bits(client, 0x0B, 0x8000, 0xFFFA);
  	}
  	
  	//fine-tune !!
  	//TUNE to freq with current setting
  	if(false == MT6616_SetFreq(client, freq)) 
    {
       WCN_DBG(L4|D_ALL, "MT6616_Tune_HiLo set freq failed++\n");
    }  
    
}                           
                            

/*
* MT6616_Seek
* pFreq: IN/OUT parm, IN start freq/OUT seek valid freq
* return true:seek success; false:seek failed
*/
static bool MT6616_Seek(struct i2c_client *client, uint16_t min_freq, uint16_t max_freq, uint16_t *pFreq, uint16_t seekdir, uint16_t space)
{
    uint16_t tmp_reg, cnt = 0;
    uint16_t startfreq = *pFreq;
    
    WCN_DBG(L7|D_IOCTL, "MT6616_Seek start freq:%d\n", startfreq); 
 
    MT6616_Mute(client, true);   
    MT6616_RampDown(client);	
    
    if (_current_frequency != startfreq)
    {
        WCN_DBG(L7|D_IOCTL, "MT6616_seek startfreq not same as _current_frequency\n");
        	
	    //set freq 
        if (false == MT6616_SetFreq(client, startfreq)) 
        {
             WCN_DBG(L4|D_ALL, "MT6616_Seek failed set freq\n");  
             return -1;
        }
    
        MT6616_RampDown(client);       
    }
    Delayms(50);
    
    //set space(100k/200k)and band(min_freq~max_freq) and up/down and enable wrap
    MT6616_set_bits(client, FM_MAIN_CFG2, (min_freq-640)<<1, 0xFC00);
    MT6616_set_bits(client, FM_MAIN_CFG1, (space<<12)|(seekdir<<10)|((max_freq-640)<<1), 0x8800);
    
    //Enable STD done intr
#if USE_INTERRUPT
    MT6616_set_bits(client, FM_MAIN_EXTINTRMASK, FM_EXT_STC_DONE_MASK, 0xFFFE); 
#endif
    WCN_DBG(L7|D_IOCTL, "+MT6616_Seek ticks:%d\n", jiffies_to_msecs(jiffies));

    //seek on    
    MT6616_set_bits(client, FM_MAIN_CTRL, 0x0002, 0xFFFD);

#if USE_INTERRUPT
    if (fm_wait_stc_done(MT6616_FM_STC_DONE_TIMEOUT) == false)
    {
        WCN_DBG(L4|D_ALL, "MT6616_Seek get stc done failed\n");    
        MT6616_set_bits(client, FM_MAIN_INTR, 0x0001, 0xFFFF);
        MT6616_RampDown(client);
        return false;        
    }
    WCN_DBG(L7|D_IOCTL, "-MT6616_Seek ticks:%d\n", jiffies_to_msecs(jiffies));
    
    //Disable STD done intr
    MT6616_set_bits(client, FM_MAIN_EXTINTRMASK, 0, 0xFFFE); 
#else   
    do
    {		
        MT6616_read(client, FM_MAIN_INTR, &tmp_reg);
        if((tmp_reg&FM_INTR_STC_DONE) == 0) {
            Delayms(50);
            cnt++;
        }
    }while(((tmp_reg&FM_INTR_STC_DONE) == 0) && (cnt < MT6616_MAX_COUNT));
    
    //seek failed
    if(cnt == MT6616_MAX_COUNT)
    {
        //clear status flag
        WCN_DBG(L4|D_ALL, "MT6616_Seek failed:can't get valid STC_Done\n"); 
        MT6616_set_bits(client, FM_MAIN_INTR, 0x0001, 0xFFFF);
        MT6616_RampDown(client);
        return false;      
    }
    //seek ok --- clear status flag
    MT6616_set_bits(client, FM_MAIN_INTR, 0x0001, 0xFFFF);
#endif
  
    MT6616_RampDown(client);
    
    MT6616_read(client, FM_MAIN_CHANDETSTAT, &tmp_reg);		
 	*pFreq = 640 + ((tmp_reg&FM_MAIN_CHANDET_MASK) >> (FM_MAIN_CHANDET_SHIFT+1));
 	_current_frequency = *pFreq;			
    
    //get the result freq
    WCN_DBG(L7|D_IOCTL, "MT6616_Seek result freq:%d\n", *pFreq); 
    MT6616_Mute(client, false);
      
    return true;   
}

static bool MT6616_Scan(struct i2c_client *client, 
                        uint16_t min_freq, uint16_t max_freq,
                        uint16_t *pFreq,
                        uint16_t *pScanTBL, 
                        uint16_t *ScanTBLsize, 
                        uint16_t scandir, 
                        uint16_t space)
{
    uint16_t tmp_reg, space_val, startfreq, offset = 0;
#if (USE_INTERRUPT == 0)	
	uint16_t cnt = 0;
#endif
    uint16_t tmp_scanTBLsize = *ScanTBLsize;

    g_bStopScan = false;   
    if((!pScanTBL) || (tmp_scanTBLsize == 0)) {
        WCN_DBG(L4|D_ALL, "MT6616_Scan failed:invalid scan table for driver\n"); 
        return false;
    }

    WCN_DBG(L7|D_IOCTL, "+MT6616_Scan:%d\n", jiffies_to_msecs(jiffies));
    WCN_DBG(L7|D_IOCTL, "MT6616_Scan start freq: %d, max_freq:%d, min_freq:%d, scan BTL size:%d, scandir:%d, space:%d\n", *pFreq, max_freq, min_freq, *ScanTBLsize, scandir, space);
    
    if(tmp_scanTBLsize > MT6616_SCANTBL_SIZE)
    {
        tmp_scanTBLsize = MT6616_SCANTBL_SIZE;
    }
        
    if (space == MT6616_FM_SPACE_200K) {
        space_val = 2; //200K
    }
    else if(space == MT6616_FM_SPACE_100K)
    {
        space_val = 1;  //100K
    }
    else //default
    {
        space_val = 1;  //100K    
    }
    
    //scan up
    if (scandir == MT6616_FM_SCAN_UP) {
        startfreq = min_freq - space_val;
	}
    else //scan down
    {
        startfreq = max_freq + space_val;//max_freq compare need or not   
    }
    
    MT6616_Mute(client, true);    
    MT6616_RampDown(client);		
	//set freq 
    if (false == MT6616_SetFreq(client, startfreq)) 
    {
         WCN_DBG(L4|D_ALL, "MT6616_Scan failed set freq\n");  
         return false;
    }
    
    MT6616_RampDown(client);	
    Delayms(50);    
    
    //set space(100k/200k)and band(min_freq~max_freq) and up/down and disable wrap
    MT6616_set_bits(client, FM_MAIN_CFG2, (min_freq-640)<<1, 0xFC00);
    MT6616_set_bits(client, FM_MAIN_CFG1, (space<<12)|(scandir<<10)|((max_freq-640)<<1), 0x8000);
    
    //Enable STD done intr
#if USE_INTERRUPT
    MT6616_set_bits(client, FM_MAIN_EXTINTRMASK, FM_EXT_STC_DONE_MASK, 0xFFFE); 
#endif

    //scan on
    MT6616_set_bits(client, FM_MAIN_CTRL, 0x0004, 0xFFFB);

#if USE_INTERRUPT
    if (fm_wait_stc_done(MT6616_FM_STC_DONE_TIMEOUT) == false)
    {
        WCN_DBG(L4|D_ALL, "MT6616_Scan get stc done failed\n");
        MT6616_set_bits(client, FM_MAIN_INTR, 0x0001, 0xFFFF);
        MT6616_RampDown(client);
        
        //get the valid freq after scan
        MT6616_read(client, FM_MAIN_CHANDETSTAT, &tmp_reg);		
        tmp_reg = 640 + ((tmp_reg&FM_MAIN_CHANDET_MASK) >> (FM_MAIN_CHANDET_SHIFT+1));			
        *pFreq = tmp_reg;
        WCN_DBG(L4|D_ALL, "MT6616_Scan failed freq:%d\n", *pFreq);   
        return false;        
    }
    
    //Disable STD done intr
    MT6616_set_bits(client, FM_MAIN_EXTINTRMASK, 0, 0xFFFE);
#else    
    do
    {		
        MT6616_read(client, FM_MAIN_INTR, &tmp_reg);
        if(g_bStopScan) {
            //clear status flag
            WCN_DBG(L4|D_ALL, "MT6616_Seek failed due to Force stop scan\n"); 
            break;
        }
        
        if((tmp_reg&FM_INTR_STC_DONE) == 0) {
            Delayms(50);
            cnt++;
        }
    }while(((tmp_reg&FM_INTR_STC_DONE) == 0) && (cnt < (MT6616_MAX_COUNT<<1))); //10s for auto_scan, is that enough?
    
    //scan failed
    if(cnt == (MT6616_MAX_COUNT<<1))
    {
        //clear status flag
        WCN_DBG(L4|D_ALL, "MT6616_Scan failed:can't get valid STC_Done, %d\n", jiffies_to_msecs(jiffies));
        MT6616_set_bits(client, FM_MAIN_INTR, 0x0001, 0xFFFF);
        MT6616_RampDown(client);
         
        //get the valid freq after scan
        MT6616_read(client, FM_MAIN_CHANDETSTAT, &tmp_reg);		
        tmp_reg = 640 + ((tmp_reg&FM_MAIN_CHANDET_MASK) >> (FM_MAIN_CHANDET_SHIFT+1));			
        *pFreq = tmp_reg;
        WCN_DBG(L4|D_ALL, "MT6616_Scan failed freq:%d\n", *pFreq);
        return false;      
    }    
    //scan ok --- clear status flag
    MT6616_set_bits(client, FM_MAIN_INTR, 0x0001, 0xFFFF);
#endif 
  
    MT6616_RampDown(client);
    
    //get the valid freq after scan
    MT6616_read(client, FM_MAIN_CHANDETSTAT, &tmp_reg);		
    tmp_reg = 640 + ((tmp_reg&FM_MAIN_CHANDET_MASK) >> (FM_MAIN_CHANDET_SHIFT+1));			
    *pFreq = tmp_reg;
    WCN_DBG(L7|D_IOCTL, "-MT6616_Scan:%d\n", jiffies_to_msecs(jiffies));
    WCN_DBG(L7|D_IOCTL, "MT6616_Scan after scan freq:%d\n", *pFreq);
    MT6616_Mute(client, false);	
    
    //get scan Table
    WCN_DBG(L7|D_IOCTL, "MT6616_Scan tbl:");    
    MT6616_write(client, FM_MAIN_PGSEL, 0x0001);
    for(offset = 0; offset < tmp_scanTBLsize; offset++)
    {        
        MT6616_read(client, (FM_BITMAP0+offset), &tmp_reg);         
        *(pScanTBL+offset) = tmp_reg;
        WCN_DBG(L7|D_IOCTL, "%04x: %04x ", FM_BITMAP0+offset, tmp_reg);
    }
    MT6616_write(client, FM_MAIN_PGSEL, 0x0);
    *ScanTBLsize = tmp_scanTBLsize;
        
    return true;
}

static void MT6616_ScanForceStop(void)
{
    WCN_DBG(L7|D_IOCTL, "MT6616_ScanForceStop will be set\n");
#if USE_INTERRUPT
    fm_stc_done = true;
    wake_up_interruptible(&fm_stc_done_wait);
#else
    g_bStopScan = true;
#endif    
}

static void MT6616_GetCurRSSI(struct i2c_client *client, uint16_t *pRSSI)
{
    uint16_t tmp_reg, rssi;
    
    MT6616_read(client, FM_ADDR_RSSI, &tmp_reg);
    WCN_DBG(L7|D_IOCTL, "MT6616 FM_ADDR_RSSI:%04x \n", tmp_reg);
    
    rssi = tmp_reg&0x03ff;
    
    /*RS=RSSI 
     *If RS>511, then RSSI(dBm)= (RS-1024)/16*6 
     *                 else RSSI(dBm)= RS/16*6 
     * dBuV             
     */ 
    if (pRSSI) {
        *pRSSI = (rssi>511) ? ((int16_t)(((rssi-1024)*6)>>4)+113):((rssi*6)>>4);   
    }
    WCN_DBG(L7|D_IOCTL, "MT6616 rssi:%d, dBuV:%d\n", rssi, *pRSSI);
}

static void MT6616_SetVol(struct i2c_client *client, uint8_t vol)
{
    uint8_t tmp_vol = vol&0x3f;
    if(tmp_vol > FM_VOL_MAX)
        tmp_vol = FM_VOL_MAX;
            
    MT6616_set_bits(client, FM_DAC_CON1, (tmp_vol<<6), 0xF03F); 
    WCN_DBG(L7|D_IOCTL, "MT6616_SetVol vol:%d\n", tmp_vol);
}

static void MT6616_GetVol(struct i2c_client *client, uint8_t *pVol)
{
    uint16_t tmp_reg;
    
    MT6616_read(client, FM_DAC_CON1, &tmp_reg);
    
    *pVol = (tmp_reg>>6) & 0x3f;
    WCN_DBG(L7|D_IOCTL, "MT6616_GetVol vol:%d\n", *pVol);
    
}

#if 0//def FMDEBUG
static void MT6616_dump_reg(struct i2c_client *client)
{
    int i;
    int err;
    uint16_t val;

    MT6616_write(client, FM_MAIN_PGSEL, 0x0001);
    for (i = 0; i < MT6616_SCANTBL_SIZE; i++)
    {
        val = 0;
        err = MT6616_read(client, FM_BITMAP0+i, &val);
        if (err == 0)
        {
            FM_DEBUG("%2d\t%04X\n", i, val);
        }
        else
        {
            FM_DEBUG("%2d\tXXXX\n", i);
        }
    }
    MT6616_write(client, FM_MAIN_PGSEL, 0x0);
}
#endif // FMDEBUG

static bool MT6616_GetMonoStereo(struct i2c_client *client, uint16_t *pMonoStereo)
{
    uint16_t tmp_reg;
        
    if(MT6616_write(client, FM_MAIN_PGSEL, 0x01))
        return false;
        
    MT6616_read(client, 0xF8, &tmp_reg);  
    MT6616_write(client, FM_MAIN_PGSEL, 0x0);
    tmp_reg = (tmp_reg&0x400)>>10;  
    
    *pMonoStereo = tmp_reg;//1:stereo, 0:mono
    return true;   
}

#if USE_INTERRUPT
static void fm_enable_eint(void)
{
    MT65XX_EINT_IRQ_UNMASK(CUST_EINT_FM_RDS_NUM);
}

static void fm_disable_eint(void)
{
    MT65XX_EINT_IRQ_MASK(CUST_EINT_FM_RDS_NUM);
}

static void fm_eint_handler(void)
{
    struct fm *fm = g_fm_struct;
    WCN_DBG(L7|D_IOCTL, "intr occur, ticks:%d, g_fm_struct:%08x\n", jiffies_to_msecs(jiffies), g_fm_struct);
    if(fm != NULL)
    {
        WCN_DBG(L7|D_RDS, "fm_workqueue schedule, ticks:%d\n", jiffies_to_msecs(jiffies));
        queue_work(fm->fm_workqueue, &fm->fm_work); 
    }
}

static void fm_request_eint(void *data)
{
    MT65XX_EINT_SET_SENSITIVITY(CUST_EINT_FM_RDS_NUM, CUST_EINT_FM_RDS_SENSITIVE);
    MT65XX_EINT_SET_HW_DEBOUNCE(CUST_EINT_FM_RDS_NUM, CUST_EINT_FM_RDS_DEBOUNCE_CN);
    WCN_DBG(L7|D_ALL, "fm_request_eint\n\r");
    MT65XX_EINT_REGISTRATION(CUST_EINT_FM_RDS_NUM,
		                     CUST_EINT_FM_RDS_DEBOUNCE_EN,
			                 CUST_EINT_FM_RDS_POLARITY,
			                 fm_eint_handler,
			                 0);
    MT65XX_EINT_IRQ_MASK(CUST_EINT_FM_RDS_NUM);
}

static void fm_workqueue_func(struct work_struct *work)
{    
    
    uint16_t tmp_reg;
    struct fm *fm = g_fm_struct;//(struct fm *)work->data;
    struct i2c_client *client = fm->i2c_client; 
#if FM_RDS_ENABLE
    RDSData_Struct *pstRDSData = fm->pstRDSData;
#endif
    MT6616_read(client, FM_MAIN_INTR, &tmp_reg);
    WCN_DBG(L7|D_RDS, "fm_workqueue_func,FM_MAIN_INTR:%x, ticks:%d\n", tmp_reg, jiffies_to_msecs(jiffies));
    
    if(tmp_reg&0x0001)
    {
        //clear status flag
        MT6616_set_bits(client, FM_MAIN_INTR, 0x0001, 0xFFFF);
        fm_stc_done = true; 
        WCN_DBG(L7|D_RDS, "wake_up_interruptible stc done, ticks:%d\n", jiffies_to_msecs(jiffies));
        wake_up_interruptible(&fm_stc_done_wait);  
    }
#if FM_RDS_ENABLE    
    if(tmp_reg&0x0020)
    {
        //clear status flag
        MT6616_set_bits(client, FM_MAIN_INTR, 0x0020, 0xFFFF);
        
        if (down_interruptible(&fm_read_mutex))
        {
            WCN_DBG(L3|D_RDS, "fm_workqueue_func can't get read mutex\n");
            return;       
        }
        MT6616_RDS_Eint_Handler(fm);
        
        up(&fm_read_mutex);
        
        //loop pstRDSData->event_status then act 
        if((pstRDSData->event_status != 0) && (pstRDSData->event_status != RDS_EVENT_AF_LIST))
        {
            fm->RDS_Data_ready = true;
            WCN_DBG(L7|D_RDS, "RDS data ready, event_status:%04x, ticks:%d\n", pstRDSData->event_status, jiffies_to_msecs(jiffies));
            wake_up_interruptible(&fm->read_wait);
        }
    }
 #endif   
    WCN_DBG(L7|D_RDS, "-fm_workqueue_func, ticks:%d\n", jiffies_to_msecs(jiffies));
    
    //re-enable eint
    fm_enable_eint();
}

static bool fm_wait_stc_done(uint32_t sec)
{
    //log for study, need mask when check in code
    //wait_event_interruptible_timeout return timeout left, if > 0, success, or failed
    if (wait_event_interruptible_timeout(fm_stc_done_wait, (fm_stc_done == true), sec*HZ))
    {
        WCN_DBG(L7|D_ALL, "stc done wait success,fm_stc_done:%d ticks:%d\n\r", fm_stc_done, jiffies_to_msecs(jiffies)); 
        fm_stc_done = false; 
        return true;
    }
    else
    {
        WCN_DBG(L7|D_ALL, "stc done wait failed, fm_stc_done:%d, ticks:%d\n\r", fm_stc_done, jiffies_to_msecs(jiffies));
        fm_stc_done = false;
        return false;
    }
}
#endif

#if FM_RDS_ENABLE
static void fm_rds_reset_func(unsigned long data)
{
	down(&fm_timer_mutex);
	if (g_stop_timer == 1) {
		up(&fm_timer_mutex);
		return;
	}

    struct fm *fm = g_fm_struct;
    WCN_DBG(L7|D_BLKC, "T:%d, fm_rds_reset_func\n", jiffies_to_msecs(jiffies));

    if(fm != NULL)
    {
        WCN_DBG(L7|D_BLKC, "fm_rds_reset_workqueue_schedule, ticks:%d\n", jiffies_to_msecs(jiffies));
        queue_work(fm->fm_rds_reset_workqueue, &fm->fm_rds_reset_work); 
    }

    //update timer
	mod_timer(&fm_rds_reset_timer, jiffies + gBLER_CHK_INTERVAL/(1000/HZ)); 
	WCN_DBG(L7|D_BLKC, "-T:%d, mod timer\n", jiffies_to_msecs(jiffies));	  
	up(&fm_timer_mutex);
}

static void fm_enable_rds_BlerCheck(struct fm *fm)
{
	down(&fm_timer_mutex);
	g_stop_timer = 0;
	mod_timer(&fm_rds_reset_timer, jiffies + gBLER_CHK_INTERVAL/(1000/HZ));
     /*init_timer(&fm_rds_reset_timer);
     fm_rds_reset_timer.expires  = jiffies + gBLER_CHK_INTERVAL/(1000/HZ);
     fm_rds_reset_timer.function = fm_rds_reset_func;
     fm_rds_reset_timer.data     = (unsigned long)fm;
     add_timer(&fm_rds_reset_timer);*/
	up(&fm_timer_mutex);
}

static void fm_disable_rds_BlerCheck()
{
	down(&fm_timer_mutex);
	g_stop_timer = 1;
    del_timer(&fm_rds_reset_timer);
	up(&fm_timer_mutex);
}

static void fm_rds_reset_workqueue_func(struct work_struct *work)
{
    struct fm *fm = g_fm_struct;	 
    if (down_interruptible(&fm_read_mutex))
    {
        WCN_DBG(L6|D_BLKC, "fm_rds_reset_workqueue_func can't get mutex");
        return;      
    }
    
    MT6616_RDS_BlerCheck(fm);
    up(&fm_read_mutex); 
    WCN_DBG(L7|D_BLKC, "-T:%d, MT6616_RDS_BlerCheck\n", jiffies_to_msecs(jiffies));
}
#endif      

static bool MT6616_GetCurPamd(struct i2c_client *client, uint16_t *pPamdLevl)
{
    uint16_t tmp_reg;
    uint16_t dBvalue;
    
    if(MT6616_read(client, FM_ADDR_PAMD, &tmp_reg))
        return false;
        
    tmp_reg &= 0x03FF;
    
    /*PA=PAMD
    *If PA>511 then PAMD(dB)=  (PA-1024)/16*6,
    *               else PAMD(dB)=PA/16*6                 
    */    
    dBvalue = (tmp_reg>511) ? ((1024-tmp_reg)*6/16):0;
        
    *pPamdLevl = dBvalue;
    return true;
}

static bool MT6616_em_test(struct i2c_client *client, uint16_t group_idx, uint16_t item_idx, uint32_t item_value)
{
    WCN_DBG(L7|D_ALL, "+MT6616_em_test  %d:%d:%d\n", group_idx, item_idx, item_value);    
	switch (group_idx)
	{
		case mono:
			if(item_value == 1)
			{
			    MT6616_set_bits(client, 0xd7, 0x02, 0xFFFF);
			}
			else
			{
			    MT6616_set_bits(client, 0xd7, 0x0, 0xFFFD);
			}
			break;
		case stereo:
			if(item_value == 0)
			{
			    MT6616_set_bits(client, 0xd7, 0x0, 0xFFFD);
			}
			else
			{
				switch (item_idx)
				{
					case Sblend_ON:
					    MT6616_set_bits(client, 0xD8, item_idx<<15, 0x7FFF);
					    break;
					case Sblend_OFF:
					    MT6616_set_bits(client, 0xD8, 0, 0x7FFF);
					    break;
				}
			}
			break;
		case RSSI_threshold:
		    MT6616_set_bits(client, 0x60, item_value, 0xFC00);
		    break;
		case HCC_Enable:
			if(item_idx)
			{
				MT6616_set_bits(client, 0xEF, 0x10, 0xFFFF);
			}
			else
			{
			    MT6616_set_bits(client, 0xEF, 0x0, 0xFFEF);		
			}
		    break;
		case PAMD_threshold:
		    MT6616_set_bits(client, 0x61, item_value, 0xFC00);
		    MT6616_write(client, FM_MAIN_PGSEL, 0x01);
		    MT6616_set_bits(client, 0x7F, item_value, 0xFC00);
			MT6616_write(client, FM_MAIN_PGSEL, 0x0);
		    break;
		case Softmute_Enable:
			if(item_idx)
			{
				MT6616_set_bits(client, 0xEF, 0x20, 0xFFFF);
			}
			else
			{
			    MT6616_set_bits(client, 0xEF, 0x0, 0xFFDF);		
			}
		    break;
		case De_emphasis:
			if(item_idx == 2) //75us
			{
			    MT6616_set_bits(client, 0xF0, 0x02, 0xFFFC);				
			}
			else if(item_idx == 1) //50us
			{
			    MT6616_set_bits(client, 0xF0, 0x01, 0xFFFC);		
			}
			else if(item_idx == 0) //0us
			{
			    MT6616_set_bits(client, 0xF0, 0x0, 0xFFFC);		
			}
		    break;
		case HL_Side:
			if(item_idx == 2) //H-Side
			{
			    MT6616_set_bits(client, 0x4F, 0x11, 0xFFFE);
			    MT6616_set_bits(client, 0x0C, 0x0400,  0xFBFF);
			}
			else if(item_idx == 1) //L-Side
			{
			    MT6616_set_bits(client, 0x4F, 0x10, 0xFFFE);
			    MT6616_set_bits(client, 0x0C, 0x0,  0xFBFF);		
			}
			else if(item_idx == 0) //Auto
			{
			    MT6616_set_bits(client, 0x4F, 0x0, 0xFFEE);		
			}
		    break;

		case Demod_BW:
			if(item_idx == 2) //force wide
			{
			    MT6616_set_bits(client, 0x73, 0x03, 0xFFFC);
			}
			else if(item_idx == 1) //force narrow
			{
			    MT6616_set_bits(client, 0x73, 0x01, 0xFFFC);		
			}
			else if(item_idx == 0) //auto
			{
			    MT6616_set_bits(client, 0x73, 0x0, 0xFFFC);		
			}
		    break;
		case Dynamic_Limiter:
		    MT6616_write(client, FM_MAIN_PGSEL, 0x01);
			if(item_idx)
			{
			    MT6616_set_bits(client, 0xFA, 0x0, 0xFFF7);
			}
			else
			{
			    MT6616_set_bits(client, 0xFA, 0x08, 0xFFF7);		
			}	
			MT6616_write(client, FM_MAIN_PGSEL, 0x0);	
		    break;
		    
		case Softmute_Rate:
		    MT6616_set_bits(client, 0xE9, item_value<<8, 0x80FF);
		    break;
		    
		case AFC_Enable:
		    if (item_idx)
		    {
		        MT6616_set_bits(client, 0x33, 0x0400, 0xFBFF);
		    }
		    else
		    {
		        MT6616_set_bits(client, 0x33, 0x0,    0xFBFF);		        
		    }
		    break;
		    
		case Softmute_Level:
		    MT6616_write(client, FM_MAIN_PGSEL, 0x01); 
		    if(item_value > 0x24)
				item_value = 0x24;
		    MT6616_set_bits(client, 0xD1, item_value, 0xFFC0);		    
		    MT6616_write(client, FM_MAIN_PGSEL, 0x0); 
		    break;
		    
		case Analog_Volume:
		    MT6616_set_bits(client, 0xB0, item_value<<6, 0xF03F);
		    break;
		    
		default:
		    break;
	} 
	
	WCN_DBG(L7|D_ALL, "-MT6616_em_test  %d:%d:%d\n", group_idx, item_idx, item_value);    
	return true;   
}

static int fm_setup_cdev(struct fm *fm)
{
    int err;

    err = alloc_chrdev_region(&fm->dev_t, 0, 1, FM_NAME);
    if (err) {
        WCN_DBG(L4|D_ALL, "alloc dev_t failed\n");
        return -1;
    }

    WCN_DBG(L7|D_IOCTL, "alloc %s:%d:%d\n", FM_NAME,
                MAJOR(fm->dev_t), MINOR(fm->dev_t));

    cdev_init(&fm->cdev, &fm_ops);

    fm->cdev.owner = THIS_MODULE;
    fm->cdev.ops = &fm_ops;

    err = cdev_add(&fm->cdev, fm->dev_t, 1);
    if (err) {
        WCN_DBG(L4|D_ALL, "alloc dev_t failed\n");
        return -1;
    }

    fm->cls = class_create(THIS_MODULE, FM_NAME);
    if (IS_ERR(fm->cls)) {
        err = PTR_ERR(fm->cls);
        WCN_DBG(L4|D_ALL, "class_create err:%d\n", err);
        return err;            
    }    
    fm->dev = device_create(fm->cls, NULL, fm->dev_t, NULL, FM_NAME);

    return 0;
}

static int fm_ops_ioctl(struct inode *inode, struct file *filp,
                          unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    struct fm *fm = container_of(inode->i_cdev, struct fm, cdev);

    switch(cmd)
    {
        case FM_IOCTL_POWERUP:
        {
            struct fm_tune_parm parm;
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_POWERUP\n");

            if (copy_from_user(&parm, (void*)arg, sizeof(struct fm_tune_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_powerup(fm, &parm);
            up(&fm_ops_mutex);
            if (copy_to_user((void*)arg, &parm, sizeof(struct fm_tune_parm)))
                return -EFAULT;

            break;
        }

        case FM_IOCTL_POWERDOWN:
	    {
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_POWERDOWN\n");
// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//                return -EPERM;
            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_powerdown(fm);
            up(&fm_ops_mutex);
            break;
	    }
	    
        // tune (frequency, auto Hi/Lo ON/OFF )
        case FM_IOCTL_TUNE:
        {
            struct fm_tune_parm parm;
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_TUNE\n");

            if (copy_from_user(&parm, (void*)arg, sizeof(struct fm_tune_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_tune(fm, &parm);
            up(&fm_ops_mutex);

            if (copy_to_user((void*)arg, &parm, sizeof(struct fm_tune_parm)))
                return -EFAULT;

            break;
        }

        case FM_IOCTL_SEEK:
        {
            struct fm_seek_parm parm;
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_SEEK\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (copy_from_user(&parm, (void*)arg, sizeof(struct fm_seek_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_seek(fm, &parm);
            up(&fm_ops_mutex);

            if (copy_to_user((void*)arg, &parm, sizeof(struct fm_seek_parm)))
                return -EFAULT;

            break;
        }
        
        case FM_IOCTL_SCAN:
        {
            struct fm_scan_parm parm;
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_SCAN\n");
            
            if(copy_from_user(&parm, (void*)arg, sizeof(struct fm_scan_parm))) {
                WCN_DBG(L4|D_IOCTL, "copy_from_user failed\n");
                return -EFAULT;
            }
 
            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
                
            ret = fm_scan(fm, &parm);
            
            up(&fm_ops_mutex);
            
            if(copy_to_user((void*)arg, &parm, sizeof(struct fm_scan_parm)))
                return -EFAULT;
                
            break;           
        }
        
        case FM_IOCTL_SETVOL:
        {
            uint32_t vol;
// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

	        if(copy_from_user(&vol, (void*)arg, sizeof(uint32_t))) {
                WCN_DBG(L4|D_IOCTL, "copy_from_user failed\n");
                return -EFAULT;
            }
            
            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;   
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_SETVOL:%d\n", vol);
            ret = fm_setvol(fm, vol);
            up(&fm_ops_mutex);

            break;
        }
        case FM_IOCTL_GETVOL:
        {
            uint32_t vol;
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_GETVOL\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            ret = fm_getvol(fm, &vol);
            up(&fm_ops_mutex);

            if (copy_to_user((void*)arg, &vol, sizeof(uint32_t)))
                return -EFAULT;

            break;
        }

        case FM_IOCTL_MUTE:
        {
            uint32_t bmute;
// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (copy_from_user(&bmute, (void*)arg, sizeof(uint32_t)))
                return -EFAULT;     
                           
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_MUTE:%d\n", bmute);            
            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
                
            if (bmute)
                ret = MT6616_Mute(fm->i2c_client, true);                
            else
                ret = MT6616_Mute(fm->i2c_client, false);

            up(&fm_ops_mutex);

            break;
        }

        case FM_IOCTL_GETRSSI:
        {
            uint32_t rssi = 0;
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_GETRSSI\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;

            ret = fm_getrssi(fm, &rssi);
            up(&fm_ops_mutex);

            if (copy_to_user((void*)arg, &rssi, sizeof(uint32_t)))
                return -EFAULT;

            break;
        }
        
        case FM_IOCTL_RW_REG:
        {
            struct fm_ctl_parm parm_ctl;
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_RW_REG\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (copy_from_user(&parm_ctl, (void*)arg, sizeof(struct fm_ctl_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;

            if(parm_ctl.rw_flag == 0) //write
            {
                WCN_DBG(L5|D_IOCTL, "Write Reg %02x:%04x\n", parm_ctl.addr, parm_ctl.val);
                ret = MT6616_write(fm->i2c_client, parm_ctl.addr, parm_ctl.val);
            }
            else
            {
                ret = MT6616_read(fm->i2c_client, parm_ctl.addr, &parm_ctl.val);           
                WCN_DBG(L5|D_IOCTL, "Read Reg %02x:%04x\n", parm_ctl.addr, parm_ctl.val);
            }
            
            up(&fm_ops_mutex);
            if ((parm_ctl.rw_flag == 0x01) && (!ret)) // Read success.
            { 
                if (copy_to_user((void*)arg, &parm_ctl, sizeof(struct fm_ctl_parm)))
                    return -EFAULT;
            }
            break;
        }
        
        case FM_IOCTL_GETCHIPID:
        {
            uint16_t chipid;            

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
                
            //ret = MT6616_read(fm->i2c_client, 0x32, &chipid); 
            chipid = fm->chip_id;
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_GETCHIPID:%04x\n", chipid);      

            up(&fm_ops_mutex);
            
            if (copy_to_user((void*)arg, &chipid, sizeof(uint16_t)))
                return -EFAULT;
                            
            break;
        }
        
        case FM_IOCTL_GETMONOSTERO:
        {
            uint16_t usStereoMono;            

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
                
            MT6616_GetMonoStereo(fm->i2c_client, &usStereoMono); 
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_GETMONOSTERO:%04x\n", usStereoMono);      

            up(&fm_ops_mutex);
            
            if (copy_to_user((void*)arg, &usStereoMono, sizeof(uint16_t)))
                return -EFAULT;
                            
            break;
        } 
        
        case FM_IOCTL_GETCURPAMD:
        {
            uint16_t PamdLevl;            

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;

            MT6616_GetCurPamd(fm->i2c_client, &PamdLevl); 
            up(&fm_ops_mutex);

			WCN_DBG(L5|D_IOCTL, "FM_IOCTL_GETCURPAMD:%d, _current_frequency = %d\n", PamdLevl, _current_frequency);

            if (copy_to_user((void*)arg, &PamdLevl, sizeof(uint16_t)))
                return -EFAULT;

            break;
        }

        case FM_IOCTL_EM_TEST:
        {
            struct fm_em_parm parm_em;
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_EM_TEST\n");

// FIXME!!
//            if (!capable(CAP_SYS_ADMIN))
//              return -EPERM;

            if (copy_from_user(&parm_em, (void*)arg, sizeof(struct fm_em_parm)))
                return -EFAULT;

            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
           
            MT6616_em_test(fm->i2c_client, parm_em.group_idx, parm_em.item_idx, parm_em.item_value);
            
            up(&fm_ops_mutex);
            
            break;
        }

		//2010.09.04 add by dongbo, to support RDS_SUPPORT inquiry
		case FM_IOCTL_RDS_SUPPORT:
		{
			WCN_DBG(L5|D_IOCTL, "FM_IOCTL_RDS_SUPPORT\n");
			int support;
			support = FM_RDS_ENABLE;
			if (copy_to_user((void*)arg, &support, sizeof(uint32_t)))
                return -EFAULT;
			break;
		}
		//2010.10.26 add by dongbo, to support inquiry if FM is powered up
		case FM_IOCTL_IS_FM_POWERED_UP:
		{
			WCN_DBG(L5|D_IOCTL, "FM_IOCTL_IS_FM_POWERED_UP\n");
			uint32_t powerup;
			if (fm->chipon && fm->powerup) {
				powerup = 1;
			} else {
				powerup = 0;
			}
			if (copy_to_user((void*)arg, &powerup, sizeof(uint32_t)))
                return -EFAULT;
			break;
		}
    #if FM_RDS_ENABLE       
        case FM_IOCTL_RDS_ONOFF:
        {
            uint16_t rdson_off = 0;;

            if (copy_from_user(&rdson_off, (void*)arg, sizeof(uint16_t)))
                return -EFAULT;
            
            if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
                    
            if(rdson_off)
            {                
                if(MT6616_RDS_OnOff(fm, true) == false)
                {
                    WCN_DBG(L4|D_IOCTL, "FM_IOCTL_RDS_ONOFF faield\n");
                    up(&fm_ops_mutex);
                    return -EPERM;                   
                }
				memset(fm->pstRDSData, 0, sizeof(RDSData_Struct));
                //fm_enable_eint();
                fm_enable_rds_BlerCheck(fm);
                WCN_DBG(L5|D_IOCTL, "FM_IOCTL_RDS_ON:%d\n", jiffies_to_msecs(jiffies));
            }
            else
            {
                //fm_disable_eint(); 
                fm_disable_rds_BlerCheck();               
                MT6616_RDS_OnOff(fm, false);
                WCN_DBG(L5|D_IOCTL, "FM_IOCTL_RDS_OFF:%d\n", jiffies_to_msecs(jiffies));        
            }
            up(&fm_ops_mutex);
                        
            break;      
        }  

	    case FM_IOCTL_GETGOODBCNT:
        {
		    uint16_t uGBLCNT = 0;			
	        if (down_interruptible(&fm_read_mutex))
            {
                WCN_DBG(L4|D_IOCTL, "FM_IOCTL_GETGOODBCNT can't get read mutex");
                return -EPERM;      
            }
		    uGBLCNT = GOOD_BLK_CNT;
		    up(&fm_read_mutex);
            WCN_DBG(L5|D_IOCTL, "FM_IOCTL_GETGOODBCNT:%d\n", uGBLCNT);
		    if (copy_to_user((void*)arg, &uGBLCNT, sizeof(uint16_t)))
                return -EFAULT;       
		    break;
	    }
        case FM_IOCTL_GETBADBNT:
		{
		    uint16_t uBadBLCNT = 0;
	        if (down_interruptible(&fm_read_mutex))
            {
                WCN_DBG(L4|D_IOCTL, "FM_IOCTL_GETGOODBCNT can't get read mutex");
                return -EPERM;      
            }
		    uBadBLCNT = BAD_BLK_CNT;
		    up(&fm_read_mutex);

			WCN_DBG(L5|D_IOCTL, "FM_IOCTL_GETBADBNT:%d\n", uBadBLCNT);
		    if (copy_to_user((void*)arg, &uBadBLCNT, sizeof(uint16_t)))
                return -EFAULT;
        
		    break;
	    }

        case FM_IOCTL_GETBLERRATIO:
        {
		    uint16_t uBlerRatio = 0;
	        if (down_interruptible(&fm_read_mutex))
            {
                WCN_DBG(L4|D_IOCTL, "FM_IOCTL_GETGOODBCNT can't get read mutex");
                return -EPERM;      
            }
		    uBlerRatio = (uint16_t)BAD_BLK_RATIO;
		    up(&fm_read_mutex);

			WCN_DBG(L5|D_IOCTL, "FM_IOCTL_GETBLERRATIO:%d\n", uBlerRatio);
		    if (copy_to_user((void*)arg, &uBlerRatio, sizeof(uint16_t)))
                return -EFAULT;
        
		    break;
	    }
		
    #endif
#ifdef FMDEBUG
        case FM_IOCTL_DUMP_REG:
        {
            /*if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;
            MT6616_dump_reg(fm->i2c_client);
            up(&fm_ops_mutex);

            break;*/
			struct fm_ctl_parm reg_op;

			if (copy_from_user(&reg_op, (void*)arg, sizeof(struct fm_ctl_parm)))
                return -EFAULT;

			if (down_interruptible(&fm_ops_mutex))
                return -EFAULT;

			if (reg_op.rw_flag == 0) {
				MT6616_read(fm->i2c_client, reg_op.addr, &reg_op.val);
			} else if (reg_op.rw_flag == 1) {
				MT6616_write(fm->i2c_client, reg_op.addr, reg_op.val);
			}

            up(&fm_ops_mutex);

            if (copy_to_user((void*)arg, &reg_op, sizeof(struct fm_ctl_parm)))
                return -EFAULT;

            break;
        }
#endif
        default:
            return -EPERM;
    }

    return ret;
}

static loff_t fm_ops_lseek(struct file *filp, loff_t off, int whence)
{
    struct fm *fm = filp->private_data;
    
    if(whence == SEEK_END){
        MT6616_ScanForceStop();    
    }
    else if(whence == SEEK_SET)
    {
    #if FM_RDS_ENABLE
        fm->RDS_Data_ready = true;
        WCN_DBG(L7|D_IOCTL, "fm->read_wait close set:%d\n", fm->RDS_Data_ready);
        wake_up_interruptible(&fm->read_wait); 
    #endif      
    }
    
    return off;    
}

static ssize_t fm_ops_read(struct file *filp, char *buf, size_t len, loff_t *off)
{
    struct fm *fm = filp->private_data;
    int copy_len = 0, left = 0;
    uint8_t indx;
    
    if (!fm)
    {
        WCN_DBG(L3|D_ALL, "fm_read invalid fm pointer\n");
        return 0;
    }
    
    if(!buf || len < sizeof(RDSData_Struct))
    {
        WCN_DBG(L3|D_ALL, "fm_read invliad buf\n");
        return 0;
    }    
    copy_len = sizeof(RDSData_Struct);
    
    WCN_DBG(L7|D_RDS, "+fm_ops_read\n");
    
#if FM_RDS_ENABLE
RESTART:    
    if(fm->RDS_Data_ready == true)
    {
        if (down_interruptible(&fm_read_mutex))
        {
            return 0;       
        }
        
        if((left = copy_to_user((void *)buf, fm->pstRDSData, (unsigned long)copy_len)))
        {
            WCN_DBG(L3|D_ALL, "fm_read copy failed\n");
        }
        WCN_DBG(L7|D_RDS, "fm_read copy len:%d\n", (copy_len-left));
        if(left == 0) //Clear
        {
            //if(fm->pstRDSData->event_status&RDS_EVENT_AF_LIST)
			//	memset(&fm->pstRDSData->AF_Data, 0, sizeof(AF_Info));
            fm->pstRDSData->event_status = 0x0;			
        }
        fm->RDS_Data_ready = false;			
        up(&fm_read_mutex);
    }
    else
    {
        WCN_DBG(L7|D_RDS, "fm_read wait event, fm->RDS_Data_ready = %d\n", fm->RDS_Data_ready);
        if (wait_event_interruptible(fm->read_wait, (fm->RDS_Data_ready == true)) == 0)
        {
            WCN_DBG(L7|D_RDS, "fm_read wait event success, event_status=0x%x\n", fm->pstRDSData->event_status);
            goto RESTART;
        }
        else 
        {
            WCN_DBG(L7|D_RDS, "fm_read wait event err:%d\n", fm->RDS_Data_ready);
            fm->RDS_Data_ready = false;
            return 0;
        }
    }
#endif
    WCN_DBG(L7|D_RDS, "-fm_ops_read:%d\n", copy_len-left);
    return (copy_len-left);
}

static ssize_t fm_ops_write(struct file *filp, const char *buf, size_t len, loff_t *off)
{
#if FM_RDS_ENABLE
	return 0;
#endif
}

static int fm_ops_open(struct inode *inode, struct file *filp)
{
    struct fm *fm = container_of(inode->i_cdev, struct fm, cdev);

    if (down_interruptible(&fm_ops_mutex))
        return -EFAULT;
    fm->ref++;
    WCN_DBG(L4|D_IOCTL, "fm->ref:%d\n", fm->ref);
    if(fm->ref == 1) {
        if (fm->chipon == false) {
            mt6616_poweron(MT6616_FM);
            fm->chipon = true;

#if USE_INTERRUPT
			mt_set_gpio_mode(GPIO_FM_RDS_PIN, GPIO_FM_RDS_PIN_M_GPIO);
			
			mt_set_gpio_pull_enable(GPIO_FM_RDS_PIN, GPIO_PULL_ENABLE);
			mt_set_gpio_pull_select(GPIO_FM_RDS_PIN, GPIO_PULL_UP);
			mt_set_gpio_mode(GPIO_FM_RDS_PIN, GPIO_FM_RDS_PIN_M_EINT);
			fm_request_eint(NULL);
#endif
        }
    }
    
    // EINT RDS
    
    up(&fm_ops_mutex);
#if FM_RDS_ENABLE
    fm->RDS_Data_ready = false;    
#endif
    filp->private_data = fm;
    
    WCN_DBG(L4|D_IOCTL, "-T:%d, fm_ops_open\n", jiffies_to_msecs(jiffies));
    
    return 0;
}

static int fm_ops_release(struct inode *inode, struct file *filp)
{
    int err = 0;
    struct fm *fm = container_of(inode->i_cdev, struct fm, cdev);
    struct i2c_client *client = fm->i2c_client;

    if (down_interruptible(&fm_ops_mutex))
        return -EFAULT;
    
    fm->ref--;
    WCN_DBG(L4|D_IOCTL, "fm_ops_release fm->ref:%d\n", fm->ref);
    if(fm->ref < 1) {
        if(fm->powerup == true) {
            fm_powerdown(fm);           
        }
        
        if (fm->chipon == true) {
#if USE_INTERRUPT
   			mt_set_gpio_mode(GPIO_FM_RDS_PIN, GPIO_FM_RDS_PIN_M_GPIO); 
   			mt_set_gpio_dir(GPIO_FM_RDS_PIN, GPIO_DIR_IN); 
#endif
            mt6616_poweroff(MT6616_FM);
            fm->chipon = false;
        }
    }
    
    // Set GPIO mode input for low power, need modify or not ?

    filp->private_data = NULL;
    
    up(&fm_ops_mutex);

    return err;
}

static int fm_init(struct i2c_client *client)
{
    int err;
    struct fm *fm = NULL;

    if (!(fm = kzalloc(sizeof(struct fm), GFP_KERNEL)))
    {
        WCN_DBG(L4|D_ALL, "-ENOMEM\n");
        err = -ENOMEM;
        goto ERR_EXIT;
    }

    fm->ref = 0;
    fm->chipon = false;
    fm->powerup = false;
    fm->chip_id = 0x6616;   

    if ((err = fm_setup_cdev(fm)))
    {
		WCN_DBG(L4|D_ALL, "fm_setup_cdev failed\n");
        goto ERR_EXIT;
    }

    fm->i2c_client = client;
    i2c_set_clientdata(client, fm);
     
    init_waitqueue_head(&fm->read_wait);
#if USE_INTERRUPT
    init_waitqueue_head(&fm_stc_done_wait);
#endif
    
#if FM_RDS_ENABLE
    if (!(fm->pstRDSData = kmalloc(sizeof(RDSData_Struct), GFP_KERNEL)))
    {
        WCN_DBG(L4|D_ALL,"-ENOMEM for RDS\n");
        err = -ENOMEM;
        goto ERR_EXIT;
    }
    
    g_fm_struct = fm;
    
    fm->fm_rds_reset_workqueue = create_singlethread_workqueue("fm_rds_reset_workqueue");
    if(!fm->fm_rds_reset_workqueue)
    {
        WCN_DBG(L4|D_ALL, "-ENOMEM for fm_rds_reset_workqueue\n");
        err = -ENOMEM;
        goto ERR_EXIT;
    }
    
    INIT_WORK(&fm->fm_rds_reset_work, fm_rds_reset_workqueue_func);
#endif

#if USE_INTERRUPT
    fm->fm_workqueue = create_singlethread_workqueue("fm_workqueue");
    if(!fm->fm_workqueue)
    {
        WCN_DBG(L4|D_ALL, "-ENOMEM for fm_workqueue\n");
        err = -ENOMEM;
        goto ERR_EXIT;
    }
    
    //fm->fm_work.data = fm;
    INIT_WORK(&fm->fm_work, fm_workqueue_func);
#endif

	/*Add porc file system*/
	g_fm_proc = create_proc_entry(FM_PROC_FILE, 0666, NULL);
	if (g_fm_proc == NULL) {
		WCN_DBG(L4|D_ALL, "create_proc_entry failed\n");
		err = -ENOMEM;
		goto ERR_EXIT;
	} else {
		g_fm_proc->read_proc = fm_proc_read;
		g_fm_proc->write_proc = fm_proc_write;
		WCN_DBG(L4|D_ALL, "create_proc_entry success\n");
	}

    return 0;

ERR_EXIT:
    if(fm->fm_workqueue)
    {
        destroy_workqueue(fm->fm_workqueue);
    }

#if FM_RDS_ENABLE    
    if(fm->fm_rds_reset_workqueue)
    {
        destroy_workqueue(fm->fm_rds_reset_workqueue);
    }
#endif
    kfree(fm);

    return err;
}

static int fm_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int cnt= 0;
	WCN_DBG(L7|D_IOCTL, "Enter fm_proc_read.\n");
	if(off != 0)
		return 0;
	struct fm *fm  = g_fm_struct;
	if (fm != NULL && fm->chipon && fm->powerup) {
		cnt = sprintf(page, "1\n");
	} else {
		cnt = sprintf(page, "0\n");
	}
	*eof = 1;
	WCN_DBG(L7|D_IOCTL, "Leave fm_proc_read. FM_on = %s\n", page);
	return cnt;
}

static int fm_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	char tmp_buf[11] = {0};
	uint32_t copysize;
	
	copysize = (count < (sizeof(tmp_buf) - 1)) ? count : (sizeof(tmp_buf) - 1);
	if (copy_from_user(tmp_buf, buffer, copysize)) {
		WCN_DBG(L4|D_ALL, "failed copy_from_user\n");
        return -EFAULT;
    }

	if (sscanf(tmp_buf, "%x", &g_dbg_level) != 1) {
		WCN_DBG(L4|D_ALL, "failed g_dbg_level = 0x%x\n", g_dbg_level);
		return -EFAULT;
	}

	WCN_DBG(L7|D_IOCTL, "success g_dbg_level = 0x%x\n", g_dbg_level);

	return count;
}

static int fm_destroy(struct fm *fm)
{
    int err = 0;

    WCN_DBG(L4|D_ALL, "%s\n", __func__);
	remove_proc_entry(FM_PROC_FILE, NULL);
    device_destroy(fm->cls, fm->dev_t);
    class_destroy(fm->cls);

    cdev_del(&fm->cdev);
    unregister_chrdev_region(fm->dev_t, 1);

    // FIXME: any other hardware configuration ?
    if(fm->fm_workqueue)
    {
        destroy_workqueue(fm->fm_workqueue);
    }
    
#if FM_RDS_ENABLE
    if(fm->fm_rds_reset_workqueue)
    {
        destroy_workqueue(fm->fm_rds_reset_workqueue);
    }
    
    if (fm->pstRDSData)
    {
        kfree(fm->pstRDSData);
        fm->pstRDSData = NULL;
    }
#endif

    // free all memory
    if(fm)
    {
        kfree(fm);
        fm = NULL;
        g_fm_struct = NULL;
    }
    

    return err;
}

/*
 *  fm_powerup
 */
static int fm_powerup(struct fm *fm, struct fm_tune_parm *parm)
{
    int i;
    uint16_t tmp_reg, tmp_reg1, cnt=0;
    struct i2c_client *client = fm->i2c_client;

#if 0
    if (fm->powerup)
    {
        parm->err = FM_BADSTATUS;
        return -EPERM;
    }
#endif
    
    //for normal case
    if (fm->chipon == false) 
    {
        mt6616_poweron(MT6616_FM);
        fm->chipon = true;
    }
    
    WCN_DBG(L5|D_IOCTL, "+MT6616 power on procedure\n");

    // MT6616 FM power on sequence
	for(i = 0; i < POWER_ON_COMMAND_COUNT; i++)
	{
        if(PowerOnSetting[i].addr == 0xFFFF) //polling status 1
		{   
		    cnt = 0;
       		do
			{		
       			MT6616_read(client, (uint8_t)PowerOnSetting[i].and, &tmp_reg);
	   			tmp_reg &= PowerOnSetting[i].or;
				if(tmp_reg == 0) {
					Delayms(10);
				    cnt++;
				}
            }while((tmp_reg == 0) && (cnt < (MT6616_MAX_COUNT<<1)));
            
            if(cnt == (MT6616_MAX_COUNT<<1)) {
                WCN_DBG(L4|D_ALL, "MT6616 polling status Active failed:0x%02X\n", (uint8_t)PowerOnSetting[i].and);  
                parm->err = FM_BADSTATUS;
                return -EPERM;
            }
		}
		else if(PowerOnSetting[i].addr == 0xFFFE) //polling status=0
		{   
		    cnt = 0;
            do
            {		
                MT6616_read(client, (uint8_t)PowerOnSetting[i].and, &tmp_reg);
	            tmp_reg &= PowerOnSetting[i].or;
				if(tmp_reg != 0) {
				    Delayms(10);
	   			    cnt++;
	   			}		
            }while((tmp_reg != 0) && (cnt < MT6616_MAX_COUNT));
            
            if(cnt == MT6616_MAX_COUNT) {
                WCN_DBG(L4|D_ALL, "MT6616 polling status Negative failed:0x%02X\n", (uint8_t)PowerOnSetting[i].and);  
                parm->err = FM_BADSTATUS;
                return -EPERM;    
            }
        }
        else if(PowerOnSetting[i].addr == 0xFFFD) //specific control
		{
		    switch(PowerOnSetting[i].and)
			{
	   	        case 0x32:
	   	            MT6616_read(client, (uint8_t)PowerOnSetting[i].and, &tmp_reg);
	     		    fm->chip_id = tmp_reg;
	     		    fm->device_id = tmp_reg;
	     		    WCN_DBG(L4|D_ALL, "MT6616 chip_id:%04x\n", tmp_reg);
	     			break;
          	    case 0x48:
            	    MT6616_read(client, (uint8_t)PowerOnSetting[i].and, &tmp_reg); //0x48
            		MT6616_read(client, (uint8_t)PowerOnSetting[i].or,  &tmp_reg1); //0x42 or 0x43
            		MT6616_write(client,(uint8_t)PowerOnSetting[i].or,  ((tmp_reg1&0xC0C0)|(tmp_reg&0x3F3F)));
	     			break;
	   			default:
	     			break;
	 		}
        }
        else if(PowerOnSetting[i].addr == 0xFFFB) //delay ms
		{   
            Delayms(PowerOnSetting[i].or);
      	}
        else if(PowerOnSetting[i].addr == 0xFFFA) //delay us
		{   
        	Delayus(PowerOnSetting[i].or);
      	}  
      	else //valid chip register address
		{
			if(PowerOnSetting[i].and != 0) //set
            {
        		if (MT6616_read(client, (uint8_t)PowerOnSetting[i].addr, &tmp_reg))
        		{
        		    WCN_DBG(L4|D_ALL, "MT6616 power up failed, can't read reg %02X\n", (uint8_t)PowerOnSetting[i].and);      
        		    return -EPERM;
        		}
        		tmp_reg &= PowerOnSetting[i].and;
        		tmp_reg |= PowerOnSetting[i].or;	  	
      		}
	        else {
		        tmp_reg = PowerOnSetting[i].or;
            }
            
            if (MT6616_write(client, (uint8_t)PowerOnSetting[i].addr, tmp_reg))
            {
                WCN_DBG(L4|D_ALL, "MT6616 power up failed, can't write reg %02X\n", (uint8_t)PowerOnSetting[i].addr);      
        		return -EPERM; 
            }
	    }
	}
	
	WCN_DBG(L5|D_ALL, "-MT6616 power on procedure\n");
#if USE_INTERRUPT	
	fm_enable_eint();
#endif
    
    WCN_DBG(L4|D_ALL, "pwron ok\n");
    fm->powerup = true;
    _current_frequency = 0xffff;

    if (0 != fm_tune(fm, parm))
    {
        return -EPERM;
    }
    
    parm->err = FM_SUCCESS;

    return 0;
}

/*
 *  fm_powerdown
 */
static int fm_powerdown(struct fm *fm)
{
    int i;
	uint16_t tmp_reg;   
    struct i2c_client *client = fm->i2c_client;   
      
    //disable all intr
#if FM_RDS_ENABLE
    fm_disable_rds_BlerCheck();
    MT6616_RDS_OnOff(fm, false);
#endif

#if USE_INTERRUPT
    MT6616_set_bits(client, FM_MAIN_EXTINTRMASK, 0, 0); 
    fm_disable_eint();
#endif

    for (i = 0; i < POWER_OFF_COMMAND_COUNT; i++)
	{
	    if(PowerOffProc[i].addr == 0xFFFB)     //delay ms
		{   
            Delayms(PowerOffProc[i].or);
      	}
        else if(PowerOffProc[i].addr == 0xFFFA) //delay us
		{   
        	Delayus(PowerOffProc[i].or);
      	}
      	else {
      	    if(PowerOffProc[i].and != 0) //set
            {
        		MT6616_read(client, (uint8_t)PowerOffProc[i].addr, &tmp_reg);
        		tmp_reg &= PowerOffProc[i].and;
        		tmp_reg |= PowerOffProc[i].or;	  	
      		}
	        else {
		        tmp_reg = PowerOffProc[i].or;
		    }
		        	
		    MT6616_write(client, (uint8_t)PowerOffProc[i].addr, tmp_reg);
	    }
	}
	
    fm->powerup = false;
    WCN_DBG(L4|D_IOCTL, "pwrdown ok\n");

    return 0;
}


/*
 *  fm_seek
 */
static int fm_seek(struct fm *fm, struct fm_seek_parm *parm)
{
    int ret = 0;
    uint16_t seekdir, space;
    struct i2c_client *client = fm->i2c_client;

    if (!fm->powerup)
    {
        parm->err = FM_BADSTATUS;
        return -EPERM;
    }

    if (parm->space == FM_SPACE_100K) {
        space = MT6616_FM_SPACE_100K;
    }
    else if (parm->space == FM_SPACE_200K) {
        space = MT6616_FM_SPACE_200K;
    }
    else //default
    {
        space = MT6616_FM_SPACE_100K;  
    }

    if (parm->band == FM_BAND_UE) {
        fm->min_freq = 875;
        fm->max_freq = 1080;
    }
    else if (parm->band == FM_BAND_JAPAN) {
        fm->min_freq = 760;
        fm->max_freq = 900;
    }
    else if (parm->band == FM_BAND_JAPANW) {
        fm->min_freq = 760;
        fm->max_freq = 1080;
    }
    else
    {
        WCN_DBG(L4|D_ALL, "band:%d out of range\n", parm->band);
        parm->err = FM_EPARM;
        return -EPERM;
    }

    if (parm->freq < fm->min_freq || parm->freq > fm->max_freq) {
        WCN_DBG(L4|D_ALL, "freq:%d out of range\n", parm->freq);
        parm->err = FM_EPARM;
        return -EPERM;
    }

    if (parm->seekdir == FM_SEEK_UP) {
        seekdir = MT6616_FM_SEEK_UP;
    }
    else 
    {
        seekdir = MT6616_FM_SEEK_DOWN;    
    }

    // seek successfully
    if(true == MT6616_Seek(client, fm->min_freq, fm->max_freq, &(parm->freq), seekdir, space)) {
        parm->err = FM_SUCCESS;
    }
    else
    {
        parm->err = FM_SEEK_FAILED;
        ret = -EPERM;
    }

    return ret;
}

/*
 *  fm_scan
 */
static int  fm_scan(struct fm *fm, struct fm_scan_parm *parm)
{
    int ret = 0;
    uint16_t scandir = MT6616_FM_SCAN_UP, space;
    struct i2c_client *client = fm->i2c_client;

    if (!fm->powerup)
    {
        parm->err = FM_BADSTATUS;
        return -EPERM;
    }

    if (parm->space == FM_SPACE_100K) {
        space = MT6616_FM_SPACE_100K;
    }
    else if (parm->space == FM_SPACE_200K) {
        space = MT6616_FM_SPACE_200K;
    }
    else //default
    {
        space = MT6616_FM_SPACE_100K;  
    }

    if (parm->band == FM_BAND_UE) {
        fm->min_freq = 875;
        fm->max_freq = 1080;
    }
    else if (parm->band == FM_BAND_JAPAN) {
        fm->min_freq = 760;
        fm->max_freq = 900;
    }
    else if (parm->band == FM_BAND_JAPANW) {
        fm->min_freq = 760;
        fm->max_freq = 1080;
    }
    else
    {
        WCN_DBG(L4|D_ALL, "fm_scan band:%d out of range\n", parm->band);
        parm->err = FM_EPARM;
        return -EPERM;
    }
    
    if(true == MT6616_Scan(client, fm->min_freq, fm->max_freq, &(parm->freq), parm->ScanTBL, &(parm->ScanTBLSize), scandir, space))
    {
        parm->err = FM_SUCCESS;
    }
    else
    {
        WCN_DBG(L4|D_ALL, "fm_scan failed\n");
        parm->err = FM_SEEK_FAILED;
        ret = -EPERM;
    }
        
    return ret;   
}

//volume?[0~15]
static int fm_setvol(struct fm *fm, uint32_t vol)
{
    uint8_t tmp_vol;
    struct i2c_client *client = fm->i2c_client;
    
    tmp_vol = vol*3;
    MT6616_SetVol(client, tmp_vol);
    return 0;
}

static int fm_getvol(struct fm *fm, uint32_t *vol)
{
    uint8_t tmp_vol;
    struct i2c_client *client = fm->i2c_client;
    
    MT6616_GetVol(client, &tmp_vol);
    if(tmp_vol == FM_VOL_MAX)
        *vol = 15;
    else
        *vol = (tmp_vol/3);
           
    return 0;
}

static int fm_getrssi(struct fm *fm, uint32_t *rssi)
{
    uint16_t val = 0;
    struct i2c_client *client = fm->i2c_client;
    
    MT6616_GetCurRSSI(client, &val);
    
    *rssi = (uint32_t)val;
        
    return 0;
}

/*
 *  fm_tune
 */
static int fm_tune(struct fm *fm, struct fm_tune_parm *parm)
{
    int ret = 0;
    struct i2c_client *client = fm->i2c_client;

    if (!fm->powerup)
    {
        parm->err = FM_BADSTATUS;
        return -EPERM;
    }

    if (parm->band == FM_BAND_UE) {
        fm->min_freq = 875;
        fm->max_freq = 1080;
    }
    else if (parm->band == FM_BAND_JAPAN) {
        fm->min_freq = 760;
        fm->max_freq = 900;
    }
    else if (parm->band == FM_BAND_JAPANW) {
        fm->min_freq = 760;
        fm->max_freq = 1080;
    }
    else
    {
        parm->err = FM_EPARM;
        return -EPERM;
    }        

    if (unlikely(parm->freq < fm->min_freq || parm->freq > fm->max_freq)) {
        parm->err = FM_EPARM;
        return -EPERM;
    }

#if 0
	if (_current_frequency == parm->freq)
    {
        WCN_DBG(L6|D_IOCTL, "fm_tune set freq same as _current_frequency\n");
        return 0;     
    }
#endif

    MT6616_Mute(client, true);
    MT6616_RampDown(client);
    MT6616_Freq_Avoid(client, parm->freq);

	if (_current_frequency != parm->freq) {
    	memset(fm->pstRDSData, 0, sizeof(RDSData_Struct));
	}

    if(false == MT6616_SetFreq(client, parm->freq))
    {
        parm->err = FM_TUNE_FAILED;   
        WCN_DBG(L4|D_ALL, "FM tune failed\n"); 
        ret = -EPERM;
    }
	WCN_DBG(L5|D_IOCTL, "tune to %d success.\n", parm->freq);

    MT6616_Mute(client, false);
	
    return ret;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))  
/*
 *  fm_i2c_attach_adapter
 */
static int fm_i2c_attach_adapter(struct i2c_adapter *adapter)
{
    if (adapter->id == MT6616_I2C_PORT)
    {
        return i2c_probe(adapter, &mt6616_addr_data, fm_i2c_detect);
    }
    else
        return 0;
}

/*
 *  fm_i2c_detect
 *  This function is called by i2c_detect
 */
static int fm_i2c_detect(struct i2c_adapter *adapter, int addr, int kind)
{
    int err;
    struct i2c_client *client = NULL;

    /* skip this since MT6516 shall support all the needed functionalities
    if (!i2c_check_functionality(adapter, xxx))
    {
        FM_DEBUG("i2c_check_functionality failed\n");
        return -ENOTSUPP;
    }
    */

    /* initial i2c client */
    if (!(client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL)))
    {
        WCN_DBG(L4|D_ALL, "kzalloc failed\n");
        err = -ENOMEM;
        goto ERR_EXIT;
    }

    client->addr = addr;
    //client->addr |= I2C_A_CHANGE_TIMING;
    //client->timing = (5<<8)|(9<<0);  //(5, 9)
    client->adapter = adapter;
    client->driver = &MT6616_driver;
    client->flags = 0;
    
    strncpy(client->name, "MT6616 FM RADIO", I2C_NAME_SIZE);

    if ((err = fm_init(client)))
    {
        WCN_DBG(L4|D_ALL, "fm_init ERR:%d\n", err);
        goto ERR_EXIT;
    }

    if ((err = i2c_attach_client(client)))
    {
        WCN_DBG(L4|D_ALL, "i2c_attach_client ERR:%d\n", err);
        goto ERR_EXIT;
    }

    return 0;

ERR_EXIT:
    kfree(client);

    return err;
}

static int fm_i2c_detach_client(struct i2c_client *client)
{
    int err = 0;
	WCN_DBG(L4|D_ALL,"Enter %s\n", __FUNCTION__);
    struct fm *fm = i2c_get_clientdata(client);

    err = i2c_detach_client(client);
    if (err)
    {
        dev_err(&client->dev, "fm_i2c_detach_client failed\n");
        return err;
    }

    fm_destroy(fm);
    kfree(client);

    return err;
}
#else
static int fm_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int err;
    WCN_DBG(L4|D_ALL,"Enter %s\n", __FUNCTION__);
    if ((err = fm_init(client)))
    {
        WCN_DBG(L4|D_ALL, "fm_init ERR:%d\n", err);
        goto ERR_EXIT;
    }   
    WCN_DBG(L4|D_ALL,"Leave %s\n", __FUNCTION__);
    return 0;   
    
ERR_EXIT:
    return err;    
}

static int fm_i2c_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    WCN_DBG(L4|D_ALL,"Enter %s\n", __FUNCTION__);
    strcpy(info->type, MT6616_DEV);
	WCN_DBG(L4|D_ALL,"Leave %s\n", __FUNCTION__);
    return 0;
}

static int fm_i2c_remove(struct i2c_client *client)
{
    struct fm *fm = i2c_get_clientdata(client);

    WCN_DBG(L4|D_ALL,"fm_i2c_remove\n");
    if(fm)
    {    
        fm_destroy(fm);
        fm = NULL;
    }
    
    return 0;
}
#endif

static int mt_fm_probe(struct platform_device *pdev)
{
    int err = -1;
    WCN_DBG(L4|D_ALL,"Enter %s\n", __FUNCTION__);

    // Open I2C driver
    // TODO: fast mode

    err = i2c_add_driver(&MT6616_driver);
    if (err)
    {
        WCN_DBG(L4|D_ALL,"i2c err\n");
    }
	WCN_DBG(L4|D_ALL,"Leave %s\n", __FUNCTION__);
    return err;   
} 
    
static int mt_fm_remove(struct platform_device *pdev)
{
    WCN_DBG(L4|D_ALL,"mt_fm_remove\n");
    i2c_del_driver(&MT6616_driver); 
    
    return 0; 
}


static struct platform_driver mt_fm_dev_drv =
{
    .probe   = mt_fm_probe,
    .remove  = mt_fm_remove,
#if 0//def CONFIG_PM //Not need now   
    .suspend = mt_fm_suspend,
    .resume  = mt_fm_resume,
#endif    
    .driver = {
        .name   = FM_NAME,
        .owner  = THIS_MODULE,    
    }
};

/*
 *  mt_fm_init
 */
static int __init mt_fm_init(void)
{
	int err = 0;

	WCN_DBG(L4|D_ALL,"Enter %s\n", __FUNCTION__);

#ifdef MT6516
	WCN_DBG(L4|D_ALL,"i2c_add_driver PORT = 2, client addr = 0xe0\n");
#else
	WCN_DBG(L4|D_ALL,"i2c_add_driver PORT = 1, client addr = 0xe0\n");
#endif

	err = platform_driver_register(&mt_fm_dev_drv);
    if (err)
    {
        WCN_DBG(L4|D_ALL,"platform_driver_register failed\n");
    }

	init_timer(&fm_rds_reset_timer);
	g_stop_timer = 1;
    fm_rds_reset_timer.expires  = jiffies + gBLER_CHK_INTERVAL/(1000/HZ);
    fm_rds_reset_timer.function = fm_rds_reset_func;
    fm_rds_reset_timer.data     = (unsigned long)g_fm_struct;
	add_timer(&fm_rds_reset_timer);

	WCN_DBG(L4|D_ALL,"Leave %s\n", __FUNCTION__);
	return err;
}

/*
 *  mt_fm_exit
 */
static void __exit mt_fm_exit(void)
{
    WCN_DBG(L4|D_ALL,"Enter %s\n", __FUNCTION__);
	del_timer(&fm_rds_reset_timer);
    platform_driver_unregister(&mt_fm_dev_drv);
	WCN_DBG(L4|D_ALL,"Leave %s\n", __FUNCTION__);
}

module_init(mt_fm_init);
module_exit(mt_fm_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek MT6616 FM Driver");
MODULE_AUTHOR("Chunhui <Chunhui.li@MediaTek.com>");

