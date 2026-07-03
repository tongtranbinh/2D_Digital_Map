#include "ShipRenderLayer.h"

#include <QColor>
#include <QGeoCoordinate>
#include <QMouseEvent>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <utility>

namespace {
constexpr double kCullMargin = 24.0;
constexpr double kClickRadius = 10.0;
constexpr float kShipLength = 15.0f;
constexpr float kShipHalfWidth = 5.5f;

}

ShipRenderLayer::ShipRenderLayer(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

void ShipRenderLayer::setShipModel(QAbstractListModel *model)
{
    if (m_shipModel == model) {
        return;
    }

    for (const auto &connection : m_modelConnections) {
        disconnect(connection);
    }
    m_modelConnections.clear();

    m_shipModel = model;

    if (m_shipModel) {
        m_modelConnections.append(connect(m_shipModel, &QAbstractItemModel::dataChanged,
                                          this, &ShipRenderLayer::refresh));
        m_modelConnections.append(connect(m_shipModel, &QAbstractItemModel::rowsInserted,
                                          this, &ShipRenderLayer::refresh));
        m_modelConnections.append(connect(m_shipModel, &QAbstractItemModel::rowsRemoved,
                                          this, &ShipRenderLayer::refresh));
        m_modelConnections.append(connect(m_shipModel, &QAbstractItemModel::modelReset,
                                          this, &ShipRenderLayer::refresh));
    }

    emit shipModelChanged();
    refresh();
}

void ShipRenderLayer::setMapObject(QObject *mapObject)
{
    if (m_mapObject == mapObject) {
        return;
    }

    m_mapObject = mapObject;
    emit mapObjectChanged();
    refresh();
}

void ShipRenderLayer::setZoomLevel(double zoomLevel)
{
    if (qFuzzyCompare(m_zoomLevel, zoomLevel)) {
        return;
    }

    m_zoomLevel = zoomLevel;
    emit zoomLevelChanged();
    refresh();
}

void ShipRenderLayer::setShowLabels(bool showLabels)
{
    if (m_showLabels == showLabels) {
        return;
    }

    m_showLabels = showLabels;
    emit showLabelsChanged();
    update();
}

void ShipRenderLayer::setSelectedShipId(const QString &shipId)
{
    if (m_selectedShipId == shipId) {
        return;
    }

    m_selectedShipId = shipId;
    emit selectedShipIdChanged();
    refresh();
}

int ShipRenderLayer::roleForName(const QHash<int, QByteArray> &roles, const QByteArray &name)
{
    for (auto it = roles.cbegin(); it != roles.cend(); ++it) {
        if (it.value() == name) {
            return it.key();
        }
    }
    return -1;
}

QPointF ShipRenderLayer::mapToPixel(double latitude, double longitude) const
{
    if (!m_mapObject) {
        return QPointF(qQNaN(), qQNaN());
    }

    QPointF pixel;
    const QGeoCoordinate coordinate(latitude, longitude);
    const bool ok = QMetaObject::invokeMethod(m_mapObject, "fromCoordinate",
                                               Q_RETURN_ARG(QPointF, pixel),
                                               Q_ARG(QGeoCoordinate, coordinate),
                                               Q_ARG(bool, false));
    return ok ? pixel : QPointF(qQNaN(), qQNaN());
}

void ShipRenderLayer::appendShip(QVector<QSGGeometry::ColoredPoint2D> &vertices,
                                 const QPointF &center,
                                 double heading,
                                 const QColor &color,
                                 double scale) const
{
    const double radians = qDegreesToRadians(heading);
    const double sinH = std::sin(radians);
    const double cosH = std::cos(radians);

    auto rotate = [&](float x, float y) {
        return QPointF(center.x() + x * cosH - y * sinH,
                       center.y() + x * sinH + y * cosH);
    };

    const float length = kShipLength * scale;
    const float halfWidth = kShipHalfWidth * scale;

    const QPointF nose = rotate(0.0f, -length * 0.65f);
    const QPointF left = rotate(-halfWidth, length * 0.35f);
    const QPointF right = rotate(halfWidth, length * 0.35f);

    const unsigned char r = static_cast<unsigned char>(color.red());
    const unsigned char g = static_cast<unsigned char>(color.green());
    const unsigned char b = static_cast<unsigned char>(color.blue());
    const unsigned char a = static_cast<unsigned char>(color.alpha());

    QSGGeometry::ColoredPoint2D vertex;
    vertex.r = r;
    vertex.g = g;
    vertex.b = b;
    vertex.a = a;

    vertex.x = static_cast<float>(nose.x());
    vertex.y = static_cast<float>(nose.y());
    vertices.append(vertex);

    vertex.x = static_cast<float>(left.x());
    vertex.y = static_cast<float>(left.y());
    vertices.append(vertex);

    vertex.x = static_cast<float>(right.x());
    vertex.y = static_cast<float>(right.y());
    vertices.append(vertex);
}

void ShipRenderLayer::refresh()
{
    m_normalVertices.clear();
    m_alertVertices.clear();
    m_outlineVertices.clear();
    m_selectedVertices.clear();
    m_visibleShips.clear();

    if (!m_shipModel || !m_mapObject || width() <= 0.0 || height() <= 0.0) {
        update();
        return;
    }

    const QHash<int, QByteArray> roles = m_shipModel->roleNames();
    const int shipIdRole = roleForName(roles, "shipId");
    const int latitudeRole = roleForName(roles, "latitude");
    const int longitudeRole = roleForName(roles, "longitude");
    const int headingRole = roleForName(roles, "heading");
    const int courseRole = roleForName(roles, "course");
    const int alertRole = roleForName(roles, "isInsideZone");

    if (shipIdRole < 0 || latitudeRole < 0 || longitudeRole < 0) {
        update();
        return;
    }

    const int rows = m_shipModel->rowCount();
    m_normalVertices.reserve(rows * 3);
    m_alertVertices.reserve(rows * 3 / 8);
    m_visibleShips.reserve(std::min(rows, 4096));

    const QColor normalColor("#06b6d4");
    const QColor alertColor("#ef4444");
    const QColor whiteColor("#ffffff");

    for (int row = 0; row < rows; ++row) {
        const QModelIndex index = m_shipModel->index(row, 0);
        const double latitude = m_shipModel->data(index, latitudeRole).toDouble();
        const double longitude = m_shipModel->data(index, longitudeRole).toDouble();
        const QPointF pixel = mapToPixel(latitude, longitude);

        if (!std::isfinite(pixel.x()) || !std::isfinite(pixel.y())) {
            continue;
        }
        if (pixel.x() < -kCullMargin || pixel.x() > width() + kCullMargin
            || pixel.y() < -kCullMargin || pixel.y() > height() + kCullMargin) {
            continue;
        }

        double heading = headingRole >= 0 ? m_shipModel->data(index, headingRole).toDouble() : 0.0;
        if (qFuzzyIsNull(heading) && courseRole >= 0) {
            heading = m_shipModel->data(index, courseRole).toDouble();
        }

        const bool alert = alertRole >= 0 && m_shipModel->data(index, alertRole).toBool();
        const QString shipId = m_shipModel->data(index, shipIdRole).toString();
        const bool isSelected = (!m_selectedShipId.isEmpty() && shipId == m_selectedShipId);

        if (isSelected) {
            // Draw a bright white outline triangle (1.35x size) underneath the ship
            appendShip(m_outlineVertices, pixel, heading, whiteColor, 1.35);
            // Draw the normal ship body (1.0x size) on top of the outline
            appendShip(m_selectedVertices, pixel, heading, alert ? alertColor : normalColor, 1.0);
        } else {
            QVector<QSGGeometry::ColoredPoint2D> &target = alert ? m_alertVertices : m_normalVertices;
            appendShip(target, pixel, heading, alert ? alertColor : normalColor, 1.0);
        }

        RenderShip ship;
        ship.shipId = shipId;
        ship.position = pixel;
        ship.alert = alert;
        m_visibleShips.append(ship);
    }

    update();
}

QSGGeometryNode* ShipRenderLayer::updateGroupNode(
    QSGGeometryNode *node,
    const QVector<QSGGeometry::ColoredPoint2D> &vertices)
{
    if (!node) {
        node = new QSGGeometryNode;
        auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
        geometry->setDrawingMode(QSGGeometry::DrawTriangles);
        node->setGeometry(geometry);
        node->setFlag(QSGNode::OwnsGeometry);

        auto *material = new QSGVertexColorMaterial;
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsMaterial);
    }

    QSGGeometry *geometry = node->geometry();
    geometry->allocate(vertices.size());
    if (!vertices.isEmpty()) {
        std::memcpy(geometry->vertexDataAsColoredPoint2D(),
                    vertices.constData(),
                    vertices.size() * sizeof(QSGGeometry::ColoredPoint2D));
    }
    node->markDirty(QSGNode::DirtyGeometry);
    return node;
}

QSGNode* ShipRenderLayer::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    QSGNode *root = oldNode;
    if (!root) {
        root = new QSGNode;
    }

    auto *normalNode = static_cast<QSGGeometryNode *>(root->firstChild());
    if (!normalNode) {
        normalNode = updateGroupNode(nullptr, {});
        root->appendChildNode(normalNode);
    }

    auto *alertNode = static_cast<QSGGeometryNode *>(normalNode->nextSibling());
    if (!alertNode) {
        alertNode = updateGroupNode(nullptr, {});
        root->appendChildNode(alertNode);
    }

    auto *outlineNode = static_cast<QSGGeometryNode *>(alertNode->nextSibling());
    if (!outlineNode) {
        outlineNode = updateGroupNode(nullptr, {});
        root->appendChildNode(outlineNode);
    }

    auto *selectedNode = static_cast<QSGGeometryNode *>(outlineNode->nextSibling());
    if (!selectedNode) {
        selectedNode = updateGroupNode(nullptr, {});
        root->appendChildNode(selectedNode);
    }

    updateGroupNode(normalNode, m_normalVertices);
    updateGroupNode(alertNode, m_alertVertices);
    updateGroupNode(outlineNode, m_outlineVertices);
    updateGroupNode(selectedNode, m_selectedVertices);

    return root;
}

void ShipRenderLayer::mousePressEvent(QMouseEvent *event)
{
    double bestDistance = kClickRadius;
    QString bestShipId;

    for (const RenderShip &ship : std::as_const(m_visibleShips)) {
        const double dx = event->position().x() - ship.position.x();
        const double dy = event->position().y() - ship.position.y();
        const double distance = std::sqrt(dx * dx + dy * dy);
        if (distance <= bestDistance) {
            bestDistance = distance;
            bestShipId = ship.shipId;
        }
    }

    if (!bestShipId.isEmpty()) {
        emit shipClicked(bestShipId);
        event->accept();
        return;
    }

    event->ignore();
}

void ShipRenderLayer::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size()) {
        refresh();
    }
}
