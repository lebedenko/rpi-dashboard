qt_add_library(dashboard_telemetry STATIC
    remote_device_registry.cpp
    remote_device_registry.h
    udp_telemetry_receiver.cpp
    udp_telemetry_receiver.h
)

add_library(RpiDashboard::telemetry ALIAS dashboard_telemetry)

target_include_directories(dashboard_telemetry PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(dashboard_telemetry PUBLIC RpiDashboard::protocol Qt6::Core Qt6::Network)
