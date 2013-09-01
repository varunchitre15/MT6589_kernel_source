@;-----------------------------------------------------------------------------
@; The confidential and proprietary information contained in this file may
@; only be used by a person authorised under and to the extent permitted
@; by a subsisting licensing agreement from ARM Limited.
@;
@;            (C) COPYRIGHT 2007-2010 ARM Limited.
@;                ALL RIGHTS RESERVED
@;
@; This entire notice must be reproduced on all copies of this file
@; and copies of this file may only be made by a person if such person is
@; permitted to do so under the terms of a subsisting license agreement
@; from ARM Limited.
@;
@;      RCS Information
@;
@;      RCS Filename        : $RCSfile$
@;
@;      Checked In          : $Date$
@;
@;      Revision            : $Revision: 73865 $
@;
@;      Release Information : r3p0-00rel0
@;-----------------------------------------------------------------------------
@;
@;;-----------------------------------------------------------------------------
@;;  Find configuration of CPU
@;;  Variable PERIPH_BASE needs to be updated according to real mapping
@;;-----------------------------------------------------------------------------





                @AREA INIT, CODE, ALIGN=5
                .text
@;                ENTRY
@;
@;                LDR     PC, Reset_Addr
@;                NOP                     ; reserved vector
@;                NOP                     ; reserved vector
@;                NOP                     ; reserved vector
@;                NOP                     ; reserved vector
@;                NOP                     ; reserved vector
@;                NOP                     ; reserved vector
@;                NOP                     ; reserved vector

@num_iteration   EQU     0x4000
    .equ num_iteration,0x4000
    
    .extern pMaxPowerTestMem       
test_mem:
    .word pMaxPowerTestMem
    .equ test_pattern,0x5a5a5a5a         
    
                @ALIGN   32
                .align 5

@;*****************************************************************************
@;***    start of main test
@;*****************************************************************************       
                @EXPORT fp0_maxpower_start
                .global fp0_maxpower_start
fp0_maxpower_start:
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

                @ALIGN 32 ; align to $ line
                .align 5
@;code start here
@;; disable MMU
@;                DSB
@;                MRC     p15, 0, r0, c1, c0, 0       ; Read SCTLR - System Control Register
@;                BIC     r0, r0, #1                  ; Clear M bit (bit 0)
@;                MCR     p15, 0, r0, c1, c0, 0       ; Write SCTLR
@;		ISB

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
@		LDR     r0, =0x82000000
@		LDR     r1, =0x12000000
@		LDR     r2, =0x10c0c
@                MRC     p15, 0, r3, c2, c0 ,0       @; read ttb
@                ORR     r0, r0, r2                  @; add control bits to physical address
@		ORR     r0, r0, #2
@                STR     r0,[r3, r1, LSR #18]        @; obtain MB offset from page (LSR), add to ttb and store page to this location
@
@		LDR     r0, =0x80000000
@		LDR     r1, =0x00000000
@		LDR     r2, =0x10c0c
@                MRC     p15, 0, r3, c2, c0 ,0       @; read ttb
@                ORR     r0, r0, r2                  @; add control bits to physical address
@		ORR     r0, r0, #2
@                STR     r0,[r3, r1, LSR #18]        @; obtain MB offset from page (LSR), add to ttb and store page to this location
@
@		LDR     r0, =0x8ff00000
@		LDR     r1, =0xfff00000
@		LDR     r2, =0x10c0c
@                MRC     p15, 0, r3, c2, c0 ,0       @; read ttb
@                ORR     r0, r0, r2                  @; add control bits to physical address
@		ORR     r0, r0, #2
@                STR     r0,[r3, r1, LSR #18]        @; obtain MB offset from page (LSR), add to ttb and store page to this location

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

                LDR     r9,test_mem
                LDR     r9,[r9]
                ADD     r9,r9,#0x1000
                
                @LDR     r0,=0x00000000
                @LDR     r1,=0xfffffffd
                @LDR     r2,=0xfffffffc             
                @LDR     r3,=0x00000006
                @LDR     r4,=0x00000000
                @LDR     r5,=0xfffffffd
                @LDR     r6,=0xfffffffc
                @LDR     r7,=0x00000006

                ADD     r0, r9, #0x00000000
                ADD     r1, r9, #0xfffffffd
                ADD     r2, r9, #0xfffffffc
                ADD     r3, r9, #0x00000006
                ADD     r4, r9, #0x00000000
                ADD     r5, r9, #0xfffffffd
                ADD     r6, r9, #0xfffffffc
                ADD     r7, r9, #0x00000006
                
				LDR		r10, =test_pattern
				SUB     r7, r9              @add by KT
				STR     r10, [r6,+r7]!
				ADD     r7, r9              @add by KT	
                LDR     r10,=num_iteration  @; num of iterations round power_loop
power_loop:
                SUBS     r10, r10, #1	

                @; Group-1
                SUB     r1, r9, LSR #1       @add by KT      
                SUB     r0, r9               @add by KT                                           
                LDR     r8, [r0,+r1,LSL #1]! @; Dual Issue Pair 0, Instruction 0                
                MOV     r6, r2               @; Dual Issue Pair 0, Instruction 1
                ADD     r1, r9, LSR #1       @add by KT  

                SUB     r3, r9               @add by KT
                LDR     r8, [r2,+r3]!        @; Dual Issue Pair 1, Instruction 0 
                ADD     r0, r1, #0x003       @; Dual Issue Pair 1, Instruction 1
                ADD     r3, r9               @add by KT

                SUB     r5, r9, LSR #1       @add by KT
                SUB     r4, r9               @add by KT
                LDR     r8, [r4,+r5,LSL #1]! @; Dual Issue Pair 2, Instruction 0                
                SUB     r2, r3, #0x00a       @; Dual Issue Pair 2, Instruction 1
                ADD     r5, r9, LSR #1       @add by KT

                SUB     r7, r9               @add by KT
                LDR     r8, [r6,+r7]!        @; Dual Issue Pair 3, Instruction 0
                ADD     r4, r5, #0x003       @; Dual Issue Pair 3, Instruction 1
                ADD     r7, r9               @add by KT

                @; Group-2
                SUB     r1, r9, LSR #1       @add by KT
                SUB     r0, r9               @add by KT
                LDR     r8, [r0,+r1,LSL #1]! @; Dual Issue Pair 0, Instruction 0                
                MOV     r6, r2               @; Dual Issue Pair 0, Instruction 1
                ADD     r1, r9, LSR #1       @add by KT

                SUB     r3, r9               @add by KT
                LDR     r8, [r2,+r3]!        @; Dual Issue Pair 1, Instruction 0
                ADD     r0, r1, #0x003       @; Dual Issue Pair 1, Instruction 1
                ADD     r3, r9               @add by KT

                SUB     r5, r9, LSR #1       @add by KT
                SUB     r4, r9               @add by KT
                LDR     r8, [r4,+r5,LSL #1]! @; Dual Issue Pair 2, Instruction 0
                SUB     r2, r3, #0x00a       @; Dual Issue Pair 2, Instruction 1
                ADD     r5, r9, LSR #1       @add by KT

                SUB     r7, r9               @add by KT
                LDR     r8, [r6,+r7]!        @; Dual Issue Pair 3, Instruction 0
                ADD     r4, r5, #0x003       @; Dual Issue Pair 3, Instruction 1
                ADD     r7, r9               @add by KT

                @; Group-3
                SUB     r1, r9, LSR #1       @add by KT
                SUB     r0, r9               @add by KT
                LDR     r8, [r0,+r1,LSL #1]! @; Dual Issue Pair 0, Instruction 0 
                MOV     r6, r2               @; Dual Issue Pair 0, Instruction 1
                ADD     r1, r9, LSR #1       @add by KT

                SUB     r3, r9               @add by KT
                LDR     r8, [r2,+r3]!        @; Dual Issue Pair 1, Instruction 0
                ADD     r0, r1, #0x003       @; Dual Issue Pair 1, Instruction 1
                ADD     r3, r9               @add by KT

                SUB     r5, r9, LSR #1       @add by KT
                SUB     r4, r9               @add by KT
                LDR     r8, [r4,+r5,LSL #1]! @; Dual Issue Pair 2, Instruction 0
                SUB     r2, r3, #0x00a       @; Dual Issue Pair 2, Instruction 1
                ADD     r5, r9, LSR #1       @add by KT

                SUB     r7, r9               @add by KT
                LDR     r8, [r6,+r7]!        @; Dual Issue Pair 3, Instruction 0
                ADD     r4, r5, #0x003       @; Dual Issue Pair 3, Instruction 1
                ADD     r7, r9               @add by KT

                @; Group-4
                SUB     r1, r9, LSR #1       @add by KT
                SUB     r0, r9               @add by KT
                LDR     r8, [r0,+r1,LSL #1]! @; Dual Issue Pair 0, Instruction 0 
                MOV     r6, r2               @; Dual Issue Pair 0, Instruction 1
                ADD     r1, r9, LSR #1       @add by KT

                SUB     r3, r9               @add by KT
                LDR     r8, [r2,+r3]!        @; Dual Issue Pair 1, Instruction 0
                ADD     r0, r1, #0x003       @; Dual Issue Pair 1, Instruction 1
                ADD     r3, r9               @add by KT

                SUB     r5, r9, LSR #1       @add by KT
                SUB     r4, r9               @add by KT
                LDR     r8, [r4,+r5,LSL #1]! @; Dual Issue Pair 2, Instruction 0
                SUB     r2, r3, #0x00a       @; Dual Issue Pair 2, Instruction 1
                ADD     r5, r9, LSR #1       @add by KT

                SUB     r7, r9               @add by KT
                LDR     r8, [r6,+r7]!        @; Dual Issue Pair 3, Instruction 0
                ADD     r4, r5, #0x003       @; Dual Issue Pair 3, Instruction 1
                ADD     r7, r9               @add by KT		

		BNE	power_loop

                @LDR     r1,=0xfffffffc       @; correct of r2
                ADD     r1, r9, #0xfffffffc        @; correct of r2
                CMP     r1,r2
		BNE     test_error
                @LDR     r3,=0x0              @; correct of r4
                ADD     r3, r9, #0x0               @; correct of r4
                CMP     r3,r4
		BNE     test_error
                @LDR     r5,=0x2              @; correct of r6
                ADD     r5, r9, #0x2               @; correct of r6
                CMP     r6,r5
		BNE     test_error
                LDR     r7,=test_pattern      @; correct of r8               
                CMP     r7,r8
		BNE     test_error

                B       test_pass

test_error:      
@; disable MMU
@                DSB
@                MRC     p15, 0, r0, c1, c0, 0       ; Read SCTLR - System Control Register
@                BIC     r0, r0, #1                  ; Clear M bit (bit 0)
@                MCR     p15, 0, r0, c1, c0, 0       ; Write SCTLR
		ISB

                MOV  r0, #0x0
                B    test_end

test_pass:       
@; disable MMU
@                DSB
@                MRC     p15, 0, r0, c1, c0, 0       ; Read SCTLR - System Control Register
@                BIC     r0, r0, #1                  ; Clear M bit (bit 0)
@                MCR     p15, 0, r0, c1, c0, 0       ; Write SCTLR
		ISB

                MOV  r0, #0x1
                B    test_end

test_end:        
                LDMIA  sp!, {r1-r12}
                BX lr


@                ALIGN   32
@;-------------------------------------------------------------------------------
@; Pagetable Data
@;-------------------------------------------------------------------------------
@                AREA    TT0,DATA,READONLY, ALIGN=14
@table_base
@                % 16384

                @END
                .end
