qt_add_executable(holonight-dashboard-agent
    main.cpp
)

target_link_libraries(holonight-dashboard-agent
    PRIVATE
        HoloNightDashboard::protocol
        Qt6::Core
        Qt6::Network
)

