import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtPositioning

ApplicationWindow {
    id: window
    visible: true
    width: 1200
    height: 800
    title: "Hệ thống Bản đồ số Theo dõi Tàu biển 2D - Real-time Ship Tracking"

    function getCardinalDirection(bearing) {
        var directions = ["Bắc (N)", "Đông Bắc (NE)", "Đông (E)", "Đông Nam (SE)", "Nam (S)", "Tây Nam (SW)", "Tây (W)", "Tây Bắc (NW)"];
        var index = Math.round(bearing / 45) % 8;
        return directions[index];
    }

    function formatDistance(m) {
        if (m < 1000) {
            return m.toFixed(1) + " m";
        }
        return (m / 1000).toFixed(2) + " km";
    }

    function formatBearing(deg) {
        var d = (deg % 360 + 360) % 360;
        return d.toFixed(1) + "° (" + getCardinalDirection(d) + ")";
    }

    function applyShipDetails(ship) {
        if (!ship || Object.keys(ship).length === 0)
            return false;

        detailPanel.shipId = ship.shipId || detailPanel.shipId;
        detailPanel.vesselName = ship.vesselName || "";
        detailPanel.mmsi = ship.mmsi || 0;
        detailPanel.latitude = ship.latitude || 0.0;
        detailPanel.longitude = ship.longitude || 0.0;
        detailPanel.speed = ship.speed || 0.0;
        detailPanel.heading = ship.heading || 0.0;
        detailPanel.course = ship.course || 0.0;
        detailPanel.timestampStr = ship.timestamp ? ship.timestamp.toLocaleTimeString(Qt.locale(), "hh:mm:ss dd/MM") : "";
        detailPanel.isInsideZone = !!ship.isInsideZone;
        return true;
    }

    function refreshSelectedShipDetails() {
        if (!detailPanel.visible || detailPanel.shipId === "")
            return;

        var index = mapController.shipModel.findShipIndex(detailPanel.shipId);
        if (index < 0)
            return;

        var ship = mapController.shipModel.getShipAt(index);
        if (!ship || Object.keys(ship).length === 0)
            return;

        applyShipDetails(ship);
    }

    // Hàm chọn tàu và hiển thị thông tin
    function showVesselDetails(shipId, name, mmsi, lat, lon, speed, heading, course, timeStr, insideZone) {
        applyShipDetails({
            shipId: shipId,
            vesselName: name,
            mmsi: mmsi,
            latitude: lat,
            longitude: lon,
            speed: speed,
            heading: heading,
            course: course,
            isInsideZone: insideZone
        });
        detailPanel.timestampStr = timeStr;

        detailPanel.visible = true;
        detailPanel.y = mapContainer.height - detailPanel.height - 20; // Trượt lên trên
        mapView.selectedShipId = shipId;

        // Tự động định vị camera bản đồ tập trung vào tàu
        mapView.mapObj.center = QtPositioning.coordinate(lat, lon);
    }

    // Kết nối tín hiệu C++ phát ra để đồng bộ với UI
    Connections {
        target: mapController
        function onVesselAlertTriggered(shipId, shipName, zoneId, zoneName, eventType, timeStr) {
            // Chỉ cập nhật trạng thái cảnh báo trên panel chi tiết của tàu nếu người dùng đang chọn xem tàu đó
            if (detailPanel.visible && detailPanel.shipId === shipId) {
                detailPanel.isInsideZone = (eventType === "ENTER");
            }
            // Hiển thị thông báo cảnh báo trên màn hình
            alertBanner.addAlert(shipId, shipName, zoneId, zoneName, eventType, timeStr);
        }
    }

    Connections {
        target: mapController.shipModel

        function onDataChanged(topLeft, bottomRight, roles) {
            refreshSelectedShipDetails();
        }

        function onRowsInserted(parent, first, last) {
            refreshSelectedShipDetails();
        }

        function onModelReset() {
            refreshSelectedShipDetails();
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        // 1. Sidebar bên trái (Danh sách tàu & Vùng Geofence)
        Sidebar {
            id: sidebar
            SplitView.minimumWidth: collapsed ? collapsedWidth : 280
            SplitView.preferredWidth: collapsed ? collapsedWidth : expandedWidth
            SplitView.maximumWidth: collapsed ? collapsedWidth : 400
            selectedShipId: mapView.selectedShipId

            onShipClicked: (shipId, name, mmsi, lat, lon, speed, heading, course, timeStr, insideZone) => {
                showVesselDetails(shipId, name, mmsi, lat, lon, speed, heading, course, timeStr, insideZone);
            }

            onZoneClicked: (zoneId, pathPoints) => {
                if (pathPoints && pathPoints.length > 0) {
                    mapView.mapObj.center = pathPoints[0];
                    mapView.mapObj.zoomLevel = 10;
                }
            }
        }

        // 2. Khu vực Bản đồ bên phải
        Item {
            id: mapContainer
            SplitView.fillWidth: true
            SplitView.fillHeight: true

            VesselMap {
                id: mapView
                anchors.fill: parent
                selectedShipId: detailPanel.visible ? detailPanel.shipId : ""

                onShipSelected: (shipId, name, mmsi, lat, lon, speed, heading, course, timeStr, insideZone) => {
                    showVesselDetails(shipId, name, mmsi, lat, lon, speed, heading, course, timeStr, insideZone);
                }

                onMapClicked: (coordinate) => {
                    if (isDrawingMode) {
                        var path = drawingPath;
                        path.push(coordinate);
                        drawingPath = path; // Gán lại để QML kích hoạt cập nhật UI
                    }
                }
            }

            // Banner hiển thị danh sách cảnh báo trượt/ẩn tự động
            AlertBanner {
                id: alertBanner
            }
            // Khung trượt thông tin / nhập liệu cho công cụ đang hoạt động (Vẽ Geofence hoặc Đo đạc)
            Rectangle {
                id: drawingPanel
                width: 280
                height: mapView.isDrawingMode ? 250 : 
                        (mapView.isMeasureMode ? 
                            (mapView.measureStart !== null && (mapView.measureEnd !== null || mapView.hoverCoordinate !== null) ? 230 : 130) 
                            : 0)
                anchors.right: toolsToolbar.left
                anchors.top: toolsToolbar.top
                anchors.rightMargin: 12
                z: 1000
                radius: 8
                color: "#1e293b" // Slate 800
                border.color: mapView.isDrawingMode ? "#3b82f6" : "#f43f5e" // Xanh dương hoặc Hồng đỏ tùy chế độ
                border.width: 1
                opacity: 0.95
                visible: mapView.isDrawingMode || mapView.isMeasureMode

                // Chặn click truyền xuống bản đồ phía dưới
                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: false
                    onPressed: (mouse) => mouse.accepted = true
                }

                // Hiệu ứng đổ bóng mờ
                layer.enabled: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 8

                    Text {
                        text: mapView.isDrawingMode ? "Vẽ vùng cảnh báo mới" : "Đo khoảng cách & phương vị"
                        color: "#f8fafc"
                        font.bold: true
                        font.pixelSize: 14
                        Layout.fillWidth: true
                    }

                    // Form nhập liệu và điều khiển khi vẽ vùng cảnh báo
                    ColumnLayout {
                        visible: mapView.isDrawingMode
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "Click trên bản đồ để chấm các đỉnh. Cần ít nhất 3 điểm."
                            color: "#94a3b8"
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Text {
                            text: "Số điểm đã chấm: " + mapView.drawingPath.length
                            color: mapView.drawingPath.length >= 3 ? "#10b981" : "#ef4444"
                            font.bold: true
                            font.pixelSize: 11
                            Layout.fillWidth: true
                        }

                        TextField {
                            id: zoneNameInput
                            placeholderText: "Nhập tên vùng..."
                            Layout.fillWidth: true
                            color: "#f8fafc"
                            font.family: "Segoe UI"
                            font.pixelSize: 13
                            placeholderTextColor: "#64748b"
                            background: Rectangle {
                                color: "#0f172a"
                                border.color: "#334155"
                                border.width: 1
                                radius: 4
                            }
                        }

                        TextField {
                            id: zoneDescInput
                            placeholderText: "Mô tả vùng..."
                            Layout.fillWidth: true
                            color: "#f8fafc"
                            font.family: "Segoe UI"
                            font.pixelSize: 13
                            placeholderTextColor: "#64748b"
                            background: Rectangle {
                                color: "#0f172a"
                                border.color: "#334155"
                                border.width: 1
                                radius: 4
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Button {
                                text: "Lưu"
                                Layout.fillWidth: true
                                enabled: mapView.drawingPath.length >= 3 && zoneNameInput.text.trim() !== ""
                                onClicked: {
                                    mapController.addAlertZone(zoneNameInput.text.trim(), zoneDescInput.text.trim(), mapView.drawingPath);
                                    mapView.isDrawingMode = false;
                                    mapView.drawingPath = [];
                                }
                                background: Rectangle {
                                    color: parent.enabled ? "#10b981" : "#334155"
                                    radius: 4
                                }
                                palette.buttonText: "#ffffff"
                            }

                            Button {
                                text: "Hủy"
                                Layout.fillWidth: true
                                onClicked: {
                                    mapView.isDrawingMode = false;
                                    mapView.drawingPath = [];
                                }
                                background: Rectangle {
                                    color: "#ef4444"
                                    radius: 4
                                }
                                palette.buttonText: "#ffffff"
                            }
                        }
                    }

                    // Giao diện khi đo đạc khoảng cách và phương vị
                    ColumnLayout {
                        visible: mapView.isMeasureMode
                        Layout.fillWidth: true
                        spacing: 8

                        Text {
                            text: "Click trên bản đồ để chọn điểm A và điểm B."
                            color: "#94a3b8"
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        // Hiển thị trạng thái toạ độ điểm A và B
                        Rectangle {
                            Layout.fillWidth: true
                            height: 60
                            radius: 4
                            color: "#0f172a"
                            border.color: "#334155"

                            GridLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                columns: 2
                                rowSpacing: 4
                                columnSpacing: 10

                                Text { text: "Điểm A:"; color: "#94a3b8"; font.pixelSize: 11; font.bold: true }
                                Text { 
                                    text: mapView.measureStart ? 
                                          mapView.measureStart.latitude.toFixed(5) + " N, " + mapView.measureStart.longitude.toFixed(5) + " E" : 
                                          "Chưa chọn"
                                    color: mapView.measureStart ? "#f43f5e" : "#64748b"
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                Text { text: "Điểm B:"; color: "#94a3b8"; font.pixelSize: 11; font.bold: true }
                                Text { 
                                    text: mapView.measureEnd ? 
                                          mapView.measureEnd.latitude.toFixed(5) + " N, " + mapView.measureEnd.longitude.toFixed(5) + " E" : 
                                          (mapView.measureStart && mapView.hoverCoordinate ? "Đang chọn..." : "Chưa chọn")
                                    color: mapView.measureEnd ? "#f43f5e" : (mapView.measureStart && mapView.hoverCoordinate ? "#38bdf8" : "#64748b")
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }
                        }

                        // Hiển thị kết quả đo đạc thời gian thực
                        Rectangle {
                            Layout.fillWidth: true
                            height: 65
                            radius: 4
                            color: "#1e293b"
                            border.color: "#f43f5e"
                            border.width: 1
                            visible: mapView.measureStart !== null && (mapView.measureEnd !== null || mapView.hoverCoordinate !== null)

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 4

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "Khoảng cách:"; color: "#94a3b8"; font.pixelSize: 11 }
                                    Text { 
                                        text: {
                                            if (mapView.measureStart === null) return "";
                                            var end = mapView.measureEnd !== null ? mapView.measureEnd : mapView.hoverCoordinate;
                                            if (!end) return "";
                                            var dist = mapView.measureStart.distanceTo(end);
                                            return formatDistance(dist);
                                        }
                                        color: "#f8fafc"
                                        font.pixelSize: 12
                                        font.bold: true
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: "Phương vị:"; color: "#94a3b8"; font.pixelSize: 11 }
                                    Text { 
                                        text: {
                                            if (mapView.measureStart === null) return "";
                                            var end = mapView.measureEnd !== null ? mapView.measureEnd : mapView.hoverCoordinate;
                                            if (!end) return "";
                                            var az = mapView.measureStart.azimuthTo(end);
                                            return formatBearing(az);
                                        }
                                        color: "#38bdf8"
                                        font.pixelSize: 11
                                        font.bold: true
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8

                            Button {
                                text: "Xóa kết quả"
                                Layout.fillWidth: true
                                enabled: mapView.measureStart !== null
                                onClicked: {
                                    mapView.measureStart = null;
                                    mapView.measureEnd = null;
                                }
                                background: Rectangle {
                                    color: parent.enabled ? "#475569" : "#334155"
                                    radius: 4
                                }
                                palette.buttonText: "#ffffff"
                            }

                            Button {
                                text: "Đóng"
                                Layout.fillWidth: true
                                onClicked: {
                                    mapView.isMeasureMode = false;
                                    mapView.measureStart = null;
                                    mapView.measureEnd = null;
                                }
                                background: Rectangle {
                                    color: "#ef4444"
                                    radius: 4
                                }
                                palette.buttonText: "#ffffff"
                            }
                        }
                    }
                }
            }

            // Thanh công cụ nút bấm nổi bên phải (Tools Toolbar)
            Rectangle {
                id: toolsToolbar
                width: 44
                height: 96
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 80
                anchors.rightMargin: 20
                z: 1000
                radius: 22 // Hình dạng capsule
                color: "#1e293b" // Slate 800
                border.color: "#334155" // Slate 700
                border.width: 1
                opacity: 0.95
                layer.enabled: true

                // Chặn click truyền xuống bản đồ phía dưới
                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: false
                    onPressed: (mouse) => mouse.accepted = true
                }

                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 8

                    // Nút vẽ Geofence
                    Rectangle {
                        width: 34
                        height: 34
                        radius: 17
                        color: mapView.isDrawingMode ? "#3b82f6" : "transparent"
                        border.color: hoverHandlerDraw.hovered ? "#3b82f6" : "transparent"
                        border.width: 1.5
                        
                        // Vector Geofence Polygon Icon (Rotated Diamond shape)
                        Rectangle {
                            anchors.centerIn: parent
                            width: 14
                            height: 14
                            color: "transparent"
                            border.color: mapView.isDrawingMode ? "#ffffff" : "#f8fafc"
                            border.width: 1.5
                            rotation: 45
                        }

                        HoverHandler {
                            id: hoverHandlerDraw
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (mapView.isDrawingMode) {
                                    mapView.isDrawingMode = false;
                                    mapView.drawingPath = [];
                                } else {
                                    mapView.isDrawingMode = true;
                                    mapView.drawingPath = [];
                                    mapView.isMeasureMode = false;
                                    zoneNameInput.text = "";
                                    zoneDescInput.text = "";
                                }
                            }
                        }
                        
                        ToolTip.visible: hoverHandlerDraw.hovered
                        ToolTip.text: "Vẽ vùng cảnh báo"
                        ToolTip.delay: 300
                    }

                    // Nút đo đạc khoảng cách & phương vị
                    Rectangle {
                        width: 34
                        height: 34
                        radius: 17
                        color: mapView.isMeasureMode ? "#f43f5e" : "transparent"
                        border.color: hoverHandlerMeasure.hovered ? "#f43f5e" : "transparent"
                        border.width: 1.5

                        // Vector Ruler Icon
                        Item {
                            anchors.centerIn: parent
                            width: 20
                            height: 12

                            Rectangle {
                                anchors.fill: parent
                                color: "transparent"
                                border.color: mapView.isMeasureMode ? "#ffffff" : "#f8fafc"
                                border.width: 1.5
                                radius: 1

                                Row {
                                    anchors.bottom: parent.bottom
                                    anchors.bottomMargin: 1
                                    anchors.left: parent.left
                                    anchors.leftMargin: 3
                                    spacing: 3

                                    Rectangle { width: 1; height: 3; color: mapView.isMeasureMode ? "#ffffff" : "#f8fafc" }
                                    Rectangle { width: 1; height: 5; color: mapView.isMeasureMode ? "#ffffff" : "#f8fafc" }
                                    Rectangle { width: 1; height: 3; color: mapView.isMeasureMode ? "#ffffff" : "#f8fafc" }
                                    Rectangle { width: 1; height: 5; color: mapView.isMeasureMode ? "#ffffff" : "#f8fafc" }
                                    Rectangle { width: 1; height: 3; color: mapView.isMeasureMode ? "#ffffff" : "#f8fafc" }
                                }
                            }
                        }

                        HoverHandler {
                            id: hoverHandlerMeasure
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                if (mapView.isMeasureMode) {
                                    mapView.isMeasureMode = false;
                                    mapView.measureStart = null;
                                    mapView.measureEnd = null;
                                } else {
                                    mapView.isMeasureMode = true;
                                    mapView.measureStart = null;
                                    mapView.measureEnd = null;
                                    mapView.isDrawingMode = false;
                                }
                            }
                        }

                        ToolTip.visible: hoverHandlerMeasure.hovered
                        ToolTip.text: "Đo khoảng cách & phương vị"
                        ToolTip.delay: 300
                    }
                }
            }

            // Panel thông tin chi tiết trượt lên dưới bản đồ khi được chọn
            ShipDetailPanel {
                id: detailPanel
                x: 20
                y: parent.height + 20 // Trạng thái ẩn lúc đầu dưới đáy màn hình
                visible: false

                onLatitudeChanged: {
                    if (visible && shipId !== "") {
                        mapView.mapObj.center = QtPositioning.coordinate(latitude, longitude);
                    }
                }

                onLongitudeChanged: {
                    if (visible && shipId !== "") {
                        mapView.mapObj.center = QtPositioning.coordinate(latitude, longitude);
                    }
                }

                onVisibleChanged: {
                    if (visible) {
                        refreshSelectedShipDetails();
                    }
                }

                onCloseRequested: {
                    detailPanel.y = parent.height + 20; // Trượt xuống dưới để ẩn đi
                    hideTimer.start(); // Chờ hiệu ứng trượt xong mới tắt visible
                }

                onCenterRequested: (lat, lon) => {
                    mapView.mapObj.center = QtPositioning.coordinate(lat, lon);
                    mapView.mapObj.zoomLevel = 13; // Phóng to để theo dõi chi tiết
                }
            }

            // Bộ hẹn giờ để tắt visible của panel sau khi trượt xuống ẩn
            Timer {
                id: hideTimer
                interval: 250
                onTriggered: {
                    detailPanel.visible = false;
                    mapView.selectedShipId = "";
                }
            }

            // Hiển thị toạ độ chuột (lat, lon) ở góc trên phải
            Rectangle {
                id: coordPanel
                width: 220
                height: 40
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.rightMargin: 20
                anchors.topMargin: 20
                z: 1000
                radius: 6
                color: "#1e293b" // Slate 800
                border.color: "#334155" // Slate 700
                border.width: 1
                opacity: 0.9
                visible: mapView.hoverCoordinate !== null

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 8

                    Text {
                        text: "📍"
                        font.pixelSize: 14
                    }

                    ColumnLayout {
                        spacing: 1
                        Layout.alignment: Qt.AlignVCenter

                        Text {
                            text: "TOẠ ĐỘ CHUỘT"
                            color: "#94a3b8"
                            font.pixelSize: 8
                            font.bold: true
                        }

                        Text {
                            text: mapView.hoverCoordinate ? 
                                  mapView.hoverCoordinate.latitude.toFixed(6) + " N, " + 
                                  mapView.hoverCoordinate.longitude.toFixed(6) + " E" : 
                                  "0.000000 N, 0.000000 E"
                            color: "#38bdf8" // Sky 400
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }
                }
            }

            // Hiển thị và điều chỉnh Zoom Level bằng thanh trượt ở góc dưới phải
            Rectangle {
                id: zoomPanel
                width: 180
                height: 70
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.rightMargin: 20
                anchors.bottomMargin: 20
                z: 1000
                radius: 6
                color: "#1e293b" // Slate 800
                border.color: "#334155" // Slate 700
                border.width: 1
                opacity: 0.9

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 4

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: "Zoom: " + (mapView.mapObj ? mapView.mapObj.zoomLevel.toFixed(1) : "0.0")
                            color: "#10b981" // Emerald 500
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    Slider {
                        id: zoomSlider
                        Layout.fillWidth: true
                        from: 3
                        to: 18
                        value: mapView.mapObj ? mapView.mapObj.zoomLevel : 6
                        onMoved: {
                            if (mapView.mapObj) {
                                mapView.mapObj.zoomLevel = value;
                            }
                        }

                        // Tùy biến giao diện thanh trượt
                        background: Rectangle {
                            x: zoomSlider.leftPadding
                            y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                            implicitWidth: 150
                            implicitHeight: 4
                            width: zoomSlider.availableWidth
                            height: implicitHeight
                            radius: 2
                            color: "#334155" // Slate 700

                            Rectangle {
                                width: zoomSlider.visualPosition * parent.width
                                height: parent.height
                                color: "#10b981" // Emerald 500
                                radius: 2
                            }
                        }

                        handle: Rectangle {
                            x: zoomSlider.leftPadding + zoomSlider.visualPosition * (zoomSlider.availableWidth - width)
                            y: zoomSlider.topPadding + zoomSlider.availableHeight / 2 - height / 2
                            implicitWidth: 12
                            implicitHeight: 12
                            radius: 6
                            color: "#ffffff"
                            border.color: "#10b981"
                            border.width: 1.5
                        }
                    }
                }
            }
        }
    }
}
