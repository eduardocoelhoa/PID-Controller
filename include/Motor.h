#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

class Motor {
  public:
    Motor(int dir1, int dir2, int pwm, int channel, int freq, int res);
    void init();
    void setPwm(int pwm);
    void stop();

  private:
    int pinPWM, pinDir1, pinDir2;
    int pwmChannel, pwmFreq, pwmRes;
};

#endif
