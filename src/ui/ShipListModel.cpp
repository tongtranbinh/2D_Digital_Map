#include "ShipListModel.h"

ShipListModel::ShipListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ShipListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_ships.size();
}

QVariant ShipListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_ships.size())
        return QVariant();

    const auto &ship = m_ships[index.row()];

    switch (role) {
    case ShipIdRole:
        return ship.shipId.toString();
    case NameRole:
        return ship.name;
    case MmsiRole:
        return ship.mmsi;
    case LatitudeRole:
        return ship.latitude;
    case LongitudeRole:
        return ship.longitude;
    case SpeedRole:
        return ship.speed;
    case HeadingRole:
        return ship.heading;
    case CourseRole:
        return ship.course;
    case TimestampRole:
        return ship.timestamp;
    case IsInsideZoneRole:
        return ship.isInsideZone;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ShipListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ShipIdRole] = "shipId";
    roles[NameRole] = "vesselName";
    roles[MmsiRole] = "mmsi";
    roles[LatitudeRole] = "latitude";
    roles[LongitudeRole] = "longitude";
    roles[SpeedRole] = "speed";
    roles[HeadingRole] = "heading";
    roles[CourseRole] = "course";
    roles[TimestampRole] = "timestamp";
    roles[IsInsideZoneRole] = "isInsideZone";
    return roles;
}

void ShipListModel::updateShipPositions(const QVector<ShipMessage> &positions,
                                       const QHash<QUuid, Vessel> &vesselCache,
                                       const QHash<QString, bool> &zoneStates,
                                       const QVector<QUuid> &activeZones)
{
    if (positions.isEmpty())
        return;

    QVector<ShipMessage> newPositions;
    QVector<int> updatedIndices;
    const int previousShipCount = m_ships.size();
    const int previousAlertCount = m_alertCount;

    for (const auto &msg : positions) {
        bool isInside = false;
        QString shipIdStr = msg.shipId.toString();
        for (const auto &zoneId : activeZones) {
            QString key = shipIdStr + "_" + zoneId.toString();
            if (zoneStates.value(key, false)) {
                isInside = true;
                break;
            }
        }

        if (m_shipIdToIndex.contains(msg.shipId)) {
            int idx = m_shipIdToIndex[msg.shipId];
            auto &ship = m_ships[idx];
            ship.latitude = msg.latitude;
            ship.longitude = msg.longitude;
            ship.speed = msg.speed;
            ship.heading = msg.heading;
            ship.course = msg.course;
            ship.timestamp = msg.timestamp;
            ship.isInsideZone = isInside;

            if (ship.name.startsWith(QLatin1String("Unknown")) && vesselCache.contains(msg.shipId)) {
                const auto &v = vesselCache[msg.shipId];
                ship.name = v.name;
                ship.mmsi = v.mmsi;
            }
            updatedIndices.push_back(idx);
        } else {
            newPositions.push_back(msg);
        }
    }

    // 1. Phát tín hiệu dataChanged hàng loạt cho các tàu cập nhật
    if (!updatedIndices.isEmpty()) {
        int minIdx = updatedIndices[0];
        int maxIdx = updatedIndices[0];
        for (int idx : updatedIndices) {
            if (idx < minIdx) minIdx = idx;
            if (idx > maxIdx) maxIdx = idx;
        }

        // Nếu số lượng tàu thay đổi lớn, phát một tín hiệu duy nhất cho toàn vùng
        if (updatedIndices.size() > 5 || (maxIdx - minIdx + 1) == updatedIndices.size()) {
            emit dataChanged(createIndex(minIdx, 0), createIndex(maxIdx, 0));
        } else {
            // Nếu chỉ có một vài tàu rải rác, phát tín hiệu riêng cho từng tàu
            for (int idx : updatedIndices) {
                emit dataChanged(createIndex(idx, 0), createIndex(idx, 0));
            }
        }
    }

    // 2. Chèn hàng loạt tàu mới trong một block duy nhất
    if (!newPositions.isEmpty()) {
        int insertStart = m_ships.size();
        int insertEnd = insertStart + newPositions.size() - 1;

        beginInsertRows(QModelIndex(), insertStart, insertEnd);
        for (const auto &msg : newPositions) {
            bool isInside = false;
            QString shipIdStr = msg.shipId.toString();
            for (const auto &zoneId : activeZones) {
                QString key = shipIdStr + "_" + zoneId.toString();
                if (zoneStates.value(key, false)) {
                    isInside = true;
                    break;
                }
            }

            ShipDisplayData ship;
            ship.shipId = msg.shipId;
            ship.latitude = msg.latitude;
            ship.longitude = msg.longitude;
            ship.speed = msg.speed;
            ship.heading = msg.heading;
            ship.course = msg.course;
            ship.timestamp = msg.timestamp;
            ship.isInsideZone = isInside;

            if (vesselCache.contains(msg.shipId)) {
                const auto &v = vesselCache[msg.shipId];
                ship.name = v.name;
                ship.mmsi = v.mmsi;
            } else {
                ship.name = QStringLiteral("Vessel %1").arg(msg.shipId.toString().mid(1, 5).toUpper());
                ship.mmsi = 100000000 + (qHash(msg.shipId) % 900000000);
            }

            m_ships.push_back(ship);
            m_shipIdToIndex[msg.shipId] = m_ships.size() - 1;
        }
        endInsertRows();
    }

    recomputeAlertCount();
    if (previousShipCount != m_ships.size() || previousAlertCount != m_alertCount) {
        emit countsChanged();
    }
}

QVariantMap ShipListModel::getShipAt(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= m_ships.size())
        return map;

    const auto &ship = m_ships[index];
    map[QStringLiteral("shipId")] = ship.shipId.toString();
    map[QStringLiteral("vesselName")] = ship.name;
    map[QStringLiteral("mmsi")] = ship.mmsi;
    map[QStringLiteral("latitude")] = ship.latitude;
    map[QStringLiteral("longitude")] = ship.longitude;
    map[QStringLiteral("speed")] = ship.speed;
    map[QStringLiteral("heading")] = ship.heading;
    map[QStringLiteral("course")] = ship.course;
    map[QStringLiteral("timestamp")] = ship.timestamp;
    map[QStringLiteral("isInsideZone")] = ship.isInsideZone;
    return map;
}

int ShipListModel::findShipIndex(const QString &shipId) const
{
    QUuid uuid = QUuid::fromString(shipId);
    if (uuid.isNull() && !shipId.isEmpty()) {
        uuid = QUuid::fromString("{" + shipId + "}");
    }
    return m_shipIdToIndex.value(uuid, -1);
}

void ShipListModel::recomputeAlertCount()
{
    int count = 0;
    for (const auto &ship : m_ships) {
        if (ship.isInsideZone) {
            ++count;
        }
    }
    m_alertCount = count;
}
