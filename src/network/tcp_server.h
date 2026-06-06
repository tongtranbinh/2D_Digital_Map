#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>

class TCPServer : public QTcpServer {
    Q_OBJECT
public:
    explicit TCPServer(QObject *parent = nullptr);
    ~TCPServer();

    // Khởi động server lắng nghe trên port chỉ định
    bool startServer(quint16 port);

signals:
    // Tín hiệu phát đi khi nhận và parse thành công một bản tin telemetry dạng JSON
    void telemetryReceived(const QJsonObject &json);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QList<QTcpSocket*> m_clients;
};

#endif // TCP_SERVER_H
