#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariantList>
#include <QGeoCoordinate>
#include <QDateTime>

struct WarningAlert {
    bool isInside = false;
    QString zoneName = "";
    QString severity = ""; // "warning" hoặc "critical"
};

struct ZoneData {
    int id;
    QString name;
    QString severity;
    QVariantList path; // Danh sách QGeoCoordinate vẽ đa giác
};

class DBManager : public QObject {
    Q_OBJECT
public:
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager();

    // Kết nối CSDL PostgreSQL
    bool connectDb(const QString &host, int port, const QString &dbName, 
                   const QString &user, const QString &pass);
    
    // Lưu telemetry tàu
    bool saveTelemetry(int mmsi, double lat, double lon, float speed, float course, const QDateTime &timestamp);
    
    // Đăng ký tàu mới nếu chưa có (dựa trên mmsi nhận được)
    bool registerShipIfNotExist(int mmsi, const QString &name);
    
    // Kiểm tra tàu đi vào vùng cảnh báo bằng PostGIS ST_Contains
    WarningAlert checkWarning(double lat, double lon);
    
    // Lấy toàn bộ vùng cảnh báo để hiển thị lên bản đồ
    QList<ZoneData> getWarningZones();

private:
    QSqlDatabase m_db;
    
    // Hàm phụ phân tích chuỗi POLYGON((lon lat, lon lat...)) của PostGIS thành danh sách tọa độ
    QVariantList parseWktPolygon(const QString &wkt);
};

#endif // DB_MANAGER_H
