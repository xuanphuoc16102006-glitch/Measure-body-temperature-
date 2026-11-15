#ifndef SANITIZER_H
#define SANITIZER_H

#include <Arduino.h>

enum class SanitizerState {
    IDLE,
    DISPENSING,
    COOLDOWN,
    WATER_LOW
};

//   - Hand detection via E18-D80NK (D4, NPN: LOW = hand present)
//   - Sanitizer pump via IRLZ44N MOSFET (D6, HIGH = pump ON)
//   - Liquid level via XKC-Y25-V (D5, HIGH = liquid present, LOW = empty)

class Sanitizer {
public:
    void begin();
    void tick();

    SanitizerState state()    const { return _state; }
    bool           waterLow() const { return _waterLow; }

private:
    SanitizerState _state = SanitizerState::IDLE;

    unsigned long _pumpStartMs     = 0;
    unsigned long _cooldownStartMs = 0;

    bool          _handRaw            = false;
    bool          _handConfirmed      = false;
    unsigned long _handDebounceMs     = 0;
    bool          _waitingForRelease  = true;   // bắt buộc nhả tay trước khi nhận lần bơm mới

    bool          _waterLow           = false;
    bool          _waterRawLow        = false;
    unsigned long _waterLowStartMs    = 0;
};

#endif