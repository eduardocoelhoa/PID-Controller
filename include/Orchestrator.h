#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include "Encoder.h"
#include "Motor.h"
#include "CommandParser.h"
#include "Constants.h"
#include "Control.h"
#include <Arduino.h>

// Método de teste ativo — permite que update() despache para o correto
enum TestMethod { METHOD_NONE, METHOD_MA, METHOD_MF };

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
    Control control;


    // Estado do teste
    TestMethod activeMethod;
    bool running;
    unsigned long startUs;
    unsigned long durationUs;
    unsigned long lastSampleUs;
    unsigned long lastControlUs;
    int currentPwm;
    float currentSetpointRpm;

    // Primitivas internas
    void beginTest(unsigned long durationMs, int pwm);
    void beginClosedLoopTest(unsigned long durationMs, float setpointRpm);
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
    void testMF();

    // Para qualquer teste em execução
    void stop();

    // Retorna true se um teste estiver em andamento
    bool isRunning() const;

    // Simula a resposta de um sistema de malha fechada a um degrau
    void simulateClosedLoopStepResponse(float step_target, int num_samples);
};

#endif // ORCHESTRATOR_H
