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

    // Nhận một lô gói bản tin vị trí và sự kiện cảnh báo từ PositionWorker để thực thi Transaction lưu DB
    void savePendingPackets(const QVector<ShipMessage> &packets, const QVector<AlertEvent> &alertEvents);

signals:
    void dbErrorOccurred(const QString &error);
    void batchProcessed(int count);

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