#include <msp430.h>

/**
 * Conexão de Hardware (IMPORTANTE):
 * - Remover o jumper JP8 e conecatar o pino da borda ao P1.2(referente ao T0.1)
 *
 * Descrição: Gera um PWM de 128Hz no pino P1.2. O duty cycle pode ser
 * ajustado em passos de 12.5% usando dois botões.
 * - S2 (P1.1): Aumenta o duty cycle.
 * - S1 (P2.1): Diminui o duty cycle.
 *
 * Cálculos:
 * - Clock (ACLK) = 32768 Hz
 * - Frequência PWM = 128 Hz
 * - Período = 32768 / 128 Hz = 256 ciclos de clock
 * - TA0CCR0 = 256 - 1 = 255 (Define o período)
 * - Duty Cycle = 50%
 * - TA0CCR1 = 128 (Define o ponto de transição do duty cycle)
 */


// A variável de duty cycle precisa ser 'volatile' porque ela é modificada
// na ISR e lida pelo hardware do timer.

volatile unsigned int counter = 0;


// Debounce dos botões
void debounce(void) {

    volatile unsigned int i;

    for (i = 2000000; i > 0; i--);

};


void main(void) {
    // 1. Desabilitar o Watchdog Timer
    WDTCTL = WDTPW | WDTHOLD;

    // 2. Configurar os LEDS
    P1DIR |= BIT0;              // Configura P1.0 (LED vermelho) como saída DIR=1
    P1OUT &= ~BIT0;             // Garante que o LED comece desligado P1OUT=0
    
    P4DIR |= BIT7;              // Configura P4.7 (LED vermelho) como saída DIR=1
    P4OUT &= ~BIT7;             // Garante que o LED comece desligado P4OUT=0


    // 3. Configurar os pinos dos botões (P1.1 e P1.3)
    P1DIR &= ~(BIT1);   // Define P1.1 como entrada
    P2DIR &= ~(BIT1);   // Define P2.1 como entrada

    P1REN |= (BIT1);    // Habilita resistores internos
    P2REN |= (BIT1);    // Habilita resistores internos

    P1OUT |= (BIT1);    // Configura os resistores como pull-up
    P2OUT |= (BIT1);    // Configura os resistores como pull-up

    P1IES |= (BIT1);    // Interrupção na borda de descida (high-to-low)
    P2IES |= (BIT1);    // Interrupção na borda de descida (high-to-low)

    P1IFG &= ~(BIT1);   // Limpa flags de interrupção
    P2IFG &= ~(BIT1);   // Limpa flags de interrupção

    P1IE  |= (BIT1);    // Habilita interrupções para P1.1
    P2IE  |= (BIT1);    // Habilita interrupções para P2.1


    // 7. Habilitar interrupções
    __enable_interrupt();
}

// Rotina de Serviço de Interrupção (ISR) para a Porta 1
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{

    // Inicia o debounce desabilitando as interrupções dos botões
    P1IE &= ~(BIT1);
    P2IE &= ~(BIT1);

    if (counter <= 1)
    {
        counter = 0;
        P4OUT &= ~BIT7;
        P1OUT &= ~BIT0;
    }
    else
    {
        if (counter == 2)
        {
        counter = 1;
        P4OUT |= BIT7;
        P1OUT &= ~BIT0;   
        }
        else {
        counter = 2;
        P4OUT &= ~BIT7;
        P1OUT |= BIT0; 
        }
    }

    // Inicia o debounce
    debounce();            // Chama a função debounce

    // Limpa os flags de interrupção
    P1IFG &= ~(BIT1);
    P1IE |= (BIT1);
    P2IE |= (BIT1);
}

// Rotina de Serviço de Interrupção (ISR) para a Porta 2
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{

    // Inicia o debounce desabilitando as interrupções dos botões
    P1IE &= ~(BIT1);
    P2IE &= ~(BIT1);

    if (counter >= 2)
    {
        counter = 3;
        P4OUT |= BIT7;
        P1OUT |= BIT0;
    }
    else
    {
        if (counter == 1)
        {
        counter = 2;
        P4OUT &= ~BIT7;
        P1OUT |= BIT0;   
        }
        else {
        counter = 1;
        P4OUT |= BIT7;
        P1OUT &= ~BIT0; 
        }
    }


    // Inicia o debounce
    debounce();            // Chama a função debounce

    // Limpa os flags de interrupção
    P2IFG &= ~(BIT1);
    P1IE |= (BIT1);
    P2IE |= (BIT1);
}