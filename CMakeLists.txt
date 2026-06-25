cmake_minimum_required(VERSION 3.16)

project(ShipTracking LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automoc for QObject-derived classes with Q_OBJECT
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Sql Positioning Qml Quick QuickControls2)

add_executable(ShipTracking
    src/main.cpp
    # network
    src/network/TcpServer.cpp
    src/network/tcpserver.h          # Q_OBJECT — thêm explicit để AUTOMOC xử lý đúng trên Linux
    # database
    src/database/PostgresConnection.cpp
    src/database/ShipRepository.cpp
    src/database/PositionRepository.cpp
    src/database/AlertRepository.cpp
    # service
    src/service/ShipService.cpp
    src/service/PositionService.cpp
    src/service/AlertService.cpp
    src/service/TrackingService.cpp
    # state
    src/state/ShipStateStore.cpp
    # worker
    src/worker/PositionWorker.cpp
    src/worker/PositionWorker.h      # Q_OBJECT
    src/worker/ShipWorker.cpp
    src/worker/ShipWorker.h          # Q_OBJECT
    src/worker/ShipJsonParser.cpp
    # ui
    src/ui/ShipListModel.cpp
    src/ui/ShipListModel.h           # Q_OBJECT
    src/ui/ZoneListModel.cpp
    src/ui/ZoneListModel.h           # Q_OBJECT
    src/ui/MapController.cpp
    src/ui/MapController.h           # Q_OBJECT
    src/ui/ShipRenderLayer.cpp
    src/ui/ShipRenderLayer.h         # Q_OBJECT
)

# Đăng ký mô-đun QML cho target ShipTracking
qt_add_qml_module(ShipTracking
    URI ShipTracking
    VERSION 1.0
    QML_FILES
        src/ui/qml/main.qml
        src/ui/qml/VesselMap.qml
        src/ui/qml/Sidebar.qml
        src/ui/qml/AlertBanner.qml
        src/ui/qml/ShipDetailPanel.qml
)

# Đổi tên file chạy đầu ra thành ShipTrackingApp để tránh trùng tên với thư mục QML Module
set_target_properties(ShipTracking PROPERTIES OUTPUT_NAME "ShipTrackingApp")

target_include_directories(ShipTracking PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ShipTracking
    PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Widgets
    Qt6::Sql
    Qt6::Positioning
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)
