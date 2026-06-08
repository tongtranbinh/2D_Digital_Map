#include "ShipStateStore.h"

ShipStateStore::ShipStateStore()
{
    // Khởi tạo bảng dữ liệu rỗng ban đầu bằng phép gán nguyên tử
    std::atomic_store(&m_store, std::make_shared<QHash<QUuid, ShipMessage>>());
    std::atomic_store(&m_alertZones, std::make_shared<QVector<AlertZone>>());
    std::atomic_store(&m_shipZoneStates, std::make_shared<QHash<QString, bool>>());
}

void ShipStateStore::updatePosition(const ShipMessage &msg)
{
    // Chỉ khóa luồng ghi để tránh xung đột nếu có nhiều luồng cùng ghi (Luồng đọc hoàn toàn tự do)
    std::lock_guard<std::mutex> lock(m_writeMutex);
    
    // 1. Lấy bản sao con trỏ thông minh hiện tại (Đảm bảo an toàn đa luồng)
    auto currentStore = std::atomic_load(&m_store);
    
    // 2. Tạo một bản sao mới của QHash dựa trên bản sao hiện tại (Copy-On-Write)
    auto newStore = std::make_shared<QHash<QUuid, ShipMessage>>(*currentStore);
    
    // 3. Thực hiện thay đổi (thêm/cập nhật vị trí tàu) trên bản sao mới này
    newStore->insert(msg.shipId, msg);
    
    // 4. Trỏ nguyên tử (atomic store) con trỏ dùng chung sang bảng mới
    std::atomic_store(&m_store, newStore);
}

void ShipStateStore::updatePositions(const QVector<ShipMessage> &positions)
{
    std::lock_guard<std::mutex> lock(m_writeMutex);
    auto currentStore = std::atomic_load(&m_store);
    auto newStore = std::make_shared<QHash<QUuid, ShipMessage>>(*currentStore);
    for (const ShipMessage &msg : positions) {
        newStore->insert(msg.shipId, msg);
    }
    std::atomic_store(&m_store, newStore);
}

std::optional<ShipMessage> ShipStateStore::getPosition(const QUuid &shipId) const
{
    // Đọc không khóa (Lock-free Read): Lấy con trỏ tại thời điểm hiện tại và đọc trực tiếp
    auto storePtr = std::atomic_load(&m_store);
    auto it = storePtr->constFind(shipId);
    if (it != storePtr->constEnd()) {
        return *it;
    }
    return std::nullopt;
}

QVector<ShipMessage> ShipStateStore::getAllPositions() const
{
    // Đọc không khóa (Lock-free Read)
    auto storePtr = std::atomic_load(&m_store);
    QVector<ShipMessage> positions;
    positions.reserve(storePtr->size());
    for (const ShipMessage &msg : *storePtr) {
        positions.push_back(msg);
    }
    return positions;
}

QVector<QUuid> ShipStateStore::getActiveShipIds() const
{
    // Đọc không khóa (Lock-free Read)
    auto storePtr = std::atomic_load(&m_store);
    QVector<QUuid> ids;
    ids.reserve(storePtr->size());
    for (auto it = storePtr->constBegin(); it != storePtr->constEnd(); ++it) {
        ids.push_back(it.key());
    }
    return ids;
}

void ShipStateStore::removeShip(const QUuid &shipId)
{
    std::lock_guard<std::mutex> lock(m_writeMutex);
    auto currentStore = std::atomic_load(&m_store);
    auto newStore = std::make_shared<QHash<QUuid, ShipMessage>>(*currentStore);
    newStore->remove(shipId);
    std::atomic_store(&m_store, newStore);
}

void ShipStateStore::setAlertZones(const QVector<AlertZone> &zones)
{
    std::lock_guard<std::mutex> lock(m_writeMutex);
    auto newZones = std::make_shared<QVector<AlertZone>>(zones);
    std::atomic_store(&m_alertZones, newZones);
}

QVector<AlertZone> ShipStateStore::getAlertZones() const
{
    auto zonesPtr = std::atomic_load(&m_alertZones);
    if (zonesPtr) {
        return *zonesPtr;
    }
    return QVector<AlertZone>();
}

void ShipStateStore::setShipZoneState(const QUuid &shipId, const QUuid &zoneId, bool isInside)
{
    std::lock_guard<std::mutex> lock(m_writeMutex);
    auto currentStates = std::atomic_load(&m_shipZoneStates);
    auto newStates = std::make_shared<QHash<QString, bool>>(*currentStates);
    QString key = shipId.toString() + "_" + zoneId.toString();
    newStates->insert(key, isInside);
    std::atomic_store(&m_shipZoneStates, newStates);
}

bool ShipStateStore::getShipZoneState(const QUuid &shipId, const QUuid &zoneId) const
{
    auto statesPtr = std::atomic_load(&m_shipZoneStates);
    if (!statesPtr) {
        return false;
    }
    QString key = shipId.toString() + "_" + zoneId.toString();
    auto it = statesPtr->constFind(key);
    if (it != statesPtr->constEnd()) {
        return *it;
    }
    return false;
}

void ShipStateStore::setShipZoneStates(const QHash<QString, bool> &states)
{
    std::lock_guard<std::mutex> lock(m_writeMutex);
    auto newStates = std::make_shared<QHash<QString, bool>>(states);
    std::atomic_store(&m_shipZoneStates, newStates);
}

void ShipStateStore::clear()
{
    std::lock_guard<std::mutex> lock(m_writeMutex);
    std::atomic_store(&m_store, std::make_shared<QHash<QUuid, ShipMessage>>());
    std::atomic_store(&m_alertZones, std::make_shared<QVector<AlertZone>>());
    std::atomic_store(&m_shipZoneStates, std::make_shared<QHash<QString, bool>>());
}
