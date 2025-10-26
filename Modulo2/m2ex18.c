/**
 * Implementação do Exercício 18: Controle IR (4 botões)
 * Sensor: VS1838B
 *
 * Funcionalidades:
 * 1. Receptor IR (NEC) em P2.6
    CH- = FFA25D
    CH++ = FFE21D
    V- = FFE01F
    V++ = FFA857
    FFFFFFFF - sergurar o botão
 * 2. led01 (P1.0) liga/desliga com V++/V-
 * 3. led02 (P4.7) com PWM por software
 * 4. CH++/CH- ajusta duty cycle em passos de 10%
 * 5. Auto-repeat a cada 100 ms para CH++/CH-
 */

#include <msp430.h>
#include <stdint.h>
#include <stdbool.h>

// --- Definições de Pinos ---
#define LED1        BIT0    // P1.0
#define LED2        BIT7    // P4.7
#define IR_PIN      BIT6    // P2.6

// --- Macros Auxiliares ---
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

// --- Códigos dos Botões (NEC) ---
#define CMD_V_PLUS      0xFFA857
#define CMD_V_MINUS     0xFFE01F
#define CMD_CH_PLUS     0xFFE21D
#define CMD_CH_MINUS    0xFFA25D

// --- Constantes do Protocolo NEC (em contagens de 1µs) ---
// Tolerâncias aproximadas
#define NEC_START_PULSE_MIN 8000  // 9ms
#define NEC_START_PULSE_MAX 10000
#define NEC_START_SPACE_MIN 4000  // 4.5ms (Novo Comando)
#define NEC_START_SPACE_MAX 5000
#define NEC_REPEAT_SPACE_MIN 2000 // 2.25ms (Repetição)
#define NEC_REPEAT_SPACE_MAX 2500
#define NEC_PULSE_MIN       460   // 560µs
#define NEC_PULSE_MAX       660
#define NEC_BIT0_SPACE_MIN  460   // 560µs
#define NEC_BIT0_SPACE_MAX  660
#define NEC_BIT1_SPACE_MIN  1580  // 1.68ms
#define NEC_BIT1_SPACE_MAX  1780

// --- Constantes do PWM por Software ---
#define PERIOD_TICKS    1000    // Para ~1kHz com SMCLK=1MHz (1000 * 1µs = 1ms)

// --- Estados da Máquina de Decodificação IR ---
typedef enum {
    IR_IDLE,
    IR_START_PULSE,
    IR_START_SPACE,
    IR_DATA_PULSE,
    IR_DATA_SPACE
} IR_State;

// --- Variáveis Globais ---

// PWM
volatile uint8_t duty_pct = 0;      // Duty cycle 0-100
volatile uint16_t on_ticks = 0;     // Duração do pulso ON em contagens
volatile uint8_t fase_pwm = 0;      // 0 = fase OFF, 1 = fase ON

// Auto-Repeat
volatile bool hold_active = false;
volatile uint8_t hold_cmd = 0;      // 1=CH++, 2=CH- (usado para ISR)
volatile uint16_t last_ir_event_ms = 0; // Contador para timeout de 200ms

// Decodificação IR
volatile IR_State ir_state = IR_IDLE;
volatile uint16_t ir_last_time = 0;
volatile uint32_t ir_raw_data = 0;
volatile uint8_t ir_bit_count = 0;
volatile bool ir_command_ready = false;
volatile bool ir_repeat_detected = false;
volatile uint32_t ir_command = 0;

// --- Protótipos de Funções ---
void init_clocks(void);
void init_gpio(void);
void init_timer_ir_capture(void); // TA1 para medição de tempo
void init_timer_pwm(void);        // TA0 para PWM por software
void init_timer_repeat(void);     // TA2 para tick de 100ms
void update_on_ticks(void);

/**
 * Função Principal
 */
void main(void) {
    // 1. Inicialização
    WDTCTL = WDTPW | WDTHOLD;   // Desabilitar o watchdog

    init_clocks();              // Configurar clock (SMCLK ≈ 1 MHz)
    init_gpio();
    init_timer_ir_capture();
    init_timer_pwm();
    init_timer_repeat();

    update_on_ticks();          // Calcular on_ticks inicial

    __enable_interrupt();     // Habilitar interrupções globais

    // Loop principal
    while(1) {
        
        // 6. Mapeamento dos 4 comandos
        if (ir_command_ready) {
            ir_command_ready = false;
            
            // NOTA: Se os 32 bits completos forem diferentes (incluindo endereço),
            // podemos precisar mascarar o comando:
            // uint32_t cmd_masked = ir_command & 0xFFFFFF; // Ou 0xFFFF se for só comando
            
            // Valor total de 32 bits
            // (que pode ser apenas 0x00FFA25D, por exemplo)
            
            switch(ir_command) {
                case CMD_V_PLUS:
                    P1OUT |= LED1; // Liga led01
                    hold_active = false;
                    break;
                case CMD_V_MINUS:
                    P1OUT &= ~LED1; // Desliga led01
                    hold_active = false;
                    break;
                case CMD_CH_PLUS:
                    // Aplica o primeiro passo de 10%
                    duty_pct = min(duty_pct + 10, 100);
                    update_on_ticks();
                    // Ativa o auto-repeat
                    hold_cmd = 1; // 1 para CH++
                    hold_active = true;
                    last_ir_event_ms = 0;
                    break;
                case CMD_CH_MINUS:
                    // Aplica o primeiro passo de 10%
                    duty_pct = max(duty_pct - 10, 0);
                    update_on_ticks();
                    // Ativa o auto-repeat
                    hold_cmd = 2; // 2 para CH--
                    hold_active = true;
                    last_ir_event_ms = 0;
                    break;
                default:
                    // Outro botão pressionado, cancela auto-repeat
                    hold_active = false;
                    break;
            }
        }

        if (ir_repeat_detected) {
            ir_repeat_detected = false;
            if (hold_active) {
                // "Alimenta" o timeout do auto-repeat
                last_ir_event_ms = 0;
            }
        }
    }
}

/**
 * Atualiza a contagem de 'on_ticks' baseada no 'duty_pct'
 */
void update_on_ticks(void) {
    // Garante saturação
    if (duty_pct > 100) duty_pct = 100;

    // Cálculo com aritmética inteira
    // (uint32_t) previne overflow no cálculo intermediário
    on_ticks = (uint16_t)((PERIOD_TICKS * (uint32_t)duty_pct) / 100);
}

/**
 * Configura Clocks (Passo 1)
 */
void init_clocks(void) {
    // SMCLK ≈ 1MHz (DCO padrão)
}

/**
 * Configura GPIO (Passo 1)
 */
void init_gpio(void) {
    // led01: P1.0 como saída, desligado
    P1DIR |= LED1; // saída
    P1OUT &= ~LED1; // desligado

    // led02: P4.7 como saída, desligado
    P4DIR |= LED2; // saída
    P4OUT &= ~LED2; // desligado
    P4SEL &= ~LED2; // Garantir que é GPIO(opcional)

    // Receptor IR: P2.6 como entrada
    P2DIR &= ~IR_PIN; // entrada
    P2SEL &= ~IR_PIN; // opcional
    P2REN |= IR_PIN;  // Habilita resistor (pull-up)
    P2OUT |= IR_PIN;  // Configura como pull-up (VS1838B fica HIGH)
    P2IES |= IR_PIN;  // Interrupção na borda de SUBIDA para DESCIDA (Início do Start)
    P2IFG &= ~IR_PIN; // Limpar flag
    P2IE |= IR_PIN;   // Habilitar interrupção de P2.6
}

/**
 * Configura Timer TA1 para Medição de Tempo IR (Passo 2)
 */
void init_timer_ir_capture(void) {
    // TA1: SMCLK, Modo Contínuo, Divisor /1 (1MHz -> 1µs/contagem)
    TA1CTL = TASSEL__SMCLK | MC__CONTINUOUS | TACLR ;
}

/**
 * Configura Timer TA0 para PWM por Software (Passo 4)
 */
void init_timer_pwm(void) {
    // TA0: SMCLK, Modo Contínuo, Divisor /1 (1MHz)
    TA0CTL = TASSEL__SMCLK | MC__CONTINUOUS | TACLR;
    TA0CCTL0 = CCIE;            // Habilitar interrupção para CCR0
    TA0CCR0 = 1000;             // Primeira interrupção
}

/**
 * Configura Timer TA2 para Auto-Repeat (Passo 5)
 */
void init_timer_repeat(void) {
    // TA2: SMCLK, Modo UP, Divisor /8 (SMCLK/8 = 125 kHz)
    TA2CTL = TASSEL__SMCLK | MC__UP | TACLR | ID_3;
    // CCR0 = 12500 -> Interrupção a cada 100 ms (12500 / 125kHz = 0.1s)
    TA2CCR0 = 12500 - 1;
    TA2CCTL0 = CCIE; // Habilitar interrupção CCR0
}


// --- Rotinas de Interrupção (ISRs) ---

/**
 * ISR da Porta 2: Decodificação IR (Passo 3)
 */
#pragma vector=PORT2_VECTOR
__interrupt void Port_2_ISR(void)
{
    if (P2IFG & IR_PIN) {
        uint16_t current_time = TA1R;
        // Calcula delta-t lidando com overflow (aritmética de 16 bits)
        uint16_t delta_t = current_time - ir_last_time;
        ir_last_time = current_time;

        // Inverte a borda de detecção
        P2IES ^= IR_PIN;
        P2IFG &= ~IR_PIN;

        // Máquina de Estados NEC
        switch(ir_state) {
            case IR_IDLE:
                // Esperando a primeira borda (LOW)
                if (!(P2IES & IR_PIN)) { // Deve ser uma borda H->L
                    ir_state = IR_START_PULSE;
                }
                break;

            case IR_START_PULSE:
                // Mediu o pulso LOW de ~9ms
                if ((delta_t >= NEC_START_PULSE_MIN) && (delta_t <= NEC_START_PULSE_MAX)) {
                    ir_state = IR_START_SPACE;
                } else {
                    ir_state = IR_IDLE; // Erro
                }
                break;

            case IR_START_SPACE:
                // Mediu o espaço HIGH
                if ((delta_t >= NEC_START_SPACE_MIN) && (delta_t <= NEC_START_SPACE_MAX)) {
                    // Novo Comando (4.5ms)
                    ir_state = IR_DATA_PULSE;
                    ir_raw_data = 0; // Reseta os dados
                    ir_bit_count = 0; // Reseta a contagem
                } else if ((delta_t >= NEC_REPEAT_SPACE_MIN) && (delta_t <= NEC_REPEAT_SPACE_MAX)) {
                    // Comando de Repetição (2.25ms)
                    ir_repeat_detected = true;
                    ir_state = IR_IDLE;
                } else {
                    ir_state = IR_IDLE; // Erro
                }
                break;

            case IR_DATA_PULSE:
                // Mediu o pulso LOW de ~560µs
                if ((delta_t >= NEC_PULSE_MIN) && (delta_t <= NEC_PULSE_MAX)) {
                    ir_state = IR_DATA_SPACE;
                } else {
                    ir_state = IR_IDLE; // Erro
                }
                break;

            case IR_DATA_SPACE:
                // Mediu o espaço HIGH (Bit 0 ou Bit 1)
                // O bit é LSB first
                if ((delta_t >= NEC_BIT0_SPACE_MIN) && (delta_t <= NEC_BIT0_SPACE_MAX)) {
                    // É um Bit 0 (~560µs)
                    // Não faz nada, o bit '0' já está em ir_raw_data
                } else if ((delta_t >= NEC_BIT1_SPACE_MIN) && (delta_t <= NEC_BIT1_SPACE_MAX)) {
                    // É um Bit 1 (~1680µs)
                    // Insere '1' na posição LSB-first
                    ir_raw_data |= (1UL << ir_bit_count); // 1UL = Unsigned Long (32 bits)
                } else {
                    ir_state = IR_IDLE; // Erro
                    break;
                }

                // Incrementa o contador de bits (seja 0 ou 1)
                ir_bit_count++;

                // Verifica se completou 32 bits
                if (ir_bit_count == 32) {
                    ir_command = ir_raw_data;
                    ir_command_ready = true;
                    ir_state = IR_IDLE;
                } else {
                    ir_state = IR_DATA_PULSE; // Próximo pulso de dados
                }
                break;
        }
    }
}

/**
 * ISR do Timer TA0 (CCR0): PWM por Software (Passo 4)
 */
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
{
    if (duty_pct == 0) {
        P4OUT &= ~LED2; // Mantém desligado
        TA0CCR0 += PERIOD_TICKS; // Próxima interrupção em 1 período
    }
    else if (duty_pct == 100) {
        P4OUT |= LED2; // Mantém ligado
        TA0CCR0 += PERIOD_TICKS; // Próxima interrupção em 1 período
    }
    else {
        if (fase_pwm == 0) { // Estava na fase OFF, inicia fase ON
            P4OUT |= LED2;
            fase_pwm = 1;
            TA0CCR0 += on_ticks; // Agenda interrupção para o fim do período ON
        } else { // Estava na fase ON, inicia fase OFF
            P4OUT &= ~LED2;
            fase_pwm = 0;
            TA0CCR0 += (PERIOD_TICKS - on_ticks); // Agenda interrupção para o fim do período OFF
        }
    }
}


/**
 * ISR do Timer TA2 (CCR0): Auto-Repeat Tick (Passo 5)
 * Esta ISR executa a cada 100 ms
 */
#pragma vector=TIMER2_A0_VECTOR
__interrupt void TIMER2_A0_ISR(void)
{
    if (hold_active) {
        // Incrementa o tempo desde o último evento IR (novo ou repeat)
        last_ir_event_ms += 100;

        // Verifica timeout de 200 ms (sugerido no doc)
        if (last_ir_event_ms >= 200) {
            hold_active = false; // Cancela auto-repeat
        }
        else {
            // Se não deu timeout, aplica o passo de auto-repeat
            if (hold_cmd == 1) { // CH++
                duty_pct = min(duty_pct + 10, 100);
            } else if (hold_cmd == 2) { // CH--
                duty_pct = max(duty_pct - 10, 0);
            }
            
            update_on_ticks();
        }
    }
}