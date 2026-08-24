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

