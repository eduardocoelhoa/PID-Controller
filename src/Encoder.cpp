#include "Encoder.h"

// =========================================================================
//  Construtor
//  Configura os pinos do encoder como entrada com pull-up, inicializa o
//  módulo PCNT (contador por hardware do ESP32) para contar pulsos em
//  ambos os sentidos, e zera todas as variáveis de estado.
// =========================================================================
Encoder::Encoder(int pprValue, pcnt_unit_t unit, pcnt_channel_t channel, int pinA, int pinB)
    : unit(unit), ppr(pprValue),
      accumulatedCount(0), lastTime(0), rpm(0.0f),
      avgIndex(0), avgCount(0), avgSum(0.0f) {

    for (size_t i = 0; i < AVG_WINDOW; ++i) avgBuffer[i] = 0.0f;

    // Configura pinos como entrada digital com pull-up interno
    gpio_set_direction((gpio_num_t)pinA, GPIO_MODE_INPUT);
    gpio_set_direction((gpio_num_t)pinB, GPIO_MODE_INPUT);
    gpio_pullup_en((gpio_num_t)pinA);
    gpio_pullup_en((gpio_num_t)pinB);

    // Configura o PCNT: conta incrementa no canal A, decrementa no B,
    // usando o canal B como controle de direção
    pcnt_config_t cfg = {};
    cfg.pulse_gpio_num = pinA;          // Pino de pulso (canal A)
    cfg.ctrl_gpio_num  = pinB;          // Pino de controle (canal B — define direção)
    cfg.channel = channel;
    cfg.unit    = unit;
    cfg.pos_mode   = PCNT_COUNT_INC;    // Borda de subida no pulso → incrementa
    cfg.neg_mode   = PCNT_COUNT_DEC;    // Borda de descida no pulso → decrementa
    cfg.lctrl_mode = PCNT_MODE_KEEP;    // Quando ctrl=LOW, mantém contagem
    cfg.hctrl_mode = PCNT_MODE_REVERSE; // Quando ctrl=HIGH, inverte contagem
    cfg.counter_h_lim = 32767;          // Limite máximo do contador (int16)
    cfg.counter_l_lim = -32768;         // Limite mínimo do contador (int16)

    // Aplica a configuração e ativa o filtro de ruído (100 ciclos de clock)
    if (pcnt_unit_config(&cfg) == ESP_OK) {
        pcnt_set_filter_value(unit, 100);
        pcnt_filter_enable(unit);
    }

    // Zera o contador e inicia a contagem
    pcnt_counter_pause(unit);
    pcnt_counter_clear(unit);
    pcnt_counter_resume(unit);
    lastTime = millis();
}

// =========================================================================
//  readDelta
//  Pausa o contador de hardware, lê o valor acumulado desde a última
//  chamada, zera o contador e retoma a contagem. Retorna o número de
//  pulsos lidos (positivo = sentido horário, negativo = anti-horário).
// =========================================================================
int32_t Encoder::readDelta() {
    pcnt_counter_pause(unit);
    int16_t raw = 0;
    pcnt_get_counter_value(unit, &raw);
    pcnt_counter_clear(unit);
    pcnt_counter_resume(unit);
    accumulatedCount += raw;
    return raw;
}

// =========================================================================
//  applyAvg
//  Filtro de média móvel com janela deslizante. Mantém um buffer circular
//  e retorna a média das últimas N amostras. Suaviza picos de ruído no
//  sinal do encoder sem introduzir atraso significativo.
// =========================================================================
float Encoder::applyAvg(float sample) {
    if (avgCount < AVG_WINDOW) {
        // Ainda preenchendo o buffer — acumula sem remover nada
        avgBuffer[avgIndex] = sample;
        avgSum += sample;
        ++avgCount;
    } else {
        // Buffer cheio — remove o valor mais antigo e insere o novo
        avgSum -= avgBuffer[avgIndex];
        avgBuffer[avgIndex] = sample;
        avgSum += sample;
    }
    avgIndex = (avgIndex + 1) % AVG_WINDOW;
    return avgSum / static_cast<float>(avgCount);
}

// =========================================================================
//  getRpm
//  Função principal de leitura. Calcula a velocidade angular em RPM:
//    1. Lê o delta de pulsos desde a última chamada (readDelta)
//    2. Converte pulsos/s → RPM:  RPM = (PPS / PPR) × 60
//    3. Aplica o filtro de média móvel para suavizar
//  Só recalcula se passou pelo menos 1 ms desde a última leitura.
// =========================================================================
float Encoder::getRpm() {
    unsigned long now = millis();
    unsigned long dt = now - lastTime;

    if (dt > 0) {
        // Converte delta de pulsos e intervalo de tempo para pulsos/segundo
        float pps = static_cast<float>(readDelta()) * 1000.0f / static_cast<float>(dt);
        // Converte PPS → RPM e aplica filtro de média móvel
        rpm = applyAvg((pps / static_cast<float>(ppr)) * 60.0f);
        lastTime = now;
    }

    return rpm;
}
