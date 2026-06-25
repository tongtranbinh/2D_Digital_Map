#!/usr/bin/env bash
# =============================================================================
# wait-for-db.sh — Chờ PostgreSQL sẵn sàng trước khi khởi động app
# Sử dụng: ./wait-for-db.sh [timeout_seconds]
# =============================================================================
set -euo pipefail

TIMEOUT=${1:-60}
ELAPSED=0
INTERVAL=2

# Load env nếu chạy standalone
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$SCRIPT_DIR/shiptracking.env" ]]; then
    set -a
    source "$SCRIPT_DIR/shiptracking.env"
    set +a
fi

: "${DB_HOST:=localhost}"
: "${DB_PORT:=5432}"
: "${DB_USER:=ship_user}"
: "${DB_NAME:=ship_tracking}"

echo "[wait-for-db] Waiting for PostgreSQL at ${DB_HOST}:${DB_PORT} (timeout: ${TIMEOUT}s)..."

until pg_isready -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -q 2>/dev/null; do
    ELAPSED=$((ELAPSED + INTERVAL))
    if [[ $ELAPSED -ge $TIMEOUT ]]; then
        echo "[wait-for-db] ERROR: PostgreSQL did not become ready within ${TIMEOUT}s. Aborting." >&2
        exit 1
    fi
    echo "[wait-for-db] PostgreSQL not ready yet, retrying in ${INTERVAL}s... (${ELAPSED}/${TIMEOUT}s)"
    sleep "$INTERVAL"
done

echo "[wait-for-db] PostgreSQL is ready."
