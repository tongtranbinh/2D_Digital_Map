#pragma once

#include <QString>
#include <QUuid>
#include <QDateTime>

struct Vessel {
    QUuid id;
    qint64 mmsi = 0;
    QString name;
    QString callsign;
    qint64 imo = 0;
    int shipType = 0;
    QString vesselType;
    QDateTime createdAt;
    QDateTime updatedAt;
};