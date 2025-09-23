#include <msp430.h>

void debounce(void) {

    volatile unsigned int i;

    for (i = 20000; i > 0; i--);
};


void main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    // 2. Configuração dos pinos
    P1DIR |= BIT0;              // Configura P1.0 (LED vermelho) como saída DIR=1
    P1OUT &= ~BIT0;             // Garante que o LED comece desligado P1OUT=0

    // Botão S1
    P2DIR &= ~BIT1;             // Configura P2.1 (Botão S1) como entrada DIR=0
    P2REN |= BIT1;              // Habilita o resistor interno para P2.1 REM=1
    P2OUT |= BIT1;              // Configura o resistor como pull-up P2OUT=1

    // Botão S2
    P1DIR &= ~BIT1;             // Configura P1.1 (Botão S2) como entrada DIR=0
    P1REN |= BIT1;              // Habilita o resistor interno para P1.1 REM=1
    P1OUT |= BIT1;              // Configura o resistor como pull-up P1OUT=1

    while(1) { 

        while (( (P2IN & BIT1) && (P1IN & BIT1) )); // Fica preso no loop até apertar o botão
        P1OUT ^= BIT0;         // Altere o estado do LED
        debounce();            // Chama a função debounce
    }
}