qt_add_executable(holonight-dashboard
    main.cpp
)

set_source_files_properties(qml/Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(holonight-dashboard
    URI HoloNight.Dashboard
    QML_FILES
        qml/Main.qml
        qml/MetricCard.qml
        qml/Theme.qml
)

target_link_libraries(holonight-dashboard
    PRIVATE
        HoloNightDashboard::protocol
        Qt6::Gui
        Qt6::Network
        Qt6::Qml
        Qt6::Quick
)

set_target_properties(holonight-dashboard PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

