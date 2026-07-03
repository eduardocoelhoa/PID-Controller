#include <Arduino.h>
#include "Encoder.h"

#define PULSES_PER_REVOLUTION 22.0f
#define EMA_ALPHA 0.2f // Coeficiente de suavização para o filtro EMA (0.0 a 1.0)

Encoder* Encoder::instance = nullptr;

Encoder::Encoder(int pin) : encoderPin(pin), pulseCount(0), lastInterruptTime(0), pulsePeriod(0), rpm_ema(0.0f) {
    instance = this;
}

void Encoder::init() {
    pinMode(encoderPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(encoderPin), handleInterrupt, RISING);
    lastInterruptTime = micros();
}

void IRAM_ATTR Encoder::handleInterrupt() {
    unsigned long currentMicros = micros();
    unsigned long timeDifference = currentMicros - instance->lastInterruptTime;

    if (timeDifference > 1000) { // Debounce de 1ms
        instance->pulseCount++;
        instance->pulsePeriod = timeDifference; 
        instance->lastInterruptTime = currentMicros;
    }
}

float Encoder::getRPM() {
    unsigned long currentMicros = micros();
    
    // Se não houver pulso por um longo período, assume-se que o motor parou.
    // Aumentado de 150ms para 500ms para detetar velocidades mais baixas.
    if (currentMicros - lastInterruptTime > 500000) {
        rpm_ema = 0.0f; // Reseta o filtro quando o motor para
        return 0.0f;
    }
    if (pulsePeriod == 0) return 0.0f;

    float raw_rpm = (1000000.0f / (float)pulsePeriod) * (60.0f / PULSES_PER_REVOLUTION);

    // Aplica o filtro Exponencial Moving Average (EMA) para suavizar a leitura
    rpm_ema = (raw_rpm * EMA_ALPHA) + (rpm_ema * (1.0f - EMA_ALPHA));

    return rpm_ema;
}