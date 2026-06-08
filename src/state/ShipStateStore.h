#pragma once

#include <QHash>
#include <QUuid>
#include <QVector>
#include <optional>
#include <memory>
#include <mutex>

#include "../model/VesselMessage.h"
#include "../model/AlertZone.h"

class ShipStateStore
{
public:
    ShipStateStore();
    ~ShipStateStore() = default;

    // Cập nhật vị trí mới nhất (Copy-On-Write, hoàn toàn không block luồng đọc)
    void updatePosition(const ShipMessage &msg);

    // Cập nhật hàng loạt vị trí mới nhất (Copy-On-Write)
    void updatePositions(const QVector<ShipMessage> &positions);

    // Lấy vị trí mới nhất của một tàu cụ thể (Lock-free read, không bao giờ bị block)
    std::optional<ShipMessage> getPosition(const QUuid &shipId) const;

    // Lấy vị trí mới nhất của tất cả các tàu đang hoạt động (Lock-free read)
    QVector<ShipMessage> getAllPositions() const;

    // Lấy danh sách ID của tất cả các tàu đang hoạt động (Lock-free read)
    QVector<QUuid> getActiveShipIds() const;

    // Xóa một tàu khỏi danh sách hoạt động (Copy-On-Write)
    void removeShip(const QUuid &shipId);

    // Cập nhật danh sách alert zones trong RAM
    void setAlertZones(const QVector<AlertZone> &zones);
    // Lấy danh sách alert zones trong RAM (Lock-free read)
    QVector<AlertZone> getAlertZones() const;

    // Cập nhật trạng thái tàu trong zone (Copy-On-Write)
    void setShipZoneState(const QUuid &shipId, const QUuid &zoneId, bool isInside);
    // Lấy trạng thái tàu trong zone (Lock-free read)
    bool getShipZoneState(const QUuid &shipId, const QUuid &zoneId) const;
    // Cập nhật tất cả trạng thái tàu trong zone cùng lúc
    void setShipZoneStates(const QHash<QString, bool> &states);

    // Xóa sạch bộ nhớ cache
    void clear();

private:
    // Con trỏ thông minh lưu trữ bảng dữ liệu trên RAM
    std::shared_ptr<QHash<QUuid, ShipMessage>> m_store;
    // Cache RAM của alert zones
    std::shared_ptr<QVector<AlertZone>> m_alertZones;
    // Cache RAM của trạng thái tàu trong zone (Key: shipId_zoneId, Value: isInside)
    std::shared_ptr<QHash<QString, bool>> m_shipZoneStates;

    // Mutex chỉ dùng để đồng bộ giữa các luồng ghi (luồng đọc không dùng mutex này)
    std::mutex m_writeMutex;
};
