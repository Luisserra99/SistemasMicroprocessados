#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

// -- Definições do LCD e I2C --
// Endereço do LCD
#define LCD_ADDR 0x27 

// Mapeamento de bits no PCF8574
#define RS_BIT   BIT0  // 0x01: Register Select (0=Cmd, 1=Char)
#define RW_BIT   BIT1  // 0x02: Read/Write (Geralmente 0)
#define EN_BIT   BIT2  // 0x04: Enable
#define BL_BIT   BIT3  // 0x08: Backlight (1=Ligado)

// -- Comandos da Tabela --
#define CMD_CLEAR_DISPLAY     0x01 // Limpa display (esperar > 1.53ms)
#define CMD_RETURN_HOME       0x02 // Retorna cursor ao inicio
#define CMD_ENTRY_MODE_SET    0x06 // (Padrão: inc cursor, sem shift)
#define CMD_DISPLAY_CONTROL   0x0F // (D=1, C=0, B=0) -> Display ON
#define CMD_FUNCTION_SET      0x28 // (M=0, L=1, F=0) -> 4-bit, 2 linhas

// -- Protótipos das Funções --
void initialize_I2C_UCB0_MasterTransmitter();
void i2cSend(uint8_t addr, uint8_t data);
void delay_us(unsigned int time_us);

// Funções do LCD (Exercícios 3, 4 e 5)
void lcdWriteNibble(uint8_t nibble, uint8_t isChar);
void lcdWriteByte(uint8_t byte, uint8_t isChar);
void lcdInit();

/**
 * main.c
 */
int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer

    // 1. Configura o Hardware I2C
    initialize_I2C_UCB0_MasterTransmitter();

    // 2. Inicializa o LCD (Exercício 5)
    lcdInit();

    // 3. Exemplo de Uso: Escrevendo "Hello World"
    lcdWriteByte('H', 1);
    lcdWriteByte('e', 1);
    lcdWriteByte('l', 1);
    lcdWriteByte('l', 1);
    lcdWriteByte('o', 1);
    lcdWriteByte(' ', 1);
    lcdWriteByte('W', 1);
    lcdWriteByte('o', 1);
    lcdWriteByte('r', 1);
    lcdWriteByte('l', 1);
    lcdWriteByte('d', 1);

    while(1)
    {
        // Loop principal
    }
}

/*
 * ---------------------------------------------------------
 * IMPLEMENTAÇÃO DAS FUNÇÕES DO LCD
 * ---------------------------------------------------------
 */

// Exercício 3: Escreve 4 bits (Nibble) gerando o pulso de Enable
// isChar: 0 = Instrução, 1 = Dado
void lcdWriteNibble(uint8_t nibble, uint8_t isChar)
{
    // Prepara o byte a ser enviado para o LCD
    // Mascara para garantir que pegamos apenas os 4 bits superiores (D7-D4)
    uint8_t i2cValue = (nibble & 0xF0);

    // Adiciona o bit de Backlight (sempre ligado)
    i2cValue |= BL_BIT;

    // Configura RS (Register Select)
    if (isChar) {
        i2cValue |= RS_BIT; // RS=1 (Dado)
    } else {
        i2cValue &= ~RS_BIT; // RS=0 (Comando)
    }

    // Garante que RW = 0 (Write) e EN começa em 0
    i2cValue &= ~(RW_BIT | EN_BIT); // R/W=0

    // 1. Envia dados com EN = 0 (Preparação)
    i2cSend(LCD_ADDR, i2cValue);

    // 2. Sobe o Enable (EN = 1)
    i2cSend(LCD_ADDR, i2cValue | EN_BIT);
    delay_us(1);

    // 3. Desce o Enable (EN = 0) - O LCD lê na descida
    i2cSend(LCD_ADDR, i2cValue);
    delay_us(1);
}

// Exercício 4: Escreve um Byte completo (2 Nibbles)
void lcdWriteByte(uint8_t byte, uint8_t isChar)
{
    // Envia os 4 bits mais significativos (High Nibble)
    lcdWriteNibble(byte, isChar);
    // Envia os 4 bits menos significativos (Low Nibble)
    // Desloca 4 casas para a esquerda para que fiquem na posição D7-D4
    lcdWriteNibble(byte << 4, isChar);
    delay_us(50);
}

// Exercício 5: Inicialização do LCD
void lcdInit()
{
    // Aguarda estabilização da tensão (>15ms)
    delay_us(50000);

    // --- Sequência de Reset para garantir Modo 8 bits ---
    // Envia 0x03 três vezes (apenas nibble superior 0x30)
    lcdWriteNibble(0x30, 0);
    delay_us(5000); // Espera > 4.1ms

    lcdWriteNibble(0x30, 0);
    delay_us(200);  // Espera > 100us

    lcdWriteNibble(0x30, 0);
    delay_us(200);

    // --- Configura para Modo 4 bits ---
    // Envia 0x02 (nibble 0x20)
    lcdWriteNibble(0x20, 0);
    delay_us(200);

    // --- Configurações Finais ---
    
    // Function Set: 4-bit mode, 2 lines, 5x8 font 
    lcdWriteByte(CMD_FUNCTION_SET, 0);

    // Display Control: Display ON, Cursor OFF, Blink OFF 
    lcdWriteByte(CMD_DISPLAY_CONTROL, 0);

    // Entry Mode: Incrementa cursor, sem shift (0x06)
    lcdWriteByte(CMD_ENTRY_MODE_SET, 0);

    // Clear Display: Limpa tela e volta cursor ao início 
    lcdWriteByte(CMD_CLEAR_DISPLAY, 0);
    lcdWriteByte(CMD_RETURN_HOME, 0);
    
    // Delay crítico de 1.53ms após limpar o display
    delay_us(2000); 
}

/*
 * ---------------------------------------------------------
 * IMPLEMENTAÇÃO DAS FUNÇÕES DE HARDWARE
 * ---------------------------------------------------------
 */

void initialize_I2C_UCB0_MasterTransmitter()
{
    //Desliga o módulo
    UCB0CTL1 |= UCSWRST;

    //Configura os pinos
    P3SEL |= BIT0;     //Configuro os pinos para "from module"
    P3SEL |= BIT1;
    P3REN |= BIT0;     //Ativa o resistor
    P3REN |= BIT1;
    P3OUT |= BIT0;     //Modo pull-up
    P3OUT |= BIT1;


    UCB0CTL0 = UCMST |          //Master Mode
               UCMODE_3 |       //I2C Mode
               UCSYNC;          //Synchronous Mode

    UCB0CTL1 = UCSSEL__ACLK |   //Clock Source: ACLK
               UCTR |           //Transmitter
               UCSWRST ;        //Mantém o módulo desligado

    //Divisor de clock para o BAUDRate
    UCB0BR0 = 2;
    UCB0BR1 = 0;

    //Liga o módulo.
    UCB0CTL1 &= ~UCSWRST;
}

void i2cSend(uint8_t addr, uint8_t data)
{
    // Desliga interrupções para garantir atomicidade simples
    UCB0IE = 0;

    // Define o endereço do escravo
    UCB0I2CSA = addr;

    // --- CORREÇÃO DE SEGURANÇA 1 ---
    // Espera o barramento estar livre antes de tentar qualquer coisa
    while (UCB0STAT & UCBBUSY);

    // Envia START e coloca em modo transmissor
    UCB0CTL1 |= UCTXSTT | UCTR;

    // Espera o buffer de transmissão estar pronto
    while ((UCB0IFG & UCTXIFG) == 0);

    // Escreve o dado no buffer
    UCB0TXBUF = data;

    // Espera o ACK do escravo (bit UCTXSTT limpa quando o ACK chega)
    while (UCB0CTL1 & UCTXSTT);

    // Verifica se houve NACK (erro)
    if (UCB0IFG & UCNACKIFG)
    {
        UCB0CTL1 |= UCTXSTP; // Envia STOP em caso de erro
        UCB0IFG &= ~UCNACKIFG; // Limpa flag de erro
    }
    else
    {
        UCB0CTL1 |= UCTXSTP; // Envia STOP normal
    }

    // Espera o STOP ser totalmente enviado antes de sair da função.
    while (UCB0CTL1 & UCTXSTP);
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