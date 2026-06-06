#include "db_manager.h"
#include <QDebug>
#include <QStringList>

DBManager::DBManager(QObject *parent) : QObject(parent) {
}

DBManager::~DBManager() {
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool DBManager::connectDb(const QString &host, int port, const QString &dbName, 
                          const QString &user, const QString &pass) {
    // Sử dụng driver PostgreSQL của Qt
    m_db = QSqlDatabase::addDatabase("QPSQL");
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(pass);

    if (!m_db.open()) {
        qCritical() << "Kết nối database thất bại:" << m_db.lastError().text();
        return false;
    }
    
    qInfo() << "Kết nối thành công đến PostgreSQL database:" << dbName;
    return true;
}

bool DBManager::registerShipIfNotExist(int mmsi, const QString &name) {
    if (!m_db.isOpen()) return false;

    QSqlQuery query;
    query.prepare("INSERT INTO ships (mmsi, name) VALUES (:mmsi, :name) "
                  "ON CONFLICT (mmsi) DO NOTHING;");
    query.bindValue(":mmsi", mmsi);
    query.bindValue(":name", name);

    if (!query.exec()) {
        qWarning() << "Không thể đăng ký tàu mới:" << query.lastError().text();
        return false;
    }
    return true;
}

bool DBManager::saveTelemetry(int mmsi, double lat, double lon, float speed, float course, const QDateTime &timestamp) {
    if (!m_db.isOpen()) return false;

    QSqlQuery query;
    // Điểm GPS PostGIS lưu dưới dạng ST_SetSRID(ST_Point(lon, lat), 4326) - Kinh độ trước, Vĩ độ sau
    query.prepare("INSERT INTO ship_telemetry (mmsi, geom, speed, course, timestamp) "
                  "VALUES (:mmsi, ST_SetSRID(ST_Point(:lon, :lat), 4326), :speed, :course, :timestamp);");
    query.bindValue(":mmsi", mmsi);
    query.bindValue(":lon", lon);
    query.bindValue(":lat", lat);
    query.bindValue(":speed", speed);
    query.bindValue(":course", course);
    query.bindValue(":timestamp", timestamp);

    if (!query.exec()) {
        qWarning() << "Lưu telemetry thất bại:" << query.lastError().text();
        return false;
    }
    return true;
}

WarningAlert DBManager::checkWarning(double lat, double lon) {
    WarningAlert alert;
    if (!m_db.isOpen()) return alert;

    QSqlQuery query;
    // Kiểm tra xem Point(lon, lat) có nằm trong bất kỳ Polygon nào trong bảng warning_zones không
    query.prepare("SELECT name, severity FROM warning_zones "
                  "WHERE ST_Contains(geom, ST_SetSRID(ST_Point(:lon, :lat), 4326)) "
                  "LIMIT 1;");
    query.bindValue(":lon", lon);
    query.bindValue(":lat", lat);

    if (query.exec() && query.next()) {
        alert.isInside = true;
        alert.zoneName = query.value(0).toString();
        alert.severity = query.value(1).toString();
    }
    return alert;
}

QList<ZoneData> DBManager::getWarningZones() {
    QList<ZoneData> zones;
    if (!m_db.isOpen()) return zones;

    QSqlQuery query("SELECT id, name, severity, ST_AsText(geom) FROM warning_zones;");
    while (query.next()) {
        ZoneData zone;
        zone.id = query.value(0).toInt();
        zone.name = query.value(1).toString();
        zone.severity = query.value(2).toString();
        
        QString wkt = query.value(3).toString();
        zone.path = parseWktPolygon(wkt);
        
        zones.append(zone);
    }
    return zones;
}

QVariantList DBManager::parseWktPolygon(const QString &wkt) {
    QVariantList list;
    QString cleaned = wkt;
    
    // Loại bỏ các chữ cái POLYGON và ngoặc để lấy cụm số tọa độ
    cleaned.remove("POLYGON");
    cleaned.remove("(");
    cleaned.remove(")");
    cleaned = cleaned.trimmed();
    
    // Split theo dấu phẩy để ra từng điểm tọa độ "lon lat"
    QStringList tokens = cleaned.split(",", Qt::SkipEmptyParts);

    for (const QString &token : tokens) {
        QString trimmedToken = token.trimmed();
        
        // Tách kinh độ và vĩ độ qua dấu cách
        QStringList coords = trimmedToken.split(" ", Qt::SkipEmptyParts);
        if (coords.size() >= 2) {
            double lon = coords.at(0).toDouble();
            double lat = coords.at(1).toDouble();
            // Thêm tọa độ QGeoCoordinate vào QVariantList
            list.append(QVariant::fromValue(QGeoCoordinate(lat, lon)));
        }
    }
    return list;
}
