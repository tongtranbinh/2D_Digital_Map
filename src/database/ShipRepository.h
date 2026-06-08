#pragma once

#include <optional>

#include <QString>
#include <QVector>

#include "PostgresConnection.h"
#include "../model/Vessel.h"

class IShipRepository
{
public:
    virtual ~IShipRepository() = default;

    virtual bool save(Vessel &vessel, QString *error = nullptr) = 0;
    virtual std::optional<Vessel> findById(const QUuid &id, QString *error = nullptr) const = 0;
    virtual std::optional<Vessel> findByMmsi(qint64 mmsi, QString *error = nullptr) const = 0;
    virtual QVector<Vessel> listAll(QString *error = nullptr) const = 0;
};

class ShipRepository final : public IShipRepository
{
public:
    explicit ShipRepository(PostgresConnection &connection);

    bool save(Vessel &vessel, QString *error = nullptr) override;
    std::optional<Vessel> findById(const QUuid &id, QString *error = nullptr) const override;
    std::optional<Vessel> findByMmsi(qint64 mmsi, QString *error = nullptr) const override;
    QVector<Vessel> listAll(QString *error = nullptr) const override;

private:
    PostgresConnection &m_connection;
};
