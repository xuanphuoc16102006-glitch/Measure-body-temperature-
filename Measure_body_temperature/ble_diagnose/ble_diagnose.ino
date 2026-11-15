// BLE DIAGNOSTIC SKETCH — Chẩn đoán lỗi E4
// Nap sketch nay len Arduino, mo Serial Monitor (9600 baud)
// va doc ket qua. KHONG can module BLE de chay sketch nay.
#include <SoftwareSerial.h>

#define PIN_BLE_RX  2
#define PIN_BLE_TX  3

SoftwareSerial bleSerial(PIN_BLE_RX, PIN_BLE_TX);

const uint32_t BAUD_LIST[] = {9600, 115200, 38400, 57600, 19200};
const uint8_t  BAUD_COUNT  = 5;

bool waitAndPrint(uint32_t timeoutMs) {
    char buf[64];
    uint8_t n = 0;
    bool foundOK = false;
    unsigned long start = millis();

    while (millis() - start < timeoutMs) {
        if (bleSerial.available()) {
            char c = bleSerial.read();
            if (n < sizeof(buf) - 1) buf[n++] = c;
            if (n >= 2 &&
                (buf[n-2] == 'O' || buf[n-2] == 'o') &&
                (buf[n-1] == 'K' || buf[n-1] == 'k')) {
                foundOK = true;
            }
        }
    }

    buf[n] = '\0';
    if (n > 0) {
        Serial.print(F("  << Nhan duoc ("));
        Serial.print(n);
        Serial.print(F(" bytes): ["));
        for (uint8_t i = 0; i < n; i++) {
            if (buf[i] >= 32 && buf[i] < 127) {
                Serial.print(buf[i]);
            } else {
                Serial.print(F("\\x"));
                if ((uint8_t)buf[i] < 0x10) Serial.print('0');
                Serial.print((uint8_t)buf[i], HEX);
            }
        }
        Serial.println(']');
    } else {
        Serial.println(F("  << Khong nhan duoc gi! (timeout)"));
    }
    return foundOK;
}

bool tryBaud(uint32_t baud) {
    Serial.print(F("\n[Baudrate "));
    Serial.print(baud);
    Serial.println(F("]"));

    bleSerial.begin(baud);
    delay(100);

    while (bleSerial.available()) bleSerial.read();
    delay(50);

    Serial.println(F("  >> Gui: AT"));
    bleSerial.print("AT");
    bool ok1 = waitAndPrint(600);
    if (ok1) {
        Serial.println(F("  [OK] Module tra loi AT!"));
        return true;
    }

    delay(50);
    while (bleSerial.available()) bleSerial.read();

    Serial.println(F("  >> Gui: AT\\r\\n"));
    bleSerial.print("AT\r\n");
    bool ok2 = waitAndPrint(600);
    if (ok2) {
        Serial.println(F("  [OK] Module tra loi AT\\r\\n!"));
        return true;
    }

    return false;
}

void setup() {
    Serial.begin(9600);
    while (!Serial) {}

    Serial.println(F("============================================"));
    Serial.println(F("  BLE DIAGNOSTIC — Chan doan loi E4"));
    Serial.println(F("============================================"));
    Serial.println(F("Chan: D2(RX Arduino) <- TX module BLE"));
    Serial.println(F("Chan: D3(TX Arduino) -> RX module BLE"));
    Serial.println(F("Dang kiem tra...\n"));

    bool found = false;
    uint32_t goodBaud = 0;
    for (uint8_t i = 0; i < BAUD_COUNT; i++) {
        if (tryBaud(BAUD_LIST[i])) {
            found = true;
            goodBaud = BAUD_LIST[i];
            break;
        }
    }

    Serial.println(F("\n============================================"));
    if (found) {
        Serial.println(F(">> KET QUA: MODULE BLE HOAT DONG BINH THUONG!"));
        Serial.print(F(">> Baudrate dung: "));
        Serial.println(goodBaud);

        if (goodBaud != 9600) {
            Serial.println(F(""));
            Serial.println(F("!! LUU Y: Baudrate KHAC 9600!"));
            Serial.println(F("   Mo config.h va sua dong nay:"));
            Serial.print(F("   #define BLE_BAUD_RATE  "));
            Serial.println(goodBaud);
        } else {
            Serial.println(F("   Baudrate 9600 dung, E4 la do phan mem."));
            Serial.println(F("   => Kiem tra: phone co dang ket noi BLE k?"));
            Serial.println(F("      Neu co: ngat BLE tren phone roi reset Arduino."));
        }

        bleSerial.begin(goodBaud);
        delay(50);
        while (bleSerial.available()) bleSerial.read();
        Serial.println(F("\n>> Thu AT+NOTI1..."));
        bleSerial.print("AT+NOTI1\r\n");
        waitAndPrint(600);

        while (bleSerial.available()) bleSerial.read();
        Serial.println(F(">> Thu AT+NAME? (lay ten module)..."));
        bleSerial.print("AT+NAME?\r\n");
        waitAndPrint(600);

        while (bleSerial.available()) bleSerial.read();
        Serial.println(F(">> Thu AT+VERS? (lay firmware version)..."));
        bleSerial.print("AT+VERS?\r\n");
        waitAndPrint(600);

    } else {
        Serial.println(F(">> KET QUA: KHONG NHAN DUOC PHAN HOI TU MODULE BLE!"));
        Serial.println(F("============================================"));
        Serial.println(F("Co the do cac nguyen nhan sau:\n"));
        Serial.println(F("[1] SAI DAY TX/RX (nguyen nhan so 1)"));
        Serial.println(F("    Kiem tra lai:"));
        Serial.println(F("    D2 (Arduino) <-- TXD (module BLE)"));
        Serial.println(F("    D3 (Arduino) --> RXD (module BLE)"));
        Serial.println(F("    KHONG phai TX->TX hoac RX->RX!\n"));
        Serial.println(F("[2] CHUA CAP NGUON cho module BLE"));
        Serial.println(F("    Kiem tra VCC va GND cua module.\n"));
        Serial.println(F("[3] MODULE BLE BI HONG hoac chua duoc han day"));
        Serial.println(F("    Thu dung multimeter do thong mach.\n"));
        Serial.println(F("[4] PHONE DANG KET NOI voi BLE luc nay"));
        Serial.println(F("    Ngat BLE tren phone, roi reset Arduino va thu lai."));
        Serial.println(F("\n=> Neu tat ca tren deu OK ma van loi:"));
        Serial.println(F("   Day la LOI PHAN CUNG, khong phai phan mem."));
    }
    Serial.println(F("============================================"));
}

void loop() {
    if (bleSerial.available()) {
        Serial.print(F("[BLE->Serial]: "));
        while (bleSerial.available()) {
            char c = bleSerial.read();
            if (c >= 32 && c < 127) Serial.print(c);
            else { Serial.print(F("\\x")); Serial.print((uint8_t)c, HEX); }
        }
        Serial.println();
    }
    if (Serial.available()) {
        // Cho phep gui lenh AT thu cong tu Serial Monitor
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        Serial.print(F("[Serial->BLE]: "));
        Serial.println(cmd);
        bleSerial.print(cmd);
        bleSerial.print("\r\n");
        delay(300);
    }
}
