

/*

+// new animation parameters example:WVGA (480*800)
// A , start point of first number rectangle
// B , left_top point of battery_capacity fill_in rectangle
// c , left_bottom point of battery_capacity fill_in rectangle

// battery capacity rectangle
#define CAPACITY_LEFT                (172) // CAPACITY_LEFT = B.x = 172
#define CAPACITY_TOP                 (330) // CAPACITY_TOP = B.y = 330
#define CAPACITY_RIGHT               (307) // CAPACITY_RIGHT = B.x + fill_line.w = 172 + 135
#define CAPACITY_BOTTOM              (546) // CAPACITY_BOTTOM  = C.y = 546

// first number rectangle
#define NUMBER_LEFT                  (178) // NUMBER_LEFT = A.x
#define NUMBER_TOP                   (190) // NUMBER_TOP  = A.y
#define NUMBER_RIGHT                 (216) // NUMBER_RIGHT = A.x + num.w = 178 + 38
#define NUMBER_BOTTOM                (244) // NUMBER_BOTTOM = A.y + num.h = 190 + 54

// %  rectangle
#define PERCENT_LEFT                 (254) // PERCENT_LEFT = A.x + 2*num.w = 178 + 2*38
#define PERCENT_TOP                  (190) // PERCENT_TOP  = A.y
#define PERCENT_RIGHT                (302) // PERCENT_LEFT = A.x + 2*num.w +(%).w 
#define PERCENT_BOTTOM               (244) // PERCENT_BOTTOM = A.y + (%).h = 190 + 54

// top animation part
#define TOP_ANIMATION_LEFT           (172) // TOP_ANIMATION_LEFT = B.x
#define TOP_ANIMATION_TOP            (100) // 100 
#define TOP_ANIMATION_RIGHT          (307) // TOP_ANIMATION_LEFT = B.x + fill_line.w = 172 + 135
#define TOP_ANIMATION_BOTTOM         (124) // TOP_ANIMATION_BOTTOM = TOP_ANIMATION_TOP + fill_line.h = 100 + 24

*/


#ifndef __CUST_DISPLAY_H__
#define __CUST_DISPLAY_H__

// color
#define BAR_OCCUPIED_COLOR  (0x07E0)    // Green
#define BAR_EMPTY_COLOR     (0xFFFF)    // White
#define BAR_BG_COLOR        (0x0000)    // Black




#if defined(FHD) || defined(CU_FHD) || defined(CMCC_FHD)
	// fhd 1080*1920
	
	// battery capacity rectangle
	#define CAPACITY_LEFT                (387) // battery capacity center
	#define CAPACITY_TOP                 (802)
	#define CAPACITY_RIGHT               (691)
	#define CAPACITY_BOTTOM              (1292)

	// first number rectangle
	#define NUMBER_LEFT                  (351+84) // number
	#define NUMBER_TOP                   (479)
	#define NUMBER_RIGHT                 (435+84)
	#define NUMBER_BOTTOM                (600)

	// %  rectangle
	#define PERCENT_LEFT                 (519+84) // percent number_left + 2*number_width
	#define PERCENT_TOP                  (479)
	#define PERCENT_RIGHT                (627+84)
	#define PERCENT_BOTTOM               (600)

	// top animation part
	#define TOP_ANIMATION_LEFT           (387) // top animation
	#define TOP_ANIMATION_TOP            (100)
	#define TOP_ANIMATION_RIGHT          (691)
	#define TOP_ANIMATION_BOTTOM         (152)

	// for old animation
	#define BAR_LEFT            (470)
	#define BAR_TOP             (356)
	#define BAR_RIGHT           (610)
	#define BAR_BOTTOM          (678)

#elif defined(HD720) || defined(CU_HD720) || defined(CMCC_HD720)
	// hd720 720*1280

	// battery capacity rectangle
	#define CAPACITY_LEFT                (278) // battery capacity center
	#define CAPACITY_TOP                 (556)
	#define CAPACITY_RIGHT               (441)
	#define CAPACITY_BOTTOM              (817)

	// first number rectangle
	#define NUMBER_LEFT                  (290) // number
	#define NUMBER_TOP                   (386)
	#define NUMBER_RIGHT                 (335)
	#define NUMBER_BOTTOM                (450)

	// %  rectangle
	#define PERCENT_LEFT                 (380) // percent number_left + 2*number_width
	#define PERCENT_TOP                  (386)
	#define PERCENT_RIGHT                (437)
	#define PERCENT_BOTTOM               (450)

	// top animation part
	#define TOP_ANIMATION_LEFT           (278) // top animation
	#define TOP_ANIMATION_TOP            (100)
	#define TOP_ANIMATION_RIGHT          (441)
	#define TOP_ANIMATION_BOTTOM         (129)

	// for old animation
	#define BAR_LEFT            (313)
	#define BAR_TOP             (238)
	#define BAR_RIGHT           (406)
	#define BAR_BOTTOM          (453)

#elif defined(FWVGA) || defined(CU_FWVGA) || defined(CMCC_FWVGA)
	// fwvga 480*854

	// battery capacity rectangle
	#define CAPACITY_LEFT                (172) // battery capacity center
	#define CAPACITY_TOP                 (357)
	#define CAPACITY_RIGHT               (307)
	#define CAPACITY_BOTTOM              (573)

	// first number rectangle
	#define NUMBER_LEFT                  (172) // number
	#define NUMBER_TOP                   (213)
	#define NUMBER_RIGHT                 (210)
	#define NUMBER_BOTTOM                (267)

	// %  rectangle
	#define PERCENT_LEFT                 (248) // percent number_left + 2*number_width
	#define PERCENT_TOP                  (213)
	#define PERCENT_RIGHT                (296)
	#define PERCENT_BOTTOM               (267)

	// top animation part
	#define TOP_ANIMATION_LEFT           (172) // top animation
	#define TOP_ANIMATION_TOP            (100)
	#define TOP_ANIMATION_RIGHT          (307)
	#define TOP_ANIMATION_BOTTOM         (124)

	// for old animation
	#define BAR_LEFT            (184)
	#define BAR_TOP             (227)
	#define BAR_RIGHT           (294)
	#define BAR_BOTTOM          (437)

#elif defined(QHD) || defined(CU_QHD) || defined(CMCC_QHD)
	// qhd 540*960

	// battery capacity rectangle
	#define CAPACITY_LEFT                (202) // battery capacity center
	#define CAPACITY_TOP                 (410)
	#define CAPACITY_RIGHT               (337)
	#define CAPACITY_BOTTOM              (626)

	// first number rectangle
	#define NUMBER_LEFT                  (202) // number
	#define NUMBER_TOP                   (266)
	#define NUMBER_RIGHT                 (240)
	#define NUMBER_BOTTOM                (320)

	// %  rectangle
	#define PERCENT_LEFT                 (278) // percent number_left + 2*number_width
	#define PERCENT_TOP                  (266)
	#define PERCENT_RIGHT                (326)
	#define PERCENT_BOTTOM               (320)

	// top animation part
	#define TOP_ANIMATION_LEFT           (202) // top animation
	#define TOP_ANIMATION_TOP            (100)
	#define TOP_ANIMATION_RIGHT          (337)
	#define TOP_ANIMATION_BOTTOM         (124)

	// for old animation
	#define BAR_LEFT            (235)
	#define BAR_TOP             (179)
	#define BAR_RIGHT           (305)
	#define BAR_BOTTOM          (340)

#elif defined(WVGA) || defined(CU_WVGA) || defined(CMCC_WVGA)
	// default wvga 480*800

	// battery capacity rectangle
	#define CAPACITY_LEFT                (172) // battery capacity center
	#define CAPACITY_TOP                 (330)
	#define CAPACITY_RIGHT               (307)
	#define CAPACITY_BOTTOM              (546)

	// first number rectangle
	#define NUMBER_LEFT                  (178) // number
	#define NUMBER_TOP                   (190)
	#define NUMBER_RIGHT                 (216)
	#define NUMBER_BOTTOM                (244)

	// %  rectangle
	#define PERCENT_LEFT                 (254) // percent number_left + 2*number_width
	#define PERCENT_TOP                  (190)
	#define PERCENT_RIGHT                (302)
	#define PERCENT_BOTTOM               (244)

	// top animation part
	#define TOP_ANIMATION_LEFT           (172) // top animation
	#define TOP_ANIMATION_TOP            (100)
	#define TOP_ANIMATION_RIGHT          (307)
	#define TOP_ANIMATION_BOTTOM         (124)

	// for old animation
	#define BAR_LEFT            (209)
	#define BAR_TOP             (149)
	#define BAR_RIGHT           (271)
	#define BAR_BOTTOM          (282)

#elif defined(HVGA) || defined(CU_HVGA) || defined(CMCC_HVGA)

	// hvga 320*480

	// battery capacity rectangle
	#define CAPACITY_LEFT                (109) // battery capacity center
	#define CAPACITY_TOP                 (189)
	#define CAPACITY_RIGHT               (211)
	#define CAPACITY_BOTTOM              (350)

	// first number rectangle
	#define NUMBER_LEFT                  (126) // number
	#define NUMBER_TOP                   (95)
	#define NUMBER_RIGHT                 (153)
	#define NUMBER_BOTTOM                (131)

	// %  rectangle
	#define PERCENT_LEFT                 (180) // percent number_left + 2*number_width
	#define PERCENT_TOP                  (95)
	#define PERCENT_RIGHT                (212)
	#define PERCENT_BOTTOM               (131)

	// top animation part
	#define TOP_ANIMATION_LEFT           (109) // top animation
	#define TOP_ANIMATION_TOP            (100)
	#define TOP_ANIMATION_RIGHT          (211)
	#define TOP_ANIMATION_BOTTOM         (118)

	// for old animation
	#define BAR_LEFT            (129)
	#define BAR_TOP             (128)
	#define BAR_RIGHT           (190)
	#define BAR_BOTTOM          (245)
#elif defined(QVGA) || defined(CU_QVGA) || defined(CMCC_QVGA)

	// hvga 320*480

	// battery capacity rectangle
	#define CAPACITY_LEFT                (82) // battery capacity center
	#define CAPACITY_TOP                 (124)
	#define CAPACITY_RIGHT               (158)
	#define CAPACITY_BOTTOM              (241)

	// first number rectangle
	#define NUMBER_LEFT                  (93) // number
	#define NUMBER_TOP                   (50)
	#define NUMBER_RIGHT                 (109)
	#define NUMBER_BOTTOM                (73)

	// %  rectangle
	#define PERCENT_LEFT                 (125) // percent number_left + 2*number_width
	#define PERCENT_TOP                  (50)
	#define PERCENT_RIGHT                (145)
	#define PERCENT_BOTTOM               (73)

	// top animation part
	#define TOP_ANIMATION_LEFT           (82) // top animation
	#define TOP_ANIMATION_TOP            (100)
	#define TOP_ANIMATION_RIGHT          (158)
	#define TOP_ANIMATION_BOTTOM         (113)

	// for old animation
	#define BAR_LEFT            (97)
	#define BAR_TOP             (96)
	#define BAR_RIGHT           (140)
	#define BAR_BOTTOM          (184)

#else 

	// default wvga 480*800

	// battery capacity rectangle
	#define CAPACITY_LEFT                (172) // battery capacity center
	#define CAPACITY_TOP                 (330)
	#define CAPACITY_RIGHT               (307)
	#define CAPACITY_BOTTOM              (546)

	// first number rectangle
	#define NUMBER_LEFT                  (178) // number
	#define NUMBER_TOP                   (190)
	#define NUMBER_RIGHT                 (216)
	#define NUMBER_BOTTOM                (244)

	// %  rectangle
	#define PERCENT_LEFT                 (254) // percent number_left + 2*number_width
	#define PERCENT_TOP                  (190)
	#define PERCENT_RIGHT                (302)
	#define PERCENT_BOTTOM               (244)

	// top animation part
	#define TOP_ANIMATION_LEFT           (172) // top animation
	#define TOP_ANIMATION_TOP            (100)
	#define TOP_ANIMATION_RIGHT          (307)
	#define TOP_ANIMATION_BOTTOM         (124)

	// for old animation
	#define BAR_LEFT            (209)
	#define BAR_TOP             (149)
	#define BAR_RIGHT           (271)
	#define BAR_BOTTOM          (282)

#endif

/* The option of new charging animation */
#define ANIMATION_NEW

#endif // __CUST_DISPLAY_H__
