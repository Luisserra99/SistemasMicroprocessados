    .cdecls "msp430.h"
    .global main

    .text

main:
    mov.w   #(WDTPW|WDTHOLD), &WDTCTL

    MOV.W   #vetor1, R12    ; vetor 1
    MOV.W   #0x0008, R13    ; vetor 2
    MOV.w   #0x4400, SP    ; inicializa a pilha
    
    CALL    #ORDENA

    jmp     $           
    nop  

OREDENA:
    PUSH    R5  ; SERA USADO PARA PERCORRER A LISTA
    PUSH    R6  ; IRA ARMAZENAR OS VALORES DURANTE AS COMPARACOES
    PUSH    R7  ; GUARDA R12 PARA CADA ITERACAO
    CMP     0x0002, R13 ;COMPARA 2 >=R13, SE MENOR QUE ISSO O VETOR NAO PRECISA DE ORDENACAO
    JGE     INICIO_LOOP ; JOGA PARA O LOOP

    ; SO RETORNA SE O VETOR FOR VAZIO OU TAMANHO 1
    POP     R7
    POP     R6
    POP     R5
    RET
INICIO_LOOP:
    MOV.W     R13,      R5     ; COLOCA O NUMERO DE MEBROS A ORDENAR EM R5
    MOV.w     #R12,     R7     ; APONTA PARA O INICIO DO VERTOR
    CMP     0X0002,     R13    ; VE SE JA ORDENOU TODOS, SE RESTA APENAS 1
    JGE     COMPARACAO_LOOP    ; VAI PARA A IRECAO DE ORDENACAO

    ; TERMINA A ORDENACAO
    POP     R7
    POP     R6
    POP     R5
    RET

COMPARACAO_LOOP:
    CMP     0X0002,     R5      ; VERIFICA O FIM DA ITERACAO
    JGE     CONTINUA_LOOP       ; VAI PARA A COMPARACAO
    ; REDUZ O VALOR DE R13
    DEC     R13
    JUMP    INICIO_LOOP         ; VOLTA PARA O INICIO PARA PROXIMA ITERACAO
    NOP

CONTINUA_LOOP:
    ;ADCIONA O VALOR A R6 E AVANCA R7 PARA O PROXIMO ENDERECO DE MEMORIA
    MOV.W   @R7+,    R6
    DEC     R5                  ; DECREMENTA R5
    CMP     R6,  @R7            ; VERIFICA SE O TERMO SEGUINTE<ANTERIOR
    JGE      COMPARACAO_LOOP    ; VOLTA PARA O INICIO SE ESTA CORRETA A ORDENACAO
    MOV.w   @R7, -2(R7)         ; COPIA O TERMO SEGUINTE PARA O ANTERIOR
    MOV.W   R6, 0(R7)           ; SALVA O TERMO ANTERIOR QUE > QUE O SEGUINTE NA MEMORIA
    JMP     COMPARACAO_LOOP     ; VOLTA PARA O INICIO PARA CONTINUAR A ITERACAO
    NOP
;-------------------------------------------------------------------------------
    .data
vetor1: .word 7, 65000, 50054, 26472, 53000, 60606, 814, 41121

