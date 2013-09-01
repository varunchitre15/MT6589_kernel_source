#ifndef _CAMERA_SENSOR_PARA_OV7675_YUV_H
#define _CAMERA_SENSOR_PARA_OV7675_YUV_H

typedef enum
{
	ISP_DRIVING_2MA=0,
	ISP_DRIVING_4MA,
	ISP_DRIVING_6MA,
	ISP_DRIVING_8MA
} ISP_DRIVING_CURRENT_ENUM;


	/* STRUCT: SENSOR */
#define OV7675_YUV_CAMERA_SENSOR_REG_DEFAULT_VALUE \
		/* ARRAY: SENSOR.reg[11] */\
		{\
			/* STRUCT: SENSOR.reg[0] */\
			{\
				/* SENSOR.reg[0].addr */ 0xFFFFFFFF, /* SENSOR.reg[0].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[1] */\
			{\
				/* SENSOR.reg[1].addr */ 0xFFFFFFFF, /* SENSOR.reg[1].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[2] */\
			{\
				/* SENSOR.reg[2].addr */ 0xFFFFFFFF, /* SENSOR.reg[2].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[3] */\
			{\
				/* SENSOR.reg[3].addr */ 0xFFFFFFFF, /* SENSOR.reg[3].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[4] */\
			{\
				/* SENSOR.reg[4].addr */ 0xFFFFFFFF, /* SENSOR.reg[4].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[5] */\
			{\
				/* SENSOR.reg[5].addr */ 0xFFFFFFFF, /* SENSOR.reg[5].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[6] */\
			{\
				/* SENSOR.reg[6].addr */ 0xFFFFFFFF, /* SENSOR.reg[6].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[7] */\
			{\
				/* SENSOR.reg[7].addr */ 0xFFFFFFFF, /* SENSOR.reg[7].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[8] */\
			{\
				/* SENSOR.reg[8].addr */ 0xFFFFFFFF, /* SENSOR.reg[8].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[9] */\
			{\
				/* SENSOR.reg[9].addr */ 0xFFFFFFFF, /* SENSOR.reg[9].para */ 0xFFFFFFFF\
			},\
			/* STRUCT: SENSOR.reg[10] */\
			{\
				/* SENSOR.reg[10].addr */ 0xFFFFFFFF, /* SENSOR.reg[10].para */ 0xFFFFFFFF\
			}\
		}


#define OV7675_YUV_CAMERA_SENSOR_CCT_DEFAULT_VALUE {{0x30a1,0x00},{0x3000,0x1B}}


#endif//#ifndef _CAMERA_SENSOR_PARA_OV7675_YUV_H


