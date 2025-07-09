# SMART SCALE - ĐỒ ÁN HỆ NHÚNG (IT4210)
- Thiết kế chế tạo cân đo trọng lượng của người, hiển thị trên màn hình LED 7 thanh + gửi kết quả về PC qua UART.
Mở rộng: quét thẻ RFID để định danh người cân, lưu trữ và theo dõi cân nặng theo thời gian. Ghép nối RC522 để quét thẻ, và built-in Real-time clock (có pin vào chân VBAT) để giữ thời gian khi mạch mất điện.
## GIỚI THIỆU
__Đề bài__: _Cân sức khỏe._
1. Đo trọng lượng chính xác qua load cell + HX711.
2. Nhận diện người dùng bằng thẻ RFID (RC522).
3. Hiển thị trọng lượng bằng LED 7 đoạn 4 số.
4. Giao tiếp UART với máy tính (Hercules/Serial Monitor) để ghi tên, xem lịch sử, reset dữ liệu.
- Ảnh minh họa:\
  ![Ảnh minh họa](https://github.com/hai9hang/iot_smart_scaler/blob/main/%E1%BA%A2nh%20m%E1%BA%ABu.jpg)

## TÁC GIẢ
- Tên nhóm: TOC
- Thành viên trong nhóm
|STT|Họ tên|MSSV|Công việc|
|--:|--|--|--|
|1|Bùi Xuân Hải|20194268|Thiết kế mạch, kiểm tra phần cứng. Giao tiếp UART, quản lý RFID. Tối ưu code hiển thị LED và logic đo cân.|

## MÔI TRƯỜNG HOẠT ĐỘNG

- **Vi điều khiển sử dụng**: ESP32 DevKitC (CH340)
- **Các module/kết nối phần cứng**:
  - HX711 + Load Cell 20kg
  - RC522 RFID Reader
  - LED 7 đoạn 4 số loại 10 chân chung
  - Nút nhấn TARE
  - Giao tiếp UART qua USB

## SƠ ĐỒ SCHEMATIC

| ESP32 Pin | Thiết bị / Chức năng           |
|-----------|--------------------------------|
| GPIO 15   | HX711 DT                       |
| GPIO 2    | HX711 SCK                      |
| GPIO 18   | RC522 SS (SDA)                 |
| GPIO 4    | RC522 RST                      |
| GPIO 17   | RC522 MOSI                     |
| GPIO 16   | RC522 MISO                     |
| GPIO 5    | RC522 SCK                      |
| GPIO 32-33, 25-27, 14, 12, 13 | LED Segments |
| GPIO 23, 22, 21, 19 | LED Digits           |
| GPIO 34   | Nút nhấn TARE                  |
| 3.3V      | Cấp nguồn cho các module       |
| GND       | Nối đất                        |

### TÍCH HỢP HỆ THỐNG

| Phần cứng | Mô tả/ vai trò |
|-----------|--------------------------------|
| ESP32 DevKitC | Bộ xử lý trung tâm của hệ thống, thực hiện đọc cảm biến, giao tiếp RFID, hiển thị LED và gửi dữ liệu UART. |
| Load Cell + HX711 | Cảm biến đo khối lượng – load cell chuyển đổi lực → điện áp, HX711 khuếch đại và đưa vào ESP32 để đọc số. |
| Module RFID RC522 | Quét và nhận dạng thẻ RFID, gửi UID đến ESP32 để đối chiếu tên người dùng. |
| LED 7 thanh 4 chữ số 5643-BS cực âm chung  | Hiển thị kết quả khối lượng dạng số thực (xx.xx kg). |
| Nút bấm | Cho phép reset cân về 0 thủ công. |
| Máy tính (PC) | 	Giao tiếp UART với ESP32 để nhập lệnh và xem kết quả bằng phần mềm Arduino ( Serial Monitor ) hoặc Hercules. |

| Phần mềm | Mô tả/ vai trò | Chạy trên |
|-----------|--------------------------------|--------------------------------|
| Firmware Arduino | Toàn bộ logic hệ thống: đọc dữ liệu, xử lý trọng lượng, giao tiếp UART, quét RFID, điều khiển LED | ESP32 |
| Serial Monitor ( Arduino ) / Hercules | Giao diện dòng lệnh cho người dùng tương tác với ESP32, xem log | Máy tính |

### ĐẶC TẢ HÀM

- Giải thích một số hàm quan trọng: ý nghĩa của hàm, tham số vào, ra

  ```

     /**
     * Khởi tạo toàn bộ hệ thống:
     * - Serial, I2C, SPI
     * - Khởi tạo cân HX711 và RFID
     * - Khởi tạo chân LED và nút TARE
     */
  void setup();

    /**
     * Cập nhật giá trị hiển thị LED 4 số từ trọng lượng
     * @param weight Trọng lượng dạng float, ví dụ 23.45 (kg), chuyển từ g -> kg.
     */
  void updateDisplayDigits(float weight);

    /**
     * Đọc dữ liệu RFID và ánh xạ sang tên người dùng (nếu có)
     * Nếu thẻ mới → hiển thị UID và tên
     */
  void checkRFID();

    /**
     * Đọc trọng lượng từ HX711 không gây delay hệ thống.
     * Dùng lấy mẫu trung bình và kiểm tra ngưỡng thay đổi.
     */
  void checkWeightNonBlocking();

    /**
     * Nhận và xử lý lệnh UART như WRITE, DELETE, RESET, HISTORY...
     * Định dạng: WRITE <UID> <Tên>
     */
  void handleSerial();

  ```
### THÔNG SỐ KỸ THUẬT

| Thành phần | Thông số          |
|-----------|--------------------------------|
| Độ chính xác   | +- 0.05kg ( 50 gram) |
| Khả năng cân    | 20 kg ( Thử nghiệm 5kg ) |
| Tần suất đo   | 5Hz ( Mỗi 200ms) |
| Thời gian ổn định    | 2 giây ( 10 lần đo mẫu ) |
| Bộ lọc trọng số  | 0.3 |
| Lưu trữ thẻ RFID  | ~100 thẻ (với 5 bản ghi mỗi thẻ) |
| Lịch sử cân    | 500 lần đo |
| UART | 115200 baud |
| COMMAND LIST | WRITE <UID> <Username> - Đặt username theo uid |
|  | DELETE <UID> - Xóa dữ liệu theo uid |
|  | RESET - Xóa toàn bộ dữ liệu|
|  | HISTORY -<UID> - Lịch sử bản ghi|
|  | SETID <Username > - Đặt username khi đang gắn thẻ|
|  | TARE - Căn chỉnh cân|


### KẾT QUẢ

Giao diện: 
- Led 7 đoạn hiển thị với format ab:cd ( Tương đương với ab,cd kilogram. Bằng 0 nếu trọng lượng bé hơn 0).
- Tần suất quét: 5Hz(200 ms).
- Đèn sáng lần lượt.
Chức năng:
- Lưu trữ bản ghi.
- Đo khối lượng với độ chính xác cao.
- Thân thiện với người dùng.
Giao tiếp với UART ( Arduino - Serial Monitor ):
```
06:51:07.272 -> Command: WRITE <UID> <Name>, DELETE <UID>, RESET, HISTORY <UID>, HISTORY, SETID, TARE
06:51:07.272 -> [RFID] UID: 7fdaafbd | Name: Unknown
06:51:08.604 -> [WEIGHT] UID: 7fdaafbd | Weight: 0.00 kg
06:51:14.992 -> [WEIGHT] UID: 7fdaafbd | Weight: 0.23 kg
```
### Video:
![Video 1](https://github.com/hai9hang/iot_smart_scaler/blob/main/Demo.mp4)
![Video 2](https://github.com/hai9hang/iot_smart_scaler/blob/main/Demo1.mp4)
