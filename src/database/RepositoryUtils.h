#pragma once

#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QUuid>
#include <optional>

#include "../model/GeoPoint.h"

inline QString uuidToString(const QUuid &uuid)
{
    return uuid.toString(QUuid::WithoutBraces);
}

inline QUuid uuidFromString(const QString &value)
{
    return QUuid::fromString(value.startsWith('{') ? value : '{' + value + '}');
}

inline QString polygonToWkt(const QVector<GeoPoint> &points)
{
    if (points.size() < 3) {
        return QString();
    }

    QStringList ring;
    ring.reserve(points.size() + 1);
    for (const GeoPoint &point : points) {
        ring << QString::number(point.longitude, 'f', 15) + ' ' + QString::number(point.latitude, 'f', 15);
    }

    if (points.first().longitude != points.last().longitude || points.first().latitude != points.last().latitude) {
        ring << QString::number(points.first().longitude, 'f', 15) + ' ' + QString::number(points.first().latitude, 'f', 15);
    }

    return QStringLiteral("POLYGON((") + ring.join(QStringLiteral(", ")) + QStringLiteral("))");
}

inline QVector<GeoPoint> polygonFromWkt(const QString &wkt)
{
    QString text = wkt.trimmed().toUpper();
    if (text.startsWith(QStringLiteral("POLYGON"))) {
        int firstParen = text.indexOf(QLatin1Char('('));
        int lastParen = text.lastIndexOf(QLatin1Char(')'));
        if (firstParen != -1 && lastParen != -1 && lastParen > firstParen) {
            text = text.mid(firstParen + 1, lastParen - firstParen - 1).trimmed();
            if (text.startsWith(QLatin1Char('(')) && text.endsWith(QLatin1Char(')'))) {
                text = text.mid(1, text.size() - 2).trimmed();
            }
        }
    }

    QVector<GeoPoint> points;
    const QStringList pairs = text.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &pairText : pairs) {
        const QStringList parts = pairText.trimmed().split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            continue;
        }

        GeoPoint point;
        point.longitude = parts.at(0).toDouble();
        point.latitude = parts.at(1).toDouble();
        points.push_back(point);
    }

    if (!points.isEmpty() && points.first().longitude == points.last().longitude && points.first().latitude == points.last().latitude) {
        points.removeLast();
    }

    return points;
}

inline QString pointToWkt(const GeoPoint &point)
{
    return QStringLiteral("POINT(")
           + QString::number(point.longitude, 'f', 15)
           + QLatin1Char(' ')
           + QString::number(point.latitude, 'f', 15)
           + QLatin1Char(')');
}

inline std::optional<GeoPoint> pointFromWkt(const QString &wkt)
{
    QString text = wkt.trimmed().toUpper();
    if (!text.startsWith(QStringLiteral("POINT"))) {
        return std::nullopt;
    }
    int firstParen = text.indexOf(QLatin1Char('('));
    int lastParen = text.lastIndexOf(QLatin1Char(')'));
    if (firstParen == -1 || lastParen == -1 || lastParen <= firstParen) {
        return std::nullopt;
    }
    text = text.mid(firstParen + 1, lastParen - firstParen - 1).trimmed();
    const QStringList parts = text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (parts.size() < 2) {
        return std::nullopt;
    }

    GeoPoint point;
    point.longitude = parts.at(0).toDouble();
    point.latitude = parts.at(1).toDouble();
    return point;
}
