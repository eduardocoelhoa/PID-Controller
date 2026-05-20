#ifndef ENCODER_H
#define ENCODER_H

#include "driver/pcnt.h"
#include "driver/gpio.h"
#include <Arduino.h>
#include <stdint.h>

// =========================================================================
//  ENCODER
//  Lê pulsos do encoder via hardware (PCNT do ESP32) e calcula a
//  velocidade angular em RPM. Aplica filtro de média móvel para
//  suavizar leituras ruidosas do sensor.
// =========================================================================

class Encoder {
  private:
    pcnt_unit_t unit;        // Unidade PCNT utilizada (hardware do ESP32)
    int ppr;                 // Pulsos por revolução do encoder
    int32_t accumulatedCount; // Contagem total acumulada de pulsos
    unsigned long lastTime;  // Timestamp da última leitura (ms)
    float rpm;               // Velocidade angular filtrada (RPM)

    // Filtro de média móvel — suaviza picos de ruído no sinal do encoder
    static constexpr size_t AVG_WINDOW = 5;  // Tamanho da janela
    float avgBuffer[AVG_WINDOW];             // Buffer circular
    size_t avgIndex;                         // Posição atual no buffer
    size_t avgCount;                         // Amostras válidas acumuladas
    float avgSum;                            // Soma das amostras na janela

    // Lê o delta de pulsos desde a última chamada e zera o contador de hardware
    int32_t readDelta();

    // Aplica média móvel sobre uma amostra bruta
    float applyAvg(float sample);

  public:
    // Construtor: configura os pinos e o PCNT para leitura por hardware.
    Encoder(int pprValue, pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB);

    // Retorna a velocidade angular atual em RPM (calcula e filtra a cada chamada)
    float getRpm();
};

#endif // ENCODER_H
