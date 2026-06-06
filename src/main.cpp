#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCommandLineParser>
#include <QDebug>
#include "app_engine.h"

int main(int argc, char *argv[]) {
    // Kích hoạt High DPI scaling
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QGuiApplication app(argc, argv);
    app.setApplicationName("2D Maritime Digital Map Demo");
    app.setApplicationVersion("1.0.0");

    // Cấu hình Parser dòng lệnh để người dùng có thể đổi tham số kết nối khi demo
    QCommandLineParser parser;
    parser.setApplicationDescription("Hệ thống giám sát tàu biển mặt nước thời gian thực");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption dbHostOption(QStringList() << "db-host", "Postgres Host IP", "host", "127.0.0.1");
    QCommandLineOption dbPortOption(QStringList() << "db-port", "Postgres Port", "port", "5432");
    QCommandLineOption dbNameOption(QStringList() << "db-name", "Database Name", "name", "digital_map_db");
    QCommandLineOption dbUserOption(QStringList() << "db-user", "Database Username", "user", "postgres");
    QCommandLineOption dbPassOption(QStringList() << "db-pass", "Database Password", "password", "postgres"); // đổi tùy theo máy người dùng
    QCommandLineOption tcpPortOption(QStringList() << "tcp-port", "TCP Server Port to listen", "port", "5555");

    parser.addOption(dbHostOption);
    parser.addOption(dbPortOption);
    parser.addOption(dbNameOption);
    parser.addOption(dbUserOption);
    parser.addOption(dbPassOption);
    parser.addOption(tcpPortOption);
    parser.process(app);

    // 1. Tạo core AppEngine
    AppEngine appEngine;

    // 2. Khởi tạo kết nối DB & TCP Server
    QString dbHost = parser.value(dbHostOption);
    int dbPort = parser.value(dbPortOption).toInt();
    QString dbName = parser.value(dbNameOption);
    QString dbUser = parser.value(dbUserOption);
    QString dbPass = parser.value(dbPassOption);
    quint16 tcpPort = parser.value(tcpPortOption).toInt();

    qInfo() << "Đang kết nối database..." << dbHost << ":" << dbPort << "DBName:" << dbName;
    if (!appEngine.initialize(dbHost, dbPort, dbName, dbUser, dbPass, tcpPort)) {
        qCritical() << "Không thể khởi động chương trình do lỗi kết nối hoặc socket.";
        return -1;
    }

    // 3. Khởi tạo QML engine và inject các module C++ vào ngữ cảnh QML
    QQmlApplicationEngine engine;
    
    // Inject Engine và Model vào QML dưới dạng Context Property
    engine.rootContext()->setContextProperty("appEngine", &appEngine);
    engine.rootContext()->setContextProperty("shipModel", appEngine.shipModel());

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);

    return app.exec();
}
