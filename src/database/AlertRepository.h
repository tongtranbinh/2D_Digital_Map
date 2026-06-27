#pragma once

#include <optional>

#include <QString>
#include <QVector>

#include "PostgresConnection.h"
#include "../model/AlertZone.h"
#include "../model/AlertMessage.h"

class IAlertRepository
{
public:
    virtual ~IAlertRepository() = default;

    virtual bool saveZone(AlertZone &zone, QString *error = nullptr) = 0;
    virtual bool deleteZone(const QUuid &id, QString *error = nullptr) = 0;
    virtual std::optional<AlertZone> findZoneById(const QUuid &id, QString *error = nullptr) const = 0;
    virtual QVector<AlertZone> listZones(QString *error = nullptr) const = 0;

    virtual bool insertEvent(AlertEvent &event, QString *error = nullptr) = 0;
    virtual std::optional<AlertEvent> findEventById(const QUuid &id, QString *error = nullptr) const = 0;
    virtual QVector<AlertEvent> listEvents(int limit = 100, QString *error = nullptr) const = 0;

    // Quản lý trạng thái tàu trong/ngoài vùng (ship_zone_state)
    virtual bool saveShipZoneState(const QUuid &vesselId, const QUuid &zoneId, bool isInside, QString *error = nullptr) = 0;
    virtual QHash<QString, bool> loadShipZoneStates(QString *error = nullptr) const = 0;
};

class AlertRepository final : public IAlertRepository
{
public:
    explicit AlertRepository(PostgresConnection &connection);

    bool saveZone(AlertZone &zone, QString *error = nullptr) override;
    bool deleteZone(const QUuid &id, QString *error = nullptr) override;
    std::optional<AlertZone> findZoneById(const QUuid &id, QString *error = nullptr) const override;
    QVector<AlertZone> listZones(QString *error = nullptr) const override;

    bool insertEvent(AlertEvent &event, QString *error = nullptr) override;
    std::optional<AlertEvent> findEventById(const QUuid &id, QString *error = nullptr) const override;
    QVector<AlertEvent> listEvents(int limit = 100, QString *error = nullptr) const override;

    bool saveShipZoneState(const QUuid &vesselId, const QUuid &zoneId, bool isInside, QString *error = nullptr) override;
    QHash<QString, bool> loadShipZoneStates(QString *error = nullptr) const override;

private:
    PostgresConnection &m_connection;
};
