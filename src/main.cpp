#include <QApplication>
#include <QMetaType>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QProcessEnvironment>

#include "model/VesselMessage.h"
#include "network/tcpserver.h"
#include "database/PostgresConnection.h"
#include "state/ShipStateStore.h"
#include "ui/MapController.h"
#include "ui/ShipRenderLayer.h"
#include "worker/PositionWorker.h"
#include "worker/ShipWorker.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<ShipMessage>("ShipMessage");
    qRegisterMetaType<QVector<ShipMessage>>("QVector<ShipMessage>");
    qRegisterMetaType<AlertEvent>("AlertEvent");
    qRegisterMetaType<QVector<AlertEvent>>("QVector<AlertEvent>");
    qRegisterMetaType<AlertZone>("AlertZone");

    qmlRegisterType<ShipRenderLayer>("ShipTracking", 1, 0, "ShipRenderLayer");

    // 1. Cấu hình Postgres — đọc từ environment variables (xem deploy/shiptracking.env)
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    PostgresConfig config;
    config.host         = env.value(QStringLiteral("DB_HOST"),     QStringLiteral("localhost"));
    config.port         = env.value(QStringLiteral("DB_PORT"),     QStringLiteral("5432")).toInt();
    config.databaseName = env.value(QStringLiteral("DB_NAME"),     QStringLiteral("ship_tracking"));
    config.userName     = env.value(QStringLiteral("DB_USER"),     QStringLiteral("ship_user"));
    config.password     = env.value(QStringLiteral("DB_PASSWORD"), QStringLiteral("123456"));

    const quint16 tcpPort = static_cast<quint16>(
        env.value(QStringLiteral("TCP_PORT"), QStringLiteral("9000")).toUInt()
    );

    // 2. Khởi tạo Database trên RAM
    ShipStateStore stateStore;

    // 3. Khởi tạo TCP Server và truyền cấu hình cùng RAM DB vào
    TcpServer server(config, stateStore);

    if (!server.start(tcpPort)) {
        qDebug() << "Unable to start the server:" << server.errorString();
        return 1;
    }

    // 4. Khởi tạo lớp Bridge MapController kết nối QML với RAM Cache
    MapController mapController(stateStore);

    // Đồng bộ hóa bất đồng bộ khi khởi chạy: initZones chỉ được kích hoạt sau khi
    // ShipWorker hoàn tất tải và lưu các vùng cảnh báo từ CSDL lên RAM.
    QObject::connect(server.dbWorker(), &ShipWorker::cachePreloaded,
                     &mapController, &MapController::initZones,
                     Qt::QueuedConnection);

    // Kết nối các tín hiệu thời gian thực từ PositionWorker phụ sang MapController trong UI thread
    QObject::connect(server.positionWorker(), &PositionWorker::positionsUpdated,
                     &mapController, &MapController::handlePositionsUpdated,
                     Qt::QueuedConnection);

    QObject::connect(server.positionWorker(), &PositionWorker::alertEventOccurred,
                     &mapController, &MapController::handleAlertEvent,
                     Qt::QueuedConnection);

    // Kết nối bất đồng bộ yêu cầu và phản hồi truy vấn lịch sử hành trình từ CSDL (giữa UI thread và DB thread)
    QObject::connect(&mapController, &MapController::requestTrackHistory,
                     server.dbWorker(), &ShipWorker::handleTrackHistoryRequest,
                     Qt::QueuedConnection);

    QObject::connect(server.dbWorker(), &ShipWorker::trackHistoryLoaded,
                     &mapController, &MapController::handleTrackHistoryLoaded,
                     Qt::QueuedConnection);

    QObject::connect(&mapController, &MapController::requestSaveZone,
                     server.dbWorker(), &ShipWorker::handleSaveZoneRequest,
                     Qt::QueuedConnection);

    // 5. Khởi tạo Engine QML và tải giao diện chính
    QQmlApplicationEngine engine;

    // Đăng ký mapController làm context property để QML gọi được trực tiếp
    engine.rootContext()->setContextProperty(QStringLiteral("mapController"), &mapController);

    const QUrl url(QStringLiteral("qrc:/ShipTracking/src/ui/qml/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
