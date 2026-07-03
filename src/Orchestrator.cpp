#include "Orchestrator.h"
#include <Arduino.h>

Orchestrator::Orchestrator(Motor& motor, Encoder& encoder, Control& control)
    : motor(motor), encoder(encoder), control(control) {}

void Orchestrator::init() {
    motor.init();
    encoder.init();
    motor.stop();
    control.reset();
}

void Orchestrator::runClosedLoopTest(float target_rpm, unsigned long duration_ms) {
    control.setSetpoint(target_rpm);
    control.reset(); 
    
    unsigned long testDurationMicros = duration_ms * 1000UL;
    unsigned long sampleIntervalMicros = 20000; // 50Hz (Obrigatório)
    
    Serial.println("--- INICIO DO TESTE MF (REAL) ---");
    Serial.println("Tempo(ms),Setpoint,RPM,PWM");

    unsigned long testStartTime = micros();
    unsigned long nextSampleTime = testStartTime;

    while (micros() - testStartTime < testDurationMicros) {
        unsigned long currentMicros = micros();
        
        if (currentMicros >= nextSampleTime) {
            float rpm = encoder.getRPM(); 
            int pwm = control.compute(rpm);
            motor.setPwm(pwm);

            float timeMs = (currentMicros - testStartTime) / 1000.0f;
            Serial.print(timeMs, 3); Serial.print(",");
            Serial.print(target_rpm, 2); Serial.print(",");
            Serial.print(rpm, 2); Serial.print(",");
            Serial.println(pwm);

            nextSampleTime += sampleIntervalMicros;
        }
        yield(); 
    }
    
    motor.stop();
    control.setSetpoint(0.0f); // O alvo agora é 0 RPM para a desaceleração

    // --- Loop Adicional para Capturar a Desaceleração ---
    // Continua a enviar dados por até 2 segundos ou até o motor parar.
    unsigned long spinDownStartTime = micros();
    while (micros() - spinDownStartTime < 2000000) {
        unsigned long currentMicros = micros();
        if (currentMicros >= nextSampleTime) {
            float rpm = encoder.getRPM();
            // O controlador pode aplicar travagem ativa (PWM negativo)
            int pwm = control.compute(rpm);
            motor.setPwm(pwm);

            float timeMs = (currentMicros - testStartTime) / 1000.0f;
            Serial.print(timeMs, 3); Serial.print(",");
            Serial.print(0.0f, 2); Serial.print(","); // Setpoint é 0.0
            Serial.print(rpm, 2); Serial.print(",");
            Serial.println(pwm);

            nextSampleTime += sampleIntervalMicros;

            // Se o motor parou, podemos terminar a captura mais cedo
            if (rpm < 1.0f && rpm > -1.0f && (micros() - spinDownStartTime > 200000)) {
                break;
            }
        }
        yield();
    }

    motor.stop(); // Garante que o motor está parado
    control.reset();
    Serial.println("--- FIM DO TESTE ---");
}
