	.section	.text.bcopy,"ax",%progbits
	.align	2
	.global	bcopy
	.type	bcopy, %function
bcopy:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	.movsp ip
	mov	ip, sp
	cmp	r1, r0
	cmpne	r2, #0
	stmfd	sp!, {r4, r5, r6, r7, r8, fp, ip, lr, pc}
	.pad #4
	.save {r4, r5, r6, r7, r8, fp, ip, lr}
	.setfp fp, ip, #-4
	sub	fp, ip, #4
	movne	r5, #0
	moveq	r5, #1
	ldmeqfd	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
	cmp	r1, r0
	mov	ip, r1
	mov	r4, r0
	bcs	.L3
	orr	r3, r1, r0
	tst	r3, #3
	moveq	r6, r2, lsr #2
	andeq	r5, r2, #3
	beq	.L5
	eor	r8, r1, r0
	ands	r6, r8, #3
	bne	.L51
	cmp	r2, #3
	bhi	.L54
	mov	r5, r6
.L51:
	mov	r7, r2
	mov	r6, r5
.L6:
	add	r8, r1, #16
	add	r3, r0, #16
	cmp	r1, r3
	cmpcc	r0, r8
	movcc	r2, #0
	movcs	r2, #1
	cmp	r7, #15
	movls	r2, #0
	andhi	r2, r2, #1
	eor	r3, r2, #1
	mov	r2, r7, lsr #4
	cmp	r2, #0
	orreq	r3, r3, #1
	cmp	r3, #0
	mov	r8, r2, asl #4
	bne	.L27
.L8:
	add	r3, r3, #1
	vld1.8	{q8}, [r4]!
	cmp	r3, r2
	vst1.8	{q8}, [ip]!
	bcc	.L8
	cmp	r7, r8
	add	r3, r1, r8
	add	ip, r0, r8
	rsb	r4, r8, r7
	beq	.L9
.L7:
	sub	r2, ip, #1
	add	r4, r3, r4
.L10:
	ldrb	ip, [r2, #1]!	@ zero_extendqisi2
	strb	ip, [r3], #1
	cmp	r3, r4
	bne	.L10
.L9:
	add	r1, r1, r7
	add	r0, r0, r7
.L5:
	cmp	r6, #0
	beq	.L11
	add	r7, r0, #16
	add	r4, r1, #16
	cmp	r1, r7
	cmpcc	r0, r4
	movcc	r3, #0
	movcs	r3, #1
	cmp	r6, #3
	movls	r3, #0
	andhi	r3, r3, #1
	eor	ip, r3, #1
	mov	r7, r6, lsr #2
	cmp	r7, #0
	orreq	ip, ip, #1
	cmp	ip, #0
	mov	r3, r7, asl #2
	bne	.L28
	mov	r4, r0
	mov	r2, r1
.L13:
	add	ip, ip, #1
	vld1.32	{q0}, [r4]!
	cmp	ip, r7
	vst1.32	{q0}, [r2]!
	bcc	.L13
	mov	r4, r3, asl #2
	cmp	r6, r3
	add	r2, r1, r4
	rsb	r3, r3, r6
	add	r4, r0, r4
	beq	.L14
.L12:
	add	r4, r4, #4
	add	ip, r2, #4
.L15:
	ldr	r2, [r4, #-4]
	subs	r3, r3, #1
	str	r2, [ip, #-4]
	add	r4, r4, #4
	add	ip, ip, #4
	bne	.L15
.L14:
	mov	r6, r6, asl #2
	add	r0, r0, r6
	add	r1, r1, r6
.L11:
	cmp	r5, #0
	ldmeqfd	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
	sub	r0, r0, #1
	add	r5, r1, r5
.L17:
	ldrb	r2, [r0, #1]!	@ zero_extendqisi2
	strb	r2, [r1], #1
	cmp	r1, r5
	bne	.L17
	ldmfd	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
.L3:
	add	r0, r0, r2
	add	r1, r1, r2
	orr	r3, r1, r0
	tst	r3, #3
	moveq	r7, r2, lsr #2
	andeq	r5, r2, #3
	beq	.L19
	eor	r4, r1, r0
	ands	r6, r4, #3
	bne	.L52
	cmp	r2, #4
	bhi	.L55
	mov	r5, r6
.L52:
	mov	r6, r2
	mov	r7, r5
.L20:
	mov	r3, r0
	mov	ip, r1
	rsb	r2, r6, r0
.L21:
	ldrb	r4, [r3, #-1]!	@ zero_extendqisi2
	cmp	r3, r2
	strb	r4, [ip, #-1]!
	bne	.L21
	rsb	r6, r6, #0
	add	r1, r1, r6
	add	r0, r0, r6
.L19:
	cmp	r7, #0
	beq	.L22
	mov	r4, r1
	mov	ip, r0
	mov	r3, r7
.L23:
	ldr	r6, [ip, #-4]!
	subs	r3, r3, #1
	str	r6, [r4, #-4]!
	bne	.L23
	rsb	r7, r7, r7, asl #30
	mov	r7, r7, asl #2
	add	r0, r0, r7
	add	r1, r1, r7
.L22:
	cmp	r5, #0
	ldmeqfd	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
	rsb	r2, r5, r0
.L24:
	ldrb	ip, [r0, #-1]!	@ zero_extendqisi2
	cmp	r0, r2
	strb	ip, [r1, #-1]!
	bne	.L24
	ldmfd	sp, {r4, r5, r6, r7, r8, fp, sp, pc}
.L55:
	and	r6, r0, #3
	rsb	r2, r6, r2
	mov	r7, r2, lsr #2
	and	r5, r2, #3
	b	.L20
.L54:
	and	r5, r0, #3
	rsb	r7, r5, #4
	rsb	r2, r7, r2
	mov	r6, r2, lsr #2
	and	r5, r2, #3
	b	.L6
.L28:
	mov	r3, r6
	mov	r4, r0
	mov	r2, r1
	b	.L12
.L27:
	mov	r4, r7
	mov	ip, r0
	mov	r3, r1
	b	.L7
	.fnend
	.size	bcopy, .-bcopy
	.ident	"GCC: (Linaro GCC 4.7-2012.11) 4.7.3 20121106 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
