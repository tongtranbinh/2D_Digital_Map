#include "AlertService.h"
#include "../database/AlertRepository.h"

AlertService::AlertService(IAlertRepository &repository)
    : m_repository(repository)
{
}

bool AlertService::saveZone(AlertZone &zone, QString *error)
{
    m_zonesLoaded = false;
    m_zonesCache.clear();
    return m_repository.saveZone(zone, error);
}

std::optional<AlertZone> AlertService::getZoneById(const QUuid &id, QString *error) const
{
    return m_repository.findZoneById(id, error);
}

QVector<AlertZone> AlertService::getAllZones(QString *error) const
{
    if (m_zonesLoaded) {
        return m_zonesCache;
    }

    m_zonesCache = m_repository.listZones(error);
    m_zonesLoaded = true;
    return m_zonesCache;
}

bool AlertService::logEvent(AlertEvent &event, QString *error)
{
    return m_repository.insertEvent(event, error);
}

QVector<AlertEvent> AlertService::getRecentEvents(int limit, QString *error) const
{
    return m_repository.listEvents(limit, error);
}

bool AlertService::isPointInPolygon(const GeoPoint &point, const QVector<GeoPoint> &polygon) const
{
    if (polygon.size() < 3) {
        return false;
    }

    int cn = 0;
    int n = polygon.size();
    for (int i = 0; i < n; i++) {
        const GeoPoint &p1 = polygon[i];
        const GeoPoint &p2 = polygon[(i + 1) % n];

        if (((p1.latitude <= point.latitude) && (p2.latitude > point.latitude))
            || ((p1.latitude > point.latitude) && (p2.latitude <= point.latitude))) {
            double vt = (point.latitude - p1.latitude) / (p2.latitude - p1.latitude);
            if (point.longitude < p1.longitude + vt * (p2.longitude - p1.longitude)) {
                cn++;
            }
        }
    }
    return (cn & 1) != 0;
}

void AlertService::ensureStatesLoaded(QString *error) const
{
    if (m_statesLoaded) {
        return;
    }

    m_shipZoneStates = m_repository.loadShipZoneStates(error);
    m_statesLoaded = true;
}

bool AlertService::saveShipZoneState(const QUuid &vesselId, const QUuid &zoneId, bool isInside, QString *error)
{
    ensureStatesLoaded(error);
    if (m_repository.saveShipZoneState(vesselId, zoneId, isInside, error)) {
        QString key = vesselId.toString() + "_" + zoneId.toString();
        m_shipZoneStates.insert(key, isInside);
        return true;
    }
    return false;
}

bool AlertService::getVesselZoneState(const QUuid &vesselId, const QUuid &zoneId, QString *error) const
{
    ensureStatesLoaded(error);
    QString key = vesselId.toString() + "_" + zoneId.toString();
    auto it = m_shipZoneStates.constFind(key);
    if (it != m_shipZoneStates.constEnd()) {
        return *it;
    }
    return false; // Mặc định trước đó là nằm ngoài vùng (outside)
}

void AlertService::preloadCache(QString *error)
{
    // Nạp trước danh sách các vùng
    getAllZones(error);
    // Nạp trước danh sách các trạng thái tàu trong vùng
    ensureStatesLoaded(error);
}
