#include <linux/types.h>

//#define MT6620_FPGA

#define FM_MAIN_PGSEL   (0x9F)
#define FM_MAIN_BASE            (0x0)
#define FM_MAIN_BITMAP0         (FM_MAIN_BASE + 0x80)
#define FM_MAIN_BITMAP1         (FM_MAIN_BASE + 0x81)
#define FM_MAIN_BITMAP2         (FM_MAIN_BASE + 0x82)
#define FM_MAIN_BITMAP3         (FM_MAIN_BASE + 0x83)
#define FM_MAIN_BITMAP4         (FM_MAIN_BASE + 0x84)
#define FM_MAIN_BITMAP5         (FM_MAIN_BASE + 0x85)
#define FM_MAIN_BITMAP6         (FM_MAIN_BASE + 0x86)
#define FM_MAIN_BITMAP7         (FM_MAIN_BASE + 0x87)
#define FM_MAIN_BITMAP8         (FM_MAIN_BASE + 0x88)
#define FM_MAIN_BITMAP9         (FM_MAIN_BASE + 0x89)
#define FM_MAIN_BITMAPA         (FM_MAIN_BASE + 0x8a)
#define FM_MAIN_BITMAPB         (FM_MAIN_BASE + 0x8b)
#define FM_MAIN_BITMAPC         (FM_MAIN_BASE + 0x8c)
#define FM_MAIN_BITMAPD         (FM_MAIN_BASE + 0x8d)
#define FM_MAIN_BITMAPE         (FM_MAIN_BASE + 0x8e)
#define FM_MAIN_BITMAPF         (FM_MAIN_BASE + 0x8f)

int mt6620_off_2_longANA(unsigned char *tx_buf, int tx_buf_size);
int mt6620_off_2_longANA_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rx_digital_init(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rx_digital_init_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_powerdown(unsigned char *tx_buf, int tx_buf_size);
int mt6620_powerdown_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rampdown(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rampdown_tx(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rampdown_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_tune_1(unsigned char *tx_buf, int tx_buf_size, uint16_t freq);
int mt6620_tune_2(unsigned char *tx_buf, int tx_buf_size, uint16_t freq);
int mt6620_tune_3(unsigned char *tx_buf, int tx_buf_size, uint16_t freq);
int mt6620_fast_tune(unsigned char *tx_buf, int tx_buf_size, uint16_t freq);
int mt6620_tune_txscan(unsigned char *tx_buf, int tx_buf_size, uint16_t freq);
int mt6620_tune_tx(unsigned char *tx_buf, int tx_buf_size, uint16_t freq);
int mt6620_tune_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_seek_1(unsigned char *tx_buf, int tx_buf_size, uint16_t seekdir, uint16_t space, uint16_t max_freq, uint16_t min_freq);
int mt6620_seek_2(unsigned char *tx_buf, int tx_buf_size, uint16_t seekdir, uint16_t space, uint16_t max_freq, uint16_t min_freq);
int mt6620_seek_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_scan_1(unsigned char *tx_buf, int tx_buf_size, uint16_t scandir, uint16_t space, uint16_t max_freq, uint16_t min_freq);
int mt6620_scan_2(unsigned char *tx_buf, int tx_buf_size, uint16_t scandir, uint16_t space, uint16_t max_freq, uint16_t min_freq);
int mt6620_scan_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rds_rx_enable(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rds_rx_enable_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rds_rx_disable(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rds_rx_disable_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_rds_tx(unsigned char *tx_buf, int tx_buf_size, uint16_t pi, uint16_t *ps, uint16_t *other_rds, uint8_t other_rds_cnt);
int mt6620_rds_tx_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_get_reg(unsigned char *tx_buf, int tx_buf_size, uint8_t addr);
int mt6620_set_reg(unsigned char *tx_buf, int tx_buf_size, uint8_t addr, uint16_t value);
int mt6620_set_reg_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_off_2_tx_shortANA(unsigned char *tx_buf, int tx_buf_size);
int mt6620_off_2_tx_shortANA_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_dig_init(unsigned char *tx_buf, int tx_buf_size);
int mt6620_dig_init_ack(unsigned char *tx_buf, int tx_buf_size);
int mt6620_com(unsigned char *tx_buf, int tx_buf_size, int opcode, void* data);
int mt6620_soft_mute_tune(fm_u8 *buf, fm_s32 buf_size, fm_u16 freq, fm_s32 cnt, fm_s32 type);

