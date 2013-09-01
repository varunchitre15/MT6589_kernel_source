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
@                AREA INIT, CODE, ALIGN=5
                .text
@;-------------------------------------------------------------------------------
@; Init Code
@;-------------------------------------------------------------------------------

@num_iteration   EQU     0x600
    .equ    num_iteration,0x600
@table_base      EQU    0x12010000  

@                ALIGN   32
                .align 5

@;*****************************************************************************
@;***    start of main test
@;*****************************************************************************
@
@                EXPORT fp4_vfp_func_start
                .global fp4_vfp_func_start
fp4_vfp_func_start:
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
                @ENABLE_FPU
@                MOV    r1,#1<<30
@                MCR    p10, #7, r1, c8, c0, 0                 @enable FPU by writing FPEXC
@                MOV    r1,#0
@                MCR    p10, #7, r1, c1, c0, 0                 @init FPSR
@                LDC    p11, c0, [r1], {32}                    @init registers d0-d15

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
@
@; SMP Enable
@  	        MRC     p15, 0, r0, c1, c0, 1
@                ORR     r0, r0, #1<<6
@                MCR     p15, 0, r0, c1, c0, 1
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

                LDR     r10,=num_iteration  @; num of iterations round power_loop

vfp_loop:

       @; FPU Instructions
                @;MESSAGE "- Load instructions\n"
                LDR     r3, =INPUT_DATA
                VLDMIA   r3!, {s0-s31}

       @; Coprocessor Data Processing Single
                @;MESSAGE "- Data processing instructions (SP)\n"
                VMLA.F32      s0, s0, s0
                VFMA.F32      s1, s1, s1
                VMUL.F32      s2, s2, s2
                VADD.F32      s3, s3, s3
                VDIV.F32      s4, s4, s4
                VSQRT.F32     s5, s5
                VNEG.F32      s6, s6
                VCVT.F64.F32  d3, s7
                VCVT.S32.F32  s8, s8
                VCVTT.F16.F32 s9, s9

       @; Coprocessor Data Processing Double
                @;MESSAGE "- Data processing instructions (DP)\n"
                VMLA.F64      d5, d5, d5
                CDP           p11, 10, c6, c6, c6, 2 @; VFMS.F64     d6, d6, d6
                VMUL.F64      d7, d7, d7
                VADD.F64      d8, d8, d8
                VDIV.F64      d9, d9, d9
                VSQRT.F64     d10, d10
                VNEG.F64      d11, d11
                VCVT.F32.F64  s22, d12
                VCVT.U32.F64  s24, d13

       @; Register transfer
                @;MESSAGE "- Register transfer instructions\n"
                LDR     r2, =0x01234567
                VMOV    s27, r2
                VMOV    r3, s27
                VMOV    d15, r2, r3
                VMRS    r0, FPSCR

       @; Stores
                @;MESSAGE "- Store instructions\n"
                LDR     r1, =CALCULATED_DATA
                STR     r0, [r1], #4
                VSTMIA  r1!, {s0-s31}

       @; Checks
                @;MESSAGE "- Checking results\n"
                MOV     r4, #33
                LDR     r0, =EXPECTED_DATA
                LDR     r2, =CALCULATED_DATA
check_loop:
                LDR     r1, [r0], #4
                LDR     r3, [r2], #4
                CMP     r1, r3
                BNE     test_error
                SUBS    r4, #1
                BNE     check_loop

                SUBS     r10, r10, #1	
                CMP  r10,#0
		BNE  vfp_loop

                B    test_pass

no_fpu:
                MOV  r0, #0x2
		B    test_error

test_error:      
@; disable MMU
@                DSB
@                MRC     p15, 0, r0, c1, c0, 0       ; Read SCTLR - System Control Register
@                BIC     r0, r0, #1                  ; Clear M bit (bit 0)
@                MCR     p15, 0, r0, c1, c0, 0       ; Write SCTLR
@		ISB
@
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
@; FPU Data
@;-------------------------------------------------------------------------------

CALCULATED_DATA:
                @% 256
                .SPACE 256                

                @AREA    |FPU DATA|, DATA, READONLY, ALIGN=12
                .data
                .align 12
                
                @SPACE   28
                .SPACE   28
                
                @ALIGN   64                           @; align code so exit routine is not fetched into cache
                .ALIGN   6                          
                           
INPUT_DATA:
@                DCD     0xB3341F14
@                DCD     0x3FCA6EF2
@                DCD     0xC00075F1
@                DCD     0x3FD39DDF
@                DCD     0xE0003AF8
@                DCD     0x3FE9CEEF
@                DCD     0xE0003AF8
@                DCD     0x3FE9CEEF
@                DCD     0xCCCEA48F
@                DCD     0x3FBB444B
@                DCD     0x13336E2B
@                DCD     0x3FED0223
@                DCD     0xE0003AF8
@                DCD     0x3FE9CEEF
@                DCD     0xF0001D7C
@                DCD     0x3FFCE777
@                DCD     0x00000000
@                DCD     0x00000000
@                DCD     0xFFFFFFFF                         
@                DCD     0xFFFFFFFF
@                DCD     0x00000000
@                DCD     0x00000000
@                DCD     0xFFFFFFFF
@                DCD     0xFFFFFFFF
@                DCD     0x00000000
@                DCD     0x00000000
@                DCD     0xFFFFFFFF
@                DCD     0xFFFFFFFF
@                DCD     0x00000000
@                DCD     0x00000000
@                DCD     0xFFFFFFFF
@                DCD     0xFFFFFFFF

                .word     0xB3341F14
                .word     0x3FCA6EF2
                .word     0xC00075F1
                .word     0x3FD39DDF
                .word     0xE0003AF8
                .word     0x3FE9CEEF
                .word     0xE0003AF8
                .word     0x3FE9CEEF
                .word     0xCCCEA48F
                .word     0x3FBB444B
                .word     0x13336E2B
                .word     0x3FED0223
                .word     0xE0003AF8
                .word     0x3FE9CEEF
                .word     0xF0001D7C
                .word     0x3FFCE777
                .word     0x00000000
                .word     0x00000000
                .word     0xFFFFFFFF                         
                .word     0xFFFFFFFF
                .word     0x00000000
                .word     0x00000000
                .word     0xFFFFFFFF
                .word     0xFFFFFFFF
                .word     0x00000000
                .word     0x00000000
                .word     0xFFFFFFFF
                .word     0xFFFFFFFF
                .word     0x00000000
                .word     0x00000000
                .word     0xFFFFFFFF
                .word     0xFFFFFFFF
                
EXPECTED_DATA:
@                DCD     0x00000011
@                DCD     0xB3341F14
@                DCD     0x4082A55F
@                DCD     0x4080EC4F
@                DCD     0x40539DDF
@                DCD     0x3F800000
@                DCD     0x3FACFEDE
@                DCD     0xE0000000
@                DCD     0x3FFD39DD
@                DCD     0xF98ADB88
@                DCD     0x3DDA444B
@                DCD     0x6544BF2A
@                DCD     0x3FFBA701
@                DCD     0x6A56EEE7
@                DCD     0x3FC3F97B
@                DCD     0x315ABD4C
@                DCD     0x400A1B9C
@                DCD     0x00000000
@                DCD     0x00000000
@                DCD     0xFFFFFFFF
@                DCD     0xFFFFFFFF
@                DCD     0x00000000
@                DCD     0x00000000
@                DCD     0x00000000
@                DCD     0x7FFFFFFF
@                DCD     0x00000000
@                DCD     0x00000000
@                DCD     0xFFFFFFFF
@                DCD     0x01234567
@                DCD     0x00000000
@                DCD     0x00000000
@                DCD     0x01234567
@                DCD     0x01234567

                .word     0x00000011
                .word     0xB3341F14
                .word     0x4082A55F
                .word     0x4080EC4F
                .word     0x40539DDF
                .word     0x3F800000
                .word     0x3FACFEDE
                .word     0xE0000000
                .word     0x3FFD39DD
                .word     0xF98ADB88
                .word     0x3DDA444B
                .word     0x6544BF2A
                .word     0x3FFBA701
                .word     0x6A56EEE7
                .word     0x3FC3F97B
                .word     0x315ABD4C
                .word     0x400A1B9C
                .word     0x00000000
                .word     0x00000000
                .word     0xFFFFFFFF
                .word     0xFFFFFFFF
                .word     0x00000000
                .word     0x00000000
                .word     0x00000000
                .word     0x7FFFFFFF
                .word     0x00000000
                .word     0x00000000
                .word     0xFFFFFFFF
                .word     0x01234567
                .word     0x00000000
                .word     0x00000000
                .word     0x01234567
                .word     0x01234567
                
                @END
                .end





























