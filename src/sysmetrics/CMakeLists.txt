qt_add_library(dashboard_sysmetrics STATIC
    linux_sys_metrics_collector.cpp
    linux_sys_metrics_collector.h
    system_metric_history_model.cpp
    system_metric_history_model.h
    sys_metrics_collector.h
    sys_metrics_service.cpp
    sys_metrics_service.h
)

add_library(RpiDashboard::sysmetrics ALIAS dashboard_sysmetrics)

target_include_directories(dashboard_sysmetrics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(dashboard_sysmetrics PUBLIC RpiDashboard::protocol Qt6::Core PRIVATE Qt6::Concurrent)
