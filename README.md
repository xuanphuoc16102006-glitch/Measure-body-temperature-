# Hệ Thống Đo Thân Nhiệt Không Tiếp Xúc & Giám Sát Từ Xa (BLE – Cloud Web)

Hệ thống kết hợp đo nhiệt độ hồng ngoại không tiếp xúc (**MLX90614**), phân loại và cảnh báo kết quả bằng đèn LED/còi, hiển thị thông tin trên màn hình **OLED**, tự động kích hoạt đo bằng cảm biến khoảng cách (**VL53L0X**), điều khiển bơm sát khuẩn tự động và truyền dữ liệu thời gian thực qua **Bluetooth Low Energy (HM-10)** lên **Web Dashboard (GitHub Pages + Firebase Cloud)** để giám sát tại chỗ và từ xa.

🌐 **Link Web Dashboard trực tuyến:** [https://xuanphuoc16102006-glitch.github.io/Measure-body-temperature-/](https://xuanphuoc16102006-glitch.github.io/Measure-body-temperature-/)

---

## 1. Chức năng tổng quát

Thiết bị đo nhiệt độ bằng cảm biến hồng ngoại **MLX90614**, hiển thị kết quả trên màn hình OLED, đồng thời cảnh báo bằng đèn LED và còi khi nhiệt độ vượt ngưỡng. Thiết bị có thể bắt đầu đo bằng nút nhấn hoặc tự kích hoạt khi cảm biến khoảng cách phát hiện người đến gần. Ngoài ra, hệ thống có chức năng điều khiển bơm nước sát khuẩn tự động và truyền dữ liệu qua Bluetooth BLE lên giao diện Web trực tuyến.

## 2. Đo và xử lý nhiệt độ

Cảm biến MLX90614 đọc nhiệt độ vật thể và nhiệt độ môi trường. Trước khi đo, chương trình kiểm tra nhiệt độ môi trường; nếu nằm ngoài khoảng **16–35 °C**, thiết bị khóa chức năng đo và thông báo trạng thái trên OLED.

Mỗi lần đo lấy **8 mẫu trong khoảng 400 ms**. Chương trình sắp xếp các mẫu, loại bỏ giá trị cao nhất và thấp nhất, rồi tính trung bình 6 mẫu còn lại. Nếu độ chênh giữa mẫu cao nhất và thấp nhất vượt **1 °C**, kết quả được xem là chưa ổn định và yêu cầu đo lại.

Nhiệt độ sau đó được hiệu chỉnh theo hệ số hiệu chuẩn lưu trong EEPROM, theo công thức:

`Nhiệt độ hiệu chỉnh = nhiệt độ da + K1 × (nhiệt độ da − nhiệt độ môi trường) + K2`

Kết quả được phân loại thành bình thường, cảnh báo, cần đo lại hoặc lỗi đo. Ngưỡng cảnh báo mặc định là **38 °C** và có thể thay đổi trong khoảng **35–40 °C**.

## 3. Khởi động và tự kiểm tra

Khi bật nguồn, thiết bị kiểm tra các thành phần chính: cảm biến MLX90614, màn hình OLED, dữ liệu EEPROM, module BLE và cảm biến khoảng cách VL53L0X. Nếu phát hiện vấn đề, chương trình hiển thị mã lỗi tương ứng trên màn hình để hỗ trợ kiểm tra phần cứng.

Dữ liệu ngưỡng và hệ số hiệu chuẩn được lưu trong EEPROM kèm mã kiểm tra CRC8. Nếu dữ liệu lưu không hợp lệ, chương trình sử dụng các giá trị mặc định.

## 4. Hiển thị và cảnh báo

Màn hình OLED hiển thị nhiệt độ, trạng thái đo, ngưỡng cảnh báo, trạng thái kết nối BLE và trạng thái bộ phận sát khuẩn. Cách báo bằng đèn và còi được điều khiển trong `indicators.cpp`:

- **LED xanh:** sáng sau kết quả bình thường.
- **LED đỏ và còi:** cảnh báo khi nhiệt độ đạt hoặc vượt ngưỡng.
- **LED đỏ nhấp nháy:** báo kết quả chưa ổn định hoặc xảy ra lỗi đo.

Theo `config.h`, còi được nối **D10 (PB2)**, LED đỏ **D11 (PB3)** và LED xanh **D12 (PB4)**.

## 5. Đo thủ công và đo tự động

Nút FUNCTION dùng để bắt đầu đo bằng thao tác nhấn ngắn. Nhấn giữ nút khoảng **3 giây** để bật hoặc tắt chế độ hiệu chuẩn. Nút RESET khởi động lại thiết bị và chạy lại quá trình tự kiểm tra.

Cảm biến VL53L0X hỗ trợ kích hoạt đo tự động khi phát hiện đối tượng trong phạm vi **2–15 cm** ổn định qua nhiều lần đọc. Sau mỗi lần kích hoạt tự động, hệ thống có thời gian chờ trước khi cho phép kích hoạt lại.

## 6. Điều khiển bộ phận sát khuẩn

Cảm biến hồng ngoại E18-D80NK phát hiện tay đưa vào. Khi tay được phát hiện ổn định, chương trình bật bơm trong khoảng **1,5 giây**, sau đó chờ **2,5 giây** trước chu kỳ kế tiếp. Người dùng phải rút tay ra trước khi hệ thống chấp nhận lần kích hoạt mới.

Chương trình có hỗ trợ cảm biến mức dung dịch XKC-Y25-V để khóa bơm và báo hết dung dịch. Tuy nhiên, trong cấu hình hiện tại, `WATER_LEVEL_SENSOR_PRESENT` đang đặt bằng `0`, tức là tính năng đọc cảm biến mức nước **chưa được bật**. Phần điều khiển này được triển khai trong `sanitizer.cpp`.

## 7. Giao tiếp Bluetooth BLE & Giám sát Web Dashboard (Tại chỗ & Từ xa)

Module BLE kết nối với Arduino qua UART phần cứng (`D0, D1`). Thiết bị gửi nhiệt độ, nhiệt độ môi trường, trạng thái kết quả đo và trạng thái bộ phận sát khuẩn ra bên ngoài, đồng thời hỗ trợ các lệnh xem ngưỡng hiện tại, thay đổi ngưỡng và cập nhật hệ số hiệu chuẩn trực tiếp vào EEPROM.

**Kiến trúc truyền dữ liệu lên Web Dashboard:**

```text
[Cảm biến MLX90614 + VL53L0X + E18-D80NK]
                    │ (I2C / Digital)
                    ▼
         [Vi điều khiển ATmega328P] ──► [OLED SSD1306 / LED / Buzzer / Bơm]
                    │ (UART 9600 bps - D0, D1)
                    ▼
          [Module BLE HM-10 (0xFFE0)]
                    │ (Web Bluetooth API)
                    ▼
   [Trạm thu phát Web (Chrome trên Laptop)]
                    │ (HTTPS REST API)
                    ▼
      [Firebase Realtime Database (Cloud)]
                    │ (Đồng bộ tự động 1.5s)
                    ▼
    [Thiết bị giám sát từ xa (Điện thoại / PC)]
