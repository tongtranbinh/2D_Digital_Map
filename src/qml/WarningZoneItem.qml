import QtQuick 2.15
import QtLocation 5.15
import QtPositioning 5.15

MapPolygon {
    id: zonePolygon
    
    // Gán danh sách các điểm tọa độ (QGeoCoordinate) của đa giác
    path: modelData.path
    
    // Màu sắc vùng đa giác (Đỏ nhạt cho Vùng Critical, Cam nhạt cho Vùng Warning)
    color: modelData.severity === "critical" ? "rgba(255, 50, 50, 0.25)" : "rgba(255, 140, 0, 0.20)"
    
    // Màu viền và độ rộng viền đa giác
    border.color: modelData.severity === "critical" ? "#ff2222" : "#ff8c00"
    border.width: 2
}
