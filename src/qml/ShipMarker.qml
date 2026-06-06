import QtQuick 2.15
import QtLocation 5.15
import QtPositioning 5.15

MapQuickItem {
    id: marker
    
    // Định nghĩa anchor point nằm chính giữa icon để xoay quanh tâm
    anchorPoint.x: iconItem.width / 2
    anchorPoint.y: iconItem.height / 2
    
    // Gán tọa độ của tàu từ Model
    coordinate: model.coordinate
    
    sourceItem: Item {
        id: iconItem
        width: 44
        height: 44
        
        // Vòng tròn hiệu ứng sóng cảnh báo lan tỏa khi tàu ở trong vùng nguy hiểm
        Rectangle {
            id: alertPulse
            anchors.centerIn: parent
            width: 32
            height: 32
            radius: 16
            color: model.isWarning ? "#ff3333" : "transparent"
            opacity: 0.0
            visible: model.isWarning

            // Hiệu ứng nhấp nháy liên tục vô hạn
            SequentialAnimation on scale {
                loops: Animation.Infinite
                running: model.isWarning
                NumberAnimation { from: 1.0; to: 2.2; duration: 1000; easing.type: Easing.OutQuad }
                NumberAnimation { from: 2.2; to: 1.0; duration: 0 }
            }
            SequentialAnimation on opacity {
                loops: Animation.Infinite
                running: model.isWarning
                NumberAnimation { from: 0.6; to: 0.0; duration: 1000; easing.type: Easing.OutQuad }
                NumberAnimation { from: 0.0; to: 0.0; duration: 100 }
            }
        }
        
        // Icon hình mũi tàu xoay theo hướng di chuyển (Bearing/Course)
        Item {
            id: shipArrow
            width: 24
            height: 24
            anchors.centerIn: parent
            rotation: model.course // Góc xoay của tàu (0 - 360 độ)
            
            Canvas {
                id: shipCanvas
                anchors.fill: parent
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.beginPath();
                    
                    // Vẽ hình đa giác mũi tàu gọn đẹp
                    ctx.moveTo(12, 2);   // Mũi tàu (trên cùng)
                    ctx.lineTo(22, 18);  // Đuôi tàu bên phải
                    ctx.lineTo(12, 14);  // Lõm đuôi tàu
                    ctx.lineTo(2, 18);   // Đuôi tàu bên trái
                    ctx.closePath();
                    
                    // Lựa chọn màu sắc dựa trên trạng thái cảnh báo
                    var gradient = ctx.createLinearGradient(12, 2, 12, 18);
                    if (model.isWarning) {
                        gradient.addColorStop(0, "#ff5555"); // Đỏ tươi
                        gradient.addColorStop(1, "#b30000"); // Đỏ đậm
                    } else {
                        gradient.addColorStop(0, "#00ffcc"); // Xanh lục neon
                        gradient.addColorStop(1, "#006644"); // Xanh lục đậm
                    }
                    
                    ctx.fillStyle = gradient;
                    ctx.fill();
                    
                    // Viền màu trắng cho nổi bật trên bản đồ
                    ctx.strokeStyle = "#ffffff";
                    ctx.lineWidth = 1.5;
                    ctx.stroke();
                }
                
                // Vẽ lại khi trạng thái cảnh báo thay đổi
                Connections {
                    target: model
                    function onIsWarningChanged() {
                        shipCanvas.requestPaint();
                    }
                }
            }
        }
        
        // Nhãn tên tàu bên dưới icon
        Rectangle {
            anchors.top: shipArrow.bottom
            anchors.topMargin: 4
            anchors.horizontalCenter: parent.horizontalCenter
            color: model.isWarning ? "rgba(220, 50, 50, 0.95)" : "rgba(30, 40, 50, 0.85)"
            border.color: model.isWarning ? "#ff9999" : "#00ffcc"
            border.width: 1
            radius: 3
            width: labelText.implicitWidth + 8
            height: labelText.implicitHeight + 4
            
            Text {
                id: labelText
                anchors.centerIn: parent
                text: model.name
                color: "#ffffff"
                font.pixelSize: 9
                font.bold: true
            }
        }
        
        // Bắt sự kiện click chuột chọn tàu
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                // Gọi hàm chọn tàu trên Component bản đồ cha
                mapComponent.selectShip(model.mmsi);
            }
        }
    }
}
