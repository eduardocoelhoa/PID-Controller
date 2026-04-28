#include "Encoder.h"

Encoder::Encoder(){}

int Encoder::setupEncoder(pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB){
    
    // Configura os pinos de entrada do encoder
    gpio_set_direction((gpio_num_t)pinA, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)pinB, GPIO_MODE_INPUT);
    
    //Ativa pull-up nos pinos do encoder
    gpio_pullup_en((gpio_num_t)pinA);
    gpio_pullup_en((gpio_num_t)pinB);

    // Configura o PCNT
    pcnt_config_t pcnt_config;
    pcnt_config.pulse_gpio_num = pinA;
    pcnt_config.ctrl_gpio_num = pinB;
    pcnt_config.channel = channel;
    pcnt_config.unit = unit;
    pcnt_config.pos_mode = PCNT_COUNT_INC;       // Incrementa na borda de subida
    pcnt_config.neg_mode = PCNT_COUNT_DEC;       // Decrementa na borda de descida
    pcnt_config.lctrl_mode = PCNT_MODE_KEEP;     // Mantém o contador quando o sinal de controle é baixo
    pcnt_config.hctrl_mode = PCNT_MODE_REVERSE;  // Inverte a contagem quando o sinal de controle é alto
    pcnt_config.counter_h_lim = 32767;
    pcnt_config.counter_l_lim = -32768;

    return pcnt_unit_config(&pcnt_config);
}

int Encoder::updateEncoder(){
    int16_t count;
    pcnt_get_counter_value(PCNT_UNIT_0, &count);
    return count;
}