import socket
import time
import json
import random
import math
import argparse

DEFAULT_HOST = '127.0.0.1'
DEFAULT_PORT = 5555
DEFAULT_SHIP_COUNT = 15

# Vùng tọa độ mặc định để sinh tàu (Khu vực Hải Phòng - Quảng Ninh)
LAT_CENTER = 20.78
LON_CENTER = 106.90
LAT_RANGE = 0.2
LON_RANGE = 0.2

# Danh sách tàu tĩnh mẫu
PRESET_SHIPS = [
    {"mmsi": 371294000, "name": "VINA LINES 01", "type": "Cargo"},
    {"mmsi": 371295000, "name": "CAT BA FAST FERRY", "type": "Passenger"},
    {"mmsi": 371296000, "name": "CONTAINER ONE", "type": "Cargo"},
    {"mmsi": 371297000, "name": "HA LONG CRUISE 01", "type": "Leisure"},
    {"mmsi": 371298000, "name": "VIETSOVPETRO 05", "type": "Tanker"}
]

class ShipSimulator:
    def __init__(self, mmsi, name, lat, lon, speed, course):
        self.mmsi = mmsi
        self.name = name
        self.lat = lat
        self.lon = lon
        self.speed = speed   # knots
        self.course = course # degrees (0-359)

    def update_position(self, dt=1.0):
        # 1 knot = 0.514444 m/s
        speed_mps = self.speed * 0.514444
        distance = speed_mps * dt # mét đi được
        
        # 1 độ vĩ độ ≈ 111111 mét
        delta_lat = (distance * math.cos(math.radians(self.course))) / 111111.0
        
        # 1 độ kinh độ ≈ 111111 * cos(lat) mét
        lat_rad = math.radians(self.lat)
        delta_lon = (distance * math.sin(math.radians(self.course))) / (111111.0 * math.cos(lat_rad))
        
        self.lat += delta_lat
        self.lon += delta_lon
        
        # Ngẫu nhiên thay đổi hướng nhẹ để mô phỏng chân thực
        self.course = (self.course + random.uniform(-4, 4)) % 360
        # Ngẫu nhiên tăng giảm tốc độ nhẹ
        self.speed = max(2.0, min(35.0, self.speed + random.uniform(-0.5, 0.5)))

    def to_json(self):
        return {
            "mmsi": self.mmsi,
            "name": self.name,
            "lat": round(self.lat, 6),
            "lon": round(self.lon, 6),
            "speed": round(self.speed, 1),
            "course": round(self.course, 1),
            "timestamp": int(time.time())
        }

def generate_ships(count):
    ships = []
    # Thêm các tàu mẫu đã đăng ký trước
    for i, p in enumerate(PRESET_SHIPS):
        if i >= count:
            break
        # Đặt tọa độ ngẫu nhiên xung quanh khu vực Hải Phòng
        lat = LAT_CENTER + random.uniform(-LAT_RANGE/2, LAT_RANGE/2)
        lon = LON_CENTER + random.uniform(-LON_RANGE/2, LON_RANGE/2)
        speed = random.uniform(8.0, 25.0)
        course = random.uniform(0.0, 360.0)
        ships.append(ShipSimulator(p["mmsi"], p["name"], lat, lon, speed, course))
    
    # Sinh thêm tàu ngẫu nhiên nếu số lượng yêu cầu lớn hơn danh sách mẫu
    for i in range(len(ships), count):
        mmsi = 371300000 + i
        name = f"VESSEL {i:03d}"
        lat = LAT_CENTER + random.uniform(-LAT_RANGE/2, LAT_RANGE/2)
        lon = LON_CENTER + random.uniform(-LON_RANGE/2, LON_RANGE/2)
        speed = random.uniform(5.0, 30.0)
        course = random.uniform(0.0, 360.0)
        ships.append(ShipSimulator(mmsi, name, lat, lon, speed, course))
        
    return ships

def main():
    parser = argparse.ArgumentParser(description="Real-time Vessel Telemetry Simulator over TCP")
    parser.add_argument("--host", default=DEFAULT_HOST, help="TCP Server Host IP")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="TCP Server Port")
    parser.add_argument("--count", type=int, default=DEFAULT_SHIP_COUNT, help="Number of simulated ships (10-100)")
    args = parser.parse_args()

    print(f"Khởi tạo bộ giả lập với {args.count} tàu...")
    sim_ships = generate_ships(args.count)
    
    while True:
        try:
            print(f"Đang kết nối tới TCP Server {args.host}:{args.port}...")
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((args.host, args.port))
            print("Đã kết nối thành công! Bắt đầu truyền dữ liệu...")
            
            while True:
                for ship in sim_ships:
                    ship.update_position(dt=1.0)
                    data = ship.to_json()
                    
                    # Chuyển đổi sang JSON String kết thúc bằng ký tự \n làm frame separator
                    message = json.dumps(data) + "\n"
                    s.sendall(message.encode('utf-8'))
                
                # In trạng thái log nhẹ
                print(f"Đã gửi telemetry của {len(sim_ships)} tàu.")
                time.sleep(1.0)
                
        except (ConnectionRefusedError, socket.error):
            print("Không thể kết nối đến TCP Server. Đang thử lại sau 3 giây...")
            time.sleep(3)
        except KeyboardInterrupt:
            print("\nDừng bộ giả lập.")
            break
        finally:
            try:
                s.close()
            except:
                pass

if __name__ == '__main__':
    main()
