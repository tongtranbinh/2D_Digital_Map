#pragma once

#include <QString>
#include <QDateTime>
#include <QUuid>
#include <optional>

#include "GeoPoint.h"

struct AlertEvent {
    QUuid id;
    QUuid vesselId;
    QUuid alertZoneId;
    QString eventType;
    QDateTime createdAt;
    std::optional<GeoPoint> position;
};