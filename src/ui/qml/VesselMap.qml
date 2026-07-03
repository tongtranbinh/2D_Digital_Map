import QtQuick
import QtPositioning
import QtLocation
import ShipTracking 1.0

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

    // Toạ độ của chuột đang hover trên bản đồ
    property var hoverCoordinate: null

    // Chế độ đo khoảng cách và phương vị
    property bool isMeasureMode: false
    property var measureStart: null
    property var measureEnd: null

    // Đường dẫn hiển thị khi đo đạc
    property var measurePath: {
        if (measureStart === null) return [];
        if (measureEnd !== null) return [measureStart, measureEnd];
        if (hoverCoordinate !== null) return [measureStart, hoverCoordinate];
        return [measureStart];
    }

    // Tín hiệu khi click vào bản đồ
    signal mapClicked(var coordinate)

    // Nhận diện click chuột trên bản đồ để thêm điểm đa giác hoặc đo đạc
    TapHandler {
        id: mapTapHandler
        acceptedButtons: Qt.LeftButton
        onTapped: (eventPoint, button) => {
            var coord = root.map.toCoordinate(eventPoint.position);
            if (root.isMeasureMode) {
                if (root.measureStart === null) {
                    root.measureStart = coord;
                } else if (root.measureEnd === null) {
                    root.measureEnd = coord;
                } else {
                    root.measureStart = coord;
                    root.measureEnd = null;
                }
            } else {
                root.mapClicked(coord);
            }
        }
    }

    // MouseArea để theo dõi toạ độ hover của chuột
    MouseArea {
        id: mapHoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        propagateComposedEvents: true
        onPositionChanged: (mouse) => {
            root.hoverCoordinate = root.map.toCoordinate(Qt.point(mouse.x, mouse.y));
        }
        onExited: {
            root.hoverCoordinate = null;
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

    // 2d. Vẽ Đường đo khoảng cách & phương vị
    MapPolyline {
        parent: root.map
        line.color: "#f43f5e" // Rose 500
        line.width: 3.5
        path: root.measurePath
        visible: root.isMeasureMode && path.length > 1
    }

    // 2e. Điểm bắt đầu đo A
    MapQuickItem {
        parent: root.map
        coordinate: root.measureStart !== null ? root.measureStart : QtPositioning.coordinate(0,0)
        visible: root.isMeasureMode && root.measureStart !== null
        anchorPoint: Qt.point(9, 9)
        sourceItem: Rectangle {
            width: 18
            height: 18
            radius: 9
            color: "#f43f5e"
            border.color: "#ffffff"
            border.width: 2
            
            Text {
                anchors.centerIn: parent
                text: "A"
                color: "#ffffff"
                font.pixelSize: 10
                font.bold: true
            }
        }
    }

    // 2f. Điểm kết thúc đo B
    MapQuickItem {
        parent: root.map
        coordinate: root.measureEnd !== null ? root.measureEnd : QtPositioning.coordinate(0,0)
        visible: root.isMeasureMode && root.measureEnd !== null
        anchorPoint: Qt.point(9, 9)
        sourceItem: Rectangle {
            width: 18
            height: 18
            radius: 9
            color: "#f43f5e"
            border.color: "#ffffff"
            border.width: 2

            Text {
                anchors.centerIn: parent
                text: "B"
                color: "#ffffff"
                font.pixelSize: 10
                font.bold: true
            }
        }
    }

    // 3. Vẽ toàn bộ tàu bằng một Scene Graph layer thay vì MapQuickItem từng tàu
    ShipRenderLayer {
        id: shipLayer
        parent: root.map
        anchors.fill: parent
        z: 100
        enabled: !root.isMeasureMode
        mapObject: root.map
        shipModel: mapController.shipModel
        zoomLevel: root.map.zoomLevel
        showLabels: root.map.zoomLevel >= 10
        selectedShipId: root.selectedShipId

        onShipClicked: function(shipId) {
            root.selectedShipId = shipId;
            var index = mapController.shipModel.findShipIndex(shipId);
            if (index < 0)
                return;

            var ship = mapController.shipModel.getShipAt(index);
            if (!ship || Object.keys(ship).length === 0)
                return;

            var timeStr = ship.timestamp ? ship.timestamp.toLocaleTimeString(Qt.locale(), "hh:mm:ss dd/MM") : "";
            root.shipSelected(ship.shipId, ship.vesselName, ship.mmsi,
                              ship.latitude, ship.longitude, ship.speed,
                              ship.heading, ship.course, timeStr,
                              ship.isInsideZone);
        }
    }

    Connections {
        target: root.map
        function onCenterChanged() { shipLayer.refresh(); }
        function onZoomLevelChanged() { shipLayer.refresh(); }
        function onWidthChanged() { shipLayer.refresh(); }
        function onHeightChanged() { shipLayer.refresh(); }
    }
}
