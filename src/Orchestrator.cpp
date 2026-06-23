#include "Orchestrator.h"

// =========================================================================
//  Construtor
// =========================================================================
Orchestrator::Orchestrator(Encoder& encoder, Motor& motor, CommandParser& parser)
    : encoder(encoder), motor(motor), parser(parser),
      activeMethod(METHOD_NONE), running(false),
      startUs(0), durationUs(0), lastSampleUs(0), currentPwm(0) {}

// =========================================================================
//  beginTest
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
// =========================================================================
void Orchestrator::endTest() {
    if (running) {
        uint32_t timeUs = micros() - startUs;
        int16_t rpmEnd = END_MARKER;
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
//  testMA
//  Teste de malha aberta — degrau (s/S) e senoide (f/F).
//  Chamada por update() tanto para iniciar quanto para continuar.
// =========================================================================
void Orchestrator::testMA() {
    if (!running) {
        // Início do teste — parser já tem os parâmetros
        if (parser.activeTest() == TEST_STEP) {
            beginTest(parser.stepDurationMs(), parser.stepPwm());
        } else if (parser.activeTest() == TEST_FREQ) {
            beginTest(parser.freqDurationMs(), parser.freqOffsetPwm());
        }
        if (running) activeMethod = METHOD_MA;
    }

    if (!running) return;

    // Execução contínua
    if (parser.activeTest() == TEST_FREQ) {
        float tSec = static_cast<float>(micros() - startUs) / 1000000.0f;
        float pwm = parser.freqOffsetPwm() + parser.freqAmplitudePwm() * sin(2.0f * PI * parser.freqHz() * tSec);
        currentPwm = constrain(static_cast<int>(pwm), -PWM_MAX, PWM_MAX);
        motor.setPwm(currentPwm);
    }

    tick();

    if (!running) {
        parser.setActiveTest(TEST_NONE);
        activeMethod = METHOD_NONE;
    }
}

// =========================================================================
//  update
//  Ponto de entrada: lê serial, despacha comandos.
// =========================================================================
void Orchestrator::update() {
    // Teste em execução — verifica parada, reconfiguração ou continua
    if (running) {
        if (Serial.available() > 0) {
            char cmd = Serial.peek();
            if (cmd == 'x' || cmd == 'X') {
                Serial.read();
                stop();
                return;
            }
            // Permite W:<n> durante teste em execução
            if (cmd == 'W') {
                String line = Serial.readStringUntil('\n');
                line.trim();
                if (line.length() > 2 && line.charAt(1) == ':') {
                    int window = line.substring(2).toInt();
                    encoder.setAvgWindow(static_cast<size_t>(window));
                }
                return;
            }
        }
        if (activeMethod == METHOD_MA) testMA();
        // else if (activeMethod == METHOD_MF) testMF();
        return;
    }

    // Aguarda comando
    if (Serial.available() <= 0) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    char cmd = line.charAt(0);

    // Configuração de janela de média móvel: W:<n>
    if (cmd == 'W' && line.length() > 2 && line.charAt(1) == ':') {
        int window = line.substring(2).toInt();
        encoder.setAvgWindow(static_cast<size_t>(window));
        return;
    }

    // Malha aberta: s/S → degrau, f/F → senoide
    if (cmd == 'f') {
        parser.setActiveTest(TEST_FREQ);
        testMA();
    } else if (cmd == 'F' && line.length() > 2 && line.charAt(1) == ':') {
        if (parser.parseFreqParams(line.substring(2))) {
            parser.setActiveTest(TEST_FREQ);
            testMA();
        }
    } else if (cmd == 's') {
        parser.setActiveTest(TEST_STEP);
        testMA();
    } else if (cmd == 'S' && line.length() > 2 && line.charAt(1) == ':') {
        if (parser.parseStepParams(line.substring(2))) {
            parser.setActiveTest(TEST_STEP);
            testMA();
        }
    }
    // Malha fechada (futuro): adicionar comandos aqui
    // else if (cmd == 'm') { ... testMF(); }
}

// =========================================================================
//  stop
// =========================================================================
void Orchestrator::stop() {
    endTest();
    parser.setActiveTest(TEST_NONE);
    activeMethod = METHOD_NONE;
}

// =========================================================================
//  isRunning
// =========================================================================
bool Orchestrator::isRunning() const {
    return running;
}
