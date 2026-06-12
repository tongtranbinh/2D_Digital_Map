# 2D_Digital_Map

Bản đồ số 2D theo dõi đối tượng mặt biển thời gian thực (Ship Tracking System).

Hệ thống được phát triển bằng ngôn ngữ **C++ (C++17)**, framework **Qt6** (Core, Network, Sql, Widgets) và cơ sở dữ liệu **PostgreSQL/PostGIS**. Dự án được xây dựng với mục tiêu xử lý telemetries thời gian thực hiệu năng cao, hỗ trợ đa luồng xử lý bất đồng bộ, lập vùng địa lý ảo (Geofencing) và lưu trữ hành trình tàu biển tối ưu.

## Tài liệu Dự án
*   [Kiến trúc Hệ thống & Workflow Chi tiết](file:///D:/Document/LapTrinh/VDT/2D_Digital_Map/docs/system_architecture.md): Tài liệu chi tiết mô tả cấu trúc các lớp (Models, Repositories, Services, Workers), cơ chế quản lý trạng thái COW (Copy-On-Write) với Lock-free Reads, thuật toán kiểm tra Geofencing Ray-Casting và quy trình lưu trữ CSDL tối ưu hóa hiệu năng (Batching & Transactions).

## Cấu trúc Dự án
*   `src/`: Thư mục mã nguồn C++ chính của dự án.
    *   `src/model/`: Các cấu trúc dữ liệu mô hình (Vessel, ShipMessage, AlertZone, AlertEvent, GeoPoint).
    *   `src/database/`: Lớp quản lý kết nối CSDL và các repositories thực hiện CRUD/Queries SQL.
    *   `src/state/`: Quản lý bộ nhớ đệm RAM Cache thời gian thực (`ShipStateStore`) với cơ chế thread-safe COW.
    *   `src/service/`: Lớp nghiệp vụ điều phối logic nghiệp vụ và geofencing.
    *   `src/worker/`: Các luồng worker chạy trên các thread riêng biệt xử lý parse JSON, Geofencing (`PositionWorker`) và ghi CSDL (`ShipWorker`).
    *   `src/network/`: Máy chủ TCP (`TcpServer`) lắng nghe telemetry từ xa.
*   `sql/migrations/`: Các tập tin SQL cài đặt schema và PostGIS spatial indexes.
*   `scripts/`: Các script hỗ trợ tự động migration, giả lập kiểm thử.
*   `CMakeLists.txt`: Cấu hình build system CMake của dự án.
