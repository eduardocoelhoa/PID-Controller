#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include "Encoder.h"
#include "Motor.h"
#include "Orchestrator.h"
#include "Constants.h"

Encoder encoder(PPR, WHEEL_RADIUS_M, PCNT_UNIT, PCNT_CHANNEL, PIN_ENC_A, PIN_ENC_B);
Motor motor(PIN_PWM, PIN_DIR1, PIN_DIR2, PWM_CHANNEL);
Orchestrator orchestrator(encoder, motor);
bool stepActive = false;
bool stepLogRequested = false;
bool stepLogOk = false;
bool spiffsOk = false;

void setup() {
  Serial.begin(115200);
	spiffsOk = SPIFFS.begin(true);
	if (!spiffsOk) {
		Serial.println("Falha ao montar SPIFFS");
	}
	Serial.println("Teste de resposta ao degrau: envie 's' para iniciar.");
	Serial.printf("PWM=%d, Duracao=%lu ms\n", STEP_PWM, STEP_DURATION_MS);
}

void loop() {
	if (stepActive) {
		if (Serial.available() > 0) {
			String line = Serial.readStringUntil('\n');
			line.trim();
			if (line == "x" || line == "X") {
				orchestrator.stop();
				stepActive = false;
				Serial.println("Teste cancelado. Envie 's' para iniciar novamente.");
				return;
			}
		}

		if (!orchestrator.runStepTest(STEP_PWM, STEP_DURATION_MS)) {
			if (!stepLogOk) {
				orchestrator.printInfos();
			}

			if (stepLogRequested) {
				if (stepLogOk) {
					Serial.println("Dados salvos em /step.txt");
				} else {
					Serial.println("Falha ao salvar /step.txt");
				}
				stepLogRequested = false;
				stepLogOk = false;
			}

			Serial.println("Teste finalizado. Envie 's' para iniciar novamente.");
			stepActive = false;
		}
		return;
	}

	if (Serial.available() <= 0) {
		return;
	}

	String line = Serial.readStringUntil('\n');
	line.trim();
	if (line.isEmpty()) {
		return;
	}

	if (line == "s" || line == "S") {
		stepLogRequested = true;
		if (spiffsOk) {
			stepLogOk = orchestrator.startStepLog("/step.txt");
			if (!stepLogOk) {
				Serial.println("Falha ao abrir /step.txt");
			}
		} else {
			stepLogOk = false;
			Serial.println("SPIFFS indisponivel; log nao sera salvo.");
		}
		stepActive = true;
		orchestrator.runStepTest(STEP_PWM, STEP_DURATION_MS);
		return;
	}

	if (line == "d" || line == "D") {
		if (!spiffsOk) {
			Serial.println("SPIFFS indisponivel.");
			return;
		}

		File file = SPIFFS.open("/step.txt", FILE_READ);
		if (!file) {
			Serial.println("Arquivo /step.txt nao encontrado.");
			return;
		}

		Serial.println("---- /step.txt ----");
		while (file.available()) {
			Serial.write(file.read());
		}
		file.close();
		Serial.println("\n---- fim ----");
		return;
	}

	Serial.println("Envie 's' para iniciar o teste de degrau.");
}