
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/xlog.h>
#include <linux/types.h>
#include <linux/kobject.h>

#include "mach/mtk_thermal_monitor.h"
#include <mach/system.h>

#if 0
#define mtk_cooler_shutdown_dprintk(fmt, args...) \
  do { xlog_printk(ANDROID_LOG_DEBUG, "thermal/cooler/shutdown", fmt, ##args); } while(0)
#else
#define mtk_cooler_shutdown_dprintk(fmt, args...) 
#endif

#define MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN  3

static struct thermal_cooling_device *cl_shutdown_dev[MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN] = {0};
static unsigned long cl_shutdown_state[MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN] = {0};

static int 
mtk_cl_shutdown_get_max_state(struct thermal_cooling_device *cdev,
                              unsigned long *state)
{        
  *state = 1;
  mtk_cooler_shutdown_dprintk("mtk_cl_shutdown_get_max_state() %s %d\n", cdev->type, *state);
  return 0;
}

static int 
mtk_cl_shutdown_get_cur_state(struct thermal_cooling_device *cdev,
                              unsigned long *state)
{
#if 1 // cannot use this way for now since devdata is used by mtk_thermal_monitor
  *state = *((unsigned long*) cdev->devdata);
#else
  *state = cl_shutdown_state[(int) cdev->type[16]];
#endif
  mtk_cooler_shutdown_dprintk("mtk_cl_shutdown_get_cur_state() %s %d\n", cdev->type, *state);
  return 0;
}

static int 
mtk_cl_shutdown_set_cur_state(struct thermal_cooling_device *cdev,
                              unsigned long state)
{
  mtk_cooler_shutdown_dprintk("mtk_cl_shutdown_set_cur_state() %s %d\n", cdev->type, state);
#if 1
  *((unsigned long*) cdev->devdata) = state;
#else
  cl_shutdown_state[(int) cdev->type[16]] = state;
#endif
  if(1 == state)
  {
    // send uevent to notify current call must be dropped
    char event[] = "SHUTDOWN=1";
    char *envp[] = { event, NULL };
    
    kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
  }
#if 0 // unnecessary...only send uevent when needed
  else
  {
    // send uevent to notify no drop call is necessary
    char event[] = "SHUTDOWN=0";
    char *envp[] = { event, NULL };
    
    kobject_uevent_env(&(cdev->device.kobj), KOBJ_CHANGE, envp);
  }
#endif
    
  return 0;
}

/* bind fan callbacks to fan device */
static struct thermal_cooling_device_ops mtk_cl_shutdown_ops = {
  .get_max_state = mtk_cl_shutdown_get_max_state,
  .get_cur_state = mtk_cl_shutdown_get_cur_state,
  .set_cur_state = mtk_cl_shutdown_set_cur_state,
};

static int mtk_cooler_shutdown_register_ltf(void)
{
  int i;
  mtk_cooler_shutdown_dprintk("register ltf\n");

  for (i = MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN; i-- > 0; )
  {
    char temp[20] = {0};
    sprintf(temp, "mtk-cl-shutdown%02d", i);
    cl_shutdown_dev[i] = mtk_thermal_cooling_device_register(temp, 
                                                             (void*) &cl_shutdown_state[i],
                                                             &mtk_cl_shutdown_ops);
  }

#if 0
  cl_shutdown_dev = mtk_thermal_cooling_device_register("mtk-cl-shutdown", 
                                                         NULL,
                                                         &mtk_cl_shutdown_ops);
#endif

  return 0;
}

static void mtk_cooler_shutdown_unregister_ltf(void)
{
  int i;
  mtk_cooler_shutdown_dprintk("unregister ltf\n");

  for (i = MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN; i-- > 0; )
  {
    if (cl_shutdown_dev[i])
    {
      mtk_thermal_cooling_device_unregister(cl_shutdown_dev[i]);
      cl_shutdown_dev[i] = NULL;
      cl_shutdown_state[i] = 0;
    }
  }
#if 0
  if (cl_shutdown_dev) 
  {
    mtk_thermal_cooling_device_unregister(cl_shutdown_dev);
    cl_shutdown_dev = NULL;
  }
#endif
}


static int __init mtk_cooler_shutdown_init(void)
{
  int err = 0;
  int i;

  for (i = MAX_NUM_INSTANCE_MTK_COOLER_SHUTDOWN; i-- > 0; )
  {
    cl_shutdown_dev[i] = NULL;
    cl_shutdown_state[i] = 0;
  }

  //cl_shutdown_dev = NULL;

  mtk_cooler_shutdown_dprintk("init\n");

  err = mtk_cooler_shutdown_register_ltf();
  if (err)
    goto err_unreg;

  return 0;

err_unreg:
  mtk_cooler_shutdown_unregister_ltf();
  return err;
}

static void __exit mtk_cooler_shutdown_exit(void)
{
  mtk_cooler_shutdown_dprintk("exit\n");
    
  mtk_cooler_shutdown_unregister_ltf();
}

module_init(mtk_cooler_shutdown_init);
module_exit(mtk_cooler_shutdown_exit);



