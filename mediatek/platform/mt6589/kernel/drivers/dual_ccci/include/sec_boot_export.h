#ifndef SEC_BOOT_H
#define SEC_BOOT_H

/**************************************************************************
 * [S-BOOT]
 **************************************************************************/
/* S-BOOT Attribute */
#define ATTR_SBOOT_DISABLE                  0x00
#define ATTR_SBOOT_ENABLE                   0x11
#define ATTR_SBOOT_ONLY_ENABLE_ON_SCHIP     0x22


/**************************************************************************
 * EXPORT FUNCTION
 **************************************************************************/
extern int sec_boot_init (void);
extern int sec_boot_enabled (void);
extern int sec_modem_auth_enabled (void);
extern int sec_schip_enabled (void);

#endif /* SEC_BOOT_H */

