-- 003_create_positions.sql
-- Create positions table for vessel tracking

CREATE TABLE IF NOT EXISTS app.positions (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    vessel_id UUID NOT NULL REFERENCES app.vessels(id) ON DELETE CASCADE,
    recorded_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT now(),
    latitude DOUBLE PRECISION NOT NULL,
    longitude DOUBLE PRECISION NOT NULL,
    speed_knots REAL,
    course REAL,
    heading INT,
    geom GEOGRAPHY(POINT, 4326) -- PostGIS geography point
);

-- Indexes to speed up spatial and time queries
CREATE INDEX IF NOT EXISTS idx_positions_vessel_time ON app.positions (vessel_id, recorded_at);
CREATE INDEX IF NOT EXISTS idx_positions_geom ON app.positions USING GIST (geom);