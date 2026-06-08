#!/bin/bash

set -e

DB_HOST="localhost"
DB_PORT="5432"
DB_NAME="ship_tracking"
DB_USER="ship_user"
DB_PASSWORD="123456"
echo "Running migrations..."

psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -f sql/migrations/001_init_postgis.sql
psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -f sql/migrations/002_create_vessels.sql
psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -f sql/migrations/003_create_positions.sql
psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -f sql/migrations/004_create_alert_zones.sql
psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -f sql/migrations/005_create_alert_events.sql
psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -f sql/migrations/006_create_ship_zone_state.sql

echo "Migrations completed."