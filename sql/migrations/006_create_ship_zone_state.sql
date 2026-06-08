-- 006_create_ship_zone_state.sql
-- Create ship_zone_state table to track current status of vessels inside/outside zones

CREATE TABLE IF NOT EXISTS app.ship_zone_state (
    ship_id UUID NOT NULL REFERENCES app.vessels(id) ON DELETE CASCADE,
    zone_id UUID NOT NULL REFERENCES app.alert_zones(id) ON DELETE CASCADE,
    is_inside BOOLEAN NOT NULL,
    last_changed_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT now(),
    PRIMARY KEY (ship_id, zone_id)
);

-- Index for quick lookups and updates on states
CREATE INDEX IF NOT EXISTS idx_ship_zone_state_lookup ON app.ship_zone_state(ship_id, zone_id);
