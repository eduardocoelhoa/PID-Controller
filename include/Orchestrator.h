#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include "Encoder.h"
#include "Motor.h"
#include "CommandParser.h"
#include "Constants.h"
#include <Arduino.h>

// Método de teste ativo — permite que update() despache para o correto
enum TestMethod { METHOD_NONE, METHOD_MA };

// =========================================================================
//  ORCHESTRATOR
//  Coordena Encoder (leitura) e Motor (ação) para executar testes.
//  Envia dados em formato binário via Serial para captura por Python.
//  Formato por amostra (8 bytes, little-endian):
//    [0-3] uint32_t time_us   — tempo desde o início do teste (µs)
//    [4-5] int16_t  rpm_div2  — RPM / 2 (resolução 2 RPM, com sinal)
//    [6-7] int16_t  pwm       — duty cycle aplicado
// =========================================================================

class Orchestrator {
  private:
    Encoder& encoder;
    Motor& motor;
    CommandParser& parser;

    // Estado do teste
    TestMethod activeMethod;
    bool running;
    unsigned long startUs;
    unsigned long durationUs;
    unsigned long lastSampleUs;
    int currentPwm;

    // Primitivas internas
    void beginTest(unsigned long durationMs, int pwm);
    void endTest();
    void sendSample(unsigned long nowUs);
    void tick();

  public:
    Orchestrator(Encoder& encoder, Motor& motor, CommandParser& parser);

    // Ponto de entrada: lê serial e despacha para o teste ativo
    void update();

    // Teste de malha aberta (degrau e senoide)
    // Comandos: s/S → degrau, f/F → senoide
    void testMA();

    // Para qualquer teste em execução
    void stop();

    // Retorna true se um teste estiver em andamento
    bool isRunning() const;
};

#endif // ORCHESTRATOR_H
