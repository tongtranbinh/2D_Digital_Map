# Bản Đồ Số 2D Theo Dõi Đối Tượng Mặt Biển Thời Gian Thực

Dự án này là mã nguồn demo hoàn chỉnh cho hệ thống giám sát và theo dõi các đối tượng mặt nước (tàu thuyền) theo thời gian thực trên nền bản đồ số 2D OpenStreetMap. 

Hệ thống được phát triển bằng **C++ / Qt / QML**, lưu trữ không gian thông qua **PostgreSQL / PostGIS** và giả lập luồng truyền tin vị trí qua **TCP Socket**.

---

## 1. Cấu Trúc Thành Phần Hệ Thống
*   **Ứng dụng C++ (Qt/QML)**: Nhận tin telemetry qua TCP, truy vấn PostGIS kiểm tra cảnh báo và cập nhật tức thời lên giao diện bản đồ 2D.
*   **Database (PostgreSQL + PostGIS)**: Lưu vết hành trình tàu (`Point`) và định nghĩa các vùng cấm/vùng nguy hiểm (`Polygon`).
*   **Bộ giả lập (Python Simulator)**: Giả lập chuyển động vật lý của 10-100 tàu biển và truyền tin JSON qua TCP Socket chu kỳ mỗi 1 giây.

---

## 2. Chuẩn Bị Môi Trường
Để chạy được bản demo, bạn cần cài đặt các công cụ sau trên máy tính (Linux/Windows):

1.  **C++ & CMake & Qt**:
    *   Yêu cầu phiên bản Qt5 hoặc Qt6 (Bao gồm các module: `Core`, `Network`, `Sql`, `Quick`, `Location`, `Positioning`).
    *   CMake phiên bản >= 3.16.
    *   Trình biên dịch hỗ trợ C++17 (GCC/Clang trên Linux, MSVC/MinGW trên Windows).
2.  **Cơ sở dữ liệu**:
    *   PostgreSQL (phiên bản >= 12).
    *   Extension PostGIS (phải được cài đặt kèm theo).
3.  **Môi trường chạy Simulator**:
    *   Python 3.x (Không cần cài thêm thư viện ngoài vì chỉ sử dụng các thư viện chuẩn như `socket`, `json`, `math`).

---

## 3. Các Bước Cài Đặt và Chạy Demo

### Bước 1: Thiết lập Database PostgreSQL / PostGIS
1.  Đăng nhập vào PostgreSQL và tạo một database mới tên là `digital_map_db`:
    ```sql
    CREATE DATABASE digital_map_db;
    ```
2.  Chạy script khởi tạo schema để kích hoạt PostGIS và tạo cấu trúc bảng:
    ```bash
    psql -U postgres -d digital_map_db -f database/schema.sql
    ```
3.  Chạy script nạp dữ liệu mẫu (đăng ký tàu và vẽ các vùng cảnh báo đa giác quanh đảo Cát Bà, Vịnh Hạ Long):
    ```bash
    psql -U postgres -d digital_map_db -f database/seed.sql
    ```

### Bước 2: Biên dịch và chạy ứng dụng C++ Qt
1.  Di chuyển vào thư mục dự án và tạo thư mục build:
    ```bash
    mkdir build && cd build
    ```
2.  Cấu hình dự án bằng CMake:
    ```bash
    cmake ..
    ```
3.  Biên dịch dự án:
    ```bash
    cmake --build .
    ```
4.  Chạy chương trình:
    *   Mặc định chương trình sẽ kết nối Postgres bằng thông tin mặc định (`127.0.0.1:5432`, user: `postgres`, pass: `postgres`).
    *   Bạn có thể truyền các tham số tùy chỉnh từ dòng lệnh:
        ```bash
        ./bin/2D_Digital_Map --db-host 127.0.0.1 --db-user postgres --db-pass matkhaucua_ban --db-name digital_map_db --tcp-port 5555
        ```
    *   *Lưu ý*: Giao diện bản đồ sẽ hiện ra ở khu vực biển Hải Phòng và Hạ Long, các vùng cảnh báo đa giác sẽ được vẽ tự động (màu đỏ/cam nhạt).

### Bước 3: Chạy bộ giả lập tàu biển
Trong khi ứng dụng Qt C++ đang chạy, mở một terminal mới và chạy script giả lập Python:
```bash
python simulator/simulator.py --count 20
```
*   Tham số `--count 20` chỉ định số lượng tàu giả lập chạy đồng thời (có thể chỉnh từ 10 đến 100 tàu).
*   Ngay khi chạy simulator, trên bản đồ Qt sẽ xuất hiện các tàu di chuyển thời gian thực mỗi 1 giây.

---

## 4. Kịch Bản Demo Đánh Giá Đề Tài

1.  **Theo dõi thời gian thực**: Quan sát sự di chuyển mượt mà của các tàu (mũi tàu tự xoay theo hướng đi `course`).
2.  **Xem chi tiết đối tượng**: Click chuột vào bất kỳ tàu nào trên bản đồ, một bảng thông tin chi tiết (Slide-in) sẽ hiện ra ở góc phải hiển thị: MMSI, Tên tàu, Vận tốc (Knots), Hướng đi, Tọa độ GPS chính xác và Trạng thái an toàn.
3.  **Cảnh báo xâm nhập vùng cấm (PostGIS ST_Contains)**:
    *   Khi một tàu di chuyển đi vào vùng đa giác màu đỏ (Vùng cấm neo đậu Cát Bà) hoặc màu cam (Vùng biển quân sự).
    *   Hệ thống PostGIS sẽ phát hiện điểm tọa độ tàu nằm trong Polygon.
    *   Trên giao diện bản đồ, tàu vi phạm sẽ lập tức đổi màu sang **Đỏ tươi nhấp nháy** có vòng sóng lan tỏa.
    *   Một thông báo Toast Alert sẽ trượt xuống từ trên cao cảnh báo: `Tàu "VINA LINES 01" đi vào "Vùng Cấm Neo Đậu Cát Bà"`.
    *   Khi tàu di chuyển ra khỏi vùng cảnh báo, trạng thái sẽ tự động chuyển lại màu xanh lá cây an toàn.
