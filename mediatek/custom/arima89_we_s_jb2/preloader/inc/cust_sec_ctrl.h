


#ifndef CUST_SEC_CTRL_H
#define CUST_SEC_CTRL_H

#include "typedefs.h"
#include "proj_cfg.h"
#include "mtk_key.h"

/**************************************************************************
 * [ROM INFO]
 **************************************************************************/
#define PROJECT_NAME                        "CUST"
#define PLATFORM_NAME                       "MT6589"

/**************************************************************************
 * [SEC ENV CONTROL]
 **************************************************************************/
#define SEC_ENV_ENABLE                      TRUE 

/**************************************************************************
 * [CRYPTO SEED]
 **************************************************************************/
#define CUSTOM_CRYPTO_SEED_SIZE             (16)
#define CUSTOM_CRYPTO_SEED                  "1A52A367CB12C458"

/**************************************************************************
 * [SML AES KEY CONTROL]
 **************************************************************************/
/* It can be enabled only if SUSBDL is turned on */
/* Please make sure SUSBDL is on before enabling this flag */
//#define SML_AES_KEY_ANTICLONE_EN

/**************************************************************************
 * [S-USBDL]
 **************************************************************************/
/* S-USBDL Attribute */
#define ATTR_SUSBDL_DISABLE                 0x00
#define ATTR_SUSBDL_ENABLE                  0x11
#define ATTR_SUSBDL_ONLY_ENABLE_ON_SCHIP    0x22
/* S-USBDL Control */
#define SEC_USBDL_CFG                       CUSTOM_SUSBDL_CFG 

/**************************************************************************
 * [FLASHTOOL SECURE CONFIG for (for both of SLA and NON-SLA mode]
 **************************************************************************/
//#define FLASHTOOL_SEC_CFG
#define BYPASS_CHECK_IMAGE_0_NAME           ""
#define BYPASS_CHECK_IMAGE_0_OFFSET         0x0
#define BYPASS_CHECK_IMAGE_0_LENGTH         0x0
#define BYPASS_CHECK_IMAGE_1_NAME           ""
#define BYPASS_CHECK_IMAGE_1_OFFSET         0x0
#define BYPASS_CHECK_IMAGE_1_LENGTH         0x0
#define BYPASS_CHECK_IMAGE_2_NAME           ""
#define BYPASS_CHECK_IMAGE_2_OFFSET         0x0
#define BYPASS_CHECK_IMAGE_2_LENGTH         0x0
/**************************************************************************
 * [FLASHTOOL FORBIT DOWNLOAD CONFIG (for NSLA mode only)]
 **************************************************************************/
//#define FLASHTOOL_FORBID_DL_NSLA_CFG
#define FORBID_DL_IMAGE_0_NAME              ""
#define FORBID_DL_IMAGE_0_OFFSET            0x0
#define FORBID_DL_IMAGE_0_LENGTH            0x0
#define FORBID_DL_IMAGE_1_NAME              ""
#define FORBID_DL_IMAGE_1_OFFSET            0x0
#define FORBID_DL_IMAGE_1_LENGTH            0x0

#define SEC_USBDL_WITHOUT_SLA_ENABLE

#ifdef SEC_USBDL_WITHOUT_SLA_ENABLE
//#define USBDL_DETECT_VIA_KEY
/* if com port wait key is enabled, define the key*/
#ifdef USBDL_DETECT_VIA_KEY
#define COM_WAIT_KEY    KPD_DL_KEY3
#endif
//#define USBDL_DETECT_VIA_AT_COMMAND
#endif

/**************************************************************************
 * [S-BOOT]
 **************************************************************************/
/* S-BOOT Attribute */
#define ATTR_SBOOT_DISABLE                  0x00
#define ATTR_SBOOT_ENABLE                   0x11
#define ATTR_SBOOT_ONLY_ENABLE_ON_SCHIP     0x22
/* S-BOOT Control */
#define SEC_BOOT_CFG                        CUSTOM_SBOOT_CFG 

/* Note : these attributes only work when S-BOOT is enabled */
#define VERIFY_PART_UBOOT                   (TRUE)
#define VERIFY_PART_LOGO                    (TRUE)
#define VERIFY_PART_BOOTIMG                 (TRUE)
#define VERIFY_PART_RECOVERY                (TRUE)
#define VERIFY_PART_ANDSYSIMG               (TRUE)
#define VERIFY_PART_SECSTATIC               (TRUE)

/* For Custom Partition Verification*/
#ifdef SONY_S1_SUPPORT // SONY also needs verification on S1SBL partition
#define VERIFY_PART_CUST                   (TRUE)
#define VERIFY_PART_CUST_NAME              "S1SBL"
#else
#define VERIFY_PART_CUST                   (FALSE)
#define VERIFY_PART_CUST_NAME              ""
#endif // SONY added


/**************************************************************************
 * [DEFINITION CHECK]
 **************************************************************************/
#ifdef SML_AES_KEY_ANTICLONE_EN
#ifndef SECRO_IMG_ANTICLONE_EN
#error "SML_AES_KEY_ANTICLONE_EN is defined. Should also enable SECRO_IMG_ANTICLONE_EN"
#endif
#endif

#if SEC_USBDL_CFG
#if !SEC_ENV_ENABLE
#error "SEC_USBDL_CFG is NOT disabled. Should set SEC_ENV_ENABLE as TRUE"
#endif
#endif

#if SEC_BOOT_CFG
#if !SEC_ENV_ENABLE
#error "SEC_BOOT_CFG is NOT disabled. Should set SEC_ENV_ENABLE as TRUE"
#endif
#endif

#ifdef SEC_USBDL_WITHOUT_SLA_ENABLE
#if !SEC_ENV_ENABLE
#error "SEC_USBDL_WITHOUT_SLA_ENABLE is NOT disabled. Should set SEC_ENV_ENABLE as TRUE"
#endif
#endif

#ifdef USBDL_DETECT_VIA_KEY
#ifndef SEC_USBDL_WITHOUT_SLA_ENABLE
#error "USBDL_DETECT_VIA_KEY can only be enabled when SEC_USBDL_WITHOUT_SLA_ENABLE is enabled"
#endif
#ifndef COM_WAIT_KEY
#error "COM_WAIT_KEY is not defined!!"
#endif
#endif

#ifdef USBDL_DETECT_VIA_AT_COMMAND
#ifndef SEC_USBDL_WITHOUT_SLA_ENABLE
#error "USBDL_DETECT_VIA_AT_COMMAND can only be enabled when SEC_USBDL_WITHOUT_SLA_ENABLE is enabled"
#endif
#endif


#endif   /* CUST_SEC_CTRL_H */
