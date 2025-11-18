/*
Módulo 2 Exercício 18 - MSP430F5529 FIXED
Changes:
1. Added Port Mapping (PMAP) to connect Timer B0 to Pin 4.7 (LED2).
2. LED2 is now driven by TB0CCR2 (Hardware PWM).
3. LED2 starts at 50% brightness.
*/
#include <msp430.h>
#include <stdint.h>

void ConfigureLeds();
void Config_Port_Mapping(); // NEW FUNCTION

// --- IR TIMING ---
#define LOW_MIN   8000
#define LOW_MAX   10000
#define HIGH_MIN  3000
#define HIGH_MAX  6000
#define BIT_MIN   400
#define BIT_MAX   800
#define ONE_MIN   1400
#define ONE_MAX   1900

#define PWM_PERIOD 1000
#define PWM_STEP   125

volatile unsigned int lastCapture = 0;
volatile unsigned int pulseWidth = 0;
volatile uint32_t command = 0;
volatile unsigned char bitIndex = 0;
volatile unsigned char irState = 0;
volatile unsigned char commandReady = 0;

int led2_level = 4; 

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog

    // 1. Configure Pins
    P1DIR |=  BIT0;             // LED1 (Red) Output
    P1OUT &= ~BIT0;
    
    P4DIR |= BIT7;              // LED2 (Green) Output
    P4SEL |= BIT7;              // Enable Peripheral function on P4.7

    P1DIR &= ~BIT2;             // IR Sensor Input
    P1SEL |= BIT2;

    // 2. EXECUTE PORT MAPPING (CRITICAL STEP)
    // This internally connects Timer B0 to P4.7
    Config_Port_Mapping();

    // 3. Setup Timer A0 (IR Decoding)
    TA0CTL = TASSEL_2 | MC__CONTINOUS | TACLR; // SMCLK
    TA0CCTL1 = CM_3 | CCIS_0 | SCS | CAP | CCIE;

    // 4. Setup Timer B0 (PWM for LED2)
    TB0CTL   = TASSEL_2 | ID__8 | MC__UP | TACLR; // SMCLK / 8
    TB0CCR0  = PWM_PERIOD;            
    TB0CCTL2 = OUTMOD_7;              // Reset/Set Mode (PWM)
    TB0CCR2  = 4 * PWM_STEP;          // Start at 50%

    __enable_interrupt();

    while (1)
    {
        if(commandReady) {
             commandReady = 0;
             // Logic is handled in ConfigureLeds called from ISR
        }
    }
}

// This function "wires" the internal Timer B0 to physical pin P4.7
void Config_Port_Mapping()
{
    // Unlock Port Mapping Controller
    PMAPKEYID = PMAPKEY;      
    PMAPCTL |= PMAPRECFG;     

    // Map P4.7 to Timer B0 CCR2 Output (PM_TB0CCR2A)
    // Note: P4MAP7 is the register for Pin 4.7
    P4MAP7 = PM_TB0CCR2A; 

    // Lock Port Mapping Controller
    PMAPKEYID = 0;            
}

void ConfigureLeds()
{
    switch (command)
    {
        case 0xFFA857: // Toggle LED 1
            P1OUT |= BIT0;
            break;

        case 0xFFE01F: // Toggle LED 1
            P1OUT &= ~BIT0;
            break;

        case 0xFFE21D: // INCREASE LED2
            if(led2_level < 8) {
                led2_level++;
                TB0CCR2 = led2_level * PWM_STEP;
            }
            break;

        case 0xFFA25D: // DECREASE LED2
            if(led2_level > 0) {
                led2_level--;
                TB0CCR2 = led2_level * PWM_STEP;
            }
            break;
    }
}

// --- IR SENSOR INTERRUPT ---
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
                irState = 1;
            break;

        case 1: // Waiting for 4.5 ms high
            if (!edgeHigh && (pulseWidth > HIGH_MIN && pulseWidth < HIGH_MAX)) {
                irState = 2;
                command = 0;
                bitIndex = 0;
            }
            else if (!edgeHigh) irState = 0;
            break;

        case 2: // Receiving bits
            if (!edgeHigh) {
                if (pulseWidth > BIT_MIN && pulseWidth < BIT_MAX)
                    command &= ~(1UL << (31 - bitIndex));
                else if (pulseWidth > ONE_MIN && pulseWidth < ONE_MAX)
                    command |= (1UL << (31 - bitIndex));
                else
                    irState = 0;

                bitIndex++;
                if (bitIndex >= 32) {
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