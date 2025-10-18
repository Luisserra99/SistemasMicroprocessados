#include <msp430.h>

/**
 * Exercício 8: PWM com duty cycle fixo
 * Conexão:
 * - P1.0 -> LED Vermelho
 *
 * Cálculos:
 * - Clock (ACLK) = 32768 Hz
 * - Frequência PWM = 128 Hz
 * - Período = 32768 / 128 Hz = 256 ciclos de clock
 * - TA0CCR0 = 256 - 1 = 255 (Define o período)
 * - Duty Cycle = 50%
 * - TA0CCR1 = 256 * 0.5 = 128 (Define o tempo em alto)
 */

void main(void) {
    // 1. Desabilitar o Watchdog Timer
    WDTCTL = WDTPW | WDTHOLD;

    // 2. Configurar o pino do LED (P1.0) como saída
    P1DIR |= BIT0;          // P1.0 como saída
    P1OUT &= ~BIT0;         // Inicia o LED desligado


    // 3. Configurar o Timer_A0
    TA0CTL = TASSEL__ACLK |    // Fonte de clock: ACLK ( 32768 Hz)
             MC__UP |        // Modo de contagem: UP (de 0 até TA0CCR0)
             TACLR;        // Limpa o contador do timer


    // 4. Definir os valores de período e duty cycle
    TA0CCR0 = 255;         // Frequência: 32768 Hz / (255 + 1) = 128 Hz
    TA0CCR1 = 128;    // Duty cycle: (128 / 256) = 50%    // Duty cycle: (128 / 256) = 50%


    // 5. Habilitar as interrupções para CCR0 e CCR1
    TA0CCTL0 |= CCIE;       // Habilita interrupção para CCR0
    TA0CCTL1 |= CCIE;       // Habilita interrupção para CCR1


    // 6. Habilitar interrupções
    __enable_interrupt();
}

// Rotina de Serviço de Interrupção (ISR) para o vetor do Timer0_A0 (apenas TA0CCR0)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A0_ISR (void)
{
    // Esta interrupção ocorre quando o timer atinge TA0CCR0 e volta para 0.
    // Este é o início de um novo ciclo PWM.
    P1OUT |= BIT0;          // Liga o LED (início do pulso)
}

// Rotina de Serviço de Interrupção (ISR) para o vetor do Timer0_A1 (TA0CCR1, TA0CCR2, TAIFG)
#pragma vector=TIMER0_A1_VECTOR
__interrupt void Timer_A1_ISR (void)
{
    // A interrupção pode vir de várias fontes, então usamos um switch no registrador TA0IV
    // para identificar qual flag a causou. Ler TA0IV também limpa o flag.
    switch(__even_in_range(TA0IV, TA0IV_TAIFG))
    {
        case TA0IV_NONE:   break;               // Nenhuma interrupção pendente
        case TA0IV_TACCR1:                      // Interrupção do CCR1
            P1OUT &= ~BIT0;                     // Desliga o LED (fim do pulso)
            break;
        case TA0IV_TACCR2: break;               // Não estamos usando CCR2
        case TA0IV_TAIFG:  break;               // Não estamos usando overflow
        default: break;
    }
}