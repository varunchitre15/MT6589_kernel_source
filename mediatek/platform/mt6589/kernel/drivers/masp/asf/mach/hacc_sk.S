	.arch armv7-a
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 2
	.eabi_attribute 18, 4
	.file	"hacc_sk.c"
	.text
.Ltext0:
	.align	2
	.global	hacc_set_key
	.type	hacc_set_key, %function
hacc_set_key:
.LFB184:
	.file 1 "alps/kernel/mediatek/platform/mt6589/kernel/drivers/masp/asf/mach/hacc_sk.c"
	.loc 1 146 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL0:
	mov	ip, sp
.LCFI0:
	stmfd	sp!, {r4, r5, fp, ip, lr, pc}
.LCFI1:
	sub	fp, ip, #4
.LCFI2:
.LVL1:
	.loc 1 151 0
	cmp	r1, #24
	beq	.L11
	cmp	r1, #32
	beq	.L5
	cmp	r1, #16
	.loc 1 154 0
	moveq	r4, #0
	.loc 1 151 0
	beq	.L4
	.loc 1 163 0
	movw	r0, #24578
.LVL2:
	.loc 1 203 0
	ldmfd	sp, {r4, r5, fp, sp, pc}
.LVL3:
.L11:
	.loc 1 157 0
	mov	r4, #16
.LVL4:
.L4:
	.loc 1 166 0
	ldr	ip, .L16
	.loc 1 169 0
	mov	r2, #40960
	movt	r2, 61440
	.loc 1 174 0
	mov	r3, #0
	.loc 1 177 0
	cmp	r0, #1
	.loc 1 166 0
	str	r1, [ip, #16]
	.loc 1 169 0
	ldr	r1, [r2, #4]
.LVL5:
	.loc 1 181 0
	moveq	r0, r3
.LVL6:
	.loc 1 169 0
	bic	r1, r1, #48
	str	r1, [r2, #4]
	.loc 1 170 0
	ldr	r1, [r2, #4]
	orr	r1, r4, r1
	str	r1, [r2, #4]
.LVL7:
	.loc 1 174 0
	str	r3, [r2, #32]
.LVL8:
	str	r3, [r2, #36]
.LVL9:
	str	r3, [r2, #40]
.LVL10:
	str	r3, [r2, #44]
.LVL11:
	str	r3, [r2, #48]
.LVL12:
	str	r3, [r2, #52]
.LVL13:
	str	r3, [r2, #56]
.LVL14:
	str	r3, [r2, #60]
.LVL15:
	.loc 1 180 0
	ldr	r1, [r2, #12]
	orreq	r3, r1, #16
	streq	r3, [r2, #12]
	.loc 1 177 0
	ldmeqfd	sp, {r4, r5, fp, sp, pc}
.LVL16:
	.loc 1 192 0
	cmp	r0, #2
	movw	r3, #40992
	.loc 1 195 0 discriminator 1
	movw	r5, #41024
	.loc 1 192 0
	addeq	r0, ip, #52
.LVL17:
	addne	r0, ip, #20
	movt	r3, 61440
	.loc 1 195 0 discriminator 1
	movt	r5, 61440
	.loc 1 192 0
	bic	r1, r1, #16
	str	r1, [r2, #12]
.LVL18:
.L9:
	.loc 1 197 0
	ldrb	r2, [r0, #1]	@ zero_extendqisi2
	ldrb	r4, [r0, #0]	@ zero_extendqisi2
	.loc 1 198 0
	ldrb	ip, [r0, #3]	@ zero_extendqisi2
	ldrb	r1, [r0, #2]	@ zero_extendqisi2
	.loc 1 197 0
	mov	r2, r2, asl #16
	orr	r2, r2, r4, asl #24
	.loc 1 199 0
	add	r0, r0, #4
	.loc 1 197 0
	orr	r2, r2, ip
	.loc 1 198 0
	orr	r2, r2, r1, asl #8
.LVL19:
	.loc 1 199 0
	str	r2, [r3], #4
	.loc 1 195 0 discriminator 1
	cmp	r3, r5
	bne	.L9
	.loc 1 202 0
	mov	r0, #0
	ldmfd	sp, {r4, r5, fp, sp, pc}
.LVL20:
.L5:
	.loc 1 160 0
	mov	r4, r1
	.loc 1 161 0
	b	.L4
.L17:
	.align	2
.L16:
	.word	.LANCHOR0
.LFE184:
	.size	hacc_set_key, .-hacc_set_key
	.align	2
	.global	hacc_do_aes
	.type	hacc_do_aes, %function
hacc_do_aes:
.LFB185:
	.loc 1 206 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL21:
	mov	ip, sp
.LCFI3:
	stmfd	sp!, {r3, r4, fp, ip, lr, pc}
.LCFI4:
	sub	fp, ip, #4
.LCFI5:
	.loc 1 211 0
	tst	r3, #15
	bne	.L28
.LVL22:
	.loc 1 220 0
	mov	ip, #40960
	movt	ip, 61440
	.loc 1 225 0
	ldr	r4, .L29
	.loc 1 228 0
	cmp	r0, #1
	.loc 1 220 0
	ldr	r0, [ip, #8]
.LVL23:
	orr	r0, r0, #2
	str	r0, [ip, #8]
.LVL24:
	.loc 1 225 0
	ldr	r0, [r4, #0]
	str	r0, [ip, #64]
.LVL25:
	ldr	r0, [r4, #4]
	str	r0, [ip, #68]
.LVL26:
	ldr	r0, [r4, #8]
	str	r0, [ip, #72]
.LVL27:
	ldr	r0, [r4, #12]
	str	r0, [ip, #76]
.LVL28:
	.loc 1 230 0
	ldr	r0, [ip, #4]
	orreq	r0, r0, #1
	.loc 1 234 0
	bicne	r0, r0, #1
	str	r0, [ip, #4]
	.loc 1 244 0
	mov	ip, #40960
	movt	ip, 61440
	.loc 1 250 0 discriminator 1
	mov	r4, ip
.LVL29:
.L24:
	.loc 1 244 0
	ldr	r0, [r1, #0]
	str	r0, [ip, #16]
.LVL30:
	ldr	r0, [r1, #4]
	str	r0, [ip, #20]
.LVL31:
	ldr	r0, [r1, #8]
	str	r0, [ip, #24]
.LVL32:
	ldr	r0, [r1, #12]
	add	r1, r1, #16
.LVL33:
	str	r0, [ip, #28]
.LVL34:
	.loc 1 247 0
	ldr	r0, [ip, #8]
	orr	r0, r0, #1
	str	r0, [ip, #8]
.L23:
	.loc 1 250 0 discriminator 1
	ldr	r0, [ip, #8]
	tst	r0, #32768
	beq	.L23
	.loc 1 254 0
	ldr	r0, [r4, #80]
	.loc 1 256 0
	cmp	r3, #0
	.loc 1 254 0
	str	r0, [r2, #0]
.LVL35:
	ldr	r0, [r4, #84]
	str	r0, [r2, #4]
.LVL36:
	ldr	r0, [r4, #88]
	str	r0, [r2, #8]
.LVL37:
	ldr	r0, [r4, #92]
	str	r0, [r2, #12]
.LVL38:
	add	r2, r2, #16
.LVL39:
	.loc 1 256 0
	beq	.L25
.LVL40:
	.loc 1 261 0
	subs	r3, r3, #16
.LVL41:
	bne	.L24
	.loc 1 265 0
	mov	r0, r3
	ldmfd	sp, {r3, r4, fp, sp, pc}
.L25:
.L20:
	mov	r0, r3
	.loc 1 266 0
	ldmfd	sp, {r3, r4, fp, sp, pc}
.LVL42:
.L28:
	.loc 1 213 0
	mov	r2, r3
.LVL43:
	ldr	r0, .L29+4
.LVL44:
	ldr	r1, .L29+8
.LVL45:
	mov	r3, #16
.LVL46:
	bl	printk
.LVL47:
	.loc 1 214 0
	movw	r0, #24579
	ldmfd	sp, {r3, r4, fp, sp, pc}
.L30:
	.align	2
.L29:
	.word	.LANCHOR0
	.word	.LC0
	.word	.LC1
.LFE185:
	.size	hacc_do_aes, .-hacc_do_aes
	.align	2
	.global	hacc_init
	.type	hacc_init, %function
hacc_init:
.LFB186:
	.loc 1 269 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL48:
	mov	ip, sp
.LCFI6:
	stmfd	sp!, {r4, r5, fp, ip, lr, pc}
.LCFI7:
	sub	fp, ip, #4
.LCFI8:
.LVL49:
.LBB10:
.LBB11:
	.loc 1 340 0
	mov	r3, #40960
	movt	r3, 61440
.LBE11:
.LBE10:
.LBB14:
	.loc 1 287 0
	ldr	r4, .L36
.LBE14:
	.loc 1 269 0
	mov	r5, r0
.LBB15:
.LBB12:
	.loc 1 340 0
	ldr	r2, [r3, #8]
.LBE12:
.LBE15:
.LBB16:
	.loc 1 287 0 discriminator 3
	mov	r1, #84
.LBE16:
.LBB17:
.LBB13:
	.loc 1 340 0
	orr	r2, r2, #2
	str	r2, [r3, #8]
.LBE13:
.LBE17:
	.loc 1 280 0
	ldr	r2, [r3, #8]
.LBB18:
	.loc 1 287 0 discriminator 3
	mov	r0, r4
.LVL50:
.LBE18:
	.loc 1 280 0
	orr	r2, r2, #2
	str	r2, [r3, #8]
	.loc 1 283 0
	ldr	r2, [r3, #8]
	bic	r2, r2, #4096
	bic	r2, r2, #2
	str	r2, [r3, #8]
	.loc 1 284 0
	ldr	r2, [r3, #8]
	orr	r2, r2, #2
	str	r2, [r3, #8]
.LVL51:
.LBB19:
	.loc 1 287 0 discriminator 3
	bl	__memzero
.LBE19:
	.loc 1 289 0 discriminator 1
	ldr	r3, [r5, #0]
	cmp	r3, #0
	beq	.L32
	add	r2, r4, #19
	.loc 1 289 0 is_stmt 0
	mov	r3, #0
.LVL52:
.L33:
	.loc 1 268 0 is_stmt 1
	add	r1, r5, r3
	.loc 1 289 0 discriminator 2
	add	r3, r3, #1
.LVL53:
	.loc 1 291 0
	ldrb	r1, [r1, #4]	@ zero_extendqisi2
	strb	r1, [r2, #1]!
	.loc 1 289 0 discriminator 1
	ldr	r1, [r5, #0]
	cmp	r1, r3
	bhi	.L33
.LVL54:
.L32:
.LBB20:
.LBB21:
	.loc 1 127 0
	mov	ip, #40960
	movt	ip, 61440
.LBE21:
.LBE20:
	.loc 1 296 0
	movw	r3, #12887
	.loc 1 297 0
	movw	r2, #21522
	.loc 1 296 0
	movt	r3, 23130
	.loc 1 297 0
	movt	r2, 26263
	.loc 1 296 0
	str	r3, [r4, #0]
.LVL55:
	.loc 1 310 0
	mov	r0, #1
	.loc 1 297 0
	str	r2, [r4, #4]
.LVL56:
	.loc 1 310 0
	mov	r1, #16
	.loc 1 298 0
	str	r2, [r4, #8]
.LVL57:
	.loc 1 299 0
	str	r3, [r4, #12]
.LVL58:
.LBB23:
.LBB22:
	.loc 1 127 0
	ldr	r3, [ip, #4]
	bic	r3, r3, #2
	str	r3, [ip, #4]
	.loc 1 136 0
	ldr	r3, [ip, #4]
	orr	r3, r3, #2
	str	r3, [ip, #4]
.LBE22:
.LBE23:
	.loc 1 310 0
	bl	hacc_set_key
.LVL59:
	.loc 1 311 0
	cmp	r0, #0
	ldmnefd	sp, {r4, r5, fp, sp, pc}
	.loc 1 314 0
	ldr	r1, .L36+4
	mov	r3, #32
	mov	r0, #1
.LVL60:
	add	r2, r1, r3
	bl	hacc_do_aes
.LVL61:
	.loc 1 315 0
	cmp	r0, #0
	ldmnefd	sp, {r4, r5, fp, sp, pc}
	.loc 1 318 0
	mov	r0, #2
.LVL62:
	mov	r1, #32
	bl	hacc_set_key
.LVL63:
	.loc 1 319 0
	cmp	r0, #0
	ldmnefd	sp, {r4, r5, fp, sp, pc}
.L34:
	.loc 1 325 0
	ldr	r2, .L36+8
	mov	r3, #1
	strb	r3, [r2, #0]
	.loc 1 328 0
	ldr	r2, .L36+12
	strb	r3, [r2, #0]
	.loc 1 333 0
	ldmfd	sp, {r4, r5, fp, sp, pc}
.L37:
	.align	2
.L36:
	.word	.LANCHOR0
	.word	.LANCHOR0+20
	.word	bHACC_HWWrapKeyInit
	.word	bHACC_SWKeyInit
.LFE186:
	.size	hacc_init, .-hacc_init
	.align	2
	.global	hacc_deinit
	.type	hacc_deinit, %function
hacc_deinit:
.LFB187:
	.loc 1 336 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
.LCFI9:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI10:
	sub	fp, ip, #4
.LCFI11:
.LVL64:
	.loc 1 340 0
	mov	r3, #40960
	movt	r3, 61440
	.loc 1 343 0
	mov	r0, #0
	.loc 1 340 0
	ldr	r2, [r3, #8]
	orr	r2, r2, #2
	str	r2, [r3, #8]
	.loc 1 343 0
	ldmfd	sp, {fp, sp, pc}
.LFE187:
	.size	hacc_deinit, .-hacc_deinit
	.section	.rodata.str1.4,"aMS",%progbits,1
	.align	2
.LC0:
	.ascii	"[%s] size = %d is not %d bytes alignment\012\000"
	.space	2
.LC1:
	.ascii	"HACC\000"
	.bss
	.align	2
.LANCHOR0 = . + 0
	.type	hacc_ctx, %object
	.size	hacc_ctx, 84
hacc_ctx:
	.space	84
	.section	.debug_frame,"",%progbits
.Lframe0:
	.4byte	.LECIE0-.LSCIE0
.LSCIE0:
	.4byte	0xffffffff
	.byte	0x1
	.ascii	"\000"
	.uleb128 0x1
	.sleb128 -4
	.byte	0xe
	.byte	0xc
	.uleb128 0xd
	.uleb128 0
	.align	2
.LECIE0:
.LSFDE0:
	.4byte	.LEFDE0-.LASFDE0
.LASFDE0:
	.4byte	.Lframe0
	.4byte	.LFB184
	.4byte	.LFE184-.LFB184
	.byte	0x4
	.4byte	.LCFI0-.LFB184
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI1-.LCFI0
	.byte	0x8e
	.uleb128 0x2
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8b
	.uleb128 0x4
	.byte	0x85
	.uleb128 0x5
	.byte	0x84
	.uleb128 0x6
	.byte	0x4
	.4byte	.LCFI2-.LCFI1
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE0:
.LSFDE2:
	.4byte	.LEFDE2-.LASFDE2
.LASFDE2:
	.4byte	.Lframe0
	.4byte	.LFB185
	.4byte	.LFE185-.LFB185
	.byte	0x4
	.4byte	.LCFI3-.LFB185
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI4-.LCFI3
	.byte	0x8e
	.uleb128 0x2
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8b
	.uleb128 0x4
	.byte	0x84
	.uleb128 0x5
	.byte	0x83
	.uleb128 0x6
	.byte	0x4
	.4byte	.LCFI5-.LCFI4
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE2:
.LSFDE4:
	.4byte	.LEFDE4-.LASFDE4
.LASFDE4:
	.4byte	.Lframe0
	.4byte	.LFB186
	.4byte	.LFE186-.LFB186
	.byte	0x4
	.4byte	.LCFI6-.LFB186
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI7-.LCFI6
	.byte	0x8e
	.uleb128 0x2
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8b
	.uleb128 0x4
	.byte	0x85
	.uleb128 0x5
	.byte	0x84
	.uleb128 0x6
	.byte	0x4
	.4byte	.LCFI8-.LCFI7
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE4:
.LSFDE6:
	.4byte	.LEFDE6-.LASFDE6
.LASFDE6:
	.4byte	.Lframe0
	.4byte	.LFB187
	.4byte	.LFE187-.LFB187
	.byte	0x4
	.4byte	.LCFI9-.LFB187
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI10-.LCFI9
	.byte	0x8e
	.uleb128 0x2
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8b
	.uleb128 0x4
	.byte	0x4
	.4byte	.LCFI11-.LCFI10
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE6:
	.text
.Letext0:
	.file 2 "alps/kernel/include/asm-generic/posix_types.h"
	.file 3 "alps/kernel/include/linux/types.h"
	.file 4 "alps/kernel/../mediatek/platform/mt6589/kernel/core/include/mach/mt_typedefs.h"
	.file 5 "alps/kernel/mediatek/platform/mt6589/kernel/drivers/masp/asf/inc/hacc_def.h"
	.file 6 "alps/kernel/arch/arm/include/asm/hwcap.h"
	.file 7 "alps/kernel/include/linux/printk.h"
	.file 8 "alps/kernel/include/linux/kernel.h"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x512
	.2byte	0x2
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.LASF55
	.byte	0x1
	.4byte	.LASF56
	.4byte	.LASF57
	.4byte	.Ltext0
	.4byte	.Letext0
	.4byte	.Ldebug_line0
	.4byte	.Ldebug_pubnames0
	.4byte	.Ldebug_pubtypes0
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF0
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.4byte	.LASF1
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF2
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF3
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF4
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF5
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.4byte	.LASF6
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.ascii	"int\000"
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF7
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF8
	.uleb128 0x2
	.byte	0x4
	.byte	0x5
	.4byte	.LASF9
	.uleb128 0x4
	.4byte	.LASF11
	.byte	0x2
	.byte	0x47
	.4byte	0x34
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF10
	.uleb128 0x4
	.4byte	.LASF12
	.byte	0x3
	.byte	0x3f
	.4byte	0x7a
	.uleb128 0x5
	.byte	0x4
	.uleb128 0x6
	.ascii	"U8\000"
	.byte	0x4
	.byte	0x4d
	.4byte	0x49
	.uleb128 0x6
	.ascii	"U32\000"
	.byte	0x4
	.byte	0x51
	.4byte	0x34
	.uleb128 0x4
	.4byte	.LASF13
	.byte	0x4
	.byte	0x59
	.4byte	0x34
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF14
	.uleb128 0x4
	.4byte	.LASF15
	.byte	0x4
	.byte	0x8b
	.4byte	0x49
	.uleb128 0x7
	.byte	0x4
	.byte	0x5
	.byte	0x5a
	.4byte	0xe0
	.uleb128 0x8
	.4byte	.LASF16
	.sleb128 0
	.uleb128 0x8
	.4byte	.LASF17
	.sleb128 1
	.byte	0
	.uleb128 0x4
	.4byte	.LASF18
	.byte	0x5
	.byte	0x5d
	.4byte	0xcb
	.uleb128 0x7
	.byte	0x4
	.byte	0x5
	.byte	0x60
	.4byte	0x100
	.uleb128 0x8
	.4byte	.LASF19
	.sleb128 0
	.uleb128 0x8
	.4byte	.LASF20
	.sleb128 1
	.byte	0
	.uleb128 0x4
	.4byte	.LASF21
	.byte	0x5
	.byte	0x63
	.4byte	0xeb
	.uleb128 0x7
	.byte	0x4
	.byte	0x5
	.byte	0x66
	.4byte	0x126
	.uleb128 0x8
	.4byte	.LASF22
	.sleb128 16
	.uleb128 0x8
	.4byte	.LASF23
	.sleb128 24
	.uleb128 0x8
	.4byte	.LASF24
	.sleb128 32
	.byte	0
	.uleb128 0x4
	.4byte	.LASF25
	.byte	0x5
	.byte	0x6a
	.4byte	0x10b
	.uleb128 0x7
	.byte	0x4
	.byte	0x5
	.byte	0x6d
	.4byte	0x14c
	.uleb128 0x8
	.4byte	.LASF26
	.sleb128 0
	.uleb128 0x8
	.4byte	.LASF27
	.sleb128 1
	.uleb128 0x8
	.4byte	.LASF28
	.sleb128 2
	.byte	0
	.uleb128 0x4
	.4byte	.LASF29
	.byte	0x5
	.byte	0x71
	.4byte	0x131
	.uleb128 0x9
	.byte	0x10
	.byte	0x5
	.byte	0x73
	.4byte	0x16e
	.uleb128 0xa
	.4byte	.LASF31
	.byte	0x5
	.byte	0x74
	.4byte	0x16e
	.byte	0x2
	.byte	0x23
	.uleb128 0
	.byte	0
	.uleb128 0xb
	.4byte	0x99
	.4byte	0x17e
	.uleb128 0xc
	.4byte	0x34
	.byte	0xf
	.byte	0
	.uleb128 0x4
	.4byte	.LASF30
	.byte	0x5
	.byte	0x75
	.4byte	0x157
	.uleb128 0x9
	.byte	0x24
	.byte	0x5
	.byte	0x77
	.4byte	0x1ae
	.uleb128 0xa
	.4byte	.LASF32
	.byte	0x5
	.byte	0x78
	.4byte	0xa3
	.byte	0x2
	.byte	0x23
	.uleb128 0
	.uleb128 0xa
	.4byte	.LASF33
	.byte	0x5
	.byte	0x79
	.4byte	0x1ae
	.byte	0x2
	.byte	0x23
	.uleb128 0x4
	.byte	0
	.uleb128 0xb
	.4byte	0x99
	.4byte	0x1be
	.uleb128 0xc
	.4byte	0x34
	.byte	0x1f
	.byte	0
	.uleb128 0x4
	.4byte	.LASF34
	.byte	0x5
	.byte	0x7a
	.4byte	0x189
	.uleb128 0xd
	.4byte	.LASF58
	.byte	0x54
	.byte	0x5
	.byte	0x7c
	.4byte	0x20e
	.uleb128 0xe
	.ascii	"cfg\000"
	.byte	0x5
	.byte	0x7d
	.4byte	0x17e
	.byte	0x2
	.byte	0x23
	.uleb128 0
	.uleb128 0xa
	.4byte	.LASF35
	.byte	0x5
	.byte	0x7e
	.4byte	0xa3
	.byte	0x2
	.byte	0x23
	.uleb128 0x10
	.uleb128 0xa
	.4byte	.LASF36
	.byte	0x5
	.byte	0x7f
	.4byte	0x1ae
	.byte	0x2
	.byte	0x23
	.uleb128 0x14
	.uleb128 0xa
	.4byte	.LASF37
	.byte	0x5
	.byte	0x80
	.4byte	0x1ae
	.byte	0x2
	.byte	0x23
	.uleb128 0x34
	.byte	0
	.uleb128 0xf
	.byte	0x1
	.4byte	.LASF59
	.byte	0x1
	.2byte	0x14f
	.byte	0x1
	.4byte	0xa3
	.byte	0x1
	.4byte	0x22e
	.uleb128 0x10
	.ascii	"ret\000"
	.byte	0x1
	.2byte	0x151
	.4byte	0xa3
	.byte	0
	.uleb128 0x11
	.4byte	.LASF38
	.byte	0x1
	.byte	0x75
	.byte	0x1
	.4byte	0xa3
	.byte	0x1
	.4byte	0x24b
	.uleb128 0x12
	.ascii	"cfg\000"
	.byte	0x1
	.byte	0x75
	.4byte	0x24b
	.byte	0
	.uleb128 0x13
	.byte	0x4
	.4byte	0x17e
	.uleb128 0x11
	.4byte	.LASF39
	.byte	0x1
	.byte	0x7b
	.byte	0x1
	.4byte	0xa3
	.byte	0x1
	.4byte	0x291
	.uleb128 0x14
	.4byte	.LASF40
	.byte	0x1
	.byte	0x7b
	.4byte	0xe0
	.uleb128 0x15
	.ascii	"cfg\000"
	.byte	0x1
	.byte	0x7d
	.4byte	0x17e
	.uleb128 0x16
	.uleb128 0x15
	.ascii	"__p\000"
	.byte	0x1
	.byte	0x84
	.4byte	0x97
	.uleb128 0x15
	.ascii	"__n\000"
	.byte	0x1
	.byte	0x84
	.4byte	0x8c
	.byte	0
	.byte	0
	.uleb128 0x17
	.byte	0x1
	.4byte	.LASF44
	.byte	0x1
	.byte	0x91
	.byte	0x1
	.4byte	0xa3
	.4byte	.LFB184
	.4byte	.LFE184
	.4byte	.LLST0
	.4byte	0x306
	.uleb128 0x18
	.ascii	"id\000"
	.byte	0x1
	.byte	0x91
	.4byte	0x14c
	.4byte	.LLST1
	.uleb128 0x18
	.ascii	"key\000"
	.byte	0x1
	.byte	0x91
	.4byte	0x126
	.4byte	.LLST2
	.uleb128 0x19
	.ascii	"i\000"
	.byte	0x1
	.byte	0x93
	.4byte	0xa3
	.4byte	.LLST3
	.uleb128 0x1a
	.4byte	.LASF41
	.byte	0x1
	.byte	0x93
	.4byte	0xa3
	.4byte	.LLST4
	.uleb128 0x1a
	.4byte	.LASF42
	.byte	0x1
	.byte	0x94
	.4byte	0xa3
	.4byte	.LLST5
	.uleb128 0x1a
	.4byte	.LASF43
	.byte	0x1
	.byte	0x95
	.4byte	0x306
	.4byte	.LLST6
	.byte	0
	.uleb128 0x13
	.byte	0x4
	.4byte	0x99
	.uleb128 0x17
	.byte	0x1
	.4byte	.LASF45
	.byte	0x1
	.byte	0xcd
	.byte	0x1
	.4byte	0xa3
	.4byte	.LFB185
	.4byte	.LFE185
	.4byte	.LLST7
	.4byte	0x3a9
	.uleb128 0x18
	.ascii	"ops\000"
	.byte	0x1
	.byte	0xcd
	.4byte	0x100
	.4byte	.LLST8
	.uleb128 0x18
	.ascii	"src\000"
	.byte	0x1
	.byte	0xcd
	.4byte	0x306
	.4byte	.LLST9
	.uleb128 0x18
	.ascii	"dst\000"
	.byte	0x1
	.byte	0xcd
	.4byte	0x306
	.4byte	.LLST10
	.uleb128 0x1b
	.4byte	.LASF32
	.byte	0x1
	.byte	0xcd
	.4byte	0xa3
	.4byte	.LLST11
	.uleb128 0x19
	.ascii	"i\000"
	.byte	0x1
	.byte	0xcf
	.4byte	0xa3
	.4byte	.LLST12
	.uleb128 0x19
	.ascii	"ds\000"
	.byte	0x1
	.byte	0xd0
	.4byte	0x3a9
	.4byte	.LLST13
	.uleb128 0x19
	.ascii	"dt\000"
	.byte	0x1
	.byte	0xd0
	.4byte	0x3a9
	.4byte	.LLST14
	.uleb128 0x19
	.ascii	"vt\000"
	.byte	0x1
	.byte	0xd0
	.4byte	0x3a9
	.4byte	.LLST15
	.uleb128 0x1c
	.4byte	.LASF48
	.byte	0x1
	.2byte	0x107
	.4byte	.L20
	.byte	0
	.uleb128 0x13
	.byte	0x4
	.4byte	0xa3
	.uleb128 0x1d
	.byte	0x1
	.4byte	.LASF46
	.byte	0x1
	.2byte	0x10c
	.byte	0x1
	.4byte	0xa3
	.4byte	.LFB186
	.4byte	.LFE186
	.4byte	.LLST16
	.4byte	0x485
	.uleb128 0x1e
	.4byte	.LASF47
	.byte	0x1
	.2byte	0x10c
	.4byte	0x485
	.4byte	.LLST17
	.uleb128 0x1f
	.ascii	"i\000"
	.byte	0x1
	.2byte	0x10e
	.4byte	0xa3
	.4byte	.LLST18
	.uleb128 0x20
	.4byte	.LASF31
	.byte	0x1
	.2byte	0x10f
	.4byte	0x3a9
	.4byte	.LLST19
	.uleb128 0x1f
	.ascii	"ret\000"
	.byte	0x1
	.2byte	0x110
	.4byte	0xa3
	.4byte	.LLST20
	.uleb128 0x1c
	.4byte	.LASF48
	.byte	0x1
	.2byte	0x14a
	.4byte	.L34
	.uleb128 0x21
	.4byte	0x20e
	.4byte	.LBB10
	.4byte	.Ldebug_ranges0+0
	.byte	0x1
	.2byte	0x112
	.4byte	0x438
	.uleb128 0x22
	.4byte	.Ldebug_ranges0+0x20
	.uleb128 0x23
	.4byte	0x221
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x24
	.4byte	.Ldebug_ranges0+0x40
	.4byte	0x462
	.uleb128 0x25
	.ascii	"__p\000"
	.byte	0x1
	.2byte	0x11f
	.4byte	0x97
	.byte	0x6
	.byte	0x3
	.4byte	hacc_ctx
	.byte	0x9f
	.uleb128 0x26
	.ascii	"__n\000"
	.byte	0x1
	.2byte	0x11f
	.4byte	0x8c
	.byte	0x54
	.byte	0
	.uleb128 0x27
	.4byte	0x251
	.4byte	.LBB20
	.4byte	.Ldebug_ranges0+0x68
	.byte	0x1
	.2byte	0x131
	.uleb128 0x28
	.4byte	0x262
	.byte	0x1
	.uleb128 0x22
	.4byte	.Ldebug_ranges0+0x80
	.uleb128 0x29
	.4byte	0x26d
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x13
	.byte	0x4
	.4byte	0x1be
	.uleb128 0x2a
	.4byte	0x20e
	.4byte	.LFB187
	.4byte	.LFE187
	.4byte	.LLST21
	.4byte	0x4a7
	.uleb128 0x23
	.4byte	0x221
	.byte	0
	.byte	0
	.uleb128 0x2b
	.4byte	.LASF49
	.byte	0x1
	.byte	0x4d
	.4byte	0x1c9
	.byte	0x5
	.byte	0x3
	.4byte	hacc_ctx
	.uleb128 0x2c
	.4byte	.LASF50
	.byte	0x6
	.byte	0x23
	.4byte	0x34
	.byte	0x1
	.byte	0x1
	.uleb128 0xb
	.4byte	0x5e
	.4byte	0x4d0
	.uleb128 0x2d
	.byte	0
	.uleb128 0x2c
	.4byte	.LASF51
	.byte	0x7
	.byte	0x1b
	.4byte	0x4c5
	.byte	0x1
	.byte	0x1
	.uleb128 0xb
	.4byte	0x3b
	.4byte	0x4e8
	.uleb128 0x2d
	.byte	0
	.uleb128 0x2e
	.4byte	.LASF52
	.byte	0x8
	.2byte	0x18a
	.4byte	0x4f6
	.byte	0x1
	.byte	0x1
	.uleb128 0x2f
	.4byte	0x4dd
	.uleb128 0x2c
	.4byte	.LASF53
	.byte	0x1
	.byte	0x47
	.4byte	0xc0
	.byte	0x1
	.byte	0x1
	.uleb128 0x2c
	.4byte	.LASF54
	.byte	0x1
	.byte	0x48
	.4byte	0xc0
	.byte	0x1
	.byte	0x1
	.byte	0
	.section	.debug_abbrev,"",%progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x6
	.uleb128 0x2134
	.uleb128 0x6
	.uleb128 0x2135
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x4
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xd
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x16
	.uleb128 0xb
	.byte	0x1
	.byte	0
	.byte	0
	.uleb128 0x17
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x18
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x19
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x1a
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x1b
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x1c
	.uleb128 0xa
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x11
	.uleb128 0x1
	.byte	0
	.byte	0
	.uleb128 0x1d
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0xc
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1e
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x1f
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x20
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x21
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x52
	.uleb128 0x1
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x22
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x55
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0x23
	.uleb128 0x34
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x24
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x25
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0x26
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x27
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x52
	.uleb128 0x1
	.uleb128 0x55
	.uleb128 0x6
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0x5
	.byte	0
	.byte	0
	.uleb128 0x28
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x29
	.uleb128 0x34
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x2a
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x2b
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0x2c
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3c
	.uleb128 0xc
	.byte	0
	.byte	0
	.uleb128 0x2d
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x2e
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0xc
	.uleb128 0x3c
	.uleb128 0xc
	.byte	0
	.byte	0
	.uleb128 0x2f
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",%progbits
.Ldebug_loc0:
.LLST0:
	.4byte	.LFB184-.Ltext0
	.4byte	.LCFI0-.Ltext0
	.2byte	0x2
	.byte	0x7d
	.sleb128 0
	.4byte	.LCFI0-.Ltext0
	.4byte	.LCFI2-.Ltext0
	.2byte	0x2
	.byte	0x7c
	.sleb128 0
	.4byte	.LCFI2-.Ltext0
	.4byte	.LFE184-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 4
	.4byte	0
	.4byte	0
.LLST1:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL2-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL3-.Ltext0
	.4byte	.LVL6-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL20-.Ltext0
	.4byte	.LFE184-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST2:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL5-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	.LVL5-.Ltext0
	.4byte	.LVL20-.Ltext0
	.2byte	0x5
	.byte	0x3
	.4byte	hacc_ctx+16
	.4byte	.LVL20-.Ltext0
	.4byte	.LFE184-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	0
	.4byte	0
.LLST3:
	.4byte	.LVL7-.Ltext0
	.4byte	.LVL8-.Ltext0
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL8-.Ltext0
	.4byte	.LVL9-.Ltext0
	.2byte	0x2
	.byte	0x34
	.byte	0x9f
	.4byte	.LVL9-.Ltext0
	.4byte	.LVL10-.Ltext0
	.2byte	0x2
	.byte	0x38
	.byte	0x9f
	.4byte	.LVL10-.Ltext0
	.4byte	.LVL11-.Ltext0
	.2byte	0x2
	.byte	0x3c
	.byte	0x9f
	.4byte	.LVL11-.Ltext0
	.4byte	.LVL12-.Ltext0
	.2byte	0x2
	.byte	0x40
	.byte	0x9f
	.4byte	.LVL12-.Ltext0
	.4byte	.LVL13-.Ltext0
	.2byte	0x2
	.byte	0x44
	.byte	0x9f
	.4byte	.LVL13-.Ltext0
	.4byte	.LVL14-.Ltext0
	.2byte	0x2
	.byte	0x48
	.byte	0x9f
	.4byte	.LVL14-.Ltext0
	.4byte	.LVL15-.Ltext0
	.2byte	0x2
	.byte	0x4c
	.byte	0x9f
	.4byte	.LVL15-.Ltext0
	.4byte	.LVL18-.Ltext0
	.2byte	0x3
	.byte	0x8
	.byte	0x20
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST4:
	.4byte	.LVL1-.Ltext0
	.4byte	.LVL4-.Ltext0
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL4-.Ltext0
	.4byte	.LVL18-.Ltext0
	.2byte	0x1
	.byte	0x54
	.4byte	.LVL20-.Ltext0
	.4byte	.LFE184-.Ltext0
	.2byte	0x3
	.byte	0x8
	.byte	0x20
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST5:
	.4byte	.LVL19-.Ltext0
	.4byte	.LVL20-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
.LLST6:
	.4byte	.LVL16-.Ltext0
	.4byte	.LVL17-.Ltext0
	.2byte	0xe
	.byte	0x7c
	.sleb128 52
	.byte	0x7c
	.sleb128 20
	.byte	0x70
	.sleb128 0
	.byte	0x32
	.byte	0x29
	.byte	0x28
	.2byte	0x1
	.byte	0x16
	.byte	0x13
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST7:
	.4byte	.LFB185-.Ltext0
	.4byte	.LCFI3-.Ltext0
	.2byte	0x2
	.byte	0x7d
	.sleb128 0
	.4byte	.LCFI3-.Ltext0
	.4byte	.LCFI5-.Ltext0
	.2byte	0x2
	.byte	0x7c
	.sleb128 0
	.4byte	.LCFI5-.Ltext0
	.4byte	.LFE185-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 4
	.4byte	0
	.4byte	0
.LLST8:
	.4byte	.LVL21-.Ltext0
	.4byte	.LVL23-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL42-.Ltext0
	.4byte	.LVL44-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST9:
	.4byte	.LVL21-.Ltext0
	.4byte	.LVL29-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	.LVL42-.Ltext0
	.4byte	.LVL45-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	0
	.4byte	0
.LLST10:
	.4byte	.LVL21-.Ltext0
	.4byte	.LVL29-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	.LVL42-.Ltext0
	.4byte	.LVL43-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
.LLST11:
	.4byte	.LVL21-.Ltext0
	.4byte	.LVL40-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	.LVL40-.Ltext0
	.4byte	.LVL41-.Ltext0
	.2byte	0x3
	.byte	0x73
	.sleb128 -16
	.byte	0x9f
	.4byte	.LVL41-.Ltext0
	.4byte	.LVL46-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	.LVL46-.Ltext0
	.4byte	.LVL47-1-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
.LLST12:
	.4byte	.LVL24-.Ltext0
	.4byte	.LVL25-.Ltext0
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL25-.Ltext0
	.4byte	.LVL26-.Ltext0
	.2byte	0x2
	.byte	0x34
	.byte	0x9f
	.4byte	.LVL26-.Ltext0
	.4byte	.LVL27-.Ltext0
	.2byte	0x2
	.byte	0x38
	.byte	0x9f
	.4byte	.LVL27-.Ltext0
	.4byte	.LVL28-.Ltext0
	.2byte	0x2
	.byte	0x3c
	.byte	0x9f
	.4byte	.LVL28-.Ltext0
	.4byte	.LVL29-.Ltext0
	.2byte	0x2
	.byte	0x40
	.byte	0x9f
	.4byte	.LVL29-.Ltext0
	.4byte	.LVL30-.Ltext0
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL30-.Ltext0
	.4byte	.LVL31-.Ltext0
	.2byte	0x2
	.byte	0x34
	.byte	0x9f
	.4byte	.LVL31-.Ltext0
	.4byte	.LVL32-.Ltext0
	.2byte	0x2
	.byte	0x38
	.byte	0x9f
	.4byte	.LVL32-.Ltext0
	.4byte	.LVL34-.Ltext0
	.2byte	0x2
	.byte	0x3c
	.byte	0x9f
	.4byte	.LVL34-.Ltext0
	.4byte	.LVL35-.Ltext0
	.2byte	0x2
	.byte	0x40
	.byte	0x9f
	.4byte	.LVL35-.Ltext0
	.4byte	.LVL36-.Ltext0
	.2byte	0x2
	.byte	0x34
	.byte	0x9f
	.4byte	.LVL36-.Ltext0
	.4byte	.LVL37-.Ltext0
	.2byte	0x2
	.byte	0x38
	.byte	0x9f
	.4byte	.LVL37-.Ltext0
	.4byte	.LVL38-.Ltext0
	.2byte	0x2
	.byte	0x3c
	.byte	0x9f
	.4byte	.LVL38-.Ltext0
	.4byte	.LVL42-.Ltext0
	.2byte	0x2
	.byte	0x40
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST13:
	.4byte	.LVL29-.Ltext0
	.4byte	.LVL30-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	.LVL30-.Ltext0
	.4byte	.LVL31-.Ltext0
	.2byte	0x3
	.byte	0x71
	.sleb128 4
	.byte	0x9f
	.4byte	.LVL31-.Ltext0
	.4byte	.LVL32-.Ltext0
	.2byte	0x3
	.byte	0x71
	.sleb128 8
	.byte	0x9f
	.4byte	.LVL32-.Ltext0
	.4byte	.LVL33-.Ltext0
	.2byte	0x3
	.byte	0x71
	.sleb128 12
	.byte	0x9f
	.4byte	.LVL33-.Ltext0
	.4byte	.LVL34-.Ltext0
	.2byte	0x3
	.byte	0x71
	.sleb128 -4
	.byte	0x9f
	.4byte	.LVL34-.Ltext0
	.4byte	.LVL42-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	0
	.4byte	0
.LLST14:
	.4byte	.LVL29-.Ltext0
	.4byte	.LVL35-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	.LVL35-.Ltext0
	.4byte	.LVL36-.Ltext0
	.2byte	0x3
	.byte	0x72
	.sleb128 4
	.byte	0x9f
	.4byte	.LVL36-.Ltext0
	.4byte	.LVL37-.Ltext0
	.2byte	0x3
	.byte	0x72
	.sleb128 8
	.byte	0x9f
	.4byte	.LVL37-.Ltext0
	.4byte	.LVL38-.Ltext0
	.2byte	0x3
	.byte	0x72
	.sleb128 12
	.byte	0x9f
	.4byte	.LVL38-.Ltext0
	.4byte	.LVL39-.Ltext0
	.2byte	0x3
	.byte	0x72
	.sleb128 16
	.byte	0x9f
	.4byte	.LVL39-.Ltext0
	.4byte	.LVL42-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
.LLST15:
	.4byte	.LVL22-.Ltext0
	.4byte	.LVL25-.Ltext0
	.2byte	0x6
	.byte	0x3
	.4byte	hacc_ctx
	.byte	0x9f
	.4byte	.LVL25-.Ltext0
	.4byte	.LVL26-.Ltext0
	.2byte	0x6
	.byte	0x3
	.4byte	hacc_ctx+4
	.byte	0x9f
	.4byte	.LVL26-.Ltext0
	.4byte	.LVL27-.Ltext0
	.2byte	0x6
	.byte	0x3
	.4byte	hacc_ctx+8
	.byte	0x9f
	.4byte	.LVL27-.Ltext0
	.4byte	.LVL28-.Ltext0
	.2byte	0x6
	.byte	0x3
	.4byte	hacc_ctx+12
	.byte	0x9f
	.4byte	.LVL28-.Ltext0
	.4byte	.LVL42-.Ltext0
	.2byte	0x6
	.byte	0x3
	.4byte	hacc_ctx+16
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST16:
	.4byte	.LFB186-.Ltext0
	.4byte	.LCFI6-.Ltext0
	.2byte	0x2
	.byte	0x7d
	.sleb128 0
	.4byte	.LCFI6-.Ltext0
	.4byte	.LCFI8-.Ltext0
	.2byte	0x2
	.byte	0x7c
	.sleb128 0
	.4byte	.LCFI8-.Ltext0
	.4byte	.LFE186-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 4
	.4byte	0
	.4byte	0
.LLST17:
	.4byte	.LVL48-.Ltext0
	.4byte	.LVL50-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL50-.Ltext0
	.4byte	.LFE186-.Ltext0
	.2byte	0x1
	.byte	0x55
	.4byte	0
	.4byte	0
.LLST18:
	.4byte	.LVL49-.Ltext0
	.4byte	.LVL52-.Ltext0
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL53-.Ltext0
	.4byte	.LVL54-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	0
	.4byte	0
.LLST19:
	.4byte	.LVL54-.Ltext0
	.4byte	.LVL55-.Ltext0
	.2byte	0x6
	.byte	0x3
	.4byte	hacc_ctx
	.byte	0x9f
	.4byte	.LVL55-.Ltext0
	.4byte	.LVL56-.Ltext0
	.2byte	0x6
	.byte	0x3
	.4byte	hacc_ctx+4
	.byte	0x9f
	.4byte	.LVL56-.Ltext0
	.4byte	.LVL57-.Ltext0
	.2byte	0x6
	.byte	0x3
	.4byte	hacc_ctx+8
	.byte	0x9f
	.4byte	.LVL57-.Ltext0
	.4byte	.LFE186-.Ltext0
	.2byte	0x6
	.byte	0x3
	.4byte	hacc_ctx+12
	.byte	0x9f
	.4byte	0
	.4byte	0
.LLST20:
	.4byte	.LVL49-.Ltext0
	.4byte	.LVL58-.Ltext0
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL59-.Ltext0
	.4byte	.LVL60-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL61-.Ltext0
	.4byte	.LVL62-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL63-.Ltext0
	.4byte	.LFE186-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST21:
	.4byte	.LFB187-.Ltext0
	.4byte	.LCFI9-.Ltext0
	.2byte	0x2
	.byte	0x7d
	.sleb128 0
	.4byte	.LCFI9-.Ltext0
	.4byte	.LCFI11-.Ltext0
	.2byte	0x2
	.byte	0x7c
	.sleb128 0
	.4byte	.LCFI11-.Ltext0
	.4byte	.LFE187-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 4
	.4byte	0
	.4byte	0
	.section	.debug_pubnames,"",%progbits
.Ldebug_pubnames0:
	.4byte	0x5a
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x516
	.4byte	0x291
	.ascii	"hacc_set_key\000"
	.4byte	0x30c
	.ascii	"hacc_do_aes\000"
	.4byte	0x3af
	.ascii	"hacc_init\000"
	.4byte	0x48b
	.ascii	"hacc_deinit\000"
	.4byte	0x4a7
	.ascii	"hacc_ctx\000"
	.4byte	0
	.section	.debug_pubtypes,"",%progbits
.Ldebug_pubtypes0:
	.4byte	0x17f
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x516
	.4byte	0x2d
	.ascii	"long unsigned int\000"
	.4byte	0x34
	.ascii	"unsigned int\000"
	.4byte	0x3b
	.ascii	"char\000"
	.4byte	0x42
	.ascii	"signed char\000"
	.4byte	0x49
	.ascii	"unsigned char\000"
	.4byte	0x50
	.ascii	"short int\000"
	.4byte	0x57
	.ascii	"short unsigned int\000"
	.4byte	0x5e
	.ascii	"int\000"
	.4byte	0x65
	.ascii	"long long int\000"
	.4byte	0x6c
	.ascii	"long long unsigned int\000"
	.4byte	0x73
	.ascii	"long int\000"
	.4byte	0x7a
	.ascii	"__kernel_size_t\000"
	.4byte	0x85
	.ascii	"_Bool\000"
	.4byte	0x8c
	.ascii	"size_t\000"
	.4byte	0x99
	.ascii	"U8\000"
	.4byte	0xa3
	.ascii	"U32\000"
	.4byte	0xae
	.ascii	"UINT32\000"
	.4byte	0xb9
	.ascii	"float\000"
	.4byte	0xc0
	.ascii	"BOOL\000"
	.4byte	0xe0
	.ascii	"AES_MODE\000"
	.4byte	0x100
	.ascii	"AES_OPS\000"
	.4byte	0x126
	.ascii	"AES_KEY\000"
	.4byte	0x14c
	.ascii	"AES_KEY_ID\000"
	.4byte	0x17e
	.ascii	"AES_CFG\000"
	.4byte	0x1be
	.ascii	"AES_KEY_SEED\000"
	.4byte	0x1c9
	.ascii	"hacc_context\000"
	.4byte	0
	.section	.debug_aranges,"",%progbits
	.4byte	0x1c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x4
	.byte	0
	.2byte	0
	.2byte	0
	.4byte	.Ltext0
	.4byte	.Letext0-.Ltext0
	.4byte	0
	.4byte	0
	.section	.debug_ranges,"",%progbits
.Ldebug_ranges0:
	.4byte	.LBB10-.Ltext0
	.4byte	.LBE10-.Ltext0
	.4byte	.LBB15-.Ltext0
	.4byte	.LBE15-.Ltext0
	.4byte	.LBB17-.Ltext0
	.4byte	.LBE17-.Ltext0
	.4byte	0
	.4byte	0
	.4byte	.LBB11-.Ltext0
	.4byte	.LBE11-.Ltext0
	.4byte	.LBB12-.Ltext0
	.4byte	.LBE12-.Ltext0
	.4byte	.LBB13-.Ltext0
	.4byte	.LBE13-.Ltext0
	.4byte	0
	.4byte	0
	.4byte	.LBB14-.Ltext0
	.4byte	.LBE14-.Ltext0
	.4byte	.LBB16-.Ltext0
	.4byte	.LBE16-.Ltext0
	.4byte	.LBB18-.Ltext0
	.4byte	.LBE18-.Ltext0
	.4byte	.LBB19-.Ltext0
	.4byte	.LBE19-.Ltext0
	.4byte	0
	.4byte	0
	.4byte	.LBB20-.Ltext0
	.4byte	.LBE20-.Ltext0
	.4byte	.LBB23-.Ltext0
	.4byte	.LBE23-.Ltext0
	.4byte	0
	.4byte	0
	.4byte	.LBB21-.Ltext0
	.4byte	.LBE21-.Ltext0
	.4byte	.LBB22-.Ltext0
	.4byte	.LBE22-.Ltext0
	.4byte	0
	.4byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF44:
	.ascii	"hacc_set_key\000"
.LASF15:
	.ascii	"BOOL\000"
.LASF5:
	.ascii	"short int\000"
.LASF12:
	.ascii	"size_t\000"
.LASF54:
	.ascii	"bHACC_SWKeyInit\000"
.LASF38:
	.ascii	"hacc_set_cfg\000"
.LASF26:
	.ascii	"AES_SW_KEY\000"
.LASF25:
	.ascii	"AES_KEY\000"
.LASF20:
	.ascii	"AES_ENC\000"
.LASF13:
	.ascii	"UINT32\000"
.LASF40:
	.ascii	"mode\000"
.LASF37:
	.ascii	"hw_key\000"
.LASF31:
	.ascii	"config\000"
.LASF57:
	.ascii	""
	.ascii	"\000"
.LASF51:
	.ascii	"console_printk\000"
.LASF50:
	.ascii	"elf_hwcap\000"
.LASF30:
	.ascii	"AES_CFG\000"
.LASF16:
	.ascii	"AES_ECB_MODE\000"
.LASF43:
	.ascii	"tkey\000"
.LASF14:
	.ascii	"float\000"
.LASF7:
	.ascii	"long long int\000"
.LASF24:
	.ascii	"AES_KEY_256\000"
.LASF53:
	.ascii	"bHACC_HWWrapKeyInit\000"
.LASF48:
	.ascii	"_end\000"
.LASF9:
	.ascii	"long int\000"
.LASF59:
	.ascii	"hacc_deinit\000"
.LASF29:
	.ascii	"AES_KEY_ID\000"
.LASF41:
	.ascii	"acon\000"
.LASF35:
	.ascii	"blk_sz\000"
.LASF4:
	.ascii	"unsigned char\000"
.LASF46:
	.ascii	"hacc_init\000"
.LASF3:
	.ascii	"signed char\000"
.LASF8:
	.ascii	"long long unsigned int\000"
.LASF17:
	.ascii	"AES_CBC_MODE\000"
.LASF1:
	.ascii	"unsigned int\000"
.LASF0:
	.ascii	"long unsigned int\000"
.LASF21:
	.ascii	"AES_OPS\000"
.LASF6:
	.ascii	"short unsigned int\000"
.LASF27:
	.ascii	"AES_HW_KEY\000"
.LASF2:
	.ascii	"char\000"
.LASF33:
	.ascii	"seed\000"
.LASF22:
	.ascii	"AES_KEY_128\000"
.LASF39:
	.ascii	"hacc_set_mode\000"
.LASF23:
	.ascii	"AES_KEY_192\000"
.LASF10:
	.ascii	"_Bool\000"
.LASF49:
	.ascii	"hacc_ctx\000"
.LASF34:
	.ascii	"AES_KEY_SEED\000"
.LASF45:
	.ascii	"hacc_do_aes\000"
.LASF47:
	.ascii	"keyseed\000"
.LASF32:
	.ascii	"size\000"
.LASF28:
	.ascii	"AES_HW_WRAP_KEY\000"
.LASF52:
	.ascii	"hex_asc\000"
.LASF19:
	.ascii	"AES_DEC\000"
.LASF42:
	.ascii	"akey\000"
.LASF11:
	.ascii	"__kernel_size_t\000"
.LASF56:
	.ascii	""
	.ascii	""
	.ascii	""
	.ascii	"\000"
.LASF18:
	.ascii	"AES_MODE\000"
.LASF36:
	.ascii	"sw_key\000"
.LASF58:
	.ascii	"hacc_context\000"
.LASF55:
	.ascii	"GNU C 4.6.x-google 20120106 (prerelease)\000"
	.ident	"GCC: (GNU) 4.6.x-google 20120106 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
