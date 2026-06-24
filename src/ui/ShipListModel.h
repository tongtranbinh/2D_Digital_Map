#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QUuid>
#include <QDateTime>
#include <QVector>

#include "../model/VesselMessage.h"
#include "../model/Vessel.h"

struct ShipDisplayData {
    QUuid shipId;
    QString name;
    qint64 mmsi = 0;
    double latitude = 0.0;
    double longitude = 0.0;
    double speed = 0.0;
    double heading = 0.0;
    double course = 0.0;
    QDateTime timestamp;
    bool isInsideZone = false;
};

class ShipListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int trackingCount READ trackingCount NOTIFY countsChanged)
    Q_PROPERTY(int alertCount READ alertCount NOTIFY countsChanged)

public:
    enum ShipRoles {
        ShipIdRole = Qt::UserRole + 1,
        NameRole,
        MmsiRole,
        LatitudeRole,
        LongitudeRole,
        SpeedRole,
        HeadingRole,
        CourseRole,
        TimestampRole,
        IsInsideZoneRole
    };
    Q_ENUM(ShipRoles)

    explicit ShipListModel(QObject *parent = nullptr);
    ~ShipListModel() override = default;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Cập nhật vị trí và thông tin tàu
    void updateShipPositions(const QVector<ShipMessage> &positions, 
                             const QHash<QUuid, Vessel> &vesselCache,
                             const QHash<QString, bool> &zoneStates,
                             const QVector<QUuid> &activeZones);

    // Lấy thông tin tàu bằng index
    Q_INVOKABLE QVariantMap getShipAt(int index) const;

    // Lấy vị trí của một tàu cụ thể
    Q_INVOKABLE int findShipIndex(const QString &shipId) const;

    int trackingCount() const { return m_ships.size(); }
    int alertCount() const { return m_alertCount; }

signals:
    void countsChanged();

private:
    void recomputeAlertCount();

    QVector<ShipDisplayData> m_ships;
    QHash<QUuid, int> m_shipIdToIndex;
    int m_alertCount{0};
};
