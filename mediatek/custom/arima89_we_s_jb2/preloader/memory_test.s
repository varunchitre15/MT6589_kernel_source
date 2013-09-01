

     
                 
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
.globl store_8word
store_8word:

   STMDB   sp!, {r4,r5,r6,r7,r8,r9}
   MVN r3,r1
   ADD r4, r1, r1
   ADD r5, r3, r3
   ADD r6, r4, r4
   ADD r7, r5, r5
   ADD r8, r6, r6
   ADD r9, r7, r7
   STMIA   r0, {r1,r3,r4,r5,r6,r7,r8,r9}   

StoreEnd:
   LDMIA   sp!, {r4,r5,r6,r7,r8,r9}
;@  BX lr
   mov	pc,	lr


;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
.globl load_8word   
load_8word:

   STMDB   sp!, {r4-r12}
   MOV r12,#0
   MVN r3,r1
   LDMIA   r0, {r4,r5,r6,r7,r8,r9,r10,r11}   
   CMP r4, r1
   MOVNE r12, #9
   BNE LoadEnd
   CMP r5, r3
   MOVNE r12, #10
   BNE LoadEnd
   ADD r1, r1, r1
   CMP r6, r1
   MOVNE r12, #11
   BNE LoadEnd
   ADD r3, r3, r3
   CMP r7, r3
   MOVNE r12, #12
   BNE LoadEnd
   
   ADD r1, r1, r1
   CMP r8, r1
   MOVNE r12, #13
   BNE LoadEnd
   ADD r3, r3, r3
   CMP r9, r3
   MOVNE r12, #14
   BNE LoadEnd
   ADD r1, r1, r1
   CMP r10, r1
   MOVNE r12, #15
   BNE LoadEnd
   ADD r3, r3, r3
   CMP r11, r3
   MOVNE r12, #16
   
LoadEnd:
   mov r0, r12
   LDMIA   sp!, {r4-r12}
;@   BX lr
   mov	pc,	lr
   
