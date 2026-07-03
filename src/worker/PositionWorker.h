#pragma once

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QUuid>
#include <QTimer>
#include <QVector>
#include "../model/VesselMessage.h"
#include "../model/AlertMessage.h"

class ShipStateStore;

class PositionWorker : public QObject
{
    Q_OBJECT
public:
    explicit PositionWorker(ShipStateStore &stateStore, QObject *parent = nullptr);
    ~PositionWorker();

public slots:
    // Mở timer khi luồng phụ đã khởi động
    void startTimer();

    // Nhận bản tin thô TCP, parse, và lưu tạm vào buffer 200ms
    void processRawMessage(const QByteArray &rawMessage);

    // Xử lý lô bản tin gom được sau mỗi 200ms (Cập nhật RAM & kiểm tra geofencing)
    void processBatch();

    // Đẩy gói tin đã gom sau 5 giây sang ShipWorker và làm sạch bộ nhớ đệm
    void flushPendingPackets();

signals:
    void pendingPacketsReady(const QVector<ShipMessage> &packets, const QVector<AlertEvent> &alertEvents);
    void parsingError(const QString &error, const QByteArray &rawMessage);
    void positionsUpdated(const QVector<ShipMessage> &positions);
    void alertEventOccurred(const AlertEvent &event);

private:
    bool isPointInPolygon(const GeoPoint &point, const QVector<GeoPoint> &polygon) const;

    ShipStateStore &m_stateStore;
    QHash<QUuid, ShipMessage> m_incomingBuffer;
    QHash<QUuid, ShipMessage> m_pendingPackets;
    QVector<AlertEvent> m_pendingAlertEvents;
    QTimer *m_timer{nullptr};
    QTimer *m_batchTimer{nullptr};

    // Performance measurements
    int m_parsedCount{0};
    double m_accumulatedParseTimeMs{0.0};
};
