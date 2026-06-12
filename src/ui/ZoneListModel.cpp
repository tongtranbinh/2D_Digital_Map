#include "ZoneListModel.h"

ZoneListModel::ZoneListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ZoneListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_zones.size();
}

QVariant ZoneListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_zones.size())
        return QVariant();

    const auto &zone = m_zones[index.row()];

    switch (role) {
    case ZoneIdRole:
        return zone.zoneId.toString();
    case NameRole:
        return zone.name;
    case DescriptionRole:
        return zone.description;
    case PathRole:
        return zone.path;
    case EnabledRole:
        return zone.enabled;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ZoneListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[ZoneIdRole] = "zoneId";
    roles[NameRole] = "zoneName";
    roles[DescriptionRole] = "description";
    roles[PathRole] = "pathPoints";
    roles[EnabledRole] = "enabled";
    return roles;
}

void ZoneListModel::setZones(const QVector<AlertZone> &zones)
{
    beginResetModel();
    m_zones.clear();
    m_zones.reserve(zones.size());
    for (const auto &zone : zones) {
        ZoneDisplayData data;
        data.zoneId = zone.id;
        data.name = zone.name;
        data.description = zone.description;
        data.enabled = zone.enabled;

        QVariantList geoPath;
        for (const auto &pt : zone.polygon) {
            geoPath.append(QVariant::fromValue(QGeoCoordinate(pt.latitude, pt.longitude)));
        }
        data.path = geoPath;

        m_zones.push_back(data);
    }
    endResetModel();
}

QVariantMap ZoneListModel::getZoneAt(int index) const
{
    QVariantMap map;
    if (index < 0 || index >= m_zones.size())
        return map;

    const auto &zone = m_zones[index];
    map[QStringLiteral("zoneId")] = zone.zoneId.toString();
    map[QStringLiteral("zoneName")] = zone.name;
    map[QStringLiteral("description")] = zone.description;
    map[QStringLiteral("pathPoints")] = zone.path;
    map[QStringLiteral("enabled")] = zone.enabled;
    return map;
}
