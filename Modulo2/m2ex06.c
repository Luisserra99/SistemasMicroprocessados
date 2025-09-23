#include <msp430.h>


void main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    // Configuração do LED
    P4DIR |= BIT7;              // Configura P4.7 (LED verde) como saída DIR=1
    P4OUT &= ~BIT7;             // Garante que o LED comece desligado P1OUT=0

    TA0CCR0 = 16383; //32768 ->1s, então 16384 igual a 0,5s

    // Configura o timer
    TA0CTL = TASSEL_1 | MC_1 | TACLR ;

    while(1) { 

        while ( !(TA0CCTL0  & CCIFG) ); // Até estourar o tempo definido no TASSEL TAIFG é zero
        
        P4OUT ^= BIT7;         // Altere o estado do LED
        
        TA0CCTL0 &= ~CCIFG;    // Zera o CCIFG
    }
}
