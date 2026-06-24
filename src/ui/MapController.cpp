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
    bool selectedShipTrackUpdated = false;
    for (const auto &msg : positions) {
        QGeoCoordinate coord(msg.latitude, msg.longitude);
        auto &history = m_trackHistories[msg.shipId];

        // Tránh thêm điểm trùng lặp kế tiếp
        if (history.isEmpty() || history.last() != coord) {
            history.push_back(coord);
            if (history.size() > 100) {
                history.removeFirst(); // Giới hạn 100 điểm gần nhất trong RAM
            }
            if (msg.shipId == m_selectedShipId) {
                selectedShipTrackUpdated = true;
            }
        }
    }
    if (selectedShipTrackUpdated && !m_selectedShipId.isNull()) {
        emit trackHistoryUpdated(m_selectedShipId.toString());
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

void MapController::addAlertZone(const QString &name, const QString &description, const QVariantList &coordinates)
{
    if (coordinates.size() < 3) {
        qWarning() << "[MapController] Cannot add alert zone with less than 3 points";
        return;
    }

    AlertZone newZone;
    newZone.id = QUuid::createUuid();
    newZone.name = name;
    newZone.description = description;
    newZone.enabled = true;

    for (const auto &var : coordinates) {
        QGeoCoordinate geoCoord = var.value<QGeoCoordinate>();
        if (geoCoord.isValid()) {
            newZone.polygon.push_back(GeoPoint{geoCoord.longitude(), geoCoord.latitude()});
        }
    }

    // Để khép kín đa giác theo yêu cầu địa lý, điểm cuối phải trùng điểm đầu
    if (!newZone.polygon.isEmpty()) {
        const auto &first = newZone.polygon.first();
        const auto &last = newZone.polygon.last();
        if (first.longitude != last.longitude || first.latitude != last.latitude) {
            newZone.polygon.push_back(first);
        }
    }

    // 1. Cập nhật vào RAM DB (ShipStateStore)
    QVector<AlertZone> zones = m_stateStore.getAlertZones();
    zones.push_back(newZone);
    m_stateStore.setAlertZones(zones);

    // 2. Cập nhật danh sách ID vùng hoạt động
    m_activeZoneIds.push_back(newZone.id);

    // 3. Cập nhật mô hình hiển thị của UI (ZoneListModel)
    m_zoneModel->setZones(zones);

    // 4. Phát tín hiệu yêu cầu ShipWorker lưu vào CSDL PostgreSQL
    emit requestSaveZone(newZone);

    qInfo() << "[MapController] Added new alert zone:" << name << "with" << newZone.polygon.size() << "points.";
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

QString MapController::selectedShipId() const
{
    return m_selectedShipId.toString();
}

void MapController::setSelectedShipId(const QString &id)
{
    QUuid uuid = QUuid::fromString(id);
    if (uuid.isNull() && !id.isEmpty()) {
        uuid = QUuid::fromString("{" + id + "}");
    }
    if (m_selectedShipId != uuid) {
        m_selectedShipId = uuid;
        emit selectedShipIdChanged();
    }
}
