#include "PostgresConnection.h"

#include <QSqlError>
#include <QUuid>

#include "RepositoryUtils.h"
#include <utility>

PostgresConnection::PostgresConnection(PostgresConfig config)
	: m_config(std::move(config))
{
	if (m_config.connectionName.isEmpty()) {
		m_connectionName = QStringLiteral("ship_tracking_%1").arg(uuidToString(QUuid::createUuid()));
	} else {
		m_connectionName = m_config.connectionName;
	}
}

PostgresConnection::~PostgresConnection()
{
	close();
}

bool PostgresConnection::open(QString *error)
{
	if (m_database.isValid() && m_database.isOpen()) {
		return true;
	}

	m_database = QSqlDatabase::addDatabase(QStringLiteral("QPSQL"), m_connectionName);
	m_database.setHostName(m_config.host);
	m_database.setPort(m_config.port);
	m_database.setDatabaseName(m_config.databaseName);
	m_database.setUserName(m_config.userName);
	m_database.setPassword(m_config.password);

	if (!m_database.open()) {
		if (error) {
			*error = m_database.lastError().text();
		}
		return false;
	}

	return true;
}

void PostgresConnection::close()
{
	if (m_database.isValid()) {
		m_database.close();
		m_database = QSqlDatabase();
		QSqlDatabase::removeDatabase(m_connectionName);
	}
}

bool PostgresConnection::isOpen() const
{
	return m_database.isValid() && m_database.isOpen();
}

QSqlDatabase PostgresConnection::database() const
{
	return m_database;
}

