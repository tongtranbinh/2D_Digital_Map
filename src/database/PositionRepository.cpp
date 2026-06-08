#include "PositionRepository.h"

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

ShipMessage readPosition(const QSqlQuery &query)
{
	ShipMessage position;
	position.id = uuidFromString(query.value(QStringLiteral("id")).toString());
	position.shipId = uuidFromString(query.value(QStringLiteral("vessel_id")).toString());
	position.timestamp = query.value(QStringLiteral("recorded_at")).toDateTime();
	position.longitude = query.value(QStringLiteral("longitude")).toDouble();
	position.latitude = query.value(QStringLiteral("latitude")).toDouble();
	position.speed = query.value(QStringLiteral("speed_knots")).toDouble();
	position.course = query.value(QStringLiteral("course")).toDouble();
	position.heading = query.value(QStringLiteral("heading")).toDouble();
	return position;
}

} // namespace

PositionRepository::PositionRepository(PostgresConnection &connection)
	: m_connection(connection)
{
}

bool PositionRepository::insert(ShipMessage &position, QString *error)
{
	if (position.id.isNull()) {
		position.id = QUuid::createUuid();
	}

	if (position.shipId.isNull()) {
		if (error) {
			*error = QStringLiteral("shipId is required");
		}
		return false;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return false;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		INSERT INTO app.positions (id, vessel_id, recorded_at, latitude, longitude, speed_knots, course, heading, geom)
		VALUES (
			:id,
			CAST(:vessel_id AS uuid),
			COALESCE(:recorded_at, now()),
			:latitude,
			:longitude,
			:speed_knots,
			:course,
			:heading,
			ST_SetSRID(ST_MakePoint(:longitude, :latitude), 4326)::geography
		)
	)");

	query.bindValue(QStringLiteral(":id"), uuidToString(position.id));
	query.bindValue(QStringLiteral(":vessel_id"), uuidToString(position.shipId));
	query.bindValue(QStringLiteral(":recorded_at"), position.timestamp.isValid() ? position.timestamp : QVariant());
	query.bindValue(QStringLiteral(":latitude"), position.latitude);
	query.bindValue(QStringLiteral(":longitude"), position.longitude);
	query.bindValue(QStringLiteral(":speed_knots"), position.speed);
	query.bindValue(QStringLiteral(":course"), position.course);
	query.bindValue(QStringLiteral(":heading"), static_cast<int>(position.heading));

	if (!query.exec()) {
		return setError(error, query);
	}

	return true;
}

bool PositionRepository::insertBatch(const QVector<ShipMessage> &positions, QString *error)
{
	if (positions.isEmpty()) {
		return true;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return false;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		INSERT INTO app.positions (id, vessel_id, recorded_at, latitude, longitude, speed_knots, course, heading, geom)
		VALUES (
			:id,
			CAST(:vessel_id AS uuid),
			COALESCE(:recorded_at, now()),
			:latitude,
			:longitude,
			:speed_knots,
			:course,
			:heading,
			ST_SetSRID(ST_MakePoint(:longitude, :latitude), 4326)::geography
		)
	)");

	QVariantList ids;
	QVariantList vesselIds;
	QVariantList recordedAts;
	QVariantList latitudes;
	QVariantList longitudes;
	QVariantList speeds;
	QVariantList courses;
	QVariantList headings;

	ids.reserve(positions.size());
	vesselIds.reserve(positions.size());
	recordedAts.reserve(positions.size());
	latitudes.reserve(positions.size());
	longitudes.reserve(positions.size());
	speeds.reserve(positions.size());
	courses.reserve(positions.size());
	headings.reserve(positions.size());

	for (const ShipMessage &pos : positions) {
		QUuid pid = pos.id.isNull() ? QUuid::createUuid() : pos.id;
		ids << uuidToString(pid);
		vesselIds << uuidToString(pos.shipId);
		recordedAts << (pos.timestamp.isValid() ? pos.timestamp : QVariant());
		latitudes << pos.latitude;
		longitudes << pos.longitude;
		speeds << pos.speed;
		courses << pos.course;
		headings << static_cast<int>(pos.heading);
	}

	query.bindValue(QStringLiteral(":id"), ids);
	query.bindValue(QStringLiteral(":vessel_id"), vesselIds);
	query.bindValue(QStringLiteral(":recorded_at"), recordedAts);
	query.bindValue(QStringLiteral(":latitude"), latitudes);
	query.bindValue(QStringLiteral(":longitude"), longitudes);
	query.bindValue(QStringLiteral(":speed_knots"), speeds);
	query.bindValue(QStringLiteral(":course"), courses);
	query.bindValue(QStringLiteral(":heading"), headings);

	if (!query.execBatch()) {
		return setError(error, query);
	}

	return true;
}

std::optional<ShipMessage> PositionRepository::findLatestByVesselId(const QUuid &vesselId, QString *error) const
{
	if (vesselId.isNull()) {
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
			recorded_at,
			longitude,
			latitude,
			speed_knots,
			course,
			heading
		FROM app.positions
		WHERE vessel_id = CAST(:vessel_id AS uuid)
		ORDER BY recorded_at DESC
		LIMIT 1
	)");
	query.bindValue(QStringLiteral(":vessel_id"), uuidToString(vesselId));

	if (!query.exec()) {
		setError(error, query);
		return std::nullopt;
	}

	if (!query.next()) {
		return std::nullopt;
	}

	return readPosition(query);
}

QVector<ShipMessage> PositionRepository::listByVesselId(const QUuid &vesselId, int limit, QString *error) const
{
	QVector<ShipMessage> positions;

	if (vesselId.isNull()) {
		return positions;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return positions;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT
			id,
			vessel_id,
			recorded_at,
			longitude,
			latitude,
			speed_knots,
			course,
			heading
		FROM app.positions
		WHERE vessel_id = CAST(:vessel_id AS uuid)
		ORDER BY recorded_at DESC
		LIMIT :limit
	)");
	query.bindValue(QStringLiteral(":vessel_id"), uuidToString(vesselId));
	query.bindValue(QStringLiteral(":limit"), limit < 1 ? 1 : limit);

	if (!query.exec()) {
		setError(error, query);
		return positions;
	}

	while (query.next()) {
		positions.push_back(readPosition(query));
	}

	return positions;
}

QVector<ShipMessage> PositionRepository::loadLatestPositions(QString *error) const
{
	QVector<ShipMessage> positions;

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return positions;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT DISTINCT ON (vessel_id)
			id,
			vessel_id,
			recorded_at,
			longitude,
			latitude,
			speed_knots,
			course,
			heading
		FROM app.positions
		ORDER BY vessel_id, recorded_at DESC
	)");

	if (!query.exec()) {
		setError(error, query);
		return positions;
	}

	while (query.next()) {
		positions.push_back(readPosition(query));
	}

	return positions;
}
