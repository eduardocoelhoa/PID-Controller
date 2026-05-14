#ifndef ENCODER_H
#define ENCODER_H

#include "driver/pcnt.h"
#include "driver/gpio.h"
#include <Arduino.h> // Necessário para usar millis()
#include <stdint.h>

class Encoder{
  private:
    // Variáveis para o cálculo de velocidade (pulsos/s)
    unsigned long lastTime;
    // Contador acumulado de pulsos (corrige wrapping do contador PCNT)
    int32_t accumulatedCount;
    float currentVelocity;
    pcnt_unit_t unit;

    // Variáveis para o cálculo físico (m/s)
    int ppr;              // Pulsos Por Revolução
    float pps;            // Pulsos por Segundo
    float rps;            // Rotações por Segundo
    float circumference;  // Circunferência da roda em metros
    float wheelRadius;    // Raio da roda em metros
    float linearVelocity; // Velocidade Linear em m/s
    float filteredLinearVelocity;

    // Simple moving average for the linear velocity
    static constexpr size_t MOVING_AVG_WINDOW = 5;
    float movingAvgBuffer[MOVING_AVG_WINDOW];
    size_t movingAvgIndex;
    size_t movingAvgCount;
    float movingAvgSum;

    // Configura o PCNT para ler os pulsos do encoder
    int setupEncoder(pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB);
    
    // Configura as dimensões físicas do robô
    void setPhysicalParameters(int pprValue, float radiusInMeters);

    // Lê o contador bruto do PCNT
    int16_t updateEncoder();

    // Atualiza a contagem acumulada e devolve o delta bruto corrigido
    int32_t refreshCount();

    // Retorna a velocidade em Pulsos por Segundo (Hz)
    float calcVelocity(); 

    // Retorna a velocidade linear em Metros por Segundo (m/s)
    void PsToMs(); 

    // Aplica media movel na velocidade linear
    float applyMovingAverage(float sample);

  public:
    Encoder(int pprValue, float radiusInMeters, pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB);
    
    //Retorna a velocidade linear calculada
    float getLinearVelocity();

    // Retorna a contagem acumulada de pulsos do encoder (corrige overflow)
    int32_t getPulseCount();

    // Zera a contagem de pulsos do encoder
    void resetPulseCount();
};

#endif // ENCODER_H