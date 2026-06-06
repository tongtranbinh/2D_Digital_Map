-- Kích hoạt PostGIS extension (yêu cầu quyền superuser hoặc postgis đã được cài đặt trên server)
CREATE EXTENSION IF NOT EXISTS postgis;

-- Xóa các bảng cũ nếu tồn tại để tránh xung đột khi chạy lại script
DROP TABLE IF EXISTS ship_telemetry CASCADE;
DROP TABLE IF EXISTS warning_zones CASCADE;
DROP TABLE IF EXISTS ships CASCADE;

-- 1. Bảng lưu trữ thông tin tĩnh của tàu
CREATE TABLE ships (
    mmsi INT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    type VARCHAR(50) DEFAULT 'Cargo',
    length DOUBLE PRECISION DEFAULT 0.0,
    width DOUBLE PRECISION DEFAULT 0.0
);

-- 2. Bảng lưu trữ danh sách các vùng cảnh báo dưới dạng đa giác (Polygon)
CREATE TABLE warning_zones (
    id SERIAL PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    severity VARCHAR(20) DEFAULT 'warning', -- 'warning' (màu cam) hoặc 'critical' (màu đỏ)
    geom GEOMETRY(Polygon, 4326) NOT NULL   -- Hệ tọa độ WGS84
);

-- Tạo Spatial Index cho các đa giác cảnh báo
CREATE INDEX idx_warning_zones_geom ON warning_zones USING GIST (geom);

-- 3. Bảng lưu trữ lịch sử telemetry (vết tàu) theo thời gian thực
CREATE TABLE ship_telemetry (
    id BIGSERIAL PRIMARY KEY,
    mmsi INT REFERENCES ships(mmsi) ON DELETE CASCADE,
    geom GEOMETRY(Point, 4326) NOT NULL,    -- Vị trí GPS (WGS84 Point)
    speed REAL NOT NULL,                    -- Vận tốc (knots)
    course REAL NOT NULL,                   -- Hướng đi (0-359 độ)
    timestamp TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Tạo Spatial Index cho tọa độ tàu để hỗ trợ các truy vấn không gian lịch sử
CREATE INDEX idx_ship_telemetry_geom ON ship_telemetry USING GIST (geom);
-- Index thường phục vụ cho việc lấy vị trí mới nhất của từng tàu nhanh nhất
CREATE INDEX idx_ship_telemetry_mmsi_time ON ship_telemetry (mmsi, timestamp DESC);
