#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QPointer>
#include <QQuickItem>
#include <QSGGeometry>
#include <QVector>

class QSGGeometryNode;

class ShipRenderLayer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QAbstractListModel* shipModel READ shipModel WRITE setShipModel NOTIFY shipModelChanged)
    Q_PROPERTY(QObject* mapObject READ mapObject WRITE setMapObject NOTIFY mapObjectChanged)
    Q_PROPERTY(double zoomLevel READ zoomLevel WRITE setZoomLevel NOTIFY zoomLevelChanged)
    Q_PROPERTY(bool showLabels READ showLabels WRITE setShowLabels NOTIFY showLabelsChanged)

public:
    explicit ShipRenderLayer(QQuickItem *parent = nullptr);
    ~ShipRenderLayer() override = default;

    QAbstractListModel* shipModel() const { return m_shipModel; }
    void setShipModel(QAbstractListModel *model);

    QObject* mapObject() const { return m_mapObject; }
    void setMapObject(QObject *mapObject);

    double zoomLevel() const { return m_zoomLevel; }
    void setZoomLevel(double zoomLevel);

    bool showLabels() const { return m_showLabels; }
    void setShowLabels(bool showLabels);

    Q_INVOKABLE void refresh();

signals:
    void shipModelChanged();
    void mapObjectChanged();
    void zoomLevelChanged();
    void showLabelsChanged();
    void shipClicked(const QString &shipId);

protected:
    QSGNode* updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *data) override;
    void mousePressEvent(QMouseEvent *event) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    struct RenderShip {
        QString shipId;
        QPointF position;
        bool alert = false;
    };

    static int roleForName(const QHash<int, QByteArray> &roles, const QByteArray &name);
    QPointF mapToPixel(double latitude, double longitude) const;
    void appendShip(QVector<QSGGeometry::ColoredPoint2D> &vertices,
                    const QPointF &center,
                    double heading,
                    const QColor &color) const;
    QSGGeometryNode* updateGroupNode(QSGGeometryNode *node,
                                     const QVector<QSGGeometry::ColoredPoint2D> &vertices);

    QPointer<QAbstractListModel> m_shipModel;
    QPointer<QObject> m_mapObject;
    QVector<QMetaObject::Connection> m_modelConnections;

    double m_zoomLevel = 0.0;
    bool m_showLabels = false;

    QVector<QSGGeometry::ColoredPoint2D> m_normalVertices;
    QVector<QSGGeometry::ColoredPoint2D> m_alertVertices;
    QVector<RenderShip> m_visibleShips;
};
