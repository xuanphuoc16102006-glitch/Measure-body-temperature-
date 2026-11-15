#ifndef BLE_PROTOCOL_H
#define BLE_PROTOCOL_H

#include <Arduino.h>
#include "config.h"
#include "eeprom_store.h"
#include "measurement.h"

//   SET_THRESH=37.8\r\n     -> OK\r\n | ERR:<reason>\r\n
//   GET_STATUS\r\n          -> OK\r\n (status is pushed via TEMP,... packets)
//   CALIBRATE=<k1>,<k2>\r\n -> OK\r\n | ERR:<reason>\r\n
//   CAL_MODE=1 / CAL_MODE=0 -> OK\r\n (enter / leave calibration mode)
// Outgoing telemetry: TEMP,<value>,<ambient>,<statusText>,<crc8-hex>\r\n
class BleProtocol {
public:
    BleProtocol();
    void begin();

    // Returns true if module responds. If the module is already connected to
    // a phone at boot time it won't answer AT — probe() returns false (E4)
    // but the module is still functional; reprobe() in loop() will recover.
    bool probe();

    void reprobe();

    void poll(CalibrationData &cal, bool &calMode);

    bool connected() const { return _connected; }

    void sendTelemetry(float tempC, float ambientC, const char *status);

    void sendSanitizerStatus(const char *state);

    void sendLine(const char *text);

    static const char *statusText(ResultClass r);

private:
    HardwareSerial &_serial;
    char _buf[BLE_CMD_MAX_LEN + 1];
    uint8_t _len = 0;
    bool _connected = false;
    bool _notiEnabled = false;
    unsigned long _lastReprobeMs = 0;
    unsigned long _lastByteMs = 0;

    bool replyIsOk(uint16_t timeoutMs);
    void handleLine(CalibrationData &cal, bool &calMode);
};

#endif