#ifndef CONTROL_H
#define CONTROL_H

// =========================================================================
//  PID CONTROLLER
//  Implementa o algoritmo PID (Proporcional-Integral-Derivativo) para
//  controle em malha fechada. Calcula a ação de controle (PWM) com base
//  no erro entre o setpoint (RPM desejado) e a medição (RPM atual).
//  Inclui anti-windup por clamping da integral.
//
//  NOTA: Esta classe ainda não está integrada ao Orchestrator/main.
//  Foi criada como preparação para a futura malha fechada.
// =========================================================================

class PIDController {
  private:
    float kp;            // Ganho proporcional
    float ki;            // Ganho integral
    float kd;            // Ganho derivativo

    float setpoint;      // Valor desejado (RPM alvo)
    float integral;      // Acumulador do termo integral
    float prevError;     // Erro da iteração anterior (para calcular a derivada)
    unsigned long prevTimeUs; // Timestamp da iteração anterior (µs)

    float outMin;        // Limite inferior da saída (anti-saturação)
    float outMax;        // Limite superior da saída (anti-saturação)

  public:
    // Construtor: recebe os ganhos Kp, Ki, Kd e os limites de saída
    PIDController(float kp, float ki, float kd, float outMin = -4095.0f, float outMax = 4095.0f);

    // Atualiza os ganhos do controlador em tempo de execução
    void setTunings(float kp, float ki, float kd);

    // Define o setpoint (valor desejado)
    void setSetpoint(float setpoint);

    // Reseta o estado interno (integral e erro anterior)
    void reset();

    // Calcula a ação de controle dado o valor medido atual. Retorna o PWM calculado.
    float compute(float measurement);
};

#endif // CONTROL_H
