#include "ShipRepository.h"

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

Vessel readVessel(const QSqlQuery &query)
{
	Vessel vessel;
	vessel.id = uuidFromString(query.value(QStringLiteral("id")).toString());
	vessel.mmsi = query.value(QStringLiteral("mmsi")).toLongLong();
	vessel.name = query.value(QStringLiteral("name")).toString();
	vessel.callsign = query.value(QStringLiteral("callsign")).toString();
	vessel.imo = query.value(QStringLiteral("imo")).toLongLong();
	vessel.shipType = query.value(QStringLiteral("ship_type")).toInt();
	vessel.createdAt = query.value(QStringLiteral("created_at")).toDateTime();
	vessel.updatedAt = query.value(QStringLiteral("updated_at")).toDateTime();
	return vessel;
}

} // namespace

ShipRepository::ShipRepository(PostgresConnection &connection)
	: m_connection(connection)
{
}

bool ShipRepository::save(Vessel &vessel, QString *error)
{
	if (vessel.id.isNull()) {
		vessel.id = QUuid::createUuid();
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return false;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		INSERT INTO app.vessels (id, mmsi, name, callsign, imo, ship_type, created_at, updated_at)
		VALUES (:id, :mmsi, :name, :callsign, :imo, :ship_type, COALESCE(:created_at, now()), COALESCE(:updated_at, now()))
		ON CONFLICT (id) DO UPDATE SET
			mmsi = EXCLUDED.mmsi,
			name = EXCLUDED.name,
			callsign = EXCLUDED.callsign,
			imo = EXCLUDED.imo,
			ship_type = EXCLUDED.ship_type,
			updated_at = now()
	)");

	query.bindValue(QStringLiteral(":id"), uuidToString(vessel.id));
	query.bindValue(QStringLiteral(":mmsi"), vessel.mmsi);
	query.bindValue(QStringLiteral(":name"), vessel.name);
	query.bindValue(QStringLiteral(":callsign"), vessel.callsign);
	query.bindValue(QStringLiteral(":imo"), vessel.imo);
	query.bindValue(QStringLiteral(":ship_type"), vessel.shipType);
	query.bindValue(QStringLiteral(":created_at"), vessel.createdAt.isValid() ? vessel.createdAt : QVariant());
	query.bindValue(QStringLiteral(":updated_at"), vessel.updatedAt.isValid() ? vessel.updatedAt : QVariant());

	if (!query.exec()) {
		return setError(error, query);
	}

	return true;
}

std::optional<Vessel> ShipRepository::findById(const QUuid &id, QString *error) const
{
	if (id.isNull()) {
		return std::nullopt;
	}

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return std::nullopt;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT id, mmsi, name, callsign, imo, ship_type, created_at, updated_at
		FROM app.vessels
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

	return readVessel(query);
}

std::optional<Vessel> ShipRepository::findByMmsi(qint64 mmsi, QString *error) const
{
	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return std::nullopt;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT id, mmsi, name, callsign, imo, ship_type, created_at, updated_at
		FROM app.vessels
		WHERE mmsi = :mmsi
		LIMIT 1
	)");
	query.bindValue(QStringLiteral(":mmsi"), mmsi);

	if (!query.exec()) {
		setError(error, query);
		return std::nullopt;
	}

	if (!query.next()) {
		return std::nullopt;
	}

	return readVessel(query);
}

QVector<Vessel> ShipRepository::listAll(QString *error) const
{
	QVector<Vessel> vessels;

	if (!m_connection.isOpen() && !m_connection.open(error)) {
		return vessels;
	}

	QSqlQuery query(m_connection.database());
	query.prepare(R"(
		SELECT id, mmsi, name, callsign, imo, ship_type, created_at, updated_at
		FROM app.vessels
		ORDER BY created_at DESC
	)");

	if (!query.exec()) {
		setError(error, query);
		return vessels;
	}

	while (query.next()) {
		vessels.push_back(readVessel(query));
	}

	return vessels;
}

