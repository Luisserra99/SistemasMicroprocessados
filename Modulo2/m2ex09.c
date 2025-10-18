#include <msp430.h>

/**
 * Conexão de Hardware (IMPORTANTE):
 * - Remover o jumper JP8 e conecatar o pino da borda ao P1.2(referente ao T0.1)
 *
 * Cálculos:
 * - Clock (ACLK) = 32768 Hz
 * - Frequência PWM = 128 Hz
 * - Período = 32768 / 128 Hz = 256 ciclos de clock
 * - TA0CCR0 = 256 - 1 = 255 (Define o período)
 * - Duty Cycle = 50%
 * - TA0CCR1 = 128 (Define o ponto de transição do duty cycle)
 */
void main(void) {
    // 1. Desabilitar o Watchdog Timer
    WDTCTL = WDTPW | WDTHOLD;

    // 2. Configurar o pino P1.2 para a função do Timer (TA0.1)
    P1DIR |= BIT2;          // Configura P1.2 como saída
    P1SEL |= BIT2;          // Seleciona a função do periférico (TA0.1) para o pino P1.2

    // 3. Configurar o Timer_A0
    TA0CTL = TASSEL__ACLK |    // Fonte de clock: ACLK (32768 Hz)
             MC__UP |        // Modo de contagem: UP (de 0 até TA0CCR0)
             TACLR;        // Limpa o contador do timer

    // 4. Definir o período e o duty cycle
    TA0CCR0 = 255;          // Período: Frequência = 32768 / (255 + 1) = 128 Hz
    TA0CCR1 = 128;          // Duty Cycle: 128 / 256 = 50%

    // 5. Configurar o modo de saída do canal 1 (TA0.1)
    // O modo "Reset/Set" é o ideal para PWM.
    // O hardware irá:
    // - SET (ligar) a saída quando o timer chegar a 0.
    // - RESET (desligar) a saída quando o timer chegar a TA0CCR1.
    TA0CCTL1 = OUTMOD_7;    // Modo de saída 7: Reset/Set
}
