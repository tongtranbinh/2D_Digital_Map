-- scripts/seed_db.sql
-- Seed dữ liệu mẫu cho vùng cảnh báo Geofence để chạy thử nghiệm

-- Xóa dữ liệu cũ nếu có
TRUNCATE TABLE app.ship_zone_state CASCADE;
TRUNCATE TABLE app.alert_events CASCADE;
DELETE FROM app.alert_zones;

-- Chèn 2 Vùng cảnh báo bằng đa giác khép kín (ST_GeogFromText)
-- Điểm đầu tiên và điểm cuối cùng của đa giác phải TRÙNG NHAU để khép kín đa giác.

-- 1. Vùng cảnh báo Hoàng Sa (Paracel Islands Zone)
INSERT INTO app.alert_zones (id, name, description, enabled, geom)
VALUES (
    'a0e0a0e0-a0e0-a0e0-a0e0-a0e0a0e0a0e1',
    'Vùng Cảnh báo Hoàng Sa',
    'Vùng giám sát tàu biển xung quanh quần đảo Hoàng Sa',
    true,
    ST_GeogFromText('POLYGON((111.0 15.5, 113.0 15.5, 113.0 17.5, 111.0 17.5, 111.0 15.5))')
);

-- 2. Vùng Cấm neo đậu Vịnh Bắc Bộ (Gulf of Tonkin Zone)
INSERT INTO app.alert_zones (id, name, description, enabled, geom)
VALUES (
    'a0e0a0e0-a0e0-a0e0-a0e0-a0e0a0e0a0e2',
    'Vùng Cấm neo đậu Vịnh Bắc Bộ',
    'Khu vực kiểm soát an ninh và cấm neo đậu tự do Vịnh Bắc Bộ',
    true,
    ST_GeogFromText('POLYGON((106.5 19.0, 108.0 18.5, 108.5 20.5, 107.0 21.0, 106.5 19.0))')
);

-- 3. Thêm 100 vùng cảnh báo tự sinh trên các dải biển lớn để test geofence.
WITH ocean_regions AS (
    SELECT * FROM (VALUES
        (1, 'Bắc Đại Tây Dương', 10.0, 55.0, -55.0, -15.0),
        (2, 'Nam Đại Tây Dương', -45.0, 5.0, -45.0, -10.0),
        (3, 'Ấn Độ Dương Tây', -35.0, 20.0, 20.0, 80.0),
        (4, 'Ấn Độ Dương Đông', -40.0, 20.0, 80.0, 110.0),
        (5, 'Bắc Thái Bình Dương Tây', 10.0, 55.0, 140.0, 175.0),
        (6, 'Bắc Thái Bình Dương Đông', 10.0, 55.0, -170.0, -120.0),
        (7, 'Nam Thái Bình Dương Tây', -45.0, 5.0, 140.0, 175.0),
        (8, 'Nam Thái Bình Dương Đông', -45.0, 5.0, -170.0, -110.0),
        (9, 'Nam Đại Dương', -60.0, -45.0, -170.0, 170.0),
        (10, 'Bắc Băng Dương', 66.0, 78.0, -170.0, 170.0)
    ) AS t(region_id, region_name, lat_min, lat_max, lon_min, lon_max)
), generated AS (
    SELECT
        gs,
        r.region_name,
        r.lat_min + random() * (r.lat_max - r.lat_min) AS center_lat,
        r.lon_min + random() * (r.lon_max - r.lon_min) AS center_lon,
        (0.6 + random() * 0.8) AS size_lat,
        (0.6 + random() * 0.8) AS size_lon
    FROM generate_series(1, 100) AS gs
    JOIN ocean_regions AS r
      ON r.region_id = ((gs - 1) % 10) + 1
)
INSERT INTO app.alert_zones (name, description, enabled, geom)
SELECT
    format('Vùng Cảnh báo Tự sinh %s', gs),
    format('Vùng cảnh báo tự sinh trên biển - %s', region_name),
    true,
    ST_GeogFromText(
        format(
            'POLYGON((%s %s, %s %s, %s %s, %s %s, %s %s))',
            round((center_lon - size_lon / 2.0)::numeric, 6),
            round((center_lat - size_lat / 2.0)::numeric, 6),
            round((center_lon + size_lon / 2.0)::numeric, 6),
            round((center_lat - size_lat / 2.0)::numeric, 6),
            round((center_lon + size_lon / 2.0)::numeric, 6),
            round((center_lat + size_lat / 2.0)::numeric, 6),
            round((center_lon - size_lon / 2.0)::numeric, 6),
            round((center_lat + size_lat / 2.0)::numeric, 6),
            round((center_lon - size_lon / 2.0)::numeric, 6),
            round((center_lat - size_lat / 2.0)::numeric, 6)
        )
    )
FROM generated;

-- Chèn trước một số thông tin tàu tĩnh mẫu (Không bắt buộc, vì server sẽ tự tạo tên "Unknown Vessel" nếu chưa có)
-- Nhưng thêm vào để hiển thị tên tàu đẹp mắt hơn trên bản đồ.
INSERT INTO app.vessels (id, mmsi, name, callsign, imo, ship_type)
VALUES 
    (
        'd9426f4f-45a7-5efb-9528-7667ff49c690', -- UUID sinh ra từ index 0 (uuid5)
        999000001,
        'Tàu Tuần Tra Hải Quân 01',
        'HQ01',
        1000001,
        1 -- Loại tàu chiến
    )
ON CONFLICT (id) DO UPDATE SET name = EXCLUDED.name, mmsi = EXCLUDED.mmsi;

SELECT id, name, ST_AsText(geom) FROM app.alert_zones;
