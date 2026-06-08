-- 004_create_alert_zones.sql
-- Create alert_zones table storing polygon zones

CREATE TABLE IF NOT EXISTS app.alert_zones (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    name TEXT NOT NULL,
    description TEXT,
    enabled BOOLEAN NOT NULL DEFAULT true,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
    geom GEOGRAPHY(POLYGON, 4326) -- PostGIS geography polygon
);

-- Spatial index for zones
CREATE INDEX IF NOT EXISTS idx_alert_zones_geom ON app.alert_zones USING GIST (geom);