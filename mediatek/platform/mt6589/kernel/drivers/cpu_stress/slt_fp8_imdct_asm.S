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


                @AREA INIT, CODE, ALIGN=5
                .text
                .align 5

@;-------------------------------------------------------------------------------
@; Init Code
@;-------------------------------------------------------------------------------

@num_iteration   EQU     0x600
    .equ num_iteration,0x600
@table_base      EQU    0x12010000
@
@                ALIGN   32
                .align 5

@;*****************************************************************************
@;***    start of main test
@;*****************************************************************************

                @EXPORT fp8_imdct_start
                .global fp8_imdct_start
fp8_imdct_start:
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
@       ; Enable FPU if present
@                FPU_PRESENT r0
                MOV     r0,#0xf00000               @Write CPACR (Coprocessor Access Control Register)
                MCR     p15,0,r0,c1,c0,2           @to enable coprocessors cp10 & cp11
                MRC     p15,0,r0,c1,c0,2           @Read CPACR
                UBFX    r0, r0, #20, #1            @if bit 20 is zero then so the bits 23:21 so FPU present
                CMP     r0, #1
                BNE     no_fpu       
@@                ;LDR     r0, =test_skipped
@@                ;MOV     r1, #1
@@                ;STR     r1, [r0]
@@                ENABLE_FPU
@@
@@                NEON_PRESENT r0
@@                CMP     r0, #1
@@                BNE     no_neon
@                MOV     r0,#0xf00000               @ Write CPACR (Coprocessor Access Control Register)
@                MCR     p15,0,r0,c1,c0,2           @ to enable coprocessors cp10 & cp11
@                MRC     p15,0,r0,c1,c0,2           @ Read CPACR;
@                 CMP     r0, #0x00f00000            @ if cp10 & cp11 are enabled and bit 31 is zero then NEON present
@                MOVEQ   r0, #1
@                MOVNE   r0, #0
@                CMP     r0, #1
@                BNE     no_neon

@@                MOV     r0, #1<<30
@@                MCR     p10, #7, r0, c8, c0, 0   ;Enable NEON by writing FPEXC

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

                MRC   p15, 0, r0, c0, c0, 5 @; Read Multiprocessor Affinity Register
                AND   r0, r0, #0x3          @; Extract CPU ID bits
		        LDR   r1, =0x800
		        MUL   r11, r0, r1            @; produce shift amount
imdct_loop:
                 
imdct36n:
   LDR         r0, =data_11290
   ADD         r1, r0, #0x120
   LDR         r2, =0x2
   LDR         r3, =data_B210
@; SCHEDULE BB# 2       : 1+261i @ 323c  =  1.23
@	;STMFD        r13!, {r4,lr}                       ; i0   : c0    (+0)
	ADD          r5, r1, #72                          @; i1   : c0    (+2)
	VLD4.32      {D14[0],D15[0],D16[0],D17[0]}, [r0]! @; i2   : c2    (+2)
	VLD4.32      {D18[0],D19[0],D20[0],D21[0]}, [r0]! @; i3   : c4    (+2)
	VLD4.32      {D22[0],D23[0],D24[0],D25[0]}, [r0]! @; i4   : c6    (+2)
	VLD4.32      {D26[0],D27[0],D28[0],D29[0]}, [r0]! @; i5   : c8    (+2)
	VLD2.32      {D30[0],D31[0]}, [r0]!               @; i6   : c10   (+1)
@	;LDR          ip, to_Coefficients_IMDCT_36    ; i7   : c11   (+1)
        LDR          ip, =Coefficients_IMDCT_36
	VLD4.32      {D14[1],D15[1],D16[1],D17[1]}, [r0]! @; i8   : c12   (+2)
	VLD4.32      {D18[1],D19[1],D20[1],D21[1]}, [r0]! @; i9   : c14   (+2)
	VLD4.32      {D22[1],D23[1],D24[1],D25[1]}, [r0]! @; i10  : c16   (+2)
	VLD4.32      {D26[1],D27[1],D28[1],D29[1]}, [r0]! @; i11  : c18   (+2)
	VLD2.32      {D30[1],D31[1]}, [r0]!               @; i12  : c20   (+1)
	VADD.F32     D31, D31, D30                        @; i13  : c21   (+0)
	VLD1.32      {D0,D1,D2,D3}, [ip]!                 @; i14  : c21   (+1)
	VADD.F32     D30, D30, D29                        @; i15  : c22   (+1)
	VADD.F32     D29, D29, D28                        @; i16  : c23   (+0)
	LDR          r0, =out_data_11290
	ADD          r0, r11, r0
@	;SUB          r0, r0, #144                         ; i17  : c23   (+1)
	VADD.F32     D28, D28, D27                        @; i18  : c24   (+0)
	ADD          r4, r0, #72                          @; i19  : c24   (+1)
	VADD.F32     D27, D27, D26                        @; i20  : c25   (+1)
	VADD.F32     D26, D26, D25                        @; i21  : c26   (+1)
	VADD.F32     D25, D25, D24                        @; i22  : c27   (+1)
	VADD.F32     D24, D24, D23                        @; i23  : c28   (+1)
	VADD.F32     D23, D23, D22                        @; i24  : c29   (+1)
	VADD.F32     D22, D22, D21                        @; i25  : c30   (+1)
	VADD.F32     D21, D21, D20                        @; i26  : c31   (+1)
	VADD.F32     D20, D20, D19                        @; i27  : c32   (+1)
	VADD.F32     D19, D19, D18                        @; i28  : c33   (+1)
	VADD.F32     D18, D18, D17                        @; i29  : c34   (+1)
	VADD.F32     D17, D17, D16                        @; i30  : c35   (+1)
	VADD.F32     D16, D16, D15                        @; i31  : c36   (+1)
	VADD.F32     D15, D15, D14                        @; i32  : c37   (+1)
	VSUB.F32     D8, D18, D22                         @; i33  : c38   (+1)
	VADD.F32     D7, D18, D30                         @; i34  : c39   (+1)
	VSUB.F32     D10, D16, D24                        @; i35  : c40   (+1)
	VADD.F32     D9, D16, D24                         @; i36  : c41   (+1)
	VSUB.F32     D8, D8, D30                          @; i37  : c42   (+1)
	VSUB.F32     D24, D28, D24                        @; i38  : c43   (+1)
	VSUB.F32     D10, D10, D28                        @; i39  : c44   (+1)
	VADD.F32     D28, D16, D28                        @; i40  : c45   (+1)
	VSUB.F32     D30, D30, D22                        @; i41  : c46   (+1)
	VSUB.F32     D5, D14, D26                         @; i42  : c47   (+1)
	VMLA.F32     D14, D26, D3[1]                      @; i43  : c48   (+1)
	VMUL.F32     D7, D7, D2[0]                        @; i44  : c49   (+1)
	VMUL.F32     D9, D9, D0[0]                        @; i45  : c50   (+1)
	VMUL.F32     D28, D28, D1[0]                      @; i46  : c51   (+2)
	VADD.F32     D18, D18, D22                        @; i47  : c53   (+1)
	VMUL.F32     D30, D30, D3[0]                      @; i48  : c54   (+1)
	VADD.F32     D31, D31, D29                        @; i49  : c55   (+1)
	VADD.F32     D29, D29, D27                        @; i50  : c56   (+1)
	VADD.F32     D27, D27, D25                        @; i51  : c57   (+1)
	VADD.F32     D25, D25, D23                        @; i52  : c58   (+1)
	VADD.F32     D23, D23, D21                        @; i53  : c59   (+1)
	VADD.F32     D21, D21, D19                        @; i54  : c60   (+1)
	VADD.F32     D19, D19, D17                        @; i55  : c61   (+1)
	VADD.F32     D17, D17, D15                        @; i56  : c62   (+1)
	VMUL.F32     D24, D24, D1[1]                      @; i57  : c63   (+1)
	VMUL.F32     D18, D18, D2[1]                      @; i58  : c64   (+1)
	VMUL.F32     D20, D20, D0[1]                      @; i59  : c65   (+1)
	VADD.F32     D4, D30, D7                          @; i60  : c66   (+1)
	VSUB.F32     D7, D14, D7                          @; i61  : c67   (+1)
	VSUB.F32     D22, D5, D8                          @; i62  : c68   (+1)
	VMLA.F32     D5, D8, D3[1]                        @; i63  : c69   (+1)
	VSUB.F32     D8, D9, D28                          @; i64  : c70   (+1)
	VMUL.F32     D10, D10, D0[1]                      @; i65  : c71   (+1)
	VSUB.F32     D7, D7, D18                          @; i66  : c72   (+2)
	VSUB.F32     D8, D8, D20                          @; i67  : c74   (+1)
	VADD.F32     D11, D24, D9                         @; i68  : c75   (+1)
	VADD.F32     D9, D24, D28                         @; i69  : c76   (+1)
	VSUB.F32     D12, D19, D23                        @; i70  : c77   (+1)
	VSUB.F32     D13, D17, D25                        @; i71  : c78   (+1)
	VADD.F32     D11, D11, D20                        @; i72  : c79   (+1)
	VSUB.F32     D9, D9, D20                          @; i73  : c80   (+1)
	VADD.F32     D6, D14, D18                         @; i74  : c81   (+1)
	VADD.F32     D16, D5, D10                         @; i75  : c82   (+1)
	VADD.F32     D20, D7, D8                          @; i76  : c83   (+1)
	VSUB.F32     D24, D7, D8                          @; i77  : c84   (+1)
	VSUB.F32     D28, D5, D10                         @; i78  : c85   (+1)
	VSUB.F32     D12, D12, D31                        @; i79  : c86   (+1)
	VSUB.F32     D13, D13, D29                        @; i80  : c87   (+1)
	VADD.F32     D7, D19, D31                         @; i81  : c88   (+1)
	VADD.F32     D8, D17, D25                         @; i82  : c89   (+1)
	VSUB.F32     D25, D29, D25                        @; i83  : c90   (+1)
	VADD.F32     D29, D17, D29                        @; i84  : c91   (+1)
	VADD.F32     D19, D19, D23                        @; i85  : c92   (+1)
	VSUB.F32     D31, D31, D23                        @; i86  : c93   (+1)
	VSUB.F32     D5, D15, D27                         @; i87  : c94   (+1)
	VMLA.F32     D15, D27, D3[1]                      @; i88  : c95   (+1)
	VADD.F32     D4, D4, D14                          @; i89  : c96   (+1)
	VSUB.F32     D6, D6, D30                          @; i90  : c97   (+1)
	VMUL.F32     D7, D7, D2[0]                        @; i91  : c98   (+2)
	VMUL.F32     D8, D8, D0[0]                        @; i92  : c100  (+1)
	VMUL.F32     D25, D25, D1[1]                      @; i93  : c101  (+1)
	VMUL.F32     D29, D29, D1[0]                      @; i94  : c102  (+1)
	VMUL.F32     D19, D19, D2[1]                      @; i95  : c103  (+1)
	VMUL.F32     D31, D31, D3[0]                      @; i96  : c104  (+1)
	VADD.F32     D14, D4, D11                         @; i97  : c105  (+1)
	VADD.F32     D18, D6, D9                          @; i98  : c106  (+1)
	VSUB.F32     D26, D6, D9                          @; i99  : c107  (+1)
	VSUB.F32     D30, D4, D11                         @; i100 : c108  (+1)
	VMUL.F32     D21, D21, D0[1]                      @; i101 : c109  (+1)
	VADD.F32     D4, D31, D7                          @; i102 : c110  (+1)
	VSUB.F32     D7, D15, D7                          @; i103 : c111  (+1)
	VSUB.F32     D23, D5, D12                         @; i104 : c112  (+1)
	VMLA.F32     D5, D12, D3[1]                       @; i105 : c113  (+1)
	VSUB.F32     D12, D8, D29                         @; i106 : c114  (+1)
	VADD.F32     D9, D25, D8                          @; i107 : c115  (+1)
	VADD.F32     D8, D25, D29                         @; i108 : c116  (+2)
	VADD.F32     D6, D15, D19                         @; i109 : c118  (+1)
	VMUL.F32     D13, D13, D0[1]                      @; i110 : c119  (+0)
	VLD1.32      {D0,D1,D2,D3}, [ip]!                 @; i111 : c119  (+1)
	VADD.F32     D4, D4, D15                          @; i112 : c120  (+1)
	VSUB.F32     D7, D7, D19                          @; i113 : c121  (+1)
	VSUB.F32     D12, D12, D21                        @; i114 : c122  (+1)
	VADD.F32     D9, D9, D21                          @; i115 : c123  (+1)
	VSUB.F32     D8, D8, D21                          @; i116 : c124  (+1)
	VSUB.F32     D6, D6, D31                          @; i117 : c125  (+1)
	VADD.F32     D17, D5, D13                         @; i118 : c126  (+1)
	VADD.F32     D15, D4, D9                          @; i119 : c127  (+1)
	VADD.F32     D21, D7, D12                         @; i120 : c128  (+1)
	VADD.F32     D19, D6, D8                          @; i121 : c129  (+1)
	VSUB.F32     D25, D7, D12                         @; i122 : c130  (+1)
	VSUB.F32     D27, D6, D8                          @; i123 : c131  (+1)
	VSUB.F32     D29, D5, D13                         @; i124 : c132  (+1)
	VSUB.F32     D31, D4, D9                          @; i125 : c133  (+0)
	VLD1.32      {D4}, [ip]!                          @; i126 : c133  (+1)
	VMUL.F32     D15, D15, D0[0]                      @; i127 : c134  (+1)
	VMUL.F32     D17, D17, D0[1]                      @; i128 : c135  (+1)
	VMUL.F32     D19, D19, D1[0]                      @; i129 : c136  (+1)
	VMUL.F32     D21, D21, D1[1]                      @; i130 : c137  (+0)
	VLD1.32      {D0,D1}, [r3]!                       @; i131 : c137  (+1)
	VMUL.F32     D23, D23, D2[0]                      @; i132 : c138  (+1)
	VMUL.F32     D25, D25, D2[1]                      @; i133 : c139  (+1)
	VMUL.F32     D27, D27, D3[0]                      @; i134 : c140  (+1)
	VMUL.F32     D29, D29, D3[1]                      @; i135 : c141  (+1)
	VMUL.F32     D31, D31, D4[0]                      @; i136 : c142  (+0)
	VLD4.32      {D2[0],D3[0],D4[0],D5[0]}, [r1]!     @; i137 : c142  (+1)
	VSUB.F32     D10, D14, D15                        @; i138 : c143  (+1)
	VADD.F32     D14, D14, D15                        @; i139 : c144  (+0)
	VLD4.32      {D2[1],D3[1],D4[1],D5[1]}, [r5]!     @; i140 : c144  (+1)
	VSUB.F32     D15, D16, D17                        @; i141 : c145  (+1)
	VADD.F32     D16, D16, D17                        @; i142 : c146  (+1)
	VSUB.F32     D17, D18, D19                        @; i143 : c147  (+1)
	VADD.F32     D18, D18, D19                        @; i144 : c148  (+1)
	VSUB.F32     D19, D20, D21                        @; i145 : c149  (+1)
	VADD.F32     D20, D20, D21                        @; i146 : c150  (+1)
	VSUB.F32     D21, D22, D23                        @; i147 : c151  (+1)
	VADD.F32     D22, D22, D23                        @; i148 : c152  (+1)
	VSUB.F32     D23, D24, D25                        @; i149 : c153  (+1)
	VADD.F32     D24, D24, D25                        @; i150 : c154  (+1)
	VSUB.F32     D25, D26, D27                        @; i151 : c155  (+1)
	VADD.F32     D26, D26, D27                        @; i152 : c156  (+1)
	VSUB.F32     D27, D28, D29                        @; i153 : c157  (+1)
	VADD.F32     D28, D28, D29                        @; i154 : c158  (+1)
	VSUB.F32     D29, D30, D31                        @; i155 : c159  (+1)
	VMUL.F32     D8, D25, D1[0]                       @; i156 : c160  (+1)
	VMUL.F32     D7, D27, D0[1]                       @; i157 : c161  (+1)
	VMUL.F32     D9, D23, D1[1]                       @; i158 : c162  (+1)
	VMUL.F32     D6, D29, D0[0]                       @; i159 : c163  (+1)
	VADD.F32     D8, D8, D4                           @; i160 : c164  (+1)
	VADD.F32     D7, D7, D3                           @; i161 : c165  (+1)
	VADD.F32     D9, D9, D5                           @; i162 : c166  (+1)
	VADD.F32     D6, D6, D2                           @; i163 : c167  (+1)
	VADD.F32     D30, D30, D31                        @; i164 : c168  (+4)
	VST4.32      {D6[0],D7[0],D8[0],D9[0]}, [r0]!     @; i165 : c172  (+0)
	VNEG.F32     D7, D7                               @; i166 : c172  (+1)
	VNEG.F32     D9, D9                               @; i167 : c173  (+4)
	VST4.32      {D6[1],D7[1],D8[1],D9[1]}, [r4]!     @; i168 : c177  (+2)
	VLD1.32      {D0,D1}, [r3]!                       @; i169 : c179  (+1)
	VMUL.F32     D6, D21, D0[0]                       @; i170 : c180  (+0)
	VLD4.32      {D2[0],D3[0],D4[0],D5[0]}, [r1]!     @; i171 : c180  (+1)
	VMUL.F32     D7, D19, D0[1]                       @; i172 : c181  (+1)
	VMUL.F32     D8, D17, D1[0]                       @; i173 : c182  (+1)
	VMUL.F32     D9, D15, D1[1]                       @; i174 : c183  (+2)
	VLD4.32      {D2[1],D3[1],D4[1],D5[1]}, [r5]!     @; i175 : c185  (+1)
	VADD.F32     D6, D6, D2                           @; i176 : c186  (+1)
	VADD.F32     D7, D7, D3                           @; i177 : c187  (+1)
	VADD.F32     D8, D8, D4                           @; i178 : c188  (+1)
	VADD.F32     D9, D9, D5                           @; i179 : c189  (+4)
	VST4.32      {D6[0],D7[0],D8[0],D9[0]}, [r0]!     @; i180 : c193  (+0)
	VNEG.F32     D7, D7                               @; i181 : c193  (+1)
	VNEG.F32     D9, D9                               @; i182 : c194  (+4)
	VST4.32      {D6[1],D7[1],D8[1],D9[1]}, [r4]!     @; i183 : c198  (+2)
	VLD1.32      {D0,D1}, [r3]!                       @; i184 : c200  (+1)
	VMUL.F32     D6, D10, D0[0]                       @; i185 : c201  (+0)
	VLD4.32      {D2[0],D3[0],D4[0],D5[0]}, [r1]!     @; i186 : c201  (+1)
	VMUL.F32     D7, D10, D0[1]                       @; i187 : c202  (+1)
	VMUL.F32     D8, D15, D1[0]                       @; i188 : c203  (+1)
	VMUL.F32     D9, D17, D1[1]                       @; i189 : c204  (+2)
	VLD4.32      {D2[1],D3[1],D4[1],D5[1]}, [r5]!     @; i190 : c206  (+1)
	VADD.F32     D6, D6, D2                           @; i191 : c207  (+1)
	VADD.F32     D7, D7, D3                           @; i192 : c208  (+1)
	VADD.F32     D8, D8, D4                           @; i193 : c209  (+1)
	VADD.F32     D9, D9, D5                           @; i194 : c210  (+4)
	VST4.32      {D6[0],D7[0],D8[0],D9[0]}, [r0]!     @; i195 : c214  (+0)
	VNEG.F32     D7, D7                               @; i196 : c214  (+1)
	VNEG.F32     D9, D9                               @; i197 : c215  (+4)
	VST4.32      {D6[1],D7[1],D8[1],D9[1]}, [r4]!     @; i198 : c219  (+2)
	VLD1.32      {D0,D1}, [r3]!                       @; i199 : c221  (+1)
	VMUL.F32     D6, D19, D0[0]                       @; i200 : c222  (+0)
	VLD4.32      {D2[0],D3[0],D4[0],D5[0]}, [r1]!     @; i201 : c222  (+1)
	VMUL.F32     D7, D21, D0[1]                       @; i202 : c223  (+1)
	VMUL.F32     D8, D23, D1[0]                       @; i203 : c224  (+1)
	VMUL.F32     D9, D25, D1[1]                       @; i204 : c225  (+2)
	VLD4.32      {D2[1],D3[1],D4[1],D5[1]}, [r5]!     @; i205 : c227  (+1)
	VADD.F32     D6, D6, D2                           @; i206 : c228  (+1)
	VADD.F32     D7, D7, D3                           @; i207 : c229  (+1)
	VADD.F32     D8, D8, D4                           @; i208 : c230  (+1)
	VADD.F32     D9, D9, D5                           @; i209 : c231  (+4)
	VST4.32      {D6[0],D7[0],D8[0],D9[0]}, [r0]!     @; i210 : c235  (+0)
	VNEG.F32     D7, D7                               @; i211 : c235  (+1)
	VNEG.F32     D9, D9                               @; i212 : c236  (+4)
	VST4.32      {D6[1],D7[1],D8[1],D9[1]}, [r4]!     @; i213 : c240  (+2)
	VLD1.32      {D0}, [r3]!                          @; i214 : c242  (+1)
	VMUL.F32     D6, D27, D0[0]                       @; i215 : c243  (+0)
	VLD2.32      {D2[0],D3[0]}, [r1]!                 @; i216 : c243  (+1)
	VMUL.F32     D7, D29, D0[1]                       @; i217 : c244  (+0)
	VLD2.32      {D2[1],D3[1]}, [r5]!                 @; i218 : c244  (+1)
        LDR          r1, =out_data_113b0
	ADD          r1, r11, r1
        LDR          r8, =data_11290
        ADD          r8, r8, #0x160
        ADD          r5, r1, #0x48
        ADD          r9, r8, #0x48
@	;SUB          r1, r1, #72                         ; i219 : c245  (+0)
@	;SUB          r5, r5, #72                         ; i220 : c245  (+2)
	VADD.F32     D6, D6, D2                           @; i221 : c247  (+1)
	VADD.F32     D7, D7, D3                           @; i222 : c248  (+5)
	VST2.32      {D6[0],D7[0]}, [r0]!                 @; i223 : c253  (+0)
	VNEG.F32     D7, D7                               @; i224 : c253  (+5)
	VST2.32      {D6[1],D7[1]}, [r4]!                 @; i225 : c258  (+1)
	VLD1.32      {D0,D1}, [r3]!                       @; i226 : c259  (+1)
	VMUL.F32     D2, D30, D0[0]                       @; i227 : c260  (+1)
	VMUL.F32     D3, D28, D0[1]                       @; i228 : c261  (+1)
	VMUL.F32     D4, D26, D1[0]                       @; i229 : c262  (+1)
	VMUL.F32     D5, D24, D1[1]                       @; i230 : c263  (+4)
	VST4.32      {D2[0],D3[0],D4[0],D5[0]}, [r1]!     @; i231 : c267  (+3)
	VST4.32      {D2[1],D3[1],D4[1],D5[1]}, [r5]!     @; i232 : c270  (+2)
	VLD1.32      {D0,D1}, [r3]!                       @; i233 : c272  (+1)
	VMUL.F32     D2, D22, D0[0]                       @; i234 : c273  (+1)
	VMUL.F32     D3, D20, D0[1]                       @; i235 : c274  (+1)
	VMUL.F32     D4, D18, D1[0]                       @; i236 : c275  (+1)
	VMUL.F32     D5, D16, D1[1]                       @; i237 : c276  (+0)
	VLD1.32      {D0,D1}, [r3]!                       @; i238 : c276  (+4)
	VST4.32      {D2[0],D3[0],D4[0],D5[0]}, [r1]!     @; i239 : c280  (+3)
	VST4.32      {D2[1],D3[1],D4[1],D5[1]}, [r5]!     @; i240 : c283  (+0)
	VMUL.F32     D2, D14, D0[0]                       @; i241 : c283  (+1)
	VMUL.F32     D3, D14, D0[1]                       @; i242 : c284  (+1)
	VMUL.F32     D4, D16, D1[0]                       @; i243 : c285  (+1)
	VMUL.F32     D5, D18, D1[1]                       @; i244 : c286  (+4)
	VST4.32      {D2[0],D3[0],D4[0],D5[0]}, [r1]!     @; i245 : c290  (+3)
	VST4.32      {D2[1],D3[1],D4[1],D5[1]}, [r5]!     @; i246 : c293  (+3)
	VLD1.32      {D0,D1}, [r3]!                       @; i247 : c296  (+1)
	VMUL.F32     D2, D20, D0[0]                       @; i248 : c297  (+1)
	VMUL.F32     D3, D22, D0[1]                       @; i249 : c298  (+1)
	VMUL.F32     D4, D24, D1[0]                       @; i250 : c299  (+1)
	VMUL.F32     D5, D26, D1[1]                       @; i251 : c300  (+6)
	VST4.32      {D2[0],D3[0],D4[0],D5[0]}, [r1]!     @; i252 : c306  (+2)
	VST4.32      {D2[1],D3[1],D4[1],D5[1]}, [r5]!     @; i253 : c308  (+2)
	VLD1.32      {D0}, [r3]!                          @; i254 : c310  (+1)
	VLD2.32      {D2[0],D3[0]}, [r8]                  @; i255 : c311  (+1)
	VLD2.32      {D2[1],D3[1]}, [r9]                  @; i256 : c312  (+0)
	VMUL.F32     D2, D28, D0[0]                       @; i257 : c312  (+1)
	VMUL.F32     D3, D30, D0[1]                       @; i258 : c313  (+8)
	VST2.32      {D2[0],D3[0]}, [r1]!                 @; i259 : c321  (+1)
	VST2.32      {D2[1],D3[1]}, [r5]!                 @; i260 : c322  (+1)
@	;LDMFD        r13!, {r4,r15}                       ; i261 : c323
                
   LDR         r12,=0x12
   MOV         r2, #0x8
   LDR         r0, =expect_11290
   LDR         r1, =out_data_11290
   ADD         r1, r11, r1
check_36n_1:
   LDRD        r4,r5,[r0],r2
   LDRD        r6,r7,[r1],r2
   CMP         r4, r6 
   BNE         test_error
   CMP         r5, r7 
   BNE         test_error
   SUB         r12, r12, #1
   CMP         r12, #0
   BNE         check_36n_1

   LDR         r12,=0x12
   MOV         r2, #0x8
   LDR         r0, =expect_113b0
   LDR         r1, =out_data_113b0
   ADD         r1, r11, r1
check_36n_2:   
   LDRD        r4,r5,[r0],r2
   LDRD        r6,r7,[r1],r2
   CMP         r4, r6 
   BNE         test_error
   CMP         r5, r7 
   BNE         test_error
   SUB         r12, r12, #1
   CMP         r12, #0
   BNE         check_36n_2

@   ;CMP         r1,r12
@   ;BNE         check_result
   

imdct12n:
@; SCHEDULE BB# 3       : 1+185i @ 231c  =  1.24
        LDR          r0, =out_data_114d0
        ADD          r0, r11, r0
        LDR          r1, =data_115f0
        LDR          r3, =data_114d0
@	;STMFD        r13!, {r4,lr}                       ; i1   : c0    (+1)
	ADD          r5, r1, #72                          @; i2   : c1    (+1)
	VLD3.32      {D14[0],D15[0],D16[0]}, [r3]!        @; i3   : c2    (+2)
	VLD3.32      {D17[0],D18[0],D19[0]}, [r3]!        @; i4   : c4    (+2)
	VLD3.32      {D20[0],D21[0],D22[0]}, [r3]!        @; i5   : c6    (+2)
	VLD3.32      {D23[0],D24[0],D25[0]}, [r3]!        @; i6   : c8    (+2)
	VLD3.32      {D26[0],D27[0],D28[0]}, [r3]!        @; i7   : c10   (+2)
	VLD3.32      {D29[0],D30[0],D31[0]}, [r3]!        @; i8   : c12   (+2)
	VLD3.32      {D14[1],D15[1],D16[1]}, [r3]!        @; i9   : c14   (+2)
	VLD3.32      {D17[1],D18[1],D19[1]}, [r3]!        @; i10  : c16   (+2)
	VLD3.32      {D20[1],D21[1],D22[1]}, [r3]!        @; i11  : c18   (+2)
	VLD3.32      {D23[1],D24[1],D25[1]}, [r3]!        @; i12  : c20   (+2)
	VLD3.32      {D26[1],D27[1],D28[1]}, [r3]!        @; i13  : c22   (+2)
	LDR          r4, =Coefficients_IMDCT_12           @; i14  : c24   (+1)
	VLD3.32      {D29[1],D30[1],D31[1]}, [r3]!        @; i15  : c25   (+0)
        LDR          r3, =out_data_114d0
        ADD          r3, r11, r3
	ADD          r3, r3, #72                          @; i16  : c25   (+1)
@	;ADD          r3, r0, #72                          ; i16  : c25   (+1)
	VADD.F32     D29, D29, D26                        @; i17  : c26   (+1)
	VADD.F32     D26, D26, D23                        @; i18  : c27   (+0)
	VLD1.32      {D0,D1}, [r4]!                       @; i19  : c27   (+1)
	VADD.F32     D23, D23, D20                        @; i20  : c28   (+1)
	VADD.F32     D20, D20, D17                        @; i21  : c29   (+0)
	VLD1.32      {D12}, [r4]!                         @; i22  : c29   (+1)
	VADD.F32     D17, D17, D14                        @; i23  : c30   (+1)
	VSUB.F32     D3, D14, D26                         @; i24  : c31   (+1)
	VMLA.F32     D14, D26, D12[0]                     @; i25  : c32   (+1)
	VMUL.F32     D20, D20, D0[0]                      @; i26  : c33   (+1)
	VSUB.F32     D6, D29, D17                         @; i27  : c34   (+1)
	VMUL.F32     D5, D17, D1[1]                       @; i28  : c35   (+1)
	VMLA.F32     D5, D29, D1[0]                       @; i29  : c36   (+1)
	VMUL.F32     D7, D17, D1[0]                       @; i30  : c37   (+1)
	VMLA.F32     D7, D29, D1[1]                       @; i31  : c38   (+1)
	VMUL.F32     D23, D23, D0[1]                      @; i32  : c39   (+2)
	VMUL.F32     D6, D6, D0[1]                        @; i33  : c41   (+2)
	VSUB.F32     D2, D14, D20                         @; i34  : c43   (+1)
	VADD.F32     D5, D5, D23                          @; i35  : c44   (+1)
	VADD.F32     D6, D6, D23                          @; i36  : c45   (+1)
	VADD.F32     D30, D30, D27                        @; i37  : c46   (+1)
	VADD.F32     D27, D27, D24                        @; i38  : c47   (+1)
	VADD.F32     D24, D24, D21                        @; i39  : c48   (+1)
	VADD.F32     D21, D21, D18                        @; i40  : c49   (+1)
	VADD.F32     D18, D18, D15                        @; i41  : c50   (+1)
	VADD.F32     D4, D14, D20                         @; i42  : c51   (+1)
	VSUB.F32     D7, D7, D23                          @; i43  : c52   (+1)
	VADD.F32     D14, D2, D5                          @; i44  : c53   (+1)
	VADD.F32     D17, D3, D6                          @; i45  : c54   (+1)
	VSUB.F32     D23, D2, D5                          @; i46  : c55   (+1)
	VSUB.F32     D26, D3, D6                          @; i47  : c56   (+1)
	VSUB.F32     D3, D15, D27                         @; i48  : c57   (+1)
	VMLA.F32     D15, D27, D12[0]                     @; i49  : c58   (+1)
	VSUB.F32     D6, D30, D18                         @; i50  : c59   (+1)
	VMUL.F32     D5, D18, D1[1]                       @; i51  : c60   (+1)
	VMLA.F32     D5, D30, D1[0]                       @; i52  : c61   (+2)
	VADD.F32     D20, D4, D7                          @; i53  : c63   (+1)
	VSUB.F32     D29, D4, D7                          @; i54  : c64   (+2)
	VMUL.F32     D7, D18, D1[0]                       @; i55  : c66   (+1)
	VMLA.F32     D7, D30, D1[1]                       @; i56  : c67   (+1)
	VMUL.F32     D21, D21, D0[0]                      @; i57  : c68   (+1)
	VMUL.F32     D24, D24, D0[1]                      @; i58  : c69   (+1)
	VMUL.F32     D6, D6, D0[1]                        @; i59  : c70   (+2)
	VSUB.F32     D2, D15, D21                         @; i60  : c72   (+1)
	VADD.F32     D5, D5, D24                          @; i61  : c73   (+1)
	VADD.F32     D6, D6, D24                          @; i62  : c74   (+1)
	VADD.F32     D31, D31, D28                        @; i63  : c75   (+1)
	VADD.F32     D28, D28, D25                        @; i64  : c76   (+1)
	VADD.F32     D25, D25, D22                        @; i65  : c77   (+1)
	VADD.F32     D22, D22, D19                        @; i66  : c78   (+1)
	VADD.F32     D19, D19, D16                        @; i67  : c79   (+1)
	VADD.F32     D4, D15, D21                         @; i68  : c80   (+1)
	VSUB.F32     D7, D7, D24                          @; i69  : c81   (+1)
	VADD.F32     D15, D2, D5                          @; i70  : c82   (+1)
	VADD.F32     D18, D3, D6                          @; i71  : c83   (+1)
	VSUB.F32     D24, D2, D5                          @; i72  : c84   (+1)
	VSUB.F32     D27, D3, D6                          @; i73  : c85   (+1)
	VSUB.F32     D3, D16, D28                         @; i74  : c86   (+1)
	VMLA.F32     D16, D28, D12[0]                     @; i75  : c87   (+1)
	VSUB.F32     D6, D31, D19                         @; i76  : c88   (+1)
	VMUL.F32     D5, D19, D1[1]                       @; i77  : c89   (+1)
	VMLA.F32     D5, D31, D1[0]                       @; i78  : c90   (+2)
	VADD.F32     D21, D4, D7                          @; i79  : c92   (+1)
	VSUB.F32     D30, D4, D7                          @; i80  : c93   (+2)
	VMUL.F32     D7, D19, D1[0]                       @; i81  : c95   (+1)
	VMLA.F32     D7, D31, D1[1]                       @; i82  : c96   (+1)
	VMUL.F32     D22, D22, D0[0]                      @; i83  : c97   (+1)
	VMUL.F32     D25, D25, D0[1]                      @; i84  : c98   (+1)
	VMUL.F32     D6, D6, D0[1]                        @; i85  : c99   (+2)
	VSUB.F32     D2, D16, D22                         @; i86  : c101  (+1)
	VADD.F32     D5, D5, D25                          @; i87  : c102  (+1)
	VADD.F32     D6, D6, D25                          @; i88  : c103  (+1)
	VADD.F32     D4, D16, D22                         @; i89  : c104  (+1)
	VSUB.F32     D7, D7, D25                          @; i90  : c105  (+1)
	VADD.F32     D16, D2, D5                          @; i91  : c106  (+1)
	VADD.F32     D19, D3, D6                          @; i92  : c107  (+1)
	VSUB.F32     D25, D2, D5                          @; i93  : c108  (+1)
	VSUB.F32     D28, D3, D6                          @; i94  : c109  (+0)
	VLD4.32      {D0[0],D1[0],D2[0],D3[0]}, [r1]!     @; i95  : c109  (+1)
	VADD.F32     D22, D4, D7                          @; i96  : c110  (+1)
	VLD4.32      {D0[1],D1[1],D2[1],D3[1]}, [r5]!     @; i97  : c111  (+0)
	VSUB.F32     D31, D4, D7                          @; i98  : c111  (+2)
	VLD2.32      {D4[0],D5[0]}, [r1]!                 @; i99  : c113  (+1)
	VLD2.32      {D4[1],D5[1]}, [r5]!                 @; i100 : c114  (+1)
	VST4.32      {D0[0],D1[0],D2[0],D3[0]}, [r0]!     @; i101 : c115  (+0)
	VNEG.F32     D1, D1                               @; i102 : c115  (+1)
	VNEG.F32     D3, D3                               @; i103 : c116  (+1)
	VST2.32      {D4[0],D5[0]}, [r0]!                 @; i104 : c117  (+0)
	VNEG.F32     D5, D5                               @; i105 : c117  (+3)
	VST4.32      {D0[1],D1[1],D2[1],D3[1]}, [r3]!     @; i106 : c120  (+2)
	VST2.32      {D4[1],D5[1]}, [r3]!                 @; i107 : c122  (+1)
	VLD1.32      {D7,D8,D9,D10}, [r4]!                @; i108 : c123  (+1)
	VMUL.F32     D6, D14, D7[0]                       @; i109 : c124  (+1)
	VLD1.32      {D11}, [r4]!                         @; i110 : c125  (+4)
	VLD4.32      {D0[0],D1[0],D2[0],D3[0]}, [r1]!     @; i111 : c129  (+5)
	VLD4.32      {D0[1],D1[1],D2[1],D3[1]}, [r5]!     @; i112 : c134  (+0)
	VMLA.F32     D1, D17, D12[0]                      @; i113 : c134  (+1)
	VMLA.F32     D2, D20, D7[1]                       @; i114 : c135  (+1)
	VMLA.F32     D3, D20, D8[0]                       @; i115 : c136  (+0)
	VLD2.32      {D4[0],D5[0]}, [r1]!                 @; i116 : c136  (+1)
	VLD2.32      {D4[1],D5[1]}, [r5]!                 @; i117 : c137  (+0)
	VMLA.F32     D4, D17, D8[1]                       @; i118 : c137  (+1)
	VMLA.F32     D5, D14, D9[0]                       @; i119 : c138  (+5)
	VADD.F32     D0, D0, D6                           @; i120 : c143  (+1)
	VSUB.F32     D6, D18, D26                         @; i121 : c144  (+4)
	VST4.32      {D0[0],D1[0],D2[0],D3[0]}, [r0]!     @; i122 : c148  (+0)
	VNEG.F32     D1, D1                               @; i123 : c148  (+1)
	VNEG.F32     D3, D3                               @; i124 : c149  (+1)
	VST2.32      {D4[0],D5[0]}, [r0]!                 @; i125 : c150  (+0)
	VNEG.F32     D5, D5                               @; i126 : c150  (+3)
	VST4.32      {D0[1],D1[1],D2[1],D3[1]}, [r3]!     @; i127 : c153  (+2)
	VST2.32      {D4[1],D5[1]}, [r3]!                 @; i128 : c155  (+1)
	VLD4.32      {D0[0],D1[0],D2[0],D3[0]}, [r1]!     @; i129 : c156  (+2)
	VLD2.32      {D4[0],D5[0]}, [r1]!                 @; i130 : c158  (+1)
        LDR          r1,=out_data_115f0
        ADD          r1, r11, r1
@	;SUB          r1, r1, #72                          ; i131 : c159  (+3)
	VLD4.32      {D0[1],D1[1],D2[1],D3[1]}, [r5]!     @; i132 : c162  (+0)
	VMLA.F32     D0, D23, D9[1]                       @; i133 : c162  (+1)
	VMLA.F32     D2, D29, D10[0]                      @; i134 : c163  (+1)
	VMLA.F32     D3, D29, D10[1]                      @; i135 : c164  (+1)
	VMLA.F32     D1, D6, D12[0]                       @; i136 : c165  (+1)
	VMLA.F32     D0, D15, D7[0]                       @; i137 : c166  (+1)
	VLD2.32      {D4[1],D5[1]}, [r5]!                 @; i138 : c167  (+0)
	VMLA.F32     D4, D26, D11[0]                      @; i139 : c167  (+1)
	VMLA.F32     D5, D23, D11[1]                      @; i140 : c168  (+0)
        ADD          r5, r1, #0x48
@	;SUB          r5, r5, #72                        ; i141 : c168  (+1)
	VMLA.F32     D2, D21, D7[1]                       @; i142 : c169  (+1)
	VMLA.F32     D3, D21, D8[0]                       @; i143 : c170  (+1)
	VMLA.F32     D4, D18, D8[1]                       @; i144 : c171  (+1)
	VMLA.F32     D5, D15, D9[0]                       @; i145 : c172  (+5)
	VSUB.F32     D6, D19, D27                         @; i146 : c177  (+1)
	VST4.32      {D0[0],D1[0],D2[0],D3[0]}, [r0]!     @; i147 : c178  (+0)
	VNEG.F32     D1, D1                               @; i148 : c178  (+1)
	VNEG.F32     D3, D3                               @; i149 : c179  (+2)
	VST2.32      {D4[0],D5[0]}, [r0]!                 @; i150 : c181  (+0)
	VNEG.F32     D5, D5                               @; i151 : c181  (+2)
	VST4.32      {D0[1],D1[1],D2[1],D3[1]}, [r3]!     @; i152 : c183  (+0)
	VMUL.F32     D0, D24, D9[1]                       @; i153 : c183  (+1)
	VMLA.F32     D0, D16, D7[0]                       @; i154 : c184  (+1)
	VMUL.F32     D2, D30, D10[0]                      @; i155 : c185  (+1)
	VMLA.F32     D2, D22, D7[1]                       @; i156 : c186  (+0)
	VST2.32      {D4[1],D5[1]}, [r3]!                 @; i157 : c186  (+1)
	VMUL.F32     D3, D30, D10[1]                      @; i158 : c187  (+1)
	VMLA.F32     D3, D22, D8[0]                       @; i159 : c188  (+1)
	VMUL.F32     D4, D27, D11[0]                      @; i160 : c189  (+1)
	VMLA.F32     D4, D19, D8[1]                       @; i161 : c190  (+1)
	VMUL.F32     D5, D24, D11[1]                      @; i162 : c191  (+1)
	VMLA.F32     D5, D16, D9[0]                       @; i163 : c192  (+1)
	VMUL.F32     D1, D6, D12[0]                       @; i164 : c193  (+2)
	VNEG.F32     D6, D28                              @; i165 : c195  (+3)
	VST4.32      {D0[0],D1[0],D2[0],D3[0]}, [r1]!     @; i166 : c198  (+3)
	VST2.32      {D4[0],D5[0]}, [r1]!                 @; i167 : c201  (+1)
	VST4.32      {D0[1],D1[1],D2[1],D3[1]}, [r5]!     @; i168 : c202  (+0)
	VMUL.F32     D0, D25, D9[1]                       @; i169 : c202  (+1)
	VMUL.F32     D1, D6, D12[0]                       @; i170 : c203  (+1)
	VMUL.F32     D2, D31, D10[0]                      @; i171 : c204  (+0)
	VST2.32      {D4[1],D5[1]}, [r5]!                 @; i172 : c204  (+1)
	VMUL.F32     D3, D31, D10[1]                      @; i173 : c205  (+1)
	VMUL.F32     D4, D28, D11[0]                      @; i174 : c206  (+1)
	VMUL.F32     D5, D25, D11[1]                      @; i175 : c207  (+4)
	VST4.32      {D0[0],D1[0],D2[0],D3[0]}, [r1]!     @; i176 : c211  (+5)
	VST2.32      {D4[0],D5[0]}, [r1]!                 @; i177 : c216  (+1)
	VST4.32      {D0[1],D1[1],D2[1],D3[1]}, [r5]!     @; i178 : c217  (+2)
	VST2.32      {D4[1],D5[1]}, [r5]!                 @; i179 : c219  (+1)
	VMOV.I16      D0, #0x0                            @; i180 : c220  (+1)
	VMOV.I16      D1, #0x0                            @; i181 : c221  (+1)
	VMOV.I16      D2, #0x0                            @; i182 : c222  (+2)
	VST3.32      {D0,D1,D2}, [r1]!                    @; i183 : c224  (+2)
	VST3.32      {D0,D1,D2}, [r5]!                    @; i184 : c226  (+5)
@	;LDMFD        r13!, {r4,r15}                       ; i185 : c231


   LDR         r12,=0x12
   MOV         r2, #0x8
   LDR         r0, =expect_114d0
   LDR         r1, =out_data_114d0
   ADD         r1, r11, r1
check_12n_1:
   LDRD        r4,r5,[r0],r2
   LDRD        r6,r7,[r1],r2
   CMP         r4, r6 
   BNE         test_error
   CMP         r5, r7 
   BNE         test_error
   SUB         r12, r12, #1
   CMP         r12, #0
   BNE         check_12n_1

   LDR         r12,=0x12
   MOV         r2, #0x8
   LDR         r0, =expect_115f0
   LDR         r1, =out_data_115f0
   ADD         r1, r11, r1
check_12n_2:   
   LDRD        r4,r5,[r0],r2
   LDRD        r6,r7,[r1],r2
   CMP         r4, r6 
   BNE         test_error
   CMP         r5, r7 
   BNE         test_error
   SUB         r12, r12, #1
   CMP         r12, #0
   BNE         check_12n_2

                MOV  r0, #0x1
                B    test_end
                
                SUBS     r10, r10, #0x01
                BCS      imdct_loop
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
                .align
Coefficients_IMDCT_36:
            .word    1065098332
            .word    1063105495
            .word    1059360187
            .word    1051663684
            .word    1064341426
            .word    3190935764
            .word    3208911741
            .word    1056964608
            .word    1056996651
            .word    1057260526
            .word    1057831805
            .word    1058816599
            .word    1060439283
            .word    1063201092
            .word    1066889177
            .word    1073170154
            .word    1085772884
Coefficients_IMDCT_12:
            .word    1063105495
            .word    1060439283
            .word    3212265194
            .word    3196355566
            .word    1056964608
            .word    1056964608
            .word    1037799170
            .word    1075134128
            .word    3225583597
            .word    3214574202
            .word    3209721555
            .word    3206542798
            .word    3201096154
            .word    3197972492
            .word    3193181133
            .word    3181934887
data_B3C0:
            .quad    0x3f5db3d73f7c1c5c
            .quad    0x3eaf1d443f248dbb
            .quad    0xbe31d0d43f708fb2
            .quad    0x3f000000bf441b7d
            .quad    0x3f0483ee3f007d2b
            .quad    0x3f1c42573f0d3b7d
            .quad    0x3f5f29443f3504f3
            .quad    0x3ff746ea3f976fd9
            .quad    0x3f5db3d740b79454
data_B210:
            .quad    0x3ddb8f023d043a95
            .quad    0x3ea6b7303e4e3f8b
            .quad    0x3f448ce23f000000
            .quad    0x40153eb03f9ee064
            .quad    0xc107385a40f7d015
            .quad    0xbff962dec0427fed
            .quad    0xbf9a827abfbcc8eb
            .quad    0xbf6894d6bf843046
            .quad    0xbf3d489dbf5076d3
            .quad    0xbf1ff5cebf2d724e
            .quad    0xbf09a043bf142bb2
            .quad    0xbeee1832bf000000
            .quad    0xbeccd9dabedd267e
            .quad    0xbead1da9bebcec3d
            .quad    0xbe8ce369be9d300c
            .quad    0xbe5413cdbe77e369
            .quad    0xbe036505be2d92c2
            .quad    0xbcf254b0bda87927
data_11290:
            .word    0x3fbd6150
            .word    0x3f67bf5c
            .word    0x3f863c90
            .word    0x3e9c7822
            .word    0x3e591126
            .word    0x3fcc7b6c
            .word    0x3f5ce0ba
            .word    0x3eec6c00
            .word    0x3f5c1e0a
            .word    0x3f711de2
            .word    0x3fe1ee1a
            .word    0x3feb3c73
            .word    0x3fb994db
            .word    0x3f1903cb
            .word    0x3f5c3158
            .word    0x3f1bd80e
            .word    0x3e26c71c
            .word    0x3fa907f7
            .word    0x3ff5bd61
            .word    0x3da230a8
            .word    0x3f7bcdca
            .word    0x3f296418
            .word    0x3fed6fb9
            .word    0x3f9c55ca
            .word    0x3d8523b8
            .word    0x3f1f7ad4
            .word    0x3fa2d8fa
            .word    0x3e8156a5
            .word    0x3faa5dc3
            .word    0x3fe3442b
            .word    0x3fcd7520
            .word    0x3d72bb28
            .word    0x3fce51ad
            .word    0x3fbc6d7b
            .word    0x3faf8e05
            .word    0x3febc524
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x0
            .word    0x41192203
            .word    0x40a02211
            .word    0x40246c3e
            .word    0x411fc1c1
            .word    0x402265ae
            .word    0x408401b8
            .word    0x416851c0
            .word    0x413c8d19
            .word    0x3f8a82d8
            .word    0x40b38afc
            .word    0x415ff22e
            .word    0x4129cf3b
            .word    0x41727809
            .word    0x41718a12
            .word    0x402d3469
            .word    0x40aba6dc
            .word    0x412ce83e
            .word    0x40d45370
            .word    0x41398788
            .word    0x413cc29c
            .word    0x4122433b
            .word    0x41689534
            .word    0x40e63bcf
            .word    0x40f5f9b7
            .word    0x40b7048c
            .word    0x4100bc35
            .word    0x3f7fce18
            .word    0x40df6864
            .word    0x415811bf
            .word    0x407d95db
            .word    0x415ea1c8
            .word    0x409e3225
            .word    0x415d1baf
            .word    0x40b68316
            .word    0x41416804
            .word    0x41745fb2

expect_11290:
            .word    0x41184988
            .word    0x409f5b05
            .word    0x401ef53a
            .word    0x412edc8a
            .word    0x4022e5e0
            .word    0x40a0a68e
            .word    0x415673be
            .word    0x41462656
            .word    0x3f015d85
            .word    0x40c7af71
            .word    0x41536fd8
            .word    0x4145dafc
            .word    0x4156f4e9
            .word    0x41713cb2
            .word    0xbe935890
            .word    0x40b7fa23
            .word    0x412fdc33
            .word    0x40fb0fd6
            .word    0x413b3591
            .word    0xc13a9ba0
            .word    0x411bd5f5
            .word    0xc179dfd4
            .word    0x40e6e971
            .word    0xc1172254
            .word    0x40a5abb0
            .word    0xc114001c
            .word    0x3fdfd5b0
            .word    0xc0c53c21
            .word    0x413ef65d
            .word    0xc09a05ae
            .word    0x41289032
            .word    0xc09c8ef5
            .word    0x41264439
            .word    0xc0f07dcc
            .word    0x4151c12a
            .word    0xc14de63e
expect_113b0:
            .word    0x3f796249
            .word    0x3f35b6f7
            .word    0xc05b415c
            .word    0xbf0dd634
            .word    0x3e760d06
            .word    0xbfe3ef41
            .word    0xc0086ec1
            .word    0x406d9591
            .word    0xc102e942
            .word    0xc0efea7d
            .word    0x40364e08
            .word    0xbfadd598
            .word    0xbf6d4f7c
            .word    0x3dcbd5d1
            .word    0xbe32e241
            .word    0xbf426e52
            .word    0x3dbf62b4
            .word    0x3d2e36a2
            .word    0xc05ca82f
            .word    0x3d6cc829
            .word    0xc090430b
            .word    0x400d69f3
            .word    0xbf3d7b59
            .word    0x3ff69b91
            .word    0xc076b407
            .word    0x4072269e
            .word    0xc1093be8
            .word    0xc0fb80ec
            .word    0x4039cf12
            .word    0xc01d2ad4
            .word    0x3f806037
            .word    0xbe9cf8ce
            .word    0x3f3259b9
            .word    0xbf7fdb53
            .word    0x3bf96226
            .word    0xbe1a253d

data_114d0:
            .word    0x3feaf534
            .word    0x3f92765f
            .word    0x3fa34468
            .word    0x3ffbbe5b
            .word    0x3f8c4842
            .word    0x3f3f0990
            .word    0x3f454c8e
            .word    0x3f14d44d
            .word    0x3fb658d2
            .word    0x3f96bfd3
            .word    0x3fbaf479
            .word    0x3e9d2238
            .word    0x3be98700
            .word    0x3ebe8a13
            .word    0x3ea6c090
            .word    0x3ff8f1bb
            .word    0x3fcced17
            .word    0x3d9206bc
            .word    0x3fed06c4
            .word    0x3ff1e2fb
            .word    0x3f4999dd
            .word    0x3f6f1edc
            .word    0x3f84f6e4
            .word    0x3f7a43e6
            .word    0x3ff44e24
            .word    0x3d629098
            .word    0x3f170b16
            .word    0x3f49a522
            .word    0x3f69d4bf
            .word    0x3fc08a42
            .word    0x3fc143b0
            .word    0x3fc1383f
            .word    0x3ff7f476
            .word    0x3f9774ff
            .word    0x3e113dfe
            .word    0x3f3cb173

data_b404:
            .quad    0x3f3504f33f5db3d7
            .quad    0xbe8483eebf7746ea
            .quad    0x3f0000003f000000
            .quad    0x40153eb03ddb8f02
            .quad    0xbf9a827ac0427fed
            .quad    0xbf1ff5cebf5076d3
            .quad    0xbe9d300cbeccd9da
            .quad    0xbda87927be5413cd
data_115f0:
            .word    0x40cf0431
            .word    0x4073141f
            .word    0x41073160
            .word    0x417261ce
            .word    0x40cf1514
            .word    0x417121bc
            .word    0x409db051
            .word    0x417f8d12
            .word    0x415d046a
            .word    0x4115310a
            .word    0x40a88ca6
            .word    0x40cb9f12
            .word    0x4069855e
            .word    0x40a125f4
            .word    0x4130ea21
            .word    0x41521766
            .word    0x40eab170
            .word    0x3f6c2b7c
            .word    0x40bf4436
            .word    0x414a1b88
            .word    0x413343a7
            .word    0x403b8dd7
            .word    0x404e041c
            .word    0x401de646
            .word    0x410f9652
            .word    0x40e611e8
            .word    0x4123ff5d
            .word    0x40deec87
            .word    0x4071219c
            .word    0x40f632a7
            .word    0x411431a9
            .word    0x412c8cc8
            .word    0x40b68e45
            .word    0x4140a71d
            .word    0x41579016
            .word    0x414edc69

expect_114d0:
            .word    0x40cf0431
            .word    0x4073141f
            .word    0x41073160
            .word    0x417261ce
            .word    0x40cf1514
            .word    0x417121bc
            .word    0x4098c477
            .word    0x4182a7ec
            .word    0x4140c597
            .word    0x413a006f
            .word    0x408cbbd2
            .word    0x40f10098
            .word    0x40289778
            .word    0x40a0e917
            .word    0x40b01895
            .word    0x413ca853
            .word    0x40d0f4b6
            .word    0x3f92fdae
            .word    0x40bf4436
            .word    0xc14a1b88
            .word    0x413343a7
            .word    0xc03b8dd7
            .word    0x404e041c
            .word    0xc01de646
            .word    0x410e1529
            .word    0xc0fc64d7
            .word    0x412a42d3
            .word    0xc0ce992e
            .word    0x40055778
            .word    0xc10686e6
            .word    0x41017c98
            .word    0xc138d589
            .word    0x3fd37283
            .word    0xc10694f2
            .word    0x416da6a4
            .word    0xc145f619

expect_115f0:
            .word    0xbf838cad
            .word    0x3faa5cfc
            .word    0xc026c982
            .word    0xc03b4334
            .word    0xbeede7a3
            .word    0xbe2386ee
            .word    0x3ec1cd57
            .word    0xbe914644
            .word    0xc02ce37a
            .word    0xc004a97e
            .word    0xbdf0b2da
            .word    0x3d4c1dc3
            .quad    0x0000000000000000
            .quad    0x0000000000000000
            .quad    0x0000000000000000
            .word    0xc01ba298
            .word    0xbddc3cb8
            .word    0xc04e682a
            .word    0xc01a9f2f
            .word    0x3f3489ab
            .word    0x3c95a530
            .word    0xbfd8b8dd
            .word    0x4018e1c4
            .word    0xc04649e9
            .word    0xc01826fb
            .word    0x3f7d4d93
            .word    0xbe64418e
            .quad    0x0000000000000000
            .quad    0x0000000000000000
            .quad    0x0000000000000000


                @ALIGN           512
                .align  9
out_data_11290: @DCD     0
                .word   0
                @ALIGN           512
                .align  9
out_data_113b0: @DCD     0
                .word   0
                @ALIGN           512
                .align  9
out_data_114d0: @DCD     0
                .word   0
                @ALIGN           512
                .align  9
out_data_115f0: @DCD     0
                .word 0
                @ALIGN           512
                .align  9

out_data_11290_CPU1: .word   0
                .align  9
out_data_113b0_CPU1: .word   0
                .align  9
out_data_114d0_CPU1: .word   0
                .align  9
out_data_115f0_CPU1: .word 0
                .align  9

out_data_11290_CPU2: .word   0
                .align  9
out_data_113b0_CPU2: .word   0
                .align  9
out_data_114d0_CPU2: .word   0
                .align  9
out_data_115f0_CPU2: .word 0
                .align  9

out_data_11290_CPU3: .word   0
                .align  9
out_data_113b0_CPU3: .word   0
                .align  9
out_data_114d0_CPU3: .word   0
                .align  9
out_data_115f0_CPU3: .word 0
                .align  9 
                
                @END
                .end
