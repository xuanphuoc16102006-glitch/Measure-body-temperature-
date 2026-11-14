#ifndef VL53L0X_H
#define VL53L0X_H

#include <Arduino.h>

// Minimal VL53L0X Time-of-Flight distance sensor driver for ATmega328P.
// Uses the shared I2C bus (Wire); address is fixed at 0x29 on GY-530 boards.
//
// Measurement cycle: single-shot, ~33 ms blocking.
// Returns VL53L0X::OUT_OF_RANGE (0xFFFF) when no target is found or on
// I2C error.  The sensor reports 8190 mm internally for out-of-range
// targets; that value is normalised to OUT_OF_RANGE here.
//
// Memory note: the 160-byte tuning table lives in Flash (PROGMEM) so it
// does not consume any of the 2 KB SRAM on ATmega328P.

class VL53L0X {
public:
    static const uint16_t OUT_OF_RANGE = 0xFFFF;

    explicit VL53L0X(uint8_t i2cAddr = 0x29);

    // Call once in setup() after Wire.begin().
    // Returns false if the sensor is not found on the bus.
    bool begin();

    // Single-shot ranging.  Blocks for up to ~50 ms.
    // outMm: distance in mm on success, OUT_OF_RANGE on failure.
    // Returns true on success.
    bool readDistanceMm(uint16_t &outMm);

private:
    uint8_t _addr;
    uint8_t _stopVar;   // undocumented register saved during init

    void     writeReg(uint8_t reg, uint8_t val);
    void     writeReg16(uint8_t reg, uint16_t val);
    uint8_t  readReg(uint8_t reg);
    uint16_t readReg16(uint8_t reg);
    void     loadTuningSettings();
    bool     setupReferenceSpads();
    bool     singleRefCalibration(uint8_t vhvInitByte);
};

#endif // VL53L0X_H
