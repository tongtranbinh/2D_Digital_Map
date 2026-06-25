#!/usr/bin/env bash
# =============================================================================
# cleanup_history.sh — Xóa dữ liệu position/alert cũ hơn 1 ngày
# Được gọi bởi systemd timer: shiptracking-cleanup.timer
# =============================================================================
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENV_FILE="$SCRIPT_DIR/shiptracking.env"

# Load config
if [[ -f "$ENV_FILE" ]]; then
    set -a && source "$ENV_FILE" && set +a
fi

: "${DB_HOST:=localhost}"
: "${DB_PORT:=5433}"
: "${DB_NAME:=ship_tracking}"
: "${DB_USER:=ship_user}"
: "${DB_PASSWORD:=123456}"

export PGPASSWORD="$DB_PASSWORD"

echo "[cleanup] Starting history cleanup at $(date -u '+%Y-%m-%d %H:%M:%S UTC')"

# Chạy cleanup SQL và capture số dòng bị xóa
RESULT=$(psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" \
    --tuples-only --no-align \
    -c "
    WITH
      del_pos AS (
        DELETE FROM app.positions
        WHERE recorded_at < NOW() - INTERVAL '1 day'
        RETURNING id
      ),
      del_alerts AS (
        DELETE FROM app.alert_events
        WHERE created_at < NOW() - INTERVAL '1 day'
        RETURNING id
      )
    SELECT
      (SELECT COUNT(*) FROM del_pos)    AS deleted_positions,
      (SELECT COUNT(*) FROM del_alerts) AS deleted_alerts;
    ")

DELETED_POS=$(echo "$RESULT" | awk -F'|' '{print $1}' | tr -d ' ')
DELETED_ALERTS=$(echo "$RESULT" | awk -F'|' '{print $2}' | tr -d ' ')

echo "[cleanup] Deleted positions : ${DELETED_POS:-0}"
echo "[cleanup] Deleted alert_events: ${DELETED_ALERTS:-0}"
echo "[cleanup] Done at $(date -u '+%Y-%m-%d %H:%M:%S UTC')"
