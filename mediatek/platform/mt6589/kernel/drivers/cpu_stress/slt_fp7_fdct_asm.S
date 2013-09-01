@;-------------------------------------------------------------------------------
@; The confidential and proprietary information contained in this file may
@; only be used by a person authorised under and to the extent permitted
@; by a subsisting licensing agreement from ARM Limited.
@;
@;            (C) COPYRIGHT 2009-2012  ARM Limited.
@;                ALL RIGHTS RESERVED
@;
@; This entire notice must be reproduced on all copies of this file
@; and copies of this file may only be made by a person if such person is
@; permitted to do so under the terms of a subsisting license agreement
@; from ARM Limited.
@;
@;      SVN Information
@;
@;      Checked In          : $Date: 2009-08-20 11:00:24 +0100 (Thu, 20 Aug 2009) $
@;
@;      Revision            : $Revision: 115814 $
@;
@;      Release Information : CORTEX-A7-r0p3-00rel0
@;
@;-------------------------------------------------------------------------------
@
@;-------------------------------------------------------------------------------
@;
@; Description:  Test to execute Advanced SIMD instructions in
@;               Cortex-A7 MPCore processor.
@;
@;-------------------------------------------------------------------------------
@
@       ; Include macros and test header
@                GET     test_header.hs
@
@;-------------------------------------------------------------------------------
@; Init Code
@;-------------------------------------------------------------------------------
@
@
                @AREA INIT, CODE, ALIGN=5
                .text
                .align 5
@;-------------------------------------------------------------------------------
@; Init Code
@;-------------------------------------------------------------------------------

@num_iteration   EQU     0x600
    .equ num_iteration,0x600
@table_base      EQU    0x12010000

                @ALIGN   32
                .align 5

@;*****************************************************************************
@;***    start of main test
@;*****************************************************************************

                @EXPORT fp7_fdct_start
                .global fp7_fdct_start
fp7_fdct_start:
                STMDB   sp!, {r1-r12}

                @; read CPU ID register
                MRC p15,0,r3,c0,c0,5
                AND r3,r3,#0xf
                CMP r0,r3
                BEQ cpu_id_check_done
    
                MOV r0, #0xFFFFFFFF     @target cpu check fail
                LDMIA   sp!, {r1-r12} 
                MOV     pc , r14        @return         
cpu_id_check_done:   

@;code start here
@
@;-------------------------------------------------------------------------------
@; vfp Test
@;-------------------------------------------------------------------------------

vfp_test_start:
       @; Enable FPU if present
                @FPU_PRESENT r0
                MOV     r0,#0xf00000               @Write CPACR (Coprocessor Access Control Register)
                MCR     p15,0,r0,c1,c0,2           @to enable coprocessors cp10 & cp11
                MRC     p15,0,r0,c1,c0,2           @Read CPACR
                UBFX    r0, r0, #20, #1            @if bit 20 is zero then so the bits 23:21 so FPU present
                CMP     r0, #1
                BNE     no_fpu                
@                ;LDR     r0, =test_skipped
@                ;MOV     r1, #1
@                ;STR     r1, [r0]
@                ENABLE_FPU
@
@                NEON_PRESENT r0
@                CMP     r0, #1
@                BNE     no_neon
@                MOV     r0, #1<<30
@                MCR     p10, #7, r0, c8, c0, 0   ;Enable NEON by writing FPEXC
@
@;; disable MMU
@;                DSB
@;                MRC     p15, 0, r0, c1, c0, 0       ; Read SCTLR - System Control Register
@;                BIC     r0, r0, #1                  ; Clear M bit (bit 0)
@;                MCR     p15, 0, r0, c1, c0, 0       ; Write SCTLR
@;		ISB
@
@;      ; Invalidate the I Cache
@;               MOV     r0, #0
@;               MCR     p15, 0, r0, c7, c5, 0   ; ICIALLU - Invalidate entire I Cache, and flushes branch target cache
@;      ; Invalidate the D Cache
@;               MRC     p15, 1, r0, c0, c0, 0       ; Read the Cache Size Identification register (CCSIDR)
@;               MOV     r0, r0, LSL #10
@;               MOV     r0, r0, LSR #23             ; Mask off to leave the NumSets
@;
@;               MOV     r2, #0x0                    ; Set r2 to initial MVA (Way=0, Set=0)
@;               MOV     r1, #0                      ; Use r1 as loop counter for WAYs
@;               MOV     r3, #0x0                    ; Use r3 as a loop counter for SETs
@;
@;invalidate_cache_loop
@;               MCR     p15, 0, r2, c7, c6, 2       ; DCISW - Invalidate data cache by set/way
@;               ADD     r2, r2, #0x0020             ; Increment the SET field
@;
@;               ADD     r3, r3, #1                  ; Increment loop counter
@;               CMP     r3, r0                      ; Compare loop counter with num_sets
@;               BLE     invalidate_cache_loop       ; If (loop_counter =< num_sets) branch
@;                                                   ; Prepare register for next pass
@;               ADD     r2, r2, #0x40000000         ; Increment WAY field
@;               AND     r2, r2, #0xC0000000         ; Clear the rest of the register (clear the SET field)
@;               MOV     r3, #0                      ; Reset loop counter
@;
@;               ADD     r1, r1, #1
@;               CMP     r1, #4
@;               BNE     invalidate_cache_loop
@
@      ; Invalidate TLBs
@               MCR     p15, 0, r0, c8, c7, 0                  ; TLBIALL - Invalidate entire Unified TLB
@
@; ------------------------------------------------------------
@; Set up Domain Access Control Reg
@; ------------------------------------------------------------
@       ; b00 - No Access (abort)
@       ; b01 - Client (respect table entry)
@       ; b10 - RESERVED
@       ; b11 - Manager (ignore access permissions)
@       ; Setting D0 to client, all others to No Access
@
@                LDR     r0,=0xffffffff                  ; D0-D15 = manager
@                MCR     p15, 0, r0, c3, c0, 0                  ; DACR - Domain Access Control Register
@
@; ------------------------------------------------------------
@; Set Table Base Control Register
@; ------------------------------------------------------------
@                MOV     r0,#0x0
@                MCR     p15, 0, r0, c2, c0, 2
@
@                LDR     r0, =table_base       ; Location of master copy of TLB
@                MCR     p15, 0, r0, c2, c0 ,0
@
@		LDR     r0, =0x12000000
@		LDR     r1, =0x12000000
@		LDR     r2, =0x10c0c
@                MRC     p15, 0, r3, c2, c0 ,0       ; read ttb
@                ORR     r0, r0, r2                  ; add control bits to physical address
@		ORR     r0, r0, #2
@                STR     r0,[r3, r1, LSR #18]        ; obtain MB offset from page (LSR), add to ttb and store page to this location
@
@		LDR     r0, =0x00000000
@		LDR     r1, =0x00000000
@		LDR     r2, =0x10c0c
@                MRC     p15, 0, r3, c2, c0 ,0       ; read ttb
@                ORR     r0, r0, r2                  ; add control bits to physical address
@		ORR     r0, r0, #2
@                STR     r0,[r3, r1, LSR #18]        ; obtain MB offset from page (LSR), add to ttb and store page to this location
@; SMP Enable
@  	        MRC     p15, 0, r0, c1, c0, 1
@                ORR     r0, r0, #1<<6
@                MCR     p15, 0, r0, c1, c0, 1
@
@; enable MMU
@                DSB
@                MRC     p15, 0, r0, c1, c0, 0       ; Read SCTLR - System Control Register
@                ORR     r0, r0, #0x01               ; Set M bit (bit 0)
@                MCR     p15, 0, r0, c1, c0, 0       ; Write SCTLR
@		ISB
@
@; enable I/D cache
@                MRC     p15, 0, r0, c1, c0, 0       ; Read System Control Register configuration data
@                ORR     r0, r0, #0x0004             ; Set C bit
@                ORR     r0, r0, #0x1000             ; Set I bit
@                MCR     p15, 0, r0, c1, c0, 0       ; Write System Control Register configuration data
@
                LDR     r10,=num_iteration  @; num of iterations round power_loop

                MRC   p15, 0, r0, c0, c0, 5 @; Read Multiprocessor Affinity Register
                AND   r0, r0, #0x3          @; Extract CPU ID bits
		        LDR   r1, =0x100
		        MUL   r9, r0, r1            @; produce shift amount

fdct_loop:

   LDR         r2, =input_data
   LDR         r0, =out_data
   ADD         r0, r0, r9
   LDR         r1, =0xc
   B           FDCT_Intra_inline

Coefficients:    
               @DCD     0x115016A
               .word    0x115016A
               @DCD     0xC4029D
               .word    0xC4029D
FDCT_Intra_inline:
	       ADR         r3, Coefficients
	       DMB
	       VLD1.8      {D18}, [r2], r1
	       VLD1.8      {D19}, [r2], r1
	       VLD1.8      {D20}, [r2], r1
	       VLD1.8      {D21}, [r2], r1
	       VLD1.8      {D22}, [r2], r1
	       VLD1.8      {D23}, [r2], r1
	       VLD1.8      {D24}, [r2], r1
	       VLD1.8      {D25}, [r2], r1
               VADDL.U8 Q4, D21, D22
	       VLD1.16     {D0}, [r3]
               VSUBL.U8 Q5, D21, D22
               VADDL.U8 Q3, D20, D23
               VSUBL.U8 Q6, D20, D23
               VADDL.U8 Q2, D19, D24
               VSUBL.U8 Q7, D19, D24
               VADDL.U8 Q1, D18, D25
               VSUBL.U8 Q8, D18, D25
	       VADD.I16    Q10, Q2, Q3
	       VSUB.I16    Q11, Q2, Q3
	       VADD.I16    Q9, Q1, Q4
	       VSUB.I16    Q12, Q1, Q4
	       VADD.I16    Q13, Q5, Q6
	       VADD.I16    Q14, Q6, Q7
	       VADD.I16    Q7, Q7, Q8
	       VADD.I16    Q15, Q11, Q12
	       VSUB.I16    Q4, Q13, Q7
	       VADD.I16    Q1, Q9, Q10
	       VSUB.I16    Q2, Q9, Q10
               VMULL.S16 Q11, D31, D0[0]
               VMULL.S16 Q5, D30, D0[0]
               VMULL.S16 Q3, D8, D0[3]
               VMULL.S16 Q4, D9, D0[3]
               VMULL.S16 Q15, D27, D0[1]
               VMULL.S16 Q13, D26, D0[1]
               VRSHRN.I32 D23, Q11, #8
               VRSHRN.I32 D22, Q5, #8
               VMULL.S16 Q5, D28, D0[0]
               VMULL.S16 Q6, D29, D0[0]
	       VADD.I32    Q13, Q13, Q3
	       VADD.I32    Q15, Q15, Q4
               VMLAL.S16 Q3, D14, D0[2]
               VMLAL.S16 Q4, D15, D0[2]
               VRSHRN.I32 D26, Q13, #8
               VRSHRN.I32 D27, Q15, #8
               VRSHRN.I32 D13, Q6, #8
               VRSHRN.I32 D12, Q5, #8
               VRSHRN.I32 D14, Q3, #8
               VRSHRN.I32 D15, Q4, #8
	       VSHL.I16    Q12, Q12, #1
	       VSHL.I16    Q1, Q1, #1
	       VSHL.I16    Q2, Q2, #1
	       VSHL.I16    Q8, Q8, #1
	       VADD.I16    Q3, Q11, Q12
	       VSUB.I16    Q4, Q12, Q11
	       VADD.I16    Q14, Q6, Q8
	       VSUB.I16    Q8, Q8, Q6
	       VADD.I16    Q5, Q13, Q8
	       VSHR.S16    Q11, Q3, #3
	       VSUB.I16    Q8, Q8, Q13
	       VADD.I16    Q6, Q14, Q7
	       VSUB.I16    Q7, Q14, Q7
	       VSHR.S16    Q14, Q6, #2
	       VSHL.I16    Q4, Q4, #1
	       VSRA.S16    Q5, Q5, #1
	       VSUB.I16    Q3, Q3, Q11
	       VTRN.16     Q1, Q2
	       VSUB.I16    Q6, Q6, Q14
	       VSHL.I16    Q7, Q7, #2
	       VTRN.16     D6, D8
	       VTRN.16     D7, D9
	       VTRN.16     D10, D12
	       VTRN.16     D11, D13
	       VTRN.16     D14, D16
	       VTRN.16     D15, D17
	       VTRN.32     D2, D6
	       VTRN.32     D3, D7
	       VTRN.32     D4, D8
	       VTRN.32     D5, D9
	       VTRN.32     D10, D14
	       VTRN.32     D11, D15
	       VTRN.32     D12, D16
	       VTRN.32     D13, D17
	       VSWP        D3, D10
	       VSWP        D5, D12
	       VSWP        D7, D14
	       VSWP        D9, D16
	       VADD.I16    Q11, Q3, Q6
	       VSUB.I16    Q6, Q3, Q6
	       VADD.I16    Q12, Q4, Q5
	       VSUB.I16    Q5, Q4, Q5
	       VADD.I16    Q9, Q1, Q8
	       VADD.I16    Q10, Q2, Q7
	       VSUB.I16    Q7, Q2, Q7
	       VSUB.I16    Q8, Q1, Q8
	       VADD.I16    Q1, Q9, Q12
	       VADD.I16    Q2, Q10, Q11
	       VSUB.I16    Q3, Q10, Q11
	       VSUB.I16    Q12, Q9, Q12
	       VADD.I16    Q13, Q5, Q6
	       VADD.I16    Q14, Q6, Q7
	       VADD.I16    Q7, Q7, Q8
	       VADD.I16    Q9, Q1, Q2
	       VADD.I16    Q11, Q3, Q12
	       VSUB.I16    Q4, Q13, Q7
	       VSUB.I16    Q2, Q1, Q2
               VMULL.S16 Q5, D22, D0[0]
               VMULL.S16 Q6, D23, D0[0]
               VMULL.S16 Q3, D8, D0[3]
               VMULL.S16 Q4, D9, D0[3]
               VMULL.S16 Q15, D27, D0[1]
               VMULL.S16 Q13, D26, D0[1]
	       VST1.16     {D18,D19}, [r0]!
               VRSHRN.I32 D22, Q5, #9
               VRSHRN.I32 D23, Q6, #9
               VMULL.S16 Q5, D28, D0[0]
               VMULL.S16 Q6, D29, D0[0]
	       VADD.I32    Q13, Q13, Q3
	       VADD.I32    Q15, Q15, Q4
               VMLAL.S16 Q3, D14, D0[2]
               VMLAL.S16 Q4, D15, D0[2]
	       VST1.16     {D4,D5}, [r0]!
               VRSHRN.I32 D26, Q13, #9
               VRSHRN.I32 D27, Q15, #9
               VRSHRN.I32 D13, Q6, #9
               VRSHRN.I32 D12, Q5, #9
               VRSHRN.I32 D14, Q3, #9
               VRSHRN.I32 D15, Q4, #9
	       VADD.I16    Q3, Q11, Q12
	       VSUB.I16    Q4, Q12, Q11
	       VADD.I16    Q14, Q6, Q8
	       VSUB.I16    Q8, Q8, Q6
	       VSHR.S16    Q11, Q3, #3
	       VSUB.I16    Q3, Q3, Q11
	       VADD.I16    Q5, Q13, Q8
	       VADD.I16    Q6, Q14, Q7
	       VSHL.I16    Q4, Q4, #1
	       VST1.16     {D6,D7}, [r0]!
	       VSHR.S16    Q15, Q6, #2
	       VSUB.I16    Q7, Q14, Q7
	       VSRA.S16    Q5, Q5, #1
	       VSUB.I16    Q6, Q6, Q15
	       VSHL.I16    Q7, Q7, #2
	       VST1.16     {D8,D9}, [r0]!
	       VSUB.I16    Q8, Q8, Q13
	       VST1.16     {D10,D11}, [r0]!
	       VST1.16     {D12,D13}, [r0]!
	       VST1.16     {D14,D15}, [r0]!
	       VST1.16     {D16,D17}, [r0]!

               LDR         r8, =0x10
               LDR         r0, =out_data
               ADD         r0, r0, r9
               LDR         r1, =expect_data
               MOV         r2, #0x8
check_result:
               SUB         r8, r8, #1
               LDRD        r4,r5,  [r0],r2
               LDRD        r6,r7,  [r1],r2

               CMP         r4,r6
               BNE         test_error
               CMP         r5,r7
               BNE         test_error
               
	           CMP         r8, #0
               BNE         check_result

               SUBS     r10, r10, #1	
               CMP  r10,#0
		       BNE  fdct_loop

               B    test_pass

no_fpu:
no_neon:
                MOV  r0, #0x2
		        B    test_error

test_error:      
@; disable MMU
@                DSB
@                MRC     p15, 0, r0, c1, c0, 0       ; Read SCTLR - System Control Register
@                BIC     r0, r0, #1                  ; Clear M bit (bit 0)
@                MCR     p15, 0, r0, c1, c0, 0       ; Write SCTLR
@		ISB

                MOV  r0, #0x0
                B    test_end

test_pass:       
@; disable MMU
@                DSB
@                MRC     p15, 0, r0, c1, c0, 0       ; Read SCTLR - System Control Register
@                BIC     r0, r0, #1                  ; Clear M bit (bit 0)
@                MCR     p15, 0, r0, c1, c0, 0       ; Write SCTLR
@		ISB

                MOV  r0, #0x1
                B    test_end

test_end:       LDMIA  sp!, {r1-r12} 
                BX lr


@;-------------------------------------------------------------------------------
@; Data
@;-------------------------------------------------------------------------------

                @AREA    |DATA|, DATA, READONLY, ALIGN=12
                .data
                .align 12
input_data:
@                DCB     0x5b
@                DCB     0x88
@                DCB     0x3a
@                DCB     0x3c
@                DCB     0xe1
@                DCB     0x68
@                DCB     0x20
@                DCB     0xdf
@                DCB     0x5e
@                DCB     0x93
@                DCB     0xe4
@                DCB     0x6b
@                DCB     0xb2
@                DCB     0x14
@                DCB     0xc0
@                DCB     0x82
@                DCB     0x3c
@                DCB     0x30
@                DCB     0x8c
@                DCB     0x49
@                DCB     0xa5
@                DCB     0x60
@                DCB     0x55
@                DCB     0xe7
@                DCB     0xed
@                DCB     0x17
@                DCB     0x53
@                DCB     0x25
@                DCB     0x3b
@                DCB     0x32
@                DCB     0x9a
@                DCB     0xfd
@                DCB     0xec
@                DCB     0x3c
@                DCB     0xfe
@                DCB     0xc2
@                DCB     0x38
@                DCB     0xe6
@                DCB     0x11
@                DCB     0x75
@                DCB     0x97
@                DCB     0xe3
@                DCB     0xa6
@                DCB     0x9f
@                DCB     0x7b
@                DCB     0x56
@                DCB     0xaf
@                DCB     0xee
@                DCB     0x93
@                DCB     0xb2
@                DCB     0x6c
@                DCB     0x3a
@                DCB     0xe7
@                DCB     0x17
@                DCB     0xe7
@                DCB     0xf7
@                DCB     0x3c
@                DCB     0x33
@                DCB     0xa4
@                DCB     0xe8
@                DCB     0x5a
@                DCB     0xaf
@                DCB     0x8a
@                DCB     0x4f
@                DCB     0x74
@                DCB     0xa5
@                DCB     0x5d
@                DCB     0x15
@                DCB     0x4c
@                DCB     0xec
@                DCB     0x46
@                DCB     0x87
@                DCB     0xe9
@                DCB     0x13
@                DCB     0x0a
@                DCB     0xda
@                DCB     0xf5
@                DCB     0x69
@                DCB     0x1e
@                DCB     0xdd
@                DCB     0xdf
@                DCB     0xc9
@                DCB     0x2e
@                DCB     0x33
@                DCB     0xd0
@                DCB     0x75
@                DCB     0x22
@                DCB     0x54
@                DCB     0x2e
@                DCB     0x9b
@                DCB     0xb5
@                DCB     0x7e
@                DCB     0x13
@                DCB     0xb9
@                DCB     0xa8
@                DCB     0xbf

                .byte     0x5b
                .byte     0x88
                .byte     0x3a
                .byte     0x3c
                .byte     0xe1
                .byte     0x68
                .byte     0x20
                .byte     0xdf
                .byte     0x5e
                .byte     0x93
                .byte     0xe4
                .byte     0x6b
                .byte     0xb2
                .byte     0x14
                .byte     0xc0
                .byte     0x82
                .byte     0x3c
                .byte     0x30
                .byte     0x8c
                .byte     0x49
                .byte     0xa5
                .byte     0x60
                .byte     0x55
                .byte     0xe7
                .byte     0xed
                .byte     0x17
                .byte     0x53
                .byte     0x25
                .byte     0x3b
                .byte     0x32
                .byte     0x9a
                .byte     0xfd
                .byte     0xec
                .byte     0x3c
                .byte     0xfe
                .byte     0xc2
                .byte     0x38
                .byte     0xe6
                .byte     0x11
                .byte     0x75
                .byte     0x97
                .byte     0xe3
                .byte     0xa6
                .byte     0x9f
                .byte     0x7b
                .byte     0x56
                .byte     0xaf
                .byte     0xee
                .byte     0x93
                .byte     0xb2
                .byte     0x6c
                .byte     0x3a
                .byte     0xe7
                .byte     0x17
                .byte     0xe7
                .byte     0xf7
                .byte     0x3c
                .byte     0x33
                .byte     0xa4
                .byte     0xe8
                .byte     0x5a
                .byte     0xaf
                .byte     0x8a
                .byte     0x4f
                .byte     0x74
                .byte     0xa5
                .byte     0x5d
                .byte     0x15
                .byte     0x4c
                .byte     0xec
                .byte     0x46
                .byte     0x87
                .byte     0xe9
                .byte     0x13
                .byte     0x0a
                .byte     0xda
                .byte     0xf5
                .byte     0x69
                .byte     0x1e
                .byte     0xdd
                .byte     0xdf
                .byte     0xc9
                .byte     0x2e
                .byte     0x33
                .byte     0xd0
                .byte     0x75
                .byte     0x22
                .byte     0x54
                .byte     0x2e
                .byte     0x9b
                .byte     0xb5
                .byte     0x7e
                .byte     0x13
                .byte     0xb9
                .byte     0xa8
                .byte     0xbf
expect_data:
@                DCQ     0xfc38fb4e04263de2
@                DCQ     0x00da02f8fd5d01db
@                DCQ     0xf88806d2fb86081a
@                DCQ     0x003c0e98ff450953
@                DCQ     0x05a7fe9002670986
@                DCQ     0xf4cb0a10016bfaf8
@                DCQ     0x01c606c2f81cfe74
@                DCQ     0xfd8a0060025800e4
@                DCQ     0xfd0703ebf5aafca9
@                DCQ     0xf3aaff55fe18f45d
@                DCQ     0x00810322fa83fb89
@                DCQ     0x0300fba5fec2fae9
@                DCQ     0xff64fe24fdb80274
@                DCQ     0x03b8091404c0f6c8
@                DCQ     0x016f0211019004fa
@                DCQ     0xfd87fc1cf97104ea

                .quad     0xfc38fb4e04263de2
                .quad     0x00da02f8fd5d01db
                .quad     0xf88806d2fb86081a
                .quad     0x003c0e98ff450953
                .quad     0x05a7fe9002670986
                .quad     0xf4cb0a10016bfaf8
                .quad     0x01c606c2f81cfe74
                .quad     0xfd8a0060025800e4
                .quad     0xfd0703ebf5aafca9
                .quad     0xf3aaff55fe18f45d
                .quad     0x00810322fa83fb89
                .quad     0x0300fba5fec2fae9
                .quad     0xff64fe24fdb80274
                .quad     0x03b8091404c0f6c8
                .quad     0x016f0211019004fa
                .quad     0xfd87fc1cf97104ea
                
                @ALIGN   512
                .align  9
                
out_data:        
                @DCD     0
                .word   0
                @ALIGN   256
                .align  8

out_data_cpu1:  .word   0
                .align  8
out_data_cpu2:  .word   0
                .align  8
out_data_cpu3:  .word   0
                .align  8                                
                
                @END
                .end







