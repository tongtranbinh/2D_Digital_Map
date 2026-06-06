#ifndef APP_ENGINE_H
#define APP_ENGINE_H

#include <QObject>
#include <QQmlApplicationEngine>
#include "db/db_manager.h"
#include "network/tcp_server.h"
#include "models/ship_model.h"

class AppEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(ShipModel* shipModel READ shipModel CONSTANT)

public:
    explicit AppEngine(QObject *parent = nullptr);
    ~AppEngine();

    // Khởi tạo toàn bộ hệ thống (kết nối DB, bật TCP server)
    bool initialize(const QString &dbHost, int dbPort, const QString &dbName,
                    const QString &dbUser, const QString &dbPass, quint16 tcpPort);

    ShipModel* shipModel() const { return m_shipModel; }

    // Cho phép QML lấy danh sách đa giác vùng cảnh báo để vẽ
    Q_INVOKABLE QVariantList getWarningZones() const;

signals:
    // Tín hiệu phát đi khi có tàu vi phạm vùng cảnh báo để QML hiển thị Popup/Notification
    void shipEnteredWarningZone(const QString &shipName, const QString &zoneName, const QString &severity, int mmsi);

private slots:
    // Xử lý khi nhận được bản tin tọa độ từ TCP Server
    void handleTelemetryReceived(const QJsonObject &json);

private:
    DBManager* m_dbManager = nullptr;
    TCPServer* m_tcpServer = nullptr;
    ShipModel* m_shipModel = nullptr;
    QHash<int, QString> m_activeWarnings; // Lưu mmsi -> tên vùng cảnh báo để tránh spam alert
};

#endif // APP_ENGINE_H
