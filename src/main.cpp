#include <Arduino.h>
#include "Constants.h"
#include "Motor.h"
#include "Encoder.h"
#include "Control.h"
#include "Orchestrator.h"

// Frequência do PWM (único parâmetro que não estava no Constants.h)
constexpr int PWM_FREQ = 5000;

// Inicializa usando EXATAMENTE as constantes do seu Constants.h
Encoder encoder(PIN_ENC_A); 
Motor motor(PIN_DIR1, PIN_DIR2, PIN_PWM, PWM_CHANNEL, PWM_FREQ, PWM_BITS);
Control control(6.388f, 277.7f, 0.0f); // Ganhos PI
Orchestrator orchestrator(motor, encoder, control);
void setup() {
    Serial.begin(BAUD_RATE);
    delay(100); 

    Serial.println("==================================================");
    Serial.println(" Controlador de Motor DC - Modo de Teste PI (MF)");
    Serial.println("==================================================");

    orchestrator.init();
    Serial.println("-&gt; Envie 'T' (teste padrão) ou 'T:rpm,duracao_ms' para iniciar.");
}

void loop() {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        if (command.length() == 0) return;

        // Valores padrão para o teste
        float target_rpm = 200.0f;
        unsigned long duration_ms = 2000;

        if (command.equalsIgnoreCase("T")) {
            // Comando 'T' simples usa os valores padrão.
        } else if (command.toUpperCase().startsWith("T:")) {
            // Tenta interpretar "T:rpm,duracao_ms"
            String params = command.substring(2);
            int commaIdx = params.indexOf(',');
            if (commaIdx > 0) {
                target_rpm = params.substring(0, commaIdx).toFloat();
                duration_ms = params.substring(commaIdx + 1).toInt();
            }
        } else {
            return; // Ignora comandos desconhecidos
        }

        Serial.printf("\n[*] A iniciar teste: %.1f RPM por %lu ms.\n", target_rpm, duration_ms);
        orchestrator.runClosedLoopTest(target_rpm, duration_ms);
        Serial.println("\n[*] Teste terminado. Envie novo comando para repetir.");
    }
}