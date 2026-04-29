#include "Motor.h"

Motor::Motor(int pwm, int dir1, int dir2, int channel)
    : pinPWM(pwm), pinDir1(dir1), pinDir2(dir2), pwmChannel(channel) {

        // Configura os pinos e PWM ao criar o objeto
        begin();
    }

void Motor::begin() {
    pinMode(pinDir1, OUTPUT);
    pinMode(pinDir2, OUTPUT);

    // Configuração do PWM para o ESP32 (Frequência de 5kHz, resolução de 8 bits: 0 a 255)
    ledcSetup(pwmChannel, 5000, 8);
    ledcAttachPin(pinPWM, pwmChannel);
    
    stop(); // Garante que inicie desligado
}

void Motor::setSpeed(int speed) {

    // Determina a direção baseada no sinal da velocidade
    if (speed > 0) {
        digitalWrite(pinDir1, HIGH);
        digitalWrite(pinDir2, LOW);
    } else if (speed < 0) {
        digitalWrite(pinDir1, LOW);
        digitalWrite(pinDir2, HIGH);
    } else {
        stop();
        return;
    }

    // Limita o valor de speed para o intervalo permitido
    constrain(speed, -255, 255); 
    // Aplica o sinal PWM
    ledcWrite(pwmChannel, abs(speed));
}

void Motor::stop() {
    digitalWrite(pinDir1, LOW);
    digitalWrite(pinDir2, LOW);
    ledcWrite(pwmChannel, 0);
}