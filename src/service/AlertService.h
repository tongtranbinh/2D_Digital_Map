#pragma once

#include <optional>
#include <QString>
#include <QVector>
#include <QUuid>
#include <QHash>

#include "../model/AlertZone.h"
#include "../model/AlertMessage.h"

class IAlertRepository;

class IAlertService
{
public:
    virtual ~IAlertService() = default;
    virtual bool saveZone(AlertZone &zone, QString *error = nullptr) = 0;
    virtual std::optional<AlertZone> getZoneById(const QUuid &id, QString *error = nullptr) const = 0;
    virtual QVector<AlertZone> getAllZones(QString *error = nullptr) const = 0;
    virtual bool logEvent(AlertEvent &event, QString *error = nullptr) = 0;
    virtual QVector<AlertEvent> getRecentEvents(int limit = 100, QString *error = nullptr) const = 0;
    virtual bool isPointInPolygon(const GeoPoint &point, const QVector<GeoPoint> &polygon) const = 0;

    // Quản lý lưu/truy xuất trạng thái tàu trong vùng cảnh báo bằng Cache RAM kết hợp DB
    virtual bool saveShipZoneState(const QUuid &vesselId, const QUuid &zoneId, bool isInside, QString *error = nullptr) = 0;
    virtual bool getVesselZoneState(const QUuid &vesselId, const QUuid &zoneId, QString *error = nullptr) const = 0;
    
    // Nạp sẵn toàn bộ Cache (vùng cảnh báo + trạng thái tàu) khi khởi động chương trình
    virtual void preloadCache(QString *error = nullptr) = 0;
};

class AlertService final : public IAlertService
{
public:
    explicit AlertService(IAlertRepository &repository);

    bool saveZone(AlertZone &zone, QString *error = nullptr) override;
    std::optional<AlertZone> getZoneById(const QUuid &id, QString *error = nullptr) const override;
    QVector<AlertZone> getAllZones(QString *error = nullptr) const override;
    bool logEvent(AlertEvent &event, QString *error = nullptr) override;
    QVector<AlertEvent> getRecentEvents(int limit = 100, QString *error = nullptr) const override;
    bool isPointInPolygon(const GeoPoint &point, const QVector<GeoPoint> &polygon) const override;

    bool saveShipZoneState(const QUuid &vesselId, const QUuid &zoneId, bool isInside, QString *error = nullptr) override;
    bool getVesselZoneState(const QUuid &vesselId, const QUuid &zoneId, QString *error = nullptr) const override;
    void preloadCache(QString *error = nullptr) override;

private:
    void ensureStatesLoaded(QString *error = nullptr) const;

    IAlertRepository &m_repository;
    mutable QVector<AlertZone> m_zonesCache;
    mutable bool m_zonesLoaded = false;

    // Cache RAM trạng thái tàu trong vùng (Key: shipId_zoneId, Value: isInside)
    mutable QHash<QString, bool> m_shipZoneStates;
    mutable bool m_statesLoaded = false;
};
