
 

 
 
// use accdet + EINT solution
#define ACCDET_EINT
// support multi_key feature
#define ACCDET_MULTI_KEY_FEATURE
// after 5s disable accdet
#define ACCDET_LOW_POWER

//<2012/2/25-22144-jessicatseng, [Pelican] ACCDET work at 1.9V MIC mode
//#define ACCDET_28V_MODE
//>2012/2/25-22144-jessicatseng
//#define ACCDET_PIN_RECOGNIZATION

#define ACCDET_SHORT_PLUGOUT_DEBOUNCE
#define ACCDET_SHORT_PLUGOUT_DEBOUNCE_CN 20

struct headset_mode_settings{
    int pwm_width;	//pwm frequence
    int pwm_thresh;	//pwm duty 
    int fall_delay;	//falling stable time
    int rise_delay;	//rising stable time
    int debounce0;	//hook switch or double check debounce
    int debounce1;	//mic bias debounce
    int debounce3;	//plug out debounce
};

//key press customization: long press time
int long_press_time = 2000;

#ifdef  ACCDET_MULTI_KEY_FEATURE
struct headset_mode_settings cust_headset_settings = {
	0x900, 0x200, 1, 0x1f0, 0x800, 0x800, 0x20
};
#else
//headset mode register settings(for MT6575)
struct headset_mode_settings cust_headset_settings = {
	0x1900, 0x140, 1, 0x12c, 0x3000, 0x3000, 0x400
};
#endif
