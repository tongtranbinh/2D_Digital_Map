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

    QStringList valuePlaceholders;
    valuePlaceholders.reserve(positions.size());

    for (int i = 0; i < positions.size(); ++i) {
        valuePlaceholders << QString(
            "(:id%1, CAST(:vessel_id%1 AS uuid), COALESCE(:recorded_at%1, now()), "
            ":latitude%1, :longitude%1, :speed%1, :course%1, :heading%1, "
            "ST_SetSRID(ST_MakePoint(:longitude%1, :latitude%1), 4326)::geography)"
        ).arg(i);
    }

    QString sql = QString(R"(
        INSERT INTO app.positions (
            id,
            vessel_id,
            recorded_at,
            latitude,
            longitude,
            speed_knots,
            course,
            heading,
            geom
        )
        VALUES %1
    )").arg(valuePlaceholders.join(","));

    query.prepare(sql);

    for (int i = 0; i < positions.size(); ++i) {
        const ShipMessage &pos = positions[i];

        QUuid pid = pos.id.isNull() ? QUuid::createUuid() : pos.id;

        query.bindValue(QString(":id%1").arg(i), uuidToString(pid));
        query.bindValue(QString(":vessel_id%1").arg(i), uuidToString(pos.shipId));

        if (pos.timestamp.isValid()) {
            query.bindValue(QString(":recorded_at%1").arg(i), pos.timestamp);
        } else {
            query.bindValue(QString(":recorded_at%1").arg(i), QVariant());
        }

        query.bindValue(QString(":latitude%1").arg(i), pos.latitude);
        query.bindValue(QString(":longitude%1").arg(i), pos.longitude);
        query.bindValue(QString(":speed%1").arg(i), pos.speed);
        query.bindValue(QString(":course%1").arg(i), pos.course);
        query.bindValue(QString(":heading%1").arg(i), static_cast<int>(pos.heading));
    }

    if (!query.exec()) {
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
