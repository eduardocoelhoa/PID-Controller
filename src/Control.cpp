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
    // Vamos usar q0, q1 e q2 para guardar nossos coeficientes fixos
    // para Ts = 0.0006s e Filtro N = 100
    this->q0 = 8.5180f;     // Coeficiente Proporcional
    this->q1 = 0.1849f;     // Coeficiente Integral
    this->q2 = 0.9417f;     // Coeficiente Derivativo 1 (Filtro)
}

void Control::reset() {
    this->erro_k1 = 0.0f;
    this->erro_k2 = 0.0f;
    
    // Agora essas variáveis guardam as memórias separadas, não a saída global!
    this->saida_k1 = 0.0f;  // Atuará como memória da Integral (u_i[k-1])
    this->saida_k2 = 0.0f;  // Atuará como memória da Derivativa (u_d[k-1])
}

float Control::compute(float measured) {
    float erro_atual = this->setpoint - measured;

    // ====================================================================
    // 1. CÁLCULO SEPARADO DAS AÇÕES (Evita travamento matemático)
    // ====================================================================
    float u_p = this->q0 * erro_atual;
    
    // u_i = u_i_anterior + Ki_coef * (erro_atual + erro_anterior)
    float u_i = this->saida_k1 + this->q1 * (erro_atual + this->erro_k1);
    
    // u_d = Kd_coef1 * u_d_anterior + Kd_coef2 * (erro_atual - erro_anterior)
    float u_d = (this->q2 * this->saida_k2) + (2.8573f * (erro_atual - this->erro_k1));

    // Soma das ações
    float saida_atual = u_p + u_i + u_d;

    // ====================================================================
    // 2. SATURAÇÃO E ANTI-WINDUP
    // ====================================================================
    if (saida_atual > 4095.0f) {
        saida_atual = 4095.0f;
        u_i = this->saida_k1; // Congela a integral (Anti-windup)
    } else if (saida_atual < 0.0f) {
        saida_atual = 0.0f;
        u_i = this->saida_k1; // Congela a integral (Anti-windup)
    }

    // ====================================================================
    // 3. ATUALIZAÇÃO DA MEMÓRIA
    // ====================================================================
    this->saida_k1 = u_i;  // Salva o estado da integral
    this->saida_k2 = u_d;  // Salva o estado da derivativa
    this->erro_k1 = erro_atual;

    return saida_atual;
}