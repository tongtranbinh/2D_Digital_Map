#include <QApplication>
#include <QMetaType>

#include "model/VesselMessage.h"
#include "network/tcpserver.h"
#include "database/PostgresConnection.h"
#include "state/ShipStateStore.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<ShipMessage>("ShipMessage");

    // 1. Cấu hình Postgres (Cách 1: Dependency Injection)
    PostgresConfig config;
    config.host = QStringLiteral("localhost");
    config.port = 5432;
    config.databaseName = QStringLiteral("ship_tracking"); // Tên database của bạn
    config.userName = QStringLiteral("ship_user");     // Username của bạn
    config.password = QStringLiteral("123456");       // Mật khẩu của bạn

    // 2. Khởi tạo Database trên RAM (ShipStateStore) để UI lấy dữ liệu thời gian thực nhanh nhất
    ShipStateStore stateStore;

    // 3. Khởi tạo TCP Server và truyền cấu hình cùng RAM DB vào
    TcpServer server(config, stateStore);

    if (!server.start(9000)) {
        qDebug() << "Unable to start the server:" << server.errorString();
        return 1;
    }

    return app.exec();
}