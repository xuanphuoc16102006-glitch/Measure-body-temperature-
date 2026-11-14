#include "indicators.h"
#include "config.h"
#include <Wire.h>

void formatTempC(char *out, float tempC) {
    dtostrf(tempC, 1, 1, out);
    strcat(out, "C");
}

bool Indicators::begin() {
    pinMode(PIN_LED_GREEN, OUTPUT);
    pinMode(PIN_LED_RED,   OUTPUT);
    pinMode(PIN_BUZZER,    OUTPUT);

    Wire.beginTransmission(I2C_ADDR_SSD1306);
    bool found = (Wire.endTransmission() == 0);

    // U8x8 would switch the shared bus to 400 kHz; MLX90614 is rated 100 kHz max.
    _oled.setBusClock(100000);
    _oled.begin();
    _oled.setFont(u8x8_font_chroma48medium8_r);
    return found;
}

// ---------------------------------------------------------------------------
// drawBle() — update BLE indicator on row 6 without a full redraw
// ---------------------------------------------------------------------------
void Indicators::drawBle(bool bleConnected) {
    _oled.drawString(0, 6, bleConnected ? "BLE:OK  " : "BLE:--  ");
}

// ---------------------------------------------------------------------------
// drawSanitizerRow() — row 7: sanitizer status overlay
// OLED layout (8 rows x 16 chars at 1x font):
//   0-1  big 2x2 temperature / status
//   3    small status line
//   5    "Thresh:xx.xC"
//   6    "BLE:OK" or "BLE:--"
//   7    sanitizer status  <-- new
// ---------------------------------------------------------------------------
void Indicators::drawSanitizerRow() {
    switch (_sanState) {
        case SanitizerState::WATER_LOW:
            _oled.drawString(0, 7, "SAT KHUAN: HET! ");
            break;
        case SanitizerState::DISPENSING:
            _oled.drawString(0, 7, "DANG PHUN...    ");
            break;
        default:
            _oled.drawString(0, 7, "SAT KHUAN: OK   ");
            break;
    }
}

// ---------------------------------------------------------------------------
// setSanitizerState() — call from loop() when state changes
// ---------------------------------------------------------------------------
void Indicators::setSanitizerState(SanitizerState state) {
    if (state == _sanState) return;
    _sanState = state;
    drawSanitizerRow(); // update row 7 in place, no full redraw needed
}

// ---------------------------------------------------------------------------
// showScreen() — idle / locked / calibration screen
// ---------------------------------------------------------------------------
void Indicators::showScreen(const char *bigLine, const char *smallLine,
                             float threshold, bool bleConnected) {
    char buf[17], t[10];
    _oled.clear();
    _oled.draw2x2String(0, 0, bigLine);
    _oled.drawString(0, 3, smallLine);

    formatTempC(t, threshold);
    snprintf(buf, sizeof(buf), "Thresh:%s", t);
    _oled.drawString(0, 5, buf);
    drawBle(bleConnected);
    drawSanitizerRow();   // row 7

    _patternActive = false;
    digitalWrite(PIN_LED_GREEN, LOW);
    digitalWrite(PIN_LED_RED,   LOW);
    digitalWrite(PIN_BUZZER,    LOW);
}

// ---------------------------------------------------------------------------
// showResult() — temperature measurement result screen
// ---------------------------------------------------------------------------
void Indicators::showResult(ResultClass result, float tempC, float threshold,
                             bool bleConnected, const char *errText) {
    char big[10];
    const char *small;

    switch (result) {
        case ResultClass::NORMAL:
            formatTempC(big, tempC);
            small = "BINH THUONG";
            break;
        case ResultClass::ALARM:
            formatTempC(big, tempC);
            small = "CANH BAO: SOT";
            break;
        case ResultClass::SUSPECT:
            strcpy(big, "--.-C");
            small = "Yeu cau do lai";
            break;
        default: // ERROR
            strcpy(big, "LOI");
            small = errText;
            break;
    }
    showScreen(big, small, threshold, bleConnected);
    // showScreen() already called drawSanitizerRow()

    _activePattern  = result;
    _patternStartMs = millis();
    _patternActive  = true;

    if (result == ResultClass::ALARM) {
        digitalWrite(PIN_BUZZER, HIGH); // PB2 = 1; beep pattern driven in tick()
    } else if (result == ResultClass::ERROR) {
        digitalWrite(PIN_BUZZER, HIGH); // turned off in tick() after 1 s
    }
}

// ---------------------------------------------------------------------------
// tick() — non-blocking LED / buzzer / water-low alert driver
// Call every loop() iteration.
// ---------------------------------------------------------------------------
void Indicators::tick() {
    unsigned long now     = millis();
    unsigned long elapsed = now - _patternStartMs;

    // Water-low buzzer: short double-beep every 4 seconds
    // Runs independently of the temperature result pattern.
    if (_sanState == SanitizerState::WATER_LOW && !_patternActive) {
        unsigned long phase = now % 4000;
        if (phase < 100 || (phase >= 200 && phase < 300)) {
            digitalWrite(PIN_BUZZER, HIGH);
        } else {
            digitalWrite(PIN_BUZZER, LOW);
        }
    }

    if (!_patternActive) return;

    switch (_activePattern) {
        case ResultClass::NORMAL:
            digitalWrite(PIN_LED_GREEN, elapsed < LED_GREEN_ON_MS ? HIGH : LOW);
            if (elapsed >= LED_GREEN_ON_MS) _patternActive = false;
            break;

        case ResultClass::ALARM: {
            digitalWrite(PIN_LED_RED, elapsed < LED_RED_ON_MS ? HIGH : LOW);
            bool beepPhase = (elapsed % (1000 / BUZZER_ALARM_BEEP_HZ))
                             < (500 / BUZZER_ALARM_BEEP_HZ);
            if (elapsed < BUZZER_ALARM_DURATION_MS && beepPhase) {
                digitalWrite(PIN_BUZZER, HIGH);  // PB2 = 1 while over threshold
            } else {
                digitalWrite(PIN_BUZZER, LOW);
            }
            if (elapsed >= LED_RED_ON_MS && elapsed >= BUZZER_ALARM_DURATION_MS)
                _patternActive = false;
            break;
        }

        case ResultClass::SUSPECT:
        case ResultClass::ERROR: {
            // Blink red LED; pattern latches until the next measurement.
            bool blinkOn = (elapsed % LED_BLINK_PERIOD_MS) < (LED_BLINK_PERIOD_MS / 2);
            digitalWrite(PIN_LED_RED, blinkOn ? HIGH : LOW);
            if (_activePattern == ResultClass::ERROR)
                digitalWrite(PIN_BUZZER, elapsed < BUZZER_ERROR_DURATION_MS ? HIGH : LOW);
            break;
        }
    }
}
