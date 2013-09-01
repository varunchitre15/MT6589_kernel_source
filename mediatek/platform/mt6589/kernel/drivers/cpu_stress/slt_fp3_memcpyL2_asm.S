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




@		GET		Mem-Macros
@
@                AREA INIT, CODE, ALIGN=5
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

@num_iteration   EQU     0x100
    .equ    num_iteration,0x100
@num_init        EQU     0x0
    .equ    num_init,0x0
@index_iteration  EQU   0x1FF ; loop to access index
    .equ    index_iteration,0x1FF
@addr_iteration  EQU   (index_iteration + 1) << 5 ;
    .equ    addr_iteration, ((index_iteration + 1) << 5)
@;L1_MEMORY       EQU     0x89000000
@;COPY_SIZE       EQU     0x00004000
@COPY_SIZE       EQU     0x00008000
    .equ    COPY_SIZE,0x00008000
@; 0x00004000 --> 16K L1 memory copy
@; lower 16K memory is copied to upper 16K memory

@Cache_Line_Size		EQU		       0x40
    .equ    Cache_Line_Size,0x40
@table_base      EQU    0x12008000
@SP_ADDR         EQU    0x1200E000
@                ALIGN   32
                .align 5

    .extern g_iTestMem_CPU0
    .extern g_iTestMem_CPU1
    .extern g_iTestMem_CPU2
    .extern g_iTestMem_CPU3

nADDR_CPU0:
    .word g_iTestMem_CPU0

nADDR_CPU1:
    .word g_iTestMem_CPU1

nADDR_CPU2:
    .word g_iTestMem_CPU2

nADDR_CPU3:
    .word g_iTestMem_CPU3
    
@;*****************************************************************************
@;***    start of main test
@;*****************************************************************************

@                EXPORT fp3_memcpyL2_start
                .global fp3_memcpyL2_start
fp3_memcpyL2_start:
@                LDR  r13, =SP_ADDR
                STMDB   sp!, {r1-r12}
@                ALIGN 32 ; align to $ line
                .align 5

                @; read CPU ID register
                MRC p15,0,r3,c0,c0,5
                AND r3,r3,#0xf
                CMP r0,r3
                BEQ cpu_id_check_done
    
                MOV r0, #0xFFFFFFFF     @target cpu check fail
                LDMIA   sp!, {r1-r12} 
                MOV     pc , r14        @return         
cpu_id_check_done:                

@		MOV  r0, PC
@		LDR  r1, =0x0000FFFF
@		BIC  r2,r0,r1
@		LDR  r3, =0x00010000
@		ADD  r3, r3 ,r2
@		LDR  r4, =PC_sync_L1_MEMORY
@		STR  r3, [r4]
@;code start here
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
@
@		LDR     r0, =0x00100000
@		LDR     r1, =0x00100000
@		LDR     r2, =0x10c0c
@                MRC     p15, 0, r3, c2, c0 ,0       ; read ttb
@                ORR     r0, r0, r2                  ; add control bits to physical address
@		ORR     r0, r0, #2
@                STR     r0,[r3, r1, LSR #18]        ; obtain MB offset from page (LSR), add to ttb and store page to this location
@
@		;LDR     r0, =0xfff00000
@		;LDR     r1, =0xfff00000
@		;LDR     r2, =0x10c0c
@                ;MRC     p15, 0, r3, c2, c0 ,0       ; read ttb
@                ;ORR     r0, r0, r2                  ; add control bits to physical address
@		;ORR     r0, r0, #2
@                ;STR     r0,[r3, r1, LSR #18]        ; obtain MB offset from page (LSR), add to ttb and store page to this location
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
@
                LDR     r10,=num_iteration      @; num of iterations round power_loop, it should not be edited by other task

@                ; turn off d cache to initialize data in outer memory
@;D_cache_off
@;                MRC p15,0,r1,c1,c0,0
@;                BIC r1,r1,#4
@;                MCR p15,0,r1,c1,c0,0
@;                ISB
@;
@;		LDR      r10, =num_init
@;                MEM_SET_64_INC r12, r9, r10
@;		;$src, $size, $init --> all variable is not changed 
@;		;r0~r6 is in used
@
@D_cache_on
@                ; turn on D cache
@                MOV     r2,#0x4
@                MRC     p15,0,r1,c1,c0,0
@                ORR     r1,r1,r2
@                MCR     p15,0,r1,c1,c0,0
@                ISB

l2_loop:
                LDR     r11, =num_init

                MRC   p15, 0, r0, c0, c0, 5 @; Read Multiprocessor Affinity Register
                AND   r0, r0, #0x3          @; Extract CPU ID bits
		CMP   r0, #0
		BEQ   cpu0_init
		CMP   r0, #1
		BEQ   cpu1_init
		CMP   r0, #2
		BEQ   cpu2_init
		CMP   r0, #3
		BEQ   cpu3_init

@; every CPU uses full 32KB data, enable full bank access for every CPU
cpu0_init:
                @LDR     r0 ,=0x12010000     @; initialize data address
                LDR     r0, nADDR_CPU0      @; initialize data address      
                LDR     r0, [r0]                  
		ADD     r11, r0, r11   @; initialize write data
		MOV     r9, r0
		B       start_l2_test
cpu1_init:
                @LDR     r0 ,=0x12018000     @; initialize data address
                LDR     r0, nADDR_CPU1      @; initialize data address   
                LDR     r0, [r0]                                 
		ADD     r11, r0, r11   @; initialize write data
		MOV     r9, r0
		B       start_l2_test
cpu2_init:
                @LDR     r0 ,=0x12000000     @; initialize data address
                LDR     r0, nADDR_CPU2      @; initialize data address
                LDR     r0, [r0]                    
		ADD     r11, r0, r11   @; initialize write data
		MOV     r9, r0
		B       start_l2_test
cpu3_init:
                @LDR     r0 ,=0x00100000     @; initialize data address
                LDR     r0, nADDR_CPU3      @; initialize data address
                LDR     r0, [r0]                    
		ADD     r11, r0, r11   @; initialize write data
		MOV     r9, r0
		B       start_l2_test

start_l2_test:
                MOV     r1,#0
                @;MOV     r2,#0xFF
                LDR     r2,=index_iteration
                MOV     r3,#0
                MOV     r4,#0
                MOV     r5,#0        
                MOV     r6,#0

init0:          STMIA   r0!,{r3-r6}
                DMB
                STMIA   r0!,{r3-r6}
                CMP     r1,r2
                ADD     r1,r1,#1
                BNE     init0

                @;LDR     r0 ,=0x12010000;
		        MOV     r0,r9
                @;MOV     r1,#0
                MOV     r1, r11
                @;MOV     r2,#0xFF
                LDR     r2,=index_iteration

set_dirty:
                STR     r1,[r0]
                STR     r1,[r0,#4]
                STR     r1,[r0,#8]
                STR     r1,[r0,#0xc]
                DMB     
                STR     r1,[r0,#0x10]
                STR     r1,[r0,#0x14]
                STR     r1,[r0,#0x18]
                STR     r1,[r0,#0x1c]
                ADD     r0,r0,#0x20
                CMP     r2,#0
                ADD     r1,r1,#1
                SUB     r2,r2,#1
                BNE     set_dirty

                @;LDR   r0,=0x12010000;
		        MOV     r0,r9
                LDR     r2,=addr_iteration
                ADD     r2,r0, r2
                @;ADD     r2,r0,  #0x2000
read_loop:
                LDR     r1,[r0],#0x40
                LDR     r3,[r0],#0x40
	            @;load 4 line within one loop
                CMP     r0,r2
                BNE     read_loop

                @;LDR     r0,=0x12010000;
		        MOV     r0,r9
                @;ADD     r1,r0, #0x2000
                MOV     r1, r2
	            @;MOV     r6,#0
	            MOV     r6, r11
		        LSL     r6, r6, #2
                @;LDR     r6, =DIRTY0

chk_every_line:
                LDR     r2,[r0]
                LDR     r3,[r0,#4]
                LDR     r4,[r0,#8]
                LDR     r5,[r0,#0xc]
                ADD     r2,r2,r3
                ADD     r2,r2,r4
                ADD     r2,r2,r5
	            CMP     r6,r2
                BNE     test_error
                LDR     r2,[r0,#0x10]
                LDR     r3,[r0,#0x14]
                LDR     r4,[r0,#0x18]
                LDR     r5,[r0,#0x1c]
                ADD     r2,r2,r3
                ADD     r2,r2,r4
                ADD     r2,r2,r5
                CMP     r6,r2
                BNE     test_error
                ADD     r6,r6,#4
                ADD     r0,r0,#0x20
                CMP     r0,r1
                BNE     chk_every_line

                MOV     r6,#0x4000
                MOV     r2,#0
                MOV     r3,#0x40000000
                MOV     r4,#0x80000000
                MOV     r5,#0xc0000000

d_clninv_loop:  MCR	p15,0,r2,c7,c14,2  @; clean and invalidate dcache by set / way
                MCR	p15,0,r3,c7,c14,2  @; clean and invalidate dcache by set / way
                MCR	p15,0,r4,c7,c14,2  @; clean and invalidate dcache by set / way
                MCR	p15,0,r5,c7,c14,2  @; clean and invalidate dcache by set / way
                ADD     r2,r2,#0x20
                ADD     r3,r3,#0x20
                ADD     r4,r4,#0x20
                ADD     r5,r5,#0x20

                CMP     r2,r6
                BNE     d_clninv_loop

                @;LDR     r0,=0x12010000;
		        MOV     r0,r9
                LDR     r2,=addr_iteration
                ADD     r2,r0, r2
                @;ADD     r2,r0,  #0x2000
read2_loop:
                LDR     r1,[r0],#0x40
                LDR     r3,[r0],#0x40
                LDR     r4,[r0],#0x40
                LDR     r5,[r0],#0x40
	            @;load 4 line within one loop
                CMP     r0,r2
                BNE     read2_loop

                @;LDR     r0,=0x12010000;
		        MOV     r0,r9
                @;ADD     r1,r0, #0x2000
                MOV     r1, r2
	            @;MOV     r6,#0
	            MOV     r6, r11
		        LSL     r6, r6, #2
                @;LDR     r6, =DIRTY0

chk2_every_line:
                LDR     r2,[r0]
                LDR     r3,[r0,#4]
                LDR     r4,[r0,#8]
                LDR     r5,[r0,#0xc]
                ADD     r2,r2,r3
                ADD     r2,r2,r4
                ADD     r2,r2,r5
	            CMP     r6,r2
                BNE     test_error
                LDR     r2,[r0,#0x10]
                LDR     r3,[r0,#0x14]
                LDR     r4,[r0,#0x18]
                LDR     r5,[r0,#0x1c]
                ADD     r2,r2,r3
                ADD     r2,r2,r4
                ADD     r2,r2,r5
                CMP     r6,r2
                BNE     test_error
                ADD     r6,r6,#4
                ADD     r0,r0,#0x20
                CMP     r0,r1
                BNE     chk2_every_line

                SUB     r10, r10, #1
                CMP     r10, #0
                BNE     l2_loop

                B       test_pass
test_error:      
@; disable MMU
@                DSB
@                MRC     p15, 0, r0, c1, c0, 0       @; Read SCTLR - System Control Register
@                BIC     r0, r0, #1                  @; Clear M bit (bit 0)
@                MCR     p15, 0, r0, c1, c0, 0       @; Write SCTLR
		        ISB

                MOV  r0, #0x0
                B    test_end

test_pass:                    
@; disable MMU
@                DSB
@                MRC     p15, 0, r0, c1, c0, 0       @; Read SCTLR - System Control Register
@                BIC     r0, r0, #1                  @; Clear M bit (bit 0)
@                MCR     p15, 0, r0, c1, c0, 0       @; Write SCTLR
		        ISB

                MOV  r0, #0x1
                B    test_end


test_end:       LDMIA  sp!, {r1-r12} 
                BX lr
                @ALIGN   32
                .align 5
@@PC_sync_L1_MEMORY DCD 0
@PC_sync_L1_MEMORY:
@    .word 0
@                @ALIGN   32
@                .align 5 
@@dst_location_base  EQU  0x00100000
@                .equ dst_location_base,0x00100000

                @END
                .end
