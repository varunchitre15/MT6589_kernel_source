#ifndef _CAMERA_FEATURE_DEBUG_H_
#define _CAMERA_FEATURE_DEBUG_H_


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/*
    (1) GLOBAL_ENABLE_MY_xxx == 0
         --> force to disable.
    (2) GLOBAL_ENABLE_MY_xxx == 1
         --> ENABLE_MY_xxx in local file decides to enable/disable.
        (2.1) ENABLE_MY_xxx in local file == 1 --> enable.
        (2.2) ENABLE_MY_xxx in local file == 0 --> disable.
        (2.3) ENABLE_MY_xxx in local file undefine
               --> ENABLE_MY_xxx in global file decides to enable/disable.
*/
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Global On/Off
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#undef  GLOBAL_ENABLE_MY_LOG
#define GLOBAL_ENABLE_MY_LOG        (1)

#undef  GLOBAL_ENABLE_MY_WARN
#define GLOBAL_ENABLE_MY_WARN       (1)

#undef  GLOBAL_ENABLE_MY_ERR
#define GLOBAL_ENABLE_MY_ERR        (1)

#undef  GLOBAL_ENABLE_MY_LOG_OBJ
#define GLOBAL_ENABLE_MY_LOG_OBJ    (1)

#undef  GLOBAL_ENABLE_MY_ASSERT
#define GLOBAL_ENABLE_MY_ASSERT     (1)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  Local On/Off
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#if (GLOBAL_ENABLE_MY_LOG==0)
  #undef  ENABLE_MY_LOG
  #define ENABLE_MY_LOG             (0) //Don't change.
#else
  #ifndef ENABLE_MY_LOG
    #define ENABLE_MY_LOG           (1) //Modify me.
  #endif
#endif

#if (GLOBAL_ENABLE_MY_WARN==0)
  #undef  ENABLE_MY_WARN
  #define ENABLE_MY_WARN            (0) //Don't change.
#else
  #ifndef ENABLE_MY_WARN
    #define ENABLE_MY_WARN          (1) //Modify me.
  #endif
#endif

#if (GLOBAL_ENABLE_MY_ERR==0)
  #undef  ENABLE_MY_ERR
  #define ENABLE_MY_ERR             (0) //Don't change.
#else
  #ifndef ENABLE_MY_ERR
    #define ENABLE_MY_ERR           (1) //Modify me.
  #endif
#endif

#if (GLOBAL_ENABLE_MY_LOG_OBJ==0)
  #undef  ENABLE_MY_LOG_OBJ
  #define ENABLE_MY_LOG_OBJ         (0) //Don't change.
#else
  #ifndef ENABLE_MY_LOG_OBJ
    #define ENABLE_MY_LOG_OBJ       (0) //Modify me.
  #endif
#endif

#if (GLOBAL_ENABLE_MY_ASSERT==0)
  #undef  ENABLE_MY_ASSERT
  #define ENABLE_MY_ASSERT          (0) //Don't change.
#else
  #ifndef ENABLE_MY_ASSERT
    #define ENABLE_MY_ASSERT        (1) //Modify me.
  #endif
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#if (ENABLE_MY_LOG != 0)
    #define MY_LOG(fmt, arg...) XLOGD(fmt, ##arg)
#else
    #define MY_LOG(fmt, arg...)
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#if (ENABLE_MY_WARN != 0)
    #define MY_WARN(fmt, arg...) XLOGW(fmt, ##arg)
#else
    #define MY_WARN(fmt, arg...)
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#if (ENABLE_MY_ERR != 0)
    #define MY_ERR(fmt, arg...)  XLOGE("<ln%5d> "fmt, __LINE__, ##arg)
#else
    #define MY_ERR(fmt, arg...)
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#if (ENABLE_MY_LOG_OBJ != 0)
    #define MY_LOG_OBJ(fmt, arg...)\
        struct MyLogObj {\
            MyLogObj() {\
                MY_LOG("[MyLogObj] %p\n", this);\
                MY_LOG(fmt, ##arg);\
            }\
        }
    #define MY_LOG_OBJ_ONCE(fmt, arg...)\
        {static MY_LOG_OBJ(fmt, ##arg) obj;}

#else
    #define MY_LOG_OBJ(fmt, arg...)
    #define MY_LOG_OBJ_ONCE(fmt, arg...)
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#if (ENABLE_MY_ASSERT != 0)
    #define MY_ASSERT(x, str)\
        if (x) {} \
        else   {  \
            MY_ERR("[Assert %s, %d]: %s", __FILE__, __LINE__, str); while(1); \
        }
#else
    #define MY_ASSERT(x, str)
#endif

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#endif // _CAMERA_FEATURE_DEBUG_H_

