

@#include <asm/arch/hardware.h>

        .macro  addruart,rx
        mrc p15, 0, \rx, c1, c0
        tst \rx, #1                 @ MMU enabled?
        mov   \rx, #0x9000
        movteq   \rx, #0xC100       @ physical base address
        movtne   \rx, #0xF100       @ virtual base
        .endm

        .macro  senduart,rd,rx
        strb    \rd, [\rx, #0x0]
        .endm

        .macro  waituart,rd,rx
1001:   ldr \rd, [\rx, #0x14]    @ UART0 status
        tst \rd, #0x40            @ write allow
        beq 1001b
        .endm

        .macro  busyuart,rd,rx
1002:   ldr \rd, [\rx, #0x14]   @ UART0 status
        tst \rd, #0x40            @ uart full .eq. busy
        beq 1002b
        .endm
