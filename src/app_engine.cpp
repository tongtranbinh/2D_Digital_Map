#include "app_engine.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

AppEngine::AppEngine(QObject *parent) : QObject(parent) {
    m_dbManager = new DBManager(this);
    m_tcpServer = new TCPServer(this);
    m_shipModel = new ShipModel(this);
}

AppEngine::~AppEngine() {
    // Các đối tượng con đã được gán parent (this) nên sẽ tự động giải phóng
}

bool AppEngine::initialize(const QString &dbHost, int dbPort, const QString &dbName,
                           const QString &dbUser, const QString &dbPass, quint16 tcpPort) {
    // 1. Kết nối cơ sở dữ liệu
    if (!m_dbManager->connectDb(dbHost, dbPort, dbName, dbUser, dbPass)) {
        qCritical() << "Khởi tạo hệ thống thất bại: Không kết nối được CSDL.";
        return false;
    }

    // 2. Bắt đầu lắng nghe TCP Server
    if (!m_tcpServer->startServer(tcpPort)) {
        qCritical() << "Khởi tạo hệ thống thất bại: Không khởi động được TCP Server.";
        return false;
    }

    // 3. Kết nối tín hiệu TCP nhận tin với Slot xử lý
    connect(m_tcpServer, &TCPServer::telemetryReceived, this, &AppEngine::handleTelemetryReceived);

    qInfo() << "Hệ thống đã khởi tạo hoàn tất. Sẵn sàng nhận dữ liệu!";
    return true;
}

void AppEngine::handleTelemetryReceived(const QJsonObject &json) {
    // Phân tích dữ liệu JSON nhận được
    int mmsi = json["mmsi"].toInt();
    QString name = json["name"].toString();
    double lat = json["lat"].toDouble();
    double lon = json["lon"].toDouble();
    float speed = json["speed"].toDouble();
    float course = json["course"].toDouble();
    qint64 epoch = json["timestamp"].toVariant().toLongLong();
    QDateTime timestamp = QDateTime::fromSecsSinceEpoch(epoch);

    // 1. Đăng ký tàu nếu chưa có trong bảng ships
    m_dbManager->registerShipIfNotExist(mmsi, name);

    // 2. Lưu vết lịch sử telemetry vào database
    m_dbManager->saveTelemetry(mmsi, lat, lon, speed, course, timestamp);

    // 3. Kiểm tra vùng cảnh báo bằng hàm không gian PostGIS
    WarningAlert alert = m_dbManager->checkWarning(lat, lon);

    // 4. Kiểm tra chuyển đổi trạng thái để phát tín hiệu cảnh báo lên UI (Tránh spam thông báo liên tục)
    if (alert.isInside) {
        if (!m_activeWarnings.contains(mmsi) || m_activeWarnings.value(mmsi) != alert.zoneName) {
            m_activeWarnings[mmsi] = alert.zoneName;
            emit shipEnteredWarningZone(name, alert.zoneName, alert.severity, mmsi);
            qInfo() << QString("CẢNH BÁO: Tàu %1 (MMSI: %2) đã đi vào %3!").arg(name).arg(mmsi).arg(alert.zoneName);
        }
    } else {
        if (m_activeWarnings.contains(mmsi)) {
            m_activeWarnings.remove(mmsi);
            qInfo() << QString("THÔNG BÁO: Tàu %1 (MMSI: %2) đã ra khỏi vùng cảnh báo.").arg(name).arg(mmsi);
        }
    }

    // 5. Cập nhật vào Model hiển thị của QML Map
    Ship ship;
    ship.mmsi = mmsi;
    ship.name = name;
    ship.coordinate = QGeoCoordinate(lat, lon);
    ship.speed = speed;
    ship.course = course;
    ship.isWarning = alert.isInside;
    ship.warningName = alert.zoneName;
    ship.timestamp = timestamp;

    m_shipModel->updateOrAddShip(ship);
}

QVariantList AppEngine::getWarningZones() const {
    QVariantList list;
    QList<ZoneData> zones = m_dbManager->getWarningZones();
    for (const ZoneData &zone : zones) {
        QVariantMap map;
        map["id"] = zone.id;
        map["name"] = zone.name;
        map["severity"] = zone.severity;
        map["path"] = zone.path; // QVariantList chứa QGeoCoordinate vẽ đa giác
        list.append(map);
    }
    return list;
}
