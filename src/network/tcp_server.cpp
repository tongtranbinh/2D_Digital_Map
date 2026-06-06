#include "tcp_server.h"
#include <QDebug>

TCPServer::TCPServer(QObject *parent) : QTcpServer(parent) {
}

TCPServer::~TCPServer() {
    close();
    qDeleteAll(m_clients);
}

bool TCPServer::startServer(quint16 port) {
    if (!listen(QHostAddress::Any, port)) {
        qCritical() << "TCP Server không thể khởi động trên port:" << port;
        return false;
    }
    qInfo() << "TCP Server đang lắng nghe trên port:" << port;
    return true;
}

void TCPServer::incomingConnection(qintptr socketDescriptor) {
    QTcpSocket* socket = new QTcpSocket(this);
    if (socket->setSocketDescriptor(socketDescriptor)) {
        m_clients.append(socket);
        connect(socket, &QTcpSocket::readyRead, this, &TCPServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &TCPServer::onDisconnected);
        qInfo() << "Có kết nối giả lập mới từ IP:" << socket->peerAddress().toString();
    } else {
        delete socket;
    }
}

void TCPServer::onReadyRead() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    // Đọc từng dòng byte cho tới khi không còn dòng nào (sử dụng dấu phân tách \n)
    while (socket->canReadLine()) {
        QByteArray line = socket->readLine().trimmed();
        if (line.isEmpty()) continue;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (error.error == QJsonParseError::NoError && doc.isObject()) {
            emit telemetryReceived(doc.object());
        } else {
            qWarning() << "Nhận bản tin sai định dạng JSON:" << error.errorString() << "Nội dung:" << line;
        }
    }
}

void TCPServer::onDisconnected() {
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    qInfo() << "Mất kết nối từ IP:" << socket->peerAddress().toString();
    m_clients.removeOne(socket);
    socket->deleteLater();
}
