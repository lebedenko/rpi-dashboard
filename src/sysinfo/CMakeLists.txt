qt_add_library(dashboard_sysinfo STATIC
    linux_sys_info_collector.cpp
    linux_sys_info_collector.h
    sys_info_collector.h
    sys_info_service.cpp
    sys_info_service.h
)

add_library(RpiDashboard::sysinfo ALIAS dashboard_sysinfo)

target_include_directories(dashboard_sysinfo
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(dashboard_sysinfo
    PUBLIC
        RpiDashboard::protocol
        Qt6::Core
    PRIVATE
        Qt6::Concurrent
)
