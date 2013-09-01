#include "tpd.h"
#include <linux/delay.h>

/* event buffer macros */
#define RING_BUF_SIZE 8 /* 2^x */
#define TPD_BUF_CP() buf.index
#define TPD_BUF_NP() ((buf.index+1)&((RING_BUF_SIZE-1)))
#define TPD_BUF_QUEUE(v1,v2,v3,v4) do { \
 buf.x[RING_BUF_SIZE]=(v1),buf.y[RING_BUF_SIZE]=(v2),\
 buf.d[RING_BUF_SIZE]=(v3),buf.p[RING_BUF_SIZE]=(v4); buf.queued=1;}while(0);
#define TPD_BUF_UNQUEUE(v1,v2,v3,v4) do { \
 v1=buf.x[RING_BUF_SIZE],v2=buf.y[RING_BUF_SIZE],\
 v3=buf.d[RING_BUF_SIZE],v4=buf.p[RING_BUF_SIZE]; buf.queued=0;}while(0);

struct touch_info {
    int x1, y1;
    int x2, y2;
    int p, count, pending;
};

static struct tpd_ringbuf {
  /* +1 : queued point */
  int x[RING_BUF_SIZE+1];        /* x coordinate              */
  int y[RING_BUF_SIZE+1];        /* y coordinate              */
  int d[RING_BUF_SIZE+1];        /* down or up                */
  int p[RING_BUF_SIZE+1];        /* preesure */
  int index;                     /* ring buffer start         */
  int count;                     /* effective series length   */
  int last;                      /* last effective index      */
  int effective;                 /* effective count           */
  int queued;                    /* queued                    */
  int avg_count;                 /* average times             */
}buf;

extern struct tpd_device *tpd;

int tpd_lpfx[5] = {0,0,0,0,0};
int tpd_lpfy[5] = {0,0,0,0,0};
int tpd_lpfw[5] = {9,4,2,1,1};
int tpd_lpfc    = 0;

int tpd_sw_status = 0;   // sw down status
int tpd_hw_status = 0;  // hw down status

static int raw_x, raw_y, raw_z1, raw_z2;

/* internal function definition */
static void tpd_timer_fn(unsigned long);
static void tpd_tasklet(unsigned long unused);
irqreturn_t tpd_handler(int, void *);

static void tpd_buffer_init(void);

static void stop_timer(int keep_state);
static void tpd_down(int cx, int cy, int cd, int cp, int *px, int *py);
static void tpd_up(int cx, int cy, int cd, int cp);
extern void MT6573_IRQMask(unsigned int line);
extern void MT6573_IRQUnmask(unsigned int line);
extern void MT6573_IRQClearInt(unsigned int line);

/************************** auxiliary functions *****************************/
/* clean buffer content */
static void tpd_buffer_init(void) {
    int i         = RING_BUF_SIZE;
    buf.count     = 0;
    buf.last      = 0;
    buf.effective = 0;
    buf.index     = 0;
    buf.queued    = 0;
    buf.avg_count = 0;
    for(;i--;) buf.d[i] = 0;
}

/* invoke by tpd_init, initialize r-type touchpanel */
int tpd_local_init(void) {
    /*if(hwEnableClock(MT6573_PDN_PERI_ADC,"Touch")==FALSE)
        TPD_DMESG("hwEnableClock ADC failed.");
    if(hwEnableClock(MT6573_PDN_PERI_TP,"Touch")==FALSE)
        TPD_DMESG("hwEnableClock TP failed.");*/
    tpd_adc_init();
    init_timer(&(tpd->timer));
    tpd->timer.function = tpd_timer_fn;
    tasklet_init(&(tpd->tasklet), tpd_tasklet, 0);
    if(request_irq(MT6573_TOUCH_IRQ_LINE, tpd_handler, 0, "MTK_TS", NULL))
        TPD_DMESG("request_irq failed.\n");
    tpd_buffer_init();
    return 0;
}

/* timer keep polling touch panel status */
void tpd_timer_fn(unsigned long arg) {
    if(tpd->tasklet.state != TASKLET_STATE_RUN)
        tasklet_hi_schedule(&(tpd->tasklet));
}

/* handle interrupt from touch panel (by dispatching to tasklet */
irqreturn_t tpd_handler(int irq, void *dev_id)
{
    tpd_hw_status = (tpd_read_status()?1:0);
    if(tpd->tasklet.state != TASKLET_STATE_RUN)
        tasklet_hi_schedule(&(tpd->tasklet));
    return IRQ_HANDLED;
}

static void stop_timer(int keep_state) {
    //del_timer(&(tpd->timer));
    if(!keep_state) tpd_buffer_init();
    mod_timer(&(tpd->timer),jiffies+HZ);
    tpd_sw_status = 0;
}

/**************************** touch down / up *******************************/
static void tpd_down(int cx, int cy, int cd, int cp, int *px, int *py) {

    tpd_lpfx[tpd_lpfc]=cx;
    tpd_lpfy[tpd_lpfc]=cy;
    tpd_lpfc=(tpd_lpfc+4)%5;
    #ifdef TPD_HAVE_TREMBLE_ELIMINATION
    /*tremble elimination */
    if(( (*px-cx)*(*px-cx)+(*py-cy)*(*py-cy)) < 17) cx=*px,cy=*py;
    else *px = cx, *py = cy;
    #endif
    //TPD_DEBUG("[DOWN] [x%4d y%4d d%1d p%6d)",cx, cy, cd, cp);
    #ifdef TPD_HAVE_BUTTON
    if(cy>=TPD_BUTTON_HEIGHT) {
        if(buf.count<5) tpd_button(cx,cy,1);
        return;
    }
    if(cy<TPD_BUTTON_HEIGHT && tpd->btn_state && cp<TPD_PRESSURE_MAX) 
        tpd_button(cx,cy,0);
    #endif
    cp=(255*(TPD_PRESSURE_MAX-cp))/(TPD_PRESSURE_MAX-TPD_PRESSURE_MIN);
    do {
        input_report_abs(tpd->dev, ABS_X, cx);
        input_report_abs(tpd->dev, ABS_Y, cy);
        input_report_abs(tpd->dev, ABS_PRESSURE, cp);
        input_report_key(tpd->dev, BTN_TOUCH, 1);
    } while(0);
    input_sync(tpd->dev);
    buf.effective++;
    buf.last = buf.count+1;
    TPD_DEBUG_PRINT_DOWN;
    TPD_EM_PRINT(raw_x, raw_y, cx, cy, (raw_z1 << 16 | raw_z2), 1);
    //printk("f[%5d %5d %5d]", cx, cy, cp);
}
  
static void tpd_up(int cx, int cy, int cd, int cp) {
    int i, pending = 0;
    //TPD_DEBUG("[ UP ] [x%4d y%4d d%1d p%6d)\n",cx, cy, cd, cp);
    tpd_buffer_init();
    #ifdef TPD_HAVE_BUTTON
    if(tpd->btn_state) tpd_button(cx,cy,0); else
    #endif
    do {
        input_report_abs(tpd->dev, ABS_X, cx);
        input_report_abs(tpd->dev, ABS_Y, cy);
        input_report_abs(tpd->dev, ABS_PRESSURE, 0);
        input_report_key(tpd->dev, BTN_TOUCH, 0);
        input_sync(tpd->dev);
    } while(0);
    for(i=0;i<5;i++) {
        tpd_lpfx[i]=0;tpd_lpfy[i]=0;tpd_lpfc=0;
    }
    TPD_DEBUG_PRINT_UP;
    TPD_EM_PRINT(raw_x, raw_y, cx, cy, (raw_z1 << 16 | raw_z2), 0);
    //printk("u[%5d %5d %5d]", cx, cy, cp);
}

/*************************** main event handler ******************************/
void tpd_sampling(int *cx, int *cy, int *cp, int *cd) {
    int x=0,y=0,z1=0,z2=0;
    int maxy, miny, i,j,k=0;
    int maxx, minx;
    u16 rx=0,ry=0,rz1=0,rz2=0;
    /* ============ sampling & averaging ===========*/
    //rx   = tpd_read(TPD_X);
    //ry   = tpd_read(TPD_Y);
    //rz1  = tpd_read(TPD_Z1);
    //rz2  = tpd_read(TPD_Z2);
    //printk("raw1[%5d %5d %5d %5d] ", rx, ry, rz1, rz2);
    
    while(k<5) {
        i=0, x=0, y=0, z1=0, z2=0;
        maxy = 0, miny = 4096;
        maxx = 0, minx = 4096;
        while(i<8+k) {
            i++;
            rx   = tpd_read(TPD_X);
            ry   = tpd_read(TPD_Y);
            rz1  = tpd_read(TPD_Z1);
            rz2  = tpd_read(TPD_Z2);    
            //printk("raw1[%5d %5d %5d %5d] \n", rx, ry, rz1, rz2);    
            if(rz1==0 && rz2==1023) { i--; break; }
            //if(rx==0 && ry==1023) { i--; break; }
            if(i==1) udelay(10);
            if(maxy<ry) maxy=ry; 
            if(miny>ry) miny=ry;
            if(maxx<rx) maxx=rx; 
            if(minx>rx) minx=rx;
            x  += rx; y  += ry; z1 += rz1; z2 += rz2;
        } 
        if(i==0 || maxy-miny<8 && maxx-minx<=8+2*k) break;  else k++;
        udelay(100);
        //if(maxy-miny<=8+2*k && maxx-minx<=8+2*k) break; else k++;
    }  
    //printk("%s(%d) ", (k==8?"fail":"pass"),k);
    //rx   = tpd_read(TPD_X);
    //ry   = tpd_read(TPD_Y);
    //rz1  = tpd_read(TPD_Z1);
    //rz2  = tpd_read(TPD_Z2);
    //printk("raw2[%5d %5d %5d %5d] (%4d %4d)", rx, ry, rz1, rz2, maxy, miny);
    if(i==0)      x   =0, y   =0, z1   =0, z2   =0;
    else          x  /=i, y  /=i, z1  /=i, z2  /=i;
    //printk("raw2[%5d %5d %5d %5d] \n", x, y, z1, z2);    
    raw_x = x; raw_y = y; raw_z1 = z1; raw_z2 = z2;
        
    // only difference from 6516 to 6573, in basic part
    //if(!z1) *cp=TPD_PRESSURE_MAX+1,*cd=0;
    if(!rx && ry==4095) *cp=TPD_PRESSURE_MAX+1,*cd=0;
    else *cp = (x+1)*(z2-z1)/(z1+1), *cd=1;
    *cx=x, *cy=y;
  
    //printk("x = %d, y = %d, p = %d\n",*cx, *cy, *cp);
}

/* handle touch panel interrupt for down / up event */
void tpd_tasklet(unsigned long unused) {
    static int x1,y1,p1;//,d1,round=0;
    int cx=0, cy=0, cp=0, cd=0;               /* current point */
    int fx=0, fy=0, fp=0, fd=0;               /* foresee point */
    int ni=0, i =0;                           /* index         */
    static int lx=0, ly=0;                    /* latest  point */
    static int px=0, py=0;
    static struct touch_info buf_kp[3];
    static int buf_p=1, buf_c=2, buf_f=0, down = 0;
    int dx;

    TPD_DEBUG_SET_TIME;

    if (tpd_em_debounce_time != 0) {
        tpd_set_debounce_time(tpd_em_debounce_time);
        tpd_em_debounce_time = 0;
    }
    
    //struct timeval tv;
    //long t1, t2;
    //do_gettimeofday(&tv);
    //t1 = tv.tv_sec*1000000l+tv.tv_usec;

    //tpd_sampling(&cx, &cy, &cp, &cd);
    //tpd_calibrate(&cx, &cy);
    //printk("c [%5d %5d %5d %5d]\n", cx, cy, cp, cd);
    //mod_timer(&(tpd->timer),jiffies+TPD_DELAY);
    //return;
    tpd_sampling(&cx, &cy, &cp, &cd);
    tpd_calibrate(&cx, &cy);

    /* ============= boundary condition ============*/
    if(cx<0) cx=0;
    if(cy<0) cy=0;
    if(cx>TPD_RES_X) cx=TPD_RES_X;
    #if defined(TPD_HAVE_BUTTON) || defined(TPD_HAVE_VIRTUAL_KEY)
    if(cy>TPD_RES_Y && cy<TPD_BUTTON_HEIGHT) cp=TPD_PRESSURE_MAX+1,cd=0;
    #else
    if(cy>TPD_RES_Y) cy=TPD_RES_Y;
    #endif

    if(tpd_mode==TPD_MODE_KEYPAD &&
        ((cd==0 && down==1) || (tpd_mode_axis==0 && cy>=tpd_mode_min && cy<=tpd_mode_max) ||
         (tpd_mode_axis==1 && cx>=tpd_mode_min && cx<=tpd_mode_max))) {
        /* this segment of code should be refactorized with following drift elimination */
        #ifdef TPD_HAVE_DRIFT_ELIMINATION
        /* drift elimination */
        if(buf.count==0 || (cp>=TPD_PRESSURE_NICE && cd)) {
          TPD_DEBUG("drift eliminated (0) or Pressure filtering\n");
        #ifdef TPD_HAVE_ADV_DRIFT_ELIMINATION
        } else if (buf.count==1 && cp>TPD_ADE_P1) {
          TPD_DEBUG("drift eliminated (1)\n");
        } else if (buf.count==1) {
          /* queue first good point, discard it if 2nd pt is bad */
          TPD_BUF_QUEUE(cx,cy,cd,cp);
        } else if (buf.count==2 && cp>TPD_ADE_P2) {
          TPD_DEBUG("drift eliminated (2)\n");
          TPD_BUF_UNQUEUE(fx,fy,fd,fp);
        #endif
        #else
        if(0) {
        #endif
        } else {
            buf_f = ((buf_f+1)%3);
            buf_c = ((buf_f+2)%3);
            buf_p = ((buf_f+1)%3);
            buf_kp[buf_f].x1 = (cd?cx:buf_kp[buf_c].x1);
            buf_kp[buf_f].y1 = (cd?cy:buf_kp[buf_c].y1);
            dx = buf_kp[buf_f].x1 - buf_kp[buf_c].x1;
            buf_kp[buf_f].count = (cd?(dx*dx<tpd_mode_keypad_tolerance?buf_kp[buf_c].count+1:1):0);
            if(buf_kp[buf_c].count<2) { if(down) {
                down=0;
                tpd_up(buf_kp[buf_p].x1, buf_kp[buf_p].y1,1,0);
                input_sync(tpd->dev); 
            } }
            if(buf_kp[buf_c].count>1 ||
               (buf_kp[buf_c].count==1 && (
                buf_kp[buf_p].count==0 || buf_kp[buf_f].count==0
                || (buf_kp[buf_f].x1-buf_kp[buf_c].x1)*(buf_kp[buf_c].x1-buf_kp[buf_p].x1)<=0))) {
                tpd_down(buf_kp[buf_c].x1, buf_kp[buf_c].y1,1,1, &px, &py);
                input_sync(tpd->dev);
                down = 1;
            }
            if(cd==0) { if(down) {
                down=0;
                tpd_up(buf_kp[buf_p].x1, buf_kp[buf_p].y1,1,0);
                input_sync(tpd->dev); 
                buf.count = 0;
            }}
            else mod_timer(&(tpd->timer),jiffies+TPD_DELAY);
        } mod_timer(&(tpd->timer),jiffies+TPD_DELAY);
        buf.count++;
    } else {

    // to avoid miss of tpd down when press light to heavy
    if(tpd_sw_status==0) {
        if(tpd_hw_status==1) {
            if(cd==0) stop_timer(true);
        } else return;
    } tpd_sw_status = 1;
    

    /* ================= buffering =================*/
    /* save new point in ni, get point from index */
    ni=TPD_BUF_NP(); 
    buf.x[ni]=cx;
    buf.y[ni]=cy;
    buf.d[ni]=cd;
    buf.p[ni]=cp;
    fx=(cx=buf.x[buf.index]);
    fy=(cy=buf.y[buf.index]);
    fd=(cp=buf.p[buf.index]);
    fp=(cd=buf.d[buf.index]);
    buf.index = ni;

    tpd_lpfx[tpd_lpfc]=cx;
    tpd_lpfy[tpd_lpfc]=cy;
    x1 = 0, y1 = 0, p1 = 0;
    for(i=tpd_lpfc;i<tpd_lpfc+5;i++) {
      x1 += (tpd_lpfx[i%5]*tpd_lpfw[i-tpd_lpfc]);
      y1 += (tpd_lpfy[i%5]*tpd_lpfw[i-tpd_lpfc]);
      if(tpd_lpfx[i%5]!=0 || tpd_lpfy[i%5]!=0) p1+=tpd_lpfw[i-tpd_lpfc];
    } 
    cx=x1/(p1?p1:1); cy=y1/(p1?p1:1);

    TPD_DEBUG(">%d [%8d %8d %8d %8d] - ",buf.count,cx,cy,cp,cd);

    /* pressure thresholding */
    if(cp>TPD_PRESSURE_MAX || (cx==TPD_RES_X && cy==TPD_RES_Y)) 
        cd=0,cp=TPD_PRESSURE_MAX+1;

    if(cd==1) {
      #ifdef TPD_HAVE_DRIFT_ELIMINATION
      /* drift elimination */
      if(buf.count==0) {
        TPD_DEBUG("drift eliminated (0)\n");
      #ifdef TPD_HAVE_ADV_DRIFT_ELIMINATION
      } else if (buf.count==1 && cp>TPD_ADE_P1) {
        TPD_DEBUG("drift eliminated (1)\n");
      } else if (buf.count==1) {
        /* queue first good point, discard it if 2nd pt is bad */
        TPD_BUF_QUEUE(cx,cy,cd,cp);
      } else if (buf.count==2 && cp>TPD_ADE_P2) {
        TPD_DEBUG("drift eliminated (2)\n");
        TPD_BUF_UNQUEUE(fx,fy,fd,fp);
      #endif
      #else
      if(0) {
      #endif
      } else {
        /* if a point is already queued, pop and send it */
        if(buf.queued) {
          TPD_BUF_UNQUEUE(fx,fy,fd,fp);
          /* if the queued point is a bad point.. */
          #ifdef TPD_HAVE_BUTTON
          /* if point is for btn, don't process it */
          if(cy>=TPD_BUTTON_HEIGHT) {}
          #else
          if(0) {}
          #endif
          else if(fp>TPD_PRESSURE_NICE && buf.avg_count<2) {
            /* average current point and queued point */
            cx = (cx+fx)/2;
            cy = (cy+fy)/2;
            buf.avg_count++;
          } else {
            TPD_DEBUG("pop and down\n");
            tpd_down(fx,fy,fd,fp, &px, &py);
            lx=fx,ly=fy;
            buf.avg_count=0;
          }
        }
        #ifdef TPD_HAVE_BUTTON
        /* queue points near bottom btns, to prevent mis-touch of menu */
        if(cy>TPD_BUTTON_HEIGHT-20 && cy<TPD_BUTTON_HEIGHT) {
          TPD_DEBUG("queued\n");
          TPD_BUF_QUEUE(cx,cy,cd,cp);
        }
        #else
        if(0) {}
        #endif
        /* discontinuity elimination - queueing */
        /* also point averaging */
        else if(buf.d[ni]==0 || cp>TPD_PRESSURE_NICE) {
          TPD_DEBUG("queued\n");
          TPD_BUF_QUEUE(cx,cy,cd,cp);
        } else {
          TPD_DEBUG("down\n");
          tpd_down(cx,cy,cd,cp, &px, &py);
          lx=cx,ly=cy;
          buf.effective++;
          buf.last = buf.count+1;
        }
      }
      buf.count++;
      mod_timer(&(tpd->timer),jiffies+TPD_DELAY);
    } else {
      if(buf.queued) {
        if(buf.count-buf.last>TPD_COUNT_TOLERANCE) {
          /* single point elimination */
          TPD_BUF_UNQUEUE(cx,cy,cd,cp);
          TPD_DEBUG("unqueue : ");
          if(buf.effective==0) {
            TPD_DEBUG("sp eliminated.\n");
            stop_timer(false);
          } else {
            /* track extension */
            if(cp<TPD_PRESSURE_NICE) { /* only when end-point is good..*/
              tpd_down(cx,cy,cd,cp, &px, &py);
              #ifdef TPD_HAVE_TRACK_EXTENSION
              if(buf.effective>0) {
                cx=cx+cx-px,cy=cy+cy-py;
                tpd_down(cx,cy,cd,cp, &px, &py);
                TPD_DEBUG("track extend.\n");
              }
              lx=cx,ly=cy;
              #endif
            } else TPD_DEBUG("end point is not good, discard\n");
            tpd_up(lx,ly,cd,cp);
            stop_timer(false);
          }
        } else {
          /* waiting for next down event */
          TPD_DEBUG("delay handling.\n");
          buf.count++;
          mod_timer(&(tpd->timer),jiffies+TPD_DELAY);
        }
      } else {
        TPD_DEBUG("no queue.\n");
        /* since no queue and no touch, stop timer after specific times of trial */
        /* only increase buf.count after real first down. */
        //if(buf.effective) buf.count++;
        if(buf.count || !buf.last) buf.count++;
        if(buf.count-buf.last>TPD_COUNT_TOLERANCE || buf.count==0) {
          tpd_up(lx,ly,0,TPD_PRESSURE_MAX+1);
          stop_timer(true);
        } else mod_timer(&(tpd->timer),jiffies+TPD_DELAY);
      }
    }
    }
    //printk("\n");
    //do_gettimeofday(&tv);
    //t2 = tv.tv_sec*1000000l+tv.tv_usec;
    //printk("%ld\n", t2-t1);
    return;
}

//#ifdef CONFIG_HAS_EARLYSUSPEND
/* platform device functions */
void tpd_suspend(struct early_suspend *h) {

    /*MT6573_IRQClearInt(MT6573_WDT_IRQ_LINE);
    MT6573_IRQMask(MT6573_TOUCH_IRQ_LINE);
    if(hwDisableClock(MT6573_PDN_PERI_TP,"Touch")==FALSE)
        TPD_DMESG("entering suspend mode - hwDisableClock failed.");
    if(hwDisableClock(MT6573_PDN_PERI_ADC,"Touch")==FALSE)
        TPD_DMESG("entering suspend mode - hwDisableClock failed.");*/
    tpd_hw_status = 0;
}
void tpd_resume(struct early_suspend *h) {
    /*if(hwEnableClock(MT6573_PDN_PERI_ADC,"Touch")==FALSE)
        TPD_DMESG("resume from suspend mode - hwEnableClock ADC failed.");
    if(hwEnableClock(MT6573_PDN_PERI_TP,"Touch")==FALSE)
        TPD_DMESG("resume from suspend mode - hwEnableClock TP failed.");*/

    //MT6573_IRQUnmask(MT6573_TOUCH_IRQ_LINE);
}
//#endif



