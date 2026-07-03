#ifndef CONSTANTS_H
#define CONSTANTS_H

// =========================================================================
//  CONSTANTES DO PROJETO — Controle de Motor DC com Encoder
//  Centraliza todas as configurações de hardware e parâmetros de teste.
//  Altere apenas aqui para adaptar a outro hardware ou condição de teste.
// =========================================================================

// --- Pinos do Motor ---
constexpr int PIN_PWM     = 16;   // Pino do sinal PWM (velocidade)
constexpr int PIN_DIR1    = 17;   // Pino de direção 1 (ponte H)
constexpr int PIN_DIR2    = 18;   // Pino de direção 2 (ponte H)
constexpr int PWM_CHANNEL = 0;    // Canal LEDC do ESP32 para gerar o PWM

// --- Pinos do Encoder ---
constexpr int PIN_ENC_A = 32;   // Canal A do encoder (pulso)

// --- Comunicação Serial ---
constexpr unsigned long BAUD_RATE = 115200;

// --- PWM ---
constexpr int PWM_BITS = 12;                   // Resolução do PWM (bits)
constexpr int PWM_MAX  = (1 << PWM_BITS) - 1;  // Valor máximo do PWM

// --- Amostragem ---
constexpr unsigned long SAMPLE_PERIOD_US = 60UL;  // Intervalo entre amostras (µs) → 1/60µ ~ 16.7 kHz

// --- Protocolo Binário ---
// Formato por amostra (8 bytes, little-endian, sem sync marker):
//   [0-3] uint32_t tempo_us   — microsegundos desde o início do teste
//   [4-5] int16_t  rpm_div2   — RPM / 2 (resolução de 2 RPM, com sinal)
//   [6-7] int16_t  pwm        — duty cycle aplicado (-PWM_MAX a +PWM_MAX)
// 8 bytes × ~16.7 kHz ≈ 133,000 bytes/s < 150,000 bytes/s (1.5 Mbaud 8N1)

constexpr int16_t END_MARKER = 0x7FFF;  // Sinal de fim de teste (rpm_div2 = 0x7FFF)
constexpr uint16_t PACKET_SIZE = 8;    // Tamanho fixo de cada pacote em bytes

// --- Teste de Malha Fechada ---
constexpr float DEFAULT_TARGET_RPM = 100.0f;    // RPM alvo padrão para o teste
constexpr unsigned long DEFAULT_DURATION_MS = 10000UL; // Duração padrão do teste (10s)

constexpr unsigned long CONTROL_PERIOD_US = 20000UL;  // Periodo de controle da malha fechada (20 ms)

// Ganhos do controlador PID.
// ATENÇÃO: Estes valores podem precisar de ajuste para o novo controlador simplificado.
constexpr float Kp  = 8.5180f;  
constexpr float Ki  = 616.3596f; 
constexpr float Kd  = 0.02943f; 

#endif // CONSTANTS_H
