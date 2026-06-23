#include <Arduino.h>
#include "Encoder.h"
#include "Motor.h"
#include "CommandParser.h"
#include "Orchestrator.h"
#include "Constants.h"

// =========================================================================
//  INSTÂNCIAS GLOBAIS
// =========================================================================
Encoder encoder(PPR, PCNT_UNIT, PCNT_CHANNEL, PIN_ENC_A, PIN_ENC_B);
Motor motor(PIN_PWM, PIN_DIR1, PIN_DIR2, PWM_CHANNEL);
CommandParser parser;
Orchestrator orchestrator(encoder, motor, parser);

// =========================================================================
//  setup / loop
// =========================================================================
void setup() {
    Serial.begin(BAUD_RATE);
}

void loop() {
    orchestrator.update();
}
