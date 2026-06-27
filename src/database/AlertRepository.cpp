#include "AlertRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include "RepositoryUtils.h"

namespace {

bool setError(QString *error, const QSqlQuery &query)
{
	if (error) {
		*error = query.lastError().text();
	}
	return false;
}

AlertZone readZone(const QSqlQuery &query)
{
	AlertZone zone;
	zone.id = uuidFromString(query.value(QStringLiteral("id")).toString());
	zone.name = query.value(QStringLiteral("name")).toString();
	zone.description = query.value(QStringLiteral("description")).toString();
	zone.enabled = query.value(QStringLiteral("enabled")).toBool();
	zone.polygon = polygonFromWkt(query.value(QStringLiteral("polygon_wkt")).toString());
	return zone;
}

AlertEvent readEvent(const QSqlQuery &query)
{
	AlertEvent event;
	event.id = uuidFromString(query.value(QStringLiteral("id")).toString());
	event.vesselId = uuidFromString(query.value(QStringLiteral("vessel_id")).toString());
	event.alertZoneId = uuidFromString(query.value(QStringLiteral("alert_zone_id")).toString());
	event.eventType = query.value(QStringLiteral("event_type")).toString();
	event.createdAt = query.value(QStringLiteral("created_at")).toDateTime();

	const QString positionWkt = query.value(QStringLiteral("position_wkt")).toString();
	if (!positionWkt.isEmpty()) {
		event.position = pointFromWkt(positionWkt);
	}

	return event;
}

} // namespace

AlertRepository::AlertRepository(PostgresConnection &connection)
	: m_connection(connection)
{
}

bool AlertRepository::saveZone(AlertZone &zone, QString *error)
{
	if (zone.id.isNull()) {
		zone.id = QUuid::createUuid();
	}

	if (zone.polygon.size() < 3) {
		if (error) {
			*error = QStringLiteral("polygon must contain at least 3 points");
		}
		return false;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return false;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		INSERT INTO app.alert_zones (id, name, description, enabled, geom)
		VALUES (
			:id,
			:name,
			:description,
			:enabled,
			ST_GeogFromText(:polygon_wkt)
		)
		ON CONFLICT (id) DO UPDATE SET
			name = EXCLUDED.name,
			description = EXCLUDED.description,
			enabled = EXCLUDED.enabled,
			geom = EXCLUDED.geom,
			updated_at = now()
	)");

	query.bindValue(QStringLiteral(":id"), uuidToString(zone.id));
	query.bindValue(QStringLiteral(":name"), zone.name);
	query.bindValue(QStringLiteral(":description"), zone.description);
	query.bindValue(QStringLiteral(":enabled"), zone.enabled);
	query.bindValue(QStringLiteral(":polygon_wkt"), polygonToWkt(zone.polygon));

	if (!query.exec()) {
		return setError(error, query);
	}

	return true;
}

bool AlertRepository::deleteZone(const QUuid &id, QString *error)
{
	if (id.isNull()) {
		return true;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return false;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		DELETE FROM app.alert_zones
		WHERE id = CAST(:id AS uuid)
	)");
	query.bindValue(QStringLiteral(":id"), uuidToString(id));

	if (!query.exec()) {
		return setError(error, query);
	}

	return true;
}

std::optional<AlertZone> AlertRepository::findZoneById(const QUuid &id, QString *error) const
{
	if (id.isNull()) {
		return std::nullopt;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return std::nullopt;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT
			id,
			name,
			description,
			enabled,
			ST_AsText(geom::geometry) AS polygon_wkt
		FROM app.alert_zones
		WHERE id = CAST(:id AS uuid)
		LIMIT 1
	)");
	query.bindValue(QStringLiteral(":id"), uuidToString(id));

	if (!query.exec()) {
		setError(error, query);
		return std::nullopt;
	}

	if (!query.next()) {
		return std::nullopt;
	}

	return readZone(query);
}

QVector<AlertZone> AlertRepository::listZones(QString *error) const
{
	QVector<AlertZone> zones;

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return zones;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT
			id,
			name,
			description,
			enabled,
			ST_AsText(geom::geometry) AS polygon_wkt
		FROM app.alert_zones
		ORDER BY created_at DESC
	)");

	if (!query.exec()) {
		setError(error, query);
		return zones;
	}

	while (query.next()) {
		zones.push_back(readZone(query));
	}

	return zones;
}

bool AlertRepository::insertEvent(AlertEvent &event, QString *error)
{
	if (event.id.isNull()) {
		event.id = QUuid::createUuid();
	}

	if (event.vesselId.isNull() || event.alertZoneId.isNull()) {
		if (error) {
			*error = QStringLiteral("vesselId and alertZoneId are required");
		}
		return false;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return false;
	}

	QSqlQuery query(m_connection.database());
	if (event.position.has_value()) {
		query.prepare(R"(
			INSERT INTO app.alert_events (id, vessel_id, alert_zone_id, event_type, created_at, position)
			VALUES (
				:id,
				CAST(:vessel_id AS uuid),
				CAST(:alert_zone_id AS uuid),
				:event_type,
				COALESCE(:created_at, now()),
				ST_SetSRID(ST_MakePoint(:longitude, :latitude), 4326)::geography
			)
		)");
		query.bindValue(QStringLiteral(":longitude"), event.position->longitude);
		query.bindValue(QStringLiteral(":latitude"), event.position->latitude);
	} else {
		query.prepare(R"(
			INSERT INTO app.alert_events (id, vessel_id, alert_zone_id, event_type, created_at, position)
			VALUES (
				:id,
				CAST(:vessel_id AS uuid),
				CAST(:alert_zone_id AS uuid),
				:event_type,
				COALESCE(:created_at, now()),
				NULL
			)
		)");
	}

	query.bindValue(QStringLiteral(":id"), uuidToString(event.id));
	query.bindValue(QStringLiteral(":vessel_id"), uuidToString(event.vesselId));
	query.bindValue(QStringLiteral(":alert_zone_id"), uuidToString(event.alertZoneId));
	query.bindValue(QStringLiteral(":event_type"), event.eventType);
	query.bindValue(QStringLiteral(":created_at"), event.createdAt.isValid() ? event.createdAt : QVariant());

	if (!query.exec()) {
		return setError(error, query);
	}

	return true;
}

std::optional<AlertEvent> AlertRepository::findEventById(const QUuid &id, QString *error) const
{
	if (id.isNull()) {
		return std::nullopt;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return std::nullopt;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT
			id,
			vessel_id,
			alert_zone_id,
			event_type,
			created_at,
			CASE
				WHEN position IS NULL THEN NULL
				ELSE ST_AsText(position::geometry)
			END AS position_wkt
		FROM app.alert_events
		WHERE id = CAST(:id AS uuid)
		LIMIT 1
	)");
	query.bindValue(QStringLiteral(":id"), uuidToString(id));

	if (!query.exec()) {
		setError(error, query);
		return std::nullopt;
	}

	if (!query.next()) {
		return std::nullopt;
	}

	return readEvent(query);
}

QVector<AlertEvent> AlertRepository::listEvents(int limit, QString *error) const
{
	QVector<AlertEvent> events;

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return events;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT
			id,
			vessel_id,
			alert_zone_id,
			event_type,
			created_at,
			CASE
				WHEN position IS NULL THEN NULL
				ELSE ST_AsText(position::geometry)
			END AS position_wkt
		FROM app.alert_events
		ORDER BY created_at DESC
		LIMIT :limit
	)");
	query.bindValue(QStringLiteral(":limit"), limit < 1 ? 1 : limit);

	if (!query.exec()) {
		setError(error, query);
		return events;
	}

	while (query.next()) {
		events.push_back(readEvent(query));
	}

	return events;
}

bool AlertRepository::saveShipZoneState(const QUuid &vesselId, const QUuid &zoneId, bool isInside, QString *error)
{
	if (vesselId.isNull() || zoneId.isNull()) {
		if (error) {
			*error = QStringLiteral("vesselId and zoneId are required");
		}
		return false;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return false;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		INSERT INTO app.ship_zone_state (ship_id, zone_id, is_inside, last_changed_at)
		VALUES (CAST(:ship_id AS uuid), CAST(:zone_id AS uuid), :is_inside, now())
		ON CONFLICT (ship_id, zone_id) DO UPDATE SET
			is_inside = EXCLUDED.is_inside,
			last_changed_at = now()
	)");
	query.bindValue(QStringLiteral(":ship_id"), uuidToString(vesselId));
	query.bindValue(QStringLiteral(":zone_id"), uuidToString(zoneId));
	query.bindValue(QStringLiteral(":is_inside"), isInside);

	if (!query.exec()) {
		return setError(error, query);
	}

	return true;
}

QHash<QString, bool> AlertRepository::loadShipZoneStates(QString *error) const
{
	QHash<QString, bool> states;

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return states;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT ship_id, zone_id, is_inside FROM app.ship_zone_state
	)");

	if (!query.exec()) {
		setError(error, query);
		return states;
	}

	while (query.next()) {
		QString shipIdStr = query.value(0).toString();
		QString zoneIdStr = query.value(1).toString();
		bool isInside = query.value(2).toBool();

		// Tạo key dạng shipId_zoneId
		QString key = shipIdStr + "_" + zoneIdStr;
		states.insert(key, isInside);
	}

	return states;
}
