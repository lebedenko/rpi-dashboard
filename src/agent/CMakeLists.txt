qt_add_executable(rpi-dashboard-agent
    main.cpp
)

target_link_libraries(rpi-dashboard-agent
    PRIVATE
        RpiDashboard::protocol
        Qt6::Core
        Qt6::Network
)
