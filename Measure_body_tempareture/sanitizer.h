#ifndef SANITIZER_H
#define SANITIZER_H

#include <Arduino.h>

enum class SanitizerState {
    IDLE,        // waiting for hand
    DISPENSING,  // pump running
    COOLDOWN,    // pump just stopped, waiting before next activation
    WATER_LOW    // sanitizer empty — alerts active, pump locked
};

// Manages the hand-sanitizer subsystem:
//   - Hand detection via E18-D80NK (D4, NPN: LOW = hand present)
//   - Sanitizer pump via IRLZ44N MOSFET (D6, HIGH = pump ON)
//   - Liquid level via XKC-Y25-V (D5, HIGH = liquid present, LOW = empty)
//
// Call begin() in setup() and tick() every loop() iteration.
// tick() is fully non-blocking (uses millis()).

class Sanitizer {
public:
    void begin();
    void tick();

    SanitizerState state()    const { return _state; }
    bool           waterLow() const { return _waterLow; }

private:
    SanitizerState _state = SanitizerState::IDLE;

    // --- Pump / cooldown ---
    unsigned long _pumpStartMs     = 0;
    unsigned long _cooldownStartMs = 0;

    // --- Hand detection state ---
    bool          _handRaw            = false;  // raw digitalRead result
    bool          _handConfirmed      = false;  // debounced state
    unsigned long _handDebounceMs     = 0;      // when raw last changed
    bool          _waitingForRelease  = true;   // bắt buộc nhả tay trước khi nhận lần bơm mới

    // --- Water level ---
    bool          _waterLow           = false;
    bool          _waterRawLow        = false;
    unsigned long _waterLowStartMs    = 0;      // when raw LOW started
};

#endif // SANITIZER_H
