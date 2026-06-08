#include "ShipJsonParser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUuid>
#include <QVariant>

bool parseShipJson(const QByteArray &raw, ShipMessage &msg, QString &error)
{
    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &perr);

    if (perr.error != QJsonParseError::NoError) {
        error = perr.errorString();
        return false;
    }

    if (!doc.isObject()) {
        error = QLatin1String("expected JSON object");
        return false;
    }

    QJsonObject o = doc.object();

    // ship id (accept shipId, id, ship_id)
    QString idStr;
    if (o.contains("shipId")) idStr = o.value("shipId").toString();
    else if (o.contains("id")) idStr = o.value("id").toString();
    else if (o.contains("ship_id")) idStr = o.value("ship_id").toString();

    if (idStr.isEmpty()) {
        error = QLatin1String("missing shipId");
        return false;
    }

    QUuid uuid = QUuid::fromString(idStr);
    if (uuid.isNull()) {
        // try with braces
        QString withBraces = '{' + idStr + '}';
        uuid = QUuid::fromString(withBraces);
    }

    if (uuid.isNull()) {
        error = QLatin1String("invalid shipId UUID: ") + idStr;
        return false;
    }

    msg.shipId = uuid;

    // numeric fields
    msg.longitude = o.value("longitude").toDouble();
    msg.latitude = o.value("latitude").toDouble();
    msg.speed = o.value("speed").toDouble();
    msg.heading = o.value("heading").toDouble();

    // timestamp: accept ISO string or milliseconds since epoch
    if (o.contains("timestamp")) {
        QJsonValue tv = o.value("timestamp");
        if (tv.isString()) {
            QString ts = tv.toString();
            QDateTime dt = QDateTime::fromString(ts, Qt::ISODate);
            if (dt.isValid()) msg.timestamp = dt;
            else {
                bool ok = false;
                qint64 ms = ts.toLongLong(&ok);
                if (ok) msg.timestamp = QDateTime::fromMSecsSinceEpoch(ms);
            }
        } else if (tv.isDouble()) {
            qint64 ms = static_cast<qint64>(tv.toDouble());
            msg.timestamp = QDateTime::fromMSecsSinceEpoch(ms);
        }
    }

    if (!msg.timestamp.isValid()) msg.timestamp = QDateTime::currentDateTimeUtc();

    return true;
}
