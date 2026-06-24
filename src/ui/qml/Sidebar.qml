import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 320
    color: "#0f172a" // Slate 900
    border.color: "#1e293b" // Slate 800
    border.width: 1

    property string selectedShipId: ""
    property string searchText: ""

    signal shipClicked(string shipId, string name, var mmsi, double lat, double lon, double speed, double heading, double course, string timeStr, bool insideZone)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Tiêu đề App
        RowLayout {
            spacing: 8
            Text {
                text: "SHIP TRACKING"
                color: "#f8fafc"
                font.bold: true
                font.pixelSize: 18
                font.letterSpacing: 1.5
            }
        }

        // Vạch phân cách
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1e293b"
        }


        // Tab chuyển đổi (Danh sách tàu / Danh sách vùng)
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            height: 32
            
            background: Rectangle {
                color: "#1e293b"
                radius: 4
            }

            TabButton {
                text: "Tàu hoạt động"
                font.pixelSize: 11
                font.bold: true
                height: 32
            }
            TabButton {
                text: "Vùng cảnh báo"
                font.pixelSize: 11
                font.bold: true
                height: 32
            }
        }

        // Stack chứa nội dung 2 tab
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Tab 1: Danh sách tàu
            Item {
                ListView {
                    id: shipListView
                    anchors.fill: parent
                    spacing: 4
                    clip: true
                    model: mapController.shipModel

                    delegate: Rectangle {
                        id: shipDelegate
                        width: shipListView.width
                        height: isMatch ? 56 : 0
                        visible: isMatch
                        radius: 6
                        color: root.selectedShipId === shipId ? "#1e293b" : (hoverArea.containsMouse ? "#1e293b80" : "transparent")

                        // Kiểm tra lọc tìm kiếm
                        property bool isMatch: {
                            if (root.searchText.trim() === "") return true;
                            var term = root.searchText.toLowerCase();
                            var nameMatch = vesselName.toLowerCase().indexOf(term) !== -1;
                            var mmsiMatch = String(mmsi).indexOf(term) !== -1;
                            return nameMatch || mmsiMatch;
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 12

                            // Cột hiển thị màu trạng thái tàu
                            Rectangle {
                                width: 8
                                height: 8
                                radius: 4
                                color: isInsideZone ? "#ef4444" : "#10b981" // Red for alarm, Emerald for normal

                                // Hiệu ứng nhấp nháy cho tàu bị cảnh báo
                                SequentialAnimation on opacity {
                                    running: isInsideZone
                                    loops: Animation.Infinite
                                    NumberAnimation { to: 0.2; duration: 500 }
                                    NumberAnimation { to: 1.0; duration: 500 }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: vesselName
                                    color: "#f8fafc"
                                    font.bold: true
                                    font.pixelSize: 13
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: "MMSI: " + mmsi + " | Vận tốc: " + speed.toFixed(1) + " km/h"
                                    color: "#64748b"
                                    font.pixelSize: 10
                                }
                            }

                            // Huy hiệu cảnh báo
                            Rectangle {
                                visible: isInsideZone
                                width: 50
                                height: 18
                                radius: 3
                                color: "#7f1d1d"
                                border.color: "#b91c1c"
                                
                                Text {
                                    anchors.centerIn: parent
                                    text: "ALERT"
                                    color: "#fecaca"
                                    font.pixelSize: 8
                                    font.bold: true
                                }
                            }
                        }

                        MouseArea {
                            id: hoverArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var timeStr = timestamp.toLocaleTimeString(Qt.locale(), "hh:mm:ss dd/MM");
                                root.shipClicked(shipId, vesselName, mmsi, latitude, longitude, speed, heading, course, timeStr, isInsideZone);
                            }
                        }
                    }
                }
            }

            // Tab 2: Danh sách vùng geofence
            Item {
                ListView {
                    id: zoneListView
                    anchors.fill: parent
                    spacing: 6
                    clip: true
                    model: mapController.zoneModel

                    delegate: Rectangle {
                        width: zoneListView.width
                        height: 60
                        radius: 6
                        color: "#1e293b40"
                        border.color: enabled ? "#ef444440" : "#334155"
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 12

                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                color: enabled ? "#ef4444" : "#64748b"
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: zoneName
                                    color: "#f8fafc"
                                    font.bold: true
                                    font.pixelSize: 12
                                }

                                Text {
                                    text: description !== "" ? description : "Vùng giám sát Geofence"
                                    color: "#64748b"
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
