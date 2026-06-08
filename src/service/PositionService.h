#pragma once

#include <optional>
#include <QString>
#include <QVector>
#include <QUuid>

#include "../model/VesselMessage.h"

class IPositionRepository;

class IPositionService
{
public:
    virtual ~IPositionService() = default;
    virtual bool recordPosition(ShipMessage &position, QString *error = nullptr) = 0;
    virtual bool recordPositionsBatch(const QVector<ShipMessage> &positions, QString *error = nullptr) = 0;
    virtual std::optional<ShipMessage> getLatestPosition(const QUuid &vesselId, QString *error = nullptr) const = 0;
    virtual QVector<ShipMessage> getPositionHistory(const QUuid &vesselId, int limit = 100, QString *error = nullptr) const = 0;
};

class PositionService final : public IPositionService
{
public:
    explicit PositionService(IPositionRepository &repository);

    bool recordPosition(ShipMessage &position, QString *error = nullptr) override;
    bool recordPositionsBatch(const QVector<ShipMessage> &positions, QString *error = nullptr) override;
    std::optional<ShipMessage> getLatestPosition(const QUuid &vesselId, QString *error = nullptr) const override;
    QVector<ShipMessage> getPositionHistory(const QUuid &vesselId, int limit = 100, QString *error = nullptr) const override;

private:
    IPositionRepository &m_repository;
};
