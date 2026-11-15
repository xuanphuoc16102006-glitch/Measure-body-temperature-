#ifndef INDICATORS_H
#define INDICATORS_H

#include <Arduino.h>
#include <U8x8lib.h>
#include "measurement.h"
#include "sanitizer.h"

// Writes e.g. "37.5C" into out (needs >= 8 bytes). AVR snprintf has no %f.
void formatTempC(char *out, float tempC);

// showResult() and showScreen() latch a non-blocking timed LED/buzzer pattern;
// tick() must be called every loop iteration.
class Indicators {
public:
    bool begin();

    void showResult(ResultClass result, float tempC, float threshold,
                    bool bleConnected, const char *errText);

    void showScreen(const char *bigLine, const char *smallLine,
                    float threshold, bool bleConnected);

    void drawBle(bool bleConnected);

    void setSanitizerState(SanitizerState state);

    void tick();

private:
    U8X8_SSD1306_128X64_NONAME_HW_I2C _oled{U8X8_PIN_NONE};
    unsigned long _patternStartMs  = 0;
    ResultClass   _activePattern   = ResultClass::NORMAL;
    bool          _patternActive   = false;

    SanitizerState _sanState = SanitizerState::IDLE;

    void drawSanitizerRow();
};

#endif
