#ifndef INDICATORS_H
#define INDICATORS_H

#include <Arduino.h>
#include <U8x8lib.h>
#include "measurement.h"
#include "sanitizer.h"

// Writes e.g. "37.5C" into out (needs >= 8 bytes). AVR snprintf has no %f.
void formatTempC(char *out, float tempC);

// Owns LED / buzzer / OLED.
// showResult() and showScreen() latch a non-blocking timed LED/buzzer pattern;
// tick() must be called every loop iteration.
//
// Sanitizer status is fed in via setSanitizerState() and is overlaid on the
// bottom row of every screen without changing the existing call-sites.
class Indicators {
public:
    // Returns false if the OLED does not answer on I2C (self-test item E2).
    bool begin();

    // errText is the small line shown for ResultClass::ERROR.
    void showResult(ResultClass result, float tempC, float threshold,
                    bool bleConnected, const char *errText);

    // Plain screen (ready / locked / calibration); stops any LED/buzzer pattern.
    void showScreen(const char *bigLine, const char *smallLine,
                    float threshold, bool bleConnected);

    // Update only the BLE indicator on row 6 (avoids a full redraw).
    void drawBle(bool bleConnected);

    // Call from loop() when the sanitizer state changes.
    // The new state is overlaid on the bottom row of the current screen.
    void setSanitizerState(SanitizerState state);

    // Non-blocking LED / buzzer pattern driver — call every loop().
    void tick();

private:
    U8X8_SSD1306_128X64_NONAME_HW_I2C _oled{U8X8_PIN_NONE};
    unsigned long _patternStartMs  = 0;
    ResultClass   _activePattern   = ResultClass::NORMAL;
    bool          _patternActive   = false;

    SanitizerState _sanState = SanitizerState::IDLE;

    // Draw the sanitizer status line (row 7) based on _sanState.
    void drawSanitizerRow();
};

#endif // INDICATORS_H
