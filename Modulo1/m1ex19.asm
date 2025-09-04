    .cdecls "msp430.h"
    .global main

    .text

main:
    mov.w   #(WDTPW|WDTHOLD), &WDTCTL

    MOV.W   #0x89AB, R12    ; vetor 1
    MOV.W   #0x2400, R13    ; vetor 2
    MOV.w   #0x4400, SP    ; inicializa a pilha
    
    CALL    #W16_ASC

    jmp     $           
    nop  

W16_ASC:
    PUSH    R5

    MOV.W     R13, R5

    CALL    #EXTRAI_4BIT
    CALL    #NIB_ASC
    MOV.B   R14, 3(R5)

    CALL    #EXTRAI_4BIT
    CALL    #NIB_ASC
    MOV.B   R14, 2(R5)

    CALL    #EXTRAI_4BIT
    CALL    #NIB_ASC
    MOV.B   R14, 1(R5)

    CALL    #EXTRAI_4BIT
    CALL    #NIB_ASC
    MOV.B   R14, 0(R5)

    POP     R5
    RET       

NIB_ASC:
    CMP     #10, R14        
    JHS     NIB_ASC_letter  ; IF >=10 IS A LETTER
    
    ; It's a digit (0-9)
    ADD     #'0', R14       ; CONVERT DIGITS(0 - ASCII 48)
    RET
    
NIB_ASC_letter:
    ADD     #'7', R14  ; CONVERT LETTERS(A - ASCII 65)
    RET

EXTRAI_4BIT:
    CLR     R14     ; CLEAR R4

    ;   REMOVE CARRY FROM R12 AND ADD TO R14
    RRA     R12     
    RRC     R14

    RRA     R12
    RRC     R14

    RRA     R12
    RRC     R14

    RRA     R12
    RRC     R14

    ;   SWP R14 BITS AND REMOVE LAST 4
    SWPB    R14
    RRA     R14
    RRA     R14
    RRA     R14
    RRA     R14

    RET

;-------------------------------------------------------------------------------
    .data
vetor1: .word 7, 65000, 50054, 26472, 53000, 60606, 814, 41121
vetor2: .word 7, 226, 3400, 26472, 470, 1020, 44444, 12345
