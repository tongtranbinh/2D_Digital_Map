# Kiến trúc Hệ thống & Workflow - Dự án Ship Tracking

Dự án **Ship Tracking** (Bản đồ số 2D theo dõi đối tượng mặt biển thời gian thực) là một hệ thống backend xây dựng trên nền tảng **C++ (C++17)** kết hợp thư viện **Qt6** và cơ sở dữ liệu quan hệ **PostgreSQL/PostGIS**. Hệ thống được thiết kế để tiếp nhận dữ liệu telemetry từ hàng ngàn tàu biển qua giao thức TCP, lưu trữ hành trình vị trí, thực hiện kiểm tra địa lý (Geofencing) thời gian thực và đồng bộ dữ liệu hiệu năng cao.

---

## 1. Tổng quan Kiến trúc Hệ thống

Hệ thống được thiết kế theo mô hình **Layered Architecture (Kiến trúc phân tầng)** kết hợp với cơ chế **Event-driven (Hướng sự kiện)** thông qua cơ chế Signal/Slot của Qt. Để tối ưu hóa hiệu năng và tránh hiện tượng tắc nghẽn luồng xử lý chính (Main/UI Thread), hệ thống chia công việc thành 3 luồng hoạt động song song độc lập:

1. **Luồng Mạng (Main TCP Server Thread)**: Tiếp nhận các kết nối TCP từ client, đọc dữ liệu thô, tách dòng và đẩy sang hàng đợi xử lý. Luồng này hoàn toàn không thực hiện parse JSON hay thao tác CSDL để đảm bảo tốc độ phản hồi kết nối.
2. **Luồng Xử lý Vị trí & Geofencing (PositionWorker Thread)**: Parse dữ liệu JSON, gom cụm (batching) vị trí để cập nhật vào RAM DB (`ShipStateStore`) mỗi 200ms, đồng thời chạy thuật toán kiểm tra vùng địa lý (Geofencing) để phát hiện sự kiện tàu đi ra/vào vùng cảnh báo.
3. **Luồng Ghi Cơ sở Dữ liệu (ShipWorker Thread)**: Nhận các gói tin vị trí đã được tối giản và các sự kiện cảnh báo định kỳ mỗi 30 giây để thực thi lưu trữ bền vững xuống PostgreSQL thông qua các Transaction và Batch Insert.

```mermaid
graph TD
    Client[TCP Telemetry Client] -->|Gửi JSON telemetry qua TCP| TcpServer[TcpServer - Main Thread]
    
    TcpServer -->|rawMessageReceived| PosWorker[PositionWorker - Thread phụ 1]
    
    subgraph RAM Cache Layer
        PosWorker -->|updatePositions / 200ms| StateStore[(ShipStateStore COW)]
        PosWorker -->|Geofencing Check / 200ms| StateStore
    end
    
    PosWorker -->|pendingPacketsReady / 30s| ShipWorker[ShipWorker - Thread phụ 2]
    
    subgraph DB Layer
        ShipWorker -->|Thực thi Transaction & Batch Insert| Postgres[(PostgreSQL / PostGIS)]
        Postgres -->|Preload Cache khi khởi động| ShipWorker
        ShipWorker -->|Đồng bộ cache ban đầu| StateStore
    end
```

---

## 2. Chi tiết các Phân lớp & Cấu trúc thư mục

Dự án được phân chia thành các thư mục tương ứng với chức năng trong `src/`:

### 2.1. Lớp Mô hình Dữ liệu (Model) - `src/model/`
Chứa các cấu trúc dữ liệu thuần túy (Plain Old Data - POD) để trao đổi thông tin giữa các lớp:
*   [GeoPoint.h](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/model/GeoPoint.h): Định nghĩa tọa độ địa lý gồm `longitude` (kinh độ) và `latitude` (vĩ độ).
*   [Vessel.h](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/model/Vessel.h): Thông tin tĩnh của tàu biển (`id`, `mmsi`, `name`, `callsign`, `imo`, `shipType`).
*   [VesselMessage.h](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/model/VesselMessage.h): Định nghĩa cấu trúc `ShipMessage` chứa thông tin hành trình (`shipId`, `longitude`, `latitude`, `speed`, `course`, `heading`, `timestamp`).
*   [AlertZone.h](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/model/AlertZone.h): Định nghĩa vùng cảnh báo bao gồm `id`, `name`, `description`, trạng thái kích hoạt và một danh sách điểm (`QVector<GeoPoint>`) tạo thành đa giác khép kín.
*   [AlertMessage.h](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/model/AlertMessage.h): Cấu trúc sự kiện cảnh báo `AlertEvent` khi tàu di chuyển qua ranh giới vùng (loại sự kiện: `ENTER` hoặc `EXIT`).

### 2.2. Lớp Truy xuất Dữ liệu (Repository) - `src/database/`
Chịu trách nhiệm giao tiếp trực tiếp với PostgreSQL thông qua module `QtSql`:
*   [PostgresConnection](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/database/PostgresConnection.h): Quản lý vòng đời kết nối CSDL PostgreSQL với các thông tin cấu hình (`PostgresConfig`).
*   [ShipRepository](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/database/ShipRepository.h): Thực hiện các truy vấn lưu trữ tàu biển, kiểm tra sự tồn tại của tàu dựa trên UUID/MMSI (sử dụng truy vấn `INSERT ... ON CONFLICT DO UPDATE`).
*   [PositionRepository](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/database/PositionRepository.h): Thực hiện lưu trữ vị trí của tàu. Hỗ trợ ghi đơn lẻ và ghi hàng loạt (`insertBatch`) sử dụng cơ chế gom tham số dạng mảng của Qt Sql (`execBatch`).
*   [AlertRepository](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/database/AlertRepository.h): Quản lý lưu/truy vấn danh sách vùng cảnh báo (chuyển đổi đa giác từ WKT - Well-Known Text sang PostGIS geography), sự kiện cảnh báo (`alert_events`) và lưu/tải trạng thái tàu nằm trong/ngoài vùng từ bảng `ship_zone_state`.
*   [RepositoryUtils.h](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/database/RepositoryUtils.h): Các hàm tiện ích chuyển đổi dữ liệu UUID và các kiểu hình học (Point, Polygon) giữa C++ và chuỗi định dạng WKT phục vụ cho PostGIS.

### 2.3. Lớp Quản lý Trạng thái Bộ nhớ (State Store) - `src/state/`
*   [ShipStateStore](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/state/ShipStateStore.h): Đây là thành phần quản lý bộ nhớ đệm (RAM Cache) của hệ thống. Nó lưu giữ vị trí mới nhất của tất cả các tàu đang hoạt động, danh sách các vùng cảnh báo và trạng thái tàu trong vùng.
    *   **Cơ chế Lock-free Read & COW (Copy-On-Write)**: Để đảm bảo việc đọc dữ liệu từ RAM phục vụ UI hoặc phân tích luôn nhanh nhất, lớp này sử dụng con trỏ `std::shared_ptr` kết hợp với các thao tác nguyên tử `std::atomic_load` và `std::atomic_store`.
    *   Khi có luồng ghi dữ liệu (ví dụ cập nhật vị trí tàu), hệ thống sẽ lock một mutex ghi (`m_writeMutex`), thực hiện nhân bản (deep copy) cấu trúc dữ liệu hiện tại, thực hiện cập nhật trên bản sao đó, rồi hoán đổi con trỏ nguyên tử thông qua `std::atomic_store`. Luồng đọc không sử dụng khóa giúp loại bỏ hoàn toàn lock contention (tranh chấp khóa) giữa luồng đọc dữ liệu thời gian thực và luồng cập nhật dữ liệu.

### 2.4. Lớp Dịch vụ Nghiệp vụ (Service) - `src/service/`
Lớp trung gian bao bọc các repository để triển khai logic nghiệp vụ:
*   [ShipService](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/service/ShipService.h), [PositionService](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/service/PositionService.h), [AlertService](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/service/AlertService.h).
*   [TrackingService](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/service/TrackingService.h): Điều phối toàn bộ quy trình kiểm tra nghiệp vụ đồng bộ bao gồm kiểm tra tàu tồn tại -> kiểm tra vùng địa lý -> ghi nhận sự kiện cảnh báo -> lưu vị trí cuối cùng. Lớp này được thiết kế theo cấu trúc Interface/Implementation rõ ràng để phục vụ việc viết unit test.

### 2.5. Lớp Xử lý Bất đồng bộ (Worker) - `src/worker/`
Thực thi các tiến trình lặp tuần hoàn hoặc xử lý hàng đợi trên các luồng phụ:
*   [ShipJsonParser](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/worker/ShipJsonParser.h): Parser tiện ích phân tích chuỗi JSON thô từ TCP client, chuyển đổi linh hoạt các trường định danh tàu (chấp nhận cả `shipId`, `id`, `ship_id`) và timestamp (dạng ISO-8601 hoặc số millisecond kỷ nguyên epoch).
*   [PositionWorker](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/worker/PositionWorker.h):
    *   Lắng nghe dữ liệu thô nhận được từ TCP thông qua khe cắm tín hiệu (slot) `processRawMessage`.
    *   Mỗi 200ms (`m_batchTimer` kích hoạt slot `processBatch`), luồng sẽ gom dữ liệu thô đã được phân tích, đẩy đồng loạt cập nhật lên RAM Cache. Sau đó chạy thuật toán Ray-Casting để xác định trạng thái di chuyển của tàu đối với các vùng cảnh báo, phát sinh các sự kiện `ENTER`/`EXIT` nếu có sự thay đổi trạng thái.
    *   Mỗi 30s (`m_timer` kích hoạt slot `flushPendingPackets`), luồng sẽ đẩy toàn bộ danh sách gói tin vị trí (đã được lọc trùng lặp chỉ giữ lại vị trí cuối cùng của mỗi tàu) và sự kiện cảnh báo sang luồng ghi DB thông qua signal `pendingPacketsReady`.
*   [ShipWorker](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/worker/ShipWorker.h): Chạy trên luồng CSDL chuyên biệt. Khi khởi tạo, nó preload dữ liệu từ PostgreSQL lên RAM Cache (`ShipStateStore`). Khi nhận được tín hiệu dữ liệu sẵn sàng từ `PositionWorker`, nó mở một transaction CSDL để lưu trữ hàng loạt gói tin vị trí tàu và cập nhật thông tin sự kiện cảnh báo.

### 2.6. Lớp Mạng (Network) - `src/network/`
*   [TcpServer](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/network/TcpServer.h): Lớp máy chủ mạng sử dụng `QTcpServer`. Lắng nghe các kết nối từ xa. Lớp này quản lý bộ đệm kết nối của từng client (`buffers`) để xử lý các gói tin TCP bị phân mảnh (đọc dữ liệu cho đến khi gặp ký tự xuống dòng `\n`). Khi nhận đủ gói tin, nó phát tín hiệu `rawMessageReceived` để chuyển dữ liệu sang luồng xử lý vị trí.

---

## 3. Quy trình Xử lý Dữ liệu (System Workflows)

### Workflow 1: Khởi động hệ thống (Bootstrapping Workflow)
```mermaid
sequenceDiagram
    participant Main as main()
    participant Store as ShipStateStore (RAM)
    participant Server as TcpServer
    participant SW as ShipWorker (DB Thread)
    participant PG as PostgreSQL
    
    Main->>Store: Khởi tạo rỗng
    Main->>Server: Khởi tạo (config, stateStore)
    Server->>SW: Chuyển sang shipWorkerThread
    Server->>Server: Khởi động posWorkerThread & shipWorkerThread
    SW->>PG: Kết nối CSDL & load dữ liệu vùng cảnh báo
    PG-->>SW: Trả về Alert Zones
    SW->>Store: setAlertZones() & setShipZoneStates() (Đồng bộ vào RAM)
    SW->>PG: Load vị trí mới nhất của tất cả tàu
    PG-->>SW: Trả về list vị trí tàu
    SW->>Store: updatePositions() (Nạp dữ liệu vị trí ban đầu lên RAM)
    Server->>Server: Lắng nghe cổng 9000
```

### Workflow 2: Tiếp nhận và phân tích telemetry thời gian thực
```mermaid
sequenceDiagram
    participant Client as TCP Client
    participant Server as TcpServer (Main Thread)
    participant PW as PositionWorker (Pos Thread)
    participant Store as ShipStateStore (RAM)
    
    Client->>Server: Gửi gói tin JSON kết thúc bằng \n
    Server->>Server: readAll() & phân tích dòng dữ liệu
    Server->>PW: emit rawMessageReceived(oneMessage) (Bất đồng bộ)
    PW->>PW: parseShipJson() chuyển thành ShipMessage
    PW->>PW: insert vào m_incomingBuffer (Buffer 200ms)
    
    Note over PW: Mỗi 200ms (Timer kích hoạt processBatch)
    PW->>Store: updatePositions(batch) (Ghi COW đồng loạt lên RAM)
    PW->>Store: getAlertZones() (Đọc lock-free danh sách vùng cảnh báo)
    PW->>PW: Chạy thuật toán Ray-Casting kiểm tra Geofencing
    PW->>Store: getShipZoneState() & setShipZoneState() (Kiểm tra & Cập nhật trạng thái)
    PW->>PW: Lưu sự kiện ENTER/EXIT vào m_pendingAlertEvents
    PW->>PW: Lưu vị trí cuối cùng của tàu vào m_pendingPackets (Buffer 30s)
```

### Workflow 3: Đồng bộ dữ liệu xuống Cơ sở dữ liệu bền vững (30 giây)
```mermaid
sequenceDiagram
    participant PW as PositionWorker (Pos Thread)
    participant SW as ShipWorker (DB Thread)
    participant PG as PostgreSQL
    
    Note over PW: Mỗi 30s (Timer kích hoạt flushPendingPackets)
    PW->>SW: emit pendingPacketsReady(packets, alertEvents) (Bất đồng bộ)
    PW->>PW: Xóa sạch buffer m_pendingPackets và m_pendingAlertEvents
    
    Note over SW: Nhận được tín hiệu trong luồng CSDL
    SW->>PG: Khởi động Transaction (BEGIN)
    loop Với mỗi tàu trong gói tin
        SW->>SW: Kiểm tra xem tàu đã tồn tại trong DB chưa
        alt Tàu chưa tồn tại
            SW->>PG: INSERT INTO app.vessels (Tạo thông tin tàu mặc định)
        end
    end
    SW->>PG: execBatch() (Batch Insert các vị trí tàu mới)
    loop Với mỗi sự kiện cảnh báo
        SW->>PG: INSERT INTO app.alert_events (Ghi log sự kiện)
        SW->>PG: INSERT INTO app.ship_zone_state ON CONFLICT DO UPDATE (Lưu trạng thái bền vững)
    end
    SW->>PG: Commit Transaction (COMMIT)
    SW-->>PW: Phát tín hiệu hoàn thành (batchProcessed)
```

---

## 4. Các tính năng và thành phần đã hoàn thiện

Dự án hiện đã hoàn thiện các tính năng cốt lõi sau:

1.  **Cơ sở dữ liệu & Migration**:
    *   Cấu hình cơ sở dữ liệu Postgres có hỗ trợ Extension không gian tự nhiên **PostGIS**.
    *   Tập tin Script migration đầy đủ tại [sql/migrations/](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/sql/migrations/) tạo cấu trúc các bảng (`vessels`, `positions`, `alert_zones`, `alert_events`, `ship_zone_state`) với các chỉ mục không gian GIST (`USING GIST (geom)`) giúp tối ưu truy vấn tọa độ địa lý.
    *   Script tự động hóa migration tại [scripts/migrate.sh](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/scripts/migrate.sh).
2.  **Cơ chế lập trình Đa luồng (Multi-threading)**:
    *   Ứng dụng chia rõ ràng nhiệm vụ mạng (TCP Server), tính toán thời gian thực (PositionWorker) và tác vụ I/O đĩa (ShipWorker) chạy trên 3 luồng riêng biệt sử dụng mô hình Signal/Slot và cơ chế hàng đợi truyền tin bất đồng bộ của Qt.
3.  **Hệ thống RAM Cache & Lock-free Reads**:
    *   Thiết kế thành công `ShipStateStore` sử dụng cơ chế Copy-On-Write (COW) kết hợp con trỏ nguyên tử để phục vụ đọc dữ liệu thời gian thực không khóa (Lock-free), đảm bảo thông tin vị trí tàu hoặc vùng cảnh báo luôn sẵn sàng cung cấp cho lớp hiển thị hoặc xử lý mà không bị nghẽn bởi các tiến trình ghi.
4.  **Tối ưu hiệu năng Xử lý Geofencing & Ghi CSDL**:
    *   Triển khai thuật toán Ray-Casting hình học chuyên biệt trong C++ tại [PositionWorker::isPointInPolygon](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/src/worker/PositionWorker.cpp#L141-L162) giúp kiểm tra vị trí tàu trong đa giác cảnh báo vô cùng nhanh chóng trên RAM mà không cần truy vấn không gian nặng nề xuống DB cho mỗi bản tin.
    *   Tối ưu hóa số lượng truy vấn đĩa bằng cách sử dụng cấu trúc **Database Transaction** và **Batch Insert** (`QSqlQuery::execBatch`) gom dữ liệu 30 giây mới ghi đĩa một lần, nâng cao đáng kể băng thông ghi dữ liệu (Throughput) của máy chủ CSDL.
5.  **Cấu trúc mã nguồn & Build System**:
    *   Mã nguồn viết theo chuẩn C++17, cấu trúc phân lớp rõ ràng, áp dụng Dependency Injection.
    *   File cấu hình [CMakeLists.txt](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/CMakeLists.txt) liên kết các module Qt6 cần thiết (`Core`, `Network`, `Widgets`, `Sql`) và tự động kích hoạt `CMAKE_AUTOMOC` xử lý siêu dữ liệu QObject.

---

## 5. Đề xuất & Các bước tiếp theo (Next Steps)

Để hoàn thiện toàn bộ ứng dụng, các bước công việc tiếp theo có thể thực hiện bao gồm:

1.  **Kịch bản Mô phỏng Client (`scripts/simulate_tcp_client.py`)**:
    *   Hoàn thiện mã nguồn Python để mô phỏng việc tạo ra hàng loạt tàu biển ảo di chuyển ngẫu nhiên và gửi dữ liệu tọa độ định kỳ lên cổng TCP 9000 dưới dạng chuỗi JSON thô để kiểm tra sức chịu tải và tính đúng đắn của thuật toán Geofencing.
2.  **Dữ liệu mẫu CSDL (`scripts/seed_db.sql`)**:
    *   Viết mã SQL chèn sẵn các thông tin tàu tĩnh giả lập và đặc biệt là vẽ sẵn một vài vùng đa giác cảnh báo địa lý (ví dụ: Vùng vịnh biển, Vùng cấm neo đậu) nhằm kiểm chứng logic phát sinh sự kiện cảnh báo của hệ thống.
3.  **Lớp hiển thị giao diện người dùng (User Interface)**:
    *   Xây dựng một module UI bản đồ 2D (có thể bằng Qt Widgets / QGraphicsView hoặc một giao diện Dashboard Web liên kết API hiển thị vị trí tàu thực tế) để trực quan hóa hành trình tàu và các cảnh báo Enter/Exit.
4.  **Hệ thống Log và Giám sát hiệu năng**:
    *   Hoàn thiện việc ghi log ra file hoặc tích hợp các công cụ thu thập số liệu (metrics) để theo dõi tốc độ xử lý gói tin trên giây (PPS) và thời gian thực thi ghi CSDL.
