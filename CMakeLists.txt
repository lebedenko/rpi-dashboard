cmake_minimum_required(VERSION 3.25)

project(HoloNightDashboard
    VERSION 0.1.0
    DESCRIPTION "Standalone Qt dashboard for local and remote system telemetry"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_CLANG_TIDY "Run clang-tidy while compiling" OFF)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Network Qml Quick)
qt_standard_project_setup(REQUIRES 6.8)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
endif()

include(CTest)

add_subdirectory(src/protocol)
add_subdirectory(src/dashboard)
add_subdirectory(src/agent)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

