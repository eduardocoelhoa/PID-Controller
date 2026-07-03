#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

class Encoder {
public:
    Encoder(int pin);
    void init();
    float getRPM();

    static void IRAM_ATTR handleInterrupt();

private:
    int encoderPin;
    volatile unsigned long pulseCount;
    volatile unsigned long lastInterruptTime;
    volatile unsigned long pulsePeriod; 
    float rpm_ema; // Para o filtro passa-baixa (EMA)

    static Encoder* instance;
};

#endif