#ifndef _ETB_REGISTER_H_
#define _ETB_REGISTER_H_

#define ETBID		0x0	/**< Identification register */
#define ETBRDP		0x4	/**< RAM depth register */
#define ETBRWT		0x8	/**< RAM width register */
#define ETBSTS		0xc	/**< Status register */
#define ETBRRD		0x10	/**< RAM read data register */
#define ETBRRP		0x14	/**< RAM read pointer register */
#define ETBRWP		0x18	/**< RAM write pointer register */
#define ETBTRG		0x1c	/**< Trigger counter register */
#define ETBCTL		0x20	/**< Control register */
#define ETBRWD		0x24	/**< RAM write data register */
#define ETBFFSR		0x300	/**< Formatter and flush status register */
#define ETBFFCR		0x304	/**< Formatter and flush control register */
#define ETBITMISCOP0	0xee0	/**< ETB integration test register */
/** Integration test trigger in and flush In acknowledge register */
#define ETBITTRFLINACK	0xee4
#define ETBITTRFLIN	0xee8	/**< Integration test trigger in and flush in */
#define ETBITATBDATA0	0xeec	/**< Integration test ATB data register 0 */
#define ETBITATBCTR2	0xef0	/**< Integration test ATB control register 2 */
#define ETBITATBCTR1	0xef4	/**< Integration test ATB control register 1 */
#define ETBITATBCTR0	0xef8	/**< Integration test ATB control register 0 */
#define ETBIMC		0xf00	/**< Integration mode control register */
#define ETBCTS		0xfa0	/**< Claim tag set register */
#define ETBCTC		0xfa4	/**< Claim tag clear register */
#define ETBLA		0xfb0	/**< Lock access register */
#define ETBLS		0xfb4	/**< Lock status register */
#define ETBAS		0xfb8	/**< Authentication status register */
#define ETBDID		0xfc8	/**< Device ID register */
#define ETBDTI		0xfcc	/**< Device type identifier register */
#define ETBPID4		0xfd0	/**< Peripheral ID4 register */
#define ETBPID5		0xfd4	/**< Peripheral ID5 register */
#define ETBPID6		0xfd8	/**< Peripheral ID6 register */
#define ETBPID7		0xfdc	/**< Peripheral ID7 register */
#define ETBPID0		0xfe0	/**< Peripheral ID0 register */
#define ETBPID1		0xfe4	/**< Peripheral ID1 register */
#define ETBPID2		0xfe8	/**< Peripheral ID2 register */
#define ETBPID3		0xfec	/**< Peripheral ID3 register */
#define ETBCID0		0xff0	/**< Component ID0 register */
#define ETBCID1		0xff4	/**< Component ID1 register */
#define ETBCID2		0xff8	/**< Component ID2 register */
#define ETBCID3		0xffc	/**< Component ID3 register */

/** magic number which enables further write access to ETB */
#define ETBLA_UNLOCK_MAGIC	0xc5acce55

#define ETBFFCR_ENABLE_FORMAT		(1<<0) /**< enable formatting */
/** enable continuous formatting */
#define ETBFFCR_ENABLE_FORMAT_CONT	(1<<1)
/** manually generate a flush of the system */
#define ETBFFCR_MANNUL_FLUSH		(1<<6)

#endif

