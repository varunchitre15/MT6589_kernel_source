#include <machine/asm.h>
#include <machine/cpu-features.h>

ENTRY(__aeabi_idiv)
	cmp	r1, #0
	beq	Ldiv0
	.word 	0xe710f110
	bx      lr
Ldiv0:
	movgt	r0, #0x7fffffff
	movlt	r0, #0x80000000
	b	__aeabi_idiv0
END(__aeabi_idiv)

ENTRY(__aeabi_idivmod)
	cmp	r1, #0
	beq	Ldiv0
	mov	r2, r0
	.word 	0xe710f110
	mls     r1, r0, r1, r2
	bx	lr
END(__aeabi_idivmod)

ENTRY(__aeabi_uidiv)
	cmp	r1, #0
	beq	Luidiv0
	.word 	0xe730f110
	bx      lr
Luidiv0:
	movne	r0, #0xffffffff
	b	__aeabi_idiv0
END(__aeabi_uidiv)

ENTRY(__aeabi_uidivmod)
	cmp	r1, #0
	beq	Luidiv0
	mov	r2, r0
	.word 	0xe730f110
	mls     r1, r0, r1, r2
	bx	lr
END(__aeabi_uidivmod)
