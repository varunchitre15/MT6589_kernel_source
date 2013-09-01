

 
#define MOD_NO_IN_1_DEVAPC                  16
#define DEVAPC_MODULE_MAX_NUM               32  
#define DEVAPC_TAG                          "DEVAPC"
#define MAX_TIMEOUT                         100
#define ABORT_EMI                           0x20000008 
 
 
// device apc attribute
 typedef enum
 {
     E_L0=0,
     E_L1,
     E_L2,
     E_L3,
     E_MAX_APC_ATTR
 }APC_ATTR;
 
 // device apc index 
 typedef enum
 {
     E_DEVAPC0=0,
     E_DEVAPC1,  
     E_DEVAPC2,
     E_DEVAPC3,
     E_DEVAPC4,
     E_MAX_DEVAPC
 }DEVAPC_NUM;
 
 // domain index 
 typedef enum
 {
     E_AP_MCU = 0,
     E_MD1_MCU ,
     E_MD2_MCU , 
     E_MM_MCU ,
     E_MAX
 }E_MASK_DOM;
 
 
 typedef struct {
     const char      *device_name;
     bool            forbidden;
 } DEVICE_INFO;
 
  
 static DEVICE_INFO D_APC0_Devices[] = {
     {"0",              FALSE},
     {"1",              TRUE},
     {"2",              TRUE},
     {"3",              FALSE},
     {"4",              TRUE},
     {"5",              TRUE},
     {"6",              TRUE},
     {"7",              TRUE},
     {"8",              TRUE},
     {"9",              TRUE},
     {"10",             FALSE},
     {"11",             TRUE},
     {"12",             TRUE},
     {"13",             TRUE},
     {"14",             TRUE},
     {"15",             TRUE},
     {"16",             TRUE},
     {"17",             TRUE},
     {"18",             TRUE},
     {"19",             TRUE},
     {"20",             TRUE},
     {"21",             FALSE},
     {"22",             TRUE},
     {"23",             TRUE},
     {NULL,             FALSE},
 };
 
 static DEVICE_INFO D_APC1_Devices[] = {
     {"0",              TRUE},
     {"1",              FALSE},
     {"2",              TRUE},
     {"3",              FALSE},
     {"4",              TRUE},
     {"5",              TRUE},
     {"6",              TRUE},
     {"7",              TRUE},
     {"(Reserved)",     FALSE},
     {"(Reserved)",     FALSE},
     {"10",             TRUE},
     {"11",             TRUE},
     {"(Reserved)",     FALSE},
     {"13",             TRUE},
     {"14",             TRUE},
     {"(Reserved)",     FALSE},
     {"16",             TRUE},
     {"17",             TRUE},
     {"18",             TRUE},
     {NULL,             FALSE},
 };
 
 
 static DEVICE_INFO D_APC2_Devices[] = {
     {"0",              TRUE},
     {"1",              FALSE},
     {"2",              TRUE},
     {"3",              FALSE},
     {"4",              FALSE},
     {"5",              FALSE},
     {"6",              TRUE},
     {"(reserved)",     FALSE},
     {"8",              TRUE},
     {"9",              FALSE},
     {"10",             TRUE},
     {"11",             TRUE},
     {"12",             TRUE},
     {"13",             FALSE},
     {"14",             TRUE},
     {"15",             FALSE},
     {"16",             TRUE},
     {"17",             TRUE},
     {"18",             TRUE},
     {"19",             TRUE},
     {"20",             FALSE},
     {"21",             TRUE},
     {"22",             TRUE},
     {"23",             TRUE},
     {"24",             TRUE},
     {"25",             TRUE},
     {"26",             TRUE},
     {"27",             TRUE},
     {"28",             TRUE},
     {"29",             TRUE},
     {NULL,             TRUE},
 };

 
 static DEVICE_INFO D_APC3_Devices[] = {
     {"0",              TRUE},
     {"1",              TRUE},
     {"2",              TRUE},
     {"3",              TRUE},
     {"4",              TRUE},
     {"5",              TRUE},
     {"6",              TRUE},
     {"7",              TRUE},
     {"8",              TRUE},
     {"9",              TRUE},
     {"10",             TRUE},
     {"11",             TRUE},
     {"12",             TRUE},
     {"13",             TRUE},
     {"14",             TRUE},
     {"15",             TRUE},
     {"16",             TRUE},
     {"17",             TRUE},
     {"18",             TRUE},
     {"19",             TRUE},
     {"20",             TRUE},
     {NULL,             FALSE},
 };
 
 
 static DEVICE_INFO D_APC4_Devices[] = {
     {"0",              TRUE},
     {"1",              TRUE},
     {"2",              TRUE},
     {"3",              TRUE},
     {"4",              TRUE},
     {"5",              TRUE},
     {"(reserved)",     FALSE},
     {"(reserved)",     FALSE},
     {"8",              TRUE},
     {"9",              TRUE},
     {"10",             TRUE},
     {"(reserved)",     FALSE},
     {"(reserved)",     FALSE},
     {"(reserved)",     FALSE},
     {"(reserved)",     FALSE},
     {"(reserved)",     FALSE},
     {"16",             TRUE},
     {"17",             TRUE},
     {"18",             TRUE},
     {"19",             TRUE},
     {"20",             TRUE},
     {"21",             TRUE},
     {"22",             FALSE},
     {"23",             TRUE},
     {NULL,             FALSE},
 };                         
 
 
#define SET_SINGLE_MODULE(apcnum, domnum, index, module, permission_control)     \
 {                                                                               \
     mt65xx_reg_sync_writel(readl(DEVAPC##apcnum##_D##domnum##_APC_##index) & ~(0x3 << (2 * module)), DEVAPC##apcnum##_D##domnum##_APC_##index); \
     mt65xx_reg_sync_writel(readl(DEVAPC##apcnum##_D##domnum##_APC_##index) | (permission_control << (2 * module)),DEVAPC##apcnum##_D##domnum##_APC_##index); \
 }                                                                               \
 
#define UNMASK_SINGLE_MODULE_IRQ(apcnum, domnum, module_index)                  \
 {                                                                               \
     mt65xx_reg_sync_writel(readl(DEVAPC##apcnum##_D##domnum##_VIO_MASK) & ~(module_index),      \
         DEVAPC##apcnum##_D##domnum##_VIO_MASK);                                 \
 }                                                                               \
 
#define CLEAR_SINGLE_VIO_STA(apcnum, domnum, module_index)                     \
 {                                                                               \
     mt65xx_reg_sync_writel(readl(DEVAPC##apcnum##_D##domnum##_VIO_STA) | (module_index),        \
         DEVAPC##apcnum##_D##domnum##_VIO_STA);                                  \
 }                                                                               \

 
