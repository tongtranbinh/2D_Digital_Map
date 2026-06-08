-- 005_create_alert_events.sql
-- Create alert events table linking vessels to alert zones

CREATE TABLE IF NOT EXISTS app.alert_events (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    vessel_id UUID NOT NULL REFERENCES app.vessels(id) ON DELETE CASCADE,
    alert_zone_id UUID NOT NULL REFERENCES app.alert_zones(id) ON DELETE CASCADE,
    event_type TEXT NOT NULL, -- e.g., 'enter', 'exit', 'dwell'
    created_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
    position GEOGRAPHY(POINT, 4326)
);

CREATE INDEX IF NOT EXISTS idx_alert_events_vessel ON app.alert_events (vessel_id);
CREATE INDEX IF NOT EXISTS idx_alert_events_zone ON app.alert_events (alert_zone_id);
CREATE INDEX IF NOT EXISTS idx_alert_events_geom ON app.alert_events USING GIST (position);