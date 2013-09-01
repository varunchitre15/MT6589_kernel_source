#ifndef __A320_H__
#define __A320_H__
/******************************************************************************
 * Function Configuration
******************************************************************************/
/******************************************************************************
 * Definition
******************************************************************************/
#define A320_REG_PRODUCT_ID         0x00
#define A320_REG_REVISION_ID        0x01
#define A320_REG_MOTION             0x02
#define A320_REG_DELTA_X            0x03
#define A320_REG_DELTA_Y            0x04
#define A320_REG_SQUAL              0x05
#define A320_REG_SHUTTER_UPPER      0x06
#define A320_REG_SHUTTER_LOWER      0x07
#define A320_REG_MAXIMUM_PIXEL      0x08
#define A320_REG_PIXEL_SUM          0x09
#define A320_REG_MINIMUM_PIXEL      0x0A
#define A320_REG_PIXEL_GRAB         0x0B
#define A320_REG_CRC0               0x0C
#define A320_REG_CRC1               0x0D
#define A320_REG_CRC2               0x0E
#define A320_REG_CRC3               0x0F
#define A320_REG_SELF_TEST          0x10
#define A320_REG_CONFIGRATIONS      0x11
#define A320_REG_LED_CONTROL        0x1A
#define A320_REG_IO_MODE            0x1C
#define A320_REG_MOTION_CONTROL     0x1D
#define A320_REG_OBSERVATION        0x2E
#define A320_REG_SOFT_RESET         0x3A
#define A320_REG_SHUTTER_MAX_HI     0x3B
#define A320_REG_SHUTTER_MAX_LO     0x3C
#define A320_REG_INVERSE_REVION_ID  0x3E
#define A320_REG_INVERSE_PRODUCT_ID 0x3F
#define A320_REG_OFN_ENGINE         0x60
#define A320_REG_OFN_RESOLUTION     0x62
#define A320_REG_OFN_SPEED_CONTROL  0x63
#define A320_REG_OFN_SPEED_ST12     0x64
#define A320_REG_OFN_SPEED_ST21     0x65
#define A320_REG_OFN_SPEED_ST23     0x66
#define A320_REG_OFN_SPEED_ST32     0x67
#define A320_REG_OFN_SPEED_ST34     0x68
#define A320_REG_OFN_SPEED_ST43     0x69
#define A320_REG_OFN_SPEED_ST45     0x6A
#define A320_REG_OFN_SPEED_ST54     0x6B
#define A320_REG_OFN_AD_CTRL        0x6D
#define A320_REG_OFN_AD_ATH_HIGH    0x6E
#define A320_REG_OFN_AD_DTH_HIGH    0x6F
#define A320_REG_OFN_AD_ATH_LOW     0x70
#define A320_REG_OFN_AD_DTH_LOW     0x71
#define A320_REG_OFN_QUANTIZE_CTRL  0x73
#define A320_REG_OFN_XYQ_THRESH     0x74
#define A320_REG_OFN_FPD_CTRL       0x75
#define A320_REG_OFN_ORIENTATION    0x77

#define A320_PRODUCT_ID             0x83
#define A320_REVISION_ID            0x01

/*MOTION*/
#define MOTION                      (1 << 7)
#define PIXRDY                      (1 << 6)
#define PIXFIRST                    (1 << 5)
#define OVERFLOW                    (1 << 4)
#define GPIO_STATUS                 (1 << 0)

/*LED_CONTROL*/
#define LED3                        (1 << 3)

/*IO_MODE*/
#define BURST_MODE                  (1 << 4)
#define SPI_MODE                    (1 << 2)
#define TWI_MODE                    (1 << 0)

/*MOTION_CONTROL*/
#define CONTROL                     (1 << 7)

/*OBSERVATION*/
#define MODE1                       (1 << 7)
#define MODE0                       (1 << 6)

/*OFN_ENGINE*/
#define ENGINE                      (1 << 7)
#define SPEED                       (1 << 6)
#define ASSERT_DEASSERT             (1 << 5)
#define XY_QUANTIZATION             (1 << 4)
#define FINGER                      (1 << 2)
#define XY_SCALE                    (1 << 1)

/*OFN_RESOLUTION*/
#define WAKEUP_RESOLUTION           (0x07 << 3)
#define NORMAL_RESOLUTION           (0x07 << 0)

/*OFN_SPEED_CONTROL*/
#define Y_SCALE                     (1    << 7)
#define X_SCALE                     (1    << 6)
#define SPEED_SWITCH_CHECK_INTERVAL (0x03 << 2)
#define LOW_CPI                     (1    << 1)
#define HIGH_CPI                    (1    << 0)

/*OFN_AD_CTRL*/
#define ST_HIGH                     (0x07 << 0)

/*OFN_QUANTIZE_CTRL*/
#define YQ_ON                       (1    << 7)
#define YQ_DIV                      (0x07 << 4)
#define XQ_ON                       (1    << 3)
#define XQ_DIV                      (0x03 << 0)

/*OFN_XYQ_THRESH*/
#define XYQ_M                       (1    << 2)
#define XYQ_C                       (0x03 << 0)

/*OFN_FPD_CTRL*/
#define FPD_POL                     (1    << 6)
#define FPD_TH                      (0x3f << 0)

/*OFN_ORIENTATION_CTRL*/
#define XY_SWAP                     (1    << 7)
#define Y_INV                       (1    << 6)
#define X_INV                       (1    << 5)
#define ORIENT                      (0x03 << 0)
#endif 
