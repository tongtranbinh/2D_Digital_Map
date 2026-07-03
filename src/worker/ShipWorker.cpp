#include "ShipWorker.h"
#include "../state/ShipStateStore.h"
#include <QDebug>
#include <QElapsedTimer>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

ShipWorker::ShipWorker(PostgresConfig dbConfig, ShipStateStore &stateStore, QObject *parent)
    : QObject(parent), m_config(std::move(dbConfig)), m_stateStore(stateStore)
{
}

ShipWorker::~ShipWorker()
{
    cleanup();
}

void ShipWorker::cleanup()
{
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
        delete m_cleanupTimer;
        m_cleanupTimer = nullptr;
    }
    delete m_shipService;  m_shipService = nullptr;
    delete m_posService;   m_posService = nullptr;
    delete m_alertService; m_alertService = nullptr;
    delete m_shipRepo;     m_shipRepo = nullptr;
    delete m_posRepo;      m_posRepo = nullptr;
    delete m_alertRepo;    m_alertRepo = nullptr;
    delete m_db;           m_db = nullptr;
}

void ShipWorker::initialize()
{
    if (!m_db) {
        m_db = new PostgresConnection(m_config);
        
        m_shipRepo = new ShipRepository(*m_db);
        m_posRepo = new PositionRepository(*m_db);
        m_alertRepo = new AlertRepository(*m_db);

        m_shipService = new ShipService(*m_shipRepo);
        m_posService = new PositionService(*m_posRepo);
        m_alertService = new AlertService(*m_alertRepo);
    }

    if (!m_db->isOpen() && !m_db->open()) {
        emit dbErrorOccurred(QStringLiteral("Failed to open DB connection during initialization: %1").arg(m_db->database().lastError().text()));
        return;
    }

    // Nạp sẵn toàn bộ Cache (vùng cảnh báo + trạng thái tàu) lên RAM từ DB
    QString preloadError;
    m_alertService->preloadCache(&preloadError);
    if (!preloadError.isEmpty()) {
        qWarning() << "[ShipWorker] Failed to preload Alert cache during initialization:" << preloadError;
    } else {
        qInfo() << "[ShipWorker] Alert zones and ship states preloaded successfully during initialization.";
        // Đồng bộ dữ liệu cache từ DB vào RAM (ShipStateStore) để PositionWorker sử dụng
        m_stateStore.setAlertZones(m_alertService->getAllZones());
        m_stateStore.setShipZoneStates(m_alertRepo->loadShipZoneStates());
        
        // Đồng bộ các vị trí tàu mới nhất từ CSDL lên RAM
        QString posError;
        QVector<ShipMessage> latestPositions = m_posRepo->loadLatestPositions(&posError);
        if (!posError.isEmpty()) {
            qWarning() << "[ShipWorker] Failed to load latest ship positions during initialization:" << posError;
        } else {
            m_stateStore.updatePositions(latestPositions);
            qInfo() << QStringLiteral("[ShipWorker] Loaded %1 latest ship positions from DB to RAM cache.")
                       .arg(latestPositions.size());
        }
    }
    emit cachePreloaded();

    // Khởi tạo timer dọn dẹp hàng ngày (5 phút kiểm tra một lần)
    if (!m_cleanupTimer) {
        m_cleanupTimer = new QTimer(this);
        connect(m_cleanupTimer, &QTimer::timeout, this, &ShipWorker::performDailyCleanup);
        m_cleanupTimer->start(300000); // 5 phút
    }
    // Chạy dọn dẹp một lần lúc khởi động
    QMetaObject::invokeMethod(this, &ShipWorker::performDailyCleanup, Qt::QueuedConnection);
}

void ShipWorker::savePendingPackets(const QVector<ShipMessage> &packets, const QVector<AlertEvent> &alertEvents)
{
    if (packets.isEmpty() && alertEvents.isEmpty()) {
        return;
    }

    if (!m_db) {
        initialize();
    }

    if (!m_db || (!m_db->isOpen() && !m_db->open())) {
        emit dbErrorOccurred(QStringLiteral("Failed to open DB connection: %1").arg(m_db ? m_db->database().lastError().text() : QStringLiteral("DB not initialized")));
        return;
    }

    QElapsedTimer transactionTimer;
    transactionTimer.start();

    // Bắt đầu một Transaction để tối ưu tối đa hiệu năng ghi hàng loạt
    QSqlDatabase db = m_db->database();
    db.transaction();

    QString dbError;

    // 1. Kiểm tra sự tồn tại của các tàu qua RAM cache (và insert nếu chưa có)
    int newShipsCount = 0;
    for (const ShipMessage &msg : packets) {
        if (!m_shipService->shipExists(msg.shipId, &dbError)) {
            Vessel newVessel;
            newVessel.id = msg.shipId;
            newVessel.name = QStringLiteral("Unknown Vessel");
            newVessel.mmsi = 100000000 + (qHash(msg.shipId) % 900000000);
            if (!m_shipService->saveShip(newVessel, &dbError)) {
                qWarning() << "[ShipWorker] Save ship failed. Error:" << dbError;
                db.rollback();
                emit dbErrorOccurred(QStringLiteral("Batch transaction rolled back: %1").arg(dbError));
                return;
            }
            newShipsCount++;
        }
    }

    QElapsedTimer batchInsertTimer;
    batchInsertTimer.start();

    // 2. Ghi hàng loạt vị trí xuống DB (Batch Insert)
    if (!packets.isEmpty()) {
        if (!m_posService->recordPositionsBatch(const_cast<QVector<ShipMessage>&>(packets), &dbError)) {
            qWarning() << "[ShipWorker] Batch insert positions failed. Error:" << dbError;
            db.rollback();
            emit dbErrorOccurred(QStringLiteral("Batch transaction rolled back: %1").arg(dbError));
            return;
        }
    }

    qint64 batchInsertTimeMs = batchInsertTimer.elapsed();

    // 3. Ghi các sự kiện cảnh báo (Alert Event) và lưu trạng thái vào CSDL
    for (const AlertEvent &event : alertEvents) {
        AlertEvent ev = event; // copy to non-const
        if (!m_alertService->logEvent(ev, &dbError)) {
            qWarning() << "[ShipWorker] Log alert event failed. Error:" << dbError;
            db.rollback();
            emit dbErrorOccurred(QStringLiteral("Batch transaction rolled back: %1").arg(dbError));
            return;
        }

        bool isInside = (event.eventType == QStringLiteral("ENTER"));
        if (!m_alertService->saveShipZoneState(event.vesselId, event.alertZoneId, isInside, &dbError)) {
            qWarning() << "[ShipWorker] Save ship zone state failed. Error:" << dbError;
            db.rollback();
            emit dbErrorOccurred(QStringLiteral("Batch transaction rolled back: %1").arg(dbError));
            return;
        }
    }

    // Commit toàn bộ bản ghi xuống ổ cứng
    db.commit();
    int successCount = packets.size();
    
    qint64 totalTransactionTimeMs = transactionTimer.elapsed();
    int totalRecords = successCount + alertEvents.size();
    double throughput = totalTransactionTimeMs > 0 ? (totalRecords * 1000.0 / totalTransactionTimeMs) : 0.0;

    qInfo().noquote() << QStringLiteral("[PERF][DB] Committed batch transaction:\n"
                                        "  * DB Commit:  %1 ms total (Registered %2 new vessels)\n"
                                        "  * Batch Pos:  %3 ms for %4 positions (Avg: %5 ms/pos)\n"
                                        "  * Alerts:      Saved %6 alert events\n"
                                        "  * Throughput:  %7 records/sec")
                       .arg(totalTransactionTimeMs)
                       .arg(newShipsCount)
                       .arg(batchInsertTimeMs)
                       .arg(successCount)
                       .arg(successCount > 0 ? QString::number((double)batchInsertTimeMs / successCount, 'f', 3) : "0.000")
                       .arg(alertEvents.size())
                       .arg(totalTransactionTimeMs > 0 ? QString::number(throughput, 'f', 1) : "N/A");

    emit batchProcessed(successCount);
}

void ShipWorker::handleTrackHistoryRequest(const QUuid &vesselId)
{
    if (!m_db) {
        initialize();
    }
    if (!m_db || (!m_db->isOpen() && !m_db->open())) {
        qWarning() << "[ShipWorker] DB connection not open, cannot retrieve track history";
        emit trackHistoryLoaded(vesselId, QVector<ShipMessage>());
        return;
    }

    QString error;
    QVector<ShipMessage> history = m_posService->getPositionHistory(vesselId, 100, &error);
    if (!error.isEmpty()) {
        qWarning() << "[ShipWorker] Failed to load position history for ship" << vesselId.toString() << ":" << error;
    }
    emit trackHistoryLoaded(vesselId, history);
}

void ShipWorker::performDailyCleanup()
{
    QDate today = QDate::currentDate();
    if (m_lastCleanupDate == today) {
        return; // Đã dọn dẹp hôm nay rồi
    }

    if (!m_db) {
        initialize();
    }

    if (!m_db || (!m_db->isOpen() && !m_db->open())) {
        qWarning() << "[ShipWorker] DB connection not open, cannot run daily cleanup";
        return;
    }

    QSqlDatabase db = m_db->database();
    QSqlQuery query(db);

    // Xóa vị trí lưu trữ cũ hơn 1 ngày
    if (query.exec(QStringLiteral("DELETE FROM app.positions WHERE recorded_at < NOW() - INTERVAL '1 day';"))) {
        qInfo() << "[ShipWorker] Daily cleanup successful: deleted positions older than 1 day.";
        m_lastCleanupDate = today;
    } else {
        qWarning() << "[ShipWorker] Daily cleanup failed:" << query.lastError().text();
    }
}

void ShipWorker::handleSaveZoneRequest(const AlertZone &zone)
{
    if (!m_db) {
        initialize();
    }
    if (!m_db || (!m_db->isOpen() && !m_db->open())) {
        qWarning() << "[ShipWorker] DB connection not open, cannot save zone";
        return;
    }

    QString error;
    AlertZone z = zone;
    if (m_alertRepo->saveZone(z, &error)) {
        qInfo() << "[ShipWorker] Successfully saved new zone to DB:" << z.name;
    } else {
        qWarning() << "[ShipWorker] Failed to save zone to DB:" << error;
    }
}

void ShipWorker::handleDeleteZoneRequest(const QUuid &zoneId)
{
    if (!m_db) {
        initialize();
    }
    if (!m_db || (!m_db->isOpen() && !m_db->open())) {
        qWarning() << "[ShipWorker] DB connection not open, cannot delete zone";
        return;
    }

    QString error;
    if (m_alertService->deleteZone(zoneId, &error)) {
        qInfo() << "[ShipWorker] Successfully deleted zone from DB:" << zoneId.toString();
    } else {
        qWarning() << "[ShipWorker] Failed to delete zone from DB:" << error;
    }
}