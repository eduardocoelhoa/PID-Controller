#ifndef ENCODER_H
#define ENCODER_H

#include "driver/pcnt.h"
#include "driver/gpio.h"

class Encoder{

  public:

    Encoder();
    
    int setupEncoder(pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB);
    int updateEncoder();
};

#endif // ENCODER_H