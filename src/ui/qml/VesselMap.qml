import QtQuick
import QtPositioning
import QtLocation

MapView {
    id: root
    anchors.fill: parent

    // Tín hiệu khi chọn tàu
    signal shipSelected(string shipId, string name, var mmsi, double lat, double lon, double speed, double heading, double course, string timeStr, bool insideZone)

    // ID tàu đang được chọn
    property string selectedShipId: ""

    // Lịch sử đường đi của tàu được chọn
    property var selectedShipTrack: []

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
            value: "https://tile.openstreetmap.org/"
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
            border.color: enabled ? "#ff0000" : "#64748b" // Pure red border for enabled geofences, grey for disabled
            border.width: 3.5
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

                Canvas {
                    id: shipCanvas
                    anchors.fill: parent
                    rotation: heading

                    property bool inside: isInsideZone
                    property bool isSelected: root.selectedShipId === shipId

                    onInsideChanged: requestPaint()
                    onIsSelectedChanged: requestPaint()

                    onPaint: {
                        var ctx = getContext("2d");
                        ctx.reset();

                        if (inside) {
                            ctx.fillStyle = "#ef4444"; // Đỏ nếu vi phạm geofence
                        } else if (isSelected) {
                            ctx.fillStyle = "#10b981"; // Xanh lục nếu chọn
                        } else {
                            ctx.fillStyle = "#06b6d4"; // Xanh lam mặc định
                        }

                        ctx.beginPath();
                        ctx.moveTo(12, 2);
                        ctx.lineTo(20, 22);
                        ctx.lineTo(12, 17);
                        ctx.lineTo(4, 22);
                        ctx.closePath();
                        ctx.fill();

                        ctx.strokeStyle = isSelected ? "#ffffff" : "#0f172a";
                        ctx.lineWidth = isSelected ? 2.5 : 1;
                        ctx.stroke();
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
