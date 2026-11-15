#include "sanitizer.h"
#include "config.h"

void Sanitizer::begin() {
    // E18-D80NK: NPN open-collector, LOW = có vật cản.
    pinMode(PIN_HAND_SENSOR,  INPUT_PULLUP);

    // Chân mức nước: chỉ cấu hình khi có cắm sensor thực tế.
    // Khi không có sensor (WATER_LEVEL_SENSOR_PRESENT=0), giữ INPUT_PULLUP
    // để pin luôn đọc HIGH → _waterLow = false → bơm không bao giờ bị khóa.
#if WATER_LEVEL_SENSOR_PRESENT
    pinMode(PIN_WATER_LEVEL,  INPUT);     // XKC-Y25-V tự drive pin (push-pull)
#else
    pinMode(PIN_WATER_LEVEL,  INPUT_PULLUP);
#endif

    // Tắt bơm ngay trước mọi thứ khác
    pinMode(PIN_PUMP, OUTPUT);
    digitalWrite(PIN_PUMP, PUMP_OFF);

    // -----------------------------------------------------------------------
    // ĐỌC trạng thái sensor THỰC TẾ ngay lúc này.
    //
    // Lý do quan trọng: Nếu khởi tạo _handRaw = false (giả định không có tay)
    // trong khi sensor đang thực sự đọc LOW (vật cản), thì lần tick() đầu tiên
    // sẽ thấy rawHand != _handRaw → reset debounce timer → _handConfirmed = false
    // → _waitingForRelease bị mở khóa sai → tick tiếp theo bơm bắn ngay.
    // -----------------------------------------------------------------------
    _handRaw        = (digitalRead(PIN_HAND_SENSOR) == LOW);
    _handConfirmed  = _handRaw;   // coi trạng thái hiện tại là đã xác nhận
    _handDebounceMs = millis();   // debounce timer tính từ bây giờ

    // Luôn khởi động ở chế độ chờ nhả tay.
    // Nếu _handConfirmed = false (không có vật) → tick() đầu giải phóng ngay.
    // Nếu _handConfirmed = true  (đang có vật)  → phải chờ vật rời đi thì mới bơm.
    _waitingForRelease = true;
}

void Sanitizer::tick() {
    unsigned long now = millis();

    // -----------------------------------------------------------------------
    // 1. Water-level (XKC-Y25-V) — chỉ kiểm tra khi có cắm sensor thực tế
    // -----------------------------------------------------------------------
#if WATER_LEVEL_SENSOR_PRESENT
    bool rawLow = (digitalRead(PIN_WATER_LEVEL) == LOW);
    if (rawLow != _waterRawLow) {
        _waterRawLow     = rawLow;
        _waterLowStartMs = now;
    }
    if (rawLow && (now - _waterLowStartMs >= WATER_LOW_CONFIRM_MS)) {
        _waterLow = true;
    } else if (!rawLow) {
        _waterLow = false;
    }
#else
    _waterLow = false;  // không có sensor → luôn coi là còn nước
#endif

    // -----------------------------------------------------------------------
    // 2. Pump state machine
    // -----------------------------------------------------------------------
    switch (_state) {
        case SanitizerState::DISPENSING:
            if (now - _pumpStartMs >= PUMP_DURATION_MS) {
                digitalWrite(PIN_PUMP, PUMP_OFF); // Tắt bơm
                _state           = SanitizerState::COOLDOWN;
                _cooldownStartMs = now;
                _waitingForRelease = true; // Xịt xong bắt buộc phải rút tay ra
            }
            return; // skip hand detection while dispensing

        case SanitizerState::COOLDOWN:
            if (now - _cooldownStartMs >= PUMP_COOLDOWN_MS) {
                _state = _waterLow ? SanitizerState::WATER_LOW
                                   : SanitizerState::IDLE;
            }
            return;

        case SanitizerState::WATER_LOW:
            if (!_waterLow) _state = SanitizerState::IDLE;
            return; // do not dispense when empty

        case SanitizerState::IDLE:
            if (_waterLow) {
                _state = SanitizerState::WATER_LOW;
                return;
            }
            break;
    }

    // -----------------------------------------------------------------------
    // 3. Hand detection (E18-D80NK: có vật cản = LOW)
    // -----------------------------------------------------------------------
    bool rawHand = (digitalRead(PIN_HAND_SENSOR) == LOW);

    // Debounce tín hiệu cảm biến
    if (rawHand != _handRaw) {
        _handRaw        = rawHand;
        _handDebounceMs = now;
    }

    if (now - _handDebounceMs >= HAND_DETECT_DEBOUNCE_MS) {
        _handConfirmed = _handRaw;
    }

    // Nếu đang ở trạng thái bắt buộc nhả tay (lúc mới boot hoặc vừa xịt xong):
    if (_waitingForRelease) {
        // Chỉ khi cảm biến ghi nhận KHÔNG CÓ TAY (HIGH) ổn định thì mới mở khóa
        if (!_handConfirmed) {
            _waitingForRelease = false;
        }
        return; // Đang chờ nhả tay thì không kích bơm
    }

    // Khi đã sẵn sàng và phát hiện đưa tay vào:
    if (_handConfirmed && _state == SanitizerState::IDLE) {
        digitalWrite(PIN_PUMP, PUMP_ON); // Bật bơm
        _state        = SanitizerState::DISPENSING;
        _pumpStartMs  = now;
        _waitingForRelease = true; // Yêu cầu rút tay ra sau khi bơm xong
    }
}
