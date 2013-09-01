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
	.file	"sec_lib.c"
	.text
.Ltext0:
	.align	2
	.global	es_sbc_enabled
	.type	es_sbc_enabled, %function
es_sbc_enabled:
.LFB1:
	.file 1 "alps/kernel/mediatek/platform/mt6589/kernel/drivers/masp/asf/mach/sec_lib.c"
	.loc 1 71 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
.LCFI0:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI1:
	sub	fp, ip, #4
.LCFI2:
	.loc 1 72 0
	mov	r3, #36864
	movt	r3, 61440
	ldr	r0, [r3, #96]
	.loc 1 73 0
	ubfx	r0, r0, #1, #1
	ldmfd	sp, {fp, sp, pc}
.LFE1:
	.size	es_sbc_enabled, .-es_sbc_enabled
	.align	2
	.global	SST_GetSecESChecksum
	.type	SST_GetSecESChecksum, %function
SST_GetSecESChecksum:
.LFB2:
	.loc 1 76 0
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
.LVL0:
	mov	ip, sp
.LCFI3:
	stmfd	sp!, {fp, ip, lr, pc}
.LCFI4:
	sub	fp, ip, #4
.LCFI5:
.LVL1:
	.loc 1 79 0
	ldr	r1, [r0, #0]
	mov	r2, #0
.LVL2:
.L3:
	.loc 1 81 0
	mov	ip, r2, asl #2
	movw	r3, #37008
	movt	r3, 61440
	.loc 1 79 0 discriminator 2
	add	r2, r2, #1
.LVL3:
	.loc 1 81 0
	add	r3, ip, r3
	.loc 1 79 0 discriminator 1
	cmp	r2, #8
	.loc 1 81 0
	ldr	r3, [r3, #0]
	add	r1, r1, r3
	str	r1, [r0, #0]
	.loc 1 79 0 discriminator 1
	bne	.L3
	.loc 1 86 0
	mov	r0, #0
.LVL4:
	ldmfd	sp, {fp, sp, pc}
.LFE2:
	.size	SST_GetSecESChecksum, .-SST_GetSecESChecksum
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
	.4byte	.LFB1
	.4byte	.LFE1-.LFB1
	.byte	0x4
	.4byte	.LCFI0-.LFB1
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
	.4byte	.LFB2
	.4byte	.LFE2-.LFB2
	.byte	0x4
	.4byte	.LCFI3-.LFB2
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
	.byte	0x4
	.4byte	.LCFI5-.LCFI4
	.byte	0xc
	.uleb128 0xb
	.uleb128 0x4
	.align	2
.LEFDE2:
	.text
.Letext0:
	.file 2 "alps/kernel/../mediatek/platform/mt6589/kernel/core/include/mach/mt_typedefs.h"
	.section	.debug_info,"",%progbits
.Ldebug_info0:
	.4byte	0xda
	.2byte	0x2
	.4byte	.Ldebug_abbrev0
	.byte	0x4
	.uleb128 0x1
	.4byte	.LASF11
	.byte	0x1
	.4byte	.LASF12
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
	.byte	0x2
	.byte	0x7
	.4byte	.LASF3
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.4byte	.LASF4
	.uleb128 0x2
	.byte	0x4
	.byte	0x5
	.4byte	.LASF5
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.4byte	.LASF6
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.4byte	.LASF7
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.ascii	"int\000"
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.4byte	.LASF8
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.4byte	.LASF9
	.uleb128 0x4
	.byte	0x4
	.4byte	0x30
	.uleb128 0x5
	.4byte	.LASF13
	.byte	0x2
	.byte	0x59
	.4byte	0x30
	.uleb128 0x2
	.byte	0x4
	.byte	0x4
	.4byte	.LASF10
	.uleb128 0x6
	.byte	0x1
	.4byte	.LASF14
	.byte	0x1
	.byte	0x46
	.byte	0x1
	.4byte	0x61
	.4byte	.LFB1
	.4byte	.LFE1
	.4byte	.LLST0
	.uleb128 0x7
	.byte	0x1
	.4byte	.LASF15
	.byte	0x1
	.byte	0x4b
	.byte	0x1
	.4byte	0x61
	.4byte	.LFB2
	.4byte	.LFE2
	.4byte	.LLST1
	.uleb128 0x8
	.4byte	.LASF16
	.byte	0x1
	.byte	0x4b
	.4byte	0x76
	.4byte	.LLST2
	.uleb128 0x9
	.ascii	"i\000"
	.byte	0x1
	.byte	0x4d
	.4byte	0x61
	.4byte	.LLST3
	.byte	0
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
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
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
	.uleb128 0x6
	.uleb128 0x2e
	.byte	0
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
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x1
	.uleb128 0x40
	.uleb128 0x6
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
	.byte	0
	.section	.debug_loc,"",%progbits
.Ldebug_loc0:
.LLST0:
	.4byte	.LFB1-.Ltext0
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
	.4byte	.LFE1-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 4
	.4byte	0
	.4byte	0
.LLST1:
	.4byte	.LFB2-.Ltext0
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
	.4byte	.LFE2-.Ltext0
	.2byte	0x2
	.byte	0x7b
	.sleb128 4
	.4byte	0
	.4byte	0
.LLST2:
	.4byte	.LVL0-.Ltext0
	.4byte	.LVL4-.Ltext0
	.2byte	0x1
	.byte	0x50
	.4byte	0
	.4byte	0
.LLST3:
	.4byte	.LVL1-.Ltext0
	.4byte	.LVL2-.Ltext0
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.4byte	.LVL3-.Ltext0
	.4byte	.LFE2-.Ltext0
	.2byte	0x1
	.byte	0x52
	.4byte	0
	.4byte	0
	.section	.debug_pubnames,"",%progbits
.Ldebug_pubnames0:
	.4byte	0x3a
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0xde
	.4byte	0x8e
	.ascii	"es_sbc_enabled\000"
	.4byte	0xa7
	.ascii	"SST_GetSecESChecksum\000"
	.4byte	0
	.section	.debug_pubtypes,"",%progbits
.Ldebug_pubtypes0:
	.4byte	0xdc
	.2byte	0x2
	.4byte	.Ldebug_info0
	.4byte	0xde
	.4byte	0x29
	.ascii	"long unsigned int\000"
	.4byte	0x30
	.ascii	"unsigned int\000"
	.4byte	0x37
	.ascii	"char\000"
	.4byte	0x3e
	.ascii	"short unsigned int\000"
	.4byte	0x45
	.ascii	"unsigned char\000"
	.4byte	0x4c
	.ascii	"long int\000"
	.4byte	0x53
	.ascii	"short int\000"
	.4byte	0x5a
	.ascii	"signed char\000"
	.4byte	0x61
	.ascii	"int\000"
	.4byte	0x68
	.ascii	"long long int\000"
	.4byte	0x6f
	.ascii	"long long unsigned int\000"
	.4byte	0x7c
	.ascii	"UINT32\000"
	.4byte	0x87
	.ascii	"float\000"
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
.LASF15:
	.ascii	"SST_GetSecESChecksum\000"
.LASF14:
	.ascii	"es_sbc_enabled\000"
.LASF10:
	.ascii	"float\000"
.LASF9:
	.ascii	"long long unsigned int\000"
.LASF4:
	.ascii	"unsigned char\000"
.LASF0:
	.ascii	"long unsigned int\000"
.LASF3:
	.ascii	"short unsigned int\000"
.LASF11:
	.ascii	"GNU C 4.6.x-google 20120106 (prerelease)\000"
.LASF12:
	.ascii	""
	.ascii	""
	.ascii	""
	.ascii	"\000"
.LASF1:
	.ascii	"unsigned int\000"
.LASF2:
	.ascii	"char\000"
.LASF8:
	.ascii	"long long int\000"
.LASF13:
	.ascii	"UINT32\000"
.LASF6:
	.ascii	"short int\000"
.LASF16:
	.ascii	"pChecksum\000"
.LASF5:
	.ascii	"long int\000"
.LASF7:
	.ascii	"signed char\000"
	.ident	"GCC: (GNU) 4.6.x-google 20120106 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
