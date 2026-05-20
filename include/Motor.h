#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

// =========================================================================
//  MOTOR
//  Controla um motor DC via ponte H. Gera sinal PWM pelo LEDC do ESP32
//  e controla a direção com dois pinos digitais. O duty cycle aceita
//  valores de -PWM_MAX a +PWM_MAX (resolução definida em PWM_BITS), onde o sinal indica
//  a direção e o módulo a velocidade.
// =========================================================================

class Motor {
  private:
    int pinPWM;     // Pino do sinal PWM
    int pinDir1;    // Pino de direção 1
    int pinDir2;    // Pino de direção 2
    int pwmChannel; // Canal LEDC do ESP32

    // Configura os pinos como saída e inicializa o PWM (5 kHz)
    void begin();

  public:
    // Construtor: recebe os 3 pinos e o canal PWM (padrão = 0)
    Motor(int pwm, int dir1, int dir2, int channel = 0);

    // Aplica um duty cycle ao motor (-PWM_MAX a +PWM_MAX). Sinal = direção.
    void setPwm(int pwm);

    // Para o motor imediatamente (ambos os pinos de direção em LOW)
    void stop();
};

#endif // MOTOR_H
