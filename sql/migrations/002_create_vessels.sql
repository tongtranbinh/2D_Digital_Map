-- 002_create_vessels.sql
-- Create vessels table

CREATE TABLE IF NOT EXISTS app.vessels (
    id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
    mmsi BIGINT UNIQUE NOT NULL,
    name TEXT,
    callsign TEXT,
    imo BIGINT,
    ship_type INT,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT now(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT now()
);

-- Index on mmsi for quick lookups
CREATE INDEX IF NOT EXISTS idx_vessels_mmsi ON app.vessels(mmsi);