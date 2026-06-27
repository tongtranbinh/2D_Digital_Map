import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    width: 320
    height: Math.min(400, listView.contentHeight)
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.rightMargin: 20
    anchors.topMargin: 20
    z: 999

    ListModel {
        id: alertModel
    }

    // Hàm public để thêm cảnh báo từ C++
    function addAlert(shipId, shipName, zoneId, zoneName, eventType, timeStr) {
        alertModel.insert(0, {
            "shipId": shipId,
            "shipName": shipName,
            "zoneId": zoneId,
            "zoneName": zoneName,
            "eventType": eventType, // "ENTER" hoặc "EXIT"
            "timeStr": timeStr
        });
    }

    ListView {
        id: listView
        anchors.fill: parent
        spacing: 10
        model: alertModel
        interactive: false
        delegate: Rectangle {
            id: alertCard
            width: 320
            height: 70
            radius: 8
            color: "#1e293b" // Slate 800
            opacity: 0.95
            border.color: eventType === "ENTER" ? "#ef4444" : "#3b82f6" // Red for ENTER, Blue for EXIT
            border.width: 1.5

            // Hiệu ứng đổ bóng mờ
            layer.enabled: true

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 12

                // Icon tròn biểu thị trạng thái
                Rectangle {
                    width: 36
                    height: 36
                    radius: 18
                    color: eventType === "ENTER" ? "#fca5a5" : "#93c5fd"
                    Layout.alignment: Qt.AlignVCenter

                    Text {
                        anchors.centerIn: parent
                        text: eventType === "ENTER" ? "⚠️" : "ℹ️"
                        font.pixelSize: 18
                    }
                }

                // Chi tiết văn bản cảnh báo
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        text: shipName
                        color: "#f8fafc"
                        font.bold: true
                        font.pixelSize: 13
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: eventType === "ENTER" 
                              ? "Đi VÀO vùng cảnh báo: " + zoneName
                              : "Đi RA khỏi vùng cảnh báo: " + zoneName
                        color: "#94a3b8"
                        font.pixelSize: 11
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: timeStr
                        color: "#64748b"
                        font.pixelSize: 9
                    }
                }

                // Nút đóng cảnh báo sớm
                ToolButton {
                    text: "✕"
                    onClicked: alertModel.remove(index)
                    palette.buttonText: "#64748b"
                    font.pixelSize: 12
                    background: null
                    Layout.alignment: Qt.AlignTop
                }
            }

            // Tự động đóng sau 6 giây
            Timer {
                interval: 6000
                running: true
                onTriggered: {
                    // Cần kiểm tra xem index có còn tồn tại không
                    if (index >= 0 && index < alertModel.count) {
                        alertModel.remove(index);
                    }
                }
            }

            // Hiệu ứng xuất hiện
            SequentialAnimation {
                id: addAnim
                PropertyAction { target: alertCard; property: "scale"; value: 0.8 }
                PropertyAction { target: alertCard; property: "opacity"; value: 0 }
                ParallelAnimation {
                    NumberAnimation { target: alertCard; property: "scale"; to: 1.0; duration: 250; easing.type: Easing.OutBack }
                    NumberAnimation { target: alertCard; property: "opacity"; to: 0.95; duration: 250 }
                }
            }

            Component.onCompleted: addAnim.start()

            // Hiệu ứng biến mất
            SequentialAnimation {
                id: removeAnim
                onStarted: ListView.delayRemove = true
                onFinished: ListView.delayRemove = false

                ParallelAnimation {
                    NumberAnimation { target: alertCard; property: "scale"; to: 0.8; duration: 200 }
                    NumberAnimation { target: alertCard; property: "opacity"; to: 0; duration: 200 }
                    NumberAnimation { target: alertCard; property: "height"; to: 0; duration: 200 }
                }
            }

            ListView.onRemove: removeAnim.start()
        }
    }
}
