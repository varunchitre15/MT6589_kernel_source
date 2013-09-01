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
	.file	"sec_uid.c"
	.text
.Ltext0:
	.align	2
	.global	secpal_memcpy
	.type	secpal_memcpy, %function
secpal_memcpy:
.LFB146:
	.file 1 "alps/kernel/mediatek/platform/mt6589/kernel/drivers/masp/asf/mach/sec_uid.c"
	.loc 1 46 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL0:
	mov	ip, sp
.LCFI0:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI1:
	sub	fp, ip, #4
.LCFI2:
	.loc 1 47 0
	bl	memcpy
.LVL1:
	.loc 1 48 0
	ldmfd	sp, {fp, sp, pc}
.LFE146:
	.size	secpal_memcpy, .-secpal_memcpy
	.align	2
	.global	secpal_memset
	.type	secpal_memset, %function
secpal_memset:
.LFB147:
	.loc 1 51 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL2:
	mov	ip, sp
.LCFI3:
	stmfd	sp!, {r3, r4, fp, ip, lr, pc}
.LCFI4:
	sub	fp, ip, #4
.LCFI5:
.LBB2:
	.loc 1 52 0
	cmp	r2, #0
.LBE2:
	.loc 1 51 0
	mov	r4, r0
.LVL3:
.LBB3:
	.loc 1 52 0
	beq	.L3
	.loc 1 52 0 is_stmt 0 discriminator 4
	bl	memset
.LVL4:
.L3:
.LBE3:
	.loc 1 53 0 is_stmt 1
	mov	r0, r4
	ldmfd	sp, {r3, r4, fp, sp, pc}
.LFE147:
	.size	secpal_memset, .-secpal_memset
	.align	2
	.global	secpal_print
	.type	secpal_print, %function
secpal_print:
.LFB148:
	.loc 1 56 0
	@ args = 4, pretend = 16, frame = 8
	@ frame_needed = 1, uses_anonymous_args = 1
.LVL5:
	mov	ip, sp
.LCFI6:
	stmfd	sp!, {r0, r1, r2, r3}
.LCFI7:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI8:
	sub	fp, ip, #20
.LCFI9:
	sub	sp, sp, #8
.LCFI10:
	.loc 1 59 0
	add	r1, fp, #8
	.loc 1 60 0
	ldr	r0, [fp, #4]
	.loc 1 59 0
	str	r1, [fp, #-16]
	.loc 1 60 0
	bl	vprintk
	.loc 1 64 0
	mov	r0, #0
	sub	sp, fp, #12
	ldmfd	sp, {fp, sp, pc}
.LFE148:
	.size	secpal_print, .-secpal_print
	.align	2
	.global	secpal_get_uuid
	.type	secpal_get_uuid, %function
secpal_get_uuid:
.LFB149:
	.loc 1 67 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL6:
	mov	ip, sp
.LCFI11:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI12:
	sub	fp, ip, #4
.LCFI13:
	.loc 1 68 0
	mov	r3, #36864
	movt	r3, 61440
	.loc 1 67 0
	mov	r2, r0
	.loc 1 74 0
	mov	r0, #0
.LVL7:
	.loc 1 68 0
	ldr	r1, [r3, #320]
	str	r1, [r2, #0]
	.loc 1 69 0
	ldr	r1, [r3, #324]
	str	r1, [r2, #4]
	.loc 1 70 0
	ldr	r1, [r3, #320]
	str	r1, [r2, #8]
	.loc 1 71 0
	ldr	r3, [r3, #324]
	str	r3, [r2, #12]
	.loc 1 74 0
	ldmfd	sp, {fp, sp, pc}
.LFE149:
	.size	secpal_get_uuid, .-secpal_get_uuid
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
	.4byte	.LFB146
	.4byte	.LFE146-.LFB146
	.byte	0x4
	.4byte	.LCFI0-.LFB146
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
	.4byte	.LFB147
	.4byte	.LFE147-.LFB147
	.byte	0x4
	.4byte	.LCFI3-.LFB147
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
	.4byte	.LFB148
	.4byte	.LFE148-.LFB148
	.byte	0x4
	.4byte	.LCFI6-.LFB148
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI8-.LCFI6
	.byte	0x8e
	.uleb128 0x6
	.byte	0x8d
	.uleb128 0x7
	.byte	0x8b
	.uleb128 0x8
	.byte	0x83
	.uleb128 0x1
	.byte	0x82
	.uleb128 0x2
	.byte	0x81
	.uleb128 0x3
	.byte	0x80
	.uleb128 0x4
	.byte	0x4
	.4byte	.LCFI9-.LCFI8
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x14
	.align	2
.LEFDE4:
.LSFDE6:
	.4byte	.LEFDE6-.LASFDE6
.LASFDE6:
	.4byte	.Lframe0
	.4byte	.LFB149
	.4byte	.LFE149-.LFB149
	.byte	0x4
	.4byte	.LCFI11-.LFB149
	.byte	0xd
	.uleb128 0xc
	.byte	0x4
	.4byte	.LCFI12-.LCFI11
	.byte	0x8e
	.uleb128 0x2
	.byte	0x8d
	.uleb128 0x3
	.byte	0x8b
	.uleb128 0x4
	.byte	0x4
	.4byte	.LCFI13-.LCFI12
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE6:
	.text
.Letext0:
	.file 2 "alps/kernel/include/asm-generic/posix_types.h"
	.file 3 "alps/kernel/include/linux/types.h"
	.file 4 "alps/prebuilts/gcc/linux-x86/arm/arm-linux-androideabi-4.6/bin/../lib/gcc/arm-linux-androideabi/4.6.x-google/include/stdarg.h"
	.file 5 "alps/kernel/arch/arm/include/asm/hwcap.h"
	.file 6 "alps/kernel/include/linux/printk.h"
	.file 7 "alps/kernel/include/linux/kernel.h"
	.file 8 "<built-in>"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0x241
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
	.uleb128 0x3
	.byte	0x4
	.4byte	0x41
	.uleb128 0x4
	.4byte	0x46
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
	.uleb128 0x5
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
	.uleb128 0x6
	.4byte	.LASF11
	.byte	0x2
	.byte	0x47
	.4byte	0x34
	.uleb128 0x2
	.byte	0x1
	.byte	0x2
	.4byte	.LASF10
	.uleb128 0x6
	.4byte	.LASF12
	.byte	0x3
	.byte	0x3f
	.4byte	0x85
	.uleb128 0x6
	.4byte	.LASF13
	.byte	0x4
	.byte	0x28
	.4byte	0xad
	.uleb128 0x7
	.4byte	.LASF29
	.byte	0x4
	.byte	0x8
	.byte	0
	.4byte	0xc7
	.uleb128 0x8
	.4byte	.LASF30
	.4byte	0xc7
	.byte	0x2
	.byte	0x23
	.uleb128 0
	.byte	0x1
	.byte	0
	.uleb128 0x9
	.byte	0x4
	.uleb128 0x6
	.4byte	.LASF14
	.byte	0x4
	.byte	0x66
	.4byte	0xa2
	.uleb128 0xa
	.byte	0x1
	.4byte	.LASF16
	.byte	0x1
	.byte	0x2d
	.byte	0x1
	.4byte	0xc7
	.4byte	.LFB146
	.4byte	.LFE146
	.4byte	.LLST0
	.4byte	0x11f
	.uleb128 0xb
	.ascii	"dst\000"
	.byte	0x1
	.byte	0x2d
	.4byte	0xc7
	.4byte	.LLST1
	.uleb128 0xb
	.ascii	"src\000"
	.byte	0x1
	.byte	0x2d
	.4byte	0xc7
	.4byte	.LLST2
	.uleb128 0xc
	.4byte	.LASF15
	.byte	0x1
	.byte	0x2d
	.4byte	0x34
	.4byte	.LLST3
	.byte	0
	.uleb128 0xa
	.byte	0x1
	.4byte	.LASF17
	.byte	0x1
	.byte	0x32
	.byte	0x1
	.4byte	0xc7
	.4byte	.LFB147
	.4byte	.LFE147
	.4byte	.LLST4
	.4byte	0x18a
	.uleb128 0xb
	.ascii	"s\000"
	.byte	0x1
	.byte	0x32
	.4byte	0xc7
	.4byte	.LLST5
	.uleb128 0xb
	.ascii	"c\000"
	.byte	0x1
	.byte	0x32
	.4byte	0x69
	.4byte	.LLST6
	.uleb128 0xc
	.4byte	.LASF18
	.byte	0x1
	.byte	0x32
	.4byte	0x34
	.4byte	.LLST7
	.uleb128 0xd
	.4byte	.Ldebug_ranges0+0
	.uleb128 0xe
	.ascii	"__p\000"
	.byte	0x1
	.byte	0x34
	.4byte	0xc7
	.4byte	.LLST8
	.uleb128 0xe
	.ascii	"__n\000"
	.byte	0x1
	.byte	0x34
	.4byte	0x97
	.4byte	.LLST9
	.byte	0
	.byte	0
	.uleb128 0xa
	.byte	0x1
	.4byte	.LASF19
	.byte	0x1
	.byte	0x37
	.byte	0x1
	.4byte	0x69
	.4byte	.LFB148
	.4byte	.LFE148
	.4byte	.LLST10
	.4byte	0x1ce
	.uleb128 0xf
	.ascii	"fmt\000"
	.byte	0x1
	.byte	0x37
	.4byte	0x3b
	.byte	0x2
	.byte	0x91
	.sleb128 -16
	.uleb128 0x10
	.uleb128 0x11
	.4byte	.LASF20
	.byte	0x1
	.byte	0x39
	.4byte	0xc9
	.byte	0x2
	.byte	0x91
	.sleb128 -36
	.uleb128 0x12
	.ascii	"r\000"
	.byte	0x1
	.byte	0x3a
	.4byte	0x69
	.byte	0
	.uleb128 0xa
	.byte	0x1
	.4byte	.LASF21
	.byte	0x1
	.byte	0x42
	.byte	0x1
	.4byte	0x69
	.4byte	.LFB149
	.4byte	.LFE149
	.4byte	.LLST11
	.4byte	0x1fb
	.uleb128 0xc
	.4byte	.LASF22
	.byte	0x1
	.byte	0x42
	.4byte	0x1fb
	.4byte	.LLST12
	.byte	0
	.uleb128 0x3
	.byte	0x4
	.4byte	0x34
	.uleb128 0x13
	.4byte	.LASF23
	.byte	0x5
	.byte	0x23
	.4byte	0x34
	.byte	0x1
	.byte	0x1
	.uleb128 0x14
	.4byte	0x69
	.4byte	0x219
	.uleb128 0x15
	.byte	0
	.uleb128 0x13
	.4byte	.LASF24
	.byte	0x6
	.byte	0x1b
	.4byte	0x20e
	.byte	0x1
	.byte	0x1
	.uleb128 0x14
	.4byte	0x46
	.4byte	0x231
	.uleb128 0x15
	.byte	0
	.uleb128 0x16
	.4byte	.LASF25
	.byte	0x7
	.2byte	0x18a
	.4byte	0x23f
	.byte	0x1
	.byte	0x1
	.uleb128 0x4
	.4byte	0x226
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
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
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
	.uleb128 0x8
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xa
	.uleb128 0x34
	.uleb128 0xc
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xa
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
	.uleb128 0xb
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
	.uleb128 0xc
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
	.uleb128 0xd
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x55
	.uleb128 0x6
	.byte	0
	.byte	0
	.uleb128 0xe
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
	.uleb128 0xf
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
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0x18
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
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0xa
	.byte	0
	.byte	0
	.uleb128 0x12
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
	.uleb128 0x13
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
	.uleb128 0x14
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x16
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
	.byte	0
	.section	.debug_loc,"",%progbits
.Ldebug_loc0:
.LLST0:
	.4byte	.LFB146-.Ltext0
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
	.4byte	.LFE146-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 4
	.4byte	0
	.4byte	0
.LLST1:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL1-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST2:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL1-1-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	0
	.4byte	0
.LLST3:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL1-1-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
.LLST4:
	.4byte	.LFB147-.Ltext0
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
	.4byte	.LFE147-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 4
	.4byte	0
	.4byte	0
.LLST5:
	.4byte	.LVL2-.Ltext0
	.4byte	.LVL4-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL4-1-.Ltext0
	.4byte	.LFE147-.Ltext0
	.2byte	0x1
	.byte	0x54
	.4byte	0
	.4byte	0
.LLST6:
	.4byte	.LVL2-.Ltext0
	.4byte	.LVL4-1-.Ltext0
	.2byte	0x1
	.byte	0x51
	.4byte	0
	.4byte	0
.LLST7:
	.4byte	.LVL2-.Ltext0
	.4byte	.LVL4-1-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
.LLST8:
	.4byte	.LVL3-.Ltext0
	.4byte	.LVL4-1-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL4-1-.Ltext0
	.4byte	.LFE147-.Ltext0
	.2byte	0x1
	.byte	0x54
	.4byte	0
	.4byte	0
.LLST9:
	.4byte	.LVL3-.Ltext0
	.4byte	.LVL4-1-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
.LLST10:
	.4byte	.LFB148-.Ltext0
	.4byte	.LCFI6-.Ltext0
	.2byte	0x2
	.byte	0x7d
	.sleb128 0
	.4byte	.LCFI6-.Ltext0
	.4byte	.LCFI9-.Ltext0
	.2byte	0x2
	.byte	0x7c
	.sleb128 0
	.4byte	.LCFI9-.Ltext0
	.4byte	.LFE148-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 20
	.4byte	0
	.4byte	0
.LLST11:
	.4byte	.LFB149-.Ltext0
	.4byte	.LCFI11-.Ltext0
	.2byte	0x2
	.byte	0x7d
	.sleb128 0
	.4byte	.LCFI11-.Ltext0
	.4byte	.LCFI13-.Ltext0
	.2byte	0x2
	.byte	0x7c
	.sleb128 0
	.4byte	.LCFI13-.Ltext0
	.4byte	.LFE149-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 4
	.4byte	0
	.4byte	0
.LLST12:
	.4byte	.LVL6-.Ltext0
	.4byte	.LVL7-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	.LVL7-.Ltext0
	.4byte	.LFE149-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
	.section	.debug_pubnames,"",%progbits
.Ldebug_pubnames0:
	.4byte	0x57
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x245
	.4byte	0xd4
	.ascii	"secpal_memcpy\000"
	.4byte	0x11f
	.ascii	"secpal_memset\000"
	.4byte	0x18a
	.ascii	"secpal_print\000"
	.4byte	0x1ce
	.ascii	"secpal_get_uuid\000"
	.4byte	0
	.section	.debug_pubtypes,"",%progbits
.Ldebug_pubtypes0:
	.4byte	0x11d
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0x245
	.4byte	0x2d
	.ascii	"long unsigned int\000"
	.4byte	0x34
	.ascii	"unsigned int\000"
	.4byte	0x46
	.ascii	"char\000"
	.4byte	0x4d
	.ascii	"signed char\000"
	.4byte	0x54
	.ascii	"unsigned char\000"
	.4byte	0x5b
	.ascii	"short int\000"
	.4byte	0x62
	.ascii	"short unsigned int\000"
	.4byte	0x69
	.ascii	"int\000"
	.4byte	0x70
	.ascii	"long long int\000"
	.4byte	0x77
	.ascii	"long long unsigned int\000"
	.4byte	0x7e
	.ascii	"long int\000"
	.4byte	0x85
	.ascii	"__kernel_size_t\000"
	.4byte	0x90
	.ascii	"_Bool\000"
	.4byte	0x97
	.ascii	"size_t\000"
	.4byte	0xad
	.ascii	"__va_list\000"
	.4byte	0xa2
	.ascii	"__gnuc_va_list\000"
	.4byte	0xc9
	.ascii	"va_list\000"
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
	.4byte	.LBB2-.Ltext0
	.4byte	.LBE2-.Ltext0
	.4byte	.LBB3-.Ltext0
	.4byte	.LBE3-.Ltext0
	.4byte	0
	.4byte	0
	.section	.debug_line,"",%progbits
.Ldebug_line0:
	.section	.debug_str,"MS",%progbits,1
.LASF24:
	.ascii	"console_printk\000"
.LASF12:
	.ascii	"size_t\000"
.LASF20:
	.ascii	"args\000"
.LASF18:
	.ascii	"count\000"
.LASF17:
	.ascii	"secpal_memset\000"
.LASF14:
	.ascii	"va_list\000"
.LASF0:
	.ascii	"long unsigned int\000"
.LASF6:
	.ascii	"short unsigned int\000"
.LASF26:
	.ascii	"GNU C 4.6.x-google 20120106 (prerelease)\000"
.LASF27:
	.ascii	""
	.ascii	""
	.ascii	""
	.ascii	"\000"
.LASF4:
	.ascii	"unsigned char\000"
.LASF16:
	.ascii	"secpal_memcpy\000"
.LASF23:
	.ascii	"elf_hwcap\000"
.LASF13:
	.ascii	"__gnuc_va_list\000"
.LASF1:
	.ascii	"unsigned int\000"
.LASF22:
	.ascii	"uuid\000"
.LASF8:
	.ascii	"long long unsigned int\000"
.LASF30:
	.ascii	"__ap\000"
.LASF11:
	.ascii	"__kernel_size_t\000"
.LASF21:
	.ascii	"secpal_get_uuid\000"
.LASF19:
	.ascii	"secpal_print\000"
.LASF29:
	.ascii	"__va_list\000"
.LASF7:
	.ascii	"long long int\000"
.LASF2:
	.ascii	"char\000"
.LASF28:
	.ascii	""
	.ascii	"\000"
.LASF5:
	.ascii	"short int\000"
.LASF25:
	.ascii	"hex_asc\000"
.LASF9:
	.ascii	"long int\000"
.LASF3:
	.ascii	"signed char\000"
.LASF15:
	.ascii	"size\000"
.LASF10:
	.ascii	"_Bool\000"
	.ident	"GCC: (GNU) 4.6.x-google 20120106 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
