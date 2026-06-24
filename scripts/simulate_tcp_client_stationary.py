import socket
import json
import time
import random
import uuid
import math

# Cấu hình kết nối TCP
HOST = 'localhost'
PORT = 9000
NUM_SHIPS = 2000

# Khu vực địa lý khởi tạo (Vùng biển Việt Nam / Biển Đông)
# Lat: 10.0 đến 22.0 N, Lon: 105.0 đến 115.0 E
LAT_MIN, LAT_MAX = 10.0, 22.0
LON_MIN, LON_MAX = 105.0, 115.0

# Định nghĩa các đa giác đất liền (Mainland / Islands) để tàu không đi vào
LAND_POLYGONS = [
    # Việt Nam (Đất liền phía Tây)
    [
        (10.0, 104.0),
        (10.0, 107.3),
        (10.9, 108.3),
        (12.2, 109.2),
        (13.7, 109.2),
        (15.1, 108.9),
        (16.2, 108.2),
        (17.5, 106.6),
        (19.0, 105.8),
        (20.8, 106.8),
        (21.5, 108.0),
        (22.0, 108.0),
        (22.0, 104.0)
    ],
    # Đảo Hải Nam (Hainan Island)
    [
        (20.1, 109.5),
        (20.1, 111.0),
        (19.5, 111.0),
        (18.2, 109.6),
        (18.2, 109.0),
        (19.0, 108.6)
    ],
    # Trung Quốc (Đất liền phía Bắc)
    [
        (21.5, 108.0),
        (21.5, 109.5),
        (21.0, 110.0),
        (20.2, 110.1),
        (20.2, 110.6),
        (21.2, 110.6),
        (21.5, 111.0),
        (21.6, 112.5),
        (22.0, 114.0),
        (22.0, 117.0),
        (23.0, 117.0),
        (23.0, 108.0)
    ]
]

def is_on_land(lat, lon):
    """
    Sử dụng thuật toán Ray-Casting để kiểm tra xem tọa độ (lat, lon) có nằm trong đất liền không.
    """
    for poly in LAND_POLYGONS:
        cn = 0
        n = len(poly)
        for i in range(n):
            p1 = poly[i]
            p2 = poly[(i + 1) % n]
            # p[0] là latitude, p[1] là longitude
            if ((p1[0] <= lat < p2[0]) or (p2[0] <= lat < p1[0])):
                vt = (lat - p1[0]) / (p2[0] - p1[0])
                if lon < p1[1] + vt * (p2[1] - p1[1]):
                    cn += 1
        if (cn % 2) != 0:
            return True
    return False

print(f"Đang khởi tạo cấu hình cho {NUM_SHIPS} tàu đứng yên...")

# Khởi tạo dữ liệu trạng thái cho 2000 tàu
random.seed(42)  # Seed cố định để các tọa độ ban đầu giống nhau mọi lúc chạy
ships = []
for i in range(NUM_SHIPS):
    # Sử dụng uuid5 với Namespace cố định để sinh ra UUID cố định cho từng chỉ số tàu
    ship_uuid = str(uuid.uuid5(uuid.NAMESPACE_DNS, f"vessel_{i}"))
    
    # Tìm vị trí khởi tạo không thuộc đất liền
    while True:
        lat = random.uniform(LAT_MIN, LAT_MAX)
        lon = random.uniform(LON_MIN, LON_MAX)
        if not is_on_land(lat, lon):
            break
            
    speed = 0.0  # Vận tốc: 0 hải lý/giờ (tàu đứng yên)
    heading = random.uniform(0.0, 360.0) # Hướng đi ban đầu
    course = heading
    
    ships.append({
        "shipId": ship_uuid,
        "latitude": lat,
        "longitude": lon,
        "speed": speed,
        "heading": heading,
        "course": course
    })

def update_position(ship, dt_seconds):
    """
    Tàu đứng yên, không thay đổi vị trí, vận tốc hay hướng đi.
    """
    pass

def main():
    print(f"Đang kết nối tới TCP Server {HOST}:{PORT}...")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        print("Đã kết nối thành công!")
    except Exception as e:
        print(f"Không thể kết nối tới server: {e}")
        print("Vui lòng đảm bảo rằng ứng dụng ShipTracking của bạn đang chạy.")
        return

    print("Bắt đầu mô phỏng gửi dữ liệu liên tục với tần suất 1s (tàu đứng yên)...")

    try:
        while True:
            current_time = time.time()
            dt_seconds = 1

            payload_parts = []
            for ship in ships:
                update_position(ship, dt_seconds)

                # Tạo bản tin JSON
                msg = {
                    "shipId": ship["shipId"],
                    "latitude": round(ship["latitude"], 6),
                    "longitude": round(ship["longitude"], 6),
                    "speed": round(ship["speed"], 1),
                    "heading": round(ship["heading"], 1),
                    "course": round(ship["course"], 1),
                    "timestamp": int(current_time * 1000)
                }
                payload_parts.append(json.dumps(msg) + "\n")

            # Ghép toàn bộ bản tin vào 1 gói TCP lớn và gửi đi
            payload = "".join(payload_parts)
            s.sendall(payload.encode('utf-8'))

            print(f"[{time.strftime('%H:%M:%S')}] Đã gửi cập nhật vị trí đứng yên cho {NUM_SHIPS} tàu.")
            
            # Đợi 1 giây trước khi gửi chu kỳ tiếp theo
            time.sleep(1)

    except KeyboardInterrupt:
        print("\nDừng mô phỏng.")
    except Exception as e:
        print(f"\nLỗi khi gửi dữ liệu: {e}")
    finally:
        s.close()
        print("Đã đóng kết nối TCP socket.")

if __name__ == '__main__':
    main()
