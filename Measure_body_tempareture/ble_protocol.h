#ifndef BLE_PROTOCOL_H
#define BLE_PROTOCOL_H

#include <Arduino.h>
#include "config.h"
#include "eeprom_store.h"
#include "measurement.h"

// Command protocol is NOT specified in the requirements doc (no command
// table given) — this is a proposed minimal ASCII line protocol for the
// starter build, easy to swap later once the app side is defined:
//   SET_THRESH=37.8\r\n     -> OK\r\n | ERR:<reason>\r\n
//   GET_STATUS\r\n          -> OK\r\n (status is pushed via TEMP,... packets)
//   CALIBRATE=<k1>,<k2>\r\n -> OK\r\n | ERR:<reason>\r\n
//   CAL_MODE=1 / CAL_MODE=0 -> OK\r\n (enter / leave calibration mode)
// Outgoing telemetry: TEMP,<value>,<ambient>,<statusText>,<crc8-hex>\r\n
class BleProtocol {
public:
    BleProtocol();
    void begin();

    // Self-test: probes the module with AT commands.
    // Returns true if module responds. If the module is already connected to
    // a phone at boot time it won't answer AT — probe() returns false (E4)
    // but the module is still functional; reprobe() in loop() will recover.
    bool probe();

    // Called from loop() to re-send AT+NOTI1 if probe() failed at boot
    // (e.g. phone was already connected). Safe to call every loop iteration —
    // it only acts once every BLE_REPROBE_INTERVAL_MS and only when needed.
    void reprobe();

    // Reads any pending bytes, executes a complete command line if one
    // arrived. Mutates cal and persists to EEPROM on SET_THRESH/CALIBRATE,
    // sets calMode on CAL_MODE.
    void poll(CalibrationData &cal, bool &calMode);

    bool connected() const { return _connected; }

    // Sends one telemetry packet now (new result or idle heartbeat).
    void sendTelemetry(float tempC, float ambientC, const char *status);

    // Sends a sanitizer-status packet: SANIT,<state>,<crc8>\r\n
    // state: "OK" | "DISPENSING" | "LOW"
    void sendSanitizerStatus(const char *state);

    // Sends one human-readable line (for terminal apps), adds \r\n.
    void sendLine(const char *text);

    static const char *statusText(ResultClass r);

private:
    HardwareSerial &_serial;             // BLE module is wired to D0/D1
    char _buf[BLE_CMD_MAX_LEN + 1];
    uint8_t _len = 0;
    bool _connected = false;
    bool _notiEnabled = false;           // true after AT+NOTI1 confirmed
    unsigned long _lastReprobeMs = 0;    // throttle reprobe() attempts
    unsigned long _lastByteMs = 0;       // for commands sent without newline

    bool replyIsOk(uint16_t timeoutMs);
    void handleLine(CalibrationData &cal, bool &calMode);
};

#endif // BLE_PROTOCOL_H
