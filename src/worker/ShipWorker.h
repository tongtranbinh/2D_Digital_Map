#pragma once

#include <QObject>
#include <QVector>
#include "../model/VesselMessage.h"
#include "../model/AlertMessage.h"
#include "../database/PostgresConnection.h"
#include "../database/ShipRepository.h"
#include "../database/PositionRepository.h"
#include "../database/AlertRepository.h"
#include "../service/ShipService.h"
#include "../service/PositionService.h"
#include "../service/AlertService.h"

class ShipStateStore;

class ShipWorker : public QObject
{
    Q_OBJECT

public:
    explicit ShipWorker(PostgresConfig dbConfig, ShipStateStore &stateStore, QObject *parent = nullptr);
    ~ShipWorker();

public slots:
    // Khởi tạo database, các service và nạp sẵn cache từ DB lên RAM
    void initialize();

    // Dọn dẹp tài nguyên CSDL trong worker thread
    void cleanup();

    // Nhận một lô gói bản tin vị trí và sự kiện cảnh báo từ PositionWorker để thực thi Transaction lưu DB
    void savePendingPackets(const QVector<ShipMessage> &packets, const QVector<AlertEvent> &alertEvents);

    // Xử lý yêu cầu truy vấn lịch sử hành trình từ CSDL của UI
    void handleTrackHistoryRequest(const QUuid &vesselId);

    // Xử lý yêu cầu ghi vùng cảnh báo mới xuống CSDL PostgreSQL từ UI thread
    void handleSaveZoneRequest(const AlertZone &zone);

    // Xử lý yêu cầu xóa vùng cảnh báo khỏi CSDL PostgreSQL từ UI thread
    void handleDeleteZoneRequest(const QUuid &zoneId);

signals:
    void dbErrorOccurred(const QString &error);
    void batchProcessed(int count);
    void cachePreloaded();
    void trackHistoryLoaded(const QUuid &vesselId, const QVector<ShipMessage> &history);

private:
    PostgresConfig m_config;
    PostgresConnection *m_db{nullptr};
    ShipStateStore &m_stateStore;

    IShipRepository *m_shipRepo{nullptr};
    IPositionRepository *m_posRepo{nullptr};
    IAlertRepository *m_alertRepo{nullptr};

    IShipService *m_shipService{nullptr};
    IPositionService *m_posService{nullptr};
    IAlertService *m_alertService{nullptr};
};