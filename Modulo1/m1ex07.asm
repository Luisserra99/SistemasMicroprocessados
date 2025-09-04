    .cdecls "msp430.h"
    .global main

    .text

main:
    mov.w   #(WDTPW|WDTHOLD), &WDTCTL

    MOV.W   #0x2400, R4    ; inicializa posicao da memoria
    MOV.w   #0x0001, 0(R4)
    ADD.W   #0x0002, R4
    MOV.w   #0x0001, 0(R4)
    ADD.W   #0x0002, R4
FIB:
    MOV.w   -2(R4),   R5
    ADD.w   -4(R4),   R5
    MOV.w   R5,       0(R4)
    ADD.W   #0x0002, R4

    CMP     #0x2428, R4
    jnz     FIB


    jmp     $           
    nop                 

; ------------------------------------
;    .data       ; Tudo abaixo vai p/ a RAM
;v1: .byte       1 , 2 , 3 , 4
