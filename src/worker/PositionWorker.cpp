#include "PositionWorker.h"
#include "../state/ShipStateStore.h"
#include "ShipJsonParser.h"
#include <QDebug>
#include <QElapsedTimer>

PositionWorker::PositionWorker(ShipStateStore &stateStore, QObject *parent)
    : QObject(parent), m_stateStore(stateStore)
{
}

PositionWorker::~PositionWorker()
{
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
    if (m_batchTimer) {
        m_batchTimer->stop();
        delete m_batchTimer;
        m_batchTimer = nullptr;
    }
}

void PositionWorker::startTimer()
{
    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &PositionWorker::flushPendingPackets);
        m_timer->start(5000); // 5 giây
        qInfo() << "[PositionWorker] Timer 5s started in background thread.";
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
    QElapsedTimer timer;
    timer.start();

    ShipMessage msg;
    QString error;

    if (!parseShipJson(rawMessage, msg, error)) {
        emit parsingError(error, rawMessage);
        return;
    }

    m_accumulatedParseTimeMs += timer.nsecsElapsed() / 1000000.0;
    m_parsedCount++;

    // Gom dữ liệu thô đã parse vào buffer để xử lý theo lô 200ms
    m_incomingBuffer.insert(msg.shipId, msg);
}

void PositionWorker::processBatch()
{
    if (m_incomingBuffer.isEmpty()) {
        return;
    }

    QElapsedTimer batchTimer;
    batchTimer.start();

    QElapsedTimer cowTimer;
    cowTimer.start();

    // 1. Chuyển đổi dữ liệu trong buffer sang QVector để bulk update lên RAM
    QVector<ShipMessage> batch;
    batch.reserve(m_incomingBuffer.size());
    for (const ShipMessage &msg : m_incomingBuffer) {
        batch.push_back(msg);
    }

    // 2. Cập nhật đồng loạt vị trí mới nhất của lô tàu lên RAM (chỉ lock/COW 1 lần duy nhất)
    m_stateStore.updatePositions(batch);
    double cowPositionsMs = cowTimer.nsecsElapsed() / 1000000.0;

    emit positionsUpdated(batch);

    QElapsedTimer geofenceTimer;
    geofenceTimer.start();

    // 3. Thực hiện kiểm tra vùng địa lý (Geofencing) cho lô tàu vừa nhận
    QVector<AlertZone> zones = m_stateStore.getAlertZones();
    QHash<QString, bool> zoneStateUpdates;

    int geofenceChecks = 0;
    for (const ShipMessage &msg : batch) {
        GeoPoint currentPoint{msg.longitude, msg.latitude};

        for (const AlertZone &zone : zones) {
            if (!zone.enabled) {
                continue;
            }

            geofenceChecks++;
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

        // 4. Lưu lại vào buffer 5 giây chờ đẩy xuống CSDL
        m_pendingPackets.insert(msg.shipId, msg);
    }

    double geofenceCheckOnlyMs = geofenceTimer.nsecsElapsed() / 1000000.0;

    QElapsedTimer cowZoneTimer;
    cowZoneTimer.start();
    // Cập nhật đồng loạt trạng thái tàu trong zone lên RAM (chỉ lock/COW 1 lần duy nhất)
    m_stateStore.updateShipZoneStates(zoneStateUpdates);
    double cowZoneMs = cowZoneTimer.nsecsElapsed() / 1000000.0;

    // 5. Xóa sạch buffer 200ms
    m_incomingBuffer.clear();

    double parseTimeMs = m_accumulatedParseTimeMs;
    int parsedCount = m_parsedCount;
    m_accumulatedParseTimeMs = 0.0;
    m_parsedCount = 0;

    double totalBatchMs = batchTimer.nsecsElapsed() / 1000000.0;
    qInfo().noquote() << QStringLiteral("[PERF][RAM] Processed batch:\n"
                                        "  * Parse JSON: %1 ms for %2 messages (Avg: %3 ms/msg)\n"
                                        "  * RAM COW Update:  %4 ms (Positions COW: %5 ms, Zone States COW: %6 ms)\n"
                                        "  * Geofence Check:  %7 ms for %8 vessels against %9 enabled zones (%10 operations) (Avg: %11 us/check)\n"
                                        "  * Total Batch:     %12 ms")
                       .arg(QString::number(parseTimeMs, 'f', 2))
                       .arg(parsedCount)
                       .arg(parsedCount > 0 ? QString::number(parseTimeMs / parsedCount, 'f', 4) : "0.0000")
                       .arg(QString::number(cowPositionsMs + cowZoneMs, 'f', 2))
                       .arg(QString::number(cowPositionsMs, 'f', 3))
                       .arg(QString::number(cowZoneMs, 'f', 3))
                       .arg(QString::number(geofenceCheckOnlyMs, 'f', 2))
                       .arg(batch.size())
                       .arg(zones.size())
                       .arg(geofenceChecks)
                       .arg(geofenceChecks > 0 ? QString::number((geofenceCheckOnlyMs * 1000.0) / geofenceChecks, 'f', 2) : "0.00")
                       .arg(QString::number(totalBatchMs, 'f', 2));
}

void PositionWorker::flushPendingPackets()
{
    if (m_pendingPackets.isEmpty() && m_pendingAlertEvents.isEmpty()) {
        return;
    }

    qInfo() << QStringLiteral("[PositionWorker] 5 seconds reached. Flushing %1 unique pending packets and %2 alert events to ShipWorker (DB)...")
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
