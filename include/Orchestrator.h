#ifndef ORCHESTRATOR_H
#define ORCHESTRATOR_H

#include "Motor.h"
#include "Encoder.h"
#include "Control.h"

class Orchestrator {
  public:
    Orchestrator(Motor& motor, Encoder& encoder, Control& control);
    void init();
    void runClosedLoopTest(float target_rpm, unsigned long duration_ms);

private:
    Motor& motor;
    Encoder& encoder;
    Control& control;
};

#endif // ORCHESTRATOR_H
