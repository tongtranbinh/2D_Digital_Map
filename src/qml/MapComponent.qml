import QtQuick 2.15
import QtLocation 5.15
import QtPositioning 5.15

Map {
    id: mapComponent
    
    // Cấu hình plugin OpenStreetMap sử dụng HTTPS và User-Agent hợp lệ
    plugin: Plugin {
        name: "osm"
        PluginParameter { name: "osm.useragent"; value: "maritime_tracking_demo/1.0" }
        PluginParameter { name: "osm.mapping.custom.host"; value: "https://tile.openstreetmap.org/" }
    }
    
    // Tự động kích hoạt loại bản đồ Custom để trỏ về HTTPS host trên
    activeMapType: {
        for (var i = 0; i < supportedMapTypes.length; i++) {
            if (supportedMapTypes[i].name.indexOf("Custom") >= 0) {
                return supportedMapTypes[i];
            }
        }
        return supportedMapTypes[0];
    }
    
    // Tọa độ trung tâm khởi tạo (Khu vực biển Hải Phòng - Vịnh Hạ Long)
    center: QtPositioning.coordinate(20.78, 106.90)
    zoomLevel: 10
    
    // Bật toàn bộ các cử chỉ thu phóng, kéo trượt bản đồ
    gesture.enabled: true
    
    // Tín hiệu phát đi khi người dùng click chọn một tàu
    signal shipSelected(int mmsi)
    
    // Hàm phụ trợ để Marker gọi khi được click
    function selectShip(mmsi) {
        shipSelected(mmsi);
    }
    
    // Biến lưu trữ danh sách đa giác vùng cảnh báo lấy từ C++
    property var warningZones: []
    
    Component.onCompleted: {
        // Tải các vùng cảnh báo từ database ngay khi component được khởi tạo
        warningZones = appEngine.getWarningZones();
    }
    
    // 1. Vẽ các Vùng Cảnh Báo (Polygons) từ CSDL lên bản đồ
    MapItemView {
        model: mapComponent.warningZones
        delegate: WarningZoneItem {}
    }
    
    // 2. Vẽ danh sách các Tàu đang di chuyển thời gian thực (Markers) từ C++ ShipModel
    MapItemView {
        model: shipModel // Binding trực tiếp vào C++ QAbstractListModel
        delegate: ShipMarker {}
    }
    
    // MouseArea bắt sự kiện click ra khoảng trống bản đồ để hủy chọn tàu
    MouseArea {
        anchors.fill: parent
        z: -1 // Đặt ở dưới cùng để không che mất các Marker
        onClicked: {
            mapComponent.shipSelected(-1); // -1 là mã định danh không chọn tàu nào
        }
    }
}
