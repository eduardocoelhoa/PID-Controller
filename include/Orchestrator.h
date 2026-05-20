#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include "Encoder.h"
#include "Motor.h"
#include "Constants.h"
#include <Arduino.h>

// =========================================================================
//  ORCHESTRATOR
//  Máquina de estados que executa rotinas de teste aberto no motor.
//  Coordena o Encoder (leitura) e o Motor (ação) para realizar testes
//  de degrau e senoide.
//
//  Durante o teste, envia dados em formato binário via Serial para
//  captura em tempo real por um script Python no PC.
//  Formato por amostra (8 bytes, little-endian, sem sync marker):
//    [0-3] uint32_t time_us   — tempo desde o início do teste (µs)
//    [4-5] int16_t  rpm_div2  — RPM / 2 (resolução 2 RPM, com sinal)
//    [6-7] int16_t  pwm       — duty cycle aplicado
// =========================================================================

class Orchestrator {
  private:
    Encoder& encoder;     // Referência ao encoder (leitura de RPM)
    Motor& motor;         // Referência ao motor (ação de controle)
    bool running;         // true enquanto um teste estiver em execução
    unsigned long startUs;     // Timestamp de início do teste (µs)
    unsigned long durationUs;  // Duração total do teste (µs)
    unsigned long lastSampleUs; // Timestamp da última amostra enviada (µs)
    int currentPwm;       // Duty cycle atual aplicado ao motor

    // Inicializa um teste: zera contadores, aplica o PWM inicial
    void beginTest(unsigned long durationMs, int pwm);

    // Finaliza um teste: envia marcador de fim, para o motor
    void endTest();

    // Envia uma amostra binária via Serial (sync + time + rpm + pwm)
    void sendSample(unsigned long nowUs);

    // Chamado a cada ciclo do loop: envia amostra se passou SAMPLE_PERIOD_US,
    // e finaliza o teste se a duração foi atingida
    void tick();

  public:
    Orchestrator(Encoder& encoder, Motor& motor);

    // Executa um teste de degrau (PWM constante). Retorna true enquanto durar.
    bool runStepTest(int pwm, unsigned long durationMs);

    // Executa um teste senoidal. Retorna true enquanto durar.
    bool runFrequencyTest(int offsetPwm, int ampPwm, float freqHz, unsigned long durationMs);

    // Para qualquer teste em execução
    void stop();

    // Retorna true se um teste estiver em andamento
    bool isRunning() const;
};

#endif // ORCHESTRATOR_H
