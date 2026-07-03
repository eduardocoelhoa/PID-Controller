#include "Motor.h"
#include "Constants.h"

Motor::Motor(int dir1, int dir2, int pwm, int channel, int freq, int res)
    : pinPWM(pwm), pinDir1(dir1), pinDir2(dir2), 
      pwmChannel(channel), pwmFreq(freq), pwmRes(res) {
    // O construtor agora apenas armazena os valores.
    // A inicialização é feita em init().
}

void Motor::init() {
    pinMode(pinDir1, OUTPUT);
    pinMode(pinDir2, OUTPUT);
    ledcSetup(pwmChannel, pwmFreq, pwmRes);
    ledcAttachPin(pinPWM, pwmChannel);
    stop();
}
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

void Motor::stop() {
    digitalWrite(pinDir1, LOW);
    digitalWrite(pinDir2, LOW);
    ledcWrite(pwmChannel, 0);
}
