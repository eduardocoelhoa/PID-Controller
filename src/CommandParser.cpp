#include "CommandParser.h"

CommandParser::CommandParser()
    : _activeTest(TEST_NONE)
    , _stepPwm(STEP_PWM)
    , _stepDurationMs(STEP_DURATION_MS)
    , _freqOffsetPwm(OFFSET_PWM)
    , _freqAmplitudePwm(AMPLITUDE_PWM)
    , _freqHz(FREQ_HZ)
    , _freqDurationMs(FREQ_DURATION_MS)
    , _closedLoopSetpointRpm(STEP_TARGET_RPM)
    , _closedLoopDurationMs(CLOSED_LOOP_DURATION_MS)
{}

bool CommandParser::parseStepParams(const String& params) {
    int commaIdx = params.indexOf(',');
    if (commaIdx < 0) return false;

    _stepPwm = constrain(params.substring(0, commaIdx).toInt(), 0, PWM_MAX);
    _stepDurationMs = params.substring(commaIdx + 1).toInt() * 1000UL;
    return _stepDurationMs > 0;
}

bool CommandParser::parseFreqParams(const String& params) {
    // Formato: "offset,amplitude,freq,duracao"
    int idx1 = params.indexOf(',');
    if (idx1 < 0) return false;
    int idx2 = params.indexOf(',', idx1 + 1);
    if (idx2 < 0) return false;
    int idx3 = params.indexOf(',', idx2 + 1);
    if (idx3 < 0) return false;

    _freqOffsetPwm = constrain(params.substring(0, idx1).toInt(), -PWM_MAX, PWM_MAX);
    _freqAmplitudePwm = constrain(params.substring(idx1 + 1, idx2).toInt(), 0, PWM_MAX);
    _freqHz = params.substring(idx2 + 1, idx3).toFloat();
    _freqDurationMs = params.substring(idx3 + 1).toInt() * 1000UL;
    return _freqDurationMs > 0 && _freqHz > 0.0f;
}

bool CommandParser::parseClosedLoopParams(const String& params) {
    int commaIdx = params.indexOf(',');
    if (commaIdx < 0) return false;

    _closedLoopSetpointRpm = params.substring(0, commaIdx).toFloat();
    _closedLoopDurationMs = params.substring(commaIdx + 1).toInt() * 1000UL;
    return _closedLoopSetpointRpm > 0.0f && _closedLoopDurationMs > 0;
}
