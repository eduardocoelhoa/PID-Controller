#ifndef CONTROL_H
#define CONTROL_H

class Control {
public:
    // Construtor
    Control(float kp, float ki, float kd);

    // Métodos principais
    void setSetpoint(float sp);
    void setTunings(float kp, float ki, float kd);
    float compute(float measured);
    
    // Zera os estados da memória (importante caso o motor seja parado)
    void reset();

private:
    float setpoint;

    // Coeficientes da equação de diferenças discreta (q0, q1, q2)
    float q0;
    float q1;
    float q2;

    // Memória de estado para o controlador digital (deslizamento no tempo)
    float erro_k1;  // Erro no instante k-1 (e[k-1])
    float erro_k2;  // Erro no instante k-2 (e[k-2])
    float saida_k1; // Saída no instante k-1 (u[k-1])
    float saida_k2; // Saída no instante k-2 (u[k-2])
};

#endif