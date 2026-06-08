#pragma once

#include <QString>
#include <QDateTime>
#include <QMetaType>
#include <QUuid>

struct ShipMessage {
    QUuid id;
    QUuid shipId;
    double longitude = 0.0;
    double latitude = 0.0;
    double speed = 0.0;
    double course = 0.0;
    double heading = 0.0;
    QDateTime timestamp;
};

Q_DECLARE_METATYPE(ShipMessage)