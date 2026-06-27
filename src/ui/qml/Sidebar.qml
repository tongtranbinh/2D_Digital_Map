import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0f172a"
    border.color: "#1e293b"
    border.width: 1
    clip: true

    property string selectedShipId: ""
    property bool collapsed: false
    readonly property int collapsedWidth: 48
    readonly property int expandedWidth: 320
    property string currentTab: "ships" // "ships" or "zones"

    signal shipClicked(string shipId, string name, var mmsi, double lat, double lon, double speed, double heading, double course, string timeStr, bool insideZone)
    signal zoneClicked(string zoneId, var pathPoints)

    Behavior on width {
        NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
    }

    ToolButton {
        id: toggleButton
        width: 32
        height: 32
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 12
        anchors.rightMargin: 8
        z: 10
        text: root.collapsed ? ">" : "<"
        font.bold: true
        font.pixelSize: 16
        onClicked: root.collapsed = !root.collapsed

        background: Rectangle {
            color: toggleButton.hovered ? "#334155" : "#1e293b"
            border.color: "#334155"
            border.width: 1
            radius: 6
        }
        palette.buttonText: "#f8fafc"
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        anchors.rightMargin: 48
        spacing: 12
        visible: !root.collapsed
        opacity: root.collapsed ? 0 : 1

        Text {
            text: "SHIP TRACKING"
            color: "#f8fafc"
            font.bold: true
            font.pixelSize: 18
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1e293b"
        }

        // Segmented Control Tabs
        RowLayout {
            Layout.fillWidth: true
            height: 36
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                height: 32
                color: root.currentTab === "ships" ? "#1e293b" : "transparent"
                radius: 6

                Text {
                    anchors.centerIn: parent
                    text: "Tàu Biển"
                    color: root.currentTab === "ships" ? "#f8fafc" : "#64748b"
                    font.bold: true
                    font.pixelSize: 12
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.currentTab = "ships"
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 32
                color: root.currentTab === "zones" ? "#1e293b" : "transparent"
                radius: 6

                Text {
                    anchors.centerIn: parent
                    text: "Vùng Cảnh Báo"
                    color: root.currentTab === "zones" ? "#f8fafc" : "#64748b"
                    font.bold: true
                    font.pixelSize: 12
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.currentTab = "zones"
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#1e293b"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            visible: root.currentTab === "ships"
            Layout.preferredHeight: visible ? 58 : 0

            Rectangle {
                Layout.fillWidth: true
                height: 58
                radius: 6
                color: "#111827"
                border.color: "#1e293b"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 2

                    Text {
                        text: "TRACKING"
                        color: "#64748b"
                        font.pixelSize: 9
                        font.bold: true
                    }

                    Text {
                        text: mapController.shipModel.trackingCount
                        color: "#f8fafc"
                        font.pixelSize: 22
                        font.bold: true
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 58
                radius: 6
                color: "#1f1115"
                border.color: "#7f1d1d"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 2

                    Text {
                        text: "ALERT"
                        color: "#fca5a5"
                        font.pixelSize: 9
                        font.bold: true
                    }

                    Text {
                        text: mapController.shipModel.alertCount
                        color: "#ef4444"
                        font.pixelSize: 22
                        font.bold: true
                    }
                }
            }
        }

        ListView {
            id: shipListView
            Layout.fillWidth: true
            Layout.fillHeight: root.currentTab === "ships"
            visible: root.currentTab === "ships"
            spacing: 4
            clip: true
            model: mapController.shipModel

            delegate: Rectangle {
                id: shipDelegate
                width: shipListView.width
                height: 56
                radius: 6
                color: root.selectedShipId === shipId ? "#1e293b" : (hoverArea.containsMouse ? "#1e293b80" : "transparent")

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: isInsideZone ? "#ef4444" : "#10b981"

                        SequentialAnimation on opacity {
                            running: isInsideZone
                            loops: Animation.Infinite
                            NumberAnimation { to: 0.25; duration: 500 }
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
                            text: "MMSI: " + mmsi + " | Speed: " + speed.toFixed(1) + " km/h"
                            color: "#64748b"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        visible: isInsideZone
                        width: 48
                        height: 18
                        radius: 3
                        color: "#7f1d1d"
                        border.color: "#b91c1c"
                        border.width: 1

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

        ListView {
            id: zoneListView
            Layout.fillWidth: true
            Layout.fillHeight: root.currentTab === "zones"
            visible: root.currentTab === "zones"
            spacing: 4
            clip: true
            model: mapController.zoneModel

            delegate: Rectangle {
                id: zoneDelegate
                width: zoneListView.width
                height: 56
                radius: 6
                color: hoverArea.containsMouse ? "#1e293b80" : "transparent"

                MouseArea {
                    id: hoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.zoneClicked(zoneId, pathPoints);
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 12

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: enabled ? "#ef4444" : "#94a3b8"
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: zoneName
                            color: "#f8fafc"
                            font.bold: true
                            font.pixelSize: 13
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }

                        Text {
                            text: description ? description : "Không có mô tả"
                            color: "#64748b"
                            font.pixelSize: 10
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    ToolButton {
                        id: deleteZoneBtn
                        text: "✕"
                        font.pixelSize: 12
                        font.bold: true
                        palette.buttonText: deleteZoneBtn.hovered ? "#ef4444" : "#64748b"

                        background: Rectangle {
                            color: deleteZoneBtn.hovered ? "#7f1d1d33" : "transparent"
                            radius: 4
                        }

                        onClicked: {
                            mapController.deleteAlertZone(zoneId);
                        }
                    }
                }
            }
        }

        Text {
            text: "Chưa có vùng cảnh báo nào.\nHãy thêm mới bằng cách vẽ trên bản đồ."
            color: "#64748b"
            font.pixelSize: 12
            horizontalAlignment: Text.AlignHCenter
            lineHeight: 1.3
            visible: root.currentTab === "zones" && zoneListView.count === 0
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.topMargin: 40
        }
    }
}
