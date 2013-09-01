#ifndef __KXTE9_H
#define __KXTE9_H
/******************************************************************************
 * Function Configuration
******************************************************************************/
#define KXTE9_TEST_MODE
/******************************************************************************
 * Definition
******************************************************************************/
#define KXT_TAG					"<KXTE9> "
#define KXT_DEV_NAME			"KXTE9"
#define KXT_FUN(f)				printk(KXT_TAG"%s\n", __FUNCTION__)
#define KXT_ERR(fmt, args...)	printk(KXT_TAG"%s %d : "fmt, __FUNCTION__, __LINE__, ##args)
#define KXT_LOG(fmt, args...)	printk(KXT_TAG fmt, ##args)
#define KXT_VER(fmt, args...)   ((void)0)
#define KXTE9_WR_SLAVE_ADDR	    (0x0F << 1)
#define KXTE9_AXES_NUM		    3
#define KXTE9_DATA_LEN          3
#define KXTE9_ADDR_MAX          0x5F
#define KXTE9_AXIS_X			0
#define KXTE9_AXIS_Y			1
#define KXTE9_AXIS_Z			2
/*position in the sysfs attribute array*/
#define KXTE9_ATTR_X_OFFSET	    0
#define KXTE9_ATTR_Y_OFFSET	    1
#define KXTE9_ATTR_Z_OFFSET	    2
#define KXTE9_ATTR_DATA		    3
#define KXTE9_ATTR_REGS         4
/*In MT6516, KXTE9 is connected to I2C Controller 3: SCL2, SDA2*/
#define KXTE9_I2C_ID	        2 

/*Register definition*/
#define KXTE9_REG_ST_RESP           0x0C
#define KXTE9_REG_WHO_AM_I          0x0F
#define KXTE9_REG_TILE_POS_CUR      0x10
#define KXTE9_REG_TILT_POS_PRE      0x11
#define KXTE9_REG_XOUT              0x12
#define KXTE9_REG_YOUT              0x13
#define KXTE9_REG_ZOUT              0x14
#define KXTE9_REG_INT_SRC_REG1      0x16
#define KXTE9_REG_INT_SRC_REG2      0x17
#define KXTE9_REG_STATUS_REG        0x18
#define KXTE9_REG_INT_REL           0x1A
#define KXTE9_REG_CTRL_REG1         0x1B
#define KXTE9_REG_CTRL_REG2         0x1C
#define KXTE9_REG_CTRL_REG3         0x1D
#define KXTE9_REG_INT_CTRL_REG1     0x1E
#define KXTE9_REG_INT_CTRL_REG2     0x1F
#define KXTE9_REG_TILT_TIMER        0x28
#define KXTE9_REG_WUF_TIMER         0x29
#define KXTE9_REG_B2S_TIMER         0x2A
#define KXTE9_REG_WUF_THRESH        0x5A
#define KXTE9_REG_B2S_THRESH        0x5B
#define KXTE9_REG_TILE_ANGLE        0x5C
#define KXTE9_REG_HYST_SET          0x5F

/*bit value in TILT_POS_CUR/TILT_POS_PRE*/
#define KXTE9_POS_FU            (0x01 << 0)
#define KXTE9_POS_FD            (0x01 << 1)
#define KXTE9_POS_UP            (0x01 << 2)
#define KXTE9_POS_DO            (0x01 << 3)
#define KXTE9_POS_RI            (0x01 << 4)
#define KXTE9_POS_LE            (0x01 << 5)

#define KXTE9_0G_OFFSET         32
#define KXTE9_SENSITIVITY       16
#define KXTE9_DATA_COUNT(X)     ((X) >> 2)
#define KXTE9_DATA_TO_G(X)      (1000 * ((X)-KXTE9_0G_OFFSET)/KXTE9_SENSITIVITY)

/*bit value in INT_SRC_REG1*/
#define KXTE9_INT_TPS           (0x01 << 0)
#define KXTE9_INT_WUFS          (0x01 << 1)
#define KXTE9_INT_B2SS          (0x01 << 2)

/*bit value in INT_SRC_REG2*/
#define KXTE9_INT_AFU           (0x01 << 0)
#define KXTE9_INT_AFD           (0x01 << 1)
#define KXTE9_INT_AUP           (0x01 << 2)
#define KXTE9_INT_ADO           (0x01 << 3)
#define KXTE9_INT_ARI           (0x01 << 4)
#define KXTE9_INT_ALE           (0x01 << 5)

/*bit value in STATUS_REG*/
#define KXTE9_STATUS_SODRA      (0x01 << 2)
#define KXTE9_STATUS_SODRB      (0x01 << 3)
#define KXTE9_STATUS_INT        (0x01 << 4)
#define KXTE9_STATUS_DOR        (0x01 << 5)

/*bit value in CTRL_REG1*/
#define KXTE9_CTRL_TPE          (0x01 << 0)
#define KXTE9_CTRL_WUFE         (0x01 << 1)
#define KXTE9_CTRL_B2SE         (0x01 << 2)
#define KXTE9_CTRL_ODRB         (0x01 << 3)
#define KXTE9_CTRL_ODRA         (0x01 << 4)
#define KXTE9_CTRL_PC1          (0x01 << 7)

/*bit value in CTRL_REG2*/
#define KXTE9_CTRL_FUM          (0x01 << 0)
#define KXTE9_CTRL_FDM          (0x01 << 1)
#define KXTE9_CTRL_UPM          (0x01 << 2)
#define KXTE9_CTRL_DOM          (0x01 << 3)
#define KXTE9_CTRL_RIM          (0x01 << 4)
#define KXTE9_CTRL_LEM          (0x01 << 5)

/*bit value in CTRL_REG3*/
#define KXTE9_CTRL_OWUFB        (0x01 << 0)
#define KXTE9_CTRL_OWUFA        (0x01 << 1)
#define KXTE9_CTRL_OB2SB        (0x01 << 2)
#define KXTE9_CTRL_OB2SA        (0x01 << 3)
#define KXTE9_CTRL_STC          (0x01 << 4)
#define KXTE9_CTRL_SRST         (0x01 << 7)

#define KXTE9_DATA_RATE_1HZ     (0x00)
#define KXTE9_DATA_RATE_3HZ     (0x01)
#define KXTE9_DATA_RATE_10HZ    (0x02)
#define KXTE9_DATA_RATE_40HZ    (0x03)

/*bit value in INT_CTRL_REG1*/
#define KXTE9_INT_CTRL_IEL      (0x01 << 2)
#define KXTE9_INT_CTRL_IEA      (0x01 << 3)
#define KXTE9_INT_CTRL_IEN      (0x01 << 4)
 
/*bit value in INT_CTRL_REG2*/
#define KXTE9_INT_CTRL_ZBW      (0x01 << 5)
#define KXTE9_INT_CTRL_YBW      (0x01 << 6)
#define KXTE9_INT_CTRL_XBW      (0x01 << 7)

/*default value*/
#define KXTE9_ODR_DEFAULT       0x18 /*40Hz*/
#define KXTE9_ODR_ACTIVE        0x03 /*40Hz*/
#define KXTE9_ODR_INACTIVE      0x0C /*40Hz*/
#define KXTE9_TILE_POS_MASK     0x3F /*first 6-bits in TILT_POS_CUR*/
#define KXTE9_BUFSIZE				256


#endif /*__KXTE9_H*/
