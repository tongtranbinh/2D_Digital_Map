-- 001_init_postgis.sql
-- Initialize PostGIS extension and helper functions

CREATE EXTENSION IF NOT EXISTS postgis;

-- Optional: Enable uuid-ossp for UUID generation if needed
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- Ensure geography type is available and create a spatial reference for WGS84
-- SRID 4326 is standard WGS84; PostGIS provides it by default.

-- Create a schema for application data
CREATE SCHEMA IF NOT EXISTS app;

-- Set search path to include app schema (optional, left commented)
-- ALTER ROLE postgres SET search_path = app, public;