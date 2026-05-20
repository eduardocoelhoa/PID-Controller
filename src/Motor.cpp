#include "Motor.h"
#include "Constants.h"

// =========================================================================
//  Construtor
//  Recebe os 3 pinos da ponte H e o canal LEDC. Chama begin() para
//  configurar o hardware imediatamente na criação do objeto.
// =========================================================================
Motor::Motor(int pwm, int dir1, int dir2, int channel)
    : pinPWM(pwm), pinDir1(dir1), pinDir2(dir2), pwmChannel(channel) {
    begin();
}

// =========================================================================
//  begin
//  Configura os pinos de direção como saída e inicializa o PWM via LEDC
//  do ESP32: frequência de 5 kHz, resolução definida em PWM_BITS.
//  Garante que o motor comece parado.
// =========================================================================
void Motor::begin() {
    pinMode(pinDir1, OUTPUT);
    pinMode(pinDir2, OUTPUT);
    ledcSetup(pwmChannel, 5000, PWM_BITS);      // 5 kHz
    ledcAttachPin(pinPWM, pwmChannel);
    stop();
}

// =========================================================================
//  setPwm
//  Aplica um duty cycle ao motor. O valor deve estar entre -PWM_MAX e +PWM_MAX.
//    - Sinal positivo → sentido horário  (DIR1=HIGH, DIR2=LOW)
//    - Sinal negativo → sentido anti-horário (DIR1=LOW, DIR2=HIGH)
//    - Zero → chama stop() e retorna imediatamente
// =========================================================================
void Motor::setPwm(int pwm) {
    if (pwm == 0) { stop(); return; }

    int pwmClamped = constrain(pwm, -PWM_MAX, PWM_MAX);
    if (pwmClamped >= 0) {
        digitalWrite(pinDir1, HIGH);
        digitalWrite(pinDir2, LOW);
    } else {
        digitalWrite(pinDir1, LOW);
        digitalWrite(pinDir2, HIGH);
    }

    uint32_t duty = static_cast<uint32_t>(abs(pwmClamped));
    ledcWrite(pwmChannel, duty);
}

// =========================================================================
//  stop
//  Para o motor: ambos os pinos de direção em LOW e PWM em zero.
// =========================================================================
void Motor::stop() {
    digitalWrite(pinDir1, LOW);
    digitalWrite(pinDir2, LOW);
    ledcWrite(pwmChannel, 0);
}
