#pragma once

#include <QAbstractListModel>
#include <QUuid>
#include <QVector>
#include <QVariantList>
#include <QGeoCoordinate>

#include "../model/AlertZone.h"

struct ZoneDisplayData {
    QUuid zoneId;
    QString name;
    QString description;
    QVariantList path; // Danh sách QGeoCoordinate
    bool enabled = true;
};

class ZoneListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum ZoneRoles {
        ZoneIdRole = Qt::UserRole + 1,
        NameRole,
        DescriptionRole,
        PathRole,
        EnabledRole
    };
    Q_ENUM(ZoneRoles)

    explicit ZoneListModel(QObject *parent = nullptr);
    ~ZoneListModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Thiết lập danh sách vùng cảnh báo từ C++ AlertZone vector
    void setZones(const QVector<AlertZone> &zones);

    Q_INVOKABLE QVariantMap getZoneAt(int index) const;

private:
    QVector<ZoneDisplayData> m_zones;
};
