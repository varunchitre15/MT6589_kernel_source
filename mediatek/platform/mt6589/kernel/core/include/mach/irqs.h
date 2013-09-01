#ifndef __IRQS_H__
#define __IRQS_H__

#include "mt_irq.h"

/*
 * Define constants.
 */
#define FIQ_START 0
#define CPU_BRINGUP_SGI 1
#define FIQ_SMP_CALL_SGI 13
#define FIQ_DBG_SGI 14
#define NR_IRQS NR_MT_IRQ_LINE
#define MT65xx_EDGE_SENSITIVE 0
#define MT65xx_LEVEL_SENSITIVE 1
#define MT65xx_POLARITY_LOW   0
#define MT65xx_POLARITY_HIGH  1

#if !defined(__ASSEMBLY__)

/*
 * Define data structures.
 */

enum {
    IRQ_MASK_HEADER = 0xF1F1F1F1, 
    IRQ_MASK_FOOTER = 0xF2F2F2F2
};

struct mtk_irq_mask
{
    unsigned int header;   /* for error checking */
    __u32 mask0;
    __u32 mask1;
    __u32 mask2;
    __u32 mask3;
    __u32 mask4;
    __u32 mask5;
    __u32 mask6;
    __u32 mask7;
    __u32 mask8;
    unsigned int footer;   /* for error checking */
};
typedef void (*fiq_isr_handler)(void *arg, void *regs, void *svc_sp);

/*
 * Define function prototypes.
 *  */
 extern void mt_init_irq(void);
 extern int mt_irq_is_active(const unsigned int irq);
 extern int request_fiq(int irq, fiq_isr_handler handler, unsigned long irq_flags, void *arg);
#if defined(CONFIG_FIQ_GLUE)
 extern void trigger_sw_irq(int irq);
 extern int mt_enable_fiq(int irq);
 extern int mt_disable_fiq(int irq);
#else
#define trigger_sw_irq(__irq) (0)
#define mt_enable_fiq(__irq) (0)
#define mt_disable_fiq(__irq) (0)
#endif
 extern void mt_enable_ppi(int irq);

extern void mt_irq_dump(void);

#if 0

/*
 * Define function prototypes.
 */

//extern void mt6589_irq_mask(struct irq_data *data);
//extern void mt6589_irq_unmask(struct irq_data *data);
//extern void mt6589_irq_ack(struct irq_data *data);
extern void mt6589_irq_set_sens(unsigned int irq, unsigned int sens);
extern void mt6589_irq_set_polarity(unsigned int irq, unsigned int polarity);
//extern int mt6589_irq_set_type(struct irq_data *data, unsigned int flow_type);
extern void mt6589_init_irq(void);
extern int mt6589_irq_is_active(const unsigned int irq);

/*
 * Define macros.
 */

#define mt65xx_irq_mask(l) mt6589_irq_mask((l))
#define mt65xx_irq_unmask(l) mt6589_irq_unmask((l))
#define mt65xx_irq_ack(l) mt6589_irq_ack((l))
#define mt65xx_irq_set_sens(l, s) mt6589_irq_set_sens((l), (s))
#define mt65xx_irq_set_polarity(l, s) mt6589_irq_set_polarity((l), (s))
#endif
#endif  /* !__ASSEMBLY__ */

#endif  /* !_IRQS_H__ */
