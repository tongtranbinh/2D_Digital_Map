import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 320
    height: 250
    radius: 12
    color: "#0f172a" // Slate 900
    opacity: 0.95
    border.color: isInsideZone ? "#ef4444" : "#334155" // Red border if inside warning zone, Slate otherwise
    border.width: 1.5
    z: 900

    property string shipId: ""
    property string vesselName: ""
    property var mmsi: 0
    property double latitude: 0.0
    property double longitude: 0.0
    property double speed: 0.0
    property double heading: 0.0
    property double course: 0.0
    property string timestampStr: ""
    property bool isInsideZone: false

    signal closeRequested()
    signal centerRequested(double lat, double lon)

    // Hiệu ứng đổ bóng
    layer.enabled: true

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        // Tiêu đề & Nút đóng
        RowLayout {
            Layout.fillWidth: true
            
            ColumnLayout {
                spacing: 2
                Layout.fillWidth: true

                Text {
                    text: vesselName !== "" ? vesselName : "Unknown Vessel"
                    color: "#f8fafc"
                    font.bold: true
                    font.pixelSize: 16
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: "MMSI: " + mmsi
                    color: "#94a3b8"
                    font.pixelSize: 11
                }
            }

            ToolButton {
                text: "✕"
                onClicked: root.closeRequested()
                palette.buttonText: "#64748b"
                font.pixelSize: 14
                background: null
            }
        }

        // Vạch phân cách mỏng
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#334155"
        }

        // Trạng thái Geofence
        Rectangle {
            Layout.fillWidth: true
            height: 30
            radius: 4
            color: isInsideZone ? "#7f1d1d" : "#064e3b" // Dark red or Dark green

            RowLayout {
                anchors.centerIn: parent
                spacing: 6
                Text {
                    text: isInsideZone ? "⚠️ ĐANG TRONG VÙNG CẢNH BÁO" : "✅ TRẠNG THÁI AN TOÀN"
                    color: isInsideZone ? "#fecaca" : "#d1fae5"
                    font.bold: true
                    font.pixelSize: 11
                }
            }
        }

        // Lưới thông số (Tốc độ, Hướng, Tọa độ)
        GridLayout {
            columns: 2
            rowSpacing: 8
            columnSpacing: 20
            Layout.fillWidth: true

            // Tốc độ
            ColumnLayout {
                spacing: 1
                Text { text: "VẬN TỐC"; color: "#64748b"; font.pixelSize: 9; font.bold: true }
                Text { text: speed.toFixed(1) + " km/h"; color: "#f8fafc"; font.pixelSize: 13; font.bold: true }
            }

            // Hướng mũi
            ColumnLayout {
                spacing: 1
                Text { text: "HƯỚNG ĐI (HEADING)"; color: "#64748b"; font.pixelSize: 9; font.bold: true }
                Text { text: heading.toFixed(1) + "°"; color: "#f8fafc"; font.pixelSize: 13; font.bold: true }
            }

            // Tọa độ Vĩ độ
            ColumnLayout {
                spacing: 1
                Text { text: "VĨ ĐỘ (LAT)"; color: "#64748b"; font.pixelSize: 9; font.bold: true }
                Text { text: latitude.toFixed(6) + " N"; color: "#38bdf8"; font.pixelSize: 12 }
            }

            // Tọa độ Kinh độ
            ColumnLayout {
                spacing: 1
                Text { text: "KINH ĐỘ (LON)"; color: "#64748b"; font.pixelSize: 9; font.bold: true }
                Text { text: longitude.toFixed(6) + " E"; color: "#38bdf8"; font.pixelSize: 12 }
            }
        }

        // Vạch phân cách mỏng
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#334155"
        }

    }

    // Hiệu ứng trượt slide-in/out
    Behavior on y {
        NumberAnimation { duration: 250; easing.type: Easing.OutCubic }
    }
}
