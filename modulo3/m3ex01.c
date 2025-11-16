#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

// - Esse código transmite 0x00 / 0xFF para o LCD
// - Endereço do LCD: 0x3F
// - UCB0 é configurado como MASTER TRANSMITTER
//Hardware setup:
// - P3.0 - SDA
// - P3.1 - SCL
// - Alimentar o LCD e conectar SDA / SCL
// - O código liga os resistores de pull-up internos



void initialize_I2C_UCB0_MasterTransmitter();

void i2cSend(uint8_t addr, uint8_t data);

void delay_us(unsigned int time_us);


/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    // Configura o I2C
    initialize_I2C_UCB0_MasterTransmitter();

    while(1)
    {

        i2cSend(0x27, 0x08);

        volatile int n;
        for (n = 0; n < 10; n++)
        {
            delay_us(50000);
        }

        i2cSend(0x27, 0x00);

        for (n = 0; n < 10; n++)
        {
            delay_us(50000);
         }

        }

        return 0;
}

/*
 *  P3.0 - SDA
 *  P3.1 - SCL
 */
void initialize_I2C_UCB0_MasterTransmitter()
{
    //Desliga o módulo
    UCB0CTL1 |= UCSWRST;

    //Configura os pinos
    P3SEL |= BIT0;     //Configuro os pinos para "from module"
    P3SEL |= BIT1;
    P3REN |= BIT0; 
    P3REN |= BIT1;
    P3OUT |= BIT0; 
    P3OUT |= BIT1;


    UCB0CTL0 = UCMST |           //Master Mode
                          UCMODE_3 |    //I2C Mode
                          UCSYNC;         //Synchronous Mode

    UCB0CTL1 = UCSSEL__ACLK |    //Clock Source: ACLK
                          UCTR |                      //Transmitter
                          UCSWRST ;             //Mantém o módulo desligado

    //Divisor de clock para o BAUDRate
    UCB0BR0 = 2;
    UCB0BR1 = 0;

    //Liga o módulo.
    UCB0CTL1 &= ~UCSWRST;

    //Se eu quisesse ligar interrupções eu iria fazer isso aqui, depois de re-ligar o módulo..
}

void i2cSend(uint8_t addr, uint8_t data)
{
    //Desligo todas as interrupções
    UCB0IE = 0;

    //Coloco o slave address
    UCB0I2CSA = addr;

    //Espero a linha estar desocupada.
    if (UCB0STAT & UCBBUSY) return;

    //Peço um START
    UCB0CTL1 |= UCTXSTT;

    //Espero até o buffer de transmissão estar disponível
    while ((UCB0IFG & UCTXIFG) == 0);

    //Escrevo o dado
    UCB0TXBUF = data;

    //Aguardo o acknowledge
    while (UCB0CTL1 & UCTXSTT);

    //Verifico se é um ACK ou um NACK
    if ((UCB0IFG & UCNACKIFG) != 0)
    {
        //Peço uma condição de parada
        UCB0CTL1 |= UCTXSTP;
    } else
    {
        //Peço uma condição de parada
        UCB0CTL1 |= UCTXSTP;
    }

    return;
}

/*
 * Delay microsseconds.
 */
void delay_us(unsigned int time_us)
{
    //Configure timer A0 and starts it.
    TA0CCR0 = time_us;
    TA0CTL = TASSEL__SMCLK | ID__1 | MC_1 | TACLR;

    //Locks, waiting for the timer.
    while((TA0CTL & TAIFG) == 0);

    //Stops the timer
    TA0CTL = MC_0 | TACLR;
}
