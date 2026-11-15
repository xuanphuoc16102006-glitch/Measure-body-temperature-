#include "sanitizer.h"
#include "config.h"

void Sanitizer::begin() {
    pinMode(PIN_HAND_SENSOR,  INPUT_PULLUP);

    // Khi không có sensor (WATER_LEVEL_SENSOR_PRESENT=0), giữ INPUT_PULLUP
    // để pin luôn đọc HIGH → _waterLow = false → bơm không bao giờ bị khóa.
#if WATER_LEVEL_SENSOR_PRESENT
    pinMode(PIN_WATER_LEVEL,  INPUT);
#else
    pinMode(PIN_WATER_LEVEL,  INPUT_PULLUP);
#endif

    pinMode(PIN_PUMP, OUTPUT);
    digitalWrite(PIN_PUMP, PUMP_OFF);

    // Lý do quan trọng: Nếu khởi tạo _handRaw = false (giả định không có tay)
    // trong khi sensor đang thực sự đọc LOW (vật cản), thì lần tick() đầu tiên
    // sẽ thấy rawHand != _handRaw → reset debounce timer → _handConfirmed = false
    // → _waitingForRelease bị mở khóa sai → tick tiếp theo bơm bắn ngay.
    _handRaw        = (digitalRead(PIN_HAND_SENSOR) == LOW);
    _handConfirmed  = _handRaw;
    _handDebounceMs = millis();

    _waitingForRelease = true;
}

void Sanitizer::tick() {
    unsigned long now = millis();

#if WATER_LEVEL_SENSOR_PRESENT
    bool rawLow = (digitalRead(PIN_WATER_LEVEL) == LOW);
    if (rawLow != _waterRawLow) {
        _waterRawLow     = rawLow;
        _waterLowStartMs = now;
    }
    if (rawLow && (now - _waterLowStartMs >= WATER_LOW_CONFIRM_MS)) {
        _waterLow = true;
    } else if (!rawLow) {
        _waterLow = false;
    }
#else
    _waterLow = false;
#endif

    switch (_state) {
        case SanitizerState::DISPENSING:
            if (now - _pumpStartMs >= PUMP_DURATION_MS) {
                digitalWrite(PIN_PUMP, PUMP_OFF);
                _state           = SanitizerState::COOLDOWN;
                _cooldownStartMs = now;
                _waitingForRelease = true;
            }
            return;

        case SanitizerState::COOLDOWN:
            if (now - _cooldownStartMs >= PUMP_COOLDOWN_MS) {
                _state = _waterLow ? SanitizerState::WATER_LOW
                                   : SanitizerState::IDLE;
            }
            return;

        case SanitizerState::WATER_LOW:
            if (!_waterLow) _state = SanitizerState::IDLE;
            return;

        case SanitizerState::IDLE:
            if (_waterLow) {
                _state = SanitizerState::WATER_LOW;
                return;
            }
            break;
    }

    bool rawHand = (digitalRead(PIN_HAND_SENSOR) == LOW);

    if (rawHand != _handRaw) {
        _handRaw        = rawHand;
        _handDebounceMs = now;
    }

    if (now - _handDebounceMs >= HAND_DETECT_DEBOUNCE_MS) {
        _handConfirmed = _handRaw;
    }

    if (_waitingForRelease) {
        if (!_handConfirmed) {
            _waitingForRelease = false;
        }
        return;
    }

    if (_handConfirmed && _state == SanitizerState::IDLE) {
        digitalWrite(PIN_PUMP, PUMP_ON);
        _state        = SanitizerState::DISPENSING;
        _pumpStartMs  = now;
        _waitingForRelease = true;
    }
}
