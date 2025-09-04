    .cdecls "msp430.h"
    .global main

    .text

main:
    mov.w   #(WDTPW|WDTHOLD), &WDTCTL

    MOV.W   #vetor1, R5    ; vetor 1
    MOV.W   #vetor2, R6    ; vetor 2
    MOV.W   #0x2420, R7    ; vetor 3
    MOV.w   #0x4400, SP    ; inicializa a pilha
    
    CALL    #mapSum16

    jmp     $           
    nop  

mapSum16:
    PUSH    R5
    PUSH    R6
    PUSH    R7
    PUSH    R11
    PUSH    R12
    MOV.w   @R5+,   0(R7)
    MOV.w   @R6+,   R11

TEST_ZERO:
    TST     R11
    JNZ     LOOP_SOMA
    ; RETORNA AS VARIAVEIS APOS O LOOP
    POP     R12
    POP     R11
    POP     R7
    POP     R6
    POP     R5
    RET

LOOP_SOMA:
    MOV.w   @R7+,   R12
    MOV.W   @R5+,   0(R7)
    ADD.W   @R6+,   0(R7)
    DEC     R11

    JMP TEST_ZERO
    NOP

;-------------------------------------------------------------------------------
    .data
vetor1: .word 7, 65000, 50054, 26472, 53000, 60606, 814, 41121
vetor2: .word 7, 226, 3400, 26472, 470, 1020, 44444, 12345
