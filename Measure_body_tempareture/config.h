#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Pin mapping (ATmega328P / Arduino Uno-compatible board)
// ---------------------------------------------------------------------------
// I2C (A4=SDA, A5=SCL) shared by MLX90614 and SSD1306 — hardware pins, not
// user-configurable, listed here only for reference.
// PB0/PB1/PB3/PB4 are fixed by the requirements doc (3.1/3.2); the rest
// were moved off those pins to avoid conflicts.
// BLE module dùng Serial phần cứng: D0 (RX) <- TXD module, D1 (TX) -> RXD module.
// Khi nạp code qua USB phải rút tạm dây D0/D1 của module BLE ra.
#define PIN_BUTTON_FUNCTION   8   // PB0, active LOW, internal pull-up
#define PIN_BUTTON_RESET      9   // PB1, active LOW, internal pull-up (soft reset trigger)
#define PIN_BUZZER            10  // PB2
#define PIN_LED_RED           11  // PB3
#define PIN_LED_GREEN         12  // PB4

// ---------------------------------------------------------------------------
// I2C addresses
// ---------------------------------------------------------------------------
#define I2C_ADDR_MLX90614     0x5A
#define I2C_ADDR_SSD1306      0x3C
#define I2C_ADDR_VL53L0X      0x29   // GY-530 ToF distance sensor

// ---------------------------------------------------------------------------
// BLE UART (HM-10 / AT-09 / clone BLE)
// ---------------------------------------------------------------------------
#define BLE_BAUD_RATE         9600
#define BLE_CMD_MAX_LEN       32   // spec 3.1: command strings, max 32 bytes, end CRLF

// BLE probe timeout (ms) per AT attempt. Total max = BLE_PROBE_TIMEOUT_MS * BLE_PROBE_RETRIES.
#define BLE_PROBE_TIMEOUT_MS        500
#define BLE_PROBE_RETRIES           3

// How often reprobe() tries to send AT+NOTI1 when notifications are not yet enabled.
#define BLE_REPROBE_INTERVAL_MS     5000

// ---------------------------------------------------------------------------
// VL53L0X — phát hiện người đứng/đưa tay trước sensor để tự động đo nhiệt độ
// ---------------------------------------------------------------------------
// VL53L0X gắn cùng cụm với MLX90614, hướng ra phía người dùng.
// Khi người đưa trán / cổ tay lại gần (cách ~3-10cm), VL53L0X đo được
// khoảng cách trong vùng MIN–MAX và sau FACE_STABLE_SAMPLES mẫu ổn định
// sẽ tự kích hoạt đo nhiệt độ.
#define FACE_DIST_MIN_MM            20    // mm —  2 cm (sát nhất chấp nhận)
#define FACE_DIST_MAX_MM           150    // mm — 15 cm (xa nhất chấp nhận)
#define FACE_STABLE_SAMPLES          3    // 3 × 100ms = 300ms ổn định → đo
#define FACE_SAMPLE_INTERVAL_MS    100    // ms — tần suất quét VL53L0X
#define FACE_LOCKOUT_MS           5000    // ms — khóa 5s sau mỗi lần đo tự động

// ---------------------------------------------------------------------------
// Hand sanitizer subsystem
// ---------------------------------------------------------------------------
// E18-D80NK (NPN reflective IR): D4, active LOW (có vật cản = LOW / 0)
#define PIN_HAND_SENSOR               4
#define HAND_DETECT_DEBOUNCE_MS     100   // ms — lọc rung tín hiệu

// Mức kích hoạt bơm (IRLZ44N MOSFET gate: HIGH = bật bơm, LOW = tắt bơm)
// Nếu dùng Module Relay kích LOW thì đổi PUMP_ON thành LOW và PUMP_OFF thành HIGH.
#define PUMP_ON    HIGH   // MOSFET gate HIGH = bơm ON
#define PUMP_OFF   LOW    // MOSFET gate LOW  = bơm OFF

// XKC-Y25-V (non-contact capacitive level): D5, HIGH = liquid present
// Nếu không dùng sensor nước, đặt WATER_LEVEL_SENSOR_PRESENT thành 0
// để bỏ qua hoàn toàn chân D5 (tránh floating pin gây lỗi).
#define WATER_LEVEL_SENSOR_PRESENT    0   // 0 = không có, 1 = có cắm sensor nước
#define PIN_WATER_LEVEL               5
#define WATER_LOW_CONFIRM_MS        800   // ms of continuous LOW before alert

// Chân điều khiển bơm (D6)
#define PIN_PUMP                      6
#define PUMP_DURATION_MS           1500   // ms the pump runs per hand detection
#define PUMP_COOLDOWN_MS           2500   // ms before the pump can activate again

// ---------------------------------------------------------------------------
// Sampling / filtering (spec section 5)
// ---------------------------------------------------------------------------
#define SAMPLE_COUNT               8      // 8 samples in 400 ms
#define SAMPLE_WINDOW_MS            400
#define SAMPLE_INTERVAL_MS          (SAMPLE_WINDOW_MS / SAMPLE_COUNT) // 50 ms
#define TRIMMED_COUNT               6      // after dropping min & max
#define MAX_SAMPLE_SPREAD_C          1.0   // reject if (max-min) > 1.0 C
#define I2C_RETRY_MAX                3      // re-reads on PEC failure (1 + 3 attempts)

// ---------------------------------------------------------------------------
// Measurement classification (spec section 6.2)
// ---------------------------------------------------------------------------
#define TEMP_RANGE_MIN_C            32.0
#define TEMP_RANGE_MAX_C            42.5
#define TEMP_SUSPECT_LOW_C          35.0    // T < 35.0 => suspect / re-measure

#define THRESHOLD_MIN_C             35.0
#define THRESHOLD_MAX_C             40.0
#define THRESHOLD_STEP_C            0.1
#define THRESHOLD_DEFAULT_C         38.0

// ---------------------------------------------------------------------------
// Environment validity (spec section 5)
// ---------------------------------------------------------------------------
#define AMBIENT_MIN_C                16.0
#define AMBIENT_MAX_C                35.0

// ---------------------------------------------------------------------------
// Timing (spec section 6.1)
// ---------------------------------------------------------------------------
#define MEASURE_TIMEOUT_MS           1000   // whole measurement must finish within 1 s
#define STARTUP_TIMEOUT_MS           60000  // boot + thermal settle budget
#define BLE_IDLE_PACKET_MS           1000   // 1 packet/second while idle
#define ALARM_LATENCY_MAX_MS         200

#define LED_GREEN_ON_MS               3000
#define LED_RED_ON_MS                 5000
#define LED_BLINK_PERIOD_MS           200    // error / suspect blink period
#define BUZZER_ALARM_BEEP_HZ          3      // 3 beeps/second
#define BUZZER_ALARM_DURATION_MS      5000
#define BUZZER_ERROR_DURATION_MS      1000

#define BUTTON_DEBOUNCE_MS            50     // shorter presses are contact bounce
#define BUTTON_HOLD_CALIBRATION_MS    3000   // hold 3s => calibration mode
#define WATCHDOG_TIMEOUT_MS           2000   // main loop stall => reset

// ---------------------------------------------------------------------------
// EEPROM layout (with CRC8 protection, spec section 3.1/5)
// ---------------------------------------------------------------------------
#define EEPROM_MAGIC                 0xA7   // bumped: default threshold -> 38.0
#define EEPROM_ADDR_MAGIC            0
#define EEPROM_ADDR_THRESHOLD        1   // float, 4 bytes
#define EEPROM_ADDR_K1               5   // float, 4 bytes (calibration gain)
#define EEPROM_ADDR_K2               9   // float, 4 bytes (calibration offset)
#define EEPROM_ADDR_CRC8             13  // 1 byte
#define EEPROM_LAYOUT_SIZE           14

// Skin->core compensation: Tbody = Tobj + K1*(Tobj - Tamb) + K2.
// Defaults = no compensation; must be calibrated on real hardware.
#define CALIBRATION_K1_DEFAULT       0.0
#define CALIBRATION_K2_DEFAULT       0.0
#define CALIBRATION_K1_MIN           0.0
#define CALIBRATION_K1_MAX           1.0
#define CALIBRATION_K2_MIN           -5.0
#define CALIBRATION_K2_MAX           5.0

#endif // CONFIG_H
