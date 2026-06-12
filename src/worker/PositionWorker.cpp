#include "PositionWorker.h"
#include "../state/ShipStateStore.h"
#include "ShipJsonParser.h"
#include <QDebug>

PositionWorker::PositionWorker(ShipStateStore &stateStore, QObject *parent)
    : QObject(parent), m_stateStore(stateStore)
{
}

void PositionWorker::startTimer()
{
    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &PositionWorker::flushPendingPackets);
        m_timer->start(30000); // 30 giây
        qInfo() << "[PositionWorker] Timer 30s started in background thread.";
    }
    if (!m_batchTimer) {
        m_batchTimer = new QTimer(this);
        connect(m_batchTimer, &QTimer::timeout, this, &PositionWorker::processBatch);
        m_batchTimer->start(200); // 200 miligiây
        qInfo() << "[PositionWorker] Timer 200ms batching started in background thread.";
    }
}

void PositionWorker::processRawMessage(const QByteArray &rawMessage)
{
    ShipMessage msg;
    QString error;

    if (!parseShipJson(rawMessage, msg, error)) {
        emit parsingError(error, rawMessage);
        return;
    }

    // Gom dữ liệu thô đã parse vào buffer để xử lý theo lô 200ms
    m_incomingBuffer.insert(msg.shipId, msg);
}

void PositionWorker::processBatch()
{
    if (m_incomingBuffer.isEmpty()) {
        return;
    }

    // 1. Chuyển đổi dữ liệu trong buffer sang QVector để bulk update lên RAM
    QVector<ShipMessage> batch;
    batch.reserve(m_incomingBuffer.size());
    for (const ShipMessage &msg : m_incomingBuffer) {
        batch.push_back(msg);
    }

    // 2. Cập nhật đồng loạt vị trí mới nhất của lô tàu lên RAM (chỉ lock/COW 1 lần duy nhất)
    m_stateStore.updatePositions(batch);
    emit positionsUpdated(batch);

    // 3. Thực hiện kiểm tra vùng địa lý (Geofencing) cho lô tàu vừa nhận
    QVector<AlertZone> zones = m_stateStore.getAlertZones();
    QHash<QString, bool> zoneStateUpdates;

    for (const ShipMessage &msg : batch) {
        GeoPoint currentPoint{msg.longitude, msg.latitude};

        for (const AlertZone &zone : zones) {
            if (!zone.enabled) {
                continue;
            }

            bool currentlyInside = isPointInPolygon(currentPoint, zone.polygon);
            bool previouslyInside = m_stateStore.getShipZoneState(msg.shipId, zone.id);

            if (!previouslyInside && currentlyInside) {
                // Tàu ĐI VÀO vùng cảnh báo (ENTER)
                AlertEvent event;
                event.id = QUuid::createUuid();
                event.vesselId = msg.shipId;
                event.alertZoneId = zone.id;
                event.eventType = QStringLiteral("ENTER");
                event.createdAt = msg.timestamp;
                event.position = currentPoint;

                m_pendingAlertEvents.push_back(event);
                zoneStateUpdates.insert(msg.shipId.toString() + "_" + zone.id.toString(), true);

                emit alertEventOccurred(event);
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

                m_pendingAlertEvents.push_back(event);
                zoneStateUpdates.insert(msg.shipId.toString() + "_" + zone.id.toString(), false);
                emit alertEventOccurred(event);
            }
        }

        // 4. Lưu lại vào buffer 30 giây chờ đẩy xuống CSDL
        m_pendingPackets.insert(msg.shipId, msg);
    }

    // Cập nhật đồng loạt trạng thái tàu trong zone lên RAM (chỉ lock/COW 1 lần duy nhất)
    m_stateStore.updateShipZoneStates(zoneStateUpdates);

    // 5. Xóa sạch buffer 200ms
    m_incomingBuffer.clear();
}

void PositionWorker::flushPendingPackets()
{
    if (m_pendingPackets.isEmpty() && m_pendingAlertEvents.isEmpty()) {
        return;
    }

    qInfo() << QStringLiteral("[PositionWorker] 30 seconds reached. Flushing %1 unique pending packets and %2 alert events to ShipWorker (DB)...")
               .arg(m_pendingPackets.size())
               .arg(m_pendingAlertEvents.size());

    // Chuyển danh sách từ QHash sang QVector để gửi tín hiệu hàng loạt
    QVector<ShipMessage> packets;
    packets.reserve(m_pendingPackets.size());
    for (const ShipMessage &msg : m_pendingPackets) {
        packets.push_back(msg);
    }

    QVector<AlertEvent> events = m_pendingAlertEvents;

    // Phát tín hiệu gửi lô dữ liệu
    emit pendingPacketsReady(packets, events);

    // Xóa cache chờ
    m_pendingPackets.clear();
    m_pendingAlertEvents.clear();
}

bool PositionWorker::isPointInPolygon(const GeoPoint &point, const QVector<GeoPoint> &polygon) const
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
