#ifndef CONTROL_H
#define CONTROL_H

class Control {
  public:
    Control(float kp, float ki, float kd = 0.0f);
    void setSetpoint(float sp);
    void setTunings(float kp, float ki, float kd = 0.0f);
    float compute(float measured);
    void reset();

  private:
    float setpoint;
    float kp;
    float ki_dig;
    float termo_integral;
};

#endif // CONTROL_H