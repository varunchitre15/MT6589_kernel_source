

#define MT65XX_TOUCH_IRQ_LINE  MT_TS_IRQ_ID
#define MT65XX_TOUCH_BATCH_LINE MT_TSATCH_IRQ_ID
#define MT65XX_PDN_PERI_TP MT_CG_PERI1_AUXADC

#define AUXADC_TP_DEBT0          (AUXADC_BASE + 0x0054)
#define AUXADC_TP_DEBT1          (AUXADC_BASE + 0x0058)
#define AUXADC_TP_CMD            (AUXADC_BASE + 0x005c)
#define AUXADC_TP_ADDR           (AUXADC_BASE + 0x0060)
#define AUXADC_TP_CON0           (AUXADC_BASE + 0x0064)
#define AUXADC_TP_CON1           (AUXADC_BASE + 0x0068)
#define AUXADC_TP_CON2           (AUXADC_BASE + 0x006c)
#define AUXADC_TP_CON3           (AUXADC_BASE + 0x0070)
#define AUXADC_TP_DATA0          (AUXADC_BASE + 0x0074)
#define AUXADC_TP_DATA1          (AUXADC_BASE + 0x0078)
#define AUXADC_TP_DATA2          (AUXADC_BASE + 0x007c)
#define AUXADC_TP_DATA3          (AUXADC_BASE + 0x0080)
#define AUXADC_DET_VOLT          (AUXADC_BASE + 0x0084)
#define AUXADC_DET_SEL           (AUXADC_BASE + 0x0088)
#define AUXADC_DET_PERIOD        (AUXADC_BASE + 0x008c)
#define AUXADC_DET_DEBT          (AUXADC_BASE + 0x0090)
#define AUXADC_MISC              (AUXADC_BASE + 0x0094)
#define AUXADC_ECC               (AUXADC_BASE + 0x0098)
#define AUXADC_SAMPLE_LIST       (AUXADC_BASE + 0x009c)
#define AUXADC_ABIST_PERIOD      (AUXADC_BASE + 0x00a0)
#define AUXADC_TP_RAW_CON        (AUXADC_BASE + 0x0100)
#define AUXADC_TP_AUTO_TIME_INTVL (AUXADC_BASE + 0x0104)
#define AUXADC_TP_RAW_X_DAT0      (AUXADC_BASE + 0x0200)
#define AUXADC_TP_RAW_Y_DAT0      (AUXADC_BASE + 0x0220)
#define AUXADC_TP_RAW_Z1_DAT0     (AUXADC_BASE + 0x0240)
#define AUXADC_TP_RAW_Z2_DAT0     (AUXADC_BASE + 0x0260)
/*
#define AUXADC_TP_DEBT0          (AUXADC_BASE + 0x0054)
#define AUXADC_TP_DEBT1          (AUXADC_BASE + 0x0058)
#define AUXADC_TP_CMD            (AUXADC_BASE + 0x005c)
#define AUXADC_TP_ADDR           (AUXADC_BASE + 0x0060)
#define AUXADC_TP_CON0           (AUXADC_BASE + 0x0064)
#define AUXADC_TP_CON1           (AUXADC_BASE + 0x0068)
#define AUXADC_TP_CON2           (AUXADC_BASE + 0x006c)
#define AUXADC_TP_CON3           (AUXADC_BASE + 0x0070)
#define AUXADC_TP_DATA0          (AUXADC_BASE + 0x0074)
#define AUXADC_TP_DATA1          (AUXADC_BASE + 0x0078)
#define AUXADC_TP_DATA2          (AUXADC_BASE + 0x007c)
#define AUXADC_TP_DATA3          (AUXADC_BASE + 0x0080)
#define AUXADC_CON3              (AUXADC_BASE + 0x0014)
*/
#define AUXADC_CON3_STA_MASK  0x0001
#define TP_DEBT_MASK          0x3fff
#define TP_CMD_PD_MASK        0x0003
#define TP_CMD_PD_YDRV_SH     0x0000
#define TP_CMD_PD_IRQ_SH      0x0001
#define TP_CMD_PD_IRQ         0x0003
#define TP_CMD_SE_DF_MASK     0x0004
#define TP_CMD_DIFFERENTIAL   0x0000
#define TP_CMD_SINGLE_END     0x0004
#define TP_CMD_MODE_MASK      0x0008
#define TP_CMD_MODE_12BIT     0x0000
#define TP_CMD_MODE_10BIT     0x0008
#define TP_CMD_ADDR_MASK      0x0007
#define TP_CMD_ADDR_Y         0x0001
#define TP_CMD_ADDR_Z1        0x0003
#define TP_CMD_ADDR_Z2        0x0004
#define TP_CMD_ADDR_X         0x0005
#define TP_CON_SPL_MASK       0x0001
#define TP_CON_SPL_TRIGGER    0x0001 // data0 reg
#define TP_CON_STATUS_MASK    0x0002
#define TP_DAT0_DAT_MASK      0x03ff
#define TP_DEBOUNCE_TIME      (4*32) // 4ms
#define TP_AUXADC_POWER_UP    0x0c000c00

/* AUXADC_TS_CON1 reg */
#define FAV_ADEL_BIT 8
#define FAV_EN_BIT   7
#define FAV_COORDSEL 5
#define FAV_INVALID  4
#define FAV_ASAMP    3
#define FAV_SEL      2
#define FAV_LCNT     0


/* DIFFERENTIAL | MODE_12BIT | PD_YDRV_SH */
#define TP_SAMPLE_SETTING     0x0000 

void tpd_adc_init(void);
void tpd_set_debounce_time_DEBT0(int debounce_time);
void tpd_set_debounce_time_DEBT1(int debounce_time);
unsigned int tpd_get_debounce_time_DEBT0(void);
unsigned int tpd_get_debounce_time_DEBT1(void);
void tpd_set_spl_number(int spl_num);
unsigned int tpd_get_spl_number(void);
u16 tpd_read(int position, int raw_data_offset);
u16 tpd_read_adc(u16 pos);
u16 tpd_read_status(void);
void tpd_fav_switch(int on_off);
void tpd_fav_config(int coord, int cnt, int asamp, int adel);
void tpd_fav_auto_interval(unsigned int ms);   
unsigned int tpd_get_sample_cnt(void);
void tpd_fav_set_auto_interval(unsigned int ms);
unsigned int tpd_fav_get_auto_interval(void);
void tpd_clear_invalid_flag(void);
unsigned int tpd_get_asamp(void);

enum tpd_mode {
	TPD_SW=0x01,
	FAV_MODE_SW=0x02,
	FAV_MODE_HW=0x04,
	RAW_DATA_MODE=0x08,
};
