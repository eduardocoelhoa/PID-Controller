#include <Arduino.h>
#include "Encoder.h"
#include "Motor.h"
#include "Orchestrator.h"
#include "Constants.h"

// =========================================================================
//  INSTÂNCIAS GLOBAIS
// =========================================================================
Encoder encoder(PPR, PCNT_UNIT, PCNT_CHANNEL, PIN_ENC_A, PIN_ENC_B);
Motor motor(PIN_PWM, PIN_DIR1, PIN_DIR2, PWM_CHANNEL);
Orchestrator orchestrator(encoder, motor);

// Tipo de teste ativo (nenhum, frequência ou degrau)
enum TestType { TEST_NONE, TEST_FREQ, TEST_STEP };
TestType activeTest = TEST_NONE;

// Parâmetros configuráveis via serial (valores padrão)
int stepPwm = STEP_PWM;
unsigned long stepDurationMs = STEP_DURATION_MS;
int freqOffsetPwm = OFFSET_PWM;
int freqAmplitudePwm = AMPLITUDE_PWM;
float freqHz = FREQ_HZ;
unsigned long freqDurationMs = FREQ_DURATION_MS;

// =========================================================================
//  setup
// =========================================================================
void setup() {
    Serial.begin(BAUD_RATE);
}

// =========================================================================
//  parseParams
//  Lê parâmetros de uma string no formato "pwm,duracao" ou
//  "offset,amplitude,freq,duracao". Retorna true se parseou com sucesso.
// =========================================================================
bool parseStepParams(const String& params) {
    int commaIdx = params.indexOf(',');
    if (commaIdx < 0) return false;

    stepPwm = constrain(params.substring(0, commaIdx).toInt(), 0, PWM_MAX);
    stepDurationMs = params.substring(commaIdx + 1).toInt() * 1000UL;
    return stepDurationMs > 0;
}

bool parseFreqParams(const String& params) {
    // Formato: "offset,amplitude,freq,duracao"
    int idx1 = params.indexOf(',');
    if (idx1 < 0) return false;
    int idx2 = params.indexOf(',', idx1 + 1);
    if (idx2 < 0) return false;
    int idx3 = params.indexOf(',', idx2 + 1);
    if (idx3 < 0) return false;

    freqOffsetPwm = constrain(params.substring(0, idx1).toInt(), -PWM_MAX, PWM_MAX);
    freqAmplitudePwm = constrain(params.substring(idx1 + 1, idx2).toInt(), 0, PWM_MAX);
    freqHz = params.substring(idx2 + 1, idx3).toFloat();
    freqDurationMs = params.substring(idx3 + 1).toInt() * 1000UL;
    return freqDurationMs > 0 && freqHz > 0.0f;
}

// =========================================================================
//  loop
// =========================================================================
void loop() {
    // Se um teste está ativo, continuá-lo
    if (orchestrator.isRunning()) {
        // Verifica comando de parada
        if (Serial.available() > 0) {
            char cmd = Serial.peek();
            if (cmd == 'x' || cmd == 'X') {
                Serial.read(); // consome o byte
                orchestrator.stop();
                activeTest = TEST_NONE;
                return;
            }
        }

        // Continua o teste ativo
        if (activeTest == TEST_FREQ) {
            if (!orchestrator.runFrequencyTest(freqOffsetPwm, freqAmplitudePwm, freqHz, freqDurationMs)) {
                activeTest = TEST_NONE;
            }
        } else if (activeTest == TEST_STEP) {
            if (!orchestrator.runStepTest(stepPwm, stepDurationMs)) {
                activeTest = TEST_NONE;
            }
        }
        return;
    }

    // Nenhum teste ativo — aguarda comando
    if (Serial.available() <= 0) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    char cmd = line.charAt(0);

    if (cmd == 'f') {
        // Frequência com parâmetros padrão
        activeTest = TEST_FREQ;
        orchestrator.runFrequencyTest(freqOffsetPwm, freqAmplitudePwm, freqHz, freqDurationMs);
    } else if (cmd == 'F') {
        // Frequência com parâmetros: F:offset,amplitude,freq,duracao
        if (line.length() > 2 && line.charAt(1) == ':') {
            if (parseFreqParams(line.substring(2))) {
                activeTest = TEST_FREQ;
                orchestrator.runFrequencyTest(freqOffsetPwm, freqAmplitudePwm, freqHz, freqDurationMs);
            }
        }
    } else if (cmd == 's') {
        // Degrau com parâmetros padrão
        activeTest = TEST_STEP;
        orchestrator.runStepTest(stepPwm, stepDurationMs);
    } else if (cmd == 'S') {
        // Degrau com parâmetros: S:pwm,duracao
        if (line.length() > 2 && line.charAt(1) == ':') {
            if (parseStepParams(line.substring(2))) {
                activeTest = TEST_STEP;
                orchestrator.runStepTest(stepPwm, stepDurationMs);
            }
        }
    }
}
