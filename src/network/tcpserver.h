#pragma once

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSet>
#include <QHash>
#include <QByteArray>
#include <QThread>

#include "../database/PostgresConnection.h"

class ShipWorker;
class PositionWorker;
class ShipStateStore;

class TcpServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpServer(PostgresConfig dbConfig, ShipStateStore &stateStore, QObject *parent = nullptr);
    bool start(quint16 port);
    QString errorString() const;

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

signals:
    // Tín hiệu phát tin thô nhận từ socket TCP sang luồng PositionWorker phụ
    void rawMessageReceived(const QByteArray &message);

private:
    QTcpServer *server{nullptr};
    QSet<QTcpSocket*> clients;
    QHash<QTcpSocket*, QByteArray> buffers;

    // Luồng và Worker cập nhật RAM DB (ShipStateStore)
    QThread *posWorkerThread{nullptr};
    PositionWorker *posWorker{nullptr};

    // Luồng và Worker lưu trữ PostgreSQL/PostGIS bền vững (Transaction)
    QThread *shipWorkerThread{nullptr};
    ShipWorker *shipWorker{nullptr};

    ShipStateStore &m_stateStore;
};
