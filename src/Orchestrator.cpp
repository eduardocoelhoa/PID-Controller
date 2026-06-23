#include "Orchestrator.h"

// =========================================================================
//  Construtor
// =========================================================================
Orchestrator::Orchestrator(Encoder& encoder, Motor& motor, CommandParser& parser)
    : encoder(encoder), motor(motor), parser(parser),
      control(Kp, Ki, Kd),
      activeMethod(METHOD_NONE), running(false),
      startUs(0), durationUs(0), lastSampleUs(0), lastControlUs(0),
      currentPwm(0), currentSetpointRpm(0.0f) {}

// =========================================================================
//  beginTest
// =========================================================================
void Orchestrator::beginTest(unsigned long durationMs, int pwm) {
    running = true;
    startUs = micros();
    durationUs = durationMs * 1000UL;
    lastSampleUs = startUs;
    lastControlUs = startUs;
    currentPwm = pwm;
    currentSetpointRpm = 0.0f;
    motor.setPwm(pwm);
}

// =========================================================================
//  beginClosedLoopTest
// =========================================================================
void Orchestrator::beginClosedLoopTest(unsigned long durationMs, float setpointRpm) {
    running = true;
    startUs = micros();
    durationUs = durationMs * 1000UL;
    lastSampleUs = startUs;
    lastControlUs = startUs;
    currentPwm = 0;
    currentSetpointRpm = setpointRpm;

    control.reset();
    control.setSetpoint(setpointRpm);
    motor.stop();
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
//  testMF
//  Teste de malha fechada com degrau de referencia em RPM.
//  Chamada por update() tanto para iniciar quanto para continuar.
// =========================================================================
void Orchestrator::testMF() {
    if (!running) {
        beginClosedLoopTest(parser.closedLoopDurationMs(), parser.closedLoopSetpointRpm());
        if (running) activeMethod = METHOD_MF;
    }

    if (!running) return;

    unsigned long nowUs = micros();
    if (nowUs - lastControlUs >= CONTROL_PERIOD_US) {
        float rpm = encoder.getRpm();
        float effort = control.compute(rpm);
        currentPwm = constrain(static_cast<int>(effort + 0.5f), 0, PWM_MAX);
        motor.setPwm(currentPwm);
        lastControlUs = nowUs;
    }

    tick();

    if (!running) {
        control.reset();
        currentSetpointRpm = 0.0f;
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
        else if (activeMethod == METHOD_MF) testMF();
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
    } else if (cmd == 'm') {
        parser.setActiveTest(TEST_CLOSED_LOOP);
        testMF();
    } else if (cmd == 'M' && line.length() > 2 && line.charAt(1) == ':') {
        if (parser.parseClosedLoopParams(line.substring(2))) {
            parser.setActiveTest(TEST_CLOSED_LOOP);
            testMF();
        }
    }
}

void Orchestrator::simulateClosedLoopStepResponse(float step_target, int num_samples) {
    Serial.println("--- INICIO DO TESTE DE MALHA FECHADA (SIMULADO) ---");
    Serial.println("Tempo(ms),Setpoint,RPM_Simulado,Esforco_Controlo");
    
    // ATENÇÃO: Altere "this->control" para o nome correto da sua instância 
    // de controlo dentro do Orchestrator (ex: this->pid, this->motorControl)
    control.reset();
    control.setSetpoint(step_target);
    
    float y_k = 0.0f;   // RPM Medido (Simulado)
    float u_k = 0.0f;   // Esforço de Controlo Calculado
    
    // Constantes da Planta Discreta (ZOH) com Ts = 20ms
    // Calculadas a partir de K=1, Tau=0.04905s
    float y_k1 = 0.0f;
    float u_k1 = 0.0f;

    for(int k = 0; k < num_samples; k++) {
        float tempo_ms = k * 20.0f; // Avança o tempo em passos de 20ms
        
        // 1. Planta Matemática: simula a reação do motor ao esforço passado
        if (k > 0) {
            y_k = 0.66515f * y_k1 + 0.33485f * u_k1;
        }
        
        // 2. O Controlador avalia a situação atual
        u_k = control.compute(y_k);
        
        // 3. Imprime os dados (Pode copiar diretamente para o Excel ou ver no Serial Plotter)
        Serial.print(tempo_ms);
        Serial.print(",");
        Serial.print(step_target);
        Serial.print(",");
        Serial.print(y_k);
        Serial.print(",");
        Serial.println(u_k);
        
        // 4. Atualiza as variáveis de estado do Motor Simulado
        y_k1 = y_k;
        u_k1 = u_k; 
        
        delay(2); // Pausa muito breve para evitar congestionamento na Serial
    }
    
    Serial.println("--- FIM DO TESTE ---");
    
    // Zera tudo novamente para deixar o controlador pronto para o uso real
    control.reset(); 
}

// =========================================================================
//  stop
// =========================================================================
void Orchestrator::stop() {
    endTest();
    control.reset();
    currentSetpointRpm = 0.0f;
    parser.setActiveTest(TEST_NONE);
    activeMethod = METHOD_NONE;
}

// =========================================================================
//  isRunning
// =========================================================================
bool Orchestrator::isRunning() const {
    return running;
}
