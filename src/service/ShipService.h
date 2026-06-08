#pragma once

#include <optional>
#include <QString>
#include <QVector>
#include <QUuid>
#include <QSet>

#include "../model/Vessel.h"

class IShipRepository;

class IShipService
{
public:
    virtual ~IShipService() = default;
    virtual bool saveShip(Vessel &vessel, QString *error = nullptr) = 0;
    virtual std::optional<Vessel> getShipById(const QUuid &id, QString *error = nullptr) const = 0;
    virtual std::optional<Vessel> getShipByMmsi(qint64 mmsi, QString *error = nullptr) const = 0;
    virtual QVector<Vessel> getAllShips(QString *error = nullptr) const = 0;
    // Kiểm tra nhanh sự tồn tại của tàu bằng Cache trên RAM
    virtual bool shipExists(const QUuid &id, QString *error = nullptr) const = 0;
};

class ShipService final : public IShipService
{
public:
    explicit ShipService(IShipRepository &repository);

    bool saveShip(Vessel &vessel, QString *error = nullptr) override;
    std::optional<Vessel> getShipById(const QUuid &id, QString *error = nullptr) const override;
    std::optional<Vessel> getShipByMmsi(qint64 mmsi, QString *error = nullptr) const override;
    QVector<Vessel> getAllShips(QString *error = nullptr) const override;
    bool shipExists(const QUuid &id, QString *error = nullptr) const override;

private:
    void ensureCacheLoaded(QString *error = nullptr) const;

    IShipRepository &m_repository;
    mutable QSet<QUuid> m_registeredIds;
    mutable bool m_cacheLoaded = false;
};
