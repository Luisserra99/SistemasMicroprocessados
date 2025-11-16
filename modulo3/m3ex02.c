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

int i2cScan(uint8_t * addrs);


#define MAX_I2C_ADDRS 127 // Máximo de endereços possíveis (0x01 a 0x7F)

/**
 * main.c - Adaptado para o Exercício 2
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    // Configura o I2C
    initialize_I2C_UCB0_MasterTransmitter();

    // Array para armazenar os endereços I2C encontrados
    uint8_t found_addrs[MAX_I2C_ADDRS];
    // Variável para armazenar o número de dispositivos encontrados
    volatile int num_devices = 0;

    // 1. Executa o scanner de endereços
    num_devices = i2cScan(found_addrs);

    while (1){};

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

/**
 * Varre o barramento I2C e armazena os endereços que responderam com ACK.
 *
 * param: addrs Ponteiro para um array onde os endereços encontrados serão armazenados.
 * return: O número de endereços I2C encontrados.
 */
int i2cScan(uint8_t * addrs)
{
    uint8_t current_addr;
    int found_count = 0;

    // Desliga todas as interrupções antes de começar
    UCB0IE = 0;

    // Varre de 0x01 até 0x7F (endereços de 7 bits)
    for (current_addr = 0x01; current_addr <= 0x7F; current_addr++)
    {
        // Coloca o endereço do "slave" (dispositivo)
        UCB0I2CSA = current_addr;

        // Espera a linha estar desocupada (opcional, mas bom para garantir)
        while (UCB0STAT & UCBBUSY);

        // Pede um START e, ao mesmo tempo, força um STOP (dica do exercício)
        // Isso faz com que apenas o endereço seja enviado, sem byte de dados.
        // O UCTXSTP deve ser setado *antes* do UCTXSTT no modo Mestre/Transmissor.
        UCB0CTL1 |= (UCTXSTT | UCTXSTP);

        // Espera o START ser concluído (a flag UCTXSTT ser limpa pelo hardware)
        // Isso indica que o endereço foi enviado e o bit de ACK/NACK foi recebido.
        while (UCB0CTL1 & UCTXSTT);

        // Verifica se houve um NACK após a transmissão do endereço
        if ((UCB0IFG & UCNACKIFG) == 0)
        {
            // Não houve NACK, então houve um ACK: Endereço encontrado!
            addrs[current_addr] = 1;
            found_count++;
        }
        else{
            addrs[current_addr] = 0;
        }

        // Limpa a flag NACK para a próxima iteração, caso tenha ocorrido
        UCB0IFG &= ~UCNACKIFG;
    }

    return found_count;
}
