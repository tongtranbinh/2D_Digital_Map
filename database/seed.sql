-- Khởi tạo thông tin tĩnh của một số tàu thủy
INSERT INTO ships (mmsi, name, type, length, width) VALUES
(371294000, 'VINA LINES 01', 'Cargo', 145.5, 22.0),
(371295000, 'CAT BA FAST FERRY', 'Passenger', 38.2, 9.5),
(371296000, 'CONTAINER ONE', 'Cargo', 210.0, 32.2),
(371297000, 'HA LONG CRUISE 01', 'Leisure', 45.0, 10.0),
(371298000, 'VIETSOVPETRO 05', 'Tanker', 180.0, 28.0)
ON CONFLICT (mmsi) DO NOTHING;

-- Khởi tạo các vùng cảnh báo dưới dạng Polygon (Hệ tọa độ 4326: WGS84, định dạng POLYGON((lon lat, ...)))
-- Lưu ý: Thứ tự các điểm trong Polygon phải khép kín (điểm đầu và điểm cuối trùng nhau)
INSERT INTO warning_zones (name, severity, geom) VALUES
(
    'Vùng Cấm Neo Đậu Cát Bà', 
    'critical', 
    ST_GeomFromText('POLYGON((106.95 20.75, 106.95 20.85, 107.05 20.85, 107.05 20.75, 106.95 20.75))', 4326)
),
(
    'Vùng Biển Quân Sự Hải Phòng', 
    'warning', 
    ST_GeomFromText('POLYGON((106.78 20.62, 106.78 20.72, 106.88 20.72, 106.88 20.62, 106.78 20.62))', 4326)
),
(
    'Hành Lang Hàng Hải Hạ Long', 
    'warning', 
    ST_GeomFromText('POLYGON((107.02 20.82, 107.02 20.90, 107.12 20.90, 107.12 20.82, 107.02 20.82))', 4326)
)
ON CONFLICT DO NOTHING;
