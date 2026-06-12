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
    }

    // Kết nối tín hiệu C++ phát ra để đồng bộ với UI
    Connections {
        target: mapController
        function onVesselAlertTriggered(shipId, shipName, zoneId, zoneName, eventType, timeStr) {
            // Chỉ cập nhật trạng thái cảnh báo trên panel chi tiết của tàu nếu người dùng đang chọn xem tàu đó
            if (detailPanel.visible && detailPanel.shipId === shipId) {
                detailPanel.isInsideZone = (eventType === "ENTER");
            }
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
            SplitView.minimumWidth: 280
            SplitView.preferredWidth: 320
            SplitView.maximumWidth: 400
            selectedShipId: mapView.selectedShipId

            onShipClicked: (shipId, name, mmsi, lat, lon, speed, heading, course, timeStr, insideZone) => {
                showVesselDetails(shipId, name, mmsi, lat, lon, speed, heading, course, timeStr, insideZone);
                // Tự động định vị camera bản đồ tập trung vào tàu
                mapView.mapObj.center = QtPositioning.coordinate(lat, lon);
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
            }

            // Panel thông tin chi tiết trượt lên dưới bản đồ khi được chọn
            ShipDetailPanel {
                id: detailPanel
                x: 20
                y: parent.height + 20 // Trạng thái ẩn lúc đầu dưới đáy màn hình
                visible: false

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
