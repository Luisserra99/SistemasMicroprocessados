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
volatile unsigned int dutyCycle = 128; // Inicia em 50% (128/256)

// Passo de incremento/decremento (12.5% de 256)
#define DUTY_STEP 32

// Debounce dos botões
void debounce(void) {

    volatile unsigned int i;

    for (i = 20000; i > 0; i--);

    // Reabilita as interrupções dos botões após o período de debounce
    P1IE |= (BIT1);
    P2IE |= (BIT1);
};


void main(void) {
    // 1. Desabilitar o Watchdog Timer
    WDTCTL = WDTPW | WDTHOLD;

    // 2. Configurar o pino P1.2 para a função do Timer (TA0.1)
    P1DIR |= BIT2;          // Configura P1.2 como saída
    P1SEL |= BIT2;          // Seleciona a função do periférico (TA0.1) para o pino P1.2

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

    // 4. Configurar o Timer_A0
    TA0CTL = TASSEL__ACLK |    // Fonte de clock: ACLK (32768 Hz)
             MC__UP |        // Modo de contagem: UP (de 0 até TA0CCR0)
             TACLR;        // Limpa o contador do timer

    // 5. Definir o período e o duty cycle
    TA0CCR0 = 255;          // Período: Frequência = 32768 / (255 + 1) = 128 Hz
    TA0CCR1 = dutyCycle;    // Define o duty cycle inicial
    TA0CCTL1 = OUTMOD_7;    // Modo de saída 7: Reset/Set

    // 7. Habilitar interrupções
    __enable_interrupt();
}

// Rotina de Serviço de Interrupção (ISR) para a Porta 1
#pragma vector=PORT1_VECTOR
__interrupt void Port_1_ISR(void)
{

    // Verifica qual botão foi pressionado
    if (P2IFG & BIT1) // Botão S1 (Diminuir)
    {
        // Inicia o debounce desabilitando as interrupções dos botões
        P2IE &= ~(BIT1);

        if (dutyCycle >= DUTY_STEP)
        {
            dutyCycle -= DUTY_STEP;
        }
        else
        {
            dutyCycle = 0; // Limite inferior
        }
    }
    else if (P1IFG & BIT1) // Botão S2 (Aumentar)
    {
        // Inicia o debounce desabilitando as interrupções dos botões
        P1IE &= ~(BIT1);

        if (dutyCycle <= (255 - DUTY_STEP))
        {
            dutyCycle += DUTY_STEP;
        }
        else
        {
            dutyCycle = 255; // Limite superior
        }
    }

    // Atualiza o registrador do timer com o novo valor de duty cycle
    TA0CCR1 = dutyCycle;

    // Limpa os flags de interrupção
    P1IFG &= (BIT1);
    P2IFG &= (BIT1);

    // Inicia o debounce
    debounce();            // Chama a função debounce
}
