import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1280
    height: 720
    title: "2D Digital Map - Real-time Maritime Object Tracking"

    // 1. Thành phần bản đồ số nền OpenStreetMap
    MapComponent {
        id: mainMap
        anchors.fill: parent
        
        onShipSelected: {
            if (mmsi === -1) {
                detailPanel.isOpen = false;
            } else {
                // Lấy dữ liệu chi tiết của tàu từ C++ Model thông qua Q_INVOKABLE
                var data = shipModel.getShipDetails(mmsi);
                if (data.mmsi !== undefined) {
                    detailPanel.shipData = data;
                    detailPanel.isOpen = true;
                }
            }
        }
    }
    
    // Timer tự động cập nhật dữ liệu của Panel chi tiết nếu tàu đang được chọn thay đổi tọa độ liên tục
    Timer {
        id: detailUpdateTimer
        interval: 1000
        repeat: true
        running: detailPanel.isOpen
        onTriggered: {
            if (detailPanel.shipData) {
                var data = shipModel.getShipDetails(detailPanel.shipData.mmsi);
                if (data.mmsi !== undefined) {
                    detailPanel.shipData = data;
                }
            }
        }
    }

    // 2. Thanh tiêu đề phía trên (Header Banner) - Phong cách Glassmorphism
    Rectangle {
        id: headerBar
        height: 60
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 15
        radius: 8
        color: "#f20f172a" // Slate 900 (95% Opacity)
        border.color: "#334155"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20

            RowLayout {
                spacing: 10
                Text {
                    text: "🌐"
                    font.pixelSize: 22
                }
                ColumnLayout {
                    spacing: 0
                    Text {
                        text: "HỆ THỐNG GIÁM SÁT TÀU BIỂN THỜI GIAN THỰC"
                        color: "#f8fafc"
                        font.pixelSize: 15
                        font.bold: true
                        font.letterSpacing: 1.0
                    }
                    Text {
                        text: "Bản đồ số 2D định vị đối tượng mặt biển sử dụng PostGIS & QML"
                        color: "#94a3b8"
                        font.pixelSize: 11
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // Widget hiển thị số lượng tàu hoạt động
            Rectangle {
                width: 140
                height: 36
                color: "#1e293b"
                radius: 6
                border.color: "#475569"
                
                RowLayout {
                    anchors.centerIn: parent
                    spacing: 8
                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#10b981" // Xanh lục nhấp nháy chỉ thị hoạt động
                        SequentialAnimation on opacity {
                            loops: Animation.Infinite
                            NumberAnimation { from: 1.0; to: 0.3; duration: 800 }
                            NumberAnimation { from: 0.3; to: 1.0; duration: 800 }
                        }
                    }
                    Text {
                        text: "Hoạt động: " + shipModel.rowCount()
                        color: "#cbd5e1"
                        font.pixelSize: 12
                        font.bold: true
                    }
                }
            }
        }
    }

    // 3. Panel hiển thị thông tin chi tiết tàu (Slide-in)
    ShipDetailPanel {
        id: detailPanel
        isOpen: false
    }

    // 4. Hộp thoại thông báo cảnh báo (Toast Alert) - Trượt xuống khi có tàu vào vùng nguy hiểm
    Rectangle {
        id: alertToast
        width: 320
        height: 70
        radius: 8
        color: toastSeverity === "critical" ? "#dc2626" : "#d97706" // Đỏ cho critical, Cam cho warning
        border.color: "#ffffff"
        border.width: 1
        
        // Vị trí trượt xuống
        x: 20
        y: toastActive ? 90 : -height - 20
        opacity: toastActive ? 1.0 : 0.0

        Behavior on y {
            NumberAnimation { duration: 400; easing.type: Easing.OutBack }
        }
        Behavior on opacity {
            NumberAnimation { duration: 300 }
        }

        property bool toastActive: false
        property string toastMessage: ""
        property string toastSeverity: ""

        // Hẹn giờ ẩn cảnh báo sau 4 giây
        Timer {
            id: toastTimer
            interval: 4000
            onTriggered: {
                alertToast.toastActive = false;
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 12

            Text {
                text: "🚨"
                font.pixelSize: 26
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: alertToast.toastSeverity === "critical" ? "CẢNH BÁO NGUY HIỂM" : "CẢNH BÁO VÙNG ĐẬU"
                    color: "#ffffff"
                    font.bold: true
                    font.pixelSize: 12
                }
                Text {
                    text: alertToast.toastMessage
                    color: "#f8fafc"
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    wrapMode: Text.Wrap
                }
            }
            
            Button {
                flat: true
                implicitWidth: 24
                implicitHeight: 24
                contentItem: Text { text: "✕"; color: "#ffffff"; font.pixelSize: 12 }
                onClicked: alertToast.toastActive = false
            }
        }
    }

    // Kết nối Signal từ AppEngine C++ để hiển thị Toast Alert trên UI
    Connections {
        target: appEngine
        
        function onShipEnteredWarningZone(shipName, zoneName, severity, mmsi) {
            alertToast.toastMessage = "Tàu \"" + shipName + "\" đi vào \"" + zoneName + "\"";
            alertToast.toastSeverity = severity;
            alertToast.toastActive = true;
            toastTimer.restart(); // Chạy lại timer 4 giây
        }
    }
}
