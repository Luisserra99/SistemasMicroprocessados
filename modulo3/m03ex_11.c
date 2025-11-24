#include <msp430.h>

// Variáveis globais para o gráfico
// Usamos char (8 bits) pois a resolução do ADC foi configurada para 8 bits.
unsigned int v = 0;

void init_ADC();
void init_timer();
void start_timer();

/*
 * main.c
 */
int main(void) {

    // Stop watchdog timer
    WDTCTL = WDTPW | WDTHOLD;

    init_timer();
    init_ADC();

    __enable_interrupt();

    start_timer();

    while (1)
    {
        // O processamento principal ocorre na interrupção.
        // Pode-se colocar o microcontrolador em Low Power Mode aqui se desejar:
        // __bis_SR_register(LPM0_bits + GIE);
        __no_operation();
    }

}

/*************************************************
 * INTERRUPT FUNCTIONS
 *************************************************/

#pragma vector = ADC12_VECTOR
__interrupt void ADC12_interrupt(void)
{
    switch (_even_in_range(ADC12IV, 0x24))
    {
    case ADC12IV_NONE: break;
    case ADC12IV_ADC12OVIFG: break;
    case ADC12IV_ADC12TOVIFG: break;
    case ADC12IV_ADC12IFG0:       // MEM0 Ready
        // Armazena o valor no vetor circularmente
        v = ADC12MEM0;

        break;
    default: break;
    }
}

void init_ADC()
{
    // Configuro o P6.0 para o pino A0 do ADC.
    P6SEL |= BIT0;

    // Desliga o módulo para permitir alterações
    ADC12CTL0 &= ~ADC12ENC;

    ADC12CTL0 = ADC12SHT0_3 |      // 32 ciclos para o tsample
                ADC12ON;           // Liga o ADC

    ADC12CTL1 = ADC12CSTARTADD_0 | // Start address: 0
                ADC12SHS_1 |       // Conversão disparada pelo TimerA0.1 (Trigger)
                ADC12SHP |         // Sample and Hold Pulse mode: input
                ADC12DIV_0 |       // Divide o clock por 1
                ADC12SSEL_0 |      // Escolhe o clock MODCLK
                ADC12CONSEQ_2;     // Modo: single channel / REPEAT conversion

    // ALTERAÇÃO AQUI: Resolução de 8 bits (ADC12RES_0)
    ADC12CTL2 = ADC12TCOFF |       // Desliga sensor temp
                ADC12RES_0;        // 8 bits resolution

    // Configurações dos canais
    ADC12MCTL0 = ADC12SREF_0 |     // Vcc/Vss
                 ADC12INCH_0;      // Amostra o pino A0

    ADC12IE = ADC12IE0;            // Liga a interrupção do canal 0

    // Habilita conversão
    ADC12CTL0 |= ADC12ENC;
}

void init_timer()
{
    // Configuração para 100ms
    // Clock Source: ACLK (32768 Hz)
    // 32768 * 0.1s = 3276.8 -> Arredondado para 3277

    TA0CTL = TASSEL__ACLK |     // ACLK
             MC__STOP;          // Parado inicialmente

    TA0CCTL1 = OUTMOD_2;        // Modo Toggle/Reset para gerar o trigger para o ADC

    TA0CCR0 = 3277;             // Período total: ~100ms
    TA0CCR1 = 1638;             // Duty cycle ~50% (o trigger ocorre aqui)
}

void start_timer()
{
    TA0CTL |= (MC__UP | TACLR); // Inicia o timer em modo UP
}
