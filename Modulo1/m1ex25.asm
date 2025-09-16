    .cdecls "msp430.h"
    .global main

    .text

main:
    mov.w #(WDTPW|WDTHOLD), &WDTCTL
    mov #matriz, R12 ; Ponteiro para a matriz de entrada
    mov #3, R13 ; Número de linhas da matriz
    mov #3, R14 ; Número de colunas da matriz
    call #SUM_SUB ; Chamar sub-rotina
    jmp $ ; Loop infinito
    nop

SUM_SUB:
    PUSH R8
    PUSH R9

    MOV.B #1, R8 ; PRIMEIRA LINHA
    MOV.B #1, R9 ; PRIMEIRA COLUNA
    call #EXECUTA_SOMA
    PUSH R15 ; COLOCA NA PILHA


    MOV.B #1, R8 ; PRIMEIRA LINHA
    MOV.B R14, R9 ; PRIMEIRA COLUNA
    call #EXECUTA_SOMA
    PUSH R15 ; COLOCA NA PILHA

    MOV.B R13, R8 ; PRIMEIRA LINHA
    MOV.B #1, R9 ; PRIMEIRA COLUNA
    call #EXECUTA_SOMA
    PUSH R15 ; COLOCA NA PILHA

    MOV.B R13, R8 ; PRIMEIRA LINHA
    MOV.B R14, R9 ; PRIMEIRA COLUNA
    call #EXECUTA_SOMA
    PUSH R15 ; COLOCA NA PILHA

RETORNO_SUM_SUB:
    POP R15
    POP R14
    POP R13
    POP R12
    POP R9
    POP R8
    RET

;--------------------------------------------------------------------------
EXECUTA_SOMA: ; soma da linha 1
    push R4
    push R5
    push R6
    CLR R15      ; SALVA O RESULTADO DA ROTINA EM R15

    mov.b   #1, R4 ; linha atual
    mov.b   #1, R5 ; coluna atual
    MOV.W   R12, R6 ; ENDERECO DE MEMORIA
 
INICIA_LOOP:
    CMP     R8, R4     ; VERIFICA SE ESTA NA LINHA REFERENCIADA POR R8
    JEQ     PULA_SOMA
    CMP     R9, R5     ; VERIFICA SE ESTA NA COLUNA REFERENCIADA POR R9
    JEQ     PULA_SOMA
    ADD.W   @R6, R15   ; ADICIONA O VALOR A R7

PULA_SOMA:
    INCD    R6 ; PROXIMO ENDERECO DE MEMORIA
    CMP     R5, R14  ; VERIFICA SE ESTA NA ULTIMA COLUNA
    JEQ     PROXIMA_LINHA
    INC     R5      ; VAI PARA A PROXIMA COLUNA
    JMP     INICIA_LOOP
    NOP

PROXIMA_LINHA:
    CMP     R4, R13  ;verifica se ja esta na ultima linha
    JEQ     RETORNO_SOMA
    INC   R4 ; vai para a proxima linha
    mov.b   #1, R5 ; reseta a coluna
    JMP     INICIA_LOOP
    NOP

RETORNO_SOMA:
    POP R6
    POP R5
    POP R4
    RET

;-------------------------------------------------------------------------------
; Especificar a matriz de entrada na seção de dados
    .data
matriz: .word 1, 2, 3, 4, 5, 6, 7, 8, 9
