cmake_minimum_required(VERSION 3.16)
project(2D_Digital_Map LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tự động hóa MOC, RCC, UIC của Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Tìm thư viện Qt (Ưu tiên Qt6, nếu không tìm thấy sẽ chọn Qt5)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Sql Quick Qml Location Positioning)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Sql Quick Qml Location Positioning)

message(STATUS "Build using Qt Version: ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}")

# Thu thập toàn bộ file nguồn C++
set(SOURCES
    src/main.cpp
    src/app_engine.h
    src/app_engine.cpp
    src/db/db_manager.h
    src/db/db_manager.cpp
    src/network/tcp_server.h
    src/network/tcp_server.cpp
    src/models/ship_model.h
    src/models/ship_model.cpp
)

# Khai báo file tài nguyên QML rcc
set(QML_RESOURCES src/qml/qml.qrc)

# Tạo file qml.qrc nếu chưa có để đóng gói file QML
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/qml/qml.qrc)
    file(WRITE ${CMAKE_CURRENT_SOURCE_DIR}/src/qml/qml.qrc
        "<!DOCTYPE RCC><RCC version=\"1.0\">\n"
        "<qresource prefix=\"/\">\n"
        "    <file>main.qml</file>\n"
        "    <file>MapComponent.qml</file>\n"
        "    <file>ShipMarker.qml</file>\n"
        "    <file>WarningZoneItem.qml</file>\n"
        "    <file>ShipDetailPanel.qml</file>\n"
        "</qresource>\n"
        "</RCC>\n"
    )
endif()

add_executable(2D_Digital_Map ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/qml/qml.qrc)

target_link_libraries(2D_Digital_Map PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Location
    Qt${QT_VERSION_MAJOR}::Positioning
)

# Cấu hình thư mục đầu ra
set_target_properties(2D_Digital_Map PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
