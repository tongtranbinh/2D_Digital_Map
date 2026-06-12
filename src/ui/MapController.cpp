#include "MapController.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

MapController::MapController(ShipStateStore &stateStore, QObject *parent)
    : QObject(parent), m_stateStore(stateStore)
{
    m_shipModel = new ShipListModel(this);
    m_zoneModel = new ZoneListModel(this);
}


void MapController::initZones()
{
    QVector<AlertZone> zones = m_stateStore.getAlertZones();
    m_activeZoneIds.clear();
    m_activeZoneIds.reserve(zones.size());
    for (const auto &zone : zones) {
        if (zone.enabled) {
            m_activeZoneIds.push_back(zone.id);
        }
    }
    m_zoneModel->setZones(zones);
    qInfo() << QStringLiteral("[MapController] Initialized %1 alert zones from RAM Cache.").arg(zones.size());
}

void MapController::handlePositionsUpdated(const QVector<ShipMessage> &positions)
{
    // 1. Cập nhật lịch sử hành trình trong RAM
    for (const auto &msg : positions) {
        QGeoCoordinate coord(msg.latitude, msg.longitude);
        auto &history = m_trackHistories[msg.shipId];

        // Tránh thêm điểm trùng lặp kế tiếp
        if (history.isEmpty() || history.last() != coord) {
            history.push_back(coord);
            if (history.size() > 100) {
                history.removeFirst(); // Giới hạn 100 điểm gần nhất trong RAM
            }
            emit trackHistoryUpdated(msg.shipId.toString());
        }
    }

    // 2. Lấy trạng thái Geofence trực tiếp từ RAM (ShipStateStore)
    QHash<QString, bool> zoneStates;
    for (const auto &msg : positions) {
        QString shipIdStr = msg.shipId.toString();
        for (const auto &zoneId : m_activeZoneIds) {
            QString key = shipIdStr + "_" + zoneId.toString();
            // Đọc lock-free từ RAM
            bool inside = m_stateStore.getShipZoneState(msg.shipId, zoneId);
            zoneStates.insert(key, inside);
        }
    }

    // 3. Cập nhật mô hình dữ liệu tàu
    m_shipModel->updateShipPositions(positions, m_vesselCache, zoneStates, m_activeZoneIds);
}

void MapController::handleAlertEvent(const AlertEvent &event)
{
    // 1. Tìm tên tàu từ cache
    QString shipName = QStringLiteral("Vessel %1").arg(event.vesselId.toString().mid(1, 5).toUpper());
    if (m_vesselCache.contains(event.vesselId)) {
        shipName = m_vesselCache[event.vesselId].name;
    }

    // 2. Tìm tên vùng cảnh báo từ RAM
    QString zoneName = QStringLiteral("Zone %1").arg(event.alertZoneId.toString().mid(1, 5).toUpper());
    QVector<AlertZone> zones = m_stateStore.getAlertZones();
    for (const auto &zone : zones) {
        if (zone.id == event.alertZoneId) {
            zoneName = zone.name;
            break;
        }
    }

    QString timeStr = event.createdAt.toLocalTime().toString(QStringLiteral("hh:mm:ss dd/MM"));


    // 3. Phát tín hiệu cho QML vẽ banner hoặc danh sách cảnh báo
    emit vesselAlertTriggered(event.vesselId.toString(), shipName,
                              event.alertZoneId.toString(), zoneName,
                              event.eventType, timeStr);
}

QVariantList MapController::getTrackHistory(const QString &shipId) const
{
    QVariantList list;
    QUuid uuid = QUuid::fromString(shipId);
    if (uuid.isNull() && !shipId.isEmpty()) {
        uuid = QUuid::fromString("{" + shipId + "}");
    }

    if (m_trackHistories.contains(uuid)) {
        const auto &history = m_trackHistories[uuid];
        list.reserve(history.size());
        for (const auto &coord : history) {
            list.append(QVariant::fromValue(coord));
        }
    }
    return list;
}

void MapController::loadTrackHistoryFromDb(const QString &shipId)
{
    QUuid uuid = QUuid::fromString(shipId);
    if (uuid.isNull() && !shipId.isEmpty()) {
        uuid = QUuid::fromString("{" + shipId + "}");
    }
    if (!uuid.isNull()) {
        qInfo() << "[MapController] Requesting DB track history for ship:" << uuid.toString();
        emit requestTrackHistory(uuid);
    }
}

void MapController::handleTrackHistoryLoaded(const QUuid &vesselId, const QVector<ShipMessage> &history)
{
    QVector<QGeoCoordinate> coords;
    coords.reserve(history.size());

    // listByVesselId trả về mới nhất trước (DESC), đảo ngược lại để vẽ theo trình tự thời gian (cũ -> mới)
    for (int i = history.size() - 1; i >= 0; --i) {
        coords.push_back(QGeoCoordinate(history[i].latitude, history[i].longitude));
    }

    m_trackHistories.insert(vesselId, coords);
    qInfo() << "[MapController] Loaded" << coords.size() << "points of history from DB for ship:" << vesselId.toString();
    emit trackHistoryUpdated(vesselId.toString());
}
