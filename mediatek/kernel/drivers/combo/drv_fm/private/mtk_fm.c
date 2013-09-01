#include <linux/sched.h>
//#include <asm-generic/current.h>
#include <linux/string.h>

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h> // udelay()

#include "stp_exp.h"
#include "wmt_exp.h"
#include "mt6620_fm.h"
#include "mtk_fm.h"


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

int Delayms(uint32_t data);
int MT6620_HL_Side_Adj(uint16_t freq, int *hl);
int MT6620_ADPLL_Freq_Avoid(uint16_t freq, int *freqavoid);
int MT6620_MCU_Freq_Avoid(uint16_t freq, int *freqavoid);
int MT6620_ADPLL_Power_OnOff(int onoff, int ADPLL_clk);
int MT6620_TX_PWR_CTRL(uint16_t freq, int *ctr);
int MT6620_RTC_Drift_CTRL(uint16_t freq, int *ctr);

struct fm_op op;
struct fm_op_cb *op_cb = &op.op_tbl;


/*
 *  delay ms
 */
int Delayms(uint32_t data)
{
    int ret = 0;
    
    msleep(data);

    return ret;
}


/*
 * GetChannelSpace - get the spcace of gived channel
 * @freq - value in 760~1080 or 7600~10800
 *
 * Return 0, if 760~1080; return 1, if 7600 ~ 10800, else err code < 0
 */
static int GetChannelSpace(int freq)
{
    if ((freq >= 760) && (freq <= 1080)) {
        return 0;
    } else if ((freq >= 7600) && (freq <= 10800)) {
        return 1;
    } else {
        return -1;
    }
}


/***********************************************************************
*  Hi-Lo Side Injection
*
***********************************************************************/
int MT6620_HL_Side_Adj(uint16_t freq, int *hl)
{
    int ret = 0;
    int isHiSide= 0;
    int tblsize = 0;
    int indx = 0;
    uint16_t tmp;
    static uint16_t Hi_Channels[] = {7950, 8070, 8210, 10640};

    if (0 == GetChannelSpace(freq)) {
            freq *= 10;
    }

    FM_LOG_DBG(D_INIT,"+%s, [freq=%d]\n", __func__, (int)freq);
    FM_COM_ASSERT(op_cb->read);
    FM_COM_ASSERT(op_cb->write);
    
    *hl = 0;
    
    if(sizeof(Hi_Channels) == 0)
        goto out;
    
    tblsize = sizeof(Hi_Channels)/sizeof(Hi_Channels[0]);
    for(indx = 0; indx < tblsize; indx++){
        if(Hi_Channels[indx] == freq)
        {
            isHiSide = 1;
            *hl = 1;
            //goto set_HL;
            break;
        }     
    }
    
    if(isHiSide){
  	    //Set high-side injection (AFC)
  	    if((ret = op_cb->read(0x0F, &tmp)))
            goto out;
  	    if((ret = op_cb->write(0x0F, tmp |0x0400)))
            goto out;
  	    if((ret = op_cb->write(FM_MAIN_PGSEL, 0)))
            goto out;
  	    //Set high-side injection (DFE)
  	    if((ret = op_cb->read(0xCB, &tmp)))
            goto out;
  	    if((ret = op_cb->write(0xCB, tmp | 0x01)))
            goto out;
  	    //op_cb->write(0xCB, dataRead&0xFFFE);
    }else{
        //Set low-side injection (AFC)
        if((ret = op_cb->read(0x0F, &tmp)))
            goto out;
  	    if((ret = op_cb->write(0x0F, tmp&0xFBFF)))
            goto out;
  	    if((ret = op_cb->write(FM_MAIN_PGSEL, 0)))
            goto out;
  	    //Set low-side injection (DFE)
  	    if((ret = op_cb->read(0xCB, &tmp)))
            goto out;
  	    //op_cb->write(0xCB, dataRead | 0x01);
        if((ret = op_cb->write(0xCB, tmp&0xFFFE)))
            goto out;
    }
 out:   
    FM_LOG_NTC(D_INIT,"-%s, [isHiSide=%d][ret=%d]\n", __func__, (int)isHiSide, ret);
    return ret;
}

/***********************************************************************
*  ADPLL Power On or Off
*
***********************************************************************/
int MT6620_ADPLL_Power_OnOff(int onoff, int ADPLL_clk)
{
    int ret = 0;

    FM_COM_ASSERT(op_cb->read);
    FM_COM_ASSERT(op_cb->write);
    FM_COM_ASSERT(op_cb->setbits);
    
    switch(onoff){
        case FM_ADPLL_ON:
            if((ret = op_cb->write(0x25, 0x040F)))
                goto out;
            //Remove the Reset_N
            if((ret = op_cb->write(0x20, 0x2720)))
                goto out;
            // change DLF loop gain  
            // Set FMCR_DLF_GAIN_A = "9"
            // Set FMCR_DLF_GAIN_B = "9"
            if((ret = op_cb->write(0x22, 0x9980)))
                goto out;
            //Configure initial I_CODE for calibration
            if((ret = op_cb->write(0x25, 0x080F)))
                goto out;
            //Enable ADPLL DCO
            //Set FMCR_DCO_ EN = "1
            if(ADPLL_clk == FM_ADPLL_16M){
                if((ret = op_cb->write(0x1E, 0x0A63)))
                    goto out;
                // wait 5ms 
                Delayms(5);
                if((ret = op_cb->write(0x1E, 0x0A65)))
                    goto out;
                // wait 5ms 
                Delayms(5);
                if((ret = op_cb->write(0x1E, 0x0A71)))
                    goto out;
            }else if(ADPLL_clk == FM_ADPLL_15M){
            if((ret = op_cb->write(0x1E, 0x0863)))
                goto out;
            // wait 5ms 
            Delayms(5);
            if((ret = op_cb->write(0x1E, 0x0865)))
                goto out;
            // wait 5ms 
            Delayms(5);
            if((ret = op_cb->write(0x1E, 0x0871)))
                goto out;
            }else{
                ret = -ERR_INVALID_PARA;
                goto out;
            }
            // wait 100ms 
            Delayms(100);
            if((ret = op_cb->write(0x2A, 0x1026)))
                goto out;
            break;
            
        //ADPLL Power Off Sequence
        case FM_ADPLL_OFF:
            // Set rgfrf_top_ck = "0"
            if((ret = op_cb->setbits(0x2A, 0, MASK(12))))//set 2A D12=0
                goto out;
            // Set FMCR_OPEN_LOOP_EN = "0"
            // Set FMCR_PLL_EN = "0"
            // Set FMCR_DCO_EN = "0"
            if((ret = op_cb->setbits(0x1E, 0, MASK(7)&MASK(4)&MASK(0))))//set 1E D7 D4 D0=0
                goto out;
            // Set rgfrf_adpll_reset_n = "0"
            if((ret = op_cb->setbits(0x20, 0, MASK(13))))//set 20 D13=0
                goto out;
            // Set rgfrf_adpll_reset_n = "1"
            if((ret = op_cb->setbits(0x20, BITn(13), MASK(13))))//set 20 D13=1
                goto out;
            break;
        default:
            break;
    }
out:
    return ret;
}

/***********************************************************************
*  Frequency Avoidance
*
***********************************************************************/
int MT6620_ADPLL_Freq_Avoid(uint16_t freq, int *freqavoid)
{
    int ret = 0;
    int ADPLL_clk = FM_ADPLL_15M;
    uint16_t dataRead = 0;
    uint16_t indx = 0;
    static uint16_t Avoid_Channels[] ={ 
        7670, 7680, 7690, 7700, 8060, 8070, 8080, 8440, 8450, 8460, 8720, 8830, 8840, 9200,
        9210, 9220, 9230, 9360, 9490, 9600, 9610, 9980, 9990, 10000, 10130, 10360, 10370, 10380, 10740,
        10750, 10760, 10770
    };

    if (0 == GetChannelSpace(freq)) {
        freq *= 10;
    }

    FM_LOG_DBG(D_MAIN,"+%s, [freq=%d]\n", __func__, (int)freq);
    FM_COM_ASSERT(op_cb->read);
    FM_COM_ASSERT(op_cb->write);

    *freqavoid = 0;
    
    dataRead = sizeof(Avoid_Channels)/sizeof(Avoid_Channels[0]);
    indx = 0;
    while((indx < dataRead) && (ADPLL_clk != FM_ADPLL_16M)){
        if(Avoid_Channels[indx] == freq){
            ADPLL_clk = FM_ADPLL_16M;
            *freqavoid = 1;
        }
        indx++;
    }
    //isADPLL_16M = 1;
    if((ret = op_cb->read(0x1E, &dataRead)))
        goto out;
    if(((dataRead&BITn(9))&&(ADPLL_clk == FM_ADPLL_16M))||(!(dataRead&BITn(9))&&(ADPLL_clk == FM_ADPLL_15M)))//1EH, D9
        goto out; //we need not do freq avoid at these caes

    if(ADPLL_clk == FM_ADPLL_16M){       
        //Set rgf_f16mode_en = X	
      	if((ret = op_cb->setbits(0x61, BITn(0), MASK(0))))//set 61H D0=1, 16.384MHZ
                goto out;
    }else if(ADPLL_clk == FM_ADPLL_15M){
        //Set rgf_f16mode_en = X  		
      	if((ret = op_cb->setbits(0x61, 0, MASK(0))))//set 61H D0=0, 15.36MHZ
                goto out;
    }else{
        ret = -ERR_INVALID_PARA;
        goto out;
    } 
    
    // Disable ADPLL
    ret = MT6620_ADPLL_Power_OnOff(FM_ADPLL_OFF, ADPLL_clk);
    if(ret){
        FM_LOG_NTC(D_MAIN,"%s, ADPLL OFF failed, [ret=%d]n", __func__, ret);
        goto out;
    }
    
    //Set FMCR_DCO_CK_SEL = ? (default = 0, 15.36)
    if(ADPLL_clk == FM_ADPLL_16M){		
        if((ret = op_cb->setbits(0x1E, BITn(9), MASK(9))))//set 1EH D9=1, 16.384MHZ
                goto out;
    }else if(ADPLL_clk == FM_ADPLL_15M){
        if((ret = op_cb->setbits(0x1E, 0, MASK(9))))//set 1EH D9=0, 15.36MHZ
                goto out;
    }else{
        ret = -ERR_INVALID_PARA;
        goto out;
    }
    
    // Ensable ADPLL
    ret = MT6620_ADPLL_Power_OnOff(FM_ADPLL_ON, ADPLL_clk);
    if(ret){
        FM_LOG_NTC(D_MAIN,"%s, ADPLL ON failed, [ret=%d]\n", __func__, ret);
        goto out;
    }
    //Set rgfrf_cnt_resync_b = 0
    if((ret = op_cb->setbits(0x2A, 0, MASK(1))))//set 2AH D1=0
        goto out;
    //Set rgfrf_cnt_resync_b = 1
    if((ret = op_cb->setbits(0x2A, BITn(1), MASK(1))))//set 2AH D1=1
        goto out; 
out:
    FM_LOG_NTC(D_MAIN,"-%s, [ADPLL_clk=%d][ret=%d]\n", __func__, (int)ADPLL_clk, ret);
    return ret;
}

/***********************************************************************
*  Frequency Avoidance
*
***********************************************************************/
int MT6620_MCU_Freq_Avoid(uint16_t freq, int *freqavoid)
{
    int ret = 0;
    int mcuDsense = FM_MCU_DESENSE_DISABLE;
    uint16_t len = 0;
    uint16_t indx = 0;
    static uint16_t FreqList[] ={7800, 7940, 8320, 9260, 9600, 10400};

    if (0 == GetChannelSpace(freq)) {
        freq *= 10;
    }

    FM_LOG_DBG(D_MAIN,"+%s, [freq=%d]\n", __func__, (int)freq);
    FM_COM_ASSERT(op_cb->read);
    FM_COM_ASSERT(op_cb->write);

    *freqavoid = 0;
    
    len = sizeof(FreqList)/sizeof(FreqList[0]);
    indx = 0;
    while((indx < len) && (mcuDsense != FM_MCU_DESENSE_ENABLE)){
        if(FreqList[indx] == freq){
            mcuDsense = FM_MCU_DESENSE_ENABLE;
            *freqavoid = 1;
        }
        indx++;
    }

	if(mcuDsense == FM_MCU_DESENSE_DISABLE){
		if(mtk_wcn_wmt_dsns_ctrl(WMTDSNS_FM_DISABLE)){
			ret = 0;
		}else{
			ret = -ERR_STP;
		}
	}else if(mcuDsense == FM_MCU_DESENSE_ENABLE){
		if(mtk_wcn_wmt_dsns_ctrl(WMTDSNS_FM_ENABLE)){
			ret = 0;
		}else{
			ret = -ERR_STP;
		}
	}else{
		FM_LOG_ERR(D_MAIN,"para error!\n");
		ret = -ERR_INVALID_PARA;
	}
    
    FM_LOG_NTC(D_MAIN,"-%s, [mcuDsense=%d][ret=%d]\n", __func__, (int)mcuDsense, ret);
    return ret;
}

/***********************************************************************
*  TX PWR CTRL
*
***********************************************************************/
int MT6620_TX_PWR_CTRL(uint16_t freq, int *ctr)
{
    #define MT6620_TX_PWR_LEV_MAX 120
    #define MT6620_TX_PWR_LEV_MIN 85
    int ret = 0;
    int tmp = 0;
    uint16_t reg = 0;
    uint16_t coarse;
    uint16_t fine;
    
    FM_LOG_DBG(D_MAIN,"+%s, [freq=%d]\n", __func__, (int)freq);
    FM_COM_ASSERT(op_cb->read);
    FM_COM_ASSERT(op_cb->write);
    FM_COM_ASSERT(ctr);

    if(freq < FM_TX_PWR_CTRL_FREQ_THR){
        //Power setting - 1dB, 3C(HEX)=A9E9
        *ctr -= 1;
    }else{
        //Power setting -2 dB, 3C(HEX)=A8E9
        *ctr -= 2;
    }
    
    if(*ctr > MT6620_TX_PWR_LEV_MAX){
        *ctr = MT6620_TX_PWR_LEV_MAX;
    }else if(*ctr < MT6620_TX_PWR_LEV_MIN){
        *ctr = MT6620_TX_PWR_LEV_MIN;
    }
    fine = 43017 + ((1<<((*ctr-85)%6))-1)*32;
    FM_LOG_DBG(D_MAIN,"0x3C = 0x%04x \n", fine);
    coarse = 514 + ((1<<((*ctr-85)/6))-1)*4;
    FM_LOG_DBG(D_MAIN,"0x3D = 0x%04x \n", coarse);
    
    if((ret = op_cb->write(0x3C, fine)))
            goto out;
    if((ret = op_cb->write(0x3D, coarse)))
            goto out;

    tmp = mtk_wcn_wmt_therm_ctrl(WMTTHERM_READ);
    if((ret = op_cb->read(0x9C, &reg)))
        goto out;
    reg &= 0xC0FF;
    if(tmp < FM_TX_PWR_CTRL_TMP_THR_DOWN){
        reg |= (0x1C << 8);  //9CH, D13~D8 = 1C
    }else if(tmp > FM_TX_PWR_CTRL_TMP_THR_UP){
        reg |= (0x33 << 8);  //9CH, D13~D8 ==33
    }else{
        reg |= (0x25 << 8);  //9CH, D13~D8 =25
    }
    if((ret = op_cb->write(0x9C, reg)))
        goto out;

out:
    FM_LOG_NTC(D_MAIN,"-%s, [temp=%d][ret=%d]\n", __func__, (int)tmp, ret);
    return ret;
}

/***********************************************************************
*  TX RTC PWR CTRL
*
***********************************************************************/
int MT6620_RTC_Drift_CTRL(uint16_t freq, int *ctr)
{
    int ret = 0;
    uint16_t reg = 0;
    int chanel_resolution = 1;
    int16_t compensation_int16 = 0;
    int tmp = 0;
    int drift = *ctr;
    
    FM_LOG_DBG(D_MAIN,"+%s, [freq=%d]\n", __func__, (int)freq);
    FM_COM_ASSERT(op_cb->read);
    FM_COM_ASSERT(op_cb->write);
    FM_COM_ASSERT(op_cb->setbits);
    FM_COM_ASSERT(ctr);

    //turn off VCO tracking
    if((ret = op_cb->setbits(0x48, 0, MASK(15))))//set 48 D15=0
        goto out;
    
    //get channel resolution
    if((ret = op_cb->read(0x46, &reg)))
        goto out;
    reg &= 0xC000;
    switch(reg >> 14){
        case 0:
            chanel_resolution = 1024;
            break;
        case 1:
            chanel_resolution = 512;
            break;
        case 2:
            chanel_resolution = 256;
            break;
        case 3:
            chanel_resolution = 128;
            break;
        default:
            FM_LOG_ERR(D_MAIN,"chanel_resolution error[%d]\n", (int)(reg >> 14));
            break;
    }

    //caculate and applye compensation
    if (0 == GetChannelSpace(freq)) {
        freq *= 10;
    }
    FM_LOG_DBG(D_MAIN,"[resolution=%d][freq=%d][drift=%d]\n", chanel_resolution, (int)(freq/100), (*ctr));
    tmp = (2*drift*(freq/100))/chanel_resolution;
    compensation_int16 = (int16_t)tmp;
    if(compensation_int16 >= 511){
        compensation_int16 = 511;
    }else if(compensation_int16 <= -512){
        compensation_int16 = -512;
    }
    if((ret = op_cb->read(0x47, &reg)))
        goto out;
    reg &= 0x003F;
    reg |= (compensation_int16 << 6);
    if((ret = op_cb->write(0x47, reg)))
        goto out;

    /*
    //turn on VCO tracking
    if((ret = op_cb->setbits(0x48, BITn(15), MASK(15))))//set 48 D15=1
        goto out;
        */
out:
    FM_LOG_NTC(D_MAIN,"-%s, [compensation=%d][ret=%d]\n", __func__, (int)(compensation_int16), ret);
    return ret;
}

/***********************************************************************
*  TX desense with WIFI/BT
*
***********************************************************************/
int MT6620_TX_DESENSE(uint16_t freq, int *ctr)
{
    int ret = 0;
    uint16_t dataRead = 0;
    uint16_t tmp = 0;
    
    FM_LOG_DBG(D_MAIN,"+%s, [freq=%d]\n", __func__, (int)freq);
    FM_COM_ASSERT(op_cb->read);
    FM_COM_ASSERT(op_cb->write);
    FM_COM_ASSERT(op_cb->setbits);

    // enable FM TX VCO tracking
    if((ret = op_cb->read(0x29, &dataRead)))//read 29 
        goto out;
    FM_LOG_NTC(D_MAIN,"Before VCO On, [0x29=0x%04x]\n", dataRead);
    if((ret = op_cb->read(0x12, &dataRead)))//read 12 
        goto out;
    FM_LOG_NTC(D_MAIN,"Before VCO On, [0x12=0x%04x]\n", dataRead);
    
    if((ret = op_cb->setbits(0x12, 0, MASK(15))))//set 12 D15=0
        goto out;
    if((ret = op_cb->setbits(0x41, BITn(0), MASK(0))))//set 41 D0=1
        goto out;
    if((ret = op_cb->setbits(0x48, BITn(15), MASK(15))))//set 48 D15=1
        goto out;

    // wait 100ms (VCO tracking 100ms)
    if(*ctr > FM_TX_TRACKING_TIME_MAX){
        *ctr = FM_TX_TRACKING_TIME_MAX;
    }
    Delayms(*ctr);

    // disable FM TX VCO tracking
    if((ret = op_cb->setbits(0x28, BITn(2), MASK(2))))//set 28 D2=1
        goto out;
    if((ret = op_cb->read(0x29, &dataRead)))//read 29 D11~D0
        goto out;
    FM_LOG_NTC(D_MAIN,"Before VCO Off, [0x29=0x%04x]\n", dataRead);
    tmp = dataRead&0x0FFF; // Read 0x29 D11~D0
    if((ret = op_cb->read(0x12, &dataRead)))//read 12 
        goto out;
    //Set 0x12 D15 to 1, D11:D0 to read(0x29 D11~D0)
    dataRead &= 0xF000; 
    dataRead |= tmp;
    dataRead |= 1<<15;
    if((ret = op_cb->write(0x12, dataRead)))
        goto out;
    FM_LOG_NTC(D_MAIN,"Before VCO Off, [0x12=0x%04x]\n", dataRead);
    if((ret = op_cb->setbits(0x48, 0, MASK(15))))//set 48 D15=0
        goto out;
    if((ret = op_cb->setbits(0x41, 0, MASK(0))))//set 41 D0=0
        goto out;
    
out:
    FM_LOG_DBG(D_MAIN,"-%s, [freq=%d][delay=%dms][ret=%d]\n", __func__, (int)freq, *ctr, ret);
    return ret;
}
static const fm_u16 DesenseChMap[] = {
    0x0000, 0x0000, 0x0000, 0x0000, /* 7675~7600, 7755~7680, 7835~7760, 7915~7840 */
    0x0000, 0x0000, 0x0000, 0x0000, /* 7995~7920, 8075~8000, 8155~8080, 8235~8160 */
    0x0000, 0x0000, 0x0000, 0x0000, /* 8315~8240, 8395~8320, 8475~8400, 8555~8480 */
    0x0000, 0x0000, 0x0000, 0x0400, /* 8635~8560, 8715~8640, 8795~8720, 8875~8800 */
    0x0000, 0x0000, 0x0000, 0x0000, /* 8955~8880, 9035~8960, 9115~9040, 9195~9120 */
    0x00FC, 0x0000, 0x0000, 0x1C00, /* 9275~9200, 9355~9280, 9435~9360, 9515~9440 */
    0x0000, 0x0001, 0x0000, 0x0000, /* 9595~9520, 9675~9600, 9755~9680, 9835~9760 */
    0x0000, 0x7800, 0x0000, 0x0000, /* 9915~9840, 9995~9920, 10075~10000, 10155~10080 */
    0x0000, 0x0000, 0x0000, 0x0000, /* 10235~10160, 10315~10240, 10395~10320, 10475~10400 */
    0x0000, 0x0000, 0x0000, 0x0000, /* 10555~10480, 10635~10560, 10715~10640, 10795~10720 */
    0x0000                          /* 10875~10800 */
};
// return value: 0, not a de-sense channel; 1, this is a de-sense channel; else error no
static int mt6620_is_dese_chan(fm_u16 freq)
{
	fm_u8 bDesenseCh = 0;

    //caculate and applye compensation
    if (0 == GetChannelSpace(freq))
    {
        freq *= 10;
    }
	FM_LOG_NTC(D_G0,"%s, freq=%d\n",__func__,freq);
    
	bDesenseCh = ((0x0001 << (((freq - 7600)%80)/5)) & DesenseChMap[((freq - 7600)/80)])>>(((freq - 7600)%80)/5); 
	FM_LOG_NTC(D_G0,"%freq[%d] desense=[%d]\n",freq,bDesenseCh);

    return bDesenseCh;
}

int init(struct fm_op *op)
{
    int ret = 0;
    struct fm_priv priv;
	//Basic functions.
	FM_COM_ASSERT(op);
    
	op->op_tbl.read = NULL;
    op->op_tbl.write = NULL;
    op->op_tbl.setbits = NULL;
    op->state = UNINITED;
    op->data = NULL;

    priv.priv_tbl.hl_side = MT6620_HL_Side_Adj;
    priv.priv_tbl.adpll_freq_avoid = MT6620_ADPLL_Freq_Avoid;
	priv.priv_tbl.mcu_freq_avoid = MT6620_MCU_Freq_Avoid;
    priv.priv_tbl.tx_pwr_ctrl = MT6620_TX_PWR_CTRL;
    priv.priv_tbl.rtc_drift_ctrl = MT6620_RTC_Drift_CTRL;
    priv.priv_tbl.tx_desense_wifi = MT6620_TX_DESENSE;
    priv.priv_tbl.is_dese_chan = mt6620_is_dese_chan;
    
    ret = fm_priv_register(&priv, op);
    if(ret){
        FM_LOG_ERR(D_INIT,"%s, init failed\n", __func__);
    }
    FM_LOG_NTC(D_INIT,"%s, FM init ok\n", __func__);
    
	return ret;
}

int uninit(struct fm_op *op)
{
    int ret = 0;
    struct fm_priv priv;
	//Basic functions.
	FM_COM_ASSERT(op);
    
	op->op_tbl.read = NULL;
    op->op_tbl.write = NULL;
    op->op_tbl.setbits = NULL;
    op->state = UNINITED;
    op->data = NULL;

    priv.priv_tbl.hl_side = NULL;
    priv.priv_tbl.adpll_freq_avoid = NULL;
    priv.priv_tbl.mcu_freq_avoid = NULL;
	priv.priv_tbl.tx_pwr_ctrl = NULL;
    priv.priv_tbl.rtc_drift_ctrl = NULL;
    priv.priv_tbl.tx_desense_wifi = NULL;
	
    ret = fm_priv_unregister(&priv, op);
    if(ret){
        FM_LOG_ERR(D_INIT,"%s, uninit failed\n", __func__);
    }
    FM_LOG_NTC(D_INIT,"%s, FM uninit ok\n", __func__);
    
	return ret;
}

static int __init mtk_fm_probe(void)
{
    int ret = 0;

    FM_LOG_NTC(D_INIT,"%s, FM probe ...\n", __func__);
    ret = init(&op);
    if(ret){
        FM_LOG_ALT(D_INIT,"%s, FM init failed\n", __func__);
    }
    FM_LOG_NTC(D_INIT,"%s, FM probe ok\n", __func__);
    return ret; 
}

static void __exit mtk_fm_remove(void)
{
    int ret = 0;
    ret = uninit(&op);
    if(ret){
        FM_LOG_ALT(D_INIT,"%s, FM uninit failed\n", __func__);
    }
    FM_LOG_NTC(D_INIT,"%s, FM remove ok\n", __func__);
    return;
}

module_init(mtk_fm_probe);
module_exit(mtk_fm_remove);
MODULE_LICENSE("Proprietary. Send bug reports to hongcheng.xia@MediaTek.com");
//MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MediaTek FM Driver Private Part, Need be loaded after FM driver");
MODULE_AUTHOR("Hongcheng <hongcheng.xia@MediaTek.com>");

