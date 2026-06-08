#pragma once

#include <QString>
#include <QUuid>

#include "../model/VesselMessage.h"

class IShipService;
class IPositionService;
class IAlertService;

class ITrackingService
{
public:
    virtual ~ITrackingService() = default;
    virtual bool processTrackingMessage(ShipMessage &msg, QString *error = nullptr) = 0;
    virtual bool processTrackingMessages(QVector<ShipMessage> &msgs, QString *error = nullptr) = 0;
};

class TrackingService final : public ITrackingService
{
public:
    TrackingService(IShipService &shipService, IPositionService &positionService, IAlertService &alertService);

    bool processTrackingMessage(ShipMessage &msg, QString *error = nullptr) override;
    bool processTrackingMessages(QVector<ShipMessage> &msgs, QString *error = nullptr) override;

private:
    IShipService &m_shipService;
    IPositionService &m_positionService;
    IAlertService &m_alertService;
};
