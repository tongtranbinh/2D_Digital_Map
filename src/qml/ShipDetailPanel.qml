import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: rootPanel
    
    // Chiều rộng và vị trí bảng điều khiển
    width: 320
    height: parent.height - 40
    anchors.top: parent.top
    anchors.topMargin: 20
    
    // Slide-in animation bằng cách thay đổi rightMargin
    anchors.right: parent.right
    anchors.rightMargin: isOpen ? 20 : -width - 40
    
    Behavior on anchors.rightMargin {
        NumberAnimation { duration: 350; easing.type: Easing.OutCubic }
    }
    
    property bool isOpen: false
    property var shipData: null // Object chứa thông tin tàu nhận từ C++
    
    // Thiết kế Glassmorphism tối màu sang trọng
    color: "#f20f172a" // Slate 900 (95% Opacity)
    radius: 12
    border.width: 1.5
    border.color: (shipData && shipData.isWarning) ? "#ef4444" : "#334155" // Viền đỏ nếu tàu bị cảnh báo
    
    // Bộ lọc bóng mờ hoặc viền phát sáng nhẹ
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        radius: 12
        border.width: 1
        border.color: "#ffffff"
        opacity: 0.05
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        // Header: Tên tàu và Nút Đóng
        RowLayout {
            Layout.fillWidth: true
            
            ColumnLayout {
                spacing: 2
                Text {
                    text: shipData ? shipData.name : ""
                    color: "#ffffff"
                    font.pixelSize: 18
                    font.bold: true
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Text {
                    text: shipData ? "MMSI: " + shipData.mmsi : ""
                    color: "#94a3b8"
                    font.pixelSize: 12
                }
            }

            // Nút đóng Panel
            Button {
                id: closeButton
                flat: true
                implicitWidth: 32
                implicitHeight: 32
                
                background: Rectangle {
                    color: closeButton.hovered ? "#334155" : "transparent"
                    radius: 16
                }
                
                contentItem: Text {
                    text: "✕"
                    color: "#ffffff"
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                
                onClicked: {
                    rootPanel.isOpen = false;
                }
            }
        }

        // Đường kẻ phân cách
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#334155"
        }

        // Body: Hiển thị trạng thái Cảnh báo
        Rectangle {
            Layout.fillWidth: true
            height: 60
            radius: 8
            color: (shipData && shipData.isWarning) ? "rgba(239, 68, 68, 0.15)" : "rgba(16, 185, 129, 0.1)"
            border.width: 1
            border.color: (shipData && shipData.isWarning) ? "#ef4444" : "#10b981"
            visible: shipData !== null

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 10

                Text {
                    text: (shipData && shipData.isWarning) ? "⚠️" : "⚓"
                    font.pixelSize: 24
                }

                ColumnLayout {
                    spacing: 2
                    Text {
                        text: (shipData && shipData.isWarning) ? "VI PHẠM CẢNH BÁO" : "AN TOÀN"
                        color: (shipData && shipData.isWarning) ? "#f87171" : "#34d399"
                        font.bold: true
                        font.pixelSize: 12
                    }
                    Text {
                        text: (shipData && shipData.isWarning) ? shipData.warningName : "Nằm ngoài vùng nguy hiểm"
                        color: "#cbd5e1"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // Chi tiết tọa độ và chuyển động của tàu
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // Hàng 1: Vĩ độ / Kinh độ
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                
                // Vĩ độ
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: "VĨ ĐỘ (LAT)"; color: "#64748b"; font.pixelSize: 9; font.bold: true }
                    Text { text: shipData ? shipData.lat.toFixed(6) + "° N" : ""; color: "#f8fafc"; font.pixelSize: 14; font.bold: true }
                }
                
                // Kinh độ
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: "KINH ĐỘ (LON)"; color: "#64748b"; font.pixelSize: 9; font.bold: true }
                    Text { text: shipData ? shipData.lon.toFixed(6) + "° E" : ""; color: "#f8fafc"; font.pixelSize: 14; font.bold: true }
                }
            }

            // Hàng 2: Vận tốc / Hướng đi
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                
                // Vận tốc
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: "TỐC ĐỘ (SPEED)"; color: "#64748b"; font.pixelSize: 9; font.bold: true }
                    Text { text: shipData ? shipData.speed.toFixed(1) + " Knots" : ""; color: "#f8fafc"; font.pixelSize: 14; font.bold: true }
                }
                
                // Hướng đi
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: "HƯỚNG ĐI (COURSE)"; color: "#64748b"; font.pixelSize: 9; font.bold: true }
                    Text { text: shipData ? shipData.course.toFixed(0) + "°" : ""; color: "#f8fafc"; font.pixelSize: 14; font.bold: true }
                }
            }

            // Hàng 3: Cập nhật cuối
            ColumnLayout {
                spacing: 2
                Text { text: "CẬP NHẬT CUỐI"; color: "#64748b"; font.pixelSize: 9; font.bold: true }
                Text { text: shipData ? shipData.timestamp : ""; color: "#cbd5e1"; font.pixelSize: 12 }
            }
        }

        Item { Layout.fillHeight: true } // Đẩy các phần tử lên trên
    }
}
