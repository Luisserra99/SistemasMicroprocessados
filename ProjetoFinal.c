#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>
#include "driverlib.h"

/* * DEFINIÇÕES DE HARDWARE 
 */
// Relé / Bomba (P2.0)
#define PUMP_PORT   GPIO_PORT_P2
#define PUMP_PIN    GPIO_PIN0

// Sensor de Umidade (Leitura em P6.0)
#define SENSOR_PORT GPIO_PORT_P6
#define SENSOR_PIN  GPIO_PIN0

// Alimentação do Sensor (P6.1 - VCC Controlado)
#define SENSOR_PWR_PORT GPIO_PORT_P6
#define SENSOR_PWR_PIN  GPIO_PIN1

// Definições do LCD I2C
#define LCD_ADDR 0x27
#define RS_BIT   BIT0
#define RW_BIT   BIT1
#define EN_BIT   BIT2
#define BL_BIT   BIT3

// Comandos LCD
#define CMD_CLEAR_DISPLAY     0x01
#define CMD_RETURN_HOME       0x02
#define CMD_ENTRY_MODE_SET    0x06
#define CMD_DISPLAY_CONTROL   0x0F
#define CMD_FUNCTION_SET      0x28
#define CMD_SECOND_LINE       0xC0

/* * VARIÁVEIS GLOBAIS 
 */
volatile unsigned int dry_cycles = 0; // Contador de ciclos de seca

/* * PROTÓTIPOS 
 */
void Init_Peripherals(void);
void LCD_Init_I2C_RegisterLevel(void);
void LCD_Init(void);
void LCD_Write_Nibble(uint8_t nibble, uint8_t isChar);
void LCD_Write_Byte(uint8_t byte, uint8_t isChar);
void LCD_Update(char *str);
void I2C_Send(uint8_t addr, uint8_t data);
void Delay_us_Custom(unsigned int time_us);
void Enter_Assistive_Wait(uint16_t seconds);

/*
 * FUNÇÃO MAIN
 */
void main(void)
{
    // Para o Watchdog Timer
    WDT_A_hold(WDT_A_BASE);

    // 1. Inicializa Periféricos
    Init_Peripherals();

    // Habilita interrupções globais (Necessário para o Timer acordar a CPU do LPM3)
    __enable_interrupt();
    
    // 2. Inicializa LCD e exibe mensagem inicial
    LCD_Init();
    LCD_Update("Iniciando...");
    
    // Espera 2s para estabilização inicial do sistema
    Enter_Assistive_Wait(2); 
    LCD_Write_Byte(CMD_CLEAR_DISPLAY, 0);

    while(1)
    {
        // --- ETAPA 1: LEITURA DO SENSOR (ADC) COM CONTROLE DE ENERGIA ---
        
        // A. Liga a alimentação do sensor
        GPIO_setOutputHighOnPin(SENSOR_PWR_PORT, SENSOR_PWR_PIN);

        // B. Aguarda estabilização da tensão (4000 ciclos = ~4ms a 1MHz)
        __delay_cycles(4000);

        // C. Inicia conversão (Trigger por software)
        ADC12_A_startConversion(ADC12_A_BASE, ADC12_A_MEMORY_0, ADC12_A_SINGLECHANNEL);

        // D. Aguarda conversão
        // ADC Clock = 1MHz. 13 ciclos conversão + 4 hold = ~13us. 
        __delay_cycles(13); 

        // E. Ler valor do ADC (0 a 255)
        uint16_t adc_result = ADC12_A_getResults(ADC12_A_BASE, ADC12_A_MEMORY_0);

        // F. Desliga a alimentação do sensor IMEDIATAMENTE para economizar
        GPIO_setOutputLowOnPin(SENSOR_PWR_PORT, SENSOR_PWR_PIN);

        // --- CÁLCULO DA PORCENTAGEM ---
        // Calcula porcentagem real (0-100)
        int moisture_pct = (adc_result * 100) / 255;

        // --- ETAPA 2: LÓGICA DE DECISÃO E CONTROLE ---
        
        if(moisture_pct < 77) 
        {
            // SOLO SECO
            if (dry_cycles < 4) 
            {
                // Modo Paciência: Espera até 4 ciclos
                dry_cycles++;
                LCD_Update("      Solo\n      Seco"); 
            }
            else 
            {
                // Ação: Irrigar
                LCD_Update("   Irrigando..."); 
                
                // 1. Liga a bomba
                GPIO_setOutputHighOnPin(PUMP_PORT, PUMP_PIN);
                
                // 2. Mantém ligada por 5 segundos
                Enter_Assistive_Wait(5);
                
                // 3. Desliga a bomba
                GPIO_setOutputLowOnPin(PUMP_PORT, PUMP_PIN);
                
                // Feedback
                LCD_Update("      Solo\n      Umido"); 
                
                // Reinicia ciclo de seca
                dry_cycles = 0;
            }
        }
        else 
        {
            // SOLO ÚMIDO - Reinicia ciclos
            dry_cycles = 0;
            LCD_Update("      Solo\n      Umido");
        }

        // --- ETAPA 3: HIBERNAÇÃO ---
        // Espera 1 hora (3600 segundos)
        // O processador desliga completamente por 1 hora.
        Enter_Assistive_Wait(3600); 
    }
}

/*
 * FUNÇÕES AUXILIARES
 */

void Init_Peripherals(void)
{
    // --- Configuração da Alimentação do Sensor (P6.1) ---
    // Define como saída e inicia em BAIXO (Desligado)
    GPIO_setAsOutputPin(SENSOR_PWR_PORT, SENSOR_PWR_PIN);
    GPIO_setOutputLowOnPin(SENSOR_PWR_PORT, SENSOR_PWR_PIN);

    // --- Configuração da Bomba (GPIO) ---
    GPIO_setAsOutputPin(PUMP_PORT, PUMP_PIN);
    GPIO_setOutputLowOnPin(PUMP_PORT, PUMP_PIN);

    // --- Configuração do Sensor (ADC) ---
    GPIO_setAsPeripheralModuleFunctionInputPin(SENSOR_PORT, SENSOR_PIN);

    // Inicializa ADC com SMCLK (1MHz)
    ADC12_A_init(ADC12_A_BASE, 
                 ADC12_A_SAMPLEHOLDSOURCE_SC, 
                 ADC12_A_CLOCKSOURCE_SMCLK, 
                 ADC12_A_CLOCKDIVIDER_1);

    ADC12_A_enable(ADC12_A_BASE);

    // Configura Amostragem: 8-bit, 4 ciclos hold
    ADC12_A_setupSamplingTimer(ADC12_A_BASE, 
                               ADC12_A_CYCLEHOLD_4_CYCLES, 
                               ADC12_A_CYCLEHOLD_4_CYCLES, 
                               ADC12_A_MULTIPLESAMPLESENABLE);

    // Configura Memória
    ADC12_A_configureMemoryParam param = {0};
    param.memoryBufferControlIndex = ADC12_A_MEMORY_0;
    param.inputSourceSelect = ADC12_A_INPUT_A0;
    param.positiveRefVoltageSourceSelect = ADC12_A_VREFPOS_AVCC;
    param.negativeRefVoltageSourceSelect = ADC12_A_VREFNEG_AVSS;
    param.endOfSequence = ADC12_A_ENDOFSEQUENCE;
    
    ADC12_A_configureMemory(ADC12_A_BASE, &param);
    
    // Garante 8 bits via registrador
    ADC12CTL2 &= ~ADC12RES_3; 
    ADC12CTL2 |= ADC12RES_0;  

    // --- Configuração do I2C ---
    LCD_Init_I2C_RegisterLevel();
}

/*
 * FUNÇÃO DE ECONOMIA DE ENERGIA (LPM3)
 */
void Enter_Assistive_Wait(uint16_t seconds)
{
    // Configura Timer0_A para contar usando ACLK (32768Hz)
    // Queremos interrupção a cada 1 segundo.
    // CCR0 = 32768 - 1
    TA0CCR0 = 32767;
    
    // TASSEL_1 = ACLK
    // MC_1 = Up Mode (conta até CCR0 e reseta)
    // TACLR = Limpa o timer
    TA0CTL = TASSEL_1 | MC_1 | TACLR;
    
    // Habilita interrupção do CCR0
    TA0CCTL0 = CCIE;

    while (seconds > 0)
    {
        // Entra em Low Power Mode 3 + Global Interrupt Enable
        // CPU OFF, SMCLK OFF, ACLK ON.
        // O programa para aqui até a interrupção do timer acontecer.
        __bis_SR_register(LPM3_bits + GIE);
        
        // Ao acordar (após 1 segundo), decrementa e repete se necessário
        seconds--;
    }

    // Para o timer para economizar energia e limpar configurações
    TA0CTL = MC_0;
    TA0CCTL0 = 0;
}

// --- INTERRUPÇÃO DO TIMER ---
// Esta função roda a cada 1 segundo quando o timer está ativo
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
    // Acorda o processador do modo LPM3
    // Limpa os bits LPM3 do registrador de status salvo na pilha
    __bic_SR_register_on_exit(LPM3_bits);
}

/*
 * DRIVER LCD I2C
 */

void LCD_Init_I2C_RegisterLevel(void)
{
    UCB0CTL1 |= UCSWRST;
    P3SEL |= BIT0 | BIT1; 
    P3REN |= BIT0 | BIT1;
    P3OUT |= BIT0 | BIT1;

    UCB0CTL0 = UCMST | UCMODE_3 | UCSYNC;
    UCB0CTL1 = UCSSEL__SMCLK | UCTR | UCSWRST; // SMCLK 1MHz
    UCB0BR0 = 10; 
    UCB0BR1 = 0;
    UCB0CTL1 &= ~UCSWRST;
}

void I2C_Send(uint8_t addr, uint8_t data)
{
    UCB0I2CSA = addr;
    while (UCB0STAT & UCBBUSY);
    UCB0CTL1 |= UCTXSTT | UCTR;
    while ((UCB0IFG & UCTXIFG) == 0);
    UCB0TXBUF = data;
    while (UCB0CTL1 & UCTXSTT);
    if (UCB0IFG & UCNACKIFG) {
        UCB0CTL1 |= UCTXSTP;
        UCB0IFG &= ~UCNACKIFG;
    } else {
        UCB0CTL1 |= UCTXSTP;
    }
    while (UCB0CTL1 & UCTXSTP);
}

void Delay_us_Custom(unsigned int time_us)
{
    //Configure timer A0 and starts it.
    TA0CCR0 = time_us;
    TA0CTL = TASSEL__SMCLK | ID__1 | MC_1 | TACLR;

    //Locks, waiting for the timer.
    while((TA0CTL & TAIFG) == 0);

    //Stops the timer
    TA0CTL = MC_0 | TACLR;
}

void LCD_Write_Nibble(uint8_t nibble, uint8_t isChar)
{
    uint8_t i2cValue = (nibble & 0xF0) | BL_BIT;
    if (isChar) i2cValue |= RS_BIT;
    else i2cValue &= ~RS_BIT;

    i2cValue &= ~(RW_BIT | EN_BIT);
    I2C_Send(LCD_ADDR, i2cValue);
    I2C_Send(LCD_ADDR, i2cValue | EN_BIT);
    Delay_us_Custom(10);
    I2C_Send(LCD_ADDR, i2cValue);
    Delay_us_Custom(50);
}

void LCD_Write_Byte(uint8_t byte, uint8_t isChar)
{
    LCD_Write_Nibble(byte, isChar);
    LCD_Write_Nibble(byte << 4, isChar);
}

void LCD_Init(void)
{
    Delay_us_Custom(20000);
    LCD_Write_Nibble(0x30, 0); Delay_us_Custom(5000);
    LCD_Write_Nibble(0x30, 0); Delay_us_Custom(100);
    LCD_Write_Nibble(0x30, 0); Delay_us_Custom(100);
    LCD_Write_Nibble(0x20, 0); Delay_us_Custom(100);

    LCD_Write_Byte(CMD_FUNCTION_SET, 0);
    LCD_Write_Byte(CMD_DISPLAY_CONTROL, 0);
    LCD_Write_Byte(CMD_ENTRY_MODE_SET, 0);
    LCD_Write_Byte(CMD_CLEAR_DISPLAY, 0);
    Delay_us_Custom(2000);
    LCD_Write_Byte(CMD_RETURN_HOME, 0);
}

void LCD_Update(char *str)
{
    LCD_Write_Byte(CMD_CLEAR_DISPLAY, 0);
    Delay_us_Custom(2000);
    while (*str)
    {
        if (*str == '\n') LCD_Write_Byte(CMD_SECOND_LINE, 0);
        else LCD_Write_Byte(*str, 1);
        str++;
    }
}