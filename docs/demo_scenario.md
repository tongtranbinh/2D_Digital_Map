# Kịch Bản Demo Hệ Thống - Dự án Ship Tracking (Bản đồ số 2D)

Tài liệu này cung cấp hướng dẫn chi tiết từng bước (Step-by-step) để thực hiện buổi Demo hoặc chạy thử nghiệm toàn bộ hệ thống **Ship Tracking**.

Kịch bản demo bao gồm:
1. **Giai đoạn 1**: Chuẩn bị và khởi động hạ tầng (Docker Compose, PostgreSQL/PostGIS).
2. **Giai đoạn 2**: Nạp dữ liệu mẫu (Seed Data) & Migrate Schema.
3. **Giai đoạn 3**: Build và khởi chạy ứng dụng C++/Qt6 (ShipTracking).
4. **Giai đoạn 4**: Chạy trình mô phỏng gửi dữ liệu hành trình tàu (Telemetry Simulator via TCP).
5. **Giai đoạn 5**: Demo các tính năng trên giao diện người dùng (QML UI).
6. **Giai đoạn 6**: Kiểm chứng dữ liệu lưu trữ bền vững dưới Database (PostgreSQL).
7. **Giai đoạn 7**: Trình bày cơ chế kỹ thuật tối ưu hóa dưới Backend (Multi-threading, RAM Cache COW, Batching DB).

---

## Giai đoạn 1: Khởi động Hạ tầng (Database & OSM Tile Server)

Để hệ thống hoạt động, ta cần khởi động cơ sở dữ liệu PostgreSQL tích hợp PostGIS và máy chủ bản đồ OpenStreetMap (OSM) để hiển thị nền bản đồ.

1. Mở Terminal (PowerShell hoặc Command Prompt) tại thư mục gốc của dự án `2D_Digital_Map`.
2. Khởi chạy Docker Compose:
   ```bash
   docker-compose up -d
   ```
3. Kiểm tra trạng thái các container:
   ```bash
   docker-compose ps
   ```
   *Yêu cầu*: Cả 2 container `ship_tracking_db` (cổng `5432`) và `osm_tile_server` (cổng `8080`) phải ở trạng thái **Up** (Đang chạy).

---

## Giai đoạn 2: Khởi tạo Cấu trúc & Nạp Dữ liệu Mẫu (Database Seeding)

Chúng ta cần tạo các bảng CSDL và thêm các vùng cảnh báo địa lý (Geofence zones) mẫu như **Vùng Cảnh báo Hoàng Sa** hay **Vùng Cấm neo đậu Vịnh Bắc Bộ** để thử nghiệm thuật toán phát hiện xâm nhập.

1. **Khởi tạo schema (Migration)**:
   Nếu khởi chạy Docker lần đầu, các file sql tại `sql/migrations/` đã được tự động chạy. Nếu muốn thực hiện thủ công hoặc chạy lại trên môi trường Windows/Linux, sử dụng script:
   * Trên Linux/macOS:
     ```bash
     ./scripts/migrate.sh
     ```
   * Trên Windows (chạy trực tiếp các file SQL qua công cụ như DBeaver, pgAdmin hoặc qua Command Line):
     Kết nối tới Postgres (`host: localhost, port: 5432, db: ship_tracking, user: ship_user, pass: 123456`) và thực thi tuần tự các file SQL trong thư mục [sql/migrations/](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/sql/migrations/).

2. **Nạp dữ liệu mẫu (Seed Alert Zones & Vessels)**:
   Thực thi tập tin SQL [scripts/seed_db.sql](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/scripts/seed_db.sql) để tạo:
   * **Vùng Cảnh báo Hoàng Sa** (Đa giác bao quanh tọa độ đảo).
   * **Vùng Cấm neo đậu Vịnh Bắc Bộ** (Đa giác kiểm soát an ninh vịnh).
   * 100 Vùng cảnh báo ngẫu nhiên để phục vụ bài test hiệu năng Geofencing.
   * Thông tin định danh của một số tàu mẫu.
   
   *Lệnh thực thi nhanh qua Docker*:
   ```bash
   docker exec -i ship_tracking_db psql -U ship_user -d ship_tracking < scripts/seed_db.sql
   ```

---

## Giai đoạn 3: Biên dịch & Khởi chạy Backend C++ (Qt6 QML UI)

Tiến hành biên dịch dự án bằng CMake và chạy ứng dụng giám sát chính.

1. Tạo thư mục build và chạy cấu hình CMake:
   ```bash
   mkdir build
   cd build
   cmake ..
   ```
2. Thực hiện build chương trình:
   * Trên Windows (MSVC/MinGW):
     ```bash
     cmake --build . --config Release
     ```
   * Trên Linux/macOS:
     ```bash
     make -j4
     ```
3. Khởi chạy file thực thi thu được:
   * Trên Windows:
     ```bash
     .\Release\ShipTracking.exe
     ```
   * Trên Linux/macOS:
     ```bash
     ./ShipTracking
     ```
   
   *Hiện tượng quan sát*: Giao diện ứng dụng Qt6 QML hiển thị bản đồ 2D (lấy nguồn từ `osm-tile-server` hoặc fallback sang OpenStreetMap public). Danh sách "Vùng cảnh báo" bên sidebar hiển thị đầy đủ các vùng đã được tải từ CSDL lên RAM tại thời điểm khởi động.

---

## Giai đoạn 4: Chạy Mô phỏng Gửi Dữ liệu Hành trình (Telemetry Simulator)

Để kiểm chứng tính năng xử lý thời gian thực, ta sẽ chạy một script Python giả lập 1000 tàu biển di chuyển liên tục trên Biển Đông và gửi tọa độ định kỳ qua cổng TCP 9000.

1. Đảm bảo máy tính đã cài đặt Python 3.
2. Mở một terminal mới và chạy file mô phỏng tàu di chuyển:
   ```bash
   python scripts/simulate_tcp_client.py
   ```
   *(Hoặc chạy script `simulate_tcp_client_stationary.py` nếu muốn test kịch bản stress-test 2000 tàu đứng yên)*.
   
3. **Hiện tượng ở Terminal Python**:
   Mỗi 1 giây xuất hiện dòng thông báo:
   `[17:25:01] Đã gửi cập nhật vị trí cho 1000 tàu.`

4. **Hiện tượng ở Terminal của Backend C++**:
   * Mỗi 200 miligiây: Backend xử lý gom cụm vị trí, cập nhật RAM Cache và chạy Geofencing Check cho các tàu.
   * Mỗi 5 giây: Xuất hiện log ghi nhận quá trình Batch Insert dữ liệu vị trí tàu và cập nhật sự kiện cảnh báo xuống PostgreSQL.

---

## Giai đoạn 5: Demo Tính Năng trên Giao diện Người dùng (QML UI)

Đây là phần tương tác trực quan nhất của buổi trình diễn.

### 1. Trực quan hóa Tàu di chuyển thời gian thực
* Trên bản đồ QML, các biểu tượng tàu xuất hiện và di chuyển mượt mà (nhờ tính toán quỹ đạo và cập nhật mỗi 200ms lên RAM Cache hiển thị).
* Người dùng có thể lăn chuột để phóng to/thu nhỏ (Zoom), giữ chuột trái để kéo (Pan) bản đồ xung quanh khu vực Biển Đông.

### 2. Theo dõi các Vùng Cảnh báo (Geofence) và Alert Banner
* Tìm các vùng đa giác màu đỏ mờ trên bản đồ (như Vùng Hoàng Sa hoặc Vịnh Bắc Bộ).
* Khi có tàu đi xuyên qua ranh giới từ ngoài vào trong vùng đỏ:
  * Sự kiện `ENTER` được kích hoạt ngay lập tức.
  * Một thông báo đỏ (Alert Banner) xuất hiện ở góc trên màn hình hiển thị: `"Tàu Tuần Tra... đi VÀO Vùng Cảnh báo Hoàng Sa"`.
* Khi tàu di chuyển ra khỏi vùng cảnh báo:
  * Sự kiện `EXIT` được phát sinh.
  * Banner xuất hiện thông báo: `"Tàu Tuần Tra... đi RA khỏi Vùng Cảnh báo Hoàng Sa"`.

### 3. Tương tác với Sidebar & Danh sách Tàu / Vùng cảnh báo
* **Sidebar bên trái**: Hiển thị danh sách tất cả các tàu đang hoạt động cùng vận tốc và tọa độ hiện tại.
* **Bộ lọc và tìm kiếm**: Nhập MMSI hoặc tên tàu vào ô tìm kiếm để lọc nhanh tàu cần quan sát. Click vào một tàu trong danh sách để tự động di chuyển tâm bản đồ (Focus/Center) đến vị trí của tàu đó và hiển thị bảng chi tiết (Vessel Detail Panel).
* **Danh sách vùng cảnh báo**: Cho phép bật/tắt hiển thị hoặc kích hoạt/vô hiệu hóa giám sát của từng vùng cụ thể.

### 4. Truy vấn lịch sử hành trình (Historical Track)
* Click chọn một tàu trên bản đồ hoặc trong danh sách.
* Nhấn nút **"Xem lịch sử hành trình"** (Show Track History).
* Ứng dụng gửi yêu cầu bất đồng bộ xuống DB Thread (`ShipWorker`) -> DB thực hiện query PostGIS -> trả về danh sách tọa độ lịch sử -> UI vẽ một đường nối (Polyline) biểu diễn quỹ đạo di chuyển từ trước đến nay của tàu đó.

---

## Giai đoạn 6: Kiểm chứng Lưu trữ bền vững dưới CSDL (PostgreSQL)

Để chứng minh dữ liệu telemetry thực sự được đồng bộ bền vững mà không làm nghẽn UI, ta sẽ kết nối trực tiếp vào database để truy vấn.

Mở công cụ quản trị CSDL hoặc chạy lệnh `psql` trong docker:
```bash
docker exec -it ship_tracking_db psql -U ship_user -d ship_tracking
```

Chạy các câu lệnh SQL kiểm tra sau:

1. **Kiểm tra số lượng bản ghi tọa độ nhận được**:
   ```sql
   SELECT COUNT(*) FROM app.positions;
   ```
   *Nhận xét*: Số lượng bản ghi tăng liên tục (cộng thêm 1000 bản ghi mới sau mỗi 5 giây nhờ cơ chế gom Batch Insert).

2. **Truy vấn danh sách sự kiện vi phạm vùng cảnh báo (Geofence Alert Events)**:
   ```sql
   SELECT e.event_time, v.name AS vessel_name, z.name AS zone_name, e.event_type 
   FROM app.alert_events e
   JOIN app.vessels v ON e.vessel_id = v.id
   JOIN app.alert_zones z ON e.zone_id = z.id
   ORDER BY e.event_time DESC 
   LIMIT 10;
   ```
   *Kết quả mong đợi*: Bảng trả về danh sách lịch sử tàu nào đã đi vào (`ENTER`) hoặc đi ra (`EXIT`) vùng nào, kèm mốc thời gian chính xác tới miligiây.

3. **Kiểm tra trạng thái hiện tại của tàu trong vùng**:
   ```sql
   SELECT v.name, z.name, s.inside 
   FROM app.ship_zone_state s
   JOIN app.vessels v ON s.vessel_id = v.id
   JOIN app.alert_zones z ON s.zone_id = z.id
   WHERE s.inside = true;
   ```
   *Kết quả*: Hiển thị danh sách các tàu hiện đang nằm trong các vùng cảnh báo tại thời điểm truy vấn.

---

## Giai đoạn 7: Trình bày Kiến trúc Tối ưu Backend (Tech Stack Highlight)

Trong quá trình demo, hãy nhấn mạnh các điểm thiết kế kỹ thuật xuất sắc sau:

```mermaid
graph LR
    subgraph UI Thread [Luồng Main - UI]
        QML[Giao diện QML/Map] <-->|Signal/Slot| Ctrl[MapController]
    end
    subgraph Pos Thread [Luồng Position Worker]
        Parsing[Parse JSON] -->|200ms| RAM[ShipStateStore COW]
        RAM -->|Ray-Casting| Geofence[Geofencing Check]
    end
    subgraph DB Thread [Luồng Ship Worker]
        Batch[Gom cụm 5s] -->|execBatch| PG[(PostgreSQL)]
    end
    
    Ctrl <==>|Lock-free Reads| RAM
    Pos Thread -->|Signal/Slot 5s| DB Thread
```

1. **Xử lý Đa luồng (Multi-threading)**:
   Giải thích rằng Main Thread (vẽ bản đồ QML) luôn mượt mà ở mức 60 FPS, không bao giờ bị giật khựng (lag/freeze). Lý do là vì tác vụ giải mã JSON thô từ mạng TCP và chạy thuật toán Geofencing được đẩy sang `PositionWorker` thread, còn tác vụ ghi CSDL (vốn chịu ảnh hưởng bởi tốc độ ổ cứng I/O) được xử lý riêng bởi `ShipWorker` thread.

2. **RAM Cache với cơ chế Copy-On-Write (COW) và Lock-free Reads**:
   Khi `MapController` cần lấy dữ liệu vị trí các tàu để vẽ lên bản đồ QML, nó truy cập trực tiếp vào `ShipStateStore`. Lớp này hoạt động hoàn toàn **không cần khóa (Lock-free)** đối với các tiến trình đọc bằng việc sử dụng con trỏ thông minh nguyên tử `std::shared_ptr`.
   Khi luồng ghi cập nhật dữ liệu, nó sẽ nhân bản dữ liệu rồi hoán đổi con trỏ bằng thao tác nguyên tử `atomic_store`. Do đó, UI có thể đọc dữ liệu liên tục với độ trễ cực thấp mà không bao giờ tranh chấp khóa với tiến trình mạng.

3. **Thuật toán Ray-Casting trên RAM**:
   Thay vì thực hiện các hàm tính toán không gian địa lý nặng nề trên cơ sở dữ liệu (ví dụ gọi `ST_Contains` xuống Postgres cho từng tọa độ nhận được), hệ thống chạy thuật toán Ray-Casting thuần túy C++ trên RAM tại `PositionWorker`. Việc này giúp kiểm tra trạng thái nằm trong/ngoài đa giác của hàng ngàn tàu mỗi 200ms chỉ tốn dưới **1ms** CPU.

4. **Gom cụm Ghi CSDL (Batch Insert & Transaction)**:
   Thay vì gửi 1000 câu lệnh `INSERT` xuống CSDL mỗi giây, hệ thống gom dữ liệu vị trí tàu lại và chỉ ghi đĩa định kỳ mỗi 5 giây bằng cách sử dụng **Database Transaction** phối hợp với phương thức **Batch Insert** (`execBatch()`). Điều này làm giảm số lượng kết nối mạng và tận dụng tối đa tốc độ ghi tuần tự của PostgreSQL.
