import QtQuick
import QtPositioning
import QtLocation
import QtQuick.Shapes

MapView {
    id: root
    anchors.fill: parent

    // Tín hiệu khi chọn tàu
    signal shipSelected(string shipId, string name, var mmsi, double lat, double lon, double speed, double heading, double course, string timeStr, bool insideZone)

    // ID tàu đang được chọn
    property string selectedShipId: ""

    Binding {
        target: mapController
        property: "selectedShipId"
        value: root.selectedShipId
    }

    // Lịch sử đường đi của tàu được chọn
    property var selectedShipTrack: []

    // Trạng thái vẽ vùng cảnh báo mới
    property bool isDrawingMode: false
    property var drawingPath: []

    // Tín hiệu khi click vào bản đồ
    signal mapClicked(var coordinate)

    // Nhận diện click chuột trên bản đồ để thêm điểm đa giác
    TapHandler {
        id: mapTapHandler
        acceptedButtons: Qt.LeftButton
        onTapped: (eventPoint, button) => {
            var coord = root.map.toCoordinate(eventPoint.position);
            root.mapClicked(coord);
        }
    }

    // MapView có thuộc tính 'map' tích hợp sẵn. Cung cấp alias mapObj để tương thích với main.qml
    property alias mapObj: root.map

    // Cập nhật đường đi khi nhận được tín hiệu từ C++
    Connections {
        target: mapController
        function onTrackHistoryUpdated(shipId) {
            if (shipId === root.selectedShipId) {
                root.selectedShipTrack = mapController.getTrackHistory(shipId);
            }
        }
    }

    // Khi ID tàu chọn thay đổi, yêu cầu CSDL tải lịch sử hành trình cũ bất đồng bộ
    onSelectedShipIdChanged: {
        if (root.selectedShipId !== "") {
            mapController.loadTrackHistoryFromDb(root.selectedShipId);
        } else {
            root.selectedShipTrack = [];
        }
    }

    // Cấu hình bản đồ thông qua thuộc tính 'map' của MapView
    map.plugin: Plugin {
        name: "osm"
        PluginParameter {
            name: "osm.useragent"
            value: "ShipTrackingApp/1.0"
        }
        PluginParameter {
            name: "osm.mapping.custom.host"
            value: "http://localhost:8080/tile/"
        }
        PluginParameter {
            name: "osm.mapping.providerserial"
            value: "custom"
        }
    }

    // Vị trí mặc định ở Biển Đông / Việt Nam
    map.center: QtPositioning.coordinate(16.0, 109.5)
    map.zoomLevel: 6

    // Chọn chế độ bản đồ CustomMap để sử dụng host OSM chính thức (không có chữ API key required)
    Component.onCompleted: {
        for (var i = 0; i < map.supportedMapTypes.length; ++i) {
            if (map.supportedMapTypes[i].style === MapType.CustomMap) {
                map.activeMapType = map.supportedMapTypes[i];
                break;
            }
        }
    }

    // Trong Qt6 MapView, các đối tượng vẽ như MapPolygon, MapPolyline, MapQuickItem
    // bắt buộc phải được gắn 'parent: root.map' để hiển thị đè lên các mảnh bản đồ.

    // 1. Vẽ các Vùng cảnh báo (Geofences)
    MapItemView {
        parent: root.map
        model: mapController.zoneModel
        delegate: MapPolygon {
            path: pathPoints
            color: enabled ? "#40ff0000" : "#1594a3b8" // 25% opacity pure red for enabled geofences, light grey for disabled
            border.width: 0
        }
    }

    // 2. Vẽ Đường hành trình của tàu đang được chọn
    MapPolyline {
        parent: root.map
        line.color: "#10b981" // Xanh lục neon
        line.width: 3.5
        path: root.selectedShipTrack
        visible: path.length > 0
    }

    // 2b. Vẽ đường đa giác dở dang của vùng mới đang tạo
    MapPolyline {
        parent: root.map
        line.color: "#3b82f6" // Xanh dương
        line.width: 3
        path: root.drawingPath
        visible: root.isDrawingMode && path.length > 0
    }

    // 2c. Vẽ các đỉnh đa giác dở dang của vùng mới dưới dạng chấm tròn
    MapItemView {
        parent: root.map
        model: root.drawingPath
        visible: root.isDrawingMode
        delegate: MapQuickItem {
            coordinate: modelData
            anchorPoint: Qt.point(6, 6)
            sourceItem: Rectangle {
                width: 12
                height: 12
                radius: 6
                color: "#3b82f6"
                border.color: "#ffffff"
                border.width: 1.5
            }
        }
    }

    // 3. Vẽ các Tàu biển trên Bản đồ
    MapItemView {
        parent: root.map
        model: mapController.shipModel
        delegate: MapQuickItem {
            coordinate: QtPositioning.coordinate(latitude, longitude)
            anchorPoint: Qt.point(12, 12)
            z: root.selectedShipId === shipId ? 100 : 1

            sourceItem: Item {
                width: 24
                height: 24

                Shape {
                    id: shipShape
                    anchors.fill: parent
                    rotation: heading
                    antialiasing: true

                    // Tối ưu hóa hiệu năng bằng cách cache texture trên GPU
                    layer.enabled: true
                    layer.smooth: true

                    ShapePath {
                        strokeWidth: root.selectedShipId === shipId ? 2.5 : 1
                        strokeColor: root.selectedShipId === shipId ? "#ffffff" : "#0f172a"
                        fillColor: isInsideZone ? "#ef4444" : (root.selectedShipId === shipId ? "#10b981" : "#06b6d4")

                        startX: 12; startY: 2
                        PathLine { x: 20; y: 22 }
                        PathLine { x: 12; y: 17 }
                        PathLine { x: 4; y: 22 }
                        PathLine { x: 12; y: 2 }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var timeStr = timestamp.toLocaleTimeString(Qt.locale(), "hh:mm:ss dd/MM");
                        root.shipSelected(shipId, vesselName, mmsi, latitude, longitude, speed, heading, course, timeStr, isInsideZone);
                    }
                }
            }
        }
    }
}
