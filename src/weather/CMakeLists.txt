qt_add_library(rpi-dashboard-weather STATIC
    weather_config.cpp
    weather_config.h
    weather_models.cpp
    weather_models.h
    weather_provider.cpp
    weather_provider.h
    weather_service.cpp
    weather_service.h
    weather_types.h
)

add_library(RpiDashboard::weather ALIAS rpi-dashboard-weather)

target_include_directories(rpi-dashboard-weather
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(rpi-dashboard-weather
    PUBLIC
        Qt6::Core
        Qt6::Network
        tomlplusplus::tomlplusplus
)
