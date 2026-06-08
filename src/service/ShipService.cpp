#include "ShipService.h"
#include "../database/ShipRepository.h"

ShipService::ShipService(IShipRepository &repository)
    : m_repository(repository)
{
}

void ShipService::ensureCacheLoaded(QString *error) const
{
    if (m_cacheLoaded) {
        return;
    }

    QVector<Vessel> vessels = m_repository.listAll(error);
    for (const Vessel &v : vessels) {
        m_registeredIds.insert(v.id);
    }
    m_cacheLoaded = true;
}

bool ShipService::saveShip(Vessel &vessel, QString *error)
{
    ensureCacheLoaded(error);
    if (m_repository.save(vessel, error)) {
        m_registeredIds.insert(vessel.id);
        return true;
    }
    return false;
}

std::optional<Vessel> ShipService::getShipById(const QUuid &id, QString *error) const
{
    ensureCacheLoaded(error);
    if (m_registeredIds.contains(id)) {
        // Nếu đã nằm trong cache, ta có thể trả về một đối tượng Vessel tượng trưng có ID 
        // để tránh phải SELECT DB (với nghiệp vụ kiểm tra tồn tại thông thường)
        Vessel v;
        v.id = id;
        return v;
    }
    return m_repository.findById(id, error);
}

std::optional<Vessel> ShipService::getShipByMmsi(qint64 mmsi, QString *error) const
{
    return m_repository.findByMmsi(mmsi, error);
}

QVector<Vessel> ShipService::getAllShips(QString *error) const
{
    ensureCacheLoaded(error);
    return m_repository.listAll(error);
}

bool ShipService::shipExists(const QUuid &id, QString *error) const
{
    ensureCacheLoaded(error);
    return m_registeredIds.contains(id);
}
