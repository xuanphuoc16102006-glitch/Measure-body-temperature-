#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// BLE module dùng Serial phần cứng: D0 (RX) <- TXD module, D1 (TX) -> RXD module.
// Khi nạp code qua USB phải rút tạm dây D0/D1 của module BLE ra.
#define PIN_BUTTON_FUNCTION   8
#define PIN_BUTTON_RESET      9
#define PIN_BUZZER            10
#define PIN_LED_RED           11
#define PIN_LED_GREEN         12

#define I2C_ADDR_MLX90614     0x5A
#define I2C_ADDR_SSD1306      0x3C
#define I2C_ADDR_VL53L0X      0x29

#define BLE_BAUD_RATE         9600
#define BLE_CMD_MAX_LEN       32

#define BLE_PROBE_TIMEOUT_MS        500
#define BLE_PROBE_RETRIES           3

#define BLE_REPROBE_INTERVAL_MS     5000

#define FACE_DIST_MIN_MM            20
#define FACE_DIST_MAX_MM           150
#define FACE_STABLE_SAMPLES          3
#define FACE_SAMPLE_INTERVAL_MS    100
#define FACE_LOCKOUT_MS           5000

#define PIN_HAND_SENSOR               4
#define HAND_DETECT_DEBOUNCE_MS     100

#define PUMP_ON    HIGH
#define PUMP_OFF   LOW

#define WATER_LEVEL_SENSOR_PRESENT    0
#define PIN_WATER_LEVEL               5
#define WATER_LOW_CONFIRM_MS        800

#define PIN_PUMP                      6
#define PUMP_DURATION_MS           1500
#define PUMP_COOLDOWN_MS           2500

#define SAMPLE_COUNT               8
#define SAMPLE_WINDOW_MS            400
#define SAMPLE_INTERVAL_MS          (SAMPLE_WINDOW_MS / SAMPLE_COUNT)
#define TRIMMED_COUNT               6
#define MAX_SAMPLE_SPREAD_C          1.0
#define I2C_RETRY_MAX                3

#define TEMP_RANGE_MIN_C            32.0
#define TEMP_RANGE_MAX_C            42.5
#define TEMP_SUSPECT_LOW_C          35.0

#define THRESHOLD_MIN_C             35.0
#define THRESHOLD_MAX_C             40.0
#define THRESHOLD_STEP_C            0.1
#define THRESHOLD_DEFAULT_C         38.0

#define AMBIENT_MIN_C                16.0
#define AMBIENT_MAX_C                35.0

#define MEASURE_TIMEOUT_MS           1000
#define STARTUP_TIMEOUT_MS           60000
#define BLE_IDLE_PACKET_MS           1000
#define ALARM_LATENCY_MAX_MS         200

#define LED_GREEN_ON_MS               3000
#define LED_RED_ON_MS                 5000
#define LED_BLINK_PERIOD_MS           200
#define BUZZER_ALARM_BEEP_HZ          3
#define BUZZER_ALARM_DURATION_MS      5000
#define BUZZER_ERROR_DURATION_MS      1000

#define BUTTON_DEBOUNCE_MS            50
#define BUTTON_HOLD_CALIBRATION_MS    3000
#define WATCHDOG_TIMEOUT_MS           2000

#define EEPROM_MAGIC                 0xA7
#define EEPROM_ADDR_MAGIC            0
#define EEPROM_ADDR_THRESHOLD        1
#define EEPROM_ADDR_K1               5
#define EEPROM_ADDR_K2               9
#define EEPROM_ADDR_CRC8             13
#define EEPROM_LAYOUT_SIZE           14

// Skin->core compensation: Tbody = Tobj + K1*(Tobj - Tamb) + K2.
// Defaults = no compensation; must be calibrated on real hardware.
#define CALIBRATION_K1_DEFAULT       0.0
#define CALIBRATION_K2_DEFAULT       0.0
#define CALIBRATION_K1_MIN           0.0
#define CALIBRATION_K1_MAX           1.0
#define CALIBRATION_K2_MIN           -5.0
#define CALIBRATION_K2_MAX           5.0

#endif
