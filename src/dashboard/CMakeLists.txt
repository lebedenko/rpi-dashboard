qt_add_library(holonight-dashboard-qml STATIC)

set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(holonight-dashboard-qml
    URI HoloNight.Dashboard
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/HoloNight/Dashboard
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

target_link_libraries(holonight-dashboard-qml
    PRIVATE
        Qt6::Qml
        Qt6::Quick
)

qt_add_executable(holonight-dashboard
    MANUAL_FINALIZATION
    main.cpp
)

target_link_libraries(holonight-dashboard
    PRIVATE
        HoloNightDashboard::protocol
        HoloNightDashboard::projects
        HoloNightDashboard::sysinfo
        HoloNightDashboard::sysmetrics
        holonight-dashboard-qml
        holonight-dashboard-qmlplugin
        holonight-dashboard-qmlplugin_init
        Qt6::Gui
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
)

qt_import_qml_plugins(holonight-dashboard)
qt_finalize_executable(holonight-dashboard)

set_target_properties(holonight-dashboard PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

install(TARGETS holonight-dashboard
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
