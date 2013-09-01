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
	.file	"hacc_lib.c"
	.text
.Ltext0:
	.align	2
	.global	SST_Secure_Algo
	.type	SST_Secure_Algo, %function
SST_Secure_Algo:
.LFB147:
	.file 1 "alps/kernel/mediatek/platform/mt6589/kernel/drivers/masp/asf/mach/hacc_lib.c"
	.loc 1 74 0
	@ args = 4, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL0:
	mov	ip, sp
.LCFI0:
	stmfd	sp!, {r4, r5, r6, r7, r8, fp, ip, lr, pc}
.LCFI1:
	sub	fp, ip, #4
.LCFI2:
	sub	sp, sp, #12
.LCFI3:
	ldr	r5, [fp, #4]
.LVL1:
	mov	r7, r0
	mov	r8, r1
	mov	r4, r2
	mov	r6, r3
.LVL2:
.L2:
	.loc 1 83 0
	bl	osal_hacc_lock
.LVL3:
	.loc 1 84 0 discriminator 1
	cmp	r0, #0
	bne	.L2
.LVL4:
	.loc 1 88 0
	mov	r0, r6
.LVL5:
	mov	r1, #16
	bl	sp_hacc_init
.LVL6:
	.loc 1 90 0
	subs	r6, r0, #0
.LVL7:
	bne	.L3
.LVL8:
	.loc 1 100 0
	cmp	r7, #0
	bne	.L14
	.loc 1 113 0
	mov	r0, r8
.LVL9:
	mov	r1, r4
	mov	r2, #1
	mov	r3, #2
	str	r7, [sp, #0]
	bl	sp_hacc_dec
.LVL10:
.L6:
	.loc 1 123 0 discriminator 1
	cmp	r4, #0
	beq	.L7
	mov	r3, #0
.LVL11:
.L8:
	.loc 1 125 0
	ldrb	r2, [r0, r3]	@ zero_extendqisi2
	strb	r2, [r5, r3]
	.loc 1 123 0 discriminator 2
	add	r3, r3, #1
.LVL12:
	.loc 1 123 0 is_stmt 0 discriminator 1
	cmp	r3, r4
	bne	.L8
.LVL13:
.L7:
	.loc 1 129 0 is_stmt 1
	bl	osal_hacc_unlock
.LVL14:
	.loc 1 140 0
	sub	sp, fp, #32
	ldmfd	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
.LVL15:
.L14:
	.loc 1 100 0
	cmp	r7, #1
	.loc 1 117 0
	movne	r6, #24576
	.loc 1 100 0
	beq	.L15
.LVL16:
.L3:
	.loc 1 135 0
	bl	osal_hacc_unlock
	.loc 1 137 0 discriminator 1
	ldr	r1, .L16
	mov	r2, r6
	ldr	r0, .L16+4
	bl	printk
	.loc 1 138 0 discriminator 1
#APP
@ 138 "alps/kernel/mediatek/platform/mt6589/kernel/drivers/masp/asf/mach/hacc_lib.c" 1
	.word 0xe7f001f2
@ 0 "" 2
.LVL17:
.L15:
	.loc 1 106 0
	mov	r0, r8
.LVL18:
	mov	r1, r4
	mov	r2, r7
	mov	r3, #2
	str	r6, [sp, #0]
	bl	sp_hacc_enc
.LVL19:
	.loc 1 107 0
	b	.L6
.L17:
	.align	2
.L16:
	.word	.LC1
	.word	.LC0
.LFE147:
	.size	SST_Secure_Algo, .-SST_Secure_Algo
	.section	.rodata.str1.4,"aMS",%progbits,1
	.align	2
.LC0:
	.ascii	"[%s] SST_Secure_Algo error (0x%x)\012\000"
	.space	1
.LC1:
	.ascii	"CE\000"
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
	.4byte	.LFB147
	.4byte	.LFE147-.LFB147
	.byte	0x4
	.4byte	.LCFI0-.LFB147
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
	.byte	0x88
	.uleb128 0x5
	.byte	0x87
	.uleb128 0x6
	.byte	0x86
	.uleb128 0x7
	.byte	0x85
	.uleb128 0x8
	.byte	0x84
	.uleb128 0x9
	.byte	0x4
	.4byte	.LCFI2-.LCFI1
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE0:
	.text
.Letext0:
	.file 2 "alps/kernel/mediatek/platform/mt6589/kernel/drivers/masp/asf/inc/sec_typedef.h"
	.file 3 "alps/kernel/arch/arm/include/asm/hwcap.h"
	.file 4 "alps/kernel/include/linux/printk.h"
	.file 5 "alps/kernel/include/linux/kernel.h"
	.file 6 "alps/kernel/mediatek/platform/mt6589/kernel/drivers/masp/asf/inc/hacc_export.h"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x1b7
	.2byte	0x2
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.LASF26
	.byte	0x1
	.4byte	.LASF27
	.4byte	.LASF28
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
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF10
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF11
	.uleb128 0x4
	.byte	0x4
	.byte	0x6
	.byte	0x2a
	.4byte	0xa3
	.uleb128 0x5
	.4byte	.LASF12
	.sleb128 0
	.uleb128 0x5
	.4byte	.LASF13
	.sleb128 1
	.uleb128 0x5
	.4byte	.LASF14
	.sleb128 2
	.byte	0
	.uleb128 0x6
	.4byte	.LASF15
	.byte	0x2
	.byte	0x32
	.4byte	0x34
	.uleb128 0x6
	.4byte	.LASF16
	.byte	0x2
	.byte	0x33
	.4byte	0x49
	.uleb128 0x7
	.byte	0x1
	.4byte	.LASF29
	.byte	0x1
	.byte	0x49
	.byte	0x1
	.4byte	.LFB147
	.4byte	.LFE147
	.4byte	.LLST0
	.4byte	0x171
	.uleb128 0x8
	.4byte	.LASF17
	.byte	0x1
	.byte	0x49
	.4byte	0xae
	.4byte	.LLST1
	.uleb128 0x8
	.4byte	.LASF18
	.byte	0x1
	.byte	0x49
	.4byte	0xa3
	.4byte	.LLST2
	.uleb128 0x8
	.4byte	.LASF19
	.byte	0x1
	.byte	0x49
	.4byte	0xa3
	.4byte	.LLST3
	.uleb128 0x8
	.4byte	.LASF20
	.byte	0x1
	.byte	0x49
	.4byte	0x171
	.4byte	.LLST4
	.uleb128 0x9
	.4byte	.LASF21
	.byte	0x1
	.byte	0x49
	.4byte	0x171
	.byte	0x2
	.byte	0x91
	.sleb128 0
	.uleb128 0xa
	.ascii	"err\000"
	.byte	0x1
	.byte	0x4b
	.4byte	0xa3
	.4byte	.LLST5
	.uleb128 0xa
	.ascii	"src\000"
	.byte	0x1
	.byte	0x4c
	.4byte	0x171
	.4byte	.LLST6
	.uleb128 0xa
	.ascii	"dst\000"
	.byte	0x1
	.byte	0x4c
	.4byte	0x171
	.4byte	.LLST7
	.uleb128 0xb
	.4byte	.LASF22
	.byte	0x1
	.byte	0x4c
	.4byte	0x171
	.4byte	.LLST8
	.uleb128 0xa
	.ascii	"i\000"
	.byte	0x1
	.byte	0x4d
	.4byte	0xa3
	.4byte	.LLST9
	.uleb128 0xc
	.4byte	.LASF30
	.byte	0x1
	.byte	0x85
	.4byte	.L3
	.byte	0
	.uleb128 0xd
	.byte	0x4
	.4byte	0xae
	.uleb128 0xe
	.4byte	.LASF23
	.byte	0x3
	.byte	0x23
	.4byte	0x34
	.byte	0x1
	.byte	0x1
	.uleb128 0xf
	.4byte	0x5e
	.4byte	0x18f
	.uleb128 0x10
	.byte	0
	.uleb128 0xe
	.4byte	.LASF24
	.byte	0x4
	.byte	0x1b
	.4byte	0x184
	.byte	0x1
	.byte	0x1
	.uleb128 0xf
	.4byte	0x3b
	.4byte	0x1a7
	.uleb128 0x10
	.byte	0
	.uleb128 0x11
	.4byte	.LASF25
	.byte	0x5
	.2byte	0x18a
	.4byte	0x1b5
	.byte	0x1
	.byte	0x1
	.uleb128 0x12
	.4byte	0x19c
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
	.uleb128 0x5
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xd
	.byte	0
	.byte	0
	.uleb128 0x6
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
	.uleb128 0x7
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
	.uleb128 0x8
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
	.uleb128 0x9
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
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0xa
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
	.uleb128 0xb
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
	.uleb128 0xc
	.uleb128 0xa
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x11
	.uleb128 0x1
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xe
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
	.uleb128 0xf
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x11
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
	.uleb128 0x12
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
	.4byte	.LFB147-.Ltext0
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
	.4byte	.LFE147-.Ltext0
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
	.4byte	0
	.4byte	0
.LLST2:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL2-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	.LVL2-.Ltext0
	.4byte	.LFE147-.Ltext0
	.2byte	0x1
	.byte	0x58
	.4byte	0
	.4byte	0
.LLST3:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL2-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	.LVL2-.Ltext0
	.4byte	.LFE147-.Ltext0
	.2byte	0x1
	.byte	0x54
	.4byte	0
	.4byte	0
.LLST4:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL2-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	.LVL2-.Ltext0
	.4byte	.LVL7-.Ltext0
	.2byte	0x1
	.byte	0x56
	.4byte	0
	.4byte	0
.LLST5:
	.4byte	.LVL3-.Ltext0
	.4byte	.LVL5-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL6-.Ltext0
	.4byte	.LVL9-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL9-.Ltext0
	.4byte	.LVL10-.Ltext0
	.2byte	0x1
	.byte	0x56
	.4byte	.LVL15-.Ltext0
	.4byte	.LVL16-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL16-.Ltext0
	.4byte	.LVL17-.Ltext0
	.2byte	0x1
	.byte	0x56
	.4byte	.LVL17-.Ltext0
	.4byte	.LVL18-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST6:
	.4byte	.LVL8-.Ltext0
	.4byte	.LVL16-.Ltext0
	.2byte	0x1
	.byte	0x58
	.4byte	.LVL17-.Ltext0
	.4byte	.LFE147-.Ltext0
	.2byte	0x1
	.byte	0x58
	.4byte	0
	.4byte	0
.LLST7:
	.4byte	.LVL8-.Ltext0
	.4byte	.LVL10-.Ltext0
	.2byte	0x2
	.byte	0x91
	.sleb128 0
	.4byte	.LVL10-.Ltext0
	.4byte	.LVL14-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL15-.Ltext0
	.4byte	.LVL16-.Ltext0
	.2byte	0x2
	.byte	0x91
	.sleb128 0
	.4byte	.LVL17-.Ltext0
	.4byte	.LVL19-.Ltext0
	.2byte	0x2
	.byte	0x91
	.sleb128 0
	.4byte	.LVL19-.Ltext0
	.4byte	.LFE147-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST8:
	.4byte	.LVL4-.Ltext0
	.4byte	.LVL7-.Ltext0
	.2byte	0x1
	.byte	0x56
	.4byte	0
	.4byte	0
.LLST9:
	.4byte	.LVL1-.Ltext0
	.4byte	.LVL11-.Ltext0
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL12-.Ltext0
	.4byte	.LVL13-.Ltext0
	.2byte	0x1
	.byte	0x53
	.4byte	.LVL15-.Ltext0
	.4byte	.LFE147-.Ltext0
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	0
	.4byte	0
	.section	.debug_pubnames,"",%progbits
.Ldebug_pubnames0:
	.4byte	0x22
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x1bb
	.4byte	0xb9
	.ascii	"SST_Secure_Algo\000"
	.4byte	0
	.section	.debug_pubtypes,"",%progbits
.Ldebug_pubtypes0:
	.4byte	0xf0
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x1bb
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
	.ascii	"_Bool\000"
	.4byte	0x81
	.ascii	"float\000"
	.4byte	0xa3
	.ascii	"uint32\000"
	.4byte	0xae
	.ascii	"uchar\000"
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
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF24:
	.ascii	"console_printk\000"
.LASF25:
	.ascii	"hex_asc\000"
.LASF27:
	.ascii	""
	.ascii	""
	.ascii	""
	.ascii	"\000"
.LASF11:
	.ascii	"float\000"
.LASF4:
	.ascii	"unsigned char\000"
.LASF20:
	.ascii	"CustomSeed\000"
.LASF0:
	.ascii	"long unsigned int\000"
.LASF6:
	.ascii	"short unsigned int\000"
.LASF26:
	.ascii	"GNU C 4.6.x-google 20120106 (prerelease)\000"
.LASF16:
	.ascii	"uchar\000"
.LASF21:
	.ascii	"ResText\000"
.LASF23:
	.ascii	"elf_hwcap\000"
.LASF19:
	.ascii	"ContentLen\000"
.LASF15:
	.ascii	"uint32\000"
.LASF1:
	.ascii	"unsigned int\000"
.LASF29:
	.ascii	"SST_Secure_Algo\000"
.LASF8:
	.ascii	"long long unsigned int\000"
.LASF7:
	.ascii	"long long int\000"
.LASF2:
	.ascii	"char\000"
.LASF28:
	.ascii	""
	.ascii	"\000"
.LASF5:
	.ascii	"short int\000"
.LASF12:
	.ascii	"HACC_USER1\000"
.LASF22:
	.ascii	"seed\000"
.LASF17:
	.ascii	"Direction\000"
.LASF9:
	.ascii	"long int\000"
.LASF3:
	.ascii	"signed char\000"
.LASF18:
	.ascii	"ContentAddr\000"
.LASF13:
	.ascii	"HACC_USER2\000"
.LASF14:
	.ascii	"HACC_USER3\000"
.LASF10:
	.ascii	"_Bool\000"
.LASF30:
	.ascii	"_error\000"
	.ident	"GCC: (GNU) 4.6.x-google 20120106 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
