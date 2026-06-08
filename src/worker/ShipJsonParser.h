#pragma once

#include <QByteArray>
#include <QString>
#include <QDateTime>
#include "../model/VesselMessage.h"

bool parseShipJson(const QByteArray &raw, ShipMessage &msg, QString &error);
