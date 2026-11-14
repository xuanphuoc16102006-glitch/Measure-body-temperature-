# Hệ Thống Đo Và Giám Sát Thân Nhiệt Trực Tuyến (BLE – Cloud Web)

Hệ thống kết hợp đo nhiệt độ hồng ngoại không tiếp xúc (**MLX90614**), phân loại và cảnh báo kết quả bằng đèn LED/còi, hiển thị thông tin trên màn hình **OLED**, tự động kích hoạt đo bằng cảm biến khoảng cách (**VL53L0X**), điều khiển bơm sát khuẩn tự động và truyền dữ liệu thời gian thực qua **Bluetooth Low Energy (HM-10)** lên **Hệ Thống Đo Và Giám Sát Thân Nhiệt Trực Tuyến (GitHub Pages + Firebase Cloud)** để giám sát tại chỗ và từ xa.

🌐 **Link Web giám sát trực tuyến:** [https://xuanphuoc16102006-glitch.github.io/Measure-body-temperature-/](https://xuanphuoc16102006-glitch.github.io/Measure-body-temperature-/)

---

## 1. Chức năng tổng quát

Thiết bị đo nhiệt độ bằng cảm biến hồng ngoại **MLX90614**, hiển thị kết quả trên màn hình OLED, đồng thời cảnh báo bằng đèn LED và còi khi nhiệt độ vượt ngưỡng. Thiết bị có thể bắt đầu đo bằng nút nhấn hoặc tự kích hoạt khi cảm biến khoảng cách phát hiện người đến gần. Ngoài ra, hệ thống có chức năng điều khiển bơm nước sát khuẩn tự động, định danh người đo và truyền dữ liệu qua Bluetooth BLE lên giao diện Web trực tuyến.

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

Cảm biến hồng ngoại **E18-D80NK** phát hiện tay đưa vào. Khi tay được phát hiện ổn định, chương trình bật bơm trong khoảng **1,5 giây**, sau đó chờ **2,5 giây** trước chu kỳ kế tiếp. Người dùng phải rút tay ra trước khi hệ thống chấp nhận lần kích hoạt mới. Phần điều khiển này được triển khai trong `sanitizer.cpp`.

## 7. Giao tiếp Bluetooth BLE & Giám sát Web trực tuyến

Module BLE kết nối với Arduino qua UART phần cứng (`D0, D1`). Thiết bị gửi nhiệt độ, nhiệt độ môi trường, trạng thái kết quả đo và trạng thái bộ phận sát khuẩn ra bên ngoài, đồng thời hỗ trợ các lệnh xem ngưỡng hiện tại, thay đổi ngưỡng và cập nhật hệ số hiệu chuẩn trực tiếp vào EEPROM.

**Kiến trúc truyền dữ liệu lên hệ thống Web:**

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
   [Trạm thu phát Web (Hệ Thống Đo & Giám Sát)] ◄── [Định danh: Chọn nhanh / Nhập tên / Quét QR]
                    │ (HTTPS REST API - history.json)
                    ▼
      [Firebase Realtime Database (Cloud)]
                    │ (Đồng bộ tự động 1.5s)
                    ▼
    [Thiết bị giám sát từ xa (Điện thoại / PC)]
```

- **Kết nối tại chỗ (Web Bluetooth API):** Mở trang Web bằng trình duyệt **Google Chrome** hoặc **Microsoft Edge** trên máy tính có Bluetooth → bấm **🔗 Kết nối HM-10** để nhận dữ liệu trực tiếp qua giao thức GATT (`Service 0xFFE0`, `Characteristic 0xFFE1`) mà không cần cài App trung gian.

## 8. Định danh sinh viên / người đo trên Web

Trước khi thực hiện phép đo, hệ thống cho phép gán danh tính người đo theo 3 cách trực tiếp trên giao diện Web:

- **Chọn nhanh 1 chạm & Danh sách thả xuống:** Tích hợp sẵn danh sách thành viên nhóm (**Xuân Phước**, **Minh Nhân**, **Gia Phúc**, **Khách vãng lai**) để chọn nhanh chỉ với một lần bấm.
- **Thêm & Lưu danh bạ tùy chỉnh:** Cho phép nhập mới họ tên hoặc MSSV và lưu trực tiếp vào bộ nhớ trình duyệt (`localStorage`) để sử dụng cho các lần sau.
- **Tùy chỉnh ngưỡng cảnh báo sốt động:** Cho phép thay đổi trực tiếp ngưỡng cảnh báo sốt ngay trên giao diện Web để phân loại trạng thái theo thời gian thực.

## 9. Điểm danh tự động bằng mã QR qua Webcam

- **Quét mã QR trực tiếp bằng camera laptop:** Tích hợp thư viện `html5-qrcode` ngay trên trình duyệt mà không cần đầu đọc mã vạch rời.
- **Nhận diện mã QR văn bản tĩnh (Static Text):** Khi đưa mã QR chứa tên hoặc MSSV vào trước camera, hệ thống tự động giải mã, điền tên người đo vào phiên đo hiện tại và phát âm thanh *bíp* xác nhận (tự động lọc bỏ các mã chứa đường link URL để tránh nhận diện nhầm).

## 10. Đồng bộ đám mây đa thiết bị & Xuất báo cáo Excel

- **Đồng bộ toàn bộ nhật ký đo qua Firebase Cloud (`history.json`):** Máy tính nối Bluetooth đóng vai trò trạm phát (`Host Station`), tự động ghép cặp `[Thời gian + Tên người đo + Nhiệt độ + Trạng thái]` đẩy lên **Firebase Realtime Database**. Mọi điện thoại hoặc máy tính khác ở xa khi mở link Web sẽ tự động tải toàn bộ bảng lịch sử đo mới nhất mỗi **1,5 giây** (`Xem từ xa`).
- **Xuất báo cáo thống kê Excel (`.csv`):** Hỗ trợ xuất toàn bộ bảng nhật ký đo ra tệp CSV chuẩn UTF-8 (hiển thị đầy đủ tiếng Việt trên Microsoft Excel) và xóa làm mới nhật ký khi cần.

## 11. Chân kết nối chính

| Chức năng | Chân Arduino |
| :--- | :---: |
| Nút FUNCTION | D8 |
| Nút RESET | D9 |
| Còi | D10 / PB2 |
| LED đỏ | D11 / PB3 |
| LED xanh | D12 / PB4 |
| Cảm biến phát hiện tay | D4 |
| Điều khiển bơm | D6 |
| Giao tiếp BLE | D0, D1 |
| I2C cho MLX90614, OLED và VL53L0X | A4 (SDA), A5 (SCL) |

## 12. Cấu trúc thư mục dự án

```text
Measure-body-temperature-/
├── doan_da_sua/          # Mã nguồn firmware C/C++ (config.h, indicators.cpp, sanitizer.cpp, ...)
├── index.html            # Giao diện Hệ Thống Đo Và Giám Sát Thân Nhiệt Trực Tuyến (BLE + QR + Cloud)
└── README.md             # Tài liệu mô tả chức năng, sơ đồ chân và hướng dẫn vận hành
```
