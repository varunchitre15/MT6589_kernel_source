#ifndef __MT6575_UNCOMPRESS_H__
#define __MT6575_UNCOMPRESS_H__

#define MT6575_UART0_PHY_BASE 0x11006000

#define MT6575_UART0_LSR *(volatile unsigned char *)(MT6575_UART0_PHY_BASE+0x14)
#define MT6575_UART0_THR *(volatile unsigned char *)(MT6575_UART0_PHY_BASE+0x0)
#define MT6575_UART0_LCR *(volatile unsigned char *)(MT6575_UART0_PHY_BASE+0xc)
#define MT6575_UART0_DLL *(volatile unsigned char *)(MT6575_UART0_PHY_BASE+0x0)
#define MT6575_UART0_DLH *(volatile unsigned char *)(MT6575_UART0_PHY_BASE+0x4)
#define MT6575_UART0_FCR *(volatile unsigned char *)(MT6575_UART0_PHY_BASE+0x8)
#define MT6575_UART0_MCR *(volatile unsigned char *)(MT6575_UART0_PHY_BASE+0x10)
#define MT6575_UART0_SPEED *(volatile unsigned char *)(MT6575_UART0_PHY_BASE+0x24)


static void arch_decomp_setup(void)
{
    unsigned char tmp;

#if defined(CONFIG_MT6575_FPGA)
        MT6575_UART0_LCR = 0x3;
        tmp = MT6575_UART0_LCR;
        MT6575_UART0_LCR = (tmp | 0x80);
        MT6575_UART0_SPEED = 0x0;
        MT6575_UART0_DLL = 0x0E;
        MT6575_UART0_DLH = 0;
        MT6575_UART0_LCR = tmp;
        MT6575_UART0_FCR = 0x0047;
        MT6575_UART0_MCR = (0x1 | 0x2);
#endif
}

/*
 * This does not append a newline
 */
static inline void putc(int c)
{
    while (!(MT6575_UART0_LSR & 0x20));    
    MT6575_UART0_THR = c;        
}

static inline void flush(void)
{
}

/*
 * nothing to do
 */
#define arch_decomp_wdog()

#endif /* !__MT6575_UNCOMPRESS_H__ */

