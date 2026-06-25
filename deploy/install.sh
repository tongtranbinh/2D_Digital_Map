#!/usr/bin/env bash
# =============================================================================
# install.sh — Build Release và cài đặt Ship Tracking lên server
#
# Chạy một lần khi deploy lần đầu hoặc khi có code mới.
# Cách dùng: sudo ./deploy/install.sh
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
INSTALL_DIR="/opt/shiptracking"
SERVICE_NAME="shiptracking-backend"
APP_USER="shiptracking"

# --- Màu terminal ---
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
info()    { echo -e "${GREEN}[install]${NC} $*"; }
warn()    { echo -e "${YELLOW}[install]${NC} $*"; }
error()   { echo -e "${RED}[install] ERROR:${NC} $*" >&2; exit 1; }

# --- Kiểm tra quyền ---
[[ $EUID -eq 0 ]] || error "Cần chạy với sudo: sudo ./deploy/install.sh"

# ============================================================
# BƯỚC 1: Tạo user hệ thống (nếu chưa có)
# ============================================================
info "Bước 1/6: Chuẩn bị user hệ thống '$APP_USER'..."
if ! id "$APP_USER" &>/dev/null; then
    useradd --system --no-create-home --shell /usr/sbin/nologin "$APP_USER"
    info "  Đã tạo user: $APP_USER"
else
    info "  User '$APP_USER' đã tồn tại, bỏ qua."
fi

# ============================================================
# BƯỚC 2: Cài dependencies Qt6 runtime
# ============================================================
info "Bước 2/6: Kiểm tra Qt6 runtime dependencies..."
PKGS_MISSING=()
for pkg in \
    libqt6core6 libqt6network6 libqt6widgets6 libqt6sql6 \
    libqt6positioning6 libqt6qml6 libqt6quick6 \
    qt6-qpa-plugins \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtqml-workerscript \
    qml6-module-qtpositioning \
    qml6-module-qtlocation \
    qml6-module-qtquick-window \
    postgresql-client; do
    dpkg -s "$pkg" &>/dev/null || PKGS_MISSING+=("$pkg")
done

if [[ ${#PKGS_MISSING[@]} -gt 0 ]]; then
    info "  Cài đặt: ${PKGS_MISSING[*]}"
    apt-get install -y "${PKGS_MISSING[@]}"
else
    info "  Tất cả dependencies đã có sẵn."
fi

# ============================================================
# BƯỚC 3: Build Release
# ============================================================
info "Bước 3/6: Build Release binary..."
BUILD_DIR="$PROJECT_DIR/build-release"

cmake -B "$BUILD_DIR" -S "$PROJECT_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG" \
    2>&1 | sed 's/^/  /'

cmake --build "$BUILD_DIR" --config Release --parallel "$(nproc)" \
    2>&1 | sed 's/^/  /'

BINARY="$BUILD_DIR/ShipTrackingApp"
[[ -f "$BINARY" ]] || error "Build thất bại, không tìm thấy binary: $BINARY"
info "  Build thành công: $BINARY ($(du -sh "$BINARY" | cut -f1))"

# ============================================================
# BƯỚC 4: Dừng service cũ (nếu đang chạy)
# ============================================================
info "Bước 4/6: Dừng service cũ (nếu đang chạy)..."
if systemctl is-active --quiet "$SERVICE_NAME" 2>/dev/null; then
    systemctl stop "$SERVICE_NAME"
    info "  Service '$SERVICE_NAME' đã dừng."
else
    info "  Service chưa chạy, bỏ qua."
fi

# ============================================================
# BƯỚC 5: Cài đặt file vào /opt/shiptracking/
# ============================================================
info "Bước 5/6: Cài đặt vào $INSTALL_DIR..."

mkdir -p "$INSTALL_DIR/bin" "$INSTALL_DIR/deploy"

# Binary
install -m 755 "$BINARY" "$INSTALL_DIR/bin/ShipTrackingApp"
info "  Installed: $INSTALL_DIR/bin/ShipTrackingApp"

# Deploy scripts (env, wait-for-db, run-ui, service file)
cp "$SCRIPT_DIR/shiptracking.env"              "$INSTALL_DIR/shiptracking.env"
cp "$SCRIPT_DIR/wait-for-db.sh"               "$INSTALL_DIR/deploy/wait-for-db.sh"
cp "$SCRIPT_DIR/run-ui.sh"                    "$INSTALL_DIR/deploy/run-ui.sh"
chmod +x "$INSTALL_DIR/deploy/wait-for-db.sh" "$INSTALL_DIR/deploy/run-ui.sh"

# Map và docker-compose
cp "$PROJECT_DIR/docker-compose.yml"          "$INSTALL_DIR/docker-compose.yml"
cp "$PROJECT_DIR/map.osm.pbf"                 "$INSTALL_DIR/map.osm.pbf"

# SQL migrations (backup tham khảo)
cp -r "$PROJECT_DIR/sql"                      "$INSTALL_DIR/sql"

# Quyền sở hữu
chown -R "$APP_USER:$APP_USER" "$INSTALL_DIR"
# env file chứa password — chỉ root và shiptracking đọc được
chmod 640 "$INSTALL_DIR/shiptracking.env"

info "  Tất cả file đã được cài đặt."

# ============================================================
# BƯỚC 6: Cài và khởi động systemd service
# ============================================================
info "Bước 6/6: Cài đặt systemd service..."

cp "$SCRIPT_DIR/shiptracking-backend.service" "/etc/systemd/system/$SERVICE_NAME.service"
systemctl daemon-reload
systemctl enable "$SERVICE_NAME"
systemctl start  "$SERVICE_NAME"

sleep 2
if systemctl is-active --quiet "$SERVICE_NAME"; then
    info "  Service '$SERVICE_NAME' đang chạy."
    systemctl status "$SERVICE_NAME" --no-pager -l | sed 's/^/  /'
else
    error "Service khởi động thất bại. Xem log: journalctl -u $SERVICE_NAME -n 50"
fi

# ============================================================
echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN} Deploy hoàn tất!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "  Xem logs backend:    journalctl -u $SERVICE_NAME -f"
echo "  Dừng backend:        sudo systemctl stop $SERVICE_NAME"
echo "  Demo giao diện:      $INSTALL_DIR/deploy/run-ui.sh"
echo "  Demo không màn hình: $INSTALL_DIR/deploy/run-ui.sh --xvfb"
echo ""
