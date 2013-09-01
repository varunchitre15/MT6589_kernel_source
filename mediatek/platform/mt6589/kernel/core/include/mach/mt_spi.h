#ifndef __MT_SPI_H__
#define __MT_SPI_H__


#include <linux/types.h>
#include <linux/io.h>
/*******************************************************************************
* define macro for spi gpio mode
********************************************************************************/
//#ifndef GPIO_SPI_CS_PIN
//  #define GPIO_SPI_CS_PIN                 83
//#endif
//
//#ifndef GPIO_SPI_SCK_PIN
//  #define GPIO_SPI_SCK_PIN                86
//#endif
//
//#ifndef GPIO_SPI_MISO_PIN
//  #define GPIO_SPI_MISO_PIN               84
//#endif
//
//#ifndef GPIO_SPI_MOSI_PIN
//  #define GPIO_SPI_MOSI_PIN               85
//#endif
//
//#ifndef GPIO_SPI_CS_PIN_M_SPI_CS_N
//  #define GPIO_SPI_CS_PIN_M_SPI_CS_N      1
//#endif
//#ifndef GPIO_SPI_SCK_PIN_M_SPI_SCK
//  #define GPIO_SPI_SCK_PIN_M_SPI_SCK      1
//#endif
//#ifndef GPIO_SPI_MISO_PIN_M_SPI_MISO
//  #define GPIO_SPI_MISO_PIN_M_SPI_MISO    1
//#endif
//#ifndef GPIO_SPI_MOSI_PIN_M_SPI_MOSI
//  #define GPIO_SPI_MOSI_PIN_M_SPI_MOSI    1
//#endif
//
//#ifndef GPIO_SPI_CS_PIN_M_GPIO
//  #define GPIO_SPI_CS_PIN_M_GPIO          0
//#endif
//#ifndef GPIO_SPI_SCK_PIN_M_GPIO
//  #define GPIO_SPI_SCK_PIN_M_GPIO         0
//#endif
//
//#ifndef GPIO_SPI_MISO_PIN_M_GPIO
//  #define GPIO_SPI_MISO_PIN_M_GPIO        0
//#endif
//
//#ifndef GPIO_SPI_MOSI_PIN_M_GPIO
//  #define GPIO_SPI_MOSI_PIN_M_GPIO        0
//#endif
/*******************************************************************************
* define macro for spi register
********************************************************************************/
#define SPI_CFG0_REG                      (0x0000)
#define SPI_CFG1_REG                      (0x0004)
#define SPI_TX_SRC_REG                    (0x0008)
#define SPI_RX_DST_REG                    (0x000c)
#define SPI_TX_DATA_REG                   (0x0010)
#define SPI_RX_DATA_REG                   (0x0014)
#define SPI_CMD_REG                       (0x0018)
#define SPI_STATUS0_REG                   (0x001c)
#define SPI_STATUS1_REG                   (0x0020)
#define SPI_GMC_SLOW_REG                  (0x0024)
#define SPI_ULTRA_HIGH_REG                (0x0028)

#define MIPI_TX_PU_TDP1_OFFSET            4
#define MIPI_TX_PU_TDP1_MASK              0x10
#define MIPI_TX_PD_TDP1_OFFSET            12
#define MIPI_TX_PD_TDP1_MASK              0x1000

#define SPI_CFG0_SCK_HIGH_OFFSET          0
#define SPI_CFG0_SCK_LOW_OFFSET           8
#define SPI_CFG0_CS_HOLD_OFFSET           16
#define SPI_CFG0_CS_SETUP_OFFSET          24

#define SPI_CFG0_SCK_HIGH_MASK            0xff
#define SPI_CFG0_SCK_LOW_MASK             0xff00
#define SPI_CFG0_CS_HOLD_MASK             0xff0000
#define SPI_CFG0_CS_SETUP_MASK            0xff000000

#define SPI_CFG1_CS_IDLE_OFFSET           0
#define SPI_CFG1_PACKET_LOOP_OFFSET       8
#define SPI_CFG1_PACKET_LENGTH_OFFSET     16
#define SPI_CFG1_GET_TICK_DLY_OFFSET      30

#define SPI_CFG1_CS_IDLE_MASK             0xff
#define SPI_CFG1_PACKET_LOOP_MASK         0xff00
#define SPI_CFG1_PACKET_LENGTH_MASK       0x3ff0000
#define SPI_CFG1_GET_TICK_DLY_MASK        0xc0000000

#define SPI_CMD_ACT_OFFSET                0
#define SPI_CMD_RESUME_OFFSET             1
#define SPI_CMD_RST_OFFSET                2
#define SPI_CMD_PAUSE_EN_OFFSET           4
#define SPI_CMD_DEASSERT_OFFSET           5
#define SPI_CMD_CPHA_OFFSET               8
#define SPI_CMD_CPOL_OFFSET               9 
#define SPI_CMD_RX_DMA_OFFSET             10
#define SPI_CMD_TX_DMA_OFFSET             11
#define SPI_CMD_TXMSBF_OFFSET             12
#define SPI_CMD_RXMSBF_OFFSET             13
#define SPI_CMD_RX_ENDIAN_OFFSET          14
#define SPI_CMD_TX_ENDIAN_OFFSET          15
#define SPI_CMD_FINISH_IE_OFFSET          16
#define SPI_CMD_PAUSE_IE_OFFSET           17

#define SPI_CMD_ACT_MASK                  0x1
#define SPI_CMD_RESUME_MASK               0x2
#define SPI_CMD_RST_MASK                  0x4
#define SPI_CMD_PAUSE_EN_MASK             0x10
#define SPI_CMD_DEASSERT_MASK             0x20
#define SPI_CMD_CPHA_MASK                 0x100
#define SPI_CMD_CPOL_MASK                 0x200
#define SPI_CMD_RX_DMA_MASK               0x400
#define SPI_CMD_TX_DMA_MASK               0x800
#define SPI_CMD_TXMSBF_MASK               0x1000
#define SPI_CMD_RXMSBF_MASK               0x2000
#define SPI_CMD_RX_ENDIAN_MASK            0x4000
#define SPI_CMD_TX_ENDIAN_MASK            0x8000
#define SPI_CMD_FINISH_IE_MASK            0x10000
#define SPI_CMD_PAUSE_IE_MASK             0x20000

#define SPI_ULTRA_HIGH_EN_OFFSET          0
#define SPI_ULTRA_HIGH_THRESH_OFFSET      16

#define SPI_ULTRA_HIGH_EN_MASK            0x1
#define SPI_ULTRA_HIGH_THRESH_MASK        0xffff0000
/*******************************************************************************
* define struct for spi driver
********************************************************************************/
enum spi_cpol {
  SPI_CPOL_0,
  SPI_CPOL_1
};

enum spi_cpha{
  SPI_CPHA_0,
  SPI_CPHA_1
};

enum spi_mlsb {
  SPI_LSB,
  SPI_MSB
};

enum spi_endian {
  SPI_LENDIAN,
  SPI_BENDIAN
};

enum spi_transfer_mode {
  FIFO_TRANSFER,
  DMA_TRANSFER,
  OTHER1,
  OTHER2,
};

enum spi_pause_mode{
  PAUSE_MODE_DISABLE,
  PAUSE_MODE_ENABLE
};
enum spi_finish_intr {
  FINISH_INTR_DIS,
  FINISH_INTR_EN,

};

enum spi_deassert_mode {
  DEASSERT_DISABLE,
  DEASSERT_ENABLE
};

enum spi_ulthigh {
  ULTRA_HIGH_DISABLE,
  ULTRA_HIGH_ENABLE
};

enum spi_tckdly {
  TICK_DLY0,
  TICK_DLY1,
  TICK_DLY2,
  TICK_DLY3
};

struct mt_chip_conf {
  u32 setuptime;
  u32 holdtime;
  u32 high_time;
  u32 low_time;
  u32 cs_idletime;
  u32 ulthgh_thrsh;
  enum spi_cpol cpol;
  enum spi_cpha cpha;
  enum spi_mlsb tx_mlsb;
  enum spi_mlsb rx_mlsb;
  enum spi_endian tx_endian;
  enum spi_endian rx_endian;
  enum spi_transfer_mode com_mod;
  enum spi_pause_mode pause;
  enum spi_finish_intr finish_intr;
  enum spi_deassert_mode deassert;
  enum spi_ulthigh ulthigh;
  enum spi_tckdly tckdly;
};

#define spi_readl(port,offset) \
  __raw_readl((port)->regs+(offset))
#define spi_writel(port, offset,value) \
  __raw_writel((value),(port)->regs+(offset))

#endif

