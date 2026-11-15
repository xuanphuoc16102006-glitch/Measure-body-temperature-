#ifndef VL53L0X_H
#define VL53L0X_H

#include <Arduino.h>

// Returns VL53L0X::OUT_OF_RANGE (0xFFFF) when no target is found or on
// I2C error.  The sensor reports 8190 mm internally for out-of-range
// targets; that value is normalised to OUT_OF_RANGE here.

class VL53L0X {
public:
    static const uint16_t OUT_OF_RANGE = 0xFFFF;

    explicit VL53L0X(uint8_t i2cAddr = 0x29);

    bool begin();

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

#endif
