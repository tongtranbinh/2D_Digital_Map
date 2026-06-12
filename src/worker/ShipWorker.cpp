#include "ShipWorker.h"
#include "../state/ShipStateStore.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlError>

ShipWorker::ShipWorker(PostgresConfig dbConfig, ShipStateStore &stateStore, QObject *parent)
    : QObject(parent), m_config(std::move(dbConfig)), m_stateStore(stateStore)
{
}

ShipWorker::~ShipWorker()
{
    delete m_shipService;
    delete m_posService;
    delete m_alertService;
    delete m_shipRepo;
    delete m_posRepo;
    delete m_alertRepo;
    delete m_db;
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

    // Bắt đầu một Transaction để tối ưu tối đa hiệu năng ghi hàng loạt
    QSqlDatabase db = m_db->database();
    db.transaction();

    QString dbError;

    // 1. Kiểm tra sự tồn tại của các tàu qua RAM cache (và insert nếu chưa có)
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
        }
    }

    // 2. Ghi hàng loạt vị trí xuống DB (Batch Insert)
    if (!packets.isEmpty()) {
        if (!m_posService->recordPositionsBatch(const_cast<QVector<ShipMessage>&>(packets), &dbError)) {
            qWarning() << "[ShipWorker] Batch insert positions failed. Error:" << dbError;
            db.rollback();
            emit dbErrorOccurred(QStringLiteral("Batch transaction rolled back: %1").arg(dbError));
            return;
        }
    }

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
    qInfo() << QStringLiteral("[ShipWorker] Batch transaction successfully committed. Saved %1 vessel records and %2 alert events.")
               .arg(successCount)
               .arg(alertEvents.size());

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