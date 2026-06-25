-- =============================================================================
-- cleanup_history.sql
-- Xóa dữ liệu cũ hơn 1 ngày khỏi positions và alert_events
-- Chạy bởi systemd timer mỗi ngày lúc 00:00 UTC
-- Để thay đổi retention: sửa INTERVAL '1 day' thành '7 days', '30 days'...
-- =============================================================================

-- Xóa position history cũ hơn 1 ngày
DELETE FROM app.positions
WHERE recorded_at < NOW() - INTERVAL '1 day';

-- Xóa alert events cũ hơn 1 ngày
DELETE FROM app.alert_events
WHERE created_at < NOW() - INTERVAL '1 day';

-- Log số dòng đã xóa ra NOTICE (thấy trong journalctl)
DO $$
DECLARE
    deleted_pos     INT;
    deleted_alerts  INT;
BEGIN
    GET DIAGNOSTICS deleted_pos    = ROW_COUNT;
    RAISE NOTICE '[cleanup] Deleted old positions and alert_events older than 1 day.';
END $$;
