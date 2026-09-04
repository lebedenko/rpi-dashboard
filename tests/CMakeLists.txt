find_package(Qt6 6.8 REQUIRED COMPONENTS QuickTest Test)

qt_add_executable(dashboard_config_test
    dashboard_config_test.cpp
    ${CMAKE_SOURCE_DIR}/src/dashboard/dashboard_config.cpp
)
target_include_directories(dashboard_config_test PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/dashboard)
target_compile_definitions(dashboard_config_test PRIVATE DASHBOARD_CONFIG_SOURCE="${CMAKE_SOURCE_DIR}/install/config.toml")
target_link_libraries(dashboard_config_test PRIVATE RpiDashboard::weather Qt6::Network Qt6::Test tomlplusplus::tomlplusplus)
add_test(NAME dashboard_config_test COMMAND dashboard_config_test)

qt_add_executable(screensaver_controller_test screensaver_controller_test.cpp)
target_include_directories(screensaver_controller_test PRIVATE ${CMAKE_SOURCE_DIR}/src/dashboard)
target_link_libraries(screensaver_controller_test PRIVATE rpi-dashboard-qml Qt6::Gui Qt6::Test)
add_test(NAME screensaver_controller_test COMMAND screensaver_controller_test -platform offscreen)

qt_add_executable(weather_test weather_test.cpp)
target_include_directories(weather_test PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/dashboard)
target_compile_definitions(weather_test PRIVATE WEATHER_ICON_SOURCE_DIR="${CMAKE_SOURCE_DIR}/assets/weather-icons")
target_link_libraries(weather_test PRIVATE RpiDashboard::weather rpi-dashboard-qml Qt6::Quick Qt6::Test)
add_test(NAME weather_test COMMAND weather_test)
set_tests_properties(weather_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

qt_add_executable(device_model_test device_model_test.cpp)
target_include_directories(device_model_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(device_model_test PRIVATE rpi-dashboard-qml RpiDashboard::sysinfo RpiDashboard::sysmetrics RpiDashboard::telemetry Qt6::Test)
add_test(NAME device_model_test COMMAND device_model_test)

qt_add_executable(device_snapshot_test
    device_snapshot_test.cpp
)

target_link_libraries(device_snapshot_test
    PRIVATE
        RpiDashboard::protocol
        Qt6::Test
)

add_test(NAME device_snapshot_test COMMAND device_snapshot_test)

qt_add_executable(remote_device_registry_test
    remote_device_registry_test.cpp
)
target_link_libraries(remote_device_registry_test PRIVATE RpiDashboard::telemetry Qt6::Test)
add_test(NAME remote_device_registry_test COMMAND remote_device_registry_test)

qt_add_executable(udp_telemetry_receiver_test
    udp_telemetry_receiver_test.cpp
)
target_link_libraries(udp_telemetry_receiver_test PRIVATE RpiDashboard::telemetry Qt6::Test)
add_test(NAME udp_telemetry_receiver_test COMMAND udp_telemetry_receiver_test)

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
        DASHBOARD_FRAME_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/Frame.qml"
        DASHBOARD_OVERVIEW_PAGE_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/OverviewPage.qml"
        DASHBOARD_PROJECTS_PAGE_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/ProjectsPage.qml"
        DASHBOARD_SEPARATOR_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/Separator.qml"
        DASHBOARD_SYSTEM_PAGE_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/SystemPage.qml"
        DASHBOARD_SYSTEM_METRIC_PANEL_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/SystemMetricPanel.qml"
        DASHBOARD_PAGE_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/DashboardPage.qml"
        DASHBOARD_SIDEBAR_BUTTON_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/SidebarButton.qml"
        DASHBOARD_THEME_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/Theme.qml"
        DASHBOARD_WEATHER_PAGE_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/WeatherPage.qml"
        DASHBOARD_WEATHER_GRAPH_SEGMENT_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/WeatherGraphSegment.qml"
        DASHBOARD_ICON_DIRECTORY="${CMAKE_SOURCE_DIR}/src/dashboard/qml/icons"
        DASHBOARD_FONT_DIRECTORY="${CMAKE_SOURCE_DIR}/src/dashboard/qml/fonts"
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
        tst_Frame.qml
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

qt_add_executable(separator_qml_test
    MANUAL_FINALIZATION
    dashboard_qml_test.cpp
)

target_sources(separator_qml_test
    PRIVATE
        separator/SeparatorTest.qml
)

target_compile_definitions(separator_qml_test
    PRIVATE
        QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/separator"
)

set_property(TARGET separator_qml_test PROPERTY
    QT_QML_IMPORT_PATH ${CMAKE_BINARY_DIR}/src/dashboard
)

target_link_libraries(separator_qml_test
    PRIVATE
        rpi-dashboard-qml
        Qt6::QuickTest
)

qt_import_qml_plugins(separator_qml_test)
qt_finalize_executable(separator_qml_test)

add_test(
    NAME separator_qml_test_1x
    COMMAND separator_qml_test -input ${CMAKE_CURRENT_SOURCE_DIR}/separator/SeparatorTest.qml -platform offscreen
)
set_tests_properties(separator_qml_test_1x PROPERTIES
    ENVIRONMENT "QT_SCALE_FACTOR=1;QT_QUICK_BACKEND=software"
)

add_test(
    NAME separator_qml_test_1_25x
    COMMAND separator_qml_test -input ${CMAKE_CURRENT_SOURCE_DIR}/separator/SeparatorTest.qml -platform offscreen
)
set_tests_properties(separator_qml_test_1_25x PROPERTIES
    ENVIRONMENT "QT_SCALE_FACTOR=1.25;QT_QUICK_BACKEND=software"
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
    NAME ci_workflow_test
    COMMAND
        sh
        ${CMAKE_CURRENT_SOURCE_DIR}/ci_workflow_test.sh
        ${CMAKE_SOURCE_DIR}/.github/workflows/ci.yml
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

add_test(
    NAME executable_version_test
    COMMAND
        sh
        ${CMAKE_CURRENT_SOURCE_DIR}/executable_version_test.sh
        $<TARGET_FILE:rpi-dashboard>
        ${RpiDashboard_VERSION}
)

add_test(
    NAME release_tool_test
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/release_tool_test.sh ${CMAKE_SOURCE_DIR}/scripts/release.sh
)

add_test(
    NAME dashboard_archive_release_tool_test
    COMMAND
        sh
        ${CMAKE_CURRENT_SOURCE_DIR}/dashboard_archive_release_tool_test.sh
        ${CMAKE_SOURCE_DIR}/scripts/release.sh
)

add_test(
    NAME release_workflow_test
    COMMAND
        sh
        ${CMAKE_CURRENT_SOURCE_DIR}/release_workflow_test.sh
        ${CMAKE_SOURCE_DIR}/.github/workflows/release.yml
)

add_test(
    NAME release_changelog_test
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/release_changelog_test.sh ${CMAKE_SOURCE_DIR}
)

add_test(
    NAME release_publish_test
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/release_publish_test.sh ${CMAKE_SOURCE_DIR}/scripts/release-publish.sh
)

add_test(
    NAME conventional_title_test
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/conventional_title_test.sh ${CMAKE_SOURCE_DIR}/scripts/validate-conventional.sh
)

add_test(
    NAME activation_flow_test
    COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/activation_flow_test.sh ${CMAKE_SOURCE_DIR}/install/activate-release.sh
)
