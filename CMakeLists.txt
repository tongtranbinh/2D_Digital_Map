cmake_minimum_required(VERSION 3.16)

project(ShipTracking LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable automoc for QObject-derived classes with Q_OBJECT
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Sql)

add_executable(ShipTracking
    src/main.cpp
    src/network/TcpServer.cpp
    src/database/PostgresConnection.cpp
    src/database/ShipRepository.cpp
    src/database/PositionRepository.cpp
    src/database/AlertRepository.cpp
    src/service/ShipService.cpp
    src/service/PositionService.cpp
    src/service/AlertService.cpp
    src/service/TrackingService.cpp
    src/state/ShipStateStore.cpp
    src/worker/PositionWorker.cpp
    src/worker/ShipWorker.cpp
    src/worker/ShipJsonParser.cpp
)

target_include_directories(ShipTracking PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(ShipTracking
    PRIVATE
    Qt6::Core
    Qt6::Network
)
    
target_link_libraries(ShipTracking
    PRIVATE
    Qt6::Widgets
    Qt6::Sql
)