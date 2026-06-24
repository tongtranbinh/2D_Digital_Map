#!/bin/bash
set -e

# Tìm xem trong thư mục hiện tại có file .osm.pbf nào không
MAP_FILE=$(ls *.osm.pbf 2>/dev/null | grep -v "map.osm.pbf" | head -n 1 || true)

if [ -z "$MAP_FILE" ] && [ ! -f "map.osm.pbf" ]; then
    echo "=====> Không tìm thấy file dữ liệu .osm.pbf nào trong thư mục dự án."
    echo "Để có dữ liệu offline đầy đủ của cả Biển Đông, bạn có thể chọn một trong các phương án:"
    echo "1. (Khuyên dùng - Nhẹ & Nhanh): Truy cập https://extract.bbbike.org/"
    echo "   - Chọn Format: Protocolbuffer (PBF)"
    echo "   - Nhập tọa độ Bounding Box bao quanh Biển Đông: South: 0, West: 100, North: 25, East: 125"
    echo "   - Điền email của bạn và nhấn Extract. Tải file PBF nhận được qua email (~100-200MB) về và đặt vào thư mục này."
    echo "2. (Tự động - Nặng): Tải toàn bộ vùng Đông Nam Á (south-east-asia-latest.osm.pbf ~3.4 GB)."
    echo ""
    read -p "Bạn muốn tải tự động file Đông Nam Á 3.4 GB từ Geofabrik không? (y/n): " confirm
    if [ "$confirm" = "y" ] || [ "$confirm" = "Y" ]; then
        echo "=====> Đang tải tự động vùng Đông Nam Á (3.4 GB)..."
        curl -L -o south-east-asia-latest.osm.pbf https://download.geofabrik.de/asia/south-east-asia-latest.osm.pbf
        MAP_FILE="south-east-asia-latest.osm.pbf"
    else
        echo "=====> Đang tải mặc định vùng Việt Nam (vietnam-latest.osm.pbf ~35MB) để demo..."
        curl -L -o vietnam-latest.osm.pbf https://download.geofabrik.de/asia/vietnam-latest.osm.pbf
        MAP_FILE="vietnam-latest.osm.pbf"
    fi
fi

# Thiết lập file map.osm.pbf cho docker-compose
if [ -n "$MAP_FILE" ]; then
    echo "=====> Phát hiện file dữ liệu mới: $MAP_FILE. Đang sao chép thành map.osm.pbf..."
    cp "$MAP_FILE" map.osm.pbf
fi

# Thực hiện import dữ liệu vào OSM Tile Server nếu chưa được khởi tạo
echo "=====> Kiểm tra xem cơ sở dữ liệu OSM đã được khởi tạo chưa..."
if docker-compose run --rm --entrypoint sh osm-tile-server -c '[ -f "/data/database/postgres/PG_VERSION" ]'; then
    echo "=====> Database OSM đã tồn tại."
    read -p "Bạn có muốn xóa database cũ để import lại từ đầu không? (y/n, mặc định là n): " reset_db
    if [ "$reset_db" = "y" ] || [ "$reset_db" = "Y" ]; then
        echo "=====> Đang dọn dẹp volume dữ liệu OSM cũ..."
        docker-compose run --rm --entrypoint sh osm-tile-server -c 'rm -rf /data/*'
        echo "=====> Đang bắt đầu import dữ liệu bản đồ mới (quá trình này có thể mất vài phút)..."
        docker-compose run --rm osm-tile-server import
    else
        echo "=====> Giữ nguyên database cũ. Tiếp tục khởi động..."
    fi
else
    echo "=====> Không tìm thấy database OSM. Bắt đầu import dữ liệu bản đồ (quá trình này có thể mất vài phút)..."
    docker-compose run --rm osm-tile-server import
fi

# Khởi động các dịch vụ
echo "=====> Khởi động PostgreSQL/PostGIS và OSM Tile Server..."
docker-compose up -d

echo "=====> Thành công!"
echo "OSM Tile Server đang chạy tại: http://localhost:8080/tile/{z}/{x}/{y}.png"
echo "PostgreSQL đang chạy tại: localhost:5432"
