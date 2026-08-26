qt_add_library(rpi-dashboard-qml STATIC)

set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(rpi-dashboard-qml
    URI Rpi.Dashboard
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Rpi/Dashboard
    SOURCES
        resource_history_geometry.cpp
        resource_history_geometry.h
        resource_history_series.cpp
        resource_history_series.h
    QML_FILES
        qml/ClockSidebar.qml
        qml/DashboardPage.qml
        qml/DeviceCard.qml
        qml/Main.qml
        qml/OverviewPage.qml
        qml/ProjectsPage.qml
        qml/SystemMetricPanel.qml
        qml/SystemPage.qml
        qml/Theme.qml
        qml/SidebarButton.qml
    RESOURCES
        qml/icons/chevron.svg
        qml/icons/detail-arch.svg
        qml/icons/detail-cores.svg
        qml/icons/detail-cpu.svg
        qml/icons/detail-hardware.svg
        qml/icons/detail-kernel.svg
        qml/icons/detail-memory.svg
        qml/icons/detail-os.svg
        qml/icons/overview.svg
        qml/icons/projects.svg
        qml/icons/systems.svg
        qml/icons/weather.svg
)

target_link_libraries(rpi-dashboard-qml
    PRIVATE
        Qt6::Qml
        Qt6::Quick
)

qt_add_executable(rpi-dashboard
    MANUAL_FINALIZATION
    main.cpp
)

target_link_libraries(rpi-dashboard
    PRIVATE
        RpiDashboard::protocol
        RpiDashboard::projects
        RpiDashboard::sysinfo
        RpiDashboard::sysmetrics
        rpi-dashboard-qml
        rpi-dashboard-qmlplugin
        rpi-dashboard-qmlplugin_init
        Qt6::Gui
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
)

qt_import_qml_plugins(rpi-dashboard)
qt_finalize_executable(rpi-dashboard)

set_target_properties(rpi-dashboard PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS rpi-dashboard
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
