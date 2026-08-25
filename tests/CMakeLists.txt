find_package(Qt6 6.8 REQUIRED COMPONENTS QuickTest Test)

qt_add_executable(device_snapshot_test
    device_snapshot_test.cpp
)

target_link_libraries(device_snapshot_test
    PRIVATE
        HoloNightDashboard::protocol
        Qt6::Test
)

add_test(NAME device_snapshot_test COMMAND device_snapshot_test)

qt_add_executable(system_info_test
    system_info_test.cpp
)

target_link_libraries(system_info_test
    PRIVATE
        HoloNightDashboard::sysinfo
        Qt6::Test
)

add_test(NAME system_info_test COMMAND system_info_test)

qt_add_executable(system_metrics_test
    system_metrics_test.cpp
)

target_link_libraries(system_metrics_test
    PRIVATE
        HoloNightDashboard::sysmetrics
        Qt6::Test
)

add_test(NAME system_metrics_test COMMAND system_metrics_test)

qt_add_executable(dashboard_startup_test
    dashboard_startup_test.cpp
)

add_dependencies(dashboard_startup_test holonight-dashboard)

target_compile_definitions(dashboard_startup_test
    PRIVATE
        DASHBOARD_EXECUTABLE="$<TARGET_FILE:holonight-dashboard>"
        DASHBOARD_MAIN_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/Main.qml"
        DASHBOARD_DEVICE_CARD_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/DeviceCard.qml"
        DASHBOARD_OVERVIEW_PAGE_QML="${CMAKE_SOURCE_DIR}/src/dashboard/qml/OverviewPage.qml"
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
        holonight-dashboard-qml
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
