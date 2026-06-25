#!/usr/bin/env bash
# =============================================================================
# run-ui.sh — Khởi chạy giao diện Qt/QML để demo hoặc kiểm tra thủ công
#
# Yêu cầu: Backend service (shiptracking-backend) phải đang chạy
# Cách dùng:
#   Local (có màn hình):  ./run-ui.sh
#   Remote (qua SSH):     ssh -X user@server "cd /opt/shiptracking && ./deploy/run-ui.sh"
#   Remote (Xvfb):        ./run-ui.sh --xvfb
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_DIR="$(dirname "$SCRIPT_DIR")"
BINARY="$INSTALL_DIR/bin/ShipTrackingApp"
# Env file nằm ở /opt/shiptracking/shiptracking.env (không phải trong deploy/)
ENV_FILE="$INSTALL_DIR/shiptracking.env"
USE_XVFB=false

# --- Parse args ---
for arg in "$@"; do
    case "$arg" in
        --xvfb) USE_XVFB=true ;;
        --help|-h)
            echo "Usage: $0 [--xvfb]"
            echo "  --xvfb   Dùng virtual framebuffer (không cần màn hình thật)"
            exit 0
            ;;
    esac
done

# --- Load env ---
if [[ -f "$ENV_FILE" ]]; then
    set -a
    source "$ENV_FILE"
    set +a
    echo "[run-ui] Loaded config from $ENV_FILE"
else
    echo "[run-ui] WARNING: $ENV_FILE not found, using defaults"
fi

# --- Kiểm tra binary ---
if [[ ! -x "$BINARY" ]]; then
    echo "[run-ui] ERROR: Binary not found or not executable: $BINARY" >&2
    echo "[run-ui] Run deploy/install.sh first." >&2
    exit 1
fi

# --- Chờ DB ---
echo "[run-ui] Checking database availability..."
"$SCRIPT_DIR/wait-for-db.sh" 30

# --- Khởi chạy ---
if [[ "$USE_XVFB" == true ]]; then
    if ! command -v Xvfb &>/dev/null; then
        echo "[run-ui] ERROR: Xvfb not found. Install: sudo apt install xvfb" >&2
        exit 1
    fi
    DISPLAY_NUM=:$(( RANDOM % 100 + 10 ))
    echo "[run-ui] Starting virtual display on $DISPLAY_NUM ..."
    Xvfb "$DISPLAY_NUM" -screen 0 1920x1080x24 &
    XVFB_PID=$!
    trap "kill $XVFB_PID 2>/dev/null; echo '[run-ui] Xvfb stopped.'" EXIT
    export DISPLAY="$DISPLAY_NUM"
    sleep 1
fi

echo "[run-ui] Launching ShipTrackingApp UI..."
echo "[run-ui] DISPLAY=${DISPLAY:-not set}, TCP_PORT=${TCP_PORT:-9000}"
exec "$BINARY"
