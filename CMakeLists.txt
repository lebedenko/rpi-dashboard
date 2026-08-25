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
option(ENABLE_ADDRESS_SANITIZER "Build with AddressSanitizer" OFF)
option(ENABLE_UNDEFINED_SANITIZER "Build with UndefinedBehaviorSanitizer" OFF)

if(ENABLE_ADDRESS_SANITIZER AND ENABLE_UNDEFINED_SANITIZER)
    message(FATAL_ERROR "Enable only one sanitizer configuration at a time")
endif()

if(ENABLE_ADDRESS_SANITIZER)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UNDEFINED_SANITIZER)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()

find_package(Qt6 6.8 REQUIRED COMPONENTS Concurrent Core Gui Network Qml Quick)
qt_standard_project_setup(REQUIRES 6.8)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXECUTABLE}")
endif()

include(CTest)

add_subdirectory(src/protocol)
add_subdirectory(src/sysinfo)
add_subdirectory(src/sysmetrics)
add_subdirectory(src/dashboard)
add_subdirectory(src/agent)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
