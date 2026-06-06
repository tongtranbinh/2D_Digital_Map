#ifndef SHIP_MODEL_H
#define SHIP_MODEL_H

#include <QAbstractListModel>
#include <QGeoCoordinate>
#include <QDateTime>
#include <QHash>
#include <QList>

struct Ship {
    int mmsi = 0;
    QString name;
    QGeoCoordinate coordinate;
    float speed = 0.0;
    float course = 0.0;
    bool isWarning = false;
    QString warningName;
    QDateTime timestamp;
};

class ShipModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum ShipRoles {
        MmsiRole = Qt::UserRole + 1,
        NameRole,
        CoordinateRole,
        SpeedRole,
        CourseRole,
        IsWarningRole,
        WarningNameRole,
        TimestampRole
    };

    explicit ShipModel(QObject *parent = nullptr);

    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Cập nhật vị trí tàu hoặc thêm tàu mới
    void updateOrAddShip(const Ship &ship);
    
    // Lấy thông tin chi tiết một tàu thông qua MMSI
    Q_INVOKABLE QVariantMap getShipDetails(int mmsi) const;

private:
    QList<Ship> m_ships;
    QHash<int, int> m_mmsiToIndex; // Tra cứu nhanh chỉ mục tàu từ MMSI
};

#endif // SHIP_MODEL_H
