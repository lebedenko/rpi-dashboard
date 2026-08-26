qt_add_library(dashboard_projects STATIC
    project_health.cpp
    project_health.h
    projects_service.cpp
    projects_service.h
)

add_library(HoloNightDashboard::projects ALIAS dashboard_projects)

target_include_directories(dashboard_projects
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(dashboard_projects
    PUBLIC
        Qt6::Core
        Qt6::Network
)
