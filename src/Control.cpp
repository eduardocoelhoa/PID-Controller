#include "Control.h"

Control::Control(float kp, float ki, float kd) {
    this->setpoint = 0.0f;
    this->reset();
    this->setTunings(kp, ki, kd);
}

void Control::setSetpoint(float sp) {
    this->setpoint = sp;
}

void Control::setTunings(float kp, float ki, float kd) {
    this->kp = kp; 
    this->ki_dig = ki * 0.02f; // Ts = 0.02s
}

void Control::reset() {
    this->termo_integral = 0.0f;
}

float Control::compute(float measured) {
    float erro = this->setpoint - measured;
    this->termo_integral += (this->ki_dig * erro);
    
    float saida_rpm = (this->kp * erro) + this->termo_integral;
    float saida_pwm = saida_rpm * 20.475f; // Fator de conversão

    // --- Saturação COM Anti-Windup bidirecional ---
    if (saida_pwm > 4095.0f) {
        saida_pwm = 4095.0f;
        this->termo_integral -= (this->ki_dig * erro); // Anti-windup (teto)
    } else if (saida_pwm < -4095.0f) { 
        saida_pwm = -4095.0f;
        this->termo_integral -= (this->ki_dig * erro); // Anti-windup (fundo)
    }

    return saida_pwm;
}