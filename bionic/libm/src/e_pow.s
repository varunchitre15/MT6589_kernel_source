	.file	"e_pow.c"
	.section	.text.hot.pow,"ax",%progbits
	.align	2
	.global	pow
	.type	pow, %function
pow:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r2, #0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	.save {r4, r5, r6, r7, r8, lr}
	fmdrr	d20, r2, r3
	fstmfdd	sp!, {d8}
	.vsave {d8}
	mov	r4, r0
	mov	r5, r1
	bic	ip, r1, #-2147483648
	bic	r6, r3, #-2147483648
	beq	.L2
	movw	r7, #65534
	movt	r7, 32751
	sub	r8, r1, #1
	cmp	r8, r7
	bhi	.L3
	mov	r7, #0
	movt	r7, 17392
	cmp	r6, r7
	bgt	.L4
	mov	r1, #0
	movt	r1, 16864
	cmp	r6, r1
	bgt	.L5
	cmp	ip, #1048576
	blt	.L6
	movw	r2, #46713
	ubfx	r3, ip, #0, #20
	movt	r2, 11
	mov	ip, ip, asr #20
	cmp	r3, r2
	sub	r6, ip, #1020
	orr	r0, r3, #1069547520
	sub	ip, r6, #3
	fmdrr	d6, r4, r5
	orr	r6, r0, #3145728
	bgt	.L7
.L28:
	movw	r4, #39054
	movt	r4, 3
	cmp	r3, r4
	movle	r1, #0
	movgt	r1, #1
.L8:
	fmrrd	r2, r3, d6
	ldr	r3, .L128+140
	mov	r1, r1, asl #3
.LPIC0:
	add	r0, pc, r3
	mov	r3, r6
	add	r5, r0, r1
	fmdrr	d27, r2, r3
	fldd	d28, [r5, #0]
	fconstd	d31, #112
	faddd	d19, d27, d28
	fsubd	d24, d27, d28
	fldd	d18, .L128
	fdivd	d31, d31, d19
	fmrrd	r2, r3, d19
	mov	r2, #0
	ldr	r6, .L128+144
	fmdrr	d3, r2, r3
	ldr	r0, .L128+148
	fldd	d26, .L128+8
	fsubd	d16, d3, d28
.LPIC1:
	add	r4, pc, r6
.LPIC2:
	add	r5, pc, r0
	add	r3, r4, r1
	fsubd	d7, d27, d16
	add	r1, r5, r1
	fldd	d25, .L128+16
	fldd	d27, [r1, #0]
	fldd	d23, .L128+24
	fldd	d22, .L128+32
	fldd	d21, .L128+40
	fmsr	s9, ip	@ int
	fconstd	d30, #8
	fldd	d5, .L128+48
	fsitod	d29, s9
	fmuld	d19, d24, d31
	fldd	d17, [r3, #0]
	fldd	d6, .L128+56
	fldd	d2, .L128+64
	fmuld	d16, d19, d19
	fmrrd	r0, r1, d19
	mov	r0, r2
	faddd	d1, d29, d27
	fmrrd	r4, r5, d20
	mov	r4, r2
	fmacd	d26, d16, d18
	fmdrr	d18, r0, r1
	fmdrr	d28, r4, r5
	fmuld	d4, d16, d16
	movw	r6, #65535
	fsubd	d0, d20, d28
	fnmacd	d24, d18, d3
	movt	r6, 16527
	fmacd	d25, d26, d16
	fmuld	d26, d18, d18
	fnmacd	d24, d18, d7
	fmacd	d23, d25, d16
	faddd	d25, d26, d30
	fmuld	d24, d24, d31
	fmacd	d22, d23, d16
	fmuld	d31, d24, d19
	fmacd	d21, d22, d16
	fmacd	d31, d24, d18
	fmacd	d31, d4, d21
	faddd	d7, d25, d31
	fmsr	s14, r2	@ int
	fsubd	d23, d7, d30
	fmuld	d22, d24, d7
	fsubd	d3, d23, d26
	fmuld	d21, d18, d7
	fsubd	d30, d31, d3
	fmacd	d22, d19, d30
	faddd	d4, d21, d22
	fmsr	s8, r2	@ int
	fmacd	d17, d4, d5
	fsubd	d5, d4, d21
	fmuld	d7, d4, d6
	fsubd	d6, d22, d5
	fmacd	d17, d6, d2
	faddd	d2, d1, d7
	faddd	d1, d2, d17
	fmsr	s2, r2	@ int
	fsubd	d29, d1, d29
	fmuld	d3, d1, d28
	fsubd	d27, d29, d27
	fsubd	d19, d27, d7
	fsubd	d17, d17, d19
	fmuld	d20, d20, d17
	fmacd	d20, d1, d0
	faddd	d25, d20, d3
	fmrrd	r0, r1, d25
	bic	ip, r1, #-2147483648
	cmp	ip, r6
	mov	r5, r1
	mov	r4, r1
	bgt	.L29
.L35:
	mov	r3, #0
	movt	r3, 16352
	cmp	ip, r3
	ble	.L83
.L30:
	mov	r6, ip, asr #20
	sub	r0, r6, #1020
	sub	r5, r0, #2
	mov	ip, #1048576
	movw	r3, #65535
	add	ip, r4, ip, asr r5
	ubfx	r2, ip, #20, #11
	sub	r6, r2, #1020
	sub	r6, r6, #3
	movt	r3, 15
	mov	r0, #0
	bic	r1, ip, r3, asr r6
	rsb	r2, r2, #1040
	fmdrr	d18, r0, r1
	ubfx	r5, ip, #0, #20
	cmp	r4, #0
	orr	ip, r5, #1048576
	add	r4, r2, #3
	fsubd	d3, d3, d18
	mov	r2, ip, asr r4
	blt	.L37
	faddd	d7, d20, d3
	flds	s14, .L128+136	@ int
	fldd	d4, .L128+72
	fcpyd	d24, d7
	fldd	d31, .L128+80
	fldd	d30, .L128+88
	fsubd	d25, d24, d3
	fmuld	d3, d24, d4
	fldd	d26, .L128+96
	fsubd	d20, d20, d25
	fldd	d23, .L128+104
	fldd	d22, .L128+112
	fmacd	d3, d20, d31
	fldd	d21, .L128+120
	fldd	d5, .L128+128
	fconstd	d7, #0
	fconstd	d6, #112
	fmacd	d3, d24, d30
	fmuld	d2, d3, d3
	fcpyd	d29, d3
	fmscd	d23, d2, d26
	fmacd	d22, d23, d2
	fmscd	d21, d22, d2
	fmacd	d5, d21, d2
	fnmacd	d29, d5, d2
	fmuld	d19, d3, d29
	fsubd	d17, d29, d7
	fdivd	d1, d19, d17
	fsubd	d28, d1, d3
	fsubd	d17, d6, d28
	fmrrd	r0, r1, d17
	add	r1, r1, r2, asl #20
	cmp	r1, #1048576
	blt	.L38
.L1:
	fldmfdd	sp!, {d8}
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}
.L83:
	mov	ip, r2
.L31:
	fmrrd	r0, r1, d25
	mov	r0, #0
	fldd	d0, .L128+72
	fmdrr	d31, r0, r1
	fldd	d16, .L128+80
	fldd	d4, .L128+88
	fsubd	d23, d31, d3
	fmuld	d18, d31, d0
	fldd	d30, .L128+96
	fsubd	d22, d20, d23
	fldd	d26, .L128+104
	fldd	d24, .L128+112
	fmacd	d18, d22, d16
	fldd	d21, .L128+120
	fldd	d25, .L128+128
	fconstd	d3, #0
	fconstd	d5, #112
	fmacd	d18, d31, d4
	fmuld	d20, d18, d18
	fcpyd	d7, d18
	fmscd	d26, d20, d30
	fmacd	d24, d26, d20
	fmscd	d21, d24, d20
	fmacd	d25, d21, d20
	fnmacd	d7, d25, d20
	fmuld	d2, d18, d7
	fsubd	d29, d7, d3
	fdivd	d27, d2, d29
	fsubd	d19, d27, d18
	fsubd	d17, d5, d19
	fmrrd	r0, r1, d17
	add	r0, ip, r1
	cmp	r0, #1048576
	blt	.L38
	fmrrd	r2, r3, d17
	mov	r3, r0
	fmdrr	d20, r2, r3
.L10:
	fmrrd	r0, r1, d20
	b	.L1
.L129:
	.align	3
.L128:
	.word	1246056175
	.word	1070235176
	.word	-1815487643
	.word	1070433866
	.word	-1457700607
	.word	1070691424
	.word	1368335949
	.word	1070945621
	.word	-613438465
	.word	1071345078
	.word	858993411
	.word	1071854387
	.word	341508597
	.word	-1103220768
	.word	-536870912
	.word	1072613129
	.word	-600177667
	.word	1072613129
	.word	212364345
	.word	-1105175455
	.word	-17155601
	.word	1072049730
	.word	0
	.word	1072049731
	.word	1925096656
	.word	1046886249
	.word	-976065551
	.word	1052491073
	.word	-1356472788
	.word	1058100842
	.word	381599123
	.word	1063698796
	.word	1431655742
	.word	1069897045
	.word	0
	.word	.LANCHOR0-(.LPIC0+8)
	.word	.LANCHOR1-(.LPIC1+8)
	.word	.LANCHOR2-(.LPIC2+8)
.L2:
	cmp	r6, #0
	beq	.L79
	mov	r7, #0
	movt	r7, 32752
	cmp	ip, r7
	bgt	.L11
	beq	.L113
.L12:
	mov	r7, #0
	movt	r7, 32752
	cmp	r6, r7
	beq	.L114
	mov	r7, #0
	movt	r7, 16368
	cmp	r3, r7
	beq	.L96
	cmp	r3, #1073741824
	beq	.L115
	mov	r7, #0
	movt	r7, 16336
	cmp	r3, r7
	beq	.L116
	mov	r7, #0
	movt	r7, 16352
	cmp	r3, r7
	beq	.L117
	mov	r7, #0
	movt	r7, 16376
	cmp	r3, r7
	beq	.L118
	mov	r7, #0
	movt	r7, 49136
	cmp	r3, r7
	beq	.L119
.L19:
	movw	r7, #65534
	movt	r7, 32751
	sub	r8, r1, #1
	cmp	r8, r7
	bhi	.L3
.L26:
	mov	r7, #0
	movt	r7, 17392
	cmp	r6, r7
	bgt	.L4
	mov	r0, #0
	movt	r0, 16864
	cmp	r6, r0
	bgt	.L5
	cmp	ip, #1048576
	blt	.L6
	fmdrr	d6, r4, r5
	mov	r1, #0
.L27:
	movw	r6, #46713
	ubfx	r3, ip, #0, #20
	mov	r4, ip, asr #20
	movt	r6, 11
	sub	r0, r4, #1020
	cmp	r3, r6
	sub	r2, r0, #3
	orr	r5, r3, #1069547520
	add	ip, r2, r1
	orr	r6, r5, #3145728
	ble	.L28
.L7:
	add	ip, ip, #1
	sub	r6, r6, #1048576
	mov	r1, #0
	b	.L8
.L96:
	fmdrr	d20, r4, r5
	b	.L10
.L79:
	fconstd	d20, #112
	b	.L10
.L117:
	cmp	r1, #0
	blt	.L19
	fmdrr	d21, r4, r5
	fsqrtd	d20, d21
	fcmpd	d20, d20
	fmstat
	beq	.L10
	mov	r0, r4
	mov	r1, r5
	bl	sqrt(PLT)
	fmdrr	d20, r0, r1
	b	.L10
.L115:
	fmdrr	d16, r4, r5
	fmuld	d20, d16, d16
	b	.L10
.L116:
	cmp	r1, #0
	blt	.L19
	fmdrr	d28, r4, r5
	fsqrtd	d0, d28
	fcmpd	d0, d0
	fmstat
	bne	.L120
.L20:
	fsqrtd	d20, d0
	fcmpd	d20, d20
	fmstat
	beq	.L10
	fmrrd	r0, r1, d0
	bl	sqrt(PLT)
	fmdrr	d20, r0, r1
	b	.L10
.L118:
	cmp	r1, #0
	blt	.L19
	fmdrr	d16, r4, r5
	fsqrtd	d0, d16
	fcmpd	d0, d0
	fmstat
	bne	.L121
.L25:
	fmdrr	d17, r4, r5
	fmuld	d20, d0, d17
	b	.L10
.L120:
	mov	r0, r4
	mov	r1, r5
	bl	sqrt(PLT)
	fmdrr	d0, r0, r1
	b	.L20
.L121:
	mov	r0, r4
	mov	r1, r5
	bl	sqrt(PLT)
	fmdrr	d0, r0, r1
	b	.L25
.L113:
	cmp	r0, #0
	beq	.L12
.L11:
	fmdrr	d31, r4, r5
	faddd	d20, d20, d31
	b	.L10
.L114:
	add	r2, ip, #-1073741824
	add	r1, r2, #1048576
	orrs	r1, r1, r0
	fsubdeq	d20, d20, d20
	beq	.L10
	movw	r0, #65535
	movt	r0, 16367
	cmp	ip, r0
	ble	.L15
	cmp	r3, #0
	bge	.L10
.L16:
	fldd	d20, .L130
	b	.L10
.L15:
	cmp	r3, #0
	bge	.L16
.L105:
	fnegd	d20, d20
	b	.L10
.L3:
	cmp	r1, #0
	bne	.L4
	cmp	r0, #0
	bne	.L26
.L4:
	mov	r7, #0
	movt	r7, 32752
	cmp	ip, r7
	bgt	.L11
	beq	.L122
.L40:
	mov	r7, #0
	movt	r7, 32752
	cmp	r6, r7
	bgt	.L11
	beq	.L123
.L41:
	cmp	r1, #0
	blt	.L124
.L88:
	mov	r7, #0
.L42:
	cmp	ip, #0
	bne	.L44
	cmp	r0, #0
	bne	.L45
.L111:
	cmp	r3, #0
	blt	.L125
	cmp	r7, #1
	fmdrrne	d22, r4, r5
	fabsdne	d20, d22
	bne	.L10
	fmdrr	d25, r4, r5
	fabsd	d20, d25
	b	.L105
.L38:
	fmrrd	r0, r1, d17
	fldmfdd	sp!, {d8}
	ldmfd	sp!, {r4, r5, r6, r7, r8, lr}
	b	scalbn(PLT)
.L125:
	fmdrr	d3, r4, r5
	fconstd	d7, #112
	fabsd	d6, d3
	fdivd	d20, d7, d6
	b	.L10
.L44:
	mov	r8, #0
	movt	r8, 32752
	cmp	ip, r8
	beq	.L111
.L45:
	cmp	r1, #0
	blt	.L126
	add	r1, r6, #-1140850688
	add	r0, r1, #1048576
	cmn	r0, #-1006632959
	bhi	.L16
	fmdrr	d2, r4, r5
	fconstd	d1, #112
	fcmpd	d2, d1
	fmstat
	beq	.L96
	movw	r2, #65535
	movt	r2, 16367
	cmp	ip, r2
	bgt	.L76
.L110:
	cmp	r3, #0
	bge	.L16
.L77:
	fldd	d1, .L130+8
	fmuld	d20, d1, d1
	b	.L10
.L124:
	movw	r7, #65535
	movt	r7, 17215
	cmp	r6, r7
	movgt	r7, #2
	bgt	.L42
	movw	r7, #65535
	movt	r7, 16367
	cmp	r6, r7
	ble	.L88
	mov	r7, r6, asr #20
	movw	r8, #1043
	cmp	r7, r8
	ble	.L43
	rsb	r7, r7, #1072
	add	r8, r7, #3
	mov	r7, r2, lsr r8
	cmp	r2, r7, asl r8
	bne	.L88
.L102:
	and	r2, r7, #1
	rsb	r7, r2, #2
	b	.L42
.L76:
	cmp	r3, #0
	bgt	.L77
	b	.L16
.L131:
	.align	3
.L130:
	.word	0
	.word	0
	.word	-2013235812
	.word	2117592124
.L126:
	cmp	r7, #0
	beq	.L127
	fconstd	d3, #240
	fconstd	d18, #112
	cmp	r7, #1
	mov	r1, #0
	fmdrr	d4, r4, r5
	movt	r1, 17392
	fcpydne	d8, d18
	fcpydeq	d8, d3
	cmp	r6, r1
	fabsd	d16, d4
	bgt	.L53
	mov	r1, #0
	movt	r1, 16864
	cmp	r6, r1
	bgt	.L54
	cmp	ip, #1048576
	movge	r1, #0
	bge	.L55
	fldd	d0, .L132
	mvn	r1, #52
	fmuld	d16, d16, d0
	fmrrd	r2, r3, d16
	mov	ip, r3
.L55:
	movw	r4, #39054
	ubfx	r2, ip, #0, #20
	mov	r3, ip, asr #20
	movt	r4, 3
	cmp	r2, r4
	sub	ip, r3, #1020
	sub	r3, ip, #3
	orr	r4, r2, #1069547520
	add	r1, r3, r1
	orr	r4, r4, #3145728
	movle	r0, #0
	ble	.L56
	movw	ip, #46713
	movt	ip, 11
	cmp	r2, ip
	movle	r0, #1
	ble	.L56
	add	r1, r1, #1
	sub	r4, r4, #1048576
	mov	r0, #0
.L56:
	fmrrd	r2, r3, d16
	ldr	r3, .L132+204
	mov	r0, r0, asl #3
.LPIC3:
	add	ip, pc, r3
	mov	r3, r4
	add	ip, ip, r0
	fmdrr	d1, r2, r3
	fldd	d17, [ip, #0]
	fconstd	d21, #112
	faddd	d2, d1, d17
	fsubd	d24, d1, d17
	ldr	r4, .L132+208
	fldd	d18, .L132+8
	fdivd	d30, d21, d2
	fmrrd	r2, r3, d2
	mov	r2, #0
	fldd	d26, .L132+16
	fmdrr	d4, r2, r3
.LPIC4:
	add	r3, pc, r4
	ldr	r4, .L132+212
	add	ip, r3, r0
.LPIC5:
	add	r3, pc, r4
	add	r0, r3, r0
	fmsr	s11, r1	@ int
	fldd	d27, [r0, #0]
	fldd	d25, .L132+24
	fsubd	d3, d4, d17
	fldd	d23, .L132+32
	fldd	d22, .L132+40
	fsubd	d7, d1, d3
	fldd	d0, .L132+48
	fsitod	d28, s11
	fconstd	d29, #8
	fldd	d6, .L132+56
	fldd	d17, [ip, #0]
	fmuld	d19, d24, d30
	fldd	d1, .L132+64
	fldd	d31, .L132+72
	faddd	d21, d28, d27
	fmuld	d16, d19, d19
	fmrrd	r0, r1, d19
	mov	r0, r2
	fmacd	d26, d16, d18
	fmdrr	d18, r0, r1
	fmuld	d5, d16, d16
	fnmacd	d24, d18, d4
	fmacd	d25, d26, d16
	fmuld	d2, d18, d18
	fnmacd	d24, d18, d7
	fmacd	d23, d25, d16
	faddd	d26, d2, d29
	fmuld	d24, d24, d30
	fmacd	d22, d23, d16
	fmuld	d30, d24, d19
	fmacd	d0, d22, d16
	fmacd	d30, d24, d18
	fmacd	d30, d5, d0
	faddd	d7, d26, d30
	fmsr	s14, r2	@ int
	fsubd	d25, d7, d29
	fmuld	d22, d24, d7
	fsubd	d23, d25, d2
	fmuld	d4, d18, d7
	fsubd	d0, d30, d23
	fmacd	d22, d19, d0
	faddd	d5, d4, d22
	fmsr	s10, r2	@ int
	fcpyd	d29, d5
	fmacd	d17, d29, d6
	fsubd	d3, d29, d4
	fmuld	d6, d29, d31
	fsubd	d7, d22, d3
	fmacd	d17, d7, d1
	faddd	d1, d21, d6
	faddd	d7, d1, d17
	fmsr	s14, r2	@ int
	fcpyd	d30, d7
	fsubd	d31, d30, d28
	fsubd	d28, d31, d27
	fsubd	d27, d28, d6
	fsubd	d19, d17, d27
.L57:
	fmrrd	r0, r1, d20
	mov	r0, #0
	fmuld	d17, d20, d19
	fmdrr	d19, r0, r1
	movw	r4, #65535
	movt	r4, 16527
	fsubd	d20, d20, d19
	fmuld	d4, d19, d30
	fmacd	d17, d20, d30
	faddd	d30, d17, d4
	fmrrd	r2, r3, d30
	bic	ip, r3, #-2147483648
	cmp	ip, r4
	ble	.L62
	cmp	r3, r4
	ble	.L63
	add	r1, r3, #-1090519040
	add	r0, r1, #7340032
	orrs	r1, r0, r2
	bne	.L104
	fldd	d5, .L132+80
	fsubd	d2, d30, d4
	faddd	d26, d17, d5
	fcmped	d26, d2
	fmstat
	bgt	.L104
.L65:
	mov	r6, ip, asr #20
	sub	r2, r6, #1020
	sub	r4, r2, #2
	mov	ip, #1048576
	movw	r5, #65535
	add	ip, r3, ip, asr r4
	ubfx	r6, ip, #20, #11
	sub	r2, r6, #1020
	sub	r4, r2, #3
	movt	r5, 15
	mov	r0, #0
	bic	r1, ip, r5, asr r4
	ubfx	r2, ip, #0, #20
	fmdrr	d24, r0, r1
	rsb	r6, r6, #1040
	cmp	r3, #0
	orr	ip, r2, #1048576
	fsubd	d4, d4, d24
	add	r3, r6, #3
	mov	r2, ip, asr r3
	faddd	d30, d17, d4
	rsblt	r2, r2, #0
.L69:
	fmrrd	r4, r5, d30
	mov	r4, #0
	fldd	d0, .L132+88
	fmdrr	d22, r4, r5
	fldd	d23, .L132+96
	fldd	d29, .L132+104
	fsubd	d6, d22, d4
	fmuld	d7, d22, d0
	fldd	d25, .L132+112
	fsubd	d1, d17, d6
	fldd	d3, .L132+120
	fldd	d20, .L132+128
	fmacd	d7, d1, d23
	fldd	d31, .L132+136
	fldd	d28, .L132+144
	fconstd	d27, #0
	fconstd	d21, #112
	fmacd	d7, d22, d29
	fmuld	d17, d7, d7
	fcpyd	d19, d7
	fmscd	d3, d17, d25
	fmacd	d20, d3, d17
	fmscd	d31, d20, d17
	fmacd	d28, d31, d17
	fnmacd	d19, d28, d17
	fmuld	d18, d7, d19
	fsubd	d5, d19, d27
	fdivd	d2, d18, d5
	fsubd	d26, d2, d7
	fsubd	d24, d21, d26
	fmrrd	r4, r5, d24
	add	r1, r5, r2, asl #20
	cmp	r1, #1048576
	fmrrdge	r2, r3, d24
	movge	r3, r1
	fmdrrge	d20, r2, r3
	bge	.L72
	fmrrd	r0, r1, d24
	bl	scalbn(PLT)
	fmdrr	d20, r0, r1
.L72:
	fmuld	d20, d20, d8
	b	.L10
.L73:
	cmp	r3, #0
	ble	.L75
.L104:
	fldd	d4, .L132+152
	fmuld	d30, d8, d4
	fmuld	d20, d30, d4
	b	.L10
.L63:
	movw	r1, #52223
	movt	r1, 16528
	cmp	ip, r1
	ble	.L65
	add	r0, r3, #1061158912
	add	r1, r0, #3080192
	add	r0, r1, #13312
	orrs	r1, r0, r2
	bne	.L75
	fsubd	d21, d30, d4
	fcmped	d17, d21
	fmstat
	bhi	.L65
.L75:
	fldd	d16, .L132+160
	fmuld	d18, d8, d16
	fmuld	d20, d18, d16
	b	.L10
.L62:
	mov	r5, #0
	movt	r5, 16352
	cmp	ip, r5
	movle	r2, r0
	ble	.L69
	b	.L65
.L54:
	movw	r0, #65534
	movt	r0, 16367
	cmp	ip, r0
	ble	.L109
	mov	r2, #0
	movt	r2, 16368
	cmp	ip, r2
	bgt	.L73
	fsubd	d5, d16, d18
	fconstd	d6, #80
	fldd	d7, .L132+168
	fconstd	d24, #96
	fnmacd	d7, d5, d6
	fldd	d25, .L132+176
	fmuld	d22, d5, d5
	fldd	d26, .L132+184
	fldd	d23, .L132+192
	fnmacd	d24, d7, d5
	fmuld	d27, d5, d23
	fmuld	d28, d22, d24
	fmuld	d29, d28, d25
	fmscd	d29, d5, d26
	faddd	d7, d27, d29
	flds	s14, .L132+200	@ int
	fcpyd	d30, d7
	fsubd	d31, d30, d27
	fsubd	d19, d29, d31
	b	.L57
.L133:
	.align	3
.L132:
	.word	0
	.word	1128267776
	.word	1246056175
	.word	1070235176
	.word	-1815487643
	.word	1070433866
	.word	-1457700607
	.word	1070691424
	.word	1368335949
	.word	1070945621
	.word	-613438465
	.word	1071345078
	.word	858993411
	.word	1071854387
	.word	341508597
	.word	-1103220768
	.word	-600177667
	.word	1072613129
	.word	-536870912
	.word	1072613129
	.word	1697350398
	.word	1016534343
	.word	212364345
	.word	-1105175455
	.word	-17155601
	.word	1072049730
	.word	0
	.word	1072049731
	.word	1925096656
	.word	1046886249
	.word	-976065551
	.word	1052491073
	.word	-1356472788
	.word	1058100842
	.word	381599123
	.word	1063698796
	.word	1431655742
	.word	1069897045
	.word	-2013235812
	.word	2117592124
	.word	-1023872167
	.word	27618847
	.word	1431655765
	.word	1070945621
	.word	1697350398
	.word	1073157447
	.word	-128065724
	.word	1045736971
	.word	1610612736
	.word	1073157447
	.word	0
	.word	.LANCHOR0-(.LPIC3+8)
	.word	.LANCHOR1-(.LPIC4+8)
	.word	.LANCHOR2-(.LPIC5+8)
.L53:
	add	r0, r6, #-1140850688
	add	r2, r0, #1048576
	cmn	r2, #-1006632959
	bhi	.L16
	fcmpd	d4, d3
	fmdrr	d20, r4, r5
	fmstat
	fcpydeq	d20, d8
	beq	.L10
	movw	r1, #65535
	movt	r1, 16367
	cmp	ip, r1
	bgt	.L73
.L109:
	cmp	r3, #0
	bge	.L75
	b	.L104
.L127:
	fmdrr	d0, r4, r5
	fsubd	d29, d0, d0
	fdivd	d20, d29, d29
	b	.L10
.L43:
	cmp	r2, #0
	bne	.L88
	rsb	r8, r7, #1040
	add	r8, r8, #3
	mov	r7, r6, asr r8
	cmp	r6, r7, asl r8
	movne	r7, r2
	bne	.L42
	b	.L102
.L123:
	cmp	r2, #0
	beq	.L41
	b	.L11
.L122:
	cmp	r0, #0
	beq	.L40
	b	.L11
.L37:
	rsb	r2, r2, #0
	faddd	d25, d20, d3
	mov	ip, r2, asl #20
	b	.L31
.L6:
	fmdrr	d1, r4, r5
	fldd	d7, .L134
	mvn	r1, #52
	fmuld	d6, d1, d7
	fmrrd	r2, r3, d6
	mov	ip, r3
	b	.L27
.L5:
	movw	r2, #65534
	movt	r2, 16367
	cmp	ip, r2
	ble	.L110
	mov	r1, #0
	movt	r1, 16368
	cmp	ip, r1
	bgt	.L76
	fmdrr	d27, r4, r5
	fconstd	d1, #112
	fconstd	d31, #80
	fldd	d28, .L134+8
	fsubd	d19, d27, d1
	fconstd	d16, #96
	fldd	d17, .L134+16
	fnmacd	d28, d19, d31
	fldd	d5, .L134+24
	fmuld	d18, d19, d19
	fldd	d21, .L134+32
	fmuld	d2, d19, d21
	fmrrd	r0, r1, d20
	movw	r5, #65535
	fnmacd	d16, d28, d19
	movt	r5, 16527
	fmuld	d26, d18, d16
	fmuld	d24, d26, d17
	fmscd	d24, d19, d5
	faddd	d4, d2, d24
	fmrrd	r2, r3, d4
	mov	r2, #0
	mov	r0, r2
	fmdrr	d30, r2, r3
	fmdrr	d0, r0, r1
	fsubd	d23, d30, d2
	fsubd	d29, d20, d0
	fmuld	d3, d30, d0
	fsubd	d22, d24, d23
	fmuld	d20, d20, d22
	fmacd	d20, d30, d29
	faddd	d25, d20, d3
	fmrrd	r0, r1, d25
	bic	ip, r1, #-2147483648
	cmp	ip, r5
	mov	r4, r1
	mov	r5, r1
	ble	.L35
.L29:
	movw	r2, #65535
	movt	r2, 16527
	cmp	r4, r2
	ble	.L36
	add	r1, r4, #-1090519040
	add	r3, r1, #7340032
	orrs	r3, r3, r0
	bne	.L77
	fldd	d28, .L134+40
	fsubd	d16, d25, d3
	faddd	d0, d20, d28
	fcmped	d0, d16
	fmstat
	ble	.L30
	b	.L77
.L119:
	fmdrr	d19, r4, r5
	fconstd	d20, #112
	fdivd	d20, d20, d19
	b	.L10
.L36:
	movw	r1, #52223
	movt	r1, 16528
	cmp	ip, r1
	ble	.L30
	mov	r3, #13312
	movt	r3, 16239
	add	r2, r5, r3
	orrs	r3, r2, r0
	bne	.L16
	fsubd	d1, d25, d3
	fcmped	d20, d1
	fmstat
	bhi	.L30
	b	.L16
.L135:
	.align	3
.L134:
	.word	0
	.word	1128267776
	.word	1431655765
	.word	1070945621
	.word	1697350398
	.word	1073157447
	.word	-128065724
	.word	1045736971
	.word	1610612736
	.word	1073157447
	.word	1697350398
	.word	1016534343
	.fnend
	.size	pow, .-pow
	.section	.rodata.dp_h,"a",%progbits
	.align	3
.LANCHOR2 = . + 0
	.type	dp_h, %object
	.size	dp_h, 16
dp_h:
	.word	0
	.word	0
	.word	1073741824
	.word	1071822851
	.section	.rodata.dp_l,"a",%progbits
	.align	3
.LANCHOR1 = . + 0
	.type	dp_l, %object
	.size	dp_l, 16
dp_l:
	.word	0
	.word	0
	.word	1137692678
	.word	1045233131
	.section	.rodata.bp,"a",%progbits
	.align	3
.LANCHOR0 = . + 0
	.type	bp, %object
	.size	bp, 16
bp:
	.word	0
	.word	1072693248
	.word	0
	.word	1073217536
	.ident	"GCC: (crosstool-NG linaro-1.13.1-4.7-2012.10-20121022 - Linaro GCC 2012.10) 4.7.3 20121001 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
