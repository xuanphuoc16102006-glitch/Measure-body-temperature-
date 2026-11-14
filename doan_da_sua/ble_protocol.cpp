#include "ble_protocol.h"
#include "config.h"
#include "crc8.h"

BleProtocol::BleProtocol() : _serial(Serial) {}

void BleProtocol::begin() {
    _serial.begin(BLE_BAUD_RATE);
}

// Reads bytes for up to timeoutMs, returns true as soon as "OK" (case-insensitive)
// is found anywhere in the stream. A sliding 2-byte window handles leading
// \r, \n, or any boot noise the module emits before its real reply.
// Any leftover bytes are flushed so they don't contaminate poll() later.
bool BleProtocol::replyIsOk(uint16_t timeoutMs) {
    char w0 = 0, w1 = 0;
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
        if (_serial.available()) {
            w0 = w1;
            w1 = (char)_serial.read();
            if ((w0 == 'O' || w0 == 'o') && (w1 == 'K' || w1 == 'k')) {
                // Found "OK" — flush any trailing bytes (\r\n) before returning
                delay(10);
                while (_serial.available()) _serial.read();
                return true;
            }
        }
    }
    return false;
}

// Probes the BLE module. Returns true if the module responds to AT commands.
// BLE is a non-critical peripheral: if probe() returns false, the caller
// (runSelfTestAndReport) shows a brief warning then continues normally —
// temperature measurement works without BLE.
//
// IMPORTANT: If the phone is already connected when Arduino boots, the module
// is in transparent mode and will NOT respond to AT. In that case probe()
// returns false (E4) but the module is still working. reprobe() in loop()
// will send AT+NOTI1 as soon as the phone disconnects and reconnects.
//
// Strategy: flush boot noise first, then try both "AT" (HM-10 genuine) and
// "AT\r\n" (AT-09, CC41, BT05, JDY-08 clones) in rotation, up to
// BLE_PROBE_RETRIES times each, with BLE_PROBE_TIMEOUT_MS per attempt.
bool BleProtocol::probe() {
    // Flush boot noise / leftover bytes from a previous reset
    delay(20);
    while (_serial.available()) _serial.read();

    for (uint8_t i = 0; i < BLE_PROBE_RETRIES; i++) {
        // Alternate between bare "AT" and "AT\r\n" to cover all known module types
        if (i % 2 == 0) {
            _serial.print("AT");
        } else {
            _serial.print("AT\r\n");
        }

        if (replyIsOk(BLE_PROBE_TIMEOUT_MS)) {
            // Module alive — enable OK+CONN / OK+LOST notifications (best-effort)
            _serial.print("AT+NOTI1\r\n");
            replyIsOk(BLE_PROBE_TIMEOUT_MS);
            _notiEnabled = true;
            return true;
        }

        // Brief pause before next attempt; flush noise that may have arrived
        delay(50);
        while (_serial.available()) _serial.read();
    }

    return false;
}

// Tries to send AT+NOTI1 if it was never successfully confirmed (e.g. because
// the phone was connected at boot and probe() got E4). Called every loop()
// iteration but only acts every BLE_REPROBE_INTERVAL_MS to avoid flooding.
// Once _notiEnabled is true this function becomes a no-op.
void BleProtocol::reprobe() {
    if (_notiEnabled) return; // already set up — nothing to do

    unsigned long now = millis();
    if (now - _lastReprobeMs < BLE_REPROBE_INTERVAL_MS) return;
    _lastReprobeMs = now;

    // Only send AT+NOTI1 when no data is pending — if the module is in
    // transparent mode the AT command would be forwarded to the phone as data.
    // We detect "command mode" by checking that the serial line is idle.
    if (_serial.available()) return; // data in buffer means module may be in transparent mode

    _serial.print("AT+NOTI1\r\n");
    if (replyIsOk(300)) {
        _notiEnabled = true;
    }
    // Whether it worked or not, flush any reply so poll() stays clean
    delay(10);
    while (_serial.available()) _serial.read();
}

const char *BleProtocol::statusText(ResultClass r) {
    switch (r) {
        case ResultClass::NORMAL:  return "NORMAL";
        case ResultClass::ALARM:   return "ALARM";
        case ResultClass::SUSPECT: return "SUSPECT";
        default:                   return "ERROR";
    }
}

void BleProtocol::sendTelemetry(float tempC, float ambientC, const char *status) {
    char payload[32], t[8], a[8];
    dtostrf(tempC, 1, 1, t);
    dtostrf(ambientC, 1, 1, a);
    snprintf(payload, sizeof(payload), "TEMP,%s,%s,%s", t, a, status);
    uint8_t sum = crc8((const uint8_t *)payload, strlen(payload));
    _serial.print(payload);
    _serial.print(',');
    if (sum < 0x10) _serial.print('0');
    _serial.print(sum, HEX);
    _serial.print("\r\n");
}

void BleProtocol::sendLine(const char *text) {
    _serial.print(text);
    _serial.print("\r\n");
}

void BleProtocol::sendSanitizerStatus(const char *state) {
    char payload[24];
    snprintf(payload, sizeof(payload), "SANIT,%s", state);
    uint8_t sum = crc8((const uint8_t *)payload, strlen(payload));
    _serial.print(payload);
    _serial.print(',');
    if (sum < 0x10) _serial.print('0');
    _serial.print(sum, HEX);
    _serial.print("\r\n");
}

void BleProtocol::poll(CalibrationData &cal, bool &calMode) {
    while (_serial.available()) {
        char c = _serial.read();
        _lastByteMs = millis();
        if (c == '\n') {
            handleLine(cal, calMode);
            _len = 0;
            continue;
        }
        if (c == '\r') continue;
        if (_len < BLE_CMD_MAX_LEN) {
            _buf[_len++] = c;
        } else {
            _len = 0; // overflow: drop and resync on next line
        }
        // HM-10 connection notifications (enabled in probe()) have no CRLF.
        if (_len == 7 && strncmp(_buf, "OK+CONN", 7) == 0) {
            _connected = true;
            _len = 0;
        } else if (_len == 7 && strncmp(_buf, "OK+LOST", 7) == 0) {
            _connected = false;
            _len = 0;
        }
    }
    // Many phone terminal apps send the command without \r\n:
    // treat 100 ms of silence after some bytes as end of command.
    if (_len > 0 && millis() - _lastByteMs > 100) {
        handleLine(cal, calMode);
        _len = 0;
    }
}

void BleProtocol::handleLine(CalibrationData &cal, bool &calMode) {
    _buf[_len] = '\0';
    _connected = true; // a command can only come from a connected phone

    if (strncmp(_buf, "SET_THRESH=", 11) == 0) {
        float v = atof(_buf + 11);
        if (v < THRESHOLD_MIN_C || v > THRESHOLD_MAX_C) {
            _serial.print(F("Loi: nguong phai tu 35.0 den 40.0\r\n"));
            return;
        }
        cal.threshold = round(v * 10) / 10.0f; // spec: 0.1 C steps
        eeprom_store::save(cal);
        char t[8];
        dtostrf(cal.threshold, 1, 1, t);
        _serial.print(F("Da dat nguong sot: "));
        _serial.print(t);
        _serial.print(F(" C\r\n"));

    } else if (strcmp(_buf, "GET_STATUS") == 0) {
        char t[8];
        dtostrf(cal.threshold, 1, 1, t);
        _serial.print(F("Nguong sot hien tai: "));
        _serial.print(t);
        _serial.print(F(" C\r\n"));

    } else if (strncmp(_buf, "CALIBRATE=", 10) == 0) {
        char *comma = strchr(_buf + 10, ',');
        if (!comma) {
            _serial.print("ERR:FORMAT\r\n");
            return;
        }
        float k1 = atof(_buf + 10);
        float k2 = atof(comma + 1);
        if (k1 < CALIBRATION_K1_MIN || k1 > CALIBRATION_K1_MAX ||
            k2 < CALIBRATION_K2_MIN || k2 > CALIBRATION_K2_MAX) {
            _serial.print("ERR:RANGE\r\n");
            return;
        }
        cal.k1 = k1;
        cal.k2 = k2;
        eeprom_store::save(cal);
        _serial.print("OK\r\n");

    } else if (strcmp(_buf, "CAL_MODE=1") == 0 || strcmp(_buf, "CAL_MODE=0") == 0) {
        calMode = _buf[9] == '1';
        _serial.print("OK\r\n");

    } else {
        _serial.print(F("Lenh khong hop le. Vi du: SET_THRESH=38.5\r\n"));
    }
}
