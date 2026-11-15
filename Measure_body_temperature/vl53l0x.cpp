#include "vl53l0x.h"
#include <Wire.h>
#include <avr/pgmspace.h>

VL53L0X::VL53L0X(uint8_t i2cAddr) : _addr(i2cAddr), _stopVar(0) {}

void VL53L0X::writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

void VL53L0X::writeReg16(uint8_t reg, uint16_t val) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    Wire.write((uint8_t)(val >> 8));
    Wire.write((uint8_t)(val & 0xFF));
    Wire.endTransmission();
}

uint8_t VL53L0X::readReg(uint8_t reg) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return 0;
    Wire.requestFrom(_addr, (uint8_t)1);
    return Wire.available() ? Wire.read() : 0;
}

uint16_t VL53L0X::readReg16(uint8_t reg) {
    Wire.beginTransmission(_addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return 0;
    Wire.requestFrom(_addr, (uint8_t)2);
    uint16_t val = Wire.available() ? ((uint16_t)Wire.read() << 8) : 0;
    val |= Wire.available() ? Wire.read() : 0;
    return val;
}

// ST VL53L0X mandatory tuning settings (from ST UM2039 / Pololu library).
static const uint8_t TUNING[] PROGMEM = {
    0xFF,0x01, 0x00,0x00, 0xFF,0x00, 0x09,0x00,
    0x10,0x00, 0x11,0x00, 0x24,0x01, 0x25,0xFF,
    0x75,0x00, 0xFF,0x01, 0x4E,0x2C, 0x48,0x00,
    0x30,0x20, 0xFF,0x00, 0x30,0x09, 0x54,0x00,
    0x31,0x04, 0x32,0x03, 0x40,0x83, 0x46,0x25,
    0x60,0x00, 0x27,0x00, 0x50,0x06, 0x51,0x00,
    0x52,0x96, 0x56,0x08, 0x57,0x30, 0x61,0x00,
    0x62,0x00, 0x64,0x00, 0x65,0x00, 0x66,0xA0,
    0xFF,0x01, 0x22,0x32, 0x47,0x14, 0x49,0xFF,
    0x4A,0x00, 0xFF,0x00, 0x7A,0x0A, 0x7B,0x00,
    0x78,0x21, 0xFF,0x01, 0x23,0x34, 0x42,0x00,
    0x44,0xFF, 0x45,0x26, 0x46,0x05, 0x40,0x40,
    0x0E,0x06, 0x20,0x1A, 0x43,0x40, 0xFF,0x00,
    0x34,0x03, 0x35,0x44, 0xFF,0x01, 0x31,0x04,
    0x4B,0x09, 0x4C,0x05, 0x4D,0x04, 0xFF,0x00,
    0x44,0x00, 0x45,0x20, 0x47,0x08, 0x48,0x28,
    0x67,0x00, 0x70,0x04, 0x71,0x01, 0x72,0xFE,
    0x76,0x00, 0x77,0x00, 0xFF,0x01, 0x0D,0x01,
    0xFF,0x00, 0x80,0x01, 0x01,0xF8, 0xFF,0x01,
    0x8E,0x01, 0x00,0x01, 0xFF,0x00, 0x80,0x00,
};

void VL53L0X::loadTuningSettings() {
    const uint8_t n = (uint8_t)(sizeof(TUNING) / 2);
    for (uint8_t i = 0; i < n; i++) {
        writeReg(pgm_read_byte(&TUNING[i * 2]),
                 pgm_read_byte(&TUNING[i * 2 + 1]));
    }
}

bool VL53L0X::begin() {
    Wire.beginTransmission(_addr);
    if (Wire.endTransmission() != 0) return false;

    if (readReg(0xC0) != 0xEE) return false;

    writeReg(0x89, readReg(0x89) | 0x01);

    writeReg(0x88, 0x00);

    writeReg(0x80, 0x01);
    writeReg(0xFF, 0x01);
    writeReg(0x00, 0x00);
    _stopVar = readReg(0x91);
    writeReg(0x00, 0x01);
    writeReg(0xFF, 0x00);
    writeReg(0x80, 0x00);

    writeReg(0x60, readReg(0x60) | 0x12);

    writeReg16(0x44, 32);

    writeReg(0x01, 0xFF);

    // Pick which reference SPADs (light-sensing cells) to use.
    // Without this the sensor ranges with garbage values / out-of-range.
    if (!setupReferenceSpads()) return false;

    loadTuningSettings();

    writeReg(0x0A, 0x04);
    writeReg(0x84, readReg(0x84) & ~0x10);
    writeReg(0x0B, 0x01);

    writeReg(0x01, 0xE8);

    writeReg(0x01, 0x01);
    if (!singleRefCalibration(0x40)) return false;
    writeReg(0x01, 0x02);
    if (!singleRefCalibration(0x00)) return false;
    writeReg(0x01, 0xE8);

    return true;
}

bool VL53L0X::setupReferenceSpads() {
    writeReg(0x80, 0x01);
    writeReg(0xFF, 0x01);
    writeReg(0x00, 0x00);
    writeReg(0xFF, 0x06);
    writeReg(0x83, readReg(0x83) | 0x04);
    writeReg(0xFF, 0x07);
    writeReg(0x81, 0x01);
    writeReg(0x80, 0x01);
    writeReg(0x94, 0x6B);
    writeReg(0x83, 0x00);
    unsigned long t = millis();
    while (readReg(0x83) == 0x00) {
        if (millis() - t > 100) return false;
    }
    writeReg(0x83, 0x01);
    uint8_t tmp = readReg(0x92);
    uint8_t spadCount = tmp & 0x7F;
    bool    isAperture = (tmp >> 7) & 0x01;
    writeReg(0x81, 0x00);
    writeReg(0xFF, 0x06);
    writeReg(0x83, readReg(0x83) & ~0x04);
    writeReg(0xFF, 0x01);
    writeReg(0x00, 0x01);
    writeReg(0xFF, 0x00);
    writeReg(0x80, 0x00);

    uint8_t map[6];
    for (uint8_t i = 0; i < 6; i++) map[i] = readReg(0xB0 + i);

    writeReg(0xFF, 0x01);
    writeReg(0x4F, 0x00);
    writeReg(0x4E, 0x2C);
    writeReg(0xFF, 0x00);
    writeReg(0xB6, 0xB4);

    uint8_t first = isAperture ? 12 : 0;
    uint8_t enabled = 0;
    for (uint8_t i = 0; i < 48; i++) {
        if (i < first || enabled == spadCount) {
            map[i / 8] &= ~(1 << (i % 8));
        } else if ((map[i / 8] >> (i % 8)) & 0x01) {
            enabled++;
        }
    }
    for (uint8_t i = 0; i < 6; i++) writeReg(0xB0 + i, map[i]);
    return true;
}

bool VL53L0X::singleRefCalibration(uint8_t vhvInitByte) {
    writeReg(0x00, 0x01 | vhvInitByte);
    unsigned long t = millis();
    while ((readReg(0x13) & 0x07) == 0) {
        if (millis() - t > 100) return false;
    }
    writeReg(0x0B, 0x01);
    writeReg(0x00, 0x00);
    return true;
}

bool VL53L0X::readDistanceMm(uint16_t &outMm) {
    writeReg(0x80, 0x01);
    writeReg(0xFF, 0x01);
    writeReg(0x00, 0x00);
    writeReg(0x91, _stopVar);
    writeReg(0x00, 0x01);
    writeReg(0xFF, 0x00);
    writeReg(0x80, 0x00);
    writeReg(0x00, 0x01);

    unsigned long t = millis();
    while (readReg(0x00) & 0x01) {
        if (millis() - t > 50) {
            outMm = OUT_OF_RANGE;
            return false;
        }
    }

    t = millis();
    while ((readReg(0x13) & 0x07) == 0) {
        if (millis() - t > 50) {
            outMm = OUT_OF_RANGE;
            return false;
        }
    }

    uint16_t mm = readReg16(0x1E);

    writeReg(0x0B, 0x01);

    if (mm >= 8190) {
        outMm = OUT_OF_RANGE;
        return false;
    }

    outMm = mm;
    return true;
}