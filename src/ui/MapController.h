#pragma once

#include <QObject>
#include <QHash>
#include <QUuid>
#include <QVector>
#include <QGeoCoordinate>
#include <QVariantList>
#include <QDateTime>

#include "../state/ShipStateStore.h"
#include "../database/PostgresConnection.h"
#include "ShipListModel.h"
#include "ZoneListModel.h"
#include "../model/AlertMessage.h"

class MapController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ShipListModel* shipModel READ shipModel CONSTANT)
    Q_PROPERTY(ZoneListModel* zoneModel READ zoneModel CONSTANT)
    Q_PROPERTY(QString selectedShipId READ selectedShipId WRITE setSelectedShipId NOTIFY selectedShipIdChanged)

public:
    explicit MapController(ShipStateStore &stateStore, QObject *parent = nullptr);
    ~MapController() override = default;

    ShipListModel* shipModel() const { return m_shipModel; }
    ZoneListModel* zoneModel() const { return m_zoneModel; }

    QString selectedShipId() const;
    void setSelectedShipId(const QString &id);


    // Lấy hành trình thời gian thực trong RAM của một tàu
    Q_INVOKABLE QVariantList getTrackHistory(const QString &shipId) const;

    // Yêu cầu CSDL tải lịch sử hành trình cũ của tàu
    Q_INVOKABLE void loadTrackHistoryFromDb(const QString &shipId);

    // Thêm vùng cảnh báo mới từ người dùng vẽ trên QML
    Q_INVOKABLE void addAlertZone(const QString &name, const QString &description, const QVariantList &coordinates);

    // Thiết lập danh sách vùng cảnh báo ban đầu từ RAM cache
    void initZones();

public slots:
    // Nhận cập nhật vị trí từ PositionWorker
    void handlePositionsUpdated(const QVector<ShipMessage> &positions);

    // Nhận sự kiện cảnh báo từ PositionWorker
    void handleAlertEvent(const AlertEvent &event);

    // Nhận lịch sử hành trình đã tải từ DB trong ShipWorker
    void handleTrackHistoryLoaded(const QUuid &vesselId, const QVector<ShipMessage> &history);

signals:
    // Báo cho QML hiển thị banner cảnh báo
    void vesselAlertTriggered(const QString &shipId, const QString &shipName, 
                              const QString &zoneId, const QString &zoneName, 
                              const QString &eventType, const QString &timeStr);

    // Báo cho QML biết hành trình của một tàu được cập nhật thêm điểm mới
    void trackHistoryUpdated(const QString &shipId);

    // Phát tín hiệu yêu cầu ShipWorker truy vấn CSDL
    void requestTrackHistory(const QUuid &vesselId);

    // Phát tín hiệu yêu cầu ShipWorker ghi vùng cảnh báo mới xuống CSDL PostgreSQL
    void requestSaveZone(const AlertZone &zone);

    void selectedShipIdChanged();

private:
    ShipStateStore &m_stateStore;
    ShipListModel *m_shipModel;
    ZoneListModel *m_zoneModel;

    // Cache thông tin tàu tĩnh
    QHash<QUuid, Vessel> m_vesselCache;

    // Lưu trữ hành trình thời gian thực trong RAM (Tối đa 100 điểm gần nhất mỗi tàu)
    QHash<QUuid, QVector<QGeoCoordinate>> m_trackHistories;

    // Danh sách ID các vùng cảnh báo hoạt động
    QVector<QUuid> m_activeZoneIds;

    QUuid m_selectedShipId;
};
