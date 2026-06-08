#include "PositionService.h"
#include "../database/PositionRepository.h"

PositionService::PositionService(IPositionRepository &repository)
    : m_repository(repository)
{
}

bool PositionService::recordPosition(ShipMessage &position, QString *error)
{
    return m_repository.insert(position, error);
}

bool PositionService::recordPositionsBatch(const QVector<ShipMessage> &positions, QString *error)
{
    return m_repository.insertBatch(positions, error);
}

std::optional<ShipMessage> PositionService::getLatestPosition(const QUuid &vesselId, QString *error) const
{
    return m_repository.findLatestByVesselId(vesselId, error);
}

QVector<ShipMessage> PositionService::getPositionHistory(const QUuid &vesselId, int limit, QString *error) const
{
    return m_repository.listByVesselId(vesselId, limit, error);
}
