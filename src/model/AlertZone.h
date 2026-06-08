#pragma once

#include <QString>
#include <QVector>
#include <QUuid>

#include "GeoPoint.h"

struct AlertZone {
    QUuid id;

    QString name;
    QString description;

    QVector<GeoPoint> polygon;

    bool enabled = true;
};