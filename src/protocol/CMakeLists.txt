qt_add_library(dashboard_protocol STATIC
    device_snapshot.cpp
    device_snapshot.h
    system_info.cpp
    system_info.h
    system_metrics.cpp
    system_metrics.h
)

add_library(HoloNightDashboard::protocol ALIAS dashboard_protocol)

target_include_directories(dashboard_protocol
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(dashboard_protocol
    PUBLIC
        Qt6::Core
)
