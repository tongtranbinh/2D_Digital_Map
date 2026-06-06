#include "ship_model.h"

ShipModel::ShipModel(QObject *parent) : QAbstractListModel(parent) {
}

int ShipModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) return 0;
    return m_ships.size();
}

QVariant ShipModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() >= m_ships.size()) return QVariant();

    const Ship &ship = m_ships.at(index.row());
    switch (role) {
        case MmsiRole:
            return ship.mmsi;
        case NameRole:
            return ship.name;
        case CoordinateRole:
            return QVariant::fromValue(ship.coordinate);
        case SpeedRole:
            return ship.speed;
        case CourseRole:
            return ship.course;
        case IsWarningRole:
            return ship.isWarning;
        case WarningNameRole:
            return ship.warningName;
        case TimestampRole:
            return ship.timestamp;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> ShipModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[MmsiRole] = "mmsi";
    roles[NameRole] = "name";
    roles[CoordinateRole] = "coordinate";
    roles[SpeedRole] = "speed";
    roles[CourseRole] = "course";
    roles[IsWarningRole] = "isWarning";
    roles[WarningNameRole] = "warningName";
    roles[TimestampRole] = "timestamp";
    return roles;
}

void ShipModel::updateOrAddShip(const Ship &ship) {
    if (m_mmsiToIndex.contains(ship.mmsi)) {
        int idx = m_mmsiToIndex.value(ship.mmsi);
        m_ships[idx] = ship;
        QModelIndex modelIdx = index(idx, 0);
        // Chỉ phát tín hiệu thay đổi dữ liệu cho dòng hiện tại để tránh render lại toàn bộ list
        emit dataChanged(modelIdx, modelIdx, {CoordinateRole, SpeedRole, CourseRole, IsWarningRole, WarningNameRole, TimestampRole});
    } else {
        // Chèn thêm một phần tử mới vào cuối danh sách
        beginInsertRows(QModelIndex(), m_ships.size(), m_ships.size());
        m_mmsiToIndex[ship.mmsi] = m_ships.size();
        m_ships.append(ship);
        endInsertRows();
    }
}

QVariantMap ShipModel::getShipDetails(int mmsi) const {
    QVariantMap details;
    if (m_mmsiToIndex.contains(mmsi)) {
        int idx = m_mmsiToIndex.value(mmsi);
        const Ship &ship = m_ships.at(idx);
        details["mmsi"] = ship.mmsi;
        details["name"] = ship.name;
        details["lat"] = ship.coordinate.latitude();
        details["lon"] = ship.coordinate.longitude();
        details["speed"] = ship.speed;
        details["course"] = ship.course;
        details["isWarning"] = ship.isWarning;
        details["warningName"] = ship.warningName;
        details["timestamp"] = ship.timestamp.toString("yyyy-MM-dd HH:mm:ss");
    }
    return details;
}
