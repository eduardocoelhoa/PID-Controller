#include "Orchestrator.h"

// =========================================================================
//  Construtor
//  Inicializa o Orchestrator com referências ao Encoder e Motor.
//  Todos os flags de estado começam como "não executando".
// =========================================================================
Orchestrator::Orchestrator(Encoder& encoder, Motor& motor)
    : encoder(encoder), motor(motor),
      running(false), startUs(0), durationUs(0), lastSampleUs(0), currentPwm(0) {}

// =========================================================================
//  beginTest
//  Rotina comum de inicialização para qualquer tipo de teste.
//  Marca o timestamp de início, configura a duração e aplica o PWM inicial.
// =========================================================================
void Orchestrator::beginTest(unsigned long durationMs, int pwm) {
    running = true;
    startUs = micros();
    durationUs = durationMs * 1000UL;
    lastSampleUs = startUs;
    currentPwm = pwm;
    motor.setPwm(pwm);
}

// =========================================================================
//  endTest
//  Finaliza o teste: envia um pacote especial com END_MARKER (0x7FFF) no
//  campo de RPM para sinalizar ao Python que o teste terminou, depois
//  para o motor.
// =========================================================================
void Orchestrator::endTest() {
    if (running) {
        uint32_t timeUs = micros() - startUs;
        int16_t rpmEnd = END_MARKER;  // 0x7FFF = sinal de fim
        int16_t pwm = static_cast<int16_t>(currentPwm);
        Serial.write(reinterpret_cast<const uint8_t*>(&timeUs), 4);
        Serial.write(reinterpret_cast<const uint8_t*>(&rpmEnd), 2);
        Serial.write(reinterpret_cast<const uint8_t*>(&pwm), 2);
    }
    motor.stop();
    running = false;
}

// =========================================================================
//  sendSample
//  Envia uma amostra binária via Serial (8 bytes, little-endian):
//    [0-3] uint32_t tempo_us   — microsegundos desde o início do teste
//    [4-5] int16_t  rpm_div2   — RPM / 2 (resolução 2 RPM, com sinal)
//    [6-7] int16_t  pwm        — duty cycle aplicado
// =========================================================================
void Orchestrator::sendSample(unsigned long nowUs) {
    uint32_t timeUs = nowUs - startUs;
    float rpm = encoder.getRpm();
    int16_t rpmDiv2 = static_cast<int16_t>(rpm / 2.0f + (rpm >= 0.0f ? 0.5f : -0.5f));
    int16_t pwm = static_cast<int16_t>(currentPwm);

    Serial.write(reinterpret_cast<const uint8_t*>(&timeUs), 4);
    Serial.write(reinterpret_cast<const uint8_t*>(&rpmDiv2), 2);
    Serial.write(reinterpret_cast<const uint8_t*>(&pwm), 2);

    lastSampleUs = nowUs;
}

// =========================================================================
//  tick
//  Chamado a cada ciclo do loop durante um teste ativo.
//  Se passou SAMPLE_PERIOD_US desde a última amostra, envia a amostra.
//  Se a duração total do teste foi atingida, finaliza automaticamente.
// =========================================================================
void Orchestrator::tick() {
    unsigned long nowUs = micros();

    if (nowUs - lastSampleUs >= SAMPLE_PERIOD_US) {
        sendSample(nowUs);
    }

    if (nowUs - startUs >= durationUs) {
        endTest();
    }
}

// =========================================================================
//  runStepTest
//  Executa um teste de degrau: aplica um PWM constante ao motor por
//  uma duração determinada. Ideal para analisar a resposta transiente
//  do sistema (tempo de subida, overshoot, regime permanente).
//  Retorna true enquanto o teste estiver em execução.
// =========================================================================
bool Orchestrator::runStepTest(int pwm, unsigned long durationMs) {
    if (!running) {
        beginTest(durationMs, pwm);
    }

    if (running) tick();
    return running;
}

// =========================================================================
//  runFrequencyTest
//  Executa um teste senoidal: o PWM varia como uma senoide em torno de
//  um offset, simulando uma referência periódica. O sinal é:
//    PWM(t) = offset + amplitude × sen(2π × freq × t)
//  Limitado ao intervalo [-PWM_MAX, PWM_MAX]. Sinal negativo inverte a direção
//  via ponte H (ver Motor::setPwm()).
// =========================================================================
bool Orchestrator::runFrequencyTest(int offsetPwm, int ampPwm, float freqHz, unsigned long durationMs) {
    if (!running) {
        beginTest(durationMs, offsetPwm);
    }

    if (running) {
        float tSec = static_cast<float>(micros() - startUs) / 1000000.0f;
        float pwm = offsetPwm + ampPwm * sin(2.0f * PI * freqHz * tSec);
        currentPwm = constrain(static_cast<int>(pwm), -PWM_MAX, PWM_MAX);
        motor.setPwm(currentPwm);
        tick();
    }

    return running;
}

// =========================================================================
//  stop
//  Para qualquer teste em execução (interface pública).
// =========================================================================
void Orchestrator::stop() {
    endTest();
}

// =========================================================================
//  isRunning
//  Retorna true se um teste estiver em andamento.
// =========================================================================
bool Orchestrator::isRunning() const {
    return running;
}
