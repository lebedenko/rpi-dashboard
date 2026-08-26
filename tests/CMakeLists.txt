find_package(Qt6 6.8 REQUIRED COMPONENTS QuickTest Test)

qt_add_executable(device_snapshot_test
    device_snapshot_test.cpp
)

target_link_libraries(device_snapshot_test
    PRIVATE
        RpiDashboard::protocol
        Qt6::Test
)

add_test(NAME device_snapshot_test COMMAND device_snapshot_test)

qt_add_executable(system_info_test
    system_info_test.cpp
)

target_link_libraries(system_info_test
    PRIVATE
        RpiDashboard::sysinfo
        Qt6::Test
)

add_test(NAME system_info_test COMMAND system_info_test)

qt_add_executable(system_metrics_test
    system_metrics_test.cpp
)

target_link_libraries(system_metrics_test
    PRIVATE
        RpiDashboard::sysmetrics
        Qt6::Test
)

add_test(NAME system_metrics_test COMMAND system_metrics_test)

qt_add_executable(projects_service_test
    projects_service_test.cpp
)

target_link_libraries(projects_service_test
    PRIVATE
        RpiDashboard::projects
        Qt6::Test
)

add_test(NAME projects_service_test COMMAND projects_service_test)

qt_add_executable(resource_history_geometry_test
    resource_history_geometry_test.cpp
    ${CMAKE_SOURCE_DIR}/src/dashboard/resource_history_geometry.cpp
)

target_include_directories(resource_history_geometry_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src/dashboard
)

target_link_libraries(resource_history_geometry_test
    PRIVATE
        Qt6::Test
)

add_test(NAME resource_history_geometry_test COMMAND resource_history_geometry_test)

qt_add_executable(resource_history_series_test
    resource_history_series_test.cpp
    ${CMAKE_SOURCE_DIR}/src/dashboard/resource_history_geometry.cpp
    ${CMAKE_SOURCE_DIR}/src/dashboard/resource_history_series.cpp
    ${CMAKE_SOURCE_DIR}/src/sysmetrics/system_metric_history_model.cpp
)

target_include_directories(resource_history_series_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/dashboard
)

target_link_libraries(resource_history_series_test
    PRIVATE
        Qt6::Quick
        Qt6::Test
)

add_test(NAME resource_history_series_test COMMAND resource_history_series_test -platform offscreen)

qt_add_executable(dashboard_startup_test
    dashboard_startup_test.cpp
)

add_dependencies(dashboard_startup_test rpi-dashboard)

target_compile_definitions(dashboard_startup_test
    PRIVATE
        DASHBOARD_EXECUTABLE="$<TARGET_FILE:rpi-dashboard>"
        DASHBOARD_CLOCK_SIDEBAR_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/ClockSidebar.qml"
        DASHBOARD_MAIN_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/Main.qml"
        DASHBOARD_DEVICE_CARD_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/DeviceCard.qml"
        DASHBOARD_OVERVIEW_PAGE_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/OverviewPage.qml"
        DASHBOARD_PROJECTS_PAGE_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/ProjectsPage.qml"
        DASHBOARD_PAGE_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/DashboardPage.qml"
        DASHBOARD_SIDEBAR_BUTTON_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/SidebarButton.qml"
        DASHBOARD_THEME_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/Theme.qml"
        DASHBOARD_ICON_DIRECTORY="${CMAKE_SOURCE_DIR}/src/dashboard/qml/icons"
)

target_link_libraries(dashboard_startup_test
    PRIVATE
        Qt6::Test
)

add_test(NAME dashboard_startup_test COMMAND dashboard_startup_test)

qt_add_executable(dashboard_qml_test
    MANUAL_FINALIZATION
    dashboard_qml_test.cpp
)

target_sources(dashboard_qml_test
    PRIVATE
        tst_Main.qml
)

target_compile_definitions(dashboard_qml_test
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

set_property(TARGET dashboard_qml_test PROPERTY
    QT_QML_IMPORT_PATH ${CMAKE_BINARY_DIR}/src/dashboard
)

target_link_libraries(dashboard_qml_test
    PRIVATE
        rpi-dashboard-qml
        Qt6::QuickTest
)

qt_import_qml_plugins(dashboard_qml_test)
qt_finalize_executable(dashboard_qml_test)

add_test(NAME dashboard_qml_test COMMAND dashboard_qml_test -platform offscreen)
set_tests_properties(dashboard_qml_test PROPERTIES
    ENVIRONMENT "QT_SCALE_FACTOR=1;QT_QUICK_BACKEND=software"
)

add_test(
    NAME kiosk_launcher_test
    COMMAND
        sh
        ${CMAKE_CURRENT_SOURCE_DIR}/kiosk_launcher_test.sh
        ${CMAKE_SOURCE_DIR}/scripts/run-kiosk.sh
        ${CMAKE_SOURCE_DIR}/scripts/run-dashboard-session.sh
)

add_test(
    NAME clang_tidy_launcher_test
    COMMAND
        sh
        ${CMAKE_CURRENT_SOURCE_DIR}/clang_tidy_launcher_test.sh
        ${CMAKE_SOURCE_DIR}/scripts/clang-tidy-launcher.sh
)

add_test(
    NAME ci_container_runner_test
    COMMAND
        sh
        ${CMAKE_CURRENT_SOURCE_DIR}/ci_container_runner_test.sh
        ${CMAKE_SOURCE_DIR}/scripts/run-ci-container.sh
)

add_test(
    NAME install_flow_test
    COMMAND
        sh
        ${CMAKE_CURRENT_SOURCE_DIR}/install_flow_test.sh
        ${CMAKE_COMMAND}
        ${CMAKE_BINARY_DIR}
        ${CMAKE_SOURCE_DIR}
)
