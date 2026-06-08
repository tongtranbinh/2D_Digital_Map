#pragma once

#include <optional>

#include <QString>
#include <QVector>

#include "PostgresConnection.h"
#include "../model/VesselMessage.h"

class IPositionRepository
{
public:
    virtual ~IPositionRepository() = default;

    virtual bool insert(ShipMessage &position, QString *error = nullptr) = 0;
    virtual bool insertBatch(const QVector<ShipMessage> &positions, QString *error = nullptr) = 0;
    virtual std::optional<ShipMessage> findLatestByVesselId(const QUuid &vesselId, QString *error = nullptr) const = 0;
    virtual QVector<ShipMessage> listByVesselId(const QUuid &vesselId, int limit = 100, QString *error = nullptr) const = 0;
    virtual QVector<ShipMessage> loadLatestPositions(QString *error = nullptr) const = 0;
};

class PositionRepository final : public IPositionRepository
{
public:
    explicit PositionRepository(PostgresConnection &connection);

    bool insert(ShipMessage &position, QString *error = nullptr) override;
    bool insertBatch(const QVector<ShipMessage> &positions, QString *error = nullptr) override;
    std::optional<ShipMessage> findLatestByVesselId(const QUuid &vesselId, QString *error = nullptr) const override;
    QVector<ShipMessage> listByVesselId(const QUuid &vesselId, int limit = 100, QString *error = nullptr) const override;
    QVector<ShipMessage> loadLatestPositions(QString *error = nullptr) const override;

private:
    PostgresConnection &m_connection;
};
