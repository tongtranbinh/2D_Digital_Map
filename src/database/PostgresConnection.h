#pragma once

#include <QString>
#include <QtSql/QSqlDatabase>

struct PostgresConfig {
    QString host = QStringLiteral("localhost");
    int port = 5432;
    QString databaseName;
    QString userName;
    QString password;
    QString connectionName;
};

class PostgresConnection final
{
public:
    explicit PostgresConnection(PostgresConfig config = {});
    ~PostgresConnection();

    bool open(QString *error = nullptr);
    void close();
    bool isOpen() const;
    QSqlDatabase database() const;

private:
    PostgresConfig m_config;
    QString m_connectionName;
    QSqlDatabase m_database;
};
