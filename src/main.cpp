#include <Arduino.h>
#include "Encoder.h"

Encoder encoder;
int encoderCount = 0;

static constexpr int ENCODER_PIN_A = 32;
static constexpr int ENCODER_PIN_B = 33;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Iniciando teste do encoder...");
  
  // Use pins that do not affect ESP32 boot mode.
  int result = encoder.setupEncoder(PCNT_UNIT_0, PCNT_CHANNEL_0, ENCODER_PIN_A, ENCODER_PIN_B);
  
  if(result == ESP_OK) {
    Serial.println("Encoder configurado com sucesso!");
  } else {
    Serial.println("Erro ao configurar encoder!");
  }
}

void loop() {
  encoderCount = encoder.updateEncoder();
  
  Serial.print("Contagem: ");
  Serial.println(encoderCount);
  
  delay(500);
}