#ifndef COMMAND_PARSER_H
#define COMMAND_PARSER_H

#include <Arduino.h>
#include "Constants.h"

// Tipo de teste ativo
enum TestType { TEST_NONE, TEST_FREQ, TEST_STEP };

class CommandParser {
public:
    CommandParser();

    // Getters do estado
    TestType activeTest() const { return _activeTest; }
    bool isTestActive() const { return _activeTest != TEST_NONE; }

    // Getters dos parâmetros de degrau
    int stepPwm() const { return _stepPwm; }
    unsigned long stepDurationMs() const { return _stepDurationMs; }

    // Getters dos parâmetros de frequência
    int freqOffsetPwm() const { return _freqOffsetPwm; }
    int freqAmplitudePwm() const { return _freqAmplitudePwm; }
    float freqHz() const { return _freqHz; }
    unsigned long freqDurationMs() const { return _freqDurationMs; }

    // Define o teste ativo
    void setActiveTest(TestType type) { _activeTest = type; }

    // Parseia parâmetros de degrau a partir de "pwm,duracao"
    bool parseStepParams(const String& params);

    // Parseia parâmetros de frequência a partir de "offset,amplitude,freq,duracao"
    bool parseFreqParams(const String& params);

private:
    TestType _activeTest;

    // Parâmetros de degrau
    int _stepPwm;
    unsigned long _stepDurationMs;

    // Parâmetros de frequência
    int _freqOffsetPwm;
    int _freqAmplitudePwm;
    float _freqHz;
    unsigned long _freqDurationMs;
};

#endif // COMMAND_PARSER_H
