qt_add_library(rpi-dashboard-qml STATIC)

set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(rpi-dashboard-qml
    URI Rpi.Dashboard
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/Rpi/Dashboard
    SOURCES
        device_model.cpp
        device_model.h
        resource_history_geometry.cpp
        resource_history_geometry.h
        resource_history_series.cpp
        resource_history_series.h
        screensaver_controller.cpp
        screensaver_controller.h
        weather_icon_provider.cpp
        weather_icon_provider.h
    QML_FILES
        qml/ClockSidebar.qml
        qml/DashboardPage.qml
        qml/DeviceCard.qml
        qml/Main.qml
        qml/OverviewPage.qml
        qml/ProjectsPage.qml
        qml/ScreensaverView.qml
        qml/SystemMetricPanel.qml
        qml/SystemPage.qml
        qml/WeatherPage.qml
        qml/Theme.qml
        qml/SidebarButton.qml
    RESOURCES
        qml/fonts/JetBrainsMono-Light.ttf
        qml/fonts/JetBrainsMono-Medium.ttf
        qml/fonts/JetBrainsMono-Regular.ttf
        qml/fonts/OFL-JetBrainsMono.txt
        qml/fonts/OFL-Rajdhani.txt
        qml/fonts/Rajdhani-Light.ttf
        qml/fonts/Rajdhani-Medium.ttf
        qml/fonts/Rajdhani-Regular.ttf
        qml/fonts/Rajdhani-SemiBold.ttf
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

set(weather_icon_files
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/01d.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/01n.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/02d.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/02n.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/03d.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/03n.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/04d.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/04n.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/09d.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/09n.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/10d.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/10n.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/11d.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/11n.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/13d.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/13n.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/50d.svg
    ${CMAKE_SOURCE_DIR}/assets/weather-icons/50n.svg
)
qt_add_resources(rpi-dashboard-qml "rpi_dashboard_weather_icons"
    PREFIX "/rpi-dashboard"
    BASE ${CMAKE_SOURCE_DIR}/assets
    FILES ${weather_icon_files}
)

set(weather_background_files
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/01d.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/01n.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/02d.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/02n.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/03d.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/03n.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/04d.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/04n.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/09d.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/09n.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/10d.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/10n.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/11d.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/11n.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/13d.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/13n.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/50d.png
    ${CMAKE_SOURCE_DIR}/assets/weather-bg/50n.png
)
qt_add_resources(rpi-dashboard-qml "rpi_dashboard_weather_backgrounds"
    PREFIX "/rpi-dashboard"
    BASE ${CMAKE_SOURCE_DIR}/assets
    FILES ${weather_background_files}
)

target_link_libraries(rpi-dashboard-qml
    PRIVATE
        RpiDashboard::protocol
        RpiDashboard::sysinfo
        RpiDashboard::sysmetrics
        RpiDashboard::telemetry
        RpiDashboard::weather
        Qt6::Qml
        Qt6::Quick
)

qt_add_executable(rpi-dashboard
    MANUAL_FINALIZATION
    dashboard_config.cpp
    dashboard_config.h
    main.cpp
)

target_link_libraries(rpi-dashboard
    PRIVATE
        RpiDashboard::protocol
        RpiDashboard::projects
        RpiDashboard::sysinfo
        RpiDashboard::sysmetrics
        RpiDashboard::telemetry
        RpiDashboard::weather
        rpi-dashboard-qml
        rpi-dashboard-qmlplugin
        rpi-dashboard-qmlplugin_init
        Qt6::Gui
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
        tomlplusplus::tomlplusplus
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
