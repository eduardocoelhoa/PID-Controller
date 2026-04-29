#ifndef ENCODER_H
#define ENCODER_H

#include "driver/pcnt.h"
#include "driver/gpio.h"
#include <Arduino.h> // Necessário para usar millis()

class Encoder{
  private:
    // Variáveis para o cálculo de velocidade (pulsos/s)
    unsigned long lastTime;
    int16_t lastCount;
    float currentVelocity;

    // Variáveis para o cálculo físico (m/s)
    int ppr;              // Pulsos Por Revolução
    float pps;            // Pulsos por Segundo
    float rps;            // Rotações por Segundo
    float circumference;  // Circunferência da roda em metros
    float wheelRadius;    // Raio da roda em metros
    float linearVelocity; // Velocidade Linear em m/s

    // Configura o PCNT para ler os pulsos do encoder
    int setupEncoder(pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB);
    
    // Configura as dimensões físicas do robô
    void setPhysicalParameters(int pprValue, float radiusInMeters);

    // Retorna a contagem absoluta de pulsos
    int updateEncoder();

    // Retorna a velocidade em Pulsos por Segundo (Hz)
    float calcVelocity(); 

    // Retorna a velocidade linear em Metros por Segundo (m/s)
    void PsToMs(); 

  public:
    Encoder(int pprValue, float radiusInMeters, pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB);
    
    //Retorna a velocidade linear calculada
    float getLinearVelocity();
};

#endif // ENCODER_H