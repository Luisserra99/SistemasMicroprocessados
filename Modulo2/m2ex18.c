/*
Módulo 2 Exercício 18
Controlar itensidade dos LEDS com o controle IR
Gustavo Henrique Gomes Barbosa - 232002771
*/
#include <msp430.h>
#include <stdint.h>

void ConfigureLeds();

// Signal Lengths:
#define LOW_MIN   8000
#define LOW_MAX   10000
#define HIGH_MIN  3000
#define HIGH_MAX  6000
#define BIT_MIN   400
#define BIT_MAX   800
#define ONE_MIN   1400
#define ONE_MAX   1900

#define duty_modification 32

volatile unsigned int lastCapture = 0;
volatile unsigned int pulseWidth = 0;

volatile uint32_t command = 0;
volatile unsigned char bitIndex = 0;

volatile unsigned char irState = 0;                                // 0 = idle, 1 = got 9 ms low, 2 = receiving bits
volatile unsigned char commandReady = 0;                           // flag when 32 bits received

int led1_level = 1;
int led2_level = 1;

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;                                      // Stop watchdog

    P1DIR |=  BIT0;                                                // P1.0 (LED1) set as output
    P1OUT &= ~BIT0; 

    P4DIR |=  BIT7;                                                // P4.7 (LED2) set as output
    P4OUT &= ~BIT7; 

    P1DIR &= ~BIT2;                                                // P1.2 as input
    P1SEL |= BIT2;                                                 // TA0.1 capture input

    P2SEL |=  BIT4;                                                // Setting P1.2 as output for
    P2DIR |=  BIT4;                                                // TA2.4 outmode 
    TA2CCTL1 = OUTMOD_7;

    P2SEL |=  BIT5;                                                // Setting P2.5 as output for
    P2DIR |=  BIT5;                                                // TA2.5 outmode 
    TA2CCTL2 = OUTMOD_7;

    // Timer A0: SMCLK, continuous, capture on both edges
    TA0CTL = TASSEL_2 | MC__CONTINOUS | TACLR;
    TA0CCTL1 = CM_3 | CCIS_0 | SCS | CAP | CCIE;

    // Timer A2: ACLK, up, for duty cicle to control LED1
    TA2CTL   = TASSEL__ACLK | MC__UP | TACLR;
    TA2CCR0  = 255;                                                // Period = ~128Hz
    TA2CCR1  = 0;                                                  // 0% duty cycle. Control LED1
    TA2CCR2  = 0;                                                  // 0% duty cycle. Control LED2


    __enable_interrupt();

    while (1)
    {
        if (commandReady)
        {
            commandReady = 0;
        }
    }
}

void ConfigureLeds()
{
    switch (command) 
    {
        case 0xFF38C7:
            TA2CCR1  = 0;                                                  
            TA2CCR2  = 0;
            led1_level = 1;
            led2_level = 1;
            break;
        
        case 0xFF5AA5:
            if(led1_level < 8)
            {
                TA2CCR1 += duty_modification;
                led1_level++;
            }
            break;

        case 0xFF10EF:
            if(led1_level > 1)
            {
                TA2CCR1 -= duty_modification;
                led1_level--;
            }
            break;

        case 0xFF18E7:
            if(led2_level < 8)
            {
                TA2CCR2 += duty_modification;
                led2_level++;
            }
            break;

        case 0xFF4AB5:
            if(led2_level > 1)
            {
                TA2CCR2 -= duty_modification;
                led2_level--;
            }
            break;
        
        default:
            break;
    }
}

// TimerA0 Capture/Compare ISR
#pragma vector = TIMER0_A1_VECTOR
__interrupt void Timer0_A1_ISR(void)
{
    unsigned int capture = TA0CCR1;
    pulseWidth = capture - lastCapture;
    unsigned char edgeHigh = (TA0CCTL1 & CCI) ? 1 : 0;

    switch (irState)
    {
        case 0: // Waiting for 9 ms low
            if (edgeHigh && (pulseWidth > LOW_MIN && pulseWidth < LOW_MAX))
                irState = 1;   // Low detected
            break;

        case 1: // Waiting for 4.5 ms high
            if (!edgeHigh && (pulseWidth > HIGH_MIN && pulseWidth < HIGH_MAX))
            {
                irState = 2;   // Start confirmed, begin data
                command = 0;
                bitIndex = 0;
            }
            else if (!edgeHigh)
                irState = 0;   // invalid pulse, reset
            break;

        case 2: // Receiving bits
            if (!edgeHigh)  // falling edge = end of high pulse
            {
                if (pulseWidth > BIT_MIN && pulseWidth < BIT_MAX)
                    command &= ~(1UL << (31 - bitIndex));   // logical 0
                else if (pulseWidth > ONE_MIN && pulseWidth < ONE_MAX)
                    command |= (1UL << (31 - bitIndex));    // logical 1
                else
                    irState = 0; // invalid pulse, reset

                bitIndex++;

                if (bitIndex >= 32)
                {
                    irState = 0;
                    commandReady = 1;
                    ConfigureLeds();
                }
            }
            break;
    }

    lastCapture = capture;
    TA0CCTL1 &= ~CCIFG;
}
