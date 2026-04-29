#include "Encoder.h"

Encoder::Encoder(int pprValue, float radiusInMeters, pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB){
    
    setupEncoder(unit, channel, pinA, pinB);
    
    lastTime = 0;
    lastCount = 0;
    currentVelocity = 0.0;
    
    // Inicialização segura das variáveis físicas
    setPhysicalParameters(pprValue, radiusInMeters);
    pps = 0.0;
    rps = 0.0;
    linearVelocity = 0.0;
}

int Encoder::setupEncoder(pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB){
    
    // Configura os pinos de entrada do encoder
    gpio_set_direction((gpio_num_t)pinA, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)pinB, GPIO_MODE_INPUT);
    
    // Ativa pull-up nos pinos do encoder
    gpio_pullup_en((gpio_num_t)pinA);
    gpio_pullup_en((gpio_num_t)pinB);

    // Configura o PCNT
    pcnt_config_t pcnt_config;
    pcnt_config.pulse_gpio_num = pinA;
    pcnt_config.ctrl_gpio_num = pinB;
    pcnt_config.channel = channel;
    pcnt_config.unit = unit;
    pcnt_config.pos_mode = PCNT_COUNT_INC;       
    pcnt_config.neg_mode = PCNT_COUNT_DEC;       
    pcnt_config.lctrl_mode = PCNT_MODE_KEEP;     
    pcnt_config.hctrl_mode = PCNT_MODE_REVERSE;  
    pcnt_config.counter_h_lim = 32767;
    pcnt_config.counter_l_lim = -32768;

    esp_err_t err = pcnt_unit_config(&pcnt_config);
    
    if (err == ESP_OK) {
        // Filtro de ruído
        pcnt_set_filter_value(unit, 100); 
        pcnt_filter_enable(unit);

        // Inicializa o contador
        pcnt_counter_pause(unit);
        pcnt_counter_clear(unit);
        pcnt_counter_resume(unit);
    }

    // Marca o tempo inicial e contagem inicial
    lastTime = millis();
    pcnt_get_counter_value(PCNT_UNIT_0, &lastCount);

    return err;
}

int Encoder::updateEncoder(){
    int16_t count;
    pcnt_get_counter_value(PCNT_UNIT_0, &count);
    return count;
}

float Encoder::calcVelocity(){
    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - lastTime;

    // Evita a divisão por zero e processa apenas se o tempo avançou
    if (deltaTime > 0) {
        int16_t currentCount = updateEncoder();
        int16_t deltaCount = currentCount - lastCount;
        
        // Calcula a velocidade em Pulsos por Segundo (PPS)
        currentVelocity = ((float)deltaCount * 1000.0) / (float)deltaTime;

        // Atualiza a memória para o próximo ciclo
        lastTime = currentTime;
        lastCount = currentCount;
    }
    
    return currentVelocity;
}

void Encoder::setPhysicalParameters(int pprValue, float radiusInMeters) {
    this->ppr = pprValue;
    this->wheelRadius = radiusInMeters;
}

void Encoder::PsToMs() {
    // 1. Pega a velocidade bruta
    this->pps = calcVelocity();
    
    // 2. Proteção (Fail-safe): Se os parâmetros não foram configurados, retorna 0
    if (this->ppr <= 0 || this->wheelRadius <= 0.0) {
        this->linearVelocity = 0.0;
        return;
    }

    // 3. Converte de Pulsos/s para Rotações/s (Voltas por segundo)
    this->rps = this->pps / (float)this->ppr;

    // 4. Calcula o perímetro (Circunferência = 2 * PI * Raio)
    this->circumference = 2.0 * PI * this->wheelRadius;

    // 5. Velocidade linear = Voltas por segundo * Tamanho da volta
    this->linearVelocity = this->rps * this->circumference;
}

float Encoder::getLinearVelocity() {
    // Atualiza a velocidade linear antes de retornar
    PsToMs();
    return this->linearVelocity;
}