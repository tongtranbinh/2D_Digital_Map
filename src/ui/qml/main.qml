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
            // B?ng di?u khi?n v? v�ng c?nh b�o (Drawing Control Panel)
            Rectangle {
                id: drawingPanel
                width: 280
                height: mapView.isDrawingMode ? 280 : 70
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: 20
                anchors.topMargin: 20
                z: 1000
                radius: 8
                color: "#1e293b" // Slate 800
                border.color: "#334155" // Slate 700
                border.width: 1
                opacity: 0.95

                // Hiệu ứng đổ bóng mờ
                layer.enabled: true

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    Text {
                        text: mapView.isDrawingMode ? "Vẽ vùng cảnh báo mới" : "Quản lý Geofence"
                        color: "#f8fafc"
                        font.bold: true
                        font.pixelSize: 14
                        Layout.fillWidth: true
                    }

                    // Nút để bắt đầu vẽ vùng mới
                    Button {
                        visible: !mapView.isDrawingMode
                        text: "Thêm vùng cảnh báo"
                        Layout.fillWidth: true
                        onClicked: {
                            mapView.isDrawingMode = true;
                            mapView.drawingPath = [];
                            zoneNameInput.text = "";
                            zoneDescInput.text = "";
                        }
                        background: Rectangle {
                            color: "#3b82f6" // Xanh dương
                            radius: 4
                        }
                        palette.buttonText: "#ffffff"
                    }

                    // Form nhập liệu và điều khiển khi vẽ vùng
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
        }
    }
}
