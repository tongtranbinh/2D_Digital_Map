#include "TrackingService.h"
#include "ShipService.h"
#include "PositionService.h"
#include "AlertService.h"
#include "../model/Vessel.h"
#include "../model/AlertZone.h"
#include <QDebug>

TrackingService::TrackingService(IShipService &shipService, IPositionService &positionService, IAlertService &alertService)
    : m_shipService(shipService)
    , m_positionService(positionService)
    , m_alertService(alertService)
{
}

bool TrackingService::processTrackingMessage(ShipMessage &msg, QString *error)
{
    // 1. Kiểm tra nhanh sự tồn tại của tàu qua RAM cache (Không query DB nếu đã tồn tại)
    if (!m_shipService.shipExists(msg.shipId, error)) {
        Vessel newVessel;
        newVessel.id = msg.shipId;
        newVessel.name = QStringLiteral("Unknown Vessel");
        // mmsi là trường UNIQUE NOT NULL trong DB, tự động sinh số mmsi 9 chữ số duy nhất dựa trên hash của shipId
        newVessel.mmsi = 100000000 + (qHash(msg.shipId) % 900000000);
        if (!m_shipService.saveShip(newVessel, error)) {
            return false;
        }
    }

    // 2. Thực hiện kiểm tra vùng địa lý và quản lý chuyển đổi trạng thái bằng RAM Cache
    QVector<AlertZone> zones = m_alertService.getAllZones(error);
    GeoPoint currentPoint{msg.longitude, msg.latitude};

    for (const AlertZone &zone : zones) {
        if (!zone.enabled) {
            continue;
        }

        // Kiểm tra xem vị trí hiện tại có nằm trong đa giác cảnh báo không
        bool currentlyInside = m_alertService.isPointInPolygon(currentPoint, zone.polygon);

        // Lấy trạng thái trước đó của tàu trong vùng này từ RAM cache
        bool previouslyInside = m_alertService.getVesselZoneState(msg.shipId, zone.id, error);

        // Phát hiện chuyển đổi trạng thái di chuyển của tàu
        if (!previouslyInside && currentlyInside) {
            // Tàu ĐI VÀO vùng cảnh báo (ENTER)
            AlertEvent event;
            event.id = QUuid::createUuid();
            event.vesselId = msg.shipId;
            event.alertZoneId = zone.id;
            event.eventType = QStringLiteral("ENTER");
            event.createdAt = msg.timestamp;
            event.position = currentPoint;

            if (m_alertService.logEvent(event, error)) {
                // Cập nhật trạng thái mới lên RAM và DB
                m_alertService.saveShipZoneState(msg.shipId, zone.id, true, error);
                qInfo() << QStringLiteral("[Geofencing] Vessel %1 ENTERED alert zone: %2")
                           .arg(msg.shipId.toString())
                           .arg(zone.name);
            }
        }
        else if (previouslyInside && !currentlyInside) {
            // Tàu ĐI RA khỏi vùng cảnh báo (EXIT)
            AlertEvent event;
            event.id = QUuid::createUuid();
            event.vesselId = msg.shipId;
            event.alertZoneId = zone.id;
            event.eventType = QStringLiteral("EXIT");
            event.createdAt = msg.timestamp;
            event.position = currentPoint;

            if (m_alertService.logEvent(event, error)) {
                // Cập nhật trạng thái mới lên RAM và DB
                m_alertService.saveShipZoneState(msg.shipId, zone.id, false, error);
                qInfo() << QStringLiteral("[Geofencing] Vessel %1 EXITED alert zone: %2")
                           .arg(msg.shipId.toString())
                           .arg(zone.name);
            }
        }
    }

    // 3. Lưu vị trí mới nhận được xuống cơ sở dữ liệu sau khi đã xử lý xong cảnh báo
    if (!m_positionService.recordPosition(msg, error)) {
        return false;
    }

    return true;
}

bool TrackingService::processTrackingMessages(QVector<ShipMessage> &msgs, QString *error)
{
    // 1. Kiểm tra nhanh sự tồn tại của các tàu qua RAM cache (và insert nếu chưa có)
    for (ShipMessage &msg : msgs) {
        if (!m_shipService.shipExists(msg.shipId, error)) {
            Vessel newVessel;
            newVessel.id = msg.shipId;
            newVessel.name = QStringLiteral("Unknown Vessel");
            newVessel.mmsi = 100000000 + (qHash(msg.shipId) % 900000000);
            if (!m_shipService.saveShip(newVessel, error)) {
                return false;
            }
        }
    }

    // 2. Thực hiện kiểm tra vùng địa lý cho từng bản tin
    QVector<AlertZone> zones = m_alertService.getAllZones(error);
    for (const ShipMessage &msg : msgs) {
        GeoPoint currentPoint{msg.longitude, msg.latitude};
        for (const AlertZone &zone : zones) {
            if (!zone.enabled) {
                continue;
            }

            bool currentlyInside = m_alertService.isPointInPolygon(currentPoint, zone.polygon);
            bool previouslyInside = m_alertService.getVesselZoneState(msg.shipId, zone.id, error);

            if (!previouslyInside && currentlyInside) {
                AlertEvent event;
                event.id = QUuid::createUuid();
                event.vesselId = msg.shipId;
                event.alertZoneId = zone.id;
                event.eventType = QStringLiteral("ENTER");
                event.createdAt = msg.timestamp;
                event.position = currentPoint;

                if (m_alertService.logEvent(event, error)) {
                    m_alertService.saveShipZoneState(msg.shipId, zone.id, true, error);
                    qInfo() << QStringLiteral("[Geofencing] Vessel %1 ENTERED alert zone: %2")
                               .arg(msg.shipId.toString())
                               .arg(zone.name);
                }
            }
            else if (previouslyInside && !currentlyInside) {
                AlertEvent event;
                event.id = QUuid::createUuid();
                event.vesselId = msg.shipId;
                event.alertZoneId = zone.id;
                event.eventType = QStringLiteral("EXIT");
                event.createdAt = msg.timestamp;
                event.position = currentPoint;

                if (m_alertService.logEvent(event, error)) {
                    m_alertService.saveShipZoneState(msg.shipId, zone.id, false, error);
                    qInfo() << QStringLiteral("[Geofencing] Vessel %1 EXITED alert zone: %2")
                               .arg(msg.shipId.toString())
                               .arg(zone.name);
                }
            }
        }
    }

    // 3. Ghi hàng loạt vị trí xuống DB (Batch Insert)
    if (!m_positionService.recordPositionsBatch(msgs, error)) {
        return false;
    }

    return true;
}
