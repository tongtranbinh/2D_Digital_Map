#include "tcpserver.h"
#include "../worker/ShipWorker.h"
#include "../worker/PositionWorker.h"
#include "../state/ShipStateStore.h"
#include "../model/AlertMessage.h"

#include <QDebug>
#include <QHostAddress>

TcpServer::TcpServer(PostgresConfig dbConfig, ShipStateStore &stateStore, QObject *parent) 
    :   QObject(parent), 
        server(new QTcpServer(this)),
        posWorkerThread(new QThread(this)),
        posWorker(new PositionWorker(stateStore)),
        shipWorkerThread(new QThread(this)),
        shipWorker(new ShipWorker(dbConfig, stateStore)),
        m_stateStore(stateStore)
{
    qRegisterMetaType<ShipMessage>("ShipMessage");
    qRegisterMetaType<QVector<ShipMessage>>("QVector<ShipMessage>");
    qRegisterMetaType<AlertEvent>("AlertEvent");
    qRegisterMetaType<QVector<AlertEvent>>("QVector<AlertEvent>");

    // 1. Chuyển các Worker sang các luồng phụ riêng biệt
    posWorker->moveToThread(posWorkerThread);
    shipWorker->moveToThread(shipWorkerThread);

    // 2. Kết nối luồng chính (TCP) -> Luồng PositionWorker (RAM DB)
    connect(this, &TcpServer::rawMessageReceived,
            posWorker, &PositionWorker::processRawMessage,
            Qt::QueuedConnection);

    // 4. Kích hoạt Timer 5s của PositionWorker khi luồng của nó chính thức bắt đầu
    connect(posWorkerThread, &QThread::started,
            posWorker, &PositionWorker::startTimer);

    // Kích hoạt nạp cache DB lên RAM ngay lập tức khi luồng ShipWorker bắt đầu
    connect(shipWorkerThread, &QThread::started,
            shipWorker, &ShipWorker::initialize);

    // 5. Kết nối Luồng PositionWorker -> Luồng ShipWorker (PostgreSQL DB, chu kỳ 5s)
    connect(posWorker, &PositionWorker::pendingPacketsReady,
            shipWorker, &ShipWorker::savePendingPackets,
            Qt::QueuedConnection);

    // 6. Xử lý log lỗi từ các luồng
    connect(posWorker, &PositionWorker::parsingError,
            this, [](const QString &error, const QByteArray &raw) {
                qWarning() << "[PositionWorker Error] Parse JSON failed:" << error << "Raw:" << raw;
            },
            Qt::QueuedConnection);

    connect(shipWorker, &ShipWorker::dbErrorOccurred,
            this, [](const QString &error) {
                qWarning() << "[ShipWorker Error] Database transaction failed:" << error;
            },
            Qt::QueuedConnection);

    connect(shipWorker, &ShipWorker::batchProcessed,
            this, [](int count) {
                qInfo() << "[Main] DB Worker finished batch. Successfully saved" << count << "vessels.";
            },
            Qt::QueuedConnection);

    // 7. Bắt đầu chạy các luồng phụ
    posWorkerThread->start();
    shipWorkerThread->start();

    connect(server, &QTcpServer::newConnection,
            this, &TcpServer::onNewConnection);
}

TcpServer::~TcpServer()
{
    if (posWorker && posWorkerThread && posWorkerThread->isRunning()) {
        connect(posWorker, &QObject::destroyed, posWorkerThread, &QThread::quit, Qt::DirectConnection);
        posWorker->deleteLater();
        posWorkerThread->wait();
    } else {
        delete posWorker;
    }

 
    if (shipWorker && shipWorkerThread && shipWorkerThread->isRunning()) {
        connect(shipWorker, &QObject::destroyed, shipWorkerThread, &QThread::quit, Qt::DirectConnection);
        shipWorker->deleteLater();
        shipWorkerThread->wait();
    } else {
        delete shipWorker;
    }
}


bool TcpServer::start(quint16 port)
{
    bool ok = server->listen(QHostAddress::Any, port);

    if (!ok) {
        qCritical() << "Server start failed:" << server->errorString();
        return false;
    }

    qInfo() << "TCP server listening on port" << port;
    return true;
}

QString TcpServer::errorString() const
{
    return server ? server->errorString() : QString();
}

void TcpServer::onNewConnection()
{
    while (server->hasPendingConnections()) {
        QTcpSocket *client = server->nextPendingConnection();

        clients.insert(client);

        qInfo() << "New client connected:"
                << client->peerAddress().toString()
                << client->peerPort();

        connect(client, &QTcpSocket::readyRead,
                this, &TcpServer::onReadyRead);

        connect(client, &QTcpSocket::disconnected,
                this, &TcpServer::onClientDisconnected);

        client->write("Welcome to Qt TCP Server\n");
    }
}

void TcpServer::onReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());

    if (!client) {
        return;
    }

    buffers[client].append(client->readAll());

    while (true) {
        int newlineIndex = buffers[client].indexOf('\n');

        if (newlineIndex == -1) {
            break;
        }

        QByteArray oneMessage = buffers[client].left(newlineIndex).trimmed();

        buffers[client].remove(0, newlineIndex + 1);

        if (oneMessage.isEmpty()) {
            continue;
        }

        // Phát thẳng dữ liệu thô sang luồng phụ PositionWorker (Hoàn toàn không block luồng mạng chính)
        emit rawMessageReceived(oneMessage);
    }
}

void TcpServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());

    if (!client) {
        return;
    }

    qInfo() << "Client disconnected:"
            << client->peerAddress().toString()
            << client->peerPort();

    clients.remove(client);
    client->deleteLater();
}