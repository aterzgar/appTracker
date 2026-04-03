cmake_minimum_required(VERSION 3.10)
project(appTracker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

# Enable Qt's MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(src)

# Source files
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/db_manager.cpp
    src/application_service.cpp
    src/analytics_visualizer.cpp
    src/llm_client.cpp
)

set(HEADERS
    src/mainwindow.h
    src/db_manager.h
    src/application_service.h
    src/analytics_visualizer.h
    src/llm_client.h
)

# Create executable
add_executable(appTracker ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(appTracker
    Qt5::Widgets
    Qt5::Network
    ${SQLITE3_LIBRARIES}
)

# Include directories for SQLite
target_include_directories(appTracker PRIVATE ${SQLITE3_INCLUDE_DIRS})

# Compiler flags for SQLite
target_compile_options(appTracker PRIVATE ${SQLITE3_CFLAGS_OTHER})

# Set output directory
set_target_properties(appTracker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Option to build tests
option(BUILD_TESTS "Build the test suite" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()