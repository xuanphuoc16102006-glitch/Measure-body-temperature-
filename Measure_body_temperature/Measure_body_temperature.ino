#include <Arduino.h>
#include <Wire.h>
#include <avr/wdt.h>
#include "config.h"
#include "mlx90614.h"
#include "eeprom_store.h"
#include "measurement.h"
#include "indicators.h"
#include "ble_protocol.h"
#include "vl53l0x.h"
#include "sanitizer.h"

static MLX90614  sensor(I2C_ADDR_MLX90614);
static Indicators indicators;
static BleProtocol ble;
static VL53L0X   tof(I2C_ADDR_VL53L0X);
static Sanitizer  san;
static CalibrationData cal;

static volatile bool measureRequested = false;
static unsigned long buttonPressStartMs = 0;
static bool buttonWasDown  = false;
static bool holdHandled    = false;

static unsigned long lastIdlePacketMs = 0;
static float lastTemp    = 0;
static float lastAmbient = 0;
static bool  calMode     = false;
static bool  envLocked   = false;
static bool  bleShown    = false;

static unsigned long lastFaceSampleMs  = 0;
static uint8_t       faceSteadyCount   = 0;
static unsigned long lastFaceTriggerMs = 0;
static bool          tofAvailable      = false;

static SanitizerState lastSanState = SanitizerState::IDLE;

static void showIdleScreen() {
    if (envLocked) {
        char t[10], small[17];
        formatTempC(t, lastAmbient);
        snprintf(small, sizeof(small), "Moi truong %s", t);
        indicators.showScreen("KHOA DO", small, cal.threshold, ble.connected());
    } else if (calMode) {
        indicators.showScreen("CAL", "Hieu chuan", cal.threshold, ble.connected());
    } else {
        indicators.showScreen("SAN SANG", "Nhan nut / lai gan", cal.threshold, ble.connected());
    }
}

// Self-test
//   E1 — MLX90614 not found   (fatal)
//   E2 — OLED not found       (fatal)
//   E3 — EEPROM CRC mismatch  (fatal, defaults saved)
//   E4 — BLE không phản hồi  (non-fatal)
//   E5 — VL53L0X not found    (non-fatal)
static void runSelfTestAndReport(bool oledOk) {
    char codes[24] = "";

    bool e1 = !sensor.begin();
    bool e2 = !oledOk;
    bool e3 = !eeprom_store::load(cal);
    bool e4 = !ble.probe();
    bool e5 = !tof.begin();

    if (e1) strcat(codes, " E1");
    if (e2) strcat(codes, " E2");
    if (e3) { strcat(codes, " E3"); eeprom_store::save(cal); }
    if (e4) strcat(codes, " E4");
    if (e5) strcat(codes, " E5");

    tofAvailable = !e5;

    if (!codes[0]) {
        showIdleScreen();
        return;
    }

    indicators.showResult(ResultClass::ERROR, 0, cal.threshold, false, codes + 1);
}

static void setCalMode(bool on) {
    calMode = on;
    showIdleScreen();
}

static void checkAmbient() {
    float ambient;
    if (!sensor.readAmbientTempC(ambient)) return;
    lastAmbient = ambient;
    bool outOfRange = ambient < AMBIENT_MIN_C || ambient > AMBIENT_MAX_C;
    if (outOfRange != envLocked) {
        envLocked = outOfRange;
        showIdleScreen();
    }
}

static void sendResultToApp(ResultClass res, float bodyTemp) {
    char t[8], th[8], line[48];
    dtostrf(bodyTemp, 1, 1, t);
    dtostrf(cal.threshold, 1, 1, th);
    switch (res) {
        case ResultClass::ALARM:
            snprintf_P(line, sizeof(line), PSTR("!!! CANH BAO SOT: %s C (nguong %s C) !!!"), t, th);
            break;
        case ResultClass::NORMAL:
            snprintf_P(line, sizeof(line), PSTR("Nhiet do: %s C - Binh thuong"), t);
            break;
        case ResultClass::SUSPECT:
            snprintf_P(line, sizeof(line), PSTR("Ket qua chua on dinh, vui long do lai"));
            break;
        default:
            snprintf_P(line, sizeof(line), PSTR("Loi do nhiet do"));
            break;
    }
    ble.sendLine(line);
}

static void measureAndReport() {
    float skin, ambient;
    MeasureStatus status;
    takeMeasurement(sensor, skin, ambient, status);

    if (status == MeasureStatus::ENV_INVALID) {
        lastAmbient = ambient;
        envLocked   = true;
        showIdleScreen();
        return;
    }

    if (calMode && status == MeasureStatus::OK) {
        char big[10], t[10], small[17];
        formatTempC(big, skin);
        formatTempC(t, ambient);
        snprintf(small, sizeof(small), "CAL Ta:%s", t);
        indicators.showScreen(big, small, cal.threshold, ble.connected());
        ble.sendTelemetry(skin, ambient, "RAW");
        return;
    }

    float bodyTemp  = (status == MeasureStatus::OK) ? compensate(skin, ambient, cal) : 0;
    ResultClass res = classify(bodyTemp, cal.threshold, status);
    const char *err = (status == MeasureStatus::SENSOR_ERROR) ? "E1 CAM BIEN"
                                                               : "E5 NGOAI DAI";
    indicators.showResult(res, bodyTemp, cal.threshold, ble.connected(), err);
    sendResultToApp(res, bodyTemp);
    lastTemp = bodyTemp;
}

void setup() {
    MCUSR = 0;
    wdt_disable();

    Wire.begin();
    bool oledOk = indicators.begin();
    ble.begin();
    san.begin();

    // Chờ BLE module và VL53L0X ổn định nguồn/UART
    delay(600);

    pinMode(PIN_BUTTON_FUNCTION, INPUT_PULLUP);
    pinMode(PIN_BUTTON_RESET,    INPUT_PULLUP);

    runSelfTestAndReport(oledOk);

    wdt_enable(WDTO_2S);
}

void loop() {
    wdt_reset();

    if (digitalRead(PIN_BUTTON_RESET) == LOW) {
        wdt_enable(WDTO_15MS);
        for (;;) {}
    }

    bool buttonDown  = (digitalRead(PIN_BUTTON_FUNCTION) == LOW);
    unsigned long heldMs = millis() - buttonPressStartMs;

    if (buttonDown && !buttonWasDown) {
        buttonPressStartMs = millis();
        holdHandled = false;
    } else if (!buttonDown && buttonWasDown) {
        if (!holdHandled && heldMs >= BUTTON_DEBOUNCE_MS)
            measureRequested = true;
    } else if (buttonDown && !holdHandled && heldMs >= BUTTON_HOLD_CALIBRATION_MS) {
        holdHandled = true;
        setCalMode(!calMode);
    }
    buttonWasDown = buttonDown;

    ble.reprobe();
    bool wantCal = calMode;
    float oldThreshold = cal.threshold;
    ble.poll(cal, wantCal);
    if (wantCal != calMode) setCalMode(wantCal);
    else if (cal.threshold != oldThreshold) showIdleScreen();

    if (tofAvailable && !calMode && !envLocked
        && millis() - lastFaceSampleMs >= FACE_SAMPLE_INTERVAL_MS) {

        lastFaceSampleMs = millis();
        uint16_t distMm;
        bool readOk = tof.readDistanceMm(distMm);

        if (readOk && distMm >= FACE_DIST_MIN_MM && distMm <= FACE_DIST_MAX_MM) {
            faceSteadyCount++;
        } else {
            faceSteadyCount = 0;
        }

        if (faceSteadyCount >= FACE_STABLE_SAMPLES
            && !measureRequested
            && millis() - lastFaceTriggerMs >= FACE_LOCKOUT_MS) {

            measureRequested  = true;
            faceSteadyCount   = 0;
            lastFaceTriggerMs = millis();
        }
    }

    san.tick();
    indicators.setSanitizerState(san.state());

    if (san.state() != lastSanState) {
        const char *sanStatus =
            san.state() == SanitizerState::WATER_LOW  ? "LOW"        :
            san.state() == SanitizerState::DISPENSING  ? "DISPENSING" : "OK";
        ble.sendSanitizerStatus(sanStatus);
        lastSanState = san.state();
    }

    if (measureRequested) {
        measureRequested = false;
        if (envLocked) {
            showIdleScreen();
        } else {
            measureAndReport();
        }
        lastIdlePacketMs = millis();
    } else if (millis() - lastIdlePacketMs >= BLE_IDLE_PACKET_MS) {
        checkAmbient();
        lastIdlePacketMs = millis();
    }

    if (ble.connected() != bleShown) {
        bleShown = ble.connected();
        indicators.drawBle(bleShown);
    }

    indicators.tick();
}