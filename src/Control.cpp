#include "Control.h"
#include <Arduino.h>

// =========================================================================
//  Construtor
//  Inicializa o controlador PID com os ganhos especificados e os limites
//  de saída (por padrão, -4095 a +4095 para compatível com o PWM de 12 bits).
// =========================================================================
PIDController::PIDController(float kp, float ki, float kd, float outMin, float outMax)
    : kp(kp), ki(ki), kd(kd),
      setpoint(0.0f), integral(0.0f), prevError(0.0f), prevTimeUs(0),
      outMin(outMin), outMax(outMax) {}

// =========================================================================
//  setTunings
//  Atualiza os três ganhos do controlador em tempo de execução.
//  Útil para ajuste fino via Serial ou interface.
// =========================================================================
void PIDController::setTunings(float kp, float ki, float kd) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

// =========================================================================
//  setSetpoint
//  Define o valor desejado (setpoint). No contexto do motor, é o RPM alvo.
// =========================================================================
void PIDController::setSetpoint(float setpoint) {
    this->setpoint = setpoint;
}

// =========================================================================
//  reset
//  Zera o estado interno do controlador (integral e erro anterior).
//  Deve ser chamado ao trocar de setpoint ou reiniciar um teste.
// =========================================================================
void PIDController::reset() {
    integral = 0.0f;
    prevError = 0.0f;
    prevTimeUs = 0;
}

// =========================================================================
//  compute
//  Calcula a ação de controle (saída PID) dado o valor medido atual.
//
//  Algoritmo:
//    1. Calcula o erro:  e = setpoint - measurement
//    2. Calcula dt (intervalo desde a última chamada em segundos)
//    3. Acumula a integral:  I += e × dt
//    4. Calcula a derivada:  D = (e - e_anterior) / dt
//    5. Saída:  u = Kp×e + Ki×I + Kd×D
//    6. Anti-windup: se a saída satura, desfaz o passo integral
//
//  Na primeira chamada (prevTimeUs == 0), retorna apenas Kp×e para
//  evitar divisão por zero no cálculo de dt.
// =========================================================================
float PIDController::compute(float measurement) {
    unsigned long nowUs = micros();

    float error = setpoint - measurement;

    // Primeira chamada: sem histórico → retorna apenas ação proporcional
    if (prevTimeUs == 0) {
        prevTimeUs = nowUs;
        prevError = error;
        return kp * error;
    }

    // Calcula dt em segundos (com proteção contra valores <= 0)
    float dt = static_cast<float>(nowUs - prevTimeUs) / 1000000.0f;
    if (dt <= 0.0f) dt = 1e-6f;

    // Termos PID
    float derivative = (error - prevError) / dt;
    integral += error * dt;
    float output = kp * error + ki * integral + kd * derivative;

    // Anti-windup por clamping: se a saída saturou, desfaz o acúmulo integral
    if (output > outMax) {
        output = outMax;
        if (error > 0) integral -= error * dt;
    } else if (output < outMin) {
        output = outMin;
        if (error < 0) integral -= error * dt;
    }

    prevError = error;
    prevTimeUs = nowUs;

    return output;
}
