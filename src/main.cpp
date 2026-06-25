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

    // Doc tat ca config tu environment variables (xem deploy/shiptracking.env)
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // Headless: backend service chay offscreen, khong can UI
    // UI mode: chay voi display thuc (xcb) hoac Xvfb
    const bool isHeadless = (env.value(QStringLiteral("QT_QPA_PLATFORM")) == QStringLiteral("offscreen"));

    // 1. Cau hinh Postgres
    PostgresConfig config;
    config.host         = env.value(QStringLiteral("DB_HOST"),     QStringLiteral("localhost"));
    config.port         = env.value(QStringLiteral("DB_PORT"),     QStringLiteral("5432")).toInt();
    config.databaseName = env.value(QStringLiteral("DB_NAME"),     QStringLiteral("ship_tracking"));
    config.userName     = env.value(QStringLiteral("DB_USER"),     QStringLiteral("ship_user"));
    config.password     = env.value(QStringLiteral("DB_PASSWORD"), QStringLiteral("123456"));

    const quint16 tcpPort = static_cast<quint16>(
        env.value(QStringLiteral("TCP_PORT"), QStringLiteral("9000")).toUInt()
    );

    // OSM Tile URL -- ghep tu OSM_TILE_HOST + OSM_TILE_PORT
    const QString osmHost = env.value(QStringLiteral("OSM_TILE_HOST"), QStringLiteral("localhost"));
    const QString osmPort = env.value(QStringLiteral("OSM_TILE_PORT"), QStringLiteral("8080"));
    const QString tileUrl = QStringLiteral("http://%1:%2/tile/").arg(osmHost, osmPort);

    // 2. Khoi tao RAM cache
    ShipStateStore stateStore;

    // 3. Khoi tao TCP Server
    TcpServer server(config, stateStore);

    if (!server.start(tcpPort)) {
        if (isHeadless) {
            // Backend mode: TCP la bat buoc
            qCritical() << "[Main] FATAL: TCP server failed on port" << tcpPort << ":" << server.errorString();
            return 1;
        } else {
            // UI mode: khong can TCP, doc data tu DB la du
            qWarning() << "[Main] WARNING: TCP server failed on port" << tcpPort << "(port in use). Running in read-only UI mode.";
        }
    }

    // 4. Khoi tao MapController ket noi QML voi RAM Cache
    MapController mapController(stateStore);

    QObject::connect(server.dbWorker(), &ShipWorker::cachePreloaded,
                     &mapController, &MapController::initZones,
                     Qt::QueuedConnection);

    QObject::connect(server.positionWorker(), &PositionWorker::positionsUpdated,
                     &mapController, &MapController::handlePositionsUpdated,
                     Qt::QueuedConnection);

    QObject::connect(server.positionWorker(), &PositionWorker::alertEventOccurred,
                     &mapController, &MapController::handleAlertEvent,
                     Qt::QueuedConnection);

    QObject::connect(&mapController, &MapController::requestTrackHistory,
                     server.dbWorker(), &ShipWorker::handleTrackHistoryRequest,
                     Qt::QueuedConnection);

    QObject::connect(server.dbWorker(), &ShipWorker::trackHistoryLoaded,
                     &mapController, &MapController::handleTrackHistoryLoaded,
                     Qt::QueuedConnection);

    QObject::connect(&mapController, &MapController::requestSaveZone,
                     server.dbWorker(), &ShipWorker::handleSaveZoneRequest,
                     Qt::QueuedConnection);

    // 5. QML Engine
    QQmlApplicationEngine engine;
    if (!isHeadless) {
        engine.rootContext()->setContextProperty(QStringLiteral("mapController"), &mapController);
        engine.rootContext()->setContextProperty(QStringLiteral("tileUrl"), tileUrl);

        const QUrl url(QStringLiteral("qrc:/ShipTracking/src/ui/qml/main.qml"));
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                         &app, [url, isHeadless](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                if (isHeadless) {
                    QCoreApplication::exit(-1);
                } else {
                    qWarning() << "[Main] WARNING: QML failed to load. UI unavailable but backend continues.";
                }
            }
        }, Qt::QueuedConnection);
        engine.load(url);
    } else {
        qInfo() << "[Main] Headless mode: QML UI skipped. Backend running.";
    }

    return app.exec();
}
