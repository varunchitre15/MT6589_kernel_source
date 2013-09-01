#ifndef AFA750_H
#define AFA750_H

#include <linux/ioctl.h>

extern struct acc_hw* AFA750_get_cust_acc_hw(void);


#if 0 // Johnson_Qian 2012_09_29
#define AFA750_I2C_SLAVE_ADDR   (0x3C<<1) //0x78 //0x3D<->SA0=1 ;0x3C<->SA0=0  0x01111000
#else
#define AFA750_I2C_SLAVE_ADDR   (0x3D<<1)//0x78 //0x3D<->SA0=1 ;0x3C<->SA0=0  0x01111000
#endif
/*----------------------------------------------------------------------------*/
#define I2C_DEVICEID_AFA750     0x3C

/* AFA750 Register Map */
#define THRESH_TAP    0x21    /* R/W Tap threshold */
#define TAP_MIN       0x24    /* R/W Tap min duration */
#define TAP_MAX       0x25    /* R/W Tap max duration */
#define LATENT        0x22    /* R/W Tap latency */
#define DLATENTCY     0x26    /* R/W DTap latency */
#define THRESH_ACT    0x1D    /* R/W Activity threshold */

#define AFA750_REG_THRESH_FF    0x1A    /* R/W Free-fall threshold */
#define AFA750_REG_FF_LATENCY   0x1B    /* R/W Free-fall time */
#define AFA750_REG_ACT_TAP_STATUS1  0x0D    /* R   Source of tap/double tap */
#define AFA750_REG_ACT_TAP_STATUS2  0x0E
#define AFA750_REG_DATA_RATE        0x05    /* R/W Data rate */
#define AFA750_REG_POWER_CTL    0x03    /* R/W Power saving features control :Frances*/
#define AFA750_REG_INT_ENABLE   0x09    /* R/W Interrupt enable control */
#define AFA750_REG_INT_MAP      0x0B    /* R/W Interrupt mapping control */
#define AFA750_REG_DATAX0       0x10    /* R   X-Axis Data 0 */
#define AFA750_REG_DATAX1       0x11    /* R   X-Axis Data 1 */
#define AFA750_REG_DATAY0       0x12    /* R   Y-Axis Data 0 */
#define AFA750_REG_DATAY1       0x13    /* R   Y-Axis Data 1 */
#define AFA750_REG_DATAZ0       0x14    /* R   Z-Axis Data 0 */
#define AFA750_REG_DATAZ1       0x15    /* R   Z-Axis Data 1 */
#define AFA750_REG_FIFO_CTL     0x04    /* R/W FIFO control */
#define AFA750_REG_FIFO_STATUS  0x0F    /* R   FIFO status */

#define AFA750_REG_WHO_AM_I    0X37    /* R   Device ID (0x3D) */

#define WMA    0x07     //Weighted moving average
#define NODR    0x05     //output data rate


/* INT_ENABLE/INT_MAP/INT_SOURCE Bits */
#define CNT_DATA_RDY (1 << 0)
#define FIFO_EMPTY   (1 << 1)
#define FIFO_OVER    (1 << 2)
#define FIFO_FULL    (1 << 3)
#define FF_EN        (1 << 4)
#define MOTION_EN    (1 << 5)
#define TAP_EN       (1 << 6)
#define ORN_EN       (1 << 7)

/* ACT_TAP_STATUS1 Bits */
#define FREE_FALL   (1 << 0)
#define MOTION      (1 << 1)
#define SINGLE_TAP  (1 << 2)
#define DOUBLE_TAP  (1 << 3)
#define ORIENTATION (1 << 4)

/* ACT_TAP_STATUS2 Bits */
#define CNT_RDY     (1 << 0)
#define FIFO_EMPTY  (1 << 1)
#define FIFO_OVER   (1 << 2)
#define FIFO_FULL   (1 << 3)


/* DATA_RATE Bits */
#define ODR_400     0x0
#define ODR_200     0x01
#define ODR_100     0x02
#define ODR_50      0x03
#define ODR_25      0x04
#define ODR_12p5    0x05
#define ODR_6p256   0x06
#define ODR_3p128   0x07
#define ODR_1p564   0x08
#define ODR_0p782   0x09
#define ODR_0p391   0x0A

#define RATE(x)        ((x) & 0xF)

/* POWER_CTL Bits */
#define NORMAL 0
#define LOW_PWR    1
#define PWR_DOWN (1 << 1)
#define Wakeup (1 << 2)


/*
 * Maximum value our axis may get in full res mode for the input device
 * (signed 16 bits)
 */
#define AFA_FULLRES_MAX_VAL 32767
#define AFA_FULLRES_MIN_VAL 32768


/* FIFO_CTL Bits */
#define FIFO_EN         1
#define FIFO_CLEAN      (1 << 1)
#define FIFO_BYPASS     (1 << 2)
#define FIFO_STREAM     (1 << 3)
#define FIFO_TRIGGER    AFA_FIFO_BYPASS | AFA_FIFO_STREAM
#define FIFO_INT1       (0 << 4) //INT1
#define FIFO_INT2       (1 << 4) //INT2

/* WMA value */
#define WMA_CTL_0    0
#define WMA_CTL_1    1
#define WMA_CTL_2    2
#define WMA_CTL_3    3
#define WMA_CTL_4    4
#define WMA_CTL_5    5
#define WMA_CTL_6    6
#define WMA_CTL_7    7
#define WMA_CTL_8    8
#define WMA_CTL_9    9
#define WMA_CTL_10    10
#define WMA_CTL_11    11
#define WMA_CTL_12    12
#define WMA_CTL_13    13
#define WMA_CTL_14    14
#define WMA_CTL_15    15


#define AFA750_SUCCESS                  0
#define AFA750_ERR_I2C                  -1
#define AFA750_ERR_STATUS               -3
#define AFA750_ERR_SETUP_FAILURE        -4
#define AFA750_ERR_GETGSENSORDATA       -5
#define AFA750_ERR_IDENTIFICATION       -6

#define AFA750_BUFSIZE                  256


#endif

