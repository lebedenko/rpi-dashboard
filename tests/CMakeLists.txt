find_package(Qt6 6.8 REQUIRED COMPONENTS Test)

qt_add_executable(device_snapshot_test
    device_snapshot_test.cpp
)

target_link_libraries(device_snapshot_test
    PRIVATE
        HoloNightDashboard::protocol
        Qt6::Test
)

add_test(NAME device_snapshot_test COMMAND device_snapshot_test)

qt_add_executable(dashboard_startup_test
    dashboard_startup_test.cpp
)

add_dependencies(dashboard_startup_test holonight-dashboard)

target_compile_definitions(dashboard_startup_test
    PRIVATE
        DASHBOARD_EXECUTABLE="$<TARGET_FILE:holonight-dashboard>"
)

target_link_libraries(dashboard_startup_test
    PRIVATE
        Qt6::Test
)

add_test(NAME dashboard_startup_test COMMAND dashboard_startup_test)
