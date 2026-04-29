#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

class Motor {
  private:
    int pinPWM;
    int pinDir1;
    int pinDir2;
    int pwmChannel;

    // Inicializa as configurações de pinos e PWM
    void begin();

  public:
    // Construtor: recebe os pinos e o canal PWM a ser utilizado
    Motor(int pwm, int dir1, int dir2, int channel = 0);
    
    // Define a velocidade e direção (-255 a 255)
    void setSpeed(int speed);
    
    // Para o motor
    void stop();
};

#endif // MOTOR_H